/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

#ifndef PageUIClientEfl_h
#define PageUIClientEfl_h

#include "WKPage.h"
#include "WKPopupItem.h"
#include <WebKit2/WKBase.h>
#include <wtf/PassOwnPtr.h>

class EwkView;

namespace WebKit {

class PageUIClientEfl {
public:
    static PassOwnPtr<PageUIClientEfl> create(EwkView* view)
    {
        return adoptPtr(new PageUIClientEfl(view));
    }

private:
    explicit PageUIClientEfl(EwkView*);

    static void close(WKPageRef, const void*);
    static void takeFocus(WKPageRef, WKFocusDirection, const void*);
    static void focus(WKPageRef, const void*);
    static void unfocus(WKPageRef, const void*);
    static void runJavaScriptAlert(WKPageRef, WKStringRef, WKFrameRef, const void*);
    static bool runJavaScriptConfirm(WKPageRef, WKStringRef, WKFrameRef, const void*);
    static WKStringRef runJavaScriptPrompt(WKPageRef, WKStringRef, WKStringRef, WKFrameRef, const void*);
    static bool toolbarsAreVisible(WKPageRef, const void* clientInfo);
    static void setToolbarsAreVisible(WKPageRef, bool, const void* clientInfo);
    static bool menuBarIsVisible(WKPageRef, const void* clientInfo);
    static void setMenuBarIsVisible(WKPageRef, bool, const void* clientInfo);
    static bool statusBarIsVisible(WKPageRef, const void* clientInfo);
    static void setStatusBarIsVisible(WKPageRef, bool, const void* clientInfo);
    static bool isResizable(WKPageRef, const void* clientInfo);
    static void setIsResizable(WKPageRef, bool, const void* clientInfo);
    static WKRect getWindowFrame(WKPageRef, const void*);
    static void setWindowFrame(WKPageRef, WKRect, const void*);
    static bool runBeforeUnloadConfirmPanel(WKPageRef, WKStringRef, WKFrameRef, const void*);
#if ENABLE(SQL_DATABASE)
    static unsigned long long exceededDatabaseQuota(WKPageRef, WKFrameRef, WKSecurityOriginRef, WKStringRef, WKStringRef, unsigned long long currentQuota, unsigned long long, unsigned long long, unsigned long long, const void*);
#endif
    static void runOpenPanel(WKPageRef, WKFrameRef, WKOpenPanelParametersRef, WKOpenPanelResultListenerRef, const void*);
    static WKPageRef createNewPage(WKPageRef, WKURLRequestRef, WKDictionaryRef, WKEventModifiers, WKEventMouseButton, const void*);
#if ENABLE(INPUT_TYPE_COLOR)
    static void showColorPicker(WKPageRef, WKStringRef initialColor, WKColorPickerResultListenerRef, const void*);
    static void hideColorPicker(WKPageRef, const void*);
#endif

    static void showPopupMenu(WKPageRef, WKPopupMenuListenerRef, WKRect, WKPopupItemTextDirection, double pageScaleFactor, WKArrayRef itemsRef, int32_t selectedIndex, const void* clientInfo);
    static void hidePopupMenu(WKPageRef, const void* clientInfo);

    EwkView* m_view;
};

} // namespace WebKit

#endif // PageUIClientEfl_h
