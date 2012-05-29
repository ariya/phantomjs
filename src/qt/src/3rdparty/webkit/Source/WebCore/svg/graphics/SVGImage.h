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

class Page;
class SVGImageChromeClient;

class SVGImage : public Image {
public:
    static PassRefPtr<SVGImage> create(ImageObserver* observer)
    {
        return adoptRef(new SVGImage(observer));
    }

private:
    virtual ~SVGImage();

    virtual String filenameExtension() const;

    virtual void setContainerSize(const IntSize&);
    virtual bool usesContainerSize() const;
    virtual bool hasRelativeWidth() const;
    virtual bool hasRelativeHeight() const;

    virtual IntSize size() const;

    virtual bool dataChanged(bool allDataReceived);

    // FIXME: SVGImages are underreporting decoded sizes and will be unable
    // to prune because these functions are not implemented yet.
    virtual void destroyDecodedData(bool) { }
    virtual unsigned decodedSize() const { return 0; }

    virtual NativeImagePtr frameAtIndex(size_t) { return 0; }

    SVGImage(ImageObserver*);
    virtual void draw(GraphicsContext*, const FloatRect& fromRect, const FloatRect& toRect, ColorSpace styleColorSpace, CompositeOperator);

    virtual NativeImagePtr nativeImageForCurrentFrame();

    OwnPtr<SVGImageChromeClient> m_chromeClient;
    OwnPtr<Page> m_page;
    RefPtr<Image> m_frameCache;
};
}

#endif // ENABLE(SVG)
#endif // SVGImage_h
