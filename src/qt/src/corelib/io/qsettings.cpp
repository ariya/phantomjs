/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qdebug.h>
#include "qplatformdefs.h"
#include "qsettings.h"

#ifndef QT_NO_SETTINGS

#include "qsettings_p.h"
#include "qcache.h"
#include "qfile.h"
#include "qdir.h"
#include "qfileinfo.h"
#include "qmutex.h"
#include "qlibraryinfo.h"
#include "qtemporaryfile.h"

#ifndef QT_NO_TEXTCODEC
#  include "qtextcodec.h"
#endif

#ifndef QT_NO_GEOM_VARIANT
#include "qsize.h"
#include "qpoint.h"
#include "qrect.h"
#endif // !QT_NO_GEOM_VARIANT

#ifndef QT_NO_QOBJECT
#include "qcoreapplication.h"
#endif

#ifdef Q_OS_WIN // for homedirpath reading from registry
#include "qt_windows.h"
#include <private/qsystemlibrary_p.h>
#endif

#ifdef Q_OS_VXWORKS
#  include <ioLib.h>
#endif

#include <stdlib.h>

#ifndef CSIDL_COMMON_APPDATA
#define CSIDL_COMMON_APPDATA	0x0023  // All Users\Application Data
#endif

#ifndef CSIDL_APPDATA
#define CSIDL_APPDATA		0x001a	// <username>\Application Data
#endif

#ifdef Q_AUTOTEST_EXPORT
#  define Q_AUTOTEST_EXPORT_HELPER Q_AUTOTEST_EXPORT
#else
#  define Q_AUTOTEST_EXPORT_HELPER static
#endif

// ************************************************************************
// QConfFile

/*
    QConfFile objects are explicitly shared within the application.
    This ensures that modification to the settings done through one
    QSettings object are immediately reflected in other setting
    objects of the same application.
*/

QT_BEGIN_NAMESPACE

struct QConfFileCustomFormat
{
    QString extension;
    QSettings::ReadFunc readFunc;
    QSettings::WriteFunc writeFunc;
    Qt::CaseSensitivity caseSensitivity;
};

typedef QHash<QString, QConfFile *> ConfFileHash;
typedef QCache<QString, QConfFile> ConfFileCache;
typedef QHash<int, QString> PathHash;
typedef QVector<QConfFileCustomFormat> CustomFormatVector;

Q_GLOBAL_STATIC(ConfFileHash, usedHashFunc)
Q_GLOBAL_STATIC(ConfFileCache, unusedCacheFunc)
Q_GLOBAL_STATIC(PathHash, pathHashFunc)
Q_GLOBAL_STATIC(CustomFormatVector, customFormatVectorFunc)
Q_GLOBAL_STATIC(QMutex, globalMutex)
static QSettings::Format globalDefaultFormat = QSettings::NativeFormat;

#ifndef Q_OS_WIN
inline bool qt_isEvilFsTypeName(const char *name)
{
    return (qstrncmp(name, "nfs", 3) == 0
            || qstrncmp(name, "autofs", 6) == 0
            || qstrncmp(name, "cachefs", 7) == 0);
}

#if defined(Q_OS_BSD4) && !defined(Q_OS_NETBSD)
QT_BEGIN_INCLUDE_NAMESPACE
# include <sys/param.h>
# include <sys/mount.h>
QT_END_INCLUDE_NAMESPACE

Q_AUTOTEST_EXPORT_HELPER bool qIsLikelyToBeNfs(int handle)
{
    struct statfs buf;
    if (fstatfs(handle, &buf) != 0)
        return false;
    return qt_isEvilFsTypeName(buf.f_fstypename);
}

#elif defined(Q_OS_LINUX) || defined(Q_OS_HURD)
QT_BEGIN_INCLUDE_NAMESPACE
# include <sys/vfs.h>
# ifdef QT_LINUXBASE
   // LSB 3.2 has fstatfs in sys/statfs.h, sys/vfs.h is just an empty dummy header
#  include <sys/statfs.h>
# endif
QT_END_INCLUDE_NAMESPACE
# ifndef NFS_SUPER_MAGIC
#  define NFS_SUPER_MAGIC       0x00006969
# endif
# ifndef AUTOFS_SUPER_MAGIC
#  define AUTOFS_SUPER_MAGIC    0x00000187
# endif
# ifndef AUTOFSNG_SUPER_MAGIC
#  define AUTOFSNG_SUPER_MAGIC  0x7d92b1a0
# endif

Q_AUTOTEST_EXPORT_HELPER bool qIsLikelyToBeNfs(int handle)
{
    struct statfs buf;
    if (fstatfs(handle, &buf) != 0)
        return false;
    return buf.f_type == NFS_SUPER_MAGIC
           || buf.f_type == AUTOFS_SUPER_MAGIC
           || buf.f_type == AUTOFSNG_SUPER_MAGIC;
}

#elif defined(Q_OS_SOLARIS) || defined(Q_OS_IRIX) || defined(Q_OS_AIX) || defined(Q_OS_HPUX) \
      || defined(Q_OS_OSF) || defined(Q_OS_QNX) || defined(Q_OS_SCO) \
      || defined(Q_OS_UNIXWARE) || defined(Q_OS_RELIANT) || defined(Q_OS_NETBSD)
QT_BEGIN_INCLUDE_NAMESPACE
# include <sys/statvfs.h>
QT_END_INCLUDE_NAMESPACE

Q_AUTOTEST_EXPORT_HELPER bool qIsLikelyToBeNfs(int handle)
{
    struct statvfs buf;
    if (fstatvfs(handle, &buf) != 0)
        return false;
#if defined(Q_OS_NETBSD)
    return qt_isEvilFsTypeName(buf.f_fstypename);
#else
    return qt_isEvilFsTypeName(buf.f_basetype);
#endif
}
#else
Q_AUTOTEST_EXPORT_HELPER inline bool qIsLikelyToBeNfs(int /* handle */)
{
    return true;
}
#endif

static bool unixLock(int handle, int lockType)
{
    /*
        NFS hangs on the fcntl() call below when statd or lockd isn't
        running. There's no way to detect this. Our work-around for
        now is to disable locking when we detect NFS (or AutoFS or
        CacheFS, which are probably wrapping NFS).
    */
    if (qIsLikelyToBeNfs(handle))
        return false;

    struct flock fl;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_type = lockType;
    return fcntl(handle, F_SETLKW, &fl) == 0;
}
#endif

QConfFile::QConfFile(const QString &fileName, bool _userPerms)
    : name(fileName), size(0), ref(1), userPerms(_userPerms)
{
    usedHashFunc()->insert(name, this);
}

QConfFile::~QConfFile()
{
    if (usedHashFunc())
        usedHashFunc()->remove(name);
}

ParsedSettingsMap QConfFile::mergedKeyMap() const
{
    ParsedSettingsMap result = originalKeys;
    ParsedSettingsMap::const_iterator i;

    for (i = removedKeys.begin(); i != removedKeys.end(); ++i)
        result.remove(i.key());
    for (i = addedKeys.begin(); i != addedKeys.end(); ++i)
        result.insert(i.key(), i.value());
    return result;
}

bool QConfFile::isWritable() const
{
    QFileInfo fileInfo(name);

#ifndef QT_NO_TEMPORARYFILE
    if (fileInfo.exists()) {
#endif
        QFile file(name);
        return file.open(QFile::ReadWrite);
#ifndef QT_NO_TEMPORARYFILE
    } else {
        // Create the directories to the file.
        QDir dir(fileInfo.absolutePath());
        if (!dir.exists()) {
            if (!dir.mkpath(dir.absolutePath()))
                return false;
        }

        // we use a temporary file to avoid race conditions
        QTemporaryFile file(name);
        return file.open();
    }
#endif
}

QConfFile *QConfFile::fromName(const QString &fileName, bool _userPerms)
{
    QString absPath = QFileInfo(fileName).absoluteFilePath();

    ConfFileHash *usedHash = usedHashFunc();
    ConfFileCache *unusedCache = unusedCacheFunc();

    QConfFile *confFile = 0;
    QMutexLocker locker(globalMutex());

    if (!(confFile = usedHash->value(absPath))) {
        if ((confFile = unusedCache->take(absPath)))
            usedHash->insert(absPath, confFile);
    }
    if (confFile) {
        confFile->ref.ref();
        return confFile;
    }
    return new QConfFile(absPath, _userPerms);
}

void QConfFile::clearCache()
{
    QMutexLocker locker(globalMutex());
    unusedCacheFunc()->clear();
}

// ************************************************************************
// QSettingsPrivate

QSettingsPrivate::QSettingsPrivate(QSettings::Format format)
    : format(format), scope(QSettings::UserScope /* nothing better to put */), iniCodec(0), spec(0), fallbacks(true),
      pendingChanges(false), status(QSettings::NoError)
{
}

QSettingsPrivate::QSettingsPrivate(QSettings::Format format, QSettings::Scope scope,
                                   const QString &organization, const QString &application)
    : format(format), scope(scope), organizationName(organization), applicationName(application),
      iniCodec(0), spec(0), fallbacks(true), pendingChanges(false), status(QSettings::NoError)
{
}

QSettingsPrivate::~QSettingsPrivate()
{
}

QString QSettingsPrivate::actualKey(const QString &key) const
{
    QString n = normalizedKey(key);
    Q_ASSERT_X(!n.isEmpty(), "QSettings", "empty key");
    n.prepend(groupPrefix);
    return n;
}

/*
    Returns a string that never starts nor ends with a slash (or an
    empty string). Examples:

            "foo"            becomes   "foo"
            "/foo//bar///"   becomes   "foo/bar"
            "///"            becomes   ""

    This function is optimized to avoid a QString deep copy in the
    common case where the key is already normalized.
*/
QString QSettingsPrivate::normalizedKey(const QString &key)
{
    QString result = key;

    int i = 0;
    while (i < result.size()) {
        while (result.at(i) == QLatin1Char('/')) {
            result.remove(i, 1);
            if (i == result.size())
                goto after_loop;
        }
        while (result.at(i) != QLatin1Char('/')) {
            ++i;
            if (i == result.size())
                return result;
        }
        ++i; // leave the slash alone
    }

after_loop:
    if (!result.isEmpty())
        result.truncate(i - 1); // remove the trailing slash
    return result;
}

// see also qsettings_win.cpp and qsettings_mac.cpp

#if !defined(Q_OS_WIN) && !defined(Q_OS_MAC)
QSettingsPrivate *QSettingsPrivate::create(QSettings::Format format, QSettings::Scope scope,
                                           const QString &organization, const QString &application)
{
    return new QConfFileSettingsPrivate(format, scope, organization, application);
}
#endif

#if !defined(Q_OS_WIN)
QSettingsPrivate *QSettingsPrivate::create(const QString &fileName, QSettings::Format format)
{
    return new QConfFileSettingsPrivate(fileName, format);
}
#endif

void QSettingsPrivate::processChild(QString key, ChildSpec spec, QMap<QString, QString> &result)
{
    if (spec != AllKeys) {
        int slashPos = key.indexOf(QLatin1Char('/'));
        if (slashPos == -1) {
            if (spec != ChildKeys)
                return;
        } else {
            if (spec != ChildGroups)
                return;
            key.truncate(slashPos);
        }
    }
    result.insert(key, QString());
}

void QSettingsPrivate::beginGroupOrArray(const QSettingsGroup &group)
{
    groupStack.push(group);
    if (!group.name().isEmpty()) {
        groupPrefix += group.name();
        groupPrefix += QLatin1Char('/');
    }
}

/*
    We only set an error if there isn't one set already. This way the user always gets the
    first error that occurred. We always allow clearing errors.
*/

void QSettingsPrivate::setStatus(QSettings::Status status) const
{
    if (status == QSettings::NoError || this->status == QSettings::NoError)
        this->status = status;
}

void QSettingsPrivate::update()
{
    flush();
    pendingChanges = false;
}

void QSettingsPrivate::requestUpdate()
{
    if (!pendingChanges) {
        pendingChanges = true;
#ifndef QT_NO_QOBJECT
        Q_Q(QSettings);
        QCoreApplication::postEvent(q, new QEvent(QEvent::UpdateRequest));
#else
        update();
#endif
    }
}

QStringList QSettingsPrivate::variantListToStringList(const QVariantList &l)
{
    QStringList result;
    QVariantList::const_iterator it = l.constBegin();
    for (; it != l.constEnd(); ++it)
        result.append(variantToString(*it));
    return result;
}

QVariant QSettingsPrivate::stringListToVariantList(const QStringList &l)
{
    QStringList outStringList = l;
    for (int i = 0; i < outStringList.count(); ++i) {
        const QString &str = outStringList.at(i);

        if (str.startsWith(QLatin1Char('@'))) {
            if (str.length() >= 2 && str.at(1) == QLatin1Char('@')) {
                outStringList[i].remove(0, 1);
            } else {
                QVariantList variantList;
                for (int j = 0; j < l.count(); ++j)
                    variantList.append(stringToVariant(l.at(j)));
                return variantList;
            }
        }
    }
    return outStringList;
}

QString QSettingsPrivate::variantToString(const QVariant &v)
{
    QString result;

    switch (v.type()) {
        case QVariant::Invalid:
            result = QLatin1String("@Invalid()");
            break;

        case QVariant::ByteArray: {
            QByteArray a = v.toByteArray();
            result = QLatin1String("@ByteArray(");
            result += QString::fromLatin1(a.constData(), a.size());
            result += QLatin1Char(')');
            break;
        }

        case QVariant::String:
        case QVariant::LongLong:
        case QVariant::ULongLong:
        case QVariant::Int:
        case QVariant::UInt:
        case QVariant::Bool:
        case QVariant::Double:
        case QVariant::KeySequence: {
            result = v.toString();
            if (result.startsWith(QLatin1Char('@')))
                result.prepend(QLatin1Char('@'));
            break;
        }
#ifndef QT_NO_GEOM_VARIANT
        case QVariant::Rect: {
            QRect r = qvariant_cast<QRect>(v);
            result += QLatin1String("@Rect(");
            result += QString::number(r.x());
            result += QLatin1Char(' ');
            result += QString::number(r.y());
            result += QLatin1Char(' ');
            result += QString::number(r.width());
            result += QLatin1Char(' ');
            result += QString::number(r.height());
            result += QLatin1Char(')');
            break;
        }
        case QVariant::Size: {
            QSize s = qvariant_cast<QSize>(v);
            result += QLatin1String("@Size(");
            result += QString::number(s.width());
            result += QLatin1Char(' ');
            result += QString::number(s.height());
            result += QLatin1Char(')');
            break;
        }
        case QVariant::Point: {
            QPoint p = qvariant_cast<QPoint>(v);
            result += QLatin1String("@Point(");
            result += QString::number(p.x());
            result += QLatin1Char(' ');
            result += QString::number(p.y());
            result += QLatin1Char(')');
            break;
        }
#endif // !QT_NO_GEOM_VARIANT

        default: {
#ifndef QT_NO_DATASTREAM
            QByteArray a;
            {
                QDataStream s(&a, QIODevice::WriteOnly);
                s.setVersion(QDataStream::Qt_4_0);
                s << v;
            }

            result = QLatin1String("@Variant(");
            result += QString::fromLatin1(a.constData(), a.size());
            result += QLatin1Char(')');
#else
            Q_ASSERT(!"QSettings: Cannot save custom types without QDataStream support");
#endif
            break;
        }
    }

    return result;
}


QVariant QSettingsPrivate::stringToVariant(const QString &s)
{
    if (s.startsWith(QLatin1Char('@'))) {
        if (s.endsWith(QLatin1Char(')'))) {
            if (s.startsWith(QLatin1String("@ByteArray("))) {
                return QVariant(s.toLatin1().mid(11, s.size() - 12));
            } else if (s.startsWith(QLatin1String("@Variant("))) {
#ifndef QT_NO_DATASTREAM
                QByteArray a(s.toLatin1().mid(9));
                QDataStream stream(&a, QIODevice::ReadOnly);
                stream.setVersion(QDataStream::Qt_4_0);
                QVariant result;
                stream >> result;
                return result;
#else
                Q_ASSERT(!"QSettings: Cannot load custom types without QDataStream support");
#endif
#ifndef QT_NO_GEOM_VARIANT
            } else if (s.startsWith(QLatin1String("@Rect("))) {
                QStringList args = QSettingsPrivate::splitArgs(s, 5);
                if (args.size() == 4)
                    return QVariant(QRect(args[0].toInt(), args[1].toInt(), args[2].toInt(), args[3].toInt()));
            } else if (s.startsWith(QLatin1String("@Size("))) {
                QStringList args = QSettingsPrivate::splitArgs(s, 5);
                if (args.size() == 2)
                    return QVariant(QSize(args[0].toInt(), args[1].toInt()));
            } else if (s.startsWith(QLatin1String("@Point("))) {
                QStringList args = QSettingsPrivate::splitArgs(s, 6);
                if (args.size() == 2)
                    return QVariant(QPoint(args[0].toInt(), args[1].toInt()));
#endif
            } else if (s == QLatin1String("@Invalid()")) {
                return QVariant();
            }

        }
        if (s.startsWith(QLatin1String("@@")))
            return QVariant(s.mid(1));
    }

    return QVariant(s);
}

static const char hexDigits[] = "0123456789ABCDEF";

void QSettingsPrivate::iniEscapedKey(const QString &key, QByteArray &result)
{
    result.reserve(result.length() + key.length() * 3 / 2);
    for (int i = 0; i < key.size(); ++i) {
        uint ch = key.at(i).unicode();

        if (ch == '/') {
            result += '\\';
        } else if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')
                || ch == '_' || ch == '-' || ch == '.') {
            result += (char)ch;
        } else if (ch <= 0xFF) {
            result += '%';
            result += hexDigits[ch / 16];
            result += hexDigits[ch % 16];
        } else {
            result += "%U";
            QByteArray hexCode;
            for (int i = 0; i < 4; ++i) {
                hexCode.prepend(hexDigits[ch % 16]);
                ch >>= 4;
            }
            result += hexCode;
        }
    }
}

bool QSettingsPrivate::iniUnescapedKey(const QByteArray &key, int from, int to, QString &result)
{
    bool lowercaseOnly = true;
    int i = from;
    result.reserve(result.length() + (to - from));
    while (i < to) {
        int ch = (uchar)key.at(i);

        if (ch == '\\') {
            result += QLatin1Char('/');
            ++i;
            continue;
        }

        if (ch != '%' || i == to - 1) {
            if (uint(ch - 'A') <= 'Z' - 'A') // only for ASCII
                lowercaseOnly = false;
            result += QLatin1Char(ch);
            ++i;
            continue;
        }

        int numDigits = 2;
        int firstDigitPos = i + 1;

        ch = key.at(i + 1);
        if (ch == 'U') {
            ++firstDigitPos;
            numDigits = 4;
        }

        if (firstDigitPos + numDigits > to) {
            result += QLatin1Char('%');
            // ### missing U
            ++i;
            continue;
        }

        bool ok;
        ch = key.mid(firstDigitPos, numDigits).toInt(&ok, 16);
        if (!ok) {
            result += QLatin1Char('%');
            // ### missing U
            ++i;
            continue;
        }

        QChar qch(ch);
        if (qch.isUpper())
            lowercaseOnly = false;
        result += qch;
        i = firstDigitPos + numDigits;
    }
    return lowercaseOnly;
}

void QSettingsPrivate::iniEscapedString(const QString &str, QByteArray &result, QTextCodec *codec)
{
    bool needsQuotes = false;
    bool escapeNextIfDigit = false;
    int i;
    int startPos = result.size();

    result.reserve(startPos + str.size() * 3 / 2);
    for (i = 0; i < str.size(); ++i) {
        uint ch = str.at(i).unicode();
        if (ch == ';' || ch == ',' || ch == '=')
            needsQuotes = true;

        if (escapeNextIfDigit
                && ((ch >= '0' && ch <= '9')
                    || (ch >= 'a' && ch <= 'f')
                    || (ch >= 'A' && ch <= 'F'))) {
            result += "\\x";
            result += QByteArray::number(ch, 16);
            continue;
        }

        escapeNextIfDigit = false;

        switch (ch) {
        case '\0':
            result += "\\0";
            escapeNextIfDigit = true;
            break;
        case '\a':
            result += "\\a";
            break;
        case '\b':
            result += "\\b";
            break;
        case '\f':
            result += "\\f";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        case '\v':
            result += "\\v";
            break;
        case '"':
        case '\\':
            result += '\\';
            result += (char)ch;
            break;
        default:
            if (ch <= 0x1F || (ch >= 0x7F && !codec)) {
                result += "\\x";
                result += QByteArray::number(ch, 16);
                escapeNextIfDigit = true;
#ifndef QT_NO_TEXTCODEC
            } else if (codec) {
                // slow
                result += codec->fromUnicode(str.at(i));
#endif
            } else {
                result += (char)ch;
            }
        }
    }

    if (needsQuotes
            || (startPos < result.size() && (result.at(startPos) == ' '
                                                || result.at(result.size() - 1) == ' '))) {
        result.insert(startPos, '"');
        result += '"';
    }
}

inline static void iniChopTrailingSpaces(QString &str)
{
    int n = str.size() - 1;
    QChar ch;
    while (n >= 0 && ((ch = str.at(n)) == QLatin1Char(' ') || ch == QLatin1Char('\t')))
        str.truncate(n--);
}

void QSettingsPrivate::iniEscapedStringList(const QStringList &strs, QByteArray &result, QTextCodec *codec)
{
    if (strs.isEmpty()) {
        /*
            We need to distinguish between empty lists and one-item
            lists that contain an empty string. Ideally, we'd have a
            @EmptyList() symbol but that would break compatibility
            with Qt 4.0. @Invalid() stands for QVariant(), and
            QVariant().toStringList() returns an empty QStringList,
            so we're in good shape.

            ### Qt 5: Use a nicer syntax, e.g. @List, for variant lists
        */
        result += "@Invalid()";
    } else {
        for (int i = 0; i < strs.size(); ++i) {
            if (i != 0)
                result += ", ";
            iniEscapedString(strs.at(i), result, codec);
        }
    }
}

bool QSettingsPrivate::iniUnescapedStringList(const QByteArray &str, int from, int to,
                                              QString &stringResult, QStringList &stringListResult,
                                              QTextCodec *codec)
{
#ifdef QT_NO_TEXTCODE
    Q_UNUSED(codec);
#endif
    static const char escapeCodes[][2] =
    {
        { 'a', '\a' },
        { 'b', '\b' },
        { 'f', '\f' },
        { 'n', '\n' },
        { 'r', '\r' },
        { 't', '\t' },
        { 'v', '\v' },
        { '"', '"' },
        { '?', '?' },
        { '\'', '\'' },
        { '\\', '\\' }
    };
    static const int numEscapeCodes = sizeof(escapeCodes) / sizeof(escapeCodes[0]);

    bool isStringList = false;
    bool inQuotedString = false;
    bool currentValueIsQuoted = false;
    int escapeVal = 0;
    int i = from;
    char ch;

StSkipSpaces:
    while (i < to && ((ch = str.at(i)) == ' ' || ch == '\t'))
        ++i;
    // fallthrough

StNormal:
    while (i < to) {
        switch (str.at(i)) {
        case '\\':
            ++i;
            if (i >= to)
                goto end;

            ch = str.at(i++);
            for (int j = 0; j < numEscapeCodes; ++j) {
                if (ch == escapeCodes[j][0]) {
                    stringResult += QLatin1Char(escapeCodes[j][1]);
                    goto StNormal;
                }
            }

            if (ch == 'x') {
                escapeVal = 0;

                if (i >= to)
                    goto end;

                ch = str.at(i);
                if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f'))
                    goto StHexEscape;
            } else if (ch >= '0' && ch <= '7') {
                escapeVal = ch - '0';
                goto StOctEscape;
            } else if (ch == '\n' || ch == '\r') {
                if (i < to) {
                    char ch2 = str.at(i);
                    // \n, \r, \r\n, and \n\r are legitimate line terminators in INI files
                    if ((ch2 == '\n' || ch2 == '\r') && ch2 != ch)
                        ++i;
                }
            } else {
                // the character is skipped
            }
            break;
        case '"':
            ++i;
            currentValueIsQuoted = true;
            inQuotedString = !inQuotedString;
            if (!inQuotedString)
                goto StSkipSpaces;
            break;
        case ',':
            if (!inQuotedString) {
                if (!currentValueIsQuoted)
                    iniChopTrailingSpaces(stringResult);
                if (!isStringList) {
                    isStringList = true;
                    stringListResult.clear();
                    stringResult.squeeze();
                }
                stringListResult.append(stringResult);
                stringResult.clear();
                currentValueIsQuoted = false;
                ++i;
                goto StSkipSpaces;
            }
            // fallthrough
        default: {
            int j = i + 1;
            while (j < to) {
                ch = str.at(j);
                if (ch == '\\' || ch == '"' || ch == ',')
                    break;
                ++j;
            }

#ifndef QT_NO_TEXTCODEC
            if (codec) {
                stringResult += codec->toUnicode(str.constData() + i, j - i);
            } else
#endif
            {
                int n = stringResult.size();
                stringResult.resize(n + (j - i));
                QChar *resultData = stringResult.data() + n;
                for (int k = i; k < j; ++k)
                    *resultData++ = QLatin1Char(str.at(k));
            }
            i = j;
        }
        }
    }
    goto end;

StHexEscape:
    if (i >= to) {
        stringResult += QChar(escapeVal);
        goto end;
    }

    ch = str.at(i);
    if (ch >= 'a')
        ch -= 'a' - 'A';
    if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F')) {
        escapeVal <<= 4;
        escapeVal += strchr(hexDigits, ch) - hexDigits;
        ++i;
        goto StHexEscape;
    } else {
        stringResult += QChar(escapeVal);
        goto StNormal;
    }

StOctEscape:
    if (i >= to) {
        stringResult += QChar(escapeVal);
        goto end;
    }

    ch = str.at(i);
    if (ch >= '0' && ch <= '7') {
        escapeVal <<= 3;
        escapeVal += ch - '0';
        ++i;
        goto StOctEscape;
    } else {
        stringResult += QChar(escapeVal);
        goto StNormal;
    }

end:
    if (!currentValueIsQuoted)
        iniChopTrailingSpaces(stringResult);
    if (isStringList)
        stringListResult.append(stringResult);
    return isStringList;
}

QStringList QSettingsPrivate::splitArgs(const QString &s, int idx)
{
    int l = s.length();
    Q_ASSERT(l > 0);
    Q_ASSERT(s.at(idx) == QLatin1Char('('));
    Q_ASSERT(s.at(l - 1) == QLatin1Char(')'));

    QStringList result;
    QString item;

    for (++idx; idx < l; ++idx) {
        QChar c = s.at(idx);
        if (c == QLatin1Char(')')) {
            Q_ASSERT(idx == l - 1);
            result.append(item);
        } else if (c == QLatin1Char(' ')) {
            result.append(item);
            item.clear();
        } else {
            item.append(c);
        }
    }

    return result;
}

// ************************************************************************
// QConfFileSettingsPrivate

void QConfFileSettingsPrivate::initFormat()
{
    extension = (format == QSettings::NativeFormat) ? QLatin1String(".conf") : QLatin1String(".ini");
    readFunc = 0;
    writeFunc = 0;
#if defined(Q_OS_MAC)
    caseSensitivity = (format == QSettings::NativeFormat) ? Qt::CaseSensitive : IniCaseSensitivity;
#else
    caseSensitivity = IniCaseSensitivity;
#endif

    if (format > QSettings::IniFormat) {
        QMutexLocker locker(globalMutex());
        const CustomFormatVector *customFormatVector = customFormatVectorFunc();

        int i = (int)format - (int)QSettings::CustomFormat1;
        if (i >= 0 && i < customFormatVector->size()) {
            QConfFileCustomFormat info = customFormatVector->at(i);
            extension = info.extension;
            readFunc = info.readFunc;
            writeFunc = info.writeFunc;
            caseSensitivity = info.caseSensitivity;
        }
    }
}

void QConfFileSettingsPrivate::initAccess()
{
    if (confFiles[spec]) {
        if (format > QSettings::IniFormat) {
            if (!readFunc)
                setStatus(QSettings::AccessError);
        }
    }

    sync();       // loads the files the first time
}

#ifdef Q_OS_WIN
static QString windowsConfigPath(int type)
{
    QString result;

#ifndef Q_OS_WINCE
    QSystemLibrary library(QLatin1String("shell32"));
#else
    QSystemLibrary library(QLatin1String("coredll"));
#endif // Q_OS_WINCE
    typedef BOOL (WINAPI*GetSpecialFolderPath)(HWND, LPWSTR, int, BOOL);
    GetSpecialFolderPath SHGetSpecialFolderPath = (GetSpecialFolderPath)library.resolve("SHGetSpecialFolderPathW");
    if (SHGetSpecialFolderPath) {
        wchar_t path[MAX_PATH];
        SHGetSpecialFolderPath(0, path, type, FALSE);
        result = QString::fromWCharArray(path);
    }

    if (result.isEmpty()) {
        switch (type) {
#ifndef Q_OS_WINCE
        case CSIDL_COMMON_APPDATA:
            result = QLatin1String("C:\\temp\\qt-common");
            break;
        case CSIDL_APPDATA:
            result = QLatin1String("C:\\temp\\qt-user");
            break;
#else
        case CSIDL_COMMON_APPDATA:
            result = QLatin1String("\\Temp\\qt-common");
            break;
        case CSIDL_APPDATA:
            result = QLatin1String("\\Temp\\qt-user");
            break;
#endif
        default:
            ;
        }
    }

    return result;
}
#endif // Q_OS_WIN

static inline int pathHashKey(QSettings::Format format, QSettings::Scope scope)
{
    return int((uint(format) << 1) | uint(scope == QSettings::SystemScope));
}

static void initDefaultPaths(QMutexLocker *locker)
{
    PathHash *pathHash = pathHashFunc();
    QString homePath = QDir::homePath();
    QString systemPath;

    locker->unlock();
	
    /*
       QLibraryInfo::location() uses QSettings, so in order to
       avoid a dead-lock, we can't hold the global mutex while
       calling it.
    */
    systemPath = QLibraryInfo::location(QLibraryInfo::SettingsPath);
    systemPath += QLatin1Char('/');

    locker->relock();
    if (pathHash->isEmpty()) {
        /*
           Lazy initialization of pathHash. We initialize the
           IniFormat paths and (on Unix) the NativeFormat paths.
           (The NativeFormat paths are not configurable for the
           Windows registry and the Mac CFPreferences.)
       */
#ifdef Q_OS_WIN
        pathHash->insert(pathHashKey(QSettings::IniFormat, QSettings::UserScope),
                         windowsConfigPath(CSIDL_APPDATA) + QDir::separator());
        pathHash->insert(pathHashKey(QSettings::IniFormat, QSettings::SystemScope),
                         windowsConfigPath(CSIDL_COMMON_APPDATA) + QDir::separator());
#else
        QString userPath;
        char *env = getenv("XDG_CONFIG_HOME");
        if (env == 0) {
            userPath = homePath;
            userPath += QLatin1Char('/');
#if defined(Q_WS_QWS) || defined(Q_WS_QPA)
            userPath += QLatin1String("Settings");
#else
            userPath += QLatin1String(".config");
#endif
        } else if (*env == '/') {
            userPath = QFile::decodeName(env);
        } else {
            userPath = homePath;
            userPath += QLatin1Char('/');
            userPath += QFile::decodeName(env);
        }
        userPath += QLatin1Char('/');

        pathHash->insert(pathHashKey(QSettings::IniFormat, QSettings::UserScope), userPath);
        pathHash->insert(pathHashKey(QSettings::IniFormat, QSettings::SystemScope), systemPath);
#ifndef Q_OS_MAC
        pathHash->insert(pathHashKey(QSettings::NativeFormat, QSettings::UserScope), userPath);
        pathHash->insert(pathHashKey(QSettings::NativeFormat, QSettings::SystemScope), systemPath);
#endif
#endif
    }
}

static QString getPath(QSettings::Format format, QSettings::Scope scope)
{
    Q_ASSERT((int)QSettings::NativeFormat == 0);
    Q_ASSERT((int)QSettings::IniFormat == 1);

    QMutexLocker locker(globalMutex());
    PathHash *pathHash = pathHashFunc();
    if (pathHash->isEmpty())
        initDefaultPaths(&locker);

    QString result = pathHash->value(pathHashKey(format, scope));
    if (!result.isEmpty())
        return result;

    // fall back on INI path
    return pathHash->value(pathHashKey(QSettings::IniFormat, scope));
}

QConfFileSettingsPrivate::QConfFileSettingsPrivate(QSettings::Format format,
                                                   QSettings::Scope scope,
                                                   const QString &organization,
                                                   const QString &application)
    : QSettingsPrivate(format, scope, organization, application),
      nextPosition(0x40000000) // big positive number
{
    int i;
    initFormat();

    QString org = organization;
    if (org.isEmpty()) {
        setStatus(QSettings::AccessError);
        org = QLatin1String("Unknown Organization");
    }

    QString appFile = org + QDir::separator() + application + extension;
    QString orgFile = org + extension;

    if (scope == QSettings::UserScope) {
        QString userPath = getPath(format, QSettings::UserScope);
        if (!application.isEmpty())
            confFiles[F_User | F_Application].reset(QConfFile::fromName(userPath + appFile, true));
        confFiles[F_User | F_Organization].reset(QConfFile::fromName(userPath + orgFile, true));
    }

    QString systemPath = getPath(format, QSettings::SystemScope);
    if (!application.isEmpty())
        confFiles[F_System | F_Application].reset(QConfFile::fromName(systemPath + appFile, false));
    confFiles[F_System | F_Organization].reset(QConfFile::fromName(systemPath + orgFile, false));

    for (i = 0; i < NumConfFiles; ++i) {
        if (confFiles[i]) {
            spec = i;
            break;
        }
    }

    initAccess();
}

QConfFileSettingsPrivate::QConfFileSettingsPrivate(const QString &fileName,
                                                   QSettings::Format format)
    : QSettingsPrivate(format),
      nextPosition(0x40000000) // big positive number
{
    initFormat();

    confFiles[0].reset(QConfFile::fromName(fileName, true));

    initAccess();
}

QConfFileSettingsPrivate::~QConfFileSettingsPrivate()
{
    QMutexLocker locker(globalMutex());
    ConfFileHash *usedHash = usedHashFunc();
    ConfFileCache *unusedCache = unusedCacheFunc();

    for (int i = 0; i < NumConfFiles; ++i) {
        if (confFiles[i] && !confFiles[i]->ref.deref()) {
            if (confFiles[i]->size == 0) {
                delete confFiles[i].take();
            } else {
                if (usedHash)
                    usedHash->remove(confFiles[i]->name);
                if (unusedCache) {
                    QT_TRY {
                        // compute a better size?
                        unusedCache->insert(confFiles[i]->name, confFiles[i].data(),
                                        10 + (confFiles[i]->originalKeys.size() / 4));
                        confFiles[i].take();
                    } QT_CATCH(...) {
                        // out of memory. Do not cache the file.
                        delete confFiles[i].take();
                    }
                } else {
                    // unusedCache is gone - delete the entry to prevent a memory leak
                    delete confFiles[i].take();
                }
            }
        }
        // prevent the ScopedPointer to deref it again.
        confFiles[i].take();
    }
}

void QConfFileSettingsPrivate::remove(const QString &key)
{
    QConfFile *confFile = confFiles[spec].data();
    if (!confFile)
        return;

    QSettingsKey theKey(key, caseSensitivity);
    QSettingsKey prefix(key + QLatin1Char('/'), caseSensitivity);
    QMutexLocker locker(&confFile->mutex);

    ensureSectionParsed(confFile, theKey);
    ensureSectionParsed(confFile, prefix);

    ParsedSettingsMap::iterator i = confFile->addedKeys.lowerBound(prefix);
    while (i != confFile->addedKeys.end() && i.key().startsWith(prefix))
        i = confFile->addedKeys.erase(i);
    confFile->addedKeys.remove(theKey);

    ParsedSettingsMap::const_iterator j = const_cast<const ParsedSettingsMap *>(&confFile->originalKeys)->lowerBound(prefix);
    while (j != confFile->originalKeys.constEnd() && j.key().startsWith(prefix)) {
        confFile->removedKeys.insert(j.key(), QVariant());
        ++j;
    }
    if (confFile->originalKeys.contains(theKey))
        confFile->removedKeys.insert(theKey, QVariant());
}

void QConfFileSettingsPrivate::set(const QString &key, const QVariant &value)
{
    QConfFile *confFile = confFiles[spec].data();
    if (!confFile)
        return;

    QSettingsKey theKey(key, caseSensitivity, nextPosition++);
    QMutexLocker locker(&confFile->mutex);
    confFile->removedKeys.remove(theKey);
    confFile->addedKeys.insert(theKey, value);
}

bool QConfFileSettingsPrivate::get(const QString &key, QVariant *value) const
{
    QSettingsKey theKey(key, caseSensitivity);
    ParsedSettingsMap::const_iterator j;
    bool found = false;

    for (int i = 0; i < NumConfFiles; ++i) {
        if (QConfFile *confFile = confFiles[i].data()) {
            QMutexLocker locker(&confFile->mutex);

            if (!confFile->addedKeys.isEmpty()) {
                j = confFile->addedKeys.constFind(theKey);
                found = (j != confFile->addedKeys.constEnd());
            }
            if (!found) {
                ensureSectionParsed(confFile, theKey);
                j = confFile->originalKeys.constFind(theKey);
                found = (j != confFile->originalKeys.constEnd()
                         && !confFile->removedKeys.contains(theKey));
            }

            if (found && value)
                *value = *j;

            if (found)
                return true;
            if (!fallbacks)
                break;
        }
    }
    return false;
}

QStringList QConfFileSettingsPrivate::children(const QString &prefix, ChildSpec spec) const
{
    QMap<QString, QString> result;
    ParsedSettingsMap::const_iterator j;

    QSettingsKey thePrefix(prefix, caseSensitivity);
    int startPos = prefix.size();

    for (int i = 0; i < NumConfFiles; ++i) {
        if (QConfFile *confFile = confFiles[i].data()) {
            QMutexLocker locker(&confFile->mutex);

            if (thePrefix.isEmpty()) {
                ensureAllSectionsParsed(confFile);
            } else {
                ensureSectionParsed(confFile, thePrefix);
            }

            j = const_cast<const ParsedSettingsMap *>(
                    &confFile->originalKeys)->lowerBound( thePrefix);
            while (j != confFile->originalKeys.constEnd() && j.key().startsWith(thePrefix)) {
                if (!confFile->removedKeys.contains(j.key()))
                    processChild(j.key().originalCaseKey().mid(startPos), spec, result);
                ++j;
            }

            j = const_cast<const ParsedSettingsMap *>(
                    &confFile->addedKeys)->lowerBound(thePrefix);
            while (j != confFile->addedKeys.constEnd() && j.key().startsWith(thePrefix)) {
                processChild(j.key().originalCaseKey().mid(startPos), spec, result);
                ++j;
            }

            if (!fallbacks)
                break;
        }
    }
    return result.keys();
}

void QConfFileSettingsPrivate::clear()
{
    QConfFile *confFile = confFiles[spec].data();
    if (!confFile)
        return;

    QMutexLocker locker(&confFile->mutex);
    ensureAllSectionsParsed(confFile);
    confFile->addedKeys.clear();
    confFile->removedKeys = confFile->originalKeys;
}

void QConfFileSettingsPrivate::sync()
{
    // people probably won't be checking the status a whole lot, so in case of
    // error we just try to go on and make the best of it

    for (int i = 0; i < NumConfFiles; ++i) {
        QConfFile *confFile = confFiles[i].data();
        if (confFile) {
            QMutexLocker locker(&confFile->mutex);
            syncConfFile(i);
        }
    }
}

void QConfFileSettingsPrivate::flush()
{
    sync();
}

QString QConfFileSettingsPrivate::fileName() const
{
    QConfFile *confFile = confFiles[spec].data();
    if (!confFile)
        return QString();
    return confFile->name;
}

bool QConfFileSettingsPrivate::isWritable() const
{
    if (format > QSettings::IniFormat && !writeFunc)
        return false;

    QConfFile *confFile = confFiles[spec].data();
    if (!confFile)
        return false;

    return confFile->isWritable();
}

void QConfFileSettingsPrivate::syncConfFile(int confFileNo)
{
    QConfFile *confFile = confFiles[confFileNo].data();
    bool readOnly = confFile->addedKeys.isEmpty() && confFile->removedKeys.isEmpty();
    bool ok;

    /*
        We can often optimize the read-only case, if the file on disk
        hasn't changed.
    */
    if (readOnly && confFile->size > 0) {
        QFileInfo fileInfo(confFile->name);
        if (confFile->size == fileInfo.size() && confFile->timeStamp == fileInfo.lastModified())
            return;
    }

    /*
        Open the configuration file and try to use it using a named
        semaphore on Windows and an advisory lock on Unix-based
        systems. This protect us against other QSettings instances
        trying to access the same file from other threads or
        processes.

        As it stands now, the locking mechanism doesn't work for
        .plist files.
    */
    QFile file(confFile->name);
    bool createFile = !file.exists();
    if (!readOnly && confFile->isWritable())
        file.open(QFile::ReadWrite);
    if (!file.isOpen())
        file.open(QFile::ReadOnly);

    if (!createFile && !file.isOpen())
        setStatus(QSettings::AccessError);

#ifdef Q_OS_WIN
    HANDLE readSemaphore = 0;
    HANDLE writeSemaphore = 0;
    static const int FileLockSemMax = 50;
    int numReadLocks = readOnly ? 1 : FileLockSemMax;

    if (file.isOpen()) {
        // Acquire the write lock if we will be writing
        if (!readOnly) {
            QString writeSemName = QLatin1String("QSettingsWriteSem ");
            writeSemName.append(file.fileName());

            writeSemaphore = CreateSemaphore(0, 1, 1, reinterpret_cast<const wchar_t *>(writeSemName.utf16()));

            if (writeSemaphore) {
                WaitForSingleObject(writeSemaphore, INFINITE);
            } else {
                setStatus(QSettings::AccessError);
                return;
            }
        }

        // Acquire all the read locks if we will be writing, to make sure nobody
        // reads while we're writing. If we are only reading, acquire a single
        // read lock.
        QString readSemName(QLatin1String("QSettingsReadSem "));
        readSemName.append(file.fileName());

        readSemaphore = CreateSemaphore(0, FileLockSemMax, FileLockSemMax, reinterpret_cast<const wchar_t *>(readSemName.utf16()));

        if (readSemaphore) {
            for (int i = 0; i < numReadLocks; ++i)
                WaitForSingleObject(readSemaphore, INFINITE);
        } else {
            setStatus(QSettings::AccessError);
            if (writeSemaphore != 0) {
                ReleaseSemaphore(writeSemaphore, 1, 0);
                CloseHandle(writeSemaphore);
            }
            return;
        }
    }
#else
    if (file.isOpen())
        unixLock(file.handle(), readOnly ? F_RDLCK : F_WRLCK);
#endif

    // If we have created the file, apply the file perms
    if (file.isOpen()) {
        if (createFile) {
            QFile::Permissions perms = file.permissions() | QFile::ReadOwner | QFile::WriteOwner;
            if (!confFile->userPerms)
                perms |= QFile::ReadGroup | QFile::ReadOther;
            file.setPermissions(perms);
        }
    }

    /*
        We hold the lock. Let's reread the file if it has changed
        since last time we read it.
    */
    QFileInfo fileInfo(confFile->name);
    bool mustReadFile = true;

    if (!readOnly)
        mustReadFile = (confFile->size != fileInfo.size()
                        || (confFile->size != 0 && confFile->timeStamp != fileInfo.lastModified()));

    if (mustReadFile) {
        confFile->unparsedIniSections.clear();
        confFile->originalKeys.clear();

        /*
            Files that we can't read (because of permissions or
            because they don't exist) are treated as empty files.
        */
        if (file.isReadable() && fileInfo.size() != 0) {
#ifdef Q_OS_MAC
            if (format == QSettings::NativeFormat) {
                ok = readPlistFile(confFile->name, &confFile->originalKeys);
            } else
#endif
            {
                if (format <= QSettings::IniFormat) {
                    QByteArray data = file.readAll();
                    ok = readIniFile(data, &confFile->unparsedIniSections);
                } else {
                    if (readFunc) {
                        QSettings::SettingsMap tempNewKeys;
                        ok = readFunc(file, tempNewKeys);

                        if (ok) {
                            QSettings::SettingsMap::const_iterator i = tempNewKeys.constBegin();
                            while (i != tempNewKeys.constEnd()) {
                                confFile->originalKeys.insert(QSettingsKey(i.key(),
                                                                           caseSensitivity),
                                                              i.value());
                                ++i;
                            }
                        }
                    } else {
                        ok = false;
                    }
                }
            }

            if (!ok)
                setStatus(QSettings::FormatError);
        }

        confFile->size = fileInfo.size();
        confFile->timeStamp = fileInfo.lastModified();
    }

    /*
        We also need to save the file. We still hold the file lock,
        so everything is under control.
    */
    if (!readOnly) {
        ensureAllSectionsParsed(confFile);
        ParsedSettingsMap mergedKeys = confFile->mergedKeyMap();

        if (file.isWritable()) {
#ifdef Q_OS_MAC
            if (format == QSettings::NativeFormat) {
                ok = writePlistFile(confFile->name, mergedKeys);
            } else
#endif
            {
                file.seek(0);
                file.resize(0);

                if (format <= QSettings::IniFormat) {
                    ok = writeIniFile(file, mergedKeys);
                    if (!ok) {
                        // try to restore old data; might work if the disk was full and the new data
                        // was larger than the old data
                        file.seek(0);
                        file.resize(0);
                        writeIniFile(file, confFile->originalKeys);
                    }
                } else {
                    if (writeFunc) {
                        QSettings::SettingsMap tempOriginalKeys;

                        ParsedSettingsMap::const_iterator i = mergedKeys.constBegin();
                        while (i != mergedKeys.constEnd()) {
                            tempOriginalKeys.insert(i.key(), i.value());
                            ++i;
                        }
                        ok = writeFunc(file, tempOriginalKeys);
                    } else {
                        ok = false;
                    }
                }
            }
        } else {
            ok = false;
        }

        if (ok) {
            confFile->unparsedIniSections.clear();
            confFile->originalKeys = mergedKeys;
            confFile->addedKeys.clear();
            confFile->removedKeys.clear();

            QFileInfo fileInfo(confFile->name);
            confFile->size = fileInfo.size();
            confFile->timeStamp = fileInfo.lastModified();
        } else {
            setStatus(QSettings::AccessError);
        }
    }

    /*
        Release the file lock.
    */
#ifdef Q_OS_WIN
    if (readSemaphore != 0) {
        ReleaseSemaphore(readSemaphore, numReadLocks, 0);
        CloseHandle(readSemaphore);
    }
    if (writeSemaphore != 0) {
        ReleaseSemaphore(writeSemaphore, 1, 0);
        CloseHandle(writeSemaphore);
    }
#endif
}

enum { Space = 0x1, Special = 0x2 };

static const char charTraits[256] =
{
    // Space: '\t', '\n', '\r', ' '
    // Special: '\n', '\r', '"', ';', '=', '\\'

    0, 0, 0, 0, 0, 0, 0, 0, 0, Space, Space | Special, 0, 0, Space | Special, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    Space, 0, Special, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, Special, 0, Special, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, Special, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

bool QConfFileSettingsPrivate::readIniLine(const QByteArray &data, int &dataPos,
                                           int &lineStart, int &lineLen, int &equalsPos)
{
    int dataLen = data.length();
    bool inQuotes = false;

    equalsPos = -1;

    lineStart = dataPos;
    while (lineStart < dataLen && (charTraits[uint(uchar(data.at(lineStart)))] & Space))
        ++lineStart;

    int i = lineStart;
    while (i < dataLen) {
        while (!(charTraits[uint(uchar(data.at(i)))] & Special)) {
            if (++i == dataLen)
                goto break_out_of_outer_loop;
        }

        char ch = data.at(i++);
        if (ch == '=') {
            if (!inQuotes && equalsPos == -1)
                equalsPos = i - 1;
        } else if (ch == '\n' || ch == '\r') {
            if (i == lineStart + 1) {
                ++lineStart;
            } else if (!inQuotes) {
                --i;
                goto break_out_of_outer_loop;
            }
        } else if (ch == '\\') {
            if (i < dataLen) {
                char ch = data.at(i++);
                if (i < dataLen) {
                    char ch2 = data.at(i);
                    // \n, \r, \r\n, and \n\r are legitimate line terminators in INI files
                    if ((ch == '\n' && ch2 == '\r') || (ch == '\r' && ch2 == '\n'))
                        ++i;
                }
            }
        } else if (ch == '"') {
            inQuotes = !inQuotes;
        } else {
            Q_ASSERT(ch == ';');

            if (i == lineStart + 1) {
                char ch;
                while (i < dataLen && ((ch = data.at(i) != '\n') && ch != '\r'))
                    ++i;
                lineStart = i;
            } else if (!inQuotes) {
                --i;
                goto break_out_of_outer_loop;
            }
        }
    }

break_out_of_outer_loop:
    dataPos = i;
    lineLen = i - lineStart;
    return lineLen > 0;
}

/*
    Returns false on parse error. However, as many keys are read as
    possible, so if the user doesn't check the status he will get the
    most out of the file anyway.
*/
bool QConfFileSettingsPrivate::readIniFile(const QByteArray &data,
                                           UnparsedSettingsMap *unparsedIniSections)
{
#define FLUSH_CURRENT_SECTION() \
    { \
        QByteArray &sectionData = (*unparsedIniSections)[QSettingsKey(currentSection, \
                                                                      IniCaseSensitivity, \
                                                                      sectionPosition)]; \
        if (!sectionData.isEmpty()) \
            sectionData.append('\n'); \
        sectionData += data.mid(currentSectionStart, lineStart - currentSectionStart); \
        sectionPosition = ++position; \
    }

    QString currentSection;
    int currentSectionStart = 0;
    int dataPos = 0;
    int lineStart;
    int lineLen;
    int equalsPos;
    int position = 0;
    int sectionPosition = 0;
    bool ok = true;

    while (readIniLine(data, dataPos, lineStart, lineLen, equalsPos)) {
        char ch = data.at(lineStart);
        if (ch == '[') {
            FLUSH_CURRENT_SECTION();

            // this is a section
            QByteArray iniSection;
            int idx = data.indexOf(']', lineStart);
            if (idx == -1 || idx >= lineStart + lineLen) {
                ok = false;
                iniSection = data.mid(lineStart + 1, lineLen - 1);
            } else {
                iniSection = data.mid(lineStart + 1, idx - lineStart - 1);
            }

            iniSection = iniSection.trimmed();

            if (qstricmp(iniSection, "general") == 0) {
                currentSection.clear();
            } else {
                if (qstricmp(iniSection, "%general") == 0) {
                    currentSection = QLatin1String(iniSection.constData() + 1);
                } else {
                    currentSection.clear();
                    iniUnescapedKey(iniSection, 0, iniSection.size(), currentSection);
                }
                currentSection += QLatin1Char('/');
            }
            currentSectionStart = dataPos;
        }
        ++position;
    }

    Q_ASSERT(lineStart == data.length());
    FLUSH_CURRENT_SECTION();

    return ok;

#undef FLUSH_CURRENT_SECTION
}

bool QConfFileSettingsPrivate::readIniSection(const QSettingsKey &section, const QByteArray &data,
                                              ParsedSettingsMap *settingsMap, QTextCodec *codec)
{
    QStringList strListValue;
    bool sectionIsLowercase = (section == section.originalCaseKey());
    int equalsPos;

    bool ok = true;
    int dataPos = 0;
    int lineStart;
    int lineLen;
    int position = section.originalKeyPosition();

    while (readIniLine(data, dataPos, lineStart, lineLen, equalsPos)) {
        char ch = data.at(lineStart);
        Q_ASSERT(ch != '[');

        if (equalsPos == -1) {
            if (ch != ';')
                ok = false;
            continue;
        }

        int keyEnd = equalsPos;
        while (keyEnd > lineStart && ((ch = data.at(keyEnd - 1)) == ' ' || ch == '\t'))
            --keyEnd;
        int valueStart = equalsPos + 1;

        QString key = section.originalCaseKey();
        bool keyIsLowercase = (iniUnescapedKey(data, lineStart, keyEnd, key) && sectionIsLowercase);

        QString strValue;
        strValue.reserve(lineLen - (valueStart - lineStart));
        bool isStringList = iniUnescapedStringList(data, valueStart, lineStart + lineLen,
                                                   strValue, strListValue, codec);
        QVariant variant;
        if (isStringList) {
            variant = stringListToVariantList(strListValue);
        } else {
            variant = stringToVariant(strValue);
        }

        /*
            We try to avoid the expensive toLower() call in
            QSettingsKey by passing Qt::CaseSensitive when the
            key is already in lowercase.
        */
        settingsMap->insert(QSettingsKey(key, keyIsLowercase ? Qt::CaseSensitive
                                                             : IniCaseSensitivity,
                                         position),
                            variant);
        ++position;
    }

    return ok;
}

class QSettingsIniKey : public QString
{
public:
    inline QSettingsIniKey() : position(-1) {}
    inline QSettingsIniKey(const QString &str, int pos = -1) : QString(str), position(pos) {}

    int position;
};

static bool operator<(const QSettingsIniKey &k1, const QSettingsIniKey &k2)
{
    if (k1.position != k2.position)
        return k1.position < k2.position;
    return static_cast<const QString &>(k1) < static_cast<const QString &>(k2);
}

typedef QMap<QSettingsIniKey, QVariant> IniKeyMap;

struct QSettingsIniSection
{
    int position;
    IniKeyMap keyMap;

    inline QSettingsIniSection() : position(-1) {}
};

typedef QMap<QString, QSettingsIniSection> IniMap;

/*
    This would be more straightforward if we didn't try to remember the original
    key order in the .ini file, but we do.
*/
bool QConfFileSettingsPrivate::writeIniFile(QIODevice &device, const ParsedSettingsMap &map)
{
    IniMap iniMap;
    IniMap::const_iterator i;

#ifdef Q_OS_WIN
    const char * const eol = "\r\n";
#else
    const char eol = '\n';
#endif

    for (ParsedSettingsMap::const_iterator j = map.constBegin(); j != map.constEnd(); ++j) {
        QString section;
        QSettingsIniKey key(j.key().originalCaseKey(), j.key().originalKeyPosition());
        int slashPos;

        if ((slashPos = key.indexOf(QLatin1Char('/'))) != -1) {
            section = key.left(slashPos);
            key.remove(0, slashPos + 1);
        }

        QSettingsIniSection &iniSection = iniMap[section];

        // -1 means infinity
        if (uint(key.position) < uint(iniSection.position))
            iniSection.position = key.position;
        iniSection.keyMap[key] = j.value();
    }

    const int sectionCount = iniMap.size();
    QVector<QSettingsIniKey> sections;
    sections.reserve(sectionCount);
    for (i = iniMap.constBegin(); i != iniMap.constEnd(); ++i)
        sections.append(QSettingsIniKey(i.key(), i.value().position));
    qSort(sections);

    bool writeError = false;
    for (int j = 0; !writeError && j < sectionCount; ++j) {
        i = iniMap.constFind(sections.at(j));
        Q_ASSERT(i != iniMap.constEnd());

        QByteArray realSection;

        iniEscapedKey(i.key(), realSection);

        if (realSection.isEmpty()) {
            realSection = "[General]";
        } else if (qstricmp(realSection, "general") == 0) {
            realSection = "[%General]";
        } else {
            realSection.prepend('[');
            realSection.append(']');
        }

        if (j != 0)
            realSection.prepend(eol);
        realSection += eol;

        device.write(realSection);

        const IniKeyMap &ents = i.value().keyMap;
        for (IniKeyMap::const_iterator j = ents.constBegin(); j != ents.constEnd(); ++j) {
            QByteArray block;
            iniEscapedKey(j.key(), block);
            block += '=';

            const QVariant &value = j.value();

            /*
                The size() != 1 trick is necessary because
                QVariant(QString("foo")).toList() returns an empty
                list, not a list containing "foo".
            */
            if (value.type() == QVariant::StringList
                    || (value.type() == QVariant::List && value.toList().size() != 1)) {
                iniEscapedStringList(variantListToStringList(value.toList()), block, iniCodec);
            } else {
                iniEscapedString(variantToString(value), block, iniCodec);
            }
            block += eol;
            if (device.write(block) == -1) {
                writeError = true;
                break;
            }
        }
    }
    return !writeError;
}

void QConfFileSettingsPrivate::ensureAllSectionsParsed(QConfFile *confFile) const
{
    UnparsedSettingsMap::const_iterator i = confFile->unparsedIniSections.constBegin();
    const UnparsedSettingsMap::const_iterator end = confFile->unparsedIniSections.constEnd();

    for (; i != end; ++i) {
        if (!QConfFileSettingsPrivate::readIniSection(i.key(), i.value(), &confFile->originalKeys, iniCodec))
            setStatus(QSettings::FormatError);
    }
    confFile->unparsedIniSections.clear();
}

void QConfFileSettingsPrivate::ensureSectionParsed(QConfFile *confFile,
                                                   const QSettingsKey &key) const
{
    if (confFile->unparsedIniSections.isEmpty())
        return;

    UnparsedSettingsMap::iterator i;

    int indexOfSlash = key.indexOf(QLatin1Char('/'));
    if (indexOfSlash != -1) {
        i = confFile->unparsedIniSections.upperBound(key);
        if (i == confFile->unparsedIniSections.begin())
            return;
        --i;
        if (i.key().isEmpty() || !key.startsWith(i.key()))
            return;
    } else {
        i = confFile->unparsedIniSections.begin();
        if (i == confFile->unparsedIniSections.end() || !i.key().isEmpty())
            return;
    }

    if (!QConfFileSettingsPrivate::readIniSection(i.key(), i.value(), &confFile->originalKeys, iniCodec))
        setStatus(QSettings::FormatError);
    confFile->unparsedIniSections.erase(i);
}

/*!
    \class QSettings
    \brief The QSettings class provides persistent platform-independent application settings.

    \ingroup io

    \reentrant

    Users normally expect an application to remember its settings
    (window sizes and positions, options, etc.) across sessions. This
    information is often stored in the system registry on Windows,
    and in XML preferences files on Mac OS X. On Unix systems, in the
    absence of a standard, many applications (including the KDE
    applications) use INI text files.

    QSettings is an abstraction around these technologies, enabling
    you to save and restore application settings in a portable
    manner. It also supports \l{registerFormat()}{custom storage
    formats}.

    QSettings's API is based on QVariant, allowing you to save
    most value-based types, such as QString, QRect, and QImage,
    with the minimum of effort.

    If all you need is a non-persistent memory-based structure,
    consider using QMap<QString, QVariant> instead.

    \tableofcontents section1

    \section1 Basic Usage

    When creating a QSettings object, you must pass the name of your
    company or organization as well as the name of your application.
    For example, if your product is called Star Runner and your
    company is called MySoft, you would construct the QSettings
    object as follows:

    \snippet doc/src/snippets/settings/settings.cpp 0

    QSettings objects can be created either on the stack or on
    the heap (i.e. using \c new). Constructing and destroying a
    QSettings object is very fast.

    If you use QSettings from many places in your application, you
    might want to specify the organization name and the application
    name using QCoreApplication::setOrganizationName() and
    QCoreApplication::setApplicationName(), and then use the default
    QSettings constructor:

    \snippet doc/src/snippets/settings/settings.cpp 1
    \snippet doc/src/snippets/settings/settings.cpp 2
    \snippet doc/src/snippets/settings/settings.cpp 3
    \dots
    \snippet doc/src/snippets/settings/settings.cpp 4

    (Here, we also specify the organization's Internet domain. When
    the Internet domain is set, it is used on Mac OS X instead of the
    organization name, since Mac OS X applications conventionally use
    Internet domains to identify themselves. If no domain is set, a
    fake domain is derived from the organization name. See the
    \l{Platform-Specific Notes} below for details.)

    QSettings stores settings. Each setting consists of a QString
    that specifies the setting's name (the \e key) and a QVariant
    that stores the data associated with the key. To write a setting,
    use setValue(). For example:

    \snippet doc/src/snippets/settings/settings.cpp 5

    If there already exists a setting with the same key, the existing
    value is overwritten by the new value. For efficiency, the
    changes may not be saved to permanent storage immediately. (You
    can always call sync() to commit your changes.)

    You can get a setting's value back using value():

    \snippet doc/src/snippets/settings/settings.cpp 6

    If there is no setting with the specified name, QSettings
    returns a null QVariant (which can be converted to the integer 0).
    You can specify another default value by passing a second
    argument to value():

    \snippet doc/src/snippets/settings/settings.cpp 7

    To test whether a given key exists, call contains(). To remove
    the setting associated with a key, call remove(). To obtain the
    list of all keys, call allKeys(). To remove all keys, call
    clear().

    \section1 QVariant and GUI Types

    Because QVariant is part of the \l QtCore library, it cannot provide
    conversion functions to data types such as QColor, QImage, and
    QPixmap, which are part of \l QtGui. In other words, there is no
    \c toColor(), \c toImage(), or \c toPixmap() functions in QVariant.

    Instead, you can use the QVariant::value() or the qVariantValue()
    template function. For example:

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 0

    The inverse conversion (e.g., from QColor to QVariant) is
    automatic for all data types supported by QVariant, including
    GUI-related types:

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 1

    Custom types registered using qRegisterMetaType() and
    qRegisterMetaTypeStreamOperators() can be stored using QSettings.

    \section1 Section and Key Syntax

    Setting keys can contain any Unicode characters. The Windows
    registry and INI files use case-insensitive keys, whereas the
    Carbon Preferences API on Mac OS X uses case-sensitive keys. To
    avoid portability problems, follow these simple rules:

    \list 1
    \o Always refer to the same key using the same case. For example,
       if you refer to a key as "text fonts" in one place in your
       code, don't refer to it as "Text Fonts" somewhere else.

    \o Avoid key names that are identical except for the case. For
       example, if you have a key called "MainWindow", don't try to
       save another key as "mainwindow".

    \o Do not use slashes ('/' and '\\') in section or key names; the
       backslash character is used to separate sub keys (see below). On
       windows '\\' are converted by QSettings to '/', which makes
       them identical.
    \endlist

    You can form hierarchical keys using the '/' character as a
    separator, similar to Unix file paths. For example:

    \snippet doc/src/snippets/settings/settings.cpp 8
    \snippet doc/src/snippets/settings/settings.cpp 9
    \snippet doc/src/snippets/settings/settings.cpp 10

    If you want to save or restore many settings with the same
    prefix, you can specify the prefix using beginGroup() and call
    endGroup() at the end. Here's the same example again, but this
    time using the group mechanism:

    \snippet doc/src/snippets/settings/settings.cpp 11
    \codeline
    \snippet doc/src/snippets/settings/settings.cpp 12

    If a group is set using beginGroup(), the behavior of most
    functions changes consequently. Groups can be set recursively.

    In addition to groups, QSettings also supports an "array"
    concept. See beginReadArray() and beginWriteArray() for details.

    \section1 Fallback Mechanism

    Let's assume that you have created a QSettings object with the
    organization name MySoft and the application name Star Runner.
    When you look up a value, up to four locations are searched in
    that order:

    \list 1
    \o a user-specific location for the Star Runner application
    \o a user-specific location for all applications by MySoft
    \o a system-wide location for the Star Runner application
    \o a system-wide location for all applications by MySoft
    \endlist

    (See \l{Platform-Specific Notes} below for information on what
    these locations are on the different platforms supported by Qt.)

    If a key cannot be found in the first location, the search goes
    on in the second location, and so on. This enables you to store
    system-wide or organization-wide settings and to override them on
    a per-user or per-application basis. To turn off this mechanism,
    call setFallbacksEnabled(false).

    Although keys from all four locations are available for reading,
    only the first file (the user-specific location for the
    application at hand) is accessible for writing. To write to any
    of the other files, omit the application name and/or specify
    QSettings::SystemScope (as opposed to QSettings::UserScope, the
    default).

    Let's see with an example:

    \snippet doc/src/snippets/settings/settings.cpp 13
    \snippet doc/src/snippets/settings/settings.cpp 14

    The table below summarizes which QSettings objects access
    which location. "\bold{X}" means that the location is the main
    location associated to the QSettings object and is used both
    for reading and for writing; "o" means that the location is used
    as a fallback when reading.

    \table
    \header \o Locations               \o \c{obj1} \o \c{obj2} \o \c{obj3} \o \c{obj4}
    \row    \o 1. User, Application    \o \bold{X} \o          \o          \o
    \row    \o 2. User, Organization   \o o        \o \bold{X} \o          \o
    \row    \o 3. System, Application  \o o        \o          \o \bold{X} \o
    \row    \o 4. System, Organization \o o        \o o        \o o        \o \bold{X}
    \endtable

    The beauty of this mechanism is that it works on all platforms
    supported by Qt and that it still gives you a lot of flexibility,
    without requiring you to specify any file names or registry
    paths.

    If you want to use INI files on all platforms instead of the
    native API, you can pass QSettings::IniFormat as the first
    argument to the QSettings constructor, followed by the scope, the
    organization name, and the application name:

    \snippet doc/src/snippets/settings/settings.cpp 15

    The \l{tools/settingseditor}{Settings Editor} example lets you
    experiment with different settings location and with fallbacks
    turned on or off.

    \section1 Restoring the State of a GUI Application

    QSettings is often used to store the state of a GUI
    application. The following example illustrates how to use QSettings
    to save and restore the geometry of an application's main window.

    \snippet doc/src/snippets/settings/settings.cpp 16
    \codeline
    \snippet doc/src/snippets/settings/settings.cpp 17

    See \l{Window Geometry} for a discussion on why it is better to
    call QWidget::resize() and QWidget::move() rather than QWidget::setGeometry()
    to restore a window's geometry.

    The \c readSettings() and \c writeSettings() functions must be
    called from the main window's constructor and close event handler
    as follows:

    \snippet doc/src/snippets/settings/settings.cpp 18
    \dots
    \snippet doc/src/snippets/settings/settings.cpp 19
    \snippet doc/src/snippets/settings/settings.cpp 20
    \codeline
    \snippet doc/src/snippets/settings/settings.cpp 21

    See the \l{mainwindows/application}{Application} example for a
    self-contained example that uses QSettings.

    \section1 Accessing Settings from Multiple Threads or Processes Simultaneously

    QSettings is \l{reentrant}. This means that you can use
    distinct QSettings object in different threads
    simultaneously. This guarantee stands even when the QSettings
    objects refer to the same files on disk (or to the same entries
    in the system registry). If a setting is modified through one
    QSettings object, the change will immediately be visible in
    any other QSettings objects that operate on the same location
    and that live in the same process.

    QSettings can safely be used from different processes (which can
    be different instances of your application running at the same
    time or different applications altogether) to read and write to
    the same system locations. It uses advisory file locking and a
    smart merging algorithm to ensure data integrity. Note that sync()
    imports changes made by other processes (in addition to writing
    the changes from this QSettings).

    \section1 Platform-Specific Notes

    \section2 Locations Where Application Settings Are Stored

    As mentioned in the \l{Fallback Mechanism} section, QSettings
    stores settings for an application in up to four locations,
    depending on whether the settings are user-specific or
    system-wide and whether the settings are application-specific
    or organization-wide. For simplicity, we're assuming the
    organization is called MySoft and the application is called Star
    Runner.

    On Unix systems, if the file format is NativeFormat, the
    following files are used by default:

    \list 1
    \o \c{$HOME/.config/MySoft/Star Runner.conf} (Qt for Embedded Linux: \c{$HOME/Settings/MySoft/Star Runner.conf})
    \o \c{$HOME/.config/MySoft.conf} (Qt for Embedded Linux: \c{$HOME/Settings/MySoft.conf})
    \o \c{/etc/xdg/MySoft/Star Runner.conf}
    \o \c{/etc/xdg/MySoft.conf}
    \endlist

    On Mac OS X versions 10.2 and 10.3, these files are used by
    default:

    \list 1
    \o \c{$HOME/Library/Preferences/com.MySoft.Star Runner.plist}
    \o \c{$HOME/Library/Preferences/com.MySoft.plist}
    \o \c{/Library/Preferences/com.MySoft.Star Runner.plist}
    \o \c{/Library/Preferences/com.MySoft.plist}
    \endlist

    On Windows, NativeFormat settings are stored in the following
    registry paths:

    \list 1
    \o \c{HKEY_CURRENT_USER\Software\MySoft\Star Runner}
    \o \c{HKEY_CURRENT_USER\Software\MySoft}
    \o \c{HKEY_LOCAL_MACHINE\Software\MySoft\Star Runner}
    \o \c{HKEY_LOCAL_MACHINE\Software\MySoft}
    \endlist

    \note On Windows, for 32-bit programs running in WOW64 mode, settings are
    stored in the following registry path:
    \c{HKEY_LOCAL_MACHINE\Software\WOW6432node}.

    If the file format is IniFormat, the following files are
    used on Unix and Mac OS X:

    \list 1
    \o \c{$HOME/.config/MySoft/Star Runner.ini} (Qt for Embedded Linux: \c{$HOME/Settings/MySoft/Star Runner.ini})
    \o \c{$HOME/.config/MySoft.ini} (Qt for Embedded Linux: \c{$HOME/Settings/MySoft.ini})
    \o \c{/etc/xdg/MySoft/Star Runner.ini}
    \o \c{/etc/xdg/MySoft.ini}
    \endlist

    On Windows, the following files are used:

    \list 1
    \o \c{%APPDATA%\MySoft\Star Runner.ini}
    \o \c{%APPDATA%\MySoft.ini}
    \o \c{%COMMON_APPDATA%\MySoft\Star Runner.ini}
    \o \c{%COMMON_APPDATA%\MySoft.ini}
    \endlist

    The \c %APPDATA% path is usually \tt{C:\\Documents and
    Settings\\\e{User Name}\\Application Data}; the \c
    %COMMON_APPDATA% path is usually \tt{C:\\Documents and
    Settings\\All Users\\Application Data}.

    On Symbian, the following files are used for both IniFormat and
    NativeFormat (in this example, we assume that the application is
    installed on the \c e-drive and its Secure ID is \c{0xECB00931}):

    \list 1
    \o \c{c:\data\.config\MySoft\Star Runner.conf}
    \o \c{c:\data\.config\MySoft.conf}
    \o \c{e:\private\ecb00931\MySoft\Star Runner.conf}
    \o \c{e:\private\ecb00931\MySoft.conf}
    \endlist

    The SystemScope settings location is determined from the installation
    drive and Secure ID (UID3) of the application. If the application is
    built-in on the ROM, the drive used for SystemScope is \c c:.

    \note Symbian SystemScope settings are by default private to the
    application and not shared between applications, unlike other
    environments.

    The paths for the \c .ini and \c .conf files can be changed using
    setPath(). On Unix and Mac OS X, the user can override them by by
    setting the \c XDG_CONFIG_HOME environment variable; see
    setPath() for details.

    \section2 Accessing INI and .plist Files Directly

    Sometimes you do want to access settings stored in a specific
    file or registry path. On all platforms, if you want to read an
    INI file directly, you can use the QSettings constructor that
    takes a file name as first argument and pass QSettings::IniFormat
    as second argument. For example:

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 2

    You can then use the QSettings object to read and write settings
    in the file.

    On Mac OS X, you can access XML-based \c .plist files by passing
    QSettings::NativeFormat as second argument. For example:

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 3

    \section2 Accessing the Windows Registry Directly

    On Windows, QSettings lets you access settings that have been
    written with QSettings (or settings in a supported format, e.g., string
    data) in the system registry. This is done by constructing a QSettings
    object with a path in the registry and QSettings::NativeFormat.

    For example:

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 4

    All the registry entries that appear under the specified path can
    be read or written through the QSettings object as usual (using
    forward slashes instead of backslashes). For example:

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 5

    Note that the backslash character is, as mentioned, used by
    QSettings to separate subkeys. As a result, you cannot read or
    write windows registry entries that contain slashes or
    backslashes; you should use a native windows API if you need to do
    so.

    \section2 Accessing Common Registry Settings on Windows

    On Windows, it is possible for a key to have both a value and subkeys.
    Its default value is accessed by using "Default" or "." in
    place of a subkey:

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 6

    On other platforms than Windows, "Default" and "." would be
    treated as regular subkeys.

    \section2 Securing application settings in Symbian

    UserScope settings in Symbian are writable by any application by
    default. To protect the application settings from access and tampering
    by other applications, the settings need to be placed in the private
    secure area of the application. This can be done by specifying the
    settings storage path directly to the private area. The following
    snippet changes the UserScope to \c{c:/private/ecb00931/MySoft.conf}
    (provided the application is installed on the \c{c-drive} and its
    Secure ID is \c{0xECB00931}:

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 30

    Framework libraries (like Qt itself) may store configuration and cache
    settings using UserScope, which is accessible and writable by other
    applications. If the application is very security sensitive or uses
    high platform security capabilities, it may be prudent to also force
    framework settings to be stored in the private directory of the
    application. This can be done by changing the default path of UserScope
    before QApplication is created:

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 31

    Note that this may affect framework libraries' functionality if they expect
    the settings to be shared between applications.

    \section2 Changing the location of global Qt settings on Mac OS X

    On Mac OS X, the global Qt settings (stored in \c com.trolltech.plist)
    are stored in the application settings file in two situations:

    \list 1
    \o If the application runs in a Mac OS X sandbox (on Mac OS X 10.7 or later) or
    \o If the \c Info.plist file of the application contains the key \c "ForAppStore" with the value \c "yes"
    \endlist

    In these situations, the application settings file is named using
    the bundle identifier of the application, which must consequently
    be set in the application's \c Info.plist file.

    This feature is provided to ease the acceptance of Qt applications into
    the Mac App Store, as the default behaviour of storing global Qt
    settings in the \c com.trolltech.plist file does not conform with Mac
    App Store file system usage requirements. For more information
    about submitting Qt applications to the Mac App Store, see
    \l{mac-differences.html#Preparing a Qt application for Mac App Store submission}{Preparing a Qt application for Mac App Store submission}.

    \section2 Platform Limitations

    While QSettings attempts to smooth over the differences between
    the different supported platforms, there are still a few
    differences that you should be aware of when porting your
    application:

    \list
    \o  The Windows system registry has the following limitations: A
        subkey may not exceed 255 characters, an entry's value may
        not exceed 16,383 characters, and all the values of a key may
        not exceed 65,535 characters. One way to work around these
        limitations is to store the settings using the IniFormat
        instead of the NativeFormat.

    \o  On Mac OS X, allKeys() will return some extra keys for global
        settings that apply to all applications. These keys can be
        read using value() but cannot be changed, only shadowed.
        Calling setFallbacksEnabled(false) will hide these global
        settings.

    \o  On Mac OS X, the CFPreferences API used by QSettings expects
        Internet domain names rather than organization names. To
        provide a uniform API, QSettings derives a fake domain name
        from the organization name (unless the organization name
        already is a domain name, e.g. OpenOffice.org). The algorithm
        appends ".com" to the company name and replaces spaces and
        other illegal characters with hyphens. If you want to specify
        a different domain name, call
        QCoreApplication::setOrganizationDomain(),
        QCoreApplication::setOrganizationName(), and
        QCoreApplication::setApplicationName() in your \c main()
        function and then use the default QSettings constructor.
        Another solution is to use preprocessor directives, for
        example:

        \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 7

    \o On Unix and Mac OS X systems, the advisory file locking is disabled
       if NFS (or AutoFS or CacheFS) is detected to work around a bug in the
       NFS fcntl() implementation, which hangs forever if statd or lockd aren't
       running. Also, the locking isn't performed when accessing \c .plist
       files.

    \endlist

    \sa QVariant, QSessionManager, {Settings Editor Example}, {Application Example}
*/

/*! \enum QSettings::Status

    The following status values are possible:

    \value NoError  No error occurred.
    \value AccessError  An access error occurred (e.g. trying to write to a read-only file).
    \value FormatError  A format error occurred (e.g. loading a malformed INI file).

    \sa status()
*/

/*! \enum QSettings::Format

    This enum type specifies the storage format used by QSettings.

    \value NativeFormat  Store the settings using the most
                         appropriate storage format for the platform.
                         On Windows, this means the system registry;
                         on Mac OS X, this means the CFPreferences
                         API; on Unix, this means textual
                         configuration files in INI format.
    \value IniFormat  Store the settings in INI files.
    \value InvalidFormat Special value returned by registerFormat().
    \omitvalue CustomFormat1
    \omitvalue CustomFormat2
    \omitvalue CustomFormat3
    \omitvalue CustomFormat4
    \omitvalue CustomFormat5
    \omitvalue CustomFormat6
    \omitvalue CustomFormat7
    \omitvalue CustomFormat8
    \omitvalue CustomFormat9
    \omitvalue CustomFormat10
    \omitvalue CustomFormat11
    \omitvalue CustomFormat12
    \omitvalue CustomFormat13
    \omitvalue CustomFormat14
    \omitvalue CustomFormat15
    \omitvalue CustomFormat16

    On Unix, NativeFormat and IniFormat mean the same thing, except
    that the file extension is different (\c .conf for NativeFormat,
    \c .ini for IniFormat).

    The INI file format is a Windows file format that Qt supports on
    all platforms. In the absence of an INI standard, we try to
    follow what Microsoft does, with the following exceptions:

    \list
    \o  If you store types that QVariant can't convert to QString
        (e.g., QPoint, QRect, and QSize), Qt uses an \c{@}-based
        syntax to encode the type. For example:

        \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 8

        To minimize compatibility issues, any \c @ that doesn't
        appear at the first position in the value or that isn't
        followed by a Qt type (\c Point, \c Rect, \c Size, etc.) is
        treated as a normal character.

    \o  Although backslash is a special character in INI files, most
        Windows applications don't escape backslashes (\c{\}) in file
        paths:

        \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 9

        QSettings always treats backslash as a special character and
        provides no API for reading or writing such entries.

    \o  The INI file format has severe restrictions on the syntax of
        a key. Qt works around this by using \c % as an escape
        character in keys. In addition, if you save a top-level
        setting (a key with no slashes in it, e.g., "someKey"), it
        will appear in the INI file's "General" section. To avoid
        overwriting other keys, if you save something using the a key
        such as "General/someKey", the key will be located in the
        "%General" section, \e not in the "General" section.

    \o  Following the philosophy that we should be liberal in what
        we accept and conservative in what we generate, QSettings
        will accept Latin-1 encoded INI files, but generate pure
        ASCII files, where non-ASCII values are encoded using standard
        INI escape sequences. To make the INI files more readable (but
        potentially less compatible), call setIniCodec().
    \endlist

    \sa registerFormat(), setPath()
*/

/*! \enum QSettings::Scope

    This enum specifies whether settings are user-specific or shared
    by all users of the same system.

    \value UserScope  Store settings in a location specific to the
                      current user (e.g., in the user's home
                      directory).
    \value SystemScope  Store settings in a global location, so that
                        all users on the same machine access the same
                        set of settings.
    \omitvalue User
    \omitvalue Global

    \sa setPath()
*/

#ifndef QT_NO_QOBJECT
/*!
    Constructs a QSettings object for accessing settings of the
    application called \a application from the organization called \a
    organization, and with parent \a parent.

    Example:
    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 10

    The scope is set to QSettings::UserScope, and the format is
    set to QSettings::NativeFormat (i.e. calling setDefaultFormat()
    before calling this constructor has no effect).

    \sa setDefaultFormat(), {Fallback Mechanism}
*/
QSettings::QSettings(const QString &organization, const QString &application, QObject *parent)
    : QObject(*QSettingsPrivate::create(NativeFormat, UserScope, organization, application),
              parent)
{
}

/*!
    Constructs a QSettings object for accessing settings of the
    application called \a application from the organization called \a
    organization, and with parent \a parent.

    If \a scope is QSettings::UserScope, the QSettings object searches
    user-specific settings first, before it searches system-wide
    settings as a fallback. If \a scope is QSettings::SystemScope, the
    QSettings object ignores user-specific settings and provides
    access to system-wide settings.

    The storage format is set to QSettings::NativeFormat (i.e. calling
    setDefaultFormat() before calling this constructor has no effect).

    If no application name is given, the QSettings object will
    only access the organization-wide \l{Fallback Mechanism}{locations}.

    \sa setDefaultFormat()
*/
QSettings::QSettings(Scope scope, const QString &organization, const QString &application,
                     QObject *parent)
    : QObject(*QSettingsPrivate::create(NativeFormat, scope, organization, application), parent)
{
}

/*!
    Constructs a QSettings object for accessing settings of the
    application called \a application from the organization called
    \a organization, and with parent \a parent.

    If \a scope is QSettings::UserScope, the QSettings object searches
    user-specific settings first, before it searches system-wide
    settings as a fallback. If \a scope is
    QSettings::SystemScope, the QSettings object ignores user-specific
    settings and provides access to system-wide settings.

    If \a format is QSettings::NativeFormat, the native API is used for
    storing settings. If \a format is QSettings::IniFormat, the INI format
    is used.

    If no application name is given, the QSettings object will
    only access the organization-wide \l{Fallback Mechanism}{locations}.
*/
QSettings::QSettings(Format format, Scope scope, const QString &organization,
                     const QString &application, QObject *parent)
    : QObject(*QSettingsPrivate::create(format, scope, organization, application), parent)
{
}

/*!
    Constructs a QSettings object for accessing the settings
    stored in the file called \a fileName, with parent \a parent. If
    the file doesn't already exist, it is created.

    If \a format is QSettings::NativeFormat, the meaning of \a
    fileName depends on the platform. On Unix, \a fileName is the
    name of an INI file. On Mac OS X, \a fileName is the name of a
    \c .plist file. On Windows, \a fileName is a path in the system
    registry.

    If \a format is QSettings::IniFormat, \a fileName is the name of an INI
    file.

    \warning This function is provided for convenience. It works well for
    accessing INI or \c .plist files generated by Qt, but might fail on some
    syntaxes found in such files originated by other programs. In particular,
    be aware of the following limitations:

    \list
    \o QSettings provides no way of reading INI "path" entries, i.e., entries
       with unescaped slash characters. (This is because these entries are
       ambiguous and cannot be resolved automatically.)
    \o In INI files, QSettings uses the \c @ character as a metacharacter in some
       contexts, to encode Qt-specific data types (e.g., \c @Rect), and might
       therefore misinterpret it when it occurs in pure INI files.
    \endlist

    \sa fileName()
*/
QSettings::QSettings(const QString &fileName, Format format, QObject *parent)
    : QObject(*QSettingsPrivate::create(fileName, format), parent)
{
}

/*!
    Constructs a QSettings object for accessing settings of the
    application and organization set previously with a call to
    QCoreApplication::setOrganizationName(),
    QCoreApplication::setOrganizationDomain(), and
    QCoreApplication::setApplicationName().

    The scope is QSettings::UserScope and the format is
    defaultFormat() (QSettings::NativeFormat by default).
    Use setDefaultFormat() before calling this constructor
    to change the default format used by this constructor.

    The code

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 11

    is equivalent to

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 12

    If QCoreApplication::setOrganizationName() and
    QCoreApplication::setApplicationName() has not been previously
    called, the QSettings object will not be able to read or write
    any settings, and status() will return AccessError.

    On Mac OS X, if both a name and an Internet domain are specified
    for the organization, the domain is preferred over the name. On
    other platforms, the name is preferred over the domain.

    \sa QCoreApplication::setOrganizationName(),
        QCoreApplication::setOrganizationDomain(),
        QCoreApplication::setApplicationName(),
        setDefaultFormat()
*/
QSettings::QSettings(QObject *parent)
    : QObject(*QSettingsPrivate::create(globalDefaultFormat, UserScope,
#ifdef Q_OS_MAC
                                        QCoreApplication::organizationDomain().isEmpty()
                                            ? QCoreApplication::organizationName()
                                            : QCoreApplication::organizationDomain()
#else
                                        QCoreApplication::organizationName().isEmpty()
                                            ? QCoreApplication::organizationDomain()
                                            : QCoreApplication::organizationName()
#endif
                                        , QCoreApplication::applicationName()),
              parent)
{
}

#else
QSettings::QSettings(const QString &organization, const QString &application)
    : d_ptr(QSettingsPrivate::create(globalDefaultFormat, QSettings::UserScope, organization, application))
{
    d_ptr->q_ptr = this;
}

QSettings::QSettings(Scope scope, const QString &organization, const QString &application)
    : d_ptr(QSettingsPrivate::create(globalDefaultFormat, scope, organization, application))
{
    d_ptr->q_ptr = this;
}

QSettings::QSettings(Format format, Scope scope, const QString &organization,
                     const QString &application)
    : d_ptr(QSettingsPrivate::create(format, scope, organization, application))
{
    d_ptr->q_ptr = this;
}

QSettings::QSettings(const QString &fileName, Format format)
    : d_ptr(QSettingsPrivate::create(fileName, format))
{
    d_ptr->q_ptr = this;
}
#endif

/*!
    Destroys the QSettings object.

    Any unsaved changes will eventually be written to permanent
    storage.

    \sa sync()
*/
QSettings::~QSettings()
{
    Q_D(QSettings);
    if (d->pendingChanges) {
        QT_TRY {
            d->flush();
        } QT_CATCH(...) {
            ; // ok. then don't flush but at least don't throw in the destructor
        }
    }
}

/*!
    Removes all entries in the primary location associated to this
    QSettings object.

    Entries in fallback locations are not removed.

    If you only want to remove the entries in the current group(),
    use remove("") instead.

    \sa remove(), setFallbacksEnabled()
*/
void QSettings::clear()
{
    Q_D(QSettings);
    d->clear();
    d->requestUpdate();
}

/*!
    Writes any unsaved changes to permanent storage, and reloads any
    settings that have been changed in the meantime by another
    application.

    This function is called automatically from QSettings's destructor and
    by the event loop at regular intervals, so you normally don't need to
    call it yourself.

    \sa status()
*/
void QSettings::sync()
{
    Q_D(QSettings);
    d->sync();
}

/*!
    Returns the path where settings written using this QSettings
    object are stored.

    On Windows, if the format is QSettings::NativeFormat, the return value
    is a system registry path, not a file path.

    \sa isWritable(), format()
*/
QString QSettings::fileName() const
{
    Q_D(const QSettings);
    return d->fileName();
}

/*!
    \since 4.4

    Returns the format used for storing the settings.

    \sa defaultFormat(), fileName(), scope(), organizationName(), applicationName()
*/
QSettings::Format QSettings::format() const
{
    Q_D(const QSettings);
    return d->format;
}

/*!
    \since 4.4

    Returns the scope used for storing the settings.

    \sa format(), organizationName(), applicationName()
*/
QSettings::Scope QSettings::scope() const
{
    Q_D(const QSettings);
    return d->scope;
}

/*!
    \since 4.4

    Returns the organization name used for storing the settings.

    \sa QCoreApplication::organizationName(), format(), scope(), applicationName()
*/
QString QSettings::organizationName() const
{
    Q_D(const QSettings);
    return d->organizationName;
}

/*!
    \since 4.4

    Returns the application name used for storing the settings.

    \sa QCoreApplication::applicationName(), format(), scope(), organizationName()
*/
QString QSettings::applicationName() const
{
    Q_D(const QSettings);
    return d->applicationName;
}

#ifndef QT_NO_TEXTCODEC

/*!
    \since 4.5

    Sets the codec for accessing INI files (including \c .conf files on Unix)
    to \a codec. The codec is used for decoding any data that is read from
    the INI file, and for encoding any data that is written to the file. By
    default, no codec is used, and non-ASCII characters are encoded using
    standard INI escape sequences.

    \warning The codec must be set immediately after creating the QSettings
    object, before accessing any data.

    \sa iniCodec()
*/
void QSettings::setIniCodec(QTextCodec *codec)
{
    Q_D(QSettings);
    d->iniCodec = codec;
}

/*!
    \since 4.5
    \overload

    Sets the codec for accessing INI files (including \c .conf files on Unix)
    to the QTextCodec for the encoding specified by \a codecName. Common
    values for \c codecName include "ISO 8859-1", "UTF-8", and "UTF-16".
    If the encoding isn't recognized, nothing happens.

    \sa QTextCodec::codecForName()
*/
void QSettings::setIniCodec(const char *codecName)
{
    Q_D(QSettings);
    if (QTextCodec *codec = QTextCodec::codecForName(codecName))
        d->iniCodec = codec;
}

/*!
    \since 4.5

    Returns the codec that is used for accessing INI files. By default,
    no codec is used, so a null pointer is returned.
*/

QTextCodec *QSettings::iniCodec() const
{
    Q_D(const QSettings);
    return d->iniCodec;
}

#endif // QT_NO_TEXTCODEC

/*!
    Returns a status code indicating the first error that was met by
    QSettings, or QSettings::NoError if no error occurred.

    Be aware that QSettings delays performing some operations. For this
    reason, you might want to call sync() to ensure that the data stored
    in QSettings is written to disk before calling status().

    \sa sync()
*/
QSettings::Status QSettings::status() const
{
    Q_D(const QSettings);
    return d->status;
}

/*!
    Appends \a prefix to the current group.

    The current group is automatically prepended to all keys
    specified to QSettings. In addition, query functions such as
    childGroups(), childKeys(), and allKeys() are based on the group.
    By default, no group is set.

    Groups are useful to avoid typing in the same setting paths over
    and over. For example:

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 13

    This will set the value of three settings:

    \list
    \o \c mainwindow/size
    \o \c mainwindow/fullScreen
    \o \c outputpanel/visible
    \endlist

    Call endGroup() to reset the current group to what it was before
    the corresponding beginGroup() call. Groups can be nested.

    \sa endGroup(), group()
*/
void QSettings::beginGroup(const QString &prefix)
{
    Q_D(QSettings);
    d->beginGroupOrArray(QSettingsGroup(d->normalizedKey(prefix)));
}

/*!
    Resets the group to what it was before the corresponding
    beginGroup() call.

    Example:

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 14

    \sa beginGroup(), group()
*/
void QSettings::endGroup()
{
    Q_D(QSettings);
    if (d->groupStack.isEmpty()) {
        qWarning("QSettings::endGroup: No matching beginGroup()");
        return;
    }

    QSettingsGroup group = d->groupStack.pop();
    int len = group.toString().size();
    if (len > 0)
        d->groupPrefix.truncate(d->groupPrefix.size() - (len + 1));

    if (group.isArray())
        qWarning("QSettings::endGroup: Expected endArray() instead");
}

/*!
    Returns the current group.

    \sa beginGroup(), endGroup()
*/
QString QSettings::group() const
{
    Q_D(const QSettings);
    return d->groupPrefix.left(d->groupPrefix.size() - 1);
}

/*!
    Adds \a prefix to the current group and starts reading from an
    array. Returns the size of the array.

    Example:

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 15

    Use beginWriteArray() to write the array in the first place.

    \sa beginWriteArray(), endArray(), setArrayIndex()
*/
int QSettings::beginReadArray(const QString &prefix)
{
    Q_D(QSettings);
    d->beginGroupOrArray(QSettingsGroup(d->normalizedKey(prefix), false));
    return value(QLatin1String("size")).toInt();
}

/*!
    Adds \a prefix to the current group and starts writing an array
    of size \a size. If \a size is -1 (the default), it is automatically
    determined based on the indexes of the entries written.

    If you have many occurrences of a certain set of keys, you can
    use arrays to make your life easier. For example, let's suppose
    that you want to save a variable-length list of user names and
    passwords. You could then write:

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 16

    The generated keys will have the form

    \list
    \o \c logins/size
    \o \c logins/1/userName
    \o \c logins/1/password
    \o \c logins/2/userName
    \o \c logins/2/password
    \o \c logins/3/userName
    \o \c logins/3/password
    \o ...
    \endlist

    To read back an array, use beginReadArray().

    \sa beginReadArray(), endArray(), setArrayIndex()
*/
void QSettings::beginWriteArray(const QString &prefix, int size)
{
    Q_D(QSettings);
    d->beginGroupOrArray(QSettingsGroup(d->normalizedKey(prefix), size < 0));

    if (size < 0)
        remove(QLatin1String("size"));
    else
        setValue(QLatin1String("size"), size);
}

/*!
    Closes the array that was started using beginReadArray() or
    beginWriteArray().

    \sa beginReadArray(), beginWriteArray()
*/
void QSettings::endArray()
{
    Q_D(QSettings);
    if (d->groupStack.isEmpty()) {
        qWarning("QSettings::endArray: No matching beginArray()");
        return;
    }

    QSettingsGroup group = d->groupStack.top();
    int len = group.toString().size();
    d->groupStack.pop();
    if (len > 0)
        d->groupPrefix.truncate(d->groupPrefix.size() - (len + 1));

    if (group.arraySizeGuess() != -1)
        setValue(group.name() + QLatin1String("/size"), group.arraySizeGuess());

    if (!group.isArray())
        qWarning("QSettings::endArray: Expected endGroup() instead");
}

/*!
    Sets the current array index to \a i. Calls to functions such as
    setValue(), value(), remove(), and contains() will operate on the
    array entry at that index.

    You must call beginReadArray() or beginWriteArray() before you
    can call this function.
*/
void QSettings::setArrayIndex(int i)
{
    Q_D(QSettings);
    if (d->groupStack.isEmpty() || !d->groupStack.top().isArray()) {
        qWarning("QSettings::setArrayIndex: Missing beginArray()");
        return;
    }

    QSettingsGroup &top = d->groupStack.top();
    int len = top.toString().size();
    top.setArrayIndex(qMax(i, 0));
    d->groupPrefix.replace(d->groupPrefix.size() - len - 1, len, top.toString());
}

/*!
    Returns a list of all keys, including subkeys, that can be read
    using the QSettings object.

    Example:

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 17

    If a group is set using beginGroup(), only the keys in the group
    are returned, without the group prefix:

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 18

    \sa childGroups(), childKeys()
*/
QStringList QSettings::allKeys() const
{
    Q_D(const QSettings);
    return d->children(d->groupPrefix, QSettingsPrivate::AllKeys);
}

/*!
    Returns a list of all top-level keys that can be read using the
    QSettings object.

    Example:

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 19

    If a group is set using beginGroup(), the top-level keys in that
    group are returned, without the group prefix:

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 20

    You can navigate through the entire setting hierarchy using
    childKeys() and childGroups() recursively.

    \sa childGroups(), allKeys()
*/
QStringList QSettings::childKeys() const
{
    Q_D(const QSettings);
    return d->children(d->groupPrefix, QSettingsPrivate::ChildKeys);
}

/*!
    Returns a list of all key top-level groups that contain keys that
    can be read using the QSettings object.

    Example:

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 21

    If a group is set using beginGroup(), the first-level keys in
    that group are returned, without the group prefix.

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 22

    You can navigate through the entire setting hierarchy using
    childKeys() and childGroups() recursively.

    \sa childKeys(), allKeys()
*/
QStringList QSettings::childGroups() const
{
    Q_D(const QSettings);
    return d->children(d->groupPrefix, QSettingsPrivate::ChildGroups);
}

/*!
    Returns true if settings can be written using this QSettings
    object; returns false otherwise.

    One reason why isWritable() might return false is if
    QSettings operates on a read-only file.

    \warning This function is not perfectly reliable, because the
    file permissions can change at any time.

    \sa fileName(), status(), sync()
*/
bool QSettings::isWritable() const
{
    Q_D(const QSettings);
    return d->isWritable();
}

/*!
  
  Sets the value of setting \a key to \a value. If the \a key already
  exists, the previous value is overwritten.

  Note that the Windows registry and INI files use case-insensitive
  keys, whereas the Carbon Preferences API on Mac OS X uses
  case-sensitive keys. To avoid portability problems, see the
  \l{Section and Key Syntax} rules.

  Example:

  \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 23

  \sa value(), remove(), contains()
*/
void QSettings::setValue(const QString &key, const QVariant &value)
{
    Q_D(QSettings);
    QString k = d->actualKey(key);
    d->set(k, value);
    d->requestUpdate();
}

/*!
    Removes the setting \a key and any sub-settings of \a key.

    Example:

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 24

    Be aware that if one of the fallback locations contains a setting
    with the same key, that setting will be visible after calling
    remove().

    If \a key is an empty string, all keys in the current group() are
    removed. For example:

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 25

    Note that the Windows registry and INI files use case-insensitive
    keys, whereas the Carbon Preferences API on Mac OS X uses
    case-sensitive keys. To avoid portability problems, see the
    \l{Section and Key Syntax} rules.

    \sa setValue(), value(), contains()
*/
void QSettings::remove(const QString &key)
{
    Q_D(QSettings);
    /*
        We cannot use actualKey(), because remove() supports empty
        keys. The code is also tricky because of slash handling.
    */
    QString theKey = d->normalizedKey(key);
    if (theKey.isEmpty())
        theKey = group();
    else
        theKey.prepend(d->groupPrefix);

    if (theKey.isEmpty()) {
        d->clear();
    } else {
        d->remove(theKey);
    }
    d->requestUpdate();
}

/*!
    Returns true if there exists a setting called \a key; returns
    false otherwise.

    If a group is set using beginGroup(), \a key is taken to be
    relative to that group.

    Note that the Windows registry and INI files use case-insensitive
    keys, whereas the Carbon Preferences API on Mac OS X uses
    case-sensitive keys. To avoid portability problems, see the
    \l{Section and Key Syntax} rules.

    \sa value(), setValue()
*/
bool QSettings::contains(const QString &key) const
{
    Q_D(const QSettings);
    QString k = d->actualKey(key);
    return d->get(k, 0);
}

/*!
    Sets whether fallbacks are enabled to \a b.

    By default, fallbacks are enabled.

    \sa fallbacksEnabled()
*/
void QSettings::setFallbacksEnabled(bool b)
{
    Q_D(QSettings);
    d->fallbacks = !!b;
}

/*!
    Returns true if fallbacks are enabled; returns false otherwise.

    By default, fallbacks are enabled.

    \sa setFallbacksEnabled()
*/
bool QSettings::fallbacksEnabled() const
{
    Q_D(const QSettings);
    return d->fallbacks;
}

#ifndef QT_NO_QOBJECT
/*!
    \reimp
*/
bool QSettings::event(QEvent *event)
{
    Q_D(QSettings);
    if (event->type() == QEvent::UpdateRequest) {
        d->update();
        return true;
    }
    return QObject::event(event);
}
#endif

/*!
    Returns the value for setting \a key. If the setting doesn't
    exist, returns \a defaultValue.

    If no default value is specified, a default QVariant is
    returned.

    Note that the Windows registry and INI files use case-insensitive
    keys, whereas the Carbon Preferences API on Mac OS X uses
    case-sensitive keys. To avoid portability problems, see the
    \l{Section and Key Syntax} rules.

    Example:

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 26

    \sa setValue(), contains(), remove()
*/
QVariant QSettings::value(const QString &key, const QVariant &defaultValue) const
{
    Q_D(const QSettings);
    QVariant result = defaultValue;
    QString k = d->actualKey(key);
    d->get(k, &result);
    return result;
}

/*!
    \since 4.4

    Sets the default file format to the given \a format, which is used
    for storing settings for the QSettings(QObject *) constructor.

    If no default format is set, QSettings::NativeFormat is used. See
    the documentation for the QSettings constructor you are using to
    see if that constructor will ignore this function.

    \sa format()
*/
void QSettings::setDefaultFormat(Format format)
{
    globalDefaultFormat = format;
}

/*!
    \since 4.4

    Returns default file format used for storing settings for the QSettings(QObject *) constructor.
    If no default format is set, QSettings::NativeFormat is used.

    \sa format()
*/
QSettings::Format QSettings::defaultFormat()
{
    return globalDefaultFormat;
}

/*!
    \obsolete

    Use setPath() instead.

    \oldcode
        setSystemIniPath(path);
    \newcode
        setPath(QSettings::NativeFormat, QSettings::SystemScope, path);
        setPath(QSettings::IniFormat, QSettings::SystemScope, path);
    \endcode
*/
void QSettings::setSystemIniPath(const QString &dir)
{
    setPath(IniFormat, SystemScope, dir);
#if !defined(Q_OS_WIN) && !defined(Q_OS_MAC)
    setPath(NativeFormat, SystemScope, dir);
#endif
}

/*!
    \obsolete

    Use setPath() instead.
*/

void QSettings::setUserIniPath(const QString &dir)
{
    setPath(IniFormat, UserScope, dir);
#if !defined(Q_OS_WIN) && !defined(Q_OS_MAC)
    setPath(NativeFormat, UserScope, dir);
#endif
}

/*!
    \since 4.1

    Sets the path used for storing settings for the given \a format
    and \a scope, to \a path. The \a format can be a custom format.

    The table below summarizes the default values:

    \table
    \header \o Platform         \o Format                       \o Scope       \o Path
    \row    \o{1,2} Windows     \o{1,2} IniFormat               \o UserScope   \o \c %APPDATA%
    \row                                                        \o SystemScope \o \c %COMMON_APPDATA%
    \row    \o{1,2} Unix        \o{1,2} NativeFormat, IniFormat \o UserScope   \o \c $HOME/.config
    \row                                                        \o SystemScope \o \c /etc/xdg
    \row    \o{1,2} Qt for Embedded Linux \o{1,2} NativeFormat, IniFormat \o UserScope   \o \c $HOME/Settings
    \row                                                        \o SystemScope \o \c /etc/xdg
    \row    \o{1,2} Mac OS X    \o{1,2} IniFormat               \o UserScope   \o \c $HOME/.config
    \row                                                        \o SystemScope \o \c /etc/xdg
    \row    \o{1,2} Symbian     \o{1,2} NativeFormat, IniFormat \o UserScope   \o \c c:/data/.config
    \row                                                        \o SystemScope \o \c <drive>/private/<uid>
    \endtable

    The default UserScope paths on Unix and Mac OS X (\c
    $HOME/.config or $HOME/Settings) can be overridden by the user by setting the
    \c XDG_CONFIG_HOME environment variable. The default SystemScope
    paths on Unix and Mac OS X (\c /etc/xdg) can be overridden when
    building the Qt library using the \c configure script's \c
    --sysconfdir flag (see QLibraryInfo for details).

    Setting the NativeFormat paths on Windows and Mac OS X has no
    effect.

    \warning This function doesn't affect existing QSettings objects.

    \sa registerFormat()
*/
void QSettings::setPath(Format format, Scope scope, const QString &path)
{
    QMutexLocker locker(globalMutex());
    PathHash *pathHash = pathHashFunc();
    if (pathHash->isEmpty())
        initDefaultPaths(&locker);
    pathHash->insert(pathHashKey(format, scope), path + QDir::separator());
}

/*!
    \typedef QSettings::SettingsMap

    Typedef for QMap<QString, QVariant>.
	
    \sa registerFormat()
*/

/*!
    \typedef QSettings::ReadFunc

    Typedef for a pointer to a function with the following signature:

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 27

    \c ReadFunc is used in \c registerFormat() as a pointer to a function
    that reads a set of key/value pairs. \c ReadFunc should read all the 
    options in one pass, and return all the settings in the \c SettingsMap 
    container, which is initially empty.

    \sa WriteFunc, registerFormat()
*/

/*!
    \typedef QSettings::WriteFunc

    Typedef for a pointer to a function with the following signature:

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 28

    \c WriteFunc is used in \c registerFormat() as a pointer to a function 
    that writes a set of key/value pairs. \c WriteFunc is only called once,
    so you need to output the settings in one go.

    \sa ReadFunc, registerFormat()
*/

/*!
    \since 4.1
    \threadsafe

    Registers a custom storage format. On success, returns a special
    Format value that can then be passed to the QSettings constructor.
    On failure, returns InvalidFormat.

    The \a extension is the file
    extension associated to the format (without the '.').

    The \a readFunc and \a writeFunc parameters are pointers to
    functions that read and write a set of key/value pairs. The
    QIODevice parameter to the read and write functions is always
    opened in binary mode (i.e., without the QIODevice::Text flag).

    The \a caseSensitivity parameter specifies whether keys are case
    sensitive or not. This makes a difference when looking up values
    using QSettings. The default is case sensitive.

    By default, if you use one of the constructors that work in terms
    of an organization name and an application name, the file system
    locations used are the same as for IniFormat. Use setPath() to
    specify other locations.

    Example:

    \snippet doc/src/snippets/code/src_corelib_io_qsettings.cpp 29

    \sa setPath()
*/
QSettings::Format QSettings::registerFormat(const QString &extension, ReadFunc readFunc,
                                            WriteFunc writeFunc,
                                            Qt::CaseSensitivity caseSensitivity)
{
#ifdef QT_QSETTINGS_ALWAYS_CASE_SENSITIVE_AND_FORGET_ORIGINAL_KEY_ORDER
    Q_ASSERT(caseSensitivity == Qt::CaseSensitive);
#endif

    QMutexLocker locker(globalMutex());
    CustomFormatVector *customFormatVector = customFormatVectorFunc();
    int index = customFormatVector->size();
    if (index == 16) // the QSettings::Format enum has room for 16 custom formats
        return QSettings::InvalidFormat;

    QConfFileCustomFormat info;
    info.extension = QLatin1Char('.');
    info.extension += extension;
    info.readFunc = readFunc;
    info.writeFunc = writeFunc;
    info.caseSensitivity = caseSensitivity;
    customFormatVector->append(info);

    return QSettings::Format((int)QSettings::CustomFormat1 + index);
}

#ifdef QT3_SUPPORT
void QSettings::setPath_helper(Scope scope, const QString &organization, const QString &application)
{
    Q_D(QSettings);
    if (d->pendingChanges)
        d->flush();
    QSettingsPrivate *oldPriv = d;
    QSettingsPrivate *newPriv = QSettingsPrivate::create(oldPriv->format, scope, organization, application);
    static_cast<QObjectPrivate &>(*newPriv) = static_cast<QObjectPrivate &>(*oldPriv);  // copy the QObject stuff over (hack)
    d_ptr.reset(newPriv);
}

/*! \fn bool QSettings::writeEntry(const QString &key, bool value)

    Sets the value of setting \a key to \a value.

    Use setValue() instead.
*/

/*! \fn bool QSettings::writeEntry(const QString &key, double value)

    \overload
*/

/*! \fn bool QSettings::writeEntry(const QString &key, int value)

    \overload
*/

/*! \fn bool QSettings::writeEntry(const QString &key, const char *value)

    \overload
*/

/*! \fn bool QSettings::writeEntry(const QString &key, const QString &value)

    \overload
*/

/*! \fn bool QSettings::writeEntry(const QString &key, const QStringList &value)

    \overload
*/

/*! \fn bool QSettings::writeEntry(const QString &key, const QStringList &value, QChar separator)

    \overload

    Use setValue(\a key, \a value) instead. You don't need \a separator.
*/

/*! \fn QStringList QSettings::readListEntry(const QString &key, bool *ok = 0)

    Returns the value of setting \a key converted to a QStringList.

    If \a ok is not 0, *\a{ok} is set to true if the key exists,
    otherwise *\a{ok} is set to false.

    Use value() instead.

    \oldcode
        bool ok;
        QStringList list = settings.readListEntry("recentFiles", &ok);
    \newcode
        bool ok = settings.contains("recentFiles");
        QStringList list = settings.value("recentFiles").toStringList();
    \endcode
*/

/*! \fn QStringList QSettings::readListEntry(const QString &key, QChar separator, bool *ok)

    Returns the value of setting \a key converted to a QStringList.
    \a separator is ignored.

    If \a ok is not 0, *\a{ok} is set to true if the key exists,
    otherwise *\a{ok} is set to false.

    Use value() instead.

    \oldcode
        bool ok;
        QStringList list = settings.readListEntry("recentFiles", ":", &ok);
    \newcode
        bool ok = settings.contains("recentFiles");
        QStringList list = settings.value("recentFiles").toStringList();
    \endcode
*/

/*! \fn QString QSettings::readEntry(const QString &key, const QString &defaultValue, bool *ok)

    Returns the value for setting \a key converted to a QString. If
    the setting doesn't exist, returns \a defaultValue.

    If \a ok is not 0, *\a{ok} is set to true if the key exists,
    otherwise *\a{ok} is set to false.

    Use value() instead.

    \oldcode
        bool ok;
        QString str = settings.readEntry("userName", "administrator", &ok);
    \newcode
        bool ok = settings.contains("userName");
        QString str = settings.value("userName", "administrator").toString();
    \endcode
*/

/*! \fn int QSettings::readNumEntry(const QString &key, int defaultValue, bool *ok)

    Returns the value for setting \a key converted to an \c int. If
    the setting doesn't exist, returns \a defaultValue.

    If \a ok is not 0, *\a{ok} is set to true if the key exists,
    otherwise *\a{ok} is set to false.

    Use value() instead.

    \oldcode
        bool ok;
        int max = settings.readNumEntry("maxConnections", 30, &ok);
    \newcode
        bool ok = settings.contains("maxConnections");
        int max = settings.value("maxConnections", 30).toInt();
    \endcode
*/

/*! \fn double QSettings::readDoubleEntry(const QString &key, double defaultValue, bool *ok)

    Returns the value for setting \a key converted to a \c double. If
    the setting doesn't exist, returns \a defaultValue.

    If \a ok is not 0, *\a{ok} is set to true if the key exists,
    otherwise *\a{ok} is set to false.

    Use value() instead.

    \oldcode
        bool ok;
        double pi = settings.readDoubleEntry("pi", 3.141592, &ok);
    \newcode
        bool ok = settings.contains("pi");
        double pi = settings.value("pi", 3.141592).toDouble();
    \endcode
*/

/*! \fn bool QSettings::readBoolEntry(const QString &key, bool defaultValue, bool *ok)

    Returns the value for setting \a key converted to a \c bool. If
    the setting doesn't exist, returns \a defaultValue.

    If \a ok is not 0, *\a{ok} is set to true if the key exists,
    otherwise *\a{ok} is set to false.

    Use value() instead.

    \oldcode
        bool ok;
        bool grid = settings.readBoolEntry("showGrid", true, &ok);
    \newcode
        bool ok = settings.contains("showGrid");
        bool grid = settings.value("showGrid", true).toBool();
    \endcode
*/

/*! \fn bool QSettings::removeEntry(const QString &key)

    Use remove() instead.
*/

/*! \enum QSettings::System
    \compat

    \value Unix Unix systems (X11 and Embedded Linux)
    \value Windows Microsoft Windows systems
    \value Mac Mac OS X systems

    \sa insertSearchPath(), removeSearchPath()
*/

/*! \fn void QSettings::insertSearchPath(System system, const QString &path)

    This function is implemented as a no-op. It is provided for
    source compatibility with Qt 3. The new QSettings class has no
    concept of "search path".
*/

/*! \fn void QSettings::removeSearchPath(System system, const QString &path)

    This function is implemented as a no-op. It is provided for
    source compatibility with Qt 3. The new QSettings class has no
    concept of "search path".
*/

/*! \fn void QSettings::setPath(const QString &organization, const QString &application, \
                                Scope scope)

    Specifies the \a organization, \a application, and \a scope to
    use by the QSettings object.

    Use the appropriate constructor instead, with QSettings::UserScope
    instead of QSettings::User and QSettings::SystemScope instead of
    QSettings::Global.

    \oldcode
        QSettings settings;
        settings.setPath("twikimaster.com", "Kanooth", QSettings::Global);
    \newcode
        QSettings settings(QSettings::SystemScope, "twikimaster.com", "Kanooth");
    \endcode
*/

/*! \fn void QSettings::resetGroup()

    Sets the current group to be the empty string.

    Use endGroup() instead (possibly multiple times).

    \oldcode
        QSettings settings;
        settings.beginGroup("mainWindow");
        settings.beginGroup("leftPanel");
        ...
        settings.resetGroup();
    \newcode
        QSettings settings;
        settings.beginGroup("mainWindow");
        settings.beginGroup("leftPanel");
        ...
        settings.endGroup();
        settings.endGroup();
    \endcode
*/

/*! \fn QStringList QSettings::entryList(const QString &key) const

    Returns a list of all sub-keys of \a key.

    Use childKeys() instead.

    \oldcode
        QSettings settings;
        QStringList keys = settings.entryList("cities");
        ...
    \newcode
        QSettings settings;
        settings.beginGroup("cities");
        QStringList keys = settings.childKeys();
        ...
        settings.endGroup();
    \endcode
*/

/*! \fn QStringList QSettings::subkeyList(const QString &key) const

    Returns a list of all sub-keys of \a key.

    Use childGroups() instead.

    \oldcode
        QSettings settings;
        QStringList groups = settings.entryList("cities");
        ...
    \newcode
        QSettings settings;
        settings.beginGroup("cities");
        QStringList groups = settings.childKeys();
        ...
        settings.endGroup();
    \endcode
*/
#endif

QT_END_NAMESPACE

#endif // QT_NO_SETTINGS
