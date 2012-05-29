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

#include "config.h"
#include "PluginStream.h"

#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "PluginDebug.h"
#include "ResourceLoadScheduler.h"
#include "SharedBuffer.h"
#include "SubresourceLoader.h"
#include <wtf/StringExtras.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringConcatenate.h>

// We use -2 here because some plugins like to return -1 to indicate error
// and this way we won't clash with them.
static const int WebReasonNone = -2;

using std::max;
using std::min;

namespace WebCore {

typedef HashMap<NPStream*, NPP> StreamMap;
static StreamMap& streams()
{
    static StreamMap staticStreams;
    return staticStreams;
}

PluginStream::PluginStream(PluginStreamClient* client, Frame* frame, const ResourceRequest& resourceRequest, bool sendNotification, void* notifyData, const NPPluginFuncs* pluginFuncs, NPP instance, const PluginQuirkSet& quirks)
    : m_resourceRequest(resourceRequest)
    , m_client(client)
    , m_frame(frame)
    , m_notifyData(notifyData)
    , m_sendNotification(sendNotification)
    , m_streamState(StreamBeforeStarted)
    , m_loadManually(false)
    , m_delayDeliveryTimer(this, &PluginStream::delayDeliveryTimerFired)
    , m_tempFileHandle(invalidPlatformFileHandle)
    , m_pluginFuncs(pluginFuncs)
    , m_instance(instance)
    , m_quirks(quirks)
{
    ASSERT(m_instance);

    m_stream.url = 0;
    m_stream.ndata = 0;
    m_stream.pdata = 0;
    m_stream.end = 0;
    m_stream.notifyData = 0;
    m_stream.lastmodified = 0;

    streams().add(&m_stream, m_instance);
}

PluginStream::~PluginStream()
{
    ASSERT(m_streamState != StreamStarted);
    ASSERT(!m_loader);

    fastFree((char*)m_stream.url);

    streams().remove(&m_stream);
}

void PluginStream::start()
{
    ASSERT(!m_loadManually);
    m_loader = resourceLoadScheduler()->schedulePluginStreamLoad(m_frame, this, m_resourceRequest);
}

void PluginStream::stop()
{
    m_streamState = StreamStopped;

    if (m_loadManually) {
        ASSERT(!m_loader);

        DocumentLoader* documentLoader = m_frame->loader()->activeDocumentLoader();
        ASSERT(documentLoader);

        if (documentLoader->isLoadingMainResource())
            documentLoader->cancelMainResourceLoad(m_frame->loader()->cancelledError(m_resourceRequest));

        return;
    }

    if (m_loader) {
        m_loader->cancel();
        m_loader = 0;
    }

    m_client = 0;
}

void PluginStream::startStream()
{
    ASSERT(m_streamState == StreamBeforeStarted);

    const KURL& responseURL = m_resourceResponse.url();

    // Some plugins (Flash) expect that javascript URLs are passed back decoded as this is the
    // format used when requesting the URL.
    if (protocolIsJavaScript(responseURL))
        m_stream.url = fastStrDup(decodeURLEscapeSequences(responseURL.string()).utf8().data());
    else
        m_stream.url = fastStrDup(responseURL.string().utf8().data());

    CString mimeTypeStr = m_resourceResponse.mimeType().utf8();

    long long expectedContentLength = m_resourceResponse.expectedContentLength();

    if (m_resourceResponse.isHTTP()) {
        Vector<UChar> stringBuilder;
        String separator(": ");

        String statusLine = makeString("HTTP ", String::number(m_resourceResponse.httpStatusCode()), " OK\n");
        stringBuilder.append(statusLine.characters(), statusLine.length());

        HTTPHeaderMap::const_iterator end = m_resourceResponse.httpHeaderFields().end();
        for (HTTPHeaderMap::const_iterator it = m_resourceResponse.httpHeaderFields().begin(); it != end; ++it) {
            stringBuilder.append(it->first.characters(), it->first.length());
            stringBuilder.append(separator.characters(), separator.length());
            stringBuilder.append(it->second.characters(), it->second.length());
            stringBuilder.append('\n');
        }

        m_headers = String::adopt(stringBuilder).utf8();

        // If the content is encoded (most likely compressed), then don't send its length to the plugin,
        // which is only interested in the decoded length, not yet known at the moment.
        // <rdar://problem/4470599> tracks a request for -[NSURLResponse expectedContentLength] to incorporate this logic.
        String contentEncoding = m_resourceResponse.httpHeaderField("Content-Encoding");
        if (!contentEncoding.isNull() && contentEncoding != "identity")
            expectedContentLength = -1;
    }

    m_stream.headers = m_headers.data();
    m_stream.pdata = 0;
    m_stream.ndata = this;
    m_stream.end = max(expectedContentLength, 0LL);
    m_stream.lastmodified = m_resourceResponse.lastModifiedDate();
    m_stream.notifyData = m_notifyData;

    m_transferMode = NP_NORMAL;
    m_offset = 0;
    m_reason = WebReasonNone;

    // Protect the stream if destroystream is called from within the newstream handler
    RefPtr<PluginStream> protect(this);

    // calling into a plug-in could result in re-entrance if the plug-in yields
    // control to the system (rdar://5744899). prevent this by deferring further
    // loading while calling into the plug-in.
    if (m_loader)
        m_loader->setDefersLoading(true);
    NPError npErr = m_pluginFuncs->newstream(m_instance, (NPMIMEType)mimeTypeStr.data(), &m_stream, false, &m_transferMode);
    if (m_loader)
        m_loader->setDefersLoading(false);
    
    // If the stream was destroyed in the call to newstream we return
    if (m_reason != WebReasonNone)
        return;
        
    if (npErr != NPERR_NO_ERROR) {
        cancelAndDestroyStream(npErr);
        return;
    }

    m_streamState = StreamStarted;

    if (m_transferMode == NP_NORMAL)
        return;

    m_path = openTemporaryFile("WKP", m_tempFileHandle);

    // Something went wrong, cancel loading the stream
    if (!isHandleValid(m_tempFileHandle))
        cancelAndDestroyStream(NPRES_NETWORK_ERR);
}

NPP PluginStream::ownerForStream(NPStream* stream)
{
    return streams().get(stream);
}

void PluginStream::cancelAndDestroyStream(NPReason reason)
{
    RefPtr<PluginStream> protect(this);

    destroyStream(reason);
    stop();
}

void PluginStream::destroyStream(NPReason reason)
{
    m_reason = reason;
    if (m_reason != NPRES_DONE) {
        // Stop any pending data from being streamed
        if (m_deliveryData)
            m_deliveryData->resize(0);
    } else if (m_deliveryData && m_deliveryData->size() > 0) {
        // There is more data to be streamed, don't destroy the stream now.
        return;
    }
    destroyStream();
}

void PluginStream::destroyStream()
{
    if (m_streamState == StreamStopped)
        return;

    ASSERT(m_reason != WebReasonNone);
    ASSERT(!m_deliveryData || m_deliveryData->size() == 0);

    closeFile(m_tempFileHandle);

    bool newStreamCalled = m_stream.ndata;

    // Protect from destruction if:
    //  NPN_DestroyStream is called from NPP_NewStream or
    //  PluginStreamClient::streamDidFinishLoading() removes the last reference
    RefPtr<PluginStream> protect(this);

    if (newStreamCalled) {
        if (m_reason == NPRES_DONE && (m_transferMode == NP_ASFILE || m_transferMode == NP_ASFILEONLY)) {
            ASSERT(!m_path.isNull());

            if (m_loader)
                m_loader->setDefersLoading(true);
            m_pluginFuncs->asfile(m_instance, &m_stream, m_path.utf8().data());
            if (m_loader)
                m_loader->setDefersLoading(false);
        }

        if (m_streamState != StreamBeforeStarted) {
            if (m_loader)
                m_loader->setDefersLoading(true);

            NPError npErr = m_pluginFuncs->destroystream(m_instance, &m_stream, m_reason);

            if (m_loader)
                m_loader->setDefersLoading(false);

            LOG_NPERROR(npErr);
        }

        m_stream.ndata = 0;
    }

    if (m_sendNotification) {
        // Flash 9 can dereference null if we call NPP_URLNotify without first calling NPP_NewStream
        // for requests made with NPN_PostURLNotify; see <rdar://5588807>
        if (m_loader)
            m_loader->setDefersLoading(true);
        if (!newStreamCalled && m_quirks.contains(PluginQuirkFlashURLNotifyBug) &&
            equalIgnoringCase(m_resourceRequest.httpMethod(), "POST")) {
            m_transferMode = NP_NORMAL;
            m_stream.url = "";
            m_stream.notifyData = m_notifyData;

            static char emptyMimeType[] = "";
            m_pluginFuncs->newstream(m_instance, emptyMimeType, &m_stream, false, &m_transferMode);
            m_pluginFuncs->destroystream(m_instance, &m_stream, m_reason);

            // in successful requests, the URL is dynamically allocated and freed in our
            // destructor, so reset it to 0
            m_stream.url = 0;
        }
        m_pluginFuncs->urlnotify(m_instance, m_resourceRequest.url().string().utf8().data(), m_reason, m_notifyData);
        if (m_loader)
            m_loader->setDefersLoading(false);
    }

    m_streamState = StreamStopped;

    if (!m_loadManually && m_client)
        m_client->streamDidFinishLoading(this);

    if (!m_path.isNull())
        deleteFile(m_path);
}

void PluginStream::delayDeliveryTimerFired(Timer<PluginStream>* timer)
{
    ASSERT(timer == &m_delayDeliveryTimer);

    deliverData();
}

void PluginStream::deliverData()
{
    ASSERT(m_deliveryData);
    
    if (m_streamState == StreamStopped)
        // FIXME: We should cancel our job in the SubresourceLoader on error so we don't reach this case
        return;

    ASSERT(m_streamState != StreamBeforeStarted);

    if (!m_stream.ndata || m_deliveryData->size() == 0)
        return;

    int32_t totalBytes = m_deliveryData->size();
    int32_t totalBytesDelivered = 0;

    if (m_loader)
        m_loader->setDefersLoading(true);
    while (totalBytesDelivered < totalBytes) {
        int32_t deliveryBytes = m_pluginFuncs->writeready(m_instance, &m_stream);

        if (deliveryBytes <= 0) {
            m_delayDeliveryTimer.startOneShot(0);
            break;
        } else {
            deliveryBytes = min(deliveryBytes, totalBytes - totalBytesDelivered);
            int32_t dataLength = deliveryBytes;
            char* data = m_deliveryData->data() + totalBytesDelivered;

            // Write the data
            deliveryBytes = m_pluginFuncs->write(m_instance, &m_stream, m_offset, dataLength, (void*)data);
            if (deliveryBytes < 0) {
                LOG_PLUGIN_NET_ERROR();
                if (m_loader)
                    m_loader->setDefersLoading(false);
                cancelAndDestroyStream(NPRES_NETWORK_ERR);
                return;
            }
            deliveryBytes = min(deliveryBytes, dataLength);
            m_offset += deliveryBytes;
            totalBytesDelivered += deliveryBytes;
        }
    }
    if (m_loader)
        m_loader->setDefersLoading(false);

    if (totalBytesDelivered > 0) {
        if (totalBytesDelivered < totalBytes) {
            int remainingBytes = totalBytes - totalBytesDelivered;
            memmove(m_deliveryData->data(), m_deliveryData->data() + totalBytesDelivered, remainingBytes);
            m_deliveryData->resize(remainingBytes);
        } else {
            m_deliveryData->resize(0);
            if (m_reason != WebReasonNone)
                destroyStream();
        }
    } 
}

void PluginStream::sendJavaScriptStream(const KURL& requestURL, const CString& resultString)
{
    didReceiveResponse(0, ResourceResponse(requestURL, "text/plain", resultString.length(), "", ""));

    if (m_streamState == StreamStopped)
        return;

    if (!resultString.isNull()) {
        didReceiveData(0, resultString.data(), resultString.length());
        if (m_streamState == StreamStopped)
            return;
    }

    m_loader = 0;

    destroyStream(resultString.isNull() ? NPRES_NETWORK_ERR : NPRES_DONE);
}

void PluginStream::didReceiveResponse(NetscapePlugInStreamLoader* loader, const ResourceResponse& response)
{
    ASSERT(loader == m_loader);
    ASSERT(m_streamState == StreamBeforeStarted);

    m_resourceResponse = response;

    startStream();
}

void PluginStream::didReceiveData(NetscapePlugInStreamLoader* loader, const char* data, int length)
{
    ASSERT(loader == m_loader);
    ASSERT(m_streamState == StreamStarted);

    // If the plug-in cancels the stream in deliverData it could be deleted, 
    // so protect it here.
    RefPtr<PluginStream> protect(this);

    if (m_transferMode != NP_ASFILEONLY) {
        if (!m_deliveryData)
            m_deliveryData = adoptPtr(new Vector<char>);

        int oldSize = m_deliveryData->size();
        m_deliveryData->resize(oldSize + length);
        memcpy(m_deliveryData->data() + oldSize, data, length);

        deliverData();
    }

    if (m_streamState != StreamStopped && isHandleValid(m_tempFileHandle)) {
        int bytesWritten = writeToFile(m_tempFileHandle, data, length);
        if (bytesWritten != length)
            cancelAndDestroyStream(NPRES_NETWORK_ERR);
    }
}

void PluginStream::didFail(NetscapePlugInStreamLoader* loader, const ResourceError&)
{
    ASSERT(loader == m_loader);

    LOG_PLUGIN_NET_ERROR();

    // destroyStream can result in our being deleted
    RefPtr<PluginStream> protect(this);

    destroyStream(NPRES_NETWORK_ERR);

    m_loader = 0;
}

void PluginStream::didFinishLoading(NetscapePlugInStreamLoader* loader)
{
    ASSERT(loader == m_loader);
    ASSERT(m_streamState == StreamStarted);

    // destroyStream can result in our being deleted
    RefPtr<PluginStream> protect(this);

    destroyStream(NPRES_DONE);

    m_loader = 0;
}

bool PluginStream::wantsAllStreams() const
{
    if (!m_pluginFuncs->getvalue)
        return false;

    void* result = 0;
    if (m_pluginFuncs->getvalue(m_instance, NPPVpluginWantsAllNetworkStreams, &result) != NPERR_NO_ERROR)
        return false;

    return result != 0;
}

}
