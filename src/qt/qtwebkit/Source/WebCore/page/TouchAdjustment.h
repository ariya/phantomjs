/*
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef TouchAdjustment_h
#define TouchAdjustment_h

#include "IntPoint.h"
#include "IntRect.h"
#include "Node.h"
#include "NodeList.h"
#include <wtf/Vector.h>

namespace WebCore {

typedef ListHashSet<RefPtr<Node> > NodeListHashSet;

bool findBestClickableCandidate(Node*& targetNode, IntPoint& targetPoint, const IntPoint& touchHotspot, const IntRect& touchArea, const NodeListHashSet&);
bool findBestContextMenuCandidate(Node*& targetNode, IntPoint& targetPoint, const IntPoint& touchHotspot, const IntRect& touchArea, const NodeListHashSet&);
bool findBestZoomableArea(Node*& targetNode, IntRect& targetArea, const IntPoint& touchHotspot, const IntRect& touchArea, const NodeListHashSet&);
// FIXME: Implement the similar functions for other gestures here as well.

} // namespace WebCore

#endif
