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

#ifndef QFSFILEENGINE_H
#define QFSFILEENGINE_H

#include <QtCore/qabstractfileengine.h>
#ifdef Q_OS_SYMBIAN
#include <f32file.h>
#endif

#ifndef QT_NO_FSFILEENGINE

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

class QFSFileEnginePrivate;

class Q_CORE_EXPORT QFSFileEngine : public QAbstractFileEngine
{
    Q_DECLARE_PRIVATE(QFSFileEngine)
public:
    QFSFileEngine();
    explicit QFSFileEngine(const QString &file);
    ~QFSFileEngine();

    bool open(QIODevice::OpenMode openMode);
    bool open(QIODevice::OpenMode flags, FILE *fh);
    bool close();
    bool flush();
    qint64 size() const;
    qint64 pos() const;
    bool seek(qint64);
    bool isSequential() const;
    bool remove();
    bool copy(const QString &newName);
    bool rename(const QString &newName);
    bool link(const QString &newName);
    bool mkdir(const QString &dirName, bool createParentDirectories) const;
    bool rmdir(const QString &dirName, bool recurseParentDirectories) const;
    bool setSize(qint64 size);
    bool caseSensitive() const;
    bool isRelativePath() const;
    QStringList entryList(QDir::Filters filters, const QStringList &filterNames) const;
    FileFlags fileFlags(FileFlags type) const;
    bool setPermissions(uint perms);
    QString fileName(FileName file) const;
    uint ownerId(FileOwner) const;
    QString owner(FileOwner) const;
    QDateTime fileTime(FileTime time) const;
    void setFileName(const QString &file);
    int handle() const;

    Iterator *beginEntryList(QDir::Filters filters, const QStringList &filterNames);
    Iterator *endEntryList();

    qint64 read(char *data, qint64 maxlen);
    qint64 readLine(char *data, qint64 maxlen);
    qint64 write(const char *data, qint64 len);

    bool extension(Extension extension, const ExtensionOption *option = 0, ExtensionReturn *output = 0);
    bool supportsExtension(Extension extension) const;

    //FS only!!
    bool open(QIODevice::OpenMode flags, int fd);
    bool open(QIODevice::OpenMode flags, int fd, QFile::FileHandleFlags handleFlags);
    bool open(QIODevice::OpenMode flags, FILE *fh, QFile::FileHandleFlags handleFlags);
#ifdef Q_OS_SYMBIAN
    bool open(QIODevice::OpenMode flags, const RFile &f, QFile::FileHandleFlags handleFlags);
#endif
    static bool setCurrentPath(const QString &path);
    static QString currentPath(const QString &path = QString());
    static QString homePath();
    static QString rootPath();
    static QString tempPath();
    static QFileInfoList drives();

protected:
    QFSFileEngine(QFSFileEnginePrivate &dd);
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QT_NO_FSFILEENGINE

#endif // QFSFILEENGINE_H
