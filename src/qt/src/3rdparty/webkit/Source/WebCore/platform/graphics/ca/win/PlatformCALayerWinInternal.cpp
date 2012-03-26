/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#if USE(ACCELERATED_COMPOSITING)

#include "PlatformCALayerWinInternal.h"

#include "Font.h"
#include "PlatformCALayer.h"
#include "TextRun.h"
#include <QuartzCore/CACFLayer.h>

using namespace std;
using namespace WebCore;

// The width and height of a single tile in a tiled layer. Should be large enough to
// avoid lots of small tiles (and therefore lots of drawing callbacks), but small enough
// to keep the overall tile cost low.
static const int cTiledLayerTileSize = 512;

PlatformCALayerWinInternal::PlatformCALayerWinInternal(PlatformCALayer* owner)
    : m_tileSize(CGSizeMake(cTiledLayerTileSize, cTiledLayerTileSize))
    , m_constrainedSize(constrainedSize(owner->bounds().size()))
    , m_owner(owner)
{
    if (m_owner->layerType() == PlatformCALayer::LayerTypeWebTiledLayer) {
        // Tiled layers are placed in a child layer that is always the first child of the TiledLayer
        m_tileParent.adoptCF(CACFLayerCreate(kCACFLayer));
        CACFLayerInsertSublayer(m_owner->platformLayer(), m_tileParent.get(), 0);
        updateTiles();
    }
}

PlatformCALayerWinInternal::~PlatformCALayerWinInternal()
{
}

void PlatformCALayerWinInternal::displayCallback(CACFLayerRef caLayer, CGContextRef context)
{
    if (!owner() || !owner()->owner())
        return;

    CGContextSaveGState(context);

    CGRect layerBounds = owner()->bounds();
    if (owner()->owner()->platformCALayerContentsOrientation() == WebCore::GraphicsLayer::CompositingCoordinatesTopDown) {
        CGContextScaleCTM(context, 1, -1);
        CGContextTranslateCTM(context, 0, -layerBounds.size.height);
    }

    if (owner()->owner()) {
        GraphicsContext graphicsContext(context);

        // It's important to get the clip from the context, because it may be significantly
        // smaller than the layer bounds (e.g. tiled layers)
        CGRect clipBounds = CGContextGetClipBoundingBox(context);
        IntRect clip(enclosingIntRect(clipBounds));
        owner()->owner()->platformCALayerPaintContents(graphicsContext, clip);
    }
#ifndef NDEBUG
    else {
        ASSERT_NOT_REACHED();

        // FIXME: ideally we'd avoid calling -setNeedsDisplay on a layer that is a plain color,
        // so CA never makes backing store for it (which is what -setNeedsDisplay will do above).
        CGContextSetRGBFillColor(context, 0.0f, 1.0f, 0.0f, 1.0f);
        CGContextFillRect(context, layerBounds);
    }
#endif

    if (owner()->owner()->platformCALayerShowRepaintCounter()) {
        String text = String::number(owner()->owner()->platformCALayerIncrementRepaintCount());

        CGContextSaveGState(context);

        // Make the background of the counter the same as the border color,
        // unless there is no border, then make it red
        float borderWidth = CACFLayerGetBorderWidth(caLayer);
        if (borderWidth > 0) {
            CGColorRef borderColor = CACFLayerGetBorderColor(caLayer);
            const CGFloat* colors = CGColorGetComponents(borderColor);
            CGContextSetRGBFillColor(context, colors[0], colors[1], colors[2], colors[3]);
        } else
            CGContextSetRGBFillColor(context, 1.0f, 0.0f, 0.0f, 0.8f);
        
        CGRect aBounds = layerBounds;

        aBounds.size.width = 10 + 10 * text.length();
        aBounds.size.height = 22;
        CGContextFillRect(context, aBounds);
        
        FontDescription desc;

        NONCLIENTMETRICS metrics;
        metrics.cbSize = sizeof(metrics);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, metrics.cbSize, &metrics, 0);
        FontFamily family;
        family.setFamily(metrics.lfSmCaptionFont.lfFaceName);
        desc.setFamily(family);

        desc.setComputedSize(18);
        
        Font font = Font(desc, 0, 0);
        font.update(0);

        GraphicsContext cg(context);
        cg.setFillColor(Color::black, ColorSpaceDeviceRGB);
        cg.drawText(font, TextRun(text), IntPoint(aBounds.origin.x + 5, aBounds.origin.y + 17));

        CGContextRestoreGState(context);        
    }

    CGContextRestoreGState(context);

    owner()->owner()->platformCALayerLayerDidDisplay(caLayer);
}

void PlatformCALayerWinInternal::internalSetNeedsDisplay(const FloatRect* dirtyRect)
{
    if (dirtyRect) {
        CGRect rect = *dirtyRect;
        CACFLayerSetNeedsDisplay(owner()->platformLayer(), &rect);
    } else
        CACFLayerSetNeedsDisplay(owner()->platformLayer(), 0);
}

void PlatformCALayerWinInternal::setNeedsDisplay(const FloatRect* dirtyRect)
{
    if (owner()->layerType() == PlatformCALayer::LayerTypeWebTiledLayer) {
        // FIXME: Only setNeedsDisplay for tiles that are currently visible
        int numTileLayers = tileCount();
        CGRect rect;
        if (dirtyRect)
            rect = *dirtyRect;
        for (int i = 0; i < numTileLayers; ++i)
            CACFLayerSetNeedsDisplay(tileAtIndex(i), dirtyRect ? &rect : 0);

        if (m_owner->owner() && m_owner->owner()->platformCALayerShowRepaintCounter()) {
            CGRect layerBounds = m_owner->bounds();
            CGRect indicatorRect = CGRectMake(layerBounds.origin.x, layerBounds.origin.y, 80, 25);
            CACFLayerSetNeedsDisplay(tileAtIndex(0), &indicatorRect);
        }
    } else if (owner()->layerType() == PlatformCALayer::LayerTypeWebLayer) {
        if (owner() && owner()->owner()) {
            if (owner()->owner()->platformCALayerShowRepaintCounter()) {
                FloatRect layerBounds = owner()->bounds();
                FloatRect repaintCounterRect = layerBounds;

                // We assume a maximum of 4 digits and a font size of 18.
                repaintCounterRect.setWidth(80);
                repaintCounterRect.setHeight(22);
                if (owner()->owner()->platformCALayerContentsOrientation() == WebCore::GraphicsLayer::CompositingCoordinatesTopDown)
                    repaintCounterRect.setY(layerBounds.height() - (layerBounds.y() + repaintCounterRect.height()));
                internalSetNeedsDisplay(&repaintCounterRect);
            }
            if (dirtyRect && owner()->owner()->platformCALayerContentsOrientation() == WebCore::GraphicsLayer::CompositingCoordinatesTopDown) {
                FloatRect flippedDirtyRect = *dirtyRect;
                flippedDirtyRect.setY(owner()->bounds().height() - (flippedDirtyRect.y() + flippedDirtyRect.height()));
                internalSetNeedsDisplay(&flippedDirtyRect);
                return;
            }
        }

        internalSetNeedsDisplay(dirtyRect);
    }
    owner()->setNeedsCommit();
}

void PlatformCALayerWinInternal::setSublayers(const PlatformCALayerList& list)
{
    // Remove all the current sublayers and add the passed layers
    CACFLayerSetSublayers(owner()->platformLayer(), 0);

    // Perform removeFromSuperLayer in a separate pass. CACF requires superlayer to
    // be null or CACFLayerInsertSublayer silently fails.
    for (size_t i = 0; i < list.size(); i++)
        CACFLayerRemoveFromSuperlayer(list[i]->platformLayer());

    for (size_t i = 0; i < list.size(); i++)
        CACFLayerInsertSublayer(owner()->platformLayer(), list[i]->platformLayer(), i);

    owner()->setNeedsCommit();

    if (owner()->layerType() == PlatformCALayer::LayerTypeWebTiledLayer) {
        // Preserve the tile parent after set
        CACFLayerInsertSublayer(owner()->platformLayer(), m_tileParent.get(), 0);
    }
}

void PlatformCALayerWinInternal::getSublayers(PlatformCALayerList& list) const
{
    CFArrayRef sublayers = CACFLayerGetSublayers(owner()->platformLayer());
    if (!sublayers) {
        list.clear();
        return;
    }

    size_t count = CFArrayGetCount(sublayers);

    size_t layersToSkip = 0;
    if (owner()->layerType() == PlatformCALayer::LayerTypeWebTiledLayer) {
        // Exclude the tile parent layer.
        layersToSkip = 1;
    }

    list.resize(count - layersToSkip);
    for (size_t arrayIndex = layersToSkip; arrayIndex < count; ++arrayIndex)
        list[arrayIndex - layersToSkip] = PlatformCALayer::platformCALayer(const_cast<void*>(CFArrayGetValueAtIndex(sublayers, arrayIndex)));
}

void PlatformCALayerWinInternal::removeAllSublayers()
{
    CACFLayerSetSublayers(owner()->platformLayer(), 0);
    owner()->setNeedsCommit();

    if (owner()->layerType() == PlatformCALayer::LayerTypeWebTiledLayer) {
        // Restore the tile parent after removal
        CACFLayerInsertSublayer(owner()->platformLayer(), m_tileParent.get(), 0);
    }
}

void PlatformCALayerWinInternal::insertSublayer(PlatformCALayer* layer, size_t index)
{
    index = min(index, sublayerCount());
    if (owner()->layerType() == PlatformCALayer::LayerTypeWebTiledLayer) {
        // Add 1 to account for the tile parent layer
        index++;
    }

    layer->removeFromSuperlayer();
    CACFLayerInsertSublayer(owner()->platformLayer(), layer->platformLayer(), index);
    owner()->setNeedsCommit();
}

size_t PlatformCALayerWinInternal::sublayerCount() const
{
    CFArrayRef sublayers = CACFLayerGetSublayers(owner()->platformLayer());
    size_t count = sublayers ? CFArrayGetCount(sublayers) : 0;

    if (owner()->layerType() == PlatformCALayer::LayerTypeWebTiledLayer) {
        // Subtract 1 to account for the tile parent layer
        ASSERT(count > 0);
        count--;
    }

    return count;
}

int PlatformCALayerWinInternal::indexOfSublayer(const PlatformCALayer* reference)
{
    CACFLayerRef ref = reference->platformLayer();
    if (!ref)
        return -1;

    CFArrayRef sublayers = CACFLayerGetSublayers(owner()->platformLayer());
    if (!sublayers)
        return -1;

    size_t n = CFArrayGetCount(sublayers);

    if (owner()->layerType() == PlatformCALayer::LayerTypeWebTiledLayer) {
        for (size_t i = 1; i < n; ++i) {
            if (CFArrayGetValueAtIndex(sublayers, i) == ref)
                return i - 1;
        }
    } else {
        for (size_t i = 0; i < n; ++i) {
            if (CFArrayGetValueAtIndex(sublayers, i) == ref)
                return i;
        }
    }

    return -1;
}

PlatformCALayer* PlatformCALayerWinInternal::sublayerAtIndex(int index) const
{
    if (owner()->layerType() == PlatformCALayer::LayerTypeWebTiledLayer) {
        // Add 1 to account for the tile parent layer
        index++;
    }

    CFArrayRef sublayers = CACFLayerGetSublayers(owner()->platformLayer());
    if (!sublayers || index < 0 || CFArrayGetCount(sublayers) <= index)
        return 0;
    
    return PlatformCALayer::platformCALayer(static_cast<CACFLayerRef>(const_cast<void*>(CFArrayGetValueAtIndex(sublayers, index))));
}

void PlatformCALayerWinInternal::setBounds(const FloatRect& rect)
{
    if (CGRectEqualToRect(rect, owner()->bounds()))
        return;

    CACFLayerSetBounds(owner()->platformLayer(), rect);
    owner()->setNeedsCommit();

    if (owner()->layerType() == PlatformCALayer::LayerTypeWebTiledLayer) {
        m_constrainedSize = constrainedSize(rect.size());
        updateTiles();
    }
}

void PlatformCALayerWinInternal::setFrame(const FloatRect& rect)
{
    CGRect oldFrame = owner()->frame();
    if (CGRectEqualToRect(rect, oldFrame))
        return;

    CACFLayerSetFrame(owner()->platformLayer(), rect);
    owner()->setNeedsCommit();

    if (owner()->layerType() == PlatformCALayer::LayerTypeWebTiledLayer)
        updateTiles();
}

CGSize PlatformCALayerWinInternal::constrainedSize(const CGSize& size) const
{
    const int cMaxTileCount = 512;
    const float cSqrtMaxTileCount = sqrtf(cMaxTileCount);

    CGSize constrainedSize = size;

    int tileColumns = ceilf(constrainedSize.width / m_tileSize.width);
    int tileRows = ceilf(constrainedSize.height / m_tileSize.height);
    int numTiles = tileColumns * tileRows;

    // If number of tiles vertically or horizontally is < sqrt(cMaxTileCount)
    // just shorten the longer dimension. Otherwise shorten both dimensions
    // according to the ratio of width to height

    if (numTiles > cMaxTileCount) {
        if (tileRows < cSqrtMaxTileCount)
            tileColumns = floorf(cMaxTileCount / tileRows);
        else if (tileColumns < cSqrtMaxTileCount)
            tileRows = floorf(cMaxTileCount / tileColumns);
        else {
            tileRows = ceilf(sqrtf(cMaxTileCount * constrainedSize.height / constrainedSize.width));
            tileColumns = floorf(cMaxTileCount / tileRows);
        }
        
        constrainedSize.width = tileColumns * m_tileSize.width;
        constrainedSize.height = tileRows * m_tileSize.height;
    }
    
    return constrainedSize;
}

void PlatformCALayerWinInternal::tileDisplayCallback(CACFLayerRef layer, CGContextRef context)
{
    static_cast<PlatformCALayerWinInternal*>(CACFLayerGetUserData(layer))->drawTile(layer, context);
}

void PlatformCALayerWinInternal::addTile()
{
    RetainPtr<CACFLayerRef> newLayer(AdoptCF, CACFLayerCreate(kCACFLayer));
    CACFLayerSetAnchorPoint(newLayer.get(), CGPointMake(0, 1));
    CACFLayerSetUserData(newLayer.get(), this);
    CACFLayerSetDisplayCallback(newLayer.get(), tileDisplayCallback);

    CFArrayRef sublayers = CACFLayerGetSublayers(m_tileParent.get());
    CACFLayerInsertSublayer(m_tileParent.get(), newLayer.get(), sublayers ? CFArrayGetCount(sublayers) : 0);

    if (owner()->owner()->platformCALayerShowDebugBorders()) {
        CGColorRef borderColor = CGColorCreateGenericRGB(0.5, 0, 0.5, 0.7);
        CACFLayerSetBorderColor(newLayer.get(), borderColor);
        CGColorRelease(borderColor);
        CACFLayerSetBorderWidth(newLayer.get(), 2);
    }
}

void PlatformCALayerWinInternal::removeTile()
{
    CACFLayerRemoveFromSuperlayer(tileAtIndex(tileCount() - 1));
}

CACFLayerRef PlatformCALayerWinInternal::tileAtIndex(int index)
{
    CFArrayRef sublayers = CACFLayerGetSublayers(m_tileParent.get());
    if (!sublayers || index < 0 || index >= tileCount())
        return 0;
    
    return static_cast<CACFLayerRef>(const_cast<void*>(CFArrayGetValueAtIndex(sublayers, index)));
}

int PlatformCALayerWinInternal::tileCount() const
{
    CFArrayRef sublayers = CACFLayerGetSublayers(m_tileParent.get());
    return sublayers ? CFArrayGetCount(sublayers) : 0;
}

void PlatformCALayerWinInternal::updateTiles()
{
    // FIXME: In addition to redoing the number of tiles, we need to only render and have backing
    // store for visible layers
    int numTilesHorizontal = ceil(m_constrainedSize.width / m_tileSize.width);
    int numTilesVertical = ceil(m_constrainedSize.height / m_tileSize.height);
    int numTilesTotal = numTilesHorizontal * numTilesVertical;

    int numTilesToChange = numTilesTotal - tileCount();
    if (numTilesToChange >= 0) {
        // Add new tiles
        for (int i = 0; i < numTilesToChange; ++i)
            addTile();
    } else {
        // Remove old tiles
        numTilesToChange = -numTilesToChange;
        for (int i = 0; i < numTilesToChange; ++i)
            removeTile();
    }

    // Set coordinates for all tiles
    CFArrayRef tileArray = CACFLayerGetSublayers(m_tileParent.get());

    for (int i = 0; i < numTilesHorizontal; ++i) {
        for (int j = 0; j < numTilesVertical; ++j) {
            CACFLayerRef tile = static_cast<CACFLayerRef>(const_cast<void*>(CFArrayGetValueAtIndex(tileArray, i * numTilesVertical + j)));
            CACFLayerSetPosition(tile, CGPointMake(i * m_tileSize.width, j * m_tileSize.height));
            int width = min(m_tileSize.width, m_constrainedSize.width - i * m_tileSize.width);
            int height = min(m_tileSize.height, m_constrainedSize.height - j * m_tileSize.height);
            CACFLayerSetBounds(tile, CGRectMake(i * m_tileSize.width, j * m_tileSize.height, width, height));

            // Flip Y to compensate for the flipping that happens during render to match the CG context coordinate space
            CATransform3D transform = CATransform3DMakeScale(1, -1, 1);
            CATransform3DTranslate(transform, 0, height, 0);
            CACFLayerSetTransform(tile, transform);

#ifndef NDEBUG
            String name = "Tile (" + String::number(i) + "," + String::number(j) + ")";
            CACFLayerSetName(tile, RetainPtr<CFStringRef>(AdoptCF, name.createCFString()).get());
#endif
        }
    }
}

void PlatformCALayerWinInternal::drawTile(CACFLayerRef tile, CGContextRef context)
{
    CGPoint tilePosition = CACFLayerGetPosition(tile);
    CGRect tileBounds = CACFLayerGetBounds(tile);

    CGContextSaveGState(context);

    // Transform context to be at the origin of the parent layer
    CGContextTranslateCTM(context, -tilePosition.x, -tilePosition.y);

    // Set the context clipping rectangle to the current tile
    CGContextClipToRect(context, CGRectMake(tilePosition.x, tilePosition.y, tileBounds.size.width, tileBounds.size.height));

    if (owner()->owner()->platformCALayerContentsOrientation() == WebCore::GraphicsLayer::CompositingCoordinatesTopDown) {
        // If the layer is rendering top-down, it will flip the coordinates in y. Tiled layers are
        // already flipping, so we need to undo that here.
        CGContextTranslateCTM(context, 0, owner()->bounds().height());
        CGContextScaleCTM(context, 1, -1);
    }

    // Draw the tile
    displayCallback(owner()->platformLayer(), context);

    CGContextRestoreGState(context);
}

#endif // USE(ACCELERATED_COMPOSITING)
