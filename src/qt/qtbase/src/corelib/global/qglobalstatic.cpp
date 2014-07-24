/****************************************************************************
**
** Copyright (C) 2013 Intel Corporation.
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

#include "qglobalstatic.h"

/*!
    \macro Q_GLOBAL_STATIC(Type, VariableName)
    \since 5.1
    \relates QGlobalStatic

    Creates a global and static object of type \l QGlobalStatic, of name \a
    VariableName and that behaves as a pointer to \a Type. The object created
    by Q_GLOBAL_STATIC initializes itself on the first use, which means that it
    will not increase the application or the library's load time. Additionally,
    the object is initialized in a thread-safe manner on all platforms.

    The typical use of this macro is as follows, in a global context (that is,
    outside of any function bodies):

    \code
        Q_GLOBAL_STATIC(MyType, staticType)
    \endcode

    This macro is intended to replace global static objects that are not POD
    (Plain Old Data, or in C++11 terms, not made of a trivial type), hence the
    name. For example, the following C++ code creates a global static:

    \code
        static MyType staticType;
    \endcode

    Compared to Q_GLOBAL_STATIC, and assuming that \c MyType is a class or
    struct that has a constructor, a destructor, or is otherwise non-POD, the
    above has the following drawbacks:

    \list
        \li it requires load-time initialization of \c MyType (that is, the
        default constructor for \c MyType is called when the library or
        application is loaded);

        \li the type will be initialized even if it is never used;

        \li the order of initialization and destruction among different
        translation units is not determined, leading to possible uses before
        initialization or after destruction;

        \li if it is found inside a function (that is, not global), it will be
        initialized on first use, but many current compilers (as of 2013) do
        not guarantee that the initialization will be thread-safe;
    \endlist

    The Q_GLOBAL_STATIC macro solves all of the above problems by guaranteeing
    thread-safe initialization on first use and allowing the user to query for
    whether the type has already been destroyed, to avoid the
    use-after-destruction problem (see QGlobalStatic::isDestroyed()).

    \section1 Constructor and destructor

    For Q_GLOBAL_STATIC, the type \c Type must be publicly
    default-constructible and publicly destructible. For
    Q_GLOBAL_STATIC_WITH_ARGS(), there must be a public constructor that
    matches the arguments passed.

    It is not possible to use Q_GLOBAL_STATIC with types that have protected or
    private default constructors or destructors (for Q_GLOBAL_STATIC_WITH_ARGS(),
    a protected or private constructor matching the arguments). If the type in
    question has those members as protected, it is possible to overcome the
    issue by deriving from the type and creating public a constructor and
    destructor. If the type has them as private, a friend declaration is
    necessary before deriving.

    For example, the following is enough to create \c MyType based on a
    previously-defined \c MyOtherType which has a protected default constructor
    and/or a protected destructor (or has them as private, but that defines \c
    MyType as a friend).

    \code
        class MyType : public MyOtherType { };
        Q_GLOBAL_STATIC(MyType, staticType)
    \endcode

    No body for \c MyType is required since the destructor is an implicit
    member and so is the default constructor if no other constructors are
    defined. For use with Q_GLOBAL_STATIC_WITH_ARGS(), however, a suitable
    constructor body is necessary:

    \code
        class MyType : public MyOtherType
        {
        public:
            MyType(int i) : MyOtherType(i) {}
        };
        Q_GLOBAL_STATIC_WITH_ARGS(MyType, staticType, (42))
   \endcode

   Alternatively, if the compiler supports C++11 inheriting constructors, one could write:

    \code
        class MyType : public MyOtherType
        {
        public:
            using MyOtherType::MyOtherType;
        };
        Q_GLOBAL_STATIC_WITH_ARGS(MyType, staticType, (42))
    \endcode

    \section1 Placement

    The Q_GLOBAL_STATIC macro creates a type that is necessarily static, at the
    global scope. It is not possible to place the Q_GLOBAL_STATIC macro inside
    a function (doing so will result in compilation errors).

    More importantly, this macro should be placed in source files, never in
    headers. Since the resulting object is has static linkage, if the macro is
    placed in a header and included by multiple source files, the object will
    be defined multiple times and will not cause linking errors. Instead, each
    translation unit will refer to a different object, which could lead to
    subtle and hard-to-track errors.

    \section1 Non-recommended uses

    Note that the macro is not recommended for use with types that are POD or
    that have C++11 constexpr constructors (trivially constructible and
    destructible). For those types, it is still recommended to use regular
    static, whether global or function-local.

    This macro will work, but it will add unnecessary overhead.

    \section1 Reentrancy, thread-safety, deadlocks, and exception-safety on construction

    The Q_GLOBAL_STATIC macro creates an object that initializes itself on
    first use in a thread-safe manner: if multiple threads attempt to
    initialize the object at the same time, only one thread will proceed to
    initialize, while all other threads wait for completion.

    If the initialization process throws an exception, the initialization is
    deemed not complete and will be attempted again when control reaches any
    use of the object. If there are any threads waiting for initialization, one
    of them will be woken up to attempt to initialize.

    The macro makes no guarantee about reentrancy from the same thread. If the
    global static object is accessed directly or indirectly from inside the
    constructor, a deadlock will surely happen.

    In addition, if two Q_GLOBAL_STATIC objects are being initialized on two
    different threads and each one's initialization sequence accesses the
    other, a deadlock might happen. For that reason, it is recommended to keep
    global static constructors simple or, failing that, to ensure that there's
    no cross-dependency of uses of global static during construction.

    \section1 Destruction

    If the object is never used during the lifetime of the program, aside from
    the QGlobalStatic::exists() and QGlobalStatic::isDestroyed() functions, the
    contents of type \a Type will not be created and there will not be any
    exit-time operation.

    If the object is created, it will be destroyed at exit-time, similar to the
    C \c atexit function. On most systems, in fact, the destructor will also be
    called if the library or plugin is unloaded from memory before exit.

    Since the destruction is meant to happen at program exit, no thread-safety
    is provided. This includes the case of plugin or library unload. In
    addition, since destructors are not supposed to throw exceptions, no
    exception safety is provided either.

    However, reentrancy is permitted: during destruction, it is possible to
    access the global static object and the pointer returned will be the same
    as it was before destruction began. After the destruction has completed,
    accessing the global static object is not permitted, except as noted in the
    \l QGlobalStatic API.

    \omit
    \section1 Compatibility with Qt 4 and Qt 5.0

    This macro, in its current form and behavior, was introduced in Qt 5.1.
    Prior to that version, Qt had another macro with the same name that was
    private API. This section is not meant to document how to use
    Q_GLOBAL_STATIC in those versions, but instead to serve as a porting guide
    for Qt code that used those macros.

    The Qt 4 Q_GLOBAL_STATIC macro differed in behavior in the following ways:

    \list
        \li the object created was not of type \l QGlobalStatic, but instead
        it was a function that returned a pointer to \a Type; that means the
        \l QGlobalStatic API was not present;

        \li the initialization was thread-safe, but not guaranteed to be
        unique: instead, if N threads tried to initialize the object at the
        same time, N objects would be created on the heap and N-1 would be
        destroyed;

        \li the object was always created on the heap.
    \endlist

    \section1 Implementation details

    Q_GLOBAL_STATIC is implemented by creating a QBasicAtomicInt called the \c
    guard and a free, inline function called \c innerFunction. The guard
    variable is initialized to value 0 (chosen so that the guard can be placed
    in the .bss section of the binary file), which denotes that construction
    has not yet taken place (uninitialized). The inner function is implemented
    by the helper macro Q_GLOBAL_STATIC_INTERNAL.

    Both the guard variable and the inner function are passed as template
    parameters to QGlobalStatic, along with the type \a Type. Both should also
    have static linkage or be placed inside an anonymous namespace, so that the
    visibility of Q_GLOBAL_STATIC is that of a global static. To permit
    multiple Q_GLOBAL_STATIC per translation unit, the guard variable and the
    inner function must have unique names, which can be accomplished by
    concatenating with \a VariableName or by placing them in a namespace that
    has \a VariableName as part of the name. To simplify and improve
    readability on Q_GLOBAL_STATIC_INTERNAL, we chose the namespace solution.
    It's also required for C++98 builds, since static symbols cannot be used as
    template parameters.

    The guard variable can assume the following values:

    \list
        \li -2: object was once initialized but has been destroyed already;
        \li -1: object was initialized and is still valid;
        \li  0: object was not initialized yet;
        \li +1: object is being initialized and any threads encountering this
        value must wait for completion (not used in the current implementation).
    \endlist

    Collectively, all positive values indicate that the initialization is
    progressing and must be waited on, whereas all negative values indicate
    that the initialization has terminated and must not be attempted again.
    Positive values are not used in the current implementation, but were in
    earlier versions. They could be used again in the future.

    The QGlobalStatic::exists() and QGlobalStatic::isDestroyed() functions
    operate solely on the guard variable: the former returns \c true if the guard
    is negative, whereas the latter returns \c true only if it is -2.

    The Q_GLOBAL_STATIC_INTERNAL macro implements the actual construction and
    destruction. There are two implementations of it: one for compilers that
    support thread-safe initialization of function-local statics and one for
    compilers that don't. Thread-safe initialization is required by C++11 in
    [stmt.decl], but as of the time of this writing, only compilers based on
    the IA-64 C++ ABI implemented it properly. The implementation requiring
    thread-safe initialization is also used on the Qt bootstrapped tools, which
    define QT_NO_THREAD.

    The implementation requiring thread-safe initialization from the compiler
    is the simplest: it creates the \a Type object as a function-local static
    and returns its address. The actual object is actually inside a holder
    structure so holder's destructor can set the guard variable to the value -2
    (destroyed) when the type has finished destruction. Since we need to set
    the guard \b after the destruction has finished, this code needs to be in a
    base struct's destructor. And it only sets to -2 (destroyed) if it finds
    the guard at -1 (initialized): this is done to ensure that the guard isn't
    set to -2 in the event the type's constructor threw an exception. A holder
    structure is used to avoid creating two statics, which the ABI might
    require duplicating the thread-safe control structures for.

    The other implementation is similar to Qt 4's Q_GLOBAL_STATIC, but unlike
    that one, it uses a \l QBasicMutex to provide locking. It is also more
    efficient memory-wise. It use a simple double-checked locking of the mutex
    and then creates the contents on the heap. After that, it creates a
    function-local structure called "Cleanup", whose destructor will be run at
    program exit and will actually destroy the contents.

    \endomit

    \sa Q_GLOBAL_STATIC_WITH_ARGS(), QGlobalStatic
*/

/*!
    \macro Q_GLOBAL_STATIC_WITH_ARGS(Type, VariableName, Arguments)
    \since 5.1
    \relates QGlobalStatic

    Creates a global and static object of type \l QGlobalStatic, of name \a
    VariableName, initialized by the arguments \a Arguments and that behaves as
    a pointer to \a Type. The object created by Q_GLOBAL_STATIC_WITH_ARGS
    initializes itself on the first use, which means that it will not increase
    the application or the library's load time. Additionally, the object is
    initialized in a thread-safe manner on all platforms.

    The typical use of this macro is as follows, in a global context (that is,
    outside of any function bodies):

    \code
        Q_GLOBAL_STATIC_WITH_ARGS(MyType, staticType, (42, "Hello", "World"))
    \endcode

    The \a Arguments macro parameter must always include the parentheses or, if
    C++11 uniform initialization is allowed, the braces.

    Aside from the actual initialization of the contents with the supplied
    arguments, this macro behaves identically to Q_GLOBAL_STATIC(). Please
    see that macro's documentation for more information.

    \sa Q_GLOBAL_STATIC(), QGlobalStatic
*/

/*!
    \class QGlobalStatic
    \threadsafe
    \inmodule QtCore
    \since 5.1
    \brief The QGlobalStatic class is used to implement a global static object

    The QGlobalStatic class is the front-end API exported when
    Q_GLOBAL_STATIC() is used. See the documentation for the macro for a
    discussion on when to use it and its requirements.

    Normally, you will never use this class directly, but instead you will use
    the Q_GLOBAL_STATIC() or Q_GLOBAL_STATIC_WITH_ARGS() macros, as
    follows:

    \code
        Q_GLOBAL_STATIC(MyType, staticType)
    \endcode

    The above example creates an object of type QGlobalStatic called \c
    staticType. After the above declaration, the \c staticType object may be
    used as if it were a pointer, guaranteed to be initialized exactly once. In
    addition to the use as a pointer, the object offers two methods to
    determine the current status of the global: exists() and isDestroyed().

    \sa Q_GLOBAL_STATIC(), Q_GLOBAL_STATIC_WITH_ARGS()
*/

/*!
    \typedef QGlobalStatic::Type

    This type is equivalent to the \c Type parameter passed to the
    Q_GLOBAL_STATIC() or Q_GLOBAL_STATIC_WITH_ARGS() macros. It is used in the
    return types of some functions.
*/

/*!
    \fn bool QGlobalStatic::isDestroyed() const

    This function returns \c true if the global static object has already
    completed destruction (that is, if the destructor for the type has already
    returned). In specific, note that this function returns \c false if
    the destruction is still in progress.

    Once this function has returned true once, it will never return
    false again until either the program is restarted or the plugin or library
    containing the global static is unloaded and reloaded.

    This function is safe to call at any point in the program execution: it
    cannot fail and cannot cause a deadlock. Additionally, it will not cause
    the contents to be created if they have not yet been created.

    This function is useful in code that may be executed at program shutdown,
    to determine whether the contents may still be accessed or not.

    \omit
    Due to the non-atomic nature of destruction, it's possible that
    QGlobalStatic::isDestroyed might return false for a short time after the
    destructor has finished. However, since the destructor is only run in an
    environment where concurrent multithreaded access is impossible, no one can
    see that state. (omitted because it's useless information)
    \endomit

    \sa exists()
*/

/*!
    \fn bool QGlobalStatic::exists() const

    This function returns \c true if the global static object has already
    completed initialization (that is, if the constructor for the type has
    already returned). In specific, note that this function returns \c false if
    the initialization is still in progress.

    Once this function has returned true once, it will never return false again
    until either the program is restarted or the plugin or library containing
    the global static is unloaded and reloaded.

    This function is safe to call at any point in the program execution: it
    cannot fail and cannot cause a deadlock. Additionally, it will not cause
    the contents to be created if they have not yet been created.

    This function is useful if one can determine the initial conditions of the
    global static object and would prefer to avoid a possibly expensive
    construction operation.

    For example, in the following code sample, this function is used to
    short-circuit the creation of the global static called \c globalState and
    returns a default value:

    \code
        Q_GLOBAL_STATIC(MyType, globalState)
        QString someState()
        {
            if (globalState.exists())
                return globalState->someState;
            return QString();
        }
    \endcode

    \b{Thread-safety notice:} this function is thread-safe in the sense that it
    may be called from any thread at any time and will always return a valid
    reply. But due to the non-atomic nature of construction, this function may
    return false for a short time after the construction has completed.

    \b{Memory ordering notice:} this function does not impose any memory
    ordering guarantees. That is instead provided by the accessor functions
    that return the pointer or reference to the contents. If you bypass the
    accessor functions and attempt to access some global state set by the
    constructor, be sure to use the correct memory ordering semantics provided
    by \l QAtomicInt or \l QAtomicPointer.

    \sa isDestroyed()
*/

/*!
    \fn QGlobalStatic::operator Type*()

    This function returns the address of the contents of this global static. If
    the contents have not yet been created, they will be created thread-safely
    by this function. If the contents have already been destroyed, this
    function will return a null pointer.

    This function can be used, for example, to store the pointer to the
    contents of the global static in a local variable, thus avoiding multiple
    calls to the function. The implementation of Q_GLOBAL_STATIC() is quite
    efficient already, but in performance-critical sections it might be useful
    to help the compiler a little. For example:

    \code
        Q_GLOBAL_STATIC(MyType, globalState)
        QString someState()
        {
            MyType *state = globalState;
            if (!state) {
                // we're in a post-destruction state
                return QString();
            }
            if (state->condition)
                return state->value1;
            else
                return state->value2;
        }
    \endcode

    \sa operator->(), operator*()
*/

/*!
    \fn Type *QGlobalStatic::operator()()
    \deprecated

    This function returns the address of the contents of this global static. If
    the contents have not yet been created, they will be created thread-safely
    by this function. If the contents have already been destroyed, this
    function will return a null pointer.

    This function is equivalent to \l {operator Type *()}. It is provided for
    compatibility with the private Q_GLOBAL_STATIC implementation that existed
    in Qt 4.x and 5.0. New code should avoid using it and should instead treat
    the object as a smart pointer.
*/

/*!
    \fn Type *QGlobalStatic::operator->()

    This function returns the address of the contents of this global static. If
    the contents have not yet been created, they will be created thread-safely
    by this function.

    This function does not check if the contents have already been destroyed and
    will never return null. If this function is called after the object has
    been destroyed, it will return a dangling pointer that should not be
    dereferenced.
*/

/*!
    \fn Type &QGlobalStatic::operator*()

    This function returns a reference to the contents of this global static. If
    the contents have not yet been created, they will be created thread-safely
    by this function.

    This function does not check if the contents have already been destroyed.
    If this function is called after the object has been destroyed, it will
    return an invalid reference that must not be used.
*/
