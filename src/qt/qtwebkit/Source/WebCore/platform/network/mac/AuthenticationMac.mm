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
#import "config.h"
#import "AuthenticationMac.h"

#import "AuthenticationCF.h"
#import "AuthenticationChallenge.h"
#import "AuthenticationClient.h"
#import "Credential.h"
#import "ProtectionSpace.h"

#import <Foundation/NSURLAuthenticationChallenge.h>
#import <Foundation/NSURLCredential.h>
#import <Foundation/NSURLProtectionSpace.h>

#if USE(CFNETWORK)

@interface NSURLProtectionSpace (Details)
- (CFURLProtectionSpaceRef) _cfurlprotectionspace;
- (id)_initWithCFURLProtectionSpace:(CFURLProtectionSpaceRef)cfProtSpace;
@end

@interface NSURLAuthenticationChallenge (Details)
+(NSURLAuthenticationChallenge *)_authenticationChallengeForCFAuthChallenge:(CFURLAuthChallengeRef)cfChallenge sender:(id <NSURLAuthenticationChallengeSender>)sender;
@end

@interface NSURLCredential (Details)
- (id) _initWithCFURLCredential:(CFURLCredentialRef)credential;
- (CFURLCredentialRef) _cfurlcredential;
@end

#endif

using namespace WebCore;

@interface WebCoreAuthenticationClientAsChallengeSender : NSObject <NSURLAuthenticationChallengeSender>
{
    AuthenticationClient* m_client;
#if USE(CFNETWORK)
    CFURLAuthChallengeRef m_cfChallenge;
#endif
}
- (id)initWithAuthenticationClient:(AuthenticationClient*)client;
- (AuthenticationClient*)client;
- (void)detachClient;
@end

@implementation WebCoreAuthenticationClientAsChallengeSender

- (id)initWithAuthenticationClient:(AuthenticationClient*)client
{
    self = [self init];
    if (!self)
        return nil;
    m_client = client;
    return self;
}

- (AuthenticationClient*)client
{
    return m_client;
}

- (void)detachClient
{
    m_client = 0;
}

- (void)useCredential:(NSURLCredential *)credential forAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge
{
    if (m_client)
        m_client->receivedCredential(core(challenge), core(credential));
}

- (void)continueWithoutCredentialForAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge
{
    if (m_client)
        m_client->receivedRequestToContinueWithoutCredential(core(challenge));
}

- (void)cancelAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge
{
    if (m_client)
        m_client->receivedCancellation(core(challenge));
}

#if USE(CFNETWORK)
- (void)setCFChallenge:(CFURLAuthChallengeRef)challenge
{
    m_cfChallenge = challenge;
}

- (CFURLAuthChallengeRef)cfChallenge
{
    return m_cfChallenge;
}
#endif

@end

namespace WebCore {

#if USE(CFNETWORK)

AuthenticationChallenge core(NSURLAuthenticationChallenge *macChallenge)
{
    WebCoreAuthenticationClientAsChallengeSender *challengeSender = (WebCoreAuthenticationClientAsChallengeSender*) [macChallenge sender];
    return AuthenticationChallenge([challengeSender cfChallenge], [challengeSender client]);
}

Credential core(NSURLCredential *macCredential)
{
    return core([macCredential _cfurlcredential]);
}

ProtectionSpace core(NSURLProtectionSpace *macSpace)
{
    return core([macSpace _cfurlprotectionspace]);
}

NSURLProtectionSpace *mac(const ProtectionSpace& coreSpace)
{
    RetainPtr<CFURLProtectionSpaceRef> protectionSpace = adoptCF(createCF(coreSpace));
    return [[[NSURLProtectionSpace alloc] _initWithCFURLProtectionSpace:protectionSpace.get()] autorelease];
}

NSURLAuthenticationChallenge *mac(const AuthenticationChallenge& coreChallenge)
{
    AuthenticationClient* authClient = coreChallenge.authenticationClient();
    RetainPtr<WebCoreAuthenticationClientAsChallengeSender> challengeSender = adoptNS([[WebCoreAuthenticationClientAsChallengeSender alloc] initWithAuthenticationClient:authClient]);
    RetainPtr<CFURLAuthChallengeRef> authChallenge = coreChallenge.cfURLAuthChallengeRef();
    if (!authChallenge)
        authChallenge = adoptCF(createCF(coreChallenge));
    [challengeSender.get() setCFChallenge:authChallenge.get()];
    return [[NSURLAuthenticationChallenge _authenticationChallengeForCFAuthChallenge:authChallenge.get() sender:challengeSender.get()] autorelease];
}

NSURLCredential *mac(const Credential& coreCredential)
{
    RetainPtr<CFURLCredentialRef> credential = adoptCF(createCF(coreCredential));
    return [[[NSURLCredential alloc] _initWithCFURLCredential:credential.get()] autorelease];
}

#else

AuthenticationChallenge::AuthenticationChallenge(const ProtectionSpace& protectionSpace,
                                                 const Credential& proposedCredential,
                                                 unsigned previousFailureCount,
                                                 const ResourceResponse& response,
                                                 const ResourceError& error)
    : AuthenticationChallengeBase(protectionSpace,
                                  proposedCredential,
                                  previousFailureCount,
                                  response,
                                  error)
{
}

AuthenticationChallenge::AuthenticationChallenge(NSURLAuthenticationChallenge *challenge)
    : AuthenticationChallengeBase(core([challenge protectionSpace]),
                                  core([challenge proposedCredential]),
                                  [challenge previousFailureCount],
                                  [challenge failureResponse],
                                  [challenge error])
    , m_sender([challenge sender])
    , m_nsChallenge(challenge)
{
}

void AuthenticationChallenge::setAuthenticationClient(AuthenticationClient* client)
{
    if (client) {
        m_sender = adoptNS([[WebCoreAuthenticationClientAsChallengeSender alloc] initWithAuthenticationClient:client]);
        if (m_nsChallenge)
            m_nsChallenge = adoptNS([[NSURLAuthenticationChallenge alloc] initWithAuthenticationChallenge:m_nsChallenge.get() sender:m_sender.get()]);
    } else {
        if ([m_sender.get() isMemberOfClass:[WebCoreAuthenticationClientAsChallengeSender class]])
            [(WebCoreAuthenticationClientAsChallengeSender *)m_sender.get() detachClient];
    }
}

AuthenticationClient* AuthenticationChallenge::authenticationClient() const
{
    if ([m_sender.get() isMemberOfClass:[WebCoreAuthenticationClientAsChallengeSender class]])
        return [static_cast<WebCoreAuthenticationClientAsChallengeSender*>(m_sender.get()) client];
    
    return 0;
}

bool AuthenticationChallenge::platformCompare(const AuthenticationChallenge& a, const AuthenticationChallenge& b)
{
    if (a.sender() != b.sender())
        return false;
        
    if (a.nsURLAuthenticationChallenge() != b.nsURLAuthenticationChallenge())
        return false;

    return true;
}

NSURLAuthenticationChallenge *mac(const AuthenticationChallenge& coreChallenge)
{
    if (coreChallenge.nsURLAuthenticationChallenge())
        return coreChallenge.nsURLAuthenticationChallenge();
        
    return [[[NSURLAuthenticationChallenge alloc] initWithProtectionSpace:mac(coreChallenge.protectionSpace())
                                                       proposedCredential:mac(coreChallenge.proposedCredential())
                                                     previousFailureCount:coreChallenge.previousFailureCount()
                                                          failureResponse:coreChallenge.failureResponse().nsURLResponse()
                                                                    error:coreChallenge.error()
                                                                   sender:coreChallenge.sender()] autorelease];
}

NSURLProtectionSpace *mac(const ProtectionSpace& coreSpace)
{
    NSString *proxyType = nil;
    NSString *protocol = nil;
    switch (coreSpace.serverType()) {
        case ProtectionSpaceServerHTTP:
            protocol = @"http";
            break;
        case ProtectionSpaceServerHTTPS:
            protocol = @"https";
            break;
        case ProtectionSpaceServerFTP:
            protocol = @"ftp";
            break;
        case ProtectionSpaceServerFTPS:
            protocol = @"ftps";
            break;
        case ProtectionSpaceProxyHTTP:
            proxyType = NSURLProtectionSpaceHTTPProxy;
            break;
        case ProtectionSpaceProxyHTTPS:
            proxyType = NSURLProtectionSpaceHTTPSProxy;
            break;
        case ProtectionSpaceProxyFTP:
            proxyType = NSURLProtectionSpaceFTPProxy;
            break;
        case ProtectionSpaceProxySOCKS:
            proxyType = NSURLProtectionSpaceSOCKSProxy;
            break;
        default:
            ASSERT_NOT_REACHED();
    }
  
    NSString *method = nil;
    switch (coreSpace.authenticationScheme()) {
        case ProtectionSpaceAuthenticationSchemeDefault:
            method = NSURLAuthenticationMethodDefault;
            break;
        case ProtectionSpaceAuthenticationSchemeHTTPBasic:
            method = NSURLAuthenticationMethodHTTPBasic;
            break;
        case ProtectionSpaceAuthenticationSchemeHTTPDigest:
            method = NSURLAuthenticationMethodHTTPDigest;
            break;
        case ProtectionSpaceAuthenticationSchemeHTMLForm:
            method = NSURLAuthenticationMethodHTMLForm;
            break;
        case ProtectionSpaceAuthenticationSchemeNTLM:
            method = NSURLAuthenticationMethodNTLM;
            break;
        case ProtectionSpaceAuthenticationSchemeNegotiate:
            method = NSURLAuthenticationMethodNegotiate;
            break;
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
        case ProtectionSpaceAuthenticationSchemeServerTrustEvaluationRequested:
            method = NSURLAuthenticationMethodServerTrust;
            break;
        case ProtectionSpaceAuthenticationSchemeClientCertificateRequested:
            method = NSURLAuthenticationMethodClientCertificate;
            break;
#endif
        default:
            ASSERT_NOT_REACHED();
    }
    
    if (proxyType)
        return [[[NSURLProtectionSpace alloc] initWithProxyHost:coreSpace.host()
                                                           port:coreSpace.port()
                                                           type:proxyType
                                                          realm:coreSpace.realm()
                                           authenticationMethod:method] autorelease];
    return [[[NSURLProtectionSpace alloc] initWithHost:coreSpace.host()
                                                  port:coreSpace.port()
                                              protocol:protocol
                                                 realm:coreSpace.realm()
                                  authenticationMethod:method] autorelease];
}

NSURLCredential *mac(const Credential& coreCredential)
{
    if (coreCredential.isEmpty())
        return nil;

    NSURLCredentialPersistence persistence = NSURLCredentialPersistenceNone;
    switch (coreCredential.persistence()) {
        case CredentialPersistenceNone:
            break;
        case CredentialPersistenceForSession:
            persistence = NSURLCredentialPersistenceForSession;
            break;
        case CredentialPersistencePermanent:
            persistence = NSURLCredentialPersistencePermanent;
            break;
        default:
            ASSERT_NOT_REACHED();
    }

#if CERTIFICATE_CREDENTIALS_SUPPORTED
    if (coreCredential.type() == CredentialTypeClientCertificate) {
        return [[[NSURLCredential alloc] initWithIdentity:coreCredential.identity()
                                             certificates:(NSArray *)coreCredential.certificates()
                                              persistence:persistence]
                                              autorelease];
    }
#endif

    return [[[NSURLCredential alloc] initWithUser:coreCredential.user()
                                        password:coreCredential.password()
                                     persistence:persistence]
                                     autorelease];
}

AuthenticationChallenge core(NSURLAuthenticationChallenge *macChallenge)
{
    return AuthenticationChallenge(macChallenge);
}

ProtectionSpace core(NSURLProtectionSpace *macSpace)
{
    ProtectionSpaceServerType serverType = ProtectionSpaceProxyHTTP;
    
    if ([macSpace isProxy]) {
        NSString *proxyType = [macSpace proxyType];
        if ([proxyType isEqualToString:NSURLProtectionSpaceHTTPProxy])
            serverType = ProtectionSpaceProxyHTTP;
        else if ([proxyType isEqualToString:NSURLProtectionSpaceHTTPSProxy])
            serverType = ProtectionSpaceProxyHTTPS;
        else if ([proxyType isEqualToString:NSURLProtectionSpaceFTPProxy])
            serverType = ProtectionSpaceProxyFTP;
        else if ([proxyType isEqualToString:NSURLProtectionSpaceSOCKSProxy])
            serverType = ProtectionSpaceProxySOCKS;
        else 
            ASSERT_NOT_REACHED();
    } else {
        NSString *protocol = [macSpace protocol];
        if ([protocol caseInsensitiveCompare:@"http"] == NSOrderedSame)
            serverType = ProtectionSpaceServerHTTP;
        else if ([protocol caseInsensitiveCompare:@"https"] == NSOrderedSame)
            serverType = ProtectionSpaceServerHTTPS;
        else if ([protocol caseInsensitiveCompare:@"ftp"] == NSOrderedSame)
            serverType = ProtectionSpaceServerFTP;
        else if ([protocol caseInsensitiveCompare:@"ftps"] == NSOrderedSame)
            serverType = ProtectionSpaceServerFTPS;
        else
            ASSERT_NOT_REACHED();
    }

    ProtectionSpaceAuthenticationScheme scheme = ProtectionSpaceAuthenticationSchemeDefault;
    NSString *method = [macSpace authenticationMethod];
    if ([method isEqualToString:NSURLAuthenticationMethodDefault])
        scheme = ProtectionSpaceAuthenticationSchemeDefault;
    else if ([method isEqualToString:NSURLAuthenticationMethodHTTPBasic])
        scheme = ProtectionSpaceAuthenticationSchemeHTTPBasic;
    else if ([method isEqualToString:NSURLAuthenticationMethodHTTPDigest])
        scheme = ProtectionSpaceAuthenticationSchemeHTTPDigest;
    else if ([method isEqualToString:NSURLAuthenticationMethodHTMLForm])
        scheme = ProtectionSpaceAuthenticationSchemeHTMLForm;
    else if ([method isEqualToString:NSURLAuthenticationMethodNTLM])
        scheme = ProtectionSpaceAuthenticationSchemeNTLM;
    else if ([method isEqualToString:NSURLAuthenticationMethodNegotiate])
        scheme = ProtectionSpaceAuthenticationSchemeNegotiate;
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    else if ([method isEqualToString:NSURLAuthenticationMethodClientCertificate])
        scheme = ProtectionSpaceAuthenticationSchemeClientCertificateRequested;
    else if ([method isEqualToString:NSURLAuthenticationMethodServerTrust])
        scheme = ProtectionSpaceAuthenticationSchemeServerTrustEvaluationRequested;
#endif
    else {
        scheme = ProtectionSpaceAuthenticationSchemeUnknown;
        ASSERT_NOT_REACHED();
    }
        
    return ProtectionSpace([macSpace host], [macSpace port], serverType, [macSpace realm], scheme);

}

Credential core(NSURLCredential *macCredential)
{
    CredentialPersistence persistence = CredentialPersistenceNone;
    switch ([macCredential persistence]) {
        case NSURLCredentialPersistenceNone:
            break;
        case NSURLCredentialPersistenceForSession:
            persistence = CredentialPersistenceForSession;
            break;
        case NSURLCredentialPersistencePermanent:
            persistence = CredentialPersistencePermanent;
            break;
        default:
            ASSERT_NOT_REACHED();
    }

#if CERTIFICATE_CREDENTIALS_SUPPORTED
    SecIdentityRef identity = [macCredential identity];
    if (identity)
        return Credential(identity, (CFArrayRef)[macCredential certificates], persistence);
#endif
    
    return Credential([macCredential user], [macCredential password], persistence);
}

#endif // USE(CFNETWORK)

} // namespace WebCore
