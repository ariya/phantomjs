/*
 * Copyright (C) 2008, 2009 Apple Inc. All Rights Reserved.
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

#include "config.h"
#include "JSDOMApplicationCache.h"

#if ENABLE(OFFLINE_WEB_APPLICATIONS)

#include "DOMApplicationCache.h"
#include "DOMWindow.h"
#include "Event.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "JSDOMWindowCustom.h"
#include "JSEvent.h"
#include "JSEventListener.h"
#include <wtf/text/AtomicString.h>

using namespace JSC;

namespace WebCore {

#if ENABLE(APPLICATION_CACHE_DYNAMIC_ENTRIES)

JSValue JSDOMApplicationCache::hasItem(ExecState* exec)
{
    Frame* frame = asJSDOMWindow(exec->dynamicGlobalObject())->impl()->frame();
    if (!frame)
        return jsUndefined();
    const KURL& url = frame->loader()->completeURL(exec->argument(0).toString(exec));

    ExceptionCode ec = 0;
    bool result = impl()->hasItem(url, ec);
    setDOMException(exec, ec);
    return jsBoolean(result);
}

JSValue JSDOMApplicationCache::add(ExecState* exec)
{
    Frame* frame = asJSDOMWindow(exec->dynamicGlobalObject())->impl()->frame();
    if (!frame)
        return jsUndefined();
    const KURL& url = frame->loader()->completeURL(exec->argument(0).toString(exec));
    
    ExceptionCode ec = 0;
    impl()->add(url, ec);
    setDOMException(exec, ec);
    return jsUndefined();
}

JSValue JSDOMApplicationCache::remove(ExecState* exec)
{
    Frame* frame = asJSDOMWindow(exec->dynamicGlobalObject())->impl()->frame();
    if (!frame)
        return jsUndefined();
    const KURL& url = frame->loader()->completeURL(exec->argument(0).toString(exec));
    
    ExceptionCode ec = 0;
    impl()->remove(url, ec);
    setDOMException(exec, ec);
    return jsUndefined();
}

#endif // ENABLE(APPLICATION_CACHE_DYNAMIC_ENTRIES)

} // namespace WebCore

#endif // ENABLE(OFFLINE_WEB_APPLICATIONS)
