/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

#ifndef ewk_url_request_private_h
#define ewk_url_request_private_h

#include "WKEinaSharedString.h"
#include "WKURL.h"
#include "WKURLRequest.h"
#include "WebURLRequest.h"
#include "ewk_object_private.h"
#include <wtf/PassRefPtr.h>

/**
 * \struct  EwkUrlRequest
 * @brief   Contains the URL request data.
 */
class EwkUrlRequest : public EwkObject {
public:
    EWK_OBJECT_DECLARE(EwkUrlRequest)

    static PassRefPtr<EwkUrlRequest> create(WKURLRequestRef requestRef)
    {
        return adoptRef(new EwkUrlRequest(requestRef));
    }

    const char* url() const;
    const char* firstParty() const;
    const char* httpMethod() const;

private:
    explicit EwkUrlRequest(WKURLRequestRef requestRef);

    WKEinaSharedString m_url;
    WKEinaSharedString m_firstParty;
    WKEinaSharedString m_httpMethod;
};

#endif // ewk_url_request_private_h
