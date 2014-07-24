/*
 * Copyright (C) 2010 Apple, Inc.  All rights reserved.
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
#include "config.h"

#include "QTDecompressionSession.h"

#include <ImageCompression.h>
#include <algorithm>

class QTDecompressionSessionClient {
public:
    static void trackingCallback(void *decompressionTrackingRefCon, OSStatus, 
        ICMDecompressionTrackingFlags decompressionTrackingFlags, CVPixelBufferRef pixelBuffer, 
        TimeValue64, TimeValue64, ICMValidTimeFlags, void *, void *)
    {
        QTDecompressionSession* session = static_cast<QTDecompressionSession*>(decompressionTrackingRefCon);
        ASSERT(session);

        if (decompressionTrackingFlags & kICMDecompressionTracking_FrameDecoded)
            session->m_latestFrame = QTPixelBuffer(pixelBuffer);
    }
};

PassOwnPtr<QTDecompressionSession> QTDecompressionSession::create(unsigned long pixelFormat, size_t width, size_t height)
{
    return adoptPtr(new QTDecompressionSession(pixelFormat, width, height));
}

QTDecompressionSession::QTDecompressionSession(unsigned long pixelFormat, size_t width, size_t height)
    : m_session(0)
    , m_pixelFormat(pixelFormat)
    , m_width(width)
    , m_height(height)
{
    initializeSession();
}

QTDecompressionSession::~QTDecompressionSession()
{
    if (m_session)
        ICMDecompressionSessionRelease(m_session);
}

void QTDecompressionSession::initializeSession()
{
    if (m_session)
        return;

    ICMPixelFormatInfo pixelFormatInfo = {sizeof(ICMPixelFormatInfo), 0};
    if (ICMGetPixelFormatInfo(m_pixelFormat, &pixelFormatInfo) != noErr) {
        // The ICM does not know anything about the pixelFormat contained in
        // the pixel buffer, so it won't be able to convert it to RGBA.  
        return;
    }

    // The depth and cType fields of the ImageDescriptionHandle are filled 
    // out according to the instructions in Technical Q&A QA1183: 
    // http://developer.apple.com/library/mac/#qa/qa2001/qa1183.html
    bool isIndexed = pixelFormatInfo.formatFlags & kICMPixelFormatIsIndexed;
    bool isQD = pixelFormatInfo.formatFlags & kICMPixelFormatIsSupportedByQD;
    bool isMonochrome = pixelFormatInfo.formatFlags & kICMPixelFormatIsMonochrome;
    bool hasAlpha = pixelFormatInfo.formatFlags & kICMPixelFormatHasAlphaChannel;

    unsigned int depth = 24; // The default depth is 24.
    if (hasAlpha)
        depth = 32; // Any pixel format with alpha gets a depth of 32.
    else if (isMonochrome) {
        // Grayscale pixel formats get depths 33 through 40, depending
        // on their bits per pixel. Yes, this means that 16-bit grayscale
        // and 8-bit grayscale have the same pixel depth.
        depth = 32 + std::min<unsigned int>(8, pixelFormatInfo.bitsPerPixel[0]);
    } else if (isIndexed) {
        // Indexed pixel formats get a depth of 1 through 8, depending on
        // the their bits per pixel.
        depth = pixelFormatInfo.bitsPerPixel[0]; 
    }

    // If QuickDraw supports the given pixel format, the cType should be kRawCodecType.
    // Otherwise, use the pixel format code for the cType.  We are assuming the pixel 
    // buffer is uncompressed.
    unsigned long cType = isQD ? kRawCodecType : m_pixelFormat;

    ImageDescriptionHandle description = (ImageDescriptionHandle)NewHandleClear(sizeof(ImageDescription));
    (**description).idSize = sizeof(ImageDescription);
    (**description).cType = cType;
    (**description).version = 2;
    (**description).spatialQuality = codecLosslessQuality;
    (**description).width = m_width;
    (**description).height = m_height;
    (**description).hRes = 72 << 16; // 72 DPI as a fixed-point number
    (**description).vRes = 72 << 16; // 72 DPI as a fixed-point number
    (**description).frameCount = 1;
    (**description).depth =  depth;
    (**description).clutID = -1;

    // Create the mandatory ICMDecompressionSessionOptions, but leave
    // all the default values.
    ICMDecompressionSessionOptionsRef options = 0;
    ICMDecompressionSessionOptionsCreate(kCFAllocatorDefault, &options);

    CFDictionaryRef pixelBufferAttributes = QTPixelBuffer::createPixelBufferAttributesDictionary(QTPixelBuffer::ConfigureForCGImage);

    ICMDecompressionTrackingCallbackRecord callback = {
        QTDecompressionSessionClient::trackingCallback,
        this,
    };

    ICMDecompressionSessionCreate(kCFAllocatorDefault,
        description,
        options,
        pixelBufferAttributes,
        &callback,
        &m_session);

    if (pixelBufferAttributes)
        CFRelease(pixelBufferAttributes);

    ICMDecompressionSessionOptionsRelease(options);
    DisposeHandle((Handle)description);
}

bool QTDecompressionSession::canDecompress(QTPixelBuffer inBuffer)
{
    return m_session 
        && inBuffer.pixelFormatType() == m_pixelFormat 
        && inBuffer.width() == m_width
        && inBuffer.height() == m_height;
}

QTPixelBuffer QTDecompressionSession::decompress(QTPixelBuffer inBuffer)
{
    if (!canDecompress(inBuffer))
        return QTPixelBuffer();
    
    inBuffer.lockBaseAddress();
    ICMDecompressionSessionDecodeFrame(m_session,
        static_cast<UInt8*>(inBuffer.baseAddress()),
        inBuffer.dataSize(),
        0, // frameOptions
        0, // frameTime
        0); // sourceFrameRefCon

    // Because we passed in 0 for frameTime, the above function
    // is synchronous, and the client callback will have been
    // called before the function returns, and m_latestFrame
    // will contain the newly decompressed frame.
    return m_latestFrame;
}
