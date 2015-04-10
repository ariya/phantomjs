/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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

#ifndef ConnectionStack_h
#define ConnectionStack_h

#include <wtf/Vector.h>

namespace CoreIPC {
    class Connection;
}

namespace WebKit {

class ConnectionStack {
public:
    static ConnectionStack& shared();

    CoreIPC::Connection* current()
    {
        return m_connectionStack.last();
    }

    class CurrentConnectionPusher {
    public:
        CurrentConnectionPusher(ConnectionStack& connectionStack, CoreIPC::Connection* connection)
            : m_connectionStack(connectionStack)
#if !ASSERT_DISABLED
            , m_connection(connection)
#endif
        {
            m_connectionStack.m_connectionStack.append(connection);
        }

        ~CurrentConnectionPusher()
        {
            ASSERT(m_connectionStack.current() == m_connection);
            m_connectionStack.m_connectionStack.removeLast();
        }

    private:
        ConnectionStack& m_connectionStack;
#if !ASSERT_DISABLED
        CoreIPC::Connection* m_connection;
#endif
    };

private:
    // It's OK for these to be weak pointers because we only push object on the stack
    // from within didReceiveMessage and didReceiveSyncMessage and the Connection objects are
    // already ref'd for the duration of those functions.
    Vector<CoreIPC::Connection*, 4> m_connectionStack;
};

} // namespace WebKit

#endif // ConnectionStack_h
