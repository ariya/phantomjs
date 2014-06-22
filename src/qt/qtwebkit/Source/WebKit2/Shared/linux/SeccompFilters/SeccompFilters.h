/*
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
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

#ifndef SeccompFilters_h
#define SeccompFilters_h

#if ENABLE(SECCOMP_FILTERS)

#include <wtf/Noncopyable.h>

namespace WebKit {

class SeccompFilters {
    WTF_MAKE_NONCOPYABLE(SeccompFilters);

public:
    enum Action {
        Allow = 0x7fff0000U,
        Kill  = 0x00000000U,
        Trap  = 0x00030000U
    };

    enum Operator {
        NotSet   = 0,
        NotEqual = 1,
        Equal    = 4
    };

    explicit SeccompFilters(Action defaultAction);
    virtual ~SeccompFilters();

    void* context() { return m_context; };

    void addRule(const char* syscallName, Action,
        unsigned argNum1 = 0, Operator operator1 = NotSet, long long data1 = 0,
        unsigned argNum2 = 0, Operator operator2 = NotSet, long long data2 = 0);

    void initialize();

private:
    virtual void platformInitialize() { }

    typedef void *HANDLE;

    HANDLE m_context;
    bool m_initialized;
};

} // namespace WebKit

#endif // ENABLE(SECCOMP_FILTERS)

#endif // SeccompFilters_h
