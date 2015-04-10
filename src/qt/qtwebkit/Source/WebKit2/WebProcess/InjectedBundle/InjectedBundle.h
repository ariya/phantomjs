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

#ifndef InjectedBundle_h
#define InjectedBundle_h

#include "APIObject.h"
#include "InjectedBundleClient.h"
#include "SandboxExtension.h"
#include "WKBundle.h"
#include <WebCore/UserContentTypes.h>
#include <WebCore/UserScriptTypes.h>
#include <wtf/PassRefPtr.h>
#include <wtf/text/WTFString.h>

#if PLATFORM(QT)
#include <QLibrary>
#endif

#if PLATFORM(GTK)
typedef struct _GModule GModule;
#endif

#if PLATFORM(EFL)
#include <Eina.h>
#endif

#if PLATFORM(MAC)
OBJC_CLASS NSBundle;
#endif

namespace CoreIPC {
    class ArgumentDecoder;
    class Connection;
}

namespace WebKit {

#if PLATFORM(MAC)
typedef NSBundle *PlatformBundle;
#elif PLATFORM(QT)
typedef QLibrary PlatformBundle;
#elif PLATFORM(GTK)
typedef ::GModule* PlatformBundle;
#elif PLATFORM(EFL)
typedef Eina_Module* PlatformBundle;
#endif

class ImmutableArray;
class InjectedBundleScriptWorld;
class WebCertificateInfo;
class WebConnection;
class WebData;
class WebFrame;
class WebPage;
class WebPageGroupProxy;

class InjectedBundle : public TypedAPIObject<APIObject::TypeBundle> {
public:
    static PassRefPtr<InjectedBundle> create(const String& path)
    {
        return adoptRef(new InjectedBundle(path));
    }
    ~InjectedBundle();

    bool load(APIObject* initializationUserData);
    void setSandboxExtension(PassRefPtr<SandboxExtension> sandboxExtension) { m_sandboxExtension = sandboxExtension; }

    // API
    void initializeClient(WKBundleClient*);
    void postMessage(const String&, APIObject*);
    void postSynchronousMessage(const String&, APIObject*, RefPtr<APIObject>& returnData);

    WebConnection* webConnectionToUIProcess() const;

    // TestRunner only SPI
    void setShouldTrackVisitedLinks(bool);
    void setAlwaysAcceptCookies(bool);
    void removeAllVisitedLinks();
    void setCacheModel(uint32_t);
    void activateMacFontAscentHack();
    void overrideBoolPreferenceForTestRunner(WebPageGroupProxy*, const String& preference, bool enabled);
    void overrideXSSAuditorEnabledForTestRunner(WebPageGroupProxy* pageGroup, bool enabled);
    void setAllowUniversalAccessFromFileURLs(WebPageGroupProxy*, bool);
    void setAllowFileAccessFromFileURLs(WebPageGroupProxy*, bool);
    void setMinimumLogicalFontSize(WebPageGroupProxy*, int size);
    void setFrameFlatteningEnabled(WebPageGroupProxy*, bool);
    void setPluginsEnabled(WebPageGroupProxy*, bool);
    void setJavaScriptCanAccessClipboard(WebPageGroupProxy*, bool);
    void setPrivateBrowsingEnabled(WebPageGroupProxy*, bool);
    void setPopupBlockingEnabled(WebPageGroupProxy*, bool);
    void switchNetworkLoaderToNewTestingSession();
    void setAuthorAndUserStylesEnabled(WebPageGroupProxy*, bool);
    void setSpatialNavigationEnabled(WebPageGroupProxy*, bool);
    void addOriginAccessWhitelistEntry(const String&, const String&, const String&, bool);
    void removeOriginAccessWhitelistEntry(const String&, const String&, const String&, bool);
    void resetOriginAccessWhitelists();
    void setAsynchronousSpellCheckingEnabled(WebPageGroupProxy*, bool);
    int numberOfPages(WebFrame*, double, double);
    int pageNumberForElementById(WebFrame*, const String&, double, double);
    String pageSizeAndMarginsInPixels(WebFrame*, int, int, int, int, int, int, int);
    bool isPageBoxVisible(WebFrame*, int);
    void setUserStyleSheetLocation(WebPageGroupProxy*, const String&);
    void setWebNotificationPermission(WebPage*, const String& originString, bool allowed);
    void removeAllWebNotificationPermissions(WebPage*);
    uint64_t webNotificationID(JSContextRef, JSValueRef);
    PassRefPtr<WebData> createWebDataFromUint8Array(JSContextRef, JSValueRef);

    // UserContent API
    void addUserScript(WebPageGroupProxy*, InjectedBundleScriptWorld*, const String& source, const String& url, ImmutableArray* whitelist, ImmutableArray* blacklist, WebCore::UserScriptInjectionTime, WebCore::UserContentInjectedFrames);
    void addUserStyleSheet(WebPageGroupProxy*, InjectedBundleScriptWorld*, const String& source, const String& url, ImmutableArray* whitelist, ImmutableArray* blacklist, WebCore::UserContentInjectedFrames);
    void removeUserScript(WebPageGroupProxy*, InjectedBundleScriptWorld*, const String& url);
    void removeUserStyleSheet(WebPageGroupProxy*, InjectedBundleScriptWorld*, const String& url);
    void removeUserScripts(WebPageGroupProxy*, InjectedBundleScriptWorld*);
    void removeUserStyleSheets(WebPageGroupProxy*, InjectedBundleScriptWorld*);
    void removeAllUserContent(WebPageGroupProxy*);

    // Local storage API
    void clearAllDatabases();
    void setDatabaseQuota(uint64_t);

    // Application Cache API
    void clearApplicationCache();
    void clearApplicationCacheForOrigin(const String& origin);
    void setAppCacheMaximumSize(uint64_t);
    uint64_t appCacheUsageForOrigin(const String& origin);
    void setApplicationCacheOriginQuota(const String& origin, uint64_t);
    void resetApplicationCacheOriginQuota(const String& origin);
    PassRefPtr<ImmutableArray> originsWithApplicationCache();

    // Garbage collection API
    void garbageCollectJavaScriptObjects();
    void garbageCollectJavaScriptObjectsOnAlternateThreadForDebugging(bool waitUntilDone);
    size_t javaScriptObjectsCount();

    // Callback hooks
    void didCreatePage(WebPage*);
    void willDestroyPage(WebPage*);
    void didInitializePageGroup(WebPageGroupProxy*);
    void didReceiveMessage(const String&, APIObject*);
    void didReceiveMessageToPage(WebPage*, const String&, APIObject*);

    static void reportException(JSContextRef, JSValueRef exception);

    static bool isProcessingUserGesture();

    void setTabKeyCyclesThroughElements(WebPage*, bool enabled);
    void setSerialLoadingEnabled(bool);
    void setShadowDOMEnabled(bool);
    void setCSSRegionsEnabled(bool);
    void setCSSCompositingEnabled(bool);
    void setSeamlessIFramesEnabled(bool);
    void dispatchPendingLoadRequests();

private:
    explicit InjectedBundle(const String&);

    String m_path;
    PlatformBundle m_platformBundle; // This is leaked right now, since we never unload the bundle/module.

    RefPtr<SandboxExtension> m_sandboxExtension;

    InjectedBundleClient m_client;
};

} // namespace WebKit

#endif // InjectedBundle_h
