/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef IDBDatabaseException_h
#define IDBDatabaseException_h

#if ENABLE(INDEXED_DATABASE)

#include "ExceptionBase.h"

namespace WebCore {

class IDBDatabaseException : public ExceptionBase {
public:
    static PassRefPtr<IDBDatabaseException> create(const ExceptionCodeDescription& description)
    {
        return adoptRef(new IDBDatabaseException(description));
    }

    static const int IDBDatabaseExceptionOffset = 1200;
    static const int IDBDatabaseExceptionMax = 1299;

    enum IDBDatabaseExceptionCode {
        NO_ERR = IDBDatabaseExceptionOffset + 0,
        UNKNOWN_ERR = IDBDatabaseExceptionOffset + 1,
        NON_TRANSIENT_ERR = IDBDatabaseExceptionOffset + 2,
        NOT_FOUND_ERR = IDBDatabaseExceptionOffset + 3,
        CONSTRAINT_ERR = IDBDatabaseExceptionOffset + 4,
        DATA_ERR = IDBDatabaseExceptionOffset + 5,
        NOT_ALLOWED_ERR = IDBDatabaseExceptionOffset + 6,
        SERIAL_ERR = IDBDatabaseExceptionOffset + 7,
        RECOVERABLE_ERR = IDBDatabaseExceptionOffset + 8,
        TRANSIENT_ERR = IDBDatabaseExceptionOffset + 9,
        TIMEOUT_ERR = IDBDatabaseExceptionOffset + 10,
        DEADLOCK_ERR = IDBDatabaseExceptionOffset + 11,
        READ_ONLY_ERR = IDBDatabaseExceptionOffset + 12,
        ABORT_ERR = IDBDatabaseExceptionOffset + 13
    };

    static int ErrorCodeToExceptionCode(int errorCode)
    {
        if (!errorCode)
            return 0;
        return errorCode + IDBDatabaseExceptionOffset;
    }

private:
    IDBDatabaseException(const ExceptionCodeDescription& description)
        : ExceptionBase(description)
    {
    }
};

} // namespace WebCore

#endif

#endif // IDBDatabaseException_h
