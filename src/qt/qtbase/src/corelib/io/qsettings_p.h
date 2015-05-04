/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSETTINGS_P_H
#define QSETTINGS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qdatetime.h"
#include "QtCore/qmap.h"
#include "QtCore/qmutex.h"
#include "QtCore/qiodevice.h"
#include "QtCore/qstack.h"
#include "QtCore/qstringlist.h"
#ifndef QT_NO_QOBJECT
#include "private/qobject_p.h"
#endif
#include "private/qscopedpointer_p.h"

QT_BEGIN_NAMESPACE

#ifndef Q_OS_WIN
#define QT_QSETTINGS_ALWAYS_CASE_SENSITIVE_AND_FORGET_ORIGINAL_KEY_ORDER
#endif

#if defined(Q_OS_WINRT)
#define QT_QTSETTINGS_FORGET_ORIGINAL_KEY_ORDER
#endif

// used in testing framework
#define QSETTINGS_P_H_VERSION 3

#ifdef QT_QSETTINGS_ALWAYS_CASE_SENSITIVE_AND_FORGET_ORIGINAL_KEY_ORDER
static const Qt::CaseSensitivity IniCaseSensitivity = Qt::CaseSensitive;

class QSettingsKey : public QString
{
public:
    inline QSettingsKey(const QString &key, Qt::CaseSensitivity cs, int /* position */ = -1)
        : QString(key) { Q_ASSERT(cs == Qt::CaseSensitive); Q_UNUSED(cs); }

    inline QString originalCaseKey() const { return *this; }
    inline int originalKeyPosition() const { return -1; }
};
#else
static const Qt::CaseSensitivity IniCaseSensitivity = Qt::CaseInsensitive;

class QSettingsKey : public QString
{
public:
    inline QSettingsKey(const QString &key, Qt::CaseSensitivity cs, int position = -1)
         : QString(key), theOriginalKey(key), theOriginalKeyPosition(position)
    {
        if (cs == Qt::CaseInsensitive)
            QString::operator=(toLower());
    }

    inline QString originalCaseKey() const { return theOriginalKey; }
    inline int originalKeyPosition() const { return theOriginalKeyPosition; }

private:
    QString theOriginalKey;
    int theOriginalKeyPosition;
};
#endif

typedef QMap<QSettingsKey, QByteArray> UnparsedSettingsMap;
typedef QMap<QSettingsKey, QVariant> ParsedSettingsMap;

class QSettingsGroup
{
public:
    inline QSettingsGroup()
        : num(-1), maxNum(-1) {}
    inline QSettingsGroup(const QString &s)
        : str(s), num(-1), maxNum(-1) {}
    inline QSettingsGroup(const QString &s, bool guessArraySize)
        : str(s), num(0), maxNum(guessArraySize ? 0 : -1) {}

    inline QString name() const { return str; }
    inline QString toString() const;
    inline bool isArray() const { return num != -1; }
    inline int arraySizeGuess() const { return maxNum; }
    inline void setArrayIndex(int i)
    { num = i + 1; if (maxNum != -1 && num > maxNum) maxNum = num; }

    QString str;
    int num;
    int maxNum;
};

inline QString QSettingsGroup::toString() const
{
    QString result;
    result = str;
    if (num > 0) {
        result += QLatin1Char('/');
        result += QString::number(num);
    }
    return result;
}

class Q_AUTOTEST_EXPORT QConfFile
{
public:
    ~QConfFile();

    ParsedSettingsMap mergedKeyMap() const;
    bool isWritable() const;

    static QConfFile *fromName(const QString &name, bool _userPerms);
    static void clearCache();

    QString name;
    QDateTime timeStamp;
    qint64 size;
    UnparsedSettingsMap unparsedIniSections;
    ParsedSettingsMap originalKeys;
    ParsedSettingsMap addedKeys;
    ParsedSettingsMap removedKeys;
    QAtomicInt ref;
    QMutex mutex;
    bool userPerms;

private:
#ifdef Q_DISABLE_COPY
    QConfFile(const QConfFile &);
    QConfFile &operator=(const QConfFile &);
#endif
    QConfFile(const QString &name, bool _userPerms);

    friend class QConfFile_createsItself; // silences compiler warning
};

class Q_AUTOTEST_EXPORT QSettingsPrivate
#ifndef QT_NO_QOBJECT
    : public QObjectPrivate
#endif
{
#ifdef QT_NO_QOBJECT
    QSettings *q_ptr;
#endif
    Q_DECLARE_PUBLIC(QSettings)

public:
    QSettingsPrivate(QSettings::Format format);
    QSettingsPrivate(QSettings::Format format, QSettings::Scope scope,
                     const QString &organization, const QString &application);
    virtual ~QSettingsPrivate();

    virtual void remove(const QString &key) = 0;
    virtual void set(const QString &key, const QVariant &value) = 0;
    virtual bool get(const QString &key, QVariant *value) const = 0;

    enum ChildSpec { AllKeys, ChildKeys, ChildGroups };
    virtual QStringList children(const QString &prefix, ChildSpec spec) const = 0;

    virtual void clear() = 0;
    virtual void sync() = 0;
    virtual void flush() = 0;
    virtual bool isWritable() const = 0;
    virtual QString fileName() const = 0;

    QString actualKey(const QString &key) const;
    void beginGroupOrArray(const QSettingsGroup &group);
    void setStatus(QSettings::Status status) const;
    void requestUpdate();
    void update();

    static QString normalizedKey(const QString &key);
    static QSettingsPrivate *create(QSettings::Format format, QSettings::Scope scope,
                                        const QString &organization, const QString &application);
    static QSettingsPrivate *create(const QString &fileName, QSettings::Format format);

    static void processChild(QString key, ChildSpec spec, QMap<QString, QString> &result);

    // Variant streaming functions
    static QStringList variantListToStringList(const QVariantList &l);
    static QVariant stringListToVariantList(const QStringList &l);

    // parser functions
    static QString variantToString(const QVariant &v);
    static QVariant stringToVariant(const QString &s);
    static void iniEscapedKey(const QString &key, QByteArray &result);
    static bool iniUnescapedKey(const QByteArray &key, int from, int to, QString &result);
    static void iniEscapedString(const QString &str, QByteArray &result, QTextCodec *codec);
    static void iniEscapedStringList(const QStringList &strs, QByteArray &result, QTextCodec *codec);
    static bool iniUnescapedStringList(const QByteArray &str, int from, int to,
                                       QString &stringResult, QStringList &stringListResult,
                                       QTextCodec *codec);
    static QStringList splitArgs(const QString &s, int idx);

    /*
    The numeric values of these enums define their search order. For example,
    F_User | F_Organization is searched before F_System | F_Application,
    because their values are respectively 1 and 2.
    */
    enum {
#if !defined(Q_OS_BLACKBERRY)
        F_Application = 0x0,
        F_Organization = 0x1,
        F_User = 0x0,
        F_System = 0x2,
        NumConfFiles = 4
#else
        SandboxConfFile = 0,
        NumConfFiles = 1
#endif
    };

    QSettings::Format format;
    QSettings::Scope scope;
    QString organizationName;
    QString applicationName;
    QTextCodec *iniCodec;

protected:
    QStack<QSettingsGroup> groupStack;
    QString groupPrefix;
    int spec;
    bool fallbacks;
    bool pendingChanges;
    mutable QSettings::Status status;
};

class QConfFileSettingsPrivate : public QSettingsPrivate
{
public:
    QConfFileSettingsPrivate(QSettings::Format format, QSettings::Scope scope,
                             const QString &organization, const QString &application);
    QConfFileSettingsPrivate(const QString &fileName, QSettings::Format format);
    ~QConfFileSettingsPrivate();

    void remove(const QString &key);
    void set(const QString &key, const QVariant &value);
    bool get(const QString &key, QVariant *value) const;

    QStringList children(const QString &prefix, ChildSpec spec) const;

    void clear();
    void sync();
    void flush();
    bool isWritable() const;
    QString fileName() const;

    bool readIniFile(const QByteArray &data, UnparsedSettingsMap *unparsedIniSections);
    static bool readIniSection(const QSettingsKey &section, const QByteArray &data,
                               ParsedSettingsMap *settingsMap, QTextCodec *codec);
    static bool readIniLine(const QByteArray &data, int &dataPos, int &lineStart, int &lineLen,
                            int &equalsPos);

private:
    void initFormat();
    void initAccess();
    void syncConfFile(int confFileNo);
    bool writeIniFile(QIODevice &device, const ParsedSettingsMap &map);
#ifdef Q_OS_MAC
    bool readPlistFile(const QString &fileName, ParsedSettingsMap *map) const;
    bool writePlistFile(const QString &fileName, const ParsedSettingsMap &map) const;
#endif
    void ensureAllSectionsParsed(QConfFile *confFile) const;
    void ensureSectionParsed(QConfFile *confFile, const QSettingsKey &key) const;

    QScopedSharedPointer<QConfFile> confFiles[NumConfFiles];
    QSettings::ReadFunc readFunc;
    QSettings::WriteFunc writeFunc;
    QString extension;
    Qt::CaseSensitivity caseSensitivity;
    int nextPosition;
};

QT_END_NAMESPACE

#endif // QSETTINGS_P_H
