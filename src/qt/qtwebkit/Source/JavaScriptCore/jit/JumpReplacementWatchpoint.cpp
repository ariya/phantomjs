/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "JumpReplacementWatchpoint.h"

#if ENABLE(JIT)

#include "LinkBuffer.h"
#include "Options.h"

namespace JSC {

void JumpReplacementWatchpoint::correctLabels(LinkBuffer& linkBuffer)
{
    MacroAssembler::Label label;
    label.m_label.m_offset = m_source;
    m_source = bitwise_cast<uintptr_t>(linkBuffer.locationOf(label).dataLocation());
    label.m_label.m_offset = m_destination;
    m_destination = bitwise_cast<uintptr_t>(linkBuffer.locationOf(label).dataLocation());
}

void JumpReplacementWatchpoint::fireInternal()
{
    void* source = bitwise_cast<void*>(m_source);
    void* destination = bitwise_cast<void*>(m_destination);
    if (Options::showDisassembly())
        dataLogF("Firing jump replacement watchpoint from %p, to %p.\n", source, destination);
    MacroAssembler::replaceWithJump(CodeLocationLabel(source), CodeLocationLabel(destination));
    if (isOnList())
        remove();
}

} // namespace JSC

#endif // ENABLE(JIT)

