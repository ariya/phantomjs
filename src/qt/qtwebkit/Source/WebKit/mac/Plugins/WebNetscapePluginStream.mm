/*
 * Copyright (C) 2005, 2006, 2007 Apple Inc. All rights reserved.
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

#if ENABLE(NETSCAPE_PLUGIN_API)
#import "WebNetscapePluginStream.h"

#import "WebFrameInternal.h"
#import "WebKitErrorsPrivate.h"
#import "WebKitLogging.h"
#import "WebNSObjectExtras.h"
#import "WebNSURLExtras.h"
#import "WebNSURLRequestExtras.h"
#import "WebNetscapePluginPackage.h"
#import "WebNetscapePluginView.h"
#import <Foundation/NSURLResponse.h>
#import <WebCore/Document.h>
#import <WebCore/DocumentLoader.h>
#import <WebCore/Frame.h>
#import <WebCore/FrameLoader.h>
#import <WebCore/JSDOMWindowBase.h>
#import <WebCore/ResourceLoadScheduler.h>
#import <WebCore/SecurityOrigin.h>
#import <WebCore/SecurityPolicy.h>
#import <WebCore/WebCoreObjCExtras.h>
#import <WebCore/WebCoreURLResponse.h>
#import <WebKitSystemInterface.h>
#import <runtime/JSLock.h>
#import <wtf/HashMap.h>
#import <wtf/StdLibExtras.h>

using namespace WebCore;

#define WEB_REASON_NONE -1

class PluginStopDeferrer {
public:
    PluginStopDeferrer(WebNetscapePluginView* pluginView)
        : m_pluginView(pluginView)
    {
        ASSERT(m_pluginView);
        
        [m_pluginView.get() willCallPlugInFunction];
    }
    
    ~PluginStopDeferrer()
    {
        ASSERT(m_pluginView);
        [m_pluginView.get() didCallPlugInFunction];
    }
    
private:
    RetainPtr<WebNetscapePluginView> m_pluginView;
};

typedef HashMap<NPStream*, NPP> StreamMap;
static StreamMap& streams()
{
    DEFINE_STATIC_LOCAL(StreamMap, staticStreams, ());
    return staticStreams;
}

NPP WebNetscapePluginStream::ownerForStream(NPStream *stream)
{
    return streams().get(stream);
}

NPReason WebNetscapePluginStream::reasonForError(NSError *error)
{
    if (!error)
        return NPRES_DONE;

    if ([[error domain] isEqualToString:NSURLErrorDomain] && [error code] == NSURLErrorCancelled)
        return NPRES_USER_BREAK;

    return NPRES_NETWORK_ERR;
}

NSError *WebNetscapePluginStream::pluginCancelledConnectionError() const
{
    return [[[NSError alloc] _initWithPluginErrorCode:WebKitErrorPlugInCancelledConnection
                                           contentURL:m_responseURL ? m_responseURL.get() : (NSURL *)m_requestURL
                                        pluginPageURL:nil
                                           pluginName:[[m_pluginView.get() pluginPackage] pluginInfo].name
                                             MIMEType:(NSString *)String::fromUTF8(m_mimeType.data(), m_mimeType.length())] autorelease];
}

NSError *WebNetscapePluginStream::errorForReason(NPReason reason) const
{
    if (reason == NPRES_DONE)
        return nil;

    if (reason == NPRES_USER_BREAK)
        return [NSError _webKitErrorWithDomain:NSURLErrorDomain
                                          code:NSURLErrorCancelled 
                                           URL:m_responseURL ? m_responseURL.get() : (NSURL *)m_requestURL];

    return pluginCancelledConnectionError();
}

WebNetscapePluginStream::WebNetscapePluginStream(FrameLoader* frameLoader)
    : m_plugin(0)
    , m_transferMode(0)
    , m_offset(0)
    , m_fileDescriptor(-1)
    , m_sendNotification(false)
    , m_notifyData(0)
    , m_headers(0)
    , m_reason(NPRES_BASE)
    , m_isTerminated(false)
    , m_newStreamSuccessful(false)
    , m_frameLoader(frameLoader)
    , m_pluginFuncs(0)
    , m_deliverDataTimer(this, &WebNetscapePluginStream::deliverDataTimerFired)
{
    memset(&m_stream, 0, sizeof(NPStream));
}

WebNetscapePluginStream::WebNetscapePluginStream(NSURLRequest *request, NPP plugin, bool sendNotification, void* notifyData)
    : m_requestURL([request URL])
    , m_plugin(0)
    , m_transferMode(0)
    , m_offset(0)
    , m_fileDescriptor(-1)
    , m_sendNotification(sendNotification)
    , m_notifyData(notifyData)
    , m_headers(0)
    , m_reason(NPRES_BASE)
    , m_isTerminated(false)
    , m_newStreamSuccessful(false)
    , m_frameLoader(0)
    , m_request(adoptNS([request mutableCopy]))
    , m_pluginFuncs(0)
    , m_deliverDataTimer(this, &WebNetscapePluginStream::deliverDataTimerFired)
{
    memset(&m_stream, 0, sizeof(NPStream));

    WebNetscapePluginView *view = (WebNetscapePluginView *)plugin->ndata;
    
    // This check has already been done by the plug-in view.
    ASSERT(core([view webFrame])->document()->securityOrigin()->canDisplay([request URL]));
    
    ASSERT([request URL]);
    ASSERT(plugin);
    
    setPlugin(plugin);
    
    streams().add(&m_stream, plugin);
    
    String referrer = SecurityPolicy::generateReferrerHeader(core([view webFrame])->document()->referrerPolicy(), [request URL], core([view webFrame])->loader()->outgoingReferrer());
    if (referrer.isEmpty())
        [m_request.get() _web_setHTTPReferrer:nil];
    else
        [m_request.get() _web_setHTTPReferrer:referrer];
}

WebNetscapePluginStream::~WebNetscapePluginStream()
{
    ASSERT(!m_plugin);
    ASSERT(m_isTerminated);
    ASSERT(!m_stream.ndata);
    
    // The stream file should have been deleted, and the path freed, in -_destroyStream
    ASSERT(!m_path);
    ASSERT(m_fileDescriptor == -1);
    
    free((void *)m_stream.url);
    free(m_headers);
    
    streams().remove(&m_stream);
}

void WebNetscapePluginStream::setPlugin(NPP plugin)
{
    if (plugin) {
        m_plugin = plugin;
        m_pluginView = static_cast<WebNetscapePluginView *>(m_plugin->ndata);

        WebNetscapePluginPackage *pluginPackage = [m_pluginView.get() pluginPackage];
        
        m_pluginFuncs = [pluginPackage pluginFuncs];
    } else {
        WebNetscapePluginView *view = m_pluginView.get();
        m_plugin = 0;
        m_pluginFuncs = 0;
        
        [view disconnectStream:this];
        m_pluginView = 0;
    }        
}

void WebNetscapePluginStream::startStream(NSURL *url, long long expectedContentLength, NSDate *lastModifiedDate, const String& mimeType, NSData *headers)
{
    ASSERT(!m_isTerminated);
    
    m_responseURL = url;
    m_mimeType = mimeType.utf8();
    
    free((void *)m_stream.url);
    m_stream.url = strdup([m_responseURL.get() _web_URLCString]);

    m_stream.ndata = this;
    m_stream.end = expectedContentLength > 0 ? (uint32_t)expectedContentLength : 0;
    m_stream.lastmodified = (uint32_t)[lastModifiedDate timeIntervalSince1970];
    m_stream.notifyData = m_notifyData;

    if (headers) {
        unsigned len = [headers length];
        m_headers = (char*) malloc(len + 1);
        [headers getBytes:m_headers];
        m_headers[len] = 0;
        m_stream.headers = m_headers;
    }
    
    m_transferMode = NP_NORMAL;
    m_offset = 0;
    m_reason = WEB_REASON_NONE;
    // FIXME: If WebNetscapePluginStream called our initializer we wouldn't have to do this here.
    m_fileDescriptor = -1;

    // FIXME: Need a way to check if stream is seekable

    NPError npErr;
    {
        PluginStopDeferrer deferrer(m_pluginView.get());
        npErr = m_pluginFuncs->newstream(m_plugin, m_mimeType.mutableData(), &m_stream, NO, &m_transferMode);
    }

    LOG(Plugins, "NPP_NewStream URL=%@ MIME=%s error=%d", m_responseURL.get(), m_mimeType.data(), npErr);

    if (npErr != NPERR_NO_ERROR) {
        LOG_ERROR("NPP_NewStream failed with error: %d responseURL: %@", npErr, m_responseURL.get());
        // Calling cancelLoadWithError: cancels the load, but doesn't call NPP_DestroyStream.
        cancelLoadWithError(pluginCancelledConnectionError());
        return;
    }

    m_newStreamSuccessful = true;

    switch (m_transferMode) {
        case NP_NORMAL:
            LOG(Plugins, "Stream type: NP_NORMAL");
            break;
        case NP_ASFILEONLY:
            LOG(Plugins, "Stream type: NP_ASFILEONLY");
            break;
        case NP_ASFILE:
            LOG(Plugins, "Stream type: NP_ASFILE");
            break;
        case NP_SEEK:
            LOG_ERROR("Stream type: NP_SEEK not yet supported");
            cancelLoadAndDestroyStreamWithError(pluginCancelledConnectionError());
            break;
        default:
            LOG_ERROR("unknown stream type");
    }
}

void WebNetscapePluginStream::start()
{
    ASSERT(m_request);
    ASSERT(!m_frameLoader);
    ASSERT(!m_loader);
    
    m_loader = resourceLoadScheduler()->schedulePluginStreamLoad(core([m_pluginView.get() webFrame]), this, m_request.get());
}

void WebNetscapePluginStream::stop()
{
    ASSERT(!m_frameLoader);
    
    if (!m_loader->isDone())
        cancelLoadAndDestroyStreamWithError(m_loader->cancelledError());
}

void WebNetscapePluginStream::didReceiveResponse(NetscapePlugInStreamLoader*, const ResourceResponse& response)
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
        
        // startStreamResponseURL:... will null-terminate.
    }
    
    startStream([r URL], expectedContentLength, WKGetNSURLResponseLastModifiedDate(r), response.mimeType(), theHeaders);
}

void WebNetscapePluginStream::startStreamWithResponse(NSURLResponse *response)
{
    didReceiveResponse(0, response);
}

bool WebNetscapePluginStream::wantsAllStreams() const
{
    if (!m_pluginFuncs->getvalue)
        return false;
    
    void *value = 0;
    NPError error;
    {
        PluginStopDeferrer deferrer(m_pluginView.get());
        JSC::JSLock::DropAllLocks dropAllLocks(JSDOMWindowBase::commonVM());
        error = m_pluginFuncs->getvalue(m_plugin, NPPVpluginWantsAllNetworkStreams, &value);
    }
    if (error != NPERR_NO_ERROR)
        return false;
    
    return value;
}

void WebNetscapePluginStream::destroyStream()
{
    if (m_isTerminated)
        return;

    RefPtr<WebNetscapePluginStream> protect(this);

    ASSERT(m_reason != WEB_REASON_NONE);
    ASSERT([m_deliveryData.get() length] == 0);
    
    m_deliverDataTimer.stop();

    if (m_stream.ndata) {
        if (m_reason == NPRES_DONE && (m_transferMode == NP_ASFILE || m_transferMode == NP_ASFILEONLY)) {
            ASSERT(m_fileDescriptor == -1);
            ASSERT(m_path);

            PluginStopDeferrer deferrer(m_pluginView.get());
            m_pluginFuncs->asfile(m_plugin, &m_stream, [m_path.get() fileSystemRepresentation]);
            LOG(Plugins, "NPP_StreamAsFile responseURL=%@ path=%s", m_responseURL.get(), m_path.get());
        }

        if (m_path) {
            // Delete the file after calling NPP_StreamAsFile(), instead of in -dealloc/-finalize.  It should be OK
            // to delete the file here -- NPP_StreamAsFile() is always called immediately before NPP_DestroyStream()
            // (the stream destruction function), so there can be no expectation that a plugin will read the stream
            // file asynchronously after NPP_StreamAsFile() is called.
            unlink([m_path.get() fileSystemRepresentation]);
            m_path = 0;

            if (m_isTerminated)
                return;
        }

        if (m_fileDescriptor != -1) {
            // The file may still be open if we are destroying the stream before it completed loading.
            close(m_fileDescriptor);
            m_fileDescriptor = -1;
        }

        if (m_newStreamSuccessful) {
            PluginStopDeferrer deferrer(m_pluginView.get());
#if !LOG_DISABLED
            NPError npErr = 
#endif
            m_pluginFuncs->destroystream(m_plugin, &m_stream, m_reason);
            LOG(Plugins, "NPP_DestroyStream responseURL=%@ error=%d", m_responseURL.get(), npErr);
        }

        free(m_headers);
        m_headers = NULL;
        m_stream.headers = NULL;

        m_stream.ndata = 0;

        if (m_isTerminated)
            return;
    }

    if (m_sendNotification) {
        // NPP_URLNotify expects the request URL, not the response URL.
        PluginStopDeferrer deferrer(m_pluginView.get());
        m_pluginFuncs->urlnotify(m_plugin, m_requestURL.string().utf8().data(), m_reason, m_notifyData);
        LOG(Plugins, "NPP_URLNotify requestURL=%@ reason=%d", (NSURL *)m_requestURL, m_reason);
    }

    m_isTerminated = true;

    setPlugin(0);
}

void WebNetscapePluginStream::destroyStreamWithReason(NPReason reason)
{
    m_reason = reason;
    if (m_reason != NPRES_DONE) {
        // Stop any pending data from being streamed.
        [m_deliveryData.get() setLength:0];
    } else if ([m_deliveryData.get() length] > 0) {
        // There is more data to be streamed, don't destroy the stream now.
        return;
    }

    RefPtr<WebNetscapePluginStream> protect(this);
    destroyStream();
    ASSERT(!m_stream.ndata);
}

void WebNetscapePluginStream::cancelLoadWithError(NSError *error)
{
    if (m_frameLoader) {
        ASSERT(!m_loader);
        
        DocumentLoader* documentLoader = m_frameLoader->activeDocumentLoader();
        ASSERT(documentLoader);
        
        if (documentLoader->isLoadingMainResource())
            documentLoader->cancelMainResourceLoad(error);
        return;
    }
    
    if (!m_loader->isDone())
        m_loader->cancel(error);
}

void WebNetscapePluginStream::destroyStreamWithError(NSError *error)
{
    destroyStreamWithReason(reasonForError(error));
}

void WebNetscapePluginStream::didFail(WebCore::NetscapePlugInStreamLoader*, const WebCore::ResourceError& error)
{
    destroyStreamWithError(error);
}

void WebNetscapePluginStream::cancelLoadAndDestroyStreamWithError(NSError *error)
{
    RefPtr<WebNetscapePluginStream> protect(this);
    cancelLoadWithError(error);
    destroyStreamWithError(error);
    setPlugin(0);
}    

void WebNetscapePluginStream::deliverData()
{
    if (!m_stream.ndata || [m_deliveryData.get() length] == 0)
        return;

    RefPtr<WebNetscapePluginStream> protect(this);

    int32_t totalBytes = [m_deliveryData.get() length];
    int32_t totalBytesDelivered = 0;

    while (totalBytesDelivered < totalBytes) {
        PluginStopDeferrer deferrer(m_pluginView.get());
        int32_t deliveryBytes = m_pluginFuncs->writeready(m_plugin, &m_stream);
        LOG(Plugins, "NPP_WriteReady responseURL=%@ bytes=%d", m_responseURL.get(), deliveryBytes);

        if (m_isTerminated)
            return;

        if (deliveryBytes <= 0) {
            // Plug-in can't receive anymore data right now. Send it later.
            if (!m_deliverDataTimer.isActive())
                m_deliverDataTimer.startOneShot(0);
            break;
        } else {
            deliveryBytes = std::min(deliveryBytes, totalBytes - totalBytesDelivered);
            NSData *subdata = [m_deliveryData.get() subdataWithRange:NSMakeRange(totalBytesDelivered, deliveryBytes)];
            PluginStopDeferrer deferrer(m_pluginView.get());
            deliveryBytes = m_pluginFuncs->write(m_plugin, &m_stream, m_offset, [subdata length], (void *)[subdata bytes]);
            if (deliveryBytes < 0) {
                // Netscape documentation says that a negative result from NPP_Write means cancel the load.
                cancelLoadAndDestroyStreamWithError(pluginCancelledConnectionError());
                return;
            }
            deliveryBytes = std::min<int32_t>(deliveryBytes, [subdata length]);
            m_offset += deliveryBytes;
            totalBytesDelivered += deliveryBytes;
            LOG(Plugins, "NPP_Write responseURL=%@ bytes=%d total-delivered=%d/%d", m_responseURL.get(), deliveryBytes, m_offset, m_stream.end);
        }
    }

    if (totalBytesDelivered > 0) {
        if (totalBytesDelivered < totalBytes) {
            NSMutableData *newDeliveryData = [[NSMutableData alloc] initWithCapacity:totalBytes - totalBytesDelivered];
            [newDeliveryData appendBytes:(char *)[m_deliveryData.get() bytes] + totalBytesDelivered length:totalBytes - totalBytesDelivered];
            
            m_deliveryData = adoptNS(newDeliveryData);
        } else {
            [m_deliveryData.get() setLength:0];
            if (m_reason != WEB_REASON_NONE) 
                destroyStream();
        }
    }
}

void WebNetscapePluginStream::deliverDataTimerFired(WebCore::Timer<WebNetscapePluginStream>* timer)
{
    deliverData();
}

void WebNetscapePluginStream::deliverDataToFile(NSData *data)
{
    if (m_fileDescriptor == -1 && !m_path) {
        NSString *temporaryFileMask = [NSTemporaryDirectory() stringByAppendingPathComponent:@"WebKitPlugInStreamXXXXXX"];
        char *temporaryFileName = strdup([temporaryFileMask fileSystemRepresentation]);
        m_fileDescriptor = mkstemp(temporaryFileName);
        if (m_fileDescriptor == -1) {
            LOG_ERROR("Can't create a temporary file.");
            // This is not a network error, but the only error codes are "network error" and "user break".
            destroyStreamWithReason(NPRES_NETWORK_ERR);
            free(temporaryFileName);
            return;
        }

        m_path = [NSString stringWithUTF8String:temporaryFileName];
        free(temporaryFileName);
    }

    int dataLength = [data length];
    if (!dataLength)
        return;

    int byteCount = write(m_fileDescriptor, [data bytes], dataLength);
    if (byteCount != dataLength) {
        // This happens only rarely, when we are out of disk space or have a disk I/O error.
        LOG_ERROR("error writing to temporary file, errno %d", errno);
        close(m_fileDescriptor);
        m_fileDescriptor = -1;

        // This is not a network error, but the only error codes are "network error" and "user break".
        destroyStreamWithReason(NPRES_NETWORK_ERR);
        m_path = 0;
    }
}

void WebNetscapePluginStream::didFinishLoading(NetscapePlugInStreamLoader*)
{
    if (!m_stream.ndata)
        return;
    
    if (m_transferMode == NP_ASFILE || m_transferMode == NP_ASFILEONLY) {
        // Fake the delivery of an empty data to ensure that the file has been created
        deliverDataToFile([NSData data]);
        if (m_fileDescriptor != -1)
            close(m_fileDescriptor);
        m_fileDescriptor = -1;
    }
    
    destroyStreamWithReason(NPRES_DONE);
}

void WebNetscapePluginStream::didReceiveData(NetscapePlugInStreamLoader*, const char* bytes, int length)
{
    NSData *data = [[NSData alloc] initWithBytesNoCopy:(void*)bytes length:length freeWhenDone:NO];

    ASSERT([data length] > 0);
    
    if (m_transferMode != NP_ASFILEONLY) {
        if (!m_deliveryData)
            m_deliveryData = adoptNS([[NSMutableData alloc] initWithCapacity:[data length]]);
        [m_deliveryData.get() appendData:data];
        deliverData();
    }
    if (m_transferMode == NP_ASFILE || m_transferMode == NP_ASFILEONLY)
        deliverDataToFile(data);
    
    [data release];
}

#endif
