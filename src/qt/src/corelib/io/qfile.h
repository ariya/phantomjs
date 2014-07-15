/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QFILE_H
#define QFILE_H

#include <QtCore/qiodevice.h>
#include <QtCore/qstring.h>
#include <stdio.h>
#ifdef Q_OS_SYMBIAN
#include <f32file.h>
#endif

#ifdef open
#error qfile.h must be included before any header file that defines open
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

class QAbstractFileEngine;
class QFilePrivate;

class Q_CORE_EXPORT QFile : public QIODevice
{
#ifndef QT_NO_QOBJECT
    Q_OBJECT
#endif
    Q_DECLARE_PRIVATE(QFile)

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
#ifdef QT3_SUPPORT
        , ConnectError = 30
#endif
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

    QFile();
    QFile(const QString &name);
#ifndef QT_NO_QOBJECT
    explicit QFile(QObject *parent);
    QFile(const QString &name, QObject *parent);
#endif
    ~QFile();

    FileError error() const;
    void unsetError();

    QString fileName() const;
    void setFileName(const QString &name);

    typedef QByteArray (*EncoderFn)(const QString &fileName);
    typedef QString (*DecoderFn)(const QByteArray &localfileName);
    static QByteArray encodeName(const QString &fileName);
    static QString decodeName(const QByteArray &localFileName);
    inline static QString decodeName(const char *localFileName)
        { return decodeName(QByteArray(localFileName)); }
    static void setEncodingFunction(EncoderFn);
    static void setDecodingFunction(DecoderFn);

    bool exists() const;
    static bool exists(const QString &fileName);

    QString readLink() const;
    static QString readLink(const QString &fileName);
    inline QString symLinkTarget() const { return readLink(); }
    inline static QString symLinkTarget(const QString &fileName) { return readLink(fileName); }

    bool remove();
    static bool remove(const QString &fileName);

    bool rename(const QString &newName);
    static bool rename(const QString &oldName, const QString &newName);

    bool link(const QString &newName);
    static bool link(const QString &oldname, const QString &newName);

    bool copy(const QString &newName);
    static bool copy(const QString &fileName, const QString &newName);

    bool isSequential() const;

    bool open(OpenMode flags);
    bool open(FILE *f, OpenMode flags);
    bool open(int fd, OpenMode flags);
#ifdef Q_OS_SYMBIAN
    bool open(const RFile &f, OpenMode flags, FileHandleFlags handleFlags = DontCloseHandle);
#endif
    bool open(FILE *f, OpenMode ioFlags, FileHandleFlags handleFlags);
    bool open(int fd, OpenMode ioFlags, FileHandleFlags handleFlags);
    virtual void close();

    qint64 size() const;
    qint64 pos() const;
    bool seek(qint64 offset);
    bool atEnd() const;
    bool flush();

    bool resize(qint64 sz);
    static bool resize(const QString &filename, qint64 sz);

    Permissions permissions() const;
    static Permissions permissions(const QString &filename);
    bool setPermissions(Permissions permissionSpec);
    static bool setPermissions(const QString &filename, Permissions permissionSpec);

    int handle() const;

    enum MemoryMapFlags {
        NoOptions = 0
    };

    uchar *map(qint64 offset, qint64 size, MemoryMapFlags flags = NoOptions);
    bool unmap(uchar *address);

    virtual QAbstractFileEngine *fileEngine() const;

#ifdef QT3_SUPPORT
    typedef Permission PermissionSpec;
    inline QT3_SUPPORT QString name() const { return fileName(); }
    inline QT3_SUPPORT void setName(const QString &aName) { setFileName(aName); }
    inline QT3_SUPPORT bool open(OpenMode aFlags, FILE *f) { return open(f, aFlags); }
    inline QT3_SUPPORT bool open(OpenMode aFlags, int fd) { return open(fd, aFlags); }
#endif

protected:
#ifdef QT_NO_QOBJECT
    QFile(QFilePrivate &dd);
#else
    QFile(QFilePrivate &dd, QObject *parent = 0);
#endif

    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    qint64 readLineData(char *data, qint64 maxlen);

private:
    Q_DISABLE_COPY(QFile)
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QFile::Permissions)

QT_END_NAMESPACE

QT_END_HEADER

#endif // QFILE_H
