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

#include "config.h"
#include "qt_instance.h"

#include "Error.h"
#include "JSDOMBinding.h"
#include "JSGlobalObject.h"
#include "JSLock.h"
#include "ObjectPrototype.h"
#include "PropertyNameArray.h"
#include "qt_class.h"
#include "qt_runtime.h"
#include "runtime_object.h"
#include "runtime/FunctionPrototype.h"

#include <qdebug.h>
#include <qhash.h>
#include <qmetaobject.h>
#include <qmetatype.h>
#include <qwebelement.h>

namespace JSC {
namespace Bindings {

// Cache QtInstances
typedef QMultiHash<void*, QtInstance*> QObjectInstanceMap;
static QObjectInstanceMap cachedInstances;

// Derived RuntimeObject
class QtRuntimeObject : public RuntimeObject {
public:
    QtRuntimeObject(ExecState*, JSGlobalObject*, PassRefPtr<Instance>);

    static const ClassInfo s_info;

    virtual void visitChildren(SlotVisitor& visitor)
    {
        RuntimeObject::visitChildren(visitor);
        QtInstance* instance = static_cast<QtInstance*>(getInternalInstance());
        if (instance)
            instance->visitAggregate(visitor);
    }

    static Structure* createStructure(JSGlobalData& globalData, JSValue prototype)
    {
        return Structure::create(globalData, prototype, TypeInfo(ObjectType,  StructureFlags), AnonymousSlotCount, &s_info);
    }

protected:
    static const unsigned StructureFlags = RuntimeObject::StructureFlags | OverridesVisitChildren;
};

const ClassInfo QtRuntimeObject::s_info = { "QtRuntimeObject", &RuntimeObject::s_info, 0, 0 };

QtRuntimeObject::QtRuntimeObject(ExecState* exec, JSGlobalObject* globalObject, PassRefPtr<Instance> instance)
    : RuntimeObject(exec, globalObject, WebCore::deprecatedGetDOMStructure<QtRuntimeObject>(exec), instance)
{
}

// QtInstance
QtInstance::QtInstance(QObject* o, PassRefPtr<RootObject> rootObject, QScriptEngine::ValueOwnership ownership)
    : Instance(rootObject)
    , m_class(0)
    , m_object(o)
    , m_hashkey(o)
    , m_ownership(ownership)
{
    // This is a good place to register Qt metatypes that are in the QtWebKit module, as this is class will initialize if we have a QObject bridge.
    qRegisterMetaType<QWebElement>();
}

QtInstance::~QtInstance()
{
    JSLock lock(SilenceAssertionsOnly);

    cachedInstances.remove(m_hashkey);

    // clean up (unprotect from gc) the JSValues we've created
    m_methods.clear();

    qDeleteAll(m_fields);
    m_fields.clear();

    if (m_object) {
        switch (m_ownership) {
        case QScriptEngine::QtOwnership:
            break;
        case QScriptEngine::AutoOwnership:
            if (m_object->parent())
                break;
            // fall through!
        case QScriptEngine::ScriptOwnership:
            delete m_object;
            break;
        }
    }
}

PassRefPtr<QtInstance> QtInstance::getQtInstance(QObject* o, PassRefPtr<RootObject> rootObject, QScriptEngine::ValueOwnership ownership)
{
    JSLock lock(SilenceAssertionsOnly);

    foreach (QtInstance* instance, cachedInstances.values(o))
        if (instance->rootObject() == rootObject) {
            // The garbage collector removes instances, but it may happen that the wrapped
            // QObject dies before the gc kicks in. To handle that case we have to do an additional
            // check if to see if the instance's wrapped object is still alive. If it isn't, then
            // we have to create a new wrapper.
            if (!instance->getObject())
                cachedInstances.remove(instance->hashKey());
            else
                return instance;
        }

    RefPtr<QtInstance> ret = QtInstance::create(o, rootObject, ownership);
    cachedInstances.insert(o, ret.get());

    return ret.release();
}

bool QtInstance::getOwnPropertySlot(JSObject* object, ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    return object->JSObject::getOwnPropertySlot(exec, propertyName, slot);
}

void QtInstance::put(JSObject* object, ExecState* exec, const Identifier& propertyName, JSValue value, PutPropertySlot& slot)
{
    object->JSObject::put(exec, propertyName, value, slot);
}

void QtInstance::removeCachedMethod(JSObject* method)
{
    if (m_defaultMethod.get() == method)
        m_defaultMethod.clear();

    for (QHash<QByteArray, WriteBarrier<JSObject> >::Iterator it = m_methods.begin(), end = m_methods.end(); it != end; ++it) {
        if (it.value().get() == method) {
            m_methods.erase(it);
            return;
        }
    }
}

QtInstance* QtInstance::getInstance(JSObject* object)
{
    if (!object)
        return 0;
    if (!object->inherits(&QtRuntimeObject::s_info))
        return 0;
    return static_cast<QtInstance*>(static_cast<RuntimeObject*>(object)->getInternalInstance());
}

Class* QtInstance::getClass() const
{
    if (!m_class) {
        if (!m_object)
            return 0;
        m_class = QtClass::classForObject(m_object);
    }
    return m_class;
}

RuntimeObject* QtInstance::newRuntimeObject(ExecState* exec)
{
    JSLock lock(SilenceAssertionsOnly);
    m_methods.clear();
    return new (exec) QtRuntimeObject(exec, exec->lexicalGlobalObject(), this);
}

void QtInstance::visitAggregate(SlotVisitor& visitor)
{
    if (m_defaultMethod)
        visitor.append(&m_defaultMethod);
    for (QHash<QByteArray, WriteBarrier<JSObject> >::Iterator it = m_methods.begin(), end = m_methods.end(); it != end; ++it)
        visitor.append(&it.value());
}

void QtInstance::begin()
{
    // Do nothing.
}

void QtInstance::end()
{
    // Do nothing.
}

void QtInstance::getPropertyNames(ExecState* exec, PropertyNameArray& array)
{
    // This is the enumerable properties, so put:
    // properties
    // dynamic properties
    // slots
    QObject* obj = getObject();
    if (obj) {
        const QMetaObject* meta = obj->metaObject();

        int i;
        for (i = 0; i < meta->propertyCount(); i++) {
            QMetaProperty prop = meta->property(i);
            if (prop.isScriptable())
                array.add(Identifier(exec, prop.name()));
        }

#ifndef QT_NO_PROPERTIES
        QList<QByteArray> dynProps = obj->dynamicPropertyNames();
        foreach (const QByteArray& ba, dynProps)
            array.add(Identifier(exec, ba.constData()));
#endif

        const int methodCount = meta->methodCount();
        for (i = 0; i < methodCount; i++) {
            QMetaMethod method = meta->method(i);
            if (method.access() != QMetaMethod::Private)
                array.add(Identifier(exec, method.signature()));
        }
    }
}

JSValue QtInstance::getMethod(ExecState* exec, const Identifier& propertyName)
{
    if (!getClass())
        return jsNull();
    MethodList methodList = m_class->methodsNamed(propertyName, this);
    return new (exec) RuntimeMethod(exec, exec->lexicalGlobalObject(), WebCore::deprecatedGetDOMStructure<RuntimeMethod>(exec), propertyName, methodList);
}

JSValue QtInstance::invokeMethod(ExecState*, RuntimeMethod*)
{
    // Implemented via fallbackMethod & QtRuntimeMetaMethod::callAsFunction
    return jsUndefined();
}

JSValue QtInstance::defaultValue(ExecState* exec, PreferredPrimitiveType hint) const
{
    if (hint == PreferString)
        return stringValue(exec);
    if (hint == PreferNumber)
        return numberValue(exec);
    return valueOf(exec);
}

JSValue QtInstance::stringValue(ExecState* exec) const
{
    QObject* obj = getObject();
    if (!obj)
        return jsNull();

    // Hmm.. see if there is a toString defined
    QByteArray buf;
    bool useDefault = true;
    getClass();
    if (m_class) {
        // Cheat and don't use the full name resolution
        int index = obj->metaObject()->indexOfMethod("toString()");
        if (index >= 0) {
            QMetaMethod m = obj->metaObject()->method(index);
            // Check to see how much we can call it
            if (m.access() != QMetaMethod::Private
                && m.methodType() != QMetaMethod::Signal
                && m.parameterTypes().isEmpty()) {
                const char* retsig = m.typeName();
                if (retsig && *retsig) {
                    QVariant ret(QMetaType::type(retsig), (void*)0);
                    void * qargs[1];
                    qargs[0] = ret.data();

                    if (QMetaObject::metacall(obj, QMetaObject::InvokeMetaMethod, index, qargs) < 0) {
                        if (ret.isValid() && ret.canConvert(QVariant::String)) {
                            buf = ret.toString().toLatin1().constData(); // ### Latin 1? Ascii?
                            useDefault = false;
                        }
                    }
                }
            }
        }
    }

    if (useDefault) {
        const QMetaObject* meta = obj ? obj->metaObject() : &QObject::staticMetaObject;
        QString name = obj ? obj->objectName() : QString::fromUtf8("unnamed");
        QString str = QString::fromUtf8("%0(name = \"%1\")")
                      .arg(QLatin1String(meta->className())).arg(name);

        buf = str.toLatin1();
    }
    return jsString(exec, buf.constData());
}

JSValue QtInstance::numberValue(ExecState*) const
{
    return jsNumber(0);
}

JSValue QtInstance::booleanValue() const
{
    // ECMA 9.2
    return jsBoolean(getObject());
}

JSValue QtInstance::valueOf(ExecState* exec) const
{
    return stringValue(exec);
}

// In qt_runtime.cpp
JSValue convertQVariantToValue(ExecState*, PassRefPtr<RootObject> root, const QVariant& variant);
QVariant convertValueToQVariant(ExecState*, JSValue, QMetaType::Type hint, int *distance);

QByteArray QtField::name() const
{
    if (m_type == MetaProperty)
        return m_property.name();
    if (m_type == ChildObject && m_childObject)
        return m_childObject->objectName().toLatin1();
#ifndef QT_NO_PROPERTIES
    if (m_type == DynamicProperty)
        return m_dynamicProperty;
#endif
    return QByteArray(); // deleted child object
}

JSValue QtField::valueFromInstance(ExecState* exec, const Instance* inst) const
{
    const QtInstance* instance = static_cast<const QtInstance*>(inst);
    QObject* obj = instance->getObject();

    if (obj) {
        QVariant val;
        if (m_type == MetaProperty) {
            if (m_property.isReadable())
                val = m_property.read(obj);
            else
                return jsUndefined();
        } else if (m_type == ChildObject)
            val = QVariant::fromValue((QObject*) m_childObject);
#ifndef QT_NO_PROPERTIES
        else if (m_type == DynamicProperty)
            val = obj->property(m_dynamicProperty);
#endif
        return convertQVariantToValue(exec, inst->rootObject(), val);
    }
    QString msg = QString(QLatin1String("cannot access member `%1' of deleted QObject")).arg(QLatin1String(name()));
    return throwError(exec, createError(exec, msg.toLatin1().constData()));
}

void QtField::setValueToInstance(ExecState* exec, const Instance* inst, JSValue aValue) const
{
    if (m_type == ChildObject) // QtScript doesn't allow setting to a named child
        return;

    const QtInstance* instance = static_cast<const QtInstance*>(inst);
    QObject* obj = instance->getObject();
    if (obj) {
        QMetaType::Type argtype = QMetaType::Void;
        if (m_type == MetaProperty)
            argtype = (QMetaType::Type) QMetaType::type(m_property.typeName());

        // dynamic properties just get any QVariant
        QVariant val = convertValueToQVariant(exec, aValue, argtype, 0);
        if (m_type == MetaProperty) {
            if (m_property.isWritable())
                m_property.write(obj, val);
        }
#ifndef QT_NO_PROPERTIES
        else if (m_type == DynamicProperty)
            obj->setProperty(m_dynamicProperty.constData(), val);
#endif
    } else {
        QString msg = QString(QLatin1String("cannot access member `%1' of deleted QObject")).arg(QLatin1String(name()));
        throwError(exec, createError(exec, msg.toLatin1().constData()));
    }
}


}
}
