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
#include "ImportanceAssertion.h"
#include "MachPort.h"
#include "MachUtilities.h"
#include <WebCore/RunLoop.h>
#include <mach/mach_error.h>
#include <mach/vm_map.h>

#if HAVE(XPC)
#include <xpc/xpc.h>
#endif

using namespace WebCore;

namespace CoreIPC {

static const size_t inlineMessageMaxSize = 4096;

// Message flags.
enum {
    MessageBodyIsOutOfLine = 1 << 0
};
    
void Connection::platformInvalidate()
{
    if (!m_isConnected)
        return;

    m_isConnected = false;

    ASSERT(m_sendPort);
    ASSERT(m_receivePort);

    // Unregister our ports.
    dispatch_source_cancel(m_deadNameSource);
    dispatch_release(m_deadNameSource);
    m_deadNameSource = 0;
    m_sendPort = MACH_PORT_NULL;

    dispatch_source_cancel(m_receivePortDataAvailableSource);
    dispatch_release(m_receivePortDataAvailableSource);
    m_receivePortDataAvailableSource = 0;
    m_receivePort = MACH_PORT_NULL;

    if (m_exceptionPort) {
        dispatch_source_cancel(m_exceptionPortDataAvailableSource);
        dispatch_release(m_exceptionPortDataAvailableSource);
        m_exceptionPortDataAvailableSource = 0;
        m_exceptionPort = MACH_PORT_NULL;
    }

#if HAVE(XPC)
    if (m_xpcConnection) {
        xpc_release(m_xpcConnection);
        m_xpcConnection = 0;
    }
#endif
}

void Connection::platformInitialize(Identifier identifier)
{
    m_exceptionPort = MACH_PORT_NULL;

    if (m_isServer) {
        m_receivePort = identifier.port;
        m_sendPort = MACH_PORT_NULL;
    } else {
        m_receivePort = MACH_PORT_NULL;
        m_sendPort = identifier.port;
    }

    m_deadNameSource = 0;
    m_receivePortDataAvailableSource = 0;
    m_exceptionPortDataAvailableSource = 0;

#if HAVE(XPC)
    m_xpcConnection = identifier.xpcConnection;
    // FIXME: Instead of explicitly retaining the connection here, Identifier::xpcConnection
    // should just be a smart pointer.
    if (m_xpcConnection)
        xpc_retain(m_xpcConnection);
#endif
}

static dispatch_source_t createDataAvailableSource(mach_port_t receivePort, WorkQueue* workQueue, const Function<void()>& function)
{
    dispatch_source_t source = dispatch_source_create(DISPATCH_SOURCE_TYPE_MACH_RECV, receivePort, 0, workQueue->dispatchQueue());
#if COMPILER(GCC) && !COMPILER(CLANG)
    Function<void()> functionCopy = function;
    dispatch_source_set_event_handler(source, ^{
        functionCopy();
    });
#else
    dispatch_source_set_event_handler(source, function);
#endif
    dispatch_source_set_cancel_handler(source, ^{
        mach_port_mod_refs(mach_task_self(), receivePort, MACH_PORT_RIGHT_RECEIVE, -1);
    });

    return source;
}

bool Connection::open()
{
    if (m_isServer) {
        ASSERT(m_receivePort);
        ASSERT(!m_sendPort);
        
    } else {
        ASSERT(!m_receivePort);
        ASSERT(m_sendPort);

        // Create the receive port.
        mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &m_receivePort);

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
        mach_port_set_attributes(mach_task_self(), m_receivePort, MACH_PORT_IMPORTANCE_RECEIVER, (mach_port_info_t)0, 0);
#endif

        m_isConnected = true;
        
        // Send the initialize message, which contains a send right for the server to use.
        OwnPtr<MessageEncoder> encoder = MessageEncoder::create("IPC", "InitializeConnection", 0);
        encoder->encode(MachPort(m_receivePort, MACH_MSG_TYPE_MAKE_SEND));

        sendMessage(encoder.release());

        initializeDeadNameSource();
    }

    // Change the message queue length for the receive port.
    setMachPortQueueLength(m_receivePort, MACH_PORT_QLIMIT_LARGE);

    // Register the data available handler.
    m_receivePortDataAvailableSource = createDataAvailableSource(m_receivePort, m_connectionQueue.get(), bind(&Connection::receiveSourceEventHandler, this));

    // If we have an exception port, register the data available handler and send over the port to the other end.
    if (m_exceptionPort) {
        m_exceptionPortDataAvailableSource = createDataAvailableSource(m_exceptionPort, m_connectionQueue.get(), bind(&Connection::exceptionSourceEventHandler, this));

        OwnPtr<MessageEncoder> encoder = MessageEncoder::create("IPC", "SetExceptionPort", 0);
        encoder->encode(MachPort(m_exceptionPort, MACH_MSG_TYPE_MAKE_SEND));

        sendMessage(encoder.release());
    }

    ref();
    dispatch_async(m_connectionQueue->dispatchQueue(), ^{
        dispatch_resume(m_receivePortDataAvailableSource);

        if (m_deadNameSource)
            dispatch_resume(m_deadNameSource);
        if (m_exceptionPortDataAvailableSource)
            dispatch_resume(m_exceptionPortDataAvailableSource);

        deref();
    });

    return true;
}

static inline size_t machMessageSize(size_t bodySize, size_t numberOfPortDescriptors = 0, size_t numberOfOOLMemoryDescriptors = 0)
{
    size_t size = sizeof(mach_msg_header_t) + bodySize;
    if (numberOfPortDescriptors || numberOfOOLMemoryDescriptors) {
        size += sizeof(mach_msg_body_t);
        if (numberOfPortDescriptors)
            size += (numberOfPortDescriptors * sizeof(mach_msg_port_descriptor_t));
        if (numberOfOOLMemoryDescriptors)
            size += (numberOfOOLMemoryDescriptors * sizeof(mach_msg_ool_ports_descriptor_t));
    }
    return round_msg(size);
}

bool Connection::platformCanSendOutgoingMessages() const
{
    return true;
}

bool Connection::sendOutgoingMessage(PassOwnPtr<MessageEncoder> encoder)
{
    Vector<Attachment> attachments = encoder->releaseAttachments();
    
    size_t numberOfPortDescriptors = 0;
    size_t numberOfOOLMemoryDescriptors = 0;
    for (size_t i = 0; i < attachments.size(); ++i) {
        Attachment::Type type = attachments[i].type();
        if (type == Attachment::MachPortType)
            numberOfPortDescriptors++;
        else if (type == Attachment::MachOOLMemoryType)
            numberOfOOLMemoryDescriptors++;
    }
    
    size_t messageSize = machMessageSize(encoder->bufferSize(), numberOfPortDescriptors, numberOfOOLMemoryDescriptors);
    char buffer[inlineMessageMaxSize];

    bool messageBodyIsOOL = false;
    if (messageSize > sizeof(buffer)) {
        messageBodyIsOOL = true;

        attachments.append(Attachment(encoder->buffer(), encoder->bufferSize(), MACH_MSG_VIRTUAL_COPY, false));
        numberOfOOLMemoryDescriptors++;
        messageSize = machMessageSize(0, numberOfPortDescriptors, numberOfOOLMemoryDescriptors);
    }

    bool isComplex = (numberOfPortDescriptors + numberOfOOLMemoryDescriptors > 0);

    mach_msg_header_t* header = reinterpret_cast<mach_msg_header_t*>(&buffer);
    header->msgh_bits = isComplex ? MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND | MACH_MSGH_BITS_COMPLEX, 0) : MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0);
    header->msgh_size = messageSize;
    header->msgh_remote_port = m_sendPort;
    header->msgh_local_port = MACH_PORT_NULL;
    header->msgh_id = 0;
    if (messageBodyIsOOL)
        header->msgh_id |= MessageBodyIsOutOfLine;

    uint8_t* messageData;

    if (isComplex) {
        mach_msg_body_t* body = reinterpret_cast<mach_msg_body_t*>(header + 1);
        body->msgh_descriptor_count = numberOfPortDescriptors + numberOfOOLMemoryDescriptors;

        uint8_t* descriptorData = reinterpret_cast<uint8_t*>(body + 1);
        for (size_t i = 0; i < attachments.size(); ++i) {
            Attachment attachment = attachments[i];

            mach_msg_descriptor_t* descriptor = reinterpret_cast<mach_msg_descriptor_t*>(descriptorData);
            switch (attachment.type()) {
            case Attachment::MachPortType:
                descriptor->port.name = attachment.port();
                descriptor->port.disposition = attachment.disposition();
                descriptor->port.type = MACH_MSG_PORT_DESCRIPTOR;            

                descriptorData += sizeof(mach_msg_port_descriptor_t);
                break;
            case Attachment::MachOOLMemoryType:
                descriptor->out_of_line.address = attachment.address();
                descriptor->out_of_line.size = attachment.size();
                descriptor->out_of_line.copy = attachment.copyOptions();
                descriptor->out_of_line.deallocate = attachment.deallocate();
                descriptor->out_of_line.type = MACH_MSG_OOL_DESCRIPTOR;            

                descriptorData += sizeof(mach_msg_ool_descriptor_t);
                break;
            default:
                ASSERT_NOT_REACHED();
            }
        }

        messageData = descriptorData;
    } else
        messageData = (uint8_t*)(header + 1);

    // Copy the data if it is not being sent out-of-line.
    if (!messageBodyIsOOL)
        memcpy(messageData, encoder->buffer(), encoder->bufferSize());

    ASSERT(m_sendPort);

    // Send the message.
    kern_return_t kr = mach_msg(header, MACH_SEND_MSG, messageSize, 0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
    if (kr != KERN_SUCCESS) {
        // FIXME: What should we do here?
    }

    return true;
}

void Connection::initializeDeadNameSource()
{
    m_deadNameSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_MACH_SEND, m_sendPort, 0, m_connectionQueue->dispatchQueue());
#if COMPILER(GCC) && !COMPILER(CLANG)
    dispatch_source_set_event_handler(m_deadNameSource, ^{
        connectionDidClose();
    });
#else
    dispatch_source_set_event_handler(m_deadNameSource, bind(&Connection::connectionDidClose, this));
#endif

    mach_port_t sendPort = m_sendPort;
    dispatch_source_set_cancel_handler(m_deadNameSource, ^{
        // Release our send right.
        mach_port_deallocate(mach_task_self(), sendPort);
    });
}

static PassOwnPtr<MessageDecoder> createMessageDecoder(mach_msg_header_t* header)
{
    if (!(header->msgh_bits & MACH_MSGH_BITS_COMPLEX)) {
        // We have a simple message.
        uint8_t* body = reinterpret_cast<uint8_t*>(header + 1);
        size_t bodySize = header->msgh_size - sizeof(mach_msg_header_t);

        return MessageDecoder::create(DataReference(body, bodySize));
    }

    bool messageBodyIsOOL = header->msgh_id & MessageBodyIsOutOfLine;

    mach_msg_body_t* body = reinterpret_cast<mach_msg_body_t*>(header + 1);
    mach_msg_size_t numDescriptors = body->msgh_descriptor_count;
    ASSERT(numDescriptors);

    uint8_t* descriptorData = reinterpret_cast<uint8_t*>(body + 1);

    // If the message body was sent out-of-line, don't treat the last descriptor
    // as an attachment, since it is really the message body.
    if (messageBodyIsOOL)
        --numDescriptors;

    // Build attachment list
    Vector<Attachment> attachments(numDescriptors);

    for (mach_msg_size_t i = 0; i < numDescriptors; ++i) {
        mach_msg_descriptor_t* descriptor = reinterpret_cast<mach_msg_descriptor_t*>(descriptorData);

        switch (descriptor->type.type) {
        case MACH_MSG_PORT_DESCRIPTOR:
            attachments[numDescriptors - i - 1] = Attachment(descriptor->port.name, descriptor->port.disposition);
            descriptorData += sizeof(mach_msg_port_descriptor_t);
            break;
        case MACH_MSG_OOL_DESCRIPTOR:
            attachments[numDescriptors - i - 1] = Attachment(descriptor->out_of_line.address, descriptor->out_of_line.size, descriptor->out_of_line.copy, descriptor->out_of_line.deallocate);
            descriptorData += sizeof(mach_msg_ool_descriptor_t);
            break;
        default:
            ASSERT(false && "Unhandled descriptor type");
        }
    }

    if (messageBodyIsOOL) {
        mach_msg_descriptor_t* descriptor = reinterpret_cast<mach_msg_descriptor_t*>(descriptorData);
        ASSERT(descriptor->type.type == MACH_MSG_OOL_DESCRIPTOR);
        Attachment messageBodyAttachment(descriptor->out_of_line.address, descriptor->out_of_line.size,
                                         descriptor->out_of_line.copy, descriptor->out_of_line.deallocate);

        uint8_t* messageBody = static_cast<uint8_t*>(messageBodyAttachment.address());
        size_t messageBodySize = messageBodyAttachment.size();

        OwnPtr<MessageDecoder> decoder;

        if (attachments.isEmpty())
            decoder = MessageDecoder::create(DataReference(messageBody, messageBodySize));
        else
            decoder = MessageDecoder::create(DataReference(messageBody, messageBodySize), attachments);

        vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(messageBodyAttachment.address()), messageBodyAttachment.size());

        return decoder.release();
    }

    uint8_t* messageBody = descriptorData;
    size_t messageBodySize = header->msgh_size - (descriptorData - reinterpret_cast<uint8_t*>(header));

    return MessageDecoder::create(DataReference(messageBody, messageBodySize), attachments);
}

// The receive buffer size should always include the maximum trailer size.
static const size_t receiveBufferSize = inlineMessageMaxSize + MAX_TRAILER_SIZE;
typedef Vector<char, receiveBufferSize> ReceiveBuffer;

static mach_msg_header_t* readFromMachPort(mach_port_t machPort, ReceiveBuffer& buffer)
{
    buffer.resize(receiveBufferSize);

    mach_msg_header_t* header = reinterpret_cast<mach_msg_header_t*>(buffer.data());
    kern_return_t kr = mach_msg(header, MACH_RCV_MSG | MACH_RCV_LARGE | MACH_RCV_TIMEOUT, 0, buffer.size(), machPort, 0, MACH_PORT_NULL);
    if (kr == MACH_RCV_TIMED_OUT)
        return 0;

    if (kr == MACH_RCV_TOO_LARGE) {
        // The message was too large, resize the buffer and try again.
        buffer.resize(header->msgh_size + MAX_TRAILER_SIZE);
        header = reinterpret_cast<mach_msg_header_t*>(buffer.data());
        
        kr = mach_msg(header, MACH_RCV_MSG | MACH_RCV_LARGE | MACH_RCV_TIMEOUT, 0, buffer.size(), machPort, 0, MACH_PORT_NULL);
        ASSERT(kr != MACH_RCV_TOO_LARGE);
    }

    if (kr != MACH_MSG_SUCCESS) {
        ASSERT_NOT_REACHED();
        return 0;
    }

    return header;
}

void Connection::receiveSourceEventHandler()
{
    ReceiveBuffer buffer;

    mach_msg_header_t* header = readFromMachPort(m_receivePort, buffer);
    if (!header)
        return;

    OwnPtr<MessageDecoder> decoder = createMessageDecoder(header);
    ASSERT(decoder);

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    decoder->setImportanceAssertion(ImportanceAssertion::create(header));
#endif

    if (decoder->messageReceiverName() == "IPC" && decoder->messageName() == "InitializeConnection") {
        ASSERT(m_isServer);
        ASSERT(!m_isConnected);
        ASSERT(!m_sendPort);

        MachPort port;
        if (!decoder->decode(port)) {
            // FIXME: Disconnect.
            return;
        }

        m_sendPort = port.port();
        
        if (m_sendPort) {
            initializeDeadNameSource();
            dispatch_resume(m_deadNameSource);
        }

        m_isConnected = true;

        // Send any pending outgoing messages.
        sendOutgoingMessages();
        
        return;
    }

    if (decoder->messageReceiverName() == "IPC" && decoder->messageName() == "SetExceptionPort") {
        if (m_isServer) {
            // Server connections aren't supposed to have their exception ports overriden. Treat this as an invalid message.
            m_clientRunLoop->dispatch(bind(&Connection::dispatchDidReceiveInvalidMessage, this, decoder->messageReceiverName().toString(), decoder->messageName().toString()));
            return;
        }
        MachPort exceptionPort;
        if (!decoder->decode(exceptionPort))
            return;

        setMachExceptionPort(exceptionPort.port());
        return;
    }

    processIncomingMessage(decoder.release());
}    

void Connection::exceptionSourceEventHandler()
{
    ReceiveBuffer buffer;

    mach_msg_header_t* header = readFromMachPort(m_exceptionPort, buffer);
    if (!header)
        return;

    // We've read the exception message. Now send it on to the real exception port.

    // The remote port should have a send once right.
    ASSERT(MACH_MSGH_BITS_REMOTE(header->msgh_bits) == MACH_MSG_TYPE_MOVE_SEND_ONCE);

    // Now get the real exception port.
    mach_port_t exceptionPort = machExceptionPort();

    // First, get the complex bit from the source message.
    mach_msg_bits_t messageBits = header->msgh_bits & MACH_MSGH_BITS_COMPLEX;
    messageBits |= MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, MACH_MSG_TYPE_MOVE_SEND_ONCE);

    header->msgh_bits = messageBits;
    header->msgh_local_port = header->msgh_remote_port;
    header->msgh_remote_port = exceptionPort;

    // Now send along the message.
    kern_return_t kr = mach_msg(header, MACH_SEND_MSG, header->msgh_size, 0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
    if (kr != KERN_SUCCESS) {
        LOG_ERROR("Failed to send message to real exception port. %s (%x)", mach_error_string(kr), kr);
        ASSERT_NOT_REACHED();
    }

    connectionDidClose();
}

void Connection::setShouldCloseConnectionOnMachExceptions()
{
    ASSERT(m_exceptionPort == MACH_PORT_NULL);

    if (mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &m_exceptionPort) != KERN_SUCCESS)
        ASSERT_NOT_REACHED();

    if (mach_port_insert_right(mach_task_self(), m_exceptionPort, m_exceptionPort, MACH_MSG_TYPE_MAKE_SEND) != KERN_SUCCESS)
        ASSERT_NOT_REACHED();
}

} // namespace CoreIPC
