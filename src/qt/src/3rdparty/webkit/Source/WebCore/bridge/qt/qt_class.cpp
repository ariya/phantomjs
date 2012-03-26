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
#include "qt_class.h"

#include "Identifier.h"
#include "qt_instance.h"
#include "qt_runtime.h"

#include <qdebug.h>
#include <qmetaobject.h>

namespace JSC {
namespace Bindings {

QtClass::QtClass(const QMetaObject* mo)
    : m_metaObject(mo)
{
}

QtClass::~QtClass()
{
}

typedef HashMap<const QMetaObject*, QtClass*> ClassesByMetaObject;
static ClassesByMetaObject* classesByMetaObject = 0;

QtClass* QtClass::classForObject(QObject* o)
{
    if (!classesByMetaObject)
        classesByMetaObject = new ClassesByMetaObject;

    const QMetaObject* mo = o->metaObject();
    QtClass* aClass = classesByMetaObject->get(mo);
    if (!aClass) {
        aClass = new QtClass(mo);
        classesByMetaObject->set(mo, aClass);
    }

    return aClass;
}

const char* QtClass::name() const
{
    return m_metaObject->className();
}

// We use this to get at signals (so we can return a proper function object,
// and not get wrapped in RuntimeMethod). Also, use this for methods,
// so we can cache the object and return the same object for the same
// identifier.
JSValue QtClass::fallbackObject(ExecState* exec, Instance* inst, const Identifier& identifier)
{
    QtInstance* qtinst = static_cast<QtInstance*>(inst);

    const UString& ustring = identifier.ustring();
    const QByteArray name = QString(reinterpret_cast<const QChar*>(ustring.characters()), ustring.length()).toAscii();

    // First see if we have a cache hit
    JSObject* val = qtinst->m_methods.value(name).get();
    if (val)
        return val;

    // Nope, create an entry
    const QByteArray normal = QMetaObject::normalizedSignature(name.constData());

    // See if there is an exact match
    int index = -1;
    if (normal.contains('(') && (index = m_metaObject->indexOfMethod(normal)) != -1) {
        QMetaMethod m = m_metaObject->method(index);
        if (m.access() != QMetaMethod::Private) {
            QtRuntimeMetaMethod* val = new (exec) QtRuntimeMetaMethod(exec, identifier, static_cast<QtInstance*>(inst), index, normal, false);
            qtinst->m_methods.insert(name, WriteBarrier<JSObject>(exec->globalData(), qtinst->createRuntimeObject(exec), val));
            return val;
        }
    }

    // Nope.. try a basename match
    const int count = m_metaObject->methodCount();
    for (index = count - 1; index >= 0; --index) {
        const QMetaMethod m = m_metaObject->method(index);
        if (m.access() == QMetaMethod::Private)
            continue;

        int iter = 0;
        const char* signature = m.signature();
        while (signature[iter] && signature[iter] != '(')
            ++iter;

        if (normal == QByteArray::fromRawData(signature, iter)) {
            QtRuntimeMetaMethod* val = new (exec) QtRuntimeMetaMethod(exec, identifier, static_cast<QtInstance*>(inst), index, normal, false);
            qtinst->m_methods.insert(name, WriteBarrier<JSObject>(exec->globalData(), qtinst->createRuntimeObject(exec), val));
            return val;
        }
    }

    return jsUndefined();
}

// This functionality is handled by the fallback case above...
MethodList QtClass::methodsNamed(const Identifier&, Instance*) const
{
    return MethodList();
}

// ### we may end up with a different search order than QtScript by not
// folding this code into the fallbackMethod above, but Fields propagate out
// of the binding code
Field* QtClass::fieldNamed(const Identifier& identifier, Instance* instance) const
{
    // Check static properties first
    QtInstance* qtinst = static_cast<QtInstance*>(instance);

    QObject* obj = qtinst->getObject();
    const UString& ustring = identifier.ustring();
    const QString name(reinterpret_cast<const QChar*>(ustring.characters()), ustring.length());
    const QByteArray ascii = name.toAscii();

    // First check for a cached field
    QtField* f = qtinst->m_fields.value(name);

    if (obj) {
        if (f) {
            // We only cache real metaproperties, but we do store the
            // other types so we can delete them later
            if (f->fieldType() == QtField::MetaProperty)
                return f;
#ifndef QT_NO_PROPERTIES
            if (f->fieldType() == QtField::DynamicProperty) {
                if (obj->dynamicPropertyNames().indexOf(ascii) >= 0)
                    return f;
                // Dynamic property that disappeared
                qtinst->m_fields.remove(name);
                delete f;
            }
#endif
            else {
                const QList<QObject*>& children = obj->children();
                const int count = children.size();
                for (int index = 0; index < count; ++index) {
                    QObject* child = children.at(index);
                    if (child->objectName() == name)
                        return f;
                }

                // Didn't find it, delete it from the cache
                qtinst->m_fields.remove(name);
                delete f;
            }
        }

        int index = m_metaObject->indexOfProperty(ascii);
        if (index >= 0) {
            const QMetaProperty prop = m_metaObject->property(index);

            if (prop.isScriptable(obj)) {
                f = new QtField(prop);
                qtinst->m_fields.insert(name, f);
                return f;
            }
        }

#ifndef QT_NO_PROPERTIES
        // Dynamic properties
        index = obj->dynamicPropertyNames().indexOf(ascii);
        if (index >= 0) {
            f = new QtField(ascii);
            qtinst->m_fields.insert(name, f);
            return f;
        }
#endif

        // Child objects

        const QList<QObject*>& children = obj->children();
        const int count = children.count();
        for (index = 0; index < count; ++index) {
            QObject* child = children.at(index);
            if (child->objectName() == name) {
                f = new QtField(child);
                qtinst->m_fields.insert(name, f);
                return f;
            }
        }

        // Nothing named this
        return 0;
    }
    // For compatibility with qtscript, cached methods don't cause
    // errors until they are accessed, so don't blindly create an error
    // here.
    if (qtinst->m_methods.contains(ascii))
        return 0;

#ifndef QT_NO_PROPERTIES
    // deleted qobject, but can't throw an error from here (no exec)
    // create a fake QtField that will throw upon access
    if (!f) {
        f = new QtField(ascii);
        qtinst->m_fields.insert(name, f);
    }
#endif
    return f;
}

}
}

