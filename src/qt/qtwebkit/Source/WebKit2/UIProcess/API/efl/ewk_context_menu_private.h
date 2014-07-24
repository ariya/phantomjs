/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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

#ifndef ewk_context_menu_private_h
#define ewk_context_menu_private_h

#include "ewk_context_menu_item.h"
#include "ewk_object_private.h"
#include <Eina.h>
#include <wtf/PassRefPtr.h>

class EwkView;

class EwkContextMenu : public EwkObject {
public:
    EWK_OBJECT_DECLARE(EwkContextMenu)

    static PassRefPtr<EwkContextMenu> create(EwkView* viewImpl, WKArrayRef items)
    {
        return adoptRef(new EwkContextMenu(viewImpl, items));
    }

    static PassRefPtr<EwkContextMenu> create()
    {
        return adoptRef(new EwkContextMenu());
    }

    static PassRefPtr<EwkContextMenu> create(Eina_List* items)
    {
        return adoptRef(new EwkContextMenu(items));
    }

    ~EwkContextMenu();

    void hide();
    void appendItem(EwkContextMenuItem*);
    void removeItem(EwkContextMenuItem*);

    const Eina_List* items() const { return m_contextMenuItems; }
    bool contextMenuItemSelected(WKContextMenuItemRef item);

    EwkView* ewkView() const { return m_viewImpl; }
    void setEwkView(EwkView* ewkView) { m_viewImpl = ewkView; }

private:
    EwkContextMenu();
    EwkContextMenu(Eina_List* items);
    EwkContextMenu(EwkView* viewImpl, WKArrayRef items);

    EwkView* m_viewImpl;
    Eina_List* m_contextMenuItems;
};

#endif // ewk_context_menu_private_h

