/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SocketStreamHandleBase_h
#define SocketStreamHandleBase_h

#include "KURL.h"

#include <wtf/Vector.h>

namespace WebCore {

    class SocketStreamHandle;
    class SocketStreamHandleClient;

    class SocketStreamHandleBase {
    public:
        enum SocketStreamState { Connecting, Open, Closed };
        virtual ~SocketStreamHandleBase() { }
        SocketStreamState state() const;

        bool send(const char* data, int length);
        void close();
        int bufferedAmount() const { return m_buffer.size(); }

        SocketStreamHandleClient* client() const { return m_client; }
        void setClient(SocketStreamHandleClient*);

    protected:
        SocketStreamHandleBase(const KURL&, SocketStreamHandleClient*);

        bool sendPendingData();
        virtual int platformSend(const char* data, int length) = 0;
        virtual void platformClose() = 0;

        KURL m_url;
        SocketStreamHandleClient* m_client;
        Vector<char> m_buffer;
        SocketStreamState m_state;
    };

}  // namespace WebCore

#endif  // SocketStreamHandleBase_h
