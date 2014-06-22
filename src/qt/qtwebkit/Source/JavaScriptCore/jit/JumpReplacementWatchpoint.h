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

#ifndef JumpReplacementWatchpoint_h
#define JumpReplacementWatchpoint_h

#include "Watchpoint.h"
#include <wtf/Platform.h>

#if ENABLE(JIT)

#include "CodeLocation.h"
#include "MacroAssembler.h"

namespace JSC {

class JumpReplacementWatchpoint : public Watchpoint {
public:
    JumpReplacementWatchpoint()
        : m_source(std::numeric_limits<uintptr_t>::max())
        , m_destination(std::numeric_limits<uintptr_t>::max())
    {
    }
    
    JumpReplacementWatchpoint(MacroAssembler::Label source)
        : m_source(source.m_label.m_offset)
        , m_destination(std::numeric_limits<uintptr_t>::max())
    {
    }
    
    MacroAssembler::Label sourceLabel() const
    {
        MacroAssembler::Label label;
        label.m_label.m_offset = m_source;
        return label;
    }
    
    void setDestination(MacroAssembler::Label destination)
    {
        m_destination = destination.m_label.m_offset;
    }
    
    void correctLabels(LinkBuffer&);

protected:
    void fireInternal();

private:
    uintptr_t m_source;
    uintptr_t m_destination;
};

} // namespace JSC

#endif // ENABLE(JIT)

#endif // JumpReplacementWatchpoint_h

