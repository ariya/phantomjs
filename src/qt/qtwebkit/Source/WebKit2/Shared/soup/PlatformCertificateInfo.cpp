/*
 * Copyright (C) 2012 Igalia S.L.
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
#include "PlatformCertificateInfo.h"

#include "ArgumentDecoder.h"
#include "ArgumentEncoder.h"
#include "DataReference.h"
#include <WebCore/ResourceError.h>
#include <WebCore/ResourceResponse.h>
#include <libsoup/soup.h>

using namespace WebCore;

namespace WebKit {

PlatformCertificateInfo::PlatformCertificateInfo()
    : m_tlsErrors(static_cast<GTlsCertificateFlags>(0))
{
}

PlatformCertificateInfo::PlatformCertificateInfo(const ResourceResponse& response)
    : m_certificate(response.soupMessageCertificate())
    , m_tlsErrors(response.soupMessageTLSErrors())
{
}

PlatformCertificateInfo::PlatformCertificateInfo(const ResourceError& resourceError)
    : m_certificate(resourceError.certificate())
    , m_tlsErrors(static_cast<GTlsCertificateFlags>(resourceError.tlsErrors()))
{
}

PlatformCertificateInfo::~PlatformCertificateInfo()
{
}

void PlatformCertificateInfo::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    if (!m_certificate) {
        encoder << false;
        return;
    }

    GByteArray* certificateData = 0;
    g_object_get(G_OBJECT(m_certificate.get()), "certificate", &certificateData, NULL);
    if (!certificateData) {
        encoder << false;
        return;
    }

    encoder << true;
    GRefPtr<GByteArray> certificate = adoptGRef(certificateData);
    encoder.encodeVariableLengthByteArray(CoreIPC::DataReference(certificate->data, certificate->len));
    encoder << static_cast<uint32_t>(m_tlsErrors);
}

bool PlatformCertificateInfo::decode(CoreIPC::ArgumentDecoder& decoder, PlatformCertificateInfo& certificateInfo)
{
    bool hasCertificate;
    if (!decoder.decode(hasCertificate))
        return false;

    if (!hasCertificate)
        return true;

    CoreIPC::DataReference certificateDataReference;
    if (!decoder.decodeVariableLengthByteArray(certificateDataReference))
        return false;

    GByteArray* certificateData = g_byte_array_sized_new(certificateDataReference.size());
    certificateData = g_byte_array_append(certificateData, certificateDataReference.data(), certificateDataReference.size());
    GRefPtr<GByteArray> certificate = adoptGRef(certificateData);

    GTlsBackend* backend = g_tls_backend_get_default();
    certificateInfo.m_certificate = adoptGRef(G_TLS_CERTIFICATE(g_initable_new(g_tls_backend_get_certificate_type(backend), 0, 0,
                                                                               "certificate", certificate.get(), NULL)));

    uint32_t tlsErrors;
    if (!decoder.decode(tlsErrors))
        return false;
    certificateInfo.m_tlsErrors = static_cast<GTlsCertificateFlags>(tlsErrors);

    return true;
}

} // namespace WebKit
