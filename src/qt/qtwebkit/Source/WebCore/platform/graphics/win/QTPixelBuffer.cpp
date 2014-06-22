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
#include "config.h"

#include "QTPixelBuffer.h"

#include <CFNumber.h>
#include <CFString.h>
#include <CGColorSpace.h>
#include <CGImage.h>
#include <CVPixelBuffer.h>
#include <QuickDraw.h>
#include <memory.h>

static OSStatus SetNumberValue(CFMutableDictionaryRef inDict, CFStringRef inKey, SInt32 inValue)
{
    CFNumberRef number;
 
    number = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &inValue);
    if (!number) 
        return coreFoundationUnknownErr;
 
    CFDictionarySetValue(inDict, inKey, number);
    CFRelease(number);

    return noErr;
}

CFDictionaryRef QTPixelBuffer::createPixelBufferAttributesDictionary(QTPixelBuffer::Type contextType)
{
    static const CFStringRef kDirect3DCompatibilityKey = CFSTR("Direct3DCompatibility");

    CFMutableDictionaryRef pixelBufferAttributes = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    if (contextType == QTPixelBuffer::ConfigureForCAImageQueue) {
        // Ask for D3D compatible pixel buffers so no further work is needed.
        CFDictionarySetValue(pixelBufferAttributes, kDirect3DCompatibilityKey, kCFBooleanTrue);
    } else {
        // Use the k32BGRAPixelFormat, as QuartzCore will be able to use the pixels directly,
        // without needing an additional copy or rendering pass.
        SetNumberValue(pixelBufferAttributes, kCVPixelBufferPixelFormatTypeKey, k32BGRAPixelFormat);
            
        // Set kCVPixelBufferBytesPerRowAlignmentKey to 16 to ensure that each row of pixels 
        // starts at a 16 byte aligned address for most efficient data reading.
        SetNumberValue(pixelBufferAttributes, kCVPixelBufferBytesPerRowAlignmentKey, 16);
        CFDictionarySetValue(pixelBufferAttributes, kCVPixelBufferCGImageCompatibilityKey, kCFBooleanTrue);
    }
    return pixelBufferAttributes;
}

QTPixelBuffer::QTPixelBuffer() 
    : m_pixelBuffer(0) 
{
}

QTPixelBuffer::QTPixelBuffer(const QTPixelBuffer& p) 
    : m_pixelBuffer(p.m_pixelBuffer) 
{
    CVPixelBufferRetain(m_pixelBuffer);
}

QTPixelBuffer::QTPixelBuffer(CVPixelBufferRef ref) 
    : m_pixelBuffer(ref)
{
    CVPixelBufferRetain(m_pixelBuffer);
}

QTPixelBuffer::~QTPixelBuffer() 
{
    clear();
}

QTPixelBuffer& QTPixelBuffer::operator=(const QTPixelBuffer& p) 
{
    set(p.m_pixelBuffer);
    return *this;
}

void QTPixelBuffer::set(CVPixelBufferRef ref)
{
    CVPixelBufferRetain(ref);
    CVPixelBufferRelease(m_pixelBuffer);
    m_pixelBuffer = ref;
}

CVPixelBufferRef QTPixelBuffer::pixelBufferRef()
{
    return m_pixelBuffer;
}

void QTPixelBuffer::adopt(CVPixelBufferRef ref)
{
    if (ref == m_pixelBuffer)
        return;
    CVPixelBufferRelease(m_pixelBuffer);
    m_pixelBuffer = ref;
}

void QTPixelBuffer::clear()
{
    CVPixelBufferRelease(m_pixelBuffer);
    m_pixelBuffer = 0;
}

CVReturn QTPixelBuffer::lockBaseAddress()
{
    return CVPixelBufferLockBaseAddress(m_pixelBuffer, 0);
}

CVReturn QTPixelBuffer::unlockBaseAddress()
{
    return CVPixelBufferUnlockBaseAddress(m_pixelBuffer, 0);
}

void* QTPixelBuffer::baseAddress()
{
    return CVPixelBufferGetBaseAddress(m_pixelBuffer);
}

size_t QTPixelBuffer::width() const
{
    return CVPixelBufferGetWidth(m_pixelBuffer);
}

size_t QTPixelBuffer::height() const
{
    return CVPixelBufferGetHeight(m_pixelBuffer);
}

unsigned long QTPixelBuffer::pixelFormatType() const
{
    return CVPixelBufferGetPixelFormatType(m_pixelBuffer);
}

bool QTPixelBuffer::pixelFormatIs32ARGB() const
{
    return CVPixelBufferGetPixelFormatType(m_pixelBuffer) == k32ARGBPixelFormat;
}

bool QTPixelBuffer::pixelFormatIs32BGRA() const
{
    return CVPixelBufferGetPixelFormatType(m_pixelBuffer) == k32BGRAPixelFormat;
}

size_t QTPixelBuffer::bytesPerRow() const
{
    return CVPixelBufferGetBytesPerRow(m_pixelBuffer);
}

size_t QTPixelBuffer::dataSize() const
{
    return CVPixelBufferGetDataSize(m_pixelBuffer);
}

bool QTPixelBuffer::isPlanar() const
{
    return CVPixelBufferIsPlanar(m_pixelBuffer);
}

size_t QTPixelBuffer::planeCount() const
{
    return CVPixelBufferGetPlaneCount(m_pixelBuffer);
}

size_t QTPixelBuffer::widthOfPlane(size_t plane) const
{
    return CVPixelBufferGetWidthOfPlane(m_pixelBuffer, plane);
}

size_t QTPixelBuffer::heightOfPlane(size_t plane) const
{
    return CVPixelBufferGetHeightOfPlane(m_pixelBuffer, plane);
}

void* QTPixelBuffer::baseAddressOfPlane(size_t plane) const
{
    return CVPixelBufferGetBaseAddressOfPlane(m_pixelBuffer, plane);
}

size_t QTPixelBuffer::bytesPerRowOfPlane(size_t plane) const
{
    return CVPixelBufferGetBytesPerRowOfPlane(m_pixelBuffer, plane);
}

void QTPixelBuffer::getExtendedPixels(size_t* left, size_t* right, size_t* top, size_t* bottom) const
{
    return CVPixelBufferGetExtendedPixels(m_pixelBuffer, left, right, top, bottom);
}

CFDictionaryRef QTPixelBuffer::attachments() const
{
    return CVBufferGetAttachments(m_pixelBuffer, kCVAttachmentMode_ShouldPropagate);
}

void QTPixelBuffer::retainCallback(void* refcon)
{
    CVPixelBufferRetain(static_cast<CVPixelBufferRef>(refcon));
}

void QTPixelBuffer::releaseCallback(void* refcon)
{
    CVPixelBufferRelease(static_cast<CVPixelBufferRef>(refcon));
}

void QTPixelBuffer::imageQueueReleaseCallback(unsigned int type, uint64_t id, void* refcon)
{
    CVPixelBufferRelease(static_cast<CVPixelBufferRef>(refcon));
}

void QTPixelBuffer::dataProviderReleaseBytePointerCallback(void* refcon, const void* pointer)
{
    CVPixelBufferUnlockBaseAddress(static_cast<CVPixelBufferRef>(refcon), 0);
}

const void* QTPixelBuffer::dataProviderGetBytePointerCallback(void* refcon)
{
    CVPixelBufferLockBaseAddress(static_cast<CVPixelBufferRef>(refcon), 0);
    return CVPixelBufferGetBaseAddress(static_cast<CVPixelBufferRef>(refcon));
}

size_t QTPixelBuffer::dataProviderGetBytesAtPositionCallback(void* refcon, void* buffer, size_t position, size_t count)
{
    char* data = (char*)CVPixelBufferGetBaseAddress(static_cast<CVPixelBufferRef>(refcon));
    size_t size = CVPixelBufferGetDataSize(static_cast<CVPixelBufferRef>(refcon));
    if (size - position < count)
        count = size - position;

    memcpy(buffer, data+position, count);
    return count;
}

void QTPixelBuffer::dataProviderReleaseInfoCallback(void* refcon)
{
    CVPixelBufferRelease(static_cast<CVPixelBufferRef>(refcon));
}
