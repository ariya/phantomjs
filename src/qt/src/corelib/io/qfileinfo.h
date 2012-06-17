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

#ifndef QFILEINFO_H
#define QFILEINFO_H

#include <QtCore/qfile.h>
#include <QtCore/qlist.h>
#include <QtCore/qshareddata.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

class QDir;
class QDirIteratorPrivate;
class QDateTime;
class QFileInfoPrivate;

class Q_CORE_EXPORT QFileInfo
{
    friend class QDirIteratorPrivate;
public:
    explicit QFileInfo(QFileInfoPrivate *d);

    QFileInfo();
    QFileInfo(const QString &file);
    QFileInfo(const QFile &file);
    QFileInfo(const QDir &dir, const QString &file);
    QFileInfo(const QFileInfo &fileinfo);
    ~QFileInfo();

    QFileInfo &operator=(const QFileInfo &fileinfo);
#ifdef Q_COMPILER_RVALUE_REFS
    inline QFileInfo&operator=(QFileInfo &&other)
    { qSwap(d_ptr, other.d_ptr); return *this; }
#endif
    bool operator==(const QFileInfo &fileinfo); // 5.0 - remove me
    bool operator==(const QFileInfo &fileinfo) const;
    inline bool operator!=(const QFileInfo &fileinfo) { return !(operator==(fileinfo)); } // 5.0 - remove me
    inline bool operator!=(const QFileInfo &fileinfo) const { return !(operator==(fileinfo)); }

    void setFile(const QString &file);
    void setFile(const QFile &file);
    void setFile(const QDir &dir, const QString &file);
    bool exists() const;
    void refresh();

    QString filePath() const;
    QString absoluteFilePath() const;
    QString canonicalFilePath() const;
    QString fileName() const;
    QString baseName() const;
    QString completeBaseName() const;
    QString suffix() const;
    QString bundleName() const;
    QString completeSuffix() const;

    QString path() const;
    QString absolutePath() const;
    QString canonicalPath() const;
    QDir dir() const;
    QDir absoluteDir() const;

    bool isReadable() const;
    bool isWritable() const;
    bool isExecutable() const;
    bool isHidden() const;

    bool isRelative() const;
    inline bool isAbsolute() const { return !isRelative(); }
    bool makeAbsolute();

    bool isFile() const;
    bool isDir() const;
    bool isSymLink() const;
    bool isRoot() const;
    bool isBundle() const;

    QString readLink() const;
    inline QString symLinkTarget() const { return readLink(); }

    QString owner() const;
    uint ownerId() const;
    QString group() const;
    uint groupId() const;

    bool permission(QFile::Permissions permissions) const;
    QFile::Permissions permissions() const;

    qint64 size() const;

    QDateTime created() const;
    QDateTime lastModified() const;
    QDateTime lastRead() const;

    void detach();

    bool caching() const;
    void setCaching(bool on);

#ifdef QT3_SUPPORT
    enum Permission {
        ReadOwner = QFile::ReadOwner, WriteOwner = QFile::WriteOwner, ExeOwner = QFile::ExeOwner,
        ReadUser  = QFile::ReadUser,  WriteUser  = QFile::WriteUser,  ExeUser  = QFile::ExeUser,
        ReadGroup = QFile::ReadGroup, WriteGroup = QFile::WriteGroup, ExeGroup = QFile::ExeGroup,
        ReadOther = QFile::ReadOther, WriteOther = QFile::WriteOther, ExeOther = QFile::ExeOther
    };
    Q_DECLARE_FLAGS(PermissionSpec, Permission)

    inline QT3_SUPPORT QString baseName(bool complete) {
        if(complete)
            return completeBaseName();
        return baseName();
    }
    inline QT3_SUPPORT QString extension(bool complete = true) const {
        if(complete)
            return completeSuffix();
        return suffix();
    }
    inline QT3_SUPPORT QString absFilePath() const { return absoluteFilePath(); }

    inline QT3_SUPPORT QString dirPath(bool absPath = false) const {
        if(absPath)
            return absolutePath();
        return path();
    }
    QT3_SUPPORT QDir dir(bool absPath) const;
    inline QT3_SUPPORT bool convertToAbs() { return makeAbsolute(); }
#if !defined(Q_NO_TYPESAFE_FLAGS)
    inline QT3_SUPPORT bool permission(PermissionSpec permissions) const
    { return permission(QFile::Permissions(static_cast<int>(permissions))); }
#endif
#endif

protected:
    QSharedDataPointer<QFileInfoPrivate> d_ptr;
private:
    inline QFileInfoPrivate* d_func()
    {
        detach();
        return const_cast<QFileInfoPrivate *>(d_ptr.constData());
    }

    inline const QFileInfoPrivate* d_func() const
    {
        return d_ptr.constData();
    }
};

Q_DECLARE_TYPEINFO(QFileInfo, Q_MOVABLE_TYPE);

#ifdef QT3_SUPPORT
Q_DECLARE_OPERATORS_FOR_FLAGS(QFileInfo::PermissionSpec)
#endif

typedef QList<QFileInfo> QFileInfoList;
#ifdef QT3_SUPPORT
typedef QList<QFileInfo>::Iterator QFileInfoListIterator;
#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif // QFILEINFO_H
