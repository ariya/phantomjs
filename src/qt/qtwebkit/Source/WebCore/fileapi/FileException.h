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

#ifndef FileException_h
#define FileException_h

#if ENABLE(BLOB)

#include "ExceptionBase.h"

namespace WebCore {

class FileException : public ExceptionBase {
public:
    static PassRefPtr<FileException> create(const ExceptionCodeDescription& description)
    {
        return adoptRef(new FileException(description));
    }

    static const int FileExceptionOffset = 1100;
    static const int FileExceptionMax = 1199;

    enum FileExceptionCode {
        NOT_FOUND_ERR = FileExceptionOffset + 1,
        SECURITY_ERR = FileExceptionOffset + 2,
        ABORT_ERR = FileExceptionOffset + 3,
        NOT_READABLE_ERR = FileExceptionOffset + 4,
        ENCODING_ERR = FileExceptionOffset + 5,
        NO_MODIFICATION_ALLOWED_ERR = FileExceptionOffset + 6,
        INVALID_STATE_ERR = FileExceptionOffset + 7,
        SYNTAX_ERR = FileExceptionOffset + 8,
        INVALID_MODIFICATION_ERR = FileExceptionOffset + 9,
        QUOTA_EXCEEDED_ERR = FileExceptionOffset + 10,
        TYPE_MISMATCH_ERR = FileExceptionOffset + 11,
        PATH_EXISTS_ERR = FileExceptionOffset + 12,
    };

    static int ErrorCodeToExceptionCode(int errorCode)
    {
        if (!errorCode)
            return 0;
        return errorCode + FileExceptionOffset;
    }

    static bool initializeDescription(ExceptionCode, ExceptionCodeDescription*);

private:
    FileException(const ExceptionCodeDescription& description)
        : ExceptionBase(description)
    {
    }
};

} // namespace WebCore

#endif // ENABLE(BLOB)

#endif // FileException_h
