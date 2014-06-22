/*
 * Copyright (C) 2009, 2010, 2011, 2012, 2013 Research In Motion Limited. All rights reserved.
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
#include "NetworkManager.h"

#include "Chrome.h"
#include "CredentialStorage.h"
#include "Frame.h"
#include "FrameLoaderClientBlackBerry.h"
#include "NetworkJob.h"
#include "Page.h"
#include "ResourceHandleInternal.h"
#include "ResourceRequest.h"
#include "SecurityOrigin.h"

#include <BlackBerryPlatformLog.h>
#include <BuildInformation.h>
#include <network/FilterStream.h>
#include <network/NetworkRequest.h>

using BlackBerry::Platform::NetworkRequest;

namespace WebCore {

SINGLETON_INITIALIZER_THREADUNSAFE(NetworkManager)

int NetworkManager::startJob(int playerId, PassRefPtr<ResourceHandle> job, Frame* frame, bool defersLoading)
{
    ASSERT(job.get());
    // We shouldn't call methods on PassRefPtr so make a new RefPt.
    RefPtr<ResourceHandle> refJob(job);
    return startJob(playerId, refJob, refJob->firstRequest(), frame, defersLoading);
}

int NetworkManager::startJob(int playerId, PassRefPtr<ResourceHandle> job, const ResourceRequest& request, Frame* frame, bool defersLoading)
{
    Page* page = frame->page();
    ASSERT(page);
    BlackBerry::Platform::NetworkStreamFactory* streamFactory = page->chrome().platformPageClient()->networkStreamFactory();
    return startJob(playerId, page->groupName(), job, request, streamFactory, frame, defersLoading ? 1 : 0);
}

void protectionSpaceToPlatformAuth(const ProtectionSpace& protectionSpace, NetworkRequest::AuthType& authType, NetworkRequest::AuthProtocol& authProtocol, NetworkRequest::AuthScheme& authScheme)
{
    authScheme = NetworkRequest::AuthSchemeNone;
    switch (protectionSpace.authenticationScheme()) {
    case ProtectionSpaceAuthenticationSchemeDefault:
        authScheme = NetworkRequest::AuthSchemeDefault;
        break;
    case ProtectionSpaceAuthenticationSchemeHTTPBasic:
        authScheme = NetworkRequest::AuthSchemeHTTPBasic;
        break;
    case ProtectionSpaceAuthenticationSchemeHTTPDigest:
        authScheme = NetworkRequest::AuthSchemeHTTPDigest;
        break;
    case ProtectionSpaceAuthenticationSchemeNegotiate:
        authScheme = NetworkRequest::AuthSchemeNegotiate;
        break;
    case ProtectionSpaceAuthenticationSchemeNTLM:
        authScheme = NetworkRequest::AuthSchemeNTLM;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    authType = NetworkRequest::AuthTypeNone;
    authProtocol = NetworkRequest::AuthProtocolNone;
    switch (protectionSpace.serverType()) {
    case ProtectionSpaceServerHTTP:
        authType = NetworkRequest::AuthTypeHost;
        authProtocol = NetworkRequest::AuthProtocolHTTP;
        break;
    case ProtectionSpaceServerHTTPS:
        authType = NetworkRequest::AuthTypeHost;
        authProtocol = NetworkRequest::AuthProtocolHTTPS;
        break;
    case ProtectionSpaceServerFTP:
        authType = NetworkRequest::AuthTypeHost;
        authProtocol = NetworkRequest::AuthProtocolFTP;
        break;
    case ProtectionSpaceServerFTPS:
        authType = NetworkRequest::AuthTypeHost;
        authProtocol = NetworkRequest::AuthProtocolFTPS;
        break;
    case ProtectionSpaceProxyHTTP:
        authType = NetworkRequest::AuthTypeProxy;
        authProtocol = NetworkRequest::AuthProtocolHTTP;
        break;
    case ProtectionSpaceProxyHTTPS:
        authType = NetworkRequest::AuthTypeProxy;
        authProtocol = NetworkRequest::AuthProtocolHTTPS;
        break;
    case ProtectionSpaceProxyFTP:
        authType = NetworkRequest::AuthTypeProxy;
        authProtocol = NetworkRequest::AuthProtocolFTP;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }
}

static void setAuthCredentials(NetworkRequest& platformRequest, const AuthenticationChallenge& challenge)
{
    if (challenge.isNull())
        return;

    Credential credential = challenge.proposedCredential();
    const ProtectionSpace& protectionSpace = challenge.protectionSpace();

    String username = credential.user();
    String password = credential.password();

    NetworkRequest::AuthType authType;
    NetworkRequest::AuthProtocol authProtocol;
    NetworkRequest::AuthScheme authScheme;
    protectionSpaceToPlatformAuth(protectionSpace, authType, authProtocol, authScheme);

    if (authType != NetworkRequest::AuthTypeNone && authProtocol != NetworkRequest::AuthProtocolNone && authScheme != NetworkRequest::AuthSchemeNone)
        platformRequest.setCredentials(authType, authProtocol, authScheme, username.utf8().data(), password.utf8().data());
}

int NetworkManager::startJob(int playerId, const String& pageGroupName, PassRefPtr<ResourceHandle> job, const ResourceRequest& request, BlackBerry::Platform::NetworkStreamFactory* streamFactory, Frame* frame, int deferLoadingCount, int redirectCount, bool rereadCookies)
{
    // Make sure the ResourceHandle doesn't go out of scope while calling callbacks.
    RefPtr<ResourceHandle> guardJob(job);

    KURL url = request.url();

    // Only load the initial url once.
    bool isInitial = (url == m_initialURL);
    if (isInitial)
        m_initialURL = KURL();

    // Always reread cookies on a redirect
    if (redirectCount)
        rereadCookies = true;

    BlackBerry::Platform::NetworkRequest platformRequest;
    request.initializePlatformRequest(platformRequest, frame->loader() && frame->loader()->client() && static_cast<FrameLoaderClientBlackBerry*>(frame->loader()->client())->cookiesEnabled(), isInitial, rereadCookies);

    // GURL and KURL consider valid URLs differently, for example http:// is parsed as
    // http:/ by KURL and considered valid, while GURL considers it invalid.
    if (!platformRequest.url().is_valid())
        return BlackBerry::Platform::FilterStream::StatusErrorInvalidUrl;

    const String& documentUrl = frame->document()->url().string();
    if (!documentUrl.isEmpty())
        platformRequest.setReferrer(documentUrl);

    platformRequest.setSecurityOrigin(frame->document()->securityOrigin()->toRawString());

    // Attach any applicable auth credentials to the NetworkRequest.
    setAuthCredentials(platformRequest, guardJob->getInternal()->m_hostWebChallenge);
    setAuthCredentials(platformRequest, guardJob->getInternal()->m_proxyWebChallenge);

    if (!request.overrideContentType().isEmpty())
        platformRequest.setOverrideContentType(request.overrideContentType());

    NetworkJob* networkJob = new NetworkJob;
    networkJob->initialize(playerId, pageGroupName, url, platformRequest, guardJob, streamFactory, frame, deferLoadingCount, redirectCount);

    // Make sure we have only one NetworkJob for one ResourceHandle.
    ASSERT(!findJobForHandle(guardJob));

    m_jobs.append(networkJob);

    switch (networkJob->streamOpen()) {
    case BlackBerry::Platform::FilterStream::ResultOk:
        return BlackBerry::Platform::FilterStream::StatusSuccess;
    case BlackBerry::Platform::FilterStream::ResultNotReady:
        return BlackBerry::Platform::FilterStream::StatusErrorNotReady;
    case BlackBerry::Platform::FilterStream::ResultNotHandled:
    default:
        // This should never happen.
        break;
    }

    ASSERT_NOT_REACHED();
    return BlackBerry::Platform::FilterStream::StatusErrorConnectionFailed;
}

bool NetworkManager::stopJob(PassRefPtr<ResourceHandle> job)
{
    if (NetworkJob* networkJob = findJobForHandle(job))
        return !networkJob->cancelJob();
    return false;
}

NetworkJob* NetworkManager::findJobForHandle(PassRefPtr<ResourceHandle> job)
{
    for (unsigned i = 0; i < m_jobs.size(); ++i) {
        NetworkJob* networkJob = m_jobs[i];
        // We have only one job for one handle (not including cancelled jobs which may hang
        // around briefly), so return the first non-cancelled job.
        if (!networkJob->isCancelled() && networkJob->handle() == job)
            return networkJob;
    }
    return 0;
}

void NetworkManager::deleteJob(NetworkJob* job)
{
    ASSERT(!job->isRunning());
    size_t position = m_jobs.find(job);
    if (position != notFound)
        m_jobs.remove(position);
    delete job;
}

void NetworkManager::setDefersLoading(PassRefPtr<ResourceHandle> job, bool defersLoading)
{
    if (NetworkJob* networkJob = findJobForHandle(job))
        networkJob->updateDeferLoadingCount(defersLoading ? 1 : -1);
}

void NetworkManager::pauseLoad(PassRefPtr<ResourceHandle> job, bool pause)
{
    if (NetworkJob* networkJob = findJobForHandle(job))
        networkJob->streamPause(pause);
}

BlackBerry::Platform::FilterStream* NetworkManager::streamForHandle(PassRefPtr<ResourceHandle> job)
{
    NetworkJob* networkJob = findJobForHandle(job);
    return networkJob ? networkJob->wrappedStream() : 0;
}

} // namespace WebCore
