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

#ifndef WebPopupMenuProxyMac_h
#define WebPopupMenuProxyMac_h

#if USE(APPKIT)

#include "WebPopupMenuProxy.h"
#include <wtf/RetainPtr.h>

OBJC_CLASS NSPopUpButtonCell;
OBJC_CLASS WKView;

namespace WebKit {

class WebPageProxy;

class WebPopupMenuProxyMac : public WebPopupMenuProxy {
public:
    static PassRefPtr<WebPopupMenuProxyMac> create(WKView *webView, WebPopupMenuProxy::Client* client)
    {
        return adoptRef(new WebPopupMenuProxyMac(webView, client));
    }
    ~WebPopupMenuProxyMac();

    virtual void showPopupMenu(const WebCore::IntRect&, WebCore::TextDirection, double pageScaleFactor, const Vector<WebPopupItem>&, const PlatformPopupMenuData&, int32_t selectedIndex);
    virtual void hidePopupMenu();

private:
    WebPopupMenuProxyMac(WKView *, WebPopupMenuProxy::Client*);

    void populate(const Vector<WebPopupItem>&, NSFont *, WebCore::TextDirection);

    RetainPtr<NSPopUpButtonCell> m_popup;
    WKView *m_webView;
};

} // namespace WebKit

#endif // USE(APPKIT)

#endif // WebPopupMenuProxyMac_h
