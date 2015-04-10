/*
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

#ifndef ewk_back_forward_list_private_h
#define ewk_back_forward_list_private_h

#include "WKRetainPtr.h"
#include "ewk_back_forward_list_item_private.h"
#include <WebKit2/WKBase.h>
#include <wtf/HashMap.h>
#include <wtf/PassOwnPtr.h>

typedef HashMap<WKBackForwardListItemRef, RefPtr<EwkBackForwardListItem> > ItemsMap;

class EwkBackForwardList {
public:
    static PassOwnPtr<EwkBackForwardList> create(WKBackForwardListRef listRef)
    {
        return adoptPtr(new EwkBackForwardList(listRef));
    }

    EwkBackForwardListItem* previousItem() const;
    EwkBackForwardListItem* currentItem() const;
    EwkBackForwardListItem* nextItem() const;
    EwkBackForwardListItem* itemAt(int index) const;

    WKRetainPtr<WKArrayRef> backList(int limit = -1) const;
    WKRetainPtr<WKArrayRef> forwardList(int limit = -1) const;
    unsigned size() const;

    void update(WKBackForwardListItemRef wkAddedItem, WKArrayRef wkRemovedItems);
    Eina_List* createEinaList(WKArrayRef wkList) const;

private:
    explicit EwkBackForwardList(WKBackForwardListRef listRef);

    EwkBackForwardListItem* getFromCacheOrCreate(WKBackForwardListItemRef wkItem) const;

    WKRetainPtr<WKBackForwardListRef> m_wkList;
    mutable ItemsMap m_wrapperCache;
};

#endif // ewk_back_forward_list_private_h
