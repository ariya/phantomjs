/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#include "WebTiledLayer.h"

#include "GraphicsLayer.h"
#include "WKCACFLayerRenderer.h"

namespace WebCore {

using namespace std;

#ifndef NDEBUG
void WebTiledLayer::internalCheckLayerConsistency()
{
    WKCACFLayer::internalCheckLayerConsistency();

    // Additionally make sure the tiled parent is valid
    CFArrayRef sublayers = CACFLayerGetSublayers(layer());

    // Make sure there is a tile parent and it is the same as we remember
    size_t n = CFArrayGetCount(sublayers);
    ASSERT(n > 0);
    const void* element = CFArrayGetValueAtIndex(sublayers, 0);
    ASSERT(m_tileParent.get() == element);

    // Make sure the tile parent doesn't have user data. If it does, it is probably
    // a WKCACFLayer in the wrong place.
    ASSERT(!layer(m_tileParent.get()));
}
#endif

void WebTiledLayer::tileDisplayCallback(CACFLayerRef layer, CGContextRef context)
{
    static_cast<WebTiledLayer*>(CACFLayerGetUserData(layer))->drawTile(layer, context);
}

PassRefPtr<WebTiledLayer> WebTiledLayer::create(const CGSize& tileSize, GraphicsLayer* owner)
{
    ASSERT(WKCACFLayerRenderer::acceleratedCompositingAvailable());
    return adoptRef(new WebTiledLayer(tileSize, owner));
}

WebTiledLayer::WebTiledLayer(const CGSize& tileSize, GraphicsLayer* owner)
    : WebLayer(WKCACFLayer::Layer, owner)
    , m_tileSize(tileSize)
    , m_constrainedSize(constrainedSize(bounds().size))
{
    // Tiled layers are placed in a child layer that is always the first child of the TiledLayer
    m_tileParent.adoptCF(CACFLayerCreate(kCACFLayer));
    CACFLayerInsertSublayer(layer(), m_tileParent.get(), 0);

    updateTiles();
}

WebTiledLayer::~WebTiledLayer()
{
}

void WebTiledLayer::setBounds(const CGRect& rect)
{
    if (CGRectEqualToRect(rect, bounds()))
        return;

    WebLayer::setBounds(rect);
    m_constrainedSize = constrainedSize(rect.size);
    updateTiles();
}

void WebTiledLayer::setFrame(const CGRect& rect)
{
    if (CGRectEqualToRect(rect, frame()))
        return;

    WebLayer::setFrame(rect);
    updateTiles();
}

void WebTiledLayer::internalSetNeedsDisplay(const CGRect* dirtyRect)
{
    // FIXME: Only setNeedsDisplay for tiles that are currently visible
    int numTileLayers = tileCount();
    for (int i = 0; i < numTileLayers; ++i)
        CACFLayerSetNeedsDisplay(tileAtIndex(i), dirtyRect);

    if (m_owner->showRepaintCounter()) {
        CGRect layerBounds = bounds();
        CGRect indicatorRect = CGRectMake(layerBounds.origin.x, layerBounds.origin.y, 80, 25);
        CACFLayerSetNeedsDisplay(tileAtIndex(0), &indicatorRect);
    }
}

size_t WebTiledLayer::internalSublayerCount() const
{
    ASSERT(WebLayer::internalSublayerCount() > 0);

    // Subtract 1 to account for the tile parent layer
    return WebLayer::internalSublayerCount() - 1;
}

void WebTiledLayer::internalRemoveAllSublayers()
{
    // Restore the tile parent after removal
    WebLayer::internalRemoveAllSublayers();
    CACFLayerInsertSublayer(layer(), m_tileParent.get(), 0);
}

void WebTiledLayer::internalSetSublayers(const Vector<RefPtr<WKCACFLayer> >& sublayers)
{
    // Preserve the tile parent after set
    WebLayer::internalSetSublayers(sublayers);
    CACFLayerInsertSublayer(layer(), m_tileParent.get(), 0);
}

void WebTiledLayer::internalInsertSublayer(PassRefPtr<WKCACFLayer> layer, size_t index)
{
    // Add 1 to account for the tile parent layer
    WebLayer::internalInsertSublayer(layer, index + 1);
}

WKCACFLayer* WebTiledLayer::internalSublayerAtIndex(int i) const
{
    // Add 1 to account for the tile parent layer
    return WebLayer::internalSublayerAtIndex(i + 1);
}

int WebTiledLayer::internalIndexOfSublayer(const WKCACFLayer* layer)
{
    int i = WebLayer::internalIndexOfSublayer(layer);

    // Add 1 to account for the tile parent layer (but be safe about it)
    return (i > 0) ? i - 1 : -1;
}

CGSize WebTiledLayer::constrainedSize(const CGSize& size) const
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

void WebTiledLayer::addTile()
{
    RetainPtr<CACFLayerRef> newLayer(AdoptCF, CACFLayerCreate(kCACFLayer));
    CACFLayerSetAnchorPoint(newLayer.get(), CGPointMake(0, 1));
    CACFLayerSetUserData(newLayer.get(), this);
    CACFLayerSetDisplayCallback(newLayer.get(), tileDisplayCallback);

    CFArrayRef sublayers = CACFLayerGetSublayers(m_tileParent.get());
    CACFLayerInsertSublayer(m_tileParent.get(), newLayer.get(), sublayers ? CFArrayGetCount(sublayers) : 0);

    if (m_owner->showDebugBorders()) {
        CGColorRef borderColor = CGColorCreateGenericRGB(0.5, 0, 0.5, 0.7);
        CACFLayerSetBorderColor(newLayer.get(), borderColor);
        CGColorRelease(borderColor);
        CACFLayerSetBorderWidth(newLayer.get(), 2);
    }
}

void WebTiledLayer::removeTile()
{
    CACFLayerRemoveFromSuperlayer(tileAtIndex(tileCount() - 1));
}

CACFLayerRef WebTiledLayer::tileAtIndex(int index)
{
    CFArrayRef sublayers = CACFLayerGetSublayers(m_tileParent.get());
    if (!sublayers || index < 0 || index >= tileCount() )
        return 0;
    
    return static_cast<CACFLayerRef>(const_cast<void*>(CFArrayGetValueAtIndex(sublayers, index)));
}

int WebTiledLayer::tileCount() const
{
    CFArrayRef sublayers = CACFLayerGetSublayers(m_tileParent.get());
    return sublayers ? CFArrayGetCount(sublayers) : 0;
}

void WebTiledLayer::updateTiles()
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

void WebTiledLayer::drawTile(CACFLayerRef tile, CGContextRef context)
{
    CGPoint tilePosition = CACFLayerGetPosition(tile);
    CGRect tileBounds = CACFLayerGetBounds(tile);

    CGContextSaveGState(context);

    // Transform context to be at the origin of the parent layer
    CGContextTranslateCTM(context, -tilePosition.x, -tilePosition.y);

    // Set the context clipping rectangle to the current tile
    CGContextClipToRect(context, CGRectMake(tilePosition.x, tilePosition.y, tileBounds.size.width, tileBounds.size.height));

    if (m_owner->contentsOrientation() == WebCore::GraphicsLayer::CompositingCoordinatesTopDown) {
        // If the layer is rendering top-down, it will flip the coordinates in y. Tiled layers are
        // already flipping, so we need to undo that here.
        CGContextTranslateCTM(context, 0, bounds().size.height);
        CGContextScaleCTM(context, 1, -1);
    }

    // Draw the tile
    drawInContext(context);

    CGContextRestoreGState(context);
}

}

#endif // USE(ACCELERATED_COMPOSITING)
