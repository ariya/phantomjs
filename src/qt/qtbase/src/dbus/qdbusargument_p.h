/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtDBus module of the Qt Toolkit.
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

#ifndef QDBUSARGUMENT_P_H
#define QDBUSARGUMENT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <qdbusargument.h>
#include "qdbusunixfiledescriptor.h"
#include "qdbus_symbols_p.h"

#ifndef QT_NO_DBUS

#ifndef DBUS_TYPE_UNIX_FD
# define DBUS_TYPE_UNIX_FD int('h')
# define DBUS_TYPE_UNIX_FD_AS_STRING "h"
#endif

QT_BEGIN_NAMESPACE

class QDBusMarshaller;
class QDBusDemarshaller;
class QDBusArgumentPrivate
{
public:
    inline QDBusArgumentPrivate(int flags = 0)
        : message(0), ref(1), capabilities(flags)
    { }
    ~QDBusArgumentPrivate();

    static bool checkRead(QDBusArgumentPrivate *d);
    static bool checkReadAndDetach(QDBusArgumentPrivate *&d);
    static bool checkWrite(QDBusArgumentPrivate *&d);

    QDBusMarshaller *marshaller();
    QDBusDemarshaller *demarshaller();

    static QByteArray createSignature(int id);
    static inline QDBusArgument create(QDBusArgumentPrivate *d)
    {
        QDBusArgument q(d);
        return q;
    }
    static inline QDBusArgumentPrivate *d(QDBusArgument &q)
    { return q.d; }

public:
    DBusMessage *message;
    QAtomicInt ref;
    int capabilities;
    enum Direction {
        Marshalling,
        Demarshalling
    } direction;
};

class QDBusMarshaller: public QDBusArgumentPrivate
{
public:
    QDBusMarshaller(int flags) : QDBusArgumentPrivate(flags), parent(0), ba(0), closeCode(0), ok(true)
    { direction = Marshalling; }
    ~QDBusMarshaller();

    QString currentSignature();

    void append(uchar arg);
    void append(bool arg);
    void append(short arg);
    void append(ushort arg);
    void append(int arg);
    void append(uint arg);
    void append(qlonglong arg);
    void append(qulonglong arg);
    void append(double arg);
    void append(const QString &arg);
    void append(const QDBusObjectPath &arg);
    void append(const QDBusSignature &arg);
    void append(const QDBusUnixFileDescriptor &arg);
    void append(const QStringList &arg);
    void append(const QByteArray &arg);
    bool append(const QDBusVariant &arg); // this one can fail

    QDBusMarshaller *beginStructure();
    QDBusMarshaller *endStructure();
    QDBusMarshaller *beginArray(int id);
    QDBusMarshaller *endArray();
    QDBusMarshaller *beginMap(int kid, int vid);
    QDBusMarshaller *endMap();
    QDBusMarshaller *beginMapEntry();
    QDBusMarshaller *endMapEntry();
    QDBusMarshaller *beginCommon(int code, const char *signature);
    QDBusMarshaller *endCommon();
    void open(QDBusMarshaller &sub, int code, const char *signature);
    void close();
    void error(const QString &message);

    bool appendVariantInternal(const QVariant &arg);
    bool appendRegisteredType(const QVariant &arg);
    bool appendCrossMarshalling(QDBusDemarshaller *arg);

public:
    DBusMessageIter iterator;
    QDBusMarshaller *parent;
    QByteArray *ba;
    QString errorString;
    char closeCode;
    bool ok;

private:
    Q_DISABLE_COPY(QDBusMarshaller)
};

class QDBusDemarshaller: public QDBusArgumentPrivate
{
public:
    inline QDBusDemarshaller(int flags) : QDBusArgumentPrivate(flags), parent(0)
    { direction = Demarshalling; }
    ~QDBusDemarshaller();

    QString currentSignature();

    uchar toByte();
    bool toBool();
    ushort toUShort();
    short toShort();
    int toInt();
    uint toUInt();
    qlonglong toLongLong();
    qulonglong toULongLong();
    double toDouble();
    QString toString();
    QDBusObjectPath toObjectPath();
    QDBusSignature toSignature();
    QDBusUnixFileDescriptor toUnixFileDescriptor();
    QDBusVariant toVariant();
    QStringList toStringList();
    QByteArray toByteArray();

    QDBusDemarshaller *beginStructure();
    QDBusDemarshaller *endStructure();
    QDBusDemarshaller *beginArray();
    QDBusDemarshaller *endArray();
    QDBusDemarshaller *beginMap();
    QDBusDemarshaller *endMap();
    QDBusDemarshaller *beginMapEntry();
    QDBusDemarshaller *endMapEntry();
    QDBusDemarshaller *beginCommon();
    QDBusDemarshaller *endCommon();
    QDBusArgument duplicate();
    inline void close() { }

    bool atEnd();

    QVariant toVariantInternal();
    QDBusArgument::ElementType currentType();
    bool isCurrentTypeStringLike();

public:
    DBusMessageIter iterator;
    QDBusDemarshaller *parent;

private:
    Q_DISABLE_COPY(QDBusDemarshaller)
    QString toStringUnchecked();
    QDBusObjectPath toObjectPathUnchecked();
    QDBusSignature toSignatureUnchecked();
    QStringList toStringListUnchecked();
    QByteArray toByteArrayUnchecked();
};

inline QDBusMarshaller *QDBusArgumentPrivate::marshaller()
{ return static_cast<QDBusMarshaller *>(this); }

inline QDBusDemarshaller *QDBusArgumentPrivate::demarshaller()
{ return static_cast<QDBusDemarshaller *>(this); }

QT_END_NAMESPACE

#endif // QT_NO_DBUS
#endif
