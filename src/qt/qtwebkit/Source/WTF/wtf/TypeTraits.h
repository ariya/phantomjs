 /*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2009, 2010 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef TypeTraits_h
#define TypeTraits_h

#include <wtf/Platform.h>

#if (defined(__GLIBCXX__) && (__GLIBCXX__ >= 20070724) && defined(__GXX_EXPERIMENTAL_CXX0X__)) || (defined(_MSC_VER) && (_MSC_VER >= 1600))
#include <type_traits>
#if defined(__GLIBCXX__) && (__GLIBCXX__ >= 20070724) && defined(__GXX_EXPERIMENTAL_CXX0X__)
#include <tr1/memory>
#endif
#endif

namespace WTF {

    // The following are provided in this file:
    //
    //   Conditional<Predicate, If, Then>::Type
    //
    //   IsInteger<T>::value
    //   IsPod<T>::value, see the definition for a note about its limitations
    //   IsConvertibleToInteger<T>::value
    //
    //   IsArray<T>::value
    //
    //   IsSameType<T, U>::value
    //
    //   RemovePointer<T>::Type
    //   RemoveReference<T>::Type
    //   RemoveConst<T>::Type
    //   RemoveVolatile<T>::Type
    //   RemoveConstVolatile<T>::Type
    //   RemoveExtent<T>::Type
    //
    //   DecayArray<T>::Type
    //
    //   COMPILE_ASSERT's in TypeTraits.cpp illustrate their usage and what they do.

    template <bool Predicate, class If, class Then> struct Conditional  { typedef If Type; };
    template <class If, class Then> struct Conditional<false, If, Then> { typedef Then Type; };

    template<typename T> struct IsInteger           { static const bool value = false; };
    template<> struct IsInteger<bool>               { static const bool value = true; };
    template<> struct IsInteger<char>               { static const bool value = true; };
    template<> struct IsInteger<signed char>        { static const bool value = true; };
    template<> struct IsInteger<unsigned char>      { static const bool value = true; };
    template<> struct IsInteger<short>              { static const bool value = true; };
    template<> struct IsInteger<unsigned short>     { static const bool value = true; };
    template<> struct IsInteger<int>                { static const bool value = true; };
    template<> struct IsInteger<unsigned int>       { static const bool value = true; };
    template<> struct IsInteger<long>               { static const bool value = true; };
    template<> struct IsInteger<unsigned long>      { static const bool value = true; };
    template<> struct IsInteger<long long>          { static const bool value = true; };
    template<> struct IsInteger<unsigned long long> { static const bool value = true; };
#if !COMPILER(MSVC) || defined(_NATIVE_WCHAR_T_DEFINED)
    template<> struct IsInteger<wchar_t>            { static const bool value = true; };
#endif

    template<typename T> struct IsFloatingPoint     { static const bool value = false; };
    template<> struct IsFloatingPoint<float>        { static const bool value = true; };
    template<> struct IsFloatingPoint<double>       { static const bool value = true; };
    template<> struct IsFloatingPoint<long double>  { static const bool value = true; };

    template<typename T> struct IsArithmetic     { static const bool value = IsInteger<T>::value || IsFloatingPoint<T>::value; };

    // IsPod is misnamed as it doesn't cover all plain old data (pod) types.
    // Specifically, it doesn't allow for enums or for structs.
    template <typename T> struct IsPod           { static const bool value = IsArithmetic<T>::value; };
    template <typename P> struct IsPod<P*>       { static const bool value = true; };

    template<typename T> class IsConvertibleToInteger {
        // Avoid "possible loss of data" warning when using Microsoft's C++ compiler
        // by not converting int's to doubles.
        template<bool performCheck, typename U> class IsConvertibleToDouble;
        template<typename U> class IsConvertibleToDouble<false, U> {
        public:
            static const bool value = false;
        };

        template<typename U> class IsConvertibleToDouble<true, U> {
            typedef char YesType;
            struct NoType {
                char padding[8];
            };

            static YesType floatCheck(long double);
            static NoType floatCheck(...);
            static T& t;
        public:
            static const bool value = sizeof(floatCheck(t)) == sizeof(YesType);
        };

    public:
        static const bool value = IsInteger<T>::value || IsConvertibleToDouble<!IsInteger<T>::value, T>::value;
    };


    template <class T> struct IsArray {
        static const bool value = false;
    };

    template <class T> struct IsArray<T[]> {
        static const bool value = true;
    };

    template <class T, size_t N> struct IsArray<T[N]> {
        static const bool value = true;
    };


    template <typename T, typename U> struct IsSameType {
        static const bool value = false;
    };

    template <typename T> struct IsSameType<T, T> {
        static const bool value = true;
    };

    template <typename T, typename U> class IsSubclass {
        typedef char YesType;
        struct NoType {
            char padding[8];
        };

        static YesType subclassCheck(U*);
        static NoType subclassCheck(...);
        static T* t;
    public:
        static const bool value = sizeof(subclassCheck(t)) == sizeof(YesType);
    };

    template <typename T, template<class V> class U> class IsSubclassOfTemplate {
        typedef char YesType;
        struct NoType {
            char padding[8];
        };

        template<typename W> static YesType subclassCheck(U<W>*);
        static NoType subclassCheck(...);
        static T* t;
    public:
        static const bool value = sizeof(subclassCheck(t)) == sizeof(YesType);
    };

    template <typename T, template <class V> class OuterTemplate> struct RemoveTemplate {
        typedef T Type;
    };

    template <typename T, template <class V> class OuterTemplate> struct RemoveTemplate<OuterTemplate<T>, OuterTemplate> {
        typedef T Type;
    };

    template <typename T> struct RemoveConst {
        typedef T Type;
    };

    template <typename T> struct RemoveConst<const T> {
        typedef T Type;
    };

    template <typename T> struct RemoveVolatile {
        typedef T Type;
    };

    template <typename T> struct RemoveVolatile<volatile T> {
        typedef T Type;
    };

    template <typename T> struct RemoveConstVolatile {
        typedef typename RemoveVolatile<typename RemoveConst<T>::Type>::Type Type;
    };

    template <typename T> struct RemovePointer {
        typedef T Type;
    };

    template <typename T> struct RemovePointer<T*> {
        typedef T Type;
    };

    template <typename T> struct RemoveReference {
        typedef T Type;
    };

    template <typename T> struct RemoveReference<T&> {
        typedef T Type;
    };

    template <typename T> struct RemoveExtent {
        typedef T Type;
    };

    template <typename T> struct RemoveExtent<T[]> {
        typedef T Type;
    };

    template <typename T, size_t N> struct RemoveExtent<T[N]> {
        typedef T Type;
    };

    template <class T> struct DecayArray {
        typedef typename RemoveReference<T>::Type U;
    public:
        typedef typename Conditional<
            IsArray<U>::value,
            typename RemoveExtent<U>::Type*,
            typename RemoveConstVolatile<U>::Type
        >::Type Type;
    };

#if COMPILER(CLANG) || GCC_VERSION_AT_LEAST(4, 6, 0) || (defined(_MSC_VER) && (_MSC_VER >= 1400) && (_MSC_VER < 1600) && !defined(__INTEL_COMPILER))
    // VC8 (VS2005) and later has __has_trivial_constructor and __has_trivial_destructor,
    // but the implementation returns false for built-in types. We add the extra IsPod condition to 
    // work around this.
    template <typename T> struct HasTrivialConstructor {
        static const bool value = __has_trivial_constructor(T) || IsPod<RemoveConstVolatile<T> >::value;
    };
    template <typename T> struct HasTrivialDestructor {
        static const bool value = __has_trivial_destructor(T) || IsPod<RemoveConstVolatile<T> >::value;
    };
#elif (defined(__GLIBCXX__) && (__GLIBCXX__ >= 20070724) && defined(__GXX_EXPERIMENTAL_CXX0X__)) || (defined(_MSC_VER) && (_MSC_VER >= 1600))
    // GCC's libstdc++ 20070724 and later supports C++ TR1 type_traits in the std namespace.
    // VC10 (VS2010) and later support C++ TR1 type_traits in the std::tr1 namespace.
    template<typename T> struct HasTrivialConstructor : public std::tr1::has_trivial_constructor<T> { };
    template<typename T> struct HasTrivialDestructor : public std::tr1::has_trivial_destructor<T> { };
#else
    // For compilers that don't support detection of trivial constructors and destructors in classes, 
    // we use a template that returns true for any POD type that IsPod can detect (see IsPod caveats above), 
    // but false for all other types (which includes all classes). This will give false negatives, which can hurt 
    // performance, but avoids false positives, which would result in incorrect behavior.
    template <typename T> struct HasTrivialConstructor {
        static const bool value = IsPod<RemoveConstVolatile<T> >::value;
    };
    template <typename T> struct HasTrivialDestructor {
        static const bool value = IsPod<RemoveConstVolatile<T> >::value;
    };
#endif

} // namespace WTF

#endif // TypeTraits_h
