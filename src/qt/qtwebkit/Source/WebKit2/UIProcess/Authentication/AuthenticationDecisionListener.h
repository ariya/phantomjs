/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef AuthenticationDecisionListener_h
#define AuthenticationDecisionListener_h

#include "APIObject.h"

#include <wtf/RefPtr.h>

namespace WebKit {

class AuthenticationChallengeProxy;
class WebCredential;

class AuthenticationDecisionListener : public TypedAPIObject<APIObject::TypeAuthenticationDecisionListener> {
public:
    static PassRefPtr<AuthenticationDecisionListener> create(AuthenticationChallengeProxy* authenticationChallenge)
    {
        return adoptRef(new AuthenticationDecisionListener(authenticationChallenge));
    }
    
    void useCredential(WebCredential*);
    void cancel();
    
    void detachChallenge();

private:
    explicit AuthenticationDecisionListener(AuthenticationChallengeProxy*);

    AuthenticationChallengeProxy* m_challengeProxy;
};

} // namespace WebKit

#endif // WebAuthenticationDecisionListener_h
