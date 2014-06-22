/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#include "OfflineStorageProcess.h"

#include "OfflineStorageProcessCreationParameters.h"
#include <WebCore/RunLoop.h>

using namespace WebCore;

namespace WebKit {

OfflineStorageProcess& OfflineStorageProcess::shared()
{
    DEFINE_STATIC_LOCAL(OfflineStorageProcess, offlineStorageProcess, ());
    return offlineStorageProcess;
}

OfflineStorageProcess::OfflineStorageProcess()
{
}

OfflineStorageProcess::~OfflineStorageProcess()
{
}

bool OfflineStorageProcess::shouldTerminate()
{
    // FIXME: Implement.
    return false;
}

void OfflineStorageProcess::didReceiveMessage(CoreIPC::Connection* connection, CoreIPC::MessageDecoder& decoder)
{
    if (messageReceiverMap().dispatchMessage(connection, decoder))
        return;

    didReceiveOfflineStorageProcessMessage(connection, decoder);
}

void OfflineStorageProcess::didReceiveSyncMessage(CoreIPC::Connection* connection, CoreIPC::MessageDecoder& decoder, OwnPtr<CoreIPC::MessageEncoder>& replyEncoder)
{
    messageReceiverMap().dispatchSyncMessage(connection, decoder, replyEncoder);
}

void OfflineStorageProcess::didClose(CoreIPC::Connection*)
{
    // The UIProcess just crashed.
    RunLoop::current()->stop();
}

void OfflineStorageProcess::didReceiveInvalidMessage(CoreIPC::Connection*, CoreIPC::StringReference, CoreIPC::StringReference)
{
    RunLoop::current()->stop();
}

void OfflineStorageProcess::initializeOfflineStorageProcess(const OfflineStorageProcessCreationParameters&)
{
}

#if !PLATFORM(MAC)
void OfflineStorageProcess::initializeProcess(const ChildProcessInitializationParameters&)
{
}

void OfflineStorageProcess::initializeProcessName(const ChildProcessInitializationParameters&)
{
}

void OfflineStorageProcess::initializeSandbox(const ChildProcessInitializationParameters&, SandboxInitializationParameters&)
{
}
#endif

} // namespace WebKit
