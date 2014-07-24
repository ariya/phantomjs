/*
 * Copyright (C) 2004, 2006, 2007, 2008, 2009, 2010, 2013 Apple Inc. All rights reserved.
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

#include "Logging.h"
#include "NetworkingContext.h"
#include "ResourceHandleClient.h"
#include "Timer.h"
#include <algorithm>
#include <wtf/MainThread.h>
#include <wtf/text/CString.h>

namespace WebCore {

static bool shouldForceContentSniffing;

typedef HashMap<AtomicString, ResourceHandle::BuiltinConstructor> BuiltinResourceHandleConstructorMap;
static BuiltinResourceHandleConstructorMap& builtinResourceHandleConstructorMap()
{
    ASSERT(isMainThread());
    DEFINE_STATIC_LOCAL(BuiltinResourceHandleConstructorMap, map, ());
    return map;
}

void ResourceHandle::registerBuiltinConstructor(const AtomicString& protocol, ResourceHandle::BuiltinConstructor constructor)
{
    builtinResourceHandleConstructorMap().add(protocol, constructor);
}

typedef HashMap<AtomicString, ResourceHandle::BuiltinSynchronousLoader> BuiltinResourceHandleSynchronousLoaderMap;
static BuiltinResourceHandleSynchronousLoaderMap& builtinResourceHandleSynchronousLoaderMap()
{
    ASSERT(isMainThread());
    DEFINE_STATIC_LOCAL(BuiltinResourceHandleSynchronousLoaderMap, map, ());
    return map;
}

void ResourceHandle::registerBuiltinSynchronousLoader(const AtomicString& protocol, ResourceHandle::BuiltinSynchronousLoader loader)
{
    builtinResourceHandleSynchronousLoaderMap().add(protocol, loader);
}

ResourceHandle::ResourceHandle(NetworkingContext* context, const ResourceRequest& request, ResourceHandleClient* client, bool defersLoading, bool shouldContentSniff)
    : d(adoptPtr(new ResourceHandleInternal(this, context, request, client, defersLoading, shouldContentSniff && shouldContentSniffURL(request.url()))))
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
    BuiltinResourceHandleConstructorMap::iterator protocolMapItem = builtinResourceHandleConstructorMap().find(request.url().protocol());

    if (protocolMapItem != builtinResourceHandleConstructorMap().end())
        return protocolMapItem->value(request, client);

    RefPtr<ResourceHandle> newHandle(adoptRef(new ResourceHandle(context, request, client, defersLoading, shouldContentSniff)));

    if (newHandle->d->m_scheduledFailureType != NoFailure)
        return newHandle.release();

    if (newHandle->start())
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

void ResourceHandle::loadResourceSynchronously(NetworkingContext* context, const ResourceRequest& request, StoredCredentials storedCredentials, ResourceError& error, ResourceResponse& response, Vector<char>& data)
{
    BuiltinResourceHandleSynchronousLoaderMap::iterator protocolMapItem = builtinResourceHandleSynchronousLoaderMap().find(request.url().protocol());

    if (protocolMapItem != builtinResourceHandleSynchronousLoaderMap().end()) {
        protocolMapItem->value(context, request, storedCredentials, error, response, data);
        return;
    }

    platformLoadResourceSynchronously(context, request, storedCredentials, error, response, data);
}

ResourceHandleClient* ResourceHandle::client() const
{
    return d->m_client;
}

void ResourceHandle::setClient(ResourceHandleClient* client)
{
    d->m_client = client;
}

#if !PLATFORM(MAC)
// ResourceHandle never uses async client calls on these platforms yet.
void ResourceHandle::continueWillSendRequest(const ResourceRequest&)
{
    ASSERT_NOT_REACHED();
}

void ResourceHandle::continueDidReceiveResponse()
{
    ASSERT_NOT_REACHED();
}

void ResourceHandle::continueShouldUseCredentialStorage(bool)
{
    ASSERT_NOT_REACHED();
}

#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
void ResourceHandle::continueCanAuthenticateAgainstProtectionSpace(bool)
{
    ASSERT_NOT_REACHED();
}
#endif
#endif

ResourceRequest& ResourceHandle::firstRequest()
{
    return d->m_firstRequest;
}

NetworkingContext* ResourceHandle::context() const
{
    return d->m_context.get();
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
    return !url.isLocalFile();
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

void ResourceHandle::didChangePriority(ResourceLoadPriority)
{
    // Optionally implemented by platform.
}

} // namespace WebCore
