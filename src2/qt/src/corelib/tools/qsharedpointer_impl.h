/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#ifndef Q_QDOC

#ifndef QSHAREDPOINTER_H
#error Do not include qsharedpointer_impl.h directly
#endif

#if 0
// These macros are duplicated here to make syncqt not complain a about
// this header, as we have a "qt_sync_stop_processing" below, which in turn
// is here because this file contains a template mess and duplicates the
// classes found in qsharedpointer.h
QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE
QT_MODULE(Core)
QT_END_NAMESPACE
QT_END_HEADER
#pragma qt_sync_stop_processing
#endif

#include <new>
#include <QtCore/qatomic.h>
#include <QtCore/qobject.h>    // for qobject_cast

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

// Macro QSHAREDPOINTER_VERIFY_AUTO_CAST
//  generates a compiler error if the following construct isn't valid:
//    T *ptr1;
//    X *ptr2 = ptr1;
//
#ifdef QT_NO_DEBUG
# define QSHAREDPOINTER_VERIFY_AUTO_CAST(T, X)          qt_noop()
#else

template<typename T> inline void qt_sharedpointer_cast_check(T *) { }
# define QSHAREDPOINTER_VERIFY_AUTO_CAST(T, X)          \
    qt_sharedpointer_cast_check<T>(static_cast<X *>(0))
#endif

//
// forward declarations
//
template <class T> class QWeakPointer;
template <class T> class QSharedPointer;

template <class X, class T>
QSharedPointer<X> qSharedPointerCast(const QSharedPointer<T> &ptr);
template <class X, class T>
QSharedPointer<X> qSharedPointerDynamicCast(const QSharedPointer<T> &ptr);
template <class X, class T>
QSharedPointer<X> qSharedPointerConstCast(const QSharedPointer<T> &ptr);

#ifndef QT_NO_QOBJECT
template <class X, class T>
QSharedPointer<X> qSharedPointerObjectCast(const QSharedPointer<T> &ptr);
#endif

namespace QtSharedPointer {
    template <class T> class InternalRefCount;
    template <class T> class ExternalRefCount;

    template <class X, class Y> QSharedPointer<X> copyAndSetPointer(X * ptr, const QSharedPointer<Y> &src);

    // used in debug mode to verify the reuse of pointers
    Q_CORE_EXPORT void internalSafetyCheckAdd2(const void *, const volatile void *);
    Q_CORE_EXPORT void internalSafetyCheckRemove2(const void *);

    template <class T, typename Klass, typename RetVal>
    inline void executeDeleter(T *t, RetVal (Klass:: *memberDeleter)())
    { (t->*memberDeleter)(); }
    template <class T, typename Deleter>
    inline void executeDeleter(T *t, Deleter d)
    { d(t); }
    template <class T> inline void normalDeleter(T *t) { delete t; }

    // this uses partial template specialization
    template <class T> struct RemovePointer;
    template <class T> struct RemovePointer<T *> { typedef T Type; };
    template <class T> struct RemovePointer<QSharedPointer<T> > { typedef T Type; };
    template <class T> struct RemovePointer<QWeakPointer<T> > { typedef T Type; };

    // This class provides the basic functionality of a pointer wrapper.
    // Its existence is mostly legacy, since originally QSharedPointer
    // could also be used for internally-refcounted objects.
    template <class T>
    class Basic
    {
#ifndef Q_CC_NOKIAX86
        typedef T *Basic:: *RestrictedBool;
#endif
    public:
        typedef T Type;
        typedef T element_type;
        typedef T value_type;
        typedef value_type *pointer;
        typedef const value_type *const_pointer;
        typedef value_type &reference;
        typedef const value_type &const_reference;
        typedef qptrdiff difference_type;

        inline T *data() const { return value; }
        inline bool isNull() const { return !data(); }
#ifndef Q_CC_NOKIAX86
        inline operator RestrictedBool() const { return isNull() ? 0 : &Basic::value; }
#else
        inline operator bool() const { return isNull() ? 0 : &Basic::value; }
#endif
        inline bool operator !() const { return isNull(); }
        inline T &operator*() const { return *data(); }
        inline T *operator->() const { return data(); }

    protected:
        inline Basic(T *ptr = 0) : value(ptr) { }
        inline Basic(Qt::Initialization) { }
        // ~Basic();

        inline void internalConstruct(T *ptr)
        {
            value = ptr;
        }

#if defined(Q_NO_TEMPLATE_FRIENDS)
    public:
#else
        template <class X> friend class QT_PREPEND_NAMESPACE(QWeakPointer);
#endif

        Type *value;
    };

    // This class is the d-pointer of QSharedPointer and QWeakPointer.
    //
    // It is a reference-counted reference counter. "strongref" is the inner
    // reference counter, and it tracks the lifetime of the pointer itself.
    // "weakref" is the outer reference counter and it tracks the lifetime of
    // the ExternalRefCountData object.
    struct ExternalRefCountData
    {
        QBasicAtomicInt weakref;
        QBasicAtomicInt strongref;

        inline ExternalRefCountData()
        {
            strongref = 1;
            weakref = 1;
        }
        inline ExternalRefCountData(Qt::Initialization) { }
        virtual inline ~ExternalRefCountData() { Q_ASSERT(!weakref); Q_ASSERT(strongref <= 0); }

        // overridden by derived classes
        // returns false to indicate caller should delete the pointer
        // returns true in case it has already done so
        virtual inline bool destroy() { return false; }

#ifndef QT_NO_QOBJECT
        Q_CORE_EXPORT static ExternalRefCountData *getAndRef(const QObject *);
        Q_CORE_EXPORT void setQObjectShared(const QObject *, bool enable);
#endif
        inline void setQObjectShared(...) { }
    };
    // sizeof(ExternalRefCount) = 12 (32-bit) / 16 (64-bit)

    // This class extends ExternalRefCountData with a pointer
    // to a function, which is called by the destroy() function.
    struct ExternalRefCountWithDestroyFn: public ExternalRefCountData
    {
        typedef void (*DestroyerFn)(ExternalRefCountData *);
        DestroyerFn destroyer;

        inline ExternalRefCountWithDestroyFn(DestroyerFn d)
            : destroyer(d)
        { }

        inline bool destroy() { destroyer(this); return true; }
        inline void operator delete(void *ptr) { ::operator delete(ptr); }
        inline void operator delete(void *, void *) { }
    };
    // sizeof(ExternalRefCountWithDestroyFn) = 16 (32-bit) / 24 (64-bit)

    // This class extends ExternalRefCountWithDestroyFn and implements
    // the static function that deletes the object. The pointer and the
    // custom deleter are kept in the "extra" member.
    template <class T, typename Deleter>
    struct ExternalRefCountWithCustomDeleter: public ExternalRefCountWithDestroyFn
    {
        typedef ExternalRefCountWithCustomDeleter Self;
        typedef ExternalRefCountWithDestroyFn BaseClass;

        struct CustomDeleter
        {
            Deleter deleter;
            T *ptr;

            inline CustomDeleter(T *p, Deleter d) : deleter(d), ptr(p) {}
        };
        CustomDeleter extra;
        // sizeof(CustomDeleter) = sizeof(Deleter) + sizeof(void*)
        // for Deleter = function pointer:  8 (32-bit) / 16 (64-bit)
        // for Deleter = PMF: 12 (32-bit) / 24 (64-bit)  (GCC)

        static inline void deleter(ExternalRefCountData *self)
        {
            Self *realself = static_cast<Self *>(self);
            executeDeleter(realself->extra.ptr, realself->extra.deleter);

            // delete the deleter too
            realself->extra.~CustomDeleter();
        }
        static void safetyCheckDeleter(ExternalRefCountData *self)
        {
            internalSafetyCheckRemove2(self);
            deleter(self);
        }

        static inline Self *create(T *ptr, Deleter userDeleter)
        {
# ifdef QT_SHAREDPOINTER_TRACK_POINTERS
            DestroyerFn destroy = &safetyCheckDeleter;
# else
            DestroyerFn destroy = &deleter;
# endif
            Self *d = static_cast<Self *>(::operator new(sizeof(Self)));

            // initialize the two sub-objects
            new (&d->extra) CustomDeleter(ptr, userDeleter);
            new (d) BaseClass(destroy); // can't throw

            return d;
        }
    private:
        // prevent construction and the emission of virtual symbols
        ExternalRefCountWithCustomDeleter();
        ~ExternalRefCountWithCustomDeleter();
    };

    // This class extends ExternalRefCountWithDestroyFn and adds a "T"
    // member. That way, when the create() function is called, we allocate
    // memory for both QSharedPointer's d-pointer and the actual object being
    // tracked.
    template <class T>
    struct ExternalRefCountWithContiguousData: public ExternalRefCountWithDestroyFn
    {
        typedef ExternalRefCountWithDestroyFn Parent;
        T data;

        static void deleter(ExternalRefCountData *self)
        {
            ExternalRefCountWithContiguousData *that =
                    static_cast<ExternalRefCountWithContiguousData *>(self);
            that->data.~T();
        }
        static void safetyCheckDeleter(ExternalRefCountData *self)
        {
            internalSafetyCheckRemove2(self);
            deleter(self);
        }

        static inline ExternalRefCountData *create(T **ptr)
        {
# ifdef QT_SHAREDPOINTER_TRACK_POINTERS
            DestroyerFn destroy = &safetyCheckDeleter;
# else
            DestroyerFn destroy = &deleter;
# endif
            ExternalRefCountWithContiguousData *d =
                static_cast<ExternalRefCountWithContiguousData *>(::operator new(sizeof(ExternalRefCountWithContiguousData)));

            // initialize the d-pointer sub-object
            // leave d->data uninitialized
            new (d) Parent(destroy); // can't throw

            *ptr = &d->data;
            return d;
        }

    private:
        // prevent construction and the emission of virtual symbols
        ExternalRefCountWithContiguousData();
        ~ExternalRefCountWithContiguousData();
    };

    // This is the main body of QSharedPointer. It implements the
    // external reference counting functionality.
    template <class T>
    class ExternalRefCount: public Basic<T>
    {
    protected:
        typedef ExternalRefCountData Data;

        inline void deref()
        { deref(d, this->value); }
        static inline void deref(Data *d, T *value)
        {
            if (!d) return;
            if (!d->strongref.deref()) {
                if (!d->destroy())
                    delete value;
            }
            if (!d->weakref.deref())
                delete d;
        }

        inline void internalConstruct(T *ptr)
        {
#ifdef QT_SHAREDPOINTER_TRACK_POINTERS
            internalConstruct<void (*)(T *)>(ptr, normalDeleter);
#else
            if (ptr)
                d = new Data;
            else
                d = 0;
            internalFinishConstruction(ptr);
#endif
        }

        template <typename Deleter>
        inline void internalConstruct(T *ptr, Deleter deleter)
        {
            if (ptr)
                d = ExternalRefCountWithCustomDeleter<T, Deleter>::create(ptr, deleter);
            else
                d = 0;
            internalFinishConstruction(ptr);
        }

        inline void internalCreate()
        {
            T *ptr;
            d = ExternalRefCountWithContiguousData<T>::create(&ptr);
            Basic<T>::internalConstruct(ptr);
        }

        inline void internalFinishConstruction(T *ptr)
        {
            Basic<T>::internalConstruct(ptr);
#ifdef QT_SHAREDPOINTER_TRACK_POINTERS
            if (ptr) internalSafetyCheckAdd2(d, ptr);
#endif
            if (ptr) d->setQObjectShared(ptr, true);
        }

        inline ExternalRefCount() : d(0) { }
        inline ExternalRefCount(Qt::Initialization i) : Basic<T>(i) { }

        inline ExternalRefCount(T *ptr) : Basic<T>(Qt::Uninitialized) // throws
        { internalConstruct(ptr); }
        template <typename Deleter>
        inline ExternalRefCount(T *ptr, Deleter deleter) : Basic<T>(Qt::Uninitialized) // throws
        { internalConstruct(ptr, deleter); }

        inline ExternalRefCount(const ExternalRefCount<T> &other) : Basic<T>(other), d(other.d)
        { if (d) ref(); }
        template <class X>
        inline ExternalRefCount(const ExternalRefCount<X> &other) : Basic<T>(other.value), d(other.d)
        { if (d) ref(); }
        inline ~ExternalRefCount() { deref(); }

        template <class X>
        inline void internalCopy(const ExternalRefCount<X> &other)
        {
            Data *o = other.d;
            T *actual = other.value;
            if (o)
                other.ref();
            qSwap(d, o);
            qSwap(this->value, actual);
            deref(o, actual);
        }

        inline void internalSwap(ExternalRefCount &other)
        {
            qSwap(d, other.d);
            qSwap(this->value, other.value);
        }

#if defined(Q_NO_TEMPLATE_FRIENDS)
    public:
#else
        template <class X> friend class ExternalRefCount;
        template <class X> friend class QT_PREPEND_NAMESPACE(QWeakPointer);
        template <class X, class Y> friend QSharedPointer<X> copyAndSetPointer(X * ptr, const QSharedPointer<Y> &src);
#endif
        inline void ref() const { d->weakref.ref(); d->strongref.ref(); }

        inline void internalSet(Data *o, T *actual)
        {
            if (o) {
                // increase the strongref, but never up from zero
                // or less (-1 is used by QWeakPointer on untracked QObject)
                register int tmp = o->strongref;
                while (tmp > 0) {
                    // try to increment from "tmp" to "tmp + 1"
                    if (o->strongref.testAndSetRelaxed(tmp, tmp + 1))
                        break;   // succeeded
                    tmp = o->strongref;  // failed, try again
                }

                if (tmp > 0)
                    o->weakref.ref();
                else
                    o = 0;
            }

            qSwap(d, o);
            qSwap(this->value, actual);
            if (!d || d->strongref == 0)
                this->value = 0;

            // dereference saved data
            deref(o, actual);
        }

        Data *d;

    private:
        template<class X> ExternalRefCount(const InternalRefCount<X> &);
    };
} // namespace QtSharedPointer

template <class T>
class QSharedPointer: public QtSharedPointer::ExternalRefCount<T>
{
    typedef typename QtSharedPointer::ExternalRefCount<T> BaseClass;
public:
    inline QSharedPointer() { }
    // inline ~QSharedPointer() { }

    inline explicit QSharedPointer(T *ptr) : BaseClass(ptr) // throws
    { }

    template <typename Deleter>
    inline QSharedPointer(T *ptr, Deleter d) : BaseClass(ptr, d) // throws
    { }

    inline QSharedPointer(const QSharedPointer<T> &other) : BaseClass(other) { }
    inline QSharedPointer<T> &operator=(const QSharedPointer<T> &other)
    {
        BaseClass::internalCopy(other);
        return *this;
    }
#ifdef Q_COMPILER_RVALUE_REFS
    inline QSharedPointer<T> &operator=(QSharedPointer<T> &&other)
    {
        QSharedPointer<T>::internalSwap(other);
        return *this;
    }
#endif

    template <class X>
    inline QSharedPointer(const QSharedPointer<X> &other) : BaseClass(other)
    { }

    template <class X>
    inline QSharedPointer<T> &operator=(const QSharedPointer<X> &other)
    {
        QSHAREDPOINTER_VERIFY_AUTO_CAST(T, X); // if you get an error in this line, the cast is invalid
        BaseClass::internalCopy(other);
        return *this;
    }

    template <class X>
    inline QSharedPointer(const QWeakPointer<X> &other) : BaseClass(Qt::Uninitialized)
    { this->d = 0; *this = other; }

    template <class X>
    inline QSharedPointer<T> &operator=(const QWeakPointer<X> &other)
    { BaseClass::internalSet(other.d, other.value); return *this; }

    inline void swap(QSharedPointer &other)
    { QSharedPointer<T>::internalSwap(other); }

    template <class X>
    QSharedPointer<X> staticCast() const
    {
        return qSharedPointerCast<X, T>(*this);
    }

    template <class X>
    QSharedPointer<X> dynamicCast() const
    {
        return qSharedPointerDynamicCast<X, T>(*this);
    }

    template <class X>
    QSharedPointer<X> constCast() const
    {
        return qSharedPointerConstCast<X, T>(*this);
    }

#ifndef QT_NO_QOBJECT
    template <class X>
    QSharedPointer<X> objectCast() const
    {
        return qSharedPointerObjectCast<X, T>(*this);
    }
#endif

    inline void clear() { *this = QSharedPointer<T>(); }

    QWeakPointer<T> toWeakRef() const;

protected:
    inline explicit QSharedPointer(Qt::Initialization i) : BaseClass(i) {}

public:
    static inline QSharedPointer<T> create()
    {
        QSharedPointer<T> result(Qt::Uninitialized);
        result.internalCreate();

        // now initialize the data
        new (result.data()) T();
        result.internalFinishConstruction(result.data());
        return result;
    }
};

template <class T>
class QWeakPointer
{
#ifndef Q_CC_NOKIAX86
    typedef T *QWeakPointer:: *RestrictedBool;
#endif
    typedef QtSharedPointer::ExternalRefCountData Data;

public:
    typedef T element_type;
    typedef T value_type;
    typedef value_type *pointer;
    typedef const value_type *const_pointer;
    typedef value_type &reference;
    typedef const value_type &const_reference;
    typedef qptrdiff difference_type;

    inline bool isNull() const { return d == 0 || d->strongref == 0 || value == 0; }
#ifndef Q_CC_NOKIAX86
    inline operator RestrictedBool() const { return isNull() ? 0 : &QWeakPointer::value; }
#else
    inline operator bool() const { return isNull() ? 0 : &QWeakPointer::value; }
#endif
    inline bool operator !() const { return isNull(); }
    inline T *data() const { return d == 0 || d->strongref == 0 ? 0 : value; }

    inline QWeakPointer() : d(0), value(0) { }
    inline ~QWeakPointer() { if (d && !d->weakref.deref()) delete d; }

#ifndef QT_NO_QOBJECT
    // special constructor that is enabled only if X derives from QObject
    template <class X>
    inline QWeakPointer(X *ptr) : d(ptr ? Data::getAndRef(ptr) : 0), value(ptr)
    { }
#endif
    template <class X>
    inline QWeakPointer &operator=(X *ptr)
    { return *this = QWeakPointer(ptr); }

    inline QWeakPointer(const QWeakPointer<T> &o) : d(o.d), value(o.value)
    { if (d) d->weakref.ref(); }
    inline QWeakPointer<T> &operator=(const QWeakPointer<T> &o)
    {
        internalSet(o.d, o.value);
        return *this;
    }

    inline QWeakPointer(const QSharedPointer<T> &o) : d(o.d), value(o.data())
    { if (d) d->weakref.ref();}
    inline QWeakPointer<T> &operator=(const QSharedPointer<T> &o)
    {
        internalSet(o.d, o.value);
        return *this;
    }

    template <class X>
    inline QWeakPointer(const QWeakPointer<X> &o) : d(0), value(0)
    { *this = o; }

    template <class X>
    inline QWeakPointer<T> &operator=(const QWeakPointer<X> &o)
    {
        // conversion between X and T could require access to the virtual table
        // so force the operation to go through QSharedPointer
        *this = o.toStrongRef();
        return *this;
    }

    template <class X>
    inline bool operator==(const QWeakPointer<X> &o) const
    { return d == o.d && value == static_cast<const T *>(o.value); }

    template <class X>
    inline bool operator!=(const QWeakPointer<X> &o) const
    { return !(*this == o); }

    template <class X>
    inline QWeakPointer(const QSharedPointer<X> &o) : d(0), value(0)
    { *this = o; }

    template <class X>
    inline QWeakPointer<T> &operator=(const QSharedPointer<X> &o)
    {
        QSHAREDPOINTER_VERIFY_AUTO_CAST(T, X); // if you get an error in this line, the cast is invalid
        internalSet(o.d, o.data());
        return *this;
    }

    template <class X>
    inline bool operator==(const QSharedPointer<X> &o) const
    { return d == o.d; }

    template <class X>
    inline bool operator!=(const QSharedPointer<X> &o) const
    { return !(*this == o); }

    inline void clear() { *this = QWeakPointer<T>(); }

    inline QSharedPointer<T> toStrongRef() const { return QSharedPointer<T>(*this); }

#if defined(QWEAKPOINTER_ENABLE_ARROW)
    inline T *operator->() const { return data(); }
#endif

private:

#if defined(Q_NO_TEMPLATE_FRIENDS)
public:
#else
    template <class X> friend class QSharedPointer;
#endif

    inline void internalSet(Data *o, T *actual)
    {
        if (d == o) return;
        if (o)
            o->weakref.ref();
        if (d && !d->weakref.deref())
            delete d;
        d = o;
        value = actual;
    }

    Data *d;
    T *value;
};

//
// operator== and operator!=
//
template <class T, class X>
bool operator==(const QSharedPointer<T> &ptr1, const QSharedPointer<X> &ptr2)
{
    return ptr1.data() == ptr2.data();
}
template <class T, class X>
bool operator!=(const QSharedPointer<T> &ptr1, const QSharedPointer<X> &ptr2)
{
    return ptr1.data() != ptr2.data();
}

template <class T, class X>
bool operator==(const QSharedPointer<T> &ptr1, const X *ptr2)
{
    return ptr1.data() == ptr2;
}
template <class T, class X>
bool operator==(const T *ptr1, const QSharedPointer<X> &ptr2)
{
    return ptr1 == ptr2.data();
}
template <class T, class X>
bool operator!=(const QSharedPointer<T> &ptr1, const X *ptr2)
{
    return !(ptr1 == ptr2);
}
template <class T, class X>
bool operator!=(const T *ptr1, const QSharedPointer<X> &ptr2)
{
    return !(ptr2 == ptr1);
}

template <class T, class X>
bool operator==(const QSharedPointer<T> &ptr1, const QWeakPointer<X> &ptr2)
{
    return ptr2 == ptr1;
}
template <class T, class X>
bool operator!=(const QSharedPointer<T> &ptr1, const QWeakPointer<X> &ptr2)
{
    return ptr2 != ptr1;
}

//
// operator-
//
template <class T, class X>
Q_INLINE_TEMPLATE typename QSharedPointer<T>::difference_type operator-(const QSharedPointer<T> &ptr1, const QSharedPointer<X> &ptr2)
{
    return ptr1.data() - ptr2.data();
}
template <class T, class X>
Q_INLINE_TEMPLATE typename QSharedPointer<T>::difference_type operator-(const QSharedPointer<T> &ptr1, X *ptr2)
{
    return ptr1.data() - ptr2;
}
template <class T, class X>
Q_INLINE_TEMPLATE typename QSharedPointer<X>::difference_type operator-(T *ptr1, const QSharedPointer<X> &ptr2)
{
    return ptr1 - ptr2.data();
}

//
// operator<
//
template <class T, class X>
Q_INLINE_TEMPLATE bool operator<(const QSharedPointer<T> &ptr1, const QSharedPointer<X> &ptr2)
{
    return ptr1.data() < ptr2.data();
}
template <class T, class X>
Q_INLINE_TEMPLATE bool operator<(const QSharedPointer<T> &ptr1, X *ptr2)
{
    return ptr1.data() < ptr2;
}
template <class T, class X>
Q_INLINE_TEMPLATE bool operator<(T *ptr1, const QSharedPointer<X> &ptr2)
{
    return ptr1 < ptr2.data();
}

//
// qHash
//
template <class T> inline uint qHash(const T *key); // defined in qhash.h
template <class T>
Q_INLINE_TEMPLATE uint qHash(const QSharedPointer<T> &ptr)
{
    return QT_PREPEND_NAMESPACE(qHash)<T>(ptr.data());
}


template <class T>
Q_INLINE_TEMPLATE QWeakPointer<T> QSharedPointer<T>::toWeakRef() const
{
    return QWeakPointer<T>(*this);
}

template <class T>
inline void qSwap(QSharedPointer<T> &p1, QSharedPointer<T> &p2)
{
    p1.swap(p2);
}

#ifndef QT_NO_STL
QT_END_NAMESPACE
namespace std {
    template <class T>
    inline void swap(QT_PREPEND_NAMESPACE(QSharedPointer)<T> &p1, QT_PREPEND_NAMESPACE(QSharedPointer)<T> &p2)
    { p1.swap(p2); }
}
QT_BEGIN_NAMESPACE
#endif

namespace QtSharedPointer {
// helper functions:
    template <class X, class T>
    Q_INLINE_TEMPLATE QSharedPointer<X> copyAndSetPointer(X *ptr, const QSharedPointer<T> &src)
    {
        QSharedPointer<X> result;
        result.internalSet(src.d, ptr);
        return result;
    }
}

// cast operators
template <class X, class T>
Q_INLINE_TEMPLATE QSharedPointer<X> qSharedPointerCast(const QSharedPointer<T> &src)
{
    register X *ptr = static_cast<X *>(src.data()); // if you get an error in this line, the cast is invalid
    return QtSharedPointer::copyAndSetPointer(ptr, src);
}
template <class X, class T>
Q_INLINE_TEMPLATE QSharedPointer<X> qSharedPointerCast(const QWeakPointer<T> &src)
{
    return qSharedPointerCast<X, T>(src.toStrongRef());
}

template <class X, class T>
Q_INLINE_TEMPLATE QSharedPointer<X> qSharedPointerDynamicCast(const QSharedPointer<T> &src)
{
    register X *ptr = dynamic_cast<X *>(src.data()); // if you get an error in this line, the cast is invalid
    return QtSharedPointer::copyAndSetPointer(ptr, src);
}
template <class X, class T>
Q_INLINE_TEMPLATE QSharedPointer<X> qSharedPointerDynamicCast(const QWeakPointer<T> &src)
{
    return qSharedPointerDynamicCast<X, T>(src.toStrongRef());
}

template <class X, class T>
Q_INLINE_TEMPLATE QSharedPointer<X> qSharedPointerConstCast(const QSharedPointer<T> &src)
{
    register X *ptr = const_cast<X *>(src.data()); // if you get an error in this line, the cast is invalid
    return QtSharedPointer::copyAndSetPointer(ptr, src);
}
template <class X, class T>
Q_INLINE_TEMPLATE QSharedPointer<X> qSharedPointerConstCast(const QWeakPointer<T> &src)
{
    return qSharedPointerConstCast<X, T>(src.toStrongRef());
}

template <class X, class T>
Q_INLINE_TEMPLATE
QWeakPointer<X> qWeakPointerCast(const QSharedPointer<T> &src)
{
    return qSharedPointerCast<X, T>(src).toWeakRef();
}

#ifndef QT_NO_QOBJECT
template <class X, class T>
Q_INLINE_TEMPLATE QSharedPointer<X> qSharedPointerObjectCast(const QSharedPointer<T> &src)
{
    register X *ptr = qobject_cast<X *>(src.data());
    return QtSharedPointer::copyAndSetPointer(ptr, src);
}
template <class X, class T>
Q_INLINE_TEMPLATE QSharedPointer<X> qSharedPointerObjectCast(const QWeakPointer<T> &src)
{
    return qSharedPointerObjectCast<X>(src.toStrongRef());
}

template <class X, class T>
inline QSharedPointer<typename QtSharedPointer::RemovePointer<X>::Type>
qobject_cast(const QSharedPointer<T> &src)
{
    return qSharedPointerObjectCast<typename QtSharedPointer::RemovePointer<X>::Type, T>(src);
}
template <class X, class T>
inline QSharedPointer<typename QtSharedPointer::RemovePointer<X>::Type>
qobject_cast(const QWeakPointer<T> &src)
{
    return qSharedPointerObjectCast<typename QtSharedPointer::RemovePointer<X>::Type, T>(src);
}
#endif


template<typename T> Q_DECLARE_TYPEINFO_BODY(QWeakPointer<T>, Q_MOVABLE_TYPE);
template<typename T> Q_DECLARE_TYPEINFO_BODY(QSharedPointer<T>, Q_MOVABLE_TYPE);


QT_END_NAMESPACE

QT_END_HEADER

#endif
