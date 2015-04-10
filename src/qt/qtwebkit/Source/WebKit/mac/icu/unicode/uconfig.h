/*  
**********************************************************************
*   Copyright (C) 2002-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
**********************************************************************
*   file name:  uconfig.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2002sep19
*   created by: Markus W. Scherer
*/

#ifndef __UCONFIG_H__
#define __UCONFIG_H__


/*!
 * \file
 * \brief Switches for excluding parts of ICU library code modules.
 *
 * Allows to build partial, smaller libraries for special purposes.
 * By default, all modules are built.
 * The switches are fairly coarse, controlling large modules.
 * Basic services cannot be turned off.
 *
 * Building with any of these options does not guarantee that the
 * ICU build process will completely work. It is recommended that
 * the ICU libraries and data be built using the normal build.
 * At that time you should remove the data used by those services.
 * After building the ICU data library, you should rebuild the ICU
 * libraries with these switches customized to your needs.
 *
 * @stable ICU 2.4
 */

/**
 * If this switch is defined, ICU will attempt to load a header file named "uconfig_local.h"
 * prior to determining default settings for uconfig variables.
 * 
 * @internal ICU 4.0
 * 
 */
#if defined(UCONFIG_USE_LOCAL)
#include "uconfig_local.h"
#endif

/**
 * \def UCONFIG_ONLY_COLLATION
 * This switch turns off modules that are not needed for collation.
 *
 * It does not turn off legacy conversion because that is necessary
 * for ICU to work on EBCDIC platforms (for the default converter).
 * If you want "only collation" and do not build for EBCDIC,
 * then you can define UCONFIG_NO_LEGACY_CONVERSION 1 as well.
 *
 * @stable ICU 2.4
 */
#ifndef UCONFIG_ONLY_COLLATION
#   define UCONFIG_ONLY_COLLATION 0
#endif

#if UCONFIG_ONLY_COLLATION
    /* common library */
#   define UCONFIG_NO_BREAK_ITERATION 1
#   define UCONFIG_NO_IDNA 1

    /* i18n library */
#   if UCONFIG_NO_COLLATION
#       error Contradictory collation switches in uconfig.h.
#   endif
#   define UCONFIG_NO_FORMATTING 1
#   define UCONFIG_NO_TRANSLITERATION 1
#   define UCONFIG_NO_REGULAR_EXPRESSIONS 1
#endif

/* common library switches -------------------------------------------------- */

/**
 * \def UCONFIG_NO_FILE_IO
 * This switch turns off all file access in the common library
 * where file access is only used for data loading.
 * ICU data must then be provided in the form of a data DLL (or with an
 * equivalent way to link to the data residing in an executable,
 * as in building a combined library with both the common library's code and
 * the data), or via udata_setCommonData().
 * Application data must be provided via udata_setAppData() or by using
 * "open" functions that take pointers to data, for example ucol_openBinary().
 *
 * File access is not used at all in the i18n library.
 *
 * File access cannot be turned off for the icuio library or for the ICU
 * test suites and ICU tools.
 *
 * @stable ICU 3.6
 */
#ifndef UCONFIG_NO_FILE_IO
#   define UCONFIG_NO_FILE_IO 0
#endif

/**
 * \def UCONFIG_NO_CONVERSION
 * ICU will not completely build with this switch turned on.
 * This switch turns off all converters.
 *
 * You may want to use this together with U_CHARSET_IS_UTF8 defined to 1
 * in utypes.h if char* strings in your environment are always in UTF-8.
 *
 * @stable ICU 3.2
 * @see U_CHARSET_IS_UTF8
 */
#ifndef UCONFIG_NO_CONVERSION
#   define UCONFIG_NO_CONVERSION 0
#endif

#if UCONFIG_NO_CONVERSION
#   define UCONFIG_NO_LEGACY_CONVERSION 1
#endif

/**
 * \def UCONFIG_NO_LEGACY_CONVERSION
 * This switch turns off all converters except for
 * - Unicode charsets (UTF-7/8/16/32, CESU-8, SCSU, BOCU-1)
 * - US-ASCII
 * - ISO-8859-1
 *
 * Turning off legacy conversion is not possible on EBCDIC platforms
 * because they need ibm-37 or ibm-1047 default converters.
 *
 * @stable ICU 2.4
 */
#ifndef UCONFIG_NO_LEGACY_CONVERSION
#   define UCONFIG_NO_LEGACY_CONVERSION 0
#endif

/**
 * \def UCONFIG_NO_NORMALIZATION
 * This switch turns off normalization.
 * It implies turning off several other services as well, for example
 * collation and IDNA.
 *
 * @stable ICU 2.6
 */
#ifndef UCONFIG_NO_NORMALIZATION
#   define UCONFIG_NO_NORMALIZATION 0
#elif UCONFIG_NO_NORMALIZATION
    /* common library */
#   define UCONFIG_NO_IDNA 1

    /* i18n library */
#   if UCONFIG_ONLY_COLLATION
#       error Contradictory collation switches in uconfig.h.
#   endif
#   define UCONFIG_NO_COLLATION 1
#   define UCONFIG_NO_TRANSLITERATION 1
#endif

/**
 * \def UCONFIG_NO_BREAK_ITERATION
 * This switch turns off break iteration.
 *
 * @stable ICU 2.4
 */
#ifndef UCONFIG_NO_BREAK_ITERATION
#   define UCONFIG_NO_BREAK_ITERATION 0
#endif

/**
 * \def UCONFIG_NO_IDNA
 * This switch turns off IDNA.
 *
 * @stable ICU 2.6
 */
#ifndef UCONFIG_NO_IDNA
#   define UCONFIG_NO_IDNA 0
#endif

/* i18n library switches ---------------------------------------------------- */

/**
 * \def UCONFIG_NO_COLLATION
 * This switch turns off collation and collation-based string search.
 *
 * @stable ICU 2.4
 */
#ifndef UCONFIG_NO_COLLATION
#   define UCONFIG_NO_COLLATION 0
#endif

/**
 * \def UCONFIG_NO_FORMATTING
 * This switch turns off formatting and calendar/timezone services.
 *
 * @stable ICU 2.4
 */
#ifndef UCONFIG_NO_FORMATTING
#   define UCONFIG_NO_FORMATTING 0
#endif

/**
 * \def UCONFIG_NO_TRANSLITERATION
 * This switch turns off transliteration.
 *
 * @stable ICU 2.4
 */
#ifndef UCONFIG_NO_TRANSLITERATION
#   define UCONFIG_NO_TRANSLITERATION 0
#endif

/**
 * \def UCONFIG_NO_REGULAR_EXPRESSIONS
 * This switch turns off regular expressions.
 *
 * @stable ICU 2.4
 */
#ifndef UCONFIG_NO_REGULAR_EXPRESSIONS
#   define UCONFIG_NO_REGULAR_EXPRESSIONS 0
#endif

/**
 * \def UCONFIG_NO_SERVICE
 * This switch turns off service registration.
 *
 * @stable ICU 3.2
 */
#ifndef UCONFIG_NO_SERVICE
#   define UCONFIG_NO_SERVICE 1
#endif

#endif
