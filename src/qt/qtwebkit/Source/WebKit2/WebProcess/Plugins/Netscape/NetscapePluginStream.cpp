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
#include "NetscapePluginStream.h"

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "NetscapePlugin.h"
#include <utility>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>

using namespace WebCore;

namespace WebKit {

NetscapePluginStream::NetscapePluginStream(PassRefPtr<NetscapePlugin> plugin, uint64_t streamID, const String& requestURLString, bool sendNotification, void* notificationData)
    : m_plugin(plugin)
    , m_streamID(streamID)
    , m_requestURLString(requestURLString)
    , m_sendNotification(sendNotification)
    , m_notificationData(notificationData)
    , m_npStream()
    , m_transferMode(NP_NORMAL)
    , m_offset(0)
    , m_fileHandle(invalidPlatformFileHandle)
    , m_isStarted(false)
#if !ASSERT_DISABLED
    , m_urlNotifyHasBeenCalled(false)
#endif    
    , m_deliveryDataTimer(RunLoop::main(), this, &NetscapePluginStream::deliverDataToPlugin)
    , m_stopStreamWhenDoneDelivering(false)
{
}

NetscapePluginStream::~NetscapePluginStream()
{
    ASSERT(!m_isStarted);
    ASSERT(!m_sendNotification || m_urlNotifyHasBeenCalled);
    ASSERT(m_fileHandle == invalidPlatformFileHandle);
}

void NetscapePluginStream::didReceiveResponse(const KURL& responseURL, uint32_t streamLength, uint32_t lastModifiedTime, const String& mimeType, const String& headers)
{
    // Starting the stream could cause the plug-in stream to go away so we keep a reference to it here.
    RefPtr<NetscapePluginStream> protect(this);

    start(responseURL, streamLength, lastModifiedTime, mimeType, headers);
}

void NetscapePluginStream::didReceiveData(const char* bytes, int length)
{
    // Delivering the data could cause the plug-in stream to go away so we keep a reference to it here.
    RefPtr<NetscapePluginStream> protect(this);

    deliverData(bytes, length);
}

void NetscapePluginStream::didFinishLoading()
{
    // Stopping the stream could cause the plug-in stream to go away so we keep a reference to it here.
    RefPtr<NetscapePluginStream> protect(this);

    stop(NPRES_DONE);
}

void NetscapePluginStream::didFail(bool wasCancelled)
{
    // Stopping the stream could cause the plug-in stream to go away so we keep a reference to it here.
    RefPtr<NetscapePluginStream> protect(this);

    stop(wasCancelled ? NPRES_USER_BREAK : NPRES_NETWORK_ERR);
}
    
void NetscapePluginStream::sendJavaScriptStream(const String& result)
{
    // starting the stream or delivering the data to it might cause the plug-in stream to go away, so we keep
    // a reference to it here.
    RefPtr<NetscapePluginStream> protect(this);

    CString resultCString = result.utf8();
    if (resultCString.isNull()) {
        // There was an error evaluating the JavaScript, call NPP_URLNotify if needed and then destroy the stream.
        notifyAndDestroyStream(NPRES_NETWORK_ERR);
        return;
    }

    if (!start(m_requestURLString, resultCString.length(), 0, "text/plain", ""))
        return;

    deliverData(resultCString.data(), resultCString.length());
    stop(NPRES_DONE);
}

NPError NetscapePluginStream::destroy(NPReason reason)
{
    // It doesn't make sense to call NPN_DestroyStream on a stream that hasn't been started yet.
    if (!m_isStarted)
        return NPERR_GENERIC_ERROR;

    // It isn't really valid for a plug-in to call NPN_DestroyStream with NPRES_DONE.
    // (At least not for browser initiated streams, and we don't support plug-in initiated streams).
    if (reason == NPRES_DONE)
        return NPERR_INVALID_PARAM;

    cancel();
    stop(reason);
    return NPERR_NO_ERROR;
}

static bool isSupportedTransferMode(uint16_t transferMode)
{
    switch (transferMode) {
    case NP_ASFILEONLY:
    case NP_ASFILE:
    case NP_NORMAL:
        return true;
    // FIXME: We don't support seekable streams.
    case NP_SEEK:
        return false;
    }

    ASSERT_NOT_REACHED();
    return false;
}
    
bool NetscapePluginStream::start(const String& responseURLString, uint32_t streamLength, uint32_t lastModifiedTime, const String& mimeType, const String& headers)
{
    m_responseURL = responseURLString.utf8();
    m_mimeType = mimeType.utf8();
    m_headers = headers.utf8();

    m_npStream.ndata = this;
    m_npStream.url = m_responseURL.data();
    m_npStream.end = streamLength;
    m_npStream.lastmodified = lastModifiedTime;
    m_npStream.notifyData = m_notificationData;
    m_npStream.headers = m_headers.length() == 0 ? 0 : m_headers.data();

    NPError error = m_plugin->NPP_NewStream(const_cast<char*>(m_mimeType.data()), &m_npStream, false, &m_transferMode);
    if (error != NPERR_NO_ERROR) {
        // We failed to start the stream, cancel the load and destroy it.
        cancel();
        notifyAndDestroyStream(NPRES_NETWORK_ERR);
        return false;
    }

    // We successfully started the stream.
    m_isStarted = true;

    if (!isSupportedTransferMode(m_transferMode)) {
        // Cancel the load and stop the stream.
        cancel();
        stop(NPRES_NETWORK_ERR);
        return false;
    }

    return true;
}

void NetscapePluginStream::deliverData(const char* bytes, int length)
{
    ASSERT(m_isStarted);

    if (m_transferMode != NP_ASFILEONLY) {
        if (!m_deliveryData)
            m_deliveryData = adoptPtr(new Vector<uint8_t>);

        m_deliveryData->reserveCapacity(m_deliveryData->size() + length);
        m_deliveryData->append(bytes, length);
        
        deliverDataToPlugin();
    }

    if (m_transferMode == NP_ASFILE || m_transferMode == NP_ASFILEONLY)
        deliverDataToFile(bytes, length);
}

void NetscapePluginStream::deliverDataToPlugin()
{
    ASSERT(m_isStarted);

    int32_t numBytesToDeliver = m_deliveryData->size();
    int32_t numBytesDelivered = 0;

    while (numBytesDelivered < numBytesToDeliver) {
        int32_t numBytesPluginCanHandle = m_plugin->NPP_WriteReady(&m_npStream);
        
        // NPP_WriteReady could call NPN_DestroyStream and destroy the stream.
        if (!m_isStarted)
            return;

        if (numBytesPluginCanHandle <= 0) {
            // The plug-in can't handle more data, we'll send the rest later
            m_deliveryDataTimer.startOneShot(0);
            break;
        }

        // Figure out how much data to send to the plug-in.
        int32_t dataLength = std::min(numBytesPluginCanHandle, numBytesToDeliver - numBytesDelivered);
        uint8_t* data = m_deliveryData->data() + numBytesDelivered;

        int32_t numBytesWritten = m_plugin->NPP_Write(&m_npStream, m_offset, dataLength, data);
        if (numBytesWritten < 0) {
            cancel();
            stop(NPRES_NETWORK_ERR);
            return;
        }

        // NPP_Write could call NPN_DestroyStream and destroy the stream.
        if (!m_isStarted)
            return;

        numBytesWritten = std::min(numBytesWritten, dataLength);
        m_offset += numBytesWritten;
        numBytesDelivered += numBytesWritten;
    }

    // We didn't write anything.
    if (!numBytesDelivered)
        return;

    if (numBytesDelivered < numBytesToDeliver) {
        // Remove the bytes that we actually delivered.
        m_deliveryData->remove(0, numBytesDelivered);
    } else {
        m_deliveryData->clear();

        if (m_stopStreamWhenDoneDelivering)
            stop(NPRES_DONE);
    }
}

void NetscapePluginStream::deliverDataToFile(const char* bytes, int length)
{
    if (m_fileHandle == invalidPlatformFileHandle && m_filePath.isNull()) {
        // Create a temporary file.
        m_filePath = openTemporaryFile("WebKitPluginStream", m_fileHandle);

        // We failed to open the file, stop the stream.
        if (m_fileHandle == invalidPlatformFileHandle) {
            stop(NPRES_NETWORK_ERR);
            return;
        }
    }

    if (!length)
        return;

    int byteCount = writeToFile(m_fileHandle, bytes, length);
    if (byteCount != length) {
        // This happens only rarely, when we are out of disk space or have a disk I/O error.
        closeFile(m_fileHandle);

        stop(NPRES_NETWORK_ERR);
    }
}

void NetscapePluginStream::stop(NPReason reason)
{
    // The stream was stopped before it got a chance to start. This can happen if a stream is cancelled by
    // WebKit before it received a response.
    if (!m_isStarted) {
        ASSERT(reason != NPRES_DONE);
        notifyAndDestroyStream(reason);
        return;
    }

    if (reason == NPRES_DONE && m_deliveryData && !m_deliveryData->isEmpty()) {
        // There is still data left that the plug-in hasn't been able to consume yet.
        ASSERT(m_deliveryDataTimer.isActive());
        
        // Set m_stopStreamWhenDoneDelivering to true so that the next time the delivery timer fires
        // and calls deliverDataToPlugin the stream will be closed if all the remaining data was
        // successfully delivered.
        m_stopStreamWhenDoneDelivering = true;
        return;
    }

    m_deliveryData = nullptr;
    m_deliveryDataTimer.stop();

    if (m_transferMode == NP_ASFILE || m_transferMode == NP_ASFILEONLY) {
        if (reason == NPRES_DONE) {
            // Ensure that the file is created.
            deliverDataToFile(0, 0);
            if (m_fileHandle != invalidPlatformFileHandle)
                closeFile(m_fileHandle);
            
            ASSERT(!m_filePath.isNull());
            
            m_plugin->NPP_StreamAsFile(&m_npStream, m_filePath.utf8().data());
        } else {
            // Just close the file.
            if (m_fileHandle != invalidPlatformFileHandle)
                closeFile(m_fileHandle);
        }

        // Delete the file after calling NPP_StreamAsFile(), instead of in the destructor.  It should be OK
        // to delete the file here -- NPP_StreamAsFile() is always called immediately before NPP_DestroyStream()
        // (the stream destruction function), so there can be no expectation that a plugin will read the stream
        // file asynchronously after NPP_StreamAsFile() is called.
        deleteFile(m_filePath);
        m_filePath = String();

        // NPP_StreamAsFile could call NPN_DestroyStream and destroy the stream.
        if (!m_isStarted)
            return;
    }

    // Set m_isStarted to false before calling NPP_DestroyStream in case NPP_DestroyStream calls NPN_DestroyStream.
    m_isStarted = false;

    m_plugin->NPP_DestroyStream(&m_npStream, reason);

    notifyAndDestroyStream(reason);
}

void NetscapePluginStream::cancel()
{
    m_plugin->cancelStreamLoad(this);
}

void NetscapePluginStream::notifyAndDestroyStream(NPReason reason)
{
    ASSERT(!m_isStarted);
    ASSERT(!m_deliveryDataTimer.isActive());
    ASSERT(!m_urlNotifyHasBeenCalled);
    
    if (m_sendNotification) {
        m_plugin->NPP_URLNotify(m_requestURLString.utf8().data(), reason, m_notificationData);
    
#if !ASSERT_DISABLED
        m_urlNotifyHasBeenCalled = true;
#endif    
    }

    m_plugin->removePluginStream(this);
}

} // namespace WebKit

#endif // ENABLE(NETSCAPE_PLUGIN_API)
