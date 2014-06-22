/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2008-2009 Torch Mobile, Inc.
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

#ifndef JPEGImageDecoder_h
#define JPEGImageDecoder_h

#include "ImageDecoder.h"
#include <stdio.h> // Needed by jpeglib.h for FILE.
#include <wtf/OwnPtr.h>

#if OS(WINCE)
// Remove warning: 'FAR' macro redefinition
#undef FAR

// jmorecfg.h in libjpeg checks for XMD_H with the comment: "X11/xmd.h correctly defines INT32"
// fix INT32 redefinition error by pretending we are X11/xmd.h
#define XMD_H
#endif

extern "C" {
#include "jpeglib.h"
}

namespace WebCore {

    class JPEGImageReader;

    // This class decodes the JPEG image format.
    class JPEGImageDecoder : public ImageDecoder {
    public:
        JPEGImageDecoder(ImageSource::AlphaOption, ImageSource::GammaAndColorProfileOption);
        virtual ~JPEGImageDecoder();

        // ImageDecoder
        virtual String filenameExtension() const { return "jpg"; }
        virtual bool isSizeAvailable();
        virtual bool setSize(unsigned width, unsigned height);
        virtual ImageFrame* frameBufferAtIndex(size_t index);
        // CAUTION: setFailed() deletes |m_reader|.  Be careful to avoid
        // accessing deleted memory, especially when calling this from inside
        // JPEGImageReader!
        virtual bool setFailed();

        bool willDownSample()
        {
            ASSERT(ImageDecoder::isSizeAvailable());
            return m_scaled;
        }

        bool outputScanlines();
        void jpegComplete();

        void setColorProfile(const ColorProfile& colorProfile) { m_colorProfile = colorProfile; }
        void setOrientation(ImageOrientation orientation) { m_orientation = orientation; }

    private:
        // Decodes the image.  If |onlySize| is true, stops decoding after
        // calculating the image size.  If decoding fails but there is no more
        // data coming, sets the "decode failure" flag.
        void decode(bool onlySize);

        template <J_COLOR_SPACE colorSpace>
        bool outputScanlines(ImageFrame& buffer);

        template <J_COLOR_SPACE colorSpace, bool isScaled>
        bool outputScanlines(ImageFrame& buffer);

        OwnPtr<JPEGImageReader> m_reader;
    };

} // namespace WebCore

#endif
