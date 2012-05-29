/*
 * Copyright (C) 2007, 2008, 2009, 2010 Apple, Inc.  All rights reserved.
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

#ifndef QTPixelBuffer_h
#define QTPixelBuffer_h

#ifdef QTMOVIEWIN_EXPORTS
#define QTMOVIEWIN_API __declspec(dllexport)
#else
#define QTMOVIEWIN_API __declspec(dllimport)
#endif

#include <stdint.h>

typedef struct __CVBuffer *CVBufferRef;
typedef CVBufferRef CVPixelBufferRef;
typedef struct CGImage* CGImageRef;
typedef int32_t CVReturn;
typedef const struct __CFDictionary * CFDictionaryRef;

// QTPixelBuffer wraps QuickTime's implementation of CVPixelBuffer, so its functions are
// safe to call within WebKit.
class QTMOVIEWIN_API QTPixelBuffer {
public:
    enum Type { ConfigureForCGImage, ConfigureForCAImageQueue };
    static CFDictionaryRef createPixelBufferAttributesDictionary(Type);

    QTPixelBuffer();
    QTPixelBuffer(const QTPixelBuffer&);
    QTPixelBuffer(CVPixelBufferRef);
    QTPixelBuffer& operator=(const QTPixelBuffer&);
    ~QTPixelBuffer();

    void set(CVPixelBufferRef);
    CVPixelBufferRef pixelBufferRef();
    void adopt(CVPixelBufferRef);
    void clear();

    CVReturn lockBaseAddress();
    CVReturn unlockBaseAddress();
    void* baseAddress();

    size_t width() const;
    size_t height() const;
    unsigned long pixelFormatType() const;
    bool pixelFormatIs32ARGB() const;
    bool pixelFormatIs32BGRA() const;
    size_t bytesPerRow() const;
    size_t dataSize() const;

    bool isPlanar() const;
    size_t planeCount() const;
    size_t widthOfPlane(size_t) const;
    size_t heightOfPlane(size_t) const;
    void* baseAddressOfPlane(size_t) const;
    size_t bytesPerRowOfPlane(size_t) const;

    void getExtendedPixels(size_t* left, size_t* right, size_t* top, size_t* bottom) const;
    CFDictionaryRef attachments() const;

    // Generic CFRetain/CFRelease callbacks
    static void releaseCallback(void* refcon);
    static void retainCallback(void* refcon);

    // CAImageQueue callbacks
    static void imageQueueReleaseCallback(unsigned int type, uint64_t id, void* refcon);

    // CGDataProvider callbacks
    static void dataProviderReleaseBytePointerCallback(void* refcon, const void* pointer);
    static const void* dataProviderGetBytePointerCallback(void* refcon);
    static size_t dataProviderGetBytesAtPositionCallback(void* refcon, void* buffer, size_t position, size_t count);
    static void dataProviderReleaseInfoCallback(void* refcon);

private:
    CVPixelBufferRef m_pixelBuffer;
};

#endif
