/*
 * Copyright (C) 2006 Eric Seidel <eric@webkit.org>
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef SVGImage_h
#define SVGImage_h

#if ENABLE(SVG)

#include "Image.h"

namespace WebCore {

class Element;
class FrameView;
class ImageBuffer;
class Page;
class RenderBox;
class SVGImageChromeClient;
class SVGImageForContainer;

class SVGImage : public Image {
public:
    static PassRefPtr<SVGImage> create(ImageObserver* observer)
    {
        return adoptRef(new SVGImage(observer));
    }

    RenderBox* embeddedContentBox() const;
    FrameView* frameView() const;

    virtual bool isSVGImage() const { return true; }
    virtual IntSize size() const OVERRIDE { return m_intrinsicSize; }

    virtual bool hasSingleSecurityOrigin() const OVERRIDE;

    virtual bool hasRelativeWidth() const;
    virtual bool hasRelativeHeight() const;

    virtual void startAnimation(bool /*catchUpIfNecessary*/ = true) OVERRIDE;
    virtual void stopAnimation() OVERRIDE;
    virtual void resetAnimation() OVERRIDE;

#if USE(CAIRO)
    virtual PassNativeImagePtr nativeImageForCurrentFrame() OVERRIDE;
#endif

private:
    friend class SVGImageChromeClient;
    friend class SVGImageForContainer;

    virtual ~SVGImage();

    virtual String filenameExtension() const;

    virtual void setContainerSize(const IntSize&);
    IntSize containerSize() const;
    virtual bool usesContainerSize() const { return true; }
    virtual void computeIntrinsicDimensions(Length& intrinsicWidth, Length& intrinsicHeight, FloatSize& intrinsicRatio);

    virtual bool dataChanged(bool allDataReceived);

    // FIXME: SVGImages are underreporting decoded sizes and will be unable
    // to prune because these functions are not implemented yet.
    virtual void destroyDecodedData(bool) { }
    virtual unsigned decodedSize() const { return 0; }

    // FIXME: Implement this to be less conservative.
    virtual bool currentFrameKnownToBeOpaque() OVERRIDE { return false; }

    SVGImage(ImageObserver*);
    virtual void draw(GraphicsContext*, const FloatRect& fromRect, const FloatRect& toRect, ColorSpace styleColorSpace, CompositeOperator, BlendMode);
    void drawForContainer(GraphicsContext*, const FloatSize, float, const FloatRect&, const FloatRect&, ColorSpace, CompositeOperator, BlendMode);
    void drawPatternForContainer(GraphicsContext*, const FloatSize, float, const FloatRect&, const AffineTransform&, const FloatPoint&, ColorSpace,
        CompositeOperator, const FloatRect&);

    OwnPtr<SVGImageChromeClient> m_chromeClient;
    OwnPtr<Page> m_page;
    IntSize m_intrinsicSize;
};

bool isInSVGImage(const Element*);

}

#endif // ENABLE(SVG)
#endif // SVGImage_h
