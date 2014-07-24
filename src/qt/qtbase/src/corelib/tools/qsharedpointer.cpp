/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qsharedpointer.h"

// to be sure we aren't causing a namespace clash:
#include "qshareddata.h"

/*!
    \class QSharedPointer
    \inmodule QtCore
    \brief The QSharedPointer class holds a strong reference to a shared pointer
    \since 4.5

    \reentrant

    The QSharedPointer is an automatic, shared pointer in C++. It
    behaves exactly like a normal pointer for normal purposes,
    including respect for constness.

    QSharedPointer will delete the pointer it is holding when it goes
    out of scope, provided no other QSharedPointer objects are
    referencing it.

    A QSharedPointer object can be created from a normal pointer,
    another QSharedPointer object or by promoting a
    QWeakPointer object to a strong reference.

    \section1 Thread-Safety

    QSharedPointer and QWeakPointer are thread-safe and operate
    atomically on the pointer value. Different threads can also access
    the QSharedPointer or QWeakPointer pointing to the same object at
    the same time without need for locking mechanisms.

    It should be noted that, while the pointer value can be accessed
    in this manner, QSharedPointer and QWeakPointer provide no
    guarantee about the object being pointed to. Thread-safety and
    reentrancy rules for that object still apply.

    \section1 Other Pointer Classes

    Qt also provides two other pointer wrapper classes: QPointer and
    QSharedDataPointer. They are incompatible with one another, since
    each has its very different use case.

    QSharedPointer holds a shared pointer by means of an external
    reference count (i.e., a reference counter placed outside the
    object). Like its name indicates, the pointer value is shared
    among all instances of QSharedPointer and QWeakPointer. The
    contents of the object pointed to by the pointer should not be
    considered shared, however: there is only one object. For that
    reason, QSharedPointer does not provide a way to detach or make
    copies of the pointed object.

    QSharedDataPointer, on the other hand, holds a pointer to shared
    data (i.e., a class derived from QSharedData). It does so by means
    of an internal reference count, placed in the QSharedData base
    class. This class can, therefore, detach based on the type of
    access made to the data being guarded: if it's a non-const access,
    it creates a copy atomically for the operation to complete.

    QExplicitlySharedDataPointer is a variant of QSharedDataPointer, except
    that it only detaches if QExplicitlySharedDataPointer::detach() is
    explicitly called (hence the name).

    QScopedPointer simply holds a pointer to a heap allocated object and
    deletes it in its destructor. This class is useful when an object needs to
    be heap allocated and deleted, but no more. QScopedPointer is lightweight,
    it makes no use of additional structure or reference counting.

    Finally, QPointer holds a pointer to a QObject-derived object, but it
    does so weakly. QWeakPointer has the same functionality, but its use for
    that function is deprecated.

    \section1 Optional pointer tracking

    A feature of QSharedPointer that can be enabled at compile-time for
    debugging purposes is a pointer tracking mechanism. When enabled,
    QSharedPointer registers in a global set all the pointers that it tracks.
    This allows one to catch mistakes like assigning the same pointer to two
    QSharedPointer objects.

    This function is enabled by defining the \tt{QT_SHAREDPOINTER_TRACK_POINTERS}
    macro before including the QSharedPointer header.

    It is safe to use this feature even with code compiled without the
    feature. QSharedPointer will ensure that the pointer is removed from the
    tracker even from code compiled without pointer tracking.

    Note, however, that the pointer tracking feature has limitations on
    multiple- or virtual-inheritance (that is, in cases where two different
    pointer addresses can refer to the same object). In that case, if a
    pointer is cast to a different type and its value changes,
    QSharedPointer's pointer tracking mechanism may fail to detect that the
    object being tracked is the same.

    \omit
    \secton1 QSharedPointer internals

    QSharedPointer has two "private" members: the pointer itself being tracked
    and a d-pointer. Those members are private to the class, but QSharedPointer
    is friends with QWeakPointer and other QSharedPointer with different
    template arguments. (On some compilers, template friends are not supported,
    so the members are technically public)

    The reason for keeping the pointer value itself outside the d-pointer is
    because of multiple inheritance needs. If you have two QSharedPointer
    objects of different pointer types, but pointing to the same object in
    memory, it could happen that the pointer values are different. The \tt
    differentPointers autotest exemplifies this problem. The same thing could
    happen in the case of virtual inheritance: a pointer of class matching
    the virtual base has different address compared to the pointer of the
    complete object. See the \tt virtualBaseDifferentPointers autotest for
    this problem.

    The d pointer is a pointer to QtSharedPointer::ExternalRefCountData, but it
    always points to one of the two classes derived from ExternalRefCountData.

    \section2 d-pointer
    \section3 QtSharedPointer::ExternalRefCountData

    It is basically a reference-counted reference-counter plus a pointer to the
    function to be used to delete the pointer. It has three members: \tt
    strongref, \tt weakref, and \tt destroyer. The strong reference counter is
    controlling the lifetime of the object tracked by QSharedPointer. A
    positive value indicates that the object is alive. It's also the number of
    QSharedObject instances that are attached to this Data.

    When the strong reference count decreases to zero, the object is deleted
    (see below for information on custom deleters). The strong reference count
    can also exceptionally be -1, indicating that there are no QSharedPointers
    attached to an object, which is tracked too. The only case where this is
    possible is that of QWeakPointers and QPointers tracking a QObject. Note
    that QWeakPointers tracking a QObject is a deprecated feature as of Qt 5.0,
    kept only for compatibility with Qt 4.x.

    The weak reference count controls the lifetime of the d-pointer itself.
    It can be thought of as an internal/intrusive reference count for
    ExternalRefCountData itself. This count is equal to the number of
    QSharedPointers and QWeakPointers that are tracking this object. In case
    the object is a QObject being tracked by QPointer, this number is increased
    by 1, since QObjectPrivate tracks it too.

    The third member is a pointer to the function that is used to delete the
    pointer being tracked. That happens when the destroy() function is called.

    The size of this class is the size of the two atomic ints plus the size of
    a pointer. On 32-bit architectures, that's 12 bytes, whereas on 64-bit ones
    it's 16 bytes. There is no padding.

    \section3 QtSharedPointer::ExternalRefCountWithCustomDeleter

    This class derives from ExternalRefCountData and is a template class. As
    template parameters, it has the type of the pointer being tracked (\tt T)
    and a \tt Deleter, which is anything. It adds two fields to its parent
    class, matching those template parameters: a member of type \tt Deleter and
    a member of type \tt T*. Those members are actually inside a template
    struct of type CustomDeleter, which is partially-specialized for normal
    deletion. See below for more details on that.

    The purpose of this class is to store the pointer to be deleted and the
    deleter code along with the d-pointer. This allows the last strong
    reference to call any arbitrary function that disposes of the object. For
    example, this allows calling QObject::deleteLater() on a given object.
    The pointer to the object is kept here because it needs to match the actual
    deleter function's parameters, regardless of what template argument the
    last QSharedPointer instance had.

    This class is never instantiated directly: the constructors and
    destructor are private and, in C++11, deleted. Only the create() function
    may be called to return an object of this type. See below for construction
    details.

    The size of this class depends on the size of \tt Deleter. If it's an empty
    functor (i.e., no members), ABIs generally assign it the size of 1. But
    given that it's followed by a pointer, padding bytes may be inserted so
    that the alignment of the class and of the pointer are correct. In that
    case, the size of this class is 12+4+4 = 20 bytes on 32-bit architectures,
    or 16+8+8 = 40 bytes on 64-bit architectures. If \tt Deleter is a function
    pointer, the size should be the same as the empty structure case. If \tt
    Deleter is a pointer to a member function (PMF), the size will be bigger
    and will depend on the ABI. For architectures using the Itanium C++ ABI, a
    PMF is twice the size of a normal pointer. In that case, the size of this
    structure will be 12+8+4 = 24 bytes on 32-bit architectures, 16+16+8 = 40
    bytes on 64-bit ones.

    If the deleter was not specified when creating the QSharedPointer object
    (i.e., if a standard \tt delete call is expected), then there's an
    optimization that avoids the need to store another function pointer in
    ExternalRefCountWithCustomDeleter. Instead, a template specialization makes
    a direct delete call. The size of the structure, in this case, is 12+4 = 16
    bytes on 32-bit architectures, 16+8 = 24 bytes on 64-bit ones.

    \section3 QtSharedPointer::ExternalRefCountWithContiguousData

    This class also derives from ExternalRefCountData and it is
    also a template class. The template parameter is the type \tt T of the
    class which QSharedPointer tracks. It adds only one member to its parent,
    which is of type \tt T (the actual type, not a pointer to it).

    The purpose of this class is to lay the \tt T object out next to the
    reference counts, saving one memory allocation per shared pointer. This
    is particularly interesting for small \tt T or for the cases when there
    are few if any QWeakPointer tracking the object. This class exists to
    implement the QSharedPointer::create() call.

    Like ExternalRefCountWithCustomDeleter, this class is never instantiated
    directly. This class also provides a create() member that returns the
    pointer, and hides its constructors and destructor. With C++11, they're
    deleted.

    The size of this class depends on the size of \tt T.

    \section3 Instantiating ExternalRefCountWithCustomDeleter and ExternalRefCountWithContiguousData

    Like explained above, these classes have private constructors. Moreover,
    they are not defined anywhere, so trying to call \tt{new ClassType} would
    result in a compilation or linker error. Instead, these classes must be
    constructed via their create() methods.

    Instead of instantiating the class by the normal way, the create() method
    calls \tt{operator new} directly with the size of the class, then calls
    the parent class's constructor only (that is, ExternalRefCountData's constructor).
    This ensures that the inherited members are initialised properly.

    After initialising the base class, the
    ExternalRefCountWithCustomDeleter::create() function initialises the new
    members directly, by using the placement \tt{operator new}. In the case
    of the ExternalRefCountWithContiguousData::create() function, the address
    to the still-uninitialised \tt T member is saved for the callee to use.
    The member is only initialised in QSharedPointer::create(), so that we
    avoid having many variants of the internal functions according to the
    arguments in use for calling the constructor.

    When initialising the parent class, the create() functions pass the
    address of the static deleter() member function. That is, when the
    destroy() function is called by QSharedPointer, the deleter() functions
    are called instead. These functions static_cast the ExternalRefCountData*
    parameter to their own type and execute their deletion: for the
    ExternalRefCountWithCustomDeleter::deleter() case, it runs the user's
    custom deleter, then destroys the deleter; for
    ExternalRefCountWithContiguousData::deleter, it simply calls the \tt T
    destructor directly.

    Only one non-inline function is required per template, which is
    the deleter() static member. All the other functions can be inlined.
    What's more, the address of deleter() is calculated only in code, which
    can be resolved at link-time if the linker can determine that the
    function lies in the current application or library module (since these
    classes are not exported, that is the case for Windows or for builds with
    \tt{-fvisibility=hidden}).

    \section3 Modifications due to pointer-tracking

    To ensure that pointers created with pointer-tracking enabled get
    un-tracked when destroyed, even if destroyed by code compiled without the
    feature, QSharedPointer modifies slightly the instructions of the
    previous sections.

    When ExternalRefCountWithCustomDeleter or
    ExternalRefCountWithContiguousData are used, their create() functions
    will set the ExternalRefCountData::destroyer function
    pointer to safetyCheckDeleter() instead. These static member functions
    simply call internalSafetyCheckRemove() before passing control to the
    normal deleter() function.

    If neither custom deleter nor QSharedPointer::create() are used, then
    QSharedPointer uses a custom deleter of its own: the normalDeleter()
    function, which simply calls \tt delete. By using a custom deleter, the
    safetyCheckDeleter() procedure described above kicks in.

    \endomit

    \sa QSharedDataPointer, QWeakPointer, QScopedPointer
*/

/*!
    \class QWeakPointer
    \inmodule QtCore
    \brief The QWeakPointer class holds a weak reference to a shared pointer
    \since 4.5
    \reentrant

    The QWeakPointer is an automatic weak reference to a
    pointer in C++. It cannot be used to dereference the pointer
    directly, but it can be used to verify if the pointer has been
    deleted or not in another context.

    QWeakPointer objects can only be created by assignment from a
    QSharedPointer.

    It's important to note that QWeakPointer provides no automatic casting
    operators to prevent mistakes from happening. Even though QWeakPointer
    tracks a pointer, it should not be considered a pointer itself, since it
    doesn't guarantee that the pointed object remains valid.

    Therefore, to access the pointer that QWeakPointer is tracking, you must
    first promote it to QSharedPointer and verify if the resulting object is
    null or not. QSharedPointer guarantees that the object isn't deleted, so
    if you obtain a non-null object, you may use the pointer. See
    QWeakPointer::toStrongRef() for an example.

    QWeakPointer also provides the QWeakPointer::data() method that returns
    the tracked pointer without ensuring that it remains valid. This function
    is provided if you can guarantee by external means that the object will
    not get deleted (or if you only need the pointer value) and the cost of
    creating a QSharedPointer using toStrongRef() is too high.

    \omit
    \secton1 QWeakPointer internals

    QWeakPointer shares most of its internal functionality with
    \l{QSharedPointer#qsharedpointer-internals}{QSharedPointer}, so see that
    class's internal documentation for more information.

    QWeakPointer requires an external reference counter in order to operate.
    Therefore, it is incompatible by design with \l QSharedData-derived
    classes.

    It has a special QObject constructor, which works by calling
    QtSharedPointer::ExternalRefCountData::getAndRef, which retrieves the
    d-pointer from QObjectPrivate. If one isn't set yet, that function
    creates the d-pointer and atomically sets it.

    If getAndRef needs to create a d-pointer, it sets the strongref to -1,
    indicating that the QObject is not shared: QWeakPointer is used only to
    determine whether the QObject has been deleted. In that case, it cannot
    be upgraded to QSharedPointer (see the previous section).

    \endomit

    \sa QSharedPointer, QScopedPointer
*/

/*!
    \fn QSharedPointer::QSharedPointer()

    Creates a QSharedPointer that points to null (0).
*/

/*!
    \fn QSharedPointer::~QSharedPointer()

    Destroys this QSharedPointer object. If it is the last reference to
    the pointer stored, this will delete the pointer as well.
*/

/*!
    \fn QSharedPointer::QSharedPointer(T *ptr)

    Creates a QSharedPointer that points to \a ptr. The pointer \a ptr
    becomes managed by this QSharedPointer and must not be passed to
    another QSharedPointer object or deleted outside this object.
*/

/*!
    \fn QSharedPointer::QSharedPointer(T *ptr, Deleter deleter)

    Creates a QSharedPointer that points to \a ptr. The pointer \a ptr
    becomes managed by this QSharedPointer and must not be passed to
    another QSharedPointer object or deleted outside this object.

    The \a deleter parameter specifies the custom deleter for this
    object. The custom deleter is called, instead of the operator delete(),
    when the strong reference count drops to 0. This is useful,
    for instance, for calling deleteLater() on a QObject instead:

    \code
    static void doDeleteLater(MyObject *obj)
    {
        obj->deleteLater();
    }

    void otherFunction()
    {
        QSharedPointer<MyObject> obj =
            QSharedPointer<MyObject>(new MyObject, doDeleteLater);

        // continue using obj
        obj.clear();    // calls obj->deleteLater();
    }
    \endcode

    It is also possible to specify a member function directly, as in:
    \code
        QSharedPointer<MyObject> obj =
            QSharedPointer<MyObject>(new MyObject, &QObject::deleteLater);
    \endcode

    \sa clear()
*/

/*!
    \fn QSharedPointer::QSharedPointer(const QSharedPointer<T> &other)

    Creates a QSharedPointer object that shares \a other's pointer.

    If \tt T is a derived type of the template parameter of this class,
    QSharedPointer will perform an automatic cast. Otherwise, you will
    get a compiler error.
*/

/*!
    \fn QSharedPointer::QSharedPointer(const QWeakPointer<T> &other)

    Creates a QSharedPointer by promoting the weak reference \a other
    to strong reference and sharing its pointer.

    If \tt T is a derived type of the template parameter of this
    class, QSharedPointer will perform an automatic cast. Otherwise,
    you will get a compiler error.

    \sa QWeakPointer::toStrongRef()
*/

/*!
    \fn QSharedPointer &QSharedPointer::operator=(const QSharedPointer<T> &other)

    Makes this object share \a other's pointer. The current pointer
    reference is discarded and, if it was the last, the pointer will
    be deleted.

    If \tt T is a derived type of the template parameter of this
    class, QSharedPointer will perform an automatic cast. Otherwise,
    you will get a compiler error.
*/

/*!
    \fn QSharedPointer &QSharedPointer::operator=(const QWeakPointer<T> &other)

    Promotes \a other to a strong reference and makes this object
    share a reference to the pointer referenced by it. The current pointer
    reference is discarded and, if it was the last, the pointer will
    be deleted.

    If \tt T is a derived type of the template parameter of this
    class, QSharedPointer will perform an automatic cast. Otherwise,
    you will get a compiler error.
*/

/*!
    \fn void QSharedPointer::swap(QSharedPointer<T> &other);
    \since 5.3

    Swaps this shared pointer instance with \a other. This function is
    very fast and never fails.
*/

/*!
    \fn T *QSharedPointer::data() const

    Returns the value of the pointer referenced by this object.

    Note: do not delete the pointer returned by this function or pass
    it to another function that could delete it, including creating
    QSharedPointer or QWeakPointer objects.
*/

/*!
    \fn T &QSharedPointer::operator *() const

    Provides access to the shared pointer's members.

    \sa isNull()
*/

/*!
    \fn T *QSharedPointer::operator ->() const

    Provides access to the shared pointer's members.

    \sa isNull()
*/

/*!
    \fn bool QSharedPointer::isNull() const

    Returns \c true if this object is holding a reference to a null
    pointer.
*/

/*!
    \fn QSharedPointer::operator bool() const

    Returns \c true if this object is not null. This function is suitable
    for use in \tt if-constructs, like:

    \code
        if (sharedptr) { ... }
    \endcode

    \sa isNull()
*/

/*!
    \fn bool QSharedPointer::operator !() const

    Returns \c true if this object is null. This function is suitable
    for use in \tt if-constructs, like:

    \code
        if (!sharedptr) { ... }
    \endcode

    \sa isNull()
*/

/*!
    \fn QSharedPointer<X> QSharedPointer::staticCast() const

    Performs a static cast from this pointer's type to \tt X and returns
    a QSharedPointer that shares the reference. This function can be
    used for up- and for down-casting, but is more useful for
    up-casting.

    Note: the template type \c X must have the same const and volatile
    qualifiers as the template of this object, or the cast will
    fail. Use constCast() if you need to drop those qualifiers.

    \sa dynamicCast(), constCast(), qSharedPointerCast()
*/

/*!
    \fn QSharedPointer<X> QSharedPointer::dynamicCast() const

    Performs a dynamic cast from this pointer's type to \tt X and
    returns a QSharedPointer that shares the reference. If this
    function is used to up-cast, then QSharedPointer will perform a \tt
    dynamic_cast, which means that if the object being pointed by this
    QSharedPointer is not of type \tt X, the returned object will be
    null.

    Note: the template type \c X must have the same const and volatile
    qualifiers as the template of this object, or the cast will
    fail. Use constCast() if you need to drop those qualifiers.

    \sa qSharedPointerDynamicCast()
*/

/*!
    \fn QSharedPointer<X> QSharedPointer::constCast() const

    Performs a \tt const_cast from this pointer's type to \tt X and returns
    a QSharedPointer that shares the reference. This function can be
    used for up- and for down-casting, but is more useful for
    up-casting.

    \sa isNull(), qSharedPointerConstCast()
*/

/*!
    \fn QSharedPointer<X> QSharedPointer::objectCast() const
    \since 4.6

    Performs a \l qobject_cast() from this pointer's type to \tt X and
    returns a QSharedPointer that shares the reference. If this
    function is used to up-cast, then QSharedPointer will perform a \tt
    qobject_cast, which means that if the object being pointed by this
    QSharedPointer is not of type \tt X, the returned object will be
    null.

    Note: the template type \c X must have the same const and volatile
    qualifiers as the template of this object, or the cast will
    fail. Use constCast() if you need to drop those qualifiers.

    \sa qSharedPointerObjectCast()
*/

/*!
    \fn QSharedPointer<T> QSharedPointer::create()
    \since 5.1

    Creates a QSharedPointer object and allocates a new item of type \tt T. The
    QSharedPointer internals and the object are allocated in one single memory
    allocation, which could help reduce memory fragmentation in a long-running
    application.

    This function calls the default constructor for type \tt T.
*/

/*!
    \fn QSharedPointer<T> QSharedPointer::create(...)
    \overload
    \since 5.1

    Creates a QSharedPointer object and allocates a new item of type \tt T. The
    QSharedPointer internals and the object are allocated in one single memory
    allocation, which could help reduce memory fragmentation in a long-running
    application.

    This function will attempt to call a constructor for type \tt T that can
    accept all the arguments passed. Arguments will be perfectly-forwarded.

    \note This function is only available with a C++11 compiler that supports
    perfect forwarding of an arbitrary number of arguments. If the compiler
    does not support the necessary C++11 features, you must use the overload
    that calls the default constructor.
*/

/*!
    \fn QWeakPointer<T> QSharedPointer::toWeakRef() const

    Returns a weak reference object that shares the pointer referenced
    by this object.

    \sa QWeakPointer::QWeakPointer()
*/

/*!
    \fn void QSharedPointer::clear()

    Clears this QSharedPointer object, dropping the reference that it
    may have had to the pointer. If this was the last reference, then
    the pointer itself will be deleted.
*/

/*!
    \fn void QSharedPointer::reset()
    \since 5.0

    Same as clear(). For std::shared_ptr compatibility.
*/

/*!
    \fn void QSharedPointer::reset(T *t)
    \since 5.0

    Resets this QSharedPointer object to point to \a t
    instead. Equivalent to:
    \code
    QSharedPointer<T> other(t); this->swap(other);
    \endcode
*/

/*!
    \fn void QSharedPointer::reset(T *t, Deleter deleter)
    \since 5.0

    Resets this QSharedPointer object to point to \a t
    instead, with deleter \a deleter. Equivalent to:
    \code
    QSharedPointer<T> other(t, deleter); this->swap(other);
    \endcode
*/

/*!
    \fn QWeakPointer::QWeakPointer()

    Creates a QWeakPointer that points to nothing.
*/

/*!
    \fn QWeakPointer::~QWeakPointer()

    Destroys this QWeakPointer object. The pointer referenced
    by this object will not be deleted.
*/

/*!
    \fn QWeakPointer::QWeakPointer(const QWeakPointer<T> &other)

    Creates a QWeakPointer that holds a weak reference to the
    pointer referenced by \a other.

    If \tt T is a derived type of the template parameter of this
    class, QWeakPointer will perform an automatic cast. Otherwise,
    you will get a compiler error.
*/

/*!
    \fn QWeakPointer::QWeakPointer(const QSharedPointer<T> &other)

    Creates a QWeakPointer that holds a weak reference to the
    pointer referenced by \a other.

    If \tt T is a derived type of the template parameter of this
    class, QWeakPointer will perform an automatic cast. Otherwise,
    you will get a compiler error.
*/

/*!
    \fn QWeakPointer::QWeakPointer(const QObject *obj)
    \since 4.6
    \deprecated

    Creates a QWeakPointer that holds a weak reference directly to the
    QObject \a obj. This constructor is only available if the template type
    \tt T is QObject or derives from it (otherwise a compilation error will
    result).

    You can use this constructor with any QObject, even if they were not
    created with \l QSharedPointer.

    Note that QWeakPointers created this way on arbitrary QObjects usually
    cannot be promoted to QSharedPointer.

    \sa QSharedPointer, QPointer
*/

/*!
    \fn QWeakPointer &QWeakPointer::operator=(const QObject *obj)
    \since 4.6
    \deprecated

    Makes this QWeakPointer hold a weak reference directly to the QObject
    \a obj. This function is only available if the template type \tt T is
    QObject or derives from it.

    \sa QPointer
*/

/*!
    \fn QWeakPointer &QWeakPointer::operator=(const QWeakPointer<T> &other)

    Makes this object share \a other's pointer. The current pointer
    reference is discarded but is not deleted.

    If \tt T is a derived type of the template parameter of this
    class, QWeakPointer will perform an automatic cast. Otherwise,
    you will get a compiler error.
*/

/*!
    \fn QWeakPointer &QWeakPointer::operator=(const QSharedPointer<T> &other)

    Makes this object share \a other's pointer. The current pointer
    reference is discarded but is not deleted.

    If \tt T is a derived type of the template parameter of this
    class, QWeakPointer will perform an automatic cast. Otherwise,
    you will get a compiler error.
*/

/*!
    \fn bool QWeakPointer::isNull() const

    Returns \c true if this object is holding a reference to a null
    pointer.

    Note that, due to the nature of weak references, the pointer that
    QWeakPointer references can become null at any moment, so
    the value returned from this function can change from false to
    true from one call to the next.
*/

/*!
    \fn QWeakPointer::operator bool() const

    Returns \c true if this object is not null. This function is suitable
    for use in \tt if-constructs, like:

    \code
        if (weakref) { ... }
    \endcode

    Note that, due to the nature of weak references, the pointer that
    QWeakPointer references can become null at any moment, so
    the value returned from this function can change from true to
    false from one call to the next.

    \sa isNull()
*/

/*!
    \fn bool QWeakPointer::operator !() const

    Returns \c true if this object is null. This function is suitable
    for use in \tt if-constructs, like:

    \code
        if (!weakref) { ... }
    \endcode

    Note that, due to the nature of weak references, the pointer that
    QWeakPointer references can become null at any moment, so
    the value returned from this function can change from false to
    true from one call to the next.

    \sa isNull()
*/

/*!
    \fn T *QWeakPointer::data() const
    \since 4.6

    Returns the value of the pointer being tracked by this QWeakPointer,
    \b without ensuring that it cannot get deleted. To have that guarantee,
    use toStrongRef(), which returns a QSharedPointer object. If this
    function can determine that the pointer has already been deleted, it
    returns 0.

    It is ok to obtain the value of the pointer and using that value itself,
    like for example in debugging statements:

    \code
        qDebug("Tracking %p", weakref.data());
    \endcode

    However, dereferencing the pointer is only allowed if you can guarantee
    by external means that the pointer does not get deleted. For example,
    if you can be certain that no other thread can delete it, nor the
    functions that you may call.

    If that is the case, then the following code is valid:

    \code
        // this pointer cannot be used in another thread
        // so other threads cannot delete it
        QWeakPointer<int> weakref = obtainReference();

        Object *obj = weakref.data();
        if (obj) {
            // if the pointer wasn't deleted yet, we know it can't get
            // deleted by our own code here nor the functions we call
            otherFunction(obj);
        }
    \endcode

    Use this function with care.

    \sa isNull(), toStrongRef()
*/

/*!
    \fn QSharedPointer<T> QWeakPointer::toStrongRef() const

    Promotes this weak reference to a strong one and returns a
    QSharedPointer object holding that reference. When promoting to
    QSharedPointer, this function verifies if the object has been deleted
    already or not. If it hasn't, this function increases the reference
    count to the shared object, thus ensuring that it will not get
    deleted.

    Since this function can fail to obtain a valid strong reference to the
    shared object, you should always verify if the conversion succeeded,
    by calling QSharedPointer::isNull() on the returned object.

    For example, the following code promotes a QWeakPointer that was held
    to a strong reference and, if it succeeded, it prints the value of the
    integer that was held:

    \code
        QWeakPointer<int> weakref;

        // ...

        QSharedPointer<int> strong = weakref.toStrongRef();
        if (strong)
            qDebug() << "The value is:" << *strong;
        else
            qDebug() << "The value has already been deleted";
    \endcode

    \sa QSharedPointer::QSharedPointer()
*/

/*!
    \fn void QWeakPointer::clear()

    Clears this QWeakPointer object, dropping the reference that it
    may have had to the pointer.
*/

/*!
    \fn bool operator==(const QSharedPointer<T> &ptr1, const QSharedPointer<X> &ptr2)
    \relates QSharedPointer

    Returns \c true if the pointer referenced by \a ptr1 is the
    same pointer as that referenced by \a ptr2.

    If \a ptr2's template parameter is different from \a ptr1's,
    QSharedPointer will attempt to perform an automatic \tt static_cast
    to ensure that the pointers being compared are equal. If \a ptr2's
    template parameter is not a base or a derived type from
    \a ptr1's, you will get a compiler error.
*/

/*!
    \fn bool operator!=(const QSharedPointer<T> &ptr1, const QSharedPointer<X> &ptr2)
    \relates QSharedPointer

    Returns \c true if the pointer referenced by \a ptr1 is not the
    same pointer as that referenced by \a ptr2.

    If \a ptr2's template parameter is different from \a ptr1's,
    QSharedPointer will attempt to perform an automatic \tt static_cast
    to ensure that the pointers being compared are equal. If \a ptr2's
    template parameter is not a base or a derived type from
    \a ptr1's, you will get a compiler error.
*/

/*!
    \fn bool operator==(const QSharedPointer<T> &ptr1, const X *ptr2)
    \relates QSharedPointer

    Returns \c true if the pointer referenced by \a ptr1 is the
    same pointer as \a ptr2.

    If \a ptr2's type is different from \a ptr1's,
    QSharedPointer will attempt to perform an automatic \tt static_cast
    to ensure that the pointers being compared are equal. If \a ptr2's
    type is not a base or a derived type from this
    \a ptr1's, you will get a compiler error.
*/

/*!
    \fn bool operator!=(const QSharedPointer<T> &ptr1, const X *ptr2)
    \relates QSharedPointer

    Returns \c true if the pointer referenced by \a ptr1 is not the
    same pointer as \a ptr2.

    If \a ptr2's type is different from \a ptr1's,
    QSharedPointer will attempt to perform an automatic \tt static_cast
    to ensure that the pointers being compared are equal. If \a ptr2's
    type is not a base or a derived type from this
    \a ptr1's, you will get a compiler error.
*/

/*!
    \fn bool operator==(const T *ptr1, const QSharedPointer<X> &ptr2)
    \relates QSharedPointer

    Returns \c true if the pointer \a ptr1 is the
    same pointer as that referenced by \a ptr2.

    If \a ptr2's template parameter is different from \a ptr1's type,
    QSharedPointer will attempt to perform an automatic \tt static_cast
    to ensure that the pointers being compared are equal. If \a ptr2's
    template parameter is not a base or a derived type from
    \a ptr1's type, you will get a compiler error.
*/

/*!
    \fn bool operator!=(const T *ptr1, const QSharedPointer<X> &ptr2)
    \relates QSharedPointer

    Returns \c true if the pointer \a ptr1 is not the
    same pointer as that referenced by \a ptr2.

    If \a ptr2's template parameter is different from \a ptr1's type,
    QSharedPointer will attempt to perform an automatic \tt static_cast
    to ensure that the pointers being compared are equal. If \a ptr2's
    template parameter is not a base or a derived type from
    \a ptr1's type, you will get a compiler error.
*/

/*!
    \fn bool operator==(const QSharedPointer<T> &ptr1, const QWeakPointer<X> &ptr2)
    \relates QWeakPointer

    Returns \c true if the pointer referenced by \a ptr1 is the
    same pointer as that referenced by \a ptr2.

    If \a ptr2's template parameter is different from \a ptr1's,
    QSharedPointer will attempt to perform an automatic \tt static_cast
    to ensure that the pointers being compared are equal. If \a ptr2's
    template parameter is not a base or a derived type from
    \a ptr1's, you will get a compiler error.
*/

/*!
    \fn bool operator!=(const QSharedPointer<T> &ptr1, const QWeakPointer<X> &ptr2)
    \relates QWeakPointer

    Returns \c true if the pointer referenced by \a ptr1 is not the
    same pointer as that referenced by \a ptr2.

    If \a ptr2's template parameter is different from \a ptr1's,
    QSharedPointer will attempt to perform an automatic \tt static_cast
    to ensure that the pointers being compared are equal. If \a ptr2's
    template parameter is not a base or a derived type from
    \a ptr1's, you will get a compiler error.
*/

/*!
    \fn bool operator==(const QWeakPointer<T> &ptr1, const QSharedPointer<X> &ptr2)
    \relates QWeakPointer

    Returns \c true if the pointer referenced by \a ptr1 is the
    same pointer as that referenced by \a ptr2.

    If \a ptr2's template parameter is different from \a ptr1's,
    QSharedPointer will attempt to perform an automatic \tt static_cast
    to ensure that the pointers being compared are equal. If \a ptr2's
    template parameter is not a base or a derived type from
    \a ptr1's, you will get a compiler error.
*/

/*!
    \fn bool operator!=(const QWeakPointer<T> &ptr1, const QSharedPointer<X> &ptr2)
    \relates QWeakPointer

    Returns \c true if the pointer referenced by \a ptr1 is not the
    same pointer as that referenced by \a ptr2.

    If \a ptr2's template parameter is different from \a ptr1's,
    QSharedPointer will attempt to perform an automatic \tt static_cast
    to ensure that the pointers being compared are equal. If \a ptr2's
    template parameter is not a base or a derived type from
    \a ptr1's, you will get a compiler error.
*/

/*!
    \fn QSharedPointer<X> qSharedPointerCast(const QSharedPointer<T> &other)
    \relates QSharedPointer

    Returns a shared pointer to the pointer held by \a other, cast to
    type \tt X.  The types \tt T and \tt X must belong to one
    hierarchy for the \tt static_cast to succeed.

    Note that \tt X must have the same cv-qualifiers (\tt const and
    \tt volatile) that \tt T has, or the code will fail to
    compile. Use qSharedPointerConstCast to cast away the constness.

    \sa QSharedPointer::staticCast(), qSharedPointerDynamicCast(), qSharedPointerConstCast()
*/

/*!
    \fn QSharedPointer<X> qSharedPointerCast(const QWeakPointer<T> &other)
    \relates QSharedPointer
    \relates QWeakPointer

    Returns a shared pointer to the pointer held by \a other, cast to
    type \tt X.  The types \tt T and \tt X must belong to one
    hierarchy for the \tt static_cast to succeed.

    The \a other object is converted first to a strong reference. If
    that conversion fails (because the object it's pointing to has
    already been deleted), this function returns a null
    QSharedPointer.

    Note that \tt X must have the same cv-qualifiers (\tt const and
    \tt volatile) that \tt T has, or the code will fail to
    compile. Use qSharedPointerConstCast to cast away the constness.

    \sa QWeakPointer::toStrongRef(), qSharedPointerDynamicCast(), qSharedPointerConstCast()
*/

/*!
    \fn QSharedPointer<X> qSharedPointerDynamicCast(const QSharedPointer<T> &other)
    \relates QSharedPointer

    Returns a shared pointer to the pointer held by \a other, using a
    dynamic cast to type \tt X to obtain an internal pointer of the
    appropriate type. If the \tt dynamic_cast fails, the object
    returned will be null.

    Note that \tt X must have the same cv-qualifiers (\tt const and
    \tt volatile) that \tt T has, or the code will fail to
    compile. Use qSharedPointerConstCast to cast away the constness.

    \sa QSharedPointer::dynamicCast(), qSharedPointerCast(), qSharedPointerConstCast()
*/

/*!
    \fn QSharedPointer<X> qSharedPointerDynamicCast(const QWeakPointer<T> &other)
    \relates QSharedPointer
    \relates QWeakPointer

    Returns a shared pointer to the pointer held by \a other, using a
    dynamic cast to type \tt X to obtain an internal pointer of the
    appropriate type. If the \tt dynamic_cast fails, the object
    returned will be null.

    The \a other object is converted first to a strong reference. If
    that conversion fails (because the object it's pointing to has
    already been deleted), this function also returns a null
    QSharedPointer.

    Note that \tt X must have the same cv-qualifiers (\tt const and
    \tt volatile) that \tt T has, or the code will fail to
    compile. Use qSharedPointerConstCast to cast away the constness.

    \sa QWeakPointer::toStrongRef(), qSharedPointerCast(), qSharedPointerConstCast()
*/

/*!
    \fn QSharedPointer<X> qSharedPointerConstCast(const QSharedPointer<T> &other)
    \relates QSharedPointer

    Returns a shared pointer to the pointer held by \a other, cast to
    type \tt X.  The types \tt T and \tt X must belong to one
    hierarchy for the \tt const_cast to succeed. The \tt const and \tt
    volatile differences between \tt T and \tt X are ignored.

    \sa QSharedPointer::constCast(), qSharedPointerCast(), qSharedPointerDynamicCast()
*/

/*!
    \fn QSharedPointer<X> qSharedPointerConstCast(const QWeakPointer<T> &other)
    \relates QSharedPointer
    \relates QWeakPointer

    Returns a shared pointer to the pointer held by \a other, cast to
    type \tt X. The types \tt T and \tt X must belong to one
    hierarchy for the \tt const_cast to succeed. The \tt const and
    \tt volatile differences between \tt T and \tt X are ignored.

    The \a other object is converted first to a strong reference. If
    that conversion fails (because the object it's pointing to has
    already been deleted), this function returns a null
    QSharedPointer.

    \sa QWeakPointer::toStrongRef(), qSharedPointerCast(), qSharedPointerDynamicCast()
*/

/*!
    \fn QSharedPointer<X> qSharedPointerObjectCast(const QSharedPointer<T> &other)
    \relates QSharedPointer
    \since 4.6

    \brief The qSharedPointerObjectCast function is for casting a shared pointer.

    Returns a shared pointer to the pointer held by \a other, using a
    \l qobject_cast() to type \tt X to obtain an internal pointer of the
    appropriate type. If the \tt qobject_cast fails, the object
    returned will be null.

    Note that \tt X must have the same cv-qualifiers (\tt const and
    \tt volatile) that \tt T has, or the code will fail to
    compile. Use qSharedPointerConstCast to cast away the constness.

    \sa QSharedPointer::objectCast(), qSharedPointerCast(), qSharedPointerConstCast()
*/

/*!
    \fn QSharedPointer<X> qSharedPointerObjectCast(const QWeakPointer<T> &other)
    \relates QSharedPointer
    \relates QWeakPointer
    \since 4.6

    \brief The qSharedPointerObjectCast function is for casting a shared pointer.

    Returns a shared pointer to the pointer held by \a other, using a
    \l qobject_cast() to type \tt X to obtain an internal pointer of the
    appropriate type. If the \tt qobject_cast fails, the object
    returned will be null.

    The \a other object is converted first to a strong reference. If
    that conversion fails (because the object it's pointing to has
    already been deleted), this function also returns a null
    QSharedPointer.

    Note that \tt X must have the same cv-qualifiers (\tt const and
    \tt volatile) that \tt T has, or the code will fail to
    compile. Use qSharedPointerConstCast to cast away the constness.

    \sa QWeakPointer::toStrongRef(), qSharedPointerCast(), qSharedPointerConstCast()
*/


/*!
    \fn QWeakPointer<X> qWeakPointerCast(const QWeakPointer<T> &other)
    \relates QWeakPointer

    Returns a weak pointer to the pointer held by \a other, cast to
    type \tt X. The types \tt T and \tt X must belong to one
    hierarchy for the \tt static_cast to succeed.

    Note that \tt X must have the same cv-qualifiers (\tt const and
    \tt volatile) that \tt T has, or the code will fail to
    compile. Use qSharedPointerConstCast to cast away the constness.
*/

#include <qset.h>
#include <qmutex.h>

#if !defined(QT_NO_QOBJECT)
#include "private/qobject_p.h"

QT_BEGIN_NAMESPACE

/*!
    \internal
    This function is called for a just-created QObject \a obj, to enable
    the use of QSharedPointer and QWeakPointer in the future.
 */
void QtSharedPointer::ExternalRefCountData::setQObjectShared(const QObject *, bool)
{}

/*!
    \internal
    This function is called when a QSharedPointer is created from a QWeakPointer

    We check that the QWeakPointer was really created from a QSharedPointer, and
    not from a QObject.
*/
void QtSharedPointer::ExternalRefCountData::checkQObjectShared(const QObject *)
{
    if (strongref.load() < 0)
        qWarning("QSharedPointer: cannot create a QSharedPointer from a QObject-tracking QWeakPointer");
}

QtSharedPointer::ExternalRefCountData *QtSharedPointer::ExternalRefCountData::getAndRef(const QObject *obj)
{
    Q_ASSERT(obj);
    QObjectPrivate *d = QObjectPrivate::get(const_cast<QObject *>(obj));
    Q_ASSERT_X(!d->wasDeleted, "QWeakPointer", "Detected QWeakPointer creation in a QObject being deleted");

    ExternalRefCountData *that = d->sharedRefcount.load();
    if (that) {
        that->weakref.ref();
        return that;
    }

    // we can create the refcount data because it doesn't exist
    ExternalRefCountData *x = new ExternalRefCountData(Qt::Uninitialized);
    x->strongref.store(-1);
    x->weakref.store(2);  // the QWeakPointer that called us plus the QObject itself
    if (!d->sharedRefcount.testAndSetRelease(0, x)) {
        delete x;
        x = d->sharedRefcount.loadAcquire();
        x->weakref.ref();
    }
    return x;
}

/**
    \internal
    Returns a QSharedPointer<QObject> if the variant contains
    a QSharedPointer<T> where T inherits QObject. Otherwise the behaviour is undefined.
*/
QSharedPointer<QObject> QtSharedPointer::sharedPointerFromVariant_internal(const QVariant &variant)
{
    Q_ASSERT(QMetaType::typeFlags(variant.userType()) & QMetaType::SharedPointerToQObject);
    return *reinterpret_cast<const QSharedPointer<QObject>*>(variant.constData());
}

/**
    \internal
    Returns a QWeakPointer<QObject> if the variant contains
    a QWeakPointer<T> where T inherits QObject. Otherwise the behaviour is undefined.
*/
QWeakPointer<QObject> QtSharedPointer::weakPointerFromVariant_internal(const QVariant &variant)
{
    Q_ASSERT(QMetaType::typeFlags(variant.userType()) & QMetaType::WeakPointerToQObject || QMetaType::typeFlags(variant.userType()) & QMetaType::TrackingPointerToQObject);
    return *reinterpret_cast<const QWeakPointer<QObject>*>(variant.constData());
}

QT_END_NAMESPACE

#endif



//#  define QT_SHARED_POINTER_BACKTRACE_SUPPORT
#  ifdef QT_SHARED_POINTER_BACKTRACE_SUPPORT
#    if defined(__GLIBC__) && (__GLIBC__ >= 2) && !defined(__UCLIBC__) && !defined(QT_LINUXBASE)
#      define BACKTRACE_SUPPORTED
#    elif defined(Q_OS_MAC)
#      define BACKTRACE_SUPPORTED
#    endif
#  endif

#  if defined(BACKTRACE_SUPPORTED)
#    include <sys/types.h>
#    include <execinfo.h>
#    include <stdio.h>
#    include <unistd.h>
#    include <sys/wait.h>

QT_BEGIN_NAMESPACE

static inline QByteArray saveBacktrace() __attribute__((always_inline));
static inline QByteArray saveBacktrace()
{
    static const int maxFrames = 32;

    QByteArray stacktrace;
    stacktrace.resize(sizeof(void*) * maxFrames);
    int stack_size = backtrace((void**)stacktrace.data(), maxFrames);
    stacktrace.resize(sizeof(void*) * stack_size);

    return stacktrace;
}

static void printBacktrace(QByteArray stacktrace)
{
    void *const *stack = (void *const *)stacktrace.constData();
    int stack_size = stacktrace.size() / sizeof(void*);
    char **stack_symbols = backtrace_symbols(stack, stack_size);

    int filter[2];
    pid_t child = -1;
    if (pipe(filter) != -1)
        child = fork();
    if (child == 0) {
        // child process
        dup2(fileno(stderr), fileno(stdout));
        dup2(filter[0], fileno(stdin));
        close(filter[0]);
        close(filter[1]);
        execlp("c++filt", "c++filt", "-n", NULL);

        // execlp failed
        execl("/bin/cat", "/bin/cat", NULL);
        _exit(127);
    }

    // parent process
    close(filter[0]);
    FILE *output;
    if (child == -1) {
        // failed forking
        close(filter[1]);
        output = stderr;
    } else {
        output = fdopen(filter[1], "w");
    }

    fprintf(stderr, "Backtrace of the first creation (most recent frame first):\n");
    for (int i = 0; i < stack_size; ++i) {
        if (strlen(stack_symbols[i]))
            fprintf(output, "#%-2d %s\n", i, stack_symbols[i]);
        else
            fprintf(output, "#%-2d %p\n", i, stack[i]);
    }

    if (child != -1) {
        fclose(output);
        waitpid(child, 0, 0);
    }
}

QT_END_NAMESPACE

#  endif  // BACKTRACE_SUPPORTED

namespace {
    QT_USE_NAMESPACE
    struct Data {
        const volatile void *pointer;
#  ifdef BACKTRACE_SUPPORTED
        QByteArray backtrace;
#  endif
    };

    class KnownPointers
    {
    public:
        QMutex mutex;
        QHash<const void *, Data> dPointers;
        QHash<const volatile void *, const void *> dataPointers;
    };
}

Q_GLOBAL_STATIC(KnownPointers, knownPointers)

QT_BEGIN_NAMESPACE

namespace QtSharedPointer {
    Q_AUTOTEST_EXPORT void internalSafetyCheckCleanCheck();
}

/*!
    \internal
*/
void QtSharedPointer::internalSafetyCheckAdd(const void *d_ptr, const volatile void *ptr)
{
    KnownPointers *const kp = knownPointers();
    if (!kp)
        return;                 // end-game: the application is being destroyed already

    QMutexLocker lock(&kp->mutex);
    Q_ASSERT(!kp->dPointers.contains(d_ptr));

    //qDebug("Adding d=%p value=%p", d_ptr, ptr);

    const void *other_d_ptr = kp->dataPointers.value(ptr, 0);
    if (other_d_ptr) {
#  ifdef BACKTRACE_SUPPORTED
        printBacktrace(knownPointers()->dPointers.value(other_d_ptr).backtrace);
#  endif
        qFatal("QSharedPointer: internal self-check failed: pointer %p was already tracked "
               "by another QSharedPointer object %p", ptr, other_d_ptr);
    }

    Data data;
    data.pointer = ptr;
#  ifdef BACKTRACE_SUPPORTED
    data.backtrace = saveBacktrace();
#  endif

    kp->dPointers.insert(d_ptr, data);
    kp->dataPointers.insert(ptr, d_ptr);
    Q_ASSERT(kp->dPointers.size() == kp->dataPointers.size());
}

/*!
    \internal
*/
void QtSharedPointer::internalSafetyCheckRemove(const void *d_ptr)
{
    KnownPointers *const kp = knownPointers();
    if (!kp)
        return;                 // end-game: the application is being destroyed already

    QMutexLocker lock(&kp->mutex);

    QHash<const void *, Data>::iterator it = kp->dPointers.find(d_ptr);
    if (it == kp->dPointers.end()) {
        qFatal("QSharedPointer: internal self-check inconsistency: pointer %p was not tracked. "
               "To use QT_SHAREDPOINTER_TRACK_POINTERS, you have to enable it throughout "
               "in your code.", d_ptr);
    }

    QHash<const volatile void *, const void *>::iterator it2 = kp->dataPointers.find(it->pointer);
    Q_ASSERT(it2 != kp->dataPointers.end());

    //qDebug("Removing d=%p value=%p", d_ptr, it->pointer);

    // remove entries
    kp->dataPointers.erase(it2);
    kp->dPointers.erase(it);
    Q_ASSERT(kp->dPointers.size() == kp->dataPointers.size());
}

/*!
    \internal
    Called by the QSharedPointer autotest
*/
void QtSharedPointer::internalSafetyCheckCleanCheck()
{
#  ifdef QT_BUILD_INTERNAL
    KnownPointers *const kp = knownPointers();
    Q_ASSERT_X(kp, "internalSafetyCheckSelfCheck()", "Called after global statics deletion!");

    if (kp->dPointers.size() != kp->dataPointers.size())
        qFatal("Internal consistency error: the number of pointers is not equal!");

    if (!kp->dPointers.isEmpty())
        qFatal("Pointer cleaning failed: %d entries remaining", kp->dPointers.size());
#  endif
}

QT_END_NAMESPACE
