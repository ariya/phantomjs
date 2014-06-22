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
#include "Attachment.h"

#include "ArgumentDecoder.h"
#include "ArgumentEncoder.h"

namespace CoreIPC {

Attachment::Attachment()
    : m_type(Uninitialized)
{
}

#if OS(DARWIN)
Attachment::Attachment(mach_port_name_t port, mach_msg_type_name_t disposition)
    : m_type(MachPortType)
{
    m_port.port = port;
    m_port.disposition = disposition;
}

Attachment::Attachment(void* address, mach_msg_size_t size, mach_msg_copy_options_t copyOptions, bool deallocate)
    : m_type(MachOOLMemoryType)
{
    m_oolMemory.address = address;
    m_oolMemory.size = size;
    m_oolMemory.copyOptions = copyOptions;
    m_oolMemory.deallocate = deallocate;
}

void Attachment::release()
{
    m_type = Uninitialized;
}
#endif

void Attachment::encode(ArgumentEncoder& encoder) const
{
    encoder.addAttachment(*this);
}

bool Attachment::decode(ArgumentDecoder& decoder, Attachment& attachment)
{
    if (!decoder.removeAttachment(attachment))
        return false;
    return true;
}

} // namespace CoreIPC
