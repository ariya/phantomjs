/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef Q_QDOC

#ifndef QSHAREDPOINTER_H
#error Do not include qsharedpointer_impl.h directly
#endif

#if 0
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#if 0
// These macros are duplicated here to make syncqt not complain a about
// this header, as we have a "qt_sync_stop_processing" below, which in turn
// is here because this file contains a template mess and duplicates the
// classes found in qsharedpointer.h
QT_BEGIN_NAMESPACE
QT_END_NAMESPACE
#pragma qt_sync_stop_processing
#endif

#include <new>
#include <QtCore/qatomic.h>
#include <QtCore/qobject.h>    // for qobject_cast
#include <QtCore/qhash.h>    // for qHash

#if defined(Q_COMPILER_RVALUE_REFS) && defined(Q_COMPILER_VARIADIC_TEMPLATES)
#  include <utility>           // for std::forward
#endif

QT_BEGIN_NAMESPACE


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
template <class T> class QEnableSharedFromThis;

class QVariant;

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
    template <class T> class ExternalRefCount;

    template <class X, class Y> QSharedPointer<X> copyAndSetPointer(X * ptr, const QSharedPointer<Y> &src);

    // used in debug mode to verify the reuse of pointers
    Q_CORE_EXPORT void internalSafetyCheckAdd(const void *, const volatile void *);
    Q_CORE_EXPORT void internalSafetyCheckRemove(const void *);

    template <class T, typename Klass, typename RetVal>
    inline void executeDeleter(T *t, RetVal (Klass:: *memberDeleter)())
    { (t->*memberDeleter)(); }
    template <class T, typename Deleter>
    inline void executeDeleter(T *t, Deleter d)
    { d(t); }
    struct NormalDeleter {};

    // this uses partial template specialization
    template <class T> struct RemovePointer;
    template <class T> struct RemovePointer<T *> { typedef T Type; };
    template <class T> struct RemovePointer<QSharedPointer<T> > { typedef T Type; };
    template <class T> struct RemovePointer<QWeakPointer<T> > { typedef T Type; };

    // This class is the d-pointer of QSharedPointer and QWeakPointer.
    //
    // It is a reference-counted reference counter. "strongref" is the inner
    // reference counter, and it tracks the lifetime of the pointer itself.
    // "weakref" is the outer reference counter and it tracks the lifetime of
    // the ExternalRefCountData object.
    //
    // The deleter is stored in the destroyer member and is always a pointer to
    // a static function in ExternalRefCountWithCustomDeleter or in
    // ExternalRefCountWithContiguousData
    struct ExternalRefCountData
    {
        typedef void (*DestroyerFn)(ExternalRefCountData *);
        QBasicAtomicInt weakref;
        QBasicAtomicInt strongref;
        DestroyerFn destroyer;

        inline ExternalRefCountData(DestroyerFn d)
            : destroyer(d)
        {
            strongref.store(1);
            weakref.store(1);
        }
        inline ExternalRefCountData(Qt::Initialization) { }
        ~ExternalRefCountData() { Q_ASSERT(!weakref.load()); Q_ASSERT(strongref.load() <= 0); }

        void destroy() { destroyer(this); }

#ifndef QT_NO_QOBJECT
        Q_CORE_EXPORT static ExternalRefCountData *getAndRef(const QObject *);
        Q_CORE_EXPORT void setQObjectShared(const QObject *, bool enable);
        Q_CORE_EXPORT void checkQObjectShared(const QObject *);
#endif
        inline void checkQObjectShared(...) { }
        inline void setQObjectShared(...) { }

        inline void operator delete(void *ptr) { ::operator delete(ptr); }
        inline void operator delete(void *, void *) { }
    };
    // sizeof(ExternalRefCountData) = 12 (32-bit) / 16 (64-bit)

    template <class T, typename Deleter>
    struct CustomDeleter
    {
        Deleter deleter;
        T *ptr;

        CustomDeleter(T *p, Deleter d) : deleter(d), ptr(p) {}
        void execute() { executeDeleter(ptr, deleter); }
    };
    // sizeof(CustomDeleter) = sizeof(Deleter) + sizeof(void*) + padding
    // for Deleter = stateless functor: 8 (32-bit) / 16 (64-bit) due to padding
    // for Deleter = function pointer:  8 (32-bit) / 16 (64-bit)
    // for Deleter = PMF: 12 (32-bit) / 24 (64-bit)  (GCC)

    // This specialization of CustomDeleter for a deleter of type NormalDeleter
    // is an optimization: instead of storing a pointer to a function that does
    // the deleting, we simply delete the pointer ourselves.
    template <class T>
    struct CustomDeleter<T, NormalDeleter>
    {
        T *ptr;

        CustomDeleter(T *p, NormalDeleter) : ptr(p) {}
        void execute() { delete ptr; }
    };
    // sizeof(CustomDeleter specialization) = sizeof(void*)

    // This class extends ExternalRefCountData and implements
    // the static function that deletes the object. The pointer and the
    // custom deleter are kept in the "extra" member so we can construct
    // and destruct it independently of the full structure.
    template <class T, typename Deleter>
    struct ExternalRefCountWithCustomDeleter: public ExternalRefCountData
    {
        typedef ExternalRefCountWithCustomDeleter Self;
        typedef ExternalRefCountData BaseClass;
        CustomDeleter<T, Deleter> extra;

        static inline void deleter(ExternalRefCountData *self)
        {
            Self *realself = static_cast<Self *>(self);
            realself->extra.execute();

            // delete the deleter too
            realself->extra.~CustomDeleter<T, Deleter>();
        }
        static void safetyCheckDeleter(ExternalRefCountData *self)
        {
            internalSafetyCheckRemove(self);
            deleter(self);
        }

        static inline Self *create(T *ptr, Deleter userDeleter, DestroyerFn actualDeleter)
        {
            Self *d = static_cast<Self *>(::operator new(sizeof(Self)));

            // initialize the two sub-objects
            new (&d->extra) CustomDeleter<T, Deleter>(ptr, userDeleter);
            new (d) BaseClass(actualDeleter); // can't throw

            return d;
        }
    private:
        // prevent construction
        ExternalRefCountWithCustomDeleter() Q_DECL_EQ_DELETE;
        ~ExternalRefCountWithCustomDeleter() Q_DECL_EQ_DELETE;
        Q_DISABLE_COPY(ExternalRefCountWithCustomDeleter)
    };

    // This class extends ExternalRefCountData and adds a "T"
    // member. That way, when the create() function is called, we allocate
    // memory for both QSharedPointer's d-pointer and the actual object being
    // tracked.
    template <class T>
    struct ExternalRefCountWithContiguousData: public ExternalRefCountData
    {
        typedef ExternalRefCountData Parent;
        T data;

        static void deleter(ExternalRefCountData *self)
        {
            ExternalRefCountWithContiguousData *that =
                    static_cast<ExternalRefCountWithContiguousData *>(self);
            that->data.~T();
        }
        static void safetyCheckDeleter(ExternalRefCountData *self)
        {
            internalSafetyCheckRemove(self);
            deleter(self);
        }

        static inline ExternalRefCountData *create(T **ptr, DestroyerFn destroy)
        {
            ExternalRefCountWithContiguousData *d =
                static_cast<ExternalRefCountWithContiguousData *>(::operator new(sizeof(ExternalRefCountWithContiguousData)));

            // initialize the d-pointer sub-object
            // leave d->data uninitialized
            new (d) Parent(destroy); // can't throw

            *ptr = &d->data;
            return d;
        }

    private:
        // prevent construction
        ExternalRefCountWithContiguousData() Q_DECL_EQ_DELETE;
        ~ExternalRefCountWithContiguousData() Q_DECL_EQ_DELETE;
        Q_DISABLE_COPY(ExternalRefCountWithContiguousData)
    };

#ifndef QT_NO_QOBJECT
    Q_CORE_EXPORT QWeakPointer<QObject> weakPointerFromVariant_internal(const QVariant &variant);
    Q_CORE_EXPORT QSharedPointer<QObject> sharedPointerFromVariant_internal(const QVariant &variant);
#endif
} // namespace QtSharedPointer

template <class T> class QSharedPointer
{
    typedef T *QSharedPointer:: *RestrictedBool;
    typedef QtSharedPointer::ExternalRefCountData Data;
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
    inline operator RestrictedBool() const { return isNull() ? 0 : &QSharedPointer::value; }
    inline bool operator !() const { return isNull(); }
    inline T &operator*() const { return *data(); }
    inline T *operator->() const { return data(); }

    QSharedPointer() : value(0), d(0) { }
    ~QSharedPointer() { deref(); }

    inline explicit QSharedPointer(T *ptr) : value(ptr) // noexcept
    { internalConstruct(ptr, QtSharedPointer::NormalDeleter()); }

    template <typename Deleter>
    inline QSharedPointer(T *ptr, Deleter deleter) : value(ptr) // throws
    { internalConstruct(ptr, deleter); }

    inline QSharedPointer(const QSharedPointer &other) : value(other.value), d(other.d)
    { if (d) ref(); }
    inline QSharedPointer &operator=(const QSharedPointer &other)
    {
        QSharedPointer copy(other);
        swap(copy);
        return *this;
    }
#ifdef Q_COMPILER_RVALUE_REFS
    inline QSharedPointer(QSharedPointer &&other)
        : value(other.value), d(other.d)
    {
        other.d = 0;
        other.value = 0;
    }
    inline QSharedPointer &operator=(QSharedPointer &&other)
    {
        swap(other);
        return *this;
    }
#endif

    template <class X>
    inline QSharedPointer(const QSharedPointer<X> &other) : value(other.value), d(other.d)
    { if (d) ref(); }

    template <class X>
    inline QSharedPointer &operator=(const QSharedPointer<X> &other)
    {
        QSHAREDPOINTER_VERIFY_AUTO_CAST(T, X); // if you get an error in this line, the cast is invalid
        internalCopy(other);
        return *this;
    }

    template <class X>
    inline QSharedPointer(const QWeakPointer<X> &other) : value(0), d(0)
    { *this = other; }

    template <class X>
    inline QSharedPointer<T> &operator=(const QWeakPointer<X> &other)
    { internalSet(other.d, other.value); return *this; }

    inline void swap(QSharedPointer &other)
    { this->internalSwap(other); }

    inline void reset() { clear(); }
    inline void reset(T *t)
    { QSharedPointer copy(t); swap(copy); }
    template <typename Deleter>
    inline void reset(T *t, Deleter deleter)
    { QSharedPointer copy(t, deleter); swap(copy); }

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

    inline void clear() { QSharedPointer copy; swap(copy); }

    QWeakPointer<T> toWeakRef() const;

#if defined(Q_COMPILER_RVALUE_REFS) && defined(Q_COMPILER_VARIADIC_TEMPLATES)
    template <typename... Args>
    static QSharedPointer create(Args && ...arguments)
    {
        typedef QtSharedPointer::ExternalRefCountWithContiguousData<T> Private;
# ifdef QT_SHAREDPOINTER_TRACK_POINTERS
        typename Private::DestroyerFn destroy = &Private::safetyCheckDeleter;
# else
        typename Private::DestroyerFn destroy = &Private::deleter;
# endif
        QSharedPointer result(Qt::Uninitialized);
        result.d = Private::create(&result.value, destroy);

        // now initialize the data
        new (result.data()) T(std::forward<Args>(arguments)...);
        result.d->setQObjectShared(result.value, true);
# ifdef QT_SHAREDPOINTER_TRACK_POINTERS
        internalSafetyCheckAdd(result.d, result.value);
# endif
        return result;
    }
#else
    static inline QSharedPointer create()
    {
        typedef QtSharedPointer::ExternalRefCountWithContiguousData<T> Private;
# ifdef QT_SHAREDPOINTER_TRACK_POINTERS
        typename Private::DestroyerFn destroy = &Private::safetyCheckDeleter;
# else
        typename Private::DestroyerFn destroy = &Private::deleter;
# endif
        QSharedPointer result(Qt::Uninitialized);
        result.d = Private::create(&result.value, destroy);

        // now initialize the data
        new (result.data()) T();
# ifdef QT_SHAREDPOINTER_TRACK_POINTERS
        internalSafetyCheckAdd(result.d, result.value);
# endif
        result.d->setQObjectShared(result.value, true);
        return result;
    }

    template <typename Arg>
    static inline QSharedPointer create(const Arg &arg)
    {
        typedef QtSharedPointer::ExternalRefCountWithContiguousData<T> Private;
# ifdef QT_SHAREDPOINTER_TRACK_POINTERS
        typename Private::DestroyerFn destroy = &Private::safetyCheckDeleter;
# else
        typename Private::DestroyerFn destroy = &Private::deleter;
# endif
        QSharedPointer result(Qt::Uninitialized);
        result.d = Private::create(&result.value, destroy);

        // now initialize the data
        new (result.data()) T(arg);
# ifdef QT_SHAREDPOINTER_TRACK_POINTERS
        internalSafetyCheckAdd(result.d, result.value);
# endif
        result.d->setQObjectShared(result.value, true);
        return result;
    }
#endif

private:
    explicit QSharedPointer(Qt::Initialization) {}

    inline void deref()
    { deref(d); }
    static inline void deref(Data *d)
    {
        if (!d) return;
        if (!d->strongref.deref()) {
            d->destroy();
        }
        if (!d->weakref.deref())
            delete d;
    }

    template <class X>
    inline void enableSharedFromThis(const QEnableSharedFromThis<X> *ptr)
    {
        ptr->initializeFromSharedPointer(*this);
    }

    inline void enableSharedFromThis(...) {}

    template <typename Deleter>
    inline void internalConstruct(T *ptr, Deleter deleter)
    {
        if (!ptr) {
            d = 0;
            return;
        }

        typedef QtSharedPointer::ExternalRefCountWithCustomDeleter<T, Deleter> Private;
# ifdef QT_SHAREDPOINTER_TRACK_POINTERS
        typename Private::DestroyerFn actualDeleter = &Private::safetyCheckDeleter;
# else
        typename Private::DestroyerFn actualDeleter = &Private::deleter;
# endif
        d = Private::create(ptr, deleter, actualDeleter);

#ifdef QT_SHAREDPOINTER_TRACK_POINTERS
        internalSafetyCheckAdd(d, ptr);
#endif
        d->setQObjectShared(ptr, true);
        enableSharedFromThis(ptr);
    }

    template <class X>
    inline void internalCopy(const QSharedPointer<X> &other)
    {
        Data *o = other.d;
        T *actual = other.value;
        if (o)
            other.ref();
        qSwap(d, o);
        qSwap(this->value, actual);
        deref(o);
    }

    inline void internalSwap(QSharedPointer &other)
    {
        qSwap(d, other.d);
        qSwap(this->value, other.value);
    }

#if defined(Q_NO_TEMPLATE_FRIENDS)
public:
#else
    template <class X> friend class QSharedPointer;
    template <class X> friend class QWeakPointer;
    template <class X, class Y> friend QSharedPointer<X> QtSharedPointer::copyAndSetPointer(X * ptr, const QSharedPointer<Y> &src);
#endif
    inline void ref() const { d->weakref.ref(); d->strongref.ref(); }

    inline void internalSet(Data *o, T *actual)
    {
        if (o) {
            // increase the strongref, but never up from zero
            // or less (-1 is used by QWeakPointer on untracked QObject)
            int tmp = o->strongref.load();
            while (tmp > 0) {
                // try to increment from "tmp" to "tmp + 1"
                if (o->strongref.testAndSetRelaxed(tmp, tmp + 1))
                    break;   // succeeded
                tmp = o->strongref.load();  // failed, try again
            }

            if (tmp > 0) {
                o->weakref.ref();
            } else {
                o->checkQObjectShared(actual);
                o = 0;
            }
        }

        qSwap(d, o);
        qSwap(this->value, actual);
        if (!d || d->strongref.load() == 0)
            this->value = 0;

        // dereference saved data
        deref(o);
    }

    Type *value;
    Data *d;
};

template <class T>
class QWeakPointer
{
    typedef T *QWeakPointer:: *RestrictedBool;
    typedef QtSharedPointer::ExternalRefCountData Data;

public:
    typedef T element_type;
    typedef T value_type;
    typedef value_type *pointer;
    typedef const value_type *const_pointer;
    typedef value_type &reference;
    typedef const value_type &const_reference;
    typedef qptrdiff difference_type;

    inline bool isNull() const { return d == 0 || d->strongref.load() == 0 || value == 0; }
    inline operator RestrictedBool() const { return isNull() ? 0 : &QWeakPointer::value; }
    inline bool operator !() const { return isNull(); }
    inline T *data() const { return d == 0 || d->strongref.load() == 0 ? 0 : value; }

    Q_DECL_CONSTEXPR inline QWeakPointer() : d(0), value(0) { }
    inline ~QWeakPointer() { if (d && !d->weakref.deref()) delete d; }

#ifndef QT_NO_QOBJECT
    // special constructor that is enabled only if X derives from QObject
#if QT_DEPRECATED_SINCE(5, 0)
    template <class X>
    QT_DEPRECATED inline QWeakPointer(X *ptr) : d(ptr ? Data::getAndRef(ptr) : 0), value(ptr)
    { }
#endif
#endif

#if QT_DEPRECATED_SINCE(5, 0)
    template <class X>
    QT_DEPRECATED inline QWeakPointer &operator=(X *ptr)
    { return *this = QWeakPointer(ptr); }
#endif

    inline QWeakPointer(const QWeakPointer &o) : d(o.d), value(o.value)
    { if (d) d->weakref.ref(); }
    inline QWeakPointer &operator=(const QWeakPointer &o)
    {
        internalSet(o.d, o.value);
        return *this;
    }

    inline void swap(QWeakPointer &other)
    {
        qSwap(this->d, other.d);
        qSwap(this->value, other.value);
    }

    inline QWeakPointer(const QSharedPointer<T> &o) : d(o.d), value(o.data())
    { if (d) d->weakref.ref();}
    inline QWeakPointer &operator=(const QSharedPointer<T> &o)
    {
        internalSet(o.d, o.value);
        return *this;
    }

    template <class X>
    inline QWeakPointer(const QWeakPointer<X> &o) : d(0), value(0)
    { *this = o; }

    template <class X>
    inline QWeakPointer &operator=(const QWeakPointer<X> &o)
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
    inline QWeakPointer &operator=(const QSharedPointer<X> &o)
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

    inline void clear() { *this = QWeakPointer(); }

    inline QSharedPointer<T> toStrongRef() const { return QSharedPointer<T>(*this); }
    // std::weak_ptr compatibility:
    inline QSharedPointer<T> lock() const { return toStrongRef(); }

#if defined(QWEAKPOINTER_ENABLE_ARROW)
    inline T *operator->() const { return data(); }
#endif

private:

#if defined(Q_NO_TEMPLATE_FRIENDS)
public:
#else
    template <class X> friend class QSharedPointer;
    template <class X> friend class QPointer;
#endif

    template <class X>
    inline QWeakPointer &assign(X *ptr)
    { return *this = QWeakPointer<X>(ptr, true); }

#ifndef QT_NO_QOBJECT
    template <class X>
    inline QWeakPointer(X *ptr, bool) : d(ptr ? Data::getAndRef(ptr) : 0), value(ptr)
    { }
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

template <class T>
class QEnableSharedFromThis
{
protected:
#ifdef Q_COMPILER_DEFAULT_MEMBERS
    QEnableSharedFromThis() = default;
#else
    Q_DECL_CONSTEXPR QEnableSharedFromThis() {}
#endif
    QEnableSharedFromThis(const QEnableSharedFromThis &) {}
    QEnableSharedFromThis &operator=(const QEnableSharedFromThis &) { return *this; }

public:
    inline QSharedPointer<T> sharedFromThis() { return QSharedPointer<T>(weakPointer); }
    inline QSharedPointer<const T> sharedFromThis() const { return QSharedPointer<const T>(weakPointer); }

#ifndef Q_NO_TEMPLATE_FRIENDS
private:
    template <class X> friend class QSharedPointer;
#else
public:
#endif
    template <class X>
    inline void initializeFromSharedPointer(const QSharedPointer<X> &ptr) const
    {
        weakPointer = ptr;
    }

    mutable QWeakPointer<T> weakPointer;
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
template <class T>
Q_INLINE_TEMPLATE uint qHash(const QSharedPointer<T> &ptr, uint seed = 0)
{
    return QT_PREPEND_NAMESPACE(qHash)(ptr.data(), seed);
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

QT_END_NAMESPACE
namespace std {
    template <class T>
    inline void swap(QT_PREPEND_NAMESPACE(QSharedPointer)<T> &p1, QT_PREPEND_NAMESPACE(QSharedPointer)<T> &p2)
    { p1.swap(p2); }
}
QT_BEGIN_NAMESPACE

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
    X *ptr = static_cast<X *>(src.data()); // if you get an error in this line, the cast is invalid
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
    X *ptr = dynamic_cast<X *>(src.data()); // if you get an error in this line, the cast is invalid
    if (!ptr)
        return QSharedPointer<X>();
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
    X *ptr = const_cast<X *>(src.data()); // if you get an error in this line, the cast is invalid
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
    X *ptr = qobject_cast<X *>(src.data());
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

template<typename T>
QWeakPointer<typename QtPrivate::QEnableIf<QtPrivate::IsPointerToTypeDerivedFromQObject<T*>::Value, T>::Type>
qWeakPointerFromVariant(const QVariant &variant)
{
    return QWeakPointer<T>(qobject_cast<T*>(QtSharedPointer::weakPointerFromVariant_internal(variant).data()));
}
template<typename T>
QSharedPointer<typename QtPrivate::QEnableIf<QtPrivate::IsPointerToTypeDerivedFromQObject<T*>::Value, T>::Type>
qSharedPointerFromVariant(const QVariant &variant)
{
    return qSharedPointerObjectCast<T>(QtSharedPointer::sharedPointerFromVariant_internal(variant));
}

#endif

template<typename T> Q_DECLARE_TYPEINFO_BODY(QWeakPointer<T>, Q_MOVABLE_TYPE);
template<typename T> Q_DECLARE_TYPEINFO_BODY(QSharedPointer<T>, Q_MOVABLE_TYPE);


QT_END_NAMESPACE

#endif
