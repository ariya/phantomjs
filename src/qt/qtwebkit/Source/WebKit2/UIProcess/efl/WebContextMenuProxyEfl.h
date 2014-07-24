/*
 * Copyright (C) 2012 Samsung Electronics.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS AS IS''
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

#ifndef WebContextMenuProxyEfl_h
#define WebContextMenuProxyEfl_h

#if ENABLE(CONTEXT_MENUS)

#include "WebContextMenuProxy.h"
#include <WebCore/ContextMenu.h>
#include <WebCore/IntPoint.h>

class EwkView;

namespace WebKit {

class WebContextMenuItemData;
class WebPageProxy;

class WebContextMenuProxyEfl : public WebContextMenuProxy {
public:
    static PassRefPtr<WebContextMenuProxyEfl> create(EwkView* viewImpl, WebPageProxy* page)
    {
        return adoptRef(new WebContextMenuProxyEfl(viewImpl, page));
    }

    ~WebContextMenuProxyEfl();

    void showContextMenu(const WebCore::IntPoint&, const Vector<WebContextMenuItemData>&);
    void hideContextMenu();

private:
    WebContextMenuProxyEfl(EwkView*, WebPageProxy*);

    EwkView* m_view;
    WebPageProxy* m_page;
};


} // namespace WebKit

#endif // ENABLE(CONTEXT_MENUS)
#endif // WebContextMenuProxyEfl_h
