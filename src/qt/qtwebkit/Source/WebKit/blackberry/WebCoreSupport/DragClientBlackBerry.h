/*
 * Copyright (C) 2009 Torch Mobile Inc. http://www.torchmobile.com/
 * Copyright (C) 2011 Research In Motion Limited. All rights reserved.
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

#ifndef DragClientBlackBerry_h
#define DragClientBlackBerry_h

#include "DragClient.h"

namespace WebCore {

class DragClientBlackBerry : public DragClient {
public:
    virtual void willPerformDragDestinationAction(DragDestinationAction, DragData*);
    virtual void willPerformDragSourceAction(DragSourceAction, const IntPoint&, Clipboard*);
    virtual DragDestinationAction actionMaskForDrag(DragData*);
    virtual DragSourceAction dragSourceActionMaskForPoint(const IntPoint&);
    virtual void startDrag(void* dragImage, const IntPoint& dragImageOrigin, const IntPoint& eventPos, Clipboard*, Frame*, bool linkDrag = false);
    virtual void dragControllerDestroyed();
};

} // WebCore

#endif // DragClientBlackBerry_h
