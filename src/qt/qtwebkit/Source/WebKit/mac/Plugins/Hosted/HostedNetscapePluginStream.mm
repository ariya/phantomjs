/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#if USE(PLUGIN_HOST_PROCESS) && ENABLE(NETSCAPE_PLUGIN_API)

#import "HostedNetscapePluginStream.h"

#import "NetscapePluginHostProxy.h"
#import "NetscapePluginInstanceProxy.h"
#import "WebFrameInternal.h"
#import "WebHostedNetscapePluginView.h"
#import "WebKitErrorsPrivate.h"
#import "WebKitPluginHost.h"
#import "WebKitSystemInterface.h"
#import "WebNSURLExtras.h"
#import "WebNSURLRequestExtras.h"
#import <WebCore/Document.h>
#import <WebCore/DocumentLoader.h>
#import <WebCore/Frame.h>
#import <WebCore/FrameLoader.h>
#import <WebCore/ResourceLoadScheduler.h>
#import <WebCore/SecurityOrigin.h>
#import <WebCore/SecurityPolicy.h>
#import <WebCore/WebCoreURLResponse.h>
#import <wtf/RefCountedLeakCounter.h>

using namespace WebCore;

namespace WebKit {

DEFINE_DEBUG_ONLY_GLOBAL(WTF::RefCountedLeakCounter, hostedNetscapePluginStreamCounter, ("HostedNetscapePluginStream"));

HostedNetscapePluginStream::HostedNetscapePluginStream(NetscapePluginInstanceProxy* instance, uint32_t streamID, NSURLRequest *request)
    : m_instance(instance)
    , m_streamID(streamID)
    , m_request(adoptNS([request mutableCopy]))
    , m_requestURL([request URL])
    , m_frameLoader(0)
{
    String referrer = SecurityPolicy::generateReferrerHeader(core([instance->pluginView() webFrame])->document()->referrerPolicy(), [request URL], core([instance->pluginView() webFrame])->loader()->outgoingReferrer());
    if (referrer.isEmpty())
        [m_request.get() _web_setHTTPReferrer:nil];
    else
        [m_request.get() _web_setHTTPReferrer:referrer];

#ifndef NDEBUG
    hostedNetscapePluginStreamCounter.increment();
#endif
}

HostedNetscapePluginStream::HostedNetscapePluginStream(NetscapePluginInstanceProxy* instance, WebCore::FrameLoader* frameLoader)
    : m_instance(instance)
    , m_streamID(1)
    , m_frameLoader(frameLoader)
{
#ifndef NDEBUG
    hostedNetscapePluginStreamCounter.increment();
#endif
}

HostedNetscapePluginStream::~HostedNetscapePluginStream()
{
#ifndef NDEBUG
    hostedNetscapePluginStreamCounter.decrement();
#endif
}

void HostedNetscapePluginStream::startStreamWithResponse(NSURLResponse *response)
{
    didReceiveResponse(0, response);
}
    
void HostedNetscapePluginStream::startStream(NSURL *responseURL, long long expectedContentLength, NSDate *lastModifiedDate, NSString *mimeType, NSData *headers)
{
    m_responseURL = responseURL;
    m_mimeType = mimeType;

    char* mimeTypeUTF8 = const_cast<char*>([mimeType UTF8String]);
    int mimeTypeUTF8Length = mimeTypeUTF8 ? strlen (mimeTypeUTF8) + 1 : 0;
    
    const char *url = [responseURL _web_URLCString];
    int urlLength = url ? strlen(url) + 1 : 0;
    
    _WKPHStartStream(m_instance->hostProxy()->port(),
                     m_instance->pluginID(),
                     m_streamID,
                     const_cast<char*>(url), urlLength,
                     expectedContentLength,
                     [lastModifiedDate timeIntervalSince1970],
                     mimeTypeUTF8, mimeTypeUTF8Length,
                     const_cast<char*>(reinterpret_cast<const char*>([headers bytes])), [headers length]);
}
                     
void HostedNetscapePluginStream::didReceiveData(WebCore::NetscapePlugInStreamLoader*, const char* bytes, int length)
{
    _WKPHStreamDidReceiveData(m_instance->hostProxy()->port(),
                              m_instance->pluginID(),
                              m_streamID,
                              const_cast<char*>(bytes), length);
}
    
void HostedNetscapePluginStream::didFinishLoading(WebCore::NetscapePlugInStreamLoader*)
{
    _WKPHStreamDidFinishLoading(m_instance->hostProxy()->port(),
                                m_instance->pluginID(),
                                m_streamID);
    m_instance->disconnectStream(this);
}
    
void HostedNetscapePluginStream::didReceiveResponse(NetscapePlugInStreamLoader*, const ResourceResponse& response)
{
    NSURLResponse *r = response.nsURLResponse();
    
    NSMutableData *theHeaders = nil;
    long long expectedContentLength = [r expectedContentLength];
    
    if ([r isKindOfClass:[NSHTTPURLResponse class]]) {
        NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *)r;
        theHeaders = [NSMutableData dataWithCapacity:1024];
        
        // FIXME: it would be nice to be able to get the raw HTTP header block.
        // This includes the HTTP version, the real status text,
        // all headers in their original order and including duplicates,
        // and all original bytes verbatim, rather than sent through Unicode translation.
        // Unfortunately NSHTTPURLResponse doesn't provide access at that low a level.
        
        [theHeaders appendBytes:"HTTP " length:5];
        char statusStr[10];
        long statusCode = [httpResponse statusCode];
        snprintf(statusStr, sizeof(statusStr), "%ld", statusCode);
        [theHeaders appendBytes:statusStr length:strlen(statusStr)];
        [theHeaders appendBytes:" OK\n" length:4];
        
        // HACK: pass the headers through as UTF-8.
        // This is not the intended behavior; we're supposed to pass original bytes verbatim.
        // But we don't have the original bytes, we have NSStrings built by the URL loading system.
        // It hopefully shouldn't matter, since RFC2616/RFC822 require ASCII-only headers,
        // but surely someone out there is using non-ASCII characters, and hopefully UTF-8 is adequate here.
        // It seems better than NSASCIIStringEncoding, which will lose information if non-ASCII is used.
        
        NSDictionary *headerDict = [httpResponse allHeaderFields];
        NSArray *keys = [[headerDict allKeys] sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)];
        NSEnumerator *i = [keys objectEnumerator];
        NSString *k;
        while ((k = [i nextObject]) != nil) {
            NSString *v = [headerDict objectForKey:k];
            [theHeaders appendData:[k dataUsingEncoding:NSUTF8StringEncoding]];
            [theHeaders appendBytes:": " length:2];
            [theHeaders appendData:[v dataUsingEncoding:NSUTF8StringEncoding]];
            [theHeaders appendBytes:"\n" length:1];
        }
        
        // If the content is encoded (most likely compressed), then don't send its length to the plugin,
        // which is only interested in the decoded length, not yet known at the moment.
        // <rdar://problem/4470599> tracks a request for -[NSURLResponse expectedContentLength] to incorporate this logic.
        NSString *contentEncoding = (NSString *)[[(NSHTTPURLResponse *)r allHeaderFields] objectForKey:@"Content-Encoding"];
        if (contentEncoding && ![contentEncoding isEqualToString:@"identity"])
            expectedContentLength = -1;

        [theHeaders appendBytes:"\0" length:1];
    }
    
    startStream([r URL], expectedContentLength, WKGetNSURLResponseLastModifiedDate(r), [r MIMEType], theHeaders);
}

NPReason HostedNetscapePluginStream::reasonForError(NSError *error)
{
    if (!error)
        return NPRES_DONE;

    if ([[error domain] isEqualToString:NSURLErrorDomain] && [error code] == NSURLErrorCancelled)
        return NPRES_USER_BREAK;

    return NPRES_NETWORK_ERR;
}

void HostedNetscapePluginStream::didFail(WebCore::NetscapePlugInStreamLoader*, const WebCore::ResourceError& error)
{
    if (NetscapePluginHostProxy* hostProxy = m_instance->hostProxy())
        _WKPHStreamDidFail(hostProxy->port(), m_instance->pluginID(), m_streamID, reasonForError(error));
    m_instance->disconnectStream(this);
}

bool HostedNetscapePluginStream::wantsAllStreams() const
{
    // FIXME: Implement.
    return false;
}

void HostedNetscapePluginStream::start()
{
    ASSERT(m_request);
    ASSERT(!m_frameLoader);
    ASSERT(!m_loader);
    
    m_loader = resourceLoadScheduler()->schedulePluginStreamLoad(core([m_instance->pluginView() webFrame]), this, m_request.get());
}

void HostedNetscapePluginStream::stop()
{
    ASSERT(!m_frameLoader);
    
    if (!m_loader->isDone())
        m_loader->cancel(m_loader->cancelledError());
}
    
void HostedNetscapePluginStream::cancelLoad(NPReason reason)
{
    cancelLoad(errorForReason(reason));
}

void HostedNetscapePluginStream::cancelLoad(NSError *error)
{
    if (m_frameLoader) {
        ASSERT(!m_loader);
        
        DocumentLoader* documentLoader = m_frameLoader->activeDocumentLoader();
        if (documentLoader && documentLoader->isLoadingMainResource())
            documentLoader->cancelMainResourceLoad(error);
        return;
    }

    if (!m_loader->isDone()) {
        // Cancelling the load will disconnect the stream so there's no need to do it explicitly.
        m_loader->cancel(error);
    } else
        m_instance->disconnectStream(this);
}

NSError *HostedNetscapePluginStream::pluginCancelledConnectionError() const
{
    return [[[NSError alloc] _initWithPluginErrorCode:WebKitErrorPlugInCancelledConnection
                                           contentURL:m_responseURL ? m_responseURL.get() : m_requestURL.get()
                                        pluginPageURL:nil
                                           pluginName:[[m_instance->pluginView() pluginPackage] pluginInfo].name
                                             MIMEType:m_mimeType.get()] autorelease];
}

NSError *HostedNetscapePluginStream::errorForReason(NPReason reason) const
{
    if (reason == NPRES_DONE)
        return nil;

    if (reason == NPRES_USER_BREAK)
        return [NSError _webKitErrorWithDomain:NSURLErrorDomain
                                          code:NSURLErrorCancelled 
                                           URL:m_responseURL ? m_responseURL.get() : m_requestURL.get()];

    return pluginCancelledConnectionError();
}

} // namespace WebKit

#endif // USE(PLUGIN_HOST_PROCESS) && ENABLE(NETSCAPE_PLUGIN_API)

