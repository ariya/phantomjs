/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "RenderReplica.h"

#include "RenderLayer.h"
#include <wtf/StackStats.h>

namespace WebCore {

RenderReplica::RenderReplica()
    : RenderBox(0)
{
    // This is a hack. Replicas are synthetic, and don't pick up the attributes of the
    // renderers being replicated, so they always report that they are inline, non-replaced.
    // However, we need transforms to be applied to replicas for reflections, so have to pass
    // the if (!isInline() || isReplaced()) check before setHasTransform().
    setReplaced(true);
}

RenderReplica* RenderReplica::createAnonymous(Document* document)
{
    RenderReplica* renderer = new (document->renderArena()) RenderReplica();
    renderer->setDocumentForAnonymous(document);
    return renderer;
}

RenderReplica::~RenderReplica()
{
}
    
void RenderReplica::layout()
{
    StackStats::LayoutCheckPoint layoutCheckPoint;
    setFrameRect(parentBox()->borderBoxRect());
    updateLayerTransform();
    setNeedsLayout(false);
}

void RenderReplica::computePreferredLogicalWidths()
{
    m_minPreferredLogicalWidth = parentBox()->width();
    m_maxPreferredLogicalWidth = m_minPreferredLogicalWidth;
    setPreferredLogicalWidthsDirty(false);
}

void RenderReplica::paint(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    if (paintInfo.phase != PaintPhaseForeground && paintInfo.phase != PaintPhaseMask)
        return;
 
    LayoutPoint adjustedPaintOffset = paintOffset + location();

    if (paintInfo.phase == PaintPhaseForeground) {
        // Turn around and paint the parent layer. Use temporary clipRects, so that the layer doesn't end up caching clip rects
        // computing using the wrong rootLayer
        RenderLayer* rootPaintingLayer = layer()->transform() ? layer()->parent() : layer()->enclosingTransformedAncestor();
        RenderLayer::LayerPaintingInfo paintingInfo(rootPaintingLayer, paintInfo.rect, PaintBehaviorNormal, LayoutSize(), 0, paintInfo.renderRegion);
        RenderLayer::PaintLayerFlags flags = RenderLayer::PaintLayerHaveTransparency | RenderLayer::PaintLayerAppliedTransform | RenderLayer::PaintLayerTemporaryClipRects | RenderLayer::PaintLayerPaintingReflection;
        layer()->parent()->paintLayer(paintInfo.context, paintingInfo, flags);
    } else if (paintInfo.phase == PaintPhaseMask)
        paintMask(paintInfo, adjustedPaintOffset);
}

} // namespace WebCore
