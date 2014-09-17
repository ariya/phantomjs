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

#ifndef qt_instance_h
#define qt_instance_h

#include "BridgeJSC.h"
#include "runtime_root.h"
#include <QtScript/qscriptengine.h>
#include <qhash.h>
#include <qpointer.h>
#include <qset.h>

namespace JSC {

namespace Bindings {

class QtClass;
class QtField;
class QtRuntimeMetaMethod;

class QtInstance : public Instance {
public:
    ~QtInstance();

    virtual Class* getClass() const;
    virtual RuntimeObject* newRuntimeObject(ExecState*);

    virtual void begin();
    virtual void end();

    virtual JSValue valueOf(ExecState*) const;
    virtual JSValue defaultValue(ExecState*, PreferredPrimitiveType) const;

    void visitAggregate(SlotVisitor&);

    virtual JSValue getMethod(ExecState* exec, const Identifier& propertyName);
    virtual JSValue invokeMethod(ExecState*, RuntimeMethod*);

    virtual void getPropertyNames(ExecState*, PropertyNameArray&);

    JSValue stringValue(ExecState* exec) const;
    JSValue numberValue(ExecState* exec) const;
    JSValue booleanValue() const;

    QObject* getObject() const { return m_object; }
    QObject* hashKey() const { return m_hashkey; }

    static PassRefPtr<QtInstance> getQtInstance(QObject*, PassRefPtr<RootObject>, QScriptEngine::ValueOwnership ownership);

    virtual bool getOwnPropertySlot(JSObject*, ExecState*, const Identifier&, PropertySlot&);
    virtual void put(JSObject*, ExecState*, const Identifier&, JSValue, PutPropertySlot&);

    void removeCachedMethod(JSObject*);

    static QtInstance* getInstance(JSObject*);

private:
    static PassRefPtr<QtInstance> create(QObject *instance, PassRefPtr<RootObject> rootObject, QScriptEngine::ValueOwnership ownership)
    {
        return adoptRef(new QtInstance(instance, rootObject, ownership));
    }

    friend class QtClass;
    friend class QtField;
    QtInstance(QObject*, PassRefPtr<RootObject>, QScriptEngine::ValueOwnership ownership); // Factory produced only..
    mutable QtClass* m_class;
    QPointer<QObject> m_object;
    QObject* m_hashkey;
    mutable QHash<QByteArray, WriteBarrier<JSObject> > m_methods;
    mutable QHash<QString, QtField*> m_fields;
    mutable WriteBarrier<QtRuntimeMetaMethod> m_defaultMethod;
    QScriptEngine::ValueOwnership m_ownership;
};

} // namespace Bindings

} // namespace JSC

#endif
