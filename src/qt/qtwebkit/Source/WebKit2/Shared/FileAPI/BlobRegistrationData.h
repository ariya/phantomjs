/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef BlobRegistrationData_h
#define BlobRegistrationData_h

#if ENABLE(BLOB) && ENABLE(NETWORK_PROCESS)

#include "SandboxExtension.h"

namespace WebCore {
class BlobData;
}

namespace WebKit {

class BlobRegistrationData {
WTF_MAKE_NONCOPYABLE(BlobRegistrationData);
public:
    BlobRegistrationData();
    BlobRegistrationData(PassOwnPtr<WebCore::BlobData>);
    ~BlobRegistrationData();

    void encode(CoreIPC::ArgumentEncoder&) const;
    static bool decode(CoreIPC::ArgumentDecoder&, BlobRegistrationData&);

    PassOwnPtr<WebCore::BlobData> releaseData() const;
    const SandboxExtension::HandleArray& sandboxExtensions() const { return m_sandboxExtensions; }

private:
    mutable OwnPtr<WebCore::BlobData> m_data;
    SandboxExtension::HandleArray m_sandboxExtensions;
};

}

#endif // ENABLE(BLOB) && ENABLE(NETWORK_PROCESS)

#endif // BlobRegistrationData_h
