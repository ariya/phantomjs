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
#ifdef _WIN32_WCE //Q_OS_WINCE

#include <windows.h>
#include <winbase.h>
#include <kfuncs.h>
#include <stdio.h>
#include <altcecrt.h>

#include "qplatformdefs.h"
#include "qfunctions_wince.h"
#include "qstring.h"
#include "qbytearray.h"
#include "qhash.h"

QT_USE_NAMESPACE

#ifdef __cplusplus
extern "C" {
#endif

wchar_t* CEPrivConvCharToWide(const char* string)
{
    size_t length = strlen(string);
    wchar_t* wString = new wchar_t[length +1];
    for (unsigned int i = 0; i < (length +1); i++)
        wString[i] = string[i];
    return wString;
}

// Time -------------------------------------------------------------
time_t qt_wince_ftToTime_t( const FILETIME ft )
{
    ULARGE_INTEGER li;
    li.LowPart  = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;

    // 100-nanosec to seconds
    li.QuadPart /= 10000000;

    // FILETIME is from 1601-01-01 T 00:00:00
    // time_t   is from 1970-01-01 T 00:00:00
    // 1970 - 1601 = 369 year (89 leap years)
    //
    // ((369y*365d) + 89d) *24h *60min *60sec
    // = 11644473600 seconds
    li.QuadPart -= 11644473600;
    return li.LowPart;
}

FILETIME qt_wince_time_tToFt( time_t tt )
{
    ULARGE_INTEGER li;
    li.QuadPart  = tt;
    li.QuadPart += 11644473600;
    li.QuadPart *= 10000000;

    FILETIME ft;
    ft.dwLowDateTime = li.LowPart;
    ft.dwHighDateTime = li.HighPart;
    return ft;
}

// File I/O ---------------------------------------------------------
int errno = 0;

int qt_wince__getdrive( void )
{
    return 1;
}

int qt_wince__waccess( const wchar_t *path, int pmode )
{
    DWORD res = GetFileAttributes( path );
    if ( 0xFFFFFFFF == res )
        return -1;

    if ( (pmode & W_OK) && (res & FILE_ATTRIBUTE_READONLY) )
        return -1;

    if ( (pmode & X_OK) && !(res & FILE_ATTRIBUTE_DIRECTORY) ) {
        QString file = QString::fromWCharArray(path);
        if ( !(file.endsWith(QString::fromLatin1(".exe")) ||
           file.endsWith(QString::fromLatin1(".com"))) )
        return -1;
    }

    return 0;
}

int qt_wince_open( const char *filename, int oflag, int pmode )
{
    QString fn( QString::fromLatin1(filename) );
    return _wopen( (wchar_t*)fn.utf16(), oflag, pmode );
}

int qt_wince__wopen( const wchar_t *filename, int oflag, int /*pmode*/ )
{
    wchar_t *flag;

    if ( oflag & _O_APPEND ) {
        if ( oflag & _O_WRONLY ) {
            flag = L"a";
        } else if ( oflag & _O_RDWR ) {
            flag = L"a+";
        }
    } else if (oflag & _O_BINARY) {
        if ( oflag & _O_WRONLY ) {
            flag = L"wb";
        } else if ( oflag & _O_RDWR ) {
            flag = L"w+b"; // slightly different from "r+" where the file must exist
        } else if ( oflag & _O_RDONLY ) {
            flag = L"rb";
        } else {
            flag = L"b";
        }
    } else {
        if ( oflag & _O_WRONLY ) {
            flag = L"wt";
        } else if ( oflag & _O_RDWR ) {
            flag = L"w+t"; // slightly different from "r+" where the file must exist
        } else if ( oflag & _O_RDONLY ) {
            flag = L"rt";
        } else {
            flag = L"t";
        }
    }

    int retval = (int)_wfopen( filename, flag );
    return (retval == NULL) ? -1 : retval;
}

long qt_wince__lseek( int handle, long offset, int origin )
{
    return fseek( (FILE*)handle, offset, origin );
}

int qt_wince__read( int handle, void *buffer, unsigned int count )
{
    return fread( buffer, 1, count, (FILE*)handle );
}

int qt_wince__write( int handle, const void *buffer, unsigned int count )
{
    return fwrite( buffer, 1, count, (FILE*)handle );
}

int qt_wince__close( int handle )
{
    if (!handle)
        return 0;
    return fclose( (FILE*)handle );
}

FILE *qt_wince__fdopen(int handle, const char* /*mode*/)
{
    return (FILE*)handle;
}

FILE *qt_wince_fdopen( int handle, const char* /*mode*/ )
{
    return (FILE*)handle;
}

void qt_wince_rewind( FILE *stream )
{
    fseek( stream, 0L, SEEK_SET );
}

int qt_wince___fileno(FILE *f)
{
    return (int) _fileno(f);
}

FILE *qt_wince_tmpfile( void )
{
    static long i = 0;
    char name[16];
    sprintf( name, "tmp%i", i++ );
    return fopen( name, "r+" );
}

int qt_wince__mkdir(const char *dirname)
{
    return CreateDirectory(reinterpret_cast<const wchar_t *> (QString(QString::fromLatin1(dirname)).utf16()), 0) ? 0 : -1;
}

int qt_wince__rmdir(const char *dirname)
{
    return RemoveDirectory(reinterpret_cast<const wchar_t *> (QString::fromLatin1(dirname).utf16())) ? 0 : -1;
}

int qt_wince__access( const char *path, int pmode )
{
    return _waccess(reinterpret_cast<const wchar_t *> (QString::fromLatin1(path).utf16()),pmode);
}

int qt_wince__rename( const char *oldname, const char *newname )
{
    return !MoveFile(reinterpret_cast<const wchar_t *> (QString::fromLatin1(oldname).utf16()), reinterpret_cast<const wchar_t *> (QString::fromLatin1(newname).utf16()));
}

int qt_wince__remove( const char *name )
{
    return !DeleteFile(reinterpret_cast<const wchar_t *> (QString::fromLatin1(name).utf16()));
}

int qt_wince_stat( const char *path, struct stat *buffer )
{
    WIN32_FIND_DATA finfo;
    HANDLE ff = FindFirstFile( reinterpret_cast<const wchar_t *> (QString::fromLatin1(path).utf16()), &finfo );

    if ( ff == INVALID_HANDLE_VALUE )
        return -1;

    buffer->st_ctime = qt_wince_ftToTime_t( finfo.ftCreationTime );
    buffer->st_atime = qt_wince_ftToTime_t( finfo.ftLastAccessTime );
    buffer->st_mtime = qt_wince_ftToTime_t( finfo.ftLastWriteTime );
    buffer->st_nlink = 0;
    buffer->st_size  = finfo.nFileSizeLow; // ### missing high!
    buffer->st_mode  = (finfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? _S_IFDIR : _S_IFREG;
    buffer->st_mode |= (finfo.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ? _O_RDONLY : _O_RDWR;
    return (FindClose(ff) == 0);
}

int qt_wince__fstat( int handle, struct stat *buffer)
{
    BY_HANDLE_FILE_INFORMATION fInfo;
    BOOL res = GetFileInformationByHandle((HANDLE)handle, &fInfo);

    buffer->st_ctime = qt_wince_ftToTime_t( fInfo.ftCreationTime );
    buffer->st_atime = qt_wince_ftToTime_t( fInfo.ftLastAccessTime );
    buffer->st_mtime = qt_wince_ftToTime_t( fInfo.ftLastWriteTime );
    buffer->st_nlink = 0;
    buffer->st_size  = fInfo.nFileSizeLow; // ### missing high!
    buffer->st_mode  = (fInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? _S_IFDIR : _S_IFREG;
    buffer->st_mode |= (fInfo.dwFileAttributes & FILE_ATTRIBUTE_READONLY) ? _O_RDONLY : _O_RDWR;
    return (res == 0);
}

int qt_wince_SetErrorMode(int newValue)
{
    static int oldValue;
    int result = oldValue;
    oldValue = newValue;
    return result;
}

bool qt_wince__chmod(const char *file, int mode)
{
    return _wchmod( reinterpret_cast<const wchar_t *> (QString::fromLatin1(file).utf16()), mode);
}

bool qt_wince__wchmod(const wchar_t *file, int mode)
{
    BOOL success = FALSE;
    // ### Does not work properly, what about just adding one property?
    if(mode&_S_IWRITE) {
        success = SetFileAttributes(file, FILE_ATTRIBUTE_NORMAL);
    } else if((mode&_S_IREAD) && !(mode&_S_IWRITE)) {
        success = SetFileAttributes(file, FILE_ATTRIBUTE_READONLY);
    }
    return success ? 0 : -1;
}

HANDLE qt_wince_CreateFileA(LPCSTR filename, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES attr, DWORD dispo, DWORD flags, HANDLE tempFile)
{
    return CreateFileW( reinterpret_cast<const wchar_t *>(QString::fromLatin1(filename).utf16()), access, share, attr, dispo, flags, tempFile);
}

// Graphics ---------------------------------------------------------
BOOL qt_wince_SetWindowOrgEx( HDC /*hdc*/, int /*X*/, int /*Y*/, LPPOINT /*lpPoint*/) {
    return TRUE;
}

// Threading --------------------------------------------------------
HANDLE qt_wince__beginthread(void( *start_address )( void * ), unsigned stack_size, void *arglist)
{
    unsigned initflag = 0;
    if (stack_size > 0)
        initflag |= STACK_SIZE_PARAM_IS_A_RESERVATION;
    return CreateThread(NULL, stack_size, (LPTHREAD_START_ROUTINE)start_address, arglist, initflag, NULL);
}

unsigned long qt_wince__beginthreadex( void *security,
                  unsigned stack_size,
                  unsigned (__stdcall *start_address)(void *),
                  void *arglist,
                  unsigned initflag,
                  unsigned *thrdaddr)
{
    if (stack_size > 0)
        initflag |= STACK_SIZE_PARAM_IS_A_RESERVATION;
    return (unsigned long)
            CreateThread( (LPSECURITY_ATTRIBUTES)security,
                          (DWORD)stack_size,
                          (LPTHREAD_START_ROUTINE)start_address,
                          (LPVOID)arglist,
                          (DWORD)initflag | CREATE_SUSPENDED,
                          (LPDWORD)thrdaddr);
}

void qt_wince__endthreadex(unsigned nExitCode) {
    ExitThread((DWORD)nExitCode);
}

void *qt_wince_bsearch(const void *key,
           const void *base,
           size_t num,
           size_t size,
           int (__cdecl *compare)(const void *, const void *))
{
    size_t low = 0;
    size_t high = num - 1;
    while (low <= high) {
        size_t mid = (low + high) >> 1;
        int c = compare(key, (char*)base + mid * size);
        if (c < 0) {
            if (!mid)
                break;
            high = mid - 1;
        } else if (c > 0)
            low = mid + 1;
        else
            return (char*) base + mid * size;
    }
    return 0;
}

void *lfind(const void* key, const void* base, size_t* elements, size_t size,
            int (__cdecl *compare)(const void*, const void*))
{
    const char* current = (char*) base;
    const char* const end = (char*) (current + (*elements) * size);
    while (current != end) {
        if (compare(current, key) == 0)
            return (void*)current;
        current += size;
    }
    return 0;
}

DWORD qt_wince_GetThreadLocale(void)
{
    return GetUserDefaultLCID();
}

void *qt_wince_calloc( size_t num, size_t size )
{
    void *ptr = malloc( num * size );
    if( ptr )
        memset( ptr, 0, num * size );
    return ptr;
}

// _getpid is currently only used for creating a temporary filename
int qt_wince__getpid()
{
    return qAbs((int)GetCurrentProcessId());
}

#ifdef __cplusplus
} // extern "C"
#endif
// Environment ------------------------------------------------------
inline QHash<QByteArray, QByteArray>& qt_app_environment()
{
    static QHash<QByteArray, QByteArray> internalEnvironment;
    return internalEnvironment;
}

errno_t qt_wince_getenv_s(size_t* sizeNeeded, char* buffer, size_t bufferSize, const char* varName)
{
    if (!sizeNeeded)
        return EINVAL;

    if (!qt_app_environment().contains(varName)) {
        if (buffer)
            buffer[0] = '\0';
        return ENOENT;
    }

    QByteArray value = qt_app_environment().value(varName);
    if (!value.endsWith('\0')) // win32 guarantees terminated string
        value.append('\0');

    if (bufferSize < (size_t)value.size()) {
        *sizeNeeded = value.size();
        return 0;
    }

    strcpy(buffer, value.constData());
    return 0;
}

errno_t qt_wince__putenv_s(const char* varName, const char* value)
{
    QByteArray input = value;
    if (input.isEmpty()) {
        if (qt_app_environment().contains(varName))
            qt_app_environment().remove(varName);
    } else {
        // win32 guarantees terminated string
        if (!input.endsWith('\0'))
            input.append('\0');
        qt_app_environment()[varName] = input;
    }

    return 0;
}

#endif // Q_OS_WINCE
