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

#ifndef WebURLRequest_h
#define WebURLRequest_h

#include "APIObject.h"
#include <WebCore/ResourceRequest.h>
#include <wtf/Forward.h>

#if PLATFORM(MAC)
typedef NSURLRequest* PlatformRequest;
#else
typedef void* PlatformRequest;
#endif

namespace WebKit {

class WebURLRequest : public TypedAPIObject<APIObject::TypeURLRequest> {
public:
    static PassRefPtr<WebURLRequest> create(const WebCore::KURL&);

    static PassRefPtr<WebURLRequest> create(const WebCore::ResourceRequest& request)
    {
        return adoptRef(new WebURLRequest(request));
    }

    static PassRefPtr<WebURLRequest> create(PlatformRequest platformRequest)
    {
        return adoptRef(new WebURLRequest(platformRequest));
    }

    PlatformRequest platformRequest() const;
    const WebCore::ResourceRequest& resourceRequest() const { return m_request; }

    const String& url() const { return m_request.url(); }

    static double defaultTimeoutInterval(); // May return 0 when using platform default.
    static void setDefaultTimeoutInterval(double);

private:
    explicit WebURLRequest(const WebCore::ResourceRequest&);
    explicit WebURLRequest(PlatformRequest);

    WebCore::ResourceRequest m_request;
};

} // namespace WebKit

#endif // WebURLRequest_h
