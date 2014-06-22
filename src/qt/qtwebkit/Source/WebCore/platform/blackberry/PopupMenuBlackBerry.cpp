/*
 * Copyright (C) 2010 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "PopupMenuBlackBerry.h"

#include "PopupMenuClient.h"

namespace WebCore {

PopupMenuBlackBerry::PopupMenuBlackBerry(PopupMenuClient* client)
    : m_popupClient(client)
{
}

PopupMenuBlackBerry::~PopupMenuBlackBerry()
{
}

void PopupMenuBlackBerry::show(const IntRect&, FrameView*, int)
{
}

void PopupMenuBlackBerry::hide()
{
    m_popupClient->popupDidHide();
}

void PopupMenuBlackBerry::updateFromElement()
{
}

void PopupMenuBlackBerry::disconnectClient()
{
    m_popupClient = 0;
}

} // namespace WebCore
