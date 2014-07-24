/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2012 Intel Corporation
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

#ifndef QGLOBAL_H
# include <QtCore/qglobal.h>
#endif

#ifndef QCOMPILERDETECTION_H
#define QCOMPILERDETECTION_H

/*
   The compiler, must be one of: (Q_CC_x)

     SYM      - Digital Mars C/C++ (used to be Symantec C++)
     MSVC     - Microsoft Visual C/C++, Intel C++ for Windows
     BOR      - Borland/Turbo C++
     WAT      - Watcom C++
     GNU      - GNU C++
     COMEAU   - Comeau C++
     EDG      - Edison Design Group C++
     OC       - CenterLine C++
     SUN      - Forte Developer, or Sun Studio C++
     MIPS     - MIPSpro C++
     DEC      - DEC C++
     HPACC    - HP aC++
     USLC     - SCO OUDK and UDK
     CDS      - Reliant C++
     KAI      - KAI C++
     INTEL    - Intel C++ for Linux, Intel C++ for Windows
     HIGHC    - MetaWare High C/C++
     PGI      - Portland Group C++
     GHS      - Green Hills Optimizing C++ Compilers
     RVCT     - ARM Realview Compiler Suite
     CLANG    - C++ front-end for the LLVM compiler


   Should be sorted most to least authoritative.
*/

/* Symantec C++ is now Digital Mars */
#if defined(__DMC__) || defined(__SC__)
#  define Q_CC_SYM
/* "explicit" semantics implemented in 8.1e but keyword recognized since 7.5 */
#  if defined(__SC__) && __SC__ < 0x750
#    error "Compiler not supported"
#  endif
#  define Q_NO_USING_KEYWORD

#elif defined(_MSC_VER)
#  define Q_CC_MSVC
#  define Q_CC_MSVC_NET
#  define Q_OUTOFLINE_TEMPLATE inline
#  define Q_NO_TEMPLATE_FRIENDS
#  define Q_COMPILER_MANGLES_RETURN_TYPE
#  define Q_FUNC_INFO __FUNCSIG__
#  define Q_ALIGNOF(type) __alignof(type)
#  define Q_DECL_ALIGN(n) __declspec(align(n))
#  define Q_ASSUME_IMPL(expr) __assume(expr)
#  define Q_UNREACHABLE_IMPL() __assume(0)
#  define Q_NORETURN __declspec(noreturn)
#  define Q_DECL_DEPRECATED __declspec(deprecated)
#  define Q_DECL_DEPRECATED_X(text) __declspec(deprecated(text))
#  define Q_DECL_EXPORT __declspec(dllexport)
#  define Q_DECL_IMPORT __declspec(dllimport)
/* Intel C++ disguising as Visual C++: the `using' keyword avoids warnings */
#  if defined(__INTEL_COMPILER)
#    define Q_DECL_VARIABLE_DEPRECATED
#    define Q_CC_INTEL
#  endif

/* only defined for MSVC since that's the only compiler that actually optimizes for this */
/* might get overridden further down when Q_COMPILER_NOEXCEPT is detected */
#  ifdef __cplusplus
#    define Q_DECL_NOTHROW  throw()
#  endif

#elif defined(__BORLANDC__) || defined(__TURBOC__)
#  define Q_CC_BOR
#  define Q_INLINE_TEMPLATE
#  if __BORLANDC__ < 0x502
#    error "Compiler not supported"
#  endif
#  define Q_NO_USING_KEYWORD

#elif defined(__WATCOMC__)
#  define Q_CC_WAT

/* ARM Realview Compiler Suite
   RVCT compiler also defines __EDG__ and __GNUC__ (if --gnu flag is given),
   so check for it before that */
#elif defined(__ARMCC__) || defined(__CC_ARM)
#  define Q_CC_RVCT
/* work-around for missing compiler intrinsics */
#  define __is_empty(X) false
#  define __is_pod(X) false
#  define Q_DECL_DEPRECATED __attribute__ ((__deprecated__))
#  ifdef Q_OS_LINUX
#    define Q_DECL_EXPORT     __attribute__((visibility("default")))
#    define Q_DECL_IMPORT     __attribute__((visibility("default")))
#    define Q_DECL_HIDDEN     __attribute__((visibility("hidden")))
#  else
#    define Q_DECL_EXPORT     __declspec(dllexport)
#    define Q_DECL_IMPORT     __declspec(dllimport)
#  endif

#elif defined(__GNUC__)
#  define Q_CC_GNU
#  define Q_C_CALLBACKS
#  if defined(__MINGW32__)
#    define Q_CC_MINGW
#  endif
#  if defined(__INTEL_COMPILER)
/* Intel C++ also masquerades as GCC */
#    define Q_CC_INTEL
#    define Q_ASSUME_IMPL(expr)  __assume(expr)
#    define Q_UNREACHABLE_IMPL() __builtin_unreachable()
#    if __INTEL_COMPILER >= 1300 && !defined(__APPLE__)
#      define Q_DECL_DEPRECATED_X(text) __attribute__ ((__deprecated__(text)))
#    endif
#  elif defined(__clang__)
/* Clang also masquerades as GCC */
#    define Q_CC_CLANG
#    define Q_ASSUME_IMPL(expr)  if (expr){} else __builtin_unreachable()
#    define Q_UNREACHABLE_IMPL() __builtin_unreachable()
#    if !defined(__has_extension)
#      /* Compatibility with older Clang versions */
#      define __has_extension __has_feature
#    endif
#    if defined(__APPLE__)
     /* Apple/clang specific features */
#      define Q_DECL_CF_RETURNS_RETAINED __attribute__((cf_returns_retained))
#      ifdef __OBJC__
#        define Q_DECL_NS_RETURNS_AUTORELEASED __attribute__((ns_returns_autoreleased))
#      endif
#    endif
#  else
/* Plain GCC */
#    if (__GNUC__ * 100 + __GNUC_MINOR__) >= 405
#      define Q_ASSUME_IMPL(expr)  if (expr){} else __builtin_unreachable()
#      define Q_UNREACHABLE_IMPL() __builtin_unreachable()
#      define Q_DECL_DEPRECATED_X(text) __attribute__ ((__deprecated__(text)))
#    endif
#  endif

#  ifdef Q_OS_WIN
#    define Q_DECL_EXPORT     __declspec(dllexport)
#    define Q_DECL_IMPORT     __declspec(dllimport)
#  elif defined(QT_VISIBILITY_AVAILABLE)
#    define Q_DECL_EXPORT     __attribute__((visibility("default")))
#    define Q_DECL_IMPORT     __attribute__((visibility("default")))
#    define Q_DECL_HIDDEN     __attribute__((visibility("hidden")))
#  endif

#  define Q_FUNC_INFO       __PRETTY_FUNCTION__
#  define Q_ALIGNOF(type)   __alignof__(type)
#  define Q_TYPEOF(expr)    __typeof__(expr)
#  define Q_DECL_DEPRECATED __attribute__ ((__deprecated__))
#  define Q_DECL_ALIGN(n)   __attribute__((__aligned__(n)))
#  define Q_DECL_UNUSED     __attribute__((__unused__))
#  define Q_LIKELY(expr)    __builtin_expect(!!(expr), true)
#  define Q_UNLIKELY(expr)  __builtin_expect(!!(expr), false)
#  define Q_NORETURN        __attribute__((__noreturn__))
#  define Q_REQUIRED_RESULT __attribute__ ((__warn_unused_result__))
#  if !defined(QT_MOC_CPP)
#    define Q_PACKED __attribute__ ((__packed__))
#    ifndef __ARM_EABI__
#      define QT_NO_ARM_EABI
#    endif
#  endif
#  if (__GNUC__ * 100 + __GNUC_MINOR__) >= 403 && !defined(Q_CC_CLANG)
#      define Q_ALLOC_SIZE(x) __attribute__((alloc_size(x)))
#  endif

/* IBM compiler versions are a bit messy. There are actually two products:
   the C product, and the C++ product. The C++ compiler is always packaged
   with the latest version of the C compiler. Version numbers do not always
   match. This little table (I'm not sure it's accurate) should be helpful:

   C++ product                C product

   C Set 3.1                  C Compiler 3.0
   ...                        ...
   C++ Compiler 3.6.6         C Compiler 4.3
   ...                        ...
   Visual Age C++ 4.0         ...
   ...                        ...
   Visual Age C++ 5.0         C Compiler 5.0
   ...                        ...
   Visual Age C++ 6.0         C Compiler 6.0

   Now:
   __xlC__    is the version of the C compiler in hexadecimal notation
              is only an approximation of the C++ compiler version
   __IBMCPP__ is the version of the C++ compiler in decimal notation
              but it is not defined on older compilers like C Set 3.1 */
#elif defined(__xlC__)
#  define Q_CC_XLC
#  define Q_FULL_TEMPLATE_INSTANTIATION
#  if __xlC__ < 0x400
#    error "Compiler not supported"
#  elif __xlC__ >= 0x0600
#    define Q_ALIGNOF(type)     __alignof__(type)
#    define Q_TYPEOF(expr)      __typeof__(expr)
#    define Q_DECL_ALIGN(n)     __attribute__((__aligned__(n)))
#    define Q_PACKED            __attribute__((__packed__))
#  endif

/* Older versions of DEC C++ do not define __EDG__ or __EDG - observed
   on DEC C++ V5.5-004. New versions do define  __EDG__ - observed on
   Compaq C++ V6.3-002.
   This compiler is different enough from other EDG compilers to handle
   it separately anyway. */
#elif defined(__DECCXX) || defined(__DECC)
#  define Q_CC_DEC
/* Compaq C++ V6 compilers are EDG-based but I'm not sure about older
   DEC C++ V5 compilers. */
#  if defined(__EDG__)
#    define Q_CC_EDG
#  endif
/* Compaq has disabled EDG's _BOOL macro and uses _BOOL_EXISTS instead
   - observed on Compaq C++ V6.3-002.
   In any case versions prior to Compaq C++ V6.0-005 do not have bool. */
#  if !defined(_BOOL_EXISTS)
#    error "Compiler not supported"
#  endif
/* Spurious (?) error messages observed on Compaq C++ V6.5-014. */
#  define Q_NO_USING_KEYWORD
/* Apply to all versions prior to Compaq C++ V6.0-000 - observed on
   DEC C++ V5.5-004. */
#  if __DECCXX_VER < 60060000
#    define Q_BROKEN_TEMPLATE_SPECIALIZATION
#  endif
/* avoid undefined symbol problems with out-of-line template members */
#  define Q_OUTOFLINE_TEMPLATE inline

/* The Portland Group C++ compiler is based on EDG and does define __EDG__
   but the C compiler does not */
#elif defined(__PGI)
#  define Q_CC_PGI
#  if defined(__EDG__)
#    define Q_CC_EDG
#  endif

/* Compilers with EDG front end are similar. To detect them we test:
   __EDG documented by SGI, observed on MIPSpro 7.3.1.1 and KAI C++ 4.0b
   __EDG__ documented in EDG online docs, observed on Compaq C++ V6.3-002
   and PGI C++ 5.2-4 */
#elif !defined(Q_OS_HPUX) && (defined(__EDG) || defined(__EDG__))
#  define Q_CC_EDG
/* From the EDG documentation (does not seem to apply to Compaq C++):
   _BOOL
        Defined in C++ mode when bool is a keyword. The name of this
        predefined macro is specified by a configuration flag. _BOOL
        is the default.
   __BOOL_DEFINED
        Defined in Microsoft C++ mode when bool is a keyword. */
#  if !defined(_BOOL) && !defined(__BOOL_DEFINED)
#    error "Compiler not supported"
#  endif

/* The Comeau compiler is based on EDG and does define __EDG__ */
#  if defined(__COMO__)
#    define Q_CC_COMEAU
#    define Q_C_CALLBACKS

/* The `using' keyword was introduced to avoid KAI C++ warnings
   but it's now causing KAI C++ errors instead. The standard is
   unclear about the use of this keyword, and in practice every
   compiler is using its own set of rules. Forget it. */
#  elif defined(__KCC)
#    define Q_CC_KAI
#    define Q_NO_USING_KEYWORD

/* Using the `using' keyword avoids Intel C++ for Linux warnings */
#  elif defined(__INTEL_COMPILER)
#    define Q_CC_INTEL

/* Uses CFront, make sure to read the manual how to tweak templates. */
#  elif defined(__ghs)
#    define Q_CC_GHS

#  elif defined(__DCC__)
#    define Q_CC_DIAB
#    if !defined(__bool)
#      error "Compiler not supported"
#    endif

/* The UnixWare 7 UDK compiler is based on EDG and does define __EDG__ */
#  elif defined(__USLC__) && defined(__SCO_VERSION__)
#    define Q_CC_USLC
/* The latest UDK 7.1.1b does not need this, but previous versions do */
#    if !defined(__SCO_VERSION__) || (__SCO_VERSION__ < 302200010)
#      define Q_OUTOFLINE_TEMPLATE inline
#    endif
#    define Q_NO_USING_KEYWORD /* ### check "using" status */

/* Never tested! */
#  elif defined(CENTERLINE_CLPP) || defined(OBJECTCENTER)
#    define Q_CC_OC
#    define Q_NO_USING_KEYWORD

/* CDS++ defines __EDG__ although this is not documented in the Reliant
   documentation. It also follows conventions like _BOOL and this documented */
#  elif defined(sinix)
#    define Q_CC_CDS
#    define Q_NO_USING_KEYWORD

/* The MIPSpro compiler defines __EDG */
#  elif defined(__sgi)
#    define Q_CC_MIPS
#    define Q_NO_USING_KEYWORD /* ### check "using" status */
#    define Q_NO_TEMPLATE_FRIENDS
#    if defined(_COMPILER_VERSION) && (_COMPILER_VERSION >= 740)
#      define Q_OUTOFLINE_TEMPLATE inline
#      pragma set woff 3624,3625,3649 /* turn off some harmless warnings */
#    endif
#  endif

/* VxWorks' DIAB toolchain has an additional EDG type C++ compiler
   (see __DCC__ above). This one is for C mode files (__EDG is not defined) */
#elif defined(_DIAB_TOOL)
#  define Q_CC_DIAB
#  define Q_FUNC_INFO       __PRETTY_FUNCTION__

/* Never tested! */
#elif defined(__HIGHC__)
#  define Q_CC_HIGHC

#elif defined(__SUNPRO_CC) || defined(__SUNPRO_C)
#  define Q_CC_SUN
#  define Q_COMPILER_MANGLES_RETURN_TYPE
/* 5.0 compiler or better
    'bool' is enabled by default but can be disabled using -features=nobool
    in which case _BOOL is not defined
        this is the default in 4.2 compatibility mode triggered by -compat=4 */
#  if __SUNPRO_CC >= 0x500
#    define QT_NO_TEMPLATE_TEMPLATE_PARAMETERS
   /* see http://developers.sun.com/sunstudio/support/Ccompare.html */
#    if __SUNPRO_CC >= 0x590
#      define Q_ALIGNOF(type)   __alignof__(type)
#      define Q_TYPEOF(expr)    __typeof__(expr)
#      define Q_DECL_ALIGN(n)   __attribute__((__aligned__(n)))
#    endif
#    if __SUNPRO_CC >= 0x550
#      define Q_DECL_EXPORT     __global
#    endif
#    if __SUNPRO_CC < 0x5a0
#      define Q_NO_TEMPLATE_FRIENDS
#    endif
#    if !defined(_BOOL)
#      error "Compiler not supported"
#    endif
#    if defined(__SUNPRO_CC_COMPAT) && (__SUNPRO_CC_COMPAT <= 4)
#      define Q_NO_USING_KEYWORD
#    endif
#    define Q_C_CALLBACKS
/* 4.2 compiler or older */
#  else
#    error "Compiler not supported"
#  endif

/* CDS++ does not seem to define __EDG__ or __EDG according to Reliant
   documentation but nevertheless uses EDG conventions like _BOOL */
#elif defined(sinix)
#  define Q_CC_EDG
#  define Q_CC_CDS
#  if !defined(_BOOL)
#    error "Compiler not supported"
#  endif
#  define Q_BROKEN_TEMPLATE_SPECIALIZATION

#elif defined(Q_OS_HPUX)
/* __HP_aCC was not defined in first aCC releases */
#  if defined(__HP_aCC) || __cplusplus >= 199707L
#    define Q_NO_TEMPLATE_FRIENDS
#    define Q_CC_HPACC
#    define Q_FUNC_INFO         __PRETTY_FUNCTION__
#    if __HP_aCC-0 < 060000
#      define QT_NO_TEMPLATE_TEMPLATE_PARAMETERS
#      define Q_DECL_EXPORT     __declspec(dllexport)
#      define Q_DECL_IMPORT     __declspec(dllimport)
#    endif
#    if __HP_aCC-0 >= 061200
#      define Q_DECL_ALIGN(n) __attribute__((aligned(n)))
#    endif
#    if __HP_aCC-0 >= 062000
#      define Q_DECL_EXPORT     __attribute__((visibility("default")))
#      define Q_DECL_HIDDEN     __attribute__((visibility("hidden")))
#      define Q_DECL_IMPORT     Q_DECL_EXPORT
#    endif
#  else
#    error "Compiler not supported"
#  endif
#  define Q_NO_USING_KEYWORD /* ### check "using" status */

#else
#  error "Qt has not been tested with this compiler - see http://www.qt-project.org/"
#endif

/*
 * C++11 support
 *
 *  Paper           Macro
 *  N2341           Q_COMPILER_ALIGNAS
 *  N2341           Q_COMPILER_ALIGNOF
 *  N2427           Q_COMPILER_ATOMICS
 *  N2761           Q_COMPILER_ATTRIBUTES
 *  N2541           Q_COMPILER_AUTO_FUNCTION
 *  N1984 N2546     Q_COMPILER_AUTO_TYPE
 *  N2437           Q_COMPILER_CLASS_ENUM
 *  N2235           Q_COMPILER_CONSTEXPR
 *  N2343 N3276     Q_COMPILER_DECLTYPE
 *  N2346           Q_COMPILER_DEFAULT_MEMBERS
 *  N2346           Q_COMPILER_DELETE_MEMBERS
 *  N1986           Q_COMPILER_DELEGATING_CONSTRUCTORS
 *  N2437           Q_COMPILER_EXPLICIT_CONVERSIONS
 *  N3206 N3272     Q_COMPILER_EXPLICIT_OVERRIDES   (v0.9 and above only)
 *  N1987           Q_COMPILER_EXTERN_TEMPLATES
 *  N2540           Q_COMPILER_INHERITING_CONSTRUCTORS
 *  N2672           Q_COMPILER_INITIALIZER_LISTS
 *  N2658 N2927     Q_COMPILER_LAMBDA   (v1.0 and above only)
 *  N2756           Q_COMPILER_NONSTATIC_MEMBER_INIT
 *  N2855 N3050     Q_COMPILER_NOEXCEPT
 *  N2431           Q_COMPILER_NULLPTR
 *  N2930           Q_COMPILER_RANGE_FOR
 *  N2442           Q_COMPILER_RAW_STRINGS
 *  N2439           Q_COMPILER_REF_QUALIFIERS
 *  N2118 N2844 N3053 Q_COMPILER_RVALUE_REFS   (Note: GCC 4.3 implements only the oldest)
 *  N1720           Q_COMPILER_STATIC_ASSERT
 *  N2258           Q_COMPILER_TEMPLATE_ALIAS
 *  N2659           Q_COMPILER_THREAD_LOCAL
 *  N2765           Q_COMPILER_UDL
 *  N2442           Q_COMPILER_UNICODE_STRINGS
 *  N2640           Q_COMPILER_UNIFORM_INIT
 *  N2544           Q_COMPILER_UNRESTRICTED_UNIONS
 *  N1653           Q_COMPILER_VARIADIC_MACROS
 *  N2242 N2555     Q_COMPILER_VARIADIC_TEMPLATES
 *
 * C++1y proposed features
 *
 *  N3472           Q_COMPILER_BINARY_LITERALS
 *  N3649           Q_COMPILER_GENERIC_LAMBDA
 *  N3638           Q_COMPILER_LAMBDA_CAPTURES
 *  N3652           Q_COMPILER_RELAXED_CONSTEXPR_FUNCTIONS
 *  N3386 N3638     Q_COMPILER_RETURN_TYPE_DEDUCTION
 *  N3651           Q_COMPILER_VARIABLE_TEMPLATES
 *
 * C++14 Technical Specifications / C++17:
 *  N3639           Q_COMPILER_VLA  (see also Q_COMPILER_RESTRICTED_VLA)
 *
 */

#ifdef Q_CC_INTEL
#  define Q_COMPILER_RESTRICTED_VLA
#  define Q_COMPILER_VARIADIC_MACROS // C++11 feature supported as an extension in other modes, too
#  if __INTEL_COMPILER < 1200
#    define Q_NO_TEMPLATE_FRIENDS
#  endif
#  if __INTEL_COMPILER >= 1310 && !defined(_WIN32)
//    ICC supports C++14 binary literals in C, C++98, and C++11 modes
//    at least since 13.1, but I can't test further back
#     define Q_COMPILER_BINARY_LITERALS
#  endif
#  if __cplusplus >= 201103L
#    if __INTEL_COMPILER >= 1200
#      define Q_COMPILER_AUTO_TYPE
#      define Q_COMPILER_CLASS_ENUM
#      define Q_COMPILER_DECLTYPE
#      define Q_COMPILER_DEFAULT_MEMBERS
#      define Q_COMPILER_DELETE_MEMBERS
#      define Q_COMPILER_EXTERN_TEMPLATES
#      define Q_COMPILER_LAMBDA
#      define Q_COMPILER_RVALUE_REFS
#      define Q_COMPILER_STATIC_ASSERT
#      define Q_COMPILER_VARIADIC_MACROS
#    endif
#    if __INTEL_COMPILER >= 1210
#      define Q_COMPILER_ATTRIBUTES
#      define Q_COMPILER_AUTO_FUNCTION
#      define Q_COMPILER_NULLPTR
#      define Q_COMPILER_TEMPLATE_ALIAS
#      define Q_COMPILER_UNICODE_STRINGS
#      define Q_COMPILER_VARIADIC_TEMPLATES
#    endif
#    if __INTEL_COMPILER >= 1300
#      define Q_COMPILER_ATOMICS
//       constexpr support is only partial
//#      define Q_COMPILER_CONSTEXPR
#      define Q_COMPILER_INITIALIZER_LISTS
#      define Q_COMPILER_UNIFORM_INIT
#      define Q_COMPILER_NOEXCEPT
#    endif
#    if __INTEL_COMPILER >= 1400
#      define Q_COMPILER_CONSTEXPR
#      define Q_COMPILER_DELEGATING_CONSTRUCTORS
#      define Q_COMPILER_EXPLICIT_OVERRIDES
#      define Q_COMPILER_NONSTATIC_MEMBER_INIT
#      define Q_COMPILER_RAW_STRINGS
#      define Q_COMPILER_REF_QUALIFIERS
#      define Q_COMPILER_UNRESTRICTED_UNIONS
#    endif
#  endif
#endif

#if defined(Q_CC_CLANG) && !defined(Q_CC_INTEL)
/* General C++ features */
#  define Q_COMPILER_RESTRICTED_VLA
#  if !__has_feature(cxx_exceptions)
#    ifndef QT_NO_EXCEPTIONS
#      define QT_NO_EXCEPTIONS
#    endif
#  endif
#  if !__has_feature(cxx_rtti)
#    define QT_NO_RTTI
#  endif
#  if __has_feature(attribute_deprecated_with_message)
#    define Q_DECL_DEPRECATED_X(text) __attribute__ ((__deprecated__(text)))
#  endif

// Clang supports binary literals in C, C++98 and C++11 modes
// It's been supported "since the dawn of time itself" (cf. commit 179883)
#  if __has_extension(cxx_binary_literals)
#    define Q_COMPILER_BINARY_LITERALS
#  endif

// Variadic macros are supported for gnu++98, c++11, c99 ... since 2.9
#  if ((__clang_major__ * 100) + __clang_minor__) >= 209
#    if !defined(__STRICT_ANSI__) || defined(__GXX_EXPERIMENTAL_CXX0X__) \
      || (defined(__cplusplus) && (__cplusplus >= 201103L)) \
      || (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L))
#      define Q_COMPILER_VARIADIC_MACROS
#    endif
#  endif

/* C++11 features, see http://clang.llvm.org/cxx_status.html */
#  if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
    /* Detect C++ features using __has_feature(), see http://clang.llvm.org/docs/LanguageExtensions.html#cxx11 */
#    if __has_feature(cxx_alignas)
#      define Q_COMPILER_ALIGNAS
#      define Q_COMPILER_ALIGNOF
#    endif
#    if 0 /* not implemented in clang yet */
#     define Q_COMPILER_ATOMICS
#    endif
#    if __has_feature(cxx_attributes)
#      define Q_COMPILER_ATTRIBUTES
#    endif
#    if __has_feature(cxx_auto_type)
#      define Q_COMPILER_AUTO_FUNCTION
#      define Q_COMPILER_AUTO_TYPE
#    endif
#    if __has_feature(cxx_strong_enums)
#      define Q_COMPILER_CLASS_ENUM
#    endif
#    if __has_feature(cxx_constexpr)
#      define Q_COMPILER_CONSTEXPR
#    endif
#    if __has_feature(cxx_decltype) /* && __has_feature(cxx_decltype_incomplete_return_types) */
#      define Q_COMPILER_DECLTYPE
#    endif
#    if __has_feature(cxx_defaulted_functions)
#      define Q_COMPILER_DEFAULT_MEMBERS
#    endif
#    if __has_feature(cxx_deleted_functions)
#      define Q_COMPILER_DELETE_MEMBERS
#    endif
#    if __has_feature(cxx_delegating_constructors)
#      define Q_COMPILER_DELEGATING_CONSTRUCTORS
#    endif
#    if __has_feature(cxx_explicit_conversions)
#      define Q_COMPILER_EXPLICIT_CONVERSIONS
#    endif
#    if __has_feature(cxx_override_control)
#      define Q_COMPILER_EXPLICIT_OVERRIDES
#    endif
#    if __has_feature(cxx_inheriting_constructors)
#      define Q_COMPILER_INHERITING_CONSTRUCTORS
#    endif
#    if __has_feature(cxx_generalized_initializers)
#      define Q_COMPILER_INITIALIZER_LISTS
#      define Q_COMPILER_UNIFORM_INIT /* both covered by this feature macro, according to docs */
#    endif
#    if __has_feature(cxx_lambdas)
#      define Q_COMPILER_LAMBDA
#    endif
#    if __has_feature(cxx_noexcept)
#      define Q_COMPILER_NOEXCEPT
#    endif
#    if __has_feature(cxx_nonstatic_member_init)
#      define Q_COMPILER_NONSTATIC_MEMBER_INIT
#    endif
#    if __has_feature(cxx_nullptr)
#      define Q_COMPILER_NULLPTR
#    endif
#    if __has_feature(cxx_range_for)
#      define Q_COMPILER_RANGE_FOR
#    endif
#    if __has_feature(cxx_raw_string_literals)
#      define Q_COMPILER_RAW_STRINGS
#    endif
#    if __has_feature(cxx_reference_qualified_functions)
#      define Q_COMPILER_REF_QUALIFIERS
#    endif
#    if __has_feature(cxx_rvalue_references)
#      define Q_COMPILER_RVALUE_REFS
#    endif
#    if __has_feature(cxx_static_assert)
#      define Q_COMPILER_STATIC_ASSERT
#    endif
#    if __has_feature(cxx_alias_templates)
#      define Q_COMPILER_TEMPLATE_ALIAS
#    endif
#    if __has_feature(cxx_thread_local)
#      define Q_COMPILER_THREAD_LOCAL
#    endif
#    if __has_feature(cxx_user_literals)
#      define Q_COMPILER_UDL
#    endif
#    if __has_feature(cxx_unicode_literals)
#      define Q_COMPILER_UNICODE_STRINGS
#    endif
#    if __has_feature(cxx_unrestricted_unions)
#      define Q_COMPILER_UNRESTRICTED_UNIONS
#    endif
#    if __has_feature(cxx_variadic_templates)
#      define Q_COMPILER_VARIADIC_TEMPLATES
#    endif
    /* Features that have no __has_feature() check */
#    if ((__clang_major__ * 100) + __clang_minor__) >= 209 /* since clang 2.9 */
#      define Q_COMPILER_EXTERN_TEMPLATES
#    endif
#  endif

/* C++1y features, see http://clang.llvm.org/cxx_status.html and
 * http://clang.llvm.org/docs/LanguageExtensions.html#checks-for-standard-language-features */
#  if __cplusplus > 201103L
//#    if __has_feature(cxx_binary_literals)
//#      define Q_COMPILER_BINARY_LITERALS  // see above
//#    endif
#    if __has_feature(cxx_generic_lambda)
#      define Q_COMPILER_GENERIC_LAMBDA
#    endif
#    if __has_feature(cxx_init_capture)
#      define Q_COMPILER_LAMBDA_CAPTURES
#    endif
#    if __has_feature(cxx_relaxed_constexpr)
#      define Q_COMPILER_RELAXED_CONSTEXPR_FUNCTIONS
#    endif
#    if __has_feature(cxx_decltype_auto) && __has_feature(cxx_return_type_deduction)
#      define Q_COMPILER_RETURN_TYPE_DEDUCTION
#    endif
#    if __has_feature(cxx_variable_templates)
#      define Q_COMPILER_VARIABLE_TEMPLATES
#    endif
#    if __has_feature(cxx_runtime_array)
#      define Q_COMPILER_VLA
#    endif
#  endif
#endif // Q_CC_CLANG

#if defined(Q_CC_GNU) && !defined(Q_CC_INTEL) && !defined(Q_CC_CLANG)
#  define Q_COMPILER_RESTRICTED_VLA
#  if (__GNUC__ * 100 + __GNUC_MINOR__) >= 403
//   GCC supports binary literals in C, C++98 and C++11 modes
#    define Q_COMPILER_BINARY_LITERALS
#  endif
#  if !defined(__STRICT_ANSI__) || defined(__GXX_EXPERIMENTAL_CXX0X__) \
    || (defined(__cplusplus) && (__cplusplus >= 201103L)) \
    || (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L))
     // Variadic macros are supported for gnu++98, c++11, C99 ... since forever (gcc 2.97)
#    define Q_COMPILER_VARIADIC_MACROS
#  endif
#  if defined(__GXX_EXPERIMENTAL_CXX0X__) || __cplusplus >= 201103L
#    if (__GNUC__ * 100 + __GNUC_MINOR__) >= 403
       /* C++11 features supported in GCC 4.3: */
#      define Q_COMPILER_DECLTYPE
#      define Q_COMPILER_RVALUE_REFS
#      define Q_COMPILER_STATIC_ASSERT
#    endif
#    if (__GNUC__ * 100 + __GNUC_MINOR__) >= 404
       /* C++11 features supported in GCC 4.4: */
#      define Q_COMPILER_AUTO_FUNCTION
#      define Q_COMPILER_AUTO_TYPE
#      define Q_COMPILER_CLASS_ENUM
#      define Q_COMPILER_DEFAULT_MEMBERS
#      define Q_COMPILER_DELETE_MEMBERS
#      define Q_COMPILER_EXTERN_TEMPLATES
#      define Q_COMPILER_INITIALIZER_LISTS
#      define Q_COMPILER_UNIFORM_INIT
#      define Q_COMPILER_UNICODE_STRINGS
#      define Q_COMPILER_VARIADIC_TEMPLATES
#    endif
#    if (__GNUC__ * 100 + __GNUC_MINOR__) >= 405
       /* C++11 features supported in GCC 4.5: */
#      define Q_COMPILER_EXPLICIT_CONVERSIONS
#      define Q_COMPILER_LAMBDA
#      define Q_COMPILER_RAW_STRINGS
#    endif
#    if (__GNUC__ * 100 + __GNUC_MINOR__) >= 406
       /* C++11 features supported in GCC 4.6: */
#      define Q_COMPILER_CONSTEXPR
#      define Q_COMPILER_NULLPTR
#      define Q_COMPILER_UNRESTRICTED_UNIONS
#      define Q_COMPILER_RANGE_FOR
#    endif
#    if (__GNUC__ * 100 + __GNUC_MINOR__) >= 407
       /* GCC 4.4 implemented <atomic> and std::atomic using its old intrinsics.
        * However, the implementation is incomplete for most platforms until GCC 4.7:
        * instead, std::atomic would use an external lock. Since we need an std::atomic
        * that is behavior-compatible with QBasicAtomic, we only enable it here */
#      define Q_COMPILER_ATOMICS
       /* GCC 4.6.x has problems dealing with noexcept expressions,
        * so turn the feature on for 4.7 and above, only */
#      define Q_COMPILER_NOEXCEPT
       /* C++11 features supported in GCC 4.7: */
#      define Q_COMPILER_NONSTATIC_MEMBER_INIT
#      define Q_COMPILER_DELEGATING_CONSTRUCTORS
#      define Q_COMPILER_EXPLICIT_OVERRIDES
#      define Q_COMPILER_TEMPLATE_ALIAS
#      define Q_COMPILER_UDL
#    endif
#    if (__GNUC__ * 100 + __GNUC_MINOR__) >= 408
#      define Q_COMPILER_ATTRIBUTES
#      define Q_COMPILER_ALIGNAS
#      define Q_COMPILER_ALIGNOF
#      define Q_COMPILER_INHERITING_CONSTRUCTORS
#      define Q_COMPILER_THREAD_LOCAL
#      if (__GNUC__ * 100 + __GNUC_MINOR__) > 408 || __GNUC_PATCHLEVEL__ >= 1
#         define Q_COMPILER_REF_QUALIFIERS
#      endif
#    endif
     /* C++11 features are complete as of GCC 4.8.1 */
#  endif
#  if __cplusplus > 201103L
#    if (__GNUC__ * 100 + __GNUC_MINOR__) >= 409
     /* C++1y features in GCC 4.9 */
//#    define Q_COMPILER_BINARY_LITERALS   // already supported since GCC 4.3 as an extension
#      define Q_COMPILER_LAMBDA_CAPTURES
#      define Q_COMPILER_RETURN_TYPE_DEDUCTION
#    endif
#  endif
#endif

#if defined(Q_CC_MSVC) && !defined(Q_CC_INTEL)
#    if _MSC_VER >= 1400
       /* C++11 features supported in VC8 = VC2005: */
#      define Q_COMPILER_VARIADIC_MACROS

#      ifndef __cplusplus_cli
       /* 2005 supports the override and final contextual keywords, in
        the same positions as the C++11 variants, but 'final' is
        called 'sealed' instead:
        http://msdn.microsoft.com/en-us/library/0w2w91tf%28v=vs.80%29.aspx
        The behavior is slightly different in C++/CLI, which requires the
        "virtual" keyword to be present too, so don't define for that.
        So don't define Q_COMPILER_EXPLICIT_OVERRIDES (since it's not
        the same as the C++11 version), but define the Q_DECL_* flags
        accordingly: */
#      define Q_DECL_OVERRIDE override
#      define Q_DECL_FINAL sealed
#      endif
#    endif
#    if _MSC_VER >= 1600
       /* C++11 features supported in VC10 = VC2010: */
#      define Q_COMPILER_AUTO_FUNCTION
#      define Q_COMPILER_AUTO_TYPE
#      define Q_COMPILER_LAMBDA
#      define Q_COMPILER_DECLTYPE
#      define Q_COMPILER_RVALUE_REFS
#      define Q_COMPILER_STATIC_ASSERT
//  MSVC's library has std::initializer_list, but the compiler does not support the braces initialization
//#      define Q_COMPILER_INITIALIZER_LISTS
//#      define Q_COMPILER_UNIFORM_INIT
#    endif
#    if _MSC_VER >= 1700
       /* C++11 features supported in VC11 = VC2012: */
#       undef Q_DECL_OVERRIDE               /* undo 2005/2008 settings... */
#       undef Q_DECL_FINAL                  /* undo 2005/2008 settings... */
#      define Q_COMPILER_EXPLICIT_OVERRIDES /* ...and use std C++11 now   */
#      define Q_COMPILER_RANGE_FOR
#      define Q_COMPILER_CLASS_ENUM
#      define Q_COMPILER_ATOMICS
#    endif /* VC 11 */
#    if _MSC_VER >= 1800
       /* C++11 features in VC12 = VC2013 */
#      define Q_COMPILER_DEFAULT_MEMBERS
#      define Q_COMPILER_DELETE_MEMBERS
#      define Q_COMPILER_DELEGATING_CONSTRUCTORS
#      define Q_COMPILER_EXPLICIT_CONVERSIONS
#      define Q_COMPILER_NONSTATIC_MEMBER_INIT
// implemented, but nested initialization fails (eg tst_qvector): http://connect.microsoft.com/VisualStudio/feedback/details/800364/initializer-list-calls-object-destructor-twice
//      #define Q_COMPILER_INITIALIZER_LISTS
// implemented in principle, but has a bug that makes it unusable: http://connect.microsoft.com/VisualStudio/feedback/details/802058/c-11-unified-initialization-fails-with-c-style-arrays
//      #define Q_COMPILER_UNIFORM_INIT
#      define Q_COMPILER_RAW_STRINGS
#      define Q_COMPILER_TEMPLATE_ALIAS
#      define Q_COMPILER_VARIADIC_TEMPLATES
#    endif /* VC 12 */
#    if _MSC_FULL_VER >= 180030324 // VC 12 SP 2 RC
#      define Q_COMPILER_INITIALIZER_LISTS
#    endif /* VC 12 SP 2 RC */

#endif /* Q_CC_MSVC */

#ifdef __cplusplus
# include <utility>
# if defined(Q_OS_QNX)
#  if defined(_YVALS) || defined(_LIBCPP_VER)
// QNX: libcpp (Dinkumware-based) doesn't have the <initializer_list>
// header, so the feature is useless, even if the compiler supports
// it. Disable.
#    undef Q_COMPILER_INITIALIZER_LISTS
// That libcpp doesn't have std::move either, so disable everything
// related to rvalue refs.
#    undef Q_COMPILER_RVALUE_REFS
#    undef Q_COMPILER_REF_QUALIFIERS
#  endif
# endif // Q_OS_QNX
# if (defined(Q_CC_CLANG) || defined(Q_CC_INTEL)) && defined(Q_OS_MAC) && defined(__GNUC_LIBSTD__) \
    && ((__GNUC_LIBSTD__-0) * 100 + __GNUC_LIBSTD_MINOR__-0 <= 402)
// Mac OS X: Apple has not updated libstdc++ since 2007, which means it does not have
// <initializer_list> or std::move. Let's disable these features
#  undef Q_COMPILER_INITIALIZER_LISTS
#  undef Q_COMPILER_RVALUE_REFS
#  undef Q_COMPILER_REF_QUALIFIERS
# endif
#endif

/*
 * C++11 keywords and expressions
 */
#ifdef Q_COMPILER_NULLPTR
# define Q_NULLPTR         nullptr
#else
# define Q_NULLPTR         NULL
#endif

#ifdef Q_COMPILER_DEFAULT_MEMBERS
#  define Q_DECL_EQ_DEFAULT = default
#else
#  define Q_DECL_EQ_DEFAULT
#endif

#ifdef Q_COMPILER_DELETE_MEMBERS
# define Q_DECL_EQ_DELETE = delete
#else
# define Q_DECL_EQ_DELETE
#endif

// Don't break code that is already using Q_COMPILER_DEFAULT_DELETE_MEMBERS
#if defined(Q_COMPILER_DEFAULT_MEMBERS) && defined(Q_COMPILER_DELETE_MEMBERS)
#  define Q_COMPILER_DEFAULT_DELETE_MEMBERS
#endif

#ifdef Q_COMPILER_CONSTEXPR
# define Q_DECL_CONSTEXPR constexpr
# define Q_CONSTEXPR constexpr
#else
# define Q_DECL_CONSTEXPR
# define Q_CONSTEXPR const
#endif

#ifdef Q_COMPILER_EXPLICIT_OVERRIDES
# define Q_DECL_OVERRIDE override
# define Q_DECL_FINAL final
#else
# ifndef Q_DECL_OVERRIDE
#  define Q_DECL_OVERRIDE
# endif
# ifndef Q_DECL_FINAL
#  define Q_DECL_FINAL
# endif
#endif

#ifdef Q_COMPILER_NOEXCEPT
# define Q_DECL_NOEXCEPT noexcept
# define Q_DECL_NOEXCEPT_EXPR(x) noexcept(x)
# ifdef Q_DECL_NOTHROW
#  undef Q_DECL_NOTHROW /* override with C++11 noexcept if available */
# endif
#else
# define Q_DECL_NOEXCEPT
# define Q_DECL_NOEXCEPT_EXPR(x)
#endif
#ifndef Q_DECL_NOTHROW
# define Q_DECL_NOTHROW Q_DECL_NOEXCEPT
#endif

#if defined(Q_COMPILER_ALIGNOF) && !defined(Q_ALIGNOF)
#  define Q_ALIGNOF(x)  alignof(x)
#endif

/*
 * Fallback macros to certain compiler features
 */

#ifndef Q_NORETURN
# define Q_NORETURN
#endif
#ifndef Q_LIKELY
#  define Q_LIKELY(x) (x)
#endif
#ifndef Q_UNLIKELY
#  define Q_UNLIKELY(x) (x)
#endif
#ifndef Q_ASSUME_IMPL
#  define Q_ASSUME_IMPL(expr) qt_noop()
#endif
#ifndef Q_UNREACHABLE_IMPL
#  define Q_UNREACHABLE_IMPL() qt_noop()
#endif
#ifndef Q_ALLOC_SIZE
#  define Q_ALLOC_SIZE(x)
#endif
#ifndef Q_REQUIRED_RESULT
#  define Q_REQUIRED_RESULT
#endif
#ifndef Q_DECL_DEPRECATED
#  define Q_DECL_DEPRECATED
#endif
#ifndef Q_DECL_VARIABLE_DEPRECATED
#  define Q_DECL_VARIABLE_DEPRECATED Q_DECL_DEPRECATED
#endif
#ifndef Q_DECL_DEPRECATED_X
#  define Q_DECL_DEPRECATED_X(text) Q_DECL_DEPRECATED
#endif
#ifndef Q_DECL_EXPORT
#  define Q_DECL_EXPORT
#endif
#ifndef Q_DECL_IMPORT
#  define Q_DECL_IMPORT
#endif
#ifndef Q_DECL_HIDDEN
#  define Q_DECL_HIDDEN
#endif
#ifndef Q_DECL_UNUSED
#  define Q_DECL_UNUSED
#endif
#ifndef Q_FUNC_INFO
#  if defined(Q_OS_SOLARIS) || defined(Q_CC_XLC)
#    define Q_FUNC_INFO __FILE__ "(line number unavailable)"
#  else
#    define Q_FUNC_INFO __FILE__ ":" QT_STRINGIFY(__LINE__)
#  endif
#endif
#ifndef Q_DECL_CF_RETURNS_RETAINED
#  define Q_DECL_CF_RETURNS_RETAINED
#endif
#ifndef Q_DECL_NS_RETURNS_AUTORELEASED
#  define Q_DECL_NS_RETURNS_AUTORELEASED
#endif

/*
   Workaround for static const members on MSVC++.
*/

#if defined(Q_CC_MSVC)
#  define QT_STATIC_CONST static
#  define QT_STATIC_CONST_IMPL
#else
#  define QT_STATIC_CONST static const
#  define QT_STATIC_CONST_IMPL const
#endif

/*
   Proper for-scoping in MIPSpro CC
*/
#ifndef QT_NO_KEYWORDS
#  if defined(Q_CC_MIPS) || (defined(Q_CC_HPACC) && defined(__ia64))
#    define for if (0) {} else for
#  endif
#endif

#ifdef Q_COMPILER_RVALUE_REFS
#define qMove(x) std::move(x)
#else
#define qMove(x) (x)
#endif

#define Q_UNREACHABLE() \
    do {\
        Q_ASSERT_X(false, "Q_UNREACHABLE()", "Q_UNREACHABLE was reached");\
        Q_UNREACHABLE_IMPL();\
    } while (0)

#define Q_ASSUME(Expr) \
    do {\
        const bool valueOfExpression = Expr;\
        Q_ASSERT_X(valueOfExpression, "Q_ASSUME()", "Assumption in Q_ASSUME(\"" #Expr "\") was not correct");\
        Q_ASSUME_IMPL(valueOfExpression);\
        Q_UNUSED(valueOfExpression); /* the value may not be used if Q_ASSERT_X and Q_ASSUME_IMPL are noop */\
    } while (0)


/*
    Sanitize compiler feature availability
*/
#if !defined(Q_PROCESSOR_X86)
#  undef QT_COMPILER_SUPPORTS_SSE2
#  undef QT_COMPILER_SUPPORTS_SSE3
#  undef QT_COMPILER_SUPPORTS_SSSE3
#  undef QT_COMPILER_SUPPORTS_SSE4_1
#  undef QT_COMPILER_SUPPORTS_SSE4_2
#  undef QT_COMPILER_SUPPORTS_AVX
#  undef QT_COMPILER_SUPPORTS_AVX2
#endif
#if !defined(Q_PROCESSOR_ARM)
#  undef QT_COMPILER_SUPPORTS_IWMMXT
#endif
#if !defined(Q_PROCESSOR_MIPS)
#  undef QT_COMPILER_SUPPORTS_MIPS_DSP
#  undef QT_COMPILER_SUPPORTS_MIPS_DSPR2
#endif

#endif // QCOMPILERDETECTION_H
