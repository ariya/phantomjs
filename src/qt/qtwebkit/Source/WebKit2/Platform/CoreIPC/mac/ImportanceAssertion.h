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

#ifndef ImportanceAssertion_h
#define ImportanceAssertion_h

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090

#include <wtf/PassOwnPtr.h>

#if __has_include(<libproc_internal.h>)
#include <libproc_internal.h>
#endif

extern "C" int proc_importance_assertion_begin_with_msg(mach_msg_header_t*, mach_msg_trailer_t*, uint64_t*);
extern "C" int proc_importance_assertion_complete(uint64_t assertion_handle);

namespace CoreIPC {

class ImportanceAssertion {
    WTF_MAKE_NONCOPYABLE(ImportanceAssertion);

public:
    static PassOwnPtr<ImportanceAssertion> create(mach_msg_header_t* header)
    {
        return adoptPtr(new ImportanceAssertion(header));
    }

    ~ImportanceAssertion()
    {
        proc_importance_assertion_complete(m_assertion);
    }

private:
    ImportanceAssertion(mach_msg_header_t* header)
        : m_assertion(0)
    {
        proc_importance_assertion_begin_with_msg(header, 0, &m_assertion);
    }

    uint64_t m_assertion;
};

}

#endif // __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090

#endif // ImportanceAssertion_h
