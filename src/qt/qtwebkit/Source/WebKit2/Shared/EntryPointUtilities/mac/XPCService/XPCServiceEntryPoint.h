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

#ifndef XPCServiceEntryPoint_h
#define XPCServiceEntryPoint_h

#if HAVE(XPC)

#import "ChildProcess.h"
#import "WebKit2Initialize.h"
#import <xpc/xpc.h>

namespace WebKit {

class XPCServiceInitializerDelegate {
public:
    XPCServiceInitializerDelegate(xpc_connection_t connection, xpc_object_t initializerMessage)
        : m_connection(connection)
        , m_initializerMessage(initializerMessage)
    {
    }

    virtual ~XPCServiceInitializerDelegate();

    virtual bool getConnectionIdentifier(CoreIPC::Connection::Identifier& identifier);
    virtual bool getClientIdentifier(String& clientIdentifier);
    virtual bool getClientProcessName(String& clientProcessName);
    virtual bool getExtraInitializationData(HashMap<String, String>& extraInitializationData);

protected:
    xpc_connection_t m_connection;
    xpc_object_t m_initializerMessage;
};

template<typename XPCServiceType, typename XPCServiceInitializerDelegateType>
void XPCServiceInitializer(xpc_connection_t connection, xpc_object_t initializerMessage)
{
    XPCServiceInitializerDelegateType delegate(connection, initializerMessage);

    // We don't want XPC to be in charge of whether the process should be terminated or not,
    // so ensure that we have an outstanding transaction here.
    xpc_transaction_begin();

    InitializeWebKit2();

    ChildProcessInitializationParameters parameters;
    if (!delegate.getConnectionIdentifier(parameters.connectionIdentifier))
        exit(EXIT_FAILURE);

    if (!delegate.getClientIdentifier(parameters.clientIdentifier))
        exit(EXIT_FAILURE);

    if (!delegate.getClientProcessName(parameters.uiProcessName))
        exit(EXIT_FAILURE);

    if (!delegate.getExtraInitializationData(parameters.extraInitializationData))
        exit(EXIT_FAILURE);

    XPCServiceType::shared().initialize(parameters);
}

} // namespace WebKit

#endif // HAVE(XPC)

#endif // XPCServiceEntryPoint_h
