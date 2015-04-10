/*
 * Copyright (C) 2013 Samsung Electronics
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TextCheckerCompletion_h
#define TextCheckerCompletion_h

#include "WebPageProxy.h"
#include <WebCore/TextChecking.h>
#include <wtf/Vector.h>

namespace WebKit {

class TextCheckerCompletion : public RefCounted<TextCheckerCompletion> {
public:
    static PassRefPtr<TextCheckerCompletion> create(uint64_t requestID, const WebCore::TextCheckingRequestData&, WebPageProxy*);

    const WebCore::TextCheckingRequestData& textCheckingRequestData() const;
    int64_t spellDocumentTag();
    void didFinishCheckingText(const Vector<WebCore::TextCheckingResult>&) const;
    void didCancelCheckingText() const;

private:
    TextCheckerCompletion(uint64_t requestID, const WebCore::TextCheckingRequestData&, WebPageProxy*);

    const uint64_t m_requestID;
    const WebCore::TextCheckingRequestData m_requestData;
    WebPageProxy* m_page;
};

} // namespace WebKit

#endif // TextCheckerCompletion_h
