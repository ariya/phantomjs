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

#ifndef QFUNCTIONS_WINCE_H
#define QFUNCTIONS_WINCE_H

#include <QtCore/qglobal.h>

#ifdef Q_OS_WINCE
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winuser.h>
#include <winbase.h>
#include <objbase.h>
#include <kfuncs.h>
#include <ctype.h>
#include <time.h>
#include <crtdefs.h>
#include <altcecrt.h>
#include <winsock.h>
#include <ceconfig.h>

QT_BEGIN_NAMESPACE

#ifdef QT_BUILD_CORE_LIB
#endif

QT_END_NAMESPACE

// The standard SDK misses this define...
#define _control87 _controlfp

#if !defined __cplusplus
#define bool int
#define true 1
#define false 0
#endif

// Environment ------------------------------------------------------
errno_t qt_wince_getenv_s(size_t*, char*, size_t, const char*);
errno_t qt_wince__putenv_s(const char*, const char*);

#ifdef __cplusplus // have this as tiff plugin is written in C
extern "C" {
#endif

#if !defined(NO_ERRNO_H)
#define NO_ERRNO_H
#endif

// Environment ------------------------------------------------------
int qt_wince__getpid(void);


// Time -------------------------------------------------------------
#ifndef _TM_DEFINED
#define _TM_DEFINED
struct tm {
    int tm_sec;     /* seconds after the minute - [0,59] */
    int tm_min;     /* minutes after the hour - [0,59] */
    int tm_hour;    /* hours since midnight - [0,23] */
    int tm_mday;    /* day of the month - [1,31] */
    int tm_mon;     /* months since January - [0,11] */
    int tm_year;    /* years since 1900 */
    int tm_wday;    /* days since Sunday - [0,6] */
    int tm_yday;    /* days since January 1 - [0,365] */
    int tm_isdst;   /* daylight savings time flag */
};
#endif // _TM_DEFINED

FILETIME qt_wince_time_tToFt( time_t tt );
time_t qt_wince_ftToTime_t( const FILETIME ft );

// File I/O ---------------------------------------------------------
#define _O_RDONLY       0x0001
#define _O_RDWR         0x0002
#define _O_WRONLY       0x0004
#define _O_CREAT        0x0008
#define _O_TRUNC        0x0010
#define _O_APPEND       0x0020
#define _O_EXCL         0x0040

#define O_RDONLY        _O_RDONLY
#define O_RDWR          _O_RDWR
#define O_WRONLY        _O_WRONLY
#define O_CREAT         _O_CREAT
#define O_TRUNC         _O_TRUNC
#define O_APPEND        _O_APPEND
#define O_EXCL          _O_EXCL

#define _S_IFMT         0x0600
#define _S_IFDIR        0x0200
#define _S_IFCHR        0x0100
#define _S_IFREG        0x0400
#define _S_IREAD        0x0010
#define _S_IWRITE       0x0008

#define S_IFMT          _S_IFMT
#define S_IFDIR         _S_IFDIR
#define S_IFCHR         _S_IFCHR
#define S_IFREG         _S_IFREG
#define S_IREAD         _S_IREAD
#define S_IWRITE        _S_IWRITE

#ifndef _IOFBF
#define _IOFBF          0x0000
#endif

#ifndef _IOLBF
#define _IOLBF          0x0040
#endif

#ifndef _IONBF
#define _IONBF          0x0004
#endif

// Regular Berkeley error constants
#ifndef _STAT_DEFINED
#define _STAT_DEFINED
struct stat
{
    int st_mode;
    int st_size;
    int st_nlink;
    time_t st_mtime;
    time_t st_atime;
    time_t st_ctime;
};
#endif

typedef int mode_t;
extern int errno;

int     qt_wince__getdrive( void );
int     qt_wince__waccess( const wchar_t *path, int pmode );
int     qt_wince__wopen( const wchar_t *filename, int oflag, int pmode );
long    qt_wince__lseek( int handle, long offset, int origin );
int     qt_wince__read( int handle, void *buffer, unsigned int count );
int     qt_wince__write( int handle, const void *buffer, unsigned int count );
int     qt_wince__close( int handle );
FILE   *qt_wince__fdopen(int handle, const char *mode);
FILE   *qt_wince_fdopen(int handle, const char *mode);
void    qt_wince_rewind( FILE *stream );
int     qt_wince___fileno(FILE *);
FILE   *qt_wince_tmpfile( void );

//For zlib we need these helper functions, but they break the build when
//set globally, so just set them for zlib use
#ifdef ZLIB_H
#define open qt_wince_open
#define close qt_wince__close
#define lseek qt_wince__lseek
#define read qt_wince__read
#define write qt_wince__write
#endif

int qt_wince__mkdir(const char *dirname);
int qt_wince__rmdir(const char *dirname);
int qt_wince__access( const char *path, int pmode );
int qt_wince__rename( const char *oldname, const char *newname );
int qt_wince__remove( const char *name );
#ifdef __cplusplus
int qt_wince_open( const char *filename, int oflag, int pmode = 0 );
#else
int qt_wince_open( const char *filename, int oflag, int pmode );
#endif
int qt_wince_stat( const char *path, struct stat *buffer );
int qt_wince__fstat( int handle, struct stat *buffer);

#define SEM_FAILCRITICALERRORS 0x0001
#define SEM_NOOPENFILEERRORBOX 0x0002
int qt_wince_SetErrorMode(int);
#ifndef CoInitialize
#define CoInitialize(x) CoInitializeEx(x, COINIT_MULTITHREADED)
#endif

bool qt_wince__chmod(const char *file, int mode);
bool qt_wince__wchmod(const wchar_t *file, int mode);

#pragma warning(disable: 4273)
HANDLE qt_wince_CreateFileA(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);

// Printer ----------------------------------------------------------
#define ETO_GLYPH_INDEX     0x0010

// Graphics ---------------------------------------------------------
#ifndef SM_CXCURSOR
#  define SM_CXCURSOR       13
#endif
#ifndef SM_CYCURSOR
#  define SM_CYCURSOR      14
#endif
BOOL qt_wince_SetWindowOrgEx( HDC hdc, int X, int Y, LPPOINT lpPoint );

// Other stuff ------------------------------------------------------
#define MWMO_ALERTABLE 0x0002
// ### not the real values
#define CREATE_NO_WINDOW        2
#define CF_HDROP                15

void *qt_wince_calloc(size_t num, size_t size);
#if !defined(TLS_OUT_OF_INDEXES)
#  define TLS_OUT_OF_INDEXES 0xffffffff
#endif
DWORD qt_wince_GetThreadLocale(void);

HANDLE qt_wince__beginthread(void( *start_address )( void * ), unsigned stack_size, void *arglist);

unsigned long qt_wince__beginthreadex( void *security,
                              unsigned stack_size,
                              unsigned (__stdcall *start_address)(void *),
                              void *arglist,
                              unsigned initflag,
                              unsigned *thrdaddr );
void qt_wince__endthreadex(unsigned nExitCode);


// bsearch is needed for building the tiff plugin
// otherwise it could go into qguifunctions_wce
void *qt_wince_bsearch(const void *key,
               const void *base,
               size_t num,
               size_t size,
               int (__cdecl *compare)(const void *, const void *));

// Missing typedefs
#ifndef _TIME_T_DEFINED
typedef unsigned long time_t;
#define _TIME_T_DEFINED
#endif
typedef HANDLE HDROP;

#ifndef WS_THICKFRAME
#define WS_THICKFRAME   WS_DLGFRAME
#endif

typedef UINT    UWORD;

// Missing definitions: not necessary equal to their Win32 values
// (the goal is to just have a clean compilation of MFC)
#define WS_MAXIMIZE               0
#define WS_MINIMIZE               0
#ifndef WS_EX_TOOLWINDOW
#define WS_EX_TOOLWINDOW          0
#endif
#define WS_EX_NOPARENTNOTIFY      0
#define WM_ENTERIDLE              0x0121
#define WM_PRINT                  WM_PAINT
#define WM_NCCREATE               (0x0081)
#define WM_PARENTNOTIFY           0
#define WM_NCDESTROY              (WM_APP-1)
#ifndef SW_RESTORE
#define SW_RESTORE                (SW_SHOWNORMAL)
#endif
#define SW_NORMAL                 (SW_SHOWNORMAL)
#define WAIT_OBJECT_0             0x00000000L
#define DEFAULT_GUI_FONT          SYSTEM_FONT
#ifndef SWP_NOREDRAW
#define SWP_NOREDRAW               0
#endif
#define WSAGETSELECTEVENT(lParam) LOWORD(lParam)
#define HWND_TOPMOST              ((HWND)-1)
#define HWND_NOTOPMOST            ((HWND)-2)
#define PS_DOT                    2
#define PD_ALLPAGES               0
#define PD_USEDEVMODECOPIES       0
#define PD_NOSELECTION            0
#define PD_HIDEPRINTTOFILE        0
#define PD_NOPAGENUMS             0
#define CF_METAFILEPICT           3
#define MM_ANISOTROPIC            8
#define KF_ALTDOWN                0x2000
#define SPI_GETWORKAREA           48

#ifndef WM_SETCURSOR
        #define WM_SETCURSOR 0x0020
        #define IDC_ARROW           MAKEINTRESOURCE(32512)
        #define IDC_IBEAM           MAKEINTRESOURCE(32513)
        #define IDC_WAIT            MAKEINTRESOURCE(32514)
        #define IDC_CROSS           MAKEINTRESOURCE(32515)
        #define IDC_UPARROW         MAKEINTRESOURCE(32516)
        #define IDC_SIZE            MAKEINTRESOURCE(32646)
        #define IDC_ICON            MAKEINTRESOURCE(32512)
        #define IDC_SIZENWSE        MAKEINTRESOURCE(32642)
        #define IDC_SIZENESW        MAKEINTRESOURCE(32643)
        #define IDC_SIZEWE          MAKEINTRESOURCE(32644)
        #define IDC_SIZENS          MAKEINTRESOURCE(32645)
        #define IDC_SIZEALL         MAKEINTRESOURCE(32646)
        #define IDC_NO              MAKEINTRESOURCE(32648)
        #define IDC_APPSTARTING     MAKEINTRESOURCE(32650)
        #define IDC_HELP            MAKEINTRESOURCE(32651)
        #define IDC_HAND            MAKEINTRESOURCE(32649)
#endif

#define GMEM_MOVEABLE             LMEM_MOVEABLE
#define GPTR                      LPTR

// WinCE: CESYSGEN prunes the following FRP defines,
// and INTERNET_TRANSFER_TYPE_ASCII breaks in wininet.h
#undef FTP_TRANSFER_TYPE_ASCII
#define FTP_TRANSFER_TYPE_ASCII 0x00000001
#undef FTP_TRANSFER_TYPE_BINARY
#define FTP_TRANSFER_TYPE_BINARY 0x00000002

typedef DWORD OLE_COLOR;

// Define the Windows Styles which are not defined by MS
#ifndef WS_POPUPWINDOW
#define WS_POPUPWINDOW WS_POPUP|WS_BORDER|WS_SYSMENU|WS_CAPTION
#endif

#ifndef WS_OVERLAPPEDWINDOW
#define WS_OVERLAPPEDWINDOW WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX
#endif

#ifndef WS_TILED
#define WS_TILED WS_OVERLAPPED
#endif

#ifndef WS_TILEDWINDOW
#define WS_TILEDWINDOW WS_OVERLAPPEDWINDOW
#endif

#ifndef WS_EX_CAPTIONOKBTN
#define WS_EX_CAPTIONOKBTN 0x80000000L
#endif

#ifndef WS_EX_NODRAG
#define WS_EX_NODRAG       0x40000000L
#endif

#ifdef __cplusplus
} // Extern C.
#endif

#ifdef __cplusplus


// As Windows CE lacks some standard functions used in Qt, these got
// reimplemented. Other projects do this as well. Inline functions are used
// that there is a central place to disable functions for newer versions if
// they get available. There are no defines used anymore, because this
// will break member functions of classes which are called like these
// functions. Also inline functions are only supported by C++, so just define
// them for C++, as only 3rd party dependencies are C, this is no issue.
// The other declarations available in this file are being used per
// define inside qplatformdefs.h of the corresponding WinCE mkspec.

#define generate_inline_return_func0(funcname, returntype) \
        inline returntype funcname() \
        { \
            return qt_wince_##funcname(); \
        }
#define generate_inline_return_func1(funcname, returntype, param1) \
        inline returntype funcname(param1 p1) \
        { \
            return qt_wince_##funcname(p1); \
        }
#define generate_inline_return_func2(funcname, returntype, param1, param2) \
        inline returntype funcname(param1 p1, param2 p2) \
        { \
            return qt_wince_##funcname(p1,  p2); \
        }
#define generate_inline_return_func3(funcname, returntype, param1, param2, param3) \
        inline returntype funcname(param1 p1, param2 p2, param3 p3) \
        { \
            return qt_wince_##funcname(p1,  p2, p3); \
        }
#define generate_inline_return_func4(funcname, returntype, param1, param2, param3, param4) \
        inline returntype funcname(param1 p1, param2 p2, param3 p3, param4 p4) \
        { \
            return qt_wince_##funcname(p1,  p2, p3, p4); \
        }
#define generate_inline_return_func5(funcname, returntype, param1, param2, param3, param4, param5) \
        inline returntype funcname(param1 p1, param2 p2, param3 p3, param4 p4, param5 p5) \
        { \
            return qt_wince_##funcname(p1,  p2, p3, p4, p5); \
        }
#define generate_inline_return_func6(funcname, returntype, param1, param2, param3, param4, param5, param6) \
        inline returntype funcname(param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6) \
        { \
            return qt_wince_##funcname(p1,  p2, p3, p4, p5, p6); \
        }
#define generate_inline_return_func7(funcname, returntype, param1, param2, param3, param4, param5, param6, param7) \
        inline returntype funcname(param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6, param7 p7) \
        { \
            return qt_wince_##funcname(p1,  p2, p3, p4, p5, p6, p7); \
        }

typedef unsigned (__stdcall *StartAdressExFunc)(void *);
typedef void(*StartAdressFunc)(void *);
typedef int ( __cdecl *CompareFunc ) (const void *, const void *) ;

generate_inline_return_func4(getenv_s, errno_t, size_t *, char *, size_t, const char *)
generate_inline_return_func2(_putenv_s, errno_t, const char *, const char *)
generate_inline_return_func0(_getpid, int)
generate_inline_return_func1(time_tToFt, FILETIME, time_t)
generate_inline_return_func1(ftToTime_t, time_t, FILETIME)
generate_inline_return_func0(_getdrive, int)
generate_inline_return_func2(_waccess, int, const wchar_t *, int)
generate_inline_return_func3(_wopen, int, const wchar_t *, int, int)
generate_inline_return_func2(_fdopen, FILE *, int, const char *)
generate_inline_return_func2(fdopen, FILE *, int, const char *)
generate_inline_return_func1(rewind, void, FILE *)
generate_inline_return_func0(tmpfile, FILE *)
generate_inline_return_func2(_rename, int, const char *, const char *)
generate_inline_return_func1(_remove, int, const char *)
generate_inline_return_func1(SetErrorMode, int, int)
generate_inline_return_func2(_chmod, bool, const char *, int)
generate_inline_return_func2(_wchmod, bool, const wchar_t *, int)
generate_inline_return_func7(CreateFileA, HANDLE, LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE)
generate_inline_return_func4(SetWindowOrgEx, BOOL, HDC, int, int, LPPOINT)
generate_inline_return_func2(calloc, void *, size_t, size_t)
generate_inline_return_func0(GetThreadLocale, DWORD)
generate_inline_return_func3(_beginthread, HANDLE, StartAdressFunc, unsigned, void *)
generate_inline_return_func6(_beginthreadex, unsigned long, void *, unsigned, StartAdressExFunc, void *, unsigned, unsigned *)
generate_inline_return_func1(_endthreadex, void, unsigned)
generate_inline_return_func5(bsearch, void *, const void *, const void *, size_t, size_t, CompareFunc)

#endif //__cplusplus

#endif // Q_OS_WINCE
#endif // QFUNCTIONS_WINCE_H
