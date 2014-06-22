/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#include "config.h"
#include "JSWrapper.h"

#include <JavaScriptCore/JSContextRefPrivate.h>

namespace WTR {

JSValueRef JSWrapper::wrap(JSContextRef context, JSWrappable* object)
{
    ASSERT_ARG(context, context);

    if (!object)
        return JSValueMakeNull(context);

    JSClassRef objectClass = object->wrapperClass();
    ASSERT(objectClass);
    JSObjectRef wrapperObject = JSObjectMake(context, objectClass, object);
    ASSERT(wrapperObject);

    return wrapperObject;
}

JSWrappable* JSWrapper::unwrap(JSContextRef context, JSValueRef value)
{
    ASSERT_ARG(context, context);
    ASSERT_ARG(value, value);
    if (!context || !value)
        return 0;
    return static_cast<JSWrappable*>(JSObjectGetPrivate(JSValueToObject(context, value, 0)));
}

static JSWrappable* unwrapObject(JSObjectRef object)
{
    JSWrappable* wrappable = static_cast<JSWrappable*>(JSObjectGetPrivate(object));
    ASSERT(wrappable);
    return wrappable;
}

void JSWrapper::initialize(JSContextRef ctx, JSObjectRef object)
{
    JSWrappable* wrappable = unwrapObject(object);
    if (!wrappable)
        return;
    wrappable->ref();
}

void JSWrapper::finalize(JSObjectRef object)
{
    JSWrappable* wrappable = unwrapObject(object);
    if (!wrappable)
        return;
    wrappable->deref();
}

} // namespace WTR
