#ifdef MG_MODULE_LINES
#line 1 "mongoose/src/common.h"
#endif
/*
 * Copyright (c) 2004-2013 Sergey Lyubka
 * Copyright (c) 2013-2015 Cesanta Software Limited
 * All rights reserved
 *
 * This software is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. For the terms of this
 * license, see <http://www.gnu.org/licenses/>.
 *
 * You are free to use this software under the terms of the GNU General
 * Public License, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * Alternatively, you can license this software under a commercial
 * license, as set out in <https://www.cesanta.com/license>.
 */

#ifndef CS_MONGOOSE_SRC_COMMON_H_
#define CS_MONGOOSE_SRC_COMMON_H_

#define MG_VERSION "6.10"

/* Local tweaks, applied before any of Mongoose's own headers. */
#ifdef MG_LOCALS
#include <mg_locals.h>
#endif

#endif /* CS_MONGOOSE_SRC_COMMON_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/platform.h"
#endif
#ifndef CS_COMMON_PLATFORM_H_
#define CS_COMMON_PLATFORM_H_

/*
 * For the "custom" platform, includes and dependencies can be
 * provided through mg_locals.h.
 */
#define CS_P_CUSTOM 0
#define CS_P_UNIX 1
#define CS_P_WINDOWS 2
#define CS_P_ESP32 15
#define CS_P_ESP8266 3
#define CS_P_CC3100 6
#define CS_P_CC3200 4
#define CS_P_CC3220 17
#define CS_P_MSP432 5
#define CS_P_TM4C129 14
#define CS_P_MBED 7
#define CS_P_WINCE 8
#define CS_P_NXP_LPC 13
#define CS_P_NXP_KINETIS 9
#define CS_P_NRF51 12
#define CS_P_NRF52 10
#define CS_P_PIC32 11
#define CS_P_STM32 16
/* Next id: 18 */

/* If not specified explicitly, we guess platform by defines. */
#ifndef CS_PLATFORM

#if defined(TARGET_IS_MSP432P4XX) || defined(__MSP432P401R__)
#define CS_PLATFORM CS_P_MSP432
#elif defined(cc3200) || defined(TARGET_IS_CC3200)
#define CS_PLATFORM CS_P_CC3200
#elif defined(cc3220) || defined(TARGET_IS_CC3220)
#define CS_PLATFORM CS_P_CC3220
#elif defined(__unix__) || defined(__APPLE__)
#define CS_PLATFORM CS_P_UNIX
#elif defined(WINCE)
#define CS_PLATFORM CS_P_WINCE
#elif defined(_WIN32)
#define CS_PLATFORM CS_P_WINDOWS
#elif defined(__MBED__)
#define CS_PLATFORM CS_P_MBED
#elif defined(__USE_LPCOPEN)
#define CS_PLATFORM CS_P_NXP_LPC
#elif defined(FRDM_K64F) || defined(FREEDOM)
#define CS_PLATFORM CS_P_NXP_KINETIS
#elif defined(PIC32)
#define CS_PLATFORM CS_P_PIC32
#elif defined(ESP_PLATFORM)
#define CS_PLATFORM CS_P_ESP32
#elif defined(ICACHE_FLASH)
#define CS_PLATFORM CS_P_ESP8266
#elif defined(TARGET_IS_TM4C129_RA0) || defined(TARGET_IS_TM4C129_RA1) || \
    defined(TARGET_IS_TM4C129_RA2)
#define CS_PLATFORM CS_P_TM4C129
#elif defined(STM32)
#define CS_PLATFORM CS_P_STM32
#endif

#ifndef CS_PLATFORM
#error "CS_PLATFORM is not specified and we couldn't guess it."
#endif

#endif /* !defined(CS_PLATFORM) */

#define MG_NET_IF_SOCKET 1
#define MG_NET_IF_SIMPLELINK 2
#define MG_NET_IF_LWIP_LOW_LEVEL 3
#define MG_NET_IF_PIC32 4

#define MG_SSL_IF_OPENSSL 1
#define MG_SSL_IF_MBEDTLS 2
#define MG_SSL_IF_SIMPLELINK 3

/* Amalgamated: #include "common/platforms/platform_unix.h" */
/* Amalgamated: #include "common/platforms/platform_windows.h" */
/* Amalgamated: #include "common/platforms/platform_esp32.h" */
/* Amalgamated: #include "common/platforms/platform_esp8266.h" */
/* Amalgamated: #include "common/platforms/platform_cc3100.h" */
/* Amalgamated: #include "common/platforms/platform_cc3200.h" */
/* Amalgamated: #include "common/platforms/platform_cc3220.h" */
/* Amalgamated: #include "common/platforms/platform_mbed.h" */
/* Amalgamated: #include "common/platforms/platform_nrf51.h" */
/* Amalgamated: #include "common/platforms/platform_nrf52.h" */
/* Amalgamated: #include "common/platforms/platform_wince.h" */
/* Amalgamated: #include "common/platforms/platform_nxp_lpc.h" */
/* Amalgamated: #include "common/platforms/platform_nxp_kinetis.h" */
/* Amalgamated: #include "common/platforms/platform_pic32.h" */
/* Amalgamated: #include "common/platforms/platform_stm32.h" */

/* Common stuff */

#if !defined(WEAK)
#if (defined(__GNUC__) || defined(__TI_COMPILER_VERSION__)) && !defined(_WIN32)
#define WEAK __attribute__((weak))
#else
#define WEAK
#endif
#endif

#ifdef __GNUC__
#define NORETURN __attribute__((noreturn))
#define NOINLINE __attribute__((noinline))
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#define NOINSTR __attribute__((no_instrument_function))
#define DO_NOT_WARN_UNUSED __attribute__((unused))
#else
#define NORETURN
#define NOINLINE
#define WARN_UNUSED_RESULT
#define NOINSTR
#define DO_NOT_WARN_UNUSED
#endif /* __GNUC__ */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
#endif

#endif /* CS_COMMON_PLATFORM_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/platforms/platform_windows.h"
#endif
#ifndef CS_COMMON_PLATFORMS_PLATFORM_WINDOWS_H_
#define CS_COMMON_PLATFORMS_PLATFORM_WINDOWS_H_
#if CS_PLATFORM == CS_P_WINDOWS

/*
 * MSVC++ 14.0 _MSC_VER == 1900 (Visual Studio 2015)
 * MSVC++ 12.0 _MSC_VER == 1800 (Visual Studio 2013)
 * MSVC++ 11.0 _MSC_VER == 1700 (Visual Studio 2012)
 * MSVC++ 10.0 _MSC_VER == 1600 (Visual Studio 2010)
 * MSVC++ 9.0  _MSC_VER == 1500 (Visual Studio 2008)
 * MSVC++ 8.0  _MSC_VER == 1400 (Visual Studio 2005)
 * MSVC++ 7.1  _MSC_VER == 1310 (Visual Studio 2003)
 * MSVC++ 7.0  _MSC_VER == 1300
 * MSVC++ 6.0  _MSC_VER == 1200
 * MSVC++ 5.0  _MSC_VER == 1100
 */
#ifdef _MSC_VER
#pragma warning(disable : 4127) /* FD_SET() emits warning, disable it */
#pragma warning(disable : 4204) /* missing c99 support */
#endif

#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#include <direct.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <limits.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <ctype.h>

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib") /* Linking with winsock library */
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>

#if _MSC_VER < 1700
typedef int bool;
#else
#include <stdbool.h>
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1800
#define strdup _strdup
#endif

#ifndef EINPROGRESS
#define EINPROGRESS WSAEINPROGRESS
#endif
#ifndef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif
#ifndef __func__
#define STRX(x) #x
#define STR(x) STRX(x)
#define __func__ __FILE__ ":" STR(__LINE__)
#endif
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define to64(x) _atoi64(x)
#if !defined(__MINGW32__) && !defined(__MINGW64__)
#define popen(x, y) _popen((x), (y))
#define pclose(x) _pclose(x)
#define fileno _fileno
#endif
#if defined(_MSC_VER) && _MSC_VER >= 1400
#define fseeko(x, y, z) _fseeki64((x), (y), (z))
#else
#define fseeko(x, y, z) fseek((x), (y), (z))
#endif
#if defined(_MSC_VER) && _MSC_VER <= 1200
typedef unsigned long uintptr_t;
typedef long intptr_t;
#endif
typedef int socklen_t;
#if _MSC_VER >= 1700
#include <stdint.h>
#else
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#endif
typedef SOCKET sock_t;
typedef uint32_t in_addr_t;
#ifndef UINT16_MAX
#define UINT16_MAX 65535
#endif
#ifndef UINT32_MAX
#define UINT32_MAX 4294967295
#endif
#ifndef pid_t
#define pid_t HANDLE
#endif
#define INT64_FMT "I64d"
#define INT64_X_FMT "I64x"
#define SIZE_T_FMT "Iu"
typedef struct _stati64 cs_stat_t;
#ifndef S_ISDIR
#define S_ISDIR(x) (((x) &_S_IFMT) == _S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(x) (((x) &_S_IFMT) == _S_IFREG)
#endif
#define DIRSEP '\\'
#define CS_DEFINE_DIRENT

#ifndef va_copy
#ifdef __va_copy
#define va_copy __va_copy
#else
#define va_copy(x, y) (x) = (y)
#endif
#endif

#ifndef MG_MAX_HTTP_REQUEST_SIZE
#define MG_MAX_HTTP_REQUEST_SIZE 8192
#endif

#ifndef MG_MAX_HTTP_SEND_MBUF
#define MG_MAX_HTTP_SEND_MBUF 4096
#endif

#ifndef MG_MAX_HTTP_HEADERS
#define MG_MAX_HTTP_HEADERS 40
#endif

#ifndef CS_ENABLE_STDIO
#define CS_ENABLE_STDIO 1
#endif

#ifndef MG_ENABLE_BROADCAST
#define MG_ENABLE_BROADCAST 1
#endif

#ifndef MG_ENABLE_DIRECTORY_LISTING
#define MG_ENABLE_DIRECTORY_LISTING 1
#endif

#ifndef MG_ENABLE_FILESYSTEM
#define MG_ENABLE_FILESYSTEM 1
#endif

#ifndef MG_ENABLE_HTTP_CGI
#define MG_ENABLE_HTTP_CGI MG_ENABLE_FILESYSTEM
#endif

#ifndef MG_NET_IF
#define MG_NET_IF MG_NET_IF_SOCKET
#endif

unsigned int sleep(unsigned int seconds);

/* https://stackoverflow.com/questions/16647819/timegm-cross-platform */
#define timegm _mkgmtime

#define gmtime_r(a, b) \
  do {                 \
    *(b) = *gmtime(a); \
  } while (0)

#endif /* CS_PLATFORM == CS_P_WINDOWS */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_WINDOWS_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/platforms/platform_unix.h"
#endif
#ifndef CS_COMMON_PLATFORMS_PLATFORM_UNIX_H_
#define CS_COMMON_PLATFORMS_PLATFORM_UNIX_H_
#if CS_PLATFORM == CS_P_UNIX

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

/* <inttypes.h> wants this for C++ */
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

/* C++ wants that for INT64_MAX */
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

/* Enable fseeko() and ftello() functions */
#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE
#endif

/* Enable 64-bit file offsets */
#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __APPLE__
#include <machine/endian.h>
#ifndef BYTE_ORDER
#define LITTLE_ENDIAN __DARWIN_LITTLE_ENDIAN
#define BIG_ENDIAN __DARWIN_BIG_ENDIAN
#define PDP_ENDIAN __DARWIN_PDP_ENDIAN
#define BYTE_ORDER __DARWIN_BYTE_ORDER
#endif
#endif

/*
 * osx correctly avoids defining strtoll when compiling in strict ansi mode.
 * c++ 11 standard defines strtoll as well.
 * We require strtoll, and if your embedded pre-c99 compiler lacks one, please
 * implement a shim.
 */
#if !(defined(__cplusplus) && __cplusplus >= 201103L) && \
    !(defined(__DARWIN_C_LEVEL) && __DARWIN_C_LEVEL >= 200809L)
long long strtoll(const char *, char **, int);
#endif

typedef int sock_t;
#define INVALID_SOCKET (-1)
#define SIZE_T_FMT "zu"
typedef struct stat cs_stat_t;
#define DIRSEP '/'
#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT PRId64
#define INT64_X_FMT PRIx64

#ifndef __cdecl
#define __cdecl
#endif

#ifndef va_copy
#ifdef __va_copy
#define va_copy __va_copy
#else
#define va_copy(x, y) (x) = (y)
#endif
#endif

#define closesocket(x) close(x)

#ifndef MG_MAX_HTTP_REQUEST_SIZE
#define MG_MAX_HTTP_REQUEST_SIZE 8192
#endif

#ifndef MG_MAX_HTTP_SEND_MBUF
#define MG_MAX_HTTP_SEND_MBUF 4096
#endif

#ifndef MG_MAX_HTTP_HEADERS
#define MG_MAX_HTTP_HEADERS 40
#endif

#ifndef CS_ENABLE_STDIO
#define CS_ENABLE_STDIO 1
#endif

#ifndef MG_ENABLE_BROADCAST
#define MG_ENABLE_BROADCAST 1
#endif

#ifndef MG_ENABLE_DIRECTORY_LISTING
#define MG_ENABLE_DIRECTORY_LISTING 1
#endif

#ifndef MG_ENABLE_FILESYSTEM
#define MG_ENABLE_FILESYSTEM 1
#endif

#ifndef MG_ENABLE_HTTP_CGI
#define MG_ENABLE_HTTP_CGI MG_ENABLE_FILESYSTEM
#endif

#ifndef MG_NET_IF
#define MG_NET_IF MG_NET_IF_SOCKET
#endif

#ifndef MG_HOSTS_FILE_NAME
#define MG_HOSTS_FILE_NAME "/etc/hosts"
#endif

#ifndef MG_RESOLV_CONF_FILE_NAME
#define MG_RESOLV_CONF_FILE_NAME "/etc/resolv.conf"
#endif

#endif /* CS_PLATFORM == CS_P_UNIX */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_UNIX_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/platforms/platform_esp32.h"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_PLATFORM_ESP32_H_
#define CS_COMMON_PLATFORMS_PLATFORM_ESP32_H_
#if CS_PLATFORM == CS_P_ESP32

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <inttypes.h>
#include <machine/endian.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

#define SIZE_T_FMT "u"
typedef struct stat cs_stat_t;
#define DIRSEP '/'
#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT PRId64
#define INT64_X_FMT PRIx64
#define __cdecl
#define _FILE_OFFSET_BITS 32

#define MG_LWIP 1

#ifndef MG_NET_IF
#define MG_NET_IF MG_NET_IF_SOCKET
#endif

#ifndef CS_ENABLE_STDIO
#define CS_ENABLE_STDIO 1
#endif

#endif /* CS_PLATFORM == CS_P_ESP32 */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_ESP32_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/platforms/platform_esp8266.h"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_PLATFORM_ESP8266_H_
#define CS_COMMON_PLATFORMS_PLATFORM_ESP8266_H_
#if CS_PLATFORM == CS_P_ESP8266

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <inttypes.h>
#include <machine/endian.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

#define SIZE_T_FMT "u"
typedef struct stat cs_stat_t;
#define DIRSEP '/'
#if !defined(MGOS_VFS_DEFINE_DIRENT)
#define CS_DEFINE_DIRENT
#endif

#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT PRId64
#define INT64_X_FMT PRIx64
#define __cdecl
#define _FILE_OFFSET_BITS 32

#if !defined(RTOS_SDK) && !defined(__cplusplus)
#define fileno(x) -1
#endif

#define MG_LWIP 1

/* struct timeval is defined in sys/time.h. */
#define LWIP_TIMEVAL_PRIVATE 0

#ifndef MG_NET_IF
#include <lwip/opt.h>
#if LWIP_SOCKET /* RTOS SDK has LWIP sockets */
#define MG_NET_IF MG_NET_IF_SOCKET
#else
#define MG_NET_IF MG_NET_IF_LWIP_LOW_LEVEL
#endif
#endif

#ifndef CS_ENABLE_STDIO
#define CS_ENABLE_STDIO 1
#endif

#define inet_ntop(af, src, dst, size)                                          \
  (((af) == AF_INET) ? ipaddr_ntoa_r((const ip_addr_t *) (src), (dst), (size)) \
                     : NULL)
#define inet_pton(af, src, dst) \
  (((af) == AF_INET) ? ipaddr_aton((src), (ip_addr_t *) (dst)) : 0)

#endif /* CS_PLATFORM == CS_P_ESP8266 */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_ESP8266_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/platforms/platform_cc3100.h"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_PLATFORM_CC3100_H_
#define CS_COMMON_PLATFORMS_PLATFORM_CC3100_H_
#if CS_PLATFORM == CS_P_CC3100

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define MG_NET_IF MG_NET_IF_SIMPLELINK
#define MG_SSL_IF MG_SSL_IF_SIMPLELINK

/*
 * CC3100 SDK and STM32 SDK include headers w/out path, just like
 * #include "simplelink.h". As result, we have to add all required directories
 * into Makefile IPATH and do the same thing (include w/out path)
 */

#include <simplelink.h>
#include <netapp.h>
#undef timeval

typedef int sock_t;
#define INVALID_SOCKET (-1)

#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT PRId64
#define INT64_X_FMT PRIx64
#define SIZE_T_FMT "u"

#define SOMAXCONN 8

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
char *inet_ntoa(struct in_addr in);
int inet_pton(int af, const char *src, void *dst);

#endif /* CS_PLATFORM == CS_P_CC3100 */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_CC3100_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/platforms/platform_cc3200.h"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_PLATFORM_CC3200_H_
#define CS_COMMON_PLATFORMS_PLATFORM_CC3200_H_
#if CS_PLATFORM == CS_P_CC3200

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#ifndef __TI_COMPILER_VERSION__
#include <fcntl.h>
#include <sys/time.h>
#endif

#define MG_NET_IF MG_NET_IF_SIMPLELINK
#define MG_SSL_IF MG_SSL_IF_SIMPLELINK

/* Only SPIFFS supports directories, SLFS does not. */
#if defined(CC3200_FS_SPIFFS) && !defined(MG_ENABLE_DIRECTORY_LISTING)
#define MG_ENABLE_DIRECTORY_LISTING 1
#endif

/* Amalgamated: #include "common/platforms/simplelink/cs_simplelink.h" */

typedef int sock_t;
#define INVALID_SOCKET (-1)
#define SIZE_T_FMT "u"
typedef struct stat cs_stat_t;
#define DIRSEP '/'
#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT PRId64
#define INT64_X_FMT PRIx64
#define __cdecl

#define fileno(x) -1

/* Some functions we implement for Mongoose. */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __TI_COMPILER_VERSION__
struct SlTimeval_t;
#define timeval SlTimeval_t
int gettimeofday(struct timeval *t, void *tz);
int settimeofday(const struct timeval *tv, const void *tz);

int asprintf(char **strp, const char *fmt, ...);

#endif

/* TI's libc does not have stat & friends, add them. */
#ifdef __TI_COMPILER_VERSION__

#include <file.h>

typedef unsigned int mode_t;
typedef size_t _off_t;
typedef long ssize_t;

struct stat {
  int st_ino;
  mode_t st_mode;
  int st_nlink;
  time_t st_mtime;
  off_t st_size;
};

int _stat(const char *pathname, struct stat *st);
int stat(const char *pathname, struct stat *st);

#define __S_IFMT 0170000

#define __S_IFDIR 0040000
#define __S_IFCHR 0020000
#define __S_IFREG 0100000

#define __S_ISTYPE(mode, mask) (((mode) &__S_IFMT) == (mask))

#define S_IFDIR __S_IFDIR
#define S_IFCHR __S_IFCHR
#define S_IFREG __S_IFREG
#define S_ISDIR(mode) __S_ISTYPE((mode), __S_IFDIR)
#define S_ISREG(mode) __S_ISTYPE((mode), __S_IFREG)

/* 5.x series compilers don't have va_copy, 16.x do. */
#if __TI_COMPILER_VERSION__ < 16000000
#define va_copy(apc, ap) ((apc) = (ap))
#endif

#endif /* __TI_COMPILER_VERSION__ */

#ifdef CC3200_FS_SLFS
#define MG_FS_SLFS
#endif

#if (defined(CC3200_FS_SPIFFS) || defined(CC3200_FS_SLFS)) && \
    !defined(MG_ENABLE_FILESYSTEM)
#define MG_ENABLE_FILESYSTEM 1
#define CS_DEFINE_DIRENT
#endif

#ifndef CS_ENABLE_STDIO
#define CS_ENABLE_STDIO 1
#endif

#ifdef __cplusplus
}
#endif

#endif /* CS_PLATFORM == CS_P_CC3200 */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_CC3200_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/platforms/platform_msp432.h"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_PLATFORM_MSP432_H_
#define CS_COMMON_PLATFORMS_PLATFORM_MSP432_H_
#if CS_PLATFORM == CS_P_MSP432

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#ifndef __TI_COMPILER_VERSION__
#include <fcntl.h>
#include <sys/time.h>
#endif

#define MG_NET_IF MG_NET_IF_SIMPLELINK
#define MG_SSL_IF MG_SSL_IF_SIMPLELINK

/* Amalgamated: #include "common/platforms/simplelink/cs_simplelink.h" */

typedef int sock_t;
#define INVALID_SOCKET (-1)
#define SIZE_T_FMT "u"
typedef struct stat cs_stat_t;
#define DIRSEP '/'
#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT PRId64
#define INT64_X_FMT PRIx64
#define __cdecl

#define fileno(x) -1

/* Some functions we implement for Mongoose. */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __TI_COMPILER_VERSION__
struct SlTimeval_t;
#define timeval SlTimeval_t
int gettimeofday(struct timeval *t, void *tz);
#endif

/* TI's libc does not have stat & friends, add them. */
#ifdef __TI_COMPILER_VERSION__

#include <file.h>

typedef unsigned int mode_t;
typedef size_t _off_t;
typedef long ssize_t;

struct stat {
  int st_ino;
  mode_t st_mode;
  int st_nlink;
  time_t st_mtime;
  off_t st_size;
};

int _stat(const char *pathname, struct stat *st);
#define stat(a, b) _stat(a, b)

#define __S_IFMT 0170000

#define __S_IFDIR 0040000
#define __S_IFCHR 0020000
#define __S_IFREG 0100000

#define __S_ISTYPE(mode, mask) (((mode) &__S_IFMT) == (mask))

#define S_IFDIR __S_IFDIR
#define S_IFCHR __S_IFCHR
#define S_IFREG __S_IFREG
#define S_ISDIR(mode) __S_ISTYPE((mode), __S_IFDIR)
#define S_ISREG(mode) __S_ISTYPE((mode), __S_IFREG)

/* As of 5.2.7, TI compiler does not support va_copy() yet. */
#define va_copy(apc, ap) ((apc) = (ap))

#endif /* __TI_COMPILER_VERSION__ */

#ifndef CS_ENABLE_STDIO
#define CS_ENABLE_STDIO 1
#endif

#if (defined(CC3200_FS_SPIFFS) || defined(CC3200_FS_SLFS)) && \
    !defined(MG_ENABLE_FILESYSTEM)
#define MG_ENABLE_FILESYSTEM 1
#endif

#ifdef __cplusplus
}
#endif

#endif /* CS_PLATFORM == CS_P_MSP432 */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_MSP432_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/platforms/platform_tm4c129.h"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_PLATFORM_TM4C129_H_
#define CS_COMMON_PLATFORMS_PLATFORM_TM4C129_H_
#if CS_PLATFORM == CS_P_TM4C129

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#ifndef __TI_COMPILER_VERSION__
#include <fcntl.h>
#include <sys/time.h>
#endif

#define SIZE_T_FMT "u"
typedef struct stat cs_stat_t;
#define DIRSEP '/'
#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT PRId64
#define INT64_X_FMT PRIx64
#define __cdecl

#ifndef MG_NET_IF
#include <lwip/opt.h>
#if LWIP_SOCKET
#define MG_NET_IF MG_NET_IF_SOCKET
#else
#define MG_NET_IF MG_NET_IF_LWIP_LOW_LEVEL
#endif
#define MG_LWIP 1
#elif MG_NET_IF == MG_NET_IF_SIMPLELINK
/* Amalgamated: #include "common/platforms/simplelink/cs_simplelink.h" */
#endif

#ifndef CS_ENABLE_STDIO
#define CS_ENABLE_STDIO 1
#endif

#ifdef __TI_COMPILER_VERSION__
/* As of 5.2.8, TI compiler does not support va_copy() yet. */
#define va_copy(apc, ap) ((apc) = (ap))
#endif /* __TI_COMPILER_VERSION__ */

#ifdef __cplusplus
}
#endif

#endif /* CS_PLATFORM == CS_P_TM4C129 */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_TM4C129_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/platforms/platform_mbed.h"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_PLATFORM_MBED_H_
#define CS_COMMON_PLATFORMS_PLATFORM_MBED_H_
#if CS_PLATFORM == CS_P_MBED

/*
 * mbed.h contains C++ code (e.g. templates), thus, it should be processed
 * only if included directly to startup file (ex: main.cpp)
 */
#ifdef __cplusplus
/* Amalgamated: #include "mbed.h" */
#endif /* __cplusplus */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>

typedef struct stat cs_stat_t;
#define DIRSEP '/'

#ifndef CS_ENABLE_STDIO
#define CS_ENABLE_STDIO 1
#endif

/*
 * mbed can be compiled with the ARM compiler which
 * just doesn't come with a gettimeofday shim
 * because it's a BSD API and ARM targets embedded
 * non-unix platforms.
 */
#if defined(__ARMCC_VERSION) || defined(__ICCARM__)
#define _TIMEVAL_DEFINED
#define gettimeofday _gettimeofday

/* copied from GCC on ARM; for some reason useconds are signed */
typedef long suseconds_t; /* microseconds (signed) */
struct timeval {
  time_t tv_sec;       /* seconds */
  suseconds_t tv_usec; /* and microseconds */
};

#endif

#if MG_NET_IF == MG_NET_IF_SIMPLELINK

#define MG_SIMPLELINK_NO_OSI 1

#include <simplelink.h>

typedef int sock_t;
#define INVALID_SOCKET (-1)

#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT PRId64
#define INT64_X_FMT PRIx64
#define SIZE_T_FMT "u"

#define SOMAXCONN 8

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
char *inet_ntoa(struct in_addr in);
int inet_pton(int af, const char *src, void *dst);
int inet_aton(const char *cp, struct in_addr *inp);
in_addr_t inet_addr(const char *cp);

#endif /* MG_NET_IF == MG_NET_IF_SIMPLELINK */

#endif /* CS_PLATFORM == CS_P_MBED */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_MBED_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/platforms/platform_nrf51.h"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */
#ifndef CS_COMMON_PLATFORMS_PLATFORM_NRF51_H_
#define CS_COMMON_PLATFORMS_PLATFORM_NRF51_H_
#if CS_PLATFORM == CS_P_NRF51

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define to64(x) strtoll(x, NULL, 10)

#define MG_NET_IF MG_NET_IF_LWIP_LOW_LEVEL
#define MG_LWIP 1
#define MG_ENABLE_IPV6 1

/*
 * For ARM C Compiler, make lwip to export `struct timeval`; for other
 * compilers, suppress it.
 */
#if !defined(__ARMCC_VERSION)
#define LWIP_TIMEVAL_PRIVATE 0
#else
struct timeval;
int gettimeofday(struct timeval *tp, void *tzp);
#endif

#define INT64_FMT PRId64
#define SIZE_T_FMT "u"

/*
 * ARM C Compiler doesn't have strdup, so we provide it
 */
#define CS_ENABLE_STRDUP defined(__ARMCC_VERSION)

#endif /* CS_PLATFORM == CS_P_NRF51 */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_NRF51_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/platforms/platform_nrf52.h"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */
#ifndef CS_COMMON_PLATFORMS_PLATFORM_NRF52_H_
#define CS_COMMON_PLATFORMS_PLATFORM_NRF52_H_
#if CS_PLATFORM == CS_P_NRF52

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define to64(x) strtoll(x, NULL, 10)

#define MG_NET_IF MG_NET_IF_LWIP_LOW_LEVEL
#define MG_LWIP 1
#define MG_ENABLE_IPV6 1

#if !defined(ENOSPC)
#define ENOSPC 28 /* No space left on device */
#endif

/*
 * For ARM C Compiler, make lwip to export `struct timeval`; for other
 * compilers, suppress it.
 */
#if !defined(__ARMCC_VERSION)
#define LWIP_TIMEVAL_PRIVATE 0
#endif

#define INT64_FMT PRId64
#define SIZE_T_FMT "u"

/*
 * ARM C Compiler doesn't have strdup, so we provide it
 */
#define CS_ENABLE_STRDUP defined(__ARMCC_VERSION)

#endif /* CS_PLATFORM == CS_P_NRF52 */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_NRF52_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/platforms/simplelink/cs_simplelink.h"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_SIMPLELINK_CS_SIMPLELINK_H_
#define CS_COMMON_PLATFORMS_SIMPLELINK_CS_SIMPLELINK_H_

#if defined(MG_NET_IF) && MG_NET_IF == MG_NET_IF_SIMPLELINK

/* If simplelink.h is already included, all bets are off. */
#if !defined(__SIMPLELINK_H__)

#include <stdbool.h>

#ifndef __TI_COMPILER_VERSION__
#undef __CONCAT
#undef FD_CLR
#undef FD_ISSET
#undef FD_SET
#undef FD_SETSIZE
#undef FD_ZERO
#undef fd_set
#endif

#if CS_PLATFORM == CS_P_CC3220
#include <ti/drivers/net/wifi/porting/user.h>
#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/drivers/net/wifi/sl_socket.h>
#include <ti/drivers/net/wifi/netapp.h>
#else
/* We want to disable SL_INC_STD_BSD_API_NAMING, so we include user.h ourselves
 * and undef it. */
#define PROVISIONING_API_H_
#include <simplelink/user.h>
#undef PROVISIONING_API_H_
#undef SL_INC_STD_BSD_API_NAMING

#include <simplelink/include/simplelink.h>
#include <simplelink/include/netapp.h>
#endif /* CS_PLATFORM == CS_P_CC3220 */

/* Now define only the subset of the BSD API that we use.
 * Notably, close(), read() and write() are not defined. */
#define AF_INET SL_AF_INET

#define socklen_t SlSocklen_t
#define sockaddr SlSockAddr_t
#define sockaddr_in SlSockAddrIn_t
#define in_addr SlInAddr_t

#define SOCK_STREAM SL_SOCK_STREAM
#define SOCK_DGRAM SL_SOCK_DGRAM

#define htonl sl_Htonl
#define ntohl sl_Ntohl
#define htons sl_Htons
#define ntohs sl_Ntohs

#ifndef EACCES
#define EACCES SL_EACCES
#endif
#ifndef EAFNOSUPPORT
#define EAFNOSUPPORT SL_EAFNOSUPPORT
#endif
#ifndef EAGAIN
#define EAGAIN SL_EAGAIN
#endif
#ifndef EBADF
#define EBADF SL_EBADF
#endif
#ifndef EINVAL
#define EINVAL SL_EINVAL
#endif
#ifndef ENOMEM
#define ENOMEM SL_ENOMEM
#endif
#ifndef EWOULDBLOCK
#define EWOULDBLOCK SL_EWOULDBLOCK
#endif

#define SOMAXCONN 8

#ifdef __cplusplus
extern "C" {
#endif

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
char *inet_ntoa(struct in_addr in);
int inet_pton(int af, const char *src, void *dst);

struct mg_mgr;
struct mg_connection;

typedef void (*mg_init_cb)(struct mg_mgr *mgr);
bool mg_start_task(int priority, int stack_size, mg_init_cb mg_init);

void mg_run_in_task(void (*cb)(struct mg_mgr *mgr, void *arg), void *cb_arg);

int sl_fs_init(void);

void sl_restart_cb(struct mg_mgr *mgr);

int sl_set_ssl_opts(int sock, struct mg_connection *nc);

#ifdef __cplusplus
}
#endif

#endif /* !defined(__SIMPLELINK_H__) */

/* Compatibility with older versions of SimpleLink */
#if SL_MAJOR_VERSION_NUM < 2

#define SL_ERROR_BSD_EAGAIN SL_EAGAIN
#define SL_ERROR_BSD_EALREADY SL_EALREADY
#define SL_ERROR_BSD_ENOPROTOOPT SL_ENOPROTOOPT
#define SL_ERROR_BSD_ESECDATEERROR SL_ESECDATEERROR
#define SL_ERROR_BSD_ESECSNOVERIFY SL_ESECSNOVERIFY
#define SL_ERROR_FS_FAILED_TO_ALLOCATE_MEM SL_FS_ERR_FAILED_TO_ALLOCATE_MEM
#define SL_ERROR_FS_FILE_HAS_NOT_BEEN_CLOSE_CORRECTLY \
  SL_FS_FILE_HAS_NOT_BEEN_CLOSE_CORRECTLY
#define SL_ERROR_FS_FILE_NAME_EXIST SL_FS_FILE_NAME_EXIST
#define SL_ERROR_FS_FILE_NOT_EXISTS SL_FS_ERR_FILE_NOT_EXISTS
#define SL_ERROR_FS_NO_AVAILABLE_NV_INDEX SL_FS_ERR_NO_AVAILABLE_NV_INDEX
#define SL_ERROR_FS_NOT_ENOUGH_STORAGE_SPACE SL_FS_ERR_NO_AVAILABLE_BLOCKS
#define SL_ERROR_FS_NOT_SUPPORTED SL_FS_ERR_NOT_SUPPORTED
#define SL_ERROR_FS_WRONG_FILE_NAME SL_FS_WRONG_FILE_NAME
#define SL_ERROR_FS_INVALID_HANDLE SL_FS_ERR_INVALID_HANDLE
#define SL_NETCFG_MAC_ADDRESS_GET SL_MAC_ADDRESS_GET
#define SL_SOCKET_FD_ZERO SL_FD_ZERO
#define SL_SOCKET_FD_SET SL_FD_SET
#define SL_SOCKET_FD_ISSET SL_FD_ISSET
#define SL_SO_SECURE_DOMAIN_NAME_VERIFICATION SO_SECURE_DOMAIN_NAME_VERIFICATION

#define SL_FS_READ FS_MODE_OPEN_READ
#define SL_FS_WRITE FS_MODE_OPEN_WRITE

#define SL_FI_FILE_SIZE(fi) ((fi).FileLen)
#define SL_FI_FILE_MAX_SIZE(fi) ((fi).AllocatedLen)

#define SlDeviceVersion_t SlVersionFull
#define sl_DeviceGet sl_DevGet
#define SL_DEVICE_GENERAL SL_DEVICE_GENERAL_CONFIGURATION
#define SL_LEN_TYPE _u8
#define SL_OPT_TYPE _u8

#else /* SL_MAJOR_VERSION_NUM >= 2 */

#define FS_MODE_OPEN_CREATE(max_size, flag) \
  (SL_FS_CREATE | SL_FS_CREATE_MAX_SIZE(max_size))
#define SL_FI_FILE_SIZE(fi) ((fi).Len)
#define SL_FI_FILE_MAX_SIZE(fi) ((fi).MaxSize)

#define SL_LEN_TYPE _u16
#define SL_OPT_TYPE _u16

#endif /* SL_MAJOR_VERSION_NUM < 2 */

int slfs_open(const unsigned char *fname, uint32_t flags);

#endif /* MG_NET_IF == MG_NET_IF_SIMPLELINK */

#endif /* CS_COMMON_PLATFORMS_SIMPLELINK_CS_SIMPLELINK_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/platforms/platform_wince.h"
#endif
#ifndef CS_COMMON_PLATFORMS_PLATFORM_WINCE_H_
#define CS_COMMON_PLATFORMS_PLATFORM_WINCE_H_

#if CS_PLATFORM == CS_P_WINCE

/*
 * MSVC++ 14.0 _MSC_VER == 1900 (Visual Studio 2015)
 * MSVC++ 12.0 _MSC_VER == 1800 (Visual Studio 2013)
 * MSVC++ 11.0 _MSC_VER == 1700 (Visual Studio 2012)
 * MSVC++ 10.0 _MSC_VER == 1600 (Visual Studio 2010)
 * MSVC++ 9.0  _MSC_VER == 1500 (Visual Studio 2008)
 * MSVC++ 8.0  _MSC_VER == 1400 (Visual Studio 2005)
 * MSVC++ 7.1  _MSC_VER == 1310 (Visual Studio 2003)
 * MSVC++ 7.0  _MSC_VER == 1300
 * MSVC++ 6.0  _MSC_VER == 1200
 * MSVC++ 5.0  _MSC_VER == 1100
 */
#pragma warning(disable : 4127) /* FD_SET() emits warning, disable it */
#pragma warning(disable : 4204) /* missing c99 support */

#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#pragma comment(lib, "ws2.lib") /* Linking with WinCE winsock library */

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#define strdup _strdup

#ifndef EINPROGRESS
#define EINPROGRESS WSAEINPROGRESS
#endif

#ifndef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif

#ifndef EAGAIN
#define EAGAIN EWOULDBLOCK
#endif

#ifndef __func__
#define STRX(x) #x
#define STR(x) STRX(x)
#define __func__ __FILE__ ":" STR(__LINE__)
#endif

#define snprintf _snprintf
#define fileno _fileno
#define vsnprintf _vsnprintf
#define sleep(x) Sleep((x) *1000)
#define to64(x) _atoi64(x)
#define rmdir _rmdir

#if defined(_MSC_VER) && _MSC_VER >= 1400
#define fseeko(x, y, z) _fseeki64((x), (y), (z))
#else
#define fseeko(x, y, z) fseek((x), (y), (z))
#endif

typedef int socklen_t;

#if _MSC_VER >= 1700
#include <stdint.h>
#else
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#endif

typedef SOCKET sock_t;
typedef uint32_t in_addr_t;

#ifndef UINT16_MAX
#define UINT16_MAX 65535
#endif

#ifndef UINT32_MAX
#define UINT32_MAX 4294967295
#endif

#ifndef pid_t
#define pid_t HANDLE
#endif

#define INT64_FMT "I64d"
#define INT64_X_FMT "I64x"
/* TODO(alashkin): check if this is correct */
#define SIZE_T_FMT "u"

#define DIRSEP '\\'
#define CS_DEFINE_DIRENT

#ifndef va_copy
#ifdef __va_copy
#define va_copy __va_copy
#else
#define va_copy(x, y) (x) = (y)
#endif
#endif

#ifndef MG_MAX_HTTP_REQUEST_SIZE
#define MG_MAX_HTTP_REQUEST_SIZE 8192
#endif

#ifndef MG_MAX_HTTP_SEND_MBUF
#define MG_MAX_HTTP_SEND_MBUF 4096
#endif

#ifndef MG_MAX_HTTP_HEADERS
#define MG_MAX_HTTP_HEADERS 40
#endif

#ifndef CS_ENABLE_STDIO
#define CS_ENABLE_STDIO 1
#endif

#define abort() DebugBreak();

#ifndef BUFSIZ
#define BUFSIZ 4096
#endif
/*
 * Explicitly disabling MG_ENABLE_THREADS for WinCE
 * because they are enabled for _WIN32 by default
 */
#ifndef MG_ENABLE_THREADS
#define MG_ENABLE_THREADS 0
#endif

#ifndef MG_ENABLE_FILESYSTEM
#define MG_ENABLE_FILESYSTEM 1
#endif

#ifndef MG_NET_IF
#define MG_NET_IF MG_NET_IF_SOCKET
#endif

typedef struct _stati64 {
  uint32_t st_mtime;
  uint32_t st_size;
  uint32_t st_mode;
} cs_stat_t;

/*
 * WinCE 6.0 has a lot of useful definitions in ATL (not windows.h) headers
 * use #ifdefs to avoid conflicts
 */

#ifndef ENOENT
#define ENOENT ERROR_PATH_NOT_FOUND
#endif

#ifndef EACCES
#define EACCES ERROR_ACCESS_DENIED
#endif

#ifndef ENOMEM
#define ENOMEM ERROR_NOT_ENOUGH_MEMORY
#endif

#ifndef _UINTPTR_T_DEFINED
typedef unsigned int *uintptr_t;
#endif

#define _S_IFREG 2
#define _S_IFDIR 4

#ifndef S_ISDIR
#define S_ISDIR(x) (((x) &_S_IFDIR) != 0)
#endif

#ifndef S_ISREG
#define S_ISREG(x) (((x) &_S_IFREG) != 0)
#endif

int open(const char *filename, int oflag, int pmode);
int _wstati64(const wchar_t *path, cs_stat_t *st);
const char *strerror();

#endif /* CS_PLATFORM == CS_P_WINCE */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_WINCE_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/platforms/platform_nxp_lpc.h"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_PLATFORM_NXP_LPC_H_
#define CS_COMMON_PLATFORMS_PLATFORM_NXP_LPC_H_

#if CS_PLATFORM == CS_P_NXP_LPC

#include <ctype.h>
#include <stdint.h>
#include <string.h>

#define SIZE_T_FMT "u"
typedef struct stat cs_stat_t;
#define INT64_FMT "lld"
#define INT64_X_FMT "llx"
#define __cdecl

#define MG_LWIP 1

#define MG_NET_IF MG_NET_IF_LWIP_LOW_LEVEL

/*
 * LPCXpress comes with 3 C library implementations: Newlib, NewlibNano and
 *Redlib.
 * See https://community.nxp.com/message/630860 for more details.
 *
 * Redlib is the default and lacks certain things, so we provide them.
 */
#ifdef __REDLIB_INTERFACE_VERSION__

/* Let LWIP define timeval for us. */
#define LWIP_TIMEVAL_PRIVATE 1

#define va_copy(d, s) __builtin_va_copy(d, s)

#define CS_ENABLE_TO64 1
#define to64(x) cs_to64(x)

#define CS_ENABLE_STRDUP 1

#else

#include <sys/time.h>
#define LWIP_TIMEVAL_PRIVATE 0
#define to64(x) strtoll(x, NULL, 10)

#endif

#endif /* CS_PLATFORM == CS_P_NXP_LPC */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_NXP_LPC_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/platforms/platform_nxp_kinetis.h"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_PLATFORM_NXP_KINETIS_H_
#define CS_COMMON_PLATFORMS_PLATFORM_NXP_KINETIS_H_

#if CS_PLATFORM == CS_P_NXP_KINETIS

#include <ctype.h>
#include <inttypes.h>
#include <string.h>
#include <sys/time.h>

#define SIZE_T_FMT "u"
typedef struct stat cs_stat_t;
#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT "lld"
#define INT64_X_FMT "llx"
#define __cdecl

#define MG_LWIP 1

#define MG_NET_IF MG_NET_IF_LWIP_LOW_LEVEL

/* struct timeval is defined in sys/time.h. */
#define LWIP_TIMEVAL_PRIVATE 0

#endif /* CS_PLATFORM == CS_P_NXP_KINETIS */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_NXP_KINETIS_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/platforms/platform_pic32.h"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_PLATFORM_PIC32_H_
#define CS_COMMON_PLATFORMS_PLATFORM_PIC32_H_

#if CS_PLATFORM == CS_P_PIC32

#define MG_NET_IF MG_NET_IF_PIC32

#include <stdint.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>

#include <system_config.h>
#include <system_definitions.h>

#include <sys/types.h>

typedef TCP_SOCKET sock_t;
#define to64(x) strtoll(x, NULL, 10)

#define SIZE_T_FMT "lu"
#define INT64_FMT "lld"

#ifndef CS_ENABLE_STDIO
#define CS_ENABLE_STDIO 1
#endif

char *inet_ntoa(struct in_addr in);

#endif /* CS_PLATFORM == CS_P_PIC32 */

#endif /* CS_COMMON_PLATFORMS_PLATFORM_PIC32_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/platforms/platform_stm32.h"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_PLATFORM_STM32_H_
#define CS_COMMON_PLATFORMS_PLATFORM_STM32_H_
#if CS_PLATFORM == CS_P_STM32

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

#include <stm32_sdk_hal.h>

#define to64(x) strtoll(x, NULL, 10)
#define INT64_FMT PRId64
#define SIZE_T_FMT "u"
typedef struct stat cs_stat_t;
#define DIRSEP '/'

#ifndef CS_ENABLE_STDIO
#define CS_ENABLE_STDIO 1
#endif

#ifndef MG_ENABLE_FILESYSTEM
#define MG_ENABLE_FILESYSTEM 1
#endif

#endif /* CS_PLATFORM == CS_P_STM32 */
#endif /* CS_COMMON_PLATFORMS_PLATFORM_STM32_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/platforms/lwip/mg_lwip.h"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_PLATFORMS_LWIP_MG_LWIP_H_
#define CS_COMMON_PLATFORMS_LWIP_MG_LWIP_H_

#ifndef MG_LWIP
#define MG_LWIP 0
#endif

#if MG_LWIP

/*
 * When compiling for nRF5x chips with arm-none-eabi-gcc, it has BYTE_ORDER
 * already defined, so in order to avoid warnings in lwip, we have to undefine
 * it.
 *
 * TODO: Check if in the future versions of nRF5 SDK that changes.
 *       Current version of nRF51 SDK: 0.8.0
 *                          nRF5 SDK:  0.9.0
 */
#if CS_PLATFORM == CS_P_NRF51 || CS_PLATFORM == CS_P_NRF52
#undef BYTE_ORDER
#endif

#include <lwip/opt.h>
#include <lwip/err.h>
#include <lwip/ip_addr.h>
#include <lwip/inet.h>
#include <lwip/netdb.h>
#include <lwip/dns.h>

#ifndef LWIP_PROVIDE_ERRNO
#include <errno.h>
#endif

#if LWIP_SOCKET
#include <lwip/sockets.h>
#else
/* We really need the definitions from sockets.h. */
#undef LWIP_SOCKET
#define LWIP_SOCKET 1
#include <lwip/sockets.h>
#undef LWIP_SOCKET
#define LWIP_SOCKET 0
#endif

#define INVALID_SOCKET (-1)
#define SOMAXCONN 10
typedef int sock_t;

#if MG_NET_IF == MG_NET_IF_LWIP_LOW_LEVEL
struct mg_mgr;
struct mg_connection;
uint32_t mg_lwip_get_poll_delay_ms(struct mg_mgr *mgr);
void mg_lwip_set_keepalive_params(struct mg_connection *nc, int idle,
                                  int interval, int count);
#endif

/* For older version of LWIP */
#ifndef ipX_2_ip
#define ipX_2_ip(x) (x)
#endif

#endif /* MG_LWIP */

#endif /* CS_COMMON_PLATFORMS_LWIP_MG_LWIP_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/cs_md5.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_MD5_H_
#define CS_COMMON_MD5_H_

/* Amalgamated: #include "common/platform.h" */

#ifndef CS_DISABLE_MD5
#define CS_DISABLE_MD5 0
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  uint32_t buf[4];
  uint32_t bits[2];
  unsigned char in[64];
} cs_md5_ctx;

void cs_md5_init(cs_md5_ctx *c);
void cs_md5_update(cs_md5_ctx *c, const unsigned char *data, size_t len);
void cs_md5_final(unsigned char *md, cs_md5_ctx *c);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_COMMON_MD5_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/cs_sha1.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_SHA1_H_
#define CS_COMMON_SHA1_H_

#ifndef CS_DISABLE_SHA1
#define CS_DISABLE_SHA1 0
#endif

#if !CS_DISABLE_SHA1

/* Amalgamated: #include "common/platform.h" */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
  uint32_t state[5];
  uint32_t count[2];
  unsigned char buffer[64];
} cs_sha1_ctx;

void cs_sha1_init(cs_sha1_ctx *);
void cs_sha1_update(cs_sha1_ctx *, const unsigned char *data, uint32_t len);
void cs_sha1_final(unsigned char digest[20], cs_sha1_ctx *);
void cs_hmac_sha1(const unsigned char *key, size_t key_len,
                  const unsigned char *text, size_t text_len,
                  unsigned char out[20]);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_DISABLE_SHA1 */

#endif /* CS_COMMON_SHA1_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/cs_time.h"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_CS_TIME_H_
#define CS_COMMON_CS_TIME_H_

#include <time.h>

/* Amalgamated: #include "common/platform.h" */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Sub-second granularity time(). */
double cs_time(void);

/*
 * Similar to (non-standard) timegm, converts broken-down time into the number
 * of seconds since Unix Epoch.
 */
double cs_timegm(const struct tm *tm);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_COMMON_CS_TIME_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/mg_str.h"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_MG_STR_H_
#define CS_COMMON_MG_STR_H_

#include <stddef.h>

/* Amalgamated: #include "common/platform.h" */

#ifdef __cplusplus
extern "C" {
#endif

/* Describes chunk of memory */
struct mg_str {
  const char *p; /* Memory chunk pointer */
  size_t len;    /* Memory chunk length */
};

/*
 * Helper function for creating mg_str struct from plain C string.
 * `NULL` is allowed and becomes `{NULL, 0}`.
 */
struct mg_str mg_mk_str(const char *s);

/*
 * Like `mg_mk_str`, but takes string length explicitly.
 */
struct mg_str mg_mk_str_n(const char *s, size_t len);

/* Macro for initializing mg_str. */
#define MG_MK_STR(str_literal) \
  { str_literal, sizeof(str_literal) - 1 }
#define MG_NULL_STR \
  { NULL, 0 }

/*
 * Cross-platform version of `strcmp()` where where first string is
 * specified by `struct mg_str`.
 */
int mg_vcmp(const struct mg_str *str2, const char *str1);

/*
 * Cross-platform version of `strncasecmp()` where first string is
 * specified by `struct mg_str`.
 */
int mg_vcasecmp(const struct mg_str *str2, const char *str1);

/* Creates a copy of s (heap-allocated). */
struct mg_str mg_strdup(const struct mg_str s);

/*
 * Creates a copy of s (heap-allocated).
 * Resulting string is NUL-terminated (but NUL is not included in len).
 */
struct mg_str mg_strdup_nul(const struct mg_str s);

/*
 * Locates character in a string.
 */
const char *mg_strchr(const struct mg_str s, int c);

/*
 * Compare two `mg_str`s; return value is the same as `strcmp`.
 */
int mg_strcmp(const struct mg_str str1, const struct mg_str str2);

/*
 * Like `mg_strcmp`, but compares at most `n` characters.
 */
int mg_strncmp(const struct mg_str str1, const struct mg_str str2, size_t n);

/*
 * Finds the first occurrence of a substring `needle` in the `haystack`.
 */
const char *mg_strstr(const struct mg_str haystack, const struct mg_str needle);

#ifdef __cplusplus
}
#endif

#endif /* CS_COMMON_MG_STR_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/mbuf.h"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

/*
 * === Memory Buffers
 *
 * Mbufs are mutable/growing memory buffers, like C++ strings.
 * Mbuf can append data to the end of a buffer or insert data into arbitrary
 * position in the middle of a buffer. The buffer grows automatically when
 * needed.
 */

#ifndef CS_COMMON_MBUF_H_
#define CS_COMMON_MBUF_H_

#include <stdlib.h>
/* Amalgamated: #include "common/platform.h" */

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef MBUF_SIZE_MULTIPLIER
#define MBUF_SIZE_MULTIPLIER 1.5
#endif

/* Memory buffer descriptor */
struct mbuf {
  char *buf;   /* Buffer pointer */
  size_t len;  /* Data length. Data is located between offset 0 and len. */
  size_t size; /* Buffer size allocated by realloc(1). Must be >= len */
};

/*
 * Initialises an Mbuf.
 * `initial_capacity` specifies the initial capacity of the mbuf.
 */
void mbuf_init(struct mbuf *, size_t initial_capacity);

/* Frees the space allocated for the mbuffer and resets the mbuf structure. */
void mbuf_free(struct mbuf *);

/*
 * Appends data to the Mbuf.
 *
 * Returns the number of bytes appended or 0 if out of memory.
 */
size_t mbuf_append(struct mbuf *, const void *data, size_t data_size);

/*
 * Inserts data at a specified offset in the Mbuf.
 *
 * Existing data will be shifted forwards and the buffer will
 * be grown if necessary.
 * Returns the number of bytes inserted.
 */
size_t mbuf_insert(struct mbuf *, size_t, const void *, size_t);

/* Removes `data_size` bytes from the beginning of the buffer. */
void mbuf_remove(struct mbuf *, size_t data_size);

/*
 * Resizes an Mbuf.
 *
 * If `new_size` is smaller than buffer's `len`, the
 * resize is not performed.
 */
void mbuf_resize(struct mbuf *, size_t new_size);

/* Shrinks an Mbuf by resizing its `size` to `len`. */
void mbuf_trim(struct mbuf *);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* CS_COMMON_MBUF_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/cs_base64.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_CS_BASE64_H_
#define CS_COMMON_CS_BASE64_H_

#ifndef DISABLE_BASE64
#define DISABLE_BASE64 0
#endif

#if !DISABLE_BASE64

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*cs_base64_putc_t)(char, void *);

struct cs_base64_ctx {
  /* cannot call it putc because it's a macro on some environments */
  cs_base64_putc_t b64_putc;
  unsigned char chunk[3];
  int chunk_size;
  void *user_data;
};

void cs_base64_init(struct cs_base64_ctx *ctx, cs_base64_putc_t putc,
                    void *user_data);
void cs_base64_update(struct cs_base64_ctx *ctx, const char *str, size_t len);
void cs_base64_finish(struct cs_base64_ctx *ctx);

void cs_base64_encode(const unsigned char *src, int src_len, char *dst);
void cs_fprint_base64(FILE *f, const unsigned char *src, int src_len);
int cs_base64_decode(const unsigned char *s, int len, char *dst, int *dec_len);

#ifdef __cplusplus
}
#endif

#endif /* DISABLE_BASE64 */

#endif /* CS_COMMON_CS_BASE64_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/str_util.h"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_STR_UTIL_H_
#define CS_COMMON_STR_UTIL_H_

#include <stdarg.h>
#include <stdlib.h>

/* Amalgamated: #include "common/mg_str.h" */
/* Amalgamated: #include "common/platform.h" */

#ifndef CS_ENABLE_STRDUP
#define CS_ENABLE_STRDUP 0
#endif

#ifndef CS_ENABLE_TO64
#define CS_ENABLE_TO64 0
#endif

/*
 * Expands to a string representation of its argument: e.g.
 * `CS_STRINGIFY_LIT(5) expands to "5"`
 */
#define CS_STRINGIFY_LIT(x) #x

/*
 * Expands to a string representation of its argument, which is allowed
 * to be a macro: e.g.
 *
 * #define FOO 123
 * CS_STRINGIFY_MACRO(FOO)
 *
 * expands to 123.
 */
#define CS_STRINGIFY_MACRO(x) CS_STRINGIFY_LIT(x)

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Equivalent of standard `strnlen()`.
 */
size_t c_strnlen(const char *s, size_t maxlen);

/*
 * Equivalent of standard `snprintf()`.
 */
int c_snprintf(char *buf, size_t buf_size, const char *format, ...);

/*
 * Equivalent of standard `vsnprintf()`.
 */
int c_vsnprintf(char *buf, size_t buf_size, const char *format, va_list ap);

/*
 * Find the first occurrence of find in s, where the search is limited to the
 * first slen characters of s.
 */
const char *c_strnstr(const char *s, const char *find, size_t slen);

/*
 * Stringify binary data. Output buffer size must be 2 * size_of_input + 1
 * because each byte of input takes 2 bytes in string representation
 * plus 1 byte for the terminating \0 character.
 */
void cs_to_hex(char *to, const unsigned char *p, size_t len);

/*
 * Convert stringified binary data back to binary.
 * Does the reverse of `cs_to_hex()`.
 */
void cs_from_hex(char *to, const char *p, size_t len);

#if CS_ENABLE_STRDUP
/*
 * Equivalent of standard `strdup()`, defined if only `CS_ENABLE_STRDUP` is 1.
 */
char *strdup(const char *src);
#endif

#if CS_ENABLE_TO64
#include <stdint.h>
/*
 * Simple string -> int64 conversion routine.
 */
int64_t cs_to64(const char *s);
#endif

/*
 * Cross-platform version of `strncasecmp()`.
 */
int mg_ncasecmp(const char *s1, const char *s2, size_t len);

/*
 * Cross-platform version of `strcasecmp()`.
 */
int mg_casecmp(const char *s1, const char *s2);

/*
 * Prints message to the buffer. If the buffer is large enough to hold the
 * message, it returns buffer. If buffer is to small, it allocates a large
 * enough buffer on heap and returns allocated buffer.
 * This is a supposed use case:
 *
 * ```c
 *    char buf[5], *p = buf;
 *    mg_avprintf(&p, sizeof(buf), "%s", "hi there");
 *    use_p_somehow(p);
 *    if (p != buf) {
 *      free(p);
 *    }
 * ```
 *
 * The purpose of this is to avoid malloc-ing if generated strings are small.
 */
int mg_asprintf(char **buf, size_t size, const char *fmt, ...);

/* Same as mg_asprintf, but takes varargs list. */
int mg_avprintf(char **buf, size_t size, const char *fmt, va_list ap);

/*
 * A helper function for traversing a comma separated list of values.
 * It returns a list pointer shifted to the next value or NULL if the end
 * of the list found.
 * The value is stored in a val vector. If the value has a form "x=y", then
 * eq_val vector is initialised to point to the "y" part, and val vector length
 * is adjusted to point only to "x".
 * If the list is just a comma separated list of entries, like "aa,bb,cc" then
 * `eq_val` will contain zero-length string.
 *
 * The purpose of this function is to parse comma separated string without
 * any copying/memory allocation.
 */
const char *mg_next_comma_list_entry(const char *list, struct mg_str *val,
                                     struct mg_str *eq_val);

/*
 * Like `mg_next_comma_list_entry()`, but takes `list` as `struct mg_str`.
 */
struct mg_str mg_next_comma_list_entry_n(struct mg_str list, struct mg_str *val,
                                         struct mg_str *eq_val);

/*
 * Matches 0-terminated string (mg_match_prefix) or string with given length
 * mg_match_prefix_n against a glob pattern. Glob syntax:
 * ```
 * - * matches zero or more characters until a slash character /
 * - ** matches zero or more characters
 * - ? Matches exactly one character which is not a slash /
 * - | or ,  divides alternative patterns
 * - any other character matches itself
 * ```
 * Match is case-insensitive. Return number of bytes matched.
 * Examples:
 * ```
 * mg_match_prefix("a*f", len, "abcdefgh") == 6
 * mg_match_prefix("a*f", len, "abcdexgh") == 0
 * mg_match_prefix("a*f|de*,xy", len, "defgh") == 5
 * mg_match_prefix("?*", len, "abc") == 3
 * mg_match_prefix("?*", len, "") == 0
 * ```
 */
size_t mg_match_prefix(const char *pattern, int pattern_len, const char *str);

/*
 * Like `mg_match_prefix()`, but takes `pattern` and `str` as `struct mg_str`.
 */
size_t mg_match_prefix_n(const struct mg_str pattern, const struct mg_str str);

#ifdef __cplusplus
}
#endif

#endif /* CS_COMMON_STR_UTIL_H_ */
#ifdef MG_MODULE_LINES
#line 1 "common/queue.h"
#endif
/* clang-format off */
/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)queue.h	8.5 (Berkeley) 8/20/94
 * $FreeBSD$
 */

#ifndef _SYS_QUEUE_H_
#define	_SYS_QUEUE_H_

/*
 * This file defines four types of data structures: singly-linked lists,
 * singly-linked tail queues, lists and tail queues.
 *
 * A singly-linked list is headed by a single forward pointer. The elements
 * are singly linked for minimum space and pointer manipulation overhead at
 * the expense of O(n) removal for arbitrary elements. New elements can be
 * added to the list after an existing element or at the head of the list.
 * Elements being removed from the head of the list should use the explicit
 * macro for this purpose for optimum efficiency. A singly-linked list may
 * only be traversed in the forward direction.  Singly-linked lists are ideal
 * for applications with large datasets and few or no removals or for
 * implementing a LIFO queue.
 *
 * A singly-linked tail queue is headed by a pair of pointers, one to the
 * head of the list and the other to the tail of the list. The elements are
 * singly linked for minimum space and pointer manipulation overhead at the
 * expense of O(n) removal for arbitrary elements. New elements can be added
 * to the list after an existing element, at the head of the list, or at the
 * end of the list. Elements being removed from the head of the tail queue
 * should use the explicit macro for this purpose for optimum efficiency.
 * A singly-linked tail queue may only be traversed in the forward direction.
 * Singly-linked tail queues are ideal for applications with large datasets
 * and few or no removals or for implementing a FIFO queue.
 *
 * A list is headed by a single forward pointer (or an array of forward
 * pointers for a hash table header). The elements are doubly linked
 * so that an arbitrary element can be removed without a need to
 * traverse the list. New elements can be added to the list before
 * or after an existing element or at the head of the list. A list
 * may be traversed in either direction.
 *
 * A tail queue is headed by a pair of pointers, one to the head of the
 * list and the other to the tail of the list. The elements are doubly
 * linked so that an arbitrary element can be removed without a need to
 * traverse the list. New elements can be added to the list before or
 * after an existing element, at the head of the list, or at the end of
 * the list. A tail queue may be traversed in either direction.
 *
 * For details on the use of these macros, see the queue(3) manual page.
 *
 *
 *				SLIST	LIST	STAILQ	TAILQ
 * _HEAD			+	+	+	+
 * _CLASS_HEAD			+	+	+	+
 * _HEAD_INITIALIZER		+	+	+	+
 * _ENTRY			+	+	+	+
 * _CLASS_ENTRY			+	+	+	+
 * _INIT			+	+	+	+
 * _EMPTY			+	+	+	+
 * _FIRST			+	+	+	+
 * _NEXT			+	+	+	+
 * _PREV			-	+	-	+
 * _LAST			-	-	+	+
 * _FOREACH			+	+	+	+
 * _FOREACH_FROM		+	+	+	+
 * _FOREACH_SAFE		+	+	+	+
 * _FOREACH_FROM_SAFE		+	+	+	+
 * _FOREACH_REVERSE		-	-	-	+
 * _FOREACH_REVERSE_FROM	-	-	-	+
 * _FOREACH_REVERSE_SAFE	-	-	-	+
 * _FOREACH_REVERSE_FROM_SAFE	-	-	-	+
 * _INSERT_HEAD			+	+	+	+
 * _INSERT_BEFORE		-	+	-	+
 * _INSERT_AFTER		+	+	+	+
 * _INSERT_TAIL			-	-	+	+
 * _CONCAT			-	-	+	+
 * _REMOVE_AFTER		+	-	+	-
 * _REMOVE_HEAD			+	-	+	-
 * _REMOVE			+	+	+	+
 * _SWAP			+	+	+	+
 *
 */
#ifdef QUEUE_MACRO_DEBUG
/* Store the last 2 places the queue element or head was altered */
struct qm_trace {
	unsigned long	 lastline;
	unsigned long	 prevline;
	const char	*lastfile;
	const char	*prevfile;
};

#define	TRACEBUF	struct qm_trace trace;
#define	TRACEBUF_INITIALIZER	{ __LINE__, 0, __FILE__, NULL } ,
#define	TRASHIT(x)	do {(x) = (void *)-1;} while (0)
#define	QMD_SAVELINK(name, link)	void **name = (void *)&(link)

#define	QMD_TRACE_HEAD(head) do {					\
	(head)->trace.prevline = (head)->trace.lastline;		\
	(head)->trace.prevfile = (head)->trace.lastfile;		\
	(head)->trace.lastline = __LINE__;				\
	(head)->trace.lastfile = __FILE__;				\
} while (0)

#define	QMD_TRACE_ELEM(elem) do {					\
	(elem)->trace.prevline = (elem)->trace.lastline;		\
	(elem)->trace.prevfile = (elem)->trace.lastfile;		\
	(elem)->trace.lastline = __LINE__;				\
	(elem)->trace.lastfile = __FILE__;				\
} while (0)

#else
#define	QMD_TRACE_ELEM(elem)
#define	QMD_TRACE_HEAD(head)
#define	QMD_SAVELINK(name, link)
#define	TRACEBUF
#define	TRACEBUF_INITIALIZER
#define	TRASHIT(x)
#endif	/* QUEUE_MACRO_DEBUG */

#ifdef __cplusplus
/*
 * In C++ there can be structure lists and class lists:
 */
#define	QUEUE_TYPEOF(type) type
#else
#define	QUEUE_TYPEOF(type) struct type
#endif

/*
 * Singly-linked List declarations.
 */
#define	SLIST_HEAD(name, type)						\
struct name {								\
	struct type *slh_first;	/* first element */			\
}

#define	SLIST_CLASS_HEAD(name, type)					\
struct name {								\
	class type *slh_first;	/* first element */			\
}

#define	SLIST_HEAD_INITIALIZER(head)					\
	{ NULL }

#define	SLIST_ENTRY(type)						\
struct {								\
	struct type *sle_next;	/* next element */			\
}

#define	SLIST_CLASS_ENTRY(type)						\
struct {								\
	class type *sle_next;		/* next element */		\
}

/*
 * Singly-linked List functions.
 */
#define	SLIST_EMPTY(head)	((head)->slh_first == NULL)

#define	SLIST_FIRST(head)	((head)->slh_first)

#define	SLIST_FOREACH(var, head, field)					\
	for ((var) = SLIST_FIRST((head));				\
	    (var);							\
	    (var) = SLIST_NEXT((var), field))

#define	SLIST_FOREACH_FROM(var, head, field)				\
	for ((var) = ((var) ? (var) : SLIST_FIRST((head)));		\
	    (var);							\
	    (var) = SLIST_NEXT((var), field))

#define	SLIST_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = SLIST_FIRST((head));				\
	    (var) && ((tvar) = SLIST_NEXT((var), field), 1);		\
	    (var) = (tvar))

#define	SLIST_FOREACH_FROM_SAFE(var, head, field, tvar)			\
	for ((var) = ((var) ? (var) : SLIST_FIRST((head)));		\
	    (var) && ((tvar) = SLIST_NEXT((var), field), 1);		\
	    (var) = (tvar))

#define	SLIST_FOREACH_PREVPTR(var, varp, head, field)			\
	for ((varp) = &SLIST_FIRST((head));				\
	    ((var) = *(varp)) != NULL;					\
	    (varp) = &SLIST_NEXT((var), field))

#define	SLIST_INIT(head) do {						\
	SLIST_FIRST((head)) = NULL;					\
} while (0)

#define	SLIST_INSERT_AFTER(slistelm, elm, field) do {			\
	SLIST_NEXT((elm), field) = SLIST_NEXT((slistelm), field);	\
	SLIST_NEXT((slistelm), field) = (elm);				\
} while (0)

#define	SLIST_INSERT_HEAD(head, elm, field) do {			\
	SLIST_NEXT((elm), field) = SLIST_FIRST((head));			\
	SLIST_FIRST((head)) = (elm);					\
} while (0)

#define	SLIST_NEXT(elm, field)	((elm)->field.sle_next)

#define	SLIST_REMOVE(head, elm, type, field) do {			\
	QMD_SAVELINK(oldnext, (elm)->field.sle_next);			\
	if (SLIST_FIRST((head)) == (elm)) {				\
		SLIST_REMOVE_HEAD((head), field);			\
	}								\
	else {								\
		QUEUE_TYPEOF(type) *curelm = SLIST_FIRST(head);		\
		while (SLIST_NEXT(curelm, field) != (elm))		\
			curelm = SLIST_NEXT(curelm, field);		\
		SLIST_REMOVE_AFTER(curelm, field);			\
	}								\
	TRASHIT(*oldnext);						\
} while (0)

#define SLIST_REMOVE_AFTER(elm, field) do {				\
	SLIST_NEXT(elm, field) =					\
	    SLIST_NEXT(SLIST_NEXT(elm, field), field);			\
} while (0)

#define	SLIST_REMOVE_HEAD(head, field) do {				\
	SLIST_FIRST((head)) = SLIST_NEXT(SLIST_FIRST((head)), field);	\
} while (0)

#define SLIST_SWAP(head1, head2, type) do {				\
	QUEUE_TYPEOF(type) *swap_first = SLIST_FIRST(head1);		\
	SLIST_FIRST(head1) = SLIST_FIRST(head2);			\
	SLIST_FIRST(head2) = swap_first;				\
} while (0)

/*
 * Singly-linked Tail queue declarations.
 */
#define	STAILQ_HEAD(name, type)						\
struct name {								\
	struct type *stqh_first;/* first element */			\
	struct type **stqh_last;/* addr of last next element */		\
}

#define	STAILQ_CLASS_HEAD(name, type)					\
struct name {								\
	class type *stqh_first;	/* first element */			\
	class type **stqh_last;	/* addr of last next element */		\
}

#define	STAILQ_HEAD_INITIALIZER(head)					\
	{ NULL, &(head).stqh_first }

#define	STAILQ_ENTRY(type)						\
struct {								\
	struct type *stqe_next;	/* next element */			\
}

#define	STAILQ_CLASS_ENTRY(type)					\
struct {								\
	class type *stqe_next;	/* next element */			\
}

/*
 * Singly-linked Tail queue functions.
 */
#define	STAILQ_CONCAT(head1, head2) do {				\
	if (!STAILQ_EMPTY((head2))) {					\
		*(head1)->stqh_last = (head2)->stqh_first;		\
		(head1)->stqh_last = (head2)->stqh_last;		\
		STAILQ_INIT((head2));					\
	}								\
} while (0)

#define	STAILQ_EMPTY(head)	((head)->stqh_first == NULL)

#define	STAILQ_FIRST(head)	((head)->stqh_first)

#define	STAILQ_FOREACH(var, head, field)				\
	for((var) = STAILQ_FIRST((head));				\
	   (var);							\
	   (var) = STAILQ_NEXT((var), field))

#define	STAILQ_FOREACH_FROM(var, head, field)				\
	for ((var) = ((var) ? (var) : STAILQ_FIRST((head)));		\
	   (var);							\
	   (var) = STAILQ_NEXT((var), field))

#define	STAILQ_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = STAILQ_FIRST((head));				\
	    (var) && ((tvar) = STAILQ_NEXT((var), field), 1);		\
	    (var) = (tvar))

#define	STAILQ_FOREACH_FROM_SAFE(var, head, field, tvar)		\
	for ((var) = ((var) ? (var) : STAILQ_FIRST((head)));		\
	    (var) && ((tvar) = STAILQ_NEXT((var), field), 1);		\
	    (var) = (tvar))

#define	STAILQ_INIT(head) do {						\
	STAILQ_FIRST((head)) = NULL;					\
	(head)->stqh_last = &STAILQ_FIRST((head));			\
} while (0)

#define	STAILQ_INSERT_AFTER(head, tqelm, elm, field) do {		\
	if ((STAILQ_NEXT((elm), field) = STAILQ_NEXT((tqelm), field)) == NULL)\
		(head)->stqh_last = &STAILQ_NEXT((elm), field);		\
	STAILQ_NEXT((tqelm), field) = (elm);				\
} while (0)

#define	STAILQ_INSERT_HEAD(head, elm, field) do {			\
	if ((STAILQ_NEXT((elm), field) = STAILQ_FIRST((head))) == NULL)	\
		(head)->stqh_last = &STAILQ_NEXT((elm), field);		\
	STAILQ_FIRST((head)) = (elm);					\
} while (0)

#define	STAILQ_INSERT_TAIL(head, elm, field) do {			\
	STAILQ_NEXT((elm), field) = NULL;				\
	*(head)->stqh_last = (elm);					\
	(head)->stqh_last = &STAILQ_NEXT((elm), field);			\
} while (0)

#define	STAILQ_LAST(head, type, field)				\
	(STAILQ_EMPTY((head)) ? NULL :				\
	    __containerof((head)->stqh_last,			\
	    QUEUE_TYPEOF(type), field.stqe_next))

#define	STAILQ_NEXT(elm, field)	((elm)->field.stqe_next)

#define	STAILQ_REMOVE(head, elm, type, field) do {			\
	QMD_SAVELINK(oldnext, (elm)->field.stqe_next);			\
	if (STAILQ_FIRST((head)) == (elm)) {				\
		STAILQ_REMOVE_HEAD((head), field);			\
	}								\
	else {								\
		QUEUE_TYPEOF(type) *curelm = STAILQ_FIRST(head);	\
		while (STAILQ_NEXT(curelm, field) != (elm))		\
			curelm = STAILQ_NEXT(curelm, field);		\
		STAILQ_REMOVE_AFTER(head, curelm, field);		\
	}								\
	TRASHIT(*oldnext);						\
} while (0)

#define STAILQ_REMOVE_AFTER(head, elm, field) do {			\
	if ((STAILQ_NEXT(elm, field) =					\
	     STAILQ_NEXT(STAILQ_NEXT(elm, field), field)) == NULL)	\
		(head)->stqh_last = &STAILQ_NEXT((elm), field);		\
} while (0)

#define	STAILQ_REMOVE_HEAD(head, field) do {				\
	if ((STAILQ_FIRST((head)) =					\
	     STAILQ_NEXT(STAILQ_FIRST((head)), field)) == NULL)		\
		(head)->stqh_last = &STAILQ_FIRST((head));		\
} while (0)

#define STAILQ_SWAP(head1, head2, type) do {				\
	QUEUE_TYPEOF(type) *swap_first = STAILQ_FIRST(head1);		\
	QUEUE_TYPEOF(type) **swap_last = (head1)->stqh_last;		\
	STAILQ_FIRST(head1) = STAILQ_FIRST(head2);			\
	(head1)->stqh_last = (head2)->stqh_last;			\
	STAILQ_FIRST(head2) = swap_first;				\
	(head2)->stqh_last = swap_last;					\
	if (STAILQ_EMPTY(head1))					\
		(head1)->stqh_last = &STAILQ_FIRST(head1);		\
	if (STAILQ_EMPTY(head2))					\
		(head2)->stqh_last = &STAILQ_FIRST(head2);		\
} while (0)


/*
 * List declarations.
 */
#define	LIST_HEAD(name, type)						\
struct name {								\
	struct type *lh_first;	/* first element */			\
}

#define	LIST_CLASS_HEAD(name, type)					\
struct name {								\
	class type *lh_first;	/* first element */			\
}

#define	LIST_HEAD_INITIALIZER(head)					\
	{ NULL }

#define	LIST_ENTRY(type)						\
struct {								\
	struct type *le_next;	/* next element */			\
	struct type **le_prev;	/* address of previous next element */	\
}

#define	LIST_CLASS_ENTRY(type)						\
struct {								\
	class type *le_next;	/* next element */			\
	class type **le_prev;	/* address of previous next element */	\
}

/*
 * List functions.
 */

#if (defined(_KERNEL) && defined(INVARIANTS))
#define	QMD_LIST_CHECK_HEAD(head, field) do {				\
	if (LIST_FIRST((head)) != NULL &&				\
	    LIST_FIRST((head))->field.le_prev !=			\
	     &LIST_FIRST((head)))					\
		panic("Bad list head %p first->prev != head", (head));	\
} while (0)

#define	QMD_LIST_CHECK_NEXT(elm, field) do {				\
	if (LIST_NEXT((elm), field) != NULL &&				\
	    LIST_NEXT((elm), field)->field.le_prev !=			\
	     &((elm)->field.le_next))					\
	     	panic("Bad link elm %p next->prev != elm", (elm));	\
} while (0)

#define	QMD_LIST_CHECK_PREV(elm, field) do {				\
	if (*(elm)->field.le_prev != (elm))				\
		panic("Bad link elm %p prev->next != elm", (elm));	\
} while (0)
#else
#define	QMD_LIST_CHECK_HEAD(head, field)
#define	QMD_LIST_CHECK_NEXT(elm, field)
#define	QMD_LIST_CHECK_PREV(elm, field)
#endif /* (_KERNEL && INVARIANTS) */

#define	LIST_EMPTY(head)	((head)->lh_first == NULL)

#define	LIST_FIRST(head)	((head)->lh_first)

#define	LIST_FOREACH(var, head, field)					\
	for ((var) = LIST_FIRST((head));				\
	    (var);							\
	    (var) = LIST_NEXT((var), field))

#define	LIST_FOREACH_FROM(var, head, field)				\
	for ((var) = ((var) ? (var) : LIST_FIRST((head)));		\
	    (var);							\
	    (var) = LIST_NEXT((var), field))

#define	LIST_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = LIST_FIRST((head));				\
	    (var) && ((tvar) = LIST_NEXT((var), field), 1);		\
	    (var) = (tvar))

#define	LIST_FOREACH_FROM_SAFE(var, head, field, tvar)			\
	for ((var) = ((var) ? (var) : LIST_FIRST((head)));		\
	    (var) && ((tvar) = LIST_NEXT((var), field), 1);		\
	    (var) = (tvar))

#define	LIST_INIT(head) do {						\
	LIST_FIRST((head)) = NULL;					\
} while (0)

#define	LIST_INSERT_AFTER(listelm, elm, field) do {			\
	QMD_LIST_CHECK_NEXT(listelm, field);				\
	if ((LIST_NEXT((elm), field) = LIST_NEXT((listelm), field)) != NULL)\
		LIST_NEXT((listelm), field)->field.le_prev =		\
		    &LIST_NEXT((elm), field);				\
	LIST_NEXT((listelm), field) = (elm);				\
	(elm)->field.le_prev = &LIST_NEXT((listelm), field);		\
} while (0)

#define	LIST_INSERT_BEFORE(listelm, elm, field) do {			\
	QMD_LIST_CHECK_PREV(listelm, field);				\
	(elm)->field.le_prev = (listelm)->field.le_prev;		\
	LIST_NEXT((elm), field) = (listelm);				\
	*(listelm)->field.le_prev = (elm);				\
	(listelm)->field.le_prev = &LIST_NEXT((elm), field);		\
} while (0)

#define	LIST_INSERT_HEAD(head, elm, field) do {				\
	QMD_LIST_CHECK_HEAD((head), field);				\
	if ((LIST_NEXT((elm), field) = LIST_FIRST((head))) != NULL)	\
		LIST_FIRST((head))->field.le_prev = &LIST_NEXT((elm), field);\
	LIST_FIRST((head)) = (elm);					\
	(elm)->field.le_prev = &LIST_FIRST((head));			\
} while (0)

#define	LIST_NEXT(elm, field)	((elm)->field.le_next)

#define	LIST_PREV(elm, head, type, field)			\
	((elm)->field.le_prev == &LIST_FIRST((head)) ? NULL :	\
	    __containerof((elm)->field.le_prev,			\
	    QUEUE_TYPEOF(type), field.le_next))

#define	LIST_REMOVE(elm, field) do {					\
	QMD_SAVELINK(oldnext, (elm)->field.le_next);			\
	QMD_SAVELINK(oldprev, (elm)->field.le_prev);			\
	QMD_LIST_CHECK_NEXT(elm, field);				\
	QMD_LIST_CHECK_PREV(elm, field);				\
	if (LIST_NEXT((elm), field) != NULL)				\
		LIST_NEXT((elm), field)->field.le_prev = 		\
		    (elm)->field.le_prev;				\
	*(elm)->field.le_prev = LIST_NEXT((elm), field);		\
	TRASHIT(*oldnext);						\
	TRASHIT(*oldprev);						\
} while (0)

#define LIST_SWAP(head1, head2, type, field) do {			\
	QUEUE_TYPEOF(type) *swap_tmp = LIST_FIRST(head1);		\
	LIST_FIRST((head1)) = LIST_FIRST((head2));			\
	LIST_FIRST((head2)) = swap_tmp;					\
	if ((swap_tmp = LIST_FIRST((head1))) != NULL)			\
		swap_tmp->field.le_prev = &LIST_FIRST((head1));		\
	if ((swap_tmp = LIST_FIRST((head2))) != NULL)			\
		swap_tmp->field.le_prev = &LIST_FIRST((head2));		\
} while (0)

/*
 * Tail queue declarations.
 */
#define	TAILQ_HEAD(name, type)						\
struct name {								\
	struct type *tqh_first;	/* first element */			\
	struct type **tqh_last;	/* addr of last next element */		\
	TRACEBUF							\
}

#define	TAILQ_CLASS_HEAD(name, type)					\
struct name {								\
	class type *tqh_first;	/* first element */			\
	class type **tqh_last;	/* addr of last next element */		\
	TRACEBUF							\
}

#define	TAILQ_HEAD_INITIALIZER(head)					\
	{ NULL, &(head).tqh_first, TRACEBUF_INITIALIZER }

#define	TAILQ_ENTRY(type)						\
struct {								\
	struct type *tqe_next;	/* next element */			\
	struct type **tqe_prev;	/* address of previous next element */	\
	TRACEBUF							\
}

#define	TAILQ_CLASS_ENTRY(type)						\
struct {								\
	class type *tqe_next;	/* next element */			\
	class type **tqe_prev;	/* address of previous next element */	\
	TRACEBUF							\
}

/*
 * Tail queue functions.
 */
#if (defined(_KERNEL) && defined(INVARIANTS))
#define	QMD_TAILQ_CHECK_HEAD(head, field) do {				\
	if (!TAILQ_EMPTY(head) &&					\
	    TAILQ_FIRST((head))->field.tqe_prev !=			\
	     &TAILQ_FIRST((head)))					\
		panic("Bad tailq head %p first->prev != head", (head));	\
} while (0)

#define	QMD_TAILQ_CHECK_TAIL(head, field) do {				\
	if (*(head)->tqh_last != NULL)					\
	    	panic("Bad tailq NEXT(%p->tqh_last) != NULL", (head)); 	\
} while (0)

#define	QMD_TAILQ_CHECK_NEXT(elm, field) do {				\
	if (TAILQ_NEXT((elm), field) != NULL &&				\
	    TAILQ_NEXT((elm), field)->field.tqe_prev !=			\
	     &((elm)->field.tqe_next))					\
		panic("Bad link elm %p next->prev != elm", (elm));	\
} while (0)

#define	QMD_TAILQ_CHECK_PREV(elm, field) do {				\
	if (*(elm)->field.tqe_prev != (elm))				\
		panic("Bad link elm %p prev->next != elm", (elm));	\
} while (0)
#else
#define	QMD_TAILQ_CHECK_HEAD(head, field)
#define	QMD_TAILQ_CHECK_TAIL(head, headname)
#define	QMD_TAILQ_CHECK_NEXT(elm, field)
#define	QMD_TAILQ_CHECK_PREV(elm, field)
#endif /* (_KERNEL && INVARIANTS) */

#define	TAILQ_CONCAT(head1, head2, field) do {				\
	if (!TAILQ_EMPTY(head2)) {					\
		*(head1)->tqh_last = (head2)->tqh_first;		\
		(head2)->tqh_first->field.tqe_prev = (head1)->tqh_last;	\
		(head1)->tqh_last = (head2)->tqh_last;			\
		TAILQ_INIT((head2));					\
		QMD_TRACE_HEAD(head1);					\
		QMD_TRACE_HEAD(head2);					\
	}								\
} while (0)

#define	TAILQ_EMPTY(head)	((head)->tqh_first == NULL)

#define	TAILQ_FIRST(head)	((head)->tqh_first)

#define	TAILQ_FOREACH(var, head, field)					\
	for ((var) = TAILQ_FIRST((head));				\
	    (var);							\
	    (var) = TAILQ_NEXT((var), field))

#define	TAILQ_FOREACH_FROM(var, head, field)				\
	for ((var) = ((var) ? (var) : TAILQ_FIRST((head)));		\
	    (var);							\
	    (var) = TAILQ_NEXT((var), field))

#define	TAILQ_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = TAILQ_FIRST((head));				\
	    (var) && ((tvar) = TAILQ_NEXT((var), field), 1);		\
	    (var) = (tvar))

#define	TAILQ_FOREACH_FROM_SAFE(var, head, field, tvar)			\
	for ((var) = ((var) ? (var) : TAILQ_FIRST((head)));		\
	    (var) && ((tvar) = TAILQ_NEXT((var), field), 1);		\
	    (var) = (tvar))

#define	TAILQ_FOREACH_REVERSE(var, head, headname, field)		\
	for ((var) = TAILQ_LAST((head), headname);			\
	    (var);							\
	    (var) = TAILQ_PREV((var), headname, field))

#define	TAILQ_FOREACH_REVERSE_FROM(var, head, headname, field)		\
	for ((var) = ((var) ? (var) : TAILQ_LAST((head), headname));	\
	    (var);							\
	    (var) = TAILQ_PREV((var), headname, field))

#define	TAILQ_FOREACH_REVERSE_SAFE(var, head, headname, field, tvar)	\
	for ((var) = TAILQ_LAST((head), headname);			\
	    (var) && ((tvar) = TAILQ_PREV((var), headname, field), 1);	\
	    (var) = (tvar))

#define	TAILQ_FOREACH_REVERSE_FROM_SAFE(var, head, headname, field, tvar) \
	for ((var) = ((var) ? (var) : TAILQ_LAST((head), headname));	\
	    (var) && ((tvar) = TAILQ_PREV((var), headname, field), 1);	\
	    (var) = (tvar))

#define	TAILQ_INIT(head) do {						\
	TAILQ_FIRST((head)) = NULL;					\
	(head)->tqh_last = &TAILQ_FIRST((head));			\
	QMD_TRACE_HEAD(head);						\
} while (0)

#define	TAILQ_INSERT_AFTER(head, listelm, elm, field) do {		\
	QMD_TAILQ_CHECK_NEXT(listelm, field);				\
	if ((TAILQ_NEXT((elm), field) = TAILQ_NEXT((listelm), field)) != NULL)\
		TAILQ_NEXT((elm), field)->field.tqe_prev = 		\
		    &TAILQ_NEXT((elm), field);				\
	else {								\
		(head)->tqh_last = &TAILQ_NEXT((elm), field);		\
		QMD_TRACE_HEAD(head);					\
	}								\
	TAILQ_NEXT((listelm), field) = (elm);				\
	(elm)->field.tqe_prev = &TAILQ_NEXT((listelm), field);		\
	QMD_TRACE_ELEM(&(elm)->field);					\
	QMD_TRACE_ELEM(&(listelm)->field);				\
} while (0)

#define	TAILQ_INSERT_BEFORE(listelm, elm, field) do {			\
	QMD_TAILQ_CHECK_PREV(listelm, field);				\
	(elm)->field.tqe_prev = (listelm)->field.tqe_prev;		\
	TAILQ_NEXT((elm), field) = (listelm);				\
	*(listelm)->field.tqe_prev = (elm);				\
	(listelm)->field.tqe_prev = &TAILQ_NEXT((elm), field);		\
	QMD_TRACE_ELEM(&(elm)->field);					\
	QMD_TRACE_ELEM(&(listelm)->field);				\
} while (0)

#define	TAILQ_INSERT_HEAD(head, elm, field) do {			\
	QMD_TAILQ_CHECK_HEAD(head, field);				\
	if ((TAILQ_NEXT((elm), field) = TAILQ_FIRST((head))) != NULL)	\
		TAILQ_FIRST((head))->field.tqe_prev =			\
		    &TAILQ_NEXT((elm), field);				\
	else								\
		(head)->tqh_last = &TAILQ_NEXT((elm), field);		\
	TAILQ_FIRST((head)) = (elm);					\
	(elm)->field.tqe_prev = &TAILQ_FIRST((head));			\
	QMD_TRACE_HEAD(head);						\
	QMD_TRACE_ELEM(&(elm)->field);					\
} while (0)

#define	TAILQ_INSERT_TAIL(head, elm, field) do {			\
	QMD_TAILQ_CHECK_TAIL(head, field);				\
	TAILQ_NEXT((elm), field) = NULL;				\
	(elm)->field.tqe_prev = (head)->tqh_last;			\
	*(head)->tqh_last = (elm);					\
	(head)->tqh_last = &TAILQ_NEXT((elm), field);			\
	QMD_TRACE_HEAD(head);						\
	QMD_TRACE_ELEM(&(elm)->field);					\
} while (0)

#define	TAILQ_LAST(head, headname)					\
	(*(((struct headname *)((head)->tqh_last))->tqh_last))

#define	TAILQ_NEXT(elm, field) ((elm)->field.tqe_next)

#define	TAILQ_PREV(elm, headname, field)				\
	(*(((struct headname *)((elm)->field.tqe_prev))->tqh_last))

#define	TAILQ_REMOVE(head, elm, field) do {				\
	QMD_SAVELINK(oldnext, (elm)->field.tqe_next);			\
	QMD_SAVELINK(oldprev, (elm)->field.tqe_prev);			\
	QMD_TAILQ_CHECK_NEXT(elm, field);				\
	QMD_TAILQ_CHECK_PREV(elm, field);				\
	if ((TAILQ_NEXT((elm), field)) != NULL)				\
		TAILQ_NEXT((elm), field)->field.tqe_prev = 		\
		    (elm)->field.tqe_prev;				\
	else {								\
		(head)->tqh_last = (elm)->field.tqe_prev;		\
		QMD_TRACE_HEAD(head);					\
	}								\
	*(elm)->field.tqe_prev = TAILQ_NEXT((elm), field);		\
	TRASHIT(*oldnext);						\
	TRASHIT(*oldprev);						\
	QMD_TRACE_ELEM(&(elm)->field);					\
} while (0)

#define TAILQ_SWAP(head1, head2, type, field) do {			\
	QUEUE_TYPEOF(type) *swap_first = (head1)->tqh_first;		\
	QUEUE_TYPEOF(type) **swap_last = (head1)->tqh_last;		\
	(head1)->tqh_first = (head2)->tqh_first;			\
	(head1)->tqh_last = (head2)->tqh_last;				\
	(head2)->tqh_first = swap_first;				\
	(head2)->tqh_last = swap_last;					\
	if ((swap_first = (head1)->tqh_first) != NULL)			\
		swap_first->field.tqe_prev = &(head1)->tqh_first;	\
	else								\
		(head1)->tqh_last = &(head1)->tqh_first;		\
	if ((swap_first = (head2)->tqh_first) != NULL)			\
		swap_first->field.tqe_prev = &(head2)->tqh_first;	\
	else								\
		(head2)->tqh_last = &(head2)->tqh_first;		\
} while (0)

#endif /* !_SYS_QUEUE_H_ */
#ifdef MG_MODULE_LINES
#line 1 "mongoose/src/features.h"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_MONGOOSE_SRC_FEATURES_H_
#define CS_MONGOOSE_SRC_FEATURES_H_

#ifndef MG_DISABLE_HTTP_DIGEST_AUTH
#define MG_DISABLE_HTTP_DIGEST_AUTH 0
#endif

#ifndef MG_DISABLE_HTTP_KEEP_ALIVE
#define MG_DISABLE_HTTP_KEEP_ALIVE 0
#endif

#ifndef MG_DISABLE_PFS
#define MG_DISABLE_PFS 0
#endif

#ifndef MG_DISABLE_WS_RANDOM_MASK
#define MG_DISABLE_WS_RANDOM_MASK 0
#endif

#ifndef MG_ENABLE_ASYNC_RESOLVER
#define MG_ENABLE_ASYNC_RESOLVER 1
#endif

#ifndef MG_ENABLE_BROADCAST
#define MG_ENABLE_BROADCAST 0
#endif

#ifndef MG_ENABLE_COAP
#define MG_ENABLE_COAP 0
#endif

#ifndef MG_ENABLE_DEBUG
#define MG_ENABLE_DEBUG 0
#endif

#ifndef MG_ENABLE_DIRECTORY_LISTING
#define MG_ENABLE_DIRECTORY_LISTING 0
#endif

#ifndef MG_ENABLE_DNS
#define MG_ENABLE_DNS 1
#endif

#ifndef MG_ENABLE_DNS_SERVER
#define MG_ENABLE_DNS_SERVER 0
#endif

#ifndef MG_ENABLE_FAKE_DAVLOCK
#define MG_ENABLE_FAKE_DAVLOCK 0
#endif

#ifndef MG_ENABLE_FILESYSTEM
#define MG_ENABLE_FILESYSTEM 0
#endif

#ifndef MG_ENABLE_GETADDRINFO
#define MG_ENABLE_GETADDRINFO 0
#endif

#ifndef MG_ENABLE_HEXDUMP
#define MG_ENABLE_HEXDUMP CS_ENABLE_STDIO
#endif

#ifndef MG_ENABLE_HTTP
#define MG_ENABLE_HTTP 1
#endif

#ifndef MG_ENABLE_HTTP_CGI
#define MG_ENABLE_HTTP_CGI 0
#endif

#ifndef MG_ENABLE_HTTP_SSI
#define MG_ENABLE_HTTP_SSI MG_ENABLE_FILESYSTEM
#endif

#ifndef MG_ENABLE_HTTP_SSI_EXEC
#define MG_ENABLE_HTTP_SSI_EXEC 0
#endif

#ifndef MG_ENABLE_HTTP_STREAMING_MULTIPART
#define MG_ENABLE_HTTP_STREAMING_MULTIPART 0
#endif

#ifndef MG_ENABLE_HTTP_WEBDAV
#define MG_ENABLE_HTTP_WEBDAV 0
#endif

#ifndef MG_ENABLE_HTTP_WEBSOCKET
#define MG_ENABLE_HTTP_WEBSOCKET MG_ENABLE_HTTP
#endif

#ifndef MG_ENABLE_IPV6
#define MG_ENABLE_IPV6 0
#endif

#ifndef MG_ENABLE_MQTT
#define MG_ENABLE_MQTT 1
#endif

#ifndef MG_ENABLE_SOCKS
#define MG_ENABLE_SOCKS 0
#endif

#ifndef MG_ENABLE_MQTT_BROKER
#define MG_ENABLE_MQTT_BROKER 0
#endif

#ifndef MG_ENABLE_SSL
#define MG_ENABLE_SSL 0
#endif

#ifndef MG_ENABLE_SYNC_RESOLVER
#define MG_ENABLE_SYNC_RESOLVER 0
#endif

#ifndef MG_ENABLE_STDIO
#define MG_ENABLE_STDIO CS_ENABLE_STDIO
#endif

#ifndef MG_NET_IF
#define MG_NET_IF MG_NET_IF_SOCKET
#endif

#ifndef MG_SSL_IF
#define MG_SSL_IF MG_SSL_IF_OPENSSL
#endif

#ifndef MG_ENABLE_THREADS /* ifdef-ok */
#ifdef _WIN32
#define MG_ENABLE_THREADS 1
#else
#define MG_ENABLE_THREADS 0
#endif
#endif

#if MG_ENABLE_DEBUG && !defined(CS_ENABLE_DEBUG)
#define CS_ENABLE_DEBUG 1
#endif

/* MQTT broker requires MQTT */
#if MG_ENABLE_MQTT_BROKER && !MG_ENABLE_MQTT
#undef MG_ENABLE_MQTT
#define MG_ENABLE_MQTT 1
#endif

#ifndef MG_ENABLE_HTTP_URL_REWRITES
#define MG_ENABLE_HTTP_URL_REWRITES \
  (CS_PLATFORM == CS_P_WINDOWS || CS_PLATFORM == CS_P_UNIX)
#endif

#ifndef MG_ENABLE_TUN
#define MG_ENABLE_TUN MG_ENABLE_HTTP_WEBSOCKET
#endif

#ifndef MG_ENABLE_SNTP
#define MG_ENABLE_SNTP 0
#endif

#ifndef MG_ENABLE_EXTRA_ERRORS_DESC
#define MG_ENABLE_EXTRA_ERRORS_DESC 0
#endif

#ifndef MG_ENABLE_CALLBACK_USERDATA
#define MG_ENABLE_CALLBACK_USERDATA 0
#endif

#if MG_ENABLE_CALLBACK_USERDATA
#define MG_UD_ARG(ud) , ud
#define MG_CB(cb, ud) cb, ud
#else
#define MG_UD_ARG(ud)
#define MG_CB(cb, ud) cb
#endif

#endif /* CS_MONGOOSE_SRC_FEATURES_H_ */
#ifdef MG_MODULE_LINES
#line 1 "mongoose/src/net_if.h"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_MONGOOSE_SRC_NET_IF_H_
#define CS_MONGOOSE_SRC_NET_IF_H_

/* Amalgamated: #include "common/platform.h" */

/*
 * Internal async networking core interface.
 * Consists of calls made by the core, which should not block,
 * and callbacks back into the core ("..._cb").
 * Callbacks may (will) cause methods to be invoked from within,
 * but methods are not allowed to invoke callbacks inline.
 *
 * Implementation must ensure that only one callback is invoked at any time.
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MG_MAIN_IFACE 0

struct mg_mgr;
struct mg_connection;
union socket_address;

struct mg_iface_vtable;

struct mg_iface {
  struct mg_mgr *mgr;
  void *data; /* Implementation-specific data */
  const struct mg_iface_vtable *vtable;
};

struct mg_iface_vtable {
  void (*init)(struct mg_iface *iface);
  void (*free)(struct mg_iface *iface);
  void (*add_conn)(struct mg_connection *nc);
  void (*remove_conn)(struct mg_connection *nc);
  time_t (*poll)(struct mg_iface *iface, int timeout_ms);

  /* Set up a listening TCP socket on a given address. rv = 0 -> ok. */
  int (*listen_tcp)(struct mg_connection *nc, union socket_address *sa);
  /* Request that a "listening" UDP socket be created. */
  int (*listen_udp)(struct mg_connection *nc, union socket_address *sa);

  /* Request that a TCP connection is made to the specified address. */
  void (*connect_tcp)(struct mg_connection *nc, const union socket_address *sa);
  /* Open a UDP socket. Doesn't actually connect anything. */
  void (*connect_udp)(struct mg_connection *nc);

  /* Send functions for TCP and UDP. Sent data is copied before return. */
  void (*tcp_send)(struct mg_connection *nc, const void *buf, size_t len);
  void (*udp_send)(struct mg_connection *nc, const void *buf, size_t len);

  void (*recved)(struct mg_connection *nc, size_t len);

  /* Perform interface-related connection initialization. Return 1 on ok. */
  int (*create_conn)(struct mg_connection *nc);
  /* Perform interface-related cleanup on connection before destruction. */
  void (*destroy_conn)(struct mg_connection *nc);

  /* Associate a socket to a connection. */
  void (*sock_set)(struct mg_connection *nc, sock_t sock);

  /* Put connection's address into *sa, local (remote = 0) or remote. */
  void (*get_conn_addr)(struct mg_connection *nc, int remote,
                        union socket_address *sa);
};

extern const struct mg_iface_vtable *mg_ifaces[];
extern int mg_num_ifaces;

/* Creates a new interface instance. */
struct mg_iface *mg_if_create_iface(const struct mg_iface_vtable *vtable,
                                    struct mg_mgr *mgr);

/*
 * Find an interface with a given implementation. The search is started from
 * interface `from`, exclusive. Returns NULL if none is found.
 */
struct mg_iface *mg_find_iface(struct mg_mgr *mgr,
                               const struct mg_iface_vtable *vtable,
                               struct mg_iface *from);
/*
 * Deliver a new TCP connection. Returns NULL in case on error (unable to
 * create connection, in which case interface state should be discarded.
 * This is phase 1 of the two-phase process - MG_EV_ACCEPT will be delivered
 * when mg_if_accept_tcp_cb is invoked.
 */
struct mg_connection *mg_if_accept_new_conn(struct mg_connection *lc);
void mg_if_accept_tcp_cb(struct mg_connection *nc, union socket_address *sa,
                         size_t sa_len);

/* Callback invoked by connect methods. err = 0 -> ok, != 0 -> error. */
void mg_if_connect_cb(struct mg_connection *nc, int err);
/* Callback that reports that data has been put on the wire. */
void mg_if_sent_cb(struct mg_connection *nc, int num_sent);
/*
 * Receive callback.
 * if `own` is true, buf must be heap-allocated and ownership is transferred
 * to the core.
 * Core will acknowledge consumption by calling iface::recved.
 */
void mg_if_recv_tcp_cb(struct mg_connection *nc, void *buf, int len, int own);
/*
 * Receive callback.
 * buf must be heap-allocated and ownership is transferred to the core.
 * Core will acknowledge consumption by calling iface::recved.
 */
void mg_if_recv_udp_cb(struct mg_connection *nc, void *buf, int len,
                       union socket_address *sa, size_t sa_len);

/* void mg_if_close_conn(struct mg_connection *nc); */

/* Deliver a POLL event to the connection. */
void mg_if_poll(struct mg_connection *nc, time_t now);

/* Deliver a TIMER event to the connection. */
void mg_if_timer(struct mg_connection *c, double now);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_MONGOOSE_SRC_NET_IF_H_ */
#ifdef MG_MODULE_LINES
#line 1 "mongoose/src/ssl_if.h"
#endif
/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_MONGOOSE_SRC_SSL_IF_H_
#define CS_MONGOOSE_SRC_SSL_IF_H_

#if MG_ENABLE_SSL

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct mg_ssl_if_ctx;
struct mg_connection;

void mg_ssl_if_init();

enum mg_ssl_if_result {
  MG_SSL_OK = 0,
  MG_SSL_WANT_READ = -1,
  MG_SSL_WANT_WRITE = -2,
  MG_SSL_ERROR = -3,
};

struct mg_ssl_if_conn_params {
  const char *cert;
  const char *key;
  const char *ca_cert;
  const char *server_name;
  const char *cipher_suites;
  const char *psk_identity;
  const char *psk_key;
};

enum mg_ssl_if_result mg_ssl_if_conn_init(
    struct mg_connection *nc, const struct mg_ssl_if_conn_params *params,
    const char **err_msg);
enum mg_ssl_if_result mg_ssl_if_conn_accept(struct mg_connection *nc,
                                            struct mg_connection *lc);
void mg_ssl_if_conn_close_notify(struct mg_connection *nc);
void mg_ssl_if_conn_free(struct mg_connection *nc);

enum mg_ssl_if_result mg_ssl_if_handshake(struct mg_connection *nc);
int mg_ssl_if_read(struct mg_connection *nc, void *buf, size_t buf_size);
int mg_ssl_if_write(struct mg_connection *nc, const void *data, size_t len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MG_ENABLE_SSL */

#endif /* CS_MONGOOSE_SRC_SSL_IF_H_ */
#ifdef MG_MODULE_LINES
#line 1 "mongoose/src/net.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 * This software is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. For the terms of this
 * license, see <http://www.gnu.org/licenses/>.
 *
 * You are free to use this software under the terms of the GNU General
 * Public License, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * Alternatively, you can license this software under a commercial
 * license, as set out in <https://www.cesanta.com/license>.
 */

/*
 * === Core API: TCP/UDP/SSL
 *
 * NOTE: Mongoose manager is single threaded. It does not protect
 * its data structures by mutexes, therefore all functions that are dealing
 * with a particular event manager should be called from the same thread,
 * with exception of the `mg_broadcast()` function. It is fine to have different
 * event managers handled by different threads.
 */

#ifndef CS_MONGOOSE_SRC_NET_H_
#define CS_MONGOOSE_SRC_NET_H_

/* Amalgamated: #include "mongoose/src/common.h" */
/* Amalgamated: #include "mongoose/src/net_if.h" */
/* Amalgamated: #include "common/mbuf.h" */

#ifndef MG_VPRINTF_BUFFER_SIZE
#define MG_VPRINTF_BUFFER_SIZE 100
#endif

#ifdef MG_USE_READ_WRITE
#define MG_RECV_FUNC(s, b, l, f) read(s, b, l)
#define MG_SEND_FUNC(s, b, l, f) write(s, b, l)
#else
#define MG_RECV_FUNC(s, b, l, f) recv(s, b, l, f)
#define MG_SEND_FUNC(s, b, l, f) send(s, b, l, f)
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

union socket_address {
  struct sockaddr sa;
  struct sockaddr_in sin;
#if MG_ENABLE_IPV6
  struct sockaddr_in6 sin6;
#else
  struct sockaddr sin6;
#endif
};

struct mg_connection;

/*
 * Callback function (event handler) prototype. Must be defined by the user.
 * Mongoose calls the event handler, passing the events defined below.
 */
typedef void (*mg_event_handler_t)(struct mg_connection *nc, int ev,
                                   void *ev_data MG_UD_ARG(void *user_data));

/* Events. Meaning of event parameter (evp) is given in the comment. */
#define MG_EV_POLL 0    /* Sent to each connection on each mg_mgr_poll() call */
#define MG_EV_ACCEPT 1  /* New connection accepted. union socket_address * */
#define MG_EV_CONNECT 2 /* connect() succeeded or failed. int *  */
#define MG_EV_RECV 3    /* Data has been received. int *num_bytes */
#define MG_EV_SEND 4    /* Data has been written to a socket. int *num_bytes */
#define MG_EV_CLOSE 5   /* Connection is closed. NULL */
#define MG_EV_TIMER 6   /* now >= conn->ev_timer_time. double * */

/*
 * Mongoose event manager.
 */
struct mg_mgr {
  struct mg_connection *active_connections;
#if MG_ENABLE_HEXDUMP
  const char *hexdump_file; /* Debug hexdump file path */
#endif
#if MG_ENABLE_BROADCAST
  sock_t ctl[2]; /* Socketpair for mg_broadcast() */
#endif
  void *user_data; /* User data */
  int num_ifaces;
  struct mg_iface **ifaces; /* network interfaces */
  const char *nameserver;   /* DNS server to use */
};

/*
 * Mongoose connection.
 */
struct mg_connection {
  struct mg_connection *next, *prev; /* mg_mgr::active_connections linkage */
  struct mg_connection *listener;    /* Set only for accept()-ed connections */
  struct mg_mgr *mgr;                /* Pointer to containing manager */

  sock_t sock; /* Socket to the remote peer */
  int err;
  union socket_address sa; /* Remote peer address */
  size_t recv_mbuf_limit;  /* Max size of recv buffer */
  struct mbuf recv_mbuf;   /* Received data */
  struct mbuf send_mbuf;   /* Data scheduled for sending */
  time_t last_io_time;     /* Timestamp of the last socket IO */
  double ev_timer_time;    /* Timestamp of the future MG_EV_TIMER */
#if MG_ENABLE_SSL
  void *ssl_if_data; /* SSL library data. */
#endif
  mg_event_handler_t proto_handler; /* Protocol-specific event handler */
  void *proto_data;                 /* Protocol-specific data */
  void (*proto_data_destructor)(void *proto_data);
  mg_event_handler_t handler; /* Event handler function */
  void *user_data;            /* User-specific data */
  union {
    void *v;
    /*
     * the C standard is fussy about fitting function pointers into
     * void pointers, since some archs might have fat pointers for functions.
     */
    mg_event_handler_t f;
  } priv_1;
  void *priv_2;
  void *mgr_data; /* Implementation-specific event manager's data. */
  struct mg_iface *iface;
  unsigned long flags;
/* Flags set by Mongoose */
#define MG_F_LISTENING (1 << 0)          /* This connection is listening */
#define MG_F_UDP (1 << 1)                /* This connection is UDP */
#define MG_F_RESOLVING (1 << 2)          /* Waiting for async resolver */
#define MG_F_CONNECTING (1 << 3)         /* connect() call in progress */
#define MG_F_SSL (1 << 4)                /* SSL is enabled on the connection */
#define MG_F_SSL_HANDSHAKE_DONE (1 << 5) /* SSL hanshake has completed */
#define MG_F_WANT_READ (1 << 6)          /* SSL specific */
#define MG_F_WANT_WRITE (1 << 7)         /* SSL specific */
#define MG_F_IS_WEBSOCKET (1 << 8)       /* Websocket specific */

/* Flags that are settable by user */
#define MG_F_SEND_AND_CLOSE (1 << 10)       /* Push remaining data and close  */
#define MG_F_CLOSE_IMMEDIATELY (1 << 11)    /* Disconnect */
#define MG_F_WEBSOCKET_NO_DEFRAG (1 << 12)  /* Websocket specific */
#define MG_F_DELETE_CHUNK (1 << 13)         /* HTTP specific */
#define MG_F_ENABLE_BROADCAST (1 << 14)     /* Allow broadcast address usage */
#define MG_F_TUN_DO_NOT_RECONNECT (1 << 15) /* Don't reconnect tunnel */

#define MG_F_USER_1 (1 << 20) /* Flags left for application */
#define MG_F_USER_2 (1 << 21)
#define MG_F_USER_3 (1 << 22)
#define MG_F_USER_4 (1 << 23)
#define MG_F_USER_5 (1 << 24)
#define MG_F_USER_6 (1 << 25)
};

/*
 * Initialise Mongoose manager. Side effect: ignores SIGPIPE signal.
 * `mgr->user_data` field will be initialised with a `user_data` parameter.
 * That is an arbitrary pointer, where the user code can associate some data
 * with the particular Mongoose manager. For example, a C++ wrapper class
 * could be written in which case `user_data` can hold a pointer to the
 * class instance.
 */
void mg_mgr_init(struct mg_mgr *mgr, void *user_data);

/*
 * Optional parameters to `mg_mgr_init_opt()`.
 *
 * If `main_iface` is not NULL, it will be used as the main interface in the
 * default interface set. The pointer will be free'd by `mg_mgr_free`.
 * Otherwise, the main interface will be autodetected based on the current
 * platform.
 *
 * If `num_ifaces` is 0 and `ifaces` is NULL, the default interface set will be
 * used.
 * This is an advanced option, as it requires you to construct a full interface
 * set, including special networking interfaces required by some optional
 * features such as TCP tunneling. Memory backing `ifaces` and each of the
 * `num_ifaces` pointers it contains will be reclaimed by `mg_mgr_free`.
 */
struct mg_mgr_init_opts {
  const struct mg_iface_vtable *main_iface;
  int num_ifaces;
  const struct mg_iface_vtable **ifaces;
  const char *nameserver;
};

/*
 * Like `mg_mgr_init` but with more options.
 *
 * Notably, this allows you to create a manger and choose
 * dynamically which networking interface implementation to use.
 */
void mg_mgr_init_opt(struct mg_mgr *mgr, void *user_data,
                     struct mg_mgr_init_opts opts);

/*
 * De-initialises Mongoose manager.
 *
 * Closes and deallocates all active connections.
 */
void mg_mgr_free(struct mg_mgr *);

/*
 * This function performs the actual IO and must be called in a loop
 * (an event loop). It returns the current timestamp.
 * `milli` is the maximum number of milliseconds to sleep.
 * `mg_mgr_poll()` checks all connections for IO readiness. If at least one
 * of the connections is IO-ready, `mg_mgr_poll()` triggers the respective
 * event handlers and returns.
 */
time_t mg_mgr_poll(struct mg_mgr *, int milli);

#if MG_ENABLE_BROADCAST
/*
 * Passes a message of a given length to all connections.
 *
 * Must be called from a thread that does NOT call `mg_mgr_poll()`.
 * Note that `mg_broadcast()` is the only function
 * that can be, and must be, called from a different (non-IO) thread.
 *
 * `func` callback function will be called by the IO thread for each
 * connection. When called, the event will be `MG_EV_POLL`, and a message will
 * be passed as the `ev_data` pointer. Maximum message size is capped
 * by `MG_CTL_MSG_MESSAGE_SIZE` which is set to 8192 bytes.
 */
void mg_broadcast(struct mg_mgr *mgr, mg_event_handler_t cb, void *data,
                  size_t len);
#endif

/*
 * Iterates over all active connections.
 *
 * Returns the next connection from the list
 * of active connections or `NULL` if there are no more connections. Below
 * is the iteration idiom:
 *
 * ```c
 * for (c = mg_next(srv, NULL); c != NULL; c = mg_next(srv, c)) {
 *   // Do something with connection `c`
 * }
 * ```
 */
struct mg_connection *mg_next(struct mg_mgr *mgr, struct mg_connection *c);

/*
 * Optional parameters to `mg_add_sock_opt()`.
 *
 * `flags` is an initial `struct mg_connection::flags` bitmask to set,
 * see `MG_F_*` flags definitions.
 */
struct mg_add_sock_opts {
  void *user_data;           /* Initial value for connection's user_data */
  unsigned int flags;        /* Initial connection flags */
  const char **error_string; /* Placeholder for the error string */
  struct mg_iface *iface;    /* Interface instance */
};

/*
 * Creates a connection, associates it with the given socket and event handler
 * and adds it to the manager.
 *
 * For more options see the `mg_add_sock_opt` variant.
 */
struct mg_connection *mg_add_sock(struct mg_mgr *mgr, sock_t sock,
                                  MG_CB(mg_event_handler_t handler,
                                        void *user_data));

/*
 * Creates a connection, associates it with the given socket and event handler
 * and adds to the manager.
 *
 * See the `mg_add_sock_opts` structure for a description of the options.
 */
struct mg_connection *mg_add_sock_opt(struct mg_mgr *mgr, sock_t sock,
                                      MG_CB(mg_event_handler_t handler,
                                            void *user_data),
                                      struct mg_add_sock_opts opts);

/*
 * Optional parameters to `mg_bind_opt()`.
 *
 * `flags` is an initial `struct mg_connection::flags` bitmask to set,
 * see `MG_F_*` flags definitions.
 */
struct mg_bind_opts {
  void *user_data;           /* Initial value for connection's user_data */
  unsigned int flags;        /* Extra connection flags */
  const char **error_string; /* Placeholder for the error string */
  struct mg_iface *iface;    /* Interface instance */
#if MG_ENABLE_SSL
  /*
   * SSL settings.
   *
   * Server certificate to present to clients or client certificate to
   * present to tunnel dispatcher (for tunneled connections).
   */
  const char *ssl_cert;
  /* Private key corresponding to the certificate. If ssl_cert is set but
   * ssl_key is not, ssl_cert is used. */
  const char *ssl_key;
  /* CA bundle used to verify client certificates or tunnel dispatchers. */
  const char *ssl_ca_cert;
  /* Colon-delimited list of acceptable cipher suites.
   * Names depend on the library used, for example:
   *
   * ECDH-ECDSA-AES128-GCM-SHA256:DHE-RSA-AES128-SHA256 (OpenSSL)
   * TLS-ECDH-ECDSA-WITH-AES-128-GCM-SHA256:TLS-DHE-RSA-WITH-AES-128-GCM-SHA256
   *   (mbedTLS)
   *
   * For OpenSSL the list can be obtained by running "openssl ciphers".
   * For mbedTLS, names can be found in library/ssl_ciphersuites.c
   * If NULL, a reasonable default is used.
   */
  const char *ssl_cipher_suites;
#endif
};

/*
 * Creates a listening connection.
 *
 * See `mg_bind_opt` for full documentation.
 */
struct mg_connection *mg_bind(struct mg_mgr *mgr, const char *address,
                              MG_CB(mg_event_handler_t handler,
                                    void *user_data));
/*
 * Creates a listening connection.
 *
 * The `address` parameter specifies which address to bind to. It's format is
 * the same as for the `mg_connect()` call, where `HOST` part is optional.
 * `address` can be just a port number, e.g. `:8000`. To bind to a specific
 * interface, an IP address can be specified, e.g. `1.2.3.4:8000`. By default,
 * a TCP connection is created. To create UDP connection, prepend `udp://`
 * prefix, e.g. `udp://:8000`. To summarize, `address` parameter has following
 * format: `[PROTO://][IP_ADDRESS]:PORT`, where `PROTO` could be `tcp` or
 * `udp`.
 *
 * See the `mg_bind_opts` structure for a description of the optional
 * parameters.
 *
 * Returns a new listening connection or `NULL` on error.
 * NOTE: The connection remains owned by the manager, do not free().
 */
struct mg_connection *mg_bind_opt(struct mg_mgr *mgr, const char *address,
                                  MG_CB(mg_event_handler_t handler,
                                        void *user_data),
                                  struct mg_bind_opts opts);

/* Optional parameters to `mg_connect_opt()` */
struct mg_connect_opts {
  void *user_data;           /* Initial value for connection's user_data */
  unsigned int flags;        /* Extra connection flags */
  const char **error_string; /* Placeholder for the error string */
  struct mg_iface *iface;    /* Interface instance */
  const char *nameserver;    /* DNS server to use, NULL for default */
#if MG_ENABLE_SSL
  /*
   * SSL settings.
   * Client certificate to present to the server.
   */
  const char *ssl_cert;
  /*
   * Private key corresponding to the certificate.
   * If ssl_cert is set but ssl_key is not, ssl_cert is used.
   */
  const char *ssl_key;
  /*
   * Verify server certificate using this CA bundle. If set to "*", then SSL
   * is enabled but no cert verification is performed.
   */
  const char *ssl_ca_cert;
  /* Colon-delimited list of acceptable cipher suites.
   * Names depend on the library used, for example:
   *
   * ECDH-ECDSA-AES128-GCM-SHA256:DHE-RSA-AES128-SHA256 (OpenSSL)
   * TLS-ECDH-ECDSA-WITH-AES-128-GCM-SHA256:TLS-DHE-RSA-WITH-AES-128-GCM-SHA256
   *   (mbedTLS)
   *
   * For OpenSSL the list can be obtained by running "openssl ciphers".
   * For mbedTLS, names can be found in library/ssl_ciphersuites.c
   * If NULL, a reasonable default is used.
   */
  const char *ssl_cipher_suites;
  /*
   * Server name verification. If ssl_ca_cert is set and the certificate has
   * passed verification, its subject will be verified against this string.
   * By default (if ssl_server_name is NULL) hostname part of the address will
   * be used. Wildcard matching is supported. A special value of "*" disables
   * name verification.
   */
  const char *ssl_server_name;
  /*
   * PSK identity and key. Identity is a NUL-terminated string and key is a hex
   * string. Key must be either 16 or 32 bytes (32 or 64 hex digits) for AES-128
   * or AES-256 respectively.
   * Note: Default list of cipher suites does not include PSK suites, if you
   * want to use PSK you will need to set ssl_cipher_suites as well.
   */
  const char *ssl_psk_identity;
  const char *ssl_psk_key;
#endif
};

/*
 * Connects to a remote host.
 *
 * See `mg_connect_opt()` for full documentation.
 */
struct mg_connection *mg_connect(struct mg_mgr *mgr, const char *address,
                                 MG_CB(mg_event_handler_t handler,
                                       void *user_data));

/*
 * Connects to a remote host.
 *
 * The `address` format is `[PROTO://]HOST:PORT`. `PROTO` could be `tcp` or
 * `udp`. `HOST` could be an IP address,
 * IPv6 address (if Mongoose is compiled with `-DMG_ENABLE_IPV6`) or a host
 * name. If `HOST` is a name, Mongoose will resolve it asynchronously. Examples
 * of valid addresses: `google.com:80`, `udp://1.2.3.4:53`, `10.0.0.1:443`,
 * `[::1]:80`
 *
 * See the `mg_connect_opts` structure for a description of the optional
 * parameters.
 *
 * Returns a new outbound connection or `NULL` on error.
 *
 * NOTE: The connection remains owned by the manager, do not free().
 *
 * NOTE: To enable IPv6 addresses `-DMG_ENABLE_IPV6` should be specified
 * in the compilation flags.
 *
 * NOTE: The new connection will receive `MG_EV_CONNECT` as its first event
 * which will report the connect success status.
 * If the asynchronous resolution fails or the `connect()` syscall fails for
 * whatever reason (e.g. with `ECONNREFUSED` or `ENETUNREACH`), then
 * `MG_EV_CONNECT` event will report failure. Code example below:
 *
 * ```c
 * static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
 *   int connect_status;
 *
 *   switch (ev) {
 *     case MG_EV_CONNECT:
 *       connect_status = * (int *) ev_data;
 *       if (connect_status == 0) {
 *         // Success
 *       } else  {
 *         // Error
 *         printf("connect() error: %s\n", strerror(connect_status));
 *       }
 *       break;
 *     ...
 *   }
 * }
 *
 *   ...
 *   mg_connect(mgr, "my_site.com:80", ev_handler);
 * ```
 */
struct mg_connection *mg_connect_opt(struct mg_mgr *mgr, const char *address,
                                     MG_CB(mg_event_handler_t handler,
                                           void *user_data),
                                     struct mg_connect_opts opts);

#if MG_ENABLE_SSL && MG_NET_IF != MG_NET_IF_SIMPLELINK
/*
 * Note: This function is deprecated. Please, use SSL options in
 * mg_connect_opt.
 *
 * Enables SSL for a given connection.
 * `cert` is a server certificate file name for a listening connection
 * or a client certificate file name for an outgoing connection.
 * The certificate files must be in PEM format. The server certificate file
 * must contain a certificate, concatenated with a private key, optionally
 * concatenated with DH parameters.
 * `ca_cert` is a CA certificate or NULL if peer verification is not
 * required.
 * Return: NULL on success or error message on error.
 */
const char *mg_set_ssl(struct mg_connection *nc, const char *cert,
                       const char *ca_cert);
#endif

/*
 * Sends data to the connection.
 *
 * Note that sending functions do not actually push data to the socket.
 * They just append data to the output buffer. MG_EV_SEND will be delivered when
 * the data has actually been pushed out.
 */
void mg_send(struct mg_connection *, const void *buf, int len);

/* Enables format string warnings for mg_printf */
#if defined(__GNUC__)
__attribute__((format(printf, 2, 3)))
#endif
/* don't separate from mg_printf declaration */

/*
 * Sends `printf`-style formatted data to the connection.
 *
 * See `mg_send` for more details on send semantics.
 */
int mg_printf(struct mg_connection *, const char *fmt, ...);

/* Same as `mg_printf()`, but takes `va_list ap` as an argument. */
int mg_vprintf(struct mg_connection *, const char *fmt, va_list ap);

/*
 * Creates a socket pair.
 * `sock_type` can be either `SOCK_STREAM` or `SOCK_DGRAM`.
 * Returns 0 on failure and 1 on success.
 */
int mg_socketpair(sock_t[2], int sock_type);

#if MG_ENABLE_SYNC_RESOLVER
/*
 * Convert domain name into IP address.
 *
 * This is a utility function. If compilation flags have
 * `-DMG_ENABLE_GETADDRINFO`, then `getaddrinfo()` call is used for name
 * resolution. Otherwise, `gethostbyname()` is used.
 *
 * CAUTION: this function can block.
 * Return 1 on success, 0 on failure.
 */
int mg_resolve(const char *domain_name, char *ip_addr_buf, size_t buf_len);
#endif

/*
 * Verify given IP address against the ACL.
 *
 * `remote_ip` - an IPv4 address to check, in host byte order
 * `acl` - a comma separated list of IP subnets: `x.x.x.x/x` or `x.x.x.x`.
 * Each subnet is
 * prepended by either a - or a + sign. A plus sign means allow, where a
 * minus sign means deny. If a subnet mask is omitted, such as `-1.2.3.4`,
 * it means that only that single IP address is denied.
 * Subnet masks may vary from 0 to 32, inclusive. The default setting
 * is to allow all access. On each request the full list is traversed,
 * and the last match wins. Example:
 *
 * `-0.0.0.0/0,+192.168/16` - deny all accesses, only allow 192.168/16 subnet
 *
 * To learn more about subnet masks, see this
 * link:https://en.wikipedia.org/wiki/Subnetwork[Wikipedia page on Subnetwork].
 *
 * Returns -1 if ACL is malformed, 0 if address is disallowed, 1 if allowed.
 */
int mg_check_ip_acl(const char *acl, uint32_t remote_ip);

/*
 * Schedules an MG_EV_TIMER event to be delivered at `timestamp` time.
 * `timestamp` is UNIX time (the number of seconds since Epoch). It is
 * `double` instead of `time_t` to allow for sub-second precision.
 * Returns the old timer value.
 *
 * Example: set the connect timeout to 1.5 seconds:
 *
 * ```
 *  c = mg_connect(&mgr, "cesanta.com", ev_handler);
 *  mg_set_timer(c, mg_time() + 1.5);
 *  ...
 *
 *  void ev_handler(struct mg_connection *c, int ev, void *ev_data) {
 *  switch (ev) {
 *    case MG_EV_CONNECT:
 *      mg_set_timer(c, 0);  // Clear connect timer
 *      break;
 *    case MG_EV_TIMER:
 *      log("Connect timeout");
 *      c->flags |= MG_F_CLOSE_IMMEDIATELY;
 *      break;
 * ```
 */
double mg_set_timer(struct mg_connection *c, double timestamp);

/*
 * A sub-second precision version of time().
 */
double mg_time(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_MONGOOSE_SRC_NET_H_ */
#ifdef MG_MODULE_LINES
#line 1 "mongoose/src/uri.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/*
 * === URI
 */

#ifndef CS_MONGOOSE_SRC_URI_H_
#define CS_MONGOOSE_SRC_URI_H_

/* Amalgamated: #include "mongoose/src/net.h" */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Parses an URI and fills string chunks with locations of the respective
 * uri components within the input uri string. NULL pointers will be
 * ignored.
 *
 * General syntax:
 *
 *     [scheme://[user_info@]]host[:port][/path][?query][#fragment]
 *
 * Example:
 *
 *     foo.com:80
 *     tcp://foo.com:1234
 *     http://foo.com:80/bar?baz=1
 *     https://user:pw@foo.com:443/blah
 *
 * `path` will include the leading slash. `query` won't include the leading `?`.
 * `host` can contain embedded colons if surrounded by square brackets in order
 * to support IPv6 literal addresses.
 *
 *
 * Returns 0 on success, -1 on error.
 */
int mg_parse_uri(const struct mg_str uri, struct mg_str *scheme,
                 struct mg_str *user_info, struct mg_str *host,
                 unsigned int *port, struct mg_str *path, struct mg_str *query,
                 struct mg_str *fragment);

/*
 * Assemble URI from parts. Any of the inputs can be NULL or zero-length mg_str.
 *
 * If normalize_path is true, path is normalized by resolving relative refs.
 *
 * Result is a heap-allocated string (uri->p must be free()d after use).
 *
 * Returns 0 on success, -1 on error.
 */
int mg_assemble_uri(const struct mg_str *scheme, const struct mg_str *user_info,
                    const struct mg_str *host, unsigned int port,
                    const struct mg_str *path, const struct mg_str *query,
                    const struct mg_str *fragment, int normalize_path,
                    struct mg_str *uri);

int mg_normalize_uri_path(const struct mg_str *in, struct mg_str *out);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CS_MONGOOSE_SRC_URI_H_ */
#ifdef MG_MODULE_LINES
#line 1 "mongoose/src/util.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/*
 * === Utility API
 */

#ifndef CS_MONGOOSE_SRC_UTIL_H_
#define CS_MONGOOSE_SRC_UTIL_H_

#include <stdio.h>

/* Amalgamated: #include "mongoose/src/common.h" */
/* Amalgamated: #include "mongoose/src/net_if.h" */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef MG_MAX_PATH
#ifdef PATH_MAX
#define MG_MAX_PATH PATH_MAX
#else
#define MG_MAX_PATH 256
#endif
#endif

/*
 * Fetches substring from input string `s`, `end` into `v`.
 * Skips initial delimiter characters. Records first non-delimiter character
 * at the beginning of substring `v`. Then scans the rest of the string
 * until a delimiter character or end-of-string is found.
 * `delimiters` is a 0-terminated string containing delimiter characters.
 * Either one of `delimiters` or `end_string` terminates the search.
 * Returns an `s` pointer, advanced forward where parsing has stopped.
 */
const char *mg_skip(const char *s, const char *end_string,
                    const char *delimiters, struct mg_str *v);

/*
 * Decodes base64-encoded string `s`, `len` into the destination `dst`.
 * The destination has to have enough space to hold the decoded buffer.
 * Decoding stops either when all strings have been decoded or invalid an
 * character appeared.
 * Destination is '\0'-terminated.
 * Returns the number of decoded characters. On success, that should be equal
 * to `len`. On error (invalid character) the return value is smaller then
 * `len`.
 */
int mg_base64_decode(const unsigned char *s, int len, char *dst);

/*
 * Base64-encode chunk of memory `src`, `src_len` into the destination `dst`.
 * Destination has to have enough space to hold encoded buffer.
 * Destination is '\0'-terminated.
 */
void mg_base64_encode(const unsigned char *src, int src_len, char *dst);

#if MG_ENABLE_FILESYSTEM
/*
 * Performs a 64-bit `stat()` call against a given file.
 *
 * `path` should be UTF8 encoded.
 *
 * Return value is the same as for `stat()` syscall.
 */
int mg_stat(const char *path, cs_stat_t *st);

/*
 * Opens the given file and returns a file stream.
 *
 * `path` and `mode` should be UTF8 encoded.
 *
 * Return value is the same as for the `fopen()` call.
 */
FILE *mg_fopen(const char *path, const char *mode);

/*
 * Opens the given file and returns a file stream.
 *
 * `path` should be UTF8 encoded.
 *
 * Return value is the same as for the `open()` syscall.
 */
int mg_open(const char *path, int flag, int mode);

/*
 * Reads data from the given file stream.
 *
 * Return value is a number of bytes readen.
 */
size_t mg_fread(void *ptr, size_t size, size_t count, FILE *f);

/*
 * Writes data to the given file stream.
 *
 * Return value is a number of bytes wtitten.
 */
size_t mg_fwrite(const void *ptr, size_t size, size_t count, FILE *f);

#endif /* MG_ENABLE_FILESYSTEM */

#if MG_ENABLE_THREADS
/*
 * Starts a new detached thread.
 * Arguments and semantics are the same as pthead's `pthread_create()`.
 * `thread_func` is a thread function, `thread_func_param` is a parameter
 * that is passed to the thread function.
 */
void *mg_start_thread(void *(*thread_func)(void *), void *thread_func_param);
#endif

void mg_set_close_on_exec(sock_t);

#define MG_SOCK_STRINGIFY_IP 1
#define MG_SOCK_STRINGIFY_PORT 2
#define MG_SOCK_STRINGIFY_REMOTE 4
/*
 * Converts a connection's local or remote address into string.
 *
 * The `flags` parameter is a bit mask that controls the behaviour,
 * see `MG_SOCK_STRINGIFY_*` definitions.
 *
 * - MG_SOCK_STRINGIFY_IP - print IP address
 * - MG_SOCK_STRINGIFY_PORT - print port number
 * - MG_SOCK_STRINGIFY_REMOTE - print remote peer's IP/port, not local address
 *
 * If both port number and IP address are printed, they are separated by `:`.
 * If compiled with `-DMG_ENABLE_IPV6`, IPv6 addresses are supported.
 * Return length of the stringified address.
 */
int mg_conn_addr_to_str(struct mg_connection *c, char *buf, size_t len,
                        int flags);
#if MG_NET_IF == MG_NET_IF_SOCKET
/* Legacy interface. */
void mg_sock_to_str(sock_t sock, char *buf, size_t len, int flags);
#endif

/*
 * Convert the socket's address into string.
 *
 * `flags` is MG_SOCK_STRINGIFY_IP and/or MG_SOCK_STRINGIFY_PORT.
 */
int mg_sock_addr_to_str(const union socket_address *sa, char *buf, size_t len,
                        int flags);

#if MG_ENABLE_HEXDUMP
/*
 * Generates a human-readable hexdump of memory chunk.
 *
 * Takes a memory buffer `buf` of length `len` and creates a hex dump of that
 * buffer in `dst`. The generated output is a-la hexdump(1).
 * Returns the length of generated string, excluding terminating `\0`. If
 * returned length is bigger than `dst_len`, the overflow bytes are discarded.
 */
int mg_hexdump(const void *buf, int len, char *dst, int dst_len);

/* Same as mg_hexdump, but with output going to file instead of a buffer. */
void mg_hexdumpf(FILE *fp, const void *buf, int len);

/*
 * Generates human-readable hexdump of the data sent or received by the
 * connection. `path` is a file name where hexdump should be written.
 * `num_bytes` is a number of bytes sent/received. `ev` is one of the `MG_*`
 * events sent to an event handler. This function is supposed to be called from
 * the event handler.
 */
void mg_hexdump_connection(struct mg_connection *nc, const char *path,
                           const void *buf, int num_bytes, int ev);
#endif

/*
 * Returns true if target platform is big endian.
 */
int mg_is_big_endian(void);

/*
 * Use with cs_base64_init/update/finish in order to write out base64 in chunks.
 */
void mg_mbuf_append_base64_putc(char ch, void *user_data);

/*
 * Encode `len` bytes starting at `data` as base64 and append them to an mbuf.
 */
void mg_mbuf_append_base64(struct mbuf *mbuf, const void *data, size_t len);

/*
 * Generate a Basic Auth header and appends it to buf.
 * If pass is NULL, then user is expected to contain the credentials pair
 * already encoded as `user:pass`.
 */
void mg_basic_auth_header(const struct mg_str user, const struct mg_str pass,
                          struct mbuf *buf);

/*
 * URL-escape the specified string.
 * All non-printable characters are escaped, plus `._-$,;~()/`.
 * Input need not be NUL-terminated, but the returned string is.
 * Returned string is heap-allocated and must be free()'d.
 */
struct mg_str mg_url_encode(const struct mg_str src);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CS_MONGOOSE_SRC_UTIL_H_ */
#ifdef MG_MODULE_LINES
#line 1 "mongoose/src/http.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/*
 * === Common API reference
 */

#ifndef CS_MONGOOSE_SRC_HTTP_H_
#define CS_MONGOOSE_SRC_HTTP_H_

#if MG_ENABLE_HTTP

/* Amalgamated: #include "mongoose/src/net.h" */
/* Amalgamated: #include "common/mg_str.h" */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef MG_MAX_HTTP_HEADERS
#define MG_MAX_HTTP_HEADERS 20
#endif

#ifndef MG_MAX_HTTP_REQUEST_SIZE
#define MG_MAX_HTTP_REQUEST_SIZE 1024
#endif

#ifndef MG_MAX_HTTP_SEND_MBUF
#define MG_MAX_HTTP_SEND_MBUF 1024
#endif

#ifndef MG_CGI_ENVIRONMENT_SIZE
#define MG_CGI_ENVIRONMENT_SIZE 8192
#endif

/* HTTP message */
struct http_message {
  struct mg_str message; /* Whole message: request line + headers + body */
  struct mg_str body;    /* Message body. 0-length for requests with no body */

  /* HTTP Request line (or HTTP response line) */
  struct mg_str method; /* "GET" */
  struct mg_str uri;    /* "/my_file.html" */
  struct mg_str proto;  /* "HTTP/1.1" -- for both request and response */

  /* For responses, code and response status message are set */
  int resp_code;
  struct mg_str resp_status_msg;

  /*
   * Query-string part of the URI. For example, for HTTP request
   *    GET /foo/bar?param1=val1&param2=val2
   *    |    uri    |     query_string     |
   *
   * Note that question mark character doesn't belong neither to the uri,
   * nor to the query_string
   */
  struct mg_str query_string;

  /* Headers */
  struct mg_str header_names[MG_MAX_HTTP_HEADERS];
  struct mg_str header_values[MG_MAX_HTTP_HEADERS];
};

#if MG_ENABLE_HTTP_WEBSOCKET
/* WebSocket message */
struct websocket_message {
  unsigned char *data;
  size_t size;
  unsigned char flags;
};
#endif

/* HTTP multipart part */
struct mg_http_multipart_part {
  const char *file_name;
  const char *var_name;
  struct mg_str data;
  int status; /* <0 on error */
  void *user_data;
};

/* SSI call context */
struct mg_ssi_call_ctx {
  struct http_message *req; /* The request being processed. */
  struct mg_str file;       /* Filesystem path of the file being processed. */
  struct mg_str arg; /* The argument passed to the tag: <!-- call arg -->. */
};

/* HTTP and websocket events. void *ev_data is described in a comment. */
#define MG_EV_HTTP_REQUEST 100 /* struct http_message * */
#define MG_EV_HTTP_REPLY 101   /* struct http_message * */
#define MG_EV_HTTP_CHUNK 102   /* struct http_message * */
#define MG_EV_SSI_CALL 105     /* char * */
#define MG_EV_SSI_CALL_CTX 106 /* struct mg_ssi_call_ctx * */

#if MG_ENABLE_HTTP_WEBSOCKET
#define MG_EV_WEBSOCKET_HANDSHAKE_REQUEST 111 /* struct http_message * */
#define MG_EV_WEBSOCKET_HANDSHAKE_DONE 112    /* NULL */
#define MG_EV_WEBSOCKET_FRAME 113             /* struct websocket_message * */
#define MG_EV_WEBSOCKET_CONTROL_FRAME 114     /* struct websocket_message * */
#endif

#if MG_ENABLE_HTTP_STREAMING_MULTIPART
#define MG_EV_HTTP_MULTIPART_REQUEST 121 /* struct http_message */
#define MG_EV_HTTP_PART_BEGIN 122        /* struct mg_http_multipart_part */
#define MG_EV_HTTP_PART_DATA 123         /* struct mg_http_multipart_part */
#define MG_EV_HTTP_PART_END 124          /* struct mg_http_multipart_part */
/* struct mg_http_multipart_part */
#define MG_EV_HTTP_MULTIPART_REQUEST_END 125
#endif

/*
 * Attaches a built-in HTTP event handler to the given connection.
 * The user-defined event handler will receive following extra events:
 *
 * - MG_EV_HTTP_REQUEST: HTTP request has arrived. Parsed HTTP request
 *  is passed as
 *   `struct http_message` through the handler's `void *ev_data` pointer.
 * - MG_EV_HTTP_REPLY: The HTTP reply has arrived. The parsed HTTP reply is
 *   passed as `struct http_message` through the handler's `void *ev_data`
 *   pointer.
 * - MG_EV_HTTP_CHUNK: The HTTP chunked-encoding chunk has arrived.
 *   The parsed HTTP reply is passed as `struct http_message` through the
 *   handler's `void *ev_data` pointer. `http_message::body` would contain
 *   incomplete, reassembled HTTP body.
 *   It will grow with every new chunk that arrives, and it can
 *   potentially consume a lot of memory. An event handler may process
 *   the body as chunks are coming, and signal Mongoose to delete processed
 *   body by setting `MG_F_DELETE_CHUNK` in `mg_connection::flags`. When
 *   the last zero chunk is received,
 *   Mongoose sends `MG_EV_HTTP_REPLY` event with
 *   full reassembled body (if handler did not signal to delete chunks) or
 *   with empty body (if handler did signal to delete chunks).
 * - MG_EV_WEBSOCKET_HANDSHAKE_REQUEST: server has received the WebSocket
 *   handshake request. `ev_data` contains parsed HTTP request.
 * - MG_EV_WEBSOCKET_HANDSHAKE_DONE: server has completed the WebSocket
 *   handshake. `ev_data` is `NULL`.
 * - MG_EV_WEBSOCKET_FRAME: new WebSocket frame has arrived. `ev_data` is
 *   `struct websocket_message *`
 *
 * When compiled with MG_ENABLE_HTTP_STREAMING_MULTIPART, Mongoose parses
 * multipart requests and splits them into separate events:
 * - MG_EV_HTTP_MULTIPART_REQUEST: Start of the request.
 *   This event is sent before body is parsed. After this, the user
 *   should expect a sequence of PART_BEGIN/DATA/END requests.
 *   This is also the last time when headers and other request fields are
 *   accessible.
 * - MG_EV_HTTP_PART_BEGIN: Start of a part of a multipart message.
 *   Argument: mg_http_multipart_part with var_name and file_name set
 *   (if present). No data is passed in this message.
 * - MG_EV_HTTP_PART_DATA: new portion of data from the multipart message.
 *   Argument: mg_http_multipart_part. var_name and file_name are preserved,
 *   data is available in mg_http_multipart_part.data.
 * - MG_EV_HTTP_PART_END: End of the current part. var_name, file_name are
 *   the same, no data in the message. If status is 0, then the part is
 *   properly terminated with a boundary, status < 0 means that connection
 *   was terminated.
 * - MG_EV_HTTP_MULTIPART_REQUEST_END: End of the multipart request.
 *   Argument: mg_http_multipart_part, var_name and file_name are NULL,
 *   status = 0 means request was properly closed, < 0 means connection
 *   was terminated (note: in this case both PART_END and REQUEST_END are
 *   delivered).
 */
void mg_set_protocol_http_websocket(struct mg_connection *nc);

#if MG_ENABLE_HTTP_WEBSOCKET
/*
 * Send websocket handshake to the server.
 *
 * `nc` must be a valid connection, connected to a server. `uri` is an URI
 * to fetch, extra_headers` is extra HTTP headers to send or `NULL`.
 *
 * This function is intended to be used by websocket client.
 *
 * Note that the Host header is mandatory in HTTP/1.1 and must be
 * included in `extra_headers`. `mg_send_websocket_handshake2` offers
 * a better API for that.
 *
 * Deprecated in favour of `mg_send_websocket_handshake2`
 */
void mg_send_websocket_handshake(struct mg_connection *nc, const char *uri,
                                 const char *extra_headers);

/*
 * Send websocket handshake to the server.
 *
 * `nc` must be a valid connection, connected to a server. `uri` is an URI
 * to fetch, `host` goes into the `Host` header, `protocol` goes into the
 * `Sec-WebSocket-Proto` header (NULL to omit), extra_headers` is extra HTTP
 * headers to send or `NULL`.
 *
 * This function is intended to be used by websocket client.
 */
void mg_send_websocket_handshake2(struct mg_connection *nc, const char *path,
                                  const char *host, const char *protocol,
                                  const char *extra_headers);

/* Like mg_send_websocket_handshake2 but also passes basic auth header */
void mg_send_websocket_handshake3(struct mg_connection *nc, const char *path,
                                  const char *host, const char *protocol,
                                  const char *extra_headers, const char *user,
                                  const char *pass);

/* Same as mg_send_websocket_handshake3 but with strings not necessarily
 * NUL-temrinated */
void mg_send_websocket_handshake3v(struct mg_connection *nc,
                                   const struct mg_str path,
                                   const struct mg_str host,
                                   const struct mg_str protocol,
                                   const struct mg_str extra_headers,
                                   const struct mg_str user,
                                   const struct mg_str pass);

/*
 * Helper function that creates an outbound WebSocket connection.
 *
 * `url` is a URL to connect to. It must be properly URL-encoded, e.g. have
 * no spaces, etc. By default, `mg_connect_ws()` sends Connection and
 * Host headers. `extra_headers` is an extra HTTP header to send, e.g.
 * `"User-Agent: my-app\r\n"`.
 * If `protocol` is not NULL, then a `Sec-WebSocket-Protocol` header is sent.
 *
 * Examples:
 *
 * ```c
 *   nc1 = mg_connect_ws(mgr, ev_handler_1, "ws://echo.websocket.org", NULL,
 *                       NULL);
 *   nc2 = mg_connect_ws(mgr, ev_handler_1, "wss://echo.websocket.org", NULL,
 *                       NULL);
 *   nc3 = mg_connect_ws(mgr, ev_handler_1, "ws://api.cesanta.com",
 *                       "clubby.cesanta.com", NULL);
 * ```
 */
struct mg_connection *mg_connect_ws(struct mg_mgr *mgr,
                                    MG_CB(mg_event_handler_t event_handler,
                                          void *user_data),
                                    const char *url, const char *protocol,
                                    const char *extra_headers);

/*
 * Helper function that creates an outbound WebSocket connection
 *
 * Mostly identical to `mg_connect_ws`, but allows to provide extra parameters
 * (for example, SSL parameters)
 */
struct mg_connection *mg_connect_ws_opt(
    struct mg_mgr *mgr, MG_CB(mg_event_handler_t ev_handler, void *user_data),
    struct mg_connect_opts opts, const char *url, const char *protocol,
    const char *extra_headers);

/*
 * Send WebSocket frame to the remote end.
 *
 * `op_and_flags` specifies the frame's type. It's one of:
 *
 * - WEBSOCKET_OP_CONTINUE
 * - WEBSOCKET_OP_TEXT
 * - WEBSOCKET_OP_BINARY
 * - WEBSOCKET_OP_CLOSE
 * - WEBSOCKET_OP_PING
 * - WEBSOCKET_OP_PONG
 *
 * Orred with one of the flags:
 *
 * - WEBSOCKET_DONT_FIN: Don't set the FIN flag on the frame to be sent.
 *
 * `data` and `data_len` contain frame data.
 */
void mg_send_websocket_frame(struct mg_connection *nc, int op_and_flags,
                             const void *data, size_t data_len);

/*
 * Sends multiple websocket frames.
 *
 * Like `mg_send_websocket_frame()`, but composes a frame from multiple
 *buffers.
 */
void mg_send_websocket_framev(struct mg_connection *nc, int op_and_flags,
                              const struct mg_str *strings, int num_strings);

/*
 * Sends WebSocket frame to the remote end.
 *
 * Like `mg_send_websocket_frame()`, but allows to create formatted messages
 * with `printf()`-like semantics.
 */
void mg_printf_websocket_frame(struct mg_connection *nc, int op_and_flags,
                               const char *fmt, ...);

/* Websocket opcodes, from http://tools.ietf.org/html/rfc6455 */
#define WEBSOCKET_OP_CONTINUE 0
#define WEBSOCKET_OP_TEXT 1
#define WEBSOCKET_OP_BINARY 2
#define WEBSOCKET_OP_CLOSE 8
#define WEBSOCKET_OP_PING 9
#define WEBSOCKET_OP_PONG 10

/*
 * If set causes the FIN flag to not be set on outbound
 * frames. This enables sending multiple fragments of a single
 * logical message.
 *
 * The WebSocket protocol mandates that if the FIN flag of a data
 * frame is not set, the next frame must be a WEBSOCKET_OP_CONTINUE.
 * The last frame must have the FIN bit set.
 *
 * Note that mongoose will automatically defragment incoming messages,
 * so this flag is used only on outbound messages.
 */
#define WEBSOCKET_DONT_FIN 0x100

#endif /* MG_ENABLE_HTTP_WEBSOCKET */

/*
 * Decodes a URL-encoded string.
 *
 * Source string is specified by (`src`, `src_len`), and destination is
 * (`dst`, `dst_len`). If `is_form_url_encoded` is non-zero, then
 * `+` character is decoded as a blank space character. This function
 * guarantees to NUL-terminate the destination. If destination is too small,
 * then the source string is partially decoded and `-1` is returned.
 *Otherwise,
 * a length of the decoded string is returned, not counting final NUL.
 */
int mg_url_decode(const char *src, int src_len, char *dst, int dst_len,
                  int is_form_url_encoded);

extern void mg_hash_md5_v(size_t num_msgs, const uint8_t *msgs[],
                          const size_t *msg_lens, uint8_t *digest);
extern void mg_hash_sha1_v(size_t num_msgs, const uint8_t *msgs[],
                           const size_t *msg_lens, uint8_t *digest);

/*
 * Flags for `mg_http_is_authorized()`.
 */
#define MG_AUTH_FLAG_IS_DIRECTORY (1 << 0)
#define MG_AUTH_FLAG_IS_GLOBAL_PASS_FILE (1 << 1)
#define MG_AUTH_FLAG_ALLOW_MISSING_FILE (1 << 2)

/*
 * Checks whether an http request is authorized. `domain` is the authentication
 * realm, `passwords_file` is a htdigest file (can be created e.g. with
 * `htdigest` utility). If either `domain` or `passwords_file` is NULL, this
 * function always returns 1; otherwise checks the authentication in the
 * http request and returns 1 only if there is a match; 0 otherwise.
 */
int mg_http_is_authorized(struct http_message *hm, struct mg_str path,
                          const char *domain, const char *passwords_file,
                          int flags);

/*
 * Sends 401 Unauthorized response.
 */
void mg_http_send_digest_auth_request(struct mg_connection *c,
                                      const char *domain);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MG_ENABLE_HTTP */

#endif /* CS_MONGOOSE_SRC_HTTP_H_ */
#ifdef MG_MODULE_LINES
#line 1 "mongoose/src/http_server.h"
#endif
/*
 * === Server API reference
 */

#ifndef CS_MONGOOSE_SRC_HTTP_SERVER_H_
#define CS_MONGOOSE_SRC_HTTP_SERVER_H_

#if MG_ENABLE_HTTP

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Parses a HTTP message.
 *
 * `is_req` should be set to 1 if parsing a request, 0 if reply.
 *
 * Returns the number of bytes parsed. If HTTP message is
 * incomplete `0` is returned. On parse error, a negative number is returned.
 */
int mg_parse_http(const char *s, int n, struct http_message *hm, int is_req);

/*
 * Searches and returns the header `name` in parsed HTTP message `hm`.
 * If header is not found, NULL is returned. Example:
 *
 *     struct mg_str *host_hdr = mg_get_http_header(hm, "Host");
 */
struct mg_str *mg_get_http_header(struct http_message *hm, const char *name);

/*
 * Parses the HTTP header `hdr`. Finds variable `var_name` and stores its value
 * in the buffer `buf`, `buf_size`. Returns 0 if variable not found, non-zero
 * otherwise.
 *
 * This function is supposed to parse cookies, authentication headers, etc.
 * Example (error handling omitted):
 *
 *     char user[20];
 *     struct mg_str *hdr = mg_get_http_header(hm, "Authorization");
 *     mg_http_parse_header(hdr, "username", user, sizeof(user));
 *
 * Returns the length of the variable's value. If buffer is not large enough,
 * or variable not found, 0 is returned.
 */
int mg_http_parse_header(struct mg_str *hdr, const char *var_name, char *buf,
                         size_t buf_size);

/*
 * Gets and parses the Authorization: Basic header
 * Returns -1 if no Authorization header is found, or if
 * mg_parse_http_basic_auth
 * fails parsing the resulting header.
 */
int mg_get_http_basic_auth(struct http_message *hm, char *user, size_t user_len,
                           char *pass, size_t pass_len);

/*
 * Parses the Authorization: Basic header
 * Returns -1 iif the authorization type is not "Basic" or any other error such
 * as incorrectly encoded base64 user password pair.
 */
int mg_parse_http_basic_auth(struct mg_str *hdr, char *user, size_t user_len,
                             char *pass, size_t pass_len);

/*
 * Parses the buffer `buf`, `buf_len` that contains multipart form data chunks.
 * Stores the chunk name in a `var_name`, `var_name_len` buffer.
 * If a chunk is an uploaded file, then `file_name`, `file_name_len` is
 * filled with an uploaded file name. `chunk`, `chunk_len`
 * points to the chunk data.
 *
 * Return: number of bytes to skip to the next chunk or 0 if there are
 *         no more chunks.
 *
 * Usage example:
 *
 * ```c
 *    static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
 *      switch(ev) {
 *        case MG_EV_HTTP_REQUEST: {
 *          struct http_message *hm = (struct http_message *) ev_data;
 *          char var_name[100], file_name[100];
 *          const char *chunk;
 *          size_t chunk_len, n1, n2;
 *
 *          n1 = n2 = 0;
 *          while ((n2 = mg_parse_multipart(hm->body.p + n1,
 *                                          hm->body.len - n1,
 *                                          var_name, sizeof(var_name),
 *                                          file_name, sizeof(file_name),
 *                                          &chunk, &chunk_len)) > 0) {
 *            printf("var: %s, file_name: %s, size: %d, chunk: [%.*s]\n",
 *                   var_name, file_name, (int) chunk_len,
 *                   (int) chunk_len, chunk);
 *            n1 += n2;
 *          }
 *        }
 *        break;
 * ```
 */
size_t mg_parse_multipart(const char *buf, size_t buf_len, char *var_name,
                          size_t var_name_len, char *file_name,
                          size_t file_name_len, const char **chunk,
                          size_t *chunk_len);

/*
 * Fetches a HTTP form variable.
 *
 * Fetches a variable `name` from a `buf` into a buffer specified by `dst`,
 * `dst_len`. The destination is always zero-terminated. Returns the length of
 * a fetched variable. If not found, 0 is returned. `buf` must be valid
 * url-encoded buffer. If destination is too small or an error occured,
 * negative number is returned.
 */
int mg_get_http_var(const struct mg_str *buf, const char *name, char *dst,
                    size_t dst_len);

#if MG_ENABLE_FILESYSTEM
/*
 * This structure defines how `mg_serve_http()` works.
 * Best practice is to set only required settings, and leave the rest as NULL.
 */
struct mg_serve_http_opts {
  /* Path to web root directory */
  const char *document_root;

  /* List of index files. Default is "" */
  const char *index_files;

  /*
   * Leave as NULL to disable authentication.
   * To enable directory protection with authentication, set this to ".htpasswd"
   * Then, creating ".htpasswd" file in any directory automatically protects
   * it with digest authentication.
   * Use `mongoose` web server binary, or `htdigest` Apache utility to
   * create/manipulate passwords file.
   * Make sure `auth_domain` is set to a valid domain name.
   */
  const char *per_directory_auth_file;

  /* Authorization domain (domain name of this web server) */
  const char *auth_domain;

  /*
   * Leave as NULL to disable authentication.
   * Normally, only selected directories in the document root are protected.
   * If absolutely every access to the web server needs to be authenticated,
   * regardless of the URI, set this option to the path to the passwords file.
   * Format of that file is the same as ".htpasswd" file. Make sure that file
   * is located outside document root to prevent people fetching it.
   */
  const char *global_auth_file;

  /* Set to "no" to disable directory listing. Enabled by default. */
  const char *enable_directory_listing;

  /*
   * SSI files pattern. If not set, "**.shtml$|**.shtm$" is used.
   *
   * All files that match ssi_pattern are treated as SSI.
   *
   * Server Side Includes (SSI) is a simple interpreted server-side scripting
   * language which is most commonly used to include the contents of a file
   * into a web page. It can be useful when it is desirable to include a common
   * piece of code throughout a website, for example, headers and footers.
   *
   * In order for a webpage to recognize an SSI-enabled HTML file, the
   * filename should end with a special extension, by default the extension
   * should be either .shtml or .shtm
   *
   * Unknown SSI directives are silently ignored by Mongoose. Currently,
   * the following SSI directives are supported:
   *    &lt;!--#include FILE_TO_INCLUDE --&gt;
   *    &lt;!--#exec "COMMAND_TO_EXECUTE" --&gt;
   *    &lt;!--#call COMMAND --&gt;
   *
   * Note that &lt;!--#include ...> directive supports three path
   *specifications:
   *
   * &lt;!--#include virtual="path" --&gt;  Path is relative to web server root
   * &lt;!--#include abspath="path" --&gt;  Path is absolute or relative to the
   *                                  web server working dir
   * &lt;!--#include file="path" --&gt;,    Path is relative to current document
   * &lt;!--#include "path" --&gt;
   *
   * The include directive may be used to include the contents of a file or
   * the result of running a CGI script.
   *
   * The exec directive is used to execute
   * a command on a server, and show command's output. Example:
   *
   * &lt;!--#exec "ls -l" --&gt;
   *
   * The call directive is a way to invoke a C handler from the HTML page.
   * On each occurence of &lt;!--#call COMMAND OPTIONAL_PARAMS> directive,
   * Mongoose calls a registered event handler with MG_EV_SSI_CALL event,
   * and event parameter will point to the COMMAND OPTIONAL_PARAMS string.
   * An event handler can output any text, for example by calling
   * `mg_printf()`. This is a flexible way of generating a web page on
   * server side by calling a C event handler. Example:
   *
   * &lt;!--#call foo --&gt; ... &lt;!--#call bar --&gt;
   *
   * In the event handler:
   *    case MG_EV_SSI_CALL: {
   *      const char *param = (const char *) ev_data;
   *      if (strcmp(param, "foo") == 0) {
   *        mg_printf(c, "hello from foo");
   *      } else if (strcmp(param, "bar") == 0) {
   *        mg_printf(c, "hello from bar");
   *      }
   *      break;
   *    }
   */
  const char *ssi_pattern;

  /* IP ACL. By default, NULL, meaning all IPs are allowed to connect */
  const char *ip_acl;

#if MG_ENABLE_HTTP_URL_REWRITES
  /* URL rewrites.
   *
   * Comma-separated list of `uri_pattern=url_file_or_directory_path` rewrites.
   * When HTTP request is received, Mongoose constructs a file name from the
   * requested URI by combining `document_root` and the URI. However, if the
   * rewrite option is used and `uri_pattern` matches requested URI, then
   * `document_root` is ignored. Instead, `url_file_or_directory_path` is used,
   * which should be a full path name or a path relative to the web server's
   * current working directory. It can also be an URI (http:// or https://)
   * in which case mongoose will behave as a reverse proxy for that destination.
   *
   * Note that `uri_pattern`, as all Mongoose patterns, is a prefix pattern.
   *
   * If uri_pattern starts with `@` symbol, then Mongoose compares it with the
   * HOST header of the request. If they are equal, Mongoose sets document root
   * to `file_or_directory_path`, implementing virtual hosts support.
   * Example: `@foo.com=/document/root/for/foo.com`
   *
   * If `uri_pattern` starts with `%` symbol, then Mongoose compares it with
   * the listening port. If they match, then Mongoose issues a 301 redirect.
   * For example, to redirect all HTTP requests to the
   * HTTPS port, do `%80=https://my.site.com`. Note that the request URI is
   * automatically appended to the redirect location.
   */
  const char *url_rewrites;
#endif

  /* DAV document root. If NULL, DAV requests are going to fail. */
  const char *dav_document_root;

  /*
   * DAV passwords file. If NULL, DAV requests are going to fail.
   * If passwords file is set to "-", then DAV auth is disabled.
   */
  const char *dav_auth_file;

  /* Glob pattern for the files to hide. */
  const char *hidden_file_pattern;

  /* Set to non-NULL to enable CGI, e.g. **.cgi$|**.php$" */
  const char *cgi_file_pattern;

  /* If not NULL, ignore CGI script hashbang and use this interpreter */
  const char *cgi_interpreter;

  /*
   * Comma-separated list of Content-Type overrides for path suffixes, e.g.
   * ".txt=text/plain; charset=utf-8,.c=text/plain"
   */
  const char *custom_mime_types;

  /*
   * Extra HTTP headers to add to each server response.
   * Example: to enable CORS, set this to "Access-Control-Allow-Origin: *".
   */
  const char *extra_headers;
};

/*
 * Serves given HTTP request according to the `options`.
 *
 * Example code snippet:
 *
 * ```c
 * static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
 *   struct http_message *hm = (struct http_message *) ev_data;
 *   struct mg_serve_http_opts opts = { .document_root = "/var/www" };  // C99
 *
 *   switch (ev) {
 *     case MG_EV_HTTP_REQUEST:
 *       mg_serve_http(nc, hm, opts);
 *       break;
 *     default:
 *       break;
 *   }
 * }
 * ```
 */
void mg_serve_http(struct mg_connection *nc, struct http_message *hm,
                   struct mg_serve_http_opts opts);

/*
 * Serves a specific file with a given MIME type and optional extra headers.
 *
 * Example code snippet:
 *
 * ```c
 * static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
 *   switch (ev) {
 *     case MG_EV_HTTP_REQUEST: {
 *       struct http_message *hm = (struct http_message *) ev_data;
 *       mg_http_serve_file(nc, hm, "file.txt",
 *                          mg_mk_str("text/plain"), mg_mk_str(""));
 *       break;
 *     }
 *     ...
 *   }
 * }
 * ```
 */
void mg_http_serve_file(struct mg_connection *nc, struct http_message *hm,
                        const char *path, const struct mg_str mime_type,
                        const struct mg_str extra_headers);

#if MG_ENABLE_HTTP_STREAMING_MULTIPART

/* Callback prototype for `mg_file_upload_handler()`. */
typedef struct mg_str (*mg_fu_fname_fn)(struct mg_connection *nc,
                                        struct mg_str fname);

/*
 * File upload handler.
 * This handler can be used to implement file uploads with minimum code.
 * This handler will process MG_EV_HTTP_PART_* events and store file data into
 * a local file.
 * `local_name_fn` will be invoked with whatever name was provided by the client
 * and will expect the name of the local file to open. A return value of NULL
 * will abort file upload (client will get a "403 Forbidden" response). If
 * non-null, the returned string must be heap-allocated and will be freed by
 * the caller.
 * Exception: it is ok to return the same string verbatim.
 *
 * Example:
 *
 * ```c
 * struct mg_str upload_fname(struct mg_connection *nc, struct mg_str fname) {
 *   // Just return the same filename. Do not actually do this except in test!
 *   // fname is user-controlled and needs to be sanitized.
 *   return fname;
 * }
 * void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
 *   switch (ev) {
 *     ...
 *     case MG_EV_HTTP_PART_BEGIN:
 *     case MG_EV_HTTP_PART_DATA:
 *     case MG_EV_HTTP_PART_END:
 *       mg_file_upload_handler(nc, ev, ev_data, upload_fname);
 *       break;
 *   }
 * }
 * ```
 */
void mg_file_upload_handler(struct mg_connection *nc, int ev, void *ev_data,
                            mg_fu_fname_fn local_name_fn
                                MG_UD_ARG(void *user_data));
#endif /* MG_ENABLE_HTTP_STREAMING_MULTIPART */
#endif /* MG_ENABLE_FILESYSTEM */

/*
 * Registers a callback for a specified http endpoint
 * Note: if callback is registered it is called instead of the
 * callback provided in mg_bind
 *
 * Example code snippet:
 *
 * ```c
 * static void handle_hello1(struct mg_connection *nc, int ev, void *ev_data) {
 *   (void) ev; (void) ev_data;
 *   mg_printf(nc, "HTTP/1.0 200 OK\r\n\r\n[I am Hello1]");
 *  nc->flags |= MG_F_SEND_AND_CLOSE;
 * }
 *
 * static void handle_hello2(struct mg_connection *nc, int ev, void *ev_data) {
 *  (void) ev; (void) ev_data;
 *   mg_printf(nc, "HTTP/1.0 200 OK\r\n\r\n[I am Hello2]");
 *  nc->flags |= MG_F_SEND_AND_CLOSE;
 * }
 *
 * void init() {
 *   nc = mg_bind(&mgr, local_addr, cb1);
 *   mg_register_http_endpoint(nc, "/hello1", handle_hello1);
 *   mg_register_http_endpoint(nc, "/hello1/hello2", handle_hello2);
 * }
 * ```
 */
void mg_register_http_endpoint(struct mg_connection *nc, const char *uri_path,
                               MG_CB(mg_event_handler_t handler,
                                     void *user_data));

struct mg_http_endpoint_opts {
  void *user_data;
  /* Authorization domain (realm) */
  const char *auth_domain;
  const char *auth_file;
};

void mg_register_http_endpoint_opt(struct mg_connection *nc,
                                   const char *uri_path,
                                   mg_event_handler_t handler,
                                   struct mg_http_endpoint_opts opts);

/*
 * Authenticates a HTTP request against an opened password file.
 * Returns 1 if authenticated, 0 otherwise.
 */
int mg_http_check_digest_auth(struct http_message *hm, const char *auth_domain,
                              FILE *fp);

/*
 * Authenticates given response params against an opened password file.
 * Returns 1 if authenticated, 0 otherwise.
 *
 * It's used by mg_http_check_digest_auth().
 */
int mg_check_digest_auth(struct mg_str method, struct mg_str uri,
                         struct mg_str username, struct mg_str cnonce,
                         struct mg_str response, struct mg_str qop,
                         struct mg_str nc, struct mg_str nonce,
                         struct mg_str auth_domain, FILE *fp);

/*
 * Sends buffer `buf` of size `len` to the client using chunked HTTP encoding.
 * This function sends the buffer size as hex number + newline first, then
 * the buffer itself, then the newline. For example,
 * `mg_send_http_chunk(nc, "foo", 3)` will append the `3\r\nfoo\r\n` string
 * to the `nc->send_mbuf` output IO buffer.
 *
 * NOTE: The HTTP header "Transfer-Encoding: chunked" should be sent prior to
 * using this function.
 *
 * NOTE: do not forget to send an empty chunk at the end of the response,
 * to tell the client that everything was sent. Example:
 *
 * ```
 *   mg_printf_http_chunk(nc, "%s", "my response!");
 *   mg_send_http_chunk(nc, "", 0); // Tell the client we're finished
 * ```
 */
void mg_send_http_chunk(struct mg_connection *nc, const char *buf, size_t len);

/*
 * Sends a printf-formatted HTTP chunk.
 * Functionality is similar to `mg_send_http_chunk()`.
 */
void mg_printf_http_chunk(struct mg_connection *nc, const char *fmt, ...);

/*
 * Sends the response status line.
 * If `extra_headers` is not NULL, then `extra_headers` are also sent
 * after the response line. `extra_headers` must NOT end end with new line.
 * Example:
 *
 *      mg_send_response_line(nc, 200, "Access-Control-Allow-Origin: *");
 *
 * Will result in:
 *
 *      HTTP/1.1 200 OK\r\n
 *      Access-Control-Allow-Origin: *\r\n
 */
void mg_send_response_line(struct mg_connection *nc, int status_code,
                           const char *extra_headers);

/*
 * Sends an error response. If reason is NULL, the message will be inferred
 * from the error code (if supported).
 */
void mg_http_send_error(struct mg_connection *nc, int code, const char *reason);

/*
 * Sends a redirect response.
 * `status_code` should be either 301 or 302 and `location` point to the
 * new location.
 * If `extra_headers` is not empty, then `extra_headers` are also sent
 * after the response line. `extra_headers` must NOT end end with new line.
 *
 * Example:
 *
 *      mg_http_send_redirect(nc, 302, mg_mk_str("/login"), mg_mk_str(NULL));
 */
void mg_http_send_redirect(struct mg_connection *nc, int status_code,
                           const struct mg_str location,
                           const struct mg_str extra_headers);

/*
 * Sends the response line and headers.
 * This function sends the response line with the `status_code`, and
 * automatically
 * sends one header: either "Content-Length" or "Transfer-Encoding".
 * If `content_length` is negative, then "Transfer-Encoding: chunked" header
 * is sent, otherwise, "Content-Length" header is sent.
 *
 * NOTE: If `Transfer-Encoding` is `chunked`, then message body must be sent
 * using `mg_send_http_chunk()` or `mg_printf_http_chunk()` functions.
 * Otherwise, `mg_send()` or `mg_printf()` must be used.
 * Extra headers could be set through `extra_headers`. Note `extra_headers`
 * must NOT be terminated by a new line.
 */
void mg_send_head(struct mg_connection *n, int status_code,
                  int64_t content_length, const char *extra_headers);

/*
 * Sends a printf-formatted HTTP chunk, escaping HTML tags.
 */
void mg_printf_html_escape(struct mg_connection *nc, const char *fmt, ...);

#if MG_ENABLE_HTTP_URL_REWRITES
/*
 * Proxies a given request to a given upstream http server. The path prefix
 * in `mount` will be stripped of the path requested to the upstream server,
 * e.g. if mount is /api and upstream is http://localhost:8001/foo
 * then an incoming request to /api/bar will cause a request to
 * http://localhost:8001/foo/bar
 *
 * EXPERIMENTAL API. Please use http_serve_http + url_rewrites if a static
 * mapping is good enough.
 */
void mg_http_reverse_proxy(struct mg_connection *nc,
                           const struct http_message *hm, struct mg_str mount,
                           struct mg_str upstream);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MG_ENABLE_HTTP */

#endif /* CS_MONGOOSE_SRC_HTTP_SERVER_H_ */
#ifdef MG_MODULE_LINES
#line 1 "mongoose/src/http_client.h"
#endif
/*
 * === Client API reference
 */

#ifndef CS_MONGOOSE_SRC_HTTP_CLIENT_H_
#define CS_MONGOOSE_SRC_HTTP_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Helper function that creates an outbound HTTP connection.
 *
 * `url` is the URL to fetch. It must be properly URL-encoded, e.g. have
 * no spaces, etc. By default, `mg_connect_http()` sends the Connection and
 * Host headers. `extra_headers` is an extra HTTP header to send, e.g.
 * `"User-Agent: my-app\r\n"`.
 * If `post_data` is NULL, then a GET request is created. Otherwise, a POST
 * request is created with the specified POST data. Note that if the data being
 * posted is a form submission, the `Content-Type` header should be set
 * accordingly (see example below).
 *
 * Examples:
 *
 * ```c
 *   nc1 = mg_connect_http(mgr, ev_handler_1, "http://www.google.com", NULL,
 *                         NULL);
 *   nc2 = mg_connect_http(mgr, ev_handler_1, "https://github.com", NULL, NULL);
 *   nc3 = mg_connect_http(
 *       mgr, ev_handler_1, "my_server:8000/form_submit/",
 *       "Content-Type: application/x-www-form-urlencoded\r\n",
 *       "var_1=value_1&var_2=value_2");
 * ```
 */
struct mg_connection *mg_connect_http(
    struct mg_mgr *mgr,
    MG_CB(mg_event_handler_t event_handler, void *user_data), const char *url,
    const char *extra_headers, const char *post_data);

/*
 * Helper function that creates an outbound HTTP connection.
 *
 * Mostly identical to mg_connect_http, but allows you to provide extra
 *parameters
 * (for example, SSL parameters)
 */
struct mg_connection *mg_connect_http_opt(
    struct mg_mgr *mgr, MG_CB(mg_event_handler_t ev_handler, void *user_data),
    struct mg_connect_opts opts, const char *url, const char *extra_headers,
    const char *post_data);

/* Creates digest authentication header for a client request. */
int mg_http_create_digest_auth_header(char *buf, size_t buf_len,
                                      const char *method, const char *uri,
                                      const char *auth_domain, const char *user,
                                      const char *passwd);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CS_MONGOOSE_SRC_HTTP_CLIENT_H_ */
#ifdef MG_MODULE_LINES
#line 1 "mongoose/src/mqtt.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 * This software is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. For the terms of this
 * license, see <http://www.gnu.org/licenses/>.
 *
 * You are free to use this software under the terms of the GNU General
 * Public License, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * Alternatively, you can license this software under a commercial
 * license, as set out in <https://www.cesanta.com/license>.
 */

/*
 * === MQTT API reference
 */

#ifndef CS_MONGOOSE_SRC_MQTT_H_
#define CS_MONGOOSE_SRC_MQTT_H_

/* Amalgamated: #include "mongoose/src/net.h" */

struct mg_mqtt_message {
  int cmd;
  int qos;
  int len; /* message length in the IO buffer */
  struct mg_str topic;
  struct mg_str payload;

  uint8_t connack_ret_code; /* connack */
  uint16_t message_id;      /* puback */

  /* connect */
  uint8_t protocol_version;
  uint8_t connect_flags;
  uint16_t keep_alive_timer;
  struct mg_str protocol_name;
  struct mg_str client_id;
  struct mg_str will_topic;
  struct mg_str will_message;
  struct mg_str user_name;
  struct mg_str password;
};

struct mg_mqtt_topic_expression {
  const char *topic;
  uint8_t qos;
};

struct mg_send_mqtt_handshake_opts {
  unsigned char flags; /* connection flags */
  uint16_t keep_alive;
  const char *will_topic;
  const char *will_message;
  const char *user_name;
  const char *password;
};

/* mg_mqtt_proto_data should be in header to allow external access to it */
struct mg_mqtt_proto_data {
  uint16_t keep_alive;
  double last_control_time;
};

/* Message types */
#define MG_MQTT_CMD_CONNECT 1
#define MG_MQTT_CMD_CONNACK 2
#define MG_MQTT_CMD_PUBLISH 3
#define MG_MQTT_CMD_PUBACK 4
#define MG_MQTT_CMD_PUBREC 5
#define MG_MQTT_CMD_PUBREL 6
#define MG_MQTT_CMD_PUBCOMP 7
#define MG_MQTT_CMD_SUBSCRIBE 8
#define MG_MQTT_CMD_SUBACK 9
#define MG_MQTT_CMD_UNSUBSCRIBE 10
#define MG_MQTT_CMD_UNSUBACK 11
#define MG_MQTT_CMD_PINGREQ 12
#define MG_MQTT_CMD_PINGRESP 13
#define MG_MQTT_CMD_DISCONNECT 14

/* MQTT event types */
#define MG_MQTT_EVENT_BASE 200
#define MG_EV_MQTT_CONNECT (MG_MQTT_EVENT_BASE + MG_MQTT_CMD_CONNECT)
#define MG_EV_MQTT_CONNACK (MG_MQTT_EVENT_BASE + MG_MQTT_CMD_CONNACK)
#define MG_EV_MQTT_PUBLISH (MG_MQTT_EVENT_BASE + MG_MQTT_CMD_PUBLISH)
#define MG_EV_MQTT_PUBACK (MG_MQTT_EVENT_BASE + MG_MQTT_CMD_PUBACK)
#define MG_EV_MQTT_PUBREC (MG_MQTT_EVENT_BASE + MG_MQTT_CMD_PUBREC)
#define MG_EV_MQTT_PUBREL (MG_MQTT_EVENT_BASE + MG_MQTT_CMD_PUBREL)
#define MG_EV_MQTT_PUBCOMP (MG_MQTT_EVENT_BASE + MG_MQTT_CMD_PUBCOMP)
#define MG_EV_MQTT_SUBSCRIBE (MG_MQTT_EVENT_BASE + MG_MQTT_CMD_SUBSCRIBE)
#define MG_EV_MQTT_SUBACK (MG_MQTT_EVENT_BASE + MG_MQTT_CMD_SUBACK)
#define MG_EV_MQTT_UNSUBSCRIBE (MG_MQTT_EVENT_BASE + MG_MQTT_CMD_UNSUBSCRIBE)
#define MG_EV_MQTT_UNSUBACK (MG_MQTT_EVENT_BASE + MG_MQTT_CMD_UNSUBACK)
#define MG_EV_MQTT_PINGREQ (MG_MQTT_EVENT_BASE + MG_MQTT_CMD_PINGREQ)
#define MG_EV_MQTT_PINGRESP (MG_MQTT_EVENT_BASE + MG_MQTT_CMD_PINGRESP)
#define MG_EV_MQTT_DISCONNECT (MG_MQTT_EVENT_BASE + MG_MQTT_CMD_DISCONNECT)

/* Message flags */
#define MG_MQTT_RETAIN 0x1
#define MG_MQTT_DUP 0x4
#define MG_MQTT_QOS(qos) ((qos) << 1)
#define MG_MQTT_GET_QOS(flags) (((flags) &0x6) >> 1)
#define MG_MQTT_SET_QOS(flags, qos) (flags) = ((flags) & ~0x6) | ((qos) << 1)

/* Connection flags */
#define MG_MQTT_CLEAN_SESSION 0x02
#define MG_MQTT_HAS_WILL 0x04
#define MG_MQTT_WILL_RETAIN 0x20
#define MG_MQTT_HAS_PASSWORD 0x40
#define MG_MQTT_HAS_USER_NAME 0x80
#define MG_MQTT_GET_WILL_QOS(flags) (((flags) &0x18) >> 3)
#define MG_MQTT_SET_WILL_QOS(flags, qos) \
  (flags) = ((flags) & ~0x18) | ((qos) << 3)

/* CONNACK return codes */
#define MG_EV_MQTT_CONNACK_ACCEPTED 0
#define MG_EV_MQTT_CONNACK_UNACCEPTABLE_VERSION 1
#define MG_EV_MQTT_CONNACK_IDENTIFIER_REJECTED 2
#define MG_EV_MQTT_CONNACK_SERVER_UNAVAILABLE 3
#define MG_EV_MQTT_CONNACK_BAD_AUTH 4
#define MG_EV_MQTT_CONNACK_NOT_AUTHORIZED 5

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Attaches a built-in MQTT event handler to the given connection.
 *
 * The user-defined event handler will receive following extra events:
 *
 * - MG_EV_MQTT_CONNACK
 * - MG_EV_MQTT_PUBLISH
 * - MG_EV_MQTT_PUBACK
 * - MG_EV_MQTT_PUBREC
 * - MG_EV_MQTT_PUBREL
 * - MG_EV_MQTT_PUBCOMP
 * - MG_EV_MQTT_SUBACK
 */
void mg_set_protocol_mqtt(struct mg_connection *nc);

/* Sends an MQTT handshake. */
void mg_send_mqtt_handshake(struct mg_connection *nc, const char *client_id);

/* Sends an MQTT handshake with optional parameters. */
void mg_send_mqtt_handshake_opt(struct mg_connection *nc, const char *client_id,
                                struct mg_send_mqtt_handshake_opts);

/* Publishes a message to a given topic. */
void mg_mqtt_publish(struct mg_connection *nc, const char *topic,
                     uint16_t message_id, int flags, const void *data,
                     size_t len);

/* Subscribes to a bunch of topics. */
void mg_mqtt_subscribe(struct mg_connection *nc,
                       const struct mg_mqtt_topic_expression *topics,
                       size_t topics_len, uint16_t message_id);

/* Unsubscribes from a bunch of topics. */
void mg_mqtt_unsubscribe(struct mg_connection *nc, char **topics,
                         size_t topics_len, uint16_t message_id);

/* Sends a DISCONNECT command. */
void mg_mqtt_disconnect(struct mg_connection *nc);

/* Sends a CONNACK command with a given `return_code`. */
void mg_mqtt_connack(struct mg_connection *nc, uint8_t return_code);

/* Sends a PUBACK command with a given `message_id`. */
void mg_mqtt_puback(struct mg_connection *nc, uint16_t message_id);

/* Sends a PUBREC command with a given `message_id`. */
void mg_mqtt_pubrec(struct mg_connection *nc, uint16_t message_id);

/* Sends a PUBREL command with a given `message_id`. */
void mg_mqtt_pubrel(struct mg_connection *nc, uint16_t message_id);

/* Sends a PUBCOMP command with a given `message_id`. */
void mg_mqtt_pubcomp(struct mg_connection *nc, uint16_t message_id);

/*
 * Sends a SUBACK command with a given `message_id`
 * and a sequence of granted QoSs.
 */
void mg_mqtt_suback(struct mg_connection *nc, uint8_t *qoss, size_t qoss_len,
                    uint16_t message_id);

/* Sends a UNSUBACK command with a given `message_id`. */
void mg_mqtt_unsuback(struct mg_connection *nc, uint16_t message_id);

/* Sends a PINGREQ command. */
void mg_mqtt_ping(struct mg_connection *nc);

/* Sends a PINGRESP command. */
void mg_mqtt_pong(struct mg_connection *nc);

/*
 * Extracts the next topic expression from a SUBSCRIBE command payload.
 *
 * The topic expression name will point to a string in the payload buffer.
 * Returns the pos of the next topic expression or -1 when the list
 * of topics is exhausted.
 */
int mg_mqtt_next_subscribe_topic(struct mg_mqtt_message *msg,
                                 struct mg_str *topic, uint8_t *qos, int pos);

/*
 * Matches a topic against a topic expression
 *
 * Returns 1 if it matches; 0 otherwise.
 */
int mg_mqtt_match_topic_expression(struct mg_str exp, struct mg_str topic);

/*
 * Same as `mg_mqtt_match_topic_expression()`, but takes `exp` as a
 * NULL-terminated string.
 */
int mg_mqtt_vmatch_topic_expression(const char *exp, struct mg_str topic);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_MONGOOSE_SRC_MQTT_H_ */
#ifdef MG_MODULE_LINES
#line 1 "mongoose/src/mqtt_server.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 * This software is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. For the terms of this
 * license, see <http://www.gnu.org/licenses/>.
 *
 * You are free to use this software under the terms of the GNU General
 * Public License, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * Alternatively, you can license this software under a commercial
 * license, as set out in <https://www.cesanta.com/license>.
 */

/*
 * === MQTT Server API reference
 */

#ifndef CS_MONGOOSE_SRC_MQTT_BROKER_H_
#define CS_MONGOOSE_SRC_MQTT_BROKER_H_

#if MG_ENABLE_MQTT_BROKER

/* Amalgamated: #include "common/queue.h" */
/* Amalgamated: #include "mongoose/src/mqtt.h" */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef MG_MQTT_MAX_SESSION_SUBSCRIPTIONS
#define MG_MQTT_MAX_SESSION_SUBSCRIPTIONS 512
#endif

struct mg_mqtt_broker;

/* MQTT session (Broker side). */
struct mg_mqtt_session {
  struct mg_mqtt_broker *brk;       /* Broker */
  LIST_ENTRY(mg_mqtt_session) link; /* mg_mqtt_broker::sessions linkage */
  struct mg_connection *nc;         /* Connection with the client */
  size_t num_subscriptions;         /* Size of `subscriptions` array */
  void *user_data;                  /* User data */
  struct mg_mqtt_topic_expression *subscriptions;
};

/* MQTT broker. */
struct mg_mqtt_broker {
  LIST_HEAD(_mg_sesshead, mg_mqtt_session) sessions; /* Session list */
  void *user_data;                                   /* User data */
};

/* Initialises a MQTT broker. */
void mg_mqtt_broker_init(struct mg_mqtt_broker *brk, void *user_data);

/*
 * Processes a MQTT broker message.
 *
 * The listening connection expects a pointer to an initialised
 * `mg_mqtt_broker` structure in the `user_data` field.
 *
 * Basic usage:
 *
 * ```c
 * mg_mqtt_broker_init(&brk, NULL);
 *
 * if ((nc = mg_bind(&mgr, address, mg_mqtt_broker)) == NULL) {
 *   // fail;
 * }
 * nc->user_data = &brk;
 * ```
 *
 * New incoming connections will receive a `mg_mqtt_session` structure
 * in the connection `user_data`. The original `user_data` will be stored
 * in the `user_data` field of the session structure. This allows the user
 * handler to store user data before `mg_mqtt_broker` creates the session.
 *
 * Since only the MG_EV_ACCEPT message is processed by the listening socket,
 * for most events the `user_data` will thus point to a `mg_mqtt_session`.
 */
void mg_mqtt_broker(struct mg_connection *brk, int ev, void *data);

/*
 * Iterates over all MQTT session connections. Example:
 *
 * ```c
 * struct mg_mqtt_session *s;
 * for (s = mg_mqtt_next(brk, NULL); s != NULL; s = mg_mqtt_next(brk, s)) {
 *   // Do something
 * }
 * ```
 */
struct mg_mqtt_session *mg_mqtt_next(struct mg_mqtt_broker *brk,
                                     struct mg_mqtt_session *s);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MG_ENABLE_MQTT_BROKER */
#endif /* CS_MONGOOSE_SRC_MQTT_BROKER_H_ */
#ifdef MG_MODULE_LINES
#line 1 "mongoose/src/dns.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/*
 * === DNS API reference
 */

#ifndef CS_MONGOOSE_SRC_DNS_H_
#define CS_MONGOOSE_SRC_DNS_H_

/* Amalgamated: #include "mongoose/src/net.h" */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MG_DNS_A_RECORD 0x01     /* Lookup IP address */
#define MG_DNS_CNAME_RECORD 0x05 /* Lookup CNAME */
#define MG_DNS_PTR_RECORD 0x0c   /* Lookup PTR */
#define MG_DNS_TXT_RECORD 0x10   /* Lookup TXT */
#define MG_DNS_AAAA_RECORD 0x1c  /* Lookup IPv6 address */
#define MG_DNS_SRV_RECORD 0x21   /* Lookup SRV */
#define MG_DNS_MX_RECORD 0x0f    /* Lookup mail server for domain */
#define MG_DNS_ANY_RECORD 0xff
#define MG_DNS_NSEC_RECORD 0x2f

#define MG_MAX_DNS_QUESTIONS 32
#define MG_MAX_DNS_ANSWERS 32

#define MG_DNS_MESSAGE 100 /* High-level DNS message event */

enum mg_dns_resource_record_kind {
  MG_DNS_INVALID_RECORD = 0,
  MG_DNS_QUESTION,
  MG_DNS_ANSWER
};

/* DNS resource record. */
struct mg_dns_resource_record {
  struct mg_str name; /* buffer with compressed name */
  int rtype;
  int rclass;
  int ttl;
  enum mg_dns_resource_record_kind kind;
  struct mg_str rdata; /* protocol data (can be a compressed name) */
};

/* DNS message (request and response). */
struct mg_dns_message {
  struct mg_str pkt; /* packet body */
  uint16_t flags;
  uint16_t transaction_id;
  int num_questions;
  int num_answers;
  struct mg_dns_resource_record questions[MG_MAX_DNS_QUESTIONS];
  struct mg_dns_resource_record answers[MG_MAX_DNS_ANSWERS];
};

struct mg_dns_resource_record *mg_dns_next_record(
    struct mg_dns_message *msg, int query, struct mg_dns_resource_record *prev);

/*
 * Parses the record data from a DNS resource record.
 *
 *  - A:     struct in_addr *ina
 *  - AAAA:  struct in6_addr *ina
 *  - CNAME: char buffer
 *
 * Returns -1 on error.
 *
 * TODO(mkm): MX
 */
int mg_dns_parse_record_data(struct mg_dns_message *msg,
                             struct mg_dns_resource_record *rr, void *data,
                             size_t data_len);

/*
 * Sends a DNS query to the remote end.
 */
void mg_send_dns_query(struct mg_connection *nc, const char *name,
                       int query_type);

/*
 * Inserts a DNS header to an IO buffer.
 *
 * Returns the number of bytes inserted.
 */
int mg_dns_insert_header(struct mbuf *io, size_t pos,
                         struct mg_dns_message *msg);

/*
 * Appends already encoded questions from an existing message.
 *
 * This is useful when generating a DNS reply message which includes
 * all question records.
 *
 * Returns the number of appended bytes.
 */
int mg_dns_copy_questions(struct mbuf *io, struct mg_dns_message *msg);

/*
 * Encodes and appends a DNS resource record to an IO buffer.
 *
 * The record metadata is taken from the `rr` parameter, while the name and data
 * are taken from the parameters, encoded in the appropriate format depending on
 * record type and stored in the IO buffer. The encoded values might contain
 * offsets within the IO buffer. It's thus important that the IO buffer doesn't
 * get trimmed while a sequence of records are encoded while preparing a DNS
 * reply.
 *
 * This function doesn't update the `name` and `rdata` pointers in the `rr`
 * struct because they might be invalidated as soon as the IO buffer grows
 * again.
 *
 * Returns the number of bytes appended or -1 in case of error.
 */
int mg_dns_encode_record(struct mbuf *io, struct mg_dns_resource_record *rr,
                         const char *name, size_t nlen, const void *rdata,
                         size_t rlen);

/*
 * Encodes a DNS name.
 */
int mg_dns_encode_name(struct mbuf *io, const char *name, size_t len);

/* Low-level: parses a DNS response. */
int mg_parse_dns(const char *buf, int len, struct mg_dns_message *msg);

/*
 * Uncompresses a DNS compressed name.
 *
 * The containing DNS message is required because of the compressed encoding
 * and reference suffixes present elsewhere in the packet.
 *
 * If the name is less than `dst_len` characters long, the remainder
 * of `dst` is terminated with `\0` characters. Otherwise, `dst` is not
 * terminated.
 *
 * If `dst_len` is 0 `dst` can be NULL.
 * Returns the uncompressed name length.
 */
size_t mg_dns_uncompress_name(struct mg_dns_message *msg, struct mg_str *name,
                              char *dst, int dst_len);

/*
 * Attaches a built-in DNS event handler to the given listening connection.
 *
 * The DNS event handler parses the incoming UDP packets, treating them as DNS
 * requests. If an incoming packet gets successfully parsed by the DNS event
 * handler, a user event handler will receive an `MG_DNS_REQUEST` event, with
 * `ev_data` pointing to the parsed `struct mg_dns_message`.
 *
 * See
 * [captive_dns_server](https://github.com/cesanta/mongoose/tree/master/examples/captive_dns_server)
 * example on how to handle DNS request and send DNS reply.
 */
void mg_set_protocol_dns(struct mg_connection *nc);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CS_MONGOOSE_SRC_DNS_H_ */
#ifdef MG_MODULE_LINES
#line 1 "mongoose/src/dns_server.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/*
 * === DNS server API reference
 *
 * Disabled by default; enable with `-DMG_ENABLE_DNS_SERVER`.
 */

#ifndef CS_MONGOOSE_SRC_DNS_SERVER_H_
#define CS_MONGOOSE_SRC_DNS_SERVER_H_

#if MG_ENABLE_DNS_SERVER

/* Amalgamated: #include "mongoose/src/dns.h" */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MG_DNS_SERVER_DEFAULT_TTL 3600

struct mg_dns_reply {
  struct mg_dns_message *msg;
  struct mbuf *io;
  size_t start;
};

/*
 * Creates a DNS reply.
 *
 * The reply will be based on an existing query message `msg`.
 * The query body will be appended to the output buffer.
 * "reply + recursion allowed" will be added to the message flags and the
 * message's num_answers will be set to 0.
 *
 * Answer records can be appended with `mg_dns_send_reply` or by lower
 * level function defined in the DNS API.
 *
 * In order to send a reply use `mg_dns_send_reply`.
 * It's possible to use a connection's send buffer as reply buffer,
 * and it will work for both UDP and TCP connections.
 *
 * Example:
 *
 * ```c
 * reply = mg_dns_create_reply(&nc->send_mbuf, msg);
 * for (i = 0; i < msg->num_questions; i++) {
 *   rr = &msg->questions[i];
 *   if (rr->rtype == MG_DNS_A_RECORD) {
 *     mg_dns_reply_record(&reply, rr, 3600, &dummy_ip_addr, 4);
 *   }
 * }
 * mg_dns_send_reply(nc, &reply);
 * ```
 */
struct mg_dns_reply mg_dns_create_reply(struct mbuf *io,
                                        struct mg_dns_message *msg);

/*
 * Appends a DNS reply record to the IO buffer and to the DNS message.
 *
 * The message's num_answers field will be incremented. It's the caller's duty
 * to ensure num_answers is properly initialised.
 *
 * Returns -1 on error.
 */
int mg_dns_reply_record(struct mg_dns_reply *reply,
                        struct mg_dns_resource_record *question,
                        const char *name, int rtype, int ttl, const void *rdata,
                        size_t rdata_len);

/*
 * Sends a DNS reply through a connection.
 *
 * The DNS data is stored in an IO buffer pointed by reply structure in `r`.
 * This function mutates the content of that buffer in order to ensure that
 * the DNS header reflects the size and flags of the message, that might have
 * been updated either with `mg_dns_reply_record` or by direct manipulation of
 * `r->message`.
 *
 * Once sent, the IO buffer will be trimmed unless the reply IO buffer
 * is the connection's send buffer and the connection is not in UDP mode.
 */
void mg_dns_send_reply(struct mg_connection *nc, struct mg_dns_reply *r);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MG_ENABLE_DNS_SERVER */
#endif /* CS_MONGOOSE_SRC_DNS_SERVER_H_ */
#ifdef MG_MODULE_LINES
#line 1 "mongoose/src/resolv.h"
#endif
/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

/*
 * === API reference
 */

#ifndef CS_MONGOOSE_SRC_RESOLV_H_
#define CS_MONGOOSE_SRC_RESOLV_H_

/* Amalgamated: #include "mongoose/src/dns.h" */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum mg_resolve_err {
  MG_RESOLVE_OK = 0,
  MG_RESOLVE_NO_ANSWERS = 1,
  MG_RESOLVE_EXCEEDED_RETRY_COUNT = 2,
  MG_RESOLVE_TIMEOUT = 3
};

typedef void (*mg_resolve_callback_t)(struct mg_dns_message *dns_message,
                                      void *user_data, enum mg_resolve_err);

/* Options for `mg_resolve_async_opt`. */
struct mg_resolve_async_opts {
  const char *nameserver;
  int max_retries;    /* defaults to 2 if zero */
  int timeout;        /* in seconds; defaults to 5 if zero */
  int accept_literal; /* pseudo-resolve literal ipv4 and ipv6 addrs */
  int only_literal;   /* only resolves literal addrs; sync cb invocation */
  struct mg_connection **dns_conn; /* return DNS connection */
};

/* See `mg_resolve_async_opt()` */
int mg_resolve_async(struct mg_mgr *mgr, const char *name, int query,
                     mg_resolve_callback_t cb, void *data);

/* Set default DNS server */
void mg_set_nameserver(struct mg_mgr *mgr, const char *nameserver);

/*
 * Resolved a DNS name asynchronously.
 *
 * Upon successful resolution, the user callback will be invoked
 * with the full DNS response message and a pointer to the user's
 * context `data`.
 *
 * In case of timeout while performing the resolution the callback
 * will receive a NULL `msg`.
 *
 * The DNS answers can be extracted with `mg_next_record` and
 * `mg_dns_parse_record_data`:
 *
 * [source,c]
 * ----
 * struct in_addr ina;
 * struct mg_dns_resource_record *rr = mg_next_record(msg, MG_DNS_A_RECORD,
 *   NULL);
 * mg_dns_parse_record_data(msg, rr, &ina, sizeof(ina));
 * ----
 */
int mg_resolve_async_opt(struct mg_mgr *mgr, const char *name, int query,
                         mg_resolve_callback_t cb, void *data,
                         struct mg_resolve_async_opts opts);

/*
 * Resolve a name from `/etc/hosts`.
 *
 * Returns 0 on success, -1 on failure.
 */
int mg_resolve_from_hosts_file(const char *host, union socket_address *usa);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* CS_MONGOOSE_SRC_RESOLV_H_ */
#ifdef MG_MODULE_LINES
#line 1 "mongoose/src/coap.h"
#endif
/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 * This software is dual-licensed: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation. For the terms of this
 * license, see <http://www.gnu.org/licenses/>.
 *
 * You are free to use this software under the terms of the GNU General
 * Public License, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * Alternatively, you can license this software under a commercial
 * license, as set out in <https://www.cesanta.com/license>.
 */

/*
 * === CoAP API reference
 *
 * CoAP message format:
 *
 * ```
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
 * |Ver| T | TKL | Code | Message ID | Token (if any, TKL bytes) ...
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
 * | Options (if any) ...            |1 1 1 1 1 1 1 1| Payload (if any) ...
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
 * ```
 */

#ifndef CS_MONGOOSE_SRC_COAP_H_
#define CS_MONGOOSE_SRC_COAP_H_

#if MG_ENABLE_COAP

#define MG_COAP_MSG_TYPE_FIELD 0x2
#define MG_COAP_CODE_CLASS_FIELD 0x4
#define MG_COAP_CODE_DETAIL_FIELD 0x8
#define MG_COAP_MSG_ID_FIELD 0x10
#define MG_COAP_TOKEN_FIELD 0x20
#define MG_COAP_OPTIOMG_FIELD 0x40
#define MG_COAP_PAYLOAD_FIELD 0x80

#define MG_COAP_ERROR 0x10000
#define MG_COAP_FORMAT_ERROR (MG_COAP_ERROR | 0x20000)
#define MG_COAP_IGNORE (MG_COAP_ERROR | 0x40000)
#define MG_COAP_NOT_ENOUGH_DATA (MG_COAP_ERROR | 0x80000)
#define MG_COAP_NETWORK_ERROR (MG_COAP_ERROR | 0x100000)

#define MG_COAP_MSG_CON 0
#define MG_COAP_MSG_NOC 1
#define MG_COAP_MSG_ACK 2
#define MG_COAP_MSG_RST 3
#define MG_COAP_MSG_MAX 3

#define MG_COAP_CODECLASS_REQUEST 0
#define MG_COAP_CODECLASS_RESP_OK 2
#define MG_COAP_CODECLASS_CLIENT_ERR 4
#define MG_COAP_CODECLASS_SRV_ERR 5

#define MG_COAP_EVENT_BASE 300
#define MG_EV_COAP_CON (MG_COAP_EVENT_BASE + MG_COAP_MSG_CON)
#define MG_EV_COAP_NOC (MG_COAP_EVENT_BASE + MG_COAP_MSG_NOC)
#define MG_EV_COAP_ACK (MG_COAP_EVENT_BASE + MG_COAP_MSG_ACK)
#define MG_EV_COAP_RST (MG_COAP_EVENT_BASE + MG_COAP_MSG_RST)

/*
 * CoAP options.
 * Use mg_coap_add_option and mg_coap_free_options
 * for creation and destruction.
 */
struct mg_coap_option {
  struct mg_coap_option *next;
  uint32_t number;
  struct mg_str value;
};

/* CoAP message. See RFC 7252 for details. */
struct mg_coap_message {
  uint32_t flags;
  uint8_t msg_type;
  uint8_t code_class;
  uint8_t code_detail;
  uint16_t msg_id;
  struct mg_str token;
  struct mg_coap_option *options;
  struct mg_str payload;
  struct mg_coap_option *optiomg_tail;
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Sets CoAP protocol handler - triggers CoAP specific events. */
int mg_set_protocol_coap(struct mg_connection *nc);

/*
 * Adds a new option to mg_coap_message structure.
 * Returns pointer to the newly created option.
 * Note: options must be freed by using mg_coap_free_options
 */
struct mg_coap_option *mg_coap_add_option(struct mg_coap_message *cm,
                                          uint32_t number, char *value,
                                          size_t len);

/*
 * Frees the memory allocated for options.
 * If the cm parameter doesn't contain any option it does nothing.
 */
void mg_coap_free_options(struct mg_coap_message *cm);

/*
 * Composes a CoAP message from `mg_coap_message`
 * and sends it into `nc` connection.
 * Returns 0 on success. On error, it is a bitmask:
 *
 * - `#define MG_COAP_ERROR 0x10000`
 * - `#define MG_COAP_FORMAT_ERROR (MG_COAP_ERROR | 0x20000)`
 * - `#define MG_COAP_IGNORE (MG_COAP_ERROR | 0x40000)`
 * - `#define MG_COAP_NOT_ENOUGH_DATA (MG_COAP_ERROR | 0x80000)`
 * - `#define MG_COAP_NETWORK_ERROR (MG_COAP_ERROR | 0x100000)`
 */
uint32_t mg_coap_send_message(struct mg_connection *nc,
                              struct mg_coap_message *cm);

/*
 * Composes CoAP acknowledgement from `mg_coap_message`
 * and sends it into `nc` connection.
 * Return value: see `mg_coap_send_message()`
 */
uint32_t mg_coap_send_ack(struct mg_connection *nc, uint16_t msg_id);

/*
 * Parses CoAP message and fills mg_coap_message and returns cm->flags.
 * This is a helper function.
 *
 * NOTE: usually CoAP works over UDP, so lack of data means format error.
 * But, in theory, it is possible to use CoAP over TCP (according to RFC)
 *
 * The caller has to check results and treat COAP_NOT_ENOUGH_DATA according to
 * underlying protocol:
 *
 * - in case of UDP COAP_NOT_ENOUGH_DATA means COAP_FORMAT_ERROR,
 * - in case of TCP client can try to receive more data
 *
 * Return value: see `mg_coap_send_message()`
 */
uint32_t mg_coap_parse(struct mbuf *io, struct mg_coap_message *cm);

/*
 * Composes CoAP message from mg_coap_message structure.
 * This is a helper function.
 * Return value: see `mg_coap_send_message()`
 */
uint32_t mg_coap_compose(struct mg_coap_message *cm, struct mbuf *io);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MG_ENABLE_COAP */

#endif /* CS_MONGOOSE_SRC_COAP_H_ */
#ifdef MG_MODULE_LINES
#line 1 "mongoose/src/sntp.h"
#endif
/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_MONGOOSE_SRC_SNTP_H_
#define CS_MONGOOSE_SRC_SNTP_H_

#if MG_ENABLE_SNTP

#define MG_SNTP_EVENT_BASE 500

/*
 * Received reply from time server. Event handler parameter contains
 * pointer to mg_sntp_message structure
 */
#define MG_SNTP_REPLY (MG_SNTP_EVENT_BASE + 1)

/* Received malformed SNTP packet */
#define MG_SNTP_MALFORMED_REPLY (MG_SNTP_EVENT_BASE + 2)

/* Failed to get time from server (timeout etc) */
#define MG_SNTP_FAILED (MG_SNTP_EVENT_BASE + 3)

struct mg_sntp_message {
  /* if server sends this flags, user should not send requests to it */
  int kiss_of_death;
  /* usual mg_time */
  double time;
};

/* Establishes connection to given sntp server */
struct mg_connection *mg_sntp_connect(struct mg_mgr *mgr,
                                      MG_CB(mg_event_handler_t event_handler,
                                            void *user_data),
                                      const char *sntp_server_name);

/* Sends time request to given connection */
void mg_sntp_send_request(struct mg_connection *c);

/*
 * Helper function
 * Establishes connection to time server, tries to send request
 * repeats sending SNTP_ATTEMPTS times every SNTP_TIMEOUT sec
 * (if needed)
 * See sntp_client example
 */
struct mg_connection *mg_sntp_get_time(struct mg_mgr *mgr,
                                       mg_event_handler_t event_handler,
                                       const char *sntp_server_name);

#endif

#endif /* CS_MONGOOSE_SRC_SNTP_H_ */
#ifdef MG_MODULE_LINES
#line 1 "mongoose/src/socks.h"
#endif
/*
 * Copyright (c) 2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_MONGOOSE_SRC_SOCKS_H_
#define CS_MONGOOSE_SRC_SOCKS_H_

#if MG_ENABLE_SOCKS

#define MG_SOCKS_VERSION 5

#define MG_SOCKS_HANDSHAKE_DONE MG_F_USER_1
#define MG_SOCKS_CONNECT_DONE MG_F_USER_2

/* SOCKS5 handshake methods */
enum mg_socks_handshake_method {
  MG_SOCKS_HANDSHAKE_NOAUTH = 0,     /* Handshake method - no authentication */
  MG_SOCKS_HANDSHAKE_GSSAPI = 1,     /* Handshake method - GSSAPI auth */
  MG_SOCKS_HANDSHAKE_USERPASS = 2,   /* Handshake method - user/password auth */
  MG_SOCKS_HANDSHAKE_FAILURE = 0xff, /* Handshake method - failure */
};

/* SOCKS5 commands */
enum mg_socks_command {
  MG_SOCKS_CMD_CONNECT = 1,       /* Command: CONNECT */
  MG_SOCKS_CMD_BIND = 2,          /* Command: BIND */
  MG_SOCKS_CMD_UDP_ASSOCIATE = 3, /* Command: UDP ASSOCIATE */
};

/* SOCKS5 address types */
enum mg_socks_address_type {
  MG_SOCKS_ADDR_IPV4 = 1,   /* Address type: IPv4 */
  MG_SOCKS_ADDR_DOMAIN = 3, /* Address type: Domain name */
  MG_SOCKS_ADDR_IPV6 = 4,   /* Address type: IPv6 */
};

/* SOCKS5 response codes */
enum mg_socks_response {
  MG_SOCKS_SUCCESS = 0,            /* Response: success */
  MG_SOCKS_FAILURE = 1,            /* Response: failure */
  MG_SOCKS_NOT_ALLOWED = 2,        /* Response: connection not allowed */
  MG_SOCKS_NET_UNREACHABLE = 3,    /* Response: network unreachable */
  MG_SOCKS_HOST_UNREACHABLE = 4,   /* Response: network unreachable */
  MG_SOCKS_CONN_REFUSED = 5,       /* Response: network unreachable */
  MG_SOCKS_TTL_EXPIRED = 6,        /* Response: network unreachable */
  MG_SOCKS_CMD_NOT_SUPPORTED = 7,  /* Response: network unreachable */
  MG_SOCKS_ADDR_NOT_SUPPORTED = 8, /* Response: network unreachable */
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Turn the connection into the SOCKS server */
void mg_set_protocol_socks(struct mg_connection *c);

/* Create socks tunnel for the client connection */
struct mg_iface *mg_socks_mk_iface(struct mg_mgr *, const char *proxy_addr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
#endif
