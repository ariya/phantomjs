/*
 * Copyright (C) 2009, 2010 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#if USE(PLUGIN_HOST_PROCESS)

#ifndef ProxyInstance_h
#define ProxyInstance_h

#import "NetscapePluginInstanceProxy.h"
#import "WebKitPluginHostTypes.h"
#import <WebCore/BridgeJSC.h>
#import <WebCore/runtime_root.h>
#import <wtf/OwnPtr.h>

namespace WebKit {

class ProxyClass;
    
class ProxyInstance : public JSC::Bindings::Instance {
public:
    static PassRefPtr<ProxyInstance> create(PassRefPtr<JSC::Bindings::RootObject> rootObject, NetscapePluginInstanceProxy* instanceProxy, uint32_t objectID)
    {
        return adoptRef(new ProxyInstance(rootObject, instanceProxy, objectID));
    }
    ~ProxyInstance();

    JSC::Bindings::Method* methodNamed(JSC::PropertyName);
    JSC::Bindings::Field* fieldNamed(JSC::PropertyName);

    JSC::JSValue fieldValue(JSC::ExecState*, const JSC::Bindings::Field*) const;
    void setFieldValue(JSC::ExecState*, const JSC::Bindings::Field*, JSC::JSValue) const;
    

    NetscapePluginInstanceProxy* instanceProxy() { return m_instanceProxy; }

    void invalidate();
    
    uint32_t objectID() const { return m_objectID; }
    
private:
    ProxyInstance(PassRefPtr<JSC::Bindings::RootObject>, NetscapePluginInstanceProxy*, uint32_t objectID);

    virtual JSC::Bindings::RuntimeObject* newRuntimeObject(JSC::ExecState*);

    virtual JSC::Bindings::Class* getClass() const;

    virtual JSC::JSValue getMethod(JSC::ExecState*, JSC::PropertyName);
    virtual JSC::JSValue invokeMethod(JSC::ExecState*, JSC::RuntimeMethod*);

    virtual bool supportsInvokeDefaultMethod() const;
    virtual JSC::JSValue invokeDefaultMethod(JSC::ExecState*);

    virtual bool supportsConstruct() const;
    virtual JSC::JSValue invokeConstruct(JSC::ExecState*, const JSC::ArgList&);

    virtual JSC::JSValue defaultValue(JSC::ExecState*, JSC::PreferredPrimitiveType) const;
    virtual JSC::JSValue valueOf(JSC::ExecState*) const;
    
    virtual void getPropertyNames(JSC::ExecState*, JSC::PropertyNameArray&);

    JSC::JSValue stringValue(JSC::ExecState*) const;
    JSC::JSValue numberValue(JSC::ExecState*) const;
    JSC::JSValue booleanValue() const;
    
    JSC::JSValue invoke(JSC::ExecState*, InvokeType, uint64_t identifier, const JSC::ArgList&);
    
    template <typename T>
    std::auto_ptr<T> waitForReply(uint32_t requestID) const {
        std::auto_ptr<T> reply = m_instanceProxy->waitForReply<T>(requestID);
        
        // If the instance proxy was invalidated, just return a null reply.
        if (!m_instanceProxy)
            return std::auto_ptr<T>();
        
        return reply;
    }

    NetscapePluginInstanceProxy* m_instanceProxy;
    uint32_t m_objectID;
    JSC::Bindings::FieldMap m_fields;
    JSC::Bindings::MethodMap m_methods;
};
    
}

#endif // ProxyInstance_h
#endif // USE(PLUGIN_HOST_PROCESS)
