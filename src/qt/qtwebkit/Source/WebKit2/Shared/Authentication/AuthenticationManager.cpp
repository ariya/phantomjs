/*
 * Copyright (C) 2010, 2013 Apple Inc. All rights reserved.
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
#include "AuthenticationManager.h"

#include "AuthenticationManagerMessages.h"
#include "ChildProcess.h"
#include "Download.h"
#include "DownloadProxyMessages.h"
#include "WebCoreArgumentCoders.h"
#include "WebFrame.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include <WebCore/AuthenticationChallenge.h>
#include <WebCore/AuthenticationClient.h>

#if ENABLE(NETWORK_PROCESS)
#include "NetworkProcessProxyMessages.h"
#endif

using namespace WebCore;

namespace WebKit {

static uint64_t generateAuthenticationChallengeID()
{
    ASSERT(isMainThread());

    static int64_t uniqueAuthenticationChallengeID;
    return ++uniqueAuthenticationChallengeID;
}

const char* AuthenticationManager::supplementName()
{
    return "AuthenticationManager";
}

AuthenticationManager::AuthenticationManager(ChildProcess* process)
    : m_process(process)
{
    m_process->addMessageReceiver(Messages::AuthenticationManager::messageReceiverName(), this);
}

uint64_t AuthenticationManager::establishIdentifierForChallenge(const WebCore::AuthenticationChallenge& authenticationChallenge)
{
    ASSERT(isMainThread());

    uint64_t challengeID = generateAuthenticationChallengeID();
    m_challenges.set(challengeID, authenticationChallenge);
    return challengeID;
}

void AuthenticationManager::didReceiveAuthenticationChallenge(WebFrame* frame, const AuthenticationChallenge& authenticationChallenge)
{
    ASSERT(frame);
    ASSERT(frame->page());
    
    m_process->send(Messages::WebPageProxy::DidReceiveAuthenticationChallenge(frame->frameID(), authenticationChallenge, establishIdentifierForChallenge(authenticationChallenge)), frame->page()->pageID());
}

#if ENABLE(NETWORK_PROCESS)
void AuthenticationManager::didReceiveAuthenticationChallenge(uint64_t pageID, uint64_t frameID, const AuthenticationChallenge& authenticationChallenge)
{
    ASSERT(pageID);
    ASSERT(frameID);
    
    m_process->send(Messages::NetworkProcessProxy::DidReceiveAuthenticationChallenge(pageID, frameID, authenticationChallenge, establishIdentifierForChallenge(authenticationChallenge)));
}
#endif

void AuthenticationManager::didReceiveAuthenticationChallenge(Download* download, const AuthenticationChallenge& authenticationChallenge)
{
    download->send(Messages::DownloadProxy::DidReceiveAuthenticationChallenge(authenticationChallenge, establishIdentifierForChallenge(authenticationChallenge)));
}

// Currently, only Mac knows how to respond to authentication challenges with certificate info.
#if !USE(SECURITY_FRAMEWORK)
bool AuthenticationManager::tryUsePlatformCertificateInfoForChallenge(const WebCore::AuthenticationChallenge&, const PlatformCertificateInfo&)
{
    return false;
}
#endif

void AuthenticationManager::useCredentialForChallenge(uint64_t challengeID, const Credential& credential, const PlatformCertificateInfo& certificateInfo)
{
    ASSERT(isMainThread());

    AuthenticationChallenge challenge = m_challenges.take(challengeID);
    ASSERT(!challenge.isNull());
    
    if (tryUsePlatformCertificateInfoForChallenge(challenge, certificateInfo))
        return;
    
    AuthenticationClient* coreClient = challenge.authenticationClient();
    if (!coreClient) {
        // This authentication challenge comes from a download.
        Download::receivedCredential(challenge, credential);
        return;
    }

    coreClient->receivedCredential(challenge, credential);
}

void AuthenticationManager::continueWithoutCredentialForChallenge(uint64_t challengeID)
{
    ASSERT(isMainThread());

    AuthenticationChallenge challenge = m_challenges.take(challengeID);
    ASSERT(!challenge.isNull());
    AuthenticationClient* coreClient = challenge.authenticationClient();
    if (!coreClient) {
        // This authentication challenge comes from a download.
        Download::receivedRequestToContinueWithoutCredential(challenge);
        return;
    }

    coreClient->receivedRequestToContinueWithoutCredential(challenge);
}

void AuthenticationManager::cancelChallenge(uint64_t challengeID)
{
    ASSERT(isMainThread());

    AuthenticationChallenge challenge = m_challenges.take(challengeID);
    ASSERT(!challenge.isNull());
    AuthenticationClient* coreClient = challenge.authenticationClient();
    if (!coreClient) {
        // This authentication challenge comes from a download.
        Download::receivedCancellation(challenge);
        return;
    }

    coreClient->receivedCancellation(challenge);
}

} // namespace WebKit
