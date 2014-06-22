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

#ifndef MachPort_h
#define MachPort_h

#include "ArgumentDecoder.h"
#include "ArgumentEncoder.h"
#include "Attachment.h"

namespace CoreIPC {

class MachPort {
public:
    MachPort()
        : m_port(MACH_PORT_NULL)
        , m_disposition(0)
    {
    }

    MachPort(mach_port_name_t port, mach_msg_type_name_t disposition)
        : m_port(port)
        , m_disposition(disposition)
    {
    }

    void encode(ArgumentEncoder& encoder) const
    {
        encoder << Attachment(m_port, m_disposition);
    }

    static bool decode(ArgumentDecoder& decoder, MachPort& p)
    {
        Attachment attachment;
        if (!decoder.decode(attachment))
            return false;
        
        p.m_port = attachment.port();
        p.m_disposition = attachment.disposition();
        return true;
    }

    mach_port_name_t port() const { return m_port; }
    mach_msg_type_name_t disposition() const { return m_disposition; }

private:
    mach_port_name_t m_port;
    mach_msg_type_name_t m_disposition;
};

} // namespace CoreIPC

#endif // MachPort_h
