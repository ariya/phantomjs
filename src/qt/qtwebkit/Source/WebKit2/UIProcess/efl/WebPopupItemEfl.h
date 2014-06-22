/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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
 * THE IMPLIED WARRANTIES OF MERCHANTAwBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebPopupItemEfl_h
#define WebPopupItemEfl_h

#include "APIObject.h"
#include "WebPopupItem.h"
#include <wtf/PassRefPtr.h>

namespace WebKit {

class WebPopupItemEfl : public TypedAPIObject<APIObject::TypePopupMenuItem> {
public:
    static PassRefPtr<WebPopupItemEfl> create(const WebPopupItem& data)
    {
        return adoptRef(new WebPopupItemEfl(data));
    }

    virtual ~WebPopupItemEfl();
    const WebPopupItem& data() const { return m_data; }

    WebPopupItem::Type itemType() const { return m_data.m_type; }
    String text() const { return m_data.m_text; }
    WebCore::TextDirection textDirection() const { return m_data.m_textDirection; }
    bool hasTextDirectionOverride() const { return m_data.m_hasTextDirectionOverride; }
    String toolTipText() const { return m_data.m_toolTip; }
    String accessibilityText() const { return m_data.m_accessibilityText; }
    bool isEnabled() const { return m_data.m_isEnabled; }
    bool isLabel() const { return m_data.m_isLabel; }
    bool isSelected() const { return m_data.m_isSelected; }

private:
    explicit WebPopupItemEfl(const WebPopupItem&);

    WebPopupItem m_data;
};

} // namespace WebKit

#endif // WebPopupItemEfl_h
