/*
 * Copyright (C) 2006 Alexander Kellett <lypanov@kde.org>
 * Copyright (C) 2006 Apple Computer, Inc.
 * Copyright (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2007, 2008, 2009 Rob Buis <buis@kde.org>
 * Copyright (C) 2009 Google, Inc.
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2010 Patrick Gansterer <paroga@paroga.com>
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

#if ENABLE(SVG)
#include "RenderSVGImage.h"

#include "Attr.h"
#include "FloatConversion.h"
#include "FloatQuad.h"
#include "GraphicsContext.h"
#include "PointerEventsHitRules.h"
#include "RenderImageResource.h"
#include "RenderLayer.h"
#include "RenderSVGResourceContainer.h"
#include "RenderSVGResourceFilter.h"
#include "SVGImageElement.h"
#include "SVGLength.h"
#include "SVGPreserveAspectRatio.h"
#include "SVGRenderSupport.h"
#include "SVGResources.h"

namespace WebCore {

RenderSVGImage::RenderSVGImage(SVGImageElement* impl)
    : RenderSVGModelObject(impl)
    , m_updateCachedRepaintRect(true)
    , m_needsTransformUpdate(true)
    , m_imageResource(RenderImageResource::create())
{
    m_imageResource->initialize(this);
}

RenderSVGImage::~RenderSVGImage()
{
    m_imageResource->shutdown();
}

void RenderSVGImage::layout()
{
    ASSERT(needsLayout());

    LayoutRepainter repainter(*this, checkForRepaintDuringLayout() && selfNeedsLayout());
    SVGImageElement* image = static_cast<SVGImageElement*>(node());

    bool transformOrBoundariesUpdate = m_needsTransformUpdate || m_updateCachedRepaintRect;
    if (m_needsTransformUpdate) {
        m_localTransform = image->animatedLocalTransform();
        m_needsTransformUpdate = false;
    }

    if (m_updateCachedRepaintRect) {
        m_repaintBoundingBox = m_objectBoundingBox;
        SVGRenderSupport::intersectRepaintRectWithResources(this, m_repaintBoundingBox);
        m_updateCachedRepaintRect = false;
    }

    // Invalidate all resources of this client if our layout changed.
    if (m_everHadLayout && selfNeedsLayout())
        SVGResourcesCache::clientLayoutChanged(this);

    // If our bounds changed, notify the parents.
    if (transformOrBoundariesUpdate)
        RenderSVGModelObject::setNeedsBoundariesUpdate();

    repainter.repaintAfterLayout();
    setNeedsLayout(false);
}

void RenderSVGImage::updateFromElement()
{
    SVGImageElement* image = static_cast<SVGImageElement*>(node());

    FloatRect oldBoundaries = m_objectBoundingBox;
    m_objectBoundingBox = FloatRect(image->x().value(image), image->y().value(image), image->width().value(image), image->height().value(image));
    if (m_objectBoundingBox != oldBoundaries) {
        m_updateCachedRepaintRect = true;
        setNeedsLayout(true);
    }
    RenderSVGModelObject::updateFromElement();
}

void RenderSVGImage::paint(PaintInfo& paintInfo, int, int)
{
    if (paintInfo.context->paintingDisabled() || style()->visibility() == HIDDEN || !m_imageResource->hasImage())
        return;

    FloatRect boundingBox = repaintRectInLocalCoordinates();
    if (!SVGRenderSupport::paintInfoIntersectsRepaintRect(boundingBox, m_localTransform, paintInfo))
        return;

    PaintInfo childPaintInfo(paintInfo);
    bool drawsOutline = style()->outlineWidth() && (childPaintInfo.phase == PaintPhaseOutline || childPaintInfo.phase == PaintPhaseSelfOutline);
    if (drawsOutline || childPaintInfo.phase == PaintPhaseForeground) {
        GraphicsContextStateSaver stateSaver(*childPaintInfo.context);
        childPaintInfo.applyTransform(m_localTransform);

        if (childPaintInfo.phase == PaintPhaseForeground) {
            PaintInfo savedInfo(childPaintInfo);

            if (SVGRenderSupport::prepareToRenderSVGContent(this, childPaintInfo)) {
                RefPtr<Image> image = m_imageResource->image();
                FloatRect destRect = m_objectBoundingBox;
                FloatRect srcRect(0, 0, image->width(), image->height());

                SVGImageElement* imageElement = static_cast<SVGImageElement*>(node());
                if (imageElement->preserveAspectRatio().align() != SVGPreserveAspectRatio::SVG_PRESERVEASPECTRATIO_NONE)
                    imageElement->preserveAspectRatio().transformRect(destRect, srcRect);

                childPaintInfo.context->drawImage(image.get(), ColorSpaceDeviceRGB, destRect, srcRect);
            }

            SVGRenderSupport::finishRenderSVGContent(this, childPaintInfo, savedInfo.context);
        }

        if (drawsOutline)
            paintOutline(childPaintInfo.context, static_cast<int>(boundingBox.x()), static_cast<int>(boundingBox.y()),
                static_cast<int>(boundingBox.width()), static_cast<int>(boundingBox.height()));
    }
}

bool RenderSVGImage::nodeAtFloatPoint(const HitTestRequest& request, HitTestResult& result, const FloatPoint& pointInParent, HitTestAction hitTestAction)
{
    // We only draw in the forground phase, so we only hit-test then.
    if (hitTestAction != HitTestForeground)
        return false;

    PointerEventsHitRules hitRules(PointerEventsHitRules::SVG_IMAGE_HITTESTING, request, style()->pointerEvents());
    bool isVisible = (style()->visibility() == VISIBLE);
    if (isVisible || !hitRules.requireVisible) {
        FloatPoint localPoint = localToParentTransform().inverse().mapPoint(pointInParent);
            
        if (!SVGRenderSupport::pointInClippingArea(this, localPoint))
            return false;

        if (hitRules.canHitFill) {
            if (m_objectBoundingBox.contains(localPoint)) {
                updateHitTestResult(result, roundedIntPoint(localPoint));
                return true;
            }
        }
    }

    return false;
}

void RenderSVGImage::imageChanged(WrappedImagePtr, const IntRect*)
{
    // The image resource defaults to nullImage until the resource arrives.
    // This empty image may be cached by SVG resources which must be invalidated.
    if (SVGResources* resources = SVGResourcesCache::cachedResourcesForRenderObject(this))
        resources->removeClientFromCache(this);

    // Eventually notify parent resources, that we've changed.
    RenderSVGResource::markForLayoutAndParentResourceInvalidation(this, false);

    repaint();
}

void RenderSVGImage::addFocusRingRects(Vector<IntRect>& rects, int, int)
{
    // this is called from paint() after the localTransform has already been applied
    IntRect contentRect = enclosingIntRect(repaintRectInLocalCoordinates());
    if (!contentRect.isEmpty())
        rects.append(contentRect);
}

} // namespace WebCore

#endif // ENABLE(SVG)
