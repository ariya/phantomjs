/*
 * Copyright (C) 2004, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013 Apple Inc. All rights reserved.
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
#import "WebCoreResourceHandleAsOperationQueueDelegate.h"

#if !USE(CFNETWORK)

#import "AuthenticationChallenge.h"
#import "AuthenticationMac.h"
#import "Logging.h"
#import "ResourceHandle.h"
#import "ResourceHandleClient.h"
#import "ResourceRequest.h"
#import "ResourceResponse.h"
#import "SharedBuffer.h"
#import "WebCoreURLResponse.h"
#import <wtf/MainThread.h>

@interface NSURLRequest (Details)
- (id)_propertyForKey:(NSString *)key;
@end

using namespace WebCore;

@implementation WebCoreResourceHandleAsOperationQueueDelegate

- (id)initWithHandle:(ResourceHandle*)handle
{
    self = [self init];
    if (!self)
        return nil;

    m_handle = handle;
    m_semaphore = dispatch_semaphore_create(0);

    return self;
}

- (void)detachHandle
{
    m_handle = 0;

    m_requestResult = nullptr;
    m_cachedResponseResult = nullptr;
    m_boolResult = NO;
    dispatch_semaphore_signal(m_semaphore); // OK to signal even if we are not waiting.
}

- (void)dealloc
{
    dispatch_release(m_semaphore);
    [super dealloc];
}

- (void)continueWillSendRequest:(NSURLRequest *)newRequest
{
    m_requestResult = [newRequest retain];
    dispatch_semaphore_signal(m_semaphore);
}

- (void)continueDidReceiveResponse
{
    dispatch_semaphore_signal(m_semaphore);
}

- (void)continueShouldUseCredentialStorage:(BOOL)useCredentialStorage
{
    m_boolResult = useCredentialStorage;
    dispatch_semaphore_signal(m_semaphore);
}

- (void)continueCanAuthenticateAgainstProtectionSpace:(BOOL)canAuthenticate
{
    m_boolResult = canAuthenticate;
    dispatch_semaphore_signal(m_semaphore);
}

- (void)continueWillCacheResponse:(NSCachedURLResponse *)response
{
    m_cachedResponseResult = response;
    dispatch_semaphore_signal(m_semaphore);
}

- (NSURLRequest *)connection:(NSURLConnection *)connection willSendRequest:(NSURLRequest *)newRequest redirectResponse:(NSURLResponse *)redirectResponse
{
    ASSERT(!isMainThread());
    UNUSED_PARAM(connection);

    redirectResponse = synthesizeRedirectResponseIfNecessary(connection, newRequest, redirectResponse);

    // See <rdar://problem/5380697>. This is a workaround for a behavior change in CFNetwork where willSendRequest gets called more often.
    if (!redirectResponse)
        return newRequest;

#if !LOG_DISABLED
    if ([redirectResponse isKindOfClass:[NSHTTPURLResponse class]])
        LOG(Network, "Handle %p delegate connection:%p willSendRequest:%@ redirectResponse:%d, Location:<%@>", m_handle, connection, [newRequest description], static_cast<int>([(id)redirectResponse statusCode]), [[(id)redirectResponse allHeaderFields] objectForKey:@"Location"]);
    else
        LOG(Network, "Handle %p delegate connection:%p willSendRequest:%@ redirectResponse:non-HTTP", m_handle, connection, [newRequest description]); 
#endif

    RetainPtr<id> protector(self);

    dispatch_async(dispatch_get_main_queue(), ^{
        if (!m_handle) {
            m_requestResult = nullptr;
            dispatch_semaphore_signal(m_semaphore);
            return;
        }

        ResourceRequest request = newRequest;

        m_handle->willSendRequest(request, redirectResponse);
    });

    dispatch_semaphore_wait(m_semaphore, DISPATCH_TIME_FOREVER);
    return [m_requestResult.leakRef() autorelease];
}

- (BOOL)connectionShouldUseCredentialStorage:(NSURLConnection *)connection
{
    ASSERT(!isMainThread());
    UNUSED_PARAM(connection);

    LOG(Network, "Handle %p delegate connectionShouldUseCredentialStorage:%p", m_handle, connection);

    RetainPtr<id> protector(self);

    dispatch_async(dispatch_get_main_queue(), ^{
        if (!m_handle) {
            m_boolResult = NO;
            dispatch_semaphore_signal(m_semaphore);
            return;
        }
        m_handle->shouldUseCredentialStorage();
    });

    dispatch_semaphore_wait(m_semaphore, DISPATCH_TIME_FOREVER);
    return m_boolResult;
}

- (void)connection:(NSURLConnection *)connection didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge
{
    ASSERT(!isMainThread());
    UNUSED_PARAM(connection);

    LOG(Network, "Handle %p delegate connection:%p didReceiveAuthenticationChallenge:%p", m_handle, connection, challenge);

    dispatch_async(dispatch_get_main_queue(), ^{
        if (!m_handle) {
            [[challenge sender] cancelAuthenticationChallenge:challenge];
            return;
        }
        m_handle->didReceiveAuthenticationChallenge(core(challenge));
    });
}

- (void)connection:(NSURLConnection *)connection didCancelAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge
{
    // FIXME: We probably don't need to implement this (see <rdar://problem/8960124>).

    ASSERT(!isMainThread());
    UNUSED_PARAM(connection);

    LOG(Network, "Handle %p delegate connection:%p didCancelAuthenticationChallenge:%p", m_handle, connection, challenge);

    dispatch_async(dispatch_get_main_queue(), ^{
        if (!m_handle)
            return;
        m_handle->didCancelAuthenticationChallenge(core(challenge));
    });
}

#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
- (BOOL)connection:(NSURLConnection *)connection canAuthenticateAgainstProtectionSpace:(NSURLProtectionSpace *)protectionSpace
{
    ASSERT(!isMainThread());
    UNUSED_PARAM(connection);

    LOG(Network, "Handle %p delegate connection:%p canAuthenticateAgainstProtectionSpace:%@://%@:%u realm:%@ method:%@ %@%@", m_handle, connection, [protectionSpace protocol], [protectionSpace host], [protectionSpace port], [protectionSpace realm], [protectionSpace authenticationMethod], [protectionSpace isProxy] ? @"proxy:" : @"", [protectionSpace isProxy] ? [protectionSpace proxyType] : @"");

    RetainPtr<id> protector(self);

    dispatch_async(dispatch_get_main_queue(), ^{
        if (!m_handle) {
            m_boolResult = NO;
            dispatch_semaphore_signal(m_semaphore);
            return;
        }
        m_handle->canAuthenticateAgainstProtectionSpace(core(protectionSpace));
    });

    dispatch_semaphore_wait(m_semaphore, DISPATCH_TIME_FOREVER);
    return m_boolResult;
}
#endif

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)r
{
    ASSERT(!isMainThread());
    UNUSED_PARAM(connection);

    LOG(Network, "Handle %p delegate connection:%p didReceiveResponse:%p (HTTP status %d, reported MIMEType '%s')", m_handle, connection, r, [r respondsToSelector:@selector(statusCode)] ? [(id)r statusCode] : 0, [[r MIMEType] UTF8String]);

    RetainPtr<id> protector(self);

    dispatch_async(dispatch_get_main_queue(), ^{
        if (!m_handle) {
            dispatch_semaphore_signal(m_semaphore);
            return;
        }

        // Avoid MIME type sniffing if the response comes back as 304 Not Modified.
        int statusCode = [r respondsToSelector:@selector(statusCode)] ? [(id)r statusCode] : 0;
        if (statusCode != 304)
            adjustMIMETypeIfNecessary([r _CFURLResponse]);

        if ([m_handle->firstRequest().nsURLRequest(DoNotUpdateHTTPBody) _propertyForKey:@"ForceHTMLMIMEType"])
            [r _setMIMEType:@"text/html"];

        m_handle->client()->didReceiveResponseAsync(m_handle, r);
    });

    dispatch_semaphore_wait(m_semaphore, DISPATCH_TIME_FOREVER);
}

#if USE(NETWORK_CFDATA_ARRAY_CALLBACK)
- (void)connection:(NSURLConnection *)connection didReceiveDataArray:(NSArray *)dataArray
{
    ASSERT(!isMainThread());
    UNUSED_PARAM(connection);

    LOG(Network, "Handle %p delegate connection:%p didReceiveDataArray:%p arraySize:%d", m_handle, connection, dataArray, [dataArray count]);

    dispatch_async(dispatch_get_main_queue(), ^{
        if (!dataArray)
            return;

        if (!m_handle || !m_handle->client())
            return;

        m_handle->handleDataArray(reinterpret_cast<CFArrayRef>(dataArray));
        // The call to didReceiveData above can cancel a load, and if so, the delegate (self) could have been deallocated by this point.
    });
}
#endif

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data lengthReceived:(long long)lengthReceived
{
    ASSERT(!isMainThread());
    UNUSED_PARAM(connection);
    UNUSED_PARAM(lengthReceived);

    LOG(Network, "Handle %p delegate connection:%p didReceiveData:%p lengthReceived:%lld", m_handle, connection, data, lengthReceived);

    dispatch_async(dispatch_get_main_queue(), ^{
        if (!m_handle || !m_handle->client())
            return;
        // FIXME: If we get more than 2B bytes in a single chunk, this code won't do the right thing.
        // However, with today's computers and networking speeds, this won't happen in practice.
        // Could be an issue with a giant local file.

        // FIXME: https://bugs.webkit.org/show_bug.cgi?id=19793
        // -1 means we do not provide any data about transfer size to inspector so it would use
        // Content-Length headers or content size to show transfer size.
        m_handle->client()->didReceiveBuffer(m_handle, SharedBuffer::wrapNSData(data), -1);
    });
}

- (void)connection:(NSURLConnection *)connection willStopBufferingData:(NSData *)data
{
    ASSERT(!isMainThread());
    UNUSED_PARAM(connection);

    LOG(Network, "Handle %p delegate connection:%p willStopBufferingData:%p", m_handle, connection, data);

    dispatch_async(dispatch_get_main_queue(), ^{
        if (!m_handle || !m_handle->client())
            return;
        // FIXME: If we get a resource with more than 2B bytes, this code won't do the right thing.
        // However, with today's computers and networking speeds, this won't happen in practice.
        // Could be an issue with a giant local file.
        m_handle->client()->willStopBufferingData(m_handle, (const char*)[data bytes], static_cast<int>([data length]));
    });
}

- (void)connection:(NSURLConnection *)connection didSendBodyData:(NSInteger)bytesWritten totalBytesWritten:(NSInteger)totalBytesWritten totalBytesExpectedToWrite:(NSInteger)totalBytesExpectedToWrite
{
    ASSERT(!isMainThread());
    UNUSED_PARAM(connection);
    UNUSED_PARAM(bytesWritten);

    LOG(Network, "Handle %p delegate connection:%p didSendBodyData:%d totalBytesWritten:%d totalBytesExpectedToWrite:%d", m_handle, connection, bytesWritten, totalBytesWritten, totalBytesExpectedToWrite);

    dispatch_async(dispatch_get_main_queue(), ^{
        if (!m_handle || !m_handle->client())
            return;
        m_handle->client()->didSendData(m_handle, totalBytesWritten, totalBytesExpectedToWrite);
    });
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection
{
    ASSERT(!isMainThread());
    UNUSED_PARAM(connection);

    LOG(Network, "Handle %p delegate connectionDidFinishLoading:%p", m_handle, connection);

    dispatch_async(dispatch_get_main_queue(), ^{
        if (!m_handle || !m_handle->client())
            return;

        m_handle->client()->didFinishLoading(m_handle, 0);
    });
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
    ASSERT(!isMainThread());
    UNUSED_PARAM(connection);

    LOG(Network, "Handle %p delegate connection:%p didFailWithError:%@", m_handle, connection, error);

    dispatch_async(dispatch_get_main_queue(), ^{
        if (!m_handle || !m_handle->client())
            return;

        m_handle->client()->didFail(m_handle, error);
    });
}


- (NSCachedURLResponse *)connection:(NSURLConnection *)connection willCacheResponse:(NSCachedURLResponse *)cachedResponse
{
    ASSERT(!isMainThread());
    UNUSED_PARAM(connection);

    LOG(Network, "Handle %p delegate connection:%p willCacheResponse:%p", m_handle, connection, cachedResponse);

    RetainPtr<id> protector(self);

    dispatch_async(dispatch_get_main_queue(), ^{
        if (!m_handle || !m_handle->client()) {
            m_cachedResponseResult = nullptr;
            dispatch_semaphore_signal(m_semaphore);
            return;
        }

        // Workaround for <rdar://problem/6300990> Caching does not respect Vary HTTP header.
        // FIXME: WebCore cache has issues with Vary, too (bug 58797, bug 71509).
        if ([[cachedResponse response] isKindOfClass:[NSHTTPURLResponse class]]
            && [[(NSHTTPURLResponse *)[cachedResponse response] allHeaderFields] objectForKey:@"Vary"]) {
            m_cachedResponseResult = nullptr;
            dispatch_semaphore_signal(m_semaphore);
            return;
        }

        m_handle->client()->willCacheResponseAsync(m_handle, cachedResponse);
    });

    dispatch_semaphore_wait(m_semaphore, DISPATCH_TIME_FOREVER);
    return [m_cachedResponseResult.leakRef() autorelease];
}

@end

#endif // !USE(CFNETWORK)

