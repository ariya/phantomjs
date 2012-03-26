/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008 Holger Hans Peter Freyther
 *
 * All rights reserved.
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
#include "ResourceHandle.h"

#include "ChromeClientQt.h"
#include "CachedResourceLoader.h"
#include "Frame.h"
#include "FrameNetworkingContext.h"
#include "FrameLoaderClientQt.h"
#include "NotImplemented.h"
#include "Page.h"
#include "QNetworkReplyHandler.h"
#include "ResourceHandleClient.h"
#include "ResourceHandleInternal.h"
#include "SharedBuffer.h"

#include <QAbstractNetworkCache>
#include <QCoreApplication>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

namespace WebCore {

class WebCoreSynchronousLoader : public ResourceHandleClient {
public:
    WebCoreSynchronousLoader(ResourceError& error, ResourceResponse& response, Vector<char>& data)
        : m_error(error)
        , m_response(response)
        , m_data(data)
    {}

    virtual void didReceiveResponse(ResourceHandle*, const ResourceResponse& response) { m_response = response; }
    virtual void didReceiveData(ResourceHandle*, const char* data, int length, int) { m_data.append(data, length); }
    virtual void didFinishLoading(ResourceHandle*, double /*finishTime*/) {}
    virtual void didFail(ResourceHandle*, const ResourceError& error) { m_error = error; }
private:
    ResourceError& m_error;
    ResourceResponse& m_response;
    Vector<char>& m_data;
};

ResourceHandleInternal::~ResourceHandleInternal()
{
}

ResourceHandle::~ResourceHandle()
{
    if (d->m_job)
        cancel();
}

bool ResourceHandle::start(NetworkingContext* context)
{
    // If NetworkingContext is invalid then we are no longer attached to a Page,
    // this must be an attempted load from an unload event handler, so let's just block it.
    if (context && !context->isValid())
        return false;

    if (!d->m_user.isEmpty() || !d->m_pass.isEmpty()) {
        // If credentials were specified for this request, add them to the url,
        // so that they will be passed to QNetworkRequest.
        KURL urlWithCredentials(firstRequest().url());
        urlWithCredentials.setUser(d->m_user);
        urlWithCredentials.setPass(d->m_pass);
        d->m_firstRequest.setURL(urlWithCredentials);
    }

    getInternal()->m_context = context;
    ResourceHandleInternal *d = getInternal();
    d->m_job = new QNetworkReplyHandler(this, QNetworkReplyHandler::AsynchronousLoad, d->m_defersLoading);
    return true;
}

void ResourceHandle::cancel()
{
    if (d->m_job) {
        d->m_job->abort();
        d->m_job = 0;
    }
}

bool ResourceHandle::loadsBlocked()
{
    return false;
}

bool ResourceHandle::willLoadFromCache(ResourceRequest& request, Frame* frame)
{
    if (!frame)
        return false;

    QNetworkAccessManager* manager = 0;
    QAbstractNetworkCache* cache = 0;
    if (frame->loader()->networkingContext()) {
        manager = frame->loader()->networkingContext()->networkAccessManager();
        cache = manager->cache();
    }

    if (!cache)
        return false;

    QNetworkCacheMetaData data = cache->metaData(request.url());
    if (data.isValid()) {
        request.setCachePolicy(ReturnCacheDataDontLoad);
        return true;
    }

    return false;
}

bool ResourceHandle::supportsBufferedData()
{
    return false;
}

PassRefPtr<SharedBuffer> ResourceHandle::bufferedData()
{
    ASSERT_NOT_REACHED();
    return 0;
}

void ResourceHandle::loadResourceSynchronously(NetworkingContext* context, const ResourceRequest& request, StoredCredentials /*storedCredentials*/, ResourceError& error, ResourceResponse& response, Vector<char>& data)
{
    WebCoreSynchronousLoader syncLoader(error, response, data);
    RefPtr<ResourceHandle> handle = adoptRef(new ResourceHandle(request, &syncLoader, true, false));

    ResourceHandleInternal* d = handle->getInternal();
    if (!d->m_user.isEmpty() || !d->m_pass.isEmpty()) {
        // If credentials were specified for this request, add them to the url,
        // so that they will be passed to QNetworkRequest.
        KURL urlWithCredentials(d->m_firstRequest.url());
        urlWithCredentials.setUser(d->m_user);
        urlWithCredentials.setPass(d->m_pass);
        d->m_firstRequest.setURL(urlWithCredentials);
    }
    d->m_context = context;

    // starting in deferred mode gives d->m_job the chance of being set before sending the request.
    d->m_job = new QNetworkReplyHandler(handle.get(), QNetworkReplyHandler::SynchronousLoad, true);
    d->m_job->setLoadingDeferred(false);
}

void ResourceHandle::platformSetDefersLoading(bool defers)
{
    if (!d->m_job)
        return;
    d->m_job->setLoadingDeferred(defers);
}

} // namespace WebCore
