/*
 * Copyright (C) 2010 Google Inc.  All rights reserved.
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

#ifndef FileError_h
#define FileError_h

#if ENABLE(BLOB) || ENABLE(FILE_SYSTEM)

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class FileError : public RefCounted<FileError> {
public:
    enum ErrorCode {
        OK = 0,
        NOT_FOUND_ERR = 1,
        SECURITY_ERR = 2,
        ABORT_ERR = 3,
        NOT_READABLE_ERR = 4,
        ENCODING_ERR = 5,
        NO_MODIFICATION_ALLOWED_ERR = 6,
        INVALID_STATE_ERR = 7,
        SYNTAX_ERR = 8,
        INVALID_MODIFICATION_ERR = 9,
        QUOTA_EXCEEDED_ERR = 10,
        TYPE_MISMATCH_ERR = 11,
        PATH_EXISTS_ERR = 12,
    };

    static PassRefPtr<FileError> create(ErrorCode code) { return adoptRef(new FileError(code)); }

    ErrorCode code() const { return m_code; }

private:
    FileError(ErrorCode code)
        : m_code(code)
    { }

    ErrorCode m_code;
};

} // namespace WebCore

#endif // ENABLE(BLOB) || ENABLE(FILE_SYSTEM)

#endif // FileError_h
