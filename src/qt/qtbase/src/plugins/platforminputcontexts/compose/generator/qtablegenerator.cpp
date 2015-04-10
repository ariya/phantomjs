/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qtablegenerator.h"

#include <QtCore/QByteArray>
#include <QtCore/QTextCodec>
#include <QtCore/QDebug>
#include <QtCore/QStringList>
#include <QtCore/QString>

#include <algorithm>

#include <xkbcommon/xkbcommon.h>

#include <locale.h> // LC_CTYPE
#include <string.h> // strchr, strncmp, etc.
#include <strings.h> // strncasecmp
#include <clocale> // LC_CTYPE

TableGenerator::TableGenerator() : m_state(NoErrors),
    m_systemComposeDir(QString())
{
    initPossibleLocations();
    findComposeFile();
    orderComposeTable();
#ifdef DEBUG_GENERATOR
    printComposeTable();
#endif
}

void TableGenerator::initPossibleLocations()
{
    // Compose files come as a part of Xlib library. Xlib doesn't provide
    // a mechanism how to retrieve the location of these files reliably, since it was
    // never meant for external software to parse compose tables directly. Best we
    // can do is to hardcode search paths. To add an extra system path use
    // the QTCOMPOSE environment variable
    if (qEnvironmentVariableIsSet("QTCOMPOSE"))
        m_possibleLocations.append(QString::fromLocal8Bit(qgetenv("QTCOMPOSE")));
    m_possibleLocations.append(QStringLiteral("/usr/share/X11/locale"));
    m_possibleLocations.append(QStringLiteral("/usr/local/share/X11/locale"));
    m_possibleLocations.append(QStringLiteral("/usr/lib/X11/locale"));
    m_possibleLocations.append(QStringLiteral("/usr/local/lib/X11/locale"));
    m_possibleLocations.append(QStringLiteral(X11_PREFIX "/share/X11/locale"));
    m_possibleLocations.append(QStringLiteral(X11_PREFIX "/lib/X11/locale"));
}

void TableGenerator::findComposeFile()
{
    bool found = false;
    // check if XCOMPOSEFILE points to a Compose file
    if (qEnvironmentVariableIsSet("XCOMPOSEFILE")) {
        QString composeFile(qgetenv("XCOMPOSEFILE"));
        if (composeFile.endsWith(QLatin1String("Compose")))
            found = processFile(composeFile);
        else
            qWarning("Qt Warning: XCOMPOSEFILE doesn't point to a valid Compose file");
#ifdef DEBUG_GENERATOR
        if (found)
            qDebug() << "Using Compose file from: " << composeFile;
#endif
    }
    // check if user’s home directory has a file named .XCompose
    if (!found && cleanState()) {
        QString composeFile = qgetenv("HOME") + QStringLiteral("/.XCompose");
        if (QFile(composeFile).exists())
            found = processFile(composeFile);
#ifdef DEBUG_GENERATOR
        if (found)
            qDebug() << "Using Compose file from: " << composeFile;
#endif
    }
    // check for the system provided compose files
    if (!found && cleanState()) {
        QByteArray loc = locale().toUpper().toUtf8();
        QString table = readLocaleMappings(loc);
        if (table.isEmpty())
            table = readLocaleMappings(readLocaleAliases(loc));

        if (cleanState()) {
            if (table.isEmpty())
                // no table mappings for the system's locale in the compose.dir
                m_state = UnsupportedLocale;
            else
                found = processFile(systemComposeDir() + QLatin1String("/") + table);
#ifdef DEBUG_GENERATOR
            if (found)
                qDebug() << "Using Compose file from: " <<
                            systemComposeDir() + QLatin1String("/") + table;
#endif
        }
    }

    if (found && m_composeTable.isEmpty())
        m_state = EmptyTable;

    if (!found)
        m_state = MissingComposeFile;
}

bool TableGenerator::findSystemComposeDir()
{
    bool found = false;
    for (int i = 0; i < m_possibleLocations.size(); ++i) {
        QString path = m_possibleLocations.at(i);
        if (QFile(path + QLatin1String("/compose.dir")).exists()) {
            m_systemComposeDir = path;
            found = true;
            break;
        }
    }

    if (!found) {
        // should we ask to report this in the qt bug tracker?
        m_state = UnknownSystemComposeDir;
        qWarning("Qt Warning: Could not find a location of the system's Compose files. "
             "Consider setting the QTCOMPOSE environment variable.");
    }

    return found;
}

QString TableGenerator::systemComposeDir()
{
    if (m_systemComposeDir.isNull()
            && !findSystemComposeDir()) {
        return QLatin1String("$QTCOMPOSE");
    }

    return m_systemComposeDir;
}

QString TableGenerator::locale() const
{
    char *name = setlocale(LC_CTYPE, (char *)0);
    return QLatin1String(name);
}

QString TableGenerator::readLocaleMappings(const QByteArray &locale)
{
    QString file;
    if (locale.isEmpty())
        return file;

    QFile mappings(systemComposeDir() + QLatin1String("/compose.dir"));
    if (mappings.open(QIODevice::ReadOnly)) {
        const int localeNameLength = locale.size();
        const char * const localeData = locale.constData();

        char l[1024];
        // formating of compose.dir has some inconsistencies
        while (!mappings.atEnd()) {
            int read = mappings.readLine(l, sizeof(l));
            if (read <= 0)
                break;

            char *line = l;
            if (*line >= 'a' && *line <= 'z') {
                // file name
                while (*line && *line != ':' && *line != ' ' && *line != '\t')
                    ++line;
                if (!*line)
                    continue;
                const char * const composeFileNameEnd = line;
                *line = '\0';
                ++line;

                // locale name
                while (*line && (*line == ' ' || *line == '\t'))
                    ++line;
                const char * const lc = line;
                while (*line && *line != ' ' && *line != '\t' && *line != '\n')
                    ++line;
                *line = '\0';
                if (localeNameLength == (line - lc) && !strncasecmp(lc, localeData, line - lc)) {
                    file = QString::fromLocal8Bit(l, composeFileNameEnd - l);
                    break;
                }
            }
        }
        mappings.close();
    }
    return file;
}

QByteArray TableGenerator::readLocaleAliases(const QByteArray &locale)
{
    QFile aliases(systemComposeDir() + QLatin1String("/locale.alias"));
    QByteArray fullLocaleName;
    if (aliases.open(QIODevice::ReadOnly)) {
        while (!aliases.atEnd()) {
            char l[1024];
            int read = aliases.readLine(l, sizeof(l));
            char *line = l;
            if (read && ((*line >= 'a' && *line <= 'z') ||
                         (*line >= 'A' && *line <= 'Z'))) {
                const char *alias = line;
                while (*line && *line != ':' && *line != ' ' && *line != '\t')
                    ++line;
                if (!*line)
                    continue;
                *line = 0;
                if (locale.size() == (line - alias)
                        && !strncasecmp(alias, locale.constData(), line - alias)) {
                    // found a match for alias, read the real locale name
                    ++line;
                    while (*line && (*line == ' ' || *line == '\t'))
                        ++line;
                    const char *fullName = line;
                    while (*line && *line != ' ' && *line != '\t' && *line != '\n')
                        ++line;
                    *line = 0;
                    fullLocaleName = fullName;
#ifdef DEBUG_GENERATOR
                    qDebug() << "Alias for: " << alias << "is: " << fullLocaleName;
                    break;
#endif
                }
            }
        }
        aliases.close();
    }
    return fullLocaleName;
}

bool TableGenerator::processFile(QString composeFileName)
{
    QFile composeFile(composeFileName);
    if (composeFile.open(QIODevice::ReadOnly)) {
        parseComposeFile(&composeFile);
        return true;
    }
    qWarning() << QString(QLatin1String("Qt Warning: Compose file: \"%1\" can't be found"))
                  .arg(composeFile.fileName());
    return false;
}

TableGenerator::~TableGenerator()
{
}

QVector<QComposeTableElement> TableGenerator::composeTable() const
{
    return m_composeTable;
}

void TableGenerator::parseComposeFile(QFile *composeFile)
{
#ifdef DEBUG_GENERATOR
    qDebug() << "TableGenerator::parseComposeFile: " << composeFile->fileName();
#endif

    char line[1024];
    while (!composeFile->atEnd()) {
        composeFile->readLine(line, sizeof(line));
        if (*line == '<')
            parseKeySequence(line);
        else if (!strncmp(line, "include", 7))
            parseIncludeInstruction(QString::fromLocal8Bit(line));
    }

    composeFile->close();
}

void TableGenerator::parseIncludeInstruction(QString line)
{
    // Parse something that looks like:
    // include "/usr/share/X11/locale/en_US.UTF-8/Compose"
    QString quote = QStringLiteral("\"");
    line.remove(0, line.indexOf(quote) + 1);
    line.chop(line.length() - line.indexOf(quote));

    // expand substitutions if present
    line.replace(QLatin1String("%H"), QString(qgetenv("HOME")));
    line.replace(QLatin1String("%L"), locale());
    line.replace(QLatin1String("%S"), systemComposeDir());

    processFile(line);
}

ushort TableGenerator::keysymToUtf8(quint32 sym)
{
    QByteArray chars;
    int bytes;
    chars.resize(8);
    bytes = xkb_keysym_to_utf8(sym, chars.data(), chars.size());
    if (bytes == -1)
        qWarning("TableGenerator::keysymToUtf8 - buffer too small");

    chars.resize(bytes-1);

#ifdef DEBUG_GENERATOR
    QTextCodec *codec = QTextCodec::codecForLocale();
    qDebug() << QString("keysym - 0x%1 : utf8 - %2").arg(QString::number(sym, 16))
                                                    .arg(codec->toUnicode(chars));
#endif
    return QString::fromUtf8(chars).at(0).unicode();
}

static inline int fromBase8(const char *s, const char *end)
{
    int result = 0;
    while (*s && s != end) {
        if (*s <= '0' && *s >= '7')
            return 0;
        result *= 8;
        result += *s - '0';
        ++s;
    }
    return result;
}

static inline int fromBase16(const char *s, const char *end)
{
    int result = 0;
    while (*s && s != end) {
        result *= 16;
        if (*s >= '0' && *s <= '9')
            result += *s - '0';
        else if (*s >= 'a' && *s <= 'f')
            result += *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'F')
            result += *s - 'A' + 10;
        else
            return 0;
        ++s;
    }
    return result;
}

void TableGenerator::parseKeySequence(char *line)
{
    // we are interested in the lines with the following format:
    // <Multi_key> <numbersign> <S> : "♬"   U266c # BEAMED SIXTEENTH NOTE
    char *keysEnd = strchr(line, ':');
    if (!keysEnd)
        return;

    QComposeTableElement elem;
    // find the composed value - strings may be direct text encoded in the locale
    // for which the compose file is to be used, or an escaped octal or hexadecimal
    // character code. Octal codes are specified as "\123" and hexadecimal codes as "\0x123a".
    char *composeValue = strchr(keysEnd, '"');
    if (!composeValue)
        return;
    ++composeValue;

    char *composeValueEnd = strchr(composeValue, '"');
    if (!composeValueEnd)
        return;

    if (*composeValue == '\\' && composeValue[1] >= '0' && composeValue[1] <= '9') {
        // handle octal and hex code values
        char detectBase = composeValue[2];
        if (detectBase == 'x') {
            // hexadecimal character code
            elem.value = keysymToUtf8(fromBase16(composeValue + 3, composeValueEnd));
        } else {
            // octal character code
            elem.value = keysymToUtf8(fromBase8(composeValue + 1, composeValueEnd));
        }
    } else {
        // handle direct text encoded in the locale
        if (*composeValue == '\\')
            ++composeValue;
        elem.value = QString::fromLocal8Bit(composeValue).at(0).unicode();
        ++composeValue;
    }

#ifdef DEBUG_GENERATOR
    // find the comment
    elem.comment = QString::fromLocal8Bit(composeValueEnd + 1).trimmed();
#endif

    // find the key sequence and convert to X11 keysym
    char *k = line;
    const char *kend = keysEnd;

    for (int i = 0; i < QT_KEYSEQUENCE_MAX_LEN; i++) {
        // find the next pair of angle brackets and get the contents within
        while (k < kend && *k != '<')
            ++k;
        char *sym = ++k;
        while (k < kend && *k != '>')
            ++k;
        *k = '\0';
        if (k < kend) {
            elem.keys[i] = xkb_keysym_from_name(sym, (xkb_keysym_flags)0);
            if (elem.keys[i] == XKB_KEY_NoSymbol) {
                if (!strcmp(sym, "dead_inverted_breve"))
                    elem.keys[i] = XKB_KEY_dead_invertedbreve;
                else if (!strcmp(sym, "dead_double_grave"))
                    elem.keys[i] = XKB_KEY_dead_doublegrave;
#ifdef DEBUG_GENERATOR
                else
                    qWarning() << QString("Qt Warning - invalid keysym: %1").arg(sym);
#endif
            }
        } else {
            elem.keys[i] = 0;
        }
    }
    m_composeTable.append(elem);
}

void TableGenerator::printComposeTable() const
{
#ifdef DEBUG_GENERATOR
    if (composeTable().isEmpty())
        return;

    QString output;
    QComposeTableElement elem;
    QString comma = QStringLiteral(",");
    int tableSize = m_composeTable.size();
    for (int i = 0; i < tableSize; ++i) {
        elem = m_composeTable.at(i);
        output.append(QLatin1String("{ {"));
        for (int j = 0; j < QT_KEYSEQUENCE_MAX_LEN; j++) {
            output.append(QString(QLatin1String("0x%1, ")).arg(QString::number(elem.keys[j],16)));
        }
        // take care of the trailing comma
        if (i == tableSize - 1)
            comma = QStringLiteral("");
        output.append(QString(QLatin1String("}, 0x%1, \"\" }%2 // %3 \n"))
                      .arg(QString::number(elem.value,16))
                      .arg(comma)
                      .arg(elem.comment));
    }
    qDebug() << "output: \n" << output;
#endif
}

void TableGenerator::orderComposeTable()
{
    // Stable-sorting to ensure that the item that appeared before the other in the
    // original container will still appear first after the sort. This property is
    // needed to handle the cases when user re-defines already defined key sequence
    std::stable_sort(m_composeTable.begin(), m_composeTable.end(), Compare());
}

