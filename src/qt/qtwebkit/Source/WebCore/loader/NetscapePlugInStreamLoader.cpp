/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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
#include "NetscapePlugInStreamLoader.h"

#include "DocumentLoader.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"

namespace WebCore {

NetscapePlugInStreamLoader::NetscapePlugInStreamLoader(Frame* frame, NetscapePlugInStreamLoaderClient* client)
    : ResourceLoader(frame, ResourceLoaderOptions(SendCallbacks, SniffContent, DoNotBufferData, AllowStoredCredentials, AskClientForAllCredentials, SkipSecurityCheck, UseDefaultOriginRestrictionsForType))
    , m_client(client)
{
}

NetscapePlugInStreamLoader::~NetscapePlugInStreamLoader()
{
}

PassRefPtr<NetscapePlugInStreamLoader> NetscapePlugInStreamLoader::create(Frame* frame, NetscapePlugInStreamLoaderClient* client, const ResourceRequest& request)
{
    RefPtr<NetscapePlugInStreamLoader> loader(adoptRef(new NetscapePlugInStreamLoader(frame, client)));
    loader->documentLoader()->addPlugInStreamLoader(loader.get());
    if (!loader->init(request))
        return 0;

    return loader.release();
}

bool NetscapePlugInStreamLoader::isDone() const
{
    return !m_client;
}

void NetscapePlugInStreamLoader::releaseResources()
{
    m_client = 0;
    ResourceLoader::releaseResources();
}

void NetscapePlugInStreamLoader::didReceiveResponse(const ResourceResponse& response)
{
    RefPtr<NetscapePlugInStreamLoader> protect(this);

    m_client->didReceiveResponse(this, response);

    // Don't continue if the stream is cancelled
    if (!m_client)
        return;

    ResourceLoader::didReceiveResponse(response);

    // Don't continue if the stream is cancelled
    if (!m_client)
        return;

    if (!response.isHTTP())
        return;
    
    if (m_client->wantsAllStreams())
        return;

    // Status code can be null when serving from a Web archive.
    if (response.httpStatusCode() && (response.httpStatusCode() < 100 || response.httpStatusCode() >= 400))
        cancel(frameLoader()->client()->fileDoesNotExistError(response));
}

void NetscapePlugInStreamLoader::didReceiveData(const char* data, int length, long long encodedDataLength, DataPayloadType dataPayloadType)
{
    didReceiveDataOrBuffer(data, length, 0, encodedDataLength, dataPayloadType);
}

void NetscapePlugInStreamLoader::didReceiveBuffer(PassRefPtr<SharedBuffer> buffer, long long encodedDataLength, DataPayloadType dataPayloadType)
{
    didReceiveDataOrBuffer(0, 0, buffer, encodedDataLength, dataPayloadType);
}

void NetscapePlugInStreamLoader::didReceiveDataOrBuffer(const char* data, int length, PassRefPtr<SharedBuffer> buffer, long long encodedDataLength, DataPayloadType dataPayloadType)
{
    RefPtr<NetscapePlugInStreamLoader> protect(this);
    
    m_client->didReceiveData(this, buffer ? buffer->data() : data, buffer ? buffer->size() : length);

    ResourceLoader::didReceiveDataOrBuffer(data, length, buffer, encodedDataLength, dataPayloadType);
}

void NetscapePlugInStreamLoader::didFinishLoading(double finishTime)
{
    RefPtr<NetscapePlugInStreamLoader> protect(this);

    m_documentLoader->removePlugInStreamLoader(this);
    m_client->didFinishLoading(this);
    ResourceLoader::didFinishLoading(finishTime);
}

void NetscapePlugInStreamLoader::didFail(const ResourceError& error)
{
    RefPtr<NetscapePlugInStreamLoader> protect(this);

    m_documentLoader->removePlugInStreamLoader(this);
    m_client->didFail(this, error);
    ResourceLoader::didFail(error);
}

void NetscapePlugInStreamLoader::willCancel(const ResourceError& error)
{
    m_client->didFail(this, error);
}

void NetscapePlugInStreamLoader::didCancel(const ResourceError&)
{
    // We need to remove the stream loader after the call to didFail, since didFail can 
    // spawn a new run loop and if the loader has been removed it won't be deferred when
    // the document loader is asked to defer loading.
    m_documentLoader->removePlugInStreamLoader(this);
}

}
