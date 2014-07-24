/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov (ap@nypop.com)
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008 Eric Seidel <eric@webkit.org>
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

#import "config.h"
#import "ScriptController.h"

#import "BridgeJSC.h"
#import "DOMAbstractViewFrame.h"
#import "DOMWindow.h"
#import "Frame.h"
#import "FrameLoader.h"
#import "FrameLoaderClient.h"
#import "JSDOMWindow.h"
#import "WebScriptObjectPrivate.h"
#import "Widget.h"
#import "objc_instance.h"
#import "runtime_root.h"
#import <JavaScriptCore/APICast.h>
#import <JavaScriptCore/JSContextInternal.h>
#import <runtime/JSLock.h>

#if ENABLE(NETSCAPE_PLUGIN_API)
#import "c_instance.h"
#import "NP_jsobject.h"
#import "npruntime_impl.h"
#endif

@interface NSObject (WebPlugin)
- (id)objectForWebScript;
- (NPObject *)createPluginScriptableObject;
- (PassRefPtr<JSC::Bindings::Instance>)createPluginBindingsInstance:(PassRefPtr<JSC::Bindings::RootObject>)rootObject;
@end

using namespace JSC::Bindings;

namespace WebCore {

PassRefPtr<JSC::Bindings::Instance> ScriptController::createScriptInstanceForWidget(Widget* widget)
{
    NSView* widgetView = widget->platformWidget();
    if (!widgetView)
        return 0;

    RefPtr<RootObject> rootObject = createRootObject(widgetView);

    if ([widgetView respondsToSelector:@selector(createPluginBindingsInstance:)])
        return [widgetView createPluginBindingsInstance:rootObject.release()];
        
    if ([widgetView respondsToSelector:@selector(objectForWebScript)]) {
        id objectForWebScript = [widgetView objectForWebScript];
        if (!objectForWebScript)
            return 0;
        return JSC::Bindings::ObjcInstance::create(objectForWebScript, rootObject.release());
    }

    if ([widgetView respondsToSelector:@selector(createPluginScriptableObject)]) {
#if !ENABLE(NETSCAPE_PLUGIN_API)
        return 0;
#else
        NPObject* npObject = [widgetView createPluginScriptableObject];
        if (!npObject)
            return 0;
        RefPtr<Instance> instance = JSC::Bindings::CInstance::create(npObject, rootObject.release());
        // -createPluginScriptableObject returns a retained NPObject.  The caller is expected to release it.
        _NPN_ReleaseObject(npObject);
        return instance.release();
#endif
    }

    return 0;
}

WebScriptObject* ScriptController::windowScriptObject()
{
    if (!canExecuteScripts(NotAboutToExecuteScript))
        return 0;

    if (!m_windowScriptObject) {
        JSC::JSLockHolder lock(JSDOMWindowBase::commonVM());
        JSC::Bindings::RootObject* root = bindingRootObject();
        m_windowScriptObject = [WebScriptObject scriptObjectForJSObject:toRef(windowShell(pluginWorld())) originRootObject:root rootObject:root];
    }

    ASSERT([m_windowScriptObject.get() isKindOfClass:[DOMAbstractView class]]);
    return m_windowScriptObject.get();
}

JSContext *ScriptController::javaScriptContext()
{
#if JSC_OBJC_API_ENABLED
    if (!canExecuteScripts(NotAboutToExecuteScript))
        return 0;
    JSContext *context = [JSContext contextWithJSGlobalContextRef:toGlobalRef(bindingRootObject()->globalObject()->globalExec())];
    return context;
#else
    return 0;
#endif
}

void ScriptController::updatePlatformScriptObjects()
{
    if (m_windowScriptObject) {
        JSC::Bindings::RootObject* root = bindingRootObject();
        [m_windowScriptObject.get() _setOriginRootObject:root andRootObject:root];
    }
}

void ScriptController::disconnectPlatformScriptObjects()
{
    if (m_windowScriptObject) {
        ASSERT([m_windowScriptObject.get() isKindOfClass:[DOMAbstractView class]]);
        [(DOMAbstractView *)m_windowScriptObject.get() _disconnectFrame];
    }
}

}
