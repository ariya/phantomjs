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

using JSC::UString;

static JSC::UString& globalExceptionString()
{
    DEFINE_STATIC_LOCAL(JSC::UString, exceptionStr, ());
    return exceptionStr;
}

void CInstance::setGlobalException(UString exception)
{
    globalExceptionString() = exception;
}

void CInstance::moveGlobalExceptionToExecState(ExecState* exec)
{
    if (globalExceptionString().isNull())
        return;

    {
        JSLock lock(SilenceAssertionsOnly);
        throwError(exec, createError(exec, globalExceptionString()));
    }

    globalExceptionString() = UString();
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
    return new (exec) CRuntimeObject(exec, exec->lexicalGlobalObject(), this);
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
    CRuntimeMethod(ExecState* exec, JSGlobalObject* globalObject, const Identifier& name, Bindings::MethodList& list)
        // FIXME: deprecatedGetDOMStructure uses the prototype off of the wrong global object
        // We need to pass in the right global object for "i".
        : RuntimeMethod(exec, globalObject, WebCore::deprecatedGetDOMStructure<CRuntimeMethod>(exec), name, list)
    {
        ASSERT(inherits(&s_info));
    }

    static Structure* createStructure(JSGlobalData& globalData, JSValue prototype)
    {
        return Structure::create(globalData, prototype, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount, &s_info);
    }

    static const ClassInfo s_info;
};

const ClassInfo CRuntimeMethod::s_info = { "CRuntimeMethod", &RuntimeMethod::s_info, 0, 0 };

JSValue CInstance::getMethod(ExecState* exec, const Identifier& propertyName)
{
    MethodList methodList = getClass()->methodsNamed(propertyName, this);
    return new (exec) CRuntimeMethod(exec, exec->lexicalGlobalObject(), propertyName, methodList);
}

JSValue CInstance::invokeMethod(ExecState* exec, RuntimeMethod* runtimeMethod)
{
    if (!asObject(runtimeMethod)->inherits(&CRuntimeMethod::s_info))
        return throwError(exec, createTypeError(exec, "Attempt to invoke non-plug-in method on plug-in object."));

    const MethodList& methodList = *runtimeMethod->methods();

    // Overloading methods are not allowed by NPObjects.  Should only be one
    // name match for a particular method.
    ASSERT(methodList.size() == 1);

    CMethod* method = static_cast<CMethod*>(methodList[0]);

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
        JSLock::DropAllLocks dropAllLocks(SilenceAssertionsOnly);
        ASSERT(globalExceptionString().isNull());
        retval = _object->_class->invoke(_object, ident, cArgs.data(), count, &resultVariant);
        moveGlobalExceptionToExecState(exec);
    }
    
    if (!retval)
        throwError(exec, createError(exec, "Error calling method on NPObject."));

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
        JSLock::DropAllLocks dropAllLocks(SilenceAssertionsOnly);
        ASSERT(globalExceptionString().isNull());
        retval = _object->_class->invokeDefault(_object, cArgs.data(), count, &resultVariant);
        moveGlobalExceptionToExecState(exec);
    }
    
    if (!retval)
        throwError(exec, createError(exec, "Error calling method on NPObject."));

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
        JSLock::DropAllLocks dropAllLocks(SilenceAssertionsOnly);
        ASSERT(globalExceptionString().isNull());
        retval = _object->_class->construct(_object, cArgs.data(), count, &resultVariant);
        moveGlobalExceptionToExecState(exec);
    }
    
    if (!retval)
        throwError(exec, createError(exec, "Error calling method on NPObject."));

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
    char buf[1024];
    snprintf(buf, sizeof(buf), "NPObject %p, NPClass %p", _object, _object->_class);
    return jsString(exec, buf);
}

JSValue CInstance::numberValue(ExecState*) const
{
    // FIXME: Implement something sensible.
    return jsNumber(0);
}

JSValue CInstance::booleanValue() const
{
    // FIXME: Implement something sensible.
    return jsBoolean(false);
}

JSValue CInstance::valueOf(ExecState* exec) const 
{
    return stringValue(exec);
}

void CInstance::getPropertyNames(ExecState* exec, PropertyNameArray& nameArray)
{
    if (!NP_CLASS_STRUCT_VERSION_HAS_ENUM(_object->_class) || !_object->_class->enumerate)
        return;

    uint32_t count;
    NPIdentifier* identifiers;

    {
        JSLock::DropAllLocks dropAllLocks(SilenceAssertionsOnly);
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
