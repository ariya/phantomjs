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

// Forward declaration of libwebp's structure. Must be outside the WebCore scope.
typedef struct WebPIDecoder WebPIDecoder;

namespace WebCore {

class WEBPImageDecoder : public ImageDecoder {
public:
    WEBPImageDecoder(ImageSource::AlphaOption, ImageSource::GammaAndColorProfileOption);
    virtual ~WEBPImageDecoder();
    virtual String filenameExtension() const { return "vp8"; }
    virtual bool isSizeAvailable();
    virtual ImageFrame* frameBufferAtIndex(size_t index);

private:
    // Returns false in case of decoding failure.
    bool decode(bool onlySize);

    WebPIDecoder* m_decoder; // This is only used when we want to decode() but not all data is available yet.
    int m_lastVisibleRow;
    Vector<uint8_t> m_rgbOutput;
};

} // namespace WebCore

#endif

#endif
