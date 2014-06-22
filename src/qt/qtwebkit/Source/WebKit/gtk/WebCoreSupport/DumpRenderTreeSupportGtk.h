/*
 *  Copyright (C) Research In Motion Limited 2010. All rights reserved.
 *  Copyright (C) 2012 Apple Inc. All Rights Reserved.
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

#ifndef DumpRenderTreeSupportGtk_h
#define DumpRenderTreeSupportGtk_h

#include "JSStringRef.h"
#include "PageVisibilityState.h"
#include <atk/atk.h>
#include <glib.h>
#include <webkit/webkitdefines.h>
#include <wtf/text/CString.h>

namespace WebKit {

enum {
    WebFindOptionsCaseInsensitive = 1 << 0,
    WebFindOptionsAtWordStarts = 1 << 1,
    WebFindOptionsTreatMedialCapitalAsWordStart = 1 << 2,
    WebFindOptionsBackwards = 1 << 3,
    WebFindOptionsWrapAround = 1 << 4,
    WebFindOptionsStartInSelection = 1 << 5
};

}
typedef unsigned WebKitFindOptions;

class DumpRenderTreeSupportGtk {

public:
    DumpRenderTreeSupportGtk();
    ~DumpRenderTreeSupportGtk();

    static void setDumpRenderTreeModeEnabled(bool);
    static bool dumpRenderTreeModeEnabled();

    static void setLinksIncludedInFocusChain(bool);
    static bool linksIncludedInFocusChain();

    static void clearOpener(WebKitWebFrame*);

    // FIXME: Move these to webkitwebframe.h once their API has been discussed.
    static GSList* getFrameChildren(WebKitWebFrame*);
    static WTF::CString getInnerText(WebKitWebFrame*);
    static WTF::CString dumpRenderTree(WebKitWebFrame*);
    static void addUserScript(WebKitWebFrame*, const char*, bool, bool);
    static void addUserStyleSheet(WebKitWebFrame*, const char* sourceCode, bool allFrames);
    static guint getPendingUnloadEventCount(WebKitWebFrame*);
    static void clearMainFrameName(WebKitWebFrame*);
    static AtkObject* getFocusedAccessibleElement(WebKitWebFrame*);
    static AtkObject* getRootAccessibleElement(WebKitWebFrame*);
    static void layoutFrame(WebKitWebFrame*);
    static void setValueForUser(JSContextRef, JSValueRef, JSStringRef);
    static bool shouldClose(WebKitWebFrame*);

    // WebKitWebView
    static void executeCoreCommandByName(WebKitWebView*, const gchar* name, const gchar* value);
    static bool isCommandEnabled(WebKitWebView*, const gchar* name);
    static bool findString(WebKitWebView*, const gchar*, WebKitFindOptions);
    static void rectangleForSelection(WebKitWebFrame*, cairo_rectangle_int_t*);
    static void scalePageBy(WebKitWebView*, float, float, float);
    static void setDefersLoading(WebKitWebView*, bool);
    static void forceWebViewPaint(WebKitWebView*);

    // Accessibility
    static WTF::CString accessibilityHelpText(AtkObject*);

    // TextInputController
    static void setComposition(WebKitWebView*, const char*, int start, int length);
    static bool hasComposition(WebKitWebView*);
    static bool compositionRange(WebKitWebView*, int* start, int* length);
    static void confirmComposition(WebKitWebView*, const char*);
    static bool firstRectForCharacterRange(WebKitWebView*, int location, int length, cairo_rectangle_int_t*);
    static bool selectedRange(WebKitWebView*, int* start, int* length);
    static void doCommand(WebKitWebView*, const char*);
    // GC
    static void gcCollectJavascriptObjects();
    static void gcCollectJavascriptObjectsOnAlternateThread(bool waitUntilDone);
    static unsigned long gcCountJavascriptObjects();

    static void whiteListAccessFromOrigin(const gchar* sourceOrigin, const gchar* destinationProtocol, const gchar* destinationHost, bool allowDestinationSubdomains);
    static void removeWhiteListAccessFromOrigin(const char* sourceOrigin, const char* destinationProtocol, const char* destinationHost, bool allowDestinationSubdomains);
    static void resetOriginAccessWhiteLists();

    static void resetGeolocationClientMock(WebKitWebView*);
    static void setMockGeolocationPermission(WebKitWebView*, bool allowed);
    static void setMockGeolocationPosition(WebKitWebView*, double latitude, double longitude, double accuracy);
    static void setMockGeolocationPositionUnavailableError(WebKitWebView*, const gchar* errorMessage);
    static int numberOfPendingGeolocationPermissionRequests(WebKitWebView*);

    static void setPageCacheSupportsPlugins(WebKitWebView*, bool enabled);
    static void setCSSGridLayoutEnabled(WebKitWebView*, bool enabled);
    static void setCSSRegionsEnabled(WebKitWebView*, bool enabled);
    static void setCSSCustomFilterEnabled(WebKitWebView*, bool enabled);
    static void setExperimentalContentSecurityPolicyFeaturesEnabled(bool);
    static void setSeamlessIFramesEnabled(bool);
    static void setShadowDOMEnabled(bool);
    static void setStyleScopedEnabled(bool);

    static void deliverAllMutationsIfNecessary();
    static void setDomainRelaxationForbiddenForURLScheme(bool forbidden, const char* urlScheme);
    static void setSerializeHTTPLoads(bool enabled);

    static void setTracksRepaints(WebKitWebFrame*, bool tracks);
    static bool isTrackingRepaints(WebKitWebFrame*);
    static GSList* trackedRepaintRects(WebKitWebFrame*);
    static void resetTrackedRepaints(WebKitWebFrame*);

    static void clearMemoryCache();
    static void clearApplicationCache();

    enum FrameLoadEvent {
        WillPerformClientRedirectToURL,
        DidCancelClientRedirect,
        DidReceiveServerRedirectForProvisionalLoad,
        DidDisplayInsecureContent,
        DidDetectXSS,
    };
    typedef void (*FrameLoadEventCallback)(WebKitWebFrame*, FrameLoadEvent, const char* url);
    static void setFrameLoadEventCallback(FrameLoadEventCallback);
    static FrameLoadEventCallback s_frameLoadEventCallback;

    typedef bool (*AuthenticationCallback) (CString& username, CString& password);
    static void setAuthenticationCallback(AuthenticationCallback);
    static AuthenticationCallback s_authenticationCallback;
    static void setPageVisibility(WebKitWebView*, WebCore::PageVisibilityState, bool);

private:
    static bool s_drtRun;
    static bool s_linksIncludedInTabChain;
};

#endif
