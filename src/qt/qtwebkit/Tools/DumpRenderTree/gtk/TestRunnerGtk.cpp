/*
 * Copyright (C) 2007, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2009 Jan Michael Alonzo <jmalonzo@gmail.com>
 * Copyright (C) 2009,2011 Collabora Ltd.
 * Copyright (C) 2010 Joone Hur <joone@kldp.org>
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
#include "WebCoreSupport/DumpRenderTreeSupportGtk.h"
#include "WorkQueue.h"
#include "WorkQueueItem.h"
#include <JavaScriptCore/JSRetainPtr.h>
#include <JavaScriptCore/JSStringRef.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <glib.h>
#include <libsoup/soup.h>
#include <webkit/webkit.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/WTFString.h>

extern "C" {
void webkit_web_inspector_execute_script(WebKitWebInspector* inspector, long callId, const gchar* script);
}

TestRunner::~TestRunner()
{
    // FIXME: implement
}

void TestRunner::addDisallowedURL(JSStringRef url)
{
    // FIXME: implement
}

void TestRunner::clearBackForwardList()
{
    WebKitWebView* webView = webkit_web_frame_get_web_view(mainFrame);
    WebKitWebBackForwardList* list = webkit_web_view_get_back_forward_list(webView);
    WebKitWebHistoryItem* item = webkit_web_back_forward_list_get_current_item(list);
    g_object_ref(item);

    // We clear the history by setting the back/forward list's capacity to 0
    // then restoring it back and adding back the current item.
    gint limit = webkit_web_back_forward_list_get_limit(list);
    webkit_web_back_forward_list_set_limit(list, 0);
    webkit_web_back_forward_list_set_limit(list, limit);
    webkit_web_back_forward_list_add_item(list, item);
    webkit_web_back_forward_list_go_to_item(list, item);
    g_object_unref(item);
}

JSStringRef TestRunner::copyDecodedHostName(JSStringRef name)
{
    // FIXME: implement
    return 0;
}

JSStringRef TestRunner::copyEncodedHostName(JSStringRef name)
{
    // FIXME: implement
    return 0;
}

void TestRunner::dispatchPendingLoadRequests()
{
    // FIXME: Implement for testing fix for 6727495
}

void TestRunner::display()
{
    displayWebView();
}

void TestRunner::keepWebHistory()
{
    // FIXME: implement
}

size_t TestRunner::webHistoryItemCount()
{
    WebKitWebView* webView = webkit_web_frame_get_web_view(mainFrame);
    WebKitWebBackForwardList* list = webkit_web_view_get_back_forward_list(webView);

    if (!list)
        return -1;

    // We do not add the current page to the total count as it's not
    // considered in DRT tests
    return webkit_web_back_forward_list_get_back_length(list) +
            webkit_web_back_forward_list_get_forward_length(list);
}

JSRetainPtr<JSStringRef> TestRunner::platformName() const
{
    JSRetainPtr<JSStringRef> platformName(Adopt, JSStringCreateWithUTF8CString("gtk"));
    return platformName;
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
    GOwnPtr<char> urlCString(JSStringCopyUTF8CString(url));
    if (!g_str_has_prefix(urlCString.get(), "file:///tmp/LayoutTests/"))
        return JSStringRetain(url);

    const char* layoutTestsSuffix = urlCString.get() + strlen("file:///tmp/");
    GOwnPtr<char> testPath(g_build_filename(getTopLevelPath().data(), layoutTestsSuffix, NULL));
    GOwnPtr<char> testURI(g_filename_to_uri(testPath.get(), 0, 0));
    return JSStringCreateWithUTF8CString(testURI.get());
}

static CString soupURIToStringPreservingPassword(SoupURI* soupURI)
{
    if (!soupURI->password) {
        GOwnPtr<char> uriString(soup_uri_to_string(soupURI, FALSE));
        return uriString.get();
    }

    // soup_uri_to_string does not insert the password into the string, so we need to create the
    // URI string and then reinsert any credentials that were present in the SoupURI. All tests that
    // use URL-embedded credentials use HTTP, so it's safe here.
    GOwnPtr<char> password(soupURI->password);
    GOwnPtr<char> user(soupURI->user);
    soupURI->password = 0;
    soupURI->user = 0;

    GOwnPtr<char> uriString(soup_uri_to_string(soupURI, FALSE));
    String absoluteURIWithoutCredentialString = String::fromUTF8(uriString.get());
    String protocolAndCredential = String::format("http://%s:%s@", user ? user.get() : "", password.get());
    return absoluteURIWithoutCredentialString.replace("http://", protocolAndCredential).utf8();
}

void TestRunner::queueLoad(JSStringRef url, JSStringRef target)
{
    GOwnPtr<gchar> relativeURL(JSStringCopyUTF8CString(url));
    SoupURI* baseURI = soup_uri_new(webkit_web_frame_get_uri(mainFrame));
    SoupURI* absoluteURI = soup_uri_new_with_base(baseURI, relativeURL.get());
    soup_uri_free(baseURI);

    if (!absoluteURI) {
        WorkQueue::shared()->queue(new LoadItem(url, target));
        return;
    }

    CString absoluteURIString = soupURIToStringPreservingPassword(absoluteURI);
    JSRetainPtr<JSStringRef> absoluteURL(Adopt, JSStringCreateWithUTF8CString(absoluteURIString.data()));
    WorkQueue::shared()->queue(new LoadItem(absoluteURL.get(), target));
    soup_uri_free(absoluteURI);
}

void TestRunner::setAcceptsEditing(bool acceptsEditing)
{
    WebKitWebView* webView = webkit_web_frame_get_web_view(mainFrame);
    webkit_web_view_set_editable(webView, acceptsEditing);
}

void TestRunner::setAlwaysAcceptCookies(bool alwaysAcceptCookies)
{
    SoupSession* session = webkit_get_default_session();
    SoupCookieJar* jar = reinterpret_cast<SoupCookieJar*>(soup_session_get_feature(session, SOUP_TYPE_COOKIE_JAR));

    /* If the jar was not created - we create it on demand, i.e, just
       in case we have HTTP requests - then we must create it here in
       order to set the proper accept policy */
    if (!jar) {
        jar = soup_cookie_jar_new();
        soup_session_add_feature(session, SOUP_SESSION_FEATURE(jar));
        g_object_unref(jar);
    }

    SoupCookieJarAcceptPolicy policy;

    if (alwaysAcceptCookies)
        policy = SOUP_COOKIE_JAR_ACCEPT_ALWAYS;
    else
        policy = SOUP_COOKIE_JAR_ACCEPT_NO_THIRD_PARTY;

    g_object_set(G_OBJECT(jar), SOUP_COOKIE_JAR_ACCEPT_POLICY, policy, NULL);
}

void TestRunner::setCustomPolicyDelegate(bool setDelegate, bool permissive)
{
    // FIXME: implement
}

void TestRunner::waitForPolicyDelegate()
{
    waitForPolicy = true;
    setWaitToDump(true);
}

void TestRunner::setScrollbarPolicy(JSStringRef orientation, JSStringRef policy)
{
    // FIXME: implement
}

void TestRunner::addOriginAccessWhitelistEntry(JSStringRef sourceOrigin, JSStringRef protocol, JSStringRef host, bool includeSubdomains)
{
    gchar* sourceOriginGChar = JSStringCopyUTF8CString(sourceOrigin);
    gchar* protocolGChar = JSStringCopyUTF8CString(protocol);
    gchar* hostGChar = JSStringCopyUTF8CString(host);
    DumpRenderTreeSupportGtk::whiteListAccessFromOrigin(sourceOriginGChar, protocolGChar, hostGChar, includeSubdomains);
    g_free(sourceOriginGChar);
    g_free(protocolGChar);
    g_free(hostGChar);
}

void TestRunner::removeOriginAccessWhitelistEntry(JSStringRef sourceOrigin, JSStringRef protocol, JSStringRef host, bool includeSubdomains)
{
    GOwnPtr<gchar> sourceOriginGChar(JSStringCopyUTF8CString(sourceOrigin));
    GOwnPtr<gchar> protocolGChar(JSStringCopyUTF8CString(protocol));
    GOwnPtr<gchar> hostGChar(JSStringCopyUTF8CString(host));
    DumpRenderTreeSupportGtk::removeWhiteListAccessFromOrigin(sourceOriginGChar.get(), protocolGChar.get(), hostGChar.get(), includeSubdomains);
}

void TestRunner::setMainFrameIsFirstResponder(bool flag)
{
    // FIXME: implement
}

void TestRunner::setTabKeyCyclesThroughElements(bool cycles)
{
    WebKitWebView* webView = webkit_web_frame_get_web_view(mainFrame);
    WebKitWebSettings* settings = webkit_web_view_get_settings(webView);
    g_object_set(G_OBJECT(settings), "tab-key-cycles-through-elements", cycles, NULL);
}

void TestRunner::setUseDashboardCompatibilityMode(bool flag)
{
    // FIXME: implement
}

static gchar* userStyleSheet = NULL;
static gboolean userStyleSheetEnabled = TRUE;

void TestRunner::setUserStyleSheetEnabled(bool flag)
{
    userStyleSheetEnabled = flag;

    WebKitWebView* webView = webkit_web_frame_get_web_view(mainFrame);
    WebKitWebSettings* settings = webkit_web_view_get_settings(webView);
    if (flag && userStyleSheet)
        g_object_set(G_OBJECT(settings), "user-stylesheet-uri", userStyleSheet, NULL);
    else
        g_object_set(G_OBJECT(settings), "user-stylesheet-uri", "", NULL);
}

void TestRunner::setUserStyleSheetLocation(JSStringRef path)
{
    g_free(userStyleSheet);
    userStyleSheet = JSStringCopyUTF8CString(path);
    if (userStyleSheetEnabled)
        setUserStyleSheetEnabled(true);
}

void TestRunner::setValueForUser(JSContextRef context, JSValueRef nodeObject, JSStringRef value)
{
    DumpRenderTreeSupportGtk::setValueForUser(context, nodeObject, value);
}

void TestRunner::setViewModeMediaFeature(JSStringRef mode)
{
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(view);

    char* viewMode = JSStringCopyUTF8CString(mode);

    if (!g_strcmp0(viewMode, "windowed"))
        webkit_web_view_set_view_mode(view, WEBKIT_WEB_VIEW_VIEW_MODE_WINDOWED);
    else if (!g_strcmp0(viewMode, "floating"))
        webkit_web_view_set_view_mode(view, WEBKIT_WEB_VIEW_VIEW_MODE_FLOATING);
    else if (!g_strcmp0(viewMode, "fullscreen"))
        webkit_web_view_set_view_mode(view, WEBKIT_WEB_VIEW_VIEW_MODE_FULLSCREEN);
    else if (!g_strcmp0(viewMode, "maximized"))
        webkit_web_view_set_view_mode(view, WEBKIT_WEB_VIEW_VIEW_MODE_MAXIMIZED);
    else if (!g_strcmp0(viewMode, "minimized"))
        webkit_web_view_set_view_mode(view, WEBKIT_WEB_VIEW_VIEW_MODE_MINIMIZED);

    g_free(viewMode);
}

void TestRunner::setWindowIsKey(bool windowIsKey)
{
    // FIXME: implement
}

static gboolean waitToDumpWatchdogFired(void*)
{
    setWaitToDumpWatchdog(0);
    gTestRunner->waitToDumpWatchdogTimerFired();
    return FALSE;
}

void TestRunner::setWaitToDump(bool waitUntilDone)
{
    static const int timeoutSeconds = 30;

    m_waitToDump = waitUntilDone;
    if (m_waitToDump && shouldSetWaitToDumpWatchdog())
        setWaitToDumpWatchdog(g_timeout_add_seconds(timeoutSeconds, waitToDumpWatchdogFired, 0));
}

int TestRunner::windowCount()
{
    // +1 -> including the main view
    return g_slist_length(webViewList) + 1;
}

void TestRunner::setPrivateBrowsingEnabled(bool flag)
{
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(view);

    WebKitWebSettings* settings = webkit_web_view_get_settings(view);
    g_object_set(G_OBJECT(settings), "enable-private-browsing", flag, NULL);
}

void TestRunner::setJavaScriptCanAccessClipboard(bool flag)
{
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(view);

    WebKitWebSettings* settings = webkit_web_view_get_settings(view);
    g_object_set(G_OBJECT(settings), "javascript-can-access-clipboard", flag, NULL);
}

void TestRunner::setXSSAuditorEnabled(bool flag)
{
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(view);

    WebKitWebSettings* settings = webkit_web_view_get_settings(view);
    g_object_set(G_OBJECT(settings), "enable-xss-auditor", flag, NULL);
}

void TestRunner::setSpatialNavigationEnabled(bool flag)
{
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(view);

    WebKitWebSettings* settings = webkit_web_view_get_settings(view);
    g_object_set(G_OBJECT(settings), "enable-spatial-navigation", flag, NULL);
}

void TestRunner::setAllowUniversalAccessFromFileURLs(bool flag)
{
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(view);

    WebKitWebSettings* settings = webkit_web_view_get_settings(view);
    g_object_set(G_OBJECT(settings), "enable-universal-access-from-file-uris", flag, NULL);
}

void TestRunner::setAllowFileAccessFromFileURLs(bool flag)
{
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(view);

    WebKitWebSettings* settings = webkit_web_view_get_settings(view);
    g_object_set(G_OBJECT(settings), "enable-file-access-from-file-uris", flag, NULL);
}

void TestRunner::setAuthorAndUserStylesEnabled(bool flag)
{
    // FIXME: implement
}

void TestRunner::setMockDeviceOrientation(bool canProvideAlpha, double alpha, bool canProvideBeta, double beta, bool canProvideGamma, double gamma)
{
    // FIXME: Implement for DeviceOrientation layout tests.
    // See https://bugs.webkit.org/show_bug.cgi?id=30335.
}

void TestRunner::setMockGeolocationPosition(double latitude, double longitude, double accuracy, bool, double, bool, double, bool, double, bool, double)
{
    WebKitWebView* view = WEBKIT_WEB_VIEW(g_slist_nth_data(webViewList, 0));
    if (!view)
        view = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(view);

    DumpRenderTreeSupportGtk::setMockGeolocationPosition(view, latitude, longitude, accuracy);
}

void TestRunner::setMockGeolocationPositionUnavailableError(JSStringRef message)
{
    WebKitWebView* view = WEBKIT_WEB_VIEW(g_slist_nth_data(webViewList, 0));
    if (!view)
        view = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(view);

    GOwnPtr<gchar> cMessage(JSStringCopyUTF8CString(message));
    DumpRenderTreeSupportGtk::setMockGeolocationPositionUnavailableError(view, cMessage.get());
}

void TestRunner::setGeolocationPermission(bool allow)
{
    setGeolocationPermissionCommon(allow);
    WebKitWebView* view = WEBKIT_WEB_VIEW(g_slist_nth_data(webViewList, 0));
    if (!view)
        view = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(view);

    DumpRenderTreeSupportGtk::setMockGeolocationPermission(view, allow);
}

int TestRunner::numberOfPendingGeolocationPermissionRequests()
{
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    if (!view)
        view = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(view);

    return DumpRenderTreeSupportGtk::numberOfPendingGeolocationPermissionRequests(view);
}

void TestRunner::addMockSpeechInputResult(JSStringRef result, double confidence, JSStringRef language)
{
    // FIXME: Implement for speech input layout tests.
    // See https://bugs.webkit.org/show_bug.cgi?id=39485.
}

void TestRunner::setMockSpeechInputDumpRect(bool flag)
{
    // FIXME: Implement for speech input layout tests.
    // See https://bugs.webkit.org/show_bug.cgi?id=39485.
}

void TestRunner::startSpeechInput(JSContextRef inputElement)
{
    // FIXME: Implement for speech input layout tests.
    // See https://bugs.webkit.org/show_bug.cgi?id=39485.
}

void TestRunner::setIconDatabaseEnabled(bool enabled)
{
    WebKitIconDatabase* database = webkit_get_icon_database();
    if (enabled) {
        GOwnPtr<gchar> iconDatabasePath(g_build_filename(g_get_tmp_dir(), "DumpRenderTree", "icondatabase", NULL));
        webkit_icon_database_set_path(database, iconDatabasePath.get());
    } else
        webkit_icon_database_set_path(database, 0);
}

void TestRunner::setPopupBlockingEnabled(bool flag)
{
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(view);

    WebKitWebSettings* settings = webkit_web_view_get_settings(view);
    g_object_set(G_OBJECT(settings), "javascript-can-open-windows-automatically", !flag, NULL);

}

void TestRunner::setPluginsEnabled(bool flag)
{
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(view);

    WebKitWebSettings* settings = webkit_web_view_get_settings(view);
    g_object_set(G_OBJECT(settings), "enable-plugins", flag, NULL);
}

void TestRunner::execCommand(JSStringRef name, JSStringRef value)
{
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(view);

    gchar* cName = JSStringCopyUTF8CString(name);
    gchar* cValue = JSStringCopyUTF8CString(value);
    DumpRenderTreeSupportGtk::executeCoreCommandByName(view, cName, cValue);
    g_free(cName);
    g_free(cValue);
}

bool TestRunner::findString(JSContextRef context, JSStringRef target, JSObjectRef optionsArray)
{
    WebKitFindOptions findOptions = 0;
    WebKitWebView* webView = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(webView);

    JSRetainPtr<JSStringRef> lengthPropertyName(Adopt, JSStringCreateWithUTF8CString("length"));
    JSValueRef lengthValue = JSObjectGetProperty(context, optionsArray, lengthPropertyName.get(), 0); 
    if (!JSValueIsNumber(context, lengthValue))
        return false;

    GOwnPtr<gchar> targetString(JSStringCopyUTF8CString(target));

    size_t length = static_cast<size_t>(JSValueToNumber(context, lengthValue, 0));
    for (size_t i = 0; i < length; ++i) {
        JSValueRef value = JSObjectGetPropertyAtIndex(context, optionsArray, i, 0); 
        if (!JSValueIsString(context, value))
            continue;
    
        JSRetainPtr<JSStringRef> optionName(Adopt, JSValueToStringCopy(context, value, 0));

        if (JSStringIsEqualToUTF8CString(optionName.get(), "CaseInsensitive"))
            findOptions |= WebKit::WebFindOptionsCaseInsensitive;
        else if (JSStringIsEqualToUTF8CString(optionName.get(), "AtWordStarts"))
            findOptions |= WebKit::WebFindOptionsAtWordStarts;
        else if (JSStringIsEqualToUTF8CString(optionName.get(), "TreatMedialCapitalAsWordStart"))
            findOptions |= WebKit::WebFindOptionsTreatMedialCapitalAsWordStart;
        else if (JSStringIsEqualToUTF8CString(optionName.get(), "Backwards"))
            findOptions |= WebKit::WebFindOptionsBackwards;
        else if (JSStringIsEqualToUTF8CString(optionName.get(), "WrapAround"))
            findOptions |= WebKit::WebFindOptionsWrapAround;
        else if (JSStringIsEqualToUTF8CString(optionName.get(), "StartInSelection"))
            findOptions |= WebKit::WebFindOptionsStartInSelection;
    }   

    return DumpRenderTreeSupportGtk::findString(webView, targetString.get(), findOptions); 
}

bool TestRunner::isCommandEnabled(JSStringRef name)
{
    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(view);

    gchar* cName = JSStringCopyUTF8CString(name);
    bool result = DumpRenderTreeSupportGtk::isCommandEnabled(view, cName);
    g_free(cName);
    return result;
}

void TestRunner::setCacheModel(int cacheModel)
{
    // These constants are derived from the Mac cache model enum in Source/WebKit/mac/WebView/WebPreferences.h.
    switch (cacheModel) {
    case 0:
        webkit_set_cache_model(WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER);
        break;
    case 1:
        webkit_set_cache_model(WEBKIT_CACHE_MODEL_DOCUMENT_BROWSER);
        break;
    case 2:
        webkit_set_cache_model(WEBKIT_CACHE_MODEL_WEB_BROWSER);
        break;
    default:
        ASSERT_NOT_REACHED();
    }
}

void TestRunner::setPersistentUserStyleSheetLocation(JSStringRef jsURL)
{
    // FIXME: implement
}

void TestRunner::clearPersistentUserStyleSheet()
{
    // FIXME: implement
}

void TestRunner::clearAllApplicationCaches()
{
    // FIXME: Implement to support application cache quotas.
}

void TestRunner::setApplicationCacheOriginQuota(unsigned long long quota)
{
    // FIXME: Implement to support application cache quotas.
}

void TestRunner::clearApplicationCacheForOrigin(OpaqueJSString*)
{
    // FIXME: Implement to support deleting all application caches for an origin.
}

long long TestRunner::localStorageDiskUsageForOrigin(JSStringRef originIdentifier)
{
    // FIXME: Implement to support getting disk usage in bytes for an origin.
    return 0;
}

JSValueRef TestRunner::originsWithApplicationCache(JSContextRef context)
{
    // FIXME: Implement to get origins that contain application caches.
    return JSValueMakeUndefined(context);
}

long long TestRunner::applicationCacheDiskUsageForOrigin(JSStringRef name)
{
    // FIXME: implement
    return 0;
}

void TestRunner::clearAllDatabases()
{
    webkit_remove_all_web_databases();
}
 
void TestRunner::setDatabaseQuota(unsigned long long quota)
{
    WebKitSecurityOrigin* origin = webkit_web_frame_get_security_origin(mainFrame);
    webkit_security_origin_set_web_database_quota(origin, quota);
}

JSValueRef TestRunner::originsWithLocalStorage(JSContextRef context)
{
    // FIXME: implement
    return JSValueMakeUndefined(context);
}

void TestRunner::deleteAllLocalStorage()
{
        // FIXME: implement
}

void TestRunner::deleteLocalStorageForOrigin(JSStringRef originIdentifier)
{
        // FIXME: implement
}

void TestRunner::observeStorageTrackerNotifications(unsigned number)
{
        // FIXME: implement
}

void TestRunner::syncLocalStorage()
{
    // FIXME: implement
}

void TestRunner::setDomainRelaxationForbiddenForURLScheme(bool forbidden, JSStringRef scheme)
{
    GOwnPtr<gchar> urlScheme(JSStringCopyUTF8CString(scheme));
    DumpRenderTreeSupportGtk::setDomainRelaxationForbiddenForURLScheme(forbidden, urlScheme.get());
}

void TestRunner::goBack()
{
    WebKitWebView* webView = webkit_web_frame_get_web_view(mainFrame);
    webkit_web_view_go_back(webView);
}

void TestRunner::setDefersLoading(bool defers)
{
    WebKitWebView* webView = webkit_web_frame_get_web_view(mainFrame);
    DumpRenderTreeSupportGtk::setDefersLoading(webView, defers);
}

void TestRunner::setAppCacheMaximumSize(unsigned long long size)
{
    webkit_application_cache_set_maximum_size(size);
}

static gboolean booleanFromValue(gchar* value)
{
    return !g_ascii_strcasecmp(value, "true") || !g_ascii_strcasecmp(value, "1");
}

void TestRunner::overridePreference(JSStringRef key, JSStringRef value)
{
    GOwnPtr<gchar> originalName(JSStringCopyUTF8CString(key));
    GOwnPtr<gchar> valueAsString(JSStringCopyUTF8CString(value));

    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(view);

    // This transformation could be handled by a hash table (and it once was), but
    // having it prominent, makes it easier for people from other ports to keep the
    // list up to date.
    const gchar* propertyName = 0;
    if (g_str_equal(originalName.get(), "WebKitJavaScriptEnabled"))
        propertyName = "enable-scripts";
    else if (g_str_equal(originalName.get(), "WebKitDefaultFontSize"))
        propertyName = "default-font-size";
    else if (g_str_equal(originalName.get(), "WebKitEnableCaretBrowsing"))
        propertyName = "enable-caret-browsing";
    else if (g_str_equal(originalName.get(), "WebKitUsesPageCachePreferenceKey"))
        propertyName = "enable-page-cache";
    else if (g_str_equal(originalName.get(), "WebKitPluginsEnabled"))
        propertyName = "enable-plugins";
    else if (g_str_equal(originalName.get(), "WebKitHyperlinkAuditingEnabled"))
        propertyName = "enable-hyperlink-auditing";
    else if (g_str_equal(originalName.get(), "WebKitWebGLEnabled"))
        propertyName = "enable-webgl";
    else if (g_str_equal(originalName.get(), "WebKitWebAudioEnabled"))
        propertyName = "enable-webaudio";
    else if (g_str_equal(originalName.get(), "WebKitDisplayImagesKey"))
        propertyName = "auto-load-images";
    else if (g_str_equal(originalName.get(), "WebKitTabToLinksPreferenceKey")) {
        DumpRenderTreeSupportGtk::setLinksIncludedInFocusChain(booleanFromValue(valueAsString.get()));
        return;
    } else if (g_str_equal(originalName.get(), "WebKitPageCacheSupportsPluginsPreferenceKey")) {
        DumpRenderTreeSupportGtk::setPageCacheSupportsPlugins(webkit_web_frame_get_web_view(mainFrame), booleanFromValue(valueAsString.get()));
        return;
    } else if (g_str_equal(originalName.get(), "WebKitCSSGridLayoutEnabled")) {
        DumpRenderTreeSupportGtk::setCSSGridLayoutEnabled(webkit_web_frame_get_web_view(mainFrame), booleanFromValue(valueAsString.get()));
        return;
    } else if (g_str_equal(originalName.get(), "WebKitCSSRegionsEnabled")) {
        DumpRenderTreeSupportGtk::setCSSRegionsEnabled(webkit_web_frame_get_web_view(mainFrame), booleanFromValue(valueAsString.get()));
        return;
    } else if (g_str_equal(originalName.get(), "WebKitCSSCustomFilterEnabled")) {
        DumpRenderTreeSupportGtk::setCSSCustomFilterEnabled(webkit_web_frame_get_web_view(mainFrame), booleanFromValue(valueAsString.get()));
        return;
    } else {
        fprintf(stderr, "TestRunner::overridePreference tried to override "
                "unknown preference '%s'.\n", originalName.get());
        return;
    }

    WebKitWebSettings* settings = webkit_web_view_get_settings(view);
    GParamSpec* pspec = g_object_class_find_property(G_OBJECT_CLASS(
        WEBKIT_WEB_SETTINGS_GET_CLASS(settings)), propertyName);
    GValue currentPropertyValue = { 0, { { 0 } } };
    g_value_init(&currentPropertyValue, pspec->value_type);

    if (G_VALUE_HOLDS_STRING(&currentPropertyValue))
        g_object_set(settings, propertyName, valueAsString.get(), NULL);
    else if (G_VALUE_HOLDS_BOOLEAN(&currentPropertyValue))
        g_object_set(G_OBJECT(settings), propertyName, booleanFromValue(valueAsString.get()), NULL);
    else if (G_VALUE_HOLDS_INT(&currentPropertyValue))
        g_object_set(G_OBJECT(settings), propertyName, atoi(valueAsString.get()), NULL);
    else if (G_VALUE_HOLDS_FLOAT(&currentPropertyValue)) {
        gfloat newValue = g_ascii_strtod(valueAsString.get(), 0);
        g_object_set(G_OBJECT(settings), propertyName, newValue, NULL);
    } else
        fprintf(stderr, "TestRunner::overridePreference failed to override "
                "preference '%s'.\n", originalName.get());
}

void TestRunner::addUserScript(JSStringRef source, bool runAtStart, bool allFrames)
{
    GOwnPtr<gchar> sourceCode(JSStringCopyUTF8CString(source));
    DumpRenderTreeSupportGtk::addUserScript(mainFrame, sourceCode.get(), runAtStart, allFrames);
}

void TestRunner::addUserStyleSheet(JSStringRef source, bool allFrames)
{
    GOwnPtr<gchar> sourceCode(JSStringCopyUTF8CString(source));
    DumpRenderTreeSupportGtk::addUserStyleSheet(mainFrame, sourceCode.get(), allFrames);
    // FIXME: needs more investigation why userscripts/user-style-top-frame-only.html fails when allFrames is false.

}

void TestRunner::setDeveloperExtrasEnabled(bool enabled)
{
    WebKitWebView* webView = webkit_web_frame_get_web_view(mainFrame);
    WebKitWebSettings* webSettings = webkit_web_view_get_settings(webView);

    g_object_set(webSettings, "enable-developer-extras", enabled, NULL);
}

void TestRunner::showWebInspector()
{
    WebKitWebView* webView = webkit_web_frame_get_web_view(mainFrame);
    WebKitWebInspector* inspector = webkit_web_view_get_inspector(webView);

    webkit_web_inspector_show(inspector);
}

void TestRunner::closeWebInspector()
{
    WebKitWebView* webView = webkit_web_frame_get_web_view(mainFrame);
    WebKitWebInspector* inspector = webkit_web_view_get_inspector(webView);

    webkit_web_inspector_close(inspector);
}

void TestRunner::evaluateInWebInspector(long callId, JSStringRef script)
{
    WebKitWebView* webView = webkit_web_frame_get_web_view(mainFrame);
    WebKitWebInspector* inspector = webkit_web_view_get_inspector(webView);
    char* scriptString = JSStringCopyUTF8CString(script);

    webkit_web_inspector_execute_script(inspector, callId, scriptString);
    g_free(scriptString);
}

void TestRunner::evaluateScriptInIsolatedWorldAndReturnValue(unsigned worldID, JSObjectRef globalObject, JSStringRef script)
{
    // FIXME: Implement this.
}

void TestRunner::evaluateScriptInIsolatedWorld(unsigned worldID, JSObjectRef globalObject, JSStringRef script)
{
    // FIXME: Implement this.
}

void TestRunner::removeAllVisitedLinks()
{
    // FIXME: Implement this.
}

bool TestRunner::callShouldCloseOnWebView()
{
    return DumpRenderTreeSupportGtk::shouldClose(mainFrame);
}

void TestRunner::apiTestNewWindowDataLoadBaseURL(JSStringRef utf8Data, JSStringRef baseURL)
{

}

void TestRunner::apiTestGoToCurrentBackForwardItem()
{

}

void TestRunner::setWebViewEditable(bool)
{
}

void TestRunner::authenticateSession(JSStringRef, JSStringRef, JSStringRef)
{
}

void TestRunner::abortModal()
{
}

void TestRunner::setSerializeHTTPLoads(bool serialize)
{
    DumpRenderTreeSupportGtk::setSerializeHTTPLoads(serialize);
}

void TestRunner::setTextDirection(JSStringRef direction)
{
    GOwnPtr<gchar> writingDirection(JSStringCopyUTF8CString(direction));

    WebKitWebView* view = webkit_web_frame_get_web_view(mainFrame);
    ASSERT(view);

    if (g_str_equal(writingDirection.get(), "auto"))
        gtk_widget_set_direction(GTK_WIDGET(view), GTK_TEXT_DIR_NONE);
    else if (g_str_equal(writingDirection.get(), "ltr"))
        gtk_widget_set_direction(GTK_WIDGET(view), GTK_TEXT_DIR_LTR);
    else if (g_str_equal(writingDirection.get(), "rtl"))
        gtk_widget_set_direction(GTK_WIDGET(view), GTK_TEXT_DIR_RTL);
    else
        fprintf(stderr, "TestRunner::setTextDirection called with unknown direction: '%s'.\n", writingDirection.get());
}

void TestRunner::addChromeInputField()
{
}

void TestRunner::removeChromeInputField()
{
}

void TestRunner::focusWebView()
{
}

void TestRunner::setBackingScaleFactor(double)
{
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
    WebKitWebView* webView = webkit_web_frame_get_web_view(mainFrame);
    DumpRenderTreeSupportGtk::setPageVisibility(webView, WebCore::PageVisibilityStateVisible, true);
}

void TestRunner::setPageVisibility(const char* visibility)
{
    WebKitWebView* webView = webkit_web_frame_get_web_view(mainFrame);
    String visibilityString(visibility);
    WebCore::PageVisibilityState visibilityState = WebCore::PageVisibilityStateVisible;

    if (visibilityString == "visible")
        visibilityState = WebCore::PageVisibilityStateVisible;
    else if (visibilityString == "hidden")
        visibilityState = WebCore::PageVisibilityStateHidden;
    else
        return;

    DumpRenderTreeSupportGtk::setPageVisibility(webView, visibilityState, false);
}

void TestRunner::setAutomaticLinkDetectionEnabled(bool)
{
    // FIXME: Implement this.
}

void TestRunner::setStorageDatabaseIdleInterval(double)
{
    // FIXME: Implement this.
}

void TestRunner::closeIdleLocalStorageDatabases()
{
}
