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

#include "config.h"
#include "WKPopupItem.h"

#include "WKAPICast.h"
#include "WebPopupItemEfl.h"

using namespace WebKit;
using namespace WebCore;

WKPopupItemType WKPopupItemGetType(WKPopupItemRef itemRef)
{
    switch (toImpl(itemRef)->itemType()) {
    case WebPopupItem::Separator:
        return kWKPopupItemTypeSeparator;
    case WebPopupItem::Item:
        return kWKPopupItemTypeItem;
    default:
        ASSERT_NOT_REACHED();
        return kWKPopupItemTypeItem;
    }
}

WKPopupItemTextDirection WKPopupItemGetTextDirection(WKPopupItemRef itemRef)
{
    switch (toImpl(itemRef)->textDirection()) {
    case RTL:
        return kWKPopupItemTextDirectionRTL;
    case LTR:
        return kWKPopupItemTextDirectionLTR;
    default:
        ASSERT_NOT_REACHED();
        return kWKPopupItemTextDirectionLTR;
    }
}

bool WKPopupItemHasTextDirectionOverride(WKPopupItemRef itemRef)
{
    return toImpl(itemRef)->hasTextDirectionOverride();
}

WKStringRef WKPopupItemCopyText(WKPopupItemRef itemRef)
{
    return toCopiedAPI(toImpl(itemRef)->text());
}

WKStringRef WKPopupItemCopyToolTipText(WKPopupItemRef itemRef)
{
    return toCopiedAPI(toImpl(itemRef)->toolTipText());
}

WKStringRef WKPopupItemCopyAccessibilityText(WKPopupItemRef itemRef)
{
    return toCopiedAPI(toImpl(itemRef)->accessibilityText());
}

bool WKPopupItemIsEnabled(WKPopupItemRef itemRef)
{
    return toImpl(itemRef)->isEnabled();
}

bool WKPopupItemIsLabel(WKPopupItemRef itemRef)
{
    return toImpl(itemRef)->isLabel();
}

bool WKPopupItemIsSelected(WKPopupItemRef itemRef)
{
    return toImpl(itemRef)->isSelected();
}
