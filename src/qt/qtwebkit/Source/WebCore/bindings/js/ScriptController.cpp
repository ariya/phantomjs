/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "ScriptController.h"

#include "BridgeJSC.h"
#include "ContentSecurityPolicy.h"
#include "Event.h"
#include "EventNames.h"
#include "Frame.h"
#include "FrameLoaderClient.h"
#include "GCController.h"
#include "HTMLPlugInElement.h"
#include "InspectorInstrumentation.h"
#include "JSDOMWindow.h"
#include "JSDocument.h"
#include "JSMainThreadExecState.h"
#include "NP_jsobject.h"
#include "Page.h"
#include "PageGroup.h"
#include "PluginView.h"
#include "ScriptCallStack.h"
#include "ScriptSourceCode.h"
#include "ScriptValue.h"
#include "ScriptableDocumentParser.h"
#include "Settings.h"
#include "StorageNamespace.h"
#include "UserGestureIndicator.h"
#include "WebCoreJSClientData.h"
#include "npruntime_impl.h"
#include "runtime_root.h"
#include <debugger/Debugger.h>
#include <heap/StrongInlines.h>
#include <runtime/InitializeThreading.h>
#include <runtime/JSLock.h>
#include <wtf/text/TextPosition.h>
#include <wtf/Threading.h>

using namespace JSC;
using namespace std;

namespace WebCore {

void ScriptController::initializeThreading()
{
    JSC::initializeThreading();
    WTF::initializeMainThread();
}

ScriptController::ScriptController(Frame* frame)
    : m_frame(frame)
    , m_sourceURL(0)
    , m_paused(false)
#if ENABLE(NETSCAPE_PLUGIN_API)
    , m_windowScriptNPObject(0)
#endif
#if PLATFORM(MAC)
    , m_windowScriptObject(0)
#endif
{
}

ScriptController::~ScriptController()
{
    disconnectPlatformScriptObjects();

    if (m_cacheableBindingRootObject) {
        JSLockHolder lock(JSDOMWindowBase::commonVM());
        m_cacheableBindingRootObject->invalidate();
        m_cacheableBindingRootObject = 0;
    }

    // It's likely that destroying m_windowShells will create a lot of garbage.
    if (!m_windowShells.isEmpty()) {
        while (!m_windowShells.isEmpty())
            destroyWindowShell(m_windowShells.begin()->key.get());
        gcController().garbageCollectSoon();
    }
}

void ScriptController::destroyWindowShell(DOMWrapperWorld* world)
{
    ASSERT(m_windowShells.contains(world));
    m_windowShells.remove(world);
    world->didDestroyWindowShell(this);
}

JSDOMWindowShell* ScriptController::createWindowShell(DOMWrapperWorld* world)
{
    ASSERT(!m_windowShells.contains(world));
    Structure* structure = JSDOMWindowShell::createStructure(*world->vm(), jsNull());
    Strong<JSDOMWindowShell> windowShell(*world->vm(), JSDOMWindowShell::create(m_frame->document()->domWindow(), structure, world));
    Strong<JSDOMWindowShell> windowShell2(windowShell);
    m_windowShells.add(world, windowShell);
    world->didCreateWindowShell(this);
    return windowShell.get();
}

ScriptValue ScriptController::evaluateInWorld(const ScriptSourceCode& sourceCode, DOMWrapperWorld* world)
{
    const SourceCode& jsSourceCode = sourceCode.jsSourceCode();
    String sourceURL = jsSourceCode.provider()->url();

    // evaluate code. Returns the JS return value or 0
    // if there was none, an error occurred or the type couldn't be converted.

    // inlineCode is true for <a href="javascript:doSomething()">
    // and false for <script>doSomething()</script>. Check if it has the
    // expected value in all cases.
    // See smart window.open policy for where this is used.
    JSDOMWindowShell* shell = windowShell(world);
    ExecState* exec = shell->window()->globalExec();
    const String* savedSourceURL = m_sourceURL;
    m_sourceURL = &sourceURL;

    JSLockHolder lock(exec);

    RefPtr<Frame> protect = m_frame;

    InspectorInstrumentationCookie cookie = InspectorInstrumentation::willEvaluateScript(m_frame, sourceURL, sourceCode.startLine());

    JSValue evaluationException;

    JSValue returnValue = JSMainThreadExecState::evaluate(exec, jsSourceCode, shell, &evaluationException);

    InspectorInstrumentation::didEvaluateScript(cookie);

    if (evaluationException) {
        reportException(exec, evaluationException, sourceCode.cachedScript());
        m_sourceURL = savedSourceURL;
        return ScriptValue();
    }

    m_sourceURL = savedSourceURL;
    return ScriptValue(exec->vm(), returnValue);
}

ScriptValue ScriptController::evaluate(const ScriptSourceCode& sourceCode) 
{
    return evaluateInWorld(sourceCode, mainThreadNormalWorld());
}

PassRefPtr<DOMWrapperWorld> ScriptController::createWorld()
{
    return DOMWrapperWorld::create(JSDOMWindow::commonVM());
}

void ScriptController::getAllWorlds(Vector<RefPtr<DOMWrapperWorld> >& worlds)
{
    static_cast<WebCoreJSClientData*>(JSDOMWindow::commonVM()->clientData)->getAllWorlds(worlds);
}

void ScriptController::clearWindowShell(DOMWindow* newDOMWindow, bool goingIntoPageCache)
{
    if (m_windowShells.isEmpty())
        return;

    JSLockHolder lock(JSDOMWindowBase::commonVM());

    for (ShellMap::iterator iter = m_windowShells.begin(); iter != m_windowShells.end(); ++iter) {
        JSDOMWindowShell* windowShell = iter->value.get();

        if (windowShell->window()->impl() == newDOMWindow)
            continue;

        // Clear the debugger from the current window before setting the new window.
        attachDebugger(windowShell, 0);

        windowShell->window()->willRemoveFromWindowShell();
        windowShell->setWindow(newDOMWindow);

        // An m_cacheableBindingRootObject persists between page navigations
        // so needs to know about the new JSDOMWindow.
        if (m_cacheableBindingRootObject)
            m_cacheableBindingRootObject->updateGlobalObject(windowShell->window());

        if (Page* page = m_frame->page()) {
            attachDebugger(windowShell, page->debugger());
            windowShell->window()->setProfileGroup(page->group().identifier());
        }
    }

    // It's likely that resetting our windows created a lot of garbage, unless
    // it went in a back/forward cache.
    if (!goingIntoPageCache)
        gcController().garbageCollectSoon();
}

JSDOMWindowShell* ScriptController::initScript(DOMWrapperWorld* world)
{
    ASSERT(!m_windowShells.contains(world));

    JSLockHolder lock(world->vm());

    JSDOMWindowShell* windowShell = createWindowShell(world);

    windowShell->window()->updateDocument();

    if (m_frame->document())
        windowShell->window()->setEvalEnabled(m_frame->document()->contentSecurityPolicy()->allowEval(0, ContentSecurityPolicy::SuppressReport), m_frame->document()->contentSecurityPolicy()->evalDisabledErrorMessage());   

    if (Page* page = m_frame->page()) {
        attachDebugger(windowShell, page->debugger());
        windowShell->window()->setProfileGroup(page->group().identifier());
    }

    m_frame->loader()->dispatchDidClearWindowObjectInWorld(world);

    return windowShell;
}

TextPosition ScriptController::eventHandlerPosition() const
{
    ScriptableDocumentParser* parser = m_frame->document()->scriptableDocumentParser();
    if (parser)
        return parser->textPosition();
    return TextPosition::minimumPosition();
}

void ScriptController::enableEval()
{
    JSDOMWindowShell* windowShell = existingWindowShell(mainThreadNormalWorld());
    if (!windowShell)
        return;
    windowShell->window()->setEvalEnabled(true);
}

void ScriptController::disableEval(const String& errorMessage)
{
    JSDOMWindowShell* windowShell = existingWindowShell(mainThreadNormalWorld());
    if (!windowShell)
        return;
    windowShell->window()->setEvalEnabled(false, errorMessage);
}

bool ScriptController::processingUserGesture()
{
    return UserGestureIndicator::processingUserGesture();
}

bool ScriptController::canAccessFromCurrentOrigin(Frame *frame)
{
    ExecState* exec = JSMainThreadExecState::currentState();
    if (exec)
        return shouldAllowAccessToFrame(exec, frame);
    // If the current state is 0 we're in a call path where the DOM security 
    // check doesn't apply (eg. parser).
    return true;
}

void ScriptController::attachDebugger(JSC::Debugger* debugger)
{
    for (ShellMap::iterator iter = m_windowShells.begin(); iter != m_windowShells.end(); ++iter)
        attachDebugger(iter->value.get(), debugger);
}

void ScriptController::attachDebugger(JSDOMWindowShell* shell, JSC::Debugger* debugger)
{
    if (!shell)
        return;

    JSDOMWindow* globalObject = shell->window();
    if (debugger)
        debugger->attach(globalObject);
    else if (JSC::Debugger* currentDebugger = globalObject->debugger())
        currentDebugger->detach(globalObject);
}

void ScriptController::updateDocument()
{
    for (ShellMap::iterator iter = m_windowShells.begin(); iter != m_windowShells.end(); ++iter) {
        JSLockHolder lock(iter->key->vm());
        iter->value->window()->updateDocument();
    }
}

Bindings::RootObject* ScriptController::cacheableBindingRootObject()
{
    if (!canExecuteScripts(NotAboutToExecuteScript))
        return 0;

    if (!m_cacheableBindingRootObject) {
        JSLockHolder lock(JSDOMWindowBase::commonVM());
        m_cacheableBindingRootObject = Bindings::RootObject::create(0, globalObject(pluginWorld()));
    }
    return m_cacheableBindingRootObject.get();
}

Bindings::RootObject* ScriptController::bindingRootObject()
{
    if (!canExecuteScripts(NotAboutToExecuteScript))
        return 0;

    if (!m_bindingRootObject) {
        JSLockHolder lock(JSDOMWindowBase::commonVM());
        m_bindingRootObject = Bindings::RootObject::create(0, globalObject(pluginWorld()));
    }
    return m_bindingRootObject.get();
}

PassRefPtr<Bindings::RootObject> ScriptController::createRootObject(void* nativeHandle)
{
    RootObjectMap::iterator it = m_rootObjects.find(nativeHandle);
    if (it != m_rootObjects.end())
        return it->value;

    RefPtr<Bindings::RootObject> rootObject = Bindings::RootObject::create(nativeHandle, globalObject(pluginWorld()));

    m_rootObjects.set(nativeHandle, rootObject);
    return rootObject.release();
}

#if ENABLE(INSPECTOR)
void ScriptController::setCaptureCallStackForUncaughtExceptions(bool)
{
}

void ScriptController::collectIsolatedContexts(Vector<std::pair<JSC::ExecState*, SecurityOrigin*> >& result)
{
    for (ShellMap::iterator iter = m_windowShells.begin(); iter != m_windowShells.end(); ++iter) {
        JSC::ExecState* exec = iter->value->window()->globalExec();
        SecurityOrigin* origin = iter->value->window()->impl()->document()->securityOrigin();
        result.append(std::pair<ScriptState*, SecurityOrigin*>(exec, origin));
    }
}

#endif

#if ENABLE(NETSCAPE_PLUGIN_API)

NPObject* ScriptController::windowScriptNPObject()
{
    if (!m_windowScriptNPObject) {
        if (canExecuteScripts(NotAboutToExecuteScript)) {
            // JavaScript is enabled, so there is a JavaScript window object.
            // Return an NPObject bound to the window object.
            JSDOMWindow* win = windowShell(pluginWorld())->window();
            ASSERT(win);
            JSC::JSLockHolder lock(win->globalExec());
            Bindings::RootObject* root = bindingRootObject();
            m_windowScriptNPObject = _NPN_CreateScriptObject(0, win, root);
        } else {
            // JavaScript is not enabled, so we cannot bind the NPObject to the JavaScript window object.
            // Instead, we create an NPObject of a different class, one which is not bound to a JavaScript object.
            m_windowScriptNPObject = _NPN_CreateNoScriptObject();
        }
    }

    return m_windowScriptNPObject;
}

NPObject* ScriptController::createScriptObjectForPluginElement(HTMLPlugInElement* plugin)
{
    JSObject* object = jsObjectForPluginElement(plugin);
    if (!object)
        return _NPN_CreateNoScriptObject();

    // Wrap the JSObject in an NPObject
    return _NPN_CreateScriptObject(0, object, bindingRootObject());
}

#endif

#if !PLATFORM(MAC) && !PLATFORM(QT)
PassRefPtr<JSC::Bindings::Instance> ScriptController::createScriptInstanceForWidget(Widget* widget)
{
    if (!widget->isPluginView())
        return 0;

    return toPluginView(widget)->bindingInstance();
}
#endif

JSObject* ScriptController::jsObjectForPluginElement(HTMLPlugInElement* plugin)
{
    // Can't create JSObjects when JavaScript is disabled
    if (!canExecuteScripts(NotAboutToExecuteScript))
        return 0;

    // Create a JSObject bound to this element
    JSDOMWindow* globalObj = globalObject(pluginWorld());
    JSLockHolder lock(globalObj->globalExec());
    // FIXME: is normal okay? - used for NP plugins?
    JSValue jsElementValue = toJS(globalObj->globalExec(), globalObj, plugin);
    if (!jsElementValue || !jsElementValue.isObject())
        return 0;
    
    return jsElementValue.getObject();
}

#if !PLATFORM(MAC)

void ScriptController::updatePlatformScriptObjects()
{
}

void ScriptController::disconnectPlatformScriptObjects()
{
}

#endif

void ScriptController::cleanupScriptObjectsForPlugin(void* nativeHandle)
{
    RootObjectMap::iterator it = m_rootObjects.find(nativeHandle);

    if (it == m_rootObjects.end())
        return;

    it->value->invalidate();
    m_rootObjects.remove(it);
}

void ScriptController::clearScriptObjects()
{
    JSLockHolder lock(JSDOMWindowBase::commonVM());

    RootObjectMap::const_iterator end = m_rootObjects.end();
    for (RootObjectMap::const_iterator it = m_rootObjects.begin(); it != end; ++it)
        it->value->invalidate();

    m_rootObjects.clear();

    if (m_bindingRootObject) {
        m_bindingRootObject->invalidate();
        m_bindingRootObject = 0;
    }

#if ENABLE(NETSCAPE_PLUGIN_API)
    if (m_windowScriptNPObject) {
        // Call _NPN_DeallocateObject() instead of _NPN_ReleaseObject() so that we don't leak if a plugin fails to release the window
        // script object properly.
        // This shouldn't cause any problems for plugins since they should have already been stopped and destroyed at this point.
        _NPN_DeallocateObject(m_windowScriptNPObject);
        m_windowScriptNPObject = 0;
    }
#endif
}

ScriptValue ScriptController::executeScriptInWorld(DOMWrapperWorld* world, const String& script, bool forceUserGesture)
{
    UserGestureIndicator gestureIndicator(forceUserGesture ? DefinitelyProcessingUserGesture : PossiblyProcessingUserGesture);
    ScriptSourceCode sourceCode(script, m_frame->document()->url());

    if (!canExecuteScripts(AboutToExecuteScript) || isPaused())
        return ScriptValue();

    return evaluateInWorld(sourceCode, world);
}

bool ScriptController::shouldBypassMainWorldContentSecurityPolicy()
{
    CallFrame* callFrame = JSDOMWindow::commonVM()->topCallFrame;
    if (!callFrame || callFrame == CallFrame::noCaller()) 
        return false;
    DOMWrapperWorld* domWrapperWorld = currentWorld(callFrame);
    if (domWrapperWorld->isNormal())
        return false;
    return true;
}

} // namespace WebCore
