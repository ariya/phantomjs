/*
 * Copyright (C) 2009, 2010 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "ResourceHandle.h"

#include "EventLoop.h"
#include "Frame.h"
#include "FrameLoaderClientBlackBerry.h"
#include "FrameNetworkingContextBlackBerry.h"
#include "NetworkManager.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PageGroupLoadDeferrer.h"
#include "ResourceError.h"
#include "ResourceHandleClient.h"
#include "ResourceHandleInternal.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "SharedBuffer.h"
#include "ThreadableLoader.h" // for StoredCredentials

#include <network/FilterStream.h>

namespace WebCore {

class WebCoreSynchronousLoader : public ResourceHandleClient {
public:
    WebCoreSynchronousLoader();

    virtual void didReceiveResponse(ResourceHandle*, const ResourceResponse&);
    virtual void didReceiveData(ResourceHandle*, const char*, int, int);
    virtual void didFinishLoading(ResourceHandle*, double);
    virtual void didFail(ResourceHandle*, const ResourceError&);

    ResourceResponse resourceResponse() const { return m_response; }
    ResourceError resourceError() const { return m_error; }
    Vector<char> data() const { return m_data; }
    bool isDone() const { return m_isDone; }

private:
    ResourceResponse m_response;
    ResourceError m_error;
    Vector<char> m_data;
    bool m_isDone;
};

WebCoreSynchronousLoader::WebCoreSynchronousLoader()
    : m_isDone(false)
{
}

void WebCoreSynchronousLoader::didReceiveResponse(ResourceHandle*, const ResourceResponse& response)
{
    m_response = response;
}

void WebCoreSynchronousLoader::didReceiveData(ResourceHandle*, const char* data, int length, int)
{
    m_data.append(data, length);
}

void WebCoreSynchronousLoader::didFinishLoading(ResourceHandle*, double)
{
    m_isDone = true;
}

void WebCoreSynchronousLoader::didFail(ResourceHandle*, const ResourceError& error)
{
    m_error = error;
    m_isDone = true;
}

ResourceHandleInternal::~ResourceHandleInternal()
{
    notImplemented();
}

ResourceHandle::~ResourceHandle()
{
    notImplemented();
}

bool ResourceHandle::loadsBlocked()
{
    notImplemented();
    return false;
}

void ResourceHandle::platformSetDefersLoading(bool defersLoading)
{
    NetworkManager::instance()->setDefersLoading(this, defersLoading);
}

bool ResourceHandle::start()
{
    if (!d->m_context || !d->m_context->isValid())
        return false;

    // FIXME: clean up use of Frame now that we have NetworkingContext (see RIM Bug #1515)
    Frame* frame = static_cast<FrameNetworkingContextBlackBerry*>(d->m_context.get())->frame();
    if (!frame || !frame->loader() || !frame->loader()->client() || !client())
        return false;
    int playerId = static_cast<FrameLoaderClientBlackBerry*>(frame->loader()->client())->playerId();
    if (NetworkManager::instance()->startJob(playerId, this, frame, d->m_defersLoading) != BlackBerry::Platform::FilterStream::StatusSuccess)
        scheduleFailure(InvalidURLFailure);
    return true;
}

void ResourceHandle::pauseLoad(bool pause)
{
    if (NetworkManager::instance())
        NetworkManager::instance()->pauseLoad(this, pause);
}

void ResourceHandle::cancel()
{
    NetworkManager::instance()->stopJob(this);
    setClient(0);
}

void ResourceHandle::platformLoadResourceSynchronously(NetworkingContext* context, const ResourceRequest& request, StoredCredentials, ResourceError& error, ResourceResponse& response, Vector<char>& data)
{
    if (!context || !context->isValid()) {
        ASSERT(false && "loadResourceSynchronously called with invalid networking context");
        return;
    }

    // FIXME: clean up use of Frame now that we have NetworkingContext (see RIM Bug #1515)
    Frame* frame = static_cast<FrameNetworkingContextBlackBerry*>(context)->frame();
    if (!frame || !frame->loader() || !frame->loader()->client() || !frame->page()) {
        ASSERT(false && "loadResourceSynchronously called without a frame or frame client");
        return;
    }

    PageGroupLoadDeferrer deferrer(frame->page(), true);
    TimerBase::fireTimersInNestedEventLoop();

    int playerId = static_cast<FrameLoaderClientBlackBerry*>(frame->loader()->client())->playerId();

    WebCoreSynchronousLoader syncLoader;

    bool defersLoading = false;
    bool shouldContentSniff = false;

    RefPtr<ResourceHandle> handle = adoptRef(new ResourceHandle(context, request, &syncLoader, defersLoading, shouldContentSniff));
    int status = NetworkManager::instance()->startJob(playerId, handle, frame, defersLoading);
    if (status != BlackBerry::Platform::FilterStream::StatusSuccess) {
        handle->cancel();
        error = ResourceError(ResourceError::platformErrorDomain, status, request.url().string(), BlackBerry::Platform::String::emptyString());
        return;
    }

    const double syncLoadTimeOut = 60; // seconds

    double startTime = currentTime();
    EventLoop loop;
    while (!syncLoader.isDone() && !loop.ended()) {
        loop.cycle();
        if (currentTime() - startTime > syncLoadTimeOut) {
            handle->cancel();
            error = ResourceError(ResourceError::platformErrorDomain, BlackBerry::Platform::FilterStream::StatusNetworkError, request.url().string(), "Time out");
            return;
        }
    }

    error = syncLoader.resourceError();
    data = syncLoader.data();
    response = syncLoader.resourceResponse();
}

} // namespace WebCore
