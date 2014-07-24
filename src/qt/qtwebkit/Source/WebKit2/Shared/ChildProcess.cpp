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

#include "config.h"
#include "ChildProcess.h"

#include "SandboxInitializationParameters.h"

#if !OS(WINDOWS)
#include <unistd.h>
#endif

using namespace WebCore;

namespace WebKit {

ChildProcess::ChildProcess()
    : m_terminationTimeout(0)
    , m_terminationCounter(0)
    , m_terminationTimer(RunLoop::main(), this, &ChildProcess::terminationTimerFired)
#if PLATFORM(MAC)
    , m_activeTaskCount(0)
    , m_shouldSuspend(false)
    , m_suspensionHysteresisTimer(RunLoop::main(), this, &ChildProcess::suspensionHysteresisTimerFired)
#endif
{
}

ChildProcess::~ChildProcess()
{
}

NO_RETURN static void watchdogCallback()
{
    // We use _exit here since the watchdog callback is called from another thread and we don't want 
    // global destructors or atexit handlers to be called from this thread while the main thread is busy
    // doing its thing.
    _exit(EXIT_FAILURE);
}

static void didCloseOnConnectionWorkQueue(CoreIPC::Connection*)
{
    // If the connection has been closed and we haven't responded in the main thread for 10 seconds
    // the process will exit forcibly.
    const double watchdogDelay = 10;

    WorkQueue::create("com.apple.WebKit.ChildProcess.WatchDogQueue")->dispatchAfterDelay(bind(static_cast<void(*)()>(watchdogCallback)), watchdogDelay);
}

void ChildProcess::initialize(const ChildProcessInitializationParameters& parameters)
{
    platformInitialize();

    initializeProcess(parameters);
    initializeProcessName(parameters);

    SandboxInitializationParameters sandboxParameters;
    initializeSandbox(parameters, sandboxParameters);
    
    m_connection = CoreIPC::Connection::createClientConnection(parameters.connectionIdentifier, this, RunLoop::main());
    m_connection->setDidCloseOnConnectionWorkQueueCallback(didCloseOnConnectionWorkQueue);
    initializeConnection(m_connection.get());
    m_connection->open();
}

void ChildProcess::initializeProcess(const ChildProcessInitializationParameters&)
{
}

void ChildProcess::initializeProcessName(const ChildProcessInitializationParameters&)
{
}

void ChildProcess::initializeConnection(CoreIPC::Connection*)
{
}

void ChildProcess::addMessageReceiver(CoreIPC::StringReference messageReceiverName, CoreIPC::MessageReceiver* messageReceiver)
{
    m_messageReceiverMap.addMessageReceiver(messageReceiverName, messageReceiver);
}

void ChildProcess::addMessageReceiver(CoreIPC::StringReference messageReceiverName, uint64_t destinationID, CoreIPC::MessageReceiver* messageReceiver)
{
    m_messageReceiverMap.addMessageReceiver(messageReceiverName, destinationID, messageReceiver);
}

void ChildProcess::removeMessageReceiver(CoreIPC::StringReference messageReceiverName, uint64_t destinationID)
{
    m_messageReceiverMap.removeMessageReceiver(messageReceiverName, destinationID);
}

void ChildProcess::disableTermination()
{
    m_terminationCounter++;
    m_terminationTimer.stop();
}

void ChildProcess::enableTermination()
{
    ASSERT(m_terminationCounter > 0);
    m_terminationCounter--;

    if (m_terminationCounter)
        return;

    if (!m_terminationTimeout) {
        terminationTimerFired();
        return;
    }

    m_terminationTimer.startOneShot(m_terminationTimeout);
}

CoreIPC::Connection* ChildProcess::messageSenderConnection()
{
    return m_connection.get();
}

uint64_t ChildProcess::messageSenderDestinationID()
{
    return 0;
}

void ChildProcess::terminationTimerFired()
{
    if (!shouldTerminate())
        return;

    terminate();
}

void ChildProcess::terminate()
{
    m_connection->invalidate();

    RunLoop::main()->stop();
}

#if !PLATFORM(MAC)
void ChildProcess::platformInitialize()
{
}

void ChildProcess::initializeSandbox(const ChildProcessInitializationParameters&, SandboxInitializationParameters&)
{
}
#endif

} // namespace WebKit
