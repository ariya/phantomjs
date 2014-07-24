/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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

#ifndef ResourceResponse_h
#define ResourceResponse_h

#include "ResourceResponseBase.h"

#include <libsoup/soup.h>
#include <wtf/gobject/GRefPtr.h>

namespace WebCore {

class ResourceResponse : public ResourceResponseBase {
public:
    ResourceResponse()
        : ResourceResponseBase()
        , m_soupFlags(static_cast<SoupMessageFlags>(0))
        , m_tlsErrors(static_cast<GTlsCertificateFlags>(0))
    {
    }

    ResourceResponse(const KURL& url, const String& mimeType, long long expectedLength, const String& textEncodingName, const String& filename)
        : ResourceResponseBase(url, mimeType, expectedLength, textEncodingName, filename)
        , m_soupFlags(static_cast<SoupMessageFlags>(0))
        , m_tlsErrors(static_cast<GTlsCertificateFlags>(0))
    {
    }

    ResourceResponse(SoupMessage* soupMessage)
        : ResourceResponseBase()
        , m_soupFlags(static_cast<SoupMessageFlags>(0))
        , m_tlsErrors(static_cast<GTlsCertificateFlags>(0))
    {
        updateFromSoupMessage(soupMessage);
    }

    SoupMessage* toSoupMessage() const;
    void updateFromSoupMessage(SoupMessage*);
    void updateFromSoupMessageHeaders(const SoupMessageHeaders*);

    SoupMessageFlags soupMessageFlags() const { return m_soupFlags; }
    void setSoupMessageFlags(SoupMessageFlags soupFlags) { m_soupFlags = soupFlags; }

    const String& sniffedContentType() const { return m_sniffedContentType; }
    void setSniffedContentType(const String& value) { m_sniffedContentType = value; }

    GTlsCertificate* soupMessageCertificate() const { return m_certificate.get(); }
    void setSoupMessageCertificate(GTlsCertificate* certificate) { m_certificate = certificate; }

    GTlsCertificateFlags soupMessageTLSErrors() const { return m_tlsErrors; }
    void setSoupMessageTLSErrors(GTlsCertificateFlags tlsErrors) { m_tlsErrors = tlsErrors; }

    bool platformResponseIsUpToDate() const { return false; }

private:
    friend class ResourceResponseBase;

    SoupMessageFlags m_soupFlags;
    String m_sniffedContentType;
    GRefPtr<GTlsCertificate> m_certificate;
    GTlsCertificateFlags m_tlsErrors;

    void doUpdateResourceResponse() { }

    PassOwnPtr<CrossThreadResourceResponseData> doPlatformCopyData(PassOwnPtr<CrossThreadResourceResponseData> data) const { return data; }
    void doPlatformAdopt(PassOwnPtr<CrossThreadResourceResponseData>) { }
};

struct CrossThreadResourceResponseData : public CrossThreadResourceResponseDataBase {
};

} // namespace WebCore

#endif // ResourceResponse_h
