/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#include "config.h"
#include "MessageReceiverMap.h"

#include "MessageDecoder.h"
#include "MessageReceiver.h"

namespace CoreIPC {

MessageReceiverMap::MessageReceiverMap()
{
}

MessageReceiverMap::~MessageReceiverMap()
{
}

void MessageReceiverMap::addMessageReceiver(StringReference messageReceiverName, MessageReceiver* messageReceiver)
{
    ASSERT(!m_globalMessageReceivers.contains(messageReceiverName));
    m_globalMessageReceivers.set(messageReceiverName, messageReceiver);
}

void MessageReceiverMap::addMessageReceiver(StringReference messageReceiverName, uint64_t destinationID, MessageReceiver* messageReceiver)
{
    ASSERT(!m_messageReceivers.contains(std::make_pair(messageReceiverName, destinationID)));
    ASSERT(!m_globalMessageReceivers.contains(messageReceiverName));

    m_messageReceivers.set(std::make_pair(messageReceiverName, destinationID), messageReceiver);
}

void MessageReceiverMap::removeMessageReceiver(StringReference messageReceiverName)
{
    ASSERT(m_globalMessageReceivers.contains(messageReceiverName));

    m_globalMessageReceivers.remove(messageReceiverName);
}

void MessageReceiverMap::removeMessageReceiver(StringReference messageReceiverName, uint64_t destinationID)
{
    ASSERT(m_messageReceivers.contains(std::make_pair(messageReceiverName, destinationID)));

    m_messageReceivers.remove(std::make_pair(messageReceiverName, destinationID));
}

void MessageReceiverMap::invalidate()
{
    m_globalMessageReceivers.clear();
    m_messageReceivers.clear();
}

bool MessageReceiverMap::dispatchMessage(Connection* connection, MessageDecoder& decoder)
{
    if (MessageReceiver* messageReceiver = m_globalMessageReceivers.get(decoder.messageReceiverName())) {
        ASSERT(!decoder.destinationID());

        messageReceiver->didReceiveMessage(connection, decoder);
        return true;
    }

    if (MessageReceiver* messageReceiver = m_messageReceivers.get(std::make_pair(decoder.messageReceiverName(), decoder.destinationID()))) {
        messageReceiver->didReceiveMessage(connection, decoder);
        return true;
    }

    return false;
}

bool MessageReceiverMap::dispatchSyncMessage(Connection* connection, MessageDecoder& decoder, OwnPtr<MessageEncoder>& replyEncoder)
{
    if (MessageReceiver* messageReceiver = m_globalMessageReceivers.get(decoder.messageReceiverName())) {
        ASSERT(!decoder.destinationID());

        messageReceiver->didReceiveSyncMessage(connection, decoder, replyEncoder);
        return true;
    }

    if (MessageReceiver* messageReceiver = m_messageReceivers.get(std::make_pair(decoder.messageReceiverName(), decoder.destinationID()))) {
        messageReceiver->didReceiveSyncMessage(connection, decoder, replyEncoder);
        return true;
    }

    return false;
}

} // namespace CoreIPC
