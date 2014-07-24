/*
    Copyright (C) 2009-2010 ProFUSION embedded systems
    Copyright (C) 2009-2012 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef ewk_history_private_h
#define ewk_history_private_h

#include "BackForwardListImpl.h"

namespace WebCore {
class HistoryItem;
class BackForwardListImpl;
}

Ewk_History_Item *ewk_history_item_new_from_core(WebCore::HistoryItem *core);
Ewk_History* ewk_history_new(WebCore::BackForwardListImpl* history);
void ewk_history_free(Ewk_History* history);

namespace EWKPrivate {
WebCore::HistoryItem *coreHistoryItem(const Ewk_History_Item *ewkHistoryItem);
} // namespace EWKPrivate

#endif // ewk_history_private_h
