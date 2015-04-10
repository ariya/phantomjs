/*
 * Copyright (C) 2006 Oliver Hunt <ojh16@student.canterbury.ac.nz>
 * Copyright (C) 2006 Apple Computer Inc.
 * Copyright (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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

#include "config.h"
#include "SVGInlineFlowBox.h"

#if ENABLE(SVG)
#include "DocumentMarkerController.h"
#include "GraphicsContext.h"
#include "RenderSVGInlineText.h"
#include "RenderedDocumentMarker.h"
#include "SVGInlineTextBox.h"
#include "SVGRenderingContext.h"

using namespace std;

namespace WebCore {

void SVGInlineFlowBox::paintSelectionBackground(PaintInfo& paintInfo)
{
    ASSERT(paintInfo.phase == PaintPhaseForeground || paintInfo.phase == PaintPhaseSelection);
    ASSERT(!paintInfo.context->paintingDisabled());

    PaintInfo childPaintInfo(paintInfo);
    for (InlineBox* child = firstChild(); child; child = child->nextOnLine()) {
        if (child->isSVGInlineTextBox())
            toSVGInlineTextBox(child)->paintSelectionBackground(childPaintInfo);
        else if (child->isSVGInlineFlowBox())
            toSVGInlineFlowBox(child)->paintSelectionBackground(childPaintInfo);
    }
}

void SVGInlineFlowBox::paint(PaintInfo& paintInfo, const LayoutPoint&, LayoutUnit, LayoutUnit)
{
    ASSERT(paintInfo.phase == PaintPhaseForeground || paintInfo.phase == PaintPhaseSelection);
    ASSERT(!paintInfo.context->paintingDisabled());

    RenderObject* boxRenderer = renderer();
    ASSERT(boxRenderer);

    SVGRenderingContext renderingContext(boxRenderer, paintInfo, SVGRenderingContext::SaveGraphicsContext);
    if (renderingContext.isRenderingPrepared()) {
        for (InlineBox* child = firstChild(); child; child = child->nextOnLine()) {
            if (child->isSVGInlineTextBox())
                computeTextMatchMarkerRectForRenderer(toRenderSVGInlineText(toSVGInlineTextBox(child)->textRenderer()));

            child->paint(paintInfo, LayoutPoint(), 0, 0);
        }
    }
}

FloatRect SVGInlineFlowBox::calculateBoundaries() const
{
    FloatRect childRect;
    for (InlineBox* child = firstChild(); child; child = child->nextOnLine()) {
        if (!child->isSVGInlineTextBox() && !child->isSVGInlineFlowBox())
            continue;
        childRect.unite(child->calculateBoundaries());
    }
    return childRect;
}

void SVGInlineFlowBox::computeTextMatchMarkerRectForRenderer(RenderSVGInlineText* textRenderer)
{
    ASSERT(textRenderer);

    Node* node = textRenderer->node();
    if (!node || !node->inDocument())
        return;

    RenderStyle* style = textRenderer->style();
    ASSERT(style);

    AffineTransform fragmentTransform;
    Document* document = textRenderer->document();
    Vector<DocumentMarker*> markers = document->markers()->markersFor(textRenderer->node());

    Vector<DocumentMarker*>::iterator markerEnd = markers.end();
    for (Vector<DocumentMarker*>::iterator markerIt = markers.begin(); markerIt != markerEnd; ++markerIt) {
        DocumentMarker* marker = *markerIt;

        // SVG is only interessted in the TextMatch marker, for now.
        if (marker->type() != DocumentMarker::TextMatch)
            continue;

        FloatRect markerRect;
        for (InlineTextBox* box = textRenderer->firstTextBox(); box; box = box->nextTextBox()) {
            if (!box->isSVGInlineTextBox())
                continue;

            SVGInlineTextBox* textBox = toSVGInlineTextBox(box);

            int markerStartPosition = max<int>(marker->startOffset() - textBox->start(), 0);
            int markerEndPosition = min<int>(marker->endOffset() - textBox->start(), textBox->len());

            if (markerStartPosition >= markerEndPosition)
                continue;

            int fragmentStartPosition = 0;
            int fragmentEndPosition = 0;

            const Vector<SVGTextFragment>& fragments = textBox->textFragments();
            unsigned textFragmentsSize = fragments.size();
            for (unsigned i = 0; i < textFragmentsSize; ++i) {
                const SVGTextFragment& fragment = fragments.at(i);

                fragmentStartPosition = markerStartPosition;
                fragmentEndPosition = markerEndPosition;
                if (!textBox->mapStartEndPositionsIntoFragmentCoordinates(fragment, fragmentStartPosition, fragmentEndPosition))
                    continue;

                FloatRect fragmentRect = textBox->selectionRectForTextFragment(fragment, fragmentStartPosition, fragmentEndPosition, style);
                fragment.buildFragmentTransform(fragmentTransform);
                if (!fragmentTransform.isIdentity())
                    fragmentRect = fragmentTransform.mapRect(fragmentRect);

                markerRect.unite(fragmentRect);
            }
        }

        toRenderedDocumentMarker(marker)->setRenderedRect(textRenderer->localToAbsoluteQuad(markerRect).enclosingBoundingBox());
    }
}

} // namespace WebCore

#endif // ENABLE(SVG)
