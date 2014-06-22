/*
 * Copyright (C) 2012 Samsung Electronics
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ewk_popup_menu_item_private_h
#define ewk_popup_menu_item_private_h

#include "WKEinaSharedString.h"
#include "WKRetainPtr.h"
#include "ewk_popup_menu_item.h"
#include <WebKit2/WKBase.h>
#include <wtf/PassOwnPtr.h>

/**
 * \struct  Ewk_Popup_Menu_Item
 * @brief   Contains the popup menu data.
 */
class EwkPopupMenuItem {
public:
    static PassOwnPtr<EwkPopupMenuItem> create(WKPopupItemRef item)
    {
        return adoptPtr(new EwkPopupMenuItem(item));
    }

    Ewk_Popup_Menu_Item_Type type() const;
    Ewk_Text_Direction textDirection() const;

    bool hasTextDirectionOverride() const;
    bool isEnabled() const;
    bool isLabel() const;
    bool isSelected() const;

    const char* text() const;
    const char* tooltipText() const;
    const char* accessibilityText() const;

private:
    explicit EwkPopupMenuItem(WKPopupItemRef item);

    WKRetainPtr<WKPopupItemRef> m_wkItem;

    // Lazily initialized.
    mutable WKEinaSharedString m_text;
    mutable WKEinaSharedString m_tooltipText;
    mutable WKEinaSharedString m_accessibilityText;
};

#endif // ewk_popup_menu_item_private_h
