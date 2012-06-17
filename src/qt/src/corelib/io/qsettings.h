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

#ifndef QSETTINGS_H
#define QSETTINGS_H

#include <QtCore/qobject.h>
#include <QtCore/qvariant.h>
#include <QtCore/qstring.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE
QT_MODULE(Core)
QT_END_NAMESPACE

#ifndef QT_NO_SETTINGS

#ifdef QT3_SUPPORT
#include <QtCore/qstringlist.h>
#endif

#include <ctype.h>

QT_BEGIN_NAMESPACE

#ifdef Status // we seem to pick up a macro Status --> int somewhere
#undef Status
#endif

class QIODevice;
class QSettingsPrivate;

#ifndef QT_NO_QOBJECT
class Q_CORE_EXPORT QSettings : public QObject
#else
class Q_CORE_EXPORT QSettings
#endif
{
#ifndef QT_NO_QOBJECT
    Q_OBJECT
#else
    QScopedPointer<QSettingsPrivate> d_ptr;
#endif
    Q_DECLARE_PRIVATE(QSettings)

public:
    enum Status {
        NoError = 0,
        AccessError,
        FormatError
    };

    enum Format {
        NativeFormat,
        IniFormat,

        InvalidFormat = 16,
        CustomFormat1,
        CustomFormat2,
        CustomFormat3,
        CustomFormat4,
        CustomFormat5,
        CustomFormat6,
        CustomFormat7,
        CustomFormat8,
        CustomFormat9,
        CustomFormat10,
        CustomFormat11,
        CustomFormat12,
        CustomFormat13,
        CustomFormat14,
        CustomFormat15,
        CustomFormat16
    };

    enum Scope {
        UserScope,
        SystemScope
#ifdef QT3_SUPPORT
        ,
        User = UserScope,
        Global = SystemScope
#endif
    };

#ifndef QT_NO_QOBJECT
    explicit QSettings(const QString &organization,
                       const QString &application = QString(), QObject *parent = 0);
    QSettings(Scope scope, const QString &organization,
              const QString &application = QString(), QObject *parent = 0);
    QSettings(Format format, Scope scope, const QString &organization,
	      const QString &application = QString(), QObject *parent = 0);
    QSettings(const QString &fileName, Format format, QObject *parent = 0);
    explicit QSettings(QObject *parent = 0);
#else
    explicit QSettings(const QString &organization,
                       const QString &application = QString());
    QSettings(Scope scope, const QString &organization,
              const QString &application = QString());
    QSettings(Format format, Scope scope, const QString &organization,
              const QString &application = QString());
    QSettings(const QString &fileName, Format format);
#endif
    ~QSettings();

    void clear();
    void sync();
    Status status() const;

    void beginGroup(const QString &prefix);
    void endGroup();
    QString group() const;

    int beginReadArray(const QString &prefix);
    void beginWriteArray(const QString &prefix, int size = -1);
    void endArray();
    void setArrayIndex(int i);

    QStringList allKeys() const;
    QStringList childKeys() const;
    QStringList childGroups() const;
    bool isWritable() const;

    void setValue(const QString &key, const QVariant &value);
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;

    void remove(const QString &key);
    bool contains(const QString &key) const;

    void setFallbacksEnabled(bool b);
    bool fallbacksEnabled() const;

    QString fileName() const;
    Format format() const;
    Scope scope() const;
    QString organizationName() const;
    QString applicationName() const;

#ifndef QT_NO_TEXTCODEC
    void setIniCodec(QTextCodec *codec);
    void setIniCodec(const char *codecName);
    QTextCodec *iniCodec() const;
#endif

    static void setDefaultFormat(Format format);
    static Format defaultFormat();
    static void setSystemIniPath(const QString &dir); // ### remove in 5.0 (use setPath() instead)
    static void setUserIniPath(const QString &dir);   // ### remove in 5.0 (use setPath() instead)
    static void setPath(Format format, Scope scope, const QString &path);

    typedef QMap<QString, QVariant> SettingsMap;
    typedef bool (*ReadFunc)(QIODevice &device, SettingsMap &map);
    typedef bool (*WriteFunc)(QIODevice &device, const SettingsMap &map);

    static Format registerFormat(const QString &extension, ReadFunc readFunc, WriteFunc writeFunc,
                                 Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive);

#ifdef QT3_SUPPORT
    inline QT3_SUPPORT bool writeEntry(const QString &key, bool value)
    { setValue(key, value); return isWritable(); }
    inline QT3_SUPPORT bool writeEntry(const QString &key, double value)
    { setValue(key, value); return isWritable(); }
    inline QT3_SUPPORT bool writeEntry(const QString &key, int value)
    { setValue(key, value); return isWritable(); }
    inline QT3_SUPPORT bool writeEntry(const QString &key, const char *value)
    { setValue(key, QString::fromAscii(value)); return isWritable(); }
    inline QT3_SUPPORT bool writeEntry(const QString &key, const QString &value)
    { setValue(key, value); return isWritable(); }
    inline QT3_SUPPORT bool writeEntry(const QString &key, const QStringList &value)
    { setValue(key, value); return isWritable(); }
    inline QT3_SUPPORT bool writeEntry(const QString &key, const QStringList &value, QChar separator)
    { setValue(key, value.join(QString(separator))); return isWritable(); }
    inline QT3_SUPPORT QStringList readListEntry(const QString &key, bool *ok = 0)
    {
        if (ok)
            *ok = contains(key);
        return value(key).toStringList();
    }
    inline QT3_SUPPORT QStringList readListEntry(const QString &key, QChar separator, bool *ok = 0)
    {
        if (ok)
            *ok = contains(key);
        QString str = value(key).toString();
        if (str.isEmpty())
            return QStringList();
        return str.split(separator);
    }
    inline QT3_SUPPORT QString readEntry(const QString &key, const QString &defaultValue = QString(),
                                         bool *ok = 0)
    {
        if (ok)
            *ok = contains(key);
        return value(key, defaultValue).toString();
    }
    inline QT3_SUPPORT int readNumEntry(const QString &key, int defaultValue = 0, bool *ok = 0)
    {
        if (ok)
            *ok = contains(key);
        return value(key, defaultValue).toInt();
    }
    inline QT3_SUPPORT double readDoubleEntry(const QString &key, double defaultValue = 0,
                                              bool *ok = 0)
    {
        if (ok)
            *ok = contains(key);
        return value(key, defaultValue).toDouble();
    }
    inline QT3_SUPPORT bool readBoolEntry(const QString &key, bool defaultValue = false,
                                          bool *ok = 0)
    {
        if (ok)
            *ok = contains(key);
        return value(key, defaultValue).toBool();
    }
    inline QT3_SUPPORT bool removeEntry(const QString &key)
    { remove(key); return true; }

    enum System { Unix, Windows, Mac };
    inline QT3_SUPPORT void insertSearchPath(System, const QString &) {}
    inline QT3_SUPPORT void removeSearchPath(System, const QString &) {}

    inline QT3_SUPPORT void setPath(const QString &organization, const QString &application,
                                    Scope scope = Global)
    {
        setPath_helper(scope == Global ? QSettings::SystemScope : QSettings::UserScope,
                       organization, application);
    }
    inline QT3_SUPPORT void resetGroup()
    {
        while (!group().isEmpty())
            endGroup();
    }
    inline QT3_SUPPORT QStringList entryList(const QString &key) const
    {
        QSettings *that = const_cast<QSettings *>(this);
        QStringList result;

        that->beginGroup(key);
        result = that->childKeys();
        that->endGroup();
        return result;
    }
    inline QT3_SUPPORT QStringList subkeyList(const QString &key) const
    {
        QSettings *that = const_cast<QSettings *>(this);
        QStringList result;

        that->beginGroup(key);
        result = that->childGroups();
        that->endGroup();
        return result;
    }
#endif

protected:
#ifndef QT_NO_QOBJECT
    bool event(QEvent *event);
#endif

private:
#ifdef QT3_SUPPORT
    void setPath_helper(Scope scope, const QString &organization, const QString &application);
#endif

    Q_DISABLE_COPY(QSettings)
};

QT_END_NAMESPACE

#endif // QT_NO_SETTINGS

QT_END_HEADER

#endif // QSETTINGS_H
