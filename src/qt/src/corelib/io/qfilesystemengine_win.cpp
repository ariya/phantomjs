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

#define _POSIX_
#include "qplatformdefs.h"
#include "qabstractfileengine.h"
#include "private/qfsfileengine_p.h"
#include <private/qsystemlibrary_p.h>
#include <qdebug.h>

#include "qfile.h"
#include "qdir.h"
#include "private/qmutexpool_p.h"
#include "qvarlengtharray.h"
#include "qdatetime.h"
#include "qt_windows.h"

#if !defined(Q_OS_WINCE)
#  include <sys/types.h>
#  include <direct.h>
#  include <winioctl.h>
#else
#  include <types.h>
#endif
#include <objbase.h>
#include <shlobj.h>
#include <initguid.h>
#include <accctrl.h>
#include <ctype.h>
#include <limits.h>
#define SECURITY_WIN32
#include <security.h>

#ifndef SPI_GETPLATFORMTYPE
#define SPI_GETPLATFORMTYPE 257
#endif

#ifndef PATH_MAX
#define PATH_MAX FILENAME_MAX
#endif

#ifndef _INTPTR_T_DEFINED
#ifdef  _WIN64
typedef __int64             intptr_t;
#else
#ifdef _W64
typedef _W64 int            intptr_t;
#else
typedef INT_PTR intptr_t;
#endif
#endif
#define _INTPTR_T_DEFINED
#endif

#ifndef INVALID_FILE_ATTRIBUTES
#  define INVALID_FILE_ATTRIBUTES (DWORD (-1))
#endif

#if !defined(Q_OS_WINCE)
#  if !defined(REPARSE_DATA_BUFFER_HEADER_SIZE)
typedef struct _REPARSE_DATA_BUFFER {
    ULONG  ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    union {
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            ULONG  Flags;
            WCHAR  PathBuffer[1];
        } SymbolicLinkReparseBuffer;
        struct {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            WCHAR  PathBuffer[1];
        } MountPointReparseBuffer;
        struct {
            UCHAR  DataBuffer[1];
        } GenericReparseBuffer;
    };
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;
#    define REPARSE_DATA_BUFFER_HEADER_SIZE  FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)
#  endif // !defined(REPARSE_DATA_BUFFER_HEADER_SIZE)

#  ifndef MAXIMUM_REPARSE_DATA_BUFFER_SIZE
#    define MAXIMUM_REPARSE_DATA_BUFFER_SIZE 16384
#  endif
#  ifndef IO_REPARSE_TAG_SYMLINK
#    define IO_REPARSE_TAG_SYMLINK (0xA000000CL)
#  endif
#  ifndef FSCTL_GET_REPARSE_POINT
#    define FSCTL_GET_REPARSE_POINT CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 42, METHOD_BUFFERED, FILE_ANY_ACCESS)
#  endif
#endif // !defined(Q_OS_WINCE)

QT_BEGIN_NAMESPACE

Q_CORE_EXPORT int qt_ntfs_permission_lookup = 0;

#if defined(Q_OS_WINCE)
static QString qfsPrivateCurrentDir = QLatin1String("");
// As none of the functions we try to resolve do exist on Windows CE
// we use QT_NO_LIBRARY to shorten everything up a little bit.
#define QT_NO_LIBRARY 1
#endif

#if !defined(QT_NO_LIBRARY)
QT_BEGIN_INCLUDE_NAMESPACE
typedef DWORD (WINAPI *PtrGetNamedSecurityInfoW)(LPWSTR, SE_OBJECT_TYPE, SECURITY_INFORMATION, PSID*, PSID*, PACL*, PACL*, PSECURITY_DESCRIPTOR*);
static PtrGetNamedSecurityInfoW ptrGetNamedSecurityInfoW = 0;
typedef BOOL (WINAPI *PtrLookupAccountSidW)(LPCWSTR, PSID, LPWSTR, LPDWORD, LPWSTR, LPDWORD, PSID_NAME_USE);
static PtrLookupAccountSidW ptrLookupAccountSidW = 0;
typedef VOID (WINAPI *PtrBuildTrusteeWithSidW)(PTRUSTEE_W, PSID);
static PtrBuildTrusteeWithSidW ptrBuildTrusteeWithSidW = 0;
typedef DWORD (WINAPI *PtrGetEffectiveRightsFromAclW)(PACL, PTRUSTEE_W, OUT PACCESS_MASK);
static PtrGetEffectiveRightsFromAclW ptrGetEffectiveRightsFromAclW = 0;
typedef BOOL (WINAPI *PtrGetUserProfileDirectoryW)(HANDLE, LPWSTR, LPDWORD);
static PtrGetUserProfileDirectoryW ptrGetUserProfileDirectoryW = 0;
typedef BOOL (WINAPI *PtrGetVolumePathNamesForVolumeNameW)(LPCWSTR,LPWSTR,DWORD,PDWORD);
static PtrGetVolumePathNamesForVolumeNameW ptrGetVolumePathNamesForVolumeNameW = 0;
QT_END_INCLUDE_NAMESPACE

static TRUSTEE_W currentUserTrusteeW;
static TRUSTEE_W worldTrusteeW;
static PSID currentUserSID = 0;
static PSID worldSID = 0;

/*
    Deletes the allocated SIDs during global static cleanup
*/
class SidCleanup
{
public:
    ~SidCleanup();
};

SidCleanup::~SidCleanup()
{
    qFree(currentUserSID);
    currentUserSID = 0;

    // worldSID was allocated with AllocateAndInitializeSid so it needs to be freed with FreeSid
    if (worldSID) {
        ::FreeSid(worldSID);
        worldSID = 0;
    }
}

Q_GLOBAL_STATIC(SidCleanup, initSidCleanup)

static void resolveLibs()
{
    static bool triedResolve = false;
    if (!triedResolve) {
        // need to resolve the security info functions

        // protect initialization
#ifndef QT_NO_THREAD
        QMutexLocker locker(QMutexPool::globalInstanceGet(&triedResolve));
        // check triedResolve again, since another thread may have already
        // done the initialization
        if (triedResolve) {
            // another thread did initialize the security function pointers,
            // so we shouldn't do it again.
            return;
        }
#endif

#if !defined(Q_OS_WINCE)
        QSystemLibrary advapi32(QLatin1String("advapi32"));
        if (advapi32.load()) {
            ptrGetNamedSecurityInfoW = (PtrGetNamedSecurityInfoW)advapi32.resolve("GetNamedSecurityInfoW");
            ptrLookupAccountSidW = (PtrLookupAccountSidW)advapi32.resolve("LookupAccountSidW");
            ptrBuildTrusteeWithSidW = (PtrBuildTrusteeWithSidW)advapi32.resolve("BuildTrusteeWithSidW");
            ptrGetEffectiveRightsFromAclW = (PtrGetEffectiveRightsFromAclW)advapi32.resolve("GetEffectiveRightsFromAclW");
        }
        if (ptrBuildTrusteeWithSidW) {
            // Create TRUSTEE for current user
            HANDLE hnd = ::GetCurrentProcess();
            HANDLE token = 0;
            initSidCleanup();
            if (::OpenProcessToken(hnd, TOKEN_QUERY, &token)) {
                DWORD retsize = 0;
                // GetTokenInformation requires a buffer big enough for the TOKEN_USER struct and
                // the SID struct. Since the SID struct can have variable number of subauthorities
                // tacked at the end, its size is variable. Obtain the required size by first
                // doing a dummy GetTokenInformation call.
                ::GetTokenInformation(token, TokenUser, 0, 0, &retsize);
                if (retsize) {
                    void *tokenBuffer = qMalloc(retsize);
                    if (::GetTokenInformation(token, TokenUser, tokenBuffer, retsize, &retsize)) {
                        PSID tokenSid = reinterpret_cast<PTOKEN_USER>(tokenBuffer)->User.Sid;
                        DWORD sidLen = ::GetLengthSid(tokenSid);
                        currentUserSID = reinterpret_cast<PSID>(qMalloc(sidLen));
                        if (::CopySid(sidLen, currentUserSID, tokenSid))
                            ptrBuildTrusteeWithSidW(&currentUserTrusteeW, currentUserSID);
                    }
                    qFree(tokenBuffer);
                }
                ::CloseHandle(token);
            }

            typedef BOOL (WINAPI *PtrAllocateAndInitializeSid)(PSID_IDENTIFIER_AUTHORITY, BYTE, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, PSID*);
            PtrAllocateAndInitializeSid ptrAllocateAndInitializeSid = (PtrAllocateAndInitializeSid)advapi32.resolve("AllocateAndInitializeSid");
            if (ptrAllocateAndInitializeSid) {
                // Create TRUSTEE for Everyone (World)
                SID_IDENTIFIER_AUTHORITY worldAuth = { SECURITY_WORLD_SID_AUTHORITY };
                if (ptrAllocateAndInitializeSid(&worldAuth, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &worldSID))
                    ptrBuildTrusteeWithSidW(&worldTrusteeW, worldSID);
            }
        }

        QSystemLibrary userenv(QLatin1String("userenv"));
        if (userenv.load())
            ptrGetUserProfileDirectoryW = (PtrGetUserProfileDirectoryW)userenv.resolve("GetUserProfileDirectoryW");

        QSystemLibrary kernel32(QLatin1String("kernel32"));
        if (kernel32.load())
            ptrGetVolumePathNamesForVolumeNameW = (PtrGetVolumePathNamesForVolumeNameW)kernel32.resolve("GetVolumePathNamesForVolumeNameW");
#endif
        triedResolve = true;
    }
}
#endif // QT_NO_LIBRARY

typedef DWORD (WINAPI *PtrNetShareEnum)(LPWSTR, DWORD, LPBYTE*, DWORD, LPDWORD, LPDWORD, LPDWORD);
static PtrNetShareEnum ptrNetShareEnum = 0;
typedef DWORD (WINAPI *PtrNetApiBufferFree)(LPVOID);
static PtrNetApiBufferFree ptrNetApiBufferFree = 0;
typedef struct _SHARE_INFO_1 {
    LPWSTR shi1_netname;
    DWORD shi1_type;
    LPWSTR shi1_remark;
} SHARE_INFO_1;


static bool resolveUNCLibs()
{
    static bool triedResolve = false;
    if (!triedResolve) {
#ifndef QT_NO_THREAD
        QMutexLocker locker(QMutexPool::globalInstanceGet(&triedResolve));
        if (triedResolve) {
            return ptrNetShareEnum && ptrNetApiBufferFree;
        }
#endif

#if !defined(Q_OS_WINCE)
        QSystemLibrary netapi32(QLatin1String("Netapi32"));
        if (netapi32.load()) {
            ptrNetShareEnum = (PtrNetShareEnum)netapi32.resolve("NetShareEnum");
            ptrNetApiBufferFree = (PtrNetApiBufferFree)netapi32.resolve("NetApiBufferFree");
        }
#endif
        triedResolve = true;
    }
    return ptrNetShareEnum && ptrNetApiBufferFree;
}

static QString readSymLink(const QFileSystemEntry &link)
{
    QString result;
#if !defined(Q_OS_WINCE)
    HANDLE handle = CreateFile((wchar_t*)link.nativeFilePath().utf16(),
                               FILE_READ_EA,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               0,
                               OPEN_EXISTING,
                               FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                               0);
    if (handle != INVALID_HANDLE_VALUE) {
        DWORD bufsize = MAXIMUM_REPARSE_DATA_BUFFER_SIZE;
        REPARSE_DATA_BUFFER *rdb = (REPARSE_DATA_BUFFER*)qMalloc(bufsize);
        DWORD retsize = 0;
        if (::DeviceIoControl(handle, FSCTL_GET_REPARSE_POINT, 0, 0, rdb, bufsize, &retsize, 0)) {
            if (rdb->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {
                int length = rdb->MountPointReparseBuffer.SubstituteNameLength / sizeof(wchar_t);
                int offset = rdb->MountPointReparseBuffer.SubstituteNameOffset / sizeof(wchar_t);
                const wchar_t* PathBuffer = &rdb->MountPointReparseBuffer.PathBuffer[offset];
                result = QString::fromWCharArray(PathBuffer, length);
            } else if (rdb->ReparseTag == IO_REPARSE_TAG_SYMLINK) {
                int length = rdb->SymbolicLinkReparseBuffer.SubstituteNameLength / sizeof(wchar_t);
                int offset = rdb->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(wchar_t);
                const wchar_t* PathBuffer = &rdb->SymbolicLinkReparseBuffer.PathBuffer[offset];
                result = QString::fromWCharArray(PathBuffer, length);
            }
            // cut-off "//?/" and "/??/"
            if (result.size() > 4 && result.at(0) == QLatin1Char('\\') && result.at(2) == QLatin1Char('?') && result.at(3) == QLatin1Char('\\'))
                result = result.mid(4);
        }
        qFree(rdb);
        CloseHandle(handle);

#if !defined(QT_NO_LIBRARY)
        resolveLibs();
        if (ptrGetVolumePathNamesForVolumeNameW) {
            QRegExp matchVolName(QLatin1String("^Volume\\{([a-z]|[0-9]|-)+\\}\\\\"), Qt::CaseInsensitive);
            if(matchVolName.indexIn(result) == 0) {
                DWORD len;
                wchar_t buffer[MAX_PATH];
                QString volumeName = result.mid(0, matchVolName.matchedLength()).prepend(QLatin1String("\\\\?\\"));
                if(ptrGetVolumePathNamesForVolumeNameW((wchar_t*)volumeName.utf16(), buffer, MAX_PATH, &len) != 0)
                    result.replace(0,matchVolName.matchedLength(), QString::fromWCharArray(buffer));
            }
        }
#endif
    }
#else
    Q_UNUSED(link);
#endif // Q_OS_WINCE
    return result;
}

static QString readLink(const QFileSystemEntry &link)
{
#if !defined(Q_OS_WINCE)
#if !defined(QT_NO_LIBRARY) && !defined(Q_CC_MWERKS)
    QString ret;

    bool neededCoInit = false;
    IShellLink *psl;                            // pointer to IShellLink i/f
    WIN32_FIND_DATA wfd;
    wchar_t szGotPath[MAX_PATH];

    // Get pointer to the IShellLink interface.
    HRESULT hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *)&psl);

    if (hres == CO_E_NOTINITIALIZED) { // COM was not initialized
        neededCoInit = true;
        CoInitialize(NULL);
        hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                    IID_IShellLink, (LPVOID *)&psl);
    }
    if (SUCCEEDED(hres)) {    // Get pointer to the IPersistFile interface.
        IPersistFile *ppf;
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID *)&ppf);
        if (SUCCEEDED(hres))  {
            hres = ppf->Load((LPOLESTR)link.nativeFilePath().utf16(), STGM_READ);
            //The original path of the link is retrieved. If the file/folder
            //was moved, the return value still have the old path.
            if (SUCCEEDED(hres)) {
                if (psl->GetPath(szGotPath, MAX_PATH, &wfd, SLGP_UNCPRIORITY) == NOERROR)
                    ret = QString::fromWCharArray(szGotPath);
            }
            ppf->Release();
        }
        psl->Release();
    }
    if (neededCoInit)
        CoUninitialize();

    return ret;
#else
    Q_UNUSED(link);
    return QString();
#endif // QT_NO_LIBRARY
#else
    wchar_t target[MAX_PATH];
    QString result;
    if (SHGetShortcutTarget((wchar_t*)QFileInfo(link.filePath()).absoluteFilePath().replace(QLatin1Char('/'),QLatin1Char('\\')).utf16(), target, MAX_PATH)) {
        result = QString::fromWCharArray(target);
        if (result.startsWith(QLatin1Char('"')))
            result.remove(0,1);
        if (result.endsWith(QLatin1Char('"')))
            result.remove(result.size()-1,1);
    }
    return result;
#endif // Q_OS_WINCE
}

static bool uncShareExists(const QString &server)
{
    // This code assumes the UNC path is always like \\?\UNC\server...
    QStringList parts = server.split(QLatin1Char('\\'), QString::SkipEmptyParts);
    if (parts.count() >= 3) {
        QStringList shares;
        if (QFileSystemEngine::uncListSharesOnServer(QLatin1String("\\\\") + parts.at(2), &shares))
            return parts.count() >= 4 ? shares.contains(parts.at(3), Qt::CaseInsensitive) : true;
    }
    return false;
}

static inline bool getFindData(QString path, WIN32_FIND_DATA &findData)
{
    // path should not end with a trailing slash
    while (path.endsWith(QLatin1Char('\\')))
        path.chop(1);

    // can't handle drives
    if (!path.endsWith(QLatin1Char(':'))) {
        HANDLE hFind = ::FindFirstFile((wchar_t*)path.utf16(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            ::FindClose(hFind);
            return true;
        }
    }

    return false;
}

bool QFileSystemEngine::uncListSharesOnServer(const QString &server, QStringList *list)
{
    if (resolveUNCLibs()) {
        SHARE_INFO_1 *BufPtr, *p;
        DWORD res;
        DWORD er = 0, tr = 0, resume = 0, i;
        do {
            res = ptrNetShareEnum((wchar_t*)server.utf16(), 1, (LPBYTE *)&BufPtr, DWORD(-1), &er, &tr, &resume);
            if (res == ERROR_SUCCESS || res == ERROR_MORE_DATA) {
                p = BufPtr;
                for (i = 1; i <= er; ++i) {
                    if (list && p->shi1_type == 0)
                        list->append(QString::fromWCharArray(p->shi1_netname));
                    p++;
                }
            }
            ptrNetApiBufferFree(BufPtr);
        } while (res == ERROR_MORE_DATA);
        return res == ERROR_SUCCESS;
    }
    return false;
}

void QFileSystemEngine::clearWinStatData(QFileSystemMetaData &data)
{
    data.size_ = 0;
    data.fileAttribute_ =  0;
    data.creationTime_ = FILETIME();
    data.lastAccessTime_ = FILETIME();
    data.lastWriteTime_ = FILETIME();
}

bool QFileSystemEngine::isCaseSensitive()
{
    return false;
}

//static
QFileSystemEntry QFileSystemEngine::getLinkTarget(const QFileSystemEntry &link,
                                                  QFileSystemMetaData &data)
{
   if (data.missingFlags(QFileSystemMetaData::LinkType))
       QFileSystemEngine::fillMetaData(link, data, QFileSystemMetaData::LinkType);

    QString ret;
    if (data.isLnkFile())
        ret = readLink(link);
    else if (data.isLink())
        ret = readSymLink(link);
    return QFileSystemEntry(ret);
}

//static
QFileSystemEntry QFileSystemEngine::canonicalName(const QFileSystemEntry &entry, QFileSystemMetaData &data)
{
    if (data.missingFlags(QFileSystemMetaData::ExistsAttribute))
       QFileSystemEngine::fillMetaData(entry, data, QFileSystemMetaData::ExistsAttribute);

    if (data.exists())
        return QFileSystemEntry(slowCanonicalized(absoluteName(entry).filePath()));
    else
        return QFileSystemEntry();
}

//static
QString QFileSystemEngine::nativeAbsoluteFilePath(const QString &path)
{
    // can be //server or //server/share
    QString absPath;
#if !defined(Q_OS_WINCE)
    QVarLengthArray<wchar_t, MAX_PATH> buf(qMax(MAX_PATH, path.size() + 1));
    wchar_t *fileName = 0;
    DWORD retLen = GetFullPathName((wchar_t*)path.utf16(), buf.size(), buf.data(), &fileName);
    if (retLen > (DWORD)buf.size()) {
        buf.resize(retLen);
        retLen = GetFullPathName((wchar_t*)path.utf16(), buf.size(), buf.data(), &fileName);
    }
    if (retLen != 0)
        absPath = QString::fromWCharArray(buf.data(), retLen);
#else
    if (path.startsWith(QLatin1Char('/')) || path.startsWith(QLatin1Char('\\')))
        absPath = QDir::toNativeSeparators(path);
    else
        absPath = QDir::toNativeSeparators(QDir::cleanPath(qfsPrivateCurrentDir + QLatin1Char('/') + path));
#endif
    // This is really ugly, but GetFullPathName strips off whitespace at the end.
    // If you for instance write ". " in the lineedit of QFileDialog,
    // (which is an invalid filename) this function will strip the space off and viola,
    // the file is later reported as existing. Therefore, we re-add the whitespace that
    // was at the end of path in order to keep the filename invalid.
    if (!path.isEmpty() && path.at(path.size() - 1) == QLatin1Char(' '))
        absPath.append(QLatin1Char(' '));
    return absPath;
}

//static
QFileSystemEntry QFileSystemEngine::absoluteName(const QFileSystemEntry &entry)
{
    QString ret;

    if (!entry.isRelative()) {
#if !defined(Q_OS_WINCE)
        if (entry.isAbsolute() && entry.isClean()) {
            ret = entry.filePath();
        }  else {
            ret = QDir::fromNativeSeparators(nativeAbsoluteFilePath(entry.filePath()));
        }
#else
        ret = entry.filePath();
#endif
    } else {
        ret = QDir::cleanPath(QDir::currentPath() + QLatin1Char('/') + entry.filePath());
    }

    // The path should be absolute at this point.
    // From the docs :
    // Absolute paths begin with the directory separator "/"
    // (optionally preceded by a drive specification under Windows).
    if (ret.at(0) != QLatin1Char('/')) {
        Q_ASSERT(ret.length() >= 2);
        Q_ASSERT(ret.at(0).isLetter());
        Q_ASSERT(ret.at(1) == QLatin1Char(':'));

        // Force uppercase drive letters.
        ret[0] = ret.at(0).toUpper();
    }
    return QFileSystemEntry(ret, QFileSystemEntry::FromInternalPath());
}

//static
QString QFileSystemEngine::owner(const QFileSystemEntry &entry, QAbstractFileEngine::FileOwner own)
{
    QString name;
#if !defined(QT_NO_LIBRARY)
    extern int qt_ntfs_permission_lookup;
    if((qt_ntfs_permission_lookup > 0) && (QSysInfo::WindowsVersion & QSysInfo::WV_NT_based)) {
        resolveLibs();
        if (ptrGetNamedSecurityInfoW && ptrLookupAccountSidW) {
            PSID pOwner = 0;
            PSECURITY_DESCRIPTOR pSD;
            if (ptrGetNamedSecurityInfoW((wchar_t*)entry.nativeFilePath().utf16(), SE_FILE_OBJECT,
                                         own == QAbstractFileEngine::OwnerGroup ? GROUP_SECURITY_INFORMATION : OWNER_SECURITY_INFORMATION,
                                         own == QAbstractFileEngine::OwnerUser ? &pOwner : 0, own == QAbstractFileEngine::OwnerGroup ? &pOwner : 0,
                                         0, 0, &pSD) == ERROR_SUCCESS) {
                DWORD lowner = 64;
                DWORD ldomain = 64;
                QVarLengthArray<wchar_t, 64> owner(lowner);
                QVarLengthArray<wchar_t, 64> domain(ldomain);
                SID_NAME_USE use = SidTypeUnknown;
                // First call, to determine size of the strings (with '\0').
                if (!ptrLookupAccountSidW(NULL, pOwner, (LPWSTR)owner.data(), &lowner,
                                          (LPWSTR)domain.data(), &ldomain, (SID_NAME_USE*)&use)) {
                    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                        if (lowner > (DWORD)owner.size())
                            owner.resize(lowner);
                        if (ldomain > (DWORD)domain.size())
                            domain.resize(ldomain);
                        // Second call, try on resized buf-s
                        if (!ptrLookupAccountSidW(NULL, pOwner, (LPWSTR)owner.data(), &lowner,
                                                  (LPWSTR)domain.data(), &ldomain, (SID_NAME_USE*)&use)) {
                            lowner = 0;
                        }
                    } else {
                        lowner = 0;
                    }
                }
                if (lowner != 0)
                    name = QString::fromWCharArray(owner.data());
                LocalFree(pSD);
            }
        }
    }
#else
    Q_UNUSED(entry);
    Q_UNUSED(own);
#endif
    return name;
}

//static
bool QFileSystemEngine::fillPermissions(const QFileSystemEntry &entry, QFileSystemMetaData &data,
                                        QFileSystemMetaData::MetaDataFlags what)
{
#if !defined(QT_NO_LIBRARY)
    if((qt_ntfs_permission_lookup > 0) && (QSysInfo::WindowsVersion & QSysInfo::WV_NT_based)) {
        resolveLibs();
        if(ptrGetNamedSecurityInfoW && ptrBuildTrusteeWithSidW && ptrGetEffectiveRightsFromAclW) {
            enum { ReadMask = 0x00000001, WriteMask = 0x00000002, ExecMask = 0x00000020 };

            QString fname = entry.filePath();
            PSID pOwner = 0;
            PSID pGroup = 0;
            PACL pDacl;
            PSECURITY_DESCRIPTOR pSD;
            DWORD res = ptrGetNamedSecurityInfoW((wchar_t*)fname.utf16(), SE_FILE_OBJECT,
                                                 OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                                 &pOwner, &pGroup, &pDacl, 0, &pSD);
            if(res == ERROR_SUCCESS) {
                ACCESS_MASK access_mask;
                TRUSTEE_W trustee;
                if (what & QFileSystemMetaData::UserPermissions) { // user
                    data.knownFlagsMask |= QFileSystemMetaData::UserPermissions;
                    if(ptrGetEffectiveRightsFromAclW(pDacl, &currentUserTrusteeW, &access_mask) != ERROR_SUCCESS)
                        access_mask = (ACCESS_MASK)-1;
                    if(access_mask & ReadMask)
                        data.entryFlags |= QFileSystemMetaData::UserReadPermission;
                    if(access_mask & WriteMask)
                        data.entryFlags|= QFileSystemMetaData::UserWritePermission;
                    if(access_mask & ExecMask)
                        data.entryFlags|= QFileSystemMetaData::UserExecutePermission;
                }
                if (what & QFileSystemMetaData::OwnerPermissions) { // owner
                    data.knownFlagsMask |= QFileSystemMetaData::OwnerPermissions;
                    ptrBuildTrusteeWithSidW(&trustee, pOwner);
                    if(ptrGetEffectiveRightsFromAclW(pDacl, &trustee, &access_mask) != ERROR_SUCCESS)
                        access_mask = (ACCESS_MASK)-1;
                    if(access_mask & ReadMask)
                        data.entryFlags |= QFileSystemMetaData::OwnerReadPermission;
                    if(access_mask & WriteMask)
                        data.entryFlags |= QFileSystemMetaData::OwnerWritePermission;
                    if(access_mask & ExecMask)
                        data.entryFlags |= QFileSystemMetaData::OwnerExecutePermission;
                }
                if (what & QFileSystemMetaData::GroupPermissions) { // group
                    data.knownFlagsMask |= QFileSystemMetaData::GroupPermissions;
                    ptrBuildTrusteeWithSidW(&trustee, pGroup);
                    if(ptrGetEffectiveRightsFromAclW(pDacl, &trustee, &access_mask) != ERROR_SUCCESS)
                        access_mask = (ACCESS_MASK)-1;
                    if(access_mask & ReadMask)
                        data.entryFlags |= QFileSystemMetaData::GroupReadPermission;
                    if(access_mask & WriteMask)
                        data.entryFlags |= QFileSystemMetaData::GroupWritePermission;
                    if(access_mask & ExecMask)
                        data.entryFlags |= QFileSystemMetaData::GroupExecutePermission;
                }
                if (what & QFileSystemMetaData::OtherPermissions) { // other (world)
                    data.knownFlagsMask |= QFileSystemMetaData::OtherPermissions;
                    if(ptrGetEffectiveRightsFromAclW(pDacl, &worldTrusteeW, &access_mask) != ERROR_SUCCESS)
                        access_mask = (ACCESS_MASK)-1; // ###
                    if(access_mask & ReadMask)
                        data.entryFlags |= QFileSystemMetaData::OtherReadPermission;
                    if(access_mask & WriteMask)
                        data.entryFlags |= QFileSystemMetaData::OtherWritePermission;
                    if(access_mask & ExecMask)
                        data.entryFlags |= QFileSystemMetaData::OwnerExecutePermission;
                }
                LocalFree(pSD);
            }
        }
    } else
#endif
    {
        //### what to do with permissions if we don't use NTFS
        // for now just add all permissions and what about exe missions ??
        // also qt_ntfs_permission_lookup is now not set by default ... should it ?
        data.entryFlags |= QFileSystemMetaData::OwnerReadPermission
                           | QFileSystemMetaData::GroupReadPermission
                           | QFileSystemMetaData::OtherReadPermission;

        if (!(data.fileAttribute_ & FILE_ATTRIBUTE_READONLY)) {
            data.entryFlags |= QFileSystemMetaData::OwnerWritePermission
                   | QFileSystemMetaData::GroupWritePermission
                   | QFileSystemMetaData::OtherWritePermission;
        }

        QString fname = entry.filePath();
        QString ext = fname.right(4).toLower();
        if (data.isDirectory() ||
            ext == QLatin1String(".exe") || ext == QLatin1String(".com") || ext == QLatin1String(".bat") ||
            ext == QLatin1String(".pif") || ext == QLatin1String(".cmd")) {
            data.entryFlags |= QFileSystemMetaData::OwnerExecutePermission | QFileSystemMetaData::GroupExecutePermission
                               | QFileSystemMetaData::OtherExecutePermission | QFileSystemMetaData::UserExecutePermission;
        }
        data.knownFlagsMask |= QFileSystemMetaData::OwnerPermissions | QFileSystemMetaData::GroupPermissions
                                | QFileSystemMetaData::OtherPermissions | QFileSystemMetaData::UserExecutePermission;
        // calculate user permissions
        if (what & QFileSystemMetaData::UserReadPermission) {
            if (::_waccess((wchar_t*)entry.nativeFilePath().utf16(), R_OK) == 0)
                data.entryFlags |= QFileSystemMetaData::UserReadPermission;
            data.knownFlagsMask |= QFileSystemMetaData::UserReadPermission;
        }
        if (what & QFileSystemMetaData::UserWritePermission) {
            if (::_waccess((wchar_t*)entry.nativeFilePath().utf16(), W_OK) == 0)
                data.entryFlags |= QFileSystemMetaData::UserWritePermission;
            data.knownFlagsMask |= QFileSystemMetaData::UserWritePermission;
        }
    }

    return data.hasFlags(what);
}

static bool tryDriveUNCFallback(const QFileSystemEntry &fname, QFileSystemMetaData &data)
{
    bool entryExists = false;
    DWORD fileAttrib = 0;
#if !defined(Q_OS_WINCE)
    if (fname.isDriveRoot()) {
        // a valid drive ??
        DWORD drivesBitmask = ::GetLogicalDrives();
        int drivebit = 1 << (fname.filePath().at(0).toUpper().unicode() - QLatin1Char('A').unicode());
        if (drivesBitmask & drivebit) {
            fileAttrib = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_SYSTEM;
            entryExists = true;
        }
    } else {
#endif
        const QString &path = fname.nativeFilePath();
        bool is_dir = false;
        if (path.startsWith(QLatin1String("\\\\?\\UNC"))) {
            // UNC - stat doesn't work for all cases (Windows bug)
            int s = path.indexOf(path.at(0),7);
            if (s > 0) {
                // "\\?\UNC\server\..."
                s = path.indexOf(path.at(0),s+1);
                if (s > 0) {
                    // "\\?\UNC\server\share\..."
                    if (s == path.size() - 1) {
                        // "\\?\UNC\server\share\"
                        is_dir = true;
                    } else {
                        // "\\?\UNC\server\share\notfound"
                    }
                } else {
                    // "\\?\UNC\server\share"
                    is_dir = true;
                }
            } else {
                // "\\?\UNC\server"
                is_dir = true;
            }
        }
        if (is_dir && uncShareExists(path)) {
            // looks like a UNC dir, is a dir.
            fileAttrib = FILE_ATTRIBUTE_DIRECTORY;
            entryExists = true;
        }
#if !defined(Q_OS_WINCE)
    }
#endif
    if (entryExists)
        data.fillFromFileAttribute(fileAttrib);
    return entryExists;
}

static bool tryFindFallback(const QFileSystemEntry &fname, QFileSystemMetaData &data)
{
    bool filledData = false;
    // This assumes the last call to a Windows API failed.
    int errorCode = GetLastError();
    if (errorCode == ERROR_ACCESS_DENIED || errorCode == ERROR_SHARING_VIOLATION) {
        WIN32_FIND_DATA findData;
        if (getFindData(fname.nativeFilePath(), findData)
            && findData.dwFileAttributes != INVALID_FILE_ATTRIBUTES) {
            data.fillFromFindData(findData, true, fname.isDriveRoot());
            filledData = true;
        }
    }
    return filledData;
}

#if !defined(Q_OS_WINCE)
//static
bool QFileSystemEngine::fillMetaData(int fd, QFileSystemMetaData &data,
                                     QFileSystemMetaData::MetaDataFlags what)
{
    HANDLE fHandle = (HANDLE)_get_osfhandle(fd);
    if (fHandle  != INVALID_HANDLE_VALUE) {
        return fillMetaData(fHandle, data, what);
    }
    return false;
}
#endif

//static
bool QFileSystemEngine::fillMetaData(HANDLE fHandle, QFileSystemMetaData &data,
                                     QFileSystemMetaData::MetaDataFlags what)
{
    data.entryFlags &= ~what;
    clearWinStatData(data);
    BY_HANDLE_FILE_INFORMATION fileInfo;
    UINT oldmode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
    if (GetFileInformationByHandle(fHandle , &fileInfo)) {
        data.fillFromFindInfo(fileInfo);
    }
    SetErrorMode(oldmode);
    return data.hasFlags(what);
}

//static
bool QFileSystemEngine::fillMetaData(const QFileSystemEntry &entry, QFileSystemMetaData &data,
                                     QFileSystemMetaData::MetaDataFlags what)
{
    what |= QFileSystemMetaData::WinLnkType | QFileSystemMetaData::WinStatFlags;
    data.entryFlags &= ~what;

    QFileSystemEntry fname;
    data.knownFlagsMask |= QFileSystemMetaData::WinLnkType;
    if(entry.filePath().endsWith(QLatin1String(".lnk"))) {
        data.entryFlags |= QFileSystemMetaData::WinLnkType;
        fname = QFileSystemEntry(readLink(entry));
    } else {
        fname = entry;
    }

    if (fname.isEmpty()) {
        data.knownFlagsMask |= what;
        clearWinStatData(data);
        return false;
    }

    if (what & QFileSystemMetaData::WinStatFlags) {
        UINT oldmode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
        clearWinStatData(data);
        WIN32_FIND_DATA findData;
        // The memory structure for WIN32_FIND_DATA is same as WIN32_FILE_ATTRIBUTE_DATA
        // for all members used by fillFindData().
        bool ok = ::GetFileAttributesEx((wchar_t*)fname.nativeFilePath().utf16(), GetFileExInfoStandard,
                                        reinterpret_cast<WIN32_FILE_ATTRIBUTE_DATA *>(&findData));
        if (ok) {
            data.fillFromFindData(findData, false, fname.isDriveRoot());
        } else {
            if (!tryFindFallback(fname, data))
                tryDriveUNCFallback(fname, data);
        }
        SetErrorMode(oldmode);
    }

    if (what & QFileSystemMetaData::Permissions)
        fillPermissions(fname, data, what);
    if ((what & QFileSystemMetaData::LinkType)
        && data.missingFlags(QFileSystemMetaData::LinkType)) {
        data.knownFlagsMask |= QFileSystemMetaData::LinkType;
        if (data.fileAttribute_ & FILE_ATTRIBUTE_REPARSE_POINT) {
            WIN32_FIND_DATA findData;
            if (getFindData(fname.nativeFilePath(), findData))
                data.fillFromFindData(findData, true);
        }
    }
    data.knownFlagsMask |= what;
    return data.hasFlags(what);
}

static inline bool mkDir(const QString &path)
{
#if defined(Q_OS_WINCE)
    // Unfortunately CreateDirectory returns true for paths longer than
    // 256, but does not create a directory. It starts to fail, when
    // path length > MAX_PATH, which is 260 usually on CE.
    // This only happens on a Windows Mobile device. Windows CE seems
    // not to be affected by this.
    static int platformId = 0;
    if (platformId == 0) {
        wchar_t platformString[64];
        if (SystemParametersInfo(SPI_GETPLATFORMTYPE, sizeof(platformString)/sizeof(*platformString),platformString,0)) {
            if (0 == wcscmp(platformString, L"PocketPC") || 0 == wcscmp(platformString, L"Smartphone"))
                platformId = 1;
            else
                platformId = 2;
        }
    }
    if (platformId == 1 && QFSFileEnginePrivate::longFileName(path).size() > 256)
        return false;
#endif
    return ::CreateDirectory((wchar_t*)QFSFileEnginePrivate::longFileName(path).utf16(), 0);
}

static inline bool rmDir(const QString &path)
{
    return ::RemoveDirectory((wchar_t*)QFSFileEnginePrivate::longFileName(path).utf16());
}

static bool isDirPath(const QString &dirPath, bool *existed)
{
    QString path = dirPath;
    if (path.length() == 2 && path.at(1) == QLatin1Char(':'))
        path += QLatin1Char('\\');

    DWORD fileAttrib = ::GetFileAttributes((wchar_t*)QFSFileEnginePrivate::longFileName(path).utf16());
    if (fileAttrib == INVALID_FILE_ATTRIBUTES) {
        int errorCode = GetLastError();
        if (errorCode == ERROR_ACCESS_DENIED || errorCode == ERROR_SHARING_VIOLATION) {
            WIN32_FIND_DATA findData;
            if (getFindData(QFSFileEnginePrivate::longFileName(path), findData))
                fileAttrib = findData.dwFileAttributes;
        }
    }

    if (existed)
        *existed = fileAttrib != INVALID_FILE_ATTRIBUTES;

    if (fileAttrib == INVALID_FILE_ATTRIBUTES)
        return false;

    return fileAttrib & FILE_ATTRIBUTE_DIRECTORY;
}

//static
bool QFileSystemEngine::createDirectory(const QFileSystemEntry &entry, bool createParents)
{
    QString dirName = entry.filePath();
    if (createParents) {
        dirName = QDir::toNativeSeparators(QDir::cleanPath(dirName));
        // We spefically search for / so \ would break it..
        int oldslash = -1;
        if (dirName.startsWith(QLatin1String("\\\\"))) {
            // Don't try to create the root path of a UNC path;
            // CreateDirectory() will just return ERROR_INVALID_NAME.
            for (int i = 0; i < dirName.size(); ++i) {
                if (dirName.at(i) != QDir::separator()) {
                    oldslash = i;
                    break;
                }
            }
            if (oldslash != -1)
                oldslash = dirName.indexOf(QDir::separator(), oldslash);
        }
        for (int slash=0; slash != -1; oldslash = slash) {
            slash = dirName.indexOf(QDir::separator(), oldslash+1);
            if (slash == -1) {
                if (oldslash == dirName.length())
                    break;
                slash = dirName.length();
            }
            if (slash) {
                QString chunk = dirName.left(slash);
                bool existed = false;
                if (!isDirPath(chunk, &existed)) {
                    if (!existed) {
                        if (!mkDir(chunk))
                            return false;
                    } else {
                        return false;
                    }
                }
            }
        }
        return true;
    }
    return mkDir(entry.filePath());
}

//static
bool QFileSystemEngine::removeDirectory(const QFileSystemEntry &entry, bool removeEmptyParents)
{
    QString dirName = entry.filePath();
    if (removeEmptyParents) {
        dirName = QDir::toNativeSeparators(QDir::cleanPath(dirName));
        for (int oldslash = 0, slash=dirName.length(); slash > 0; oldslash = slash) {
            QString chunk = dirName.left(slash);
            if (chunk.length() == 2 && chunk.at(0).isLetter() && chunk.at(1) == QLatin1Char(':'))
                break;
            if (!isDirPath(chunk, 0))
                return false;
            if (!rmDir(chunk))
                return oldslash != 0;
            slash = dirName.lastIndexOf(QDir::separator(), oldslash-1);
        }
        return true;
    }
    return rmDir(entry.filePath());
}

//static
QString QFileSystemEngine::rootPath()
{
#if defined(Q_OS_WINCE)
    QString ret = QLatin1String("/");
#elif defined(Q_FS_FAT)
    QString ret = QString::fromLatin1(qgetenv("SystemDrive").constData());
    if (ret.isEmpty())
        ret = QLatin1String("c:");
    ret.append(QLatin1Char('/'));
#elif defined(Q_OS_OS2EMX)
    char dir[4];
    _abspath(dir, QLatin1String("/"), _MAX_PATH);
    QString ret(dir);
#endif
    return ret;
}

//static
QString QFileSystemEngine::homePath()
{
    QString ret;
#if !defined(QT_NO_LIBRARY)
    resolveLibs();
    if (ptrGetUserProfileDirectoryW) {
        HANDLE hnd = ::GetCurrentProcess();
        HANDLE token = 0;
        BOOL ok = ::OpenProcessToken(hnd, TOKEN_QUERY, &token);
        if (ok) {
            DWORD dwBufferSize = 0;
            // First call, to determine size of the strings (with '\0').
            ok = ptrGetUserProfileDirectoryW(token, NULL, &dwBufferSize);
            if (!ok && dwBufferSize != 0) {        // We got the required buffer size
                wchar_t *userDirectory = new wchar_t[dwBufferSize];
                // Second call, now we can fill the allocated buffer.
                ok = ptrGetUserProfileDirectoryW(token, userDirectory, &dwBufferSize);
                if (ok)
                    ret = QString::fromWCharArray(userDirectory);
                delete [] userDirectory;
            }
            ::CloseHandle(token);
        }
    }
#endif
    if (ret.isEmpty() || !QFile::exists(ret)) {
        ret = QString::fromLocal8Bit(qgetenv("USERPROFILE").constData());
        if (ret.isEmpty() || !QFile::exists(ret)) {
            ret = QString::fromLocal8Bit(qgetenv("HOMEDRIVE").constData())
                  + QString::fromLocal8Bit(qgetenv("HOMEPATH").constData());
            if (ret.isEmpty() || !QFile::exists(ret)) {
                ret = QString::fromLocal8Bit(qgetenv("HOME").constData());
                if (ret.isEmpty() || !QFile::exists(ret)) {
#if defined(Q_OS_WINCE)
                    ret = QLatin1String("\\My Documents");
                    if (!QFile::exists(ret))
#endif
                        ret = rootPath();
                }
            }
        }
    }
    return QDir::fromNativeSeparators(ret);
}

QString QFileSystemEngine::tempPath()
{
    QString ret;
    wchar_t tempPath[MAX_PATH];
    DWORD len = GetTempPath(MAX_PATH, tempPath);
    if (len)
        ret = QString::fromWCharArray(tempPath, len);
    if (!ret.isEmpty()) {
        while (ret.endsWith(QLatin1Char('\\')))
            ret.chop(1);
        ret = QDir::fromNativeSeparators(ret);
    }
    if (ret.isEmpty()) {
#if !defined(Q_OS_WINCE)
        ret = QLatin1String("C:/tmp");
#else
        ret = QLatin1String("/Temp");
#endif
    } else if (ret.length() >= 2 && ret[1] == QLatin1Char(':'))
        ret[0] = ret.at(0).toUpper(); // Force uppercase drive letters.
    return ret;
}

bool QFileSystemEngine::setCurrentPath(const QFileSystemEntry &entry)
{
    QFileSystemMetaData meta;
    fillMetaData(entry, meta, QFileSystemMetaData::ExistsAttribute | QFileSystemMetaData::DirectoryType);
    if(!(meta.exists() && meta.isDirectory()))
        return false;

#if !defined(Q_OS_WINCE)
    //TODO: this should really be using nativeFilePath(), but that returns a path in long format \\?\c:\foo
    //which causes many problems later on when it's returned through currentPath()
    return ::SetCurrentDirectory(reinterpret_cast<const wchar_t*>(QDir::toNativeSeparators(entry.filePath()).utf16())) != 0;
#else
    qfsPrivateCurrentDir = entry.filePath();
    return true;
#endif
}

QFileSystemEntry QFileSystemEngine::currentPath()
{
    QString ret;
#if !defined(Q_OS_WINCE)
    DWORD size = 0;
    wchar_t currentName[PATH_MAX];
    size = ::GetCurrentDirectory(PATH_MAX, currentName);
    if (size != 0) {
        if (size > PATH_MAX) {
            wchar_t *newCurrentName = new wchar_t[size];
            if (::GetCurrentDirectory(PATH_MAX, newCurrentName) != 0)
                ret = QString::fromWCharArray(newCurrentName, size);
            delete [] newCurrentName;
        } else {
            ret = QString::fromWCharArray(currentName, size);
        }
    }
    if (ret.length() >= 2 && ret[1] == QLatin1Char(':'))
        ret[0] = ret.at(0).toUpper(); // Force uppercase drive letters.
#else
    //TODO - a race condition exists when using currentPath / setCurrentPath from multiple threads
    if (qfsPrivateCurrentDir.isEmpty())
        qfsPrivateCurrentDir = QCoreApplication::applicationDirPath();

    ret = qfsPrivateCurrentDir;
#endif
    return QFileSystemEntry(ret, QFileSystemEntry::FromNativePath());
}

//static
bool QFileSystemEngine::createLink(const QFileSystemEntry &source, const QFileSystemEntry &target, QSystemError &error)
{
    Q_ASSERT(false);
    Q_UNUSED(source)
    Q_UNUSED(target)
    Q_UNUSED(error)

    return false; // TODO implement; - code needs to be moved from qfsfileengine_win.cpp
}

//static
bool QFileSystemEngine::copyFile(const QFileSystemEntry &source, const QFileSystemEntry &target, QSystemError &error)
{
    bool ret = ::CopyFile((wchar_t*)source.nativeFilePath().utf16(),
                          (wchar_t*)target.nativeFilePath().utf16(), true) != 0;
    if(!ret)
        error = QSystemError(::GetLastError(), QSystemError::NativeError);
    return ret;
}

//static
bool QFileSystemEngine::renameFile(const QFileSystemEntry &source, const QFileSystemEntry &target, QSystemError &error)
{
    bool ret = ::MoveFile((wchar_t*)source.nativeFilePath().utf16(),
                          (wchar_t*)target.nativeFilePath().utf16()) != 0;
    if(!ret)
        error = QSystemError(::GetLastError(), QSystemError::NativeError);
    return ret;
}

//static
bool QFileSystemEngine::removeFile(const QFileSystemEntry &entry, QSystemError &error)
{
    bool ret = ::DeleteFile((wchar_t*)entry.nativeFilePath().utf16()) != 0;
    if(!ret)
        error = QSystemError(::GetLastError(), QSystemError::NativeError);
    return ret;
}

//static
bool QFileSystemEngine::setPermissions(const QFileSystemEntry &entry, QFile::Permissions permissions, QSystemError &error,
                                       QFileSystemMetaData *data)
{
    Q_UNUSED(data);
    int mode = 0;

    if (permissions & QFile::ReadOwner || permissions & QFile::ReadUser
        || permissions & QFile::ReadGroup || permissions & QFile::ReadOther)
        mode |= _S_IREAD;
    if (permissions & QFile::WriteOwner || permissions & QFile::WriteUser
        || permissions & QFile::WriteGroup || permissions & QFile::WriteOther)
        mode |= _S_IWRITE;

    if (mode == 0) // not supported
        return false;

    bool ret = (::_wchmod((wchar_t*)entry.nativeFilePath().utf16(), mode) == 0);
    if(!ret)
        error = QSystemError(errno, QSystemError::StandardLibraryError);
    return ret;
}

static inline QDateTime fileTimeToQDateTime(const FILETIME *time)
{
    QDateTime ret;

#if defined(Q_OS_WINCE)
    SYSTEMTIME systime;
    FILETIME ftime;
    systime.wYear = 1970;
    systime.wMonth = 1;
    systime.wDay = 1;
    systime.wHour = 0;
    systime.wMinute = 0;
    systime.wSecond = 0;
    systime.wMilliseconds = 0;
    systime.wDayOfWeek = 4;
    SystemTimeToFileTime(&systime, &ftime);
    unsigned __int64 acttime = (unsigned __int64)time->dwHighDateTime << 32 | time->dwLowDateTime;
    FileTimeToSystemTime(time, &systime);
    unsigned __int64 time1970 = (unsigned __int64)ftime.dwHighDateTime << 32 | ftime.dwLowDateTime;
    unsigned __int64 difftime = acttime - time1970;
    difftime /= 10000000;
    ret.setTime_t((unsigned int)difftime);
#else
    SYSTEMTIME sTime, lTime;
    FileTimeToSystemTime(time, &sTime);
    SystemTimeToTzSpecificLocalTime(0, &sTime, &lTime);
    ret.setDate(QDate(lTime.wYear, lTime.wMonth, lTime.wDay));
    ret.setTime(QTime(lTime.wHour, lTime.wMinute, lTime.wSecond, lTime.wMilliseconds));
#endif

    return ret;
}

QDateTime QFileSystemMetaData::creationTime() const
{
    return fileTimeToQDateTime(&creationTime_);
}
QDateTime QFileSystemMetaData::modificationTime() const
{
    return fileTimeToQDateTime(&lastWriteTime_);
}
QDateTime QFileSystemMetaData::accessTime() const
{
    return fileTimeToQDateTime(&lastAccessTime_);
}

QT_END_NAMESPACE
