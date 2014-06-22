/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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

#ifndef JSClassRef_h
#define JSClassRef_h

#include <JavaScriptCore/JSObjectRef.h>

#include "Weak.h"
#include "Protect.h"
#include <wtf/HashMap.h>
#include <wtf/text/WTFString.h>

struct StaticValueEntry {
    WTF_MAKE_FAST_ALLOCATED;
public:
    StaticValueEntry(JSObjectGetPropertyCallback _getProperty, JSObjectSetPropertyCallback _setProperty, JSPropertyAttributes _attributes)
        : getProperty(_getProperty), setProperty(_setProperty), attributes(_attributes)
    {
    }
    
    JSObjectGetPropertyCallback getProperty;
    JSObjectSetPropertyCallback setProperty;
    JSPropertyAttributes attributes;
};

struct StaticFunctionEntry {
    WTF_MAKE_FAST_ALLOCATED;
public:
    StaticFunctionEntry(JSObjectCallAsFunctionCallback _callAsFunction, JSPropertyAttributes _attributes)
        : callAsFunction(_callAsFunction), attributes(_attributes)
    {
    }

    JSObjectCallAsFunctionCallback callAsFunction;
    JSPropertyAttributes attributes;
};

typedef HashMap<RefPtr<StringImpl>, OwnPtr<StaticValueEntry> > OpaqueJSClassStaticValuesTable;
typedef HashMap<RefPtr<StringImpl>, OwnPtr<StaticFunctionEntry> > OpaqueJSClassStaticFunctionsTable;

struct OpaqueJSClass;

// An OpaqueJSClass (JSClass) is created without a context, so it can be used with any context, even across context groups.
// This structure holds data members that vary across context groups.
struct OpaqueJSClassContextData {
    WTF_MAKE_NONCOPYABLE(OpaqueJSClassContextData); WTF_MAKE_FAST_ALLOCATED;
public:
    OpaqueJSClassContextData(JSC::VM&, OpaqueJSClass*);

    // It is necessary to keep OpaqueJSClass alive because of the following rare scenario:
    // 1. A class is created and used, so its context data is stored in VM hash map.
    // 2. The class is released, and when all JS objects that use it are collected, OpaqueJSClass
    // is deleted (that's the part prevented by this RefPtr).
    // 3. Another class is created at the same address.
    // 4. When it is used, the old context data is found in VM and used.
    RefPtr<OpaqueJSClass> m_class;

    OwnPtr<OpaqueJSClassStaticValuesTable> staticValues;
    OwnPtr<OpaqueJSClassStaticFunctionsTable> staticFunctions;
    JSC::Weak<JSC::JSObject> cachedPrototype;
};

struct OpaqueJSClass : public ThreadSafeRefCounted<OpaqueJSClass> {
    static PassRefPtr<OpaqueJSClass> create(const JSClassDefinition*);
    static PassRefPtr<OpaqueJSClass> createNoAutomaticPrototype(const JSClassDefinition*);
    JS_EXPORT_PRIVATE ~OpaqueJSClass();
    
    String className();
    OpaqueJSClassStaticValuesTable* staticValues(JSC::ExecState*);
    OpaqueJSClassStaticFunctionsTable* staticFunctions(JSC::ExecState*);
    JSC::JSObject* prototype(JSC::ExecState*);

    OpaqueJSClass* parentClass;
    OpaqueJSClass* prototypeClass;
    
    JSObjectInitializeCallback initialize;
    JSObjectFinalizeCallback finalize;
    JSObjectHasPropertyCallback hasProperty;
    JSObjectGetPropertyCallback getProperty;
    JSObjectSetPropertyCallback setProperty;
    JSObjectDeletePropertyCallback deleteProperty;
    JSObjectGetPropertyNamesCallback getPropertyNames;
    JSObjectCallAsFunctionCallback callAsFunction;
    JSObjectCallAsConstructorCallback callAsConstructor;
    JSObjectHasInstanceCallback hasInstance;
    JSObjectConvertToTypeCallback convertToType;

private:
    friend struct OpaqueJSClassContextData;

    OpaqueJSClass();
    OpaqueJSClass(const OpaqueJSClass&);
    OpaqueJSClass(const JSClassDefinition*, OpaqueJSClass* protoClass);

    OpaqueJSClassContextData& contextData(JSC::ExecState*);

    // Strings in these data members should not be put into any IdentifierTable.
    String m_className;
    OwnPtr<OpaqueJSClassStaticValuesTable> m_staticValues;
    OwnPtr<OpaqueJSClassStaticFunctionsTable> m_staticFunctions;
};

#endif // JSClassRef_h
