/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this item of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this item of conditions and the following disclaimer in the
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

#ifndef ewk_back_forward_list_item_private_h
#define ewk_back_forward_list_item_private_h

#include "WKEinaSharedString.h"
#include "ewk_object_private.h"
#include <WebKit2/WKBase.h>
#include <wtf/PassRefPtr.h>

/**
 * \struct  Ewk_Back_Forward_List
 * @brief   Contains the Back Forward List data.
 */
class EwkBackForwardListItem : public EwkObject {
public:
    EWK_OBJECT_DECLARE(EwkBackForwardListItem)

    static PassRefPtr<EwkBackForwardListItem> create(WKBackForwardListItemRef itemRef)
    {
        return adoptRef(new EwkBackForwardListItem(itemRef));
    }

    const char* url() const;
    const char* title() const;
    const char* originalURL() const;

    WKBackForwardListItemRef wkItem() const { return m_wkItem.get(); }

private:
    explicit EwkBackForwardListItem(WKBackForwardListItemRef itemRef);

    WKRetainPtr<WKBackForwardListItemRef> m_wkItem;
    mutable WKEinaSharedString m_url;
    mutable WKEinaSharedString m_title;
    mutable WKEinaSharedString m_originalURL;
};

#endif // ewk_back_forward_list_private_h
