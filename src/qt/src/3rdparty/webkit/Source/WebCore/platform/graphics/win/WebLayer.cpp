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

#include "WebLayer.h"

#include "Font.h"
#include "GraphicsLayer.h"

namespace WebCore {

using namespace std;

void WebLayer::internalSetNeedsDisplay(const CGRect* dirtyRect)
{
    if (m_owner) {
        if (m_owner->showRepaintCounter()) {
            CGRect layerBounds = bounds();
            CGRect repaintCounterRect = layerBounds;
            // We assume a maximum of 4 digits and a font size of 18.
            repaintCounterRect.size.width = 80;
            repaintCounterRect.size.height = 22;
            if (m_owner->contentsOrientation() == WebCore::GraphicsLayer::CompositingCoordinatesTopDown)
                repaintCounterRect.origin.y = layerBounds.size.height - (layerBounds.origin.y + repaintCounterRect.size.height);
            WKCACFLayer::internalSetNeedsDisplay(&repaintCounterRect);
        }
        if (dirtyRect && m_owner->contentsOrientation() == WebCore::GraphicsLayer::CompositingCoordinatesTopDown) {
            CGRect flippedDirtyRect = *dirtyRect;
            flippedDirtyRect.origin.y = bounds().size.height - (flippedDirtyRect.origin.y + flippedDirtyRect.size.height);
            WKCACFLayer::internalSetNeedsDisplay(&flippedDirtyRect);
            return;
        }
    }

    WKCACFLayer::internalSetNeedsDisplay(dirtyRect);
}

void WebLayer::drawInContext(PlatformGraphicsContext* context)
{
    if (!m_owner)
        return;

    CGContextSaveGState(context);

    CGRect layerBounds = bounds();
    if (m_owner->contentsOrientation() == WebCore::GraphicsLayer::CompositingCoordinatesTopDown) {
        CGContextScaleCTM(context, 1, -1);
        CGContextTranslateCTM(context, 0, -layerBounds.size.height);
    }

    if (m_owner->client()) {
        GraphicsContext graphicsContext(context);

        // It's important to get the clip from the context, because it may be significantly
        // smaller than the layer bounds (e.g. tiled layers)
        CGRect clipBounds = CGContextGetClipBoundingBox(context);
        IntRect clip(enclosingIntRect(clipBounds));
        m_owner->paintGraphicsLayerContents(graphicsContext, clip);
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

    if (m_owner->showRepaintCounter()) {
        String text = String::number(m_owner->incrementRepaintCount());

        CGContextSaveGState(context);

        // Make the background of the counter the same as the border color,
        // unless there is no border, then make it red
        float borderWidth = CACFLayerGetBorderWidth(layer());
        if (borderWidth > 0) {
            CGColorRef borderColor = CACFLayerGetBorderColor(layer());
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
}

}

#endif // USE(ACCELERATED_COMPOSITING)
