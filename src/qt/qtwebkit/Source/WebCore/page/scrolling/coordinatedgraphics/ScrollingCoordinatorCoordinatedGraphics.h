/*
 * Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies).
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ScrollingCoordinatorCoordinatedGraphics_h
#define ScrollingCoordinatorCoordinatedGraphics_h

#if USE(COORDINATED_GRAPHICS)

#include "ScrollingCoordinator.h"

namespace WebCore {

class ScrollingStateTree;

class ScrollingCoordinatorCoordinatedGraphics : public ScrollingCoordinator {
public:
    explicit ScrollingCoordinatorCoordinatedGraphics(Page*);
    virtual ~ScrollingCoordinatorCoordinatedGraphics();

    virtual bool supportsFixedPositionLayers() const OVERRIDE { return true; }

    virtual ScrollingNodeID attachToStateTree(ScrollingNodeType, ScrollingNodeID newNodeID, ScrollingNodeID parentID) OVERRIDE;
    virtual void detachFromStateTree(ScrollingNodeID) OVERRIDE;
    virtual void clearStateTree() OVERRIDE;

    virtual void updateViewportConstrainedNode(ScrollingNodeID, const ViewportConstraints&, GraphicsLayer*) OVERRIDE;

    virtual void scrollableAreaScrollLayerDidChange(ScrollableArea*) OVERRIDE;
    virtual void willDestroyScrollableArea(ScrollableArea*) OVERRIDE;

private:
    OwnPtr<ScrollingStateTree> m_scrollingStateTree;
};

} // namespace WebCore

#endif // USE(COORDINATED_GRAPHICS)

#endif // ScrollingCoordinatorCoordinatedGraphics_h
