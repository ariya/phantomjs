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
#include "Connection.h"

#include "DataReference.h"
#include <wtf/Functional.h>
#include <wtf/RandomNumber.h>
#include <wtf/text/WTFString.h>
#include <wtf/threads/BinarySemaphore.h>

namespace CoreIPC {

// FIXME: Rename this or use a different constant on windows.
static const size_t inlineMessageMaxSize = 4096;

bool Connection::createServerAndClientIdentifiers(HANDLE& serverIdentifier, HANDLE& clientIdentifier)
{
    String pipeName;

    while (true) {
        unsigned uniqueID = randomNumber() * std::numeric_limits<unsigned>::max();
        pipeName = String::format("\\\\.\\pipe\\com.apple.WebKit.%x", uniqueID);

        serverIdentifier = ::CreateNamedPipe(pipeName.charactersWithNullTermination().data(),
                                             PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE | FILE_FLAG_OVERLAPPED,
                                             PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 1, inlineMessageMaxSize, inlineMessageMaxSize,
                                             0, 0);
        if (!serverIdentifier && ::GetLastError() == ERROR_PIPE_BUSY) {
            // There was already a pipe with this name, try again.
            continue;
        }

        break;
    }

    if (!serverIdentifier)
        return false;

    clientIdentifier = ::CreateFileW(pipeName.charactersWithNullTermination().data(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
    if (!clientIdentifier) {
        ::CloseHandle(serverIdentifier);
        return false;
    }

    DWORD mode = PIPE_READMODE_MESSAGE;
    if (!::SetNamedPipeHandleState(clientIdentifier, &mode, 0, 0)) {
        ::CloseHandle(serverIdentifier);
        ::CloseHandle(clientIdentifier);
        return false;
    }

    return true;
}

void Connection::platformInitialize(Identifier identifier)
{
    memset(&m_readState, 0, sizeof(m_readState));
    m_readState.hEvent = ::CreateEventW(0, FALSE, FALSE, 0);

    memset(&m_writeState, 0, sizeof(m_writeState));
    m_writeState.hEvent = ::CreateEventW(0, FALSE, FALSE, 0);

    m_connectionPipe = identifier;
}

void Connection::platformInvalidate()
{
    if (m_connectionPipe == INVALID_HANDLE_VALUE)
        return;

    m_isConnected = false;

    m_connectionQueue->unregisterAndCloseHandle(m_readState.hEvent);
    m_readState.hEvent = 0;

    m_connectionQueue->unregisterAndCloseHandle(m_writeState.hEvent);
    m_writeState.hEvent = 0;

    ::CloseHandle(m_connectionPipe);
    m_connectionPipe = INVALID_HANDLE_VALUE;
}

void Connection::readEventHandler()
{
    if (m_connectionPipe == INVALID_HANDLE_VALUE)
        return;

    while (true) {
        // Check if we got some data.
        DWORD numberOfBytesRead = 0;
        if (!::GetOverlappedResult(m_connectionPipe, &m_readState, &numberOfBytesRead, FALSE)) {
            DWORD error = ::GetLastError();

            switch (error) {
            case ERROR_BROKEN_PIPE:
                connectionDidClose();
                return;
            case ERROR_MORE_DATA: {
                // Read the rest of the message out of the pipe.

                DWORD bytesToRead = 0;
                if (!::PeekNamedPipe(m_connectionPipe, 0, 0, 0, 0, &bytesToRead)) {
                    DWORD error = ::GetLastError();
                    if (error == ERROR_BROKEN_PIPE) {
                        connectionDidClose();
                        return;
                    }
                    ASSERT_NOT_REACHED();
                    return;
                }

                // ::GetOverlappedResult told us there's more data. ::PeekNamedPipe shouldn't
                // contradict it!
                ASSERT(bytesToRead);
                if (!bytesToRead)
                    break;

                m_readBuffer.grow(m_readBuffer.size() + bytesToRead);
                if (!::ReadFile(m_connectionPipe, m_readBuffer.data() + numberOfBytesRead, bytesToRead, 0, &m_readState)) {
                    DWORD error = ::GetLastError();
                    ASSERT_NOT_REACHED();
                    return;
                }
                continue;
            }

            // FIXME: We should figure out why we're getting this error.
            case ERROR_IO_INCOMPLETE:
                return;
            default:
                ASSERT_NOT_REACHED();
            }
        }

        if (!m_readBuffer.isEmpty()) {
            // We have a message, let's dispatch it.

            OwnPtr<MessageDecoder> decoder = MessageDecoder::create(DataReference(m_readBuffer.data(), m_readBuffer.size()));
            processIncomingMessage(decoder.release());
        }

        // Find out the size of the next message in the pipe (if there is one) so that we can read
        // it all in one operation. (This is just an optimization to avoid an extra pass through the
        // loop (if we chose a buffer size that was too small) or allocating extra memory (if we
        // chose a buffer size that was too large).)
        DWORD bytesToRead = 0;
        if (!::PeekNamedPipe(m_connectionPipe, 0, 0, 0, 0, &bytesToRead)) {
            DWORD error = ::GetLastError();
            if (error == ERROR_BROKEN_PIPE) {
                connectionDidClose();
                return;
            }
            ASSERT_NOT_REACHED();
        }
        if (!bytesToRead) {
            // There's no message waiting in the pipe. Schedule a read of the first byte of the
            // next message. We'll find out the message's actual size when it arrives. (If we
            // change this to read more than a single byte for performance reasons, we'll have to
            // deal with m_readBuffer potentially being larger than the message we read after
            // calling ::GetOverlappedResult above.)
            bytesToRead = 1;
        }

        m_readBuffer.resize(bytesToRead);

        // Either read the next available message (which should occur synchronously), or start an
        // asynchronous read of the next message that becomes available.
        BOOL result = ::ReadFile(m_connectionPipe, m_readBuffer.data(), m_readBuffer.size(), 0, &m_readState);
        if (result) {
            // There was already a message waiting in the pipe, and we read it synchronously.
            // Process it.
            continue;
        }

        DWORD error = ::GetLastError();

        if (error == ERROR_IO_PENDING) {
            // There are no messages in the pipe currently. readEventHandler will be called again once there is a message.
            return;
        }

        if (error == ERROR_MORE_DATA) {
            // Either a message is available when we didn't think one was, or the message is larger
            // than ::PeekNamedPipe told us. The former seems far more likely. Probably the message
            // became available between our calls to ::PeekNamedPipe and ::ReadFile above. Go back
            // to the top of the loop to use ::GetOverlappedResult to retrieve the available data.
            continue;
        }

        // FIXME: We need to handle other errors here.
        ASSERT_NOT_REACHED();
    }
}

void Connection::writeEventHandler()
{
    if (m_connectionPipe == INVALID_HANDLE_VALUE)
        return;

    DWORD numberOfBytesWritten = 0;
    if (!::GetOverlappedResult(m_connectionPipe, &m_writeState, &numberOfBytesWritten, FALSE)) {
        DWORD error = ::GetLastError();
        if (error == ERROR_IO_INCOMPLETE) {
            // FIXME: We should figure out why we're getting this error.
            return;
        }
        if (error == ERROR_BROKEN_PIPE) {
            connectionDidClose();
            return;
        }
        ASSERT_NOT_REACHED();
    }

    // The pending write has finished, so we are now done with its encoder. Clearing this member
    // will allow us to send messages again.
    m_pendingWriteEncoder = nullptr;

    // Now that the pending write has finished, we can try to send a new message.
    sendOutgoingMessages();
}

bool Connection::open()
{
    // We connected the two ends of the pipe in createServerAndClientIdentifiers.
    m_isConnected = true;

    // Start listening for read and write state events.
    m_connectionQueue->registerHandle(m_readState.hEvent, bind(&Connection::readEventHandler, this));
    m_connectionQueue->registerHandle(m_writeState.hEvent, bind(&Connection::writeEventHandler, this));

    // Schedule a read.
    m_connectionQueue->dispatch(bind(&Connection::readEventHandler, this));

    return true;
}

bool Connection::platformCanSendOutgoingMessages() const
{
    // We only allow sending one asynchronous message at a time. If we wanted to send more than one
    // at once, we'd have to use multiple OVERLAPPED structures and hold onto multiple pending
    // MessageEncoders (one of each for each simultaneous asynchronous message).
    return !m_pendingWriteEncoder;
}

bool Connection::sendOutgoingMessage(PassOwnPtr<MessageEncoder> encoder)
{
    ASSERT(!m_pendingWriteEncoder);

    // Just bail if the handle has been closed.
    if (m_connectionPipe == INVALID_HANDLE_VALUE)
        return false;

    // We put the message ID last.
    *encoder << 0;

    // Write the outgoing message.

    if (::WriteFile(m_connectionPipe, encoder->buffer(), encoder->bufferSize(), 0, &m_writeState)) {
        // We successfully sent this message.
        return true;
    }

    DWORD error = ::GetLastError();

    if (error == ERROR_NO_DATA) {
        // The pipe is being closed.
        connectionDidClose();
        return false;
    }

    if (error != ERROR_IO_PENDING) {
        ASSERT_NOT_REACHED();
        return false;
    }

    // The message will be sent soon. Hold onto the encoder so that it won't be destroyed
    // before the write completes.
    m_pendingWriteEncoder = encoder;

    // We can only send one asynchronous message at a time (see comment in platformCanSendOutgoingMessages).
    return false;
}

bool Connection::dispatchSentMessagesUntil(const Vector<HWND>& windows, WTF::BinarySemaphore& semaphore, double absoluteTime)
{
    if (windows.isEmpty())
        return semaphore.wait(absoluteTime);

    HANDLE handle = semaphore.event();
    DWORD handleCount = 1;

    while (true) {
        DWORD interval = absoluteTimeToWaitTimeoutInterval(absoluteTime);
        if (!interval) {
            // Consider the wait to have timed out, even if the semaphore is currently signaled.
            // This matches the WTF::ThreadCondition implementation of BinarySemaphore::wait.
            return false;
        }

        DWORD result = ::MsgWaitForMultipleObjectsEx(handleCount, &handle, interval, QS_SENDMESSAGE, 0);
        if (result == WAIT_OBJECT_0) {
            // The semaphore was signaled.
            return true;
        }
        if (result == WAIT_TIMEOUT) {
            // absoluteTime was reached.
            return false;
        }
        if (result == WAIT_OBJECT_0 + handleCount) {
            // One or more sent messages are available. Process sent messages for all the windows
            // we were given, since we don't have a way of knowing which window has available sent
            // messages.
            for (size_t i = 0; i < windows.size(); ++i) {
                MSG message;
                ::PeekMessageW(&message, windows[i], 0, 0, PM_NOREMOVE | PM_QS_SENDMESSAGE);
            }
            continue;
        }
        ASSERT_WITH_MESSAGE(result != WAIT_FAILED, "::MsgWaitForMultipleObjectsEx failed with error %lu", ::GetLastError());
        ASSERT_WITH_MESSAGE(false, "::MsgWaitForMultipleObjectsEx returned unexpected result %lu", result);
        return false;
    }
}

} // namespace CoreIPC
