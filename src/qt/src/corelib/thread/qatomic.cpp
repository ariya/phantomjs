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

/*!
    \class QAtomicInt
    \brief The QAtomicInt class provides platform-independent atomic operations on integers.
    \since 4.4

    \ingroup thread

    For atomic operations on pointers, see the QAtomicPointer class.

    An \e atomic operation is a complex operation that completes without interruption. 
    The QAtomicInt class provides atomic reference counting, test-and-set, fetch-and-store,
    and fetch-and-add for integers.

    \section1 Non-atomic convenience operators

    For convenience, QAtomicInt provides integer comparison, cast, and
    assignment operators. Note that a combination of these operators
    is \e not an atomic operation.

    \section1 The Atomic API

    \section2 Reference counting

    The ref() and deref() functions provide an efficient reference
    counting API. The return value of these functions are used to
    indicate when the last reference has been released. These
    functions allow you to implement your own implicitly shared
    classes.

    \snippet doc/src/snippets/code/src_corelib_thread_qatomic.cpp 0

    \section2 Memory ordering

    QAtomicInt provides several implementations of the atomic
    test-and-set, fetch-and-store, and fetch-and-add functions. Each
    implementation defines a memory ordering semantic that describes
    how memory accesses surrounding the atomic instruction are
    executed by the processor. Since many modern architectures allow
    out-of-order execution and memory ordering, using the correct
    semantic is necessary to ensure that your application functions
    properly on all processors.

    \list

    \o Relaxed - memory ordering is unspecified, leaving the compiler
    and processor to freely reorder memory accesses.

    \o Acquire - memory access following the atomic operation (in
    program order) may not be re-ordered before the atomic operation.

    \o Release - memory access before the atomic operation (in program
    order) may not be re-ordered after the atomic operation.

    \o Ordered - the same Acquire and Release semantics combined.

    \endlist

    \section2 Test-and-set

    If the current value of the QAtomicInt is an expected value, the
    test-and-set functions assign a new value to the QAtomicInt and
    return true. If values are \a not the same, these functions do
    nothing and return false. This operation equates to the following
    code:

    \snippet doc/src/snippets/code/src_corelib_thread_qatomic.cpp 1

    There are 4 test-and-set functions: testAndSetRelaxed(),
    testAndSetAcquire(), testAndSetRelease(), and
    testAndSetOrdered(). See above for an explanation of the different
    memory ordering semantics.

    \section2 Fetch-and-store

    The atomic fetch-and-store functions read the current value of the
    QAtomicInt and then assign a new value, returning the original
    value. This operation equates to the following code:

    \snippet doc/src/snippets/code/src_corelib_thread_qatomic.cpp 2

    There are 4 fetch-and-store functions: fetchAndStoreRelaxed(),
    fetchAndStoreAcquire(), fetchAndStoreRelease(), and
    fetchAndStoreOrdered(). See above for an explanation of the
    different memory ordering semantics.

    \section2 Fetch-and-add

    The atomic fetch-and-add functions read the current value of the
    QAtomicInt and then add the given value to the current value,
    returning the original value. This operation equates to the
    following code:

    \snippet doc/src/snippets/code/src_corelib_thread_qatomic.cpp 3

    There are 4 fetch-and-add functions: fetchAndAddRelaxed(),
    fetchAndAddAcquire(), fetchAndAddRelease(), and
    fetchAndAddOrdered(). See above for an explanation of the
    different memory ordering semantics.

    \section1 Feature Tests for the Atomic API

    Providing a platform-independent atomic API that works on all
    processors is challenging. The API provided by QAtomicInt is
    guaranteed to work atomically on all processors. However, since
    not all processors implement support for every operation provided
    by QAtomicInt, it is necessary to expose information about the
    processor.

    You can check at compile time which features are supported on your
    hardware using various macros. These will tell you if your
    hardware always, sometimes, or does not support a particular
    operation. The macros have the form
    Q_ATOMIC_INT_\e{OPERATION}_IS_\e{HOW}_NATIVE. \e{OPERATION}
    is one of REFERENCE_COUNTING, TEST_AND_SET,
    FETCH_AND_STORE, or FETCH_AND_ADD, and \e{HOW} is one of
    ALWAYS, SOMETIMES, or NOT. There will always be exactly one
    defined macro per operation. For example, if
    Q_ATOMIC_INT_REFERENCE_COUNTING_IS_ALWAYS_NATIVE is defined,
    neither Q_ATOMIC_INT_REFERENCE_COUNTING_IS_SOMETIMES_NATIVE nor
    Q_ATOMIC_INT_REFERENCE_COUNTING_IS_NOT_NATIVE will be defined.

    An operation that completes in constant time is said to be
    wait-free. Such operations are not implemented using locks or
    loops of any kind. For atomic operations that are always
    supported, and that are wait-free, Qt defines the
    Q_ATOMIC_INT_\e{OPERATION}_IS_WAIT_FREE in addition to the
    Q_ATOMIC_INT_\e{OPERATION}_IS_ALWAYS_NATIVE.

    In cases where an atomic operation is only supported in newer
    generations of the processor, QAtomicInt also provides a way to
    check at runtime what your hardware supports with the
    isReferenceCountingNative(), isTestAndSetNative(),
    isFetchAndStoreNative(), and isFetchAndAddNative()
    functions. Wait-free implementations can be detected using the
    isReferenceCountingWaitFree(), isTestAndSetWaitFree(),
    isFetchAndStoreWaitFree(), and isFetchAndAddWaitFree() functions.

    Below is a complete list of all feature macros for QAtomicInt:

    \list

    \o Q_ATOMIC_INT_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
    \o Q_ATOMIC_INT_REFERENCE_COUNTING_IS_SOMETIMES_NATIVE
    \o Q_ATOMIC_INT_REFERENCE_COUNTING_IS_NOT_NATIVE
    \o Q_ATOMIC_INT_REFERENCE_COUNTING_IS_WAIT_FREE

    \o Q_ATOMIC_INT_TEST_AND_SET_IS_ALWAYS_NATIVE
    \o Q_ATOMIC_INT_TEST_AND_SET_IS_SOMETIMES_NATIVE
    \o Q_ATOMIC_INT_TEST_AND_SET_IS_NOT_NATIVE
    \o Q_ATOMIC_INT_TEST_AND_SET_IS_WAIT_FREE

    \o Q_ATOMIC_INT_FETCH_AND_STORE_IS_ALWAYS_NATIVE
    \o Q_ATOMIC_INT_FETCH_AND_STORE_IS_SOMETIMES_NATIVE
    \o Q_ATOMIC_INT_FETCH_AND_STORE_IS_NOT_NATIVE
    \o Q_ATOMIC_INT_FETCH_AND_STORE_IS_WAIT_FREE

    \o Q_ATOMIC_INT_FETCH_AND_ADD_IS_ALWAYS_NATIVE
    \o Q_ATOMIC_INT_FETCH_AND_ADD_IS_SOMETIMES_NATIVE
    \o Q_ATOMIC_INT_FETCH_AND_ADD_IS_NOT_NATIVE
    \o Q_ATOMIC_INT_FETCH_AND_ADD_IS_WAIT_FREE

    \endlist

    \sa QAtomicPointer
*/

/*! \fn QAtomicInt::QAtomicInt(int value)

    Constructs a QAtomicInt with the given \a value.
*/

/*! \fn QAtomicInt::QAtomicInt(const QAtomicInt &other)

    Constructs a copy of \a other.
*/

/*! \fn QAtomicInt &QAtomicInt::operator=(int value)

    Assigns the \a value to this QAtomicInt and returns a reference to
    this QAtomicInt.
*/

/*! \fn QAtomicInt &QAtomicInt::operator=(const QAtomicInt &other)

    Assigns \a other to this QAtomicInt and returns a reference to
    this QAtomicInt.
*/

/*! \fn bool QAtomicInt::operator==(int value) const

    Returns true if the \a value is equal to the value in this
    QAtomicInt; otherwise returns false.
*/

/*! \fn bool QAtomicInt::operator!=(int value) const

    Returns true if the value of this QAtomicInt is not equal to \a
    value; otherwise returns false.
*/

/*! \fn bool QAtomicInt::operator!() const

    Returns true is the value of this QAtomicInt is zero; otherwise
    returns false.
*/

/*! \fn QAtomicInt::operator int() const

    Returns the value stored by the QAtomicInt object as an integer.
*/

/*! \fn bool QAtomicInt::isReferenceCountingNative()

    Returns true if reference counting is implemented using atomic
    processor instructions, false otherwise.
*/

/*! \fn bool QAtomicInt::isReferenceCountingWaitFree()

    Returns true if atomic reference counting is wait-free, false
    otherwise.
*/

/*! \fn bool QAtomicInt::ref()
    Atomically increments the value of this QAtomicInt. Returns true
    if the new value is non-zero, false otherwise.

    This function uses \e ordered \l {QAtomicInt#Memory
    ordering}{memory ordering} semantics, which ensures that memory
    access before and after the atomic operation (in program order)
    may not be re-ordered.

    \sa deref()
*/

/*! \fn bool QAtomicInt::deref()
    Atomically decrements the value of this QAtomicInt. Returns true
    if the new value is non-zero, false otherwise.

    This function uses \e ordered \l {QAtomicInt#Memory
    ordering}{memory ordering} semantics, which ensures that memory
    access before and after the atomic operation (in program order)
    may not be re-ordered.

    \sa ref()
*/

/*! \fn bool QAtomicInt::isTestAndSetNative()

    Returns true if test-and-set is implemented using atomic processor
    instructions, false otherwise.
*/

/*! \fn bool QAtomicInt::isTestAndSetWaitFree()

    Returns true if atomic test-and-set is wait-free, false otherwise.
*/

/*! \fn bool QAtomicInt::testAndSetRelaxed(int expectedValue, int newValue)

    Atomic test-and-set.

    If the current value of this QAtomicInt is the \a expectedValue,
    the test-and-set functions assign the \a newValue to this
    QAtomicInt and return true. If the values are \e not the same,
    this function does nothing and returns false.

    This function uses \e relaxed \l {QAtomicInt#Memory
    ordering}{memory ordering} semantics, leaving the compiler and
    processor to freely reorder memory accesses.
*/

/*! \fn bool QAtomicInt::testAndSetAcquire(int expectedValue, int newValue)

    Atomic test-and-set.

    If the current value of this QAtomicInt is the \a expectedValue,
    the test-and-set functions assign the \a newValue to this
    QAtomicInt and return true. If the values are \e not the same,
    this function does nothing and returns false.

    This function uses \e acquire \l {QAtomicInt#Memory
    ordering}{memory ordering} semantics, which ensures that memory
    access following the atomic operation (in program order) may not
    be re-ordered before the atomic operation.
*/

/*! \fn bool QAtomicInt::testAndSetRelease(int expectedValue, int newValue)

    Atomic test-and-set.

    If the current value of this QAtomicInt is the \a expectedValue,
    the test-and-set functions assign the \a newValue to this
    QAtomicInt and return true. If the values are \e not the same,
    this function does nothing and returns false.

    This function uses \e release \l {QAtomicInt#Memory
    ordering}{memory ordering} semantics, which ensures that memory
    access before the atomic operation (in program order) may not be
    re-ordered after the atomic operation.
*/

/*! \fn bool QAtomicInt::testAndSetOrdered(int expectedValue, int newValue)

    Atomic test-and-set.

    If the current value of this QAtomicInt is the \a expectedValue,
    the test-and-set functions assign the \a newValue to this
    QAtomicInt and return true. If the values are \e not the same,
    this function does nothing and returns false.

    This function uses \e ordered \l {QAtomicInt#Memory
    ordering}{memory ordering} semantics, which ensures that memory
    access before and after the atomic operation (in program order)
    may not be re-ordered.
*/

/*! \fn bool QAtomicInt::isFetchAndStoreNative()

    Returns true if fetch-and-store is implemented using atomic
    processor instructions, false otherwise.
*/

/*! \fn bool QAtomicInt::isFetchAndStoreWaitFree()

    Returns true if atomic fetch-and-store is wait-free, false
    otherwise.
*/

/*! \fn int QAtomicInt::fetchAndStoreRelaxed(int newValue)

    Atomic fetch-and-store.

    Reads the current value of this QAtomicInt and then assigns it the
    \a newValue, returning the original value.

    This function uses \e relaxed \l {QAtomicInt#Memory
    ordering}{memory ordering} semantics, leaving the compiler and
    processor to freely reorder memory accesses.
*/

/*! \fn int QAtomicInt::fetchAndStoreAcquire(int newValue)

    Atomic fetch-and-store.

    Reads the current value of this QAtomicInt and then assigns it the
    \a newValue, returning the original value.

    This function uses \e acquire \l {QAtomicInt#Memory
    ordering}{memory ordering} semantics, which ensures that memory
    access following the atomic operation (in program order) may not
    be re-ordered before the atomic operation.
*/

/*! \fn int QAtomicInt::fetchAndStoreRelease(int newValue)

    Atomic fetch-and-store.

    Reads the current value of this QAtomicInt and then assigns it the
    \a newValue, returning the original value.

    This function uses \e release \l {QAtomicInt#Memory
    ordering}{memory ordering} semantics, which ensures that memory
    access before the atomic operation (in program order) may not be
    re-ordered after the atomic operation.
*/

/*! \fn int QAtomicInt::fetchAndStoreOrdered(int newValue)

    Atomic fetch-and-store.

    Reads the current value of this QAtomicInt and then assigns it the
    \a newValue, returning the original value.

    This function uses \e ordered \l {QAtomicInt#Memory
    ordering}{memory ordering} semantics, which ensures that memory
    access before and after the atomic operation (in program order)
    may not be re-ordered.
*/

/*! \fn bool QAtomicInt::isFetchAndAddNative()

    Returns true if fetch-and-add is implemented using atomic
    processor instructions, false otherwise.
*/

/*! \fn bool QAtomicInt::isFetchAndAddWaitFree()

    Returns true if atomic fetch-and-add is wait-free, false
    otherwise.
*/

/*! \fn int QAtomicInt::fetchAndAddRelaxed(int valueToAdd)

    Atomic fetch-and-add.

    Reads the current value of this QAtomicInt and then adds
    \a valueToAdd to the current value, returning the original value.

    This function uses \e relaxed \l {QAtomicInt#Memory
    ordering}{memory ordering} semantics, leaving the compiler and
    processor to freely reorder memory accesses.
*/

/*! \fn int QAtomicInt::fetchAndAddAcquire(int valueToAdd)

    Atomic fetch-and-add.

    Reads the current value of this QAtomicInt and then adds
    \a valueToAdd to the current value, returning the original value.

    This function uses \e acquire \l {QAtomicInt#Memory
    ordering}{memory ordering} semantics, which ensures that memory
    access following the atomic operation (in program order) may not
    be re-ordered before the atomic operation.
*/

/*! \fn int QAtomicInt::fetchAndAddRelease(int valueToAdd)

    Atomic fetch-and-add.

    Reads the current value of this QAtomicInt and then adds
    \a valueToAdd to the current value, returning the original value.

    This function uses \e release \l {QAtomicInt#Memory
    ordering}{memory ordering} semantics, which ensures that memory
    access before the atomic operation (in program order) may not be
    re-ordered after the atomic operation.
*/

/*! \fn int QAtomicInt::fetchAndAddOrdered(int valueToAdd)

    Atomic fetch-and-add.

    Reads the current value of this QAtomicInt and then adds
    \a valueToAdd to the current value, returning the original value.

    This function uses \e ordered \l {QAtomicInt#Memory
    ordering}{memory ordering} semantics, which ensures that memory
    access before and after the atomic operation (in program order)
    may not be re-ordered.
*/

/*!
    \macro Q_ATOMIC_INT_REFERENCE_COUNTING_IS_ALWAYS_NATIVE
    \relates QAtomicInt

    This macro is defined if and only if all generations of your
    processor support atomic reference counting.
*/

/*!
    \macro Q_ATOMIC_INT_REFERENCE_COUNTING_IS_SOMETIMES_NATIVE
    \relates QAtomicInt

    This macro is defined when only certain generations of the
    processor support atomic reference counting. Use the
    QAtomicInt::isReferenceCountingNative() function to check what
    your processor supports.
*/

/*!
    \macro Q_ATOMIC_INT_REFERENCE_COUNTING_IS_NOT_NATIVE
    \relates QAtomicInt

    This macro is defined when the hardware does not support atomic
    reference counting.
*/

/*!
    \macro Q_ATOMIC_INT_REFERENCE_COUNTING_IS_WAIT_FREE
    \relates QAtomicInt

    This macro is defined together with
    Q_ATOMIC_INT_REFERENCE_COUNTING_IS_ALWAYS_NATIVE to indicate that
    the reference counting is wait-free.
*/

/*!
    \macro Q_ATOMIC_INT_TEST_AND_SET_IS_ALWAYS_NATIVE
    \relates QAtomicInt

    This macro is defined if and only if your processor supports
    atomic test-and-set on integers.
*/

/*!
    \macro Q_ATOMIC_INT_TEST_AND_SET_IS_SOMETIMES_NATIVE
    \relates QAtomicInt

    This macro is defined when only certain generations of the
    processor support atomic test-and-set on integers. Use the
    QAtomicInt::isTestAndSetNative() function to check what your
    processor supports.
*/

/*!
    \macro Q_ATOMIC_INT_TEST_AND_SET_IS_NOT_NATIVE
    \relates QAtomicInt

    This macro is defined when the hardware does not support atomic
    test-and-set on integers.
*/

/*!
    \macro Q_ATOMIC_INT_TEST_AND_SET_IS_WAIT_FREE
    \relates QAtomicInt

    This macro is defined together with
    Q_ATOMIC_INT_TEST_AND_SET_IS_ALWAYS_NATIVE to indicate that the
    atomic test-and-set on integers is wait-free.
*/

/*!
    \macro Q_ATOMIC_INT_FETCH_AND_STORE_IS_ALWAYS_NATIVE
    \relates QAtomicInt

    This macro is defined if and only if your processor supports
    atomic fetch-and-store on integers.
*/

/*!
    \macro Q_ATOMIC_INT_FETCH_AND_STORE_IS_SOMETIMES_NATIVE
    \relates QAtomicInt

    This macro is defined when only certain generations of the
    processor support atomic fetch-and-store on integers. Use the
    QAtomicInt::isFetchAndStoreNative() function to check what your
    processor supports.
*/

/*!
    \macro Q_ATOMIC_INT_FETCH_AND_STORE_IS_NOT_NATIVE
    \relates QAtomicInt

    This macro is defined when the hardware does not support atomic
    fetch-and-store on integers.
*/

/*!
    \macro Q_ATOMIC_INT_FETCH_AND_STORE_IS_WAIT_FREE
    \relates QAtomicInt

    This macro is defined together with
    Q_ATOMIC_INT_FETCH_AND_STORE_IS_ALWAYS_NATIVE to indicate that the
    atomic fetch-and-store on integers is wait-free.
*/

/*!
    \macro Q_ATOMIC_INT_FETCH_AND_ADD_IS_ALWAYS_NATIVE
    \relates QAtomicInt

    This macro is defined if and only if your processor supports
    atomic fetch-and-add on integers.
*/

/*!
    \macro Q_ATOMIC_INT_FETCH_AND_ADD_IS_SOMETIMES_NATIVE
    \relates QAtomicInt

    This macro is defined when only certain generations of the
    processor support atomic fetch-and-add on integers. Use the
    QAtomicInt::isFetchAndAddNative() function to check what your
    processor supports.
*/

/*!
    \macro Q_ATOMIC_INT_FETCH_AND_ADD_IS_NOT_NATIVE
    \relates QAtomicInt

    This macro is defined when the hardware does not support atomic
    fetch-and-add on integers.
*/

/*!
    \macro Q_ATOMIC_INT_FETCH_AND_ADD_IS_WAIT_FREE
    \relates QAtomicInt

    This macro is defined together with
    Q_ATOMIC_INT_FETCH_AND_ADD_IS_ALWAYS_NATIVE to indicate that the
    atomic fetch-and-add on integers is wait-free.
*/




/*!
    \class QAtomicPointer
    \brief The QAtomicPointer class is a template class that provides platform-independent atomic operations on pointers.
    \since 4.4

    \ingroup thread

    For atomic operations on integers, see the QAtomicInt class.

    An \e atomic operation is a complex operation that completes without interruption.
    The QAtomicPointer class provides atomic test-and-set, fetch-and-store, and fetch-and-add for pointers.

    \section1 Non-atomic convenience operators

    For convenience, QAtomicPointer provides pointer comparison, cast,
    dereference, and assignment operators. Note that these operators
    are \e not atomic.

    \section1 The Atomic API

    \section2 Memory ordering

    QAtomicPointer provides several implementations of the atomic
    test-and-set, fetch-and-store, and fetch-and-add functions. Each
    implementation defines a memory ordering semantic that describes
    how memory accesses surrounding the atomic instruction are
    executed by the processor. Since many modern architectures allow
    out-of-order execution and memory ordering, using the correct
    semantic is necessary to ensure that your application functions
    properly on all processors.

    \list

    \o Relaxed - memory ordering is unspecified, leaving the compiler
    and processor to freely reorder memory accesses.

    \o Acquire - memory access following the atomic operation (in
    program order) may not be re-ordered before the atomic operation.

    \o Release - memory access before the atomic operation (in program
    order) may not be re-ordered after the atomic operation.

    \o Ordered - the same Acquire and Release semantics combined.

    \endlist

    \section2 Test-and-set

    If the current value of the QAtomicPointer is an expected value,
    the test-and-set functions assign a new value to the
    QAtomicPointer and return true. If values are \a not the same,
    these functions do nothing and return false. This operation
    equates to the following code:

    \snippet doc/src/snippets/code/src_corelib_thread_qatomic.cpp 4

    There are 4 test-and-set functions: testAndSetRelaxed(),
    testAndSetAcquire(), testAndSetRelease(), and
    testAndSetOrdered(). See above for an explanation of the different
    memory ordering semantics.

    \section2 Fetch-and-store

    The atomic fetch-and-store functions read the current value of the
    QAtomicPointer and then assign a new value, returning the original
    value. This operation equates to the following code:

    \snippet doc/src/snippets/code/src_corelib_thread_qatomic.cpp 5

    There are 4 fetch-and-store functions: fetchAndStoreRelaxed(),
    fetchAndStoreAcquire(), fetchAndStoreRelease(), and
    fetchAndStoreOrdered(). See above for an explanation of the
    different memory ordering semantics.

    \section2 Fetch-and-add

    The atomic fetch-and-add functions read the current value of the
    QAtomicPointer and then add the given value to the current value,
    returning the original value. This operation equates to the
    following code:

    \snippet doc/src/snippets/code/src_corelib_thread_qatomic.cpp 6

    There are 4 fetch-and-add functions: fetchAndAddRelaxed(),
    fetchAndAddAcquire(), fetchAndAddRelease(), and
    fetchAndAddOrdered(). See above for an explanation of the
    different memory ordering semantics.

    \section1 Feature Tests for the Atomic API

    Providing a platform-independent atomic API that works on all
    processors is challenging. The API provided by QAtomicPointer is
    guaranteed to work atomically on all processors. However, since
    not all processors implement support for every operation provided
    by QAtomicPointer, it is necessary to expose information about the
    processor.

    You can check at compile time which features are supported on your
    hardware using various macros. These will tell you if your
    hardware always, sometimes, or does not support a particular
    operation. The macros have the form
    Q_ATOMIC_POINTER_\e{OPERATION}_IS_\e{HOW}_NATIVE. \e{OPERATION} is
    one of TEST_AND_SET, FETCH_AND_STORE, or FETCH_AND_ADD, and
    \e{HOW} is one of ALWAYS, SOMETIMES, or NOT. There will always be
    exactly one defined macro per operation. For example, if
    Q_ATOMIC_POINTER_TEST_AND_SET_IS_ALWAYS_NATIVE is defined, neither
    Q_ATOMIC_POINTER_TEST_AND_SET_IS_SOMETIMES_NATIVE nor
    Q_ATOMIC_POINTER_TEST_AND_SET_IS_NOT_NATIVE will be defined.

    An operation that completes in constant time is said to be
    wait-free. Such operations are not implemented using locks or
    loops of any kind. For atomic operations that are always
    supported, and that are wait-free, Qt defines the
    Q_ATOMIC_POINTER_\e{OPERATION}_IS_WAIT_FREE in addition to the
    Q_ATOMIC_POINTER_\e{OPERATION}_IS_ALWAYS_NATIVE.

    In cases where an atomic operation is only supported in newer
    generations of the processor, QAtomicPointer also provides a way
    to check at runtime what your hardware supports with the
    isTestAndSetNative(), isFetchAndStoreNative(), and
    isFetchAndAddNative() functions. Wait-free implementations can be
    detected using the isTestAndSetWaitFree(),
    isFetchAndStoreWaitFree(), and isFetchAndAddWaitFree() functions.

    Below is a complete list of all feature macros for QAtomicPointer:

    \list

    \o Q_ATOMIC_POINTER_TEST_AND_SET_IS_ALWAYS_NATIVE
    \o Q_ATOMIC_POINTER_TEST_AND_SET_IS_SOMETIMES_NATIVE
    \o Q_ATOMIC_POINTER_TEST_AND_SET_IS_NOT_NATIVE
    \o Q_ATOMIC_POINTER_TEST_AND_SET_IS_WAIT_FREE

    \o Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_ALWAYS_NATIVE
    \o Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_SOMETIMES_NATIVE
    \o Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_NOT_NATIVE
    \o Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_WAIT_FREE

    \o Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_ALWAYS_NATIVE
    \o Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_SOMETIMES_NATIVE
    \o Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_NOT_NATIVE
    \o Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_WAIT_FREE

    \endlist

    \sa QAtomicInt
*/

/*! \fn QAtomicPointer::QAtomicPointer(T *value)

    Constructs a QAtomicPointer with the given \a value.
*/

/*! \fn QAtomicPointer::QAtomicPointer(const QAtomicPointer<T> &other)

    Constructs a copy of \a other.
*/

/*! \fn QAtomicPointer<T> &QAtomicPointer::operator=(T *value)

    Assigns the \a value to this QAtomicPointer and returns a
    reference to this QAtomicPointer.
*/

/*! \fn QAtomicPointer<T> &QAtomicPointer::operator=(const QAtomicPointer<T> &other)

    Assigns \a other to this QAtomicPointer and returns a reference to
    this QAtomicPointer.
*/

/*! \fn bool QAtomicPointer::operator==(T *value) const

    Returns true if the \a value is equal to the value in this
    QAtomicPointer; otherwise returns false.
*/

/*! \fn bool QAtomicPointer::operator!=(T *value) const

    Returns true if the value of this QAtomicPointer is not equal to
    \a value; otherwise returns false.
*/

/*! \fn bool QAtomicPointer::operator!() const

    Returns true is the current value of this QAtomicPointer is zero;
    otherwise returns false.
*/

/*! \fn QAtomicPointer::operator T *() const

    Returns the current pointer value stored by this QAtomicPointer
    object.
*/

/*! \fn T *QAtomicPointer::operator->() const

*/

/*! \fn bool QAtomicPointer::isTestAndSetNative()

    Returns true if test-and-set is implemented using atomic processor
    instructions, false otherwise.
*/

/*! \fn bool QAtomicPointer::isTestAndSetWaitFree()

    Returns true if atomic test-and-set is wait-free, false otherwise.
*/

/*! \fn bool QAtomicPointer::testAndSetRelaxed(T *expectedValue, T *newValue)

    Atomic test-and-set.

    If the current value of this QAtomicPointer is the \a expectedValue,
    the test-and-set functions assign the \a newValue to this
    QAtomicPointer and return true. If the values are \e not the same,
    this function does nothing and returns false.

    This function uses \e relaxed \l {QAtomicPointer#Memory
    ordering}{memory ordering} semantics, leaving the compiler and
    processor to freely reorder memory accesses.
*/

/*! \fn bool QAtomicPointer::testAndSetAcquire(T *expectedValue, T *newValue)

    Atomic test-and-set.

    If the current value of this QAtomicPointer is the \a expectedValue,
    the test-and-set functions assign the \a newValue to this
    QAtomicPointer and return true. If the values are \e not the same,
    this function does nothing and returns false.

    This function uses \e acquire \l {QAtomicPointer#Memory
    ordering}{memory ordering} semantics, which ensures that memory
    access following the atomic operation (in program order) may not
    be re-ordered before the atomic operation.
*/

/*! \fn bool QAtomicPointer::testAndSetRelease(T *expectedValue, T *newValue)

    Atomic test-and-set.

    If the current value of this QAtomicPointer is the \a expectedValue,
    the test-and-set functions assign the \a newValue to this
    QAtomicPointer and return true. If the values are \e not the same,
    this function does nothing and returns false.

    This function uses \e release \l {QAtomicPointer#Memory
    ordering}{memory ordering} semantics, which ensures that memory
    access before the atomic operation (in program order) may not be
    re-ordered after the atomic operation.
*/

/*! \fn bool QAtomicPointer::testAndSetOrdered(T *expectedValue, T *newValue)

    Atomic test-and-set.

    If the current value of this QAtomicPointer is the \a expectedValue,
    the test-and-set functions assign the \a newValue to this
    QAtomicPointer and return true. If the values are \e not the same,
    this function does nothing and returns false.

    This function uses \e ordered \l {QAtomicPointer#Memory
    ordering}{memory ordering} semantics, which ensures that memory
    access before and after the atomic operation (in program order)
    may not be re-ordered.
*/

/*! \fn bool QAtomicPointer::isFetchAndStoreNative()

    Returns true if fetch-and-store is implemented using atomic
    processor instructions, false otherwise.
*/

/*! \fn bool QAtomicPointer::isFetchAndStoreWaitFree()

    Returns true if atomic fetch-and-store is wait-free, false
    otherwise.
*/

/*! \fn T *QAtomicPointer::fetchAndStoreRelaxed(T *newValue)

    Atomic fetch-and-store.

    Reads the current value of this QAtomicPointer and then assigns it the
    \a newValue, returning the original value.

    This function uses \e relaxed \l {QAtomicPointer#Memory
    ordering}{memory ordering} semantics, leaving the compiler and
    processor to freely reorder memory accesses.
*/

/*! \fn T *QAtomicPointer::fetchAndStoreAcquire(T *newValue)

    Atomic fetch-and-store.

    Reads the current value of this QAtomicPointer and then assigns it the
    \a newValue, returning the original value.

    This function uses \e acquire \l {QAtomicPointer#Memory
    ordering}{memory ordering} semantics, which ensures that memory
    access following the atomic operation (in program order) may not
    be re-ordered before the atomic operation.
*/

/*! \fn T *QAtomicPointer::fetchAndStoreRelease(T *newValue)

    Atomic fetch-and-store.

    Reads the current value of this QAtomicPointer and then assigns it the
    \a newValue, returning the original value.

    This function uses \e release \l {QAtomicPointer#Memory
    ordering}{memory ordering} semantics, which ensures that memory
    access before the atomic operation (in program order) may not be
    re-ordered after the atomic operation.
*/

/*! \fn T *QAtomicPointer::fetchAndStoreOrdered(T *newValue)

    Atomic fetch-and-store.

    Reads the current value of this QAtomicPointer and then assigns it the
    \a newValue, returning the original value.

    This function uses \e ordered \l {QAtomicPointer#Memory
    ordering}{memory ordering} semantics, which ensures that memory
    access before and after the atomic operation (in program order)
    may not be re-ordered.
*/

/*! \fn bool QAtomicPointer::isFetchAndAddNative()

    Returns true if fetch-and-add is implemented using atomic
    processor instructions, false otherwise.
*/

/*! \fn bool QAtomicPointer::isFetchAndAddWaitFree()

    Returns true if atomic fetch-and-add is wait-free, false
    otherwise.
*/

/*! \fn T *QAtomicPointer::fetchAndAddRelaxed(qptrdiff valueToAdd)

    Atomic fetch-and-add.

    Reads the current value of this QAtomicPointer and then adds
    \a valueToAdd to the current value, returning the original value.

    This function uses \e relaxed \l {QAtomicPointer#Memory
    ordering}{memory ordering} semantics, leaving the compiler and
    processor to freely reorder memory accesses.
*/

/*! \fn T *QAtomicPointer::fetchAndAddAcquire(qptrdiff valueToAdd)

    Atomic fetch-and-add.

    Reads the current value of this QAtomicPointer and then adds
    \a valueToAdd to the current value, returning the original value.

    This function uses \e acquire \l {QAtomicPointer#Memory
    ordering}{memory ordering} semantics, which ensures that memory
    access following the atomic operation (in program order) may not
    be re-ordered before the atomic operation.
*/

/*! \fn T *QAtomicPointer::fetchAndAddRelease(qptrdiff valueToAdd)

    Atomic fetch-and-add.

    Reads the current value of this QAtomicPointer and then adds
    \a valueToAdd to the current value, returning the original value.

    This function uses \e release \l {QAtomicPointer#Memory
    ordering}{memory ordering} semantics, which ensures that memory
    access before the atomic operation (in program order) may not be
    re-ordered after the atomic operation.
*/

/*! \fn T *QAtomicPointer::fetchAndAddOrdered(qptrdiff valueToAdd)

    Atomic fetch-and-add.

    Reads the current value of this QAtomicPointer and then adds
    \a valueToAdd to the current value, returning the original value.

    This function uses \e ordered \l {QAtomicPointer#Memory
    ordering}{memory ordering} semantics, which ensures that memory
    access before and after the atomic operation (in program order)
    may not be re-ordered.
*/

/*!
    \macro Q_ATOMIC_POINTER_TEST_AND_SET_IS_ALWAYS_NATIVE
    \relates QAtomicPointer

    This macro is defined if and only if your processor supports
    atomic test-and-set on pointers.
*/

/*!
    \macro Q_ATOMIC_POINTER_TEST_AND_SET_IS_SOMETIMES_NATIVE
    \relates QAtomicPointer

    This macro is defined when only certain generations of the
    processor support atomic test-and-set on pointers. Use the
    QAtomicPointer::isTestAndSetNative() function to check what your
    processor supports.
*/

/*!
    \macro Q_ATOMIC_POINTER_TEST_AND_SET_IS_NOT_NATIVE
    \relates QAtomicPointer

    This macro is defined when the hardware does not support atomic
    test-and-set on pointers.
*/

/*!
    \macro Q_ATOMIC_POINTER_TEST_AND_SET_IS_WAIT_FREE
    \relates QAtomicPointer

    This macro is defined together with
    Q_ATOMIC_POINTER_TEST_AND_SET_IS_ALWAYS_NATIVE to indicate that
    the atomic test-and-set on pointers is wait-free.
*/

/*!
    \macro Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_ALWAYS_NATIVE
    \relates QAtomicPointer

    This macro is defined if and only if your processor supports
    atomic fetch-and-store on pointers.
*/

/*!
    \macro Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_SOMETIMES_NATIVE
    \relates QAtomicPointer

    This macro is defined when only certain generations of the
    processor support atomic fetch-and-store on pointers. Use the
    QAtomicPointer::isFetchAndStoreNative() function to check what
    your processor supports.
*/

/*!
    \macro Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_NOT_NATIVE
    \relates QAtomicPointer

    This macro is defined when the hardware does not support atomic
    fetch-and-store on pointers.
*/

/*!
    \macro Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_WAIT_FREE
    \relates QAtomicPointer

    This macro is defined together with
    Q_ATOMIC_POINTER_FETCH_AND_STORE_IS_ALWAYS_NATIVE to indicate that
    the atomic fetch-and-store on pointers is wait-free.
*/

/*!
    \macro Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_ALWAYS_NATIVE
    \relates QAtomicPointer

    This macro is defined if and only if your processor supports
    atomic fetch-and-add on pointers.
*/

/*!
    \macro Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_SOMETIMES_NATIVE
    \relates QAtomicPointer

    This macro is defined when only certain generations of the
    processor support atomic fetch-and-add on pointers. Use the
    QAtomicPointer::isFetchAndAddNative() function to check what your
    processor supports.
*/

/*!
    \macro Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_NOT_NATIVE
    \relates QAtomicPointer

    This macro is defined when the hardware does not support atomic
    fetch-and-add on pointers.
*/

/*!
    \macro Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_WAIT_FREE
    \relates QAtomicPointer

    This macro is defined together with
    Q_ATOMIC_POINTER_FETCH_AND_ADD_IS_ALWAYS_NATIVE to indicate that
    the atomic fetch-and-add on pointers is wait-free.
*/
