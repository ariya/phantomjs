/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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
#include "ewk_auth_request.h"

#include "WKAuthenticationChallenge.h"
#include "WKAuthenticationDecisionListener.h"
#include "WKCredential.h"
#include "WKProtectionSpace.h"
#include "WKString.h"
#include "ewk_auth_request_private.h"
#include <wtf/text/CString.h>

using namespace WebKit;

EwkAuthRequest::EwkAuthRequest(WKAuthenticationChallengeRef authenticationChallenge)
    : m_authenticationChallenge(authenticationChallenge)
    , m_wasHandled(false)
{
    ASSERT(m_authenticationChallenge);
}

EwkAuthRequest::~EwkAuthRequest()
{
    if (!m_wasHandled)
        continueWithoutCredential();
}

const char* EwkAuthRequest::suggestedUsername() const
{
    if (!m_suggestedUsername) {
        WKRetainPtr<WKCredentialRef> credential = WKAuthenticationChallengeGetProposedCredential(m_authenticationChallenge.get());
        ASSERT(credential);

        WKRetainPtr<WKStringRef> suggestedUsername(AdoptWK, WKCredentialCopyUser(credential.get()));
        if (!suggestedUsername || WKStringIsEmpty(suggestedUsername.get()))
            return 0;

        m_suggestedUsername = suggestedUsername.get();
    }

    return m_suggestedUsername;
}

const char* EwkAuthRequest::realm() const
{
    if (!m_realm) {
        WKRetainPtr<WKProtectionSpaceRef> protectionSpace = WKAuthenticationChallengeGetProtectionSpace(m_authenticationChallenge.get());
        ASSERT(protectionSpace);

        WKRetainPtr<WKStringRef> realm(AdoptWK, WKProtectionSpaceCopyRealm(protectionSpace.get()));
        if (!realm || WKStringIsEmpty(realm.get()))
            return 0;

        m_realm = realm.get();
    }

    return m_realm;
}

const char* EwkAuthRequest::host() const
{
    if (!m_host) {
        WKRetainPtr<WKProtectionSpaceRef> protectionSpace = WKAuthenticationChallengeGetProtectionSpace(m_authenticationChallenge.get());
        ASSERT(protectionSpace);

        WKRetainPtr<WKStringRef> host(AdoptWK, WKProtectionSpaceCopyHost(protectionSpace.get()));
        if (!host || WKStringIsEmpty(host.get()))
            return 0;

        m_host = host.get();
    }

    return m_host;
}

bool EwkAuthRequest::continueWithoutCredential()
{
    if (m_wasHandled)
        return false;

    m_wasHandled = true;
    WKAuthenticationDecisionListenerRef decisionListener = WKAuthenticationChallengeGetDecisionListener(m_authenticationChallenge.get());
    WKAuthenticationDecisionListenerUseCredential(decisionListener, 0);

    return true;
}

bool EwkAuthRequest::authenticate(const char* username, const char* password)
{
    if (m_wasHandled)
        return false;

    m_wasHandled = true;
    WKRetainPtr<WKStringRef> wkUsername(AdoptWK, WKStringCreateWithUTF8CString(username));
    WKRetainPtr<WKStringRef> wkPassword(AdoptWK, WKStringCreateWithUTF8CString(password));
    WKRetainPtr<WKCredentialRef> credential(AdoptWK, WKCredentialCreate(wkUsername.get(), wkPassword.get(), kWKCredentialPersistenceForSession));
    WKAuthenticationDecisionListenerRef decisionListener = WKAuthenticationChallengeGetDecisionListener(m_authenticationChallenge.get());
    WKAuthenticationDecisionListenerUseCredential(decisionListener, credential.get());

    return true;
}

bool EwkAuthRequest::isRetrying() const
{
    return WKAuthenticationChallengeGetPreviousFailureCount(m_authenticationChallenge.get()) > 0;
}

const char* ewk_auth_request_suggested_username_get(const Ewk_Auth_Request* request)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkAuthRequest, request, impl, 0);

    return impl->suggestedUsername();
}

Eina_Bool ewk_auth_request_cancel(Ewk_Auth_Request* request)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkAuthRequest, request, impl, false);

    return impl->continueWithoutCredential();
}

Eina_Bool ewk_auth_request_authenticate(Ewk_Auth_Request* request, const char* username, const char* password)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkAuthRequest, request, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(username, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(password, false);

    return impl->authenticate(username, password);
}

Eina_Bool ewk_auth_request_retrying_get(const Ewk_Auth_Request* request)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkAuthRequest, request, impl, false);

    return impl->isRetrying();
}

const char* ewk_auth_request_realm_get(const Ewk_Auth_Request* request)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkAuthRequest, request, impl, 0);

    return impl->realm();
}

const char* ewk_auth_request_host_get(const Ewk_Auth_Request* request)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkAuthRequest, request, impl, 0);

    return impl->host();
}
