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
#include "JSWeakObjectMapRefPrivate.h"

#include "APICast.h"
#include "APIShims.h"
#include "JSCJSValue.h"
#include "JSCallbackObject.h"
#include "JSWeakObjectMapRefInternal.h"
#include "Operations.h"
#include "Weak.h"
#include <wtf/HashMap.h>
#include <wtf/text/StringHash.h>

using namespace WTF;
using namespace JSC;

#ifdef __cplusplus
extern "C" {
#endif

JSWeakObjectMapRef JSWeakObjectMapCreate(JSContextRef context, void* privateData, JSWeakMapDestroyedCallback callback)
{
    ExecState* exec = toJS(context);
    APIEntryShim entryShim(exec);
    RefPtr<OpaqueJSWeakObjectMap> map = OpaqueJSWeakObjectMap::create(privateData, callback);
    exec->lexicalGlobalObject()->registerWeakMap(map.get());
    return map.get();
}

void JSWeakObjectMapSet(JSContextRef ctx, JSWeakObjectMapRef map, void* key, JSObjectRef object)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);
    JSObject* obj = toJS(object);
    if (!obj)
        return;
    ASSERT(obj->inherits(&JSCallbackObject<JSGlobalObject>::s_info) || obj->inherits(&JSCallbackObject<JSDestructibleObject>::s_info));
    map->map().set(key, obj);
}

JSObjectRef JSWeakObjectMapGet(JSContextRef ctx, JSWeakObjectMapRef map, void* key)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return 0;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);
    return toRef(jsCast<JSObject*>(map->map().get(key)));
}

void JSWeakObjectMapRemove(JSContextRef ctx, JSWeakObjectMapRef map, void* key)
{
    if (!ctx) {
        ASSERT_NOT_REACHED();
        return;
    }
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);
    map->map().remove(key);
}

// We need to keep this function in the build to keep the nightlies running.
JS_EXPORT bool JSWeakObjectMapClear(JSContextRef, JSWeakObjectMapRef, void*, JSObjectRef);
bool JSWeakObjectMapClear(JSContextRef, JSWeakObjectMapRef, void*, JSObjectRef)
{
    return true;
}

#ifdef __cplusplus
}
#endif
