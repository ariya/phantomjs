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
#include "CertificateCFWin.h"

namespace WebCore {

static void deallocCertContext(void* ptr, void* info)
{
    if (ptr)
        CertFreeCertificateContext(static_cast<PCCERT_CONTEXT>(ptr));
}

static CFAllocatorRef createCertContextDeallocator()
{
    CFAllocatorContext allocContext = {
        0, 0, 0, 0, 0, 0, 0, deallocCertContext, 0
    };
    return CFAllocatorCreate(kCFAllocatorDefault, &allocContext);
}

RetainPtr<CFDataRef> copyCertificateToData(PCCERT_CONTEXT certificate)
{
    static CFAllocatorRef certDealloc = createCertContextDeallocator();
    PCCERT_CONTEXT certificateCopy = CertDuplicateCertificateContext(certificate);
    return RetainPtr<CFDataRef>(AdoptCF, CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, reinterpret_cast<const UInt8*>(certificateCopy), sizeof(*certificateCopy), certDealloc));
}

} // namespace WebCore
