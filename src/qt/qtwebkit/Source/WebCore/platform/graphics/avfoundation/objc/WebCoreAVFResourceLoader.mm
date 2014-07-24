/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#import "config.h"
#import "WebCoreAVFResourceLoader.h"

#if ENABLE(VIDEO) && USE(AVFOUNDATION) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090

#import "CachedRawResource.h"
#import "CachedResourceLoader.h"
#import "CachedResourceRequest.h"
#import "MediaPlayerPrivateAVFoundationObjC.h"
#import "ResourceBuffer.h"
#import "ResourceLoaderOptions.h"
#import "SharedBuffer.h"
#import "SoftLinking.h"
#import "UTIUtilities.h"
#import <AVFoundation/AVAssetResourceLoader.h>
#import <objc/runtime.h>
#import <wtf/text/CString.h>

namespace WebCore {

PassOwnPtr<WebCoreAVFResourceLoader> WebCoreAVFResourceLoader::create(MediaPlayerPrivateAVFoundationObjC* parent, AVAssetResourceLoadingRequest* avRequest)
{
    ASSERT(avRequest);
    ASSERT(parent);
    return adoptPtr(new WebCoreAVFResourceLoader(parent, avRequest));
}

WebCoreAVFResourceLoader::WebCoreAVFResourceLoader(MediaPlayerPrivateAVFoundationObjC* parent, AVAssetResourceLoadingRequest* avRequest)
    : m_parent(parent)
    , m_avRequest(avRequest)
{
}

WebCoreAVFResourceLoader::~WebCoreAVFResourceLoader()
{
    stopLoading();
}

void WebCoreAVFResourceLoader::startLoading()
{
    if (m_resource)
        return;

    KURL requestURL = [[m_avRequest.get() request] URL];

    CachedResourceRequest request(ResourceRequest(requestURL), ResourceLoaderOptions(SendCallbacks, DoNotSniffContent, BufferData, DoNotAllowStoredCredentials, DoNotAskClientForCrossOriginCredentials, DoSecurityCheck, UseDefaultOriginRestrictionsForType));

    request.mutableResourceRequest().setPriority(ResourceLoadPriorityLow);
    CachedResourceLoader* loader = m_parent->player()->cachedResourceLoader();
    m_resource = loader ? loader->requestRawResource(request) : 0;
    if (m_resource)
        m_resource->addClient(this);
    else {
        LOG_ERROR("Failed to start load for media at url %s", requestURL.string().ascii().data());
        [m_avRequest.get() finishLoadingWithError:0];
    }
}

void WebCoreAVFResourceLoader::stopLoading()
{
    if (!m_resource)
        return;

    m_resource->removeClient(this);
    m_resource = 0;
}

void WebCoreAVFResourceLoader::responseReceived(CachedResource* resource, const ResourceResponse& response)
{
    ASSERT(resource == m_resource);
    UNUSED_PARAM(resource);

    int status = response.httpStatusCode();
    if (status && (status < 200 || status > 299)) {
        [m_avRequest.get() finishLoadingWithError:0];
        return;
    }

    if (AVAssetResourceLoadingContentInformationRequest* contentInfo = [m_avRequest.get() contentInformationRequest]) {
        String uti = UTIFromMIMEType(response.mimeType().createCFString().get()).get();

        [contentInfo setContentType:uti];
        [contentInfo setContentLength:response.expectedContentLength()];
        [contentInfo setByteRangeAccessSupported:YES];

        if (![m_avRequest dataRequest]) {
            [m_avRequest.get() finishLoading];
            stopLoading();
        }
    }
}

void WebCoreAVFResourceLoader::dataReceived(CachedResource* resource, const char*, int)
{
    fulfillRequestWithResource(resource);
}

void WebCoreAVFResourceLoader::notifyFinished(CachedResource* resource)
{
    if (resource->loadFailedOrCanceled()) {
        // <rdar://problem/13987417> Set the contentType of the contentInformationRequest to an empty
        // string to trigger AVAsset's playable value to complete loading.
        if ([m_avRequest.get() contentInformationRequest] && ![[m_avRequest.get() contentInformationRequest] contentType])
            [[m_avRequest.get() contentInformationRequest] setContentType:@""];

        [m_avRequest.get() finishLoadingWithError:0];
    } else {
        fulfillRequestWithResource(resource);
        [m_avRequest.get() finishLoading];
    }
    stopLoading();
}

void WebCoreAVFResourceLoader::fulfillRequestWithResource(CachedResource* resource)
{
    ASSERT(resource == m_resource);
    AVAssetResourceLoadingDataRequest* dataRequest = [m_avRequest dataRequest];
    if (!dataRequest)
        return;

    SharedBuffer* data = resource->resourceBuffer() ? resource->resourceBuffer()->sharedBuffer() : 0;
    if (!data)
        return;

    // Check for possible unsigned overflow.
    ASSERT([dataRequest currentOffset] >= [dataRequest requestedOffset]);
    ASSERT([dataRequest requestedLength] >= ([dataRequest currentOffset] - [dataRequest requestedOffset]));

    NSUInteger remainingLength = [dataRequest requestedLength] - static_cast<NSUInteger>([dataRequest currentOffset] - [dataRequest requestedOffset]);
    do {
        // Check to see if there is any data available in the buffer to fulfill the data request.
        if (data->size() <= [dataRequest currentOffset])
            return;

        const char* someData;
        NSUInteger receivedLength = data->getSomeData(someData, static_cast<unsigned>([dataRequest currentOffset]));

        // Create an NSData with only as much of the received data as necessary to fulfill the request.
        NSUInteger length = MIN(receivedLength, remainingLength);
        RetainPtr<NSData> nsData = adoptNS([[NSData alloc] initWithBytes:someData length:length]);

        [dataRequest respondWithData:nsData.get()];
        remainingLength -= length;
    } while (remainingLength);

    if ([dataRequest currentOffset] + [dataRequest requestedLength] >= [dataRequest requestedOffset]) {
        [m_avRequest.get() finishLoading];
        stopLoading();
    }
}

}

#endif // ENABLE(VIDEO) && USE(AVFOUNDATION)
