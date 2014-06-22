/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WEBPImageDecoder_h
#define WEBPImageDecoder_h

#include "ImageDecoder.h"

#if USE(WEBP)

#include "webp/decode.h"
#if USE(QCMSLIB) && (WEBP_DECODER_ABI_VERSION > 0x200)
#define QCMS_WEBP_COLOR_CORRECTION
#endif

namespace WebCore {

class WEBPImageDecoder : public ImageDecoder {
public:
    WEBPImageDecoder(ImageSource::AlphaOption, ImageSource::GammaAndColorProfileOption);
    virtual ~WEBPImageDecoder();

    virtual String filenameExtension() const { return "webp"; }
    virtual bool isSizeAvailable();
    virtual ImageFrame* frameBufferAtIndex(size_t index);

private:
    bool decode(bool onlySize);

    WebPIDecoder* m_decoder;
    bool m_hasAlpha;
    int m_formatFlags;

#ifdef QCMS_WEBP_COLOR_CORRECTION
    qcms_transform* colorTransform() const { return m_transform; }
    void createColorTransform(const char* data, size_t);
    void readColorProfile(const uint8_t* data, size_t);
    void applyColorProfile(const uint8_t* data, size_t, ImageFrame&);

    bool m_haveReadProfile;
    qcms_transform* m_transform;
    int m_decodedHeight;
#else
    void applyColorProfile(const uint8_t*, size_t, ImageFrame&) { };
#endif
    void clear();
};

} // namespace WebCore

#endif

#endif
