/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Collabora, Ltd. All rights reserved.
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

#ifndef PluginStream_h
#define PluginStream_h

#include "FileSystem.h"
#include "KURL.h"
#include "NetscapePlugInStreamLoader.h"
#include "PluginQuirkSet.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "Timer.h"
#include "npruntime_internal.h"
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace WebCore {
    class Frame;
    class PluginStream;

    enum PluginStreamState { StreamBeforeStarted, StreamStarted, StreamStopped };

    class PluginStreamClient {
    public:
        virtual ~PluginStreamClient() {}
        virtual void streamDidFinishLoading(PluginStream*) {}
    };

    class PluginStream : public RefCounted<PluginStream>, private NetscapePlugInStreamLoaderClient {
    public:
        static PassRefPtr<PluginStream> create(PluginStreamClient* client, Frame* frame, const ResourceRequest& request, bool sendNotification, void* notifyData, const NPPluginFuncs* functions, NPP instance, const PluginQuirkSet& quirks)
        {
            return adoptRef(new PluginStream(client, frame, request, sendNotification, notifyData, functions, instance, quirks));
        }
        virtual ~PluginStream();
        
        void start();
        void stop();

        void startStream();

        void setLoadManually(bool loadManually) { m_loadManually = loadManually; }

        void sendJavaScriptStream(const KURL& requestURL, const WTF::CString& resultString);
        void cancelAndDestroyStream(NPReason);

        static NPP ownerForStream(NPStream*);

        // NetscapePlugInStreamLoaderClient
        virtual void didReceiveResponse(NetscapePlugInStreamLoader*, const ResourceResponse&);
        virtual void didReceiveData(NetscapePlugInStreamLoader*, const char*, int);
        virtual void didFail(NetscapePlugInStreamLoader*, const ResourceError&);
        virtual void didFinishLoading(NetscapePlugInStreamLoader*);
        virtual bool wantsAllStreams() const;

    private:
        PluginStream(PluginStreamClient*, Frame*, const ResourceRequest&, bool sendNotification, void* notifyData, const NPPluginFuncs*, NPP instance, const PluginQuirkSet&);

        void deliverData();
        void destroyStream(NPReason);
        void destroyStream();

        ResourceRequest m_resourceRequest;
        ResourceResponse m_resourceResponse;

        PluginStreamClient* m_client;
        Frame* m_frame;
        RefPtr<NetscapePlugInStreamLoader> m_loader;
        void* m_notifyData;
        bool m_sendNotification;
        PluginStreamState m_streamState;
        bool m_loadManually;

        Timer<PluginStream> m_delayDeliveryTimer;
        void delayDeliveryTimerFired(Timer<PluginStream>*);

        OwnPtr< Vector<char> > m_deliveryData;

        PlatformFileHandle m_tempFileHandle;

        const NPPluginFuncs* m_pluginFuncs;
        NPP m_instance;
        uint16_t m_transferMode;
        int32_t m_offset;
        CString m_headers;
        String m_path;
        NPReason m_reason;
        NPStream m_stream;
        PluginQuirkSet m_quirks;
    };

} // namespace WebCore

#endif
