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
#include "MessageDecoder.h"

#include "ArgumentCoders.h"
#include "DataReference.h"
#include "MessageFlags.h"
#include "StringReference.h"

#if PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
#include "ImportanceAssertion.h"
#endif

namespace CoreIPC {

PassOwnPtr<MessageDecoder> MessageDecoder::create(const DataReference& buffer)
{
    Vector<Attachment> attachments;
    return adoptPtr(new MessageDecoder(buffer, attachments));
}

PassOwnPtr<MessageDecoder> MessageDecoder::create(const DataReference& buffer, Vector<Attachment>& attachments)
{
    return adoptPtr(new MessageDecoder(buffer, attachments));
}

MessageDecoder::~MessageDecoder()
{
}

MessageDecoder::MessageDecoder(const DataReference& buffer, Vector<Attachment>& attachments)
    : ArgumentDecoder(buffer.data(), buffer.size(), attachments)
{
    if (!decode(m_messageFlags))
        return;

    if (!decode(m_messageReceiverName))
        return;

    if (!decode(m_messageName))
        return;

    decode(m_destinationID);
}

bool MessageDecoder::isSyncMessage() const
{
    return m_messageFlags & SyncMessage;
}

bool MessageDecoder::shouldDispatchMessageWhenWaitingForSyncReply() const
{
    return m_messageFlags & DispatchMessageWhenWaitingForSyncReply;
}

#if PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
void MessageDecoder::setImportanceAssertion(PassOwnPtr<ImportanceAssertion> assertion)
{
    m_importanceAssertion = assertion;
}
#endif

} // namespace CoreIPC
