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

#ifndef MessageDecoder_h
#define MessageDecoder_h

#include "ArgumentDecoder.h"
#include "StringReference.h"

namespace CoreIPC {

class DataReference;
class ImportanceAssertion;

class MessageDecoder : public ArgumentDecoder {
public:
    static PassOwnPtr<MessageDecoder> create(const DataReference& buffer);
    static PassOwnPtr<MessageDecoder> create(const DataReference& buffer, Vector<Attachment>&);
    virtual ~MessageDecoder();

    StringReference messageReceiverName() const { return m_messageReceiverName; }
    StringReference messageName() const { return m_messageName; }

    bool isSyncMessage() const;
    bool shouldDispatchMessageWhenWaitingForSyncReply() const;

#if PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    void setImportanceAssertion(PassOwnPtr<ImportanceAssertion>);
#endif

private:
    MessageDecoder(const DataReference& buffer, Vector<Attachment>&);

    uint8_t m_messageFlags;
    StringReference m_messageReceiverName;
    StringReference m_messageName;

#if PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    OwnPtr<ImportanceAssertion> m_importanceAssertion;
#endif
};

} // namespace CoreIPC

#endif // MessageDecoder_h
