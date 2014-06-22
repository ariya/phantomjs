/*
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef DumpRenderTreeBlackBerry_h
#define DumpRenderTreeBlackBerry_h

#include "BlackBerryGlobal.h"


#include "DumpRenderTreeClient.h"
#include "Timer.h"
#include <BlackBerryPlatformLayoutTest.h>
#include <FindOptions.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {
class Credential;
class DOMWrapperWorld;
class Frame;
class Range;
}

extern WebCore::Frame* mainFrame;
extern WebCore::Frame* topLoadingFrame;
extern bool waitForPolicy;

class AccessibilityController;
class GCController;

namespace BlackBerry {
namespace WebKit {
class WebPage;

class DumpRenderTree : public BlackBerry::WebKit::DumpRenderTreeClient, public BlackBerry::Platform::LayoutTestClient {
public:
    DumpRenderTree(WebPage*);
    virtual ~DumpRenderTree();

    static DumpRenderTree* currentInstance() { return s_currentInstance; }

    void dump();

    void setWaitToDumpWatchdog(double interval);

    WebPage* page() { return m_page; }

    bool loadFinished() const { return m_loadFinished; }

    // FrameLoaderClient delegates
    bool willSendRequestForFrame(WebCore::Frame*, WebCore::ResourceRequest&, const WebCore::ResourceResponse&);
    void didStartProvisionalLoadForFrame(WebCore::Frame*);
    void didCommitLoadForFrame(WebCore::Frame*);
    void didFailProvisionalLoadForFrame(WebCore::Frame*);
    void didFailLoadForFrame(WebCore::Frame*);
    void didFinishLoadForFrame(WebCore::Frame*);
    void didFinishDocumentLoadForFrame(WebCore::Frame*);
    void didClearWindowObjectInWorld(WebCore::DOMWrapperWorld*, JSGlobalContextRef, JSObjectRef windowObject);
    void didReceiveTitleForFrame(const String& title, WebCore::Frame*);
    void didDecidePolicyForNavigationAction(const WebCore::NavigationAction&, const WebCore::ResourceRequest&, WebCore::Frame*);
    void didDecidePolicyForResponse(const WebCore::ResourceResponse&);
    void didDispatchWillPerformClientRedirect();
    void didHandleOnloadEventsForFrame(WebCore::Frame*);
    void didReceiveResponseForFrame(WebCore::Frame*, const WebCore::ResourceResponse&);
    bool policyDelegateEnabled() const { return m_policyDelegateEnabled; }
    bool policyDelegateIsPermissive() const { return m_policyDelegateIsPermissive; }

    // ChromeClient delegates
    void addMessageToConsole(const String& message, unsigned lineNumber, const String& sourceID);
    void runJavaScriptAlert(const String& message);
    bool runJavaScriptConfirm(const String& message);
    String runJavaScriptPrompt(const String& message, const String& defaultValue);
    bool runBeforeUnloadConfirmPanel(const String& message);
    void setStatusText(const String&);
    void exceededDatabaseQuota(WebCore::SecurityOrigin*, const String& name);
    bool allowsOpeningWindow();
    void windowCreated(WebPage*);

    // EditorClient delegates
    void setAcceptsEditing(bool acceptsEditing) { m_acceptsEditing = acceptsEditing; }

    void didBeginEditing();
    void didEndEditing();
    void didChange();
    void didChangeSelection();
    bool shouldBeginEditingInDOMRange(WebCore::Range*);
    bool shouldEndEditingInDOMRange(WebCore::Range*);
    bool shouldDeleteDOMRange(WebCore::Range*);
    bool shouldChangeSelectedDOMRangeToDOMRangeAffinityStillSelecting(WebCore::Range* fromRange, WebCore::Range* toRange, int affinity, bool stillSelecting);
    bool shouldInsertNode(WebCore::Node*, WebCore::Range*, int insertAction);
    bool shouldInsertText(const String&, WebCore::Range*, int insertAction);

    bool didReceiveAuthenticationChallenge(WebCore::Credential&);

    // BlackBerry::Platform::BlackBerryPlatformLayoutTestClient method
    virtual void addTest(const char* testFile);
    void setCustomPolicyDelegate(bool setDelegate, bool permissive);
private:
    void runTest(const String& url, const String& imageHash);
    void runTests();
    void runCurrentTest();

    void processWork(WebCore::Timer<DumpRenderTree>*);

private:
    static DumpRenderTree* s_currentInstance;

    String dumpFramesAsText(WebCore::Frame*);
    void locationChangeForFrame(WebCore::Frame*);

    void doneDrt();
    bool isHTTPTest(const String& test);
    String renderTreeDump() const;
    void resetToConsistentStateBeforeTesting(const String& url, const String& imageHash);
    void runRemainingTests();
    void invalidateAnyPreviousWaitToDumpWatchdog();
    void waitToDumpWatchdogTimerFired(WebCore::Timer<DumpRenderTree>*);

    Vector<String> m_tests;
    Vector<String>::iterator m_currentTest;
    Vector<String> m_bufferedTests;

    String m_resultsDir;
    String m_doneFile;
    String m_currentHttpTest;
    String m_currentTestFile;

    GCController* m_gcController;
    AccessibilityController* m_accessibilityController;
    WebPage* m_page;
    bool m_enablePixelTests;
    WebCore::Timer<DumpRenderTree> m_waitToDumpWatchdogTimer;
    WebCore::Timer<DumpRenderTree> m_workTimer;

    bool m_acceptsEditing;
    bool m_loadFinished;
    bool m_policyDelegateEnabled;
    bool m_policyDelegateIsPermissive;
};
}
}

#endif // DumpRenderTreeBlackBerry_h
