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
#include "WebConnectionToUIProcess.h"

#include "InjectedBundleUserMessageCoders.h"
#include "WebConnectionMessages.h"
#include "WebProcess.h"

using namespace WebCore;

namespace WebKit {

PassRefPtr<WebConnectionToUIProcess> WebConnectionToUIProcess::create(WebProcess* process)
{
    return adoptRef(new WebConnectionToUIProcess(process));
}

WebConnectionToUIProcess::WebConnectionToUIProcess(WebProcess* process)
    : m_process(process)
{
    m_process->addMessageReceiver(Messages::WebConnection::messageReceiverName(), this);
}

void WebConnectionToUIProcess::invalidate()
{
    m_process = 0;
}

// WebConnection

void WebConnectionToUIProcess::encodeMessageBody(CoreIPC::ArgumentEncoder& encoder, APIObject* messageBody)
{
    encoder << InjectedBundleUserMessageEncoder(messageBody);
}

bool WebConnectionToUIProcess::decodeMessageBody(CoreIPC::ArgumentDecoder& decoder, RefPtr<APIObject>& messageBody)
{
    InjectedBundleUserMessageDecoder messageBodyDecoder(messageBody);
    return decoder.decode(messageBodyDecoder);
}

bool WebConnectionToUIProcess::hasValidConnection() const
{
    return m_process;
}

CoreIPC::Connection* WebConnectionToUIProcess::messageSenderConnection()
{
    return m_process->parentProcessConnection();
}

uint64_t WebConnectionToUIProcess::messageSenderDestinationID()
{
    return 0;
}

} // namespace WebKit
