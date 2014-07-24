/*
 * Copyright (C) 2007, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2009 Jan Michael Alonzo <jmalonzo@gmail.com>
 * Copyright (C) 2009,2011 Collabora Ltd.
 * Copyright (C) 2010 Joone Hur <joone@kldp.org>
 * Copyright (C) 2011 ProFUSION Embedded Systems
 * Copyright (C) 2011 Samsung Electronics
 * Copyright (C) 2012 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "TestRunner.h"

#include "DumpRenderTree.h"
#include "DumpRenderTreeChrome.h"
#include "JSStringUtils.h"
#include "NotImplemented.h"
#include "WebCoreSupport/DumpRenderTreeSupportEfl.h"
#include "WorkQueue.h"
#include "WorkQueueItem.h"
#include "ewk_private.h"
#include <EWebKit.h>
#include <Ecore_File.h>
#include <JavaScriptCore/JSRetainPtr.h>
#include <JavaScriptCore/JSStringRef.h>
#include <JavaScriptCore/OpaqueJSString.h>
#include <KURL.h>
#include <editing/FindOptions.h>
#include <stdio.h>
#include <wtf/text/WTFString.h>

// Same as Mac cache model enum in Source/WebKit/mac/WebView/WebPreferences.h.
enum {
    WebCacheModelDocumentViewer = 0,
    WebCacheModelDocumentBrowser = 1,
    WebCacheModelPrimaryWebBrowser = 2
};

TestRunner::~TestRunner()
{
}

void TestRunner::addDisallowedURL(JSStringRef)
{
    notImplemented();
}

void TestRunner::clearBackForwardList()
{
    Ewk_History* history = ewk_view_history_get(browser->mainView());
    if (!history)
        return;

    Ewk_History_Item* item = ewk_history_history_item_current_get(history);
    ewk_history_clear(history);
    ewk_history_history_item_add(history, item);
    ewk_history_history_item_set(history, item);
    ewk_history_item_free(item);
}

JSStringRef TestRunner::copyDecodedHostName(JSStringRef)
{
    notImplemented();
    return 0;
}

JSStringRef TestRunner::copyEncodedHostName(JSStringRef)
{
    notImplemented();
    return 0;
}

void TestRunner::dispatchPendingLoadRequests()
{
    // FIXME: Implement for testing fix for 6727495
    notImplemented();
}

void TestRunner::display()
{
    displayWebView();
}

void TestRunner::keepWebHistory()
{
    DumpRenderTreeSupportEfl::setShouldTrackVisitedLinks(true);
}

size_t TestRunner::webHistoryItemCount()
{
    const Ewk_History* history = ewk_view_history_get(browser->mainView());
    if (!history)
        return -1;

    return ewk_history_back_list_length(history) + ewk_history_forward_list_length(history);
}

void TestRunner::notifyDone()
{
    if (m_waitToDump && !topLoadingFrame && !WorkQueue::shared()->count())
        dump();
    m_waitToDump = false;
    waitForPolicy = false;
}

JSStringRef TestRunner::pathToLocalResource(JSContextRef context, JSStringRef url)
{
    String requestedUrl(url->characters(), url->length());
    String resourceRoot;
    String requestedRoot;

    if (requestedUrl.find("LayoutTests") != notFound) {
        // If the URL contains LayoutTests we need to remap that to
        // LOCAL_RESOURCE_ROOT which is the path of the LayoutTests directory
        // within the WebKit source tree.
        requestedRoot = "/tmp/LayoutTests";
        resourceRoot = getenv("LOCAL_RESOURCE_ROOT");
    } else if (requestedUrl.find("tmp") != notFound) {
        // If the URL is a child of /tmp we need to convert it to be a child
        // DUMPRENDERTREE_TEMP replace tmp with DUMPRENDERTREE_TEMP
        requestedRoot = "/tmp";
        resourceRoot = getenv("DUMPRENDERTREE_TEMP");
    }

    size_t indexOfRootStart = requestedUrl.reverseFind(requestedRoot);
    size_t indexOfSeparatorAfterRoot = indexOfRootStart + requestedRoot.length();
    String fullPathToUrl = "file://" + resourceRoot + requestedUrl.substring(indexOfSeparatorAfterRoot);

    return JSStringCreateWithUTF8CString(fullPathToUrl.utf8().data());
}

void TestRunner::queueLoad(JSStringRef url, JSStringRef target)
{
    WebCore::KURL baseURL(WebCore::KURL(), String::fromUTF8(ewk_frame_uri_get(browser->mainFrame())));
    WebCore::KURL absoluteURL(baseURL, url->string());

    JSRetainPtr<JSStringRef> jsAbsoluteURL(
        Adopt, JSStringCreateWithUTF8CString(absoluteURL.string().utf8().data()));

    WorkQueue::shared()->queue(new LoadItem(jsAbsoluteURL.get(), target));
}

void TestRunner::setAcceptsEditing(bool acceptsEditing)
{
    ewk_view_editable_set(browser->mainView(), acceptsEditing);
}

void TestRunner::setAlwaysAcceptCookies(bool alwaysAcceptCookies)
{
    ewk_cookies_policy_set(alwaysAcceptCookies ? EWK_COOKIE_JAR_ACCEPT_ALWAYS : EWK_COOKIE_JAR_ACCEPT_NEVER);
}

void TestRunner::setCustomPolicyDelegate(bool enabled, bool permissive)
{
    policyDelegateEnabled = enabled;
    policyDelegatePermissive = permissive;
}

void TestRunner::waitForPolicyDelegate()
{
    setCustomPolicyDelegate(true, false);
    waitForPolicy = true;
    setWaitToDump(true);
}

void TestRunner::setScrollbarPolicy(JSStringRef, JSStringRef)
{
    notImplemented();
}

void TestRunner::addOriginAccessWhitelistEntry(JSStringRef sourceOrigin, JSStringRef protocol, JSStringRef host, bool includeSubdomains)
{
    WebCore::KURL kurl;
    kurl.setProtocol(String(protocol->characters(), protocol->length()));
    kurl.setHost(String(host->characters(), host->length()));

    ewk_security_policy_whitelist_origin_add(sourceOrigin->string().utf8().data(), kurl.string().utf8().data(), includeSubdomains);
}

void TestRunner::removeOriginAccessWhitelistEntry(JSStringRef sourceOrigin, JSStringRef protocol, JSStringRef host, bool includeSubdomains)
{
    WebCore::KURL kurl;
    kurl.setProtocol(String(protocol->characters(), protocol->length()));
    kurl.setHost(String(host->characters(), host->length()));

    ewk_security_policy_whitelist_origin_del(sourceOrigin->string().utf8().data(), kurl.string().utf8().data(), includeSubdomains);
}

void TestRunner::setMainFrameIsFirstResponder(bool)
{
    notImplemented();
}

void TestRunner::setTabKeyCyclesThroughElements(bool)
{
    notImplemented();
}

void TestRunner::setUseDashboardCompatibilityMode(bool)
{
    notImplemented();
}

static CString gUserStyleSheet;
static bool gUserStyleSheetEnabled = true;

void TestRunner::setUserStyleSheetEnabled(bool flag)
{
    gUserStyleSheetEnabled = flag;
    ewk_view_setting_user_stylesheet_set(browser->mainView(), flag ? gUserStyleSheet.data() : 0);
}

void TestRunner::setUserStyleSheetLocation(JSStringRef path)
{
    gUserStyleSheet = path->string().utf8();

    if (gUserStyleSheetEnabled)
        setUserStyleSheetEnabled(true);
}

void TestRunner::setValueForUser(JSContextRef context, JSValueRef nodeObject, JSStringRef value)
{
    DumpRenderTreeSupportEfl::setValueForUser(context, nodeObject, value->string());
}

void TestRunner::setViewModeMediaFeature(JSStringRef mode)
{
#if ENABLE(VIEW_MODE_CSS_MEDIA)
    Evas_Object* view = browser->mainView();
    if (!view)
        return;

    if (equals(mode, "windowed"))
        ewk_view_mode_set(view, EWK_VIEW_MODE_WINDOWED);
    else if (equals(mode, "floating"))
        ewk_view_mode_set(view, EWK_VIEW_MODE_FLOATING);
    else if (equals(mode, "fullscreen"))
        ewk_view_mode_set(view, EWK_VIEW_MODE_FULLSCREEN);
    else if (equals(mode, "maximized"))
        ewk_view_mode_set(view, EWK_VIEW_MODE_MAXIMIZED);
    else if (equals(mode, "minimized"))
        ewk_view_mode_set(view, EWK_VIEW_MODE_MINIMIZED);
#else
    UNUSED_PARAM(mode);
#endif
}

void TestRunner::setWindowIsKey(bool)
{
    notImplemented();
}

static Eina_Bool waitToDumpWatchdogFired(void*)
{
    waitToDumpWatchdog = 0;
    gTestRunner->waitToDumpWatchdogTimerFired();
    return ECORE_CALLBACK_CANCEL;
}

void TestRunner::setWaitToDump(bool waitUntilDone)
{
    static const double timeoutSeconds = 30;

    m_waitToDump = waitUntilDone;
    if (m_waitToDump && shouldSetWaitToDumpWatchdog())
        waitToDumpWatchdog = ecore_timer_add(timeoutSeconds, waitToDumpWatchdogFired, 0);
}

int TestRunner::windowCount()
{
    return browser->extraViews().size() + 1; // + 1 for the main view.
}

void TestRunner::setPrivateBrowsingEnabled(bool flag)
{
    ewk_view_setting_private_browsing_set(browser->mainView(), flag);
}

void TestRunner::setJavaScriptCanAccessClipboard(bool flag)
{
    ewk_view_setting_scripts_can_access_clipboard_set(browser->mainView(), flag);
}

void TestRunner::setXSSAuditorEnabled(bool flag)
{
    ewk_view_setting_enable_xss_auditor_set(browser->mainView(), flag);
}

void TestRunner::setSpatialNavigationEnabled(bool flag)
{
    ewk_view_setting_spatial_navigation_set(browser->mainView(), flag);
}

void TestRunner::setAllowUniversalAccessFromFileURLs(bool flag)
{
    ewk_view_setting_allow_universal_access_from_file_urls_set(browser->mainView(), flag);
}
 
void TestRunner::setAllowFileAccessFromFileURLs(bool flag)
{
    ewk_view_setting_allow_file_access_from_file_urls_set(browser->mainView(), flag);
}
 
void TestRunner::setAuthorAndUserStylesEnabled(bool flag)
{
    DumpRenderTreeSupportEfl::setAuthorAndUserStylesEnabled(browser->mainView(), flag);
}

void TestRunner::setMockDeviceOrientation(bool, double, bool, double, bool, double)
{
    // FIXME: Implement for DeviceOrientation layout tests.
    // See https://bugs.webkit.org/show_bug.cgi?id=30335.
    notImplemented();
}

void TestRunner::setMockGeolocationPosition(double latitude, double longitude, double accuracy, bool canProvideAltitude, double altitude, bool canProvideAltitudeAccuracy, double altitudeAccuracy, bool canProvideHeading, double heading, bool canProvideSpeed, double speed)
{
    Evas_Object* view = browser->mainView();
    if (browser->extraViews().size() > 0)
        view = browser->extraViews().last();

    DumpRenderTreeSupportEfl::setMockGeolocationPosition(view, latitude, longitude, accuracy, canProvideAltitude, altitude, canProvideAltitudeAccuracy, altitudeAccuracy, canProvideHeading, heading, canProvideSpeed, speed);
}

void TestRunner::setMockGeolocationPositionUnavailableError(JSStringRef message)
{
    Evas_Object* view = browser->mainView();
    if (browser->extraViews().size() > 0)
        view = browser->extraViews().last();

    DumpRenderTreeSupportEfl::setMockGeolocationPositionUnavailableError(view, message->string().utf8().data());
}

void TestRunner::setGeolocationPermission(bool allow)
{
    setGeolocationPermissionCommon(allow);
    Evas_Object* view = browser->mainView();
    if (browser->extraViews().size() > 0)
        view = browser->extraViews().last();

    DumpRenderTreeSupportEfl::setMockGeolocationPermission(view, allow);
}

int TestRunner::numberOfPendingGeolocationPermissionRequests()
{
    Evas_Object* view = browser->mainView();
    if (browser->extraViews().size() > 0)
        view = browser->extraViews().last();

    return DumpRenderTreeSupportEfl::numberOfPendingGeolocationPermissionRequests(view);
}

void TestRunner::addMockSpeechInputResult(JSStringRef, double, JSStringRef)
{
    // FIXME: Implement for speech input layout tests.
    // See https://bugs.webkit.org/show_bug.cgi?id=39485.
    notImplemented();
}

void TestRunner::setMockSpeechInputDumpRect(bool)
{
    // FIXME: Implement for speech input layout tests.
    // See https://bugs.webkit.org/show_bug.cgi?id=39485.
    notImplemented();
}

void TestRunner::startSpeechInput(JSContextRef inputElement)
{
    // FIXME: Implement for speech input layout tests.
    // See https://bugs.webkit.org/show_bug.cgi?id=39485.
    notImplemented();
}

void TestRunner::setIconDatabaseEnabled(bool enabled)
{
    ewk_settings_icon_database_path_set(0);

    if (!enabled)
        return;

    String databasePath;
    const char* tempDir = getenv("TMPDIR");

    if (tempDir)
        databasePath = String::fromUTF8(tempDir);
    else if (tempDir = getenv("TEMP"))
        databasePath = String::fromUTF8(tempDir);
    else
        databasePath = String::fromUTF8("/tmp");

    databasePath.append("/DumpRenderTree/IconDatabase");

    if (ecore_file_mkpath(databasePath.utf8().data()))
        ewk_settings_icon_database_path_set(databasePath.utf8().data());
}

void TestRunner::setPopupBlockingEnabled(bool flag)
{
    ewk_view_setting_scripts_can_open_windows_set(browser->mainView(), !flag);
}

void TestRunner::setPluginsEnabled(bool flag)
{
    ewk_view_setting_enable_plugins_set(browser->mainView(), flag);
}

void TestRunner::execCommand(JSStringRef name, JSStringRef value)
{
    DumpRenderTreeSupportEfl::executeCoreCommandByName(browser->mainView(), name->string().utf8().data(), value->string().utf8().data());
}

bool TestRunner::findString(JSContextRef context, JSStringRef target, JSObjectRef optionsArray)
{
    JSRetainPtr<JSStringRef> lengthPropertyName(Adopt, JSStringCreateWithUTF8CString("length"));
    JSValueRef lengthValue = JSObjectGetProperty(context, optionsArray, lengthPropertyName.get(), 0);
    if (!JSValueIsNumber(context, lengthValue))
        return false;

    WebCore::FindOptions options = 0;

    const size_t length = static_cast<size_t>(JSValueToNumber(context, lengthValue, 0));
    for (size_t i = 0; i < length; ++i) {
        JSValueRef value = JSObjectGetPropertyAtIndex(context, optionsArray, i, 0);
        if (!JSValueIsString(context, value))
            continue;

        JSRetainPtr<JSStringRef> optionName(Adopt, JSValueToStringCopy(context, value, 0));

        if (equals(optionName, "CaseInsensitive"))
            options |= WebCore::CaseInsensitive;
        else if (equals(optionName, "AtWordStarts"))
            options |= WebCore::AtWordStarts;
        else if (equals(optionName, "TreatMedialCapitalAsWordStart"))
            options |= WebCore::TreatMedialCapitalAsWordStart;
        else if (equals(optionName, "Backwards"))
            options |= WebCore::Backwards;
        else if (equals(optionName, "WrapAround"))
            options |= WebCore::WrapAround;
        else if (equals(optionName, "StartInSelection"))
            options |= WebCore::StartInSelection;
    }

    return DumpRenderTreeSupportEfl::findString(browser->mainView(), target->string(), options);
}

bool TestRunner::isCommandEnabled(JSStringRef name)
{
    return DumpRenderTreeSupportEfl::isCommandEnabled(browser->mainView(), name->string().utf8().data());
}

void TestRunner::setCacheModel(int cacheModel)
{
    unsigned int cacheTotalCapacity;
    unsigned int cacheMinDeadCapacity;
    unsigned int cacheMaxDeadCapacity;
    double deadDecodedDataDeletionInterval;
    unsigned int pageCacheCapacity;

    // These constants are derived from the Mac cache model enum in Source/WebKit/mac/WebView/WebPreferences.h.
    switch (cacheModel) {
    case WebCacheModelDocumentViewer:
        pageCacheCapacity = 0;
        cacheTotalCapacity = 0;
        cacheMinDeadCapacity = 0;
        cacheMaxDeadCapacity = 0;
        deadDecodedDataDeletionInterval = 0;
        break;
    case WebCacheModelDocumentBrowser:
        pageCacheCapacity = 2;
        cacheTotalCapacity = 16 * 1024 * 1024;
        cacheMinDeadCapacity = cacheTotalCapacity / 8;
        cacheMaxDeadCapacity = cacheTotalCapacity / 4;
        deadDecodedDataDeletionInterval = 0;
        break;
    case WebCacheModelPrimaryWebBrowser:
        pageCacheCapacity = 3;
        cacheTotalCapacity = 32 * 1024 * 1024;
        cacheMinDeadCapacity = cacheTotalCapacity / 4;
        cacheMaxDeadCapacity = cacheTotalCapacity / 2;
        deadDecodedDataDeletionInterval = 60;
        break;
    default:
        fprintf(stderr, "trying to set an invalid value %d for the Cache model.", cacheModel);
        return;
    }

    ewk_settings_object_cache_capacity_set(cacheMinDeadCapacity, cacheMaxDeadCapacity, cacheTotalCapacity);
    DumpRenderTreeSupportEfl::setDeadDecodedDataDeletionInterval(deadDecodedDataDeletionInterval);
    ewk_settings_page_cache_capacity_set(pageCacheCapacity);
}

void TestRunner::setPersistentUserStyleSheetLocation(JSStringRef)
{
    notImplemented();
}

void TestRunner::clearPersistentUserStyleSheet()
{
    notImplemented();
}

void TestRunner::clearAllApplicationCaches()
{
    ewk_settings_application_cache_clear();
}

void TestRunner::setApplicationCacheOriginQuota(unsigned long long quota)
{
    Ewk_Security_Origin* origin = ewk_frame_security_origin_get(browser->mainFrame());
    ewk_security_origin_application_cache_quota_set(origin, quota);
    ewk_security_origin_free(origin);
}

void TestRunner::clearApplicationCacheForOrigin(OpaqueJSString* url)
{
    Ewk_Security_Origin* origin = ewk_security_origin_new_from_string(url->string().utf8().data());
    ewk_security_origin_application_cache_clear(origin);
    ewk_security_origin_free(origin);
}

long long TestRunner::localStorageDiskUsageForOrigin(JSStringRef)
{
    // FIXME: Implement to support getting disk usage in bytes for an origin.
    notImplemented();
    return 0;
}

JSValueRef TestRunner::originsWithApplicationCache(JSContextRef context)
{
    // FIXME: Implement to get origins that contain application caches.
    notImplemented();
    return JSValueMakeUndefined(context);
}

long long TestRunner::applicationCacheDiskUsageForOrigin(JSStringRef)
{
    notImplemented();
    return 0;
}

void TestRunner::clearAllDatabases()
{
    ewk_web_database_remove_all();
}

void TestRunner::setDatabaseQuota(unsigned long long quota)
{
    Ewk_Security_Origin* origin = ewk_frame_security_origin_get(browser->mainFrame());
    ewk_security_origin_web_database_quota_set(origin, quota);
    ewk_security_origin_free(origin);
}

JSValueRef TestRunner::originsWithLocalStorage(JSContextRef context)
{
    notImplemented();
    return JSValueMakeUndefined(context);
}

void TestRunner::deleteAllLocalStorage()
{
    notImplemented();
}

void TestRunner::deleteLocalStorageForOrigin(JSStringRef)
{
    notImplemented();
}

void TestRunner::observeStorageTrackerNotifications(unsigned)
{
    notImplemented();
}

void TestRunner::syncLocalStorage()
{
    notImplemented();
}

void TestRunner::setDomainRelaxationForbiddenForURLScheme(bool forbidden, JSStringRef scheme)
{
    DumpRenderTreeSupportEfl::setDomainRelaxationForbiddenForURLScheme(forbidden, scheme->string());
}

void TestRunner::goBack()
{
    ewk_frame_back(browser->mainFrame());
}

void TestRunner::setDefersLoading(bool defers)
{
    DumpRenderTreeSupportEfl::setDefersLoading(browser->mainView(), defers);
}

void TestRunner::setAppCacheMaximumSize(unsigned long long size)
{
    ewk_settings_application_cache_max_quota_set(size);
}

static inline bool toBool(JSStringRef value)
{
    return equals(value, "true") || equals(value, "1");
}

static inline int toInt(JSStringRef value)
{
    return atoi(value->string().utf8().data());
}

void TestRunner::overridePreference(JSStringRef key, JSStringRef value)
{
    if (equals(key, "WebKitJavaScriptEnabled"))
        ewk_view_setting_enable_scripts_set(browser->mainView(), toBool(value));
    else if (equals(key, "WebKitDefaultFontSize"))
        ewk_view_setting_font_default_size_set(browser->mainView(), toInt(value));
    else if (equals(key, "WebKitMinimumFontSize"))
        ewk_view_setting_font_minimum_size_set(browser->mainView(), toInt(value));
    else if (equals(key, "WebKitPluginsEnabled"))
        ewk_view_setting_enable_plugins_set(browser->mainView(), toBool(value));
    else if (equals(key, "WebKitWebGLEnabled"))
        ewk_view_setting_enable_webgl_set(browser->mainView(), toBool(value));
    else if (equals(key, "WebKitEnableCaretBrowsing"))
        ewk_view_setting_caret_browsing_set(browser->mainView(), toBool(value));
    else if (equals(key, "WebKitUsesPageCachePreferenceKey"))
        ewk_view_setting_page_cache_set(browser->mainView(), toBool(value));
    else if (equals(key, "WebKitHyperlinkAuditingEnabled"))
        ewk_view_setting_enable_hyperlink_auditing_set(browser->mainView(), toBool(value));
    else if (equals(key, "WebKitTabToLinksPreferenceKey"))
        ewk_view_setting_include_links_in_focus_chain_set(browser->mainView(), toBool(value));
    else if (equals(key, "WebKitOfflineWebApplicationCacheEnabled"))
        ewk_view_setting_application_cache_set(browser->mainView(), toBool(value));
    else if (equals(key, "WebKitLoadSiteIconsKey"))
        DumpRenderTreeSupportEfl::setLoadsSiteIconsIgnoringImageLoadingSetting(browser->mainView(), toBool(value));
    else if (equals(key, "WebKitCSSGridLayoutEnabled"))
        DumpRenderTreeSupportEfl::setCSSGridLayoutEnabled(browser->mainView(), toBool(value));
    else if (equals(key, "WebKitCSSRegionsEnabled"))
        DumpRenderTreeSupportEfl::setCSSRegionsEnabled(browser->mainView(), toBool(value));
    else if (equals(key, "WebKitWebAudioEnabled"))
        DumpRenderTreeSupportEfl::setWebAudioEnabled(browser->mainView(), toBool(value));
    else if (equals(key, "WebKitDisplayImagesKey"))
        ewk_view_setting_auto_load_images_set(browser->mainView(), toBool(value));
    else
        fprintf(stderr, "TestRunner::overridePreference tried to override unknown preference '%s'.\n", value->string().utf8().data());
}

void TestRunner::addUserScript(JSStringRef source, bool runAtStart, bool allFrames)
{
    DumpRenderTreeSupportEfl::addUserScript(browser->mainView(), source->string(), runAtStart, allFrames);
}

void TestRunner::addUserStyleSheet(JSStringRef source, bool allFrames)
{
    DumpRenderTreeSupportEfl::addUserStyleSheet(browser->mainView(), source->string(), allFrames);
}

void TestRunner::setDeveloperExtrasEnabled(bool enabled)
{
    ewk_view_setting_enable_developer_extras_set(browser->mainView(), enabled);
}

void TestRunner::showWebInspector()
{
    ewk_view_inspector_show(browser->mainView());
    browser->waitInspectorLoadFinished();
}

void TestRunner::closeWebInspector()
{
    ewk_view_inspector_close(browser->mainView());
}

void TestRunner::evaluateInWebInspector(long callId, JSStringRef script)
{
    DumpRenderTreeSupportEfl::evaluateInWebInspector(browser->mainView(), callId, script->string());
}

void TestRunner::evaluateScriptInIsolatedWorldAndReturnValue(unsigned, JSObjectRef, JSStringRef)
{
    notImplemented();
}

void TestRunner::evaluateScriptInIsolatedWorld(unsigned worldID, JSObjectRef globalObject, JSStringRef script)
{
    DumpRenderTreeSupportEfl::evaluateScriptInIsolatedWorld(browser->mainFrame(), worldID, globalObject, script->string());
}

void TestRunner::removeAllVisitedLinks()
{
    Ewk_History* history = ewk_view_history_get(browser->mainView());
    if (!history)
        return;

    ewk_history_clear(history);
}

bool TestRunner::callShouldCloseOnWebView()
{
    return DumpRenderTreeSupportEfl::callShouldCloseOnWebView(browser->mainFrame());
}

void TestRunner::apiTestNewWindowDataLoadBaseURL(JSStringRef, JSStringRef)
{
    notImplemented();
}

void TestRunner::apiTestGoToCurrentBackForwardItem()
{
    notImplemented();
}

void TestRunner::setWebViewEditable(bool)
{
    ewk_frame_editable_set(browser->mainFrame(), EINA_TRUE);
}

void TestRunner::authenticateSession(JSStringRef, JSStringRef, JSStringRef)
{
    notImplemented();
}

void TestRunner::abortModal()
{
    notImplemented();
}

void TestRunner::setSerializeHTTPLoads(bool serialize)
{
    DumpRenderTreeSupportEfl::setSerializeHTTPLoads(serialize);
}

void TestRunner::setTextDirection(JSStringRef direction)
{
    Ewk_Text_Direction ewkDirection;
    if (JSStringIsEqualToUTF8CString(direction, "auto"))
        ewkDirection = EWK_TEXT_DIRECTION_DEFAULT;
    else if (JSStringIsEqualToUTF8CString(direction, "rtl"))
        ewkDirection = EWK_TEXT_DIRECTION_RIGHT_TO_LEFT;
    else if (JSStringIsEqualToUTF8CString(direction, "ltr"))
        ewkDirection = EWK_TEXT_DIRECTION_LEFT_TO_RIGHT;
    else {
        fprintf(stderr, "TestRunner::setTextDirection called with unknown direction: '%s'.\n", direction->string().utf8().data());
        return;
    }

    ewk_view_text_direction_set(browser->mainView(), ewkDirection);
}

void TestRunner::addChromeInputField()
{
    notImplemented();
}

void TestRunner::removeChromeInputField()
{
    notImplemented();
}

void TestRunner::focusWebView()
{
    notImplemented();
}

void TestRunner::setBackingScaleFactor(double)
{
    notImplemented();
}

void TestRunner::grantWebNotificationPermission(JSStringRef origin)
{
}

void TestRunner::denyWebNotificationPermission(JSStringRef jsOrigin)
{
}

void TestRunner::removeAllWebNotificationPermissions()
{
}

void TestRunner::simulateWebNotificationClick(JSValueRef jsNotification)
{
}

void TestRunner::simulateLegacyWebNotificationClick(JSStringRef title)
{
}

void TestRunner::resetPageVisibility()
{
    ewk_view_visibility_state_set(browser->mainView(), EWK_PAGE_VISIBILITY_STATE_VISIBLE, true);
}

void TestRunner::setPageVisibility(const char* visibility)
{
    String newVisibility(visibility);
    if (newVisibility == "visible")
        ewk_view_visibility_state_set(browser->mainView(), EWK_PAGE_VISIBILITY_STATE_VISIBLE, false);
    else if (newVisibility == "hidden")
        ewk_view_visibility_state_set(browser->mainView(), EWK_PAGE_VISIBILITY_STATE_HIDDEN, false);
    else if (newVisibility == "prerender")
        ewk_view_visibility_state_set(browser->mainView(), EWK_PAGE_VISIBILITY_STATE_PRERENDER, false);
    else if (newVisibility == "unloaded")
        ewk_view_visibility_state_set(browser->mainView(), EWK_PAGE_VISIBILITY_STATE_UNLOADED, false);
}

void TestRunner::setAutomaticLinkDetectionEnabled(bool)
{
    notImplemented();
}

void TestRunner::setStorageDatabaseIdleInterval(double)
{
    notImplemented();
}

void TestRunner::closeIdleLocalStorageDatabases()
{
    notImplemented();
}

JSRetainPtr<JSStringRef> TestRunner::platformName() const
{
    JSRetainPtr<JSStringRef> platformName(Adopt, JSStringCreateWithUTF8CString("efl"));
    return platformName;
}
