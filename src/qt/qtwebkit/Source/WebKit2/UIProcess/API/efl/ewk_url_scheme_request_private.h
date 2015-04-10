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

#ifndef ewk_url_scheme_request_private_h
#define ewk_url_scheme_request_private_h

#include "WKAPICast.h"
#include "WKBase.h"
#include "WKEinaSharedString.h"
#include "WKRetainPtr.h"
#include "WKSoupRequestManager.h"
#include "ewk_object_private.h"

/**
 * \struct  EwkUrlSchemeRequest
 * @brief   Contains the URL scheme request data.
 */
class EwkUrlSchemeRequest : public EwkObject {
public:
    EWK_OBJECT_DECLARE(EwkUrlSchemeRequest)

    static PassRefPtr<EwkUrlSchemeRequest> create(WKSoupRequestManagerRef manager, WKURLRef url, uint64_t requestID)
    {
        if (!manager || !url)
            return 0;

        return adoptRef(new EwkUrlSchemeRequest(manager, url, requestID));
    }

    uint64_t id() const;
    const char* url() const;
    const char* scheme() const;
    const char* path() const;

    void finish(const void* contentData, uint64_t contentLength, const char* mimeType);

private:
    EwkUrlSchemeRequest(WKSoupRequestManagerRef manager, WKURLRef urlRef, uint64_t requestID);

    WKRetainPtr<WKSoupRequestManagerRef> m_wkRequestManager;
    WKEinaSharedString m_url;
    uint64_t m_requestID;
    WKEinaSharedString m_scheme;
    WKEinaSharedString m_path;
};

#endif // ewk_url_scheme_request_private_h
