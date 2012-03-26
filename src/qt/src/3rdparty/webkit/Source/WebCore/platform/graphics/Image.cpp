/*
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
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
#include "Image.h"

#include "AffineTransform.h"
#include "BitmapImage.h"
#include "GraphicsContext.h"
#include "IntRect.h"
#include "MIMETypeRegistry.h"
#include "SharedBuffer.h"
#include <math.h>
#include <wtf/StdLibExtras.h>

#if USE(CG)
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace WebCore {

Image::Image(ImageObserver* observer)
    : m_imageObserver(observer)
{
}

Image::~Image()
{
}

Image* Image::nullImage()
{
    ASSERT(isMainThread());
    DEFINE_STATIC_LOCAL(RefPtr<Image>, nullImage, (BitmapImage::create()));;
    return nullImage.get();
}

bool Image::supportsType(const String& type)
{
    return MIMETypeRegistry::isSupportedImageResourceMIMEType(type); 
} 

bool Image::setData(PassRefPtr<SharedBuffer> data, bool allDataReceived)
{
    m_data = data;
    if (!m_data.get())
        return true;

    int length = m_data->size();
    if (!length)
        return true;
    
    return dataChanged(allDataReceived);
}

void Image::fillWithSolidColor(GraphicsContext* ctxt, const FloatRect& dstRect, const Color& color, ColorSpace styleColorSpace, CompositeOperator op)
{
    if (color.alpha() <= 0)
        return;
    
    CompositeOperator previousOperator = ctxt->compositeOperation();
    ctxt->setCompositeOperation(!color.hasAlpha() && op == CompositeSourceOver ? CompositeCopy : op);
    ctxt->fillRect(dstRect, color, styleColorSpace);
    ctxt->setCompositeOperation(previousOperator);
}

static inline FloatSize calculatePatternScale(const FloatRect& dstRect, const FloatRect& srcRect, Image::TileRule hRule, Image::TileRule vRule)
{
    float scaleX = 1.0f, scaleY = 1.0f;
    
    if (hRule == Image::StretchTile)
        scaleX = dstRect.width() / srcRect.width();
    if (vRule == Image::StretchTile)
        scaleY = dstRect.height() / srcRect.height();
    
    if (hRule == Image::RepeatTile)
        scaleX = scaleY;
    if (vRule == Image::RepeatTile)
        scaleY = scaleX;
    
    return FloatSize(scaleX, scaleY);
}


void Image::drawTiled(GraphicsContext* ctxt, const FloatRect& destRect, const FloatPoint& srcPoint, const FloatSize& scaledTileSize, ColorSpace styleColorSpace, CompositeOperator op)
{    
    if (mayFillWithSolidColor()) {
        fillWithSolidColor(ctxt, destRect, solidColor(), styleColorSpace, op);
        return;
    }

    // See <https://webkit.org/b/59043>.
#if !PLATFORM(WX)
    ASSERT(!isBitmapImage() || static_cast<BitmapImage*>(this)->notSolidColor());
#endif

    FloatSize intrinsicTileSize = size();
    if (hasRelativeWidth())
        intrinsicTileSize.setWidth(scaledTileSize.width());
    if (hasRelativeHeight())
        intrinsicTileSize.setHeight(scaledTileSize.height());

    FloatSize scale(scaledTileSize.width() / intrinsicTileSize.width(),
                    scaledTileSize.height() / intrinsicTileSize.height());

    FloatRect oneTileRect;
    oneTileRect.setX(destRect.x() + fmodf(fmodf(-srcPoint.x(), scaledTileSize.width()) - scaledTileSize.width(), scaledTileSize.width()));
    oneTileRect.setY(destRect.y() + fmodf(fmodf(-srcPoint.y(), scaledTileSize.height()) - scaledTileSize.height(), scaledTileSize.height()));
    oneTileRect.setSize(scaledTileSize);
    
    // Check and see if a single draw of the image can cover the entire area we are supposed to tile.    
    if (oneTileRect.contains(destRect)) {
        FloatRect visibleSrcRect;
        visibleSrcRect.setX((destRect.x() - oneTileRect.x()) / scale.width());
        visibleSrcRect.setY((destRect.y() - oneTileRect.y()) / scale.height());
        visibleSrcRect.setWidth(destRect.width() / scale.width());
        visibleSrcRect.setHeight(destRect.height() / scale.height());
        draw(ctxt, destRect, visibleSrcRect, styleColorSpace, op);
        return;
    }

    AffineTransform patternTransform = AffineTransform().scaleNonUniform(scale.width(), scale.height());
    FloatRect tileRect(FloatPoint(), intrinsicTileSize);    
    drawPattern(ctxt, tileRect, patternTransform, oneTileRect.location(), styleColorSpace, op, destRect);
    
    startAnimation();
}

// FIXME: Merge with the other drawTiled eventually, since we need a combination of both for some things.
void Image::drawTiled(GraphicsContext* ctxt, const FloatRect& dstRect, const FloatRect& srcRect, TileRule hRule, TileRule vRule, ColorSpace styleColorSpace, CompositeOperator op)
{    
    if (mayFillWithSolidColor()) {
        fillWithSolidColor(ctxt, dstRect, solidColor(), styleColorSpace, op);
        return;
    }
    
    // FIXME: We do not support 'round' yet.  For now just map it to 'repeat'.
    if (hRule == RoundTile)
        hRule = RepeatTile;
    if (vRule == RoundTile)
        vRule = RepeatTile;

    FloatSize scale = calculatePatternScale(dstRect, srcRect, hRule, vRule);
    AffineTransform patternTransform = AffineTransform().scaleNonUniform(scale.width(), scale.height());

    // We want to construct the phase such that the pattern is centered (when stretch is not
    // set for a particular rule).
    float hPhase = scale.width() * srcRect.x();
    float vPhase = scale.height() * srcRect.y();
    if (hRule == Image::RepeatTile)
        hPhase -= fmodf(dstRect.width(), scale.width() * srcRect.width()) / 2.0f;
    if (vRule == Image::RepeatTile)
        vPhase -= fmodf(dstRect.height(), scale.height() * srcRect.height()) / 2.0f;
    FloatPoint patternPhase(dstRect.x() - hPhase, dstRect.y() - vPhase);
    
    drawPattern(ctxt, srcRect, patternTransform, patternPhase, styleColorSpace, op, dstRect);

    startAnimation();
}


}
