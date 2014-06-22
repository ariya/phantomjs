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

#ifndef ewk_url_response_private_h
#define ewk_url_response_private_h

#include "WKAPICast.h"
#include "WKEinaSharedString.h"
#include "WKURLResponse.h"
#include "ewk_object_private.h"
#include <wtf/PassRefPtr.h>

/**
 * \struct  EwkUrlResponse
 * @brief   Contains the URL response data.
 */
class EwkUrlResponse : public EwkObject {
public:
    EWK_OBJECT_DECLARE(EwkUrlResponse)

    static PassRefPtr<EwkUrlResponse> create(WKURLResponseRef wkResponse)
    {
        if (!wkResponse)
            return 0;

        return adoptRef(new EwkUrlResponse(wkResponse));
    }

    int httpStatusCode() const;
    const char* url() const;
    const char* mimeType() const;
    unsigned long contentLength() const;

private:
    explicit EwkUrlResponse(WKURLResponseRef response);

    WKRetainPtr<WKURLResponseRef> m_response;
    WKEinaSharedString m_url;
    WKEinaSharedString m_mimeType;
};

#endif // ewk_url_response_private_h
