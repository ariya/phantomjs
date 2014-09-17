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

#include "config.h"
#include "WKCAImageQueue.h"

#if USE(ACCELERATED_COMPOSITING)

#include <CoreFoundation/CoreFoundation.h>
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RetainPtr.h>

namespace WebCore {

class WKCAImageQueuePrivate {
public:
    RetainPtr<CAImageQueueRef> m_imageQueue;
};

static CAImageQueueRef WKCAImageQueueRetain(CAImageQueueRef iq) 
{
    if (iq)
        return (CAImageQueueRef)CFRetain(iq);
    return 0;
}

static void WKCAImageQueueRelease(CAImageQueueRef iq)
{
    if (iq)
        CFRelease(iq);
}

WKCAImageQueue::WKCAImageQueue(uint32_t width, uint32_t height, uint32_t capacity)
    : m_private(adoptPtr(new WKCAImageQueuePrivate()))
{
    m_private->m_imageQueue.adoptCF(wkCAImageQueueCreate(width, height, capacity));
}

WKCAImageQueue::WKCAImageQueue(const WKCAImageQueue& o)
    : m_private(adoptPtr(new WKCAImageQueuePrivate()))
{
    m_private->m_imageQueue = o.m_private->m_imageQueue;
}

WKCAImageQueue::~WKCAImageQueue(void)
{
}

WKCAImageQueue& WKCAImageQueue::operator=(const WKCAImageQueue& o)
{
    m_private->m_imageQueue = o.m_private->m_imageQueue;
    return *this;
}

size_t WKCAImageQueue::collect()
{
    return wkCAImageQueueCollect(m_private->m_imageQueue.get());
}

bool WKCAImageQueue::insertImage(double t, unsigned int type, uint64_t id, uint32_t flags, ReleaseCallback release, void* info)
{
    return wkCAImageQueueInsertImage(m_private->m_imageQueue.get(), t, type, id, flags, release, info);
}

uint64_t WKCAImageQueue::registerPixelBuffer(void *data, size_t data_size, size_t rowbytes, size_t width, size_t height, uint32_t pixel_format, CFDictionaryRef attachments, uint32_t flags)
{
    return wkCAImageQueueRegisterPixelBuffer(m_private->m_imageQueue.get(), data, data_size, rowbytes, width, height, pixel_format, attachments, flags);
}

void WKCAImageQueue::setFlags(uint32_t mask, uint32_t flags)
{
    wkCAImageQueueSetFlags(m_private->m_imageQueue.get(), mask, flags);
}

CFTypeRef WKCAImageQueue::get()
{
    return m_private->m_imageQueue.get();
}

}

#endif
