/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
#include "WebConnectionToWebProcess.h"

#include "WebConnectionMessages.h"
#include "WebContextUserMessageCoders.h"
#include "WebProcessProxy.h"

namespace WebKit {

PassRefPtr<WebConnectionToWebProcess> WebConnectionToWebProcess::create(WebProcessProxy* process)
{
    return adoptRef(new WebConnectionToWebProcess(process));
}

WebConnectionToWebProcess::WebConnectionToWebProcess(WebProcessProxy* process)
    : m_process(process)
{
    m_process->addMessageReceiver(Messages::WebConnection::messageReceiverName(), this);
}

void WebConnectionToWebProcess::invalidate()
{
    m_process = 0;
}

// WebConnection

void WebConnectionToWebProcess::encodeMessageBody(CoreIPC::ArgumentEncoder& encoder, APIObject* messageBody)
{
    encoder << WebContextUserMessageEncoder(messageBody);
}

bool WebConnectionToWebProcess::decodeMessageBody(CoreIPC::ArgumentDecoder& decoder, RefPtr<APIObject>& messageBody)
{
    WebContextUserMessageDecoder messageBodyDecoder(messageBody, m_process);
    return decoder.decode(messageBodyDecoder);
}

bool WebConnectionToWebProcess::hasValidConnection() const
{
    return m_process;
}

CoreIPC::Connection* WebConnectionToWebProcess::messageSenderConnection()
{
    return m_process->connection();
}

uint64_t WebConnectionToWebProcess::messageSenderDestinationID()
{
    return 0;
}

} // namespace WebKit
