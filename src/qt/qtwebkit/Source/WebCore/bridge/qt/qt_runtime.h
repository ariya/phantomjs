/*
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef BINDINGS_QT_RUNTIME_H_
#define BINDINGS_QT_RUNTIME_H_

#include "BridgeJSC.h"
#include "JSDOMBinding.h"
#include "JavaScript.h"
#include "Weak.h"
#include "WeakInlines.h"
#include "qt_instance.h"
#include "runtime_method.h"

#include <QPointer>

#include <qbytearray.h>
#include <qmetaobject.h>
#include <qvariant.h>

namespace WebCore {
class JSDOMGlobalObject;
}

namespace JSC {
namespace Bindings {

class QtField : public Field {
public:

    typedef enum {
        MetaProperty,
#ifndef QT_NO_PROPERTIES
        DynamicProperty,
#endif
        ChildObject
    } QtFieldType;

    QtField(const QMetaProperty &p)
        : m_type(MetaProperty), m_property(p)
        {}

#ifndef QT_NO_PROPERTIES
    QtField(const QByteArray &b)
        : m_type(DynamicProperty), m_dynamicProperty(b)
        {}
#endif

    QtField(QObject *child)
        : m_type(ChildObject), m_childObject(child)
        {}

    virtual JSValue valueFromInstance(ExecState*, const Instance*) const;
    virtual void setValueToInstance(ExecState*, const Instance*, JSValue) const;
    QByteArray name() const;
    QtFieldType fieldType() const {return m_type;}
private:
    QtFieldType m_type;
    QByteArray m_dynamicProperty;
    QMetaProperty m_property;
    QPointer<QObject> m_childObject;
};

class QtRuntimeMethod {
public:
    enum MethodFlags {
        MethodIsSignal = 1,
        AllowPrivate = 2
    };

    QtRuntimeMethod(JSContextRef, QObject*, const QByteArray& identifier, int signalIndex, int flags, QtInstance*);
    ~QtRuntimeMethod();

    static JSValueRef call(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);
    static JSValueRef connect(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);
    static JSValueRef disconnect(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);

    JSObjectRef jsObjectRef(JSContextRef, JSValueRef* exception);

    const QByteArray& name() { return m_identifier; }

private:
    static QtRuntimeMethod* toRuntimeMethod(JSContextRef, JSObjectRef);

    static JSValueRef connectOrDisconnect(JSContextRef ctx, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception, bool connect);
    QPointer<QObject> m_object;
    QByteArray m_identifier;
    int m_index;
    int m_flags;
    Weak<JSObject> m_jsObject;
    QtInstance* m_instance;
};

// A QtConnectionObject represents a connection created inside JS. It will connect its own execute() slot
// with the appropriate signal of 'sender'. When execute() is called, it will call JS 'receiverFunction'.
class QtConnectionObject : public QObject
{
    Q_OBJECT_FAKE
public:
    QtConnectionObject(JSContextRef, PassRefPtr<QtInstance> senderInstance, int signalIndex, JSObjectRef receiver, JSObjectRef receiverFunction);
    ~QtConnectionObject();

    void execute(void **argv);

    bool match(JSContextRef, QObject* sender, int signalIndex, JSObjectRef thisObject, JSObjectRef funcObject);

private:
    JSGlobalContextRef m_context;
    RefPtr<RootObject> m_rootObject;

    int m_signalIndex;
    JSObjectRef m_receiver;
    JSObjectRef m_receiverFunction;

    friend class QtRuntimeMethod;
    static QMultiMap<QObject*, QtConnectionObject*> connections;
};


typedef QVariant (*ConvertToVariantFunction)(JSObject* object, int *distance, HashSet<JSObjectRef>* visitedObjects);
typedef JSValue (*ConvertToJSValueFunction)(ExecState* exec, WebCore::JSDOMGlobalObject* globalObject, const QVariant& variant);

void registerCustomType(int qtMetaTypeId, ConvertToVariantFunction, ConvertToJSValueFunction);

QVariant convertValueToQVariant(JSContextRef, JSValueRef, QMetaType::Type hint, int *distance, JSValueRef* exception);
JSValueRef convertQVariantToValue(JSContextRef, PassRefPtr<RootObject>, const QVariant&, JSValueRef* exception);

void setException(JSContextRef, JSValueRef* exception, const QString& text);

} // namespace Bindings
} // namespace JSC

#endif
