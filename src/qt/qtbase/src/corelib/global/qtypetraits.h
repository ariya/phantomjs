/****************************************************************************
**
** Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
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

// BEGIN Google Code

// Copyright (c) 2006, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// ----
//
// This code is compiled directly on many platforms, including client
// platforms like Windows, Mac, and embedded systems.  Before making
// any changes here, make sure that you're not breaking any platforms.
//
// Define a small subset of tr1 type traits. The traits we define are:
//   is_integral
//   is_floating_point
//   is_pointer
//   is_enum
//   is_reference
//   is_const
//   is_volatile
//   is_pod
//   has_trivial_constructor
//   has_trivial_copy
//   has_trivial_assign
//   has_trivial_destructor
//   is_signed
//   is_unsigned
//   remove_const
//   remove_volatile
//   remove_cv
//   remove_reference
//   add_reference
//   remove_pointer
//   is_same
//   is_convertible
// We can add more type traits as required.

// Changes from the original implementation:
//  - Move base types from template_util.h directly into this header.
//  - Use Qt macros for long long type differences on Windows.
//  - Enclose in QtPrivate namespace.

#include "QtCore/qglobal.h"

#ifndef QTYPETRAITS_H
#define QTYPETRAITS_H

#include <utility>                  // For pair

QT_BEGIN_NAMESPACE

namespace QtPrivate {

// Types small_ and big_ are guaranteed such that sizeof(small_) <
// sizeof(big_)
typedef char small_;

struct big_ {
  char dummy[2];
};

// Identity metafunction.
template <class T>
struct identity_ {
  typedef T type;
};

// integral_constant, defined in tr1, is a wrapper for an integer
// value. We don't really need this generality; we could get away
// with hardcoding the integer type to bool. We use the fully
// general integer_constant for compatibility with tr1.

template<class T, T v>
struct integral_constant {
  static const T value = v;
  typedef T value_type;
  typedef integral_constant<T, v> type;
};

template <class T, T v> const T integral_constant<T, v>::value;


// Abbreviations: true_type and false_type are structs that represent boolean
// true and false values. Also define the boost::mpl versions of those names,
// true_ and false_.
typedef integral_constant<bool, true>  true_type;
typedef integral_constant<bool, false> false_type;
typedef true_type  true_;
typedef false_type false_;

// if_ is a templatized conditional statement.
// if_<cond, A, B> is a compile time evaluation of cond.
// if_<>::type contains A if cond is true, B otherwise.
template<bool cond, typename A, typename B>
struct if_{
  typedef A type;
};

template<typename A, typename B>
struct if_<false, A, B> {
  typedef B type;
};


// type_equals_ is a template type comparator, similar to Loki IsSameType.
// type_equals_<A, B>::value is true iff "A" is the same type as "B".
//
// New code should prefer base::is_same, defined in base/type_traits.h.
// It is functionally identical, but is_same is the standard spelling.
template<typename A, typename B>
struct type_equals_ : public false_ {
};

template<typename A>
struct type_equals_<A, A> : public true_ {
};

// and_ is a template && operator.
// and_<A, B>::value evaluates "A::value && B::value".
template<typename A, typename B>
struct and_ : public integral_constant<bool, (A::value && B::value)> {
};

// or_ is a template || operator.
// or_<A, B>::value evaluates "A::value || B::value".
template<typename A, typename B>
struct or_ : public integral_constant<bool, (A::value || B::value)> {
};

template <class T> struct is_integral;
template <class T> struct is_floating_point;
template <class T> struct is_pointer;
// MSVC can't compile this correctly, and neither can gcc 3.3.5 (at least)
#if !defined(_MSC_VER) && !(defined(__GNUC__) && __GNUC__ <= 3)
// is_enum uses is_convertible, which is not available on MSVC.
template <class T> struct is_enum;
#endif
template <class T> struct is_reference;
template <class T> struct is_pod;
template <class T> struct has_trivial_constructor;
template <class T> struct has_trivial_copy;
template <class T> struct has_trivial_assign;
template <class T> struct has_trivial_destructor;
template <class T> struct remove_const;
template <class T> struct remove_volatile;
template <class T> struct remove_cv;
template <class T> struct remove_reference;
template <class T> struct add_reference;
template <class T> struct remove_pointer;
template <class T, class U> struct is_same;
#if !defined(_MSC_VER) && !(defined(__GNUC__) && __GNUC__ <= 3)
template <class From, class To> struct is_convertible;
#endif

// is_integral is false except for the built-in integer types. A
// cv-qualified type is integral if and only if the underlying type is.
template <class T> struct is_integral : false_type { };
template<> struct is_integral<bool> : true_type { };
template<> struct is_integral<char> : true_type { };
template<> struct is_integral<unsigned char> : true_type { };
template<> struct is_integral<signed char> : true_type { };
#if defined(_MSC_VER)
// wchar_t is not by default a distinct type from unsigned short in
// Microsoft C.
// See http://msdn2.microsoft.com/en-us/library/dh8che7s(VS.80).aspx
template<> struct is_integral<__wchar_t> : true_type { };
#else
template<> struct is_integral<wchar_t> : true_type { };
#endif
template<> struct is_integral<short> : true_type { };
template<> struct is_integral<unsigned short> : true_type { };
template<> struct is_integral<int> : true_type { };
template<> struct is_integral<unsigned int> : true_type { };
template<> struct is_integral<long> : true_type { };
template<> struct is_integral<unsigned long> : true_type { };
#if defined(Q_OS_WIN) && !defined(Q_CC_GNU)
template<> struct is_integral<__int64> : true_type { };
template<> struct is_integral<unsigned __int64> : true_type { };
#else
template<> struct is_integral<long long> : true_type { };
template<> struct is_integral<unsigned long long> : true_type { };
#endif
template <class T> struct is_integral<const T> : is_integral<T> { };
template <class T> struct is_integral<volatile T> : is_integral<T> { };
template <class T> struct is_integral<const volatile T> : is_integral<T> { };
#if defined (Q_COMPILER_UNICODE_STRINGS)
template<> struct is_integral<char16_t> : true_type { };
template<> struct is_integral<char32_t> : true_type { };
#endif

// is_floating_point is false except for the built-in floating-point types.
// A cv-qualified type is integral if and only if the underlying type is.
template <class T> struct is_floating_point : false_type { };
template<> struct is_floating_point<float> : true_type { };
template<> struct is_floating_point<double> : true_type { };
template<> struct is_floating_point<long double> : true_type { };
template <class T> struct is_floating_point<const T>
    : is_floating_point<T> { };
template <class T> struct is_floating_point<volatile T>
    : is_floating_point<T> { };
template <class T> struct is_floating_point<const volatile T>
    : is_floating_point<T> { };

// is_pointer is false except for pointer types. A cv-qualified type (e.g.
// "int* const", as opposed to "int const*") is cv-qualified if and only if
// the underlying type is.
template <class T> struct is_pointer : false_type { };
template <class T> struct is_pointer<T*> : true_type { };
template <class T> struct is_pointer<const T> : is_pointer<T> { };
template <class T> struct is_pointer<volatile T> : is_pointer<T> { };
template <class T> struct is_pointer<const volatile T> : is_pointer<T> { };

#if !defined(_MSC_VER) && !(defined(__GNUC__) && __GNUC__ <= 3)

namespace internal {

template <class T> struct is_class_or_union {
  template <class U> static small_ tester(void (U::*)());
  template <class U> static big_ tester(...);
  static const bool value = sizeof(tester<T>(0)) == sizeof(small_);
};

// is_convertible chokes if the first argument is an array. That's why
// we use add_reference here.
template <bool NotUnum, class T> struct is_enum_impl
    : is_convertible<typename add_reference<T>::type, int> { };

template <class T> struct is_enum_impl<true, T> : false_type { };

}  // namespace internal

// Specified by TR1 [4.5.1] primary type categories.

// Implementation note:
//
// Each type is either void, integral, floating point, array, pointer,
// reference, member object pointer, member function pointer, enum,
// union or class. Out of these, only integral, floating point, reference,
// class and enum types are potentially convertible to int. Therefore,
// if a type is not a reference, integral, floating point or class and
// is convertible to int, it's a enum. Adding cv-qualification to a type
// does not change whether it's an enum.
//
// Is-convertible-to-int check is done only if all other checks pass,
// because it can't be used with some types (e.g. void or classes with
// inaccessible conversion operators).
template <class T> struct is_enum
    : internal::is_enum_impl<
          is_same<T, void>::value ||
              is_integral<T>::value ||
              is_floating_point<T>::value ||
              is_reference<T>::value ||
              internal::is_class_or_union<T>::value,
          T> { };

template <class T> struct is_enum<const T> : is_enum<T> { };
template <class T> struct is_enum<volatile T> : is_enum<T> { };
template <class T> struct is_enum<const volatile T> : is_enum<T> { };

#endif

// is_reference is false except for reference types.
template<typename T> struct is_reference : false_type {};
template<typename T> struct is_reference<T&> : true_type {};

// Specified by TR1 [4.5.3] Type Properties
template <typename T> struct is_const : false_type {};
template <typename T> struct is_const<const T> : true_type {};
template <typename T> struct is_volatile : false_type {};
template <typename T> struct is_volatile<volatile T> : true_type {};

// We can't get is_pod right without compiler help, so fail conservatively.
// We will assume it's false except for arithmetic types, enumerations,
// pointers and cv-qualified versions thereof. Note that std::pair<T,U>
// is not a POD even if T and U are PODs.
template <class T> struct is_pod
 : integral_constant<bool, (is_integral<T>::value ||
                            is_floating_point<T>::value ||
#if !defined(_MSC_VER) && !(defined(__GNUC__) && __GNUC__ <= 3)
                            // is_enum is not available on MSVC.
                            is_enum<T>::value ||
#endif
                            is_pointer<T>::value)> { };
template <class T> struct is_pod<const T> : is_pod<T> { };
template <class T> struct is_pod<volatile T> : is_pod<T> { };
template <class T> struct is_pod<const volatile T> : is_pod<T> { };


// We can't get has_trivial_constructor right without compiler help, so
// fail conservatively. We will assume it's false except for: (1) types
// for which is_pod is true. (2) std::pair of types with trivial
// constructors. (3) array of a type with a trivial constructor.
// (4) const versions thereof.
template <class T> struct has_trivial_constructor : is_pod<T> { };
template <class T, class U> struct has_trivial_constructor<std::pair<T, U> >
  : integral_constant<bool,
                      (has_trivial_constructor<T>::value &&
                       has_trivial_constructor<U>::value)> { };
template <class A, int N> struct has_trivial_constructor<A[N]>
  : has_trivial_constructor<A> { };
template <class T> struct has_trivial_constructor<const T>
  : has_trivial_constructor<T> { };

// We can't get has_trivial_copy right without compiler help, so fail
// conservatively. We will assume it's false except for: (1) types
// for which is_pod is true. (2) std::pair of types with trivial copy
// constructors. (3) array of a type with a trivial copy constructor.
// (4) const versions thereof.
template <class T> struct has_trivial_copy : is_pod<T> { };
template <class T, class U> struct has_trivial_copy<std::pair<T, U> >
  : integral_constant<bool,
                      (has_trivial_copy<T>::value &&
                       has_trivial_copy<U>::value)> { };
template <class A, int N> struct has_trivial_copy<A[N]>
  : has_trivial_copy<A> { };
template <class T> struct has_trivial_copy<const T> : has_trivial_copy<T> { };

// We can't get has_trivial_assign right without compiler help, so fail
// conservatively. We will assume it's false except for: (1) types
// for which is_pod is true. (2) std::pair of types with trivial copy
// constructors. (3) array of a type with a trivial assign constructor.
template <class T> struct has_trivial_assign : is_pod<T> { };
template <class T, class U> struct has_trivial_assign<std::pair<T, U> >
  : integral_constant<bool,
                      (has_trivial_assign<T>::value &&
                       has_trivial_assign<U>::value)> { };
template <class A, int N> struct has_trivial_assign<A[N]>
  : has_trivial_assign<A> { };

// We can't get has_trivial_destructor right without compiler help, so
// fail conservatively. We will assume it's false except for: (1) types
// for which is_pod is true. (2) std::pair of types with trivial
// destructors. (3) array of a type with a trivial destructor.
// (4) const versions thereof.
template <class T> struct has_trivial_destructor : is_pod<T> { };
template <class T, class U> struct has_trivial_destructor<std::pair<T, U> >
  : integral_constant<bool,
                      (has_trivial_destructor<T>::value &&
                       has_trivial_destructor<U>::value)> { };
template <class A, int N> struct has_trivial_destructor<A[N]>
  : has_trivial_destructor<A> { };
template <class T> struct has_trivial_destructor<const T>
  : has_trivial_destructor<T> { };

// Specified by TR1 [4.7.1]
template<typename T> struct remove_const { typedef T type; };
template<typename T> struct remove_const<T const> { typedef T type; };
template<typename T> struct remove_volatile { typedef T type; };
template<typename T> struct remove_volatile<T volatile> { typedef T type; };
template<typename T> struct remove_cv {
  typedef typename remove_const<typename remove_volatile<T>::type>::type type;
};


// Specified by TR1 [4.7.2] Reference modifications.
template<typename T> struct remove_reference { typedef T type; };
template<typename T> struct remove_reference<T&> { typedef T type; };

template <typename T> struct add_reference { typedef T& type; };
template <typename T> struct add_reference<T&> { typedef T& type; };

// Specified by TR1 [4.7.4] Pointer modifications.
template<typename T> struct remove_pointer { typedef T type; };
template<typename T> struct remove_pointer<T*> { typedef T type; };
template<typename T> struct remove_pointer<T* const> { typedef T type; };
template<typename T> struct remove_pointer<T* volatile> { typedef T type; };
template<typename T> struct remove_pointer<T* const volatile> {
  typedef T type; };

// Specified by TR1 [4.6] Relationships between types
template<typename T, typename U> struct is_same : public false_type { };
template<typename T> struct is_same<T, T> : public true_type { };

// Specified by TR1 [4.6] Relationships between types
#if !defined(_MSC_VER) && !(defined(__GNUC__) && __GNUC__ <= 3)
namespace internal {

// This class is an implementation detail for is_convertible, and you
// don't need to know how it works to use is_convertible. For those
// who care: we declare two different functions, one whose argument is
// of type To and one with a variadic argument list. We give them
// return types of different size, so we can use sizeof to trick the
// compiler into telling us which function it would have chosen if we
// had called it with an argument of type From.  See Alexandrescu's
// _Modern C++ Design_ for more details on this sort of trick.

template <typename From, typename To>
struct ConvertHelper {
  static small_ Test(To);
  static big_ Test(...);
  static From Create();
};
}  // namespace internal

// Inherits from true_type if From is convertible to To, false_type otherwise.
template <typename From, typename To>
struct is_convertible
    : integral_constant<bool,
                        sizeof(internal::ConvertHelper<From, To>::Test(
                                  internal::ConvertHelper<From, To>::Create()))
                        == sizeof(small_)> {
};
#endif

// END Google Code

// a metafunction to invert an integral_constant:
template <typename T>
struct not_
    : integral_constant<bool, !T::value> {};

// same, with a bool argument:
template <bool B>
struct not_c
    : integral_constant<bool, !B> {};

// Checks whether a type is unsigned (T must be convertible to unsigned int):
template <typename T>
struct is_unsigned
    : integral_constant<bool, (T(0) < T(-1))> {};

// Checks whether a type is signed (T must be convertible to int):
template <typename T>
struct is_signed
    : not_< is_unsigned<T> > {};

Q_STATIC_ASSERT(( is_unsigned<quint8>::value));
Q_STATIC_ASSERT((!is_unsigned<qint8>::value));

Q_STATIC_ASSERT((!is_signed<quint8>::value));
Q_STATIC_ASSERT(( is_signed<qint8>::value));

Q_STATIC_ASSERT(( is_unsigned<quint16>::value));
Q_STATIC_ASSERT((!is_unsigned<qint16>::value));

Q_STATIC_ASSERT((!is_signed<quint16>::value));
Q_STATIC_ASSERT(( is_signed<qint16>::value));

Q_STATIC_ASSERT(( is_unsigned<quint32>::value));
Q_STATIC_ASSERT((!is_unsigned<qint32>::value));

Q_STATIC_ASSERT((!is_signed<quint32>::value));
Q_STATIC_ASSERT(( is_signed<qint32>::value));

Q_STATIC_ASSERT(( is_unsigned<quint64>::value));
Q_STATIC_ASSERT((!is_unsigned<qint64>::value));

Q_STATIC_ASSERT((!is_signed<quint64>::value));
Q_STATIC_ASSERT(( is_signed<qint64>::value));

} // namespace QtPrivate

QT_END_NAMESPACE
#endif  // QTYPETRAITS_H
