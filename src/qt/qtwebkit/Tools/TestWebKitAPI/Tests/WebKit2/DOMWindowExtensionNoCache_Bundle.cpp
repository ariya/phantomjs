/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#include "InjectedBundleTest.h"
#include <WebKit2/WKBundleDOMWindowExtension.h>
#include <WebKit2/WKBundleFrame.h>
#include <WebKit2/WKBundlePage.h>
#include <WebKit2/WKBundlePageGroup.h>
#include <WebKit2/WKBundlePrivate.h>
#include <WebKit2/WKBundleScriptWorld.h>
#include <WebKit2/WKRetainPtr.h>
#include <wtf/HashMap.h>
#include <assert.h>

namespace TestWebKitAPI {

static void didFinishLoadForFrameCallback(WKBundlePageRef, WKBundleFrameRef, WKTypeRef*, const void* clientInfo);
static void globalObjectIsAvailableForFrameCallback(WKBundlePageRef, WKBundleFrameRef, WKBundleScriptWorldRef, const void* clientInfo);
static void willDisconnectDOMWindowExtensionFromGlobalObjectCallback(WKBundlePageRef, WKBundleDOMWindowExtensionRef, const void* clientInfo);
static void didReconnectDOMWindowExtensionToGlobalObjectCallback(WKBundlePageRef, WKBundleDOMWindowExtensionRef, const void* clientInfo);
static void willDestroyGlobalObjectForDOMWindowExtensionCallback(WKBundlePageRef, WKBundleDOMWindowExtensionRef, const void* clientInfo);

enum ExtensionState {
    Uncreated = 0, Connected, Disconnected, Destroyed, Removed
};

const char* states[5] = {
    "Uncreated",
    "Connected",
    "Disconnected",
    "Destroyed",
    "Removed"
};

typedef struct {
    const char* name;
    ExtensionState state;
} ExtensionRecord;

class DOMWindowExtensionNoCache : public InjectedBundleTest {
public:
    DOMWindowExtensionNoCache(const std::string& identifier);

    virtual void initialize(WKBundleRef, WKTypeRef userData);
    virtual void didCreatePage(WKBundleRef, WKBundlePageRef);
    virtual void willDestroyPage(WKBundleRef, WKBundlePageRef);

    void globalObjectIsAvailableForFrame(WKBundleFrameRef, WKBundleScriptWorldRef);
    void willDisconnectDOMWindowExtensionFromGlobalObject(WKBundleDOMWindowExtensionRef);
    void didReconnectDOMWindowExtensionToGlobalObject(WKBundleDOMWindowExtensionRef);
    void willDestroyGlobalObjectForDOMWindowExtension(WKBundleDOMWindowExtensionRef);

    void frameLoadFinished(WKBundleFrameRef);

private:
    void updateExtensionStateRecord(WKBundleDOMWindowExtensionRef, ExtensionState);
    void sendExtensionStateMessage();
    void sendBundleMessage(const char*);

    WKBundleRef m_bundle;
    ExtensionRecord m_extensionRecords[10];
    HashMap<WKBundleDOMWindowExtensionRef, int> m_extensionToRecordMap;
    int m_numberMainFrameLoads;
};

static InjectedBundleTest::Register<DOMWindowExtensionNoCache> registrar("DOMWindowExtensionNoCache");

DOMWindowExtensionNoCache::DOMWindowExtensionNoCache(const std::string& identifier)
    : InjectedBundleTest(identifier)
    , m_numberMainFrameLoads(0)
{
    m_extensionRecords[0].name = "First page, main frame, standard world";
    m_extensionRecords[1].name = "First page, main frame, non-standard world";
    m_extensionRecords[2].name = "First page, subframe, standard world";
    m_extensionRecords[3].name = "First page, subframe, non-standard world";
    m_extensionRecords[4].name = "Second page, main frame, standard world";
    m_extensionRecords[5].name = "Second page, main frame, non-standard world";
    m_extensionRecords[6].name = "First page, main frame, standard world";
    m_extensionRecords[7].name = "First page, main frame, non-standard world";
    m_extensionRecords[8].name = "First page, subframe, standard world";
    m_extensionRecords[9].name = "First page, subframe, non-standard world";

    for (size_t i = 0; i < 10; ++i)
      m_extensionRecords[i].state = Uncreated;
}

void DOMWindowExtensionNoCache::frameLoadFinished(WKBundleFrameRef frame)
{
    bool mainFrame = !WKBundleFrameGetParentFrame(frame);
    if (mainFrame)
        m_numberMainFrameLoads++;

    char body[16384];
    sprintf(body, "%s finished loading", mainFrame ? "Main frame" : "Subframe");

    // Only consider load finished for the main frame
    const char* name = mainFrame ? "DidFinishLoadForMainFrame" : "DidFinishLoadForFrame";

    WKRetainPtr<WKStringRef> messageName = adoptWK(WKStringCreateWithUTF8CString(name));
    WKRetainPtr<WKStringRef> messageBody = adoptWK(WKStringCreateWithUTF8CString(body));
    WKBundlePostMessage(m_bundle, messageName.get(), messageBody.get());

    sendExtensionStateMessage();
}

void DOMWindowExtensionNoCache::sendExtensionStateMessage()
{
    char body[16384];
    sprintf(body, "Extension states:\n%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s\n%s - %s",
            m_extensionRecords[0].name, states[m_extensionRecords[0].state],
            m_extensionRecords[1].name, states[m_extensionRecords[1].state],
            m_extensionRecords[2].name, states[m_extensionRecords[2].state],
            m_extensionRecords[3].name, states[m_extensionRecords[3].state],
            m_extensionRecords[4].name, states[m_extensionRecords[4].state],
            m_extensionRecords[5].name, states[m_extensionRecords[5].state],
            m_extensionRecords[6].name, states[m_extensionRecords[6].state],
            m_extensionRecords[7].name, states[m_extensionRecords[7].state],
            m_extensionRecords[8].name, states[m_extensionRecords[8].state],
            m_extensionRecords[9].name, states[m_extensionRecords[9].state]);

    WKRetainPtr<WKStringRef> messageName = adoptWK(WKStringCreateWithUTF8CString("ExtensionStates"));
    WKRetainPtr<WKStringRef> messageBody = adoptWK(WKStringCreateWithUTF8CString(body));
    WKBundlePostMessage(m_bundle, messageName.get(), messageBody.get());
}

void DOMWindowExtensionNoCache::initialize(WKBundleRef bundle, WKTypeRef userData)
{
    assert(WKGetTypeID(userData) == WKBundlePageGroupGetTypeID());
    WKBundlePageGroupRef pageGroup = static_cast<WKBundlePageGroupRef>(userData);

    WKRetainPtr<WKStringRef> source(AdoptWK, WKStringCreateWithUTF8CString("alert('Unimportant alert');"));
    WKBundleAddUserScript(bundle, pageGroup, WKBundleScriptWorldCreateWorld(), source.get(), 0, 0, 0, kWKInjectAtDocumentStart, kWKInjectInAllFrames);
}

void DOMWindowExtensionNoCache::didCreatePage(WKBundleRef bundle, WKBundlePageRef page)
{
    m_bundle = bundle;

    WKBundlePageLoaderClient pageLoaderClient;
    memset(&pageLoaderClient, 0, sizeof(pageLoaderClient));

    pageLoaderClient.version = kWKBundlePageLoaderClientCurrentVersion;
    pageLoaderClient.clientInfo = this;
    pageLoaderClient.didFinishLoadForFrame = didFinishLoadForFrameCallback;
    pageLoaderClient.globalObjectIsAvailableForFrame = globalObjectIsAvailableForFrameCallback;
    pageLoaderClient.willDisconnectDOMWindowExtensionFromGlobalObject = willDisconnectDOMWindowExtensionFromGlobalObjectCallback;
    pageLoaderClient.didReconnectDOMWindowExtensionToGlobalObject = didReconnectDOMWindowExtensionToGlobalObjectCallback;
    pageLoaderClient.willDestroyGlobalObjectForDOMWindowExtension = willDestroyGlobalObjectForDOMWindowExtensionCallback;

    WKBundlePageSetPageLoaderClient(page, &pageLoaderClient);
}

void DOMWindowExtensionNoCache::willDestroyPage(WKBundleRef, WKBundlePageRef)
{
    HashMap<WKBundleDOMWindowExtensionRef, int>::iterator it = m_extensionToRecordMap.begin();
    HashMap<WKBundleDOMWindowExtensionRef, int>::iterator end = m_extensionToRecordMap.end();
    for (; it != end; ++it) {
        updateExtensionStateRecord(it->key, Removed);
        WKRelease(it->key);
    }

    m_extensionToRecordMap.clear();

    sendExtensionStateMessage();
    sendBundleMessage("TestComplete");
}

void DOMWindowExtensionNoCache::updateExtensionStateRecord(WKBundleDOMWindowExtensionRef extension, ExtensionState state)
{
    int index = m_extensionToRecordMap.get(extension);
    m_extensionRecords[index].state = state;
}

void DOMWindowExtensionNoCache::sendBundleMessage(const char* message)
{
    WKRetainPtr<WKStringRef> wkMessage = adoptWK(WKStringCreateWithUTF8CString(message));
    WKBundlePostMessage(m_bundle, wkMessage.get(), wkMessage.get());
}

void DOMWindowExtensionNoCache::globalObjectIsAvailableForFrame(WKBundleFrameRef frame, WKBundleScriptWorldRef world)
{
    WKBundleDOMWindowExtensionRef extension = WKBundleDOMWindowExtensionCreate(frame, world);

    int index = 0;
    bool standard;
    standard = world == WKBundleScriptWorldNormalWorld();

    bool mainFrame = !WKBundleFrameGetParentFrame(frame);
    switch (m_numberMainFrameLoads) {
    case 0:
        index = mainFrame ? (standard ? 0 : 1) : (standard ? 2 : 3);
        break;
    case 1:
        index = standard ? 4 : 5;
        break;
    case 2:
        index = mainFrame ? (standard ? 6 : 7) : (standard ? 8 : 9);
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    m_extensionToRecordMap.set(extension, index);

    updateExtensionStateRecord(extension, Connected);
    sendBundleMessage("GlobalObjectIsAvailableForFrame called");
}

void DOMWindowExtensionNoCache::willDisconnectDOMWindowExtensionFromGlobalObject(WKBundleDOMWindowExtensionRef extension)
{
    // No items should be going into a 0-capacity page cache.
    ASSERT_NOT_REACHED();
}

void DOMWindowExtensionNoCache::didReconnectDOMWindowExtensionToGlobalObject(WKBundleDOMWindowExtensionRef)
{
    // No items should be coming out of a 0-capacity page cache.
    ASSERT_NOT_REACHED();
}

void DOMWindowExtensionNoCache::willDestroyGlobalObjectForDOMWindowExtension(WKBundleDOMWindowExtensionRef extension)
{
    sendBundleMessage("WillDestroyDOMWindowExtensionToGlobalObject called");
    updateExtensionStateRecord(extension, Destroyed);
    m_extensionToRecordMap.remove(extension);
    WKRelease(extension);
}

static void didFinishLoadForFrameCallback(WKBundlePageRef, WKBundleFrameRef frame, WKTypeRef*, const void *clientInfo)
{
    ((DOMWindowExtensionNoCache*)clientInfo)->frameLoadFinished(frame);
}

static void globalObjectIsAvailableForFrameCallback(WKBundlePageRef, WKBundleFrameRef frame, WKBundleScriptWorldRef world, const void* clientInfo)
{
    ((DOMWindowExtensionNoCache*)clientInfo)->globalObjectIsAvailableForFrame(frame, world);
}

static void willDisconnectDOMWindowExtensionFromGlobalObjectCallback(WKBundlePageRef, WKBundleDOMWindowExtensionRef extension, const void* clientInfo)
{
    ((DOMWindowExtensionNoCache*)clientInfo)->willDisconnectDOMWindowExtensionFromGlobalObject(extension);
}

static void didReconnectDOMWindowExtensionToGlobalObjectCallback(WKBundlePageRef, WKBundleDOMWindowExtensionRef extension, const void* clientInfo)
{
    ((DOMWindowExtensionNoCache*)clientInfo)->didReconnectDOMWindowExtensionToGlobalObject(extension);
}

static void willDestroyGlobalObjectForDOMWindowExtensionCallback(WKBundlePageRef, WKBundleDOMWindowExtensionRef extension , const void* clientInfo)
{
    ((DOMWindowExtensionNoCache*)clientInfo)->willDestroyGlobalObjectForDOMWindowExtension(extension);
}

} // namespace TestWebKitAPI
