/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SocketStreamErrorBase_h
#define SocketStreamErrorBase_h

#include "PlatformString.h"

namespace WebCore {

    class SocketStreamError;

    class SocketStreamErrorBase {
    public:
        // Makes a deep copy.  Useful for when you need to use a SocketStreamError on another thread.
        SocketStreamError copy() const;

        bool isNull() const { return m_isNull; }

        int errorCode() const { return m_errorCode; }
        const String& failingURL() const { return m_failingURL; }
        const String& localizedDescription() const { return m_localizedDescription; }

        static bool compare(const SocketStreamError&, const SocketStreamError&);

    protected:
        SocketStreamErrorBase()
            : m_errorCode(0)
            , m_isNull(true)
        {
        }

        explicit SocketStreamErrorBase(int errorCode)
            : m_errorCode(errorCode)
            , m_isNull(false)
        {
        }

        SocketStreamErrorBase(int errorCode, const String& failingURL, const String& localizedDescription)
            : m_errorCode(errorCode)
            , m_failingURL(failingURL)
            , m_localizedDescription(localizedDescription)
            , m_isNull(false)
        {
        }

        int m_errorCode;
        String m_failingURL;
        String m_localizedDescription;
        bool m_isNull;
    };

    inline bool operator==(const SocketStreamError& a, const SocketStreamError& b) { return SocketStreamErrorBase::compare(a, b); }
    inline bool operator!=(const SocketStreamError& a, const SocketStreamError& b) { return !(a == b); }

}  // namespace WebCore

#endif  // SocketStreamErrorBase_h
