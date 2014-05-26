/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QOBJECTDEFS_H
#define QOBJECTDEFS_H

#include <QtCore/qnamespace.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

class QByteArray;

class QString;

#ifndef Q_MOC_OUTPUT_REVISION
#define Q_MOC_OUTPUT_REVISION 63
#endif

// The following macros are our "extensions" to C++
// They are used, strictly speaking, only by the moc.

#ifndef Q_MOC_RUN
# if defined(QT_NO_KEYWORDS)
#  define QT_NO_EMIT
# else
#   define slots
#   define signals protected
# endif
# define Q_SLOTS
# define Q_SIGNALS protected
# define Q_PRIVATE_SLOT(d, signature)
# define Q_EMIT
#ifndef QT_NO_EMIT
# define emit
#endif
#define Q_CLASSINFO(name, value)
#define Q_INTERFACES(x)
#define Q_PROPERTY(text)
#define Q_PRIVATE_PROPERTY(d, text)
#define Q_REVISION(v)
#define Q_OVERRIDE(text)
#define Q_ENUMS(x)
#define Q_FLAGS(x)
#ifdef QT3_SUPPORT
# define Q_SETS(x)
#endif
#define Q_SCRIPTABLE
#define Q_INVOKABLE
#define Q_SIGNAL
#define Q_SLOT

#ifndef QT_NO_TRANSLATION
# ifndef QT_NO_TEXTCODEC
// full set of tr functions
// ### Qt 5: merge overloads
#  define QT_TR_FUNCTIONS \
    static inline QString tr(const char *s, const char *c = 0) \
        { return staticMetaObject.tr(s, c); } \
    static inline QString trUtf8(const char *s, const char *c = 0) \
        { return staticMetaObject.trUtf8(s, c); } \
    static inline QString tr(const char *s, const char *c, int n) \
        { return staticMetaObject.tr(s, c, n); } \
    static inline QString trUtf8(const char *s, const char *c, int n) \
        { return staticMetaObject.trUtf8(s, c, n); }
# else
// no QTextCodec, no utf8
// ### Qt 5: merge overloads
#  define QT_TR_FUNCTIONS \
    static inline QString tr(const char *s, const char *c = 0) \
        { return staticMetaObject.tr(s, c); } \
    static inline QString tr(const char *s, const char *c, int n) \
        { return staticMetaObject.tr(s, c, n); }
# endif
#else
// inherit the ones from QObject
# define QT_TR_FUNCTIONS
#endif

#if defined(QT_NO_QOBJECT_CHECK)
/* tmake ignore Q_OBJECT */
#define Q_OBJECT_CHECK
#else

/* This is a compile time check that ensures that any class cast with qobject_cast
   actually contains a Q_OBJECT macro. Note: qobject_cast will fail if a QObject
   subclass doesn't contain Q_OBJECT.

   In qt_check_for_QOBJECT_macro, we call a dummy templated function with two
   parameters, the first being "this" and the other the target of the qobject
   cast. If the types are not identical, we know that a Q_OBJECT macro is missing.

   If you get a compiler error here, make sure that the class you are casting
   to contains a Q_OBJECT macro.
*/

/* tmake ignore Q_OBJECT */
#define Q_OBJECT_CHECK \
    template <typename T> inline void qt_check_for_QOBJECT_macro(const T &_q_argument) const \
    { int i = qYouForgotTheQ_OBJECT_Macro(this, &_q_argument); i = i + 1; }

template <typename T>
inline int qYouForgotTheQ_OBJECT_Macro(T, T) { return 0; }

template <typename T1, typename T2>
inline void qYouForgotTheQ_OBJECT_Macro(T1, T2) {}
#endif // QT_NO_QOBJECT_CHECK

#ifdef Q_NO_DATA_RELOCATION
#define Q_OBJECT_GETSTATICMETAOBJECT static const QMetaObject &getStaticMetaObject();
#else
#define Q_OBJECT_GETSTATICMETAOBJECT
#endif

/* tmake ignore Q_OBJECT */
#define Q_OBJECT \
public: \
    Q_OBJECT_CHECK \
    static const QMetaObject staticMetaObject; \
    Q_OBJECT_GETSTATICMETAOBJECT \
    virtual const QMetaObject *metaObject() const; \
    virtual void *qt_metacast(const char *); \
    QT_TR_FUNCTIONS \
    virtual int qt_metacall(QMetaObject::Call, int, void **); \
private: \
    Q_DECL_HIDDEN static const QMetaObjectExtraData staticMetaObjectExtraData; \
    Q_DECL_HIDDEN static void qt_static_metacall(QObject *, QMetaObject::Call, int, void **);

/* tmake ignore Q_OBJECT */
#define Q_OBJECT_FAKE Q_OBJECT
/* tmake ignore Q_GADGET */
#define Q_GADGET \
public: \
    static const QMetaObject staticMetaObject; \
    Q_OBJECT_GETSTATICMETAOBJECT \
private:
#else // Q_MOC_RUN
#define slots slots
#define signals signals
#define Q_SLOTS Q_SLOTS
#define Q_SIGNALS Q_SIGNALS
#define Q_CLASSINFO(name, value) Q_CLASSINFO(name, value)
#define Q_INTERFACES(x) Q_INTERFACES(x)
#define Q_PROPERTY(text) Q_PROPERTY(text)
#define Q_PRIVATE_PROPERTY(d, text) Q_PRIVATE_PROPERTY(d, text)
#define Q_REVISION(v) Q_REVISION(v)
#define Q_OVERRIDE(text) Q_OVERRIDE(text)
#define Q_ENUMS(x) Q_ENUMS(x)
#define Q_FLAGS(x) Q_FLAGS(x)
#ifdef QT3_SUPPORT
# define Q_SETS(x) Q_SETS(x)
#endif
 /* tmake ignore Q_OBJECT */
#define Q_OBJECT Q_OBJECT
 /* tmake ignore Q_OBJECT */
#define Q_OBJECT_FAKE Q_OBJECT_FAKE
 /* tmake ignore Q_GADGET */
#define Q_GADGET Q_GADGET
#define Q_SCRIPTABLE Q_SCRIPTABLE
#define Q_INVOKABLE Q_INVOKABLE
#define Q_SIGNAL Q_SIGNAL
#define Q_SLOT Q_SLOT
#endif //Q_MOC_RUN

// macro for onaming members
#ifdef METHOD
#undef METHOD
#endif
#ifdef SLOT
#undef SLOT
#endif
#ifdef SIGNAL
#undef SIGNAL
#endif

Q_CORE_EXPORT const char *qFlagLocation(const char *method);

#define QTOSTRING_HELPER(s) #s
#define QTOSTRING(s) QTOSTRING_HELPER(s)
#ifndef QT_NO_DEBUG
# define QLOCATION "\0" __FILE__ ":" QTOSTRING(__LINE__)
# ifndef QT_NO_KEYWORDS
#  define METHOD(a)   qFlagLocation("0"#a QLOCATION)
# endif
# define SLOT(a)     qFlagLocation("1"#a QLOCATION)
# define SIGNAL(a)   qFlagLocation("2"#a QLOCATION)
#else
# ifndef QT_NO_KEYWORDS
#  define METHOD(a)   "0"#a
# endif
# define SLOT(a)     "1"#a
# define SIGNAL(a)   "2"#a
#endif

#ifdef QT3_SUPPORT
#define METHOD_CODE   0                        // member type codes
#define SLOT_CODE     1
#define SIGNAL_CODE   2
#endif

#define QMETHOD_CODE  0                        // member type codes
#define QSLOT_CODE    1
#define QSIGNAL_CODE  2

#define Q_ARG(type, data) QArgument<type >(#type, data)
#define Q_RETURN_ARG(type, data) QReturnArgument<type >(#type, data)

class QObject;
class QMetaMethod;
class QMetaEnum;
class QMetaProperty;
class QMetaClassInfo;


class Q_CORE_EXPORT QGenericArgument
{
public:
    inline QGenericArgument(const char *aName = 0, const void *aData = 0)
        : _data(aData), _name(aName) {}
    inline void *data() const { return const_cast<void *>(_data); }
    inline const char *name() const { return _name; }

private:
    const void *_data;
    const char *_name;
};

class Q_CORE_EXPORT QGenericReturnArgument: public QGenericArgument
{
public:
    inline QGenericReturnArgument(const char *aName = 0, void *aData = 0)
        : QGenericArgument(aName, aData)
        {}
};

template <class T>
class QArgument: public QGenericArgument
{
public:
    inline QArgument(const char *aName, const T &aData)
        : QGenericArgument(aName, static_cast<const void *>(&aData))
        {}
};
template <class T>
class QArgument<T &>: public QGenericArgument
{
public:
    inline QArgument(const char *aName, T &aData)
        : QGenericArgument(aName, static_cast<const void *>(&aData))
        {}
};


template <typename T>
class QReturnArgument: public QGenericReturnArgument
{
public:
    inline QReturnArgument(const char *aName, T &aData)
        : QGenericReturnArgument(aName, static_cast<void *>(&aData))
        {}
};

struct Q_CORE_EXPORT QMetaObject
{
    const char *className() const;
    const QMetaObject *superClass() const;

    QObject *cast(QObject *obj) const;
    const QObject *cast(const QObject *obj) const;

#ifndef QT_NO_TRANSLATION
    // ### Qt 4: Merge overloads
    QString tr(const char *s, const char *c) const;
    QString trUtf8(const char *s, const char *c) const;
    QString tr(const char *s, const char *c, int n) const;
    QString trUtf8(const char *s, const char *c, int n) const;
#endif // QT_NO_TRANSLATION

    int methodOffset() const;
    int enumeratorOffset() const;
    int propertyOffset() const;
    int classInfoOffset() const;

    int constructorCount() const;
    int methodCount() const;
    int enumeratorCount() const;
    int propertyCount() const;
    int classInfoCount() const;

    int indexOfConstructor(const char *constructor) const;
    int indexOfMethod(const char *method) const;
    int indexOfSignal(const char *signal) const;
    int indexOfSlot(const char *slot) const;
    int indexOfEnumerator(const char *name) const;
    int indexOfProperty(const char *name) const;
    int indexOfClassInfo(const char *name) const;

    QMetaMethod constructor(int index) const;
    QMetaMethod method(int index) const;
    QMetaEnum enumerator(int index) const;
    QMetaProperty property(int index) const;
    QMetaClassInfo classInfo(int index) const;
    QMetaProperty userProperty() const;

    static bool checkConnectArgs(const char *signal, const char *method);
    static QByteArray normalizedSignature(const char *method);
    static QByteArray normalizedType(const char *type);

    // internal index-based connect
    static bool connect(const QObject *sender, int signal_index,
                        const QObject *receiver, int method_index,
                        int type = 0, int *types = 0);
    // internal index-based disconnect
    static bool disconnect(const QObject *sender, int signal_index,
                           const QObject *receiver, int method_index);
    static bool disconnectOne(const QObject *sender, int signal_index,
                              const QObject *receiver, int method_index);
    // internal slot-name based connect
    static void connectSlotsByName(QObject *o);

    // internal index-based signal activation
    static void activate(QObject *sender, int signal_index, void **argv);  //obsolete
    static void activate(QObject *sender, int from_signal_index, int to_signal_index, void **argv); //obsolete
    static void activate(QObject *sender, const QMetaObject *, int local_signal_index, void **argv);
    static void activate(QObject *sender, const QMetaObject *, int from_local_signal_index, int to_local_signal_index, void **argv); //obsolete

    // internal guarded pointers
    static void addGuard(QObject **ptr);
    static void removeGuard(QObject **ptr);
    static void changeGuard(QObject **ptr, QObject *o);

    static bool invokeMethod(QObject *obj, const char *member,
                             Qt::ConnectionType,
                             QGenericReturnArgument ret,
                             QGenericArgument val0 = QGenericArgument(0),
                             QGenericArgument val1 = QGenericArgument(),
                             QGenericArgument val2 = QGenericArgument(),
                             QGenericArgument val3 = QGenericArgument(),
                             QGenericArgument val4 = QGenericArgument(),
                             QGenericArgument val5 = QGenericArgument(),
                             QGenericArgument val6 = QGenericArgument(),
                             QGenericArgument val7 = QGenericArgument(),
                             QGenericArgument val8 = QGenericArgument(),
                             QGenericArgument val9 = QGenericArgument());

    static inline bool invokeMethod(QObject *obj, const char *member,
                             QGenericReturnArgument ret,
                             QGenericArgument val0 = QGenericArgument(0),
                             QGenericArgument val1 = QGenericArgument(),
                             QGenericArgument val2 = QGenericArgument(),
                             QGenericArgument val3 = QGenericArgument(),
                             QGenericArgument val4 = QGenericArgument(),
                             QGenericArgument val5 = QGenericArgument(),
                             QGenericArgument val6 = QGenericArgument(),
                             QGenericArgument val7 = QGenericArgument(),
                             QGenericArgument val8 = QGenericArgument(),
                             QGenericArgument val9 = QGenericArgument())
    {
        return invokeMethod(obj, member, Qt::AutoConnection, ret, val0, val1, val2, val3,
                val4, val5, val6, val7, val8, val9);
    }

    static inline bool invokeMethod(QObject *obj, const char *member,
                             Qt::ConnectionType type,
                             QGenericArgument val0 = QGenericArgument(0),
                             QGenericArgument val1 = QGenericArgument(),
                             QGenericArgument val2 = QGenericArgument(),
                             QGenericArgument val3 = QGenericArgument(),
                             QGenericArgument val4 = QGenericArgument(),
                             QGenericArgument val5 = QGenericArgument(),
                             QGenericArgument val6 = QGenericArgument(),
                             QGenericArgument val7 = QGenericArgument(),
                             QGenericArgument val8 = QGenericArgument(),
                             QGenericArgument val9 = QGenericArgument())
    {
        return invokeMethod(obj, member, type, QGenericReturnArgument(), val0, val1, val2,
                                 val3, val4, val5, val6, val7, val8, val9);
    }

    static inline bool invokeMethod(QObject *obj, const char *member,
                             QGenericArgument val0 = QGenericArgument(0),
                             QGenericArgument val1 = QGenericArgument(),
                             QGenericArgument val2 = QGenericArgument(),
                             QGenericArgument val3 = QGenericArgument(),
                             QGenericArgument val4 = QGenericArgument(),
                             QGenericArgument val5 = QGenericArgument(),
                             QGenericArgument val6 = QGenericArgument(),
                             QGenericArgument val7 = QGenericArgument(),
                             QGenericArgument val8 = QGenericArgument(),
                             QGenericArgument val9 = QGenericArgument())
    {
        return invokeMethod(obj, member, Qt::AutoConnection, QGenericReturnArgument(), val0,
                val1, val2, val3, val4, val5, val6, val7, val8, val9);
    }

    QObject *newInstance(QGenericArgument val0 = QGenericArgument(0),
                         QGenericArgument val1 = QGenericArgument(),
                         QGenericArgument val2 = QGenericArgument(),
                         QGenericArgument val3 = QGenericArgument(),
                         QGenericArgument val4 = QGenericArgument(),
                         QGenericArgument val5 = QGenericArgument(),
                         QGenericArgument val6 = QGenericArgument(),
                         QGenericArgument val7 = QGenericArgument(),
                         QGenericArgument val8 = QGenericArgument(),
                         QGenericArgument val9 = QGenericArgument()) const;

    enum Call {
        InvokeMetaMethod,
        ReadProperty,
        WriteProperty,
        ResetProperty,
        QueryPropertyDesignable,
        QueryPropertyScriptable,
        QueryPropertyStored,
        QueryPropertyEditable,
        QueryPropertyUser,
        CreateInstance
    };

    int static_metacall(Call, int, void **) const;
    static int metacall(QObject *, Call, int, void **);

#ifdef QT3_SUPPORT
    QT3_SUPPORT const char *superClassName() const;
#endif

    struct { // private data
        const QMetaObject *superdata;
        const char *stringdata;
        const uint *data;
        const void *extradata;
    } d;
};

typedef const QMetaObject& (*QMetaObjectAccessor)();

struct QMetaObjectExtraData
{
#ifdef Q_NO_DATA_RELOCATION
    const QMetaObjectAccessor *objects;
#else
    const QMetaObject **objects;
#endif

    typedef void (*StaticMetacallFunction)(QObject *, QMetaObject::Call, int, void **); //from revision 6
    //typedef int (*StaticMetaCall)(QMetaObject::Call, int, void **); //used from revison 2 until revison 5
    StaticMetacallFunction static_metacall;
};

inline const char *QMetaObject::className() const
{ return d.stringdata; }

inline const QMetaObject *QMetaObject::superClass() const
{ return d.superdata; }

#ifdef QT3_SUPPORT
inline const char *QMetaObject::superClassName() const
{ return d.superdata ? d.superdata->className() : 0; }
#endif

QT_END_NAMESPACE

QT_END_HEADER

#endif // QOBJECTDEFS_H
