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
#include "ResourceError.h"

#if USE(CFNETWORK)

#include "KURL.h"
#include <CoreFoundation/CFError.h>
#include <CFNetwork/CFNetworkErrors.h>
#include <wtf/RetainPtr.h>

#if PLATFORM(WIN)
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#endif

namespace WebCore {

ResourceError::ResourceError(CFErrorRef cfError)
    : m_dataIsUpToDate(false)
    , m_platformError(cfError)
{
    m_isNull = !cfError;
    if (!m_isNull)
        m_isTimeout = CFErrorGetCode(m_platformError.get()) == kCFURLErrorTimedOut;
}

#if PLATFORM(WIN)
ResourceError::ResourceError(const String& domain, int errorCode, const String& failingURL, const String& localizedDescription, CFDataRef certificate)
    : ResourceErrorBase(domain, errorCode, failingURL, localizedDescription)
    , m_dataIsUpToDate(true)
    , m_certificate(certificate)
{
}

PCCERT_CONTEXT ResourceError::certificate() const
{
    if (!m_certificate)
        return 0;
    
    return reinterpret_cast<PCCERT_CONTEXT>(CFDataGetBytePtr(m_certificate.get()));
}

void ResourceError::setCertificate(CFDataRef certificate)
{
    m_certificate = certificate;
}
#endif // PLATFORM(WIN)

const CFStringRef failingURLStringKey = CFSTR("NSErrorFailingURLStringKey");
const CFStringRef failingURLKey = CFSTR("NSErrorFailingURLKey");

void ResourceError::platformLazyInit()
{
    if (m_dataIsUpToDate)
        return;

    if (!m_platformError)
        return;

    CFStringRef domain = CFErrorGetDomain(m_platformError.get());
    if (domain == kCFErrorDomainMach || domain == kCFErrorDomainCocoa)
        m_domain ="NSCustomErrorDomain";
    else if (domain == kCFErrorDomainCFNetwork)
        m_domain = "CFURLErrorDomain";
    else if (domain == kCFErrorDomainPOSIX)
        m_domain = "NSPOSIXErrorDomain";
    else if (domain == kCFErrorDomainOSStatus)
        m_domain = "NSOSStatusErrorDomain";
    else if (domain == kCFErrorDomainWinSock)
        m_domain = "kCFErrorDomainWinSock";
    else
        m_domain = domain;

    m_errorCode = CFErrorGetCode(m_platformError.get());

    RetainPtr<CFDictionaryRef> userInfo = adoptCF(CFErrorCopyUserInfo(m_platformError.get()));
    if (userInfo.get()) {
        CFStringRef failingURLString = (CFStringRef) CFDictionaryGetValue(userInfo.get(), failingURLStringKey);
        if (failingURLString)
            m_failingURL = String(failingURLString);
        else {
            CFURLRef failingURL = (CFURLRef) CFDictionaryGetValue(userInfo.get(), failingURLKey);
            if (failingURL) {
                RetainPtr<CFURLRef> absoluteURLRef = adoptCF(CFURLCopyAbsoluteURL(failingURL));
                if (absoluteURLRef.get()) {
                    // FIXME: CFURLGetString returns a normalized URL which is different from what is actually used by CFNetwork.
                    // We should use CFURLGetBytes instead.
                    failingURLString = CFURLGetString(absoluteURLRef.get());
                    m_failingURL = String(failingURLString);
                }
            }
        }
        m_localizedDescription = (CFStringRef) CFDictionaryGetValue(userInfo.get(), kCFErrorLocalizedDescriptionKey);
        
#if PLATFORM(WIN)
        m_certificate = wkGetSSLPeerCertificateData(userInfo.get());
#endif
    }

    m_dataIsUpToDate = true;
}

void ResourceError::platformCopy(ResourceError& errorCopy) const
{
#if PLATFORM(WIN)
    errorCopy.m_certificate = m_certificate;
#else
    UNUSED_PARAM(errorCopy);
#endif
}

bool ResourceError::platformCompare(const ResourceError& a, const ResourceError& b)
{
    return a.cfError() == b.cfError();
}

CFErrorRef ResourceError::cfError() const
{
    if (m_isNull) {
        ASSERT(!m_platformError);
        return 0;
    }

    if (!m_platformError) {
        RetainPtr<CFMutableDictionaryRef> userInfo = adoptCF(CFDictionaryCreateMutable(0, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));

        if (!m_localizedDescription.isEmpty())
            CFDictionarySetValue(userInfo.get(), kCFErrorLocalizedDescriptionKey, m_localizedDescription.createCFString().get());

        if (!m_failingURL.isEmpty()) {
            RetainPtr<CFStringRef> failingURLString = m_failingURL.createCFString();
            CFDictionarySetValue(userInfo.get(), failingURLStringKey, failingURLString.get());
            RetainPtr<CFURLRef> url = adoptCF(CFURLCreateWithString(0, failingURLString.get(), 0));
            CFDictionarySetValue(userInfo.get(), failingURLKey, url.get());
        }

#if PLATFORM(WIN)
        if (m_certificate)
            wkSetSSLPeerCertificateData(userInfo.get(), m_certificate.get());
#endif
        
        m_platformError = adoptCF(CFErrorCreate(0, m_domain.createCFString().get(), m_errorCode, userInfo.get()));
    }

    return m_platformError.get();
}

ResourceError::operator CFErrorRef() const
{
    return cfError();
}

// FIXME: Once <rdar://problem/5050841> is fixed we can remove this constructor.
ResourceError::ResourceError(CFStreamError error)
    : m_dataIsUpToDate(true)
{
    m_isNull = false;
    m_errorCode = error.error;

    switch(error.domain) {
    case kCFStreamErrorDomainCustom:
        m_domain ="NSCustomErrorDomain";
        break;
    case kCFStreamErrorDomainPOSIX:
        m_domain = "NSPOSIXErrorDomain";
        break;
    case kCFStreamErrorDomainMacOSStatus:
        m_domain = "NSOSStatusErrorDomain";
        break;
    }
}

CFStreamError ResourceError::cfStreamError() const
{
    lazyInit();

    CFStreamError result;
    result.error = m_errorCode;

    if (m_domain == "NSCustomErrorDomain")
        result.domain = kCFStreamErrorDomainCustom;
    else if (m_domain == "NSPOSIXErrorDomain")
        result.domain = kCFStreamErrorDomainPOSIX;
    else if (m_domain == "NSOSStatusErrorDomain")
        result.domain = kCFStreamErrorDomainMacOSStatus;
    else {
        result.domain = kCFStreamErrorDomainCustom;
        ASSERT_NOT_REACHED();
    }

    return result;
}

ResourceError::operator CFStreamError() const
{
    return cfStreamError();
}

} // namespace WebCore

#endif // USE(CFNETWORK)
