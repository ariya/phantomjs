/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#if USE(SECURITY_FRAMEWORK)

#include "PlatformCertificateInfo.h"
#include <Security/SecIdentity.h>
#include <WebCore/AuthenticationChallenge.h>

using namespace WebCore;

namespace WebKit {

bool AuthenticationManager::tryUsePlatformCertificateInfoForChallenge(const AuthenticationChallenge& challenge, const PlatformCertificateInfo& certificateInfo)
{
    CFArrayRef chain = certificateInfo.certificateChain();
    if (!chain)
        return false;
        
    ASSERT(CFArrayGetCount(chain));

    // The passed-in certificate chain includes the identity certificate at index 0, and additional certificates starting at index 1.
    SecIdentityRef identity;
    OSStatus result = SecIdentityCreateWithCertificate(NULL, (SecCertificateRef)CFArrayGetValueAtIndex(chain, 0), &identity);
    if (result != errSecSuccess) {
        LOG_ERROR("Unable to create SecIdentityRef with certificate - %i", result);
        [challenge.sender() cancelAuthenticationChallenge:challenge.nsURLAuthenticationChallenge()];
        return true;
    }

    CFIndex chainCount = CFArrayGetCount(chain);
    NSArray *nsChain = chainCount > 1 ? [(NSArray *)chain subarrayWithRange:NSMakeRange(1, chainCount - 1)] : nil;

    NSURLCredential *credential = [NSURLCredential credentialWithIdentity:identity
                                                             certificates:nsChain
                                                              persistence:NSURLCredentialPersistenceNone];

    [challenge.sender() useCredential:credential forAuthenticationChallenge:challenge.nsURLAuthenticationChallenge()];
    return true;
}

} // namespace WebKit

#endif // USE(SECURITY_FRAMEWORK)
