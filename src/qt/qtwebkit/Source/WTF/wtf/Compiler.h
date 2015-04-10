/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef WTF_Compiler_h
#define WTF_Compiler_h

/* COMPILER() - the compiler being used to build the project */
#define COMPILER(WTF_FEATURE) (defined WTF_COMPILER_##WTF_FEATURE  && WTF_COMPILER_##WTF_FEATURE)

/* COMPILER_SUPPORTS() - whether the compiler being used to build the project supports the given feature. */
#define COMPILER_SUPPORTS(WTF_COMPILER_FEATURE) (defined WTF_COMPILER_SUPPORTS_##WTF_COMPILER_FEATURE  && WTF_COMPILER_SUPPORTS_##WTF_COMPILER_FEATURE)

/* COMPILER_QUIRK() - whether the compiler being used to build the project requires a given quirk. */
#define COMPILER_QUIRK(WTF_COMPILER_QUIRK) (defined WTF_COMPILER_QUIRK_##WTF_COMPILER_QUIRK  && WTF_COMPILER_QUIRK_##WTF_COMPILER_QUIRK)

/* ==== COMPILER() - the compiler being used to build the project ==== */

/* COMPILER(CLANG) - Clang  */
#if defined(__clang__)
#define WTF_COMPILER_CLANG 1

#define CLANG_PRAGMA(PRAGMA) _Pragma(PRAGMA)

/* Specific compiler features */
#define WTF_COMPILER_SUPPORTS_CXX_VARIADIC_TEMPLATES __has_feature(cxx_variadic_templates)

/* There is a bug in clang that comes with Xcode 4.2 where AtomicStrings can't be implicitly converted to Strings
   in the presence of move constructors and/or move assignment operators. This bug has been fixed in Xcode 4.3 clang, so we
   check for both cxx_rvalue_references as well as the unrelated cxx_nonstatic_member_init feature which we know was added in 4.3 */
#define WTF_COMPILER_SUPPORTS_CXX_RVALUE_REFERENCES __has_feature(cxx_rvalue_references) && __has_feature(cxx_nonstatic_member_init)

#define WTF_COMPILER_SUPPORTS_CXX_DELETED_FUNCTIONS __has_feature(cxx_deleted_functions)
#define WTF_COMPILER_SUPPORTS_CXX_NULLPTR __has_feature(cxx_nullptr)
#define WTF_COMPILER_SUPPORTS_CXX_EXPLICIT_CONVERSIONS __has_feature(cxx_explicit_conversions)
#define WTF_COMPILER_SUPPORTS_BLOCKS __has_feature(blocks)
#define WTF_COMPILER_SUPPORTS_C_STATIC_ASSERT __has_feature(c_static_assert)
#define WTF_COMPILER_SUPPORTS_CXX_STATIC_ASSERT __has_feature(cxx_static_assert)
#define WTF_COMPILER_SUPPORTS_CXX_OVERRIDE_CONTROL __has_feature(cxx_override_control)
#define WTF_COMPILER_SUPPORTS_HAS_TRIVIAL_DESTRUCTOR __has_feature(has_trivial_destructor)
#define WTF_COMPILER_SUPPORTS_CXX_STRONG_ENUMS __has_feature(cxx_strong_enums)
#define WTF_COMPILER_SUPPORTS_CXX_REFERENCE_QUALIFIED_FUNCTIONS __has_feature(cxx_reference_qualified_functions)

#if defined(__APPLE__) && COMPILER_SUPPORTS(CXX_RVALUE_REFERENCES) && defined(_GLIBCXX_VERSION) && (_GLIBCXX_VERSION <= 20070719)
/* WTF expects the standard library to have std::move when the compiler supports rvalue references, but some old versions of stdc++11 shipped by Apple does not. */
#define WTF_COMPILER_SUPPORTS_CXX_RVALUE_REFERENCES 0
#endif

#endif

#ifndef CLANG_PRAGMA
#define CLANG_PRAGMA(PRAGMA)
#endif

/* COMPILER(MSVC) - Microsoft Visual C++ */
/* COMPILER(MSVC9_OR_LOWER) - Microsoft Visual C++ 2008 or lower*/
#if defined(_MSC_VER)
#define WTF_COMPILER_MSVC 1
#if _MSC_VER < 1600
#define WTF_COMPILER_MSVC9_OR_LOWER 1
#endif

/* Specific compiler features */
#if !COMPILER(CLANG) && _MSC_VER >= 1600
#define WTF_COMPILER_SUPPORTS_CXX_NULLPTR 1
#endif

#if !COMPILER(CLANG)
#define WTF_COMPILER_SUPPORTS_CXX_OVERRIDE_CONTROL 1
#define WTF_COMPILER_QUIRK_FINAL_IS_CALLED_SEALED 1
#endif

/* Check for VS2010 or newer */
#if _MSC_VER >= 1600
#define WTF_COMPILER_SUPPORTS_CXX_RVALUE_REFERENCES 1
#define WTF_COMPILER_SUPPORTS_CXX_STATIC_ASSERT 1
#endif

#endif /* defined(_MSC_VER) */

/* COMPILER(RVCT) - ARM RealView Compilation Tools */
#if defined(__CC_ARM) || defined(__ARMCC__)
#define WTF_COMPILER_RVCT 1
#define RVCT_VERSION_AT_LEAST(major, minor, patch, build) (__ARMCC_VERSION >= (major * 100000 + minor * 10000 + patch * 1000 + build))
#else
/* Define this for !RVCT compilers, just so we can write things like RVCT_VERSION_AT_LEAST(3, 0, 0, 0). */
#define RVCT_VERSION_AT_LEAST(major, minor, patch, build) 0
#endif

/* COMPILER(GCCE) - GNU Compiler Collection for Embedded */
#if defined(__GCCE__)
#define WTF_COMPILER_GCCE 1
#define GCCE_VERSION (__GCCE__ * 10000 + __GCCE_MINOR__ * 100 + __GCCE_PATCHLEVEL__)
#define GCCE_VERSION_AT_LEAST(major, minor, patch) (GCCE_VERSION >= (major * 10000 + minor * 100 + patch))
#endif

/* COMPILER(GCC) - GNU Compiler Collection */
/* --gnu option of the RVCT compiler also defines __GNUC__ */
#if defined(__GNUC__) && !COMPILER(RVCT)
#define WTF_COMPILER_GCC 1
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#define GCC_VERSION_AT_LEAST(major, minor, patch) (GCC_VERSION >= (major * 10000 + minor * 100 + patch))
#else
/* Define this for !GCC compilers, just so we can write things like GCC_VERSION_AT_LEAST(4, 1, 0). */
#define GCC_VERSION_AT_LEAST(major, minor, patch) 0
#endif

/* Specific compiler features */
#if COMPILER(GCC) && !COMPILER(CLANG)
#if GCC_VERSION_AT_LEAST(4, 8, 0)
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
/* C11 support */
#define WTF_COMPILER_SUPPORTS_C_STATIC_ASSERT 1
#endif
#if defined(__GXX_EXPERIMENTAL_CXX0X__) || (defined(__cplusplus) && __cplusplus >= 201103L)
/* C++11 support */
#if GCC_VERSION_AT_LEAST(4, 3, 0)
#define WTF_COMPILER_SUPPORTS_CXX_RVALUE_REFERENCES 1
#define WTF_COMPILER_SUPPORTS_CXX_STATIC_ASSERT 1
#define WTF_COMPILER_SUPPORTS_CXX_VARIADIC_TEMPLATES 1
#endif
#if GCC_VERSION_AT_LEAST(4, 4, 0)
#define WTF_COMPILER_SUPPORTS_CXX_DELETED_FUNCTIONS 1
#endif
#if GCC_VERSION_AT_LEAST(4, 5, 0)
#define WTF_COMPILER_SUPPORTS_CXX_EXPLICIT_CONVERSIONS 1
#endif
#if GCC_VERSION_AT_LEAST(4, 6, 0)
#define WTF_COMPILER_SUPPORTS_CXX_NULLPTR 1
/* Strong enums should work from gcc 4.4, but doesn't seem to support some operators */
#define WTF_COMPILER_SUPPORTS_CXX_STRONG_ENUMS 1
#endif
#if GCC_VERSION_AT_LEAST(4, 7, 0)
#define WTF_COMPILER_SUPPORTS_CXX_OVERRIDE_CONTROL 1
#endif
#endif /* defined(__GXX_EXPERIMENTAL_CXX0X__) || (defined(__cplusplus) && __cplusplus >= 201103L) */
#endif /* COMPILER(GCC) */

/* COMPILER(MINGW) - MinGW GCC */
/* COMPILER(MINGW64) - mingw-w64 GCC - only used as additional check to exclude mingw.org specific functions */
#if defined(__MINGW32__)
#define WTF_COMPILER_MINGW 1
#include <_mingw.h> /* private MinGW header */
    #if defined(__MINGW64_VERSION_MAJOR) /* best way to check for mingw-w64 vs mingw.org */
        #define WTF_COMPILER_MINGW64 1
    #endif /* __MINGW64_VERSION_MAJOR */
#endif /* __MINGW32__ */

/* COMPILER(INTEL) - Intel C++ Compiler */
#if defined(__INTEL_COMPILER)
#define WTF_COMPILER_INTEL 1
#endif

/* COMPILER(SUNCC) */
#if defined(__SUNPRO_CC) || defined(__SUNPRO_C)
#define WTF_COMPILER_SUNCC 1
#endif

/* ABI */
#if defined(__ARM_EABI__) || defined(__EABI__)
#define WTF_COMPILER_SUPPORTS_EABI 1
#endif

/* ==== Compiler features ==== */

/* ALWAYS_INLINE */

#ifndef ALWAYS_INLINE
#if COMPILER(GCC) && defined(NDEBUG) && !COMPILER(MINGW)
#define ALWAYS_INLINE inline __attribute__((__always_inline__))
#elif (COMPILER(MSVC) || COMPILER(RVCT)) && defined(NDEBUG)
#define ALWAYS_INLINE __forceinline
#else
#define ALWAYS_INLINE inline
#endif
#endif


/* NEVER_INLINE */

#ifndef NEVER_INLINE
#if COMPILER(GCC)
#define NEVER_INLINE __attribute__((__noinline__))
#elif COMPILER(RVCT)
#define NEVER_INLINE __declspec(noinline)
#else
#define NEVER_INLINE
#endif
#endif


/* UNLIKELY */

#ifndef UNLIKELY
#if COMPILER(GCC) || (COMPILER(RVCT) && defined(__GNUC__))
#define UNLIKELY(x) __builtin_expect((x), 0)
#else
#define UNLIKELY(x) (x)
#endif
#endif


/* LIKELY */

#ifndef LIKELY
#if COMPILER(GCC) || (COMPILER(RVCT) && defined(__GNUC__))
#define LIKELY(x) __builtin_expect((x), 1)
#else
#define LIKELY(x) (x)
#endif
#endif


/* NO_RETURN */


#ifndef NO_RETURN
#if COMPILER(GCC)
#define NO_RETURN __attribute((__noreturn__))
#elif COMPILER(MSVC) || COMPILER(RVCT)
#define NO_RETURN __declspec(noreturn)
#else
#define NO_RETURN
#endif
#endif


/* NO_RETURN_WITH_VALUE */

#ifndef NO_RETURN_WITH_VALUE
#if !COMPILER(MSVC)
#define NO_RETURN_WITH_VALUE NO_RETURN
#else
#define NO_RETURN_WITH_VALUE
#endif
#endif


/* WARN_UNUSED_RETURN */

#if COMPILER(GCC)
#define WARN_UNUSED_RETURN __attribute__ ((warn_unused_result))
#else
#define WARN_UNUSED_RETURN
#endif

/* OVERRIDE and FINAL */

#if COMPILER_SUPPORTS(CXX_OVERRIDE_CONTROL)
#define OVERRIDE override

#if COMPILER_QUIRK(FINAL_IS_CALLED_SEALED)
#define FINAL sealed
#else
#define FINAL final
#endif

#else
#define OVERRIDE
#define FINAL
#endif

#if COMPILER_SUPPORTS(CXX_DELETED_FUNCTIONS)
#define WTF_DELETED_FUNCTION = delete
#else
#define WTF_DELETED_FUNCTION
#endif

/* REFERENCED_FROM_ASM */

#ifndef REFERENCED_FROM_ASM
#if COMPILER(GCC)
#define REFERENCED_FROM_ASM __attribute__((used))
#else
#define REFERENCED_FROM_ASM
#endif
#endif

/* OBJC_CLASS */

#ifndef OBJC_CLASS
#ifdef __OBJC__
#define OBJC_CLASS @class
#else
#define OBJC_CLASS class
#endif
#endif

/* UNUSED_PARAM */

#if COMPILER(RVCT)
template<typename T>
inline void unusedParam(T& x) { (void)x; }
#define UNUSED_PARAM(variable) unusedParam(variable)
#elif COMPILER(MSVC)
#define UNUSED_PARAM(variable) (void)&variable
#else
#define UNUSED_PARAM(variable) (void)variable
#endif

/* UNUSED_LABEL */

/* This is to keep the compiler from complaining when for local labels are
 declared but not referenced. For example, this can happen with code that
 works with auto-generated code.
 */
#if COMPILER(MSVC)
#define UNUSED_LABEL(label) if (false) goto label
#else
#define UNUSED_LABEL(label) UNUSED_PARAM(&& label)
#endif



#endif /* WTF_Compiler_h */
