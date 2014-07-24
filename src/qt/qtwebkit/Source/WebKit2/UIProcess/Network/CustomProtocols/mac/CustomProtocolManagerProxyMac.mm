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
#import "CustomProtocolManagerProxy.h"

#if ENABLE(CUSTOM_PROTOCOLS)

#import "ChildProcessProxy.h"
#import "Connection.h"
#import "CustomProtocolManagerMessages.h"
#import "CustomProtocolManagerProxyMessages.h"
#import "DataReference.h"
#import "WebCoreArgumentCoders.h"
#import <WebCore/ResourceError.h>
#import <WebCore/ResourceRequest.h>
#import <WebCore/ResourceResponse.h>

using namespace CoreIPC;
using namespace WebCore;
using namespace WebKit;

@interface WKCustomProtocolLoader : NSObject <NSURLConnectionDelegate> {
@private
    CustomProtocolManagerProxy* _customProtocolManagerProxy;
    uint64_t _customProtocolID;
    RefPtr<Connection> _connection;
    NSURLCacheStoragePolicy _storagePolicy;
    NSURLConnection *_urlConnection;
}
- (id)initWithCustomProtocolManagerProxy:(CustomProtocolManagerProxy*)customProtocolManagerProxy customProtocolID:(uint64_t)customProtocolID request:(NSURLRequest *)request connection:(Connection *)connection;
@end

@implementation WKCustomProtocolLoader

- (id)initWithCustomProtocolManagerProxy:(CustomProtocolManagerProxy*)customProtocolManagerProxy customProtocolID:(uint64_t)customProtocolID request:(NSURLRequest *)request connection:(Connection *)connection
{
    self = [super init];
    if (!self)
        return nil;

    ASSERT(customProtocolManagerProxy);
    ASSERT(request);
    ASSERT(connection);
    _customProtocolManagerProxy = customProtocolManagerProxy;
    _customProtocolID = customProtocolID;
    _connection = connection;
    _storagePolicy = NSURLCacheStorageNotAllowed;
    _urlConnection = [[NSURLConnection alloc] initWithRequest:request delegate:self startImmediately:YES];
    
    return self;
}

- (void)dealloc
{
    _connection.clear();
    [_urlConnection cancel];
    [_urlConnection release];
    [super dealloc];
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
    ResourceError coreError(error);
    _connection->send(Messages::CustomProtocolManager::DidFailWithError(_customProtocolID, coreError), 0);
    _customProtocolManagerProxy->stopLoading(_customProtocolID);
}

- (NSCachedURLResponse *)connection:(NSURLConnection *)connection willCacheResponse:(NSCachedURLResponse *)cachedResponse
{
    ASSERT(_storagePolicy == NSURLCacheStorageNotAllowed);
    _storagePolicy = [cachedResponse storagePolicy];
    return cachedResponse;
}

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response
{
    ResourceResponse coreResponse(response);
    _connection->send(Messages::CustomProtocolManager::DidReceiveResponse(_customProtocolID, coreResponse, _storagePolicy), 0);
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data
{
    CoreIPC::DataReference coreData(static_cast<const uint8_t*>([data bytes]), [data length]);
    _connection->send(Messages::CustomProtocolManager::DidLoadData(_customProtocolID, coreData), 0);
}

- (NSURLRequest *)connection:(NSURLConnection *)connection willSendRequest:(NSURLRequest *)request redirectResponse:(NSURLResponse *)redirectResponse
{
    return request;
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection
{
    _connection->send(Messages::CustomProtocolManager::DidFinishLoading(_customProtocolID), 0);
    _customProtocolManagerProxy->stopLoading(_customProtocolID);
}

@end

namespace WebKit {

CustomProtocolManagerProxy::CustomProtocolManagerProxy(ChildProcessProxy* childProcessProxy)
    : m_childProcessProxy(childProcessProxy)
{
    ASSERT(m_childProcessProxy);
    m_childProcessProxy->addMessageReceiver(Messages::CustomProtocolManagerProxy::messageReceiverName(), this);
}

void CustomProtocolManagerProxy::startLoading(uint64_t customProtocolID, const ResourceRequest& coreRequest)
{
    NSURLRequest *request = coreRequest.nsURLRequest(DoNotUpdateHTTPBody);
    if (!request)
        return;

    WKCustomProtocolLoader *loader = [[WKCustomProtocolLoader alloc] initWithCustomProtocolManagerProxy:this customProtocolID:customProtocolID request:request connection:m_childProcessProxy->connection()];
    ASSERT(loader);
    ASSERT(!m_loaderMap.contains(customProtocolID));
    m_loaderMap.add(customProtocolID, loader);
    [loader release];
}

void CustomProtocolManagerProxy::stopLoading(uint64_t customProtocolID)
{
    m_loaderMap.remove(customProtocolID);
}

} // namespace WebKit

#endif // ENABLE(CUSTOM_PROTOCOLS)
