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

#include "config.h"
#include "SeccompFilters.h"

#if ENABLE(SECCOMP_FILTERS)

#include "SeccompBroker.h"
#include <seccomp.h>
#include <wtf/Assertions.h>

namespace WebKit {

COMPILE_ASSERT(SeccompFilters::Allow == SCMP_ACT_ALLOW, Allow);
COMPILE_ASSERT(SeccompFilters::Kill == SCMP_ACT_KILL, Kill);
COMPILE_ASSERT(SeccompFilters::Trap == SCMP_ACT_TRAP, Trap);

COMPILE_ASSERT(SeccompFilters::NotSet == static_cast<SeccompFilters::Operator>(_SCMP_CMP_MIN), NotSet);
COMPILE_ASSERT(SeccompFilters::NotEqual == static_cast<SeccompFilters::Operator>(SCMP_CMP_NE), NotEqual);
COMPILE_ASSERT(SeccompFilters::Equal == static_cast<SeccompFilters::Operator>(SCMP_CMP_EQ), Equal);

COMPILE_ASSERT(sizeof(scmp_datum_t) == sizeof(long long), scmp_datum_t);

SeccompFilters::SeccompFilters(Action defaultAction)
    : m_context(seccomp_init(defaultAction))
    , m_initialized(false)
{
    if (!m_context)
        CRASH();
}

SeccompFilters::~SeccompFilters()
{
    seccomp_release(m_context);
}

void SeccompFilters::addRule(const char* syscallName, Action action,
    unsigned argNum1, Operator operator1, long long data1,
    unsigned argNum2, Operator operator2, long long data2)
{
    int syscall = seccomp_syscall_resolve_name(syscallName);
    if (syscall == __NR_SCMP_ERROR)
        CRASH();

    int result;
    if (operator2 != NotSet)
        result = seccomp_rule_add(m_context, action, syscall, 2,
            SCMP_CMP(argNum1, static_cast<scmp_compare>(operator1), data1, 0),
            SCMP_CMP(argNum2, static_cast<scmp_compare>(operator2), data2, 0));
    else if (operator1 != NotSet)
        result = seccomp_rule_add(m_context, action, syscall, 1,
            SCMP_CMP(argNum1, static_cast<scmp_compare>(operator1), data1, 0));
    else
        result = seccomp_rule_add(m_context, action, syscall, 0);

    if (result < 0)
        CRASH();
}

void SeccompFilters::initialize()
{
    if (m_initialized)
        return;

    // Implement this is not required in case we are just
    // setting filters. This is a good place to create the
    // broker and syscall policy otherwise.
    platformInitialize();

    if (seccomp_load(m_context) < 0)
        CRASH();

    m_initialized = true;
}

} // namespace WebKit

#endif // ENABLE(SECCOMP_FILTERS)
