/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qprinterinfo.h"
#include "qprinterinfo_p.h"

#include <qfile.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qprintdialog.h>
#include <qlibrary.h>
#include <qtextstream.h>

#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
#  include <private/qcups_p.h>
#  include <cups/cups.h>
#  include <private/qpdf_p.h>
#endif

#include <private/qprinterinfo_unix_p.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_PRINTER

#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
// preserver names in ascending order for the binary search
static const struct NamedPaperSize {
    const char *const name;
    QPrinter::PaperSize size;
} named_sizes_map[QPrinter::NPageSize] = {
    { "A0", QPrinter::A0 },
    { "A1", QPrinter::A1 },
    { "A2", QPrinter::A2 },
    { "A3", QPrinter::A3 },
    { "A4", QPrinter::A4 },
    { "A5", QPrinter::A5 },
    { "A6", QPrinter::A6 },
    { "A7", QPrinter::A7 },
    { "A8", QPrinter::A8 },
    { "A9", QPrinter::A9 },
    { "B0", QPrinter::B0 },
    { "B1", QPrinter::B1 },
    { "B10", QPrinter::B10 },
    { "B2", QPrinter::B2 },
    { "B4", QPrinter::B4 },
    { "B5", QPrinter::B5 },
    { "B6", QPrinter::B6 },
    { "B7", QPrinter::B7 },
    { "B8", QPrinter::B8 },
    { "B9", QPrinter::B9 },
    { "C5E", QPrinter::C5E },
    { "Comm10E", QPrinter::Comm10E },
    { "Custom", QPrinter::Custom },
    { "DLE", QPrinter::DLE },
    { "Executive", QPrinter::Executive },
    { "Folio", QPrinter::Folio },
    { "Ledger", QPrinter::Ledger },
    { "Legal", QPrinter::Legal },
    { "Letter", QPrinter::Letter },
    { "Tabloid", QPrinter::Tabloid }
};

inline bool operator<(const char *name, const NamedPaperSize &data)
{ return qstrcmp(name, data.name) < 0; }
inline bool operator<(const NamedPaperSize &data, const char *name)
{ return qstrcmp(data.name, name) < 0; }

static inline QPrinter::PaperSize string2PaperSize(const char *name)
{
    const NamedPaperSize *r = qBinaryFind(named_sizes_map, named_sizes_map + QPrinter::NPageSize, name);
    if (r - named_sizes_map != QPrinter::NPageSize)
        return r->size;
    return QPrinter::Custom;
}

static inline const char *paperSize2String(QPrinter::PaperSize size)
{
    for (int i = 0; i < QPrinter::NPageSize; ++i) {
        if (size == named_sizes_map[i].size)
            return named_sizes_map[i].name;
    }
    return 0;
}
#endif

void qt_perhapsAddPrinter(QList<QPrinterDescription> *printers, const QString &name,
                               QString host, QString comment,
                               QStringList aliases)
{
    for (int i = 0; i < printers->size(); ++i)
        if (printers->at(i).samePrinter(name))
            return;

#ifndef QT_NO_PRINTDIALOG
    if (host.isEmpty())
        host = QPrintDialog::tr("locally connected");
#endif
    printers->append(QPrinterDescription(name.simplified(), host.simplified(), comment.simplified(), aliases));
}

void qt_parsePrinterDesc(QString printerDesc, QList<QPrinterDescription> *printers)
{
    if (printerDesc.length() < 1)
        return;

    printerDesc = printerDesc.simplified();
    int i = printerDesc.indexOf(QLatin1Char(':'));
    QString printerName, printerComment, printerHost;
    QStringList aliases;

    if (i >= 0) {
        // have ':' want '|'
        int j = printerDesc.indexOf(QLatin1Char('|'));
        if (j > 0 && j < i) {
            printerName = printerDesc.left(j);
            aliases = printerDesc.mid(j + 1, i - j - 1).split(QLatin1Char('|'));
#ifndef QT_NO_PRINTDIALOG
            // try extracting a comment from the aliases
            printerComment = QPrintDialog::tr("Aliases: %1")
                             .arg(aliases.join(QLatin1String(", ")));
#endif
        } else {
            printerName = printerDesc.left(i);
        }
        // look for lprng pseudo all printers entry
        i = printerDesc.indexOf(QRegExp(QLatin1String(": *all *=")));
        if (i >= 0)
            printerName = QString();
        // look for signs of this being a remote printer
        i = printerDesc.indexOf(QRegExp(QLatin1String(": *rm *=")));
        if (i >= 0) {
            // point k at the end of remote host name
            while (printerDesc[i] != QLatin1Char('='))
                i++;
            while (printerDesc[i] == QLatin1Char('=') || printerDesc[i].isSpace())
                i++;
            j = i;
            while (j < (int)printerDesc.length() && printerDesc[j] != QLatin1Char(':'))
                j++;

            // and stuff that into the string
            printerHost = printerDesc.mid(i, j - i);
        }
    }
    if (printerName.length())
        qt_perhapsAddPrinter(printers, printerName, printerHost, printerComment,
                             aliases);
}

int qt_parsePrintcap(QList<QPrinterDescription> *printers, const QString& fileName)
{
    QFile printcap(fileName);
    if (!printcap.open(QIODevice::ReadOnly))
        return NotFound;

    char *line_ascii = new char[1025];
    line_ascii[1024] = '\0';

    QString printerDesc;
    bool atEnd = false;

    while (!atEnd) {
        if (printcap.atEnd() || printcap.readLine(line_ascii, 1024) <= 0)
            atEnd = true;
        QString line = QString::fromLocal8Bit(line_ascii);
        line = line.trimmed();
        if (line.length() >= 1 && line[int(line.length()) - 1] == QLatin1Char('\\'))
            line.chop(1);
        if (line[0] == QLatin1Char('#')) {
            if (!atEnd)
                continue;
        } else if (line[0] == QLatin1Char('|') || line[0] == QLatin1Char(':')
                || line.isEmpty()) {
            printerDesc += line;
            if (!atEnd)
                continue;
        }

        qt_parsePrinterDesc(printerDesc, printers);

        // add the first line of the new printer definition
        printerDesc = line;
    }
    delete[] line_ascii;
    return Success;
}

/*!
  \internal

  Checks $HOME/.printers for a line matching '_default <name>' (where
  <name> does not contain any white space). The first such match
  results in <name> being returned.
  If no lines match then an empty string is returned.
*/
QString qt_getDefaultFromHomePrinters()
{
    QFile file(QDir::homePath() + QLatin1String("/.printers"));
    if (!file.open(QIODevice::ReadOnly))
        return QString();
    QString all(QLatin1String(file.readAll()));
    QStringList words = all.split(QRegExp(QLatin1String("\\W+")), QString::SkipEmptyParts);
    const int i = words.indexOf(QLatin1String("_default"));
    if (i != -1 && i < words.size() - 1)
        return words.at(i + 1);
    return QString();
}

// solaris, not 2.6
void qt_parseEtcLpPrinters(QList<QPrinterDescription> *printers)
{
    QDir lp(QLatin1String("/etc/lp/printers"));
    QFileInfoList dirs = lp.entryInfoList();
    if (dirs.isEmpty())
        return;

    QString tmp;
    for (int i = 0; i < dirs.size(); ++i) {
        QFileInfo printer = dirs.at(i);
        if (printer.isDir()) {
            tmp.sprintf("/etc/lp/printers/%s/configuration",
                         printer.fileName().toAscii().data());
            QFile configuration(tmp);
            char *line = new char[1025];
            QString remote(QLatin1String("Remote:"));
            QString contentType(QLatin1String("Content types:"));
            QString printerHost;
            bool canPrintPostscript = false;
            if (configuration.open(QIODevice::ReadOnly)) {
                while (!configuration.atEnd() &&
                        configuration.readLine(line, 1024) > 0) {
                    if (QString::fromLatin1(line).startsWith(remote)) {
                        const char *p = line;
                        while (*p != ':')
                            p++;
                        p++;
                        while (isspace((uchar) *p))
                            p++;
                        printerHost = QString::fromLocal8Bit(p);
                        printerHost = printerHost.simplified();
                    } else if (QString::fromLatin1(line).startsWith(contentType)) {
                        char *p = line;
                        while (*p != ':')
                            p++;
                        p++;
                        char *e;
                        while (*p) {
                            while (isspace((uchar) *p))
                                p++;
                            if (*p) {
                                char s;
                                e = p;
                                while (isalnum((uchar) *e))
                                    e++;
                                s = *e;
                                *e = '\0';
                                if (!qstrcmp(p, "postscript") ||
                                     !qstrcmp(p, "any"))
                                    canPrintPostscript = true;
                                *e = s;
                                if (s == ',')
                                    e++;
                                p = e;
                            }
                        }
                    }
                }
                if (canPrintPostscript)
                    qt_perhapsAddPrinter(printers, printer.fileName(),
                                         printerHost, QLatin1String(""));
            }
            delete[] line;
        }
    }
}

// solaris 2.6
char *qt_parsePrintersConf(QList<QPrinterDescription> *printers, bool *found)
{
    QFile pc(QLatin1String("/etc/printers.conf"));
    if (!pc.open(QIODevice::ReadOnly)) {
        if (found)
            *found = false;
        return 0;
    }
    if (found)
        *found = true;

    char *line = new char[1025];
    line[1024] = '\0';

    QString printerDesc;
    int lineLength = 0;

    char *defaultPrinter = 0;

    while (!pc.atEnd() &&
            (lineLength=pc.readLine(line, 1024)) > 0) {
        if (*line == '#') {
            *line = '\0';
            lineLength = 0;
        }
        if (lineLength >= 2 && line[lineLength-2] == '\\') {
            line[lineLength-2] = '\0';
            printerDesc += QString::fromLocal8Bit(line);
        } else {
            printerDesc += QString::fromLocal8Bit(line);
            printerDesc = printerDesc.simplified();
            int i = printerDesc.indexOf(QLatin1Char(':'));
            QString printerName, printerHost, printerComment;
            QStringList aliases;
            if (i >= 0) {
                // have : want |
                int j = printerDesc.indexOf(QLatin1Char('|'));
                if (j >= i)
                    j = -1;
                printerName = printerDesc.mid(0, j < 0 ? i : j);
                if (printerName == QLatin1String("_default")) {
                    i = printerDesc.indexOf(
                        QRegExp(QLatin1String(": *use *=")));
                    while (printerDesc[i] != QLatin1Char('='))
                        i++;
                    while (printerDesc[i] == QLatin1Char('=') || printerDesc[i].isSpace())
                        i++;
                    j = i;
                    while (j < (int)printerDesc.length() &&
                            printerDesc[j] != QLatin1Char(':') && printerDesc[j] != QLatin1Char(','))
                        j++;
                    // that's our default printer
                    defaultPrinter =
                        qstrdup(printerDesc.mid(i, j-i).toAscii().data());
                    printerName = QString();
                    printerDesc = QString();
                } else if (printerName == QLatin1String("_all")) {
                    // skip it.. any other cases we want to skip?
                    printerName = QString();
                    printerDesc = QString();
                }

                if (j > 0) {
                    // try extracting a comment from the aliases
                    aliases = printerDesc.mid(j + 1, i - j - 1).split(QLatin1Char('|'));
#ifndef QT_NO_PRINTDIALOG
                    printerComment = QPrintDialog::tr("Aliases: %1")
                                     .arg(aliases.join(QLatin1String(", ")));
#endif
                }
                // look for signs of this being a remote printer
                i = printerDesc.indexOf(
                    QRegExp(QLatin1String(": *bsdaddr *=")));
                if (i >= 0) {
                    // point k at the end of remote host name
                    while (printerDesc[i] != QLatin1Char('='))
                        i++;
                    while (printerDesc[i] == QLatin1Char('=') || printerDesc[i].isSpace())
                        i++;
                    j = i;
                    while (j < (int)printerDesc.length() &&
                            printerDesc[j] != QLatin1Char(':') && printerDesc[j] != QLatin1Char(','))
                        j++;
                    // and stuff that into the string
                    printerHost = printerDesc.mid(i, j-i);
                    // maybe stick the remote printer name into the comment
                    if (printerDesc[j] == QLatin1Char(',')) {
                        i = ++j;
                        while (printerDesc[i].isSpace())
                            i++;
                        j = i;
                        while (j < (int)printerDesc.length() &&
                                printerDesc[j] != QLatin1Char(':') && printerDesc[j] != QLatin1Char(','))
                            j++;
                        if (printerName != printerDesc.mid(i, j-i)) {
                            printerComment =
                                QLatin1String("Remote name: ");
                            printerComment += printerDesc.mid(i, j-i);
                        }
                    }
                }
            }
            if (printerComment == QLatin1String(":"))
                printerComment = QString(); // for cups
            if (printerName.length())
                qt_perhapsAddPrinter(printers, printerName, printerHost,
                                     printerComment, aliases);
            // chop away the line, for processing the next one
            printerDesc = QString();
        }
    }
    delete[] line;
    return defaultPrinter;
}

#ifndef QT_NO_NIS

#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

int qt_pd_foreach(int /*status */, char * /*key */, int /*keyLen */,
                    char *val, int valLen, char *data)
{
    qt_parsePrinterDesc(QString::fromLatin1(val, valLen), (QList<QPrinterDescription> *)data);
    return 0;
}

#if defined(Q_C_CALLBACKS)
}
#endif

int qt_retrieveNisPrinters(QList<QPrinterDescription> *printers)
{
#ifndef QT_NO_LIBRARY
    typedef int (*WildCast)(int, char *, int, char *, int, char *);
    char printersConfByname[] = "printers.conf.byname";
    char *domain;
    int err;

    QLibrary lib(QLatin1String("nsl"));
    typedef int (*ypGetDefaultDomain)(char **);
    ypGetDefaultDomain _ypGetDefaultDomain = (ypGetDefaultDomain)lib.resolve("yp_get_default_domain");
    typedef int (*ypAll)(const char *, const char *, const struct ypall_callback *);
    ypAll _ypAll = (ypAll)lib.resolve("yp_all");

    if (_ypGetDefaultDomain && _ypAll) {
        err = _ypGetDefaultDomain(&domain);
        if (err == 0) {
            ypall_callback cb;
            // wild cast to support K&R-style system headers
            (WildCast &) cb.foreach = (WildCast) qt_pd_foreach;
            cb.data = (char *) printers;
            err = _ypAll(domain, printersConfByname, &cb);
        }
        if (!err)
            return Success;
    }
#endif //QT_NO_LIBRARY
    return Unavail;
}

#endif // QT_NO_NIS

char *qt_parseNsswitchPrintersEntry(QList<QPrinterDescription> *printers, char *line)
{
#define skipSpaces() \
    while (line[k] != '\0' && isspace((uchar) line[k])) \
        k++

    char *defaultPrinter = 0;
    bool stop = false;
    int lastStatus = NotFound;

    int k = 8;
    skipSpaces();
    if (line[k] != ':')
        return 0;
    k++;

    char *cp = strchr(line, '#');
    if (cp != 0)
        *cp = '\0';

    while (line[k] != '\0') {
        if (isspace((uchar) line[k])) {
            k++;
        } else if (line[k] == '[') {
            k++;
            skipSpaces();
            while (line[k] != '\0') {
                char status = tolower(line[k]);
                char action = '?';

                while (line[k] != '=' && line[k] != ']' && line[k] != '\0')
                    k++;
                if (line[k] == '=') {
                    k++;
                    skipSpaces();
                    action = tolower(line[k]);
                    while (line[k] != '\0' && !isspace((uchar) line[k]) && line[k] != ']')
                        k++;
                } else if (line[k] == ']') {
                    k++;
                    break;
                }
                skipSpaces();

                if (lastStatus == status)
                    stop = (action == (char) Return);
            }
        } else {
            if (stop)
                break;

            QByteArray source;
            while (line[k] != '\0' && !isspace((uchar) line[k]) && line[k] != '[') {
                source += line[k];
                k++;
            }

            if (source == "user") {
                lastStatus = qt_parsePrintcap(printers,
                        QDir::homePath() + QLatin1String("/.printers"));
            } else if (source == "files") {
                bool found;
                defaultPrinter = qt_parsePrintersConf(printers, &found);
                if (found)
                    lastStatus = Success;
#ifndef QT_NO_NIS
            } else if (source == "nis") {
                lastStatus = qt_retrieveNisPrinters(printers);
#endif
            } else {
                // nisplus, dns, etc., are not implemented yet
                lastStatus = NotFound;
            }
            stop = (lastStatus == Success);
        }
    }
    return defaultPrinter;
}

char *qt_parseNsswitchConf(QList<QPrinterDescription> *printers)
{
    QFile nc(QLatin1String("/etc/nsswitch.conf"));
    if (!nc.open(QIODevice::ReadOnly))
        return 0;

    char *defaultPrinter = 0;

    char *line = new char[1025];
    line[1024] = '\0';

    while (!nc.atEnd() &&
            nc.readLine(line, 1024) > 0) {
        if (qstrncmp(line, "printers", 8) == 0) {
            defaultPrinter = qt_parseNsswitchPrintersEntry(printers, line);
            delete[] line;
            return defaultPrinter;
        }
    }

    strcpy(line, "printers: user files nis nisplus xfn");
    defaultPrinter = qt_parseNsswitchPrintersEntry(printers, line);
    delete[] line;
    return defaultPrinter;
}

// HP-UX
void qt_parseEtcLpMember(QList<QPrinterDescription> *printers)
{
    QDir lp(QLatin1String("/etc/lp/member"));
    if (!lp.exists())
        return;
    QFileInfoList dirs = lp.entryInfoList();
    if (dirs.isEmpty())
        return;

#ifdef QT_NO_PRINTDIALOG
    Q_UNUSED(printers);
#else
    QString tmp;
    for (int i = 0; i < dirs.size(); ++i) {
        QFileInfo printer = dirs.at(i);
        // I haven't found any real documentation, so I'm guessing that
        // since lpstat uses /etc/lp/member rather than one of the
        // other directories, it's the one to use.  I did not find a
        // decent way to locate aliases and remote printers.
        if (printer.isFile())
            qt_perhapsAddPrinter(printers, printer.fileName(),
                                 QPrintDialog::tr("unknown"),
                                 QLatin1String(""));
    }
#endif
}

// IRIX 6.x
void qt_parseSpoolInterface(QList<QPrinterDescription> *printers)
{
    QDir lp(QLatin1String("/usr/spool/lp/interface"));
    if (!lp.exists())
        return;
    QFileInfoList files = lp.entryInfoList();
    if(files.isEmpty())
        return;

    for (int i = 0; i < files.size(); ++i) {
        QFileInfo printer = files.at(i);

        if (!printer.isFile())
            continue;

        // parse out some information
        QFile configFile(printer.filePath());
        if (!configFile.open(QIODevice::ReadOnly))
            continue;

        QByteArray line;
        line.resize(1025);
        QString namePrinter;
        QString hostName;
        QString hostPrinter;
        QString printerType;

        QString nameKey(QLatin1String("NAME="));
        QString typeKey(QLatin1String("TYPE="));
        QString hostKey(QLatin1String("HOSTNAME="));
        QString hostPrinterKey(QLatin1String("HOSTPRINTER="));

        while (!configFile.atEnd() &&
                (configFile.readLine(line.data(), 1024)) > 0) {
            QString uline = QString::fromLocal8Bit(line);
            if (uline.startsWith(typeKey) ) {
                printerType = uline.mid(nameKey.length());
                printerType = printerType.simplified();
            } else if (uline.startsWith(hostKey)) {
                hostName = uline.mid(hostKey.length());
                hostName = hostName.simplified();
            } else if (uline.startsWith(hostPrinterKey)) {
                hostPrinter = uline.mid(hostPrinterKey.length());
                hostPrinter = hostPrinter.simplified();
            } else if (uline.startsWith(nameKey)) {
                namePrinter = uline.mid(nameKey.length());
                namePrinter = namePrinter.simplified();
            }
        }
        configFile.close();

        printerType = printerType.trimmed();
        if (printerType.indexOf(QLatin1String("postscript"), 0, Qt::CaseInsensitive) < 0)
            continue;

        int ii = 0;
        while ((ii = namePrinter.indexOf(QLatin1Char('"'), ii)) >= 0)
            namePrinter.remove(ii, 1);

        if (hostName.isEmpty() || hostPrinter.isEmpty()) {
            qt_perhapsAddPrinter(printers, printer.fileName(),
                                 QLatin1String(""), namePrinter);
        } else {
            QString comment;
            comment = namePrinter;
            comment += QLatin1String(" (");
            comment += hostPrinter;
            comment += QLatin1Char(')');
            qt_perhapsAddPrinter(printers, printer.fileName(),
                                 hostName, comment);
        }
    }
}


// Every unix must have its own.  It's a standard.  Here is AIX.
void qt_parseQconfig(QList<QPrinterDescription> *printers)
{
    QFile qconfig(QLatin1String("/etc/qconfig"));
    if (!qconfig.open(QIODevice::ReadOnly))
        return;

    QTextStream ts(&qconfig);
    QString line;

    QString stanzaName; // either a queue or a device name
    bool up = true; // queue up?  default true, can be false
    QString remoteHost; // null if local
    QString deviceName; // null if remote

    QRegExp newStanza(QLatin1String("^[0-z\\-]*:$"));

    // our basic strategy here is to process each line, detecting new
    // stanzas.  each time we see a new stanza, we check if the
    // previous stanza was a valid queue for a) a remote printer or b)
    // a local printer.  if it wasn't, we assume that what we see is
    // the start of the first stanza, or that the previous stanza was
    // a device stanza, or that there is some syntax error (we don't
    // report those).

    do {
        line = ts.readLine();
        bool indented = line[0].isSpace();
        line = line.simplified();

        int i = line.indexOf(QLatin1Char('='));
        if (indented && i != -1) { // line in stanza
            QString variable = line.left(i).simplified();
            QString value=line.mid(i+1, line.length()).simplified();
            if (variable == QLatin1String("device"))
                deviceName = value;
            else if (variable == QLatin1String("host"))
                remoteHost = value;
            else if (variable == QLatin1String("up"))
                up = !(value.toLower() == QLatin1String("false"));
        } else if (line[0] == QLatin1Char('*')) { // comment
            // nothing to do
        } else if (ts.atEnd() || // end of file, or beginning of new stanza
                    (!indented && line.contains(newStanza))) {
            if (up && stanzaName.length() > 0 && stanzaName.length() < 21) {
                if (remoteHost.length()) // remote printer
                    qt_perhapsAddPrinter(printers, stanzaName, remoteHost,
                                         QString());
                else if (deviceName.length()) // local printer
                    qt_perhapsAddPrinter(printers, stanzaName, QString(),
                                         QString());
            }
            line.chop(1);
            if (line.length() >= 1 && line.length() <= 20)
                stanzaName = line;
            up = true;
            remoteHost.clear();
            deviceName.clear();
        } else {
            // syntax error?  ignore.
        }
    } while (!ts.atEnd());
}

int qt_getLprPrinters(QList<QPrinterDescription>& printers)
{
    QByteArray etcLpDefault;
    qt_parsePrintcap(&printers, QLatin1String("/etc/printcap"));
    qt_parseEtcLpMember(&printers);
    qt_parseSpoolInterface(&printers);
    qt_parseQconfig(&printers);

    QFileInfo f;
    f.setFile(QLatin1String("/etc/lp/printers"));
    if (f.isDir()) {
        qt_parseEtcLpPrinters(&printers);
        QFile def(QLatin1String("/etc/lp/default"));
        if (def.open(QIODevice::ReadOnly)) {
            etcLpDefault.resize(1025);
            if (def.readLine(etcLpDefault.data(), 1024) > 0) {
                QRegExp rx(QLatin1String("^(\\S+)"));
                if (rx.indexIn(QString::fromLatin1(etcLpDefault)) != -1)
                    etcLpDefault = rx.cap(1).toAscii();
            }
        }
    }

    char *def = 0;
    f.setFile(QLatin1String("/etc/nsswitch.conf"));
    if (f.isFile()) {
        def = qt_parseNsswitchConf(&printers);
    } else {
        f.setFile(QLatin1String("/etc/printers.conf"));
        if (f.isFile())
            def = qt_parsePrintersConf(&printers);
    }

    if (def) {
        etcLpDefault = def;
        delete [] def;
    }

    QString homePrintersDefault = qt_getDefaultFromHomePrinters();

    // all printers hopefully known.  try to find a good default
    QString dollarPrinter;
    {
        dollarPrinter = QString::fromLocal8Bit(qgetenv("PRINTER"));
        if (dollarPrinter.isEmpty())
            dollarPrinter = QString::fromLocal8Bit(qgetenv("LPDEST"));
        if (dollarPrinter.isEmpty())
            dollarPrinter = QString::fromLocal8Bit(qgetenv("NPRINTER"));
        if (dollarPrinter.isEmpty())
            dollarPrinter = QString::fromLocal8Bit(qgetenv("NGPRINTER"));
#ifndef QT_NO_PRINTDIALOG
        if (!dollarPrinter.isEmpty())
            qt_perhapsAddPrinter(&printers, dollarPrinter,
                                 QPrintDialog::tr("unknown"),
                                 QLatin1String(""));
#endif
    }

    QRegExp ps(QLatin1String("[^a-z]ps(?:[^a-z]|$)"));
    QRegExp lp(QLatin1String("[^a-z]lp(?:[^a-z]|$)"));

    int quality = 0;
    int best = 0;
    for (int i = 0; i < printers.size(); ++i) {
        QString name = printers.at(i).name;
        QString comment = printers.at(i).comment;
        if (quality < 5 && name == dollarPrinter) {
            best = i;
            quality = 5;
        } else if (quality < 4 && !homePrintersDefault.isEmpty() &&
                   name == homePrintersDefault) {
            best = i;
            quality = 4;
        } else if (quality < 3 && !etcLpDefault.isEmpty() &&
                    name == QLatin1String(etcLpDefault)) {
            best = i;
            quality = 3;
        } else if (quality < 2 &&
                    (name == QLatin1String("ps") ||
                     ps.indexIn(comment) != -1)) {
            best = i;
            quality = 2;
        } else if (quality < 1 &&
                    (name == QLatin1String("lp") ||
                     lp.indexIn(comment) > -1)) {
            best = i;
            quality = 1;
        }
    }

    return best;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

QList<QPrinterInfo> QPrinterInfo::availablePrinters()
{
    QList<QPrinterInfo> printers;

#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    if (QCUPSSupport::isAvailable()) {
        QCUPSSupport cups;
        int cupsPrinterCount = cups.availablePrintersCount();
        const cups_dest_t* cupsPrinters = cups.availablePrinters();
        for (int i = 0; i < cupsPrinterCount; ++i) {
            QString printerName(QString::fromLocal8Bit(cupsPrinters[i].name));
            if (cupsPrinters[i].instance)
                printerName += QLatin1Char('/') + QString::fromLocal8Bit(cupsPrinters[i].instance);

            QPrinterInfo printerInfo(printerName);
            if (cupsPrinters[i].is_default)
                printerInfo.d_ptr->isDefault = true;
            printerInfo.d_ptr->cupsPrinterIndex = i;
            printers.append(printerInfo);
        }
    } else
#endif
           {
        QList<QPrinterDescription> lprPrinters;
        int defprn = qt_getLprPrinters(lprPrinters);
        // populating printer combo
        foreach (const QPrinterDescription &description, lprPrinters)
            printers.append(QPrinterInfo(description.name));
        if (defprn >= 0 && defprn < printers.size())
            printers[defprn].d_ptr->isDefault = true;
    }

    return printers;
}

QPrinterInfo QPrinterInfo::defaultPrinter()
{
    QList<QPrinterInfo> printers = availablePrinters();
    foreach (const QPrinterInfo &printerInfo, printers) {
        if (printerInfo.isDefault())
            return printerInfo;
    }

    return printers.value(0);
}

QList<QPrinter::PaperSize> QPrinterInfo::supportedPaperSizes() const
{
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    const Q_D(QPrinterInfo);

    if (isNull())
        return d->paperSizes;

    if (!d->hasPaperSizes) {
        d->hasPaperSizes = true;

        if (QCUPSSupport::isAvailable()) {
            // Find paper sizes from CUPS.
            QCUPSSupport cups;
            cups.setCurrentPrinter(d->cupsPrinterIndex);
            const ppd_option_t* sizes = cups.pageSizes();
            if (sizes) {
                for (int j = 0; j < sizes->num_choices; ++j)
                    d->paperSizes.append(string2PaperSize(sizes->choices[j].choice));
            }
        }
    }

    return d->paperSizes;
#else
    return QList<QPrinter::PaperSize>();
#endif
}

#endif // QT_NO_PRINTER

QT_END_NAMESPACE
