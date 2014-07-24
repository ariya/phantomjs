/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef SVGImageForContainer_h
#define SVGImageForContainer_h

#if ENABLE(SVG)

#include "AffineTransform.h"
#include "FloatRect.h"
#include "FloatSize.h"
#include "Image.h"
#include "SVGImage.h"

namespace WebCore {

class SVGImageForContainer : public Image {
public:
    static PassRefPtr<SVGImageForContainer> create(SVGImage* image, const FloatSize& containerSize, float zoom)
    {
        return adoptRef(new SVGImageForContainer(image, containerSize, zoom));
    }

    virtual bool isSVGImage() const OVERRIDE { return true; }

    virtual IntSize size() const OVERRIDE;

    virtual bool usesContainerSize() const OVERRIDE { return m_image->usesContainerSize(); }
    virtual bool hasRelativeWidth() const OVERRIDE { return m_image->hasRelativeWidth(); }
    virtual bool hasRelativeHeight() const OVERRIDE { return m_image->hasRelativeHeight(); }
    virtual void computeIntrinsicDimensions(Length& intrinsicWidth, Length& intrinsicHeight, FloatSize& intrinsicRatio) OVERRIDE
    {
        m_image->computeIntrinsicDimensions(intrinsicWidth, intrinsicHeight, intrinsicRatio);
    }

    virtual void draw(GraphicsContext*, const FloatRect&, const FloatRect&, ColorSpace, CompositeOperator, BlendMode) OVERRIDE;

    virtual void drawPattern(GraphicsContext*, const FloatRect&, const AffineTransform&, const FloatPoint&, ColorSpace, CompositeOperator, const FloatRect&, BlendMode) OVERRIDE;

    // FIXME: Implement this to be less conservative.
    virtual bool currentFrameKnownToBeOpaque() OVERRIDE { return false; }

    virtual PassNativeImagePtr nativeImageForCurrentFrame() OVERRIDE;

private:
    SVGImageForContainer(SVGImage* image, const FloatSize& containerSize, float zoom)
        : m_image(image)
        , m_containerSize(containerSize)
        , m_zoom(zoom)
    {
    }

    virtual void destroyDecodedData(bool /*destroyAll*/ = true) { }
    virtual unsigned decodedSize() const { return 0; }

    SVGImage* m_image;
    const FloatSize m_containerSize;
    const float m_zoom;
};
}

#endif // ENABLE(SVG)
#endif // SVGImageForContainer_h
