/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2009, 2010, 2011 Apple Inc. All rights reserved.
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

#include "ResourceHandleInternal.h"

#include "AuthenticationCF.h"
#include "AuthenticationChallenge.h"
#include "CredentialStorage.h"
#include "CachedResourceLoader.h"
#include "FormDataStreamCFNet.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "LoaderRunLoopCF.h"
#include "Logging.h"
#include "MIMETypeRegistry.h"
#include "NetworkingContext.h"
#include "ResourceError.h"
#include "ResourceHandleClient.h"
#include "ResourceResponse.h"
#include "SharedBuffer.h"
#include "SynchronousLoaderClient.h"
#include <CFNetwork/CFNetwork.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <wtf/HashMap.h>
#include <wtf/Threading.h>
#include <wtf/text/Base64.h>
#include <wtf/text/CString.h>

#if PLATFORM(MAC)
#include "WebCoreSystemInterface.h"
#if USE(CFNETWORK)
#include "WebCoreURLResponse.h"
#include <CFNetwork/CFURLConnectionPriv.h>
#include <CFNetwork/CFURLRequestPriv.h>
#endif
#endif

#if PLATFORM(WIN)
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#include <process.h>

// FIXME: Remove this declaration once it's in WebKitSupportLibrary.
extern "C" {
__declspec(dllimport) CFURLConnectionRef CFURLConnectionCreateWithProperties(
  CFAllocatorRef           alloc,
  CFURLRequestRef          request,
  CFURLConnectionClient *  client,
  CFDictionaryRef properties);
}
#endif

namespace WebCore {

#if USE(CFNETWORK)

static HashSet<String>& allowsAnyHTTPSCertificateHosts()
{
    DEFINE_STATIC_LOCAL(HashSet<String>, hosts, ());
    return hosts;
}

static HashMap<String, RetainPtr<CFDataRef> >& clientCerts()
{
    typedef HashMap<String, RetainPtr<CFDataRef> > CertsMap;
    DEFINE_STATIC_LOCAL(CertsMap, certs, ());
    return certs;
}

#if !PLATFORM(MAC)
static void setDefaultMIMEType(CFURLResponseRef response)
{
    static CFStringRef defaultMIMETypeString = defaultMIMEType().createCFString().leakRef();
    
    CFURLResponseSetMIMEType(response, defaultMIMETypeString);
}
#endif

static void applyBasicAuthorizationHeader(ResourceRequest& request, const Credential& credential)
{
    String authenticationHeader = "Basic " + base64Encode(String(credential.user() + ":" + credential.password()).utf8());
    request.clearHTTPAuthorization(); // FIXME: Should addHTTPHeaderField be smart enough to not build comma-separated lists in headers like Authorization?
    request.addHTTPHeaderField("Authorization", authenticationHeader);
}

static CFURLRequestRef willSendRequest(CFURLConnectionRef conn, CFURLRequestRef cfRequest, CFURLResponseRef cfRedirectResponse, const void* clientInfo)
{
#if LOG_DISABLED
    UNUSED_PARAM(conn);
#endif
    ResourceHandle* handle = static_cast<ResourceHandle*>(const_cast<void*>(clientInfo));

    if (!cfRedirectResponse) {
        CFRetain(cfRequest);
        return cfRequest;
    }

    LOG(Network, "CFNet - willSendRequest(conn=%p, handle=%p) (%s)", conn, handle, handle->firstRequest().url().string().utf8().data());

    ResourceRequest request;
    if (cfRedirectResponse) {
        CFHTTPMessageRef httpMessage = CFURLResponseGetHTTPResponse(cfRedirectResponse);
        if (httpMessage && CFHTTPMessageGetResponseStatusCode(httpMessage) == 307) {
            RetainPtr<CFStringRef> lastHTTPMethod = handle->lastHTTPMethod().createCFString();
            RetainPtr<CFStringRef> newMethod = adoptCF(CFURLRequestCopyHTTPRequestMethod(cfRequest));
            if (CFStringCompareWithOptions(lastHTTPMethod.get(), newMethod.get(), CFRangeMake(0, CFStringGetLength(lastHTTPMethod.get())), kCFCompareCaseInsensitive)) {
                RetainPtr<CFMutableURLRequestRef> mutableRequest = adoptCF(CFURLRequestCreateMutableCopy(0, cfRequest));
                wkSetRequestStorageSession(handle->storageSession(), mutableRequest.get());
                CFURLRequestSetHTTPRequestMethod(mutableRequest.get(), lastHTTPMethod.get());

                FormData* body = handle->firstRequest().httpBody();
                if (!equalIgnoringCase(handle->firstRequest().httpMethod(), "GET") && body && !body->isEmpty())
                    WebCore::setHTTPBody(mutableRequest.get(), body);

                String originalContentType = handle->firstRequest().httpContentType();
                if (!originalContentType.isEmpty())
                    CFURLRequestSetHTTPHeaderFieldValue(mutableRequest.get(), CFSTR("Content-Type"), originalContentType.createCFString().get());

                request = mutableRequest.get();
            }
        }
    }
    if (request.isNull())
        request = cfRequest;

    // Should not set Referer after a redirect from a secure resource to non-secure one.
    if (!request.url().protocolIs("https") && protocolIs(request.httpReferrer(), "https") && handle->context()->shouldClearReferrerOnHTTPSToHTTPRedirect())
        request.clearHTTPReferrer();

    handle->willSendRequest(request, cfRedirectResponse);

    if (request.isNull())
        return 0;

    cfRequest = request.cfURLRequest(UpdateHTTPBody);

    CFRetain(cfRequest);
    return cfRequest;
}

static void didReceiveResponse(CFURLConnectionRef conn, CFURLResponseRef cfResponse, const void* clientInfo)
{
#if LOG_DISABLED
    UNUSED_PARAM(conn);
#endif
    ResourceHandle* handle = static_cast<ResourceHandle*>(const_cast<void*>(clientInfo));

    LOG(Network, "CFNet - didReceiveResponse(conn=%p, handle=%p) (%s)", conn, handle, handle->firstRequest().url().string().utf8().data());

    if (!handle->client())
        return;

#if PLATFORM(MAC)
    // Avoid MIME type sniffing if the response comes back as 304 Not Modified.
    CFHTTPMessageRef msg = wkGetCFURLResponseHTTPResponse(cfResponse);
    int statusCode = msg ? CFHTTPMessageGetResponseStatusCode(msg) : 0;

    if (statusCode != 304)
        adjustMIMETypeIfNecessary(cfResponse);

    if (_CFURLRequestCopyProtocolPropertyForKey(handle->firstRequest().cfURLRequest(DoNotUpdateHTTPBody), CFSTR("ForceHTMLMIMEType")))
        wkSetCFURLResponseMIMEType(cfResponse, CFSTR("text/html"));
#else
    if (!CFURLResponseGetMIMEType(cfResponse)) {
        // We should never be applying the default MIMEType if we told the networking layer to do content sniffing for handle.
        ASSERT(!handle->shouldContentSniff());
        setDefaultMIMEType(cfResponse);
    }
#endif
    
    handle->client()->didReceiveResponse(handle, cfResponse);
}

#if USE(NETWORK_CFDATA_ARRAY_CALLBACK)
static void didReceiveDataArray(CFURLConnectionRef conn, CFArrayRef dataArray, const void* clientInfo)
{
#if LOG_DISABLED
    UNUSED_PARAM(conn);
#endif
    ResourceHandle* handle = static_cast<ResourceHandle*>(const_cast<void*>(clientInfo));
    if (!handle->client())
        return;

    LOG(Network, "CFNet - didReceiveDataArray(conn=%p, handle=%p, arrayLength=%ld) (%s)", conn, handle, CFArrayGetCount(dataArray), handle->firstRequest().url().string().utf8().data());

    handle->handleDataArray(dataArray);
}
#endif

static void didReceiveData(CFURLConnectionRef conn, CFDataRef data, CFIndex originalLength, const void* clientInfo)
{
#if LOG_DISABLED
    UNUSED_PARAM(conn);
#endif
    ResourceHandle* handle = static_cast<ResourceHandle*>(const_cast<void*>(clientInfo));
    const UInt8* bytes = CFDataGetBytePtr(data);
    CFIndex length = CFDataGetLength(data);

    LOG(Network, "CFNet - didReceiveData(conn=%p, handle=%p, bytes=%ld) (%s)", conn, handle, length, handle->firstRequest().url().string().utf8().data());

    if (handle->client())
        handle->client()->didReceiveData(handle, (const char*)bytes, length, originalLength);
}

static void didSendBodyData(CFURLConnectionRef, CFIndex, CFIndex totalBytesWritten, CFIndex totalBytesExpectedToWrite, const void *clientInfo)
{
    ResourceHandle* handle = static_cast<ResourceHandle*>(const_cast<void*>(clientInfo));
    if (!handle || !handle->client())
        return;
    handle->client()->didSendData(handle, totalBytesWritten, totalBytesExpectedToWrite);
}

static Boolean shouldUseCredentialStorageCallback(CFURLConnectionRef conn, const void* clientInfo)
{
#if LOG_DISABLED
    UNUSED_PARAM(conn);
#endif
    ResourceHandle* handle = const_cast<ResourceHandle*>(static_cast<const ResourceHandle*>(clientInfo));

    LOG(Network, "CFNet - shouldUseCredentialStorage(conn=%p, handle=%p) (%s)", conn, handle, handle->firstRequest().url().string().utf8().data());

    if (!handle)
        return false;

    return handle->shouldUseCredentialStorage();
}

static void didFinishLoading(CFURLConnectionRef conn, const void* clientInfo)
{
#if LOG_DISABLED
    UNUSED_PARAM(conn);
#endif
    ResourceHandle* handle = static_cast<ResourceHandle*>(const_cast<void*>(clientInfo));

    LOG(Network, "CFNet - didFinishLoading(conn=%p, handle=%p) (%s)", conn, handle, handle->firstRequest().url().string().utf8().data());

    if (handle->client())
        handle->client()->didFinishLoading(handle, 0);
}

static void didFail(CFURLConnectionRef conn, CFErrorRef error, const void* clientInfo)
{
#if LOG_DISABLED
    UNUSED_PARAM(conn);
#endif
    ResourceHandle* handle = static_cast<ResourceHandle*>(const_cast<void*>(clientInfo));

    LOG(Network, "CFNet - didFail(conn=%p, handle=%p, error = %p) (%s)", conn, handle, error, handle->firstRequest().url().string().utf8().data());

    if (handle->client())
        handle->client()->didFail(handle, ResourceError(error));
}

static CFCachedURLResponseRef willCacheResponse(CFURLConnectionRef, CFCachedURLResponseRef cachedResponse, const void* clientInfo)
{
    ResourceHandle* handle = static_cast<ResourceHandle*>(const_cast<void*>(clientInfo));
    CFURLResponseRef wrappedResponse = CFCachedURLResponseGetWrappedResponse(cachedResponse);

    // Workaround for <rdar://problem/6300990> Caching does not respect Vary HTTP header.
    // FIXME: WebCore cache has issues with Vary, too (bug 58797, bug 71509).
    if (CFHTTPMessageRef httpResponse = CFURLResponseGetHTTPResponse(wrappedResponse)) {
        ASSERT(CFHTTPMessageIsHeaderComplete(httpResponse));
        RetainPtr<CFStringRef> varyValue = adoptCF(CFHTTPMessageCopyHeaderFieldValue(httpResponse, CFSTR("Vary")));
        if (varyValue)
            return 0;
    }

#if PLATFORM(WIN)
    if (handle->client() && !handle->client()->shouldCacheResponse(handle, cachedResponse))
        return 0;
#else
    CFCachedURLResponseRef newResponse = handle->client()->willCacheResponse(handle, cachedResponse);
    if (newResponse != cachedResponse)
        return newResponse;
#endif

    CFRetain(cachedResponse);

    return cachedResponse;
}

static void didReceiveChallenge(CFURLConnectionRef conn, CFURLAuthChallengeRef challenge, const void* clientInfo)
{
#if LOG_DISABLED
    UNUSED_PARAM(conn);
#endif
    ResourceHandle* handle = static_cast<ResourceHandle*>(const_cast<void*>(clientInfo));
    ASSERT(handle);
    LOG(Network, "CFNet - didReceiveChallenge(conn=%p, handle=%p (%s)", conn, handle, handle->firstRequest().url().string().utf8().data());

    handle->didReceiveAuthenticationChallenge(AuthenticationChallenge(challenge, handle));
}

#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
static Boolean canRespondToProtectionSpace(CFURLConnectionRef conn, CFURLProtectionSpaceRef protectionSpace, const void* clientInfo)
{
#if LOG_DISABLED
    UNUSED_PARAM(conn);
#endif
    ResourceHandle* handle = static_cast<ResourceHandle*>(const_cast<void*>(clientInfo));
    ASSERT(handle);

    LOG(Network, "CFNet - canRespondToProtectionSpace(conn=%p, handle=%p (%s)", conn, handle, handle->firstRequest().url().string().utf8().data());

    return handle->canAuthenticateAgainstProtectionSpace(core(protectionSpace));
}
#endif

ResourceHandleInternal::~ResourceHandleInternal()
{
    if (m_connection) {
        LOG(Network, "CFNet - Cancelling connection %p (%s)", m_connection.get(), m_firstRequest.url().string().utf8().data());
        CFURLConnectionCancel(m_connection.get());
    }
}

ResourceHandle::~ResourceHandle()
{
    LOG(Network, "CFNet - Destroying job %p (%s)", this, d->m_firstRequest.url().string().utf8().data());
}

void ResourceHandle::createCFURLConnection(bool shouldUseCredentialStorage, bool shouldContentSniff)
{
    if ((!d->m_user.isEmpty() || !d->m_pass.isEmpty()) && !firstRequest().url().protocolIsInHTTPFamily()) {
        // Credentials for ftp can only be passed in URL, the didReceiveAuthenticationChallenge delegate call won't be made.
        KURL urlWithCredentials(firstRequest().url());
        urlWithCredentials.setUser(d->m_user);
        urlWithCredentials.setPass(d->m_pass);
        firstRequest().setURL(urlWithCredentials);
    }

    // <rdar://problem/7174050> - For URLs that match the paths of those previously challenged for HTTP Basic authentication,
    // try and reuse the credential preemptively, as allowed by RFC 2617.
    if (shouldUseCredentialStorage && firstRequest().url().protocolIsInHTTPFamily()) {
        if (d->m_user.isEmpty() && d->m_pass.isEmpty()) {
            // <rdar://problem/7174050> - For URLs that match the paths of those previously challenged for HTTP Basic authentication, 
            // try and reuse the credential preemptively, as allowed by RFC 2617.
            d->m_initialCredential = CredentialStorage::get(firstRequest().url());
        } else {
            // If there is already a protection space known for the URL, update stored credentials before sending a request.
            // This makes it possible to implement logout by sending an XMLHttpRequest with known incorrect credentials, and aborting it immediately
            // (so that an authentication dialog doesn't pop up).
            CredentialStorage::set(Credential(d->m_user, d->m_pass, CredentialPersistenceNone), firstRequest().url());
        }
    }
        
    if (!d->m_initialCredential.isEmpty()) {
        // FIXME: Support Digest authentication, and Proxy-Authorization.
        applyBasicAuthorizationHeader(firstRequest(), d->m_initialCredential);
    }

    RetainPtr<CFMutableURLRequestRef> request = adoptCF(CFURLRequestCreateMutableCopy(kCFAllocatorDefault, firstRequest().cfURLRequest(UpdateHTTPBody)));
    wkSetRequestStorageSession(d->m_storageSession.get(), request.get());
    
    if (!shouldContentSniff)
        wkSetCFURLRequestShouldContentSniff(request.get(), false);

    RetainPtr<CFMutableDictionaryRef> sslProps;

    if (allowsAnyHTTPSCertificateHosts().contains(firstRequest().url().host().lower())) {
        sslProps = adoptCF(CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
        CFDictionaryAddValue(sslProps.get(), kCFStreamSSLAllowsAnyRoot, kCFBooleanTrue);
        CFDictionaryAddValue(sslProps.get(), kCFStreamSSLAllowsExpiredRoots, kCFBooleanTrue);
        CFDictionaryAddValue(sslProps.get(), kCFStreamSSLAllowsExpiredCertificates, kCFBooleanTrue);
        CFDictionaryAddValue(sslProps.get(), kCFStreamSSLValidatesCertificateChain, kCFBooleanFalse);
    }

    HashMap<String, RetainPtr<CFDataRef> >::iterator clientCert = clientCerts().find(firstRequest().url().host().lower());
    if (clientCert != clientCerts().end()) {
        if (!sslProps)
            sslProps = adoptCF(CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
#if PLATFORM(WIN)
        wkSetClientCertificateInSSLProperties(sslProps.get(), (clientCert->value).get());
#endif
    }

    if (sslProps)
        CFURLRequestSetSSLProperties(request.get(), sslProps.get());

#if PLATFORM(WIN)
    if (CFHTTPCookieStorageRef cookieStorage = overridenCookieStorage()) {
        // Overridden cookie storage doesn't come from a session, so the request does not have it yet.
        CFURLRequestSetHTTPCookieStorage(request.get(), cookieStorage);
    }
#endif

    CFURLConnectionClient_V6 client = { 6, this, 0, 0, 0, WebCore::willSendRequest, didReceiveResponse, didReceiveData, 0, didFinishLoading, didFail, willCacheResponse, didReceiveChallenge, didSendBodyData, shouldUseCredentialStorageCallback, 0,
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
        canRespondToProtectionSpace,
#else
        0,
#endif
        0,
#if USE(NETWORK_CFDATA_ARRAY_CALLBACK)
        didReceiveDataArray
#else 
        0
#endif
    };

    CFMutableDictionaryRef streamProperties  = CFDictionaryCreateMutable(0, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    if (!shouldUseCredentialStorage)
        CFDictionarySetValue(streamProperties, CFSTR("_kCFURLConnectionSessionID"), CFSTR("WebKitPrivateSession"));

#if PLATFORM(IOS) || (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090)
    RetainPtr<CFDataRef> sourceApplicationAuditData = d->m_context->sourceApplicationAuditData();
    if (sourceApplicationAuditData)
        CFDictionarySetValue(streamProperties, CFSTR("kCFStreamPropertySourceApplication"), sourceApplicationAuditData.get());
#endif

    static const CFStringRef kCFURLConnectionSocketStreamProperties = CFSTR("kCFURLConnectionSocketStreamProperties");
    RetainPtr<CFDictionaryRef> propertiesDictionary = adoptCF(CFDictionaryCreate(0, (const void**)&kCFURLConnectionSocketStreamProperties, (const void**)&streamProperties, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
    CFRelease(streamProperties);

    d->m_connection = adoptCF(CFURLConnectionCreateWithProperties(0, request.get(), reinterpret_cast<CFURLConnectionClient*>(&client), propertiesDictionary.get()));
}

bool ResourceHandle::start()
{
    if (!d->m_context)
        return false;

    // If NetworkingContext is invalid then we are no longer attached to a Page,
    // this must be an attempted load from an unload handler, so let's just block it.
    if (!d->m_context->isValid())
        return false;

    d->m_storageSession = d->m_context->storageSession().platformSession();

    bool shouldUseCredentialStorage = !client() || client()->shouldUseCredentialStorage(this);

    createCFURLConnection(shouldUseCredentialStorage, d->m_shouldContentSniff);

#if PLATFORM(WIN)
    CFURLConnectionScheduleWithCurrentMessageQueue(d->m_connection.get());
#else
    CFURLConnectionScheduleWithRunLoop(d->m_connection.get(), CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
#endif
    CFURLConnectionScheduleDownloadWithRunLoop(d->m_connection.get(), loaderRunLoop(), kCFRunLoopDefaultMode);
    CFURLConnectionStart(d->m_connection.get());

    LOG(Network, "CFNet - Starting URL %s (handle=%p, conn=%p)", firstRequest().url().string().utf8().data(), this, d->m_connection.get());

    return true;
}

void ResourceHandle::cancel()
{
    if (d->m_connection) {
        CFURLConnectionCancel(d->m_connection.get());
        d->m_connection = 0;
    }
}

void ResourceHandle::willSendRequest(ResourceRequest& request, const ResourceResponse& redirectResponse)
{
    const KURL& url = request.url();
    d->m_user = url.user();
    d->m_pass = url.pass();
    d->m_lastHTTPMethod = request.httpMethod();
    request.removeCredentials();

    if (!protocolHostAndPortAreEqual(request.url(), redirectResponse.url())) {
        // If the network layer carries over authentication headers from the original request
        // in a cross-origin redirect, we want to clear those headers here.
        request.clearHTTPAuthorization();
    } else {
        // Only consider applying authentication credentials if this is actually a redirect and the redirect
        // URL didn't include credentials of its own.
        if (d->m_user.isEmpty() && d->m_pass.isEmpty() && !redirectResponse.isNull()) {
            Credential credential = CredentialStorage::get(request.url());
            if (!credential.isEmpty()) {
                d->m_initialCredential = credential;
                
                // FIXME: Support Digest authentication, and Proxy-Authorization.
                applyBasicAuthorizationHeader(request, d->m_initialCredential);
            }
        }
    }

    RefPtr<ResourceHandle> protect(this);
    client()->willSendRequest(this, request, redirectResponse);

    // Client call may not preserve the session, especially if the request is sent over IPC.
    if (!request.isNull())
        request.setStorageSession(d->m_storageSession.get());
}

bool ResourceHandle::shouldUseCredentialStorage()
{
    LOG(Network, "CFNet - shouldUseCredentialStorage()");
    if (client())
        return client()->shouldUseCredentialStorage(this);

    return false;
}

void ResourceHandle::didReceiveAuthenticationChallenge(const AuthenticationChallenge& challenge)
{
    LOG(Network, "CFNet - didReceiveAuthenticationChallenge()");
    ASSERT(d->m_currentWebChallenge.isNull());
    // Since CFURLConnection networking relies on keeping a reference to the original CFURLAuthChallengeRef,
    // we make sure that is actually present
    ASSERT(challenge.cfURLAuthChallengeRef());
    ASSERT(challenge.authenticationClient() == this); // Should be already set.

#if !PLATFORM(WIN)
    // Proxy authentication is handled by CFNetwork internally. We can get here if the user cancels
    // CFNetwork authentication dialog, and we shouldn't ask the client to display another one in that case.
    if (challenge.protectionSpace().isProxy()) {
        // Cannot use receivedRequestToContinueWithoutCredential(), because current challenge is not yet set.
        CFURLConnectionUseCredential(d->m_connection.get(), 0, challenge.cfURLAuthChallengeRef());
        return;
    }
#endif

    if (!d->m_user.isNull() && !d->m_pass.isNull()) {
        RetainPtr<CFURLCredentialRef> credential = adoptCF(CFURLCredentialCreate(kCFAllocatorDefault, d->m_user.createCFString().get(), d->m_pass.createCFString().get(), 0, kCFURLCredentialPersistenceNone));
        
        KURL urlToStore;
        if (challenge.failureResponse().httpStatusCode() == 401)
            urlToStore = challenge.failureResponse().url();
        CredentialStorage::set(core(credential.get()), challenge.protectionSpace(), urlToStore);
        
        CFURLConnectionUseCredential(d->m_connection.get(), credential.get(), challenge.cfURLAuthChallengeRef());
        d->m_user = String();
        d->m_pass = String();
        // FIXME: Per the specification, the user shouldn't be asked for credentials if there were incorrect ones provided explicitly.
        return;
    }

    if (!client() || client()->shouldUseCredentialStorage(this)) {
        if (!d->m_initialCredential.isEmpty() || challenge.previousFailureCount()) {
            // The stored credential wasn't accepted, stop using it.
            // There is a race condition here, since a different credential might have already been stored by another ResourceHandle,
            // but the observable effect should be very minor, if any.
            CredentialStorage::remove(challenge.protectionSpace());
        }

        if (!challenge.previousFailureCount()) {
            Credential credential = CredentialStorage::get(challenge.protectionSpace());
            if (!credential.isEmpty() && credential != d->m_initialCredential) {
                ASSERT(credential.persistence() == CredentialPersistenceNone);
                if (challenge.failureResponse().httpStatusCode() == 401) {
                    // Store the credential back, possibly adding it as a default for this directory.
                    CredentialStorage::set(credential, challenge.protectionSpace(), challenge.failureResponse().url());
                }
                RetainPtr<CFURLCredentialRef> cfCredential = adoptCF(createCF(credential));
                CFURLConnectionUseCredential(d->m_connection.get(), cfCredential.get(), challenge.cfURLAuthChallengeRef());
                return;
            }
        }
    }

    d->m_currentWebChallenge = challenge;
    
    if (client())
        client()->didReceiveAuthenticationChallenge(this, d->m_currentWebChallenge);
}

#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
bool ResourceHandle::canAuthenticateAgainstProtectionSpace(const ProtectionSpace& protectionSpace)
{
    if (client())
        return client()->canAuthenticateAgainstProtectionSpace(this, protectionSpace);

    return false;
}
#endif

void ResourceHandle::receivedCredential(const AuthenticationChallenge& challenge, const Credential& credential)
{
    LOG(Network, "CFNet - receivedCredential()");
    ASSERT(!challenge.isNull());
    ASSERT(challenge.cfURLAuthChallengeRef());
    if (challenge != d->m_currentWebChallenge)
        return;

    // FIXME: Support empty credentials. Currently, an empty credential cannot be stored in WebCore credential storage, as that's empty value for its map.
    if (credential.isEmpty()) {
        receivedRequestToContinueWithoutCredential(challenge);
        return;
    }

    if (credential.persistence() == CredentialPersistenceForSession) {
        // Manage per-session credentials internally, because once NSURLCredentialPersistencePerSession is used, there is no way
        // to ignore it for a particular request (short of removing it altogether).
        Credential webCredential(credential.user(), credential.password(), CredentialPersistenceNone);
        RetainPtr<CFURLCredentialRef> cfCredential = adoptCF(createCF(webCredential));
        
        KURL urlToStore;
        if (challenge.failureResponse().httpStatusCode() == 401)
            urlToStore = challenge.failureResponse().url();      
        CredentialStorage::set(webCredential, challenge.protectionSpace(), urlToStore);

        CFURLConnectionUseCredential(d->m_connection.get(), cfCredential.get(), challenge.cfURLAuthChallengeRef());
    } else {
        RetainPtr<CFURLCredentialRef> cfCredential = adoptCF(createCF(credential));
        CFURLConnectionUseCredential(d->m_connection.get(), cfCredential.get(), challenge.cfURLAuthChallengeRef());
    }

    clearAuthentication();
}

void ResourceHandle::receivedRequestToContinueWithoutCredential(const AuthenticationChallenge& challenge)
{
    LOG(Network, "CFNet - receivedRequestToContinueWithoutCredential()");
    ASSERT(!challenge.isNull());
    ASSERT(challenge.cfURLAuthChallengeRef());
    if (challenge != d->m_currentWebChallenge)
        return;

    CFURLConnectionUseCredential(d->m_connection.get(), 0, challenge.cfURLAuthChallengeRef());

    clearAuthentication();
}

void ResourceHandle::receivedCancellation(const AuthenticationChallenge& challenge)
{
    LOG(Network, "CFNet - receivedCancellation()");
    if (challenge != d->m_currentWebChallenge)
        return;

    if (client())
        client()->receivedCancellation(this, challenge);
}

CFURLStorageSessionRef ResourceHandle::storageSession() const
{
    return d->m_storageSession.get();
}

CFURLConnectionRef ResourceHandle::connection() const
{
    return d->m_connection.get();
}

CFURLConnectionRef ResourceHandle::releaseConnectionForDownload()
{
    LOG(Network, "CFNet - Job %p releasing connection %p for download", this, d->m_connection.get());
    return d->m_connection.leakRef();
}

CFStringRef ResourceHandle::synchronousLoadRunLoopMode()
{
    return CFSTR("WebCoreSynchronousLoaderRunLoopMode");
}

void ResourceHandle::platformLoadResourceSynchronously(NetworkingContext* context, const ResourceRequest& request, StoredCredentials storedCredentials, ResourceError& error, ResourceResponse& response, Vector<char>& data)
{
    LOG(Network, "ResourceHandle::platformLoadResourceSynchronously:%s allowStoredCredentials:%u", request.url().string().utf8().data(), storedCredentials);

    ASSERT(!request.isEmpty());

    ASSERT(response.isNull());
    ASSERT(error.isNull());

    OwnPtr<SynchronousLoaderClient> client = SynchronousLoaderClient::create();
    client->setAllowStoredCredentials(storedCredentials == AllowStoredCredentials);

    RefPtr<ResourceHandle> handle = adoptRef(new ResourceHandle(context, request, client.get(), false /*defersLoading*/, true /*shouldContentSniff*/));

    handle->d->m_storageSession = context->storageSession().platformSession();

    if (handle->d->m_scheduledFailureType != NoFailure) {
        error = context->blockedError(request);
        return;
    }

    handle->createCFURLConnection(storedCredentials == AllowStoredCredentials, ResourceHandle::shouldContentSniffURL(request.url()));

    CFURLConnectionScheduleWithRunLoop(handle->connection(), CFRunLoopGetCurrent(), synchronousLoadRunLoopMode());
    CFURLConnectionScheduleDownloadWithRunLoop(handle->connection(), CFRunLoopGetCurrent(), synchronousLoadRunLoopMode());
    CFURLConnectionStart(handle->connection());

    while (!client->isDone())
        CFRunLoopRunInMode(synchronousLoadRunLoopMode(), UINT_MAX, true);

    error = client->error();

    CFURLConnectionCancel(handle->connection());
    
    if (error.isNull())
        response = client->response();
    else {
        response = ResourceResponse(request.url(), String(), 0, String(), String());
        // FIXME: ResourceHandleMac also handles authentication errors by setting code to 401. CFNet version should probably do the same.
        if (error.domain() == String(kCFErrorDomainCFNetwork))
            response.setHTTPStatusCode(error.errorCode());
        else
            response.setHTTPStatusCode(404);
    }

    data.swap(client->mutableData());
}

void ResourceHandle::setHostAllowsAnyHTTPSCertificate(const String& host)
{
    allowsAnyHTTPSCertificateHosts().add(host.lower());
}

void ResourceHandle::setClientCertificate(const String& host, CFDataRef cert)
{
    clientCerts().set(host.lower(), cert);
}

void ResourceHandle::platformSetDefersLoading(bool defers)
{
    if (!d->m_connection)
        return;

    if (defers)
        CFURLConnectionHalt(d->m_connection.get());
    else
        CFURLConnectionResume(d->m_connection.get());
}

bool ResourceHandle::loadsBlocked()
{
    return false;
}

#if PLATFORM(MAC)
void ResourceHandle::schedule(SchedulePair* pair)
{
    CFRunLoopRef runLoop = pair->runLoop();
    if (!runLoop)
        return;

    CFURLConnectionScheduleWithRunLoop(d->m_connection.get(), runLoop, pair->mode());
    if (d->m_startWhenScheduled) {
        CFURLConnectionStart(d->m_connection.get());
        d->m_startWhenScheduled = false;
    }
}

void ResourceHandle::unschedule(SchedulePair* pair)
{
    CFRunLoopRef runLoop = pair->runLoop();
    if (!runLoop)
        return;

    CFURLConnectionUnscheduleFromRunLoop(d->m_connection.get(), runLoop, pair->mode());
}
#endif

#endif // USE(CFNETWORK)

#if USE(NETWORK_CFDATA_ARRAY_CALLBACK)
void ResourceHandle::handleDataArray(CFArrayRef dataArray)
{
    ASSERT(client());
    if (client()->supportsDataArray()) {
        client()->didReceiveDataArray(this, dataArray);
        return;
    }

    CFIndex count = CFArrayGetCount(dataArray);
    ASSERT(count);
    if (count == 1) {
        CFDataRef data = static_cast<CFDataRef>(CFArrayGetValueAtIndex(dataArray, 0));
        CFIndex length = CFDataGetLength(data);
        client()->didReceiveData(this, reinterpret_cast<const char*>(CFDataGetBytePtr(data)), length, static_cast<int>(length));
        return;
    }

    CFIndex totalSize = 0;
    CFIndex index;
    for (index = 0; index < count; index++)
        totalSize += CFDataGetLength(static_cast<CFDataRef>(CFArrayGetValueAtIndex(dataArray, index)));

    RetainPtr<CFMutableDataRef> mergedData = adoptCF(CFDataCreateMutable(kCFAllocatorDefault, totalSize));
    for (index = 0; index < count; index++) {
        CFDataRef data = static_cast<CFDataRef>(CFArrayGetValueAtIndex(dataArray, index));
        CFDataAppendBytes(mergedData.get(), CFDataGetBytePtr(data), CFDataGetLength(data));
    }

    client()->didReceiveData(this, reinterpret_cast<const char*>(CFDataGetBytePtr(mergedData.get())), totalSize, static_cast<int>(totalSize));
}
#endif

} // namespace WebCore
