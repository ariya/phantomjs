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

#include "qfilesystemengine_p.h"
#include "qfsfileengine.h"
#include <QtCore/private/qcore_symbian_p.h>
#include <QtCore/qcoreapplication.h>

#include <f32file.h>
#include <pathinfo.h>
#include <wchar.h>

QT_BEGIN_NAMESPACE

bool QFileSystemEngine::isCaseSensitive()
{
    return false;
}

//TODO: resolve this with QDir::cleanPath, without breaking the behaviour of that
//function which is documented only by autotest
//input: a dirty absolute path, e.g. c:/../../foo/./
//output: a clean absolute path, e.g. c:/foo/
static QString symbianCleanAbsolutePath(const QString& path)
{
    bool isDir = path.endsWith(QLatin1Char('/'));
    //using SkipEmptyParts flag to eliminate duplicated slashes
    QStringList components = path.split(QLatin1Char('/'), QString::SkipEmptyParts);
    int cdups = 0;
    for(int i=components.count() - 1; i>=0; --i) {
        if(components.at(i) == QLatin1String("..")) {
            components.removeAt(i);
            cdups++;
        }
        else if(components.at(i) == QLatin1String(".")) {
            components.removeAt(i);
        }
        else if(cdups && i > 0) {
            --cdups;
            components.removeAt(i);
        }
    }
    QString result = components.join(QLatin1String("/"));
    if ((isDir&& !result.endsWith(QLatin1Char('/')))
        || (result.length() == 2 && result.at(1).unicode() == ':'))
        result.append(QLatin1Char('/'));
    return result;
}

//static
QFileSystemEntry QFileSystemEngine::getLinkTarget(const QFileSystemEntry &link, QFileSystemMetaData &data)
{
    Q_UNUSED(data);
    return link;
}

//static
QFileSystemEntry QFileSystemEngine::canonicalName(const QFileSystemEntry &entry, QFileSystemMetaData &data)
{
    if (entry.isEmpty() || entry.isRoot())
        return entry;

    QFileSystemEntry result = absoluteName(entry);
    if (!data.hasFlags(QFileSystemMetaData::ExistsAttribute))
        fillMetaData(result, data, QFileSystemMetaData::ExistsAttribute);
    if (!data.exists()) {
        // file doesn't exist
        return QFileSystemEntry();
    } else {
        return result;
    }
}

//static
QFileSystemEntry QFileSystemEngine::absoluteName(const QFileSystemEntry &entry)
{
    QString orig = entry.filePath();
    const bool isAbsolute = entry.isAbsolute();
    const bool isDirty = !entry.isClean();
    if (isAbsolute && !isDirty)
        return entry;

    const bool isRelative = entry.isRelative();
    const bool needsDrive = (!orig.isEmpty() && orig.at(0).unicode() == '/');
    const bool isDriveLetter = !needsDrive && !isAbsolute && !isRelative && orig.length() == 2;
    const bool isDriveRelative = !needsDrive && !isAbsolute && !isRelative && orig.length() > 2;

    QString result;
    if (needsDrive || isDriveLetter || isDriveRelative || !isAbsolute || orig.isEmpty()) {
        QFileSystemEntry cur(currentPath());
        if(needsDrive)
            result = cur.filePath().left(2);
        else if(isDriveRelative && cur.filePath().at(0) != orig.at(0))
            result = orig.left(2); // for BC, see tst_QFileInfo::absolutePath(<not current drive>:my.dll)
        else
            result = cur.filePath();
        if(isDriveLetter) {
            result[0] = orig.at(0); //copy drive letter
            orig.clear();
        }
        if(isDriveRelative) {
            orig = orig.mid(2); //discard the drive specifier from orig
        }
    }
    if (!orig.isEmpty() && !(orig.length() == 1 && orig.at(0).unicode() == '.')) {
        if (!result.isEmpty() && !result.endsWith(QLatin1Char('/')))
            result.append(QLatin1Char('/'));
        result.append(orig);
    }

    return QFileSystemEntry(symbianCleanAbsolutePath(result), QFileSystemEntry::FromInternalPath());
}

void QFileSystemMetaData::fillFromTEntry(const TEntry& entry)
{
    entryFlags &= ~(QFileSystemMetaData::SymbianTEntryFlags);
    knownFlagsMask |= QFileSystemMetaData::SymbianTEntryFlags;
    //Symbian doesn't have unix type file permissions
    entryFlags |= QFileSystemMetaData::ReadPermissions;
    if(!entry.IsReadOnly()) {
        entryFlags |= QFileSystemMetaData::WritePermissions;
    }
    //set the type
    if(entry.IsDir())
        entryFlags |= (QFileSystemMetaData::DirectoryType | QFileSystemMetaData::ExecutePermissions);
    else
        entryFlags |= QFileSystemMetaData::FileType;

    //set the attributes
    entryFlags |= QFileSystemMetaData::ExistsAttribute;
    if(entry.IsHidden())
        entryFlags |= QFileSystemMetaData::HiddenAttribute;

#ifdef SYMBIAN_ENABLE_64_BIT_FILE_SERVER_API
    size_ = entry.FileSize();
#else
    size_ = (TUint)(entry.iSize);
#endif

    modificationTime_ = entry.iModified;
}

void QFileSystemMetaData::fillFromVolumeInfo(const TVolumeInfo& info)
{
    entryFlags &= ~(QFileSystemMetaData::SymbianTEntryFlags);
    knownFlagsMask |= QFileSystemMetaData::SymbianTEntryFlags;
    entryFlags |= QFileSystemMetaData::ExistsAttribute;
    entryFlags |= QFileSystemMetaData::Permissions;
    if(info.iDrive.iDriveAtt & KDriveAttRom) {
        entryFlags &= ~(QFileSystemMetaData::WritePermissions);
    }
    entryFlags |= QFileSystemMetaData::DirectoryType;
    size_ = info.iSize;
    modificationTime_ = qt_symbian_time_t_To_TTime(0);
}

//static
bool QFileSystemEngine::fillMetaData(const QFileSystemEntry &entry, QFileSystemMetaData &data, QFileSystemMetaData::MetaDataFlags what)
{
    if (what & QFileSystemMetaData::SymbianTEntryFlags) {
        RFs& fs(qt_s60GetRFs());
        TInt err;
        QFileSystemEntry absentry(absoluteName(entry));
        if (entry.isEmpty()) {
            err = KErrNotFound;
        } else if (absentry.isRoot()) {
            //Root directories don't have an entry, and Entry() returns KErrBadName.
            //Therefore get information about the volume instead.
            TInt drive;
            err = RFs::CharToDrive(TChar(absentry.nativeFilePath().at(0).unicode()), drive);
            if (!err) {
                TVolumeInfo info;
                err = fs.Volume(info, drive);
                if (!err)
                    data.fillFromVolumeInfo(info);
            }
        } else {
            TEntry ent;
            err = fs.Entry(qt_QString2TPtrC(absentry.nativeFilePath()), ent);
            if (!err)
                data.fillFromTEntry(ent);
        }
        if (err) {
            data.size_ = 0;
            data.modificationTime_ = TTime(0);
            data.entryFlags &= ~(QFileSystemMetaData::SymbianTEntryFlags);
        }
        //files in /sys/bin on any drive are executable, even though we don't normally have permission to check whether they exist or not
        if(absentry.filePath().midRef(1,10).compare(QLatin1String(":/sys/bin/"), Qt::CaseInsensitive) == 0)
            data.entryFlags |= QFileSystemMetaData::ExecutePermissions;
    }
    return data.hasFlags(what);
}

//static
bool QFileSystemEngine::createDirectory(const QFileSystemEntry &entry, bool createParents)
{
    QString abspath = absoluteName(entry).nativeFilePath();
    if (!abspath.endsWith(QLatin1Char('\\')))
        abspath.append(QLatin1Char('\\'));
    TInt r;
    TPtrC symPath(qt_QString2TPtrC(abspath));
    if (createParents)
        r = qt_s60GetRFs().MkDirAll(symPath);
    else
        r = qt_s60GetRFs().MkDir(symPath);
    if (createParents && r == KErrAlreadyExists)
        return true; //# Qt5 - QDir::mkdir returns false for existing dir, QDir::mkpath returns true (should be made consistent in Qt 5)
    if (createParents && r == KErrPermissionDenied) {
        // check for already exists, which is not returned from RFs when it denies permission
        TEntry entry;
        if (qt_s60GetRFs().Entry(symPath, entry) == KErrNone)
            r = KErrNone;
    }
    return (r == KErrNone);
}

//static
bool QFileSystemEngine::removeDirectory(const QFileSystemEntry &entry, bool removeEmptyParents)
{
    QString abspath = absoluteName(entry).nativeFilePath();
    if (!abspath.endsWith(QLatin1Char('\\')))
        abspath.append(QLatin1Char('\\'));
    TPtrC dir(qt_QString2TPtrC(abspath));
    RFs& fs = qt_s60GetRFs();
    bool ok = false;
    //behaviour of FS file engine:
    //returns true if the directory could be removed
    //success/failure of removing parent directories does not matter
    while (KErrNone == fs.RmDir(dir)) {
        ok = true;
        if (!removeEmptyParents)
            break;
        //RFs::RmDir treats "c:\foo\bar" and "c:\foo\" the same, so it is sufficient to remove the last \ to the end
        dir.Set(dir.Left(dir.LocateReverse(TChar('\\'))));
    }
    return ok;
}

//static
bool QFileSystemEngine::createLink(const QFileSystemEntry &source, const QFileSystemEntry &target, QSystemError &error)
{
    Q_UNUSED(source)
    Q_UNUSED(target)
    error = QSystemError(KErrNotSupported, QSystemError::NativeError);
    return false;
}

//static
bool QFileSystemEngine::copyFile(const QFileSystemEntry &source, const QFileSystemEntry &target, QSystemError &error)
{
    //CFileMan is allocated each time because it is not thread-safe
    CFileMan *fm = 0;
    TRAPD(err, fm = CFileMan::NewL(qt_s60GetRFs()));
    if (err == KErrNone) {
        err = fm->Copy(qt_QString2TPtrC(absoluteName(source).nativeFilePath()), qt_QString2TPtrC(absoluteName(target).nativeFilePath()), 0);
        delete fm;
    }
    if (err == KErrNone)
        return true;
    error = QSystemError(err, QSystemError::NativeError);
    return false;
}

//static
bool QFileSystemEngine::renameFile(const QFileSystemEntry &source, const QFileSystemEntry &target, QSystemError &error)
{
    QString sourcepath = absoluteName(source).nativeFilePath();
    QString targetpath = absoluteName(target).nativeFilePath();
    RFs& fs(qt_s60GetRFs());
    TInt err = fs.Rename(qt_QString2TPtrC(sourcepath), qt_QString2TPtrC(targetpath));
    if (err == KErrNone)
        return true;
    error = QSystemError(err, QSystemError::NativeError);
    return false;
}

//static
bool QFileSystemEngine::removeFile(const QFileSystemEntry &entry, QSystemError &error)
{
    QString targetpath = absoluteName(entry).nativeFilePath();
    RFs& fs(qt_s60GetRFs());
    TInt err = fs.Delete(qt_QString2TPtrC(targetpath));
    if (err == KErrNone)
        return true;
    error = QSystemError(err, QSystemError::NativeError);
    return false;
}

//static
bool QFileSystemEngine::setPermissions(const QFileSystemEntry &entry, QFile::Permissions permissions, QSystemError &error, QFileSystemMetaData *data)
{
    QString targetpath = absoluteName(entry).nativeFilePath();
    TUint setmask = 0;
    TUint clearmask = 0;
    RFs& fs(qt_s60GetRFs());
    if (permissions & (QFile::WriteOwner | QFile::WriteUser | QFile::WriteGroup | QFile::WriteOther))
        clearmask = KEntryAttReadOnly; //if anyone can write, it's not read-only
    else
        setmask = KEntryAttReadOnly;
    TInt err = fs.SetAtt(qt_QString2TPtrC(targetpath), setmask, clearmask);
    if (data && !err) {
        data->entryFlags &= ~QFileSystemMetaData::Permissions;
        data->entryFlags |= QFileSystemMetaData::MetaDataFlag(uint(permissions));
        data->knownFlagsMask |= QFileSystemMetaData::Permissions;
    }
    if (err == KErrNone)
        return true;
    error = QSystemError(err, QSystemError::NativeError);
    return false;
}

QString QFileSystemEngine::homePath()
{
    QString home = QDir::fromNativeSeparators(qt_TDesC2QString(PathInfo::PhoneMemoryRootPath()));
    if(home.endsWith(QLatin1Char('/')))
        home.chop(1);
    return home;
}

QString QFileSystemEngine::rootPath()
{
    TChar drive;
    TInt err = RFs::DriveToChar(RFs::GetSystemDrive(), drive); //RFs::GetSystemDriveChar not supported on S60 3.1
    Q_ASSERT(err == KErrNone); //RFs::GetSystemDrive() shall always return a convertible drive number on a valid OS configuration
    return QString(QChar(drive)).append(QLatin1String(":/"));
}

QString QFileSystemEngine::tempPath()
{
    return rootPath().append(QLatin1String("system/temp"));
}

//static
bool QFileSystemEngine::setCurrentPath(const QFileSystemEntry &entry)
{
    QFileSystemMetaData meta;
    QFileSystemEntry absname = absoluteName(entry);
    fillMetaData(absname, meta, QFileSystemMetaData::ExistsAttribute | QFileSystemMetaData::DirectoryType);
    if(!(meta.exists() && meta.isDirectory()))
        return false;

    RFs& fs = qt_s60GetRFs();
    QString abspath = absname.nativeFilePath();
    if(!abspath.endsWith(QLatin1Char('\\')))
        abspath.append(QLatin1Char('\\'));
    TInt r = fs.SetSessionPath(qt_QString2TPtrC(abspath));
    //SetSessionPath succeeds for non existent directory, which is why it's checked above
    if (r == KErrNone) {
        __ASSERT_COMPILE(sizeof(wchar_t) == sizeof(unsigned short));
        //attempt to set open C to the same path
        r = ::wchdir(reinterpret_cast<const wchar_t *>(absname.filePath().utf16()));
        if (r < 0)
            qWarning("failed to sync path to open C");
        return true;
    }
    return false;
}

//static
QFileSystemEntry QFileSystemEngine::currentPath()
{
    TFileName fn;
    QFileSystemEntry ret;
    TInt r = qt_s60GetRFs().SessionPath(fn);
    if(r == KErrNone) {
        //remove terminating slash from non root paths (session path is clean, absolute and always ends in a \)
        if(fn.Length() > 3 && fn[fn.Length() - 1] == '\\')
            fn.SetLength(fn.Length() - 1);
        ret = QFileSystemEntry(qt_TDesC2QString(fn), QFileSystemEntry::FromNativePath());
    }
    return ret;
}

QT_END_NAMESPACE
