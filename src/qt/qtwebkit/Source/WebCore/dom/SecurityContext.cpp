/*
 * Copyright (C) 2011 Google Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE, INC. ``AS IS'' AND ANY
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
 *
 */

#include "config.h"
#include "SecurityContext.h"

#include "ContentSecurityPolicy.h"
#include "HTMLParserIdioms.h"
#include "SecurityOrigin.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

SecurityContext::SecurityContext()
    : m_mayDisplaySeamlesslyWithParent(false)
    , m_haveInitializedSecurityOrigin(false)
    , m_sandboxFlags(SandboxNone)
{
}

SecurityContext::~SecurityContext()
{
}

void SecurityContext::setSecurityOrigin(PassRefPtr<SecurityOrigin> securityOrigin)
{
    m_securityOrigin = securityOrigin;
    m_haveInitializedSecurityOrigin = true;
}

void SecurityContext::setContentSecurityPolicy(PassOwnPtr<ContentSecurityPolicy> contentSecurityPolicy)
{
    m_contentSecurityPolicy = contentSecurityPolicy;
}

bool SecurityContext::isSecureTransitionTo(const KURL& url) const
{
    // If we haven't initialized our security origin by now, this is probably
    // a new window created via the API (i.e., that lacks an origin and lacks
    // a place to inherit the origin from).
    if (!haveInitializedSecurityOrigin())
        return true;

    RefPtr<SecurityOrigin> other = SecurityOrigin::create(url);
    return securityOrigin()->canAccess(other.get());
}

void SecurityContext::enforceSandboxFlags(SandboxFlags mask)
{
    m_sandboxFlags |= mask;

    // The SandboxOrigin is stored redundantly in the security origin.
    if (isSandboxed(SandboxOrigin) && securityOrigin() && !securityOrigin()->isUnique())
        setSecurityOrigin(SecurityOrigin::createUnique());
}

SandboxFlags SecurityContext::parseSandboxPolicy(const String& policy, String& invalidTokensErrorMessage)
{
    // http://www.w3.org/TR/html5/the-iframe-element.html#attr-iframe-sandbox
    // Parse the unordered set of unique space-separated tokens.
    SandboxFlags flags = SandboxAll;
    unsigned length = policy.length();
    unsigned start = 0;
    unsigned numberOfTokenErrors = 0;
    StringBuilder tokenErrors;
    while (true) {
        while (start < length && isHTMLSpace(policy[start]))
            ++start;
        if (start >= length)
            break;
        unsigned end = start + 1;
        while (end < length && !isHTMLSpace(policy[end]))
            ++end;

        // Turn off the corresponding sandbox flag if it's set as "allowed".
        String sandboxToken = policy.substring(start, end - start);
        if (equalIgnoringCase(sandboxToken, "allow-same-origin"))
            flags &= ~SandboxOrigin;
        else if (equalIgnoringCase(sandboxToken, "allow-forms"))
            flags &= ~SandboxForms;
        else if (equalIgnoringCase(sandboxToken, "allow-scripts")) {
            flags &= ~SandboxScripts;
            flags &= ~SandboxAutomaticFeatures;
        } else if (equalIgnoringCase(sandboxToken, "allow-top-navigation"))
            flags &= ~SandboxTopNavigation;
        else if (equalIgnoringCase(sandboxToken, "allow-popups"))
            flags &= ~SandboxPopups;
        else if (equalIgnoringCase(sandboxToken, "allow-pointer-lock"))
            flags &= ~SandboxPointerLock;
        else {
            if (numberOfTokenErrors)
                tokenErrors.appendLiteral(", '");
            else
                tokenErrors.append('\'');
            tokenErrors.append(sandboxToken);
            tokenErrors.append('\'');
            numberOfTokenErrors++;
        }

        start = end + 1;
    }

    if (numberOfTokenErrors) {
        if (numberOfTokenErrors > 1)
            tokenErrors.appendLiteral(" are invalid sandbox flags.");
        else
            tokenErrors.appendLiteral(" is an invalid sandbox flag.");
        invalidTokensErrorMessage = tokenErrors.toString();
    }

    return flags;
}

}
