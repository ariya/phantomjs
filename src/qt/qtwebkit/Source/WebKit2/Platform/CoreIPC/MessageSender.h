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

#ifndef MessageSender_h
#define MessageSender_h

#include <wtf/Assertions.h>
#include "Connection.h"

namespace CoreIPC {

class MessageSender {
public:
    virtual ~MessageSender();

    template<typename U> bool send(const U& message)
    {
        return send(message, messageSenderDestinationID());
    }

    template<typename U> bool send(const U& message, uint64_t destinationID)
    {
        COMPILE_ASSERT(!U::isSync, AsyncMessageExpected);
        OwnPtr<MessageEncoder> encoder = MessageEncoder::create(U::receiverName(), U::name(), destinationID);
        encoder->encode(message);
        
        return sendMessage(encoder.release());
    }

    template<typename U> bool sendSync(const U& message, const typename U::Reply& reply, double timeout = Connection::NoTimeout)
    {
        COMPILE_ASSERT(U::isSync, SyncMessageExpected);
        return sendSync(message, reply, messageSenderDestinationID(), timeout);
    }

    template<typename U> bool sendSync(const U& message, const typename U::Reply& reply, uint64_t destinationID, double timeout = Connection::NoTimeout)
    {
        ASSERT(messageSenderConnection());

        return messageSenderConnection()->sendSync(message, reply, destinationID, timeout);
    }

    bool sendMessage(PassOwnPtr<MessageEncoder>);

private:
    virtual Connection* messageSenderConnection() = 0;
    virtual uint64_t messageSenderDestinationID() = 0;
};

} // namespace CoreIPC

#endif // MessageSender_h
