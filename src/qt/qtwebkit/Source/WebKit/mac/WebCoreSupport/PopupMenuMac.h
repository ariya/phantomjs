/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef PopupMenuMac_h
#define PopupMenuMac_h

#include <WebCore/PopupMenu.h>
#include <wtf/RetainPtr.h>

@class NSPopUpButtonCell;

namespace WebCore {
class PopupMenuClient;
}

class PopupMenuMac : public WebCore::PopupMenu {
public:
    PopupMenuMac(WebCore::PopupMenuClient*);
    ~PopupMenuMac();

    virtual void show(const WebCore::IntRect&, WebCore::FrameView*, int index) override;
    virtual void hide() override;
    virtual void updateFromElement() override;
    virtual void disconnectClient() override;

private:
    void clear();
    void populate();

    WebCore::PopupMenuClient* m_client;
    RetainPtr<NSPopUpButtonCell> m_popup;
};

#endif // PopupMenuMac_h
