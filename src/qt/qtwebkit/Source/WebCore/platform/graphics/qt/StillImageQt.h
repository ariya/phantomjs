/*
 * Copyright (C) 2008 Holger Hans Peter Freyther
 *
 * All rights reserved.
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

#ifndef StillImageQt_h
#define StillImageQt_h

#include "Image.h"

namespace WebCore {

    class StillImage : public Image {
    public:
        static PassRefPtr<StillImage> create(const QPixmap& pixmap)
        {
            return adoptRef(new StillImage(pixmap));
        }

        static PassRefPtr<StillImage> createForRendering(const QPixmap* pixmap)
        {
            return adoptRef(new StillImage(pixmap));
        }

        virtual bool currentFrameKnownToBeOpaque();

        // FIXME: StillImages are underreporting decoded sizes and will be unable
        // to prune because these functions are not implemented yet.
        virtual void destroyDecodedData(bool destroyAll = true) { Q_UNUSED(destroyAll); }
        virtual unsigned decodedSize() const { return 0; }

        virtual IntSize size() const;
        virtual PassNativeImagePtr nativeImageForCurrentFrame();
        virtual void draw(GraphicsContext*, const FloatRect& dstRect, const FloatRect& srcRect, ColorSpace styleColorSpace, CompositeOperator, BlendMode);

    private:
        StillImage(const QPixmap&);
        StillImage(const QPixmap*);
        virtual ~StillImage();
        
        const QPixmap* m_pixmap;
        bool m_ownsPixmap;
    };

}

#endif
