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

#import "config.h"
#import "PlatformCertificateInfo.h"

#import "ArgumentCodersCF.h"
#import "ArgumentDecoder.h"
#import "ArgumentEncoder.h"

using namespace WebCore;

namespace WebKit {

PlatformCertificateInfo::PlatformCertificateInfo()
{
}

PlatformCertificateInfo::PlatformCertificateInfo(const ResourceResponse& response)
    : m_certificateChain(response.certificateChain())
{
}

PlatformCertificateInfo::PlatformCertificateInfo(CFArrayRef certificateChain)
    : m_certificateChain(certificateChain)
{
}

void PlatformCertificateInfo::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    if (!m_certificateChain) {
        encoder << false;
        return;
    }

    encoder << true;
    CoreIPC::encode(encoder, m_certificateChain.get());
}

bool PlatformCertificateInfo::decode(CoreIPC::ArgumentDecoder& decoder, PlatformCertificateInfo& c)
{
    bool hasCertificateChain;
    if (!decoder.decode(hasCertificateChain))
        return false;

    if (!hasCertificateChain)
        return true;

    if (!CoreIPC::decode(decoder, c.m_certificateChain))
        return false;

    return true;
}

#ifndef NDEBUG
void PlatformCertificateInfo::dump() const
{
    unsigned entries = m_certificateChain ? CFArrayGetCount(m_certificateChain.get()) : 0;

    NSLog(@"PlatformCertificateInfo\n");
    NSLog(@"  Entries: %d\n", entries);
    for (unsigned i = 0; i < entries; ++i) {
        RetainPtr<CFStringRef> summary = adoptCF(SecCertificateCopySubjectSummary((SecCertificateRef)CFArrayGetValueAtIndex(m_certificateChain.get(), i)));
        NSLog(@"  %@", (NSString *)summary.get());
    }
}
#endif

} // namespace WebKit
