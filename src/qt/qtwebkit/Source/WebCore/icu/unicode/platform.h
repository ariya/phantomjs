/*
******************************************************************************
*
*   Copyright (C) 1997-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*
* Note: autoconf creates platform.h from platform.h.in at configure time.
*
******************************************************************************
*
*  FILE NAME : platform.h
*
*   Date        Name        Description
*   05/13/98    nos         Creation (content moved here from ptypes.h).
*   03/02/99    stephen     Added AS400 support.
*   03/30/99    stephen     Added Linux support.
*   04/13/99    stephen     Reworked for autoconf.
******************************************************************************
*/

#ifndef _PLATFORM_H
#define _PLATFORM_H

/**
 * \file 
 * \brief Basic types for the platform 
 */

/* This file should be included before uvernum.h. */
#if defined(UVERNUM_H)
# error Do not include unicode/uvernum.h before #including unicode/platform.h.  Instead of unicode/uvernum.h, #include unicode/uversion.h
#endif

/**
 * Determine wheter to enable auto cleanup of libraries. 
 * @internal
 */
#ifndef UCLN_NO_AUTO_CLEANUP
#define UCLN_NO_AUTO_CLEANUP 1
#endif

/* Need platform.h when using CYGWINMSVC to get definitions above. Ignore everything else. */
#ifndef CYGWINMSVC

/** Define the platform we're on. */
#ifndef U_DARWIN
#define U_DARWIN
#endif

/**
 * \def U_HAVE_DIRENT_H
 * Define whether dirent.h is available 
 * @internal
 */
#ifndef U_HAVE_DIRENT_H
#define U_HAVE_DIRENT_H 1
#endif

/** Define whether inttypes.h is available */
#ifndef U_HAVE_INTTYPES_H
#define U_HAVE_INTTYPES_H 1
#endif

/**
 * Define what support for C++ streams is available.
 *     If U_IOSTREAM_SOURCE is set to 199711, then &lt;iostream&gt; is available
 * (1997711 is the date the ISO/IEC C++ FDIS was published), and then
 * one should qualify streams using the std namespace in ICU header
 * files.
 *     If U_IOSTREAM_SOURCE is set to 198506, then &lt;iostream.h&gt; is
 * available instead (198506 is the date when Stroustrup published
 * "An Extensible I/O Facility for C++" at the summer USENIX conference).
 *     If U_IOSTREAM_SOURCE is 0, then C++ streams are not available and
 * support for them will be silently suppressed in ICU.
 *
 */

#ifndef U_IOSTREAM_SOURCE
#define U_IOSTREAM_SOURCE 199711
#endif

/**
 * \def U_HAVE_STD_STRING
 * Define whether the standard C++ (STL) &lt;string&gt; header is available.
 * For platforms that do not use platform.h and do not define this constant
 * in their platform-specific headers, std_string.h defaults
 * U_HAVE_STD_STRING to 1.
 * @internal
 */
#ifndef U_HAVE_STD_STRING
#define U_HAVE_STD_STRING 1
#endif

/** @{ Determines whether specific types are available */
#ifndef U_HAVE_INT8_T
#define U_HAVE_INT8_T 1
#endif

#ifndef U_HAVE_UINT8_T
#define U_HAVE_UINT8_T 0
#endif

#ifndef U_HAVE_INT16_T
#define U_HAVE_INT16_T 1
#endif

#ifndef U_HAVE_UINT16_T
#define U_HAVE_UINT16_T 0
#endif

#ifndef U_HAVE_INT32_T
#define U_HAVE_INT32_T 1
#endif

#ifndef U_HAVE_UINT32_T
#define U_HAVE_UINT32_T 0
#endif

#ifndef U_HAVE_INT64_T
#define U_HAVE_INT64_T 1
#endif

#ifndef U_HAVE_UINT64_T
#define U_HAVE_UINT64_T 0
#endif

/** @} */

/*===========================================================================*/
/** @{ Compiler and environment features                                     */
/*===========================================================================*/

/* Define whether namespace is supported */
#ifndef U_HAVE_NAMESPACE
#define U_HAVE_NAMESPACE 1
#endif

/* Determines the endianness of the platform
   It's done this way in case multiple architectures are being built at once.
   For example, Darwin supports fat binaries, which can be both PPC and x86 based. */
#if defined(BYTE_ORDER) && defined(BIG_ENDIAN)
#define U_IS_BIG_ENDIAN (BYTE_ORDER == BIG_ENDIAN)
#else
#define U_IS_BIG_ENDIAN 1
#endif

/* 1 or 0 to enable or disable threads.  If undefined, default is: enable threads. */
#ifndef ICU_USE_THREADS 
#define ICU_USE_THREADS 1
#endif

/* On strong memory model CPUs (e.g. x86 CPUs), we use a safe & quick double check lock. */
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define UMTX_STRONG_MEMORY_MODEL 1
#endif

#ifndef U_DEBUG
#define U_DEBUG 0
#endif

#ifndef U_RELEASE
#define U_RELEASE 1
#endif

/* Determine whether to disable renaming or not. This overrides the
   setting in umachine.h which is for all platforms. */
#ifndef U_DISABLE_RENAMING
#define U_DISABLE_RENAMING 1
#endif

/* Determine whether to override new and delete. */
#ifndef U_OVERRIDE_CXX_ALLOCATION
#define U_OVERRIDE_CXX_ALLOCATION 1
#endif
/* Determine whether to override placement new and delete for STL. */
#ifndef U_HAVE_PLACEMENT_NEW
#define U_HAVE_PLACEMENT_NEW 1
#endif

/* Determine whether to enable tracing. */
#ifndef U_ENABLE_TRACING
#define U_ENABLE_TRACING 1
#endif

/**
 * Whether to enable Dynamic loading in ICU
 * @internal
 */
#ifndef U_ENABLE_DYLOAD
#define U_ENABLE_DYLOAD 1
#endif

/**
 * Whether to test Dynamic loading as an OS capabilty
 * @internal
 */
#ifndef U_CHECK_DYLOAD
#define U_CHECK_DYLOAD 1
#endif


/** Do we allow ICU users to use the draft APIs by default? */
#ifndef U_DEFAULT_SHOW_DRAFT
#define U_DEFAULT_SHOW_DRAFT 1
#endif

/** @} */

/*===========================================================================*/
/** @{ Character data types                                                      */
/*===========================================================================*/

#if ((defined(OS390) && (!defined(__CHARSET_LIB) || !__CHARSET_LIB))) || defined(OS400)
#   define U_CHARSET_FAMILY 1
#endif

/** @} */

/*===========================================================================*/
/** @{ Information about wchar support                                           */
/*===========================================================================*/

#ifndef U_HAVE_WCHAR_H
#define U_HAVE_WCHAR_H      1
#endif

#ifndef U_SIZEOF_WCHAR_T
#define U_SIZEOF_WCHAR_T    4
#endif

#ifndef U_HAVE_WCSCPY
#define U_HAVE_WCSCPY       1
#endif

/** @} */

/**
 * @{
 * \def U_DECLARE_UTF16
 * Do not use this macro. Use the UNICODE_STRING or U_STRING_DECL macros
 * instead.
 * @internal
 *
 * \def U_GNUC_UTF16_STRING
 * @internal
 */
#ifndef U_GNUC_UTF16_STRING
#define U_GNUC_UTF16_STRING 0
#endif
#if 1 || defined(U_CHECK_UTF16_STRING)
#if (defined(__xlC__) && defined(__IBM_UTF_LITERAL) && U_SIZEOF_WCHAR_T != 2) \
    || (defined(__HP_aCC) && __HP_aCC >= 035000) \
    || (defined(__HP_cc) && __HP_cc >= 111106) \
    || U_GNUC_UTF16_STRING
#define U_DECLARE_UTF16(string) u ## string
#elif (defined(__SUNPRO_CC) && __SUNPRO_CC >= 0x550)
/* || (defined(__SUNPRO_C) && __SUNPRO_C >= 0x580) */
/* Sun's C compiler has issues with this notation, and it's unreliable. */
#define U_DECLARE_UTF16(string) U ## string
#elif U_SIZEOF_WCHAR_T == 2 \
    && (U_CHARSET_FAMILY == 0 || ((defined(OS390) || defined(OS400)) && defined(__UCS2__)))
#define U_DECLARE_UTF16(string) L ## string
#endif
#endif

/** @} */

/*===========================================================================*/
/** @{ Information about POSIX support                                           */
/*===========================================================================*/

#ifndef U_HAVE_NL_LANGINFO_CODESET
#define U_HAVE_NL_LANGINFO_CODESET  1
#endif

#ifndef U_NL_LANGINFO_CODESET
#define U_NL_LANGINFO_CODESET       CODESET
#endif

#if 1
#define U_TZSET         tzset
#endif
#if 0
#define U_TIMEZONE      timezone
#endif
#if 1
#define U_TZNAME        tzname
#endif

#define U_HAVE_MMAP     1
#define U_HAVE_POPEN    1

/** @} */

/*===========================================================================*/
/** @{ Symbol import-export control                                              */
/*===========================================================================*/

#if 1
#define U_EXPORT __attribute__((visibility("default")))
#elif (defined(__SUNPRO_CC) && __SUNPRO_CC >= 0x550) \
   || (defined(__SUNPRO_C) && __SUNPRO_C >= 0x550) 
#define U_EXPORT __global
/*#elif defined(__HP_aCC) || defined(__HP_cc)
#define U_EXPORT __declspec(dllexport)*/
#else
#define U_EXPORT
#endif

/* U_CALLCONV is releated to U_EXPORT2 */
#define U_EXPORT2

/* cygwin needs to export/import data */
#if defined(U_CYGWIN) && !defined(__GNUC__)
#define U_IMPORT __declspec(dllimport)
#else
#define U_IMPORT 
#endif

/* @} */

/*===========================================================================*/
/** @{ Code alignment and C function inlining                                    */
/*===========================================================================*/

#ifndef U_INLINE
#   ifdef __cplusplus
#       define U_INLINE inline
#   else
#       define U_INLINE __inline__
#   endif
#endif

#ifndef U_ALIGN_CODE
#define U_ALIGN_CODE(n) 
#endif

/** @} */

/*===========================================================================*/
/** @{ GCC built in functions for atomic memory operations                       */
/*===========================================================================*/

/**
 * \def U_HAVE_GCC_ATOMICS
 * @internal
 */
#ifndef U_HAVE_GCC_ATOMICS
#define U_HAVE_GCC_ATOMICS 1
#endif

/** @} */

/*===========================================================================*/
/** @{ Programs used by ICU code                                                 */
/*===========================================================================*/

/**
 * \def U_MAKE
 * What program to execute to run 'make'
 */
#ifndef U_MAKE
#define U_MAKE  "/usr/bin/gnumake"
#endif

/** @} */

#endif /* CYGWINMSVC */

/*===========================================================================*/
/* Custom icu entry point renaming                                                  */
/*===========================================================================*/

/**
 * Define the library suffix with C syntax.
 * @internal
 */
# define U_LIB_SUFFIX_C_NAME 
/**
 * Define the library suffix as a string with C syntax
 * @internal
 */
# define U_LIB_SUFFIX_C_NAME_STRING ""
/**
 * 1 if a custom library suffix is set
 * @internal
 */
# define U_HAVE_LIB_SUFFIX 0

#if U_HAVE_LIB_SUFFIX
# ifndef U_ICU_ENTRY_POINT_RENAME
/* Renaming pattern:    u_strcpy_41_suffix */
#  define U_ICU_ENTRY_POINT_RENAME(x)    x ## _ ## 46 ## 
#  define U_DEF_ICUDATA_ENTRY_POINT(major, minor) icudt####major##minor##_dat

# endif
#endif

#endif
