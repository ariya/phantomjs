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

#ifndef QSETTINGS_H
#define QSETTINGS_H

#include <QtCore/qobject.h>
#include <QtCore/qvariant.h>
#include <QtCore/qstring.h>
#include <QtCore/qscopedpointer.h>

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

#ifndef QT_NO_SETTINGS

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
    static void setSystemIniPath(const QString &dir); // ### Qt 6: remove (use setPath() instead)
    static void setUserIniPath(const QString &dir);   // ### Qt 6: remove (use setPath() instead)
    static void setPath(Format format, Scope scope, const QString &path);

    typedef QMap<QString, QVariant> SettingsMap;
    typedef bool (*ReadFunc)(QIODevice &device, SettingsMap &map);
    typedef bool (*WriteFunc)(QIODevice &device, const SettingsMap &map);

    static Format registerFormat(const QString &extension, ReadFunc readFunc, WriteFunc writeFunc,
                                 Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive);

protected:
#ifndef QT_NO_QOBJECT
    bool event(QEvent *event);
#endif

private:
    Q_DISABLE_COPY(QSettings)
};

QT_END_NAMESPACE

#endif // QT_NO_SETTINGS

#endif // QSETTINGS_H
