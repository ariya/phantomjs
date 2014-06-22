/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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
 
#include "config.h"
#include "ResourceLoader.h"

#include "FrameLoader.h"
#include "FrameLoaderClient.h"

#if USE(NETWORK_CFDATA_ARRAY_CALLBACK)
#include "InspectorInstrumentation.h"
#include "ResourceBuffer.h"
#endif

#if USE(CFNETWORK)
@interface NSCachedURLResponse (Details)
-(id)_initWithCFCachedURLResponse:(CFCachedURLResponseRef)cachedResponse;
-(CFCachedURLResponseRef)_CFCachedURLResponse;
@end
#endif

namespace WebCore {

#if USE(CFNETWORK)

CFCachedURLResponseRef ResourceLoader::willCacheResponse(ResourceHandle*, CFCachedURLResponseRef cachedResponse)
{
    if (m_options.sendLoadCallbacks == DoNotSendCallbacks)
        return 0;

    RetainPtr<NSCachedURLResponse> nsCachedResponse = adoptNS([[NSCachedURLResponse alloc] _initWithCFCachedURLResponse:cachedResponse]);
    return [frameLoader()->client()->willCacheResponse(documentLoader(), identifier(), nsCachedResponse.get()) _CFCachedURLResponse];
}

#else

NSCachedURLResponse* ResourceLoader::willCacheResponse(ResourceHandle*, NSCachedURLResponse* response)
{
    if (m_options.sendLoadCallbacks == DoNotSendCallbacks)
        return 0;
    return frameLoader()->client()->willCacheResponse(documentLoader(), identifier(), response);
}

#endif

#if USE(NETWORK_CFDATA_ARRAY_CALLBACK)

void ResourceLoader::didReceiveDataArray(CFArrayRef dataArray)
{
    // Protect this in this delegate method since the additional processing can do
    // anything including possibly derefing this; one example of this is Radar 3266216.
    RefPtr<ResourceLoader> protector(this);

    CFIndex arrayCount = CFArrayGetCount(dataArray);
    for (CFIndex i = 0; i < arrayCount; ++i) {
        CFDataRef data = static_cast<CFDataRef>(CFArrayGetValueAtIndex(dataArray, i));
        int dataLen = static_cast<int>(CFDataGetLength(data));

        if (m_options.dataBufferingPolicy == BufferData) {
            if (!m_resourceData)
                m_resourceData = ResourceBuffer::create();
            m_resourceData->append(data);
        }

        // FIXME: If we get a resource with more than 2B bytes, this code won't do the right thing.
        // However, with today's computers and networking speeds, this won't happen in practice.
        // Could be an issue with a giant local file.
        if (m_options.sendLoadCallbacks == SendCallbacks && m_frame)
            frameLoader()->notifier()->didReceiveData(this, reinterpret_cast<const char*>(CFDataGetBytePtr(data)), dataLen, dataLen);
    }
}

void ResourceLoader::didReceiveDataArray(ResourceHandle*, CFArrayRef dataArray)
{
    CFIndex arrayCount = CFArrayGetCount(dataArray);
    CFIndex dataLength = 0;
    for (CFIndex i = 0; i < arrayCount; ++i) {
        CFDataRef data = static_cast<CFDataRef>(CFArrayGetValueAtIndex(dataArray, i));
        dataLength += CFDataGetLength(data);
    }

    // FIXME: didReceiveData() passes encoded data length to InspectorInstrumentation, but it is not available here.
    // This probably results in incorrect size being displayed in Web Inspector.
    InspectorInstrumentationCookie cookie = InspectorInstrumentation::willReceiveResourceData(m_frame.get(), identifier(), dataLength);
    didReceiveDataArray(dataArray);
    InspectorInstrumentation::didReceiveResourceData(cookie);
}

#endif

}
