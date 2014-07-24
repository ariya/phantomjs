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

#ifndef APIClient_h
#define APIClient_h

#include "APIClientTraits.h"

namespace WebKit {

template<typename ClientInterface, int currentVersion> class APIClient {
public:
    APIClient()
    {
        initialize(0);
    }
    
    void initialize(const ClientInterface* client)
    {
        COMPILE_ASSERT(sizeof(APIClientTraits<ClientInterface>::interfaceSizesByVersion) / sizeof(size_t) == currentVersion + 1, size_of_some_interfaces_are_unknown);

        if (client && client->version == currentVersion) {
            m_client = *client;
            return;
        }

        memset(&m_client, 0, sizeof(m_client));

        if (client && client->version < currentVersion)
            memcpy(&m_client, client, APIClientTraits<ClientInterface>::interfaceSizesByVersion[client->version]);
    }

    const ClientInterface& client() const { return m_client; }

protected:
    ClientInterface m_client;
};

} // namespace WebKit

#endif // APIClient_h
