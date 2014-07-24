/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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
#include "AuthenticationChallenge.h"

namespace WebCore {

AuthenticationChallengeBase::AuthenticationChallengeBase()
    : m_isNull(true)
    , m_previousFailureCount(0)
{
}

AuthenticationChallengeBase::AuthenticationChallengeBase(const ProtectionSpace& protectionSpace,
                                                         const Credential& proposedCredential,
                                                         unsigned previousFailureCount,
                                                         const ResourceResponse& response,
                                                         const ResourceError& error)
    : m_isNull(false)
    , m_protectionSpace(protectionSpace)
    , m_proposedCredential(proposedCredential)
    , m_previousFailureCount(previousFailureCount)
    , m_failureResponse(response)
    , m_error(error)
{
}

unsigned AuthenticationChallengeBase::previousFailureCount() const 
{ 
    return m_previousFailureCount; 
}

const Credential& AuthenticationChallengeBase::proposedCredential() const 
{ 
    return m_proposedCredential; 
}

const ProtectionSpace& AuthenticationChallengeBase::protectionSpace() const 
{ 
    return m_protectionSpace; 
}

const ResourceResponse& AuthenticationChallengeBase::failureResponse() const 
{ 
    return m_failureResponse; 
}

const ResourceError& AuthenticationChallengeBase::error() const 
{ 
    return m_error; 
}

bool AuthenticationChallengeBase::isNull() const
{
    return m_isNull;
}

void AuthenticationChallengeBase::nullify()
{
    m_isNull = true;
}

bool AuthenticationChallengeBase::compare(const AuthenticationChallenge& a, const AuthenticationChallenge& b)
{
    if (a.isNull() && b.isNull())
        return true;

    if (a.isNull() || b.isNull())
        return false;
        
    if (a.protectionSpace() != b.protectionSpace())
        return false;
        
    if (a.proposedCredential() != b.proposedCredential())
        return false;
        
    if (a.previousFailureCount() != b.previousFailureCount())
        return false;
        
    if (a.failureResponse() != b.failureResponse())
        return false;
        
    if (a.error() != b.error())
        return false;
        
    return AuthenticationChallenge::platformCompare(a, b);
}

}
