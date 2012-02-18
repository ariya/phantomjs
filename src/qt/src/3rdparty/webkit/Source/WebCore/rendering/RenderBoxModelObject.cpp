/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2005 Allan Sandfeld Jensen (kde@carewolf.com)
 *           (C) 2005, 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
 *
 */

#include "config.h"
#include "RenderBoxModelObject.h"

#include "GraphicsContext.h"
#include "HTMLFrameOwnerElement.h"
#include "HTMLNames.h"
#include "ImageBuffer.h"
#include "Path.h"
#include "RenderBlock.h"
#include "RenderInline.h"
#include "RenderLayer.h"
#include "RenderView.h"
#include <wtf/CurrentTime.h>

using namespace std;

namespace WebCore {

using namespace HTMLNames;

bool RenderBoxModelObject::s_wasFloating = false;
bool RenderBoxModelObject::s_hadLayer = false;
bool RenderBoxModelObject::s_layerWasSelfPainting = false;

static const double cInterpolationCutoff = 800. * 800.;
static const double cLowQualityTimeThreshold = 0.500; // 500 ms

typedef HashMap<const void*, IntSize> LayerSizeMap;
typedef HashMap<RenderBoxModelObject*, LayerSizeMap> ObjectLayerSizeMap;

// The HashMap for storing continuation pointers.
// An inline can be split with blocks occuring in between the inline content.
// When this occurs we need a pointer to the next object. We can basically be
// split into a sequence of inlines and blocks. The continuation will either be
// an anonymous block (that houses other blocks) or it will be an inline flow.
// <b><i><p>Hello</p></i></b>. In this example the <i> will have a block as
// its continuation but the <b> will just have an inline as its continuation.
typedef HashMap<const RenderBoxModelObject*, RenderBoxModelObject*> ContinuationMap;
static ContinuationMap* continuationMap = 0;

class ImageQualityController {
    WTF_MAKE_NONCOPYABLE(ImageQualityController); WTF_MAKE_FAST_ALLOCATED;
public:
    ImageQualityController();
    bool shouldPaintAtLowQuality(GraphicsContext*, RenderBoxModelObject*, Image*, const void* layer, const IntSize&);
    void removeLayer(RenderBoxModelObject*, LayerSizeMap* innerMap, const void* layer);
    void set(RenderBoxModelObject*, LayerSizeMap* innerMap, const void* layer, const IntSize&);
    void objectDestroyed(RenderBoxModelObject*);
    bool isEmpty() { return m_objectLayerSizeMap.isEmpty(); }

private:
    void highQualityRepaintTimerFired(Timer<ImageQualityController>*);
    void restartTimer();

    ObjectLayerSizeMap m_objectLayerSizeMap;
    Timer<ImageQualityController> m_timer;
    bool m_animatedResizeIsActive;
};

ImageQualityController::ImageQualityController()
    : m_timer(this, &ImageQualityController::highQualityRepaintTimerFired)
    , m_animatedResizeIsActive(false)
{
}

void ImageQualityController::removeLayer(RenderBoxModelObject* object, LayerSizeMap* innerMap, const void* layer)
{
    if (innerMap) {
        innerMap->remove(layer);
        if (innerMap->isEmpty())
            objectDestroyed(object);
    }
}
    
void ImageQualityController::set(RenderBoxModelObject* object, LayerSizeMap* innerMap, const void* layer, const IntSize& size)
{
    if (innerMap)
        innerMap->set(layer, size);
    else {
        LayerSizeMap newInnerMap;
        newInnerMap.set(layer, size);
        m_objectLayerSizeMap.set(object, newInnerMap);
    }
}
    
void ImageQualityController::objectDestroyed(RenderBoxModelObject* object)
{
    m_objectLayerSizeMap.remove(object);
    if (m_objectLayerSizeMap.isEmpty()) {
        m_animatedResizeIsActive = false;
        m_timer.stop();
    }
}

void ImageQualityController::highQualityRepaintTimerFired(Timer<ImageQualityController>*)
{
    if (m_animatedResizeIsActive) {
        m_animatedResizeIsActive = false;
        for (ObjectLayerSizeMap::iterator it = m_objectLayerSizeMap.begin(); it != m_objectLayerSizeMap.end(); ++it)
            it->first->repaint();
    }
}

void ImageQualityController::restartTimer()
{
    m_timer.startOneShot(cLowQualityTimeThreshold);
}

bool ImageQualityController::shouldPaintAtLowQuality(GraphicsContext* context, RenderBoxModelObject* object, Image* image, const void *layer, const IntSize& size)
{
    // If the image is not a bitmap image, then none of this is relevant and we just paint at high
    // quality.
    if (!image || !image->isBitmapImage() || context->paintingDisabled())
        return false;

    // Make sure to use the unzoomed image size, since if a full page zoom is in effect, the image
    // is actually being scaled.
    IntSize imageSize(image->width(), image->height());

    // Look ourselves up in the hashtables.
    ObjectLayerSizeMap::iterator i = m_objectLayerSizeMap.find(object);
    LayerSizeMap* innerMap = i != m_objectLayerSizeMap.end() ? &i->second : 0;
    IntSize oldSize;
    bool isFirstResize = true;
    if (innerMap) {
        LayerSizeMap::iterator j = innerMap->find(layer);
        if (j != innerMap->end()) {
            isFirstResize = false;
            oldSize = j->second;
        }
    }

    const AffineTransform& currentTransform = context->getCTM();
    bool contextIsScaled = !currentTransform.isIdentityOrTranslationOrFlipped();
    if (!contextIsScaled && imageSize == size) {
        // There is no scale in effect. If we had a scale in effect before, we can just remove this object from the list.
        removeLayer(object, innerMap, layer);
        return false;
    }

    // There is no need to hash scaled images that always use low quality mode when the page demands it. This is the iChat case.
    if (object->document()->page()->inLowQualityImageInterpolationMode()) {
        double totalPixels = static_cast<double>(image->width()) * static_cast<double>(image->height());
        if (totalPixels > cInterpolationCutoff)
            return true;
    }

    // If an animated resize is active, paint in low quality and kick the timer ahead.
    if (m_animatedResizeIsActive) {
        set(object, innerMap, layer, size);
        restartTimer();
        return true;
    }
    // If this is the first time resizing this image, or its size is the
    // same as the last resize, draw at high res, but record the paint
    // size and set the timer.
    if (isFirstResize || oldSize == size) {
        restartTimer();
        set(object, innerMap, layer, size);
        return false;
    }
    // If the timer is no longer active, draw at high quality and don't
    // set the timer.
    if (!m_timer.isActive()) {
        removeLayer(object, innerMap, layer);
        return false;
    }
    // This object has been resized to two different sizes while the timer
    // is active, so draw at low quality, set the flag for animated resizes and
    // the object to the list for high quality redraw.
    set(object, innerMap, layer, size);
    m_animatedResizeIsActive = true;
    restartTimer();
    return true;
}

static ImageQualityController* gImageQualityController = 0;

static ImageQualityController* imageQualityController()
{
    if (!gImageQualityController)
        gImageQualityController = new ImageQualityController;

    return gImageQualityController;
}

void RenderBoxModelObject::setSelectionState(SelectionState s)
{
    if (selectionState() == s)
        return;
    
    if (s == SelectionInside && selectionState() != SelectionNone)
        return;

    if ((s == SelectionStart && selectionState() == SelectionEnd)
        || (s == SelectionEnd && selectionState() == SelectionStart))
        RenderObject::setSelectionState(SelectionBoth);
    else
        RenderObject::setSelectionState(s);
    
    // FIXME:
    // We should consider whether it is OK propagating to ancestor RenderInlines.
    // This is a workaround for http://webkit.org/b/32123
    RenderBlock* cb = containingBlock();
    if (cb && !cb->isRenderView())
        cb->setSelectionState(s);
}

bool RenderBoxModelObject::shouldPaintAtLowQuality(GraphicsContext* context, Image* image, const void* layer, const IntSize& size)
{
    return imageQualityController()->shouldPaintAtLowQuality(context, this, image, layer, size);
}

RenderBoxModelObject::RenderBoxModelObject(Node* node)
    : RenderObject(node)
    , m_layer(0)
{
}

RenderBoxModelObject::~RenderBoxModelObject()
{
    // Our layer should have been destroyed and cleared by now
    ASSERT(!hasLayer());
    ASSERT(!m_layer);
    if (gImageQualityController) {
        gImageQualityController->objectDestroyed(this);
        if (gImageQualityController->isEmpty()) {
            delete gImageQualityController;
            gImageQualityController = 0;
        }
    }
}

void RenderBoxModelObject::destroyLayer()
{
    ASSERT(!hasLayer()); // Callers should have already called setHasLayer(false)
    ASSERT(m_layer);
    m_layer->destroy(renderArena());
    m_layer = 0;
}

void RenderBoxModelObject::destroy()
{
    // This must be done before we destroy the RenderObject.
    if (m_layer)
        m_layer->clearClipRects();

    // A continuation of this RenderObject should be destroyed at subclasses.
    ASSERT(!continuation());

    // RenderObject::destroy calls back to destroyLayer() for layer destruction
    RenderObject::destroy();
}

bool RenderBoxModelObject::hasSelfPaintingLayer() const
{
    return m_layer && m_layer->isSelfPaintingLayer();
}

void RenderBoxModelObject::styleWillChange(StyleDifference diff, const RenderStyle* newStyle)
{
    s_wasFloating = isFloating();
    s_hadLayer = hasLayer();
    if (s_hadLayer)
        s_layerWasSelfPainting = layer()->isSelfPaintingLayer();

    // If our z-index changes value or our visibility changes,
    // we need to dirty our stacking context's z-order list.
    if (style() && newStyle) {
        if (parent()) {
            // Do a repaint with the old style first, e.g., for example if we go from
            // having an outline to not having an outline.
            if (diff == StyleDifferenceRepaintLayer) {
                layer()->repaintIncludingDescendants();
                if (!(style()->clip() == newStyle->clip()))
                    layer()->clearClipRectsIncludingDescendants();
            } else if (diff == StyleDifferenceRepaint || newStyle->outlineSize() < style()->outlineSize())
                repaint();
        }
        
        if (diff == StyleDifferenceLayout || diff == StyleDifferenceSimplifiedLayout) {
            // When a layout hint happens, we go ahead and do a repaint of the layer, since the layer could
            // end up being destroyed.
            if (hasLayer()) {
                if (style()->position() != newStyle->position() ||
                    style()->zIndex() != newStyle->zIndex() ||
                    style()->hasAutoZIndex() != newStyle->hasAutoZIndex() ||
                    !(style()->clip() == newStyle->clip()) ||
                    style()->hasClip() != newStyle->hasClip() ||
                    style()->opacity() != newStyle->opacity() ||
                    style()->transform() != newStyle->transform())
                layer()->repaintIncludingDescendants();
            } else if (newStyle->hasTransform() || newStyle->opacity() < 1) {
                // If we don't have a layer yet, but we are going to get one because of transform or opacity,
                //  then we need to repaint the old position of the object.
                repaint();
            }
        }

        if (hasLayer() && (style()->hasAutoZIndex() != newStyle->hasAutoZIndex() ||
                           style()->zIndex() != newStyle->zIndex() ||
                           style()->visibility() != newStyle->visibility())) {
            layer()->dirtyStackingContextZOrderLists();
            if (style()->hasAutoZIndex() != newStyle->hasAutoZIndex() || style()->visibility() != newStyle->visibility())
                layer()->dirtyZOrderLists();
        }
    }

    RenderObject::styleWillChange(diff, newStyle);
}

void RenderBoxModelObject::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderObject::styleDidChange(diff, oldStyle);
    updateBoxModelInfoFromStyle();
    
    if (requiresLayer()) {
        if (!layer()) {
            if (s_wasFloating && isFloating())
                setChildNeedsLayout(true);
            m_layer = new (renderArena()) RenderLayer(this);
            setHasLayer(true);
            m_layer->insertOnlyThisLayer();
            if (parent() && !needsLayout() && containingBlock()) {
                m_layer->setNeedsFullRepaint();
                m_layer->updateLayerPositions();
            }
        }
    } else if (layer() && layer()->parent()) {
        setHasTransform(false); // Either a transform wasn't specified or the object doesn't support transforms, so just null out the bit.
        setHasReflection(false);
        m_layer->removeOnlyThisLayer(); // calls destroyLayer() which clears m_layer
        if (s_wasFloating && isFloating())
            setChildNeedsLayout(true);
    }

    if (layer()) {
        layer()->styleChanged(diff, oldStyle);
        if (s_hadLayer && layer()->isSelfPaintingLayer() != s_layerWasSelfPainting)
            setChildNeedsLayout(true);
    }
}

void RenderBoxModelObject::updateBoxModelInfoFromStyle()
{
    // Set the appropriate bits for a box model object.  Since all bits are cleared in styleWillChange,
    // we only check for bits that could possibly be set to true.
    setHasBoxDecorations(hasBackground() || style()->hasBorder() || style()->hasAppearance() || style()->boxShadow());
    setInline(style()->isDisplayInlineType());
    setRelPositioned(style()->position() == RelativePosition);
    setHorizontalWritingMode(style()->isHorizontalWritingMode());
}

int RenderBoxModelObject::relativePositionOffsetX() const
{
    // Objects that shrink to avoid floats normally use available line width when computing containing block width.  However
    // in the case of relative positioning using percentages, we can't do this.  The offset should always be resolved using the
    // available width of the containing block.  Therefore we don't use containingBlockLogicalWidthForContent() here, but instead explicitly
    // call availableWidth on our containing block.
    if (!style()->left().isAuto()) {
        RenderBlock* cb = containingBlock();
        if (!style()->right().isAuto() && !cb->style()->isLeftToRightDirection())
            return -style()->right().calcValue(cb->availableWidth());
        return style()->left().calcValue(cb->availableWidth());
    }
    if (!style()->right().isAuto()) {
        RenderBlock* cb = containingBlock();
        return -style()->right().calcValue(cb->availableWidth());
    }
    return 0;
}

int RenderBoxModelObject::relativePositionOffsetY() const
{
    RenderBlock* containingBlock = this->containingBlock();

    // If the containing block of a relatively positioned element does not
    // specify a height, a percentage top or bottom offset should be resolved as
    // auto. An exception to this is if the containing block has the WinIE quirk
    // where <html> and <body> assume the size of the viewport. In this case,
    // calculate the percent offset based on this height.
    // See <https://bugs.webkit.org/show_bug.cgi?id=26396>.
    if (!style()->top().isAuto()
        && (!containingBlock->style()->height().isAuto()
            || !style()->top().isPercent()
            || containingBlock->stretchesToViewport()))
        return style()->top().calcValue(containingBlock->availableHeight());

    if (!style()->bottom().isAuto()
        && (!containingBlock->style()->height().isAuto()
            || !style()->bottom().isPercent()
            || containingBlock->stretchesToViewport()))
        return -style()->bottom().calcValue(containingBlock->availableHeight());

    return 0;
}

int RenderBoxModelObject::offsetLeft() const
{
    // If the element is the HTML body element or does not have an associated box
    // return 0 and stop this algorithm.
    if (isBody())
        return 0;
    
    RenderBoxModelObject* offsetPar = offsetParent();
    int xPos = (isBox() ? toRenderBox(this)->x() : 0);
    
    // If the offsetParent of the element is null, or is the HTML body element,
    // return the distance between the canvas origin and the left border edge 
    // of the element and stop this algorithm.
    if (offsetPar) {
        if (offsetPar->isBox() && !offsetPar->isBody())
            xPos -= toRenderBox(offsetPar)->borderLeft();
        if (!isPositioned()) {
            if (isRelPositioned())
                xPos += relativePositionOffsetX();
            RenderObject* curr = parent();
            while (curr && curr != offsetPar) {
                // FIXME: What are we supposed to do inside SVG content?
                if (curr->isBox() && !curr->isTableRow())
                    xPos += toRenderBox(curr)->x();
                curr = curr->parent();
            }
            if (offsetPar->isBox() && offsetPar->isBody() && !offsetPar->isRelPositioned() && !offsetPar->isPositioned())
                xPos += toRenderBox(offsetPar)->x();
        }
    }

    return xPos;
}

int RenderBoxModelObject::offsetTop() const
{
    // If the element is the HTML body element or does not have an associated box
    // return 0 and stop this algorithm.
    if (isBody())
        return 0;
    
    RenderBoxModelObject* offsetPar = offsetParent();
    int yPos = (isBox() ? toRenderBox(this)->y() : 0);
    
    // If the offsetParent of the element is null, or is the HTML body element,
    // return the distance between the canvas origin and the top border edge 
    // of the element and stop this algorithm.
    if (offsetPar) {
        if (offsetPar->isBox() && !offsetPar->isBody())
            yPos -= toRenderBox(offsetPar)->borderTop();
        if (!isPositioned()) {
            if (isRelPositioned())
                yPos += relativePositionOffsetY();
            RenderObject* curr = parent();
            while (curr && curr != offsetPar) {
                // FIXME: What are we supposed to do inside SVG content?
                if (curr->isBox() && !curr->isTableRow())
                    yPos += toRenderBox(curr)->y();
                curr = curr->parent();
            }
            if (offsetPar->isBox() && offsetPar->isBody() && !offsetPar->isRelPositioned() && !offsetPar->isPositioned())
                yPos += toRenderBox(offsetPar)->y();
        }
    }
    return yPos;
}

int RenderBoxModelObject::paddingTop(bool) const
{
    int w = 0;
    Length padding = style()->paddingTop();
    if (padding.isPercent())
        w = containingBlock()->availableLogicalWidth();
    return padding.calcMinValue(w);
}

int RenderBoxModelObject::paddingBottom(bool) const
{
    int w = 0;
    Length padding = style()->paddingBottom();
    if (padding.isPercent())
        w = containingBlock()->availableLogicalWidth();
    return padding.calcMinValue(w);
}

int RenderBoxModelObject::paddingLeft(bool) const
{
    int w = 0;
    Length padding = style()->paddingLeft();
    if (padding.isPercent())
        w = containingBlock()->availableLogicalWidth();
    return padding.calcMinValue(w);
}

int RenderBoxModelObject::paddingRight(bool) const
{
    int w = 0;
    Length padding = style()->paddingRight();
    if (padding.isPercent())
        w = containingBlock()->availableLogicalWidth();
    return padding.calcMinValue(w);
}

int RenderBoxModelObject::paddingBefore(bool) const
{
    int w = 0;
    Length padding = style()->paddingBefore();
    if (padding.isPercent())
        w = containingBlock()->availableLogicalWidth();
    return padding.calcMinValue(w);
}

int RenderBoxModelObject::paddingAfter(bool) const
{
    int w = 0;
    Length padding = style()->paddingAfter();
    if (padding.isPercent())
        w = containingBlock()->availableLogicalWidth();
    return padding.calcMinValue(w);
}

int RenderBoxModelObject::paddingStart(bool) const
{
    int w = 0;
    Length padding = style()->paddingStart();
    if (padding.isPercent())
        w = containingBlock()->availableLogicalWidth();
    return padding.calcMinValue(w);
}

int RenderBoxModelObject::paddingEnd(bool) const
{
    int w = 0;
    Length padding = style()->paddingEnd();
    if (padding.isPercent())
        w = containingBlock()->availableLogicalWidth();
    return padding.calcMinValue(w);
}

RoundedIntRect RenderBoxModelObject::getBackgroundRoundedRect(const IntRect& borderRect, InlineFlowBox* box, int inlineBoxWidth, int inlineBoxHeight,
    bool includeLogicalLeftEdge, bool includeLogicalRightEdge)
{
    RoundedIntRect border = style()->getRoundedBorderFor(borderRect, includeLogicalLeftEdge, includeLogicalRightEdge);
    if (box && (box->nextLineBox() || box->prevLineBox())) {
        RoundedIntRect segmentBorder = style()->getRoundedBorderFor(IntRect(0, 0, inlineBoxWidth, inlineBoxHeight), includeLogicalLeftEdge, includeLogicalRightEdge);
        border.setRadii(segmentBorder.radii());
    }

    return border;
}

static IntRect backgroundRectAdjustedForBleedAvoidance(GraphicsContext* context, const IntRect& borderRect, BackgroundBleedAvoidance bleedAvoidance)
{
    if (bleedAvoidance != BackgroundBleedShrinkBackground)
        return borderRect;

    IntRect adjustedRect = borderRect;
    // We need to shrink the border by one device pixel on each side.
    AffineTransform ctm = context->getCTM();
    FloatSize contextScale(static_cast<float>(ctm.xScale()), static_cast<float>(ctm.yScale()));
    adjustedRect.inflateX(-ceilf(1 / contextScale.width()));
    adjustedRect.inflateY(-ceilf(1 / contextScale.height()));
    return adjustedRect;
}

void RenderBoxModelObject::paintFillLayerExtended(const PaintInfo& paintInfo, const Color& color, const FillLayer* bgLayer, int tx, int ty, int w, int h,
    BackgroundBleedAvoidance bleedAvoidance, InlineFlowBox* box, int inlineBoxWidth, int inlineBoxHeight, CompositeOperator op, RenderObject* backgroundObject)
{
    GraphicsContext* context = paintInfo.context;
    if (context->paintingDisabled())
        return;

    IntRect borderRect(tx, ty, w, h);
    if (borderRect.isEmpty())
        return;

    bool includeLeftEdge = box ? box->includeLogicalLeftEdge() : true;
    bool includeRightEdge = box ? box->includeLogicalRightEdge() : true;

    bool hasRoundedBorder = style()->hasBorderRadius() && (includeLeftEdge || includeRightEdge);
    bool clippedWithLocalScrolling = hasOverflowClip() && bgLayer->attachment() == LocalBackgroundAttachment;
    bool isBorderFill = bgLayer->clip() == BorderFillBox;
    bool isRoot = this->isRoot();

    Color bgColor = color;
    StyleImage* bgImage = bgLayer->image();
    bool shouldPaintBackgroundImage = bgImage && bgImage->canRender(style()->effectiveZoom());
    
    // When this style flag is set, change existing background colors and images to a solid white background.
    // If there's no bg color or image, leave it untouched to avoid affecting transparency.
    // We don't try to avoid loading the background images, because this style flag is only set
    // when printing, and at that point we've already loaded the background images anyway. (To avoid
    // loading the background images we'd have to do this check when applying styles rather than
    // while rendering.)
    if (style()->forceBackgroundsToWhite()) {
        // Note that we can't reuse this variable below because the bgColor might be changed
        bool shouldPaintBackgroundColor = !bgLayer->next() && bgColor.isValid() && bgColor.alpha() > 0;
        if (shouldPaintBackgroundImage || shouldPaintBackgroundColor) {
            bgColor = Color::white;
            shouldPaintBackgroundImage = false;
        }
    }

    bool colorVisible = bgColor.isValid() && bgColor.alpha() > 0;
    
    // Fast path for drawing simple color backgrounds.
    if (!isRoot && !clippedWithLocalScrolling && !shouldPaintBackgroundImage && isBorderFill) {
        if (!colorVisible)
            return;

        if (hasRoundedBorder && bleedAvoidance != BackgroundBleedUseTransparencyLayer) {
            RoundedIntRect border = getBackgroundRoundedRect(backgroundRectAdjustedForBleedAvoidance(context, borderRect, bleedAvoidance), box, inlineBoxWidth, inlineBoxHeight, includeLeftEdge, includeRightEdge);
            context->fillRoundedRect(border, bgColor, style()->colorSpace());
        } else
            context->fillRect(borderRect, bgColor, style()->colorSpace());
        
        return;
    }

    bool clipToBorderRadius = hasRoundedBorder && bleedAvoidance != BackgroundBleedUseTransparencyLayer;
    GraphicsContextStateSaver clipToBorderStateSaver(*context, clipToBorderRadius);
    if (clipToBorderRadius) {
        RoundedIntRect border = getBackgroundRoundedRect(backgroundRectAdjustedForBleedAvoidance(context, borderRect, bleedAvoidance), box, inlineBoxWidth, inlineBoxHeight, includeLeftEdge, includeRightEdge);
        context->addRoundedRectClip(border);
    }
    
    int bLeft = includeLeftEdge ? borderLeft() : 0;
    int bRight = includeRightEdge ? borderRight() : 0;
    int pLeft = includeLeftEdge ? paddingLeft() : 0;
    int pRight = includeRightEdge ? paddingRight() : 0;

    GraphicsContextStateSaver clipWithScrollingStateSaver(*context, clippedWithLocalScrolling);
    if (clippedWithLocalScrolling) {
        // Clip to the overflow area.
        context->clip(toRenderBox(this)->overflowClipRect(tx, ty));
        
        // Now adjust our tx, ty, w, h to reflect a scrolled content box with borders at the ends.
        IntSize offset = layer()->scrolledContentOffset();
        tx -= offset.width();
        ty -= offset.height();
        w = bLeft + layer()->scrollWidth() + bRight;
        h = borderTop() + layer()->scrollHeight() + borderBottom();
    }
    
    GraphicsContextStateSaver backgroundClipStateSaver(*context, false);
    if (bgLayer->clip() == PaddingFillBox || bgLayer->clip() == ContentFillBox) {
        // Clip to the padding or content boxes as necessary.
        bool includePadding = bgLayer->clip() == ContentFillBox;
        int x = tx + bLeft + (includePadding ? pLeft : 0);
        int y = ty + borderTop() + (includePadding ? paddingTop() : 0);
        int width = w - bLeft - bRight - (includePadding ? pLeft + pRight : 0);
        int height = h - borderTop() - borderBottom() - (includePadding ? paddingTop() + paddingBottom() : 0);
        backgroundClipStateSaver.save();
        context->clip(IntRect(x, y, width, height));
    } else if (bgLayer->clip() == TextFillBox) {
        // We have to draw our text into a mask that can then be used to clip background drawing.
        // First figure out how big the mask has to be.  It should be no bigger than what we need
        // to actually render, so we should intersect the dirty rect with the border box of the background.
        IntRect maskRect(tx, ty, w, h);
        maskRect.intersect(paintInfo.rect);
        
        // Now create the mask.
        OwnPtr<ImageBuffer> maskImage = ImageBuffer::create(maskRect.size());
        if (!maskImage)
            return;
        
        GraphicsContext* maskImageContext = maskImage->context();
        maskImageContext->translate(-maskRect.x(), -maskRect.y());
        
        // Now add the text to the clip.  We do this by painting using a special paint phase that signals to
        // InlineTextBoxes that they should just add their contents to the clip.
        PaintInfo info(maskImageContext, maskRect, PaintPhaseTextClip, true, 0, 0);
        if (box) {
            RootInlineBox* root = box->root();
            box->paint(info, tx - box->x(), ty - box->y(), root->lineTop(), root->lineBottom());
        } else {
            int x = isBox() ? toRenderBox(this)->x() : 0;
            int y = isBox() ? toRenderBox(this)->y() : 0;
            paint(info, tx - x, ty - y);
        }
        
        // The mask has been created.  Now we just need to clip to it.
        backgroundClipStateSaver.save();
        context->clipToImageBuffer(maskImage.get(), maskRect);
    }
    
    // Only fill with a base color (e.g., white) if we're the root document, since iframes/frames with
    // no background in the child document should show the parent's background.
    bool isOpaqueRoot = false;
    if (isRoot) {
        isOpaqueRoot = true;
        if (!bgLayer->next() && !(bgColor.isValid() && bgColor.alpha() == 255) && view()->frameView()) {
            Element* ownerElement = document()->ownerElement();
            if (ownerElement) {
                if (!ownerElement->hasTagName(frameTag)) {
                    // Locate the <body> element using the DOM.  This is easier than trying
                    // to crawl around a render tree with potential :before/:after content and
                    // anonymous blocks created by inline <body> tags etc.  We can locate the <body>
                    // render object very easily via the DOM.
                    HTMLElement* body = document()->body();
                    if (body) {
                        // Can't scroll a frameset document anyway.
                        isOpaqueRoot = body->hasLocalName(framesetTag);
                    }
#if ENABLE(SVG)
                    else {
                        // SVG documents and XML documents with SVG root nodes are transparent.
                        isOpaqueRoot = !document()->hasSVGRootNode();
                    }
#endif
                }
            } else
                isOpaqueRoot = !view()->frameView()->isTransparent();
        }
        view()->frameView()->setContentIsOpaque(isOpaqueRoot);
    }

    // Paint the color first underneath all images.
    if (!bgLayer->next()) {
        IntRect rect(tx, ty, w, h);
        rect.intersect(paintInfo.rect);
        // If we have an alpha and we are painting the root element, go ahead and blend with the base background color.
        if (isOpaqueRoot) {
            Color baseColor = view()->frameView()->baseBackgroundColor();
            if (baseColor.alpha() > 0) {
                CompositeOperator previousOperator = context->compositeOperation();
                context->setCompositeOperation(CompositeCopy);
                context->fillRect(rect, baseColor, style()->colorSpace());
                context->setCompositeOperation(previousOperator);
            } else
                context->clearRect(rect);
        }

        if (bgColor.isValid() && bgColor.alpha() > 0)
            context->fillRect(rect, bgColor, style()->colorSpace());
    }

    // no progressive loading of the background image
    if (shouldPaintBackgroundImage) {
        IntRect destRect;
        IntPoint phase;
        IntSize tileSize;

        calculateBackgroundImageGeometry(bgLayer, tx, ty, w, h, destRect, phase, tileSize);
        IntPoint destOrigin = destRect.location();
        destRect.intersect(paintInfo.rect);
        if (!destRect.isEmpty()) {
            phase += destRect.location() - destOrigin;
            CompositeOperator compositeOp = op == CompositeSourceOver ? bgLayer->composite() : op;
            RenderObject* clientForBackgroundImage = backgroundObject ? backgroundObject : this;
            RefPtr<Image> image = bgImage->image(clientForBackgroundImage, tileSize);
            bool useLowQualityScaling = shouldPaintAtLowQuality(context, image.get(), bgLayer, tileSize);
            context->drawTiledImage(image.get(), style()->colorSpace(), destRect, phase, tileSize, compositeOp, useLowQualityScaling);
        }
    }
}

IntSize RenderBoxModelObject::calculateFillTileSize(const FillLayer* fillLayer, IntSize positioningAreaSize) const
{
    StyleImage* image = fillLayer->image();
    image->setImageContainerSize(positioningAreaSize); // Use the box established by background-origin.

    EFillSizeType type = fillLayer->size().type;

    switch (type) {
        case SizeLength: {
            int w = positioningAreaSize.width();
            int h = positioningAreaSize.height();

            Length layerWidth = fillLayer->size().size.width();
            Length layerHeight = fillLayer->size().size.height();

            if (layerWidth.isFixed())
                w = layerWidth.value();
            else if (layerWidth.isPercent())
                w = layerWidth.calcValue(positioningAreaSize.width());
            
            if (layerHeight.isFixed())
                h = layerHeight.value();
            else if (layerHeight.isPercent())
                h = layerHeight.calcValue(positioningAreaSize.height());
            
            // If one of the values is auto we have to use the appropriate
            // scale to maintain our aspect ratio.
            if (layerWidth.isAuto() && !layerHeight.isAuto()) {
                IntSize imageIntrinsicSize = image->imageSize(this, style()->effectiveZoom());
                if (imageIntrinsicSize.height())
                    w = imageIntrinsicSize.width() * h / imageIntrinsicSize.height();        
            } else if (!layerWidth.isAuto() && layerHeight.isAuto()) {
                IntSize imageIntrinsicSize = image->imageSize(this, style()->effectiveZoom());
                if (imageIntrinsicSize.width())
                    h = imageIntrinsicSize.height() * w / imageIntrinsicSize.width();
            } else if (layerWidth.isAuto() && layerHeight.isAuto()) {
                // If both width and height are auto, use the image's intrinsic size.
                IntSize imageIntrinsicSize = image->imageSize(this, style()->effectiveZoom());
                w = imageIntrinsicSize.width();
                h = imageIntrinsicSize.height();
            }
            
            return IntSize(max(1, w), max(1, h));
        }
        case Contain:
        case Cover: {
            IntSize imageIntrinsicSize = image->imageSize(this, 1);
            float horizontalScaleFactor = imageIntrinsicSize.width()
                ? static_cast<float>(positioningAreaSize.width()) / imageIntrinsicSize.width() : 1;
            float verticalScaleFactor = imageIntrinsicSize.height()
                ? static_cast<float>(positioningAreaSize.height()) / imageIntrinsicSize.height() : 1;
            float scaleFactor = type == Contain ? min(horizontalScaleFactor, verticalScaleFactor) : max(horizontalScaleFactor, verticalScaleFactor);
            return IntSize(max<int>(1, imageIntrinsicSize.width() * scaleFactor), max<int>(1, imageIntrinsicSize.height() * scaleFactor));
        }
        case SizeNone:
            break;
    }

    return image->imageSize(this, style()->effectiveZoom());
}

void RenderBoxModelObject::calculateBackgroundImageGeometry(const FillLayer* fillLayer, int tx, int ty, int w, int h, 
                                                            IntRect& destRect, IntPoint& phase, IntSize& tileSize)
{
    int left = 0;
    int top = 0;
    IntSize positioningAreaSize;

    // Determine the background positioning area and set destRect to the background painting area.
    // destRect will be adjusted later if the background is non-repeating.
    bool fixedAttachment = fillLayer->attachment() == FixedBackgroundAttachment;

#if ENABLE(FAST_MOBILE_SCROLLING)
    if (view()->frameView() && view()->frameView()->canBlitOnScroll()) {
        // As a side effect of an optimization to blit on scroll, we do not honor the CSS
        // property "background-attachment: fixed" because it may result in rendering
        // artifacts. Note, these artifacts only appear if we are blitting on scroll of
        // a page that has fixed background images.
        fixedAttachment = false;
    }
#endif

    if (!fixedAttachment) {
        destRect = IntRect(tx, ty, w, h);

        int right = 0;
        int bottom = 0;
        // Scroll and Local.
        if (fillLayer->origin() != BorderFillBox) {
            left = borderLeft();
            right = borderRight();
            top = borderTop();
            bottom = borderBottom();
            if (fillLayer->origin() == ContentFillBox) {
                left += paddingLeft();
                right += paddingRight();
                top += paddingTop();
                bottom += paddingBottom();
            }
        }

        // The background of the box generated by the root element covers the entire canvas including
        // its margins. Since those were added in already, we have to factor them out when computing
        // the background positioning area.
        if (isRoot()) {
            positioningAreaSize = IntSize(toRenderBox(this)->width() - left - right, toRenderBox(this)->height() - top - bottom);
            left += marginLeft();
            top += marginTop();
        } else
            positioningAreaSize = IntSize(w - left - right, h - top - bottom);
    } else {
        destRect = viewRect();
        positioningAreaSize = destRect.size();
    }

    tileSize = calculateFillTileSize(fillLayer, positioningAreaSize);

    EFillRepeat backgroundRepeatX = fillLayer->repeatX();
    EFillRepeat backgroundRepeatY = fillLayer->repeatY();

    int xPosition = fillLayer->xPosition().calcMinValue(positioningAreaSize.width() - tileSize.width(), true);
    if (backgroundRepeatX == RepeatFill)
        phase.setX(tileSize.width() ? tileSize.width() - (xPosition + left) % tileSize.width() : 0);
    else {
        destRect.move(max(xPosition + left, 0), 0);
        phase.setX(-min(xPosition + left, 0));
        destRect.setWidth(tileSize.width() + min(xPosition + left, 0));
    }

    int yPosition = fillLayer->yPosition().calcMinValue(positioningAreaSize.height() - tileSize.height(), true);
    if (backgroundRepeatY == RepeatFill)
        phase.setY(tileSize.height() ? tileSize.height() - (yPosition + top) % tileSize.height() : 0);
    else {
        destRect.move(0, max(yPosition + top, 0));
        phase.setY(-min(yPosition + top, 0));
        destRect.setHeight(tileSize.height() + min(yPosition + top, 0));
    }

    if (fixedAttachment)
        phase.move(max(tx - destRect.x(), 0), max(ty - destRect.y(), 0));

    destRect.intersect(IntRect(tx, ty, w, h));
}

bool RenderBoxModelObject::paintNinePieceImage(GraphicsContext* graphicsContext, int tx, int ty, int w, int h, const RenderStyle* style,
                                               const NinePieceImage& ninePieceImage, CompositeOperator op)
{
    StyleImage* styleImage = ninePieceImage.image();
    if (!styleImage)
        return false;

    if (!styleImage->isLoaded())
        return true; // Never paint a nine-piece image incrementally, but don't paint the fallback borders either.

    if (!styleImage->canRender(style->effectiveZoom()))
        return false;

    // FIXME: border-image is broken with full page zooming when tiling has to happen, since the tiling function
    // doesn't have any understanding of the zoom that is in effect on the tile.
    styleImage->setImageContainerSize(IntSize(w, h));
    IntSize imageSize = styleImage->imageSize(this, 1.0f);
    int imageWidth = imageSize.width();
    int imageHeight = imageSize.height();

    int topSlice = min(imageHeight, ninePieceImage.slices().top().calcValue(imageHeight));
    int bottomSlice = min(imageHeight, ninePieceImage.slices().bottom().calcValue(imageHeight));
    int leftSlice = min(imageWidth, ninePieceImage.slices().left().calcValue(imageWidth));
    int rightSlice = min(imageWidth, ninePieceImage.slices().right().calcValue(imageWidth));

    ENinePieceImageRule hRule = ninePieceImage.horizontalRule();
    ENinePieceImageRule vRule = ninePieceImage.verticalRule();

    bool fitToBorder = style->borderImage() == ninePieceImage;
    
    int leftWidth = fitToBorder ? style->borderLeftWidth() : leftSlice;
    int topWidth = fitToBorder ? style->borderTopWidth() : topSlice;
    int rightWidth = fitToBorder ? style->borderRightWidth() : rightSlice;
    int bottomWidth = fitToBorder ? style->borderBottomWidth() : bottomSlice;

    bool drawLeft = leftSlice > 0 && leftWidth > 0;
    bool drawTop = topSlice > 0 && topWidth > 0;
    bool drawRight = rightSlice > 0 && rightWidth > 0;
    bool drawBottom = bottomSlice > 0 && bottomWidth > 0;
    bool drawMiddle = (imageWidth - leftSlice - rightSlice) > 0 && (w - leftWidth - rightWidth) > 0 &&
                      (imageHeight - topSlice - bottomSlice) > 0 && (h - topWidth - bottomWidth) > 0;

    RefPtr<Image> image = styleImage->image(this, imageSize);
    ColorSpace colorSpace = style->colorSpace();

    if (drawLeft) {
        // Paint the top and bottom left corners.

        // The top left corner rect is (tx, ty, leftWidth, topWidth)
        // The rect to use from within the image is obtained from our slice, and is (0, 0, leftSlice, topSlice)
        if (drawTop)
            graphicsContext->drawImage(image.get(), colorSpace, IntRect(tx, ty, leftWidth, topWidth),
                                       IntRect(0, 0, leftSlice, topSlice), op);

        // The bottom left corner rect is (tx, ty + h - bottomWidth, leftWidth, bottomWidth)
        // The rect to use from within the image is (0, imageHeight - bottomSlice, leftSlice, botomSlice)
        if (drawBottom)
            graphicsContext->drawImage(image.get(), colorSpace, IntRect(tx, ty + h - bottomWidth, leftWidth, bottomWidth),
                                       IntRect(0, imageHeight - bottomSlice, leftSlice, bottomSlice), op);

        // Paint the left edge.
        // Have to scale and tile into the border rect.
        graphicsContext->drawTiledImage(image.get(), colorSpace, IntRect(tx, ty + topWidth, leftWidth,
                                        h - topWidth - bottomWidth),
                                        IntRect(0, topSlice, leftSlice, imageHeight - topSlice - bottomSlice),
                                        Image::StretchTile, (Image::TileRule)vRule, op);
    }

    if (drawRight) {
        // Paint the top and bottom right corners
        // The top right corner rect is (tx + w - rightWidth, ty, rightWidth, topWidth)
        // The rect to use from within the image is obtained from our slice, and is (imageWidth - rightSlice, 0, rightSlice, topSlice)
        if (drawTop)
            graphicsContext->drawImage(image.get(), colorSpace, IntRect(tx + w - rightWidth, ty, rightWidth, topWidth),
                                       IntRect(imageWidth - rightSlice, 0, rightSlice, topSlice), op);

        // The bottom right corner rect is (tx + w - rightWidth, ty + h - bottomWidth, rightWidth, bottomWidth)
        // The rect to use from within the image is (imageWidth - rightSlice, imageHeight - bottomSlice, rightSlice, bottomSlice)
        if (drawBottom)
            graphicsContext->drawImage(image.get(), colorSpace, IntRect(tx + w - rightWidth, ty + h - bottomWidth, rightWidth, bottomWidth),
                                       IntRect(imageWidth - rightSlice, imageHeight - bottomSlice, rightSlice, bottomSlice), op);

        // Paint the right edge.
        graphicsContext->drawTiledImage(image.get(), colorSpace, IntRect(tx + w - rightWidth, ty + topWidth, rightWidth,
                                        h - topWidth - bottomWidth),
                                        IntRect(imageWidth - rightSlice, topSlice, rightSlice, imageHeight - topSlice - bottomSlice),
                                        Image::StretchTile, (Image::TileRule)vRule, op);
    }

    // Paint the top edge.
    if (drawTop)
        graphicsContext->drawTiledImage(image.get(), colorSpace, IntRect(tx + leftWidth, ty, w - leftWidth - rightWidth, topWidth),
                                        IntRect(leftSlice, 0, imageWidth - rightSlice - leftSlice, topSlice),
                                        (Image::TileRule)hRule, Image::StretchTile, op);

    // Paint the bottom edge.
    if (drawBottom)
        graphicsContext->drawTiledImage(image.get(), colorSpace, IntRect(tx + leftWidth, ty + h - bottomWidth,
                                        w - leftWidth - rightWidth, bottomWidth),
                                        IntRect(leftSlice, imageHeight - bottomSlice, imageWidth - rightSlice - leftSlice, bottomSlice),
                                        (Image::TileRule)hRule, Image::StretchTile, op);

    // Paint the middle.
    if (drawMiddle)
        graphicsContext->drawTiledImage(image.get(), colorSpace, IntRect(tx + leftWidth, ty + topWidth, w - leftWidth - rightWidth,
                                        h - topWidth - bottomWidth),
                                        IntRect(leftSlice, topSlice, imageWidth - rightSlice - leftSlice, imageHeight - topSlice - bottomSlice),
                                        (Image::TileRule)hRule, (Image::TileRule)vRule, op);

    return true;
}

class BorderEdge {
public:
    BorderEdge(int edgeWidth, const Color& edgeColor, EBorderStyle edgeStyle, bool edgeIsTransparent, bool edgeIsPresent = true)
        : width(edgeWidth)
        , color(edgeColor)
        , style(edgeStyle)
        , isTransparent(edgeIsTransparent)
        , isPresent(edgeIsPresent)
    {
        if (style == DOUBLE && edgeWidth < 3)
            style = SOLID;
    }
    
    BorderEdge()
        : width(0)
        , style(BHIDDEN)
        , isTransparent(false)
        , isPresent(false)
    {
    }
    
    bool hasVisibleColorAndStyle() const { return style > BHIDDEN && !isTransparent; }
    bool shouldRender() const { return isPresent && hasVisibleColorAndStyle(); }
    bool presentButInvisible() const { return usedWidth() && !hasVisibleColorAndStyle(); }
    bool obscuresBackgroundEdge(float scale) const
    {
        if (!isPresent || isTransparent || width < (2 * scale) || color.hasAlpha() || style == BHIDDEN)
            return false;

        if (style == DOTTED || style == DASHED)
            return false;

        if (style == DOUBLE)
            return width >= 5 * scale; // The outer band needs to be >= 2px wide at unit scale.

        return true;
    }

    int usedWidth() const { return isPresent ? width : 0; }
    
    void getDoubleBorderStripeWidths(int& outerWidth, int& innerWidth) const
    {
        int fullWidth = usedWidth();
        outerWidth = fullWidth / 3;
        innerWidth = fullWidth * 2 / 3;

        // We need certain integer rounding results
        if (fullWidth % 3 == 2)
            outerWidth += 1;

        if (fullWidth % 3 == 1)
            innerWidth += 1;
    }
    
    int width;
    Color color;
    EBorderStyle style;
    bool isTransparent;
    bool isPresent;
};

#if HAVE(PATH_BASED_BORDER_RADIUS_DRAWING)
static bool borderWillArcInnerEdge(const IntSize& firstRadius, const IntSize& secondRadius)
{
    return !firstRadius.isZero() || !secondRadius.isZero();
}

enum BorderEdgeFlag {
    TopBorderEdge = 1 << BSTop,
    RightBorderEdge = 1 << BSRight,
    BottomBorderEdge = 1 << BSBottom,
    LeftBorderEdge = 1 << BSLeft,
    AllBorderEdges = TopBorderEdge | BottomBorderEdge | LeftBorderEdge | RightBorderEdge
};

static inline BorderEdgeFlag edgeFlagForSide(BoxSide side)
{
    return static_cast<BorderEdgeFlag>(1 << side);
}

static inline bool includesEdge(BorderEdgeFlags flags, BoxSide side)
{
    return flags & edgeFlagForSide(side);
}

inline bool edgesShareColor(const BorderEdge& firstEdge, const BorderEdge& secondEdge)
{
    return firstEdge.color == secondEdge.color;
}

inline bool styleRequiresClipPolygon(EBorderStyle style)
{
    return style == DOTTED || style == DASHED; // These are drawn with a stroke, so we have to clip to get corner miters.
}

static bool borderStyleFillsBorderArea(EBorderStyle style)
{
    return !(style == DOTTED || style == DASHED || style == DOUBLE);
}

static bool borderStyleHasInnerDetail(EBorderStyle style)
{
    return style == GROOVE || style == RIDGE || style == DOUBLE;
}

static bool borderStyleIsDottedOrDashed(EBorderStyle style)
{
    return style == DOTTED || style == DASHED;
}

// OUTSET darkens the bottom and right (and maybe lightens the top and left)
// INSET darkens the top and left (and maybe lightens the bottom and right)
static inline bool borderStyleHasUnmatchedColorsAtCorner(EBorderStyle style, BoxSide side, BoxSide adjacentSide)
{
    // These styles match at the top/left and bottom/right.
    if (style == INSET || style == GROOVE || style == RIDGE || style == OUTSET) {
        const BorderEdgeFlags topRightFlags = edgeFlagForSide(BSTop) | edgeFlagForSide(BSRight);
        const BorderEdgeFlags bottomLeftFlags = edgeFlagForSide(BSBottom) | edgeFlagForSide(BSLeft);

        BorderEdgeFlags flags = edgeFlagForSide(side) | edgeFlagForSide(adjacentSide);
        return flags == topRightFlags || flags == bottomLeftFlags;
    }
    return false;
}

static inline bool colorsMatchAtCorner(BoxSide side, BoxSide adjacentSide, const BorderEdge edges[])
{
    if (edges[side].shouldRender() != edges[adjacentSide].shouldRender())
        return false;

    if (!edgesShareColor(edges[side], edges[adjacentSide]))
        return false;

    return !borderStyleHasUnmatchedColorsAtCorner(edges[side].style, side, adjacentSide);
}

// This assumes that we draw in order: top, bottom, left, right.
static inline bool willBeOverdrawn(BoxSide side, BoxSide adjacentSide, const BorderEdge edges[])
{
    switch (side) {
    case BSTop:
    case BSBottom:
        if (edges[adjacentSide].presentButInvisible())
            return false;

        if (!edgesShareColor(edges[side], edges[adjacentSide]) && edges[adjacentSide].color.hasAlpha())
            return false;
        
        if (!borderStyleFillsBorderArea(edges[adjacentSide].style))
            return false;

        return true;

    case BSLeft:
    case BSRight:
        // These draw last, so are never overdrawn.
        return false;
    }
    return false;
}

static inline bool borderStylesRequireMitre(BoxSide side, BoxSide adjacentSide, EBorderStyle style, EBorderStyle adjacentStyle)
{
    if (style == DOUBLE || adjacentStyle == DOUBLE || adjacentStyle == GROOVE || adjacentStyle == RIDGE)
        return true;

    if (borderStyleIsDottedOrDashed(style) != borderStyleIsDottedOrDashed(adjacentStyle))
        return true;

    if (style != adjacentStyle)
        return true;

    return borderStyleHasUnmatchedColorsAtCorner(style, side, adjacentSide);
}

static bool joinRequiresMitre(BoxSide side, BoxSide adjacentSide, const BorderEdge edges[], bool allowOverdraw)
{
    if ((edges[side].isTransparent && edges[adjacentSide].isTransparent) || !edges[adjacentSide].isPresent)
        return false;

    if (allowOverdraw && willBeOverdrawn(side, adjacentSide, edges))
        return false;

    if (!edgesShareColor(edges[side], edges[adjacentSide]))
        return true;

    if (borderStylesRequireMitre(side, adjacentSide, edges[side].style, edges[adjacentSide].style))
        return true;
    
    return false;
}

void RenderBoxModelObject::paintOneBorderSide(GraphicsContext* graphicsContext, const RenderStyle* style, const RoundedIntRect& outerBorder, const RoundedIntRect& innerBorder,
    const IntRect& sideRect, BoxSide side, BoxSide adjacentSide1, BoxSide adjacentSide2, const BorderEdge edges[], const Path* path, 
    BackgroundBleedAvoidance bleedAvoidance, bool includeLogicalLeftEdge, bool includeLogicalRightEdge, bool antialias, const Color* overrideColor)
{
    const BorderEdge& edgeToRender = edges[side];
    const BorderEdge& adjacentEdge1 = edges[adjacentSide1];
    const BorderEdge& adjacentEdge2 = edges[adjacentSide2];

    bool mitreAdjacentSide1 = joinRequiresMitre(side, adjacentSide1, edges, !antialias);
    bool mitreAdjacentSide2 = joinRequiresMitre(side, adjacentSide2, edges, !antialias);
    
    bool adjacentSide1StylesMatch = colorsMatchAtCorner(side, adjacentSide1, edges);
    bool adjacentSide2StylesMatch = colorsMatchAtCorner(side, adjacentSide2, edges);

    const Color& colorToPaint = overrideColor ? *overrideColor : edgeToRender.color;

    if (path) {
        GraphicsContextStateSaver stateSaver(*graphicsContext);
        clipBorderSidePolygon(graphicsContext, outerBorder, innerBorder, side, adjacentSide1StylesMatch, adjacentSide2StylesMatch);
        float thickness = max(max(edgeToRender.width, adjacentEdge1.width), adjacentEdge2.width);
        drawBoxSideFromPath(graphicsContext, outerBorder.rect(), *path, edges, edgeToRender.width, thickness, side, style,
            colorToPaint, edgeToRender.style, bleedAvoidance, includeLogicalLeftEdge, includeLogicalRightEdge);
    } else {
        bool shouldClip = styleRequiresClipPolygon(edgeToRender.style) && (mitreAdjacentSide1 || mitreAdjacentSide2);
        
        GraphicsContextStateSaver clipStateSaver(*graphicsContext, shouldClip);
        if (shouldClip) {
            clipBorderSidePolygon(graphicsContext, outerBorder, innerBorder, side, !mitreAdjacentSide1, !mitreAdjacentSide2);
            // Since we clipped, no need to draw with a mitre.
            mitreAdjacentSide1 = false;
            mitreAdjacentSide2 = false;
        }
        
        drawLineForBoxSide(graphicsContext, sideRect.x(), sideRect.y(), sideRect.maxX(), sideRect.maxY(), side, colorToPaint, edgeToRender.style,
                mitreAdjacentSide1 ? adjacentEdge1.width : 0, mitreAdjacentSide2 ? adjacentEdge2.width : 0, antialias);
    }
}

void RenderBoxModelObject::paintBorderSides(GraphicsContext* graphicsContext, const RenderStyle* style, const RoundedIntRect& outerBorder, const RoundedIntRect& innerBorder,
                                            const BorderEdge edges[], BorderEdgeFlags edgeSet, BackgroundBleedAvoidance bleedAvoidance,
                                            bool includeLogicalLeftEdge, bool includeLogicalRightEdge, bool antialias, const Color* overrideColor)
{
    bool renderRadii = outerBorder.isRounded();

    Path roundedPath;
    if (renderRadii)
        roundedPath.addRoundedRect(outerBorder);
    
    if (edges[BSTop].shouldRender() && includesEdge(edgeSet, BSTop)) {
        IntRect sideRect = outerBorder.rect();
        sideRect.setHeight(edges[BSTop].width);

        bool usePath = renderRadii && (borderStyleHasInnerDetail(edges[BSTop].style) || borderWillArcInnerEdge(innerBorder.radii().topLeft(), innerBorder.radii().topRight()));
        paintOneBorderSide(graphicsContext, style, outerBorder, innerBorder, sideRect, BSTop, BSLeft, BSRight, edges, usePath ? &roundedPath : 0, bleedAvoidance, includeLogicalLeftEdge, includeLogicalRightEdge, antialias, overrideColor);
    }

    if (edges[BSBottom].shouldRender() && includesEdge(edgeSet, BSBottom)) {
        IntRect sideRect = outerBorder.rect();
        sideRect.shiftYEdgeTo(sideRect.maxY() - edges[BSBottom].width);

        bool usePath = renderRadii && (borderStyleHasInnerDetail(edges[BSBottom].style) || borderWillArcInnerEdge(innerBorder.radii().bottomLeft(), innerBorder.radii().bottomRight()));
        paintOneBorderSide(graphicsContext, style, outerBorder, innerBorder, sideRect, BSBottom, BSLeft, BSRight, edges, usePath ? &roundedPath : 0, bleedAvoidance, includeLogicalLeftEdge, includeLogicalRightEdge, antialias, overrideColor);
    }

    if (edges[BSLeft].shouldRender() && includesEdge(edgeSet, BSLeft)) {
        IntRect sideRect = outerBorder.rect();
        sideRect.setWidth(edges[BSLeft].width);

        bool usePath = renderRadii && (borderStyleHasInnerDetail(edges[BSLeft].style) || borderWillArcInnerEdge(innerBorder.radii().bottomLeft(), innerBorder.radii().topLeft()));
        paintOneBorderSide(graphicsContext, style, outerBorder, innerBorder, sideRect, BSLeft, BSTop, BSBottom, edges, usePath ? &roundedPath : 0, bleedAvoidance, includeLogicalLeftEdge, includeLogicalRightEdge, antialias, overrideColor);
    }

    if (edges[BSRight].shouldRender() && includesEdge(edgeSet, BSRight)) {
        IntRect sideRect = outerBorder.rect();
        sideRect.shiftXEdgeTo(sideRect.maxX() - edges[BSRight].width);

        bool usePath = renderRadii && (borderStyleHasInnerDetail(edges[BSRight].style) || borderWillArcInnerEdge(innerBorder.radii().bottomRight(), innerBorder.radii().topRight()));
        paintOneBorderSide(graphicsContext, style, outerBorder, innerBorder, sideRect, BSRight, BSTop, BSBottom, edges, usePath ? &roundedPath : 0, bleedAvoidance, includeLogicalLeftEdge, includeLogicalRightEdge, antialias, overrideColor);
    }
}

void RenderBoxModelObject::paintTranslucentBorderSides(GraphicsContext* graphicsContext, const RenderStyle* style, const RoundedIntRect& outerBorder, const RoundedIntRect& innerBorder,
                                                       const BorderEdge edges[], BackgroundBleedAvoidance bleedAvoidance, bool includeLogicalLeftEdge, bool includeLogicalRightEdge, bool antialias)
{
    BorderEdgeFlags edgesToDraw = AllBorderEdges;
    while (edgesToDraw) {
        // Find undrawn edges sharing a color.
        Color commonColor;
        
        BorderEdgeFlags commonColorEdgeSet = 0;
        for (int i = BSTop; i <= BSLeft; ++i) {
            BoxSide currSide = static_cast<BoxSide>(i);
            if (!includesEdge(edgesToDraw, currSide))
                continue;

            bool includeEdge;
            if (!commonColorEdgeSet) {
                commonColor = edges[currSide].color;
                includeEdge = true;
            } else
                includeEdge = edges[currSide].color == commonColor;

            if (includeEdge)
                commonColorEdgeSet |= edgeFlagForSide(currSide);
        }

        bool useTransparencyLayer = commonColor.hasAlpha();
        if (useTransparencyLayer) {
            graphicsContext->beginTransparencyLayer(static_cast<float>(commonColor.alpha()) / 255);
            commonColor = Color(commonColor.red(), commonColor.green(), commonColor.blue());
        }

        paintBorderSides(graphicsContext, style, outerBorder, innerBorder, edges, commonColorEdgeSet, bleedAvoidance, includeLogicalLeftEdge, includeLogicalRightEdge, antialias, &commonColor);
            
        if (useTransparencyLayer)
            graphicsContext->endTransparencyLayer();
        
        edgesToDraw &= ~commonColorEdgeSet;
    }
}

void RenderBoxModelObject::paintBorder(GraphicsContext* graphicsContext, int tx, int ty, int w, int h,
                                       const RenderStyle* style, BackgroundBleedAvoidance bleedAvoidance, bool includeLogicalLeftEdge, bool includeLogicalRightEdge)
{
    // border-image is not affected by border-radius.
    if (paintNinePieceImage(graphicsContext, tx, ty, w, h, style, style->borderImage()))
        return;

    if (graphicsContext->paintingDisabled())
        return;

    BorderEdge edges[4];
    getBorderEdgeInfo(edges, includeLogicalLeftEdge, includeLogicalRightEdge);

    IntRect borderRect(tx, ty, w, h);
    RoundedIntRect outerBorder = style->getRoundedBorderFor(borderRect, includeLogicalLeftEdge, includeLogicalRightEdge);
    RoundedIntRect innerBorder = style->getRoundedInnerBorderFor(borderRect, includeLogicalLeftEdge, includeLogicalRightEdge);

    const AffineTransform& currentCTM = graphicsContext->getCTM();
    // FIXME: this isn't quite correct. We may want to antialias when scaled by a non-integral value, or when the translation is non-integral.
    bool antialias = !currentCTM.isIdentityOrTranslationOrFlipped();
    
    bool haveAlphaColor = false;
    bool haveAllSolidEdges = true;
    bool allEdgesVisible = true;
    bool allEdgesShareColor = true;
    int firstVisibleEdge = -1;

    for (int i = BSTop; i <= BSLeft; ++i) {
        const BorderEdge& currEdge = edges[i];
        if (currEdge.presentButInvisible()) {
            allEdgesVisible = false;
            continue;
        }
        
        if (!currEdge.width)
            continue;

        if (firstVisibleEdge == -1)
            firstVisibleEdge = i;
        else if (currEdge.color != edges[firstVisibleEdge].color)
            allEdgesShareColor = false;

        if (currEdge.color.hasAlpha())
            haveAlphaColor = true;
        
        if (currEdge.style != SOLID)
            haveAllSolidEdges = false;
    }
    
    // isRenderable() check avoids issue described in https://bugs.webkit.org/show_bug.cgi?id=38787
    if (haveAllSolidEdges && allEdgesVisible && allEdgesShareColor && innerBorder.isRenderable()) {
        // Fast path for drawing all solid edges.
        if (outerBorder.isRounded() || haveAlphaColor) {
            Path path;
            
            if (outerBorder.isRounded() && bleedAvoidance != BackgroundBleedUseTransparencyLayer)
                path.addRoundedRect(outerBorder);
            else
                path.addRect(outerBorder.rect());

            if (innerBorder.isRounded())
                path.addRoundedRect(innerBorder);
            else
                path.addRect(innerBorder.rect());
            
            graphicsContext->setFillRule(RULE_EVENODD);
            graphicsContext->setFillColor(edges[firstVisibleEdge].color, style->colorSpace());
            graphicsContext->fillPath(path);
        } else
            paintBorderSides(graphicsContext, style, outerBorder, innerBorder, edges, AllBorderEdges, bleedAvoidance, includeLogicalLeftEdge, includeLogicalRightEdge, antialias);
    
        return;
    }

    bool clipToOuterBorder = outerBorder.isRounded();
    GraphicsContextStateSaver stateSaver(*graphicsContext, clipToOuterBorder);
    if (clipToOuterBorder) {
        // Clip to the inner and outer radii rects.
        if (bleedAvoidance != BackgroundBleedUseTransparencyLayer)
            graphicsContext->addRoundedRectClip(outerBorder);
        graphicsContext->clipOutRoundedRect(innerBorder);
    }

    if (haveAlphaColor)
        paintTranslucentBorderSides(graphicsContext, style, outerBorder, innerBorder, edges, bleedAvoidance, includeLogicalLeftEdge, includeLogicalRightEdge, antialias);
    else
        paintBorderSides(graphicsContext, style, outerBorder, innerBorder, edges, AllBorderEdges, bleedAvoidance, includeLogicalLeftEdge, includeLogicalRightEdge, antialias);
}

void RenderBoxModelObject::drawBoxSideFromPath(GraphicsContext* graphicsContext, const IntRect& borderRect, const Path& borderPath, const BorderEdge edges[],
                                    float thickness, float drawThickness, BoxSide side, const RenderStyle* style, 
                                    Color color, EBorderStyle borderStyle, BackgroundBleedAvoidance bleedAvoidance, bool includeLogicalLeftEdge, bool includeLogicalRightEdge)
{
    if (thickness <= 0)
        return;

    if (borderStyle == DOUBLE && thickness < 3)
        borderStyle = SOLID;

    switch (borderStyle) {
    case BNONE:
    case BHIDDEN:
        return;
    case DOTTED:
    case DASHED: {
        graphicsContext->setStrokeColor(color, style->colorSpace());

        // The stroke is doubled here because the provided path is the 
        // outside edge of the border so half the stroke is clipped off. 
        // The extra multiplier is so that the clipping mask can antialias
        // the edges to prevent jaggies.
        graphicsContext->setStrokeThickness(drawThickness * 2 * 1.1f);
        graphicsContext->setStrokeStyle(borderStyle == DASHED ? DashedStroke : DottedStroke);

        // If the number of dashes that fit in the path is odd and non-integral then we
        // will have an awkwardly-sized dash at the end of the path. To try to avoid that
        // here, we simply make the whitespace dashes ever so slightly bigger.
        // FIXME: This could be even better if we tried to manipulate the dash offset
        // and possibly the gapLength to get the corners dash-symmetrical.
        float dashLength = thickness * ((borderStyle == DASHED) ? 3.0f : 1.0f);
        float gapLength = dashLength;
        float numberOfDashes = borderPath.length() / dashLength;
        // Don't try to show dashes if we have less than 2 dashes + 2 gaps.
        // FIXME: should do this test per side.
        if (numberOfDashes >= 4) {
            bool evenNumberOfFullDashes = !((int)numberOfDashes % 2);
            bool integralNumberOfDashes = !(numberOfDashes - (int)numberOfDashes);
            if (!evenNumberOfFullDashes && !integralNumberOfDashes) {
                float numberOfGaps = numberOfDashes / 2;
                gapLength += (dashLength  / numberOfGaps);
            }

            DashArray lineDash;
            lineDash.append(dashLength);
            lineDash.append(gapLength);
            graphicsContext->setLineDash(lineDash, dashLength);
        }
        
        // FIXME: stroking the border path causes issues with tight corners:
        // https://bugs.webkit.org/show_bug.cgi?id=58711
        // Also, to get the best appearance we should stroke a path between the two borders.
        graphicsContext->strokePath(borderPath);
        return;
    }
    case DOUBLE: {
        // Get the inner border rects for both the outer border line and the inner border line
        int outerBorderTopWidth;
        int innerBorderTopWidth;
        edges[BSTop].getDoubleBorderStripeWidths(outerBorderTopWidth, innerBorderTopWidth);

        int outerBorderRightWidth;
        int innerBorderRightWidth;
        edges[BSRight].getDoubleBorderStripeWidths(outerBorderRightWidth, innerBorderRightWidth);

        int outerBorderBottomWidth;
        int innerBorderBottomWidth;
        edges[BSBottom].getDoubleBorderStripeWidths(outerBorderBottomWidth, innerBorderBottomWidth);

        int outerBorderLeftWidth;
        int innerBorderLeftWidth;
        edges[BSLeft].getDoubleBorderStripeWidths(outerBorderLeftWidth, innerBorderLeftWidth);

        // Draw inner border line
        {
            GraphicsContextStateSaver stateSaver(*graphicsContext);
            RoundedIntRect innerClip = style->getRoundedInnerBorderFor(borderRect,
                innerBorderTopWidth, innerBorderBottomWidth, innerBorderLeftWidth, innerBorderRightWidth,
                includeLogicalLeftEdge, includeLogicalRightEdge);
            
            graphicsContext->addRoundedRectClip(innerClip);
            drawBoxSideFromPath(graphicsContext, borderRect, borderPath, edges, thickness, drawThickness, side, style, color, SOLID, bleedAvoidance, includeLogicalLeftEdge, includeLogicalRightEdge);
        }

        // Draw outer border line
        {
            GraphicsContextStateSaver stateSaver(*graphicsContext);
            IntRect outerRect = borderRect;
            if (bleedAvoidance == BackgroundBleedUseTransparencyLayer) {
                outerRect.inflate(1);
                ++outerBorderTopWidth;
                ++outerBorderBottomWidth;
                ++outerBorderLeftWidth;
                ++outerBorderRightWidth;
            }
                
            RoundedIntRect outerClip = style->getRoundedInnerBorderFor(outerRect,
                outerBorderTopWidth, outerBorderBottomWidth, outerBorderLeftWidth, outerBorderRightWidth,
                includeLogicalLeftEdge, includeLogicalRightEdge);
            graphicsContext->clipOutRoundedRect(outerClip);
            drawBoxSideFromPath(graphicsContext, borderRect, borderPath, edges, thickness, drawThickness, side, style, color, SOLID, bleedAvoidance, includeLogicalLeftEdge, includeLogicalRightEdge);
        }
        return;
    }
    case RIDGE:
    case GROOVE:
    {
        EBorderStyle s1;
        EBorderStyle s2;
        if (borderStyle == GROOVE) {
            s1 = INSET;
            s2 = OUTSET;
        } else {
            s1 = OUTSET;
            s2 = INSET;
        }
        
        // Paint full border
        drawBoxSideFromPath(graphicsContext, borderRect, borderPath, edges, thickness, drawThickness, side, style, color, s1, bleedAvoidance, includeLogicalLeftEdge, includeLogicalRightEdge);

        // Paint inner only
        GraphicsContextStateSaver stateSaver(*graphicsContext);
        int topWidth = edges[BSTop].usedWidth() / 2;
        int bottomWidth = edges[BSBottom].usedWidth() / 2;
        int leftWidth = edges[BSLeft].usedWidth() / 2;
        int rightWidth = edges[BSRight].usedWidth() / 2;

        RoundedIntRect clipRect = style->getRoundedInnerBorderFor(borderRect,
            topWidth, bottomWidth, leftWidth, rightWidth,
            includeLogicalLeftEdge, includeLogicalRightEdge);

        graphicsContext->addRoundedRectClip(clipRect);
        drawBoxSideFromPath(graphicsContext, borderRect, borderPath, edges, thickness, drawThickness, side, style, color, s2, bleedAvoidance, includeLogicalLeftEdge, includeLogicalRightEdge);
        return;
    }
    case INSET:
        if (side == BSTop || side == BSLeft)
            color = color.dark();
        break;
    case OUTSET:
        if (side == BSBottom || side == BSRight)
            color = color.dark();
        break;
    default:
        break;
    }

    graphicsContext->setStrokeStyle(NoStroke);
    graphicsContext->setFillColor(color, style->colorSpace());
    graphicsContext->drawRect(borderRect);
}
#else
void RenderBoxModelObject::paintBorder(GraphicsContext* graphicsContext, int tx, int ty, int w, int h,
                                       const RenderStyle* style, BackgroundBleedAvoidance, bool includeLogicalLeftEdge, bool includeLogicalRightEdge)
{
    // FIXME: This old version of paintBorder should be removed when all ports implement 
    // GraphicsContext::clipConvexPolygon()!! This should happen soon.
    if (paintNinePieceImage(graphicsContext, tx, ty, w, h, style, style->borderImage()))
        return;

    const Color& topColor = style->visitedDependentColor(CSSPropertyBorderTopColor);
    const Color& bottomColor = style->visitedDependentColor(CSSPropertyBorderBottomColor);
    const Color& leftColor = style->visitedDependentColor(CSSPropertyBorderLeftColor);
    const Color& rightColor = style->visitedDependentColor(CSSPropertyBorderRightColor);

    bool topTransparent = style->borderTopIsTransparent();
    bool bottomTransparent = style->borderBottomIsTransparent();
    bool rightTransparent = style->borderRightIsTransparent();
    bool leftTransparent = style->borderLeftIsTransparent();

    EBorderStyle topStyle = style->borderTopStyle();
    EBorderStyle bottomStyle = style->borderBottomStyle();
    EBorderStyle leftStyle = style->borderLeftStyle();
    EBorderStyle rightStyle = style->borderRightStyle();

    bool horizontal = style->isHorizontalWritingMode();
    bool renderTop = topStyle > BHIDDEN && !topTransparent && (horizontal || includeLogicalLeftEdge);
    bool renderLeft = leftStyle > BHIDDEN && !leftTransparent && (!horizontal || includeLogicalLeftEdge);
    bool renderRight = rightStyle > BHIDDEN && !rightTransparent && (!horizontal || includeLogicalRightEdge);
    bool renderBottom = bottomStyle > BHIDDEN && !bottomTransparent && (horizontal || includeLogicalRightEdge);


    RoundedIntRect border(tx, ty, w, h);
    
    GraphicsContextStateSaver stateSaver(*graphicsContext, false);
    if (style->hasBorderRadius()) {
        border.includeLogicalEdges(style->getRoundedBorderFor(border.rect()).radii(),
                                   horizontal, includeLogicalLeftEdge, includeLogicalRightEdge);
        if (border.isRounded()) {
            stateSaver.save();
            graphicsContext->addRoundedRectClip(border);
        }
    }

    int firstAngleStart, secondAngleStart, firstAngleSpan, secondAngleSpan;
    float thickness;
    bool renderRadii = border.isRounded();
    bool upperLeftBorderStylesMatch = renderLeft && (topStyle == leftStyle) && (topColor == leftColor);
    bool upperRightBorderStylesMatch = renderRight && (topStyle == rightStyle) && (topColor == rightColor) && (topStyle != OUTSET) && (topStyle != RIDGE) && (topStyle != INSET) && (topStyle != GROOVE);
    bool lowerLeftBorderStylesMatch = renderLeft && (bottomStyle == leftStyle) && (bottomColor == leftColor) && (bottomStyle != OUTSET) && (bottomStyle != RIDGE) && (bottomStyle != INSET) && (bottomStyle != GROOVE);
    bool lowerRightBorderStylesMatch = renderRight && (bottomStyle == rightStyle) && (bottomColor == rightColor);

    if (renderTop) {
        bool ignoreLeft = (renderRadii && border.radii().topLeft().width() > 0)
            || (topColor == leftColor && topTransparent == leftTransparent && topStyle >= OUTSET
                && (leftStyle == DOTTED || leftStyle == DASHED || leftStyle == SOLID || leftStyle == OUTSET));
        
        bool ignoreRight = (renderRadii && border.radii().topRight().width() > 0)
            || (topColor == rightColor && topTransparent == rightTransparent && topStyle >= OUTSET
                && (rightStyle == DOTTED || rightStyle == DASHED || rightStyle == SOLID || rightStyle == INSET));

        int x = tx;
        int x2 = tx + w;
        if (renderRadii) {
            x += border.radii().topLeft().width();
            x2 -= border.radii().topRight().width();
        }

        drawLineForBoxSide(graphicsContext, x, ty, x2, ty + style->borderTopWidth(), BSTop, topColor, topStyle,
                   ignoreLeft ? 0 : style->borderLeftWidth(), ignoreRight ? 0 : style->borderRightWidth());

        if (renderRadii) {
            int leftY = ty;

            // We make the arc double thick and let the clip rect take care of clipping the extra off.
            // We're doing this because it doesn't seem possible to match the curve of the clip exactly
            // with the arc-drawing function.
            thickness = style->borderTopWidth() * 2;

            if (border.radii().topLeft().width()) {
                int leftX = tx;
                // The inner clip clips inside the arc. This is especially important for 1px borders.
                bool applyLeftInnerClip = (style->borderLeftWidth() < border.radii().topLeft().width())
                    && (style->borderTopWidth() < border.radii().topLeft().height())
                    && (topStyle != DOUBLE || style->borderTopWidth() > 6);
                
                GraphicsContextStateSaver stateSaver(*graphicsContext, applyLeftInnerClip);
                if (applyLeftInnerClip)
                    graphicsContext->addInnerRoundedRectClip(IntRect(leftX, leftY, border.radii().topLeft().width() * 2, border.radii().topLeft().height() * 2),
                                                             style->borderTopWidth());

                firstAngleStart = 90;
                firstAngleSpan = upperLeftBorderStylesMatch ? 90 : 45;

                // Draw upper left arc
                drawArcForBoxSide(graphicsContext, leftX, leftY, thickness, border.radii().topLeft(), firstAngleStart, firstAngleSpan,
                              BSTop, topColor, topStyle, true);
            }

            if (border.radii().topRight().width()) {
                int rightX = tx + w - border.radii().topRight().width() * 2;
                bool applyRightInnerClip = (style->borderRightWidth() < border.radii().topRight().width())
                    && (style->borderTopWidth() < border.radii().topRight().height())
                    && (topStyle != DOUBLE || style->borderTopWidth() > 6);

                GraphicsContextStateSaver stateSaver(*graphicsContext, applyRightInnerClip);
                if (applyRightInnerClip)
                    graphicsContext->addInnerRoundedRectClip(IntRect(rightX, leftY, border.radii().topRight().width() * 2, border.radii().topRight().height() * 2),
                                                             style->borderTopWidth());

                if (upperRightBorderStylesMatch) {
                    secondAngleStart = 0;
                    secondAngleSpan = 90;
                } else {
                    secondAngleStart = 45;
                    secondAngleSpan = 45;
                }

                // Draw upper right arc
                drawArcForBoxSide(graphicsContext, rightX, leftY, thickness, border.radii().topRight(), secondAngleStart, secondAngleSpan,
                              BSTop, topColor, topStyle, false);
            }
        }
    }

    if (renderBottom) {
        bool ignoreLeft = (renderRadii && border.radii().bottomLeft().width() > 0)
            || (bottomColor == leftColor && bottomTransparent == leftTransparent && bottomStyle >= OUTSET
                && (leftStyle == DOTTED || leftStyle == DASHED || leftStyle == SOLID || leftStyle == OUTSET));

        bool ignoreRight = (renderRadii && border.radii().bottomRight().width() > 0)
            || (bottomColor == rightColor && bottomTransparent == rightTransparent && bottomStyle >= OUTSET
                && (rightStyle == DOTTED || rightStyle == DASHED || rightStyle == SOLID || rightStyle == INSET));

        int x = tx;
        int x2 = tx + w;
        if (renderRadii) {
            x += border.radii().bottomLeft().width();
            x2 -= border.radii().bottomRight().width();
        }

        drawLineForBoxSide(graphicsContext, x, ty + h - style->borderBottomWidth(), x2, ty + h, BSBottom, bottomColor, bottomStyle,
                   ignoreLeft ? 0 : style->borderLeftWidth(), ignoreRight ? 0 : style->borderRightWidth());

        if (renderRadii) {
            thickness = style->borderBottomWidth() * 2;

            if (border.radii().bottomLeft().width()) {
                int leftX = tx;
                int leftY = ty + h - border.radii().bottomLeft().height() * 2;
                bool applyLeftInnerClip = (style->borderLeftWidth() < border.radii().bottomLeft().width())
                    && (style->borderBottomWidth() < border.radii().bottomLeft().height())
                    && (bottomStyle != DOUBLE || style->borderBottomWidth() > 6);

                GraphicsContextStateSaver stateSaver(*graphicsContext, applyLeftInnerClip);
                if (applyLeftInnerClip)
                    graphicsContext->addInnerRoundedRectClip(IntRect(leftX, leftY, border.radii().bottomLeft().width() * 2, border.radii().bottomLeft().height() * 2),
                                                             style->borderBottomWidth());

                if (lowerLeftBorderStylesMatch) {
                    firstAngleStart = 180;
                    firstAngleSpan = 90;
                } else {
                    firstAngleStart = 225;
                    firstAngleSpan = 45;
                }

                // Draw lower left arc
                drawArcForBoxSide(graphicsContext, leftX, leftY, thickness, border.radii().bottomLeft(), firstAngleStart, firstAngleSpan,
                              BSBottom, bottomColor, bottomStyle, true);
            }

            if (border.radii().bottomRight().width()) {
                int rightY = ty + h - border.radii().bottomRight().height() * 2;
                int rightX = tx + w - border.radii().bottomRight().width() * 2;
                bool applyRightInnerClip = (style->borderRightWidth() < border.radii().bottomRight().width())
                    && (style->borderBottomWidth() < border.radii().bottomRight().height())
                    && (bottomStyle != DOUBLE || style->borderBottomWidth() > 6);

                GraphicsContextStateSaver stateSaver(*graphicsContext, applyRightInnerClip);
                if (applyRightInnerClip)
                    graphicsContext->addInnerRoundedRectClip(IntRect(rightX, rightY, border.radii().bottomRight().width() * 2, border.radii().bottomRight().height() * 2),
                                                             style->borderBottomWidth());

                secondAngleStart = 270;
                secondAngleSpan = lowerRightBorderStylesMatch ? 90 : 45;

                // Draw lower right arc
                drawArcForBoxSide(graphicsContext, rightX, rightY, thickness, border.radii().bottomRight(), secondAngleStart, secondAngleSpan,
                              BSBottom, bottomColor, bottomStyle, false);
            }
        }
    }

    if (renderLeft) {
        bool ignoreTop = (renderRadii && border.radii().topLeft().height() > 0)
            || (topColor == leftColor && topTransparent == leftTransparent && leftStyle >= OUTSET
                && (topStyle == DOTTED || topStyle == DASHED || topStyle == SOLID || topStyle == OUTSET));

        bool ignoreBottom = (renderRadii && border.radii().bottomLeft().height() > 0)
            || (bottomColor == leftColor && bottomTransparent == leftTransparent && leftStyle >= OUTSET
                && (bottomStyle == DOTTED || bottomStyle == DASHED || bottomStyle == SOLID || bottomStyle == INSET));

        int y = ty;
        int y2 = ty + h;
        if (renderRadii) {
            y += border.radii().topLeft().height();
            y2 -= border.radii().bottomLeft().height();
        }

        drawLineForBoxSide(graphicsContext, tx, y, tx + style->borderLeftWidth(), y2, BSLeft, leftColor, leftStyle,
                   ignoreTop ? 0 : style->borderTopWidth(), ignoreBottom ? 0 : style->borderBottomWidth());

        if (renderRadii && (!upperLeftBorderStylesMatch || !lowerLeftBorderStylesMatch)) {
            int topX = tx;
            thickness = style->borderLeftWidth() * 2;

            if (!upperLeftBorderStylesMatch && border.radii().topLeft().width()) {
                int topY = ty;
                bool applyTopInnerClip = (style->borderLeftWidth() < border.radii().topLeft().width())
                    && (style->borderTopWidth() < border.radii().topLeft().height())
                    && (leftStyle != DOUBLE || style->borderLeftWidth() > 6);

                GraphicsContextStateSaver stateSaver(*graphicsContext, applyTopInnerClip);
                if (applyTopInnerClip)
                    graphicsContext->addInnerRoundedRectClip(IntRect(topX, topY, border.radii().topLeft().width() * 2, border.radii().topLeft().height() * 2),
                                                             style->borderLeftWidth());

                firstAngleStart = 135;
                firstAngleSpan = 45;

                // Draw top left arc
                drawArcForBoxSide(graphicsContext, topX, topY, thickness, border.radii().topLeft(), firstAngleStart, firstAngleSpan,
                              BSLeft, leftColor, leftStyle, true);
            }

            if (!lowerLeftBorderStylesMatch && border.radii().bottomLeft().width()) {
                int bottomY = ty + h - border.radii().bottomLeft().height() * 2;
                bool applyBottomInnerClip = (style->borderLeftWidth() < border.radii().bottomLeft().width())
                    && (style->borderBottomWidth() < border.radii().bottomLeft().height())
                    && (leftStyle != DOUBLE || style->borderLeftWidth() > 6);

                GraphicsContextStateSaver stateSaver(*graphicsContext, applyBottomInnerClip);
                if (applyBottomInnerClip)
                    graphicsContext->addInnerRoundedRectClip(IntRect(topX, bottomY, border.radii().bottomLeft().width() * 2, border.radii().bottomLeft().height() * 2),
                                                             style->borderLeftWidth());

                secondAngleStart = 180;
                secondAngleSpan = 45;

                // Draw bottom left arc
                drawArcForBoxSide(graphicsContext, topX, bottomY, thickness, border.radii().bottomLeft(), secondAngleStart, secondAngleSpan,
                              BSLeft, leftColor, leftStyle, false);
            }
        }
    }

    if (renderRight) {
        bool ignoreTop = (renderRadii && border.radii().topRight().height() > 0)
            || ((topColor == rightColor) && (topTransparent == rightTransparent)
                && (rightStyle >= DOTTED || rightStyle == INSET)
                && (topStyle == DOTTED || topStyle == DASHED || topStyle == SOLID || topStyle == OUTSET));

        bool ignoreBottom = (renderRadii && border.radii().bottomRight().height() > 0)
            || ((bottomColor == rightColor) && (bottomTransparent == rightTransparent)
                && (rightStyle >= DOTTED || rightStyle == INSET)
                && (bottomStyle == DOTTED || bottomStyle == DASHED || bottomStyle == SOLID || bottomStyle == INSET));

        int y = ty;
        int y2 = ty + h;
        if (renderRadii) {
            y += border.radii().topRight().height();
            y2 -= border.radii().bottomRight().height();
        }

        drawLineForBoxSide(graphicsContext, tx + w - style->borderRightWidth(), y, tx + w, y2, BSRight, rightColor, rightStyle,
                   ignoreTop ? 0 : style->borderTopWidth(), ignoreBottom ? 0 : style->borderBottomWidth());

        if (renderRadii && (!upperRightBorderStylesMatch || !lowerRightBorderStylesMatch)) {
            thickness = style->borderRightWidth() * 2;

            if (!upperRightBorderStylesMatch && border.radii().topRight().width()) {
                int topX = tx + w - border.radii().topRight().width() * 2;
                int topY = ty;
                bool applyTopInnerClip = (style->borderRightWidth() < border.radii().topRight().width())
                    && (style->borderTopWidth() < border.radii().topRight().height())
                    && (rightStyle != DOUBLE || style->borderRightWidth() > 6);

                GraphicsContextStateSaver stateSaver(*graphicsContext, applyTopInnerClip);
                if (applyTopInnerClip)
                    graphicsContext->addInnerRoundedRectClip(IntRect(topX, topY, border.radii().topRight().width() * 2, border.radii().topRight().height() * 2),
                                                             style->borderRightWidth());

                firstAngleStart = 0;
                firstAngleSpan = 45;

                // Draw top right arc
                drawArcForBoxSide(graphicsContext, topX, topY, thickness, border.radii().topRight(), firstAngleStart, firstAngleSpan,
                              BSRight, rightColor, rightStyle, true);
            }

            if (!lowerRightBorderStylesMatch && border.radii().bottomRight().width()) {
                int bottomX = tx + w - border.radii().bottomRight().width() * 2;
                int bottomY = ty + h - border.radii().bottomRight().height() * 2;
                bool applyBottomInnerClip = (style->borderRightWidth() < border.radii().bottomRight().width())
                    && (style->borderBottomWidth() < border.radii().bottomRight().height())
                    && (rightStyle != DOUBLE || style->borderRightWidth() > 6);

                GraphicsContextStateSaver stateSaver(*graphicsContext, applyBottomInnerClip);
                if (applyBottomInnerClip)
                    graphicsContext->addInnerRoundedRectClip(IntRect(bottomX, bottomY, border.radii().bottomRight().width() * 2, border.radii().bottomRight().height() * 2),
                                                             style->borderRightWidth());

                secondAngleStart = 315;
                secondAngleSpan = 45;

                // Draw bottom right arc
                drawArcForBoxSide(graphicsContext, bottomX, bottomY, thickness, border.radii().bottomRight(), secondAngleStart, secondAngleSpan,
                              BSRight, rightColor, rightStyle, false);
            }
        }
    }
}
#endif

static void findInnerVertex(const FloatPoint& outerCorner, const FloatPoint& innerCorner, const FloatPoint& centerPoint, FloatPoint& result)
{
    // If the line between outer and inner corner is towards the horizontal, intersect with a vertical line through the center,
    // otherwise with a horizontal line through the center. The points that form this line are arbitrary (we use 0, 100).
    // Note that if findIntersection fails, it will leave result untouched.
    if (fabs(outerCorner.x() - innerCorner.x()) > fabs(outerCorner.y() - innerCorner.y()))
        findIntersection(outerCorner, innerCorner, FloatPoint(centerPoint.x(), 0), FloatPoint(centerPoint.x(), 100), result);
    else
        findIntersection(outerCorner, innerCorner, FloatPoint(0, centerPoint.y()), FloatPoint(100, centerPoint.y()), result);
}

void RenderBoxModelObject::clipBorderSidePolygon(GraphicsContext* graphicsContext, const RoundedIntRect& outerBorder, const RoundedIntRect& innerBorder,
                                                 BoxSide side, bool firstEdgeMatches, bool secondEdgeMatches)
{
    FloatPoint quad[4];

    const IntRect& outerRect = outerBorder.rect();
    const IntRect& innerRect = innerBorder.rect();

    FloatPoint centerPoint(innerRect.location().x() + static_cast<float>(innerRect.width()) / 2, innerRect.location().y() + static_cast<float>(innerRect.height()) / 2);

    // For each side, create a quad that encompasses all parts of that side that may draw,
    // including areas inside the innerBorder.
    //
    //         0----------------3
    //       0  \              /  0
    //       |\  1----------- 2  /|
    //       | 1                1 |   
    //       | |                | |
    //       | |                | |  
    //       | 2                2 |  
    //       |/  1------------2  \| 
    //       3  /              \  3   
    //         0----------------3
    //
    switch (side) {
    case BSTop:
        quad[0] = outerRect.minXMinYCorner();
        quad[1] = innerRect.minXMinYCorner();
        quad[2] = innerRect.maxXMinYCorner();
        quad[3] = outerRect.maxXMinYCorner();

        if (!innerBorder.radii().topLeft().isZero())
            findInnerVertex(outerRect.minXMinYCorner(), innerRect.minXMinYCorner(), centerPoint, quad[1]);

        if (!innerBorder.radii().topRight().isZero())
            findInnerVertex(outerRect.maxXMinYCorner(), innerRect.maxXMinYCorner(), centerPoint, quad[2]);
        break;

    case BSLeft:
        quad[0] = outerRect.minXMinYCorner();
        quad[1] = innerRect.minXMinYCorner();
        quad[2] = innerRect.minXMaxYCorner();
        quad[3] = outerRect.minXMaxYCorner();

        if (!innerBorder.radii().topLeft().isZero())
            findInnerVertex(outerRect.minXMinYCorner(), innerRect.minXMinYCorner(), centerPoint, quad[1]);

        if (!innerBorder.radii().bottomLeft().isZero())
            findInnerVertex(outerRect.minXMaxYCorner(), innerRect.minXMaxYCorner(), centerPoint, quad[2]);
        break;

    case BSBottom:
        quad[0] = outerRect.minXMaxYCorner();
        quad[1] = innerRect.minXMaxYCorner();
        quad[2] = innerRect.maxXMaxYCorner();
        quad[3] = outerRect.maxXMaxYCorner();

        if (!innerBorder.radii().bottomLeft().isZero())
            findInnerVertex(outerRect.minXMaxYCorner(), innerRect.minXMaxYCorner(), centerPoint, quad[1]);

        if (!innerBorder.radii().bottomRight().isZero())
            findInnerVertex(outerRect.maxXMaxYCorner(), innerRect.maxXMaxYCorner(), centerPoint, quad[2]);
        break;

    case BSRight:
        quad[0] = outerRect.maxXMinYCorner();
        quad[1] = innerRect.maxXMinYCorner();
        quad[2] = innerRect.maxXMaxYCorner();
        quad[3] = outerRect.maxXMaxYCorner();

        if (!innerBorder.radii().topRight().isZero())
            findInnerVertex(outerRect.maxXMinYCorner(), innerRect.maxXMinYCorner(), centerPoint, quad[1]);

        if (!innerBorder.radii().bottomRight().isZero())
            findInnerVertex(outerRect.maxXMaxYCorner(), innerRect.maxXMaxYCorner(), centerPoint, quad[2]);
        break;
    }

    // If the border matches both of its adjacent sides, don't anti-alias the clip, and
    // if neither side matches, anti-alias the clip.
    if (firstEdgeMatches == secondEdgeMatches) {
        graphicsContext->clipConvexPolygon(4, quad, !firstEdgeMatches);
        return;
    }

    // Square off the end which shouldn't be affected by antialiasing, and clip.
    FloatPoint firstQuad[4];
    firstQuad[0] = quad[0];
    firstQuad[1] = quad[1];
    firstQuad[2] = side == BSTop || side == BSBottom ? FloatPoint(quad[3].x(), quad[2].y())
        : FloatPoint(quad[2].x(), quad[3].y());
    firstQuad[3] = quad[3];
    graphicsContext->clipConvexPolygon(4, firstQuad, !firstEdgeMatches);

    FloatPoint secondQuad[4];
    secondQuad[0] = quad[0];
    secondQuad[1] = side == BSTop || side == BSBottom ? FloatPoint(quad[0].x(), quad[1].y())
        : FloatPoint(quad[1].x(), quad[0].y());
    secondQuad[2] = quad[2];
    secondQuad[3] = quad[3];
    // Antialiasing affects the second side.
    graphicsContext->clipConvexPolygon(4, secondQuad, !secondEdgeMatches);
}

void RenderBoxModelObject::getBorderEdgeInfo(BorderEdge edges[], bool includeLogicalLeftEdge, bool includeLogicalRightEdge) const
{
    const RenderStyle* style = this->style();
    bool horizontal = style->isHorizontalWritingMode();

    edges[BSTop] = BorderEdge(style->borderTopWidth(),
        style->visitedDependentColor(CSSPropertyBorderTopColor),
        style->borderTopStyle(),
        style->borderTopIsTransparent(),
        horizontal || includeLogicalLeftEdge);

    edges[BSRight] = BorderEdge(style->borderRightWidth(),
        style->visitedDependentColor(CSSPropertyBorderRightColor),
        style->borderRightStyle(),
        style->borderRightIsTransparent(),
        !horizontal || includeLogicalRightEdge);

    edges[BSBottom] = BorderEdge(style->borderBottomWidth(),
        style->visitedDependentColor(CSSPropertyBorderBottomColor),
        style->borderBottomStyle(),
        style->borderBottomIsTransparent(),
        horizontal || includeLogicalRightEdge);

    edges[BSLeft] = BorderEdge(style->borderLeftWidth(),
        style->visitedDependentColor(CSSPropertyBorderLeftColor),
        style->borderLeftStyle(),
        style->borderLeftIsTransparent(),
        !horizontal || includeLogicalLeftEdge);
}

bool RenderBoxModelObject::borderObscuresBackgroundEdge(const FloatSize& contextScale) const
{
    BorderEdge edges[4];
    getBorderEdgeInfo(edges);

    for (int i = BSTop; i <= BSLeft; ++i) {
        const BorderEdge& currEdge = edges[i];
        // FIXME: for vertical text
        float axisScale = (i == BSTop || i == BSBottom) ? contextScale.height() : contextScale.width();
        if (!currEdge.obscuresBackgroundEdge(axisScale))
            return false;
    }

    return true;
}

static inline IntRect areaCastingShadowInHole(const IntRect& holeRect, int shadowBlur, int shadowSpread, const IntSize& shadowOffset)
{
    IntRect bounds(holeRect);
    
    bounds.inflate(shadowBlur);

    if (shadowSpread < 0)
        bounds.inflate(-shadowSpread);
    
    IntRect offsetBounds = bounds;
    offsetBounds.move(-shadowOffset);
    return unionRect(bounds, offsetBounds);
}

void RenderBoxModelObject::paintBoxShadow(GraphicsContext* context, int tx, int ty, int w, int h, const RenderStyle* s, ShadowStyle shadowStyle, bool includeLogicalLeftEdge, bool includeLogicalRightEdge)
{
    // FIXME: Deal with border-image.  Would be great to use border-image as a mask.

    if (context->paintingDisabled() || !s->boxShadow())
        return;

    IntRect borderRect(tx, ty, w, h);
    RoundedIntRect border = (shadowStyle == Inset) ? s->getRoundedInnerBorderFor(borderRect, includeLogicalLeftEdge, includeLogicalRightEdge)
                                                   : s->getRoundedBorderFor(borderRect, includeLogicalLeftEdge, includeLogicalRightEdge);

    bool hasBorderRadius = s->hasBorderRadius();
    bool isHorizontal = s->isHorizontalWritingMode();
    
    bool hasOpaqueBackground = s->visitedDependentColor(CSSPropertyBackgroundColor).isValid() && s->visitedDependentColor(CSSPropertyBackgroundColor).alpha() == 255;
    for (const ShadowData* shadow = s->boxShadow(); shadow; shadow = shadow->next()) {
        if (shadow->style() != shadowStyle)
            continue;

        IntSize shadowOffset(shadow->x(), shadow->y());
        int shadowBlur = shadow->blur();
        int shadowSpread = shadow->spread();
        
        if (shadowOffset.isZero() && !shadowBlur && !shadowSpread)
            continue;
        
        const Color& shadowColor = shadow->color();

        if (shadow->style() == Normal) {
            RoundedIntRect fillRect = border;
            fillRect.inflate(shadowSpread);
            if (fillRect.isEmpty())
                continue;

            IntRect shadowRect(border.rect());
            shadowRect.inflate(shadowBlur + shadowSpread);
            shadowRect.move(shadowOffset);

            GraphicsContextStateSaver stateSaver(*context);
            context->clip(shadowRect);

            // Move the fill just outside the clip, adding 1 pixel separation so that the fill does not
            // bleed in (due to antialiasing) if the context is transformed.
            IntSize extraOffset(w + max(0, shadowOffset.width()) + shadowBlur + 2 * shadowSpread + 1, 0);
            shadowOffset -= extraOffset;
            fillRect.move(extraOffset);

            if (shadow->isWebkitBoxShadow())
                context->setLegacyShadow(shadowOffset, shadowBlur, shadowColor, s->colorSpace());
            else
                context->setShadow(shadowOffset, shadowBlur, shadowColor, s->colorSpace());

            if (hasBorderRadius) {
                RoundedIntRect rectToClipOut = border;

                // If the box is opaque, it is unnecessary to clip it out. However, doing so saves time
                // when painting the shadow. On the other hand, it introduces subpixel gaps along the
                // corners. Those are avoided by insetting the clipping path by one pixel.
                if (hasOpaqueBackground) {
                    rectToClipOut.inflateWithRadii(-1);
                }

                if (!rectToClipOut.isEmpty())
                    context->clipOutRoundedRect(rectToClipOut);

                fillRect.expandRadii(shadowSpread);
                context->fillRoundedRect(fillRect, Color::black, s->colorSpace());
            } else {
                IntRect rectToClipOut = border.rect();

                // If the box is opaque, it is unnecessary to clip it out. However, doing so saves time
                // when painting the shadow. On the other hand, it introduces subpixel gaps along the
                // edges if they are not pixel-aligned. Those are avoided by insetting the clipping path
                // by one pixel.
                if (hasOpaqueBackground) {
                    AffineTransform currentTransformation = context->getCTM();
                    if (currentTransformation.a() != 1 || (currentTransformation.d() != 1 && currentTransformation.d() != -1)
                            || currentTransformation.b() || currentTransformation.c())
                        rectToClipOut.inflate(-1);
                }

                if (!rectToClipOut.isEmpty())
                    context->clipOut(rectToClipOut);
                context->fillRect(fillRect.rect(), Color::black, s->colorSpace());
            }
        } else {
            // Inset shadow.
            IntRect holeRect(border.rect());
            holeRect.inflate(-shadowSpread);

            if (holeRect.isEmpty()) {
                if (hasBorderRadius)
                    context->fillRoundedRect(border, shadowColor, s->colorSpace());
                else
                    context->fillRect(border.rect(), shadowColor, s->colorSpace());
                continue;
            }

            if (!includeLogicalLeftEdge) {
                if (isHorizontal) {
                    holeRect.move(-max(shadowOffset.width(), 0) - shadowBlur, 0);
                    holeRect.setWidth(holeRect.width() + max(shadowOffset.width(), 0) + shadowBlur);
                } else {
                    holeRect.move(0, -max(shadowOffset.height(), 0) - shadowBlur);
                    holeRect.setHeight(holeRect.height() + max(shadowOffset.height(), 0) + shadowBlur);
                }
            }
            if (!includeLogicalRightEdge) {
                if (isHorizontal)
                    holeRect.setWidth(holeRect.width() - min(shadowOffset.width(), 0) + shadowBlur);
                else
                    holeRect.setHeight(holeRect.height() - min(shadowOffset.height(), 0) + shadowBlur);
            }

            Color fillColor(shadowColor.red(), shadowColor.green(), shadowColor.blue(), 255);

            IntRect outerRect = areaCastingShadowInHole(border.rect(), shadowBlur, shadowSpread, shadowOffset);
            RoundedIntRect roundedHole(holeRect, border.radii());

            GraphicsContextStateSaver stateSaver(*context);
            if (hasBorderRadius) {
                Path path;
                path.addRoundedRect(border);
                context->clip(path);
                roundedHole.shrinkRadii(shadowSpread);
            } else
                context->clip(border.rect());

            IntSize extraOffset(2 * w + max(0, shadowOffset.width()) + shadowBlur - 2 * shadowSpread + 1, 0);
            context->translate(extraOffset.width(), extraOffset.height());
            shadowOffset -= extraOffset;

            if (shadow->isWebkitBoxShadow())
                context->setLegacyShadow(shadowOffset, shadowBlur, shadowColor, s->colorSpace());
            else
                context->setShadow(shadowOffset, shadowBlur, shadowColor, s->colorSpace());

            context->fillRectWithRoundedHole(outerRect, roundedHole, fillColor, s->colorSpace());
        }
    }
}

int RenderBoxModelObject::containingBlockLogicalWidthForContent() const
{
    return containingBlock()->availableLogicalWidth();
}

RenderBoxModelObject* RenderBoxModelObject::continuation() const
{
    if (!continuationMap)
        return 0;
    return continuationMap->get(this);
}

void RenderBoxModelObject::setContinuation(RenderBoxModelObject* continuation)
{
    if (continuation) {
        if (!continuationMap)
            continuationMap = new ContinuationMap;
        continuationMap->set(this, continuation);
    } else {
        if (continuationMap)
            continuationMap->remove(this);
    }
}

} // namespace WebCore
