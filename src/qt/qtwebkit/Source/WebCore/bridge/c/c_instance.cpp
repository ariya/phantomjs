/*
 * Copyright (C) 2003, 2006 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "c_instance.h"

#include "CRuntimeObject.h"
#include "IdentifierRep.h"
#include "JSDOMBinding.h"
#include "c_class.h"
#include "c_runtime.h"
#include "c_utility.h"
#include "npruntime_impl.h"
#include "runtime_method.h"
#include "runtime_root.h"
#include <interpreter/CallFrame.h>
#include <runtime/ArgList.h>
#include <runtime/Error.h>
#include <runtime/FunctionPrototype.h>
#include <runtime/JSLock.h>
#include <runtime/PropertyNameArray.h>
#include <wtf/Assertions.h>
#include <wtf/StdLibExtras.h>
#include <wtf/StringExtras.h>
#include <wtf/Vector.h>

using namespace WebCore;

namespace JSC {
namespace Bindings {

static String& globalExceptionString()
{
    DEFINE_STATIC_LOCAL(String, exceptionStr, ());
    return exceptionStr;
}

void CInstance::setGlobalException(String exception)
{
    globalExceptionString() = exception;
}

void CInstance::moveGlobalExceptionToExecState(ExecState* exec)
{
    if (globalExceptionString().isNull())
        return;

    {
        JSLockHolder lock(exec);
        throwError(exec, createError(exec, globalExceptionString()));
    }

    globalExceptionString() = String();
}

CInstance::CInstance(NPObject* o, PassRefPtr<RootObject> rootObject)
    : Instance(rootObject)
{
    _object = _NPN_RetainObject(o);
    _class = 0;
}

CInstance::~CInstance()
{
    _NPN_ReleaseObject(_object);
}

RuntimeObject* CInstance::newRuntimeObject(ExecState* exec)
{
    return CRuntimeObject::create(exec, exec->lexicalGlobalObject(), this);
}

Class *CInstance::getClass() const
{
    if (!_class)
        _class = CClass::classForIsA(_object->_class);
    return _class;
}

bool CInstance::supportsInvokeDefaultMethod() const
{
    return _object->_class->invokeDefault;
}

class CRuntimeMethod : public RuntimeMethod {
public:
    typedef RuntimeMethod Base;

    static CRuntimeMethod* create(ExecState* exec, JSGlobalObject* globalObject, const String& name, Bindings::Method* method)
    {
        // FIXME: deprecatedGetDOMStructure uses the prototype off of the wrong global object
        // We need to pass in the right global object for "i".
        Structure* domStructure = WebCore::deprecatedGetDOMStructure<CRuntimeMethod>(exec);
        CRuntimeMethod* runtimeMethod = new (NotNull, allocateCell<CRuntimeMethod>(*exec->heap())) CRuntimeMethod(globalObject, domStructure, method);
        runtimeMethod->finishCreation(exec->vm(), name);
        return runtimeMethod;
    }

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
    {
        return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), &s_info);
    }

    static const ClassInfo s_info;

private:
    CRuntimeMethod(JSGlobalObject* globalObject, Structure* structure, Bindings::Method* method)
        : RuntimeMethod(globalObject, structure, method)
    {
    }

    void finishCreation(VM& vm, const String& name)
    {
        Base::finishCreation(vm, name);
        ASSERT(inherits(&s_info));
    }

};

const ClassInfo CRuntimeMethod::s_info = { "CRuntimeMethod", &RuntimeMethod::s_info, 0, 0, CREATE_METHOD_TABLE(CRuntimeMethod) };

JSValue CInstance::getMethod(ExecState* exec, PropertyName propertyName)
{
    Method* method = getClass()->methodNamed(propertyName, this);
    return CRuntimeMethod::create(exec, exec->lexicalGlobalObject(), propertyName.publicName(), method);
}

JSValue CInstance::invokeMethod(ExecState* exec, RuntimeMethod* runtimeMethod)
{
    if (!asObject(runtimeMethod)->inherits(&CRuntimeMethod::s_info))
        return throwError(exec, createTypeError(exec, "Attempt to invoke non-plug-in method on plug-in object."));

    CMethod* method = static_cast<CMethod*>(runtimeMethod->method());
    ASSERT(method);

    NPIdentifier ident = method->identifier();
    if (!_object->_class->hasMethod(_object, ident))
        return jsUndefined();

    unsigned count = exec->argumentCount();
    Vector<NPVariant, 8> cArgs(count);

    unsigned i;
    for (i = 0; i < count; i++)
        convertValueToNPVariant(exec, exec->argument(i), &cArgs[i]);

    // Invoke the 'C' method.
    bool retval = true;
    NPVariant resultVariant;
    VOID_TO_NPVARIANT(resultVariant);

    {
        JSLock::DropAllLocks dropAllLocks(exec);
        ASSERT(globalExceptionString().isNull());
        retval = _object->_class->invoke(_object, ident, cArgs.data(), count, &resultVariant);
        moveGlobalExceptionToExecState(exec);
    }

    if (!retval)
        throwError(exec, createError(exec, ASCIILiteral("Error calling method on NPObject.")));

    for (i = 0; i < count; i++)
        _NPN_ReleaseVariantValue(&cArgs[i]);

    JSValue resultValue = convertNPVariantToValue(exec, &resultVariant, m_rootObject.get());
    _NPN_ReleaseVariantValue(&resultVariant);
    return resultValue;
}


JSValue CInstance::invokeDefaultMethod(ExecState* exec)
{
    if (!_object->_class->invokeDefault)
        return jsUndefined();

    unsigned count = exec->argumentCount();
    Vector<NPVariant, 8> cArgs(count);

    unsigned i;
    for (i = 0; i < count; i++)
        convertValueToNPVariant(exec, exec->argument(i), &cArgs[i]);

    // Invoke the 'C' method.
    bool retval = true;
    NPVariant resultVariant;
    VOID_TO_NPVARIANT(resultVariant);
    {
        JSLock::DropAllLocks dropAllLocks(exec);
        ASSERT(globalExceptionString().isNull());
        retval = _object->_class->invokeDefault(_object, cArgs.data(), count, &resultVariant);
        moveGlobalExceptionToExecState(exec);
    }

    if (!retval)
        throwError(exec, createError(exec, ASCIILiteral("Error calling method on NPObject.")));

    for (i = 0; i < count; i++)
        _NPN_ReleaseVariantValue(&cArgs[i]);

    JSValue resultValue = convertNPVariantToValue(exec, &resultVariant, m_rootObject.get());
    _NPN_ReleaseVariantValue(&resultVariant);
    return resultValue;
}

bool CInstance::supportsConstruct() const
{
    return _object->_class->construct;
}

JSValue CInstance::invokeConstruct(ExecState* exec, const ArgList& args)
{
    if (!_object->_class->construct)
        return jsUndefined();

    unsigned count = args.size();
    Vector<NPVariant, 8> cArgs(count);

    unsigned i;
    for (i = 0; i < count; i++)
        convertValueToNPVariant(exec, args.at(i), &cArgs[i]);

    // Invoke the 'C' method.
    bool retval = true;
    NPVariant resultVariant;
    VOID_TO_NPVARIANT(resultVariant);
    {
        JSLock::DropAllLocks dropAllLocks(exec);
        ASSERT(globalExceptionString().isNull());
        retval = _object->_class->construct(_object, cArgs.data(), count, &resultVariant);
        moveGlobalExceptionToExecState(exec);
    }

    if (!retval)
        throwError(exec, createError(exec, ASCIILiteral("Error calling method on NPObject.")));

    for (i = 0; i < count; i++)
        _NPN_ReleaseVariantValue(&cArgs[i]);

    JSValue resultValue = convertNPVariantToValue(exec, &resultVariant, m_rootObject.get());
    _NPN_ReleaseVariantValue(&resultVariant);
    return resultValue;
}

JSValue CInstance::defaultValue(ExecState* exec, PreferredPrimitiveType hint) const
{
    if (hint == PreferString)
        return stringValue(exec);
    if (hint == PreferNumber)
        return numberValue(exec);
    return valueOf(exec);
}

JSValue CInstance::stringValue(ExecState* exec) const
{
    JSValue value;
    if (toJSPrimitive(exec, "toString", value))
        return value;

    // Fallback to default implementation.
    return jsString(exec, "NPObject");
}

JSValue CInstance::numberValue(ExecState*) const
{
    // FIXME: Implement something sensible.
    return jsNumber(0);
}

JSValue CInstance::booleanValue() const
{
    // As per ECMA 9.2.
    return jsBoolean(getObject());
}

JSValue CInstance::valueOf(ExecState* exec) const
{
    JSValue value;
    if (toJSPrimitive(exec, "valueOf", value))
        return value;

    // Fallback to default implementation.
    return stringValue(exec);
}

bool CInstance::toJSPrimitive(ExecState* exec, const char* name, JSValue& resultValue) const
{
    NPIdentifier ident = _NPN_GetStringIdentifier(name);
    if (!_object->_class->hasMethod(_object, ident))
        return false;

    // Invoke the 'C' method.
    bool retval = true;
    NPVariant resultVariant;
    VOID_TO_NPVARIANT(resultVariant);

    {
        JSLock::DropAllLocks dropAllLocks(exec);
        ASSERT(globalExceptionString().isNull());
        retval = _object->_class->invoke(_object, ident, 0, 0, &resultVariant);
        moveGlobalExceptionToExecState(exec);
    }

    if (!retval)
        throwError(exec, createError(exec, ASCIILiteral("Error calling method on NPObject.")));

    resultValue = convertNPVariantToValue(exec, &resultVariant, m_rootObject.get());
    _NPN_ReleaseVariantValue(&resultVariant);
    return true;
}

void CInstance::getPropertyNames(ExecState* exec, PropertyNameArray& nameArray)
{
    if (!NP_CLASS_STRUCT_VERSION_HAS_ENUM(_object->_class) || !_object->_class->enumerate)
        return;

    uint32_t count;
    NPIdentifier* identifiers;

    {
        JSLock::DropAllLocks dropAllLocks(exec);
        ASSERT(globalExceptionString().isNull());
        bool ok = _object->_class->enumerate(_object, &identifiers, &count);
        moveGlobalExceptionToExecState(exec);
        if (!ok)
            return;
    }

    for (uint32_t i = 0; i < count; i++) {
        IdentifierRep* identifier = static_cast<IdentifierRep*>(identifiers[i]);

        if (identifier->isString())
            nameArray.add(identifierFromNPIdentifier(exec, identifier->string()));
        else
            nameArray.add(Identifier::from(exec, identifier->number()));
    }

    // FIXME: This should really call NPN_MemFree but that's in WebKit
    free(identifiers);
}

}
}

#endif // ENABLE(NETSCAPE_PLUGIN_API)
