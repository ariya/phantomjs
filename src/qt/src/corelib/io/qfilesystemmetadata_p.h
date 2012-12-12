/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QFILESYSTEMMETADATA_P_H_INCLUDED
#define QFILESYSTEMMETADATA_P_H_INCLUDED

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

#include "qplatformdefs.h"
#include <QtCore/qglobal.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qabstractfileengine.h>

// Platform-specific includes
#if defined(Q_OS_WIN)
#ifndef IO_REPARSE_TAG_SYMLINK
#define IO_REPARSE_TAG_SYMLINK (0xA000000CL)
#endif
#elif defined(Q_OS_SYMBIAN)
#include <f32file.h>
#include <QtCore/private/qdatetime_p.h>
#endif

QT_BEGIN_NAMESPACE

class QFileSystemEngine;

class QFileSystemMetaData
{
public:
    QFileSystemMetaData()
        : knownFlagsMask(0)
    {
    }

    enum MetaDataFlag {
        // Permissions, overlaps with QFile::Permissions
        OtherReadPermission = 0x00000004,   OtherWritePermission = 0x00000002,  OtherExecutePermission = 0x00000001,
        GroupReadPermission = 0x00000040,   GroupWritePermission = 0x00000020,  GroupExecutePermission = 0x00000010,
        UserReadPermission  = 0x00000400,   UserWritePermission  = 0x00000200,  UserExecutePermission  = 0x00000100,
        OwnerReadPermission = 0x00004000,   OwnerWritePermission = 0x00002000,  OwnerExecutePermission = 0x00001000,

        OtherPermissions    = OtherReadPermission | OtherWritePermission | OtherExecutePermission,
        GroupPermissions    = GroupReadPermission | GroupWritePermission | GroupExecutePermission,
        UserPermissions     = UserReadPermission  | UserWritePermission  | UserExecutePermission,
        OwnerPermissions    = OwnerReadPermission | OwnerWritePermission | OwnerExecutePermission,

        ReadPermissions     = OtherReadPermission | GroupReadPermission | UserReadPermission | OwnerReadPermission,
        WritePermissions    = OtherWritePermission | GroupWritePermission | UserWritePermission | OwnerWritePermission,
        ExecutePermissions  = OtherExecutePermission | GroupExecutePermission | UserExecutePermission | OwnerExecutePermission,

        Permissions         = OtherPermissions | GroupPermissions | UserPermissions | OwnerPermissions,

        // Type
#ifdef Q_OS_SYMBIAN
        LinkType            = 0,
#else
        LinkType            = 0x00010000,
#endif
        FileType            = 0x00020000,
        DirectoryType       = 0x00040000,
#if !defined(QWS) && !defined(Q_WS_QPA) && defined(Q_OS_MAC)
        BundleType          = 0x00080000,
        AliasType           = 0x08000000,
#else
        BundleType          =        0x0,
        AliasType           =        0x0,
#endif
#if defined(Q_OS_WIN)
        WinLnkType          = 0x08000000,   // Note: Uses the same position for AliasType on Mac
#else
        WinLnkType          =        0x0,
#endif
        SequentialType      = 0x00800000,   // Note: overlaps with QAbstractFileEngine::RootFlag

        LegacyLinkType      = LinkType | AliasType | WinLnkType,

        Type                = LinkType | FileType | DirectoryType | BundleType | SequentialType | AliasType,

        // Attributes
        HiddenAttribute     = 0x00100000,
        SizeAttribute       = 0x00200000,   // Note: overlaps with QAbstractFileEngine::LocalDiskFlag
        ExistsAttribute     = 0x00400000,

        Attributes          = HiddenAttribute | SizeAttribute | ExistsAttribute,

        // Times
        CreationTime        = 0x01000000,   // Note: overlaps with QAbstractFileEngine::Refresh
        ModificationTime    = 0x02000000,
        AccessTime          = 0x04000000,

        Times               = CreationTime | ModificationTime | AccessTime,

        // Owner IDs
        UserId              = 0x10000000,
        GroupId             = 0x20000000,

        OwnerIds            = UserId | GroupId,

        PosixStatFlags      = QFileSystemMetaData::OtherPermissions
                            | QFileSystemMetaData::GroupPermissions
                            | QFileSystemMetaData::OwnerPermissions
                            | QFileSystemMetaData::FileType
                            | QFileSystemMetaData::DirectoryType
                            | QFileSystemMetaData::SequentialType
                            | QFileSystemMetaData::SizeAttribute
                            | QFileSystemMetaData::Times
                            | QFileSystemMetaData::OwnerIds,

        SymbianTEntryFlags  = QFileSystemMetaData::Permissions
                            | QFileSystemMetaData::FileType
                            | QFileSystemMetaData::DirectoryType
                            | QFileSystemMetaData::SequentialType
                            | QFileSystemMetaData::Attributes
                            | QFileSystemMetaData::Times,
#if defined(Q_OS_WIN)
        WinStatFlags        = QFileSystemMetaData::FileType
                            | QFileSystemMetaData::DirectoryType
                            | QFileSystemMetaData::HiddenAttribute
                            | QFileSystemMetaData::ExistsAttribute
                            | QFileSystemMetaData::SizeAttribute
                            | QFileSystemMetaData::Times,
#endif

        AllMetaDataFlags    = 0xFFFFFFFF

    };
    Q_DECLARE_FLAGS(MetaDataFlags, MetaDataFlag)

    bool hasFlags(MetaDataFlags flags) const
    {
        return ((knownFlagsMask & flags) == flags);
    }

    MetaDataFlags missingFlags(MetaDataFlags flags)
    {
        return flags & ~knownFlagsMask;
    }

    void clear()
    {
        knownFlagsMask = 0;
    }

    void clearFlags(MetaDataFlags flags = AllMetaDataFlags)
    {
        knownFlagsMask &= ~flags;
    }

    bool exists() const                     { return (entryFlags & ExistsAttribute); }

    bool isLink() const                     { return  (entryFlags & LinkType); }
    bool isFile() const                     { return (entryFlags & FileType); }
    bool isDirectory() const                { return (entryFlags & DirectoryType); }
    bool isBundle() const;
    bool isAlias() const;
    bool isLegacyLink() const               { return (entryFlags & LegacyLinkType); }
    bool isSequential() const               { return (entryFlags & SequentialType); }
    bool isHidden() const                   { return (entryFlags & HiddenAttribute); }
#if defined(Q_OS_WIN)
    bool isLnkFile() const                  { return (entryFlags & WinLnkType); }
#else
    bool isLnkFile() const                  { return false; }
#endif

    qint64 size() const                     { return size_; }

    QFile::Permissions permissions() const  { return QFile::Permissions(Permissions & entryFlags); }

    QDateTime creationTime() const;
    QDateTime modificationTime() const;
    QDateTime accessTime() const;

    QDateTime fileTime(QAbstractFileEngine::FileTime time) const;
    uint userId() const;
    uint groupId() const;
    uint ownerId(QAbstractFileEngine::FileOwner owner) const;

#ifdef Q_OS_UNIX
    void fillFromStatBuf(const QT_STATBUF &statBuffer);
    void fillFromDirEnt(const QT_DIRENT &statBuffer);
#endif
#ifdef Q_OS_SYMBIAN
    void fillFromTEntry(const TEntry& entry);
    void fillFromVolumeInfo(const TVolumeInfo& info);
#endif

#if defined(Q_OS_WIN)
    inline void fillFromFileAttribute(DWORD fileAttribute, bool isDriveRoot = false);
    inline void fillFromFindData(WIN32_FIND_DATA &findData, bool setLinkType = false, bool isDriveRoot = false);
    inline void fillFromFindInfo(BY_HANDLE_FILE_INFORMATION &fileInfo);
#endif
private:
    friend class QFileSystemEngine;

    MetaDataFlags knownFlagsMask;
    MetaDataFlags entryFlags;

    qint64 size_;

    // Platform-specific data goes here:
#if defined(Q_OS_WIN)
    DWORD fileAttribute_;
    FILETIME creationTime_;
    FILETIME lastAccessTime_;
    FILETIME lastWriteTime_;
#elif defined(Q_OS_SYMBIAN)
    TTime modificationTime_;
#else
    time_t creationTime_;
    time_t modificationTime_;
    time_t accessTime_;

    uint userId_;
    uint groupId_;
#endif

};

Q_DECLARE_OPERATORS_FOR_FLAGS(QFileSystemMetaData::MetaDataFlags)

#if !defined(QWS) && !defined(Q_WS_QPA) && defined(Q_OS_MAC)
inline bool QFileSystemMetaData::isBundle() const                   { return (entryFlags & BundleType); }
inline bool QFileSystemMetaData::isAlias() const                    { return (entryFlags & AliasType); }
#else
inline bool QFileSystemMetaData::isBundle() const                   { return false; }
inline bool QFileSystemMetaData::isAlias() const                    { return false; }
#endif

#if (defined(Q_OS_UNIX) && !defined (Q_OS_SYMBIAN)) || defined (Q_OS_WIN)
inline QDateTime QFileSystemMetaData::fileTime(QAbstractFileEngine::FileTime time) const
{
    switch (time) {
    case QAbstractFileEngine::ModificationTime:
        return modificationTime();

    case QAbstractFileEngine::AccessTime:
        return accessTime();

    case QAbstractFileEngine::CreationTime:
        return creationTime();
    }

    return QDateTime();
}
#endif

#if defined(Q_OS_UNIX) && !defined (Q_OS_SYMBIAN)
inline QDateTime QFileSystemMetaData::creationTime() const          { return QDateTime::fromTime_t(creationTime_); }
inline QDateTime QFileSystemMetaData::modificationTime() const      { return QDateTime::fromTime_t(modificationTime_); }
inline QDateTime QFileSystemMetaData::accessTime() const            { return QDateTime::fromTime_t(accessTime_); }

inline uint QFileSystemMetaData::userId() const                     { return userId_; }
inline uint QFileSystemMetaData::groupId() const                    { return groupId_; }

inline uint QFileSystemMetaData::ownerId(QAbstractFileEngine::FileOwner owner) const
{
    if (owner == QAbstractFileEngine::OwnerUser)
        return userId();
    else
        return groupId();
}
#endif

#ifdef Q_OS_SYMBIAN
inline QDateTime QFileSystemMetaData::creationTime() const          { return modificationTime(); }
inline QDateTime QFileSystemMetaData::modificationTime() const      { return qt_symbian_TTime_To_QDateTime(modificationTime_); }
inline QDateTime QFileSystemMetaData::accessTime() const            { return modificationTime(); }

inline QDateTime QFileSystemMetaData::fileTime(QAbstractFileEngine::FileTime time) const
{
    Q_UNUSED(time);
    return modificationTime();
}
inline uint QFileSystemMetaData::userId() const                     { return (uint) -2; }
inline uint QFileSystemMetaData::groupId() const                    { return (uint) -2; }
inline uint QFileSystemMetaData::ownerId(QAbstractFileEngine::FileOwner owner) const
{
    Q_UNUSED(owner);
    return (uint) -2;
}
#endif

#if defined(Q_OS_WIN)
inline uint QFileSystemMetaData::userId() const                     { return (uint) -2; }
inline uint QFileSystemMetaData::groupId() const                    { return (uint) -2; }
inline uint QFileSystemMetaData::ownerId(QAbstractFileEngine::FileOwner owner) const
{
    if (owner == QAbstractFileEngine::OwnerUser)
        return userId();
    else
        return groupId();
}

inline void QFileSystemMetaData::fillFromFileAttribute(DWORD fileAttribute,bool isDriveRoot)
{
    fileAttribute_ = fileAttribute;
    // Ignore the hidden attribute for drives.
    if (!isDriveRoot && (fileAttribute_ & FILE_ATTRIBUTE_HIDDEN))
        entryFlags |= HiddenAttribute;
    entryFlags |= ((fileAttribute & FILE_ATTRIBUTE_DIRECTORY) ? DirectoryType: FileType);
    entryFlags |= ExistsAttribute;
    knownFlagsMask |= FileType | DirectoryType | HiddenAttribute | ExistsAttribute;
}

inline void QFileSystemMetaData::fillFromFindData(WIN32_FIND_DATA &findData, bool setLinkType, bool isDriveRoot)
{
    fillFromFileAttribute(findData.dwFileAttributes, isDriveRoot);
    creationTime_ = findData.ftCreationTime;
    lastAccessTime_ = findData.ftLastAccessTime;
    lastWriteTime_ = findData.ftLastWriteTime;
    if (fileAttribute_ & FILE_ATTRIBUTE_DIRECTORY) {
        size_ = 0;
    } else {
        size_ = findData.nFileSizeHigh;
        size_ <<= 32;
        size_ += findData.nFileSizeLow;
    }
    knownFlagsMask |=  Times | SizeAttribute;
    if (setLinkType) {
        knownFlagsMask |=  LinkType;
        entryFlags &= ~LinkType;
#if !defined(Q_OS_WINCE)
        if ((fileAttribute_ & FILE_ATTRIBUTE_REPARSE_POINT)
            && findData.dwReserved0 == IO_REPARSE_TAG_SYMLINK) {
            entryFlags |= LinkType;
        }
#endif

    }
}

inline void QFileSystemMetaData::fillFromFindInfo(BY_HANDLE_FILE_INFORMATION &fileInfo)
{
    fillFromFileAttribute(fileInfo.dwFileAttributes);
    creationTime_ = fileInfo.ftCreationTime;
    lastAccessTime_ = fileInfo.ftLastAccessTime;
    lastWriteTime_ = fileInfo.ftLastWriteTime;
    if (fileAttribute_ & FILE_ATTRIBUTE_DIRECTORY) {
        size_ = 0;
    } else {
        size_ = fileInfo.nFileSizeHigh;
        size_ <<= 32;
        size_ += fileInfo.nFileSizeLow;
    }
    knownFlagsMask |=  Times | SizeAttribute;
}
#endif

QT_END_NAMESPACE

#endif // include guard
