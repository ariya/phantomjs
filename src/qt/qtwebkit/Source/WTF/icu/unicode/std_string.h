/*
*******************************************************************************
*
*   Copyright (C) 2009-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  std_string.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2009feb19
*   created by: Markus W. Scherer
*/

#ifndef __STD_STRING_H__
#define __STD_STRING_H__

/**
 * \file 
 * \brief C++ API: Central ICU header for including the C++ standard &lt;string&gt;
 *                 header and for related definitions.
 */

#include "unicode/utypes.h"

/**
 * \def U_HAVE_STD_STRING
 * Define whether the standard C++ (STL) &lt;string&gt; header is available.
 * @internal
 */
#ifndef U_HAVE_STD_STRING
#define U_HAVE_STD_STRING 1
#endif

#if U_HAVE_STD_STRING

#include <string>

/**
 * \def U_STD_NS
 * Define the namespace to use for standard C++ (STL) classes.
 * Either std or empty.
 * @draft ICU 4.2
 */

/**
 * \def U_STD_NSQ
 * Define the namespace qualifier to use for standard C++ (STL) classes.
 * Either std:: or empty.
 * For example,
 *   U_STD_NSQ string StringFromUnicodeString(const UnicodeString &unistr);
 * @draft ICU 4.2
 */

/**
 * \def U_STD_NS_USE
 * This is used to specify that the rest of the code uses the
 * standard (STL) namespace.
 * Either "using namespace std;" or empty.
 * @draft ICU 4.2
 */
#ifndef U_STD_NSQ
#   if U_HAVE_NAMESPACE
#       define U_STD_NS std
#       define U_STD_NSQ U_STD_NS::
#       define U_STD_NS_USE using namespace U_STD_NS;
#   else
#       define U_STD_NS
#       define U_STD_NSQ
#       define U_STD_NS_USE
#   endif
#endif

#endif  // U_HAVE_STD_STRING

#endif  // __STD_STRING_H__
