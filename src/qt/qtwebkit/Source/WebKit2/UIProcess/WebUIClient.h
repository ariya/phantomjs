/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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

#ifndef WebUIClient_h
#define WebUIClient_h

#include "APIClient.h"
#include "WKPage.h"
#include "WebEvent.h"
#include "WebHitTestResult.h"
#include "WebOpenPanelParameters.h"
#include <wtf/Forward.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {
    class FloatRect;
    class IntSize;
    class ResourceRequest;
    struct WindowFeatures;
}

namespace WebKit {

class APIObject;
class GeolocationPermissionRequestProxy;
class ImmutableDictionary;
class NativeWebKeyboardEvent;
class NativeWebWheelEvent;
class NotificationPermissionRequest;
class WebColorPickerResultListenerProxy;
class WebData;
class WebFrameProxy;
class WebPageProxy;
class WebSecurityOrigin;
class WebOpenPanelResultListenerProxy;

class WebUIClient : public APIClient<WKPageUIClient, kWKPageUIClientCurrentVersion> {
public:
    PassRefPtr<WebPageProxy> createNewPage(WebPageProxy*, const WebCore::ResourceRequest&, const WebCore::WindowFeatures&, WebEvent::Modifiers, WebMouseEvent::Button);
    void showPage(WebPageProxy*);
    void close(WebPageProxy*);

    void takeFocus(WebPageProxy*, WKFocusDirection);
    void focus(WebPageProxy*);
    void unfocus(WebPageProxy*);

    void runJavaScriptAlert(WebPageProxy*, const String&, WebFrameProxy*);
    bool runJavaScriptConfirm(WebPageProxy*, const String&, WebFrameProxy*);
    String runJavaScriptPrompt(WebPageProxy*, const String&, const String&, WebFrameProxy*);

    void setStatusText(WebPageProxy*, const String&);
    void mouseDidMoveOverElement(WebPageProxy*, const WebHitTestResult::Data&, WebEvent::Modifiers, APIObject*);
    void unavailablePluginButtonClicked(WebPageProxy*, WKPluginUnavailabilityReason, ImmutableDictionary*);
    
    bool implementsDidNotHandleKeyEvent() const;
    void didNotHandleKeyEvent(WebPageProxy*, const NativeWebKeyboardEvent&);

    bool implementsDidNotHandleWheelEvent() const;
    void didNotHandleWheelEvent(WebPageProxy*, const NativeWebWheelEvent&);

    bool toolbarsAreVisible(WebPageProxy*);
    void setToolbarsAreVisible(WebPageProxy*, bool);
    bool menuBarIsVisible(WebPageProxy*);
    void setMenuBarIsVisible(WebPageProxy*, bool);
    bool statusBarIsVisible(WebPageProxy*);
    void setStatusBarIsVisible(WebPageProxy*, bool);
    bool isResizable(WebPageProxy*);
    void setIsResizable(WebPageProxy*, bool);

    void setWindowFrame(WebPageProxy*, const WebCore::FloatRect&);
    WebCore::FloatRect windowFrame(WebPageProxy*);

    bool canRunBeforeUnloadConfirmPanel() const;
    bool runBeforeUnloadConfirmPanel(WebPageProxy*, const String&, WebFrameProxy*);

    void didDraw(WebPageProxy*);
    void pageDidScroll(WebPageProxy*);

    unsigned long long exceededDatabaseQuota(WebPageProxy*, WebFrameProxy*, WebSecurityOrigin*, const String& databaseName, const String& databaseDisplayName, unsigned long long currentQuota, unsigned long long currentOriginUsage, unsigned long long currentDatabaseUsage, unsigned long long expectedUsage);

    bool runOpenPanel(WebPageProxy*, WebFrameProxy*, WebOpenPanelParameters*, WebOpenPanelResultListenerProxy*);
    bool decidePolicyForGeolocationPermissionRequest(WebPageProxy*, WebFrameProxy*, WebSecurityOrigin*, GeolocationPermissionRequestProxy*);
    bool decidePolicyForNotificationPermissionRequest(WebPageProxy*, WebSecurityOrigin*, NotificationPermissionRequest*);

    // Printing.
    float headerHeight(WebPageProxy*, WebFrameProxy*);
    float footerHeight(WebPageProxy*, WebFrameProxy*);
    void drawHeader(WebPageProxy*, WebFrameProxy*, const WebCore::FloatRect&);
    void drawFooter(WebPageProxy*, WebFrameProxy*, const WebCore::FloatRect&);
    void printFrame(WebPageProxy*, WebFrameProxy*);

    bool canRunModal() const;
    void runModal(WebPageProxy*);

    void saveDataToFileInDownloadsFolder(WebPageProxy*, const String& suggestedFilename, const String& mimeType, const String& originatingURLString, WebData*);

    bool shouldInterruptJavaScript(WebPageProxy*);

#if ENABLE(INPUT_TYPE_COLOR)
    bool showColorPicker(WebPageProxy*, const String&, WebColorPickerResultListenerProxy*);
    bool hideColorPicker(WebPageProxy*);
#endif
};

} // namespace WebKit

#endif // WebUIClient_h
