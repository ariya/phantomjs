/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QFILEDEVICE_H
#define QFILEDEVICE_H

#include <QtCore/qiodevice.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

class QFileDevicePrivate;

class Q_CORE_EXPORT QFileDevice : public QIODevice
{
#ifndef QT_NO_QOBJECT
    Q_OBJECT
#endif
    Q_DECLARE_PRIVATE(QFileDevice)

public:
    enum FileError {
        NoError = 0,
        ReadError = 1,
        WriteError = 2,
        FatalError = 3,
        ResourceError = 4,
        OpenError = 5,
        AbortError = 6,
        TimeOutError = 7,
        UnspecifiedError = 8,
        RemoveError = 9,
        RenameError = 10,
        PositionError = 11,
        ResizeError = 12,
        PermissionsError = 13,
        CopyError = 14
    };

    enum Permission {
        ReadOwner = 0x4000, WriteOwner = 0x2000, ExeOwner = 0x1000,
        ReadUser  = 0x0400, WriteUser  = 0x0200, ExeUser  = 0x0100,
        ReadGroup = 0x0040, WriteGroup = 0x0020, ExeGroup = 0x0010,
        ReadOther = 0x0004, WriteOther = 0x0002, ExeOther = 0x0001
    };
    Q_DECLARE_FLAGS(Permissions, Permission)

    enum FileHandleFlag {
        AutoCloseHandle = 0x0001,
        DontCloseHandle = 0
    };
    Q_DECLARE_FLAGS(FileHandleFlags, FileHandleFlag)

    ~QFileDevice();

    FileError error() const;
    void unsetError();

    virtual void close();

    bool isSequential() const;

    int handle() const;
    virtual QString fileName() const;

    qint64 pos() const;
    bool seek(qint64 offset);
    bool atEnd() const;
    bool flush();

    qint64 size() const;

    virtual bool resize(qint64 sz);
    virtual Permissions permissions() const;
    virtual bool setPermissions(Permissions permissionSpec);

    enum MemoryMapFlags {
        NoOptions = 0
    };

    uchar *map(qint64 offset, qint64 size, MemoryMapFlags flags = NoOptions);
    bool unmap(uchar *address);

protected:
    QFileDevice();
#ifdef QT_NO_QOBJECT
    QFileDevice(QFileDevicePrivate &dd);
#else
    explicit QFileDevice(QObject *parent);
    QFileDevice(QFileDevicePrivate &dd, QObject *parent = 0);
#endif

    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    qint64 readLineData(char *data, qint64 maxlen);

private:
    Q_DISABLE_COPY(QFileDevice)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QFileDevice::Permissions)

QT_END_NAMESPACE

#endif // QFILEDEVICE_H
