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

#ifndef MessageReceiverMap_h
#define MessageReceiverMap_h

#include "StringReference.h"
#include <wtf/HashMap.h>
#include <wtf/text/CString.h>

namespace CoreIPC {

class Connection;
class MessageDecoder;
class MessageEncoder;
class MessageReceiver;

class MessageReceiverMap {
public:
    MessageReceiverMap();
    ~MessageReceiverMap();

    void addMessageReceiver(StringReference messageReceiverName, MessageReceiver*);
    void addMessageReceiver(StringReference messageReceiverName, uint64_t destinationID, MessageReceiver*);

    void removeMessageReceiver(StringReference messageReceiverName);
    void removeMessageReceiver(StringReference messageReceiverName, uint64_t destinationID);

    void invalidate();

    bool dispatchMessage(Connection*, MessageDecoder&);
    bool dispatchSyncMessage(Connection*, MessageDecoder&, OwnPtr<MessageEncoder>&);

private:
    // Message receivers that don't require a destination ID.
    HashMap<StringReference, MessageReceiver*> m_globalMessageReceivers;

    HashMap<std::pair<StringReference, uint64_t>, MessageReceiver*> m_messageReceivers;
};

};

#endif // MessageReceiverMap_h
