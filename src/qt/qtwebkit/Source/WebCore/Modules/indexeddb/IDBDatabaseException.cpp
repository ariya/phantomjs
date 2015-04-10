/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE AND ITS CONTRIBUTORS "AS IS" AND ANY
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

#include "config.h"

#if ENABLE(INDEXED_DATABASE)

#include "IDBDatabaseException.h"

namespace WebCore {

static const struct IDBDatabaseExceptionNameDescription {
    const char* const name;
    const char* const description;
    const ExceptionCode code;
} idbDatabaseExceptions[] = {
    // These are IDB-specific errors from the spec.
    { "UnknownError", "An unknown error occurred within Indexed Database.", 0 },
    { "ConstraintError", "A mutation operation in the transaction failed because a constraint was not satisfied.", 0 },
    { "DataError", "The data provided does not meet requirements.", 0 },
    { "TransactionInactiveError", "A request was placed against a transaction which is either currently not active, or which is finished.", 0 },
    { "ReadOnlyError", "A write operation was attempted in a read-only transaction.", 0 },
    { "VersionError", "An attempt was made to open a database using a lower version than the existing version.", 0 },

    // These are IDB-specific descriptions of generic DOM Exceptions when they are thrown from IDB APIs
    { "NotFoundError", "An operation failed because the requested database object could not be found.", NOT_FOUND_ERR },
    { "InvalidStateError", "An operation was called on an object on which it is not allowed or at a time when it is not allowed.", INVALID_STATE_ERR },
    { "InvalidAccessError", "An invalid operation was performed on an object.", INVALID_ACCESS_ERR },
    { "AbortError", "The transaction was aborted, so the request cannot be fulfilled.", ABORT_ERR },
    { "TimeoutError", "A lock for the transaction could not be obtained in a reasonable time.", TIMEOUT_ERR }, // FIXME: This isn't used yet.
    { "QuotaExceededError", "The operation failed because there was not enough remaining storage space, or the storage quota was reached and the user declined to give more space to the database.", QUOTA_EXCEEDED_ERR },
    { "SyntaxError", "The keypath argument contains an invalid key path.", SYNTAX_ERR },
    { "DataCloneError", "The data being stored could not be cloned by the internal structured cloning algorithm.", DATA_CLONE_ERR },
};

static const IDBDatabaseExceptionNameDescription* getErrorEntry(ExceptionCode ec)
{
    if (ec < IDBDatabaseException::IDBDatabaseExceptionOffset || ec > IDBDatabaseException::IDBDatabaseExceptionMax)
        return 0;

    size_t tableSize = WTF_ARRAY_LENGTH(idbDatabaseExceptions);
    size_t tableIndex = ec - IDBDatabaseException::UnknownError;

    return tableIndex < tableSize ? &idbDatabaseExceptions[tableIndex] : 0;
}

bool IDBDatabaseException::initializeDescription(ExceptionCode ec, ExceptionCodeDescription* description)
{
    const IDBDatabaseExceptionNameDescription* entry = getErrorEntry(ec);
    if (!entry)
        return false;

    description->typeName = "DOM IDBDatabase";
    description->code = entry->code;
    description->type = DOMCoreExceptionType;

    description->name = entry ? entry->name : 0;
    description->description = entry ? entry->description : 0;

    return true;
}

String IDBDatabaseException::getErrorName(ExceptionCode ec)
{
    const IDBDatabaseExceptionNameDescription* entry = getErrorEntry(ec);
    ASSERT(entry);
    if (!entry)
        return "UnknownError";

    return entry->name;
}

String IDBDatabaseException::getErrorDescription(ExceptionCode ec)
{
    const IDBDatabaseExceptionNameDescription* entry = getErrorEntry(ec);
    ASSERT(entry);
    if (!entry)
        return "Unknown error.";

    return entry->description;
}

ExceptionCode IDBDatabaseException::getLegacyErrorCode(ExceptionCode ec)
{
    const IDBDatabaseExceptionNameDescription* entry = getErrorEntry(ec);
    ASSERT(entry);

    return (entry && entry->code) ? entry->code : ec - IDBDatabaseExceptionOffset;
}

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
