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

#ifndef NPJSObjectWrapperMap_h
#define NPJSObjectWrapperMap_h

#if ENABLE(NETSCAPE_PLUGIN_API)

#include <WebCore/RunLoop.h>
#include <heap/Weak.h>
#include <heap/WeakInlines.h>
#include <wtf/Forward.h>
#include <wtf/HashMap.h>

struct NPObject;
typedef struct _NPVariant NPVariant;

namespace JSC {
    class ExecState;
    class VM;
    class JSGlobalObject;
    class JSObject;
    class JSValue;
}

namespace WebKit {

class JSNPObject;
class NPJSObject;
class PluginView;

// A per plug-in map of NPObjects that wrap JavaScript objects.
class NPRuntimeObjectMap : private JSC::WeakHandleOwner {
public:
    explicit NPRuntimeObjectMap(PluginView*);

    class PluginProtector {
    public:
        explicit PluginProtector(NPRuntimeObjectMap* npRuntimeObjectMap);
        ~PluginProtector();
        
    private:
        RefPtr<PluginView> m_pluginView;
    };

    // Returns an NPObject that wraps the given JSObject object. If there is already an NPObject that wraps this JSObject, it will
    // retain it and return it.
    NPObject* getOrCreateNPObject(JSC::VM&, JSC::JSObject*);
    void npJSObjectDestroyed(NPJSObject*);

    // Returns a JSObject object that wraps the given NPObject.
    JSC::JSObject* getOrCreateJSObject(JSC::JSGlobalObject*, NPObject*);
    void jsNPObjectDestroyed(JSNPObject*);

    void convertJSValueToNPVariant(JSC::ExecState*, JSC::JSValue, NPVariant&);
    JSC::JSValue convertNPVariantToJSValue(JSC::ExecState*, JSC::JSGlobalObject*, const NPVariant&);

    bool evaluate(NPObject*, const String& scriptString, NPVariant* result);

    // Called when the plug-in is destroyed. Will invalidate all the NPObjects.
    void invalidate();

    JSC::JSGlobalObject* globalObject() const;
    JSC::ExecState* globalExec() const;

    static void setGlobalException(const String& exceptionString);
    static void moveGlobalExceptionToExecState(JSC::ExecState*);

private:
    // WeakHandleOwner
    virtual void finalize(JSC::Handle<JSC::Unknown>, void* context);
    void addToInvalidationQueue(NPObject*);
    void invalidateQueuedObjects();

    PluginView* m_pluginView;
    HashMap<JSC::JSObject*, NPJSObject*> m_npJSObjects;
    HashMap<NPObject*, JSC::Weak<JSNPObject> > m_jsNPObjects;
    Vector<NPObject*> m_npObjectsToFinalize;
    WebCore::RunLoop::Timer<NPRuntimeObjectMap> m_finalizationTimer;
};

} // namespace WebKit

#endif // ENABLE(NETSCAPE_PLUGIN_API)

#endif // NPJSObjectWrapperMap_h
