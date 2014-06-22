/*
 * Copyright (C) 2005 Apple Computer, Inc.  All rights reserved.
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
#import <Foundation/Foundation.h>

#import <WebCore/Timer.h>
#import <WebCore/NetscapePlugInStreamLoader.h>
#import <WebKit/npfunctions.h>
#import <wtf/PassRefPtr.h>
#import <wtf/RefCounted.h>
#import <wtf/RefPtr.h>
#import <wtf/RetainPtr.h>
#import <wtf/text/CString.h>

#import "WebNetscapePluginView.h"

namespace WebCore {
    class FrameLoader;
    class NetscapePlugInStreamLoader;
}

@class WebNetscapePluginView;
@class NSURLResponse;

class WebNetscapePluginStream : public RefCounted<WebNetscapePluginStream>
                              , private WebCore::NetscapePlugInStreamLoaderClient
{
public:
    static PassRefPtr<WebNetscapePluginStream> create(NSURLRequest *request, NPP plugin, bool sendNotification, void* notifyData)
    {
        return adoptRef(new WebNetscapePluginStream(request, plugin, sendNotification, notifyData));
    }
    static PassRefPtr<WebNetscapePluginStream> create(WebCore::FrameLoader* frameLoader)
    {
        return adoptRef(new WebNetscapePluginStream(frameLoader));
    }
    virtual ~WebNetscapePluginStream();

    NPP plugin() const { return m_plugin; }
    void setPlugin(NPP);
    
    static NPP ownerForStream(NPStream *);

    static NPReason reasonForError(NSError *);
    NSError *errorForReason(NPReason) const;

    void cancelLoadAndDestroyStreamWithError(NSError *);

    void setRequestURL(const WebCore::KURL& requestURL) { m_requestURL = requestURL; }

    void start();
    void stop();
    
    void startStreamWithResponse(NSURLResponse *response);
    
    void didReceiveData(WebCore::NetscapePlugInStreamLoader*, const char* bytes, int length);
    void destroyStreamWithError(NSError *);
    void didFinishLoading(WebCore::NetscapePlugInStreamLoader*);

private:
    void destroyStream();
    void cancelLoadWithError(NSError *);
    void destroyStreamWithReason(NPReason);
    void deliverDataToFile(NSData *data);
    void deliverData();

    void startStream(NSURL *, long long expectedContentLength, NSDate *lastModifiedDate, const WTF::String& mimeType, NSData *headers);
    
    NSError *pluginCancelledConnectionError() const;

    // NetscapePlugInStreamLoaderClient methods.
    void didReceiveResponse(WebCore::NetscapePlugInStreamLoader*, const WebCore::ResourceResponse&);
    void didFail(WebCore::NetscapePlugInStreamLoader*, const WebCore::ResourceError&);
    bool wantsAllStreams() const;

    RetainPtr<NSMutableData> m_deliveryData;
    WebCore::KURL m_requestURL;
    RetainPtr<NSURL> m_responseURL;
    CString m_mimeType;

    NPP m_plugin;
    uint16_t m_transferMode;
    int32_t m_offset;
    NPStream m_stream;
    RetainPtr<NSString> m_path;
    int m_fileDescriptor;
    BOOL m_sendNotification;
    void *m_notifyData;
    char *m_headers;
    RetainPtr<WebNetscapePluginView> m_pluginView;
    NPReason m_reason;
    bool m_isTerminated;
    bool m_newStreamSuccessful;
    
    WebCore::FrameLoader* m_frameLoader;
    RefPtr<WebCore::NetscapePlugInStreamLoader> m_loader;
    RetainPtr<NSMutableURLRequest> m_request;
    NPPluginFuncs *m_pluginFuncs;

    void deliverDataTimerFired(WebCore::Timer<WebNetscapePluginStream>* timer);
    WebCore::Timer<WebNetscapePluginStream> m_deliverDataTimer;
    
    WebNetscapePluginStream(WebCore::FrameLoader*);
    WebNetscapePluginStream(NSURLRequest *, NPP, bool sendNotification, void* notifyData);
};

#endif
