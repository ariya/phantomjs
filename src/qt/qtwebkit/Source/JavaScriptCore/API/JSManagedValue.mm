/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */


#import "config.h"
#import "JSManagedValue.h"

#if JSC_OBJC_API_ENABLED

#import "APICast.h"
#import "Heap.h"
#import "JSCJSValueInlines.h"
#import "JSContextInternal.h"
#import "JSValueInternal.h"
#import "Weak.h"
#import "WeakHandleOwner.h"
#import "ObjcRuntimeExtras.h"

class JSManagedValueHandleOwner : public JSC::WeakHandleOwner {
public:
    virtual void finalize(JSC::Handle<JSC::Unknown>, void* context);
    virtual bool isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown>, void* context, JSC::SlotVisitor&);
};

static JSManagedValueHandleOwner* managedValueHandleOwner()
{
    DEFINE_STATIC_LOCAL(JSManagedValueHandleOwner, jsManagedValueHandleOwner, ());
    return &jsManagedValueHandleOwner;
}

@implementation JSManagedValue {
    JSC::Weak<JSC::JSObject> m_value;
}

+ (JSManagedValue *)managedValueWithValue:(JSValue *)value
{
    return [[[self alloc] initWithValue:value] autorelease];
}

- (id)init
{
    return [self initWithValue:nil];
}

- (id)initWithValue:(JSValue *)value
{
    self = [super init];
    if (!self)
        return nil;
    
    if (!value || !JSValueIsObject([value.context JSGlobalContextRef], [value JSValueRef])) {
        JSC::Weak<JSC::JSObject> weak;
        m_value.swap(weak);
    } else {
        JSC::JSObject* object = toJS(const_cast<OpaqueJSValue*>([value JSValueRef]));
        JSC::Weak<JSC::JSObject> weak(object, managedValueHandleOwner(), self);
        m_value.swap(weak);
    }

    return self;
}

- (JSValue *)value
{
    if (!m_value)
        return nil;
    JSC::JSObject* object = m_value.get();
    JSContext *context = [JSContext contextWithJSGlobalContextRef:toGlobalRef(object->structure()->globalObject()->globalExec())];
    return [JSValue valueWithJSValueRef:toRef(object) inContext:context];
}

- (void)disconnectValue
{
    m_value.clear();
}

@end

@interface JSManagedValue (PrivateMethods)
- (void)disconnectValue;
@end

bool JSManagedValueHandleOwner::isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown>, void* context, JSC::SlotVisitor& visitor)
{
    JSManagedValue *managedValue = static_cast<JSManagedValue *>(context);
    return visitor.containsOpaqueRoot(managedValue);
}

void JSManagedValueHandleOwner::finalize(JSC::Handle<JSC::Unknown>, void* context)
{
    JSManagedValue *managedValue = static_cast<JSManagedValue *>(context);
    [managedValue disconnectValue];
}

#endif // JSC_OBJC_API_ENABLED
