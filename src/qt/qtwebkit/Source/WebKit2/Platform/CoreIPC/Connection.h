/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
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

#ifndef Connection_h
#define Connection_h

#include "Arguments.h"
#include "MessageDecoder.h"
#include "MessageEncoder.h"
#include "MessageReceiver.h"
#include "WorkQueue.h"
#include <wtf/Deque.h>
#include <wtf/Forward.h>
#include <wtf/PassRefPtr.h>
#include <wtf/OwnPtr.h>
#include <wtf/Threading.h>
#include <wtf/text/CString.h>

#if OS(DARWIN)
#include <mach/mach_port.h>
#if HAVE(XPC)
#include <xpc/xpc.h>
#endif
#elif PLATFORM(QT)
QT_BEGIN_NAMESPACE
class QSocketNotifier;
QT_END_NAMESPACE
#endif

#if PLATFORM(QT) || PLATFORM(GTK) || PLATFORM(EFL)
#include "PlatformProcessIdentifier.h"
#endif

namespace WebCore {
class RunLoop;
}

namespace CoreIPC {

enum MessageSendFlags {
    // Whether this message should be dispatched when waiting for a sync reply.
    // This is the default for synchronous messages.
    DispatchMessageEvenWhenWaitingForSyncReply = 1 << 0,
};

enum SyncMessageSendFlags {
    // Will allow events to continue being handled while waiting for the sync reply.
    SpinRunLoopWhileWaitingForReply = 1 << 0,
};
    
#define MESSAGE_CHECK_BASE(assertion, connection) do \
    if (!(assertion)) { \
        ASSERT(assertion); \
        (connection)->markCurrentlyDispatchedMessageAsInvalid(); \
        return; \
    } \
while (0)

class Connection : public ThreadSafeRefCounted<Connection> {
public:
    class Client : public MessageReceiver {
    public:
        virtual void didClose(Connection*) = 0;
        virtual void didReceiveInvalidMessage(Connection*, StringReference messageReceiverName, StringReference messageName) = 0;

    protected:
        virtual ~Client() { }
    };

    class WorkQueueMessageReceiver : public MessageReceiver, public ThreadSafeRefCounted<WorkQueueMessageReceiver> {
    };

#if OS(DARWIN)
    struct Identifier {
        Identifier()
            : port(MACH_PORT_NULL)
#if HAVE(XPC)
            , xpcConnection(0)
#endif
        {
        }

        Identifier(mach_port_t port)
            : port(port)
#if HAVE(XPC)
            , xpcConnection(0)
#endif
        {
        }

#if HAVE(XPC)
        Identifier(mach_port_t port, xpc_connection_t xpcConnection)
            : port(port)
            , xpcConnection(xpcConnection)
        {
        }
#endif

        mach_port_t port;
#if HAVE(XPC)
        xpc_connection_t xpcConnection;
#endif
    };
    static bool identifierIsNull(Identifier identifier) { return identifier.port == MACH_PORT_NULL; }
#elif OS(WINDOWS)
    typedef HANDLE Identifier;
    static bool createServerAndClientIdentifiers(Identifier& serverIdentifier, Identifier& clientIdentifier);
    static bool identifierIsNull(Identifier identifier) { return !identifier; }
#elif USE(UNIX_DOMAIN_SOCKETS)
    typedef int Identifier;
    static bool identifierIsNull(Identifier identifier) { return !identifier; }
#endif

    static PassRefPtr<Connection> createServerConnection(Identifier, Client*, WebCore::RunLoop* clientRunLoop);
    static PassRefPtr<Connection> createClientConnection(Identifier, Client*, WebCore::RunLoop* clientRunLoop);
    ~Connection();

    Client* client() const { return m_client; }

#if OS(DARWIN)
    void setShouldCloseConnectionOnMachExceptions();
#elif PLATFORM(QT)
    void setShouldCloseConnectionOnProcessTermination(WebKit::PlatformProcessIdentifier);
#endif

    void setOnlySendMessagesAsDispatchWhenWaitingForSyncReplyWhenProcessingSuchAMessage(bool);
    void setShouldExitOnSyncMessageSendFailure(bool shouldExitOnSyncMessageSendFailure);

    // The set callback will be called on the connection work queue when the connection is closed, 
    // before didCall is called on the client thread. Must be called before the connection is opened.
    // In the future we might want a more generic way to handle sync or async messages directly
    // on the work queue, for example if we want to handle them on some other thread we could avoid
    // handling the message on the client thread first.
    typedef void (*DidCloseOnConnectionWorkQueueCallback)(Connection*);
    void setDidCloseOnConnectionWorkQueueCallback(DidCloseOnConnectionWorkQueueCallback callback);

    void addWorkQueueMessageReceiver(StringReference messageReceiverName, WorkQueue*, WorkQueueMessageReceiver*);
    void removeWorkQueueMessageReceiver(StringReference messageReceiverName);

    bool open();
    void invalidate();
    void markCurrentlyDispatchedMessageAsInvalid();

    void postConnectionDidCloseOnConnectionWorkQueue();

    static const int NoTimeout = -1;

    template<typename T> bool send(const T& message, uint64_t destinationID, unsigned messageSendFlags = 0);
    template<typename T> bool sendSync(const T& message, const typename T::Reply& reply, uint64_t destinationID, double timeout = NoTimeout, unsigned syncSendFlags = 0);
    template<typename T> bool waitForAndDispatchImmediately(uint64_t destinationID, double timeout);

    PassOwnPtr<MessageEncoder> createSyncMessageEncoder(StringReference messageReceiverName, StringReference messageName, uint64_t destinationID, uint64_t& syncRequestID);
    bool sendMessage(PassOwnPtr<MessageEncoder>, unsigned messageSendFlags = 0);
    PassOwnPtr<MessageDecoder> sendSyncMessage(uint64_t syncRequestID, PassOwnPtr<MessageEncoder>, double timeout, unsigned syncSendFlags = 0);
    PassOwnPtr<MessageDecoder> sendSyncMessageFromSecondaryThread(uint64_t syncRequestID, PassOwnPtr<MessageEncoder>, double timeout);
    bool sendSyncReply(PassOwnPtr<MessageEncoder>);

    void wakeUpRunLoop();

    void incrementDispatchMessageMarkedDispatchWhenWaitingForSyncReplyCount() { ++m_inDispatchMessageMarkedDispatchWhenWaitingForSyncReplyCount; }
    void decrementDispatchMessageMarkedDispatchWhenWaitingForSyncReplyCount() { --m_inDispatchMessageMarkedDispatchWhenWaitingForSyncReplyCount; }

    bool inSendSync() const { return m_inSendSyncCount; }

private:
    Connection(Identifier, bool isServer, Client*, WebCore::RunLoop* clientRunLoop);
    void platformInitialize(Identifier);
    void platformInvalidate();
    
    bool isValid() const { return m_client; }
    
    PassOwnPtr<MessageDecoder> waitForMessage(StringReference messageReceiverName, StringReference messageName, uint64_t destinationID, double timeout);
    
    PassOwnPtr<MessageDecoder> waitForSyncReply(uint64_t syncRequestID, double timeout, unsigned syncSendFlags);

    // Called on the connection work queue.
    void processIncomingMessage(PassOwnPtr<MessageDecoder>);
    void processIncomingSyncReply(PassOwnPtr<MessageDecoder>);

    void addWorkQueueMessageReceiverOnConnectionWorkQueue(StringReference messageReceiverName, WorkQueue*, WorkQueueMessageReceiver*);
    void removeWorkQueueMessageReceiverOnConnectionWorkQueue(StringReference messageReceiverName);
    void dispatchWorkQueueMessageReceiverMessage(WorkQueueMessageReceiver*, MessageDecoder*);

    bool canSendOutgoingMessages() const;
    bool platformCanSendOutgoingMessages() const;
    void sendOutgoingMessages();
    bool sendOutgoingMessage(PassOwnPtr<MessageEncoder>);
    void connectionDidClose();
    
    // Called on the listener thread.
    void dispatchConnectionDidClose();
    void dispatchOneMessage();
    void dispatchMessage(PassOwnPtr<MessageDecoder>);
    void dispatchMessage(MessageDecoder&);
    void dispatchSyncMessage(MessageDecoder&);
    void dispatchDidReceiveInvalidMessage(const CString& messageReceiverNameString, const CString& messageNameString);
    void didFailToSendSyncMessage();

    // Can be called on any thread.
    void enqueueIncomingMessage(PassOwnPtr<MessageDecoder>);

    Client* m_client;
    bool m_isServer;
    uint64_t m_syncRequestID;

    bool m_onlySendMessagesAsDispatchWhenWaitingForSyncReplyWhenProcessingSuchAMessage;
    bool m_shouldExitOnSyncMessageSendFailure;
    DidCloseOnConnectionWorkQueueCallback m_didCloseOnConnectionWorkQueueCallback;

    bool m_isConnected;
    RefPtr<WorkQueue> m_connectionQueue;
    WebCore::RunLoop* m_clientRunLoop;

    HashMap<StringReference, std::pair<RefPtr<WorkQueue>, RefPtr<WorkQueueMessageReceiver> > > m_workQueueMessageReceivers;

    unsigned m_inSendSyncCount;
    unsigned m_inDispatchMessageCount;
    unsigned m_inDispatchMessageMarkedDispatchWhenWaitingForSyncReplyCount;
    bool m_didReceiveInvalidMessage;

    // Incoming messages.
    Mutex m_incomingMessagesLock;
    Deque<OwnPtr<MessageDecoder> > m_incomingMessages;

    // Outgoing messages.
    Mutex m_outgoingMessagesLock;
    Deque<OwnPtr<MessageEncoder> > m_outgoingMessages;
    
    ThreadCondition m_waitForMessageCondition;
    Mutex m_waitForMessageMutex;
    HashMap<std::pair<std::pair<StringReference, StringReference>, uint64_t>, OwnPtr<MessageDecoder> > m_waitForMessageMap;

#if !HAVE(ATOMICS_64BIT)
    Mutex m_syncRequestLock;
#endif

    // Represents a sync request for which we're waiting on a reply.
    struct PendingSyncReply {
        // The request ID.
        uint64_t syncRequestID;

        // The reply decoder, will be null if there was an error processing the sync
        // message on the other side.
        MessageDecoder* replyDecoder;

        // Will be set to true once a reply has been received.
        bool didReceiveReply;
    
        PendingSyncReply()
            : syncRequestID(0)
            , replyDecoder(0)
            , didReceiveReply(false)
        {
        }

        explicit PendingSyncReply(uint64_t syncRequestID)
            : syncRequestID(syncRequestID)
            , replyDecoder(0)
            , didReceiveReply(0)
        {
        }

        PassOwnPtr<MessageDecoder> releaseReplyDecoder()
        {
            OwnPtr<MessageDecoder> reply = adoptPtr(replyDecoder);
            replyDecoder = 0;
            
            return reply.release();
        }
    };

    class SyncMessageState;
    friend class SyncMessageState;
    RefPtr<SyncMessageState> m_syncMessageState;

    Mutex m_syncReplyStateMutex;
    bool m_shouldWaitForSyncReplies;
    Vector<PendingSyncReply> m_pendingSyncReplies;

    class SecondaryThreadPendingSyncReply;
    typedef HashMap<uint64_t, SecondaryThreadPendingSyncReply*> SecondaryThreadPendingSyncReplyMap;
    SecondaryThreadPendingSyncReplyMap m_secondaryThreadPendingSyncReplyMap;

#if OS(DARWIN)
    // Called on the connection queue.
    void receiveSourceEventHandler();
    void initializeDeadNameSource();
    void exceptionSourceEventHandler();

    mach_port_t m_sendPort;
    dispatch_source_t m_deadNameSource;

    mach_port_t m_receivePort;
    dispatch_source_t m_receivePortDataAvailableSource;

    // If setShouldCloseConnectionOnMachExceptions has been called, this has
    // the exception port that exceptions from the other end will be sent on.
    mach_port_t m_exceptionPort;
    dispatch_source_t m_exceptionPortDataAvailableSource;

#if HAVE(XPC)
    xpc_connection_t m_xpcConnection;
#endif

#elif OS(WINDOWS)
    // Called on the connection queue.
    void readEventHandler();
    void writeEventHandler();

    // Called by Connection::SyncMessageState::waitWhileDispatchingSentWin32Messages.
    // The absoluteTime is in seconds, starting on January 1, 1970. The time is assumed to use the
    // same time zone as WTF::currentTime(). Dispatches sent (not posted) messages to the passed-in
    // set of HWNDs until the semaphore is signaled or absoluteTime is reached. Returns true if the
    // semaphore is signaled, false otherwise.
    static bool dispatchSentMessagesUntil(const Vector<HWND>& windows, WTF::BinarySemaphore& semaphore, double absoluteTime);

    Vector<uint8_t> m_readBuffer;
    OVERLAPPED m_readState;
    OwnPtr<MessageEncoder> m_pendingWriteEncoder;
    OVERLAPPED m_writeState;
    HANDLE m_connectionPipe;
#elif USE(UNIX_DOMAIN_SOCKETS)
    // Called on the connection queue.
    void readyReadHandler();
    bool processMessage();

    Vector<uint8_t> m_readBuffer;
    size_t m_readBufferSize;
    Vector<int> m_fileDescriptors;
    size_t m_fileDescriptorsSize;
    int m_socketDescriptor;
#if PLATFORM(QT)
    QSocketNotifier* m_socketNotifier;
#endif
#endif
};

template<typename T> bool Connection::send(const T& message, uint64_t destinationID, unsigned messageSendFlags)
{
    COMPILE_ASSERT(!T::isSync, AsyncMessageExpected);

    OwnPtr<MessageEncoder> encoder = MessageEncoder::create(T::receiverName(), T::name(), destinationID);
    encoder->encode(message);
    
    return sendMessage(encoder.release(), messageSendFlags);
}

template<typename T> bool Connection::sendSync(const T& message, const typename T::Reply& reply, uint64_t destinationID, double timeout, unsigned syncSendFlags)
{
    COMPILE_ASSERT(T::isSync, SyncMessageExpected);

    uint64_t syncRequestID = 0;
    OwnPtr<MessageEncoder> encoder = createSyncMessageEncoder(T::receiverName(), T::name(), destinationID, syncRequestID);
    
    // Encode the rest of the input arguments.
    encoder->encode(message);

    // Now send the message and wait for a reply.
    OwnPtr<MessageDecoder> replyDecoder = sendSyncMessage(syncRequestID, encoder.release(), timeout, syncSendFlags);
    if (!replyDecoder)
        return false;

    // Decode the reply.
    return replyDecoder->decode(const_cast<typename T::Reply&>(reply));
}

template<typename T> bool Connection::waitForAndDispatchImmediately(uint64_t destinationID, double timeout)
{
    OwnPtr<MessageDecoder> decoder = waitForMessage(T::receiverName(), T::name(), destinationID, timeout);
    if (!decoder)
        return false;

    ASSERT(decoder->destinationID() == destinationID);
    m_client->didReceiveMessage(this, *decoder);
    return true;
}

} // namespace CoreIPC

#endif // Connection_h
