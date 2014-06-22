/*
 * Copyright (C) 2011, 2012 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "WebKitUIClient.h"

#include "WebKitFileChooserRequestPrivate.h"
#include "WebKitGeolocationPermissionRequestPrivate.h"
#include "WebKitPrivate.h"
#include "WebKitWebViewBasePrivate.h"
#include "WebKitWebViewPrivate.h"
#include "WebKitWindowPropertiesPrivate.h"
#include "WebPageProxy.h"
#include <WebCore/GtkUtilities.h>
#include <wtf/gobject/GRefPtr.h>

using namespace WebKit;

static WKPageRef createNewPage(WKPageRef page, WKURLRequestRef, WKDictionaryRef wkWindowFeatures, WKEventModifiers, WKEventMouseButton, const void* clientInfo)
{
    return static_cast<WKPageRef>(toAPI(webkitWebViewCreateNewPage(WEBKIT_WEB_VIEW(clientInfo), toImpl(wkWindowFeatures))));
}

static void showPage(WKPageRef page, const void* clientInfo)
{
    webkitWebViewReadyToShowPage(WEBKIT_WEB_VIEW(clientInfo));
}

static void closePage(WKPageRef page, const void* clientInfo)
{
    webkitWebViewClosePage(WEBKIT_WEB_VIEW(clientInfo));
}

static void runJavaScriptAlert(WKPageRef page, WKStringRef message, WKFrameRef, const void* clientInfo)
{
    webkitWebViewRunJavaScriptAlert(WEBKIT_WEB_VIEW(clientInfo), toImpl(message)->string().utf8());
}

static bool runJavaScriptConfirm(WKPageRef page, WKStringRef message, WKFrameRef, const void* clientInfo)
{
    return webkitWebViewRunJavaScriptConfirm(WEBKIT_WEB_VIEW(clientInfo), toImpl(message)->string().utf8());
}

static WKStringRef runJavaScriptPrompt(WKPageRef page, WKStringRef message, WKStringRef defaultValue, WKFrameRef, const void* clientInfo)
{
    CString result = webkitWebViewRunJavaScriptPrompt(WEBKIT_WEB_VIEW(clientInfo), toImpl(message)->string().utf8(),
                                                      toImpl(defaultValue)->string().utf8());
    return WKStringCreateWithUTF8CString(result.data());
}

static bool toolbarsAreVisible(WKPageRef page, const void* clientInfo)
{
    WebKitWindowProperties* windowProperties = webkit_web_view_get_window_properties(WEBKIT_WEB_VIEW(clientInfo));
    return webkit_window_properties_get_toolbar_visible(windowProperties);
}

static void setToolbarsAreVisible(WKPageRef page, bool toolbarsVisible, const void* clientInfo)
{
    WebKitWindowProperties* windowProperties = webkit_web_view_get_window_properties(WEBKIT_WEB_VIEW(clientInfo));
    webkitWindowPropertiesSetToolbarVisible(windowProperties, toolbarsVisible);
}

static bool menuBarIsVisible(WKPageRef page, const void* clientInfo)
{
    WebKitWindowProperties* windowProperties = webkit_web_view_get_window_properties(WEBKIT_WEB_VIEW(clientInfo));
    return webkit_window_properties_get_menubar_visible(windowProperties);
}

static void setMenuBarIsVisible(WKPageRef page, bool menuBarVisible, const void* clientInfo)
{
    WebKitWindowProperties* windowProperties = webkit_web_view_get_window_properties(WEBKIT_WEB_VIEW(clientInfo));
    webkitWindowPropertiesSetMenubarVisible(windowProperties, menuBarVisible);
}

static bool statusBarIsVisible(WKPageRef page, const void* clientInfo)
{
    WebKitWindowProperties* windowProperties = webkit_web_view_get_window_properties(WEBKIT_WEB_VIEW(clientInfo));
    return webkit_window_properties_get_statusbar_visible(windowProperties);
}

static void setStatusBarIsVisible(WKPageRef page, bool statusBarVisible, const void* clientInfo)
{
    WebKitWindowProperties* windowProperties = webkit_web_view_get_window_properties(WEBKIT_WEB_VIEW(clientInfo));
    webkitWindowPropertiesSetStatusbarVisible(windowProperties, statusBarVisible);
}

static bool isResizable(WKPageRef page, const void* clientInfo)
{
    WebKitWindowProperties* windowProperties = webkit_web_view_get_window_properties(WEBKIT_WEB_VIEW(clientInfo));
    return webkit_window_properties_get_resizable(windowProperties);
}

static void setIsResizable(WKPageRef page, bool resizable, const void* clientInfo)
{
    WebKitWindowProperties* windowProperties = webkit_web_view_get_window_properties(WEBKIT_WEB_VIEW(clientInfo));
    webkitWindowPropertiesSetResizable(windowProperties, resizable);
}

static WKRect getWindowFrame(WKPageRef page, const void* clientInfo)
{
    GdkRectangle geometry = { 0, 0, 0, 0 };
    GtkWidget* window = gtk_widget_get_toplevel(GTK_WIDGET(clientInfo));
    if (WebCore::widgetIsOnscreenToplevelWindow(window) && gtk_widget_get_visible(window)) {
        gtk_window_get_position(GTK_WINDOW(window), &geometry.x, &geometry.y);
        gtk_window_get_size(GTK_WINDOW(window), &geometry.width, &geometry.height);
    }
    return WKRectMake(geometry.x, geometry.y, geometry.width, geometry.height);
}

static void setWindowFrame(WKPageRef page, WKRect frame, const void* clientInfo)
{
    WebKitWindowProperties* windowProperties = webkit_web_view_get_window_properties(WEBKIT_WEB_VIEW(clientInfo));
    GdkRectangle geometry = { static_cast<int>(frame.origin.x), static_cast<int>(frame.origin.y),
        static_cast<int>(frame.size.width), static_cast<int>(frame.size.height) };
    webkitWindowPropertiesSetGeometry(windowProperties, &geometry);
}

static void mouseDidMoveOverElement(WKPageRef page, WKHitTestResultRef hitTestResult, WKEventModifiers modifiers, WKTypeRef userData, const void* clientInfo)
{
    webkitWebViewMouseTargetChanged(WEBKIT_WEB_VIEW(clientInfo), toImpl(hitTestResult), wkEventModifiersToGdkModifiers(modifiers));
}

static void printFrame(WKPageRef page, WKFrameRef frame, const void*)
{
    webkitWebViewPrintFrame(WEBKIT_WEB_VIEW(toImpl(page)->viewWidget()), toImpl(frame));
}

static void runOpenPanel(WKPageRef page, WKFrameRef frame, WKOpenPanelParametersRef parameters, WKOpenPanelResultListenerRef listener, const void *clientInfo)
{
    GRefPtr<WebKitFileChooserRequest> request = adoptGRef(webkitFileChooserRequestCreate(toImpl(parameters), toImpl(listener)));
    webkitWebViewRunFileChooserRequest(WEBKIT_WEB_VIEW(clientInfo), request.get());
}

static void decidePolicyForGeolocationPermissionRequest(WKPageRef, WKFrameRef, WKSecurityOriginRef, WKGeolocationPermissionRequestRef request, const void* clientInfo)
{
    GRefPtr<WebKitGeolocationPermissionRequest> geolocationPermissionRequest = adoptGRef(webkitGeolocationPermissionRequestCreate(toImpl(request)));
    webkitWebViewMakePermissionRequest(WEBKIT_WEB_VIEW(clientInfo), WEBKIT_PERMISSION_REQUEST(geolocationPermissionRequest.get()));
}

static void runModal(WKPageRef page, const void* clientInfo)
{
    webkitWebViewRunAsModal(WEBKIT_WEB_VIEW(clientInfo));
}

void attachUIClientToView(WebKitWebView* webView)
{
    WKPageUIClient wkUIClient = {
        kWKPageUIClientCurrentVersion,
        webView, // clientInfo
        0, // createNewPage_deprecatedForUseWithV0
        showPage,
        closePage,
        0, // takeFocus
        0, // focus
        0, // unfocus
        runJavaScriptAlert,
        runJavaScriptConfirm,
        runJavaScriptPrompt,
        0, // setStatusText
        0, // mouseDidMoveOverElement_deprecatedForUseWithV0
        0, // missingPluginButtonClicked
        0, // didNotHandleKeyEvent
        0, // didNotHandleWheelEvent
        toolbarsAreVisible,
        setToolbarsAreVisible,
        menuBarIsVisible,
        setMenuBarIsVisible,
        statusBarIsVisible,
        setStatusBarIsVisible,
        isResizable,
        setIsResizable,
        getWindowFrame,
        setWindowFrame,
        0, // runBeforeUnloadConfirmPanel
        0, // didDraw
        0, // pageDidScroll
        0, // exceededDatabaseQuota
        runOpenPanel,
        decidePolicyForGeolocationPermissionRequest,
        0, // headerHeight
        0, // footerHeight
        0, // drawHeader
        0, // drawFooter
        printFrame,
        runModal,
        0, // didCompleteRubberBandForMainFrame
        0, // saveDataToFileInDownloadsFolder
        0, // shouldInterruptJavaScript
        createNewPage,
        mouseDidMoveOverElement,
        0, // decidePolicyForNotificationPermissionRequest
        0, // unavailablePluginButtonClicked
        0, // showColorPicker
        0, // hideColorPicker
        0, // pluginLoadPolicy
    };
    WKPageRef wkPage = toAPI(webkitWebViewBaseGetPage(WEBKIT_WEB_VIEW_BASE(webView)));
    WKPageSetPageUIClient(wkPage, &wkUIClient);
}

