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

#include "qsharedpointer.h"

// to be sure we aren't causing a namespace clash:
#include "qshareddata.h"

/*!
    \class QSharedPointer
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
    does so weakly. QPointer can be replaced by QWeakPointer in almost all
    cases, since they have the same functionality. See
    \l{QWeakPointer#tracking-qobject} for more information.

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

    QSharedPointer is in reality implemented by two ancestor classes:
    QtSharedPointer::Basic and QtSharedPointer::ExternalRefCount. The reason
    for having that split is now mostly legacy: in the beginning,
    QSharedPointer was meant to support both internal reference counting and
    external reference counting.

    QtSharedPointer::Basic implements the basic functionality that is shared
    between internal- and external-reference counting. That is, it's mostly
    the accessor functions into QSharedPointer. Those are all inherited by
    QSharedPointer, which adds another level of shared functionality (the
    constructors and assignment operators). The Basic class has one member
    variable, which is the actual pointer being tracked.

    QtSharedPointer::ExternalRefCount implements the actual reference
    counting and introduces the d-pointer for QSharedPointer. That d-pointer
    itself is shared with other QSharedPointer objects as well as
    QWeakPointer.

    The reason for keeping the pointer value itself outside the d-pointer is
    because of multiple inheritance needs. If you have two QSharedPointer
    objects of different pointer types, but pointing to the same object in
    memory, it could happen that the pointer values are different. The \tt
    differentPointers autotest exemplifies this problem. The same thing could
    happen in the case of virtual inheritance: a pointer of class matching
    the virtual base has different address compared to the pointer of the
    complete object. See the \tt virtualBaseDifferentPointers autotest for
    this problem.

    The d pointer is of type QtSharedPointer::ExternalRefCountData for simple
    QSharedPointer objects, but could be of a derived type in some cases. It
    is basically a reference-counted reference-counter.

    \section2 d-pointer
    \section3 QtSharedPointer::ExternalRefCountData

    This class is basically a reference-counted reference-counter. It has two
    members: \tt strongref and \tt weakref. The strong reference counter is
    controlling the lifetime of the object tracked by QSharedPointer. a
    positive value indicates that the object is alive. It's also the number
    of QSharedObject instances that are attached to this Data.

    When the strong reference count decreases to zero, the object is deleted
    (see below for information on custom deleters). The strong reference
    count can also exceptionally be -1, indicating that there are no
    QSharedPointers attached to an object, which is tracked too. The only
    case where this is possible is that of
    \l{QWeakPointer#tracking-qobject}{QWeakPointers tracking a QObject}.

    The weak reference count controls the lifetime of the d-pointer itself.
    It can be thought of as an internal/intrusive reference count for
    ExternalRefCountData itself. This count is equal to the number of
    QSharedPointers and QWeakPointers that are tracking this object. (In case
    the object tracked derives from QObject, this number is increased by 1,
    since QObjectPrivate tracks it too).

    ExternalRefCountData is a virtual class: it has a virtual destructor and
    a virtual destroy() function. The destroy() function is supposed to
    delete the object being tracked and return true if it does so. Otherwise,
    it returns false to indicate that the caller must simply call delete.
    This allows the normal use-case of QSharedPointer without custom deleters
    to use only one 12- or 16-byte (depending on whether it's a 32- or 64-bit
    architecture) external descriptor structure, without paying the price for
    the custom deleter that it isn't using.

    \section3 QtSharedPointer::ExternalRefCountDataWithDestroyFn

    This class is not used directly, per se. It only exists to enable the two
    classes that derive from it. It adds one member variable, which is a
    pointer to a function (which returns void and takes an
    ExternalRefCountData* as a parameter). It also overrides the destroy()
    function: it calls that function pointer with \tt this as parameter, and
    returns true.

    That means when ExternalRefCountDataWithDestroyFn is used, the \tt
    destroyer field must be set to a valid function that \b will delete the
    object tracked.

    This class also adds an operator delete function to ensure that it simply
    calls the global operator delete. That should be the behaviour in all
    compilers already, but to be on the safe side, this class ensures that no
    funny business happens.

    On a 32-bit architecture, this class is 16 bytes in size, whereas it's 24
    bytes on 64-bit. (On Itanium where function pointers contain the global
    pointer, it can be 32 bytes).

    \section3 QtSharedPointer::ExternalRefCountWithCustomDeleter

    This class derives from ExternalRefCountDataWithDestroyFn and is a
    template class. As template parameters, it has the type of the pointer
    being tracked (\tt T) and a \tt Deleter, which is anything. It adds two
    fields to its parent class, matching those template parameters: a member
    of type \tt Deleter and a member of type \tt T*.

    The purpose of this class is to store the pointer to be deleted and the
    deleter code along with the d-pointer. This allows the last strong
    reference to call any arbitrary function that disposes of the object. For
    example, this allows calling QObject::deleteLater() on a given object.
    The pointer to the object is kept here to avoid the extra cost of keeping
    the deleter in the generic case.

    This class is never instantiated directly: the constructors and
    destructor are private. Only the create() function may be called to
    return an object of this type. See below for construction details.

    The size of this class depends on the size of \tt Deleter. If it's an
    empty functor (i.e., no members), ABIs generally assign it the size of 1.
    But given that it's followed by a pointer, up to 3 or 7 padding bytes may
    be inserted: in that case, the size of this class is 16+4+4 = 24 bytes on
    32-bit architectures, or 24+8+8 = 40 bytes on 64-bit architectures (48
    bytes on Itanium with global pointers stored). If \tt Deleter is a
    function pointer, the size should be the same as the empty structure
    case, except for Itanium where it may be 56 bytes due to another global
    pointer. If \tt Deleter is a pointer to a member function (PMF), the size
    will be even bigger and will depend on the ABI. For architectures using
    the Itanium C++ ABI, a PMF is twice the size of a normal pointer, or 24
    bytes on Itanium itself. In that case, the size of this structure will be
    16+8+4 = 28 bytes on 32-bit architectures, 24+16+8 = 48 bytes on 64-bit,
    and 32+24+8 = 64 bytes on Itanium.

    (Values for Itanium consider an LP64 architecture; for ILP32, pointers
    are 32-bit in length, function pointers are 64-bit and PMF are 96-bit, so
    the sizes are slightly less)

    \section3 QtSharedPointer::ExternalRefCountWithContiguousData

    This class also derives from ExternalRefCountDataWithDestroyFn and it is
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
    pointer, and hides its constructors and destructor. (With C++0x, we'd
    delete them).

    The size of this class depends on the size of \tt T.

    \section3 Instantiating ExternalRefCountWithCustomDeleter and ExternalRefCountWithContiguousData

    Like explained above, these classes have private constructors. Moreover,
    they are not defined anywhere, so trying to call \tt{new ClassType} would
    result in a compilation or linker error. Instead, these classes must be
    constructed via their create() methods.

    Instead of instantiating the class by the normal way, the create() method
    calls \tt{operator new} directly with the size of the class, then calls
    the parent class's constructor only (ExternalRefCountDataWithDestroyFn).
    This ensures that the inherited members are initialised properly, as well
    as the virtual table pointer, which must point to
    ExternalRefCountDataWithDestroyFn's virtual table. That way, we also
    ensure that the virtual destructor being called is
    ExternalRefCountDataWithDestroyFn's.

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
    virtual destroy() is called by QSharedPointer, the deleter() functions
    are called instead. These functions static_cast the ExternalRefCountData*
    parameter to their own type and execute their deletion: for the
    ExternalRefCountWithCustomDeleter::deleter() case, it runs the user's
    custom deleter, then destroys the deleter; for
    ExternalRefCountWithContiguousData::deleter, it simply calls the \tt T
    destructor directly.

    By not calling the constructor of the derived classes, we avoid
    instantiating their virtual tables. Since these classes are
    template-based, there would be one virtual table per \tt T and \tt
    Deleter type. (This is what Qt 4.5 did.)

    Instead, only one non-inline function is required per template, which is
    the deleter() static member. All the other functions can be inlined.
    What's more, the address of deleter() is calculated only in code, which
    can be resolved at link-time if the linker can determine that the
    function lies in the current application or library module (since these
    classes are not exported, that is the case for Windows or for builds with
    \tt{-fvisibility=hidden}).

    In contrast, a virtual table would require at least 3 relocations to be
    resolved at module load-time, per module where these classes are used.
    (In the Itanium C++ ABI, there would be more relocations, due to the
    RTTI)

    \section3 Modifications due to pointer-tracking

    To ensure that pointers created with pointer-tracking enabled get
    un-tracked when destroyed, even if destroyed by code compiled without the
    feature, QSharedPointer modifies slightly the instructions of the
    previous sections.

    When ExternalRefCountWithCustomDeleter or
    ExternalRefCountWithContiguousData are used, their create() functions
    will set the ExternalRefCountDataWithDestroyFn::destroyer function
    pointer to safetyCheckDeleter() instead. These static member functions
    simply call internalSafetyCheckRemove2() before passing control to the
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
    \brief The QWeakPointer class holds a weak reference to a shared pointer
    \since 4.5
    \reentrant

    The QWeakPointer is an automatic weak reference to a
    pointer in C++. It cannot be used to dereference the pointer
    directly, but it can be used to verify if the pointer has been
    deleted or not in another context.

    QWeakPointer objects can only be created by assignment from a
    QSharedPointer. The exception is pointers derived from QObject: in that
    case, QWeakPointer serves as a replacement to QPointer.

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

    That function can also be used to obtain the tracked pointer for
    QWeakPointers that cannot be promoted to QSharedPointer, such as those
    created directly from a QObject pointer (not via QSharedPointer).

    \section1 Tracking QObject

    QWeakPointer can be used to track deletion of classes that derive from QObject,
    even if they are not managed by QSharedPointer. When used in that role,
    QWeakPointer replaces the older QPointer in all use-cases. QWeakPointer
    is also more efficient than QPointer, so it should be preferred in all
    new code.

    To do that, QWeakPointer provides a special constructor that is only
    available if the template parameter \tt T is either QObject or a class
    deriving from it. Trying to use that constructor if \tt T does not derive
    from QObject will result in compilation errors.

    To obtain the QObject being tracked by QWeakPointer, you must use the
    QWeakPointer::data() function, but only if you can guarantee that the
    object cannot get deleted by another context. It should be noted that
    QPointer had the same constraint, so use of QWeakPointer forces you to
    consider whether the pointer is still valid.

    QObject-derived classes can only be deleted in the thread they have
    affinity to (which is the thread they were created in or moved to, using
    QObject::moveToThread()). In special, QWidget-derived classes cannot be
    created in non-GUI threads nor moved there. Therefore, guaranteeing that
    the tracked QObject has affinity to the current thread is enough to also
    guarantee that it won't be deleted asynchronously.

    Note that QWeakPointer's size and data layout do not match QPointer, so
    it cannot replace that class in a binary-compatible manner.

    Care must also be taken with QWeakPointers created directly from QObject
    pointers when dealing with code that was compiled with Qt versions prior
    to 4.6. Those versions may not track the reference counters correctly, so
    QWeakPointers created from QObject should never be passed to code that
    hasn't been recompiled.

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

    Returns true if this object is holding a reference to a null
    pointer.
*/

/*!
    \fn QSharedPointer::operator bool() const

    Returns true if this object is not null. This function is suitable
    for use in \tt if-constructs, like:

    \code
        if (sharedptr) { ... }
    \endcode

    \sa isNull()
*/

/*!
    \fn bool QSharedPointer::operator !() const

    Returns true if this object is null. This function is suitable
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

    Creates a QWeakPointer that holds a weak reference directly to the
    QObject \a obj. This constructor is only available if the template type
    \tt T is QObject or derives from it (otherwise a compilation error will
    result).

    You can use this constructor with any QObject, even if they were not
    created with \l QSharedPointer.

    Note that QWeakPointers created this way on arbitrary QObjects usually
    cannot be promoted to QSharedPointer.

    \sa QSharedPointer, QWeakPointer#tracking-qobject
*/

/*!
    \fn QWeakPointer &QWeakPointer::operator=(const QObject *obj)
    \since 4.6

    Makes this QWeakPointer hold a weak reference directly to the QObject
    \a obj. This function is only available if the template type \tt T is
    QObject or derives from it.

    \sa QWeakPointer#tracking-qobject
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

    Returns true if this object is holding a reference to a null
    pointer.

    Note that, due to the nature of weak references, the pointer that
    QWeakPointer references can become null at any moment, so
    the value returned from this function can change from false to
    true from one call to the next.
*/

/*!
    \fn QWeakPointer::operator bool() const

    Returns true if this object is not null. This function is suitable
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

    Returns true if this object is null. This function is suitable
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

    Returns true if the pointer referenced by \a ptr1 is the
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

    Returns true if the pointer referenced by \a ptr1 is not the
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

    Returns true if the pointer referenced by \a ptr1 is the
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

    Returns true if the pointer referenced by \a ptr1 is not the
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

    Returns true if the pointer \a ptr1 is the
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

    Returns true if the pointer \a ptr1 is not the
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

    Returns true if the pointer referenced by \a ptr1 is the
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

    Returns true if the pointer referenced by \a ptr1 is not the
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

    Returns true if the pointer referenced by \a ptr1 is the
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

    Returns true if the pointer referenced by \a ptr1 is not the
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
    the use of QSharedPointer and QWeakPointer.

    When QSharedPointer is active in a QObject, the object must not be deleted
    directly: the lifetime is managed by the QSharedPointer object. In that case,
    the deleteLater() and parent-child relationship in QObject only decrease
    the strong reference count, instead of deleting the object.
*/
void QtSharedPointer::ExternalRefCountData::setQObjectShared(const QObject *obj, bool)
{
    Q_ASSERT(obj);
    QObjectPrivate *d = QObjectPrivate::get(const_cast<QObject *>(obj));

    if (d->sharedRefcount)
        qFatal("QSharedPointer: pointer %p already has reference counting", obj);
    d->sharedRefcount = this;

    // QObject decreases the refcount too, so increase it up
    weakref.ref();
}

QtSharedPointer::ExternalRefCountData *QtSharedPointer::ExternalRefCountData::getAndRef(const QObject *obj)
{
    Q_ASSERT(obj);
    QObjectPrivate *d = QObjectPrivate::get(const_cast<QObject *>(obj));
    Q_ASSERT_X(!d->wasDeleted, "QWeakPointer", "Detected QWeakPointer creation in a QObject being deleted");

    ExternalRefCountData *that = d->sharedRefcount;
    if (that) {
        that->weakref.ref();
        return that;
    }

    // we can create the refcount data because it doesn't exist
    ExternalRefCountData *x = new ExternalRefCountData(Qt::Uninitialized);
    x->strongref = -1;
    x->weakref = 2;  // the QWeakPointer that called us plus the QObject itself
    if (!d->sharedRefcount.testAndSetRelease(0, x)) {
        delete x;
        d->sharedRefcount->weakref.ref();
    }
    return d->sharedRefcount;
}

QT_END_NAMESPACE

#endif



//#  define QT_SHARED_POINTER_BACKTRACE_SUPPORT
#  ifdef QT_SHARED_POINTER_BACKTRACE_SUPPORT
#    if defined(__GLIBC__) && (__GLIBC__ >= 2) && !defined(__UCLIBC__) && !defined(QT_LINUXBASE)
#      define BACKTRACE_SUPPORTED
#    elif defined(Q_OS_MACX)
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
    Q_CORE_EXPORT void internalSafetyCheckAdd(const volatile void *);
    Q_CORE_EXPORT void internalSafetyCheckRemove(const volatile void *);
    Q_AUTOTEST_EXPORT void internalSafetyCheckCleanCheck();
}

/*!
    \internal
*/
void QtSharedPointer::internalSafetyCheckAdd(const volatile void *)
{
    // Qt 4.5 compatibility
    // this function is broken by design, so it was replaced with internalSafetyCheckAdd2
    //
    // it's broken because we tracked the pointers added and
    // removed from QSharedPointer, converted to void*.
    // That is, this is supposed to track the "top-of-object" pointer in
    // case of multiple inheritance.
    //
    // However, it doesn't work well in some compilers:
    // if you create an object with a class of type A and the last reference
    // is dropped of type B, then the value passed to internalSafetyCheckRemove could
    // be different than was added. That would leave dangling addresses.
    //
    // So instead, we track the pointer by the d-pointer instead.
}

/*!
    \internal
*/
void QtSharedPointer::internalSafetyCheckRemove(const volatile void *)
{
    // Qt 4.5 compatibility
    // see comments above
}

/*!
    \internal
*/
void QtSharedPointer::internalSafetyCheckAdd2(const void *d_ptr, const volatile void *ptr)
{
    // see comments above for the rationale for this function
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
void QtSharedPointer::internalSafetyCheckRemove2(const void *d_ptr)
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
