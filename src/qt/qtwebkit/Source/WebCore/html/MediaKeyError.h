/*
 * Copyright (C) 2012 Google Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef MediaKeyError_h
#define MediaKeyError_h

#if ENABLE(ENCRYPTED_MEDIA) || ENABLE(ENCRYPTED_MEDIA_V2)

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class MediaKeyError : public RefCounted<MediaKeyError> {
public:
    enum {
        MEDIA_KEYERR_UNKNOWN = 1,
        MEDIA_KEYERR_CLIENT,
        MEDIA_KEYERR_SERVICE,
        MEDIA_KEYERR_OUTPUT,
        MEDIA_KEYERR_HARDWARECHANGE,
        MEDIA_KEYERR_DOMAIN
    };
    typedef unsigned short Code;

    static PassRefPtr<MediaKeyError> create(Code code, unsigned long systemCode = 0) { return adoptRef(new MediaKeyError(code, systemCode)); }

    Code code() const { return m_code; }
    unsigned long systemCode() { return m_systemCode; }

private:
    explicit MediaKeyError(Code code, unsigned long systemCode) : m_code(code), m_systemCode(systemCode) { }

    Code m_code;
    unsigned long m_systemCode;
};

} // namespace WebCore

#endif
#endif
