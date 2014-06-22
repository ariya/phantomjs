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

#ifndef WebError_h
#define WebError_h

#include "APIObject.h"
#include <WebCore/ResourceError.h>
#include <wtf/PassRefPtr.h>

namespace WebKit {

// WebError - An error type suitable for vending to an API.

class WebError : public TypedAPIObject<APIObject::TypeError> {
public:
    static PassRefPtr<WebError> create()
    {
        return adoptRef(new WebError);
    }

    static PassRefPtr<WebError> create(const WebCore::ResourceError& error)
    {
        return adoptRef(new WebError(error));
    }

    static const String& webKitErrorDomain();

    const String& domain() const { return m_platformError.domain(); }
    int errorCode() const { return m_platformError.errorCode(); }
    const String& failingURL() const { return m_platformError.failingURL(); }
    const String& localizedDescription() const { return m_platformError.localizedDescription(); }

    const WebCore::ResourceError& platformError() const { return m_platformError; }

private:
    WebError()
    {
    }

    WebError(const WebCore::ResourceError& error)
        : m_platformError(error)
    {
    }

    WebCore::ResourceError m_platformError;
};

} // namespace WebKit

#endif // WebError_h
