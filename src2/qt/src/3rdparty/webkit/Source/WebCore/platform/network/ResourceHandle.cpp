/*
 * Copyright (C) 2004, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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
#include "ResourceHandleInternal.h"

#include "BlobRegistry.h"
#include "DNS.h"
#include "Logging.h"
#include "ResourceHandleClient.h"
#include "Timer.h"
#include <algorithm>
#include <wtf/text/CString.h>

namespace WebCore {

static bool shouldForceContentSniffing;

ResourceHandle::ResourceHandle(const ResourceRequest& request, ResourceHandleClient* client, bool defersLoading, bool shouldContentSniff)
    : d(adoptPtr(new ResourceHandleInternal(this, request, client, defersLoading, shouldContentSniff && shouldContentSniffURL(request.url()))))
{
    if (!request.url().isValid()) {
        scheduleFailure(InvalidURLFailure);
        return;
    }

    if (!portAllowed(request.url())) {
        scheduleFailure(BlockedFailure);
        return;
    }
}

PassRefPtr<ResourceHandle> ResourceHandle::create(NetworkingContext* context, const ResourceRequest& request, ResourceHandleClient* client, bool defersLoading, bool shouldContentSniff)
{
#if ENABLE(BLOB)
    if (request.url().protocolIs("blob")) {
        PassRefPtr<ResourceHandle> handle = blobRegistry().createResourceHandle(request, client);
        if (handle)
            return handle;
    }
#endif

    RefPtr<ResourceHandle> newHandle(adoptRef(new ResourceHandle(request, client, defersLoading, shouldContentSniff)));

    if (newHandle->d->m_scheduledFailureType != NoFailure)
        return newHandle.release();

    if (newHandle->start(context))
        return newHandle.release();

    return 0;
}

void ResourceHandle::scheduleFailure(FailureType type)
{
    d->m_scheduledFailureType = type;
    d->m_failureTimer.startOneShot(0);
}

void ResourceHandle::fireFailure(Timer<ResourceHandle>*)
{
    if (!client())
        return;

    switch (d->m_scheduledFailureType) {
        case NoFailure:
            ASSERT_NOT_REACHED();
            return;
        case BlockedFailure:
            d->m_scheduledFailureType = NoFailure;
            client()->wasBlocked(this);
            return;
        case InvalidURLFailure:
            d->m_scheduledFailureType = NoFailure;
            client()->cannotShowURL(this);
            return;
    }

    ASSERT_NOT_REACHED();
}

ResourceHandleClient* ResourceHandle::client() const
{
    return d->m_client;
}

void ResourceHandle::setClient(ResourceHandleClient* client)
{
    d->m_client = client;
}

ResourceRequest& ResourceHandle::firstRequest()
{
    return d->m_firstRequest;
}

const String& ResourceHandle::lastHTTPMethod() const
{
    return d->m_lastHTTPMethod;
}

bool ResourceHandle::hasAuthenticationChallenge() const
{
    return !d->m_currentWebChallenge.isNull();
}

void ResourceHandle::clearAuthentication()
{
#if PLATFORM(MAC)
    d->m_currentMacChallenge = nil;
#endif
    d->m_currentWebChallenge.nullify();
}
  
bool ResourceHandle::shouldContentSniff() const
{
    return d->m_shouldContentSniff;
}

bool ResourceHandle::shouldContentSniffURL(const KURL& url)
{
#if PLATFORM(MAC)
    if (shouldForceContentSniffing)
        return true;
#endif
    // We shouldn't content sniff file URLs as their MIME type should be established via their extension.
    return !url.protocolIs("file");
}

void ResourceHandle::forceContentSniffing()
{
    shouldForceContentSniffing = true;
}

void ResourceHandle::setDefersLoading(bool defers)
{
    LOG(Network, "Handle %p setDefersLoading(%s)", this, defers ? "true" : "false");

    ASSERT(d->m_defersLoading != defers); // Deferring is not counted, so calling setDefersLoading() repeatedly is likely to be in error.
    d->m_defersLoading = defers;

    if (defers) {
        ASSERT(d->m_failureTimer.isActive() == (d->m_scheduledFailureType != NoFailure));
        if (d->m_failureTimer.isActive())
            d->m_failureTimer.stop();
    } else if (d->m_scheduledFailureType != NoFailure) {
        ASSERT(!d->m_failureTimer.isActive());
        d->m_failureTimer.startOneShot(0);
    }

    platformSetDefersLoading(defers);
}

#if !USE(SOUP)
void ResourceHandle::prepareForURL(const KURL& url)
{
    return prefetchDNS(url.host());
}
#endif

void ResourceHandle::cacheMetadata(const ResourceResponse&, const Vector<char>&)
{
    // Optionally implemented by platform.
}

#if USE(CFURLSTORAGESESSIONS)

static RetainPtr<CFURLStorageSessionRef>& privateStorageSession()
{
    DEFINE_STATIC_LOCAL(RetainPtr<CFURLStorageSessionRef>, storageSession, ());
    return storageSession;
}

static String& privateBrowsingStorageSessionIdentifierBase()
{
    DEFINE_STATIC_LOCAL(String, base, ());
    return base;
}

void ResourceHandle::setPrivateBrowsingEnabled(bool enabled)
{
    if (!enabled) {
        privateStorageSession() = nullptr;
        return;
    }

    if (privateStorageSession())
        return;

    String base = privateBrowsingStorageSessionIdentifierBase().isNull() ? privateBrowsingStorageSessionIdentifierDefaultBase() : privateBrowsingStorageSessionIdentifierBase();
    RetainPtr<CFStringRef> cfIdentifier(AdoptCF, String::format("%s.PrivateBrowsing", base.utf8().data()).createCFString());

    privateStorageSession() = createPrivateBrowsingStorageSession(cfIdentifier.get());
}

CFURLStorageSessionRef ResourceHandle::privateBrowsingStorageSession()
{
    return privateStorageSession().get();
}

void ResourceHandle::setPrivateBrowsingStorageSessionIdentifierBase(const String& identifier)
{
    privateBrowsingStorageSessionIdentifierBase() = identifier;
}

#endif // USE(CFURLSTORAGESESSIONS)

} // namespace WebCore
