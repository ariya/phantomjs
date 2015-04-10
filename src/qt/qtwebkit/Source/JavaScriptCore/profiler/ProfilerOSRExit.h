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

#ifndef ProfilerOSRExit_h
#define ProfilerOSRExit_h

#include "ExitKind.h"
#include "JSCJSValue.h"
#include "ProfilerOriginStack.h"

namespace JSC { namespace Profiler {

class OSRExit {
public:
    OSRExit(unsigned id, const OriginStack&, ExitKind, bool isWatchpoint);
    ~OSRExit();
    
    unsigned id() const { return m_id; }
    const OriginStack& origin() const { return m_origin; }
    ExitKind exitKind() const { return m_exitKind; }
    bool isWatchpoint() const { return m_isWatchpoint; }
    
    uint64_t* counterAddress() { return &m_counter; }
    uint64_t count() const { return m_counter; }
    
    JSValue toJS(ExecState*) const;

private:
    unsigned m_id;
    OriginStack m_origin;
    ExitKind m_exitKind;
    bool m_isWatchpoint;
    uint64_t m_counter;
};

} } // namespace JSC::Profiler

#endif // ProfilerOSRExit_h

