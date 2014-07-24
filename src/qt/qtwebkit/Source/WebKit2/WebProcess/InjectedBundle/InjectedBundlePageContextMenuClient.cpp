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

#include "config.h"

#if ENABLE(CONTEXT_MENUS)

#include "InjectedBundlePageContextMenuClient.h"

#include "ImmutableArray.h"
#include "InjectedBundleHitTestResult.h"
#include "Logging.h"
#include "MutableArray.h"
#include "WebContextMenuItem.h"
#include "WKAPICast.h"
#include "WKBundleAPICast.h"
#include <WebCore/ContextMenu.h>

using namespace WebCore;

namespace WebKit {

bool InjectedBundlePageContextMenuClient::getCustomMenuFromDefaultItems(WebPage* page, InjectedBundleHitTestResult* hitTestResult, const Vector<WebContextMenuItemData>& defaultMenu, Vector<WebContextMenuItemData>& newMenu, RefPtr<APIObject>& userData)
{
    if (!m_client.getContextMenuFromDefaultMenu)
        return false;

    RefPtr<MutableArray> defaultMenuArray = MutableArray::create();
    defaultMenuArray->reserveCapacity(defaultMenu.size());
    for (unsigned i = 0; i < defaultMenu.size(); ++i)
        defaultMenuArray->append(WebContextMenuItem::create(defaultMenu[i]).get());

    WKArrayRef newMenuWK = 0;
    WKTypeRef userDataToPass = 0;
    m_client.getContextMenuFromDefaultMenu(toAPI(page), toAPI(hitTestResult), toAPI(defaultMenuArray.get()), &newMenuWK, &userDataToPass, m_client.clientInfo);
    RefPtr<ImmutableArray> array = adoptRef(toImpl(newMenuWK));
    userData = adoptRef(toImpl(userDataToPass));
    
    newMenu.clear();
    
    if (!array || !array->size())
        return true;
    
    size_t size = array->size();
    for (size_t i = 0; i < size; ++i) {
        WebContextMenuItem* item = array->at<WebContextMenuItem>(i);
        if (!item) {
            LOG(ContextMenu, "New menu entry at index %i is not a WebContextMenuItem", (int)i);
            continue;
        }
        
        newMenu.append(*item->data());
    }
    
    return true;
}

} // namespace WebKit
#endif // ENABLE(CONTEXT_MENUS)
