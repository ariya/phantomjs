/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WKCAImageQueue_h
#define WKCAImageQueue_h

#if USE(ACCELERATED_COMPOSITING)

typedef const void * CFTypeRef;
typedef const struct __CFDictionary * CFDictionaryRef;

#include <stdint.h>
#include <wtf/OwnPtr.h>

namespace WebCore {

class WKCAImageQueuePrivate;

class WKCAImageQueue {
public:
    enum Flags {
        Async = 1U << 0,
        Fill  = 1U << 1,
        Protected = 1U << 2,
        UseCleanAperture = 1U << 3,
        UseAspectRatio = 1U << 4,
        LowQualityColor = 1U << 5,
    };

    enum ImageType {
        Nil = 1,
        Surface,
        Buffer,
        IOSurface,
    };

    enum ImageFlags {
        Opaque = 1U << 0,
        Flush = 1U << 1,
        WillFlush = 1U << 2,
        Flipped = 1U << 3,
        WaitGPU = 1U << 4,
    };

    typedef void (*ReleaseCallback)(unsigned int type, uint64_t id, void* info);

    WKCAImageQueue(uint32_t width, uint32_t height, uint32_t capacity);
    ~WKCAImageQueue(void);

    size_t collect();

    bool insertImage(double t, unsigned int type, uint64_t id, uint32_t flags, ReleaseCallback release, void* info);
    uint64_t registerPixelBuffer(void *data, size_t data_size, size_t rowbytes, size_t width, size_t height, uint32_t pixel_format, CFDictionaryRef attachments, uint32_t flags);

    uint32_t flags() const;
    void setFlags(uint32_t mask, uint32_t flags);

    CFTypeRef get();

private:
    WKCAImageQueue(const WKCAImageQueue&);
    WKCAImageQueue& operator=(const WKCAImageQueue&);
    OwnPtr<WKCAImageQueuePrivate> m_private;
};

}

#endif

#endif
