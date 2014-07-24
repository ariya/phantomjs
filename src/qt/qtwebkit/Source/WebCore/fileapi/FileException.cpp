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

#if ENABLE(BLOB)

#include "FileException.h"

namespace WebCore {

static struct FileExceptionNameDescription {
    const char* const name;
    const char* const description;
} fileExceptions[] = {
    { "NOT_FOUND_ERR", "A requested file or directory could not be found at the time an operation was processed." },
    { "SECURITY_ERR", "It was determined that certain files are unsafe for access within a Web application, or that too many calls are being made on file resources." },
    { "ABORT_ERR", "An ongoing operation was aborted, typically with a call to abort()." },
    { "NOT_READABLE_ERR", "The requested file could not be read, typically due to permission problems that have occurred after a reference to a file was acquired." },
    { "ENCODING_ERR", "A URI supplied to the API was malformed, or the resulting Data URL has exceeded the URL length limitations for Data URLs." },
    { "NO_MODIFICATION_ALLOWED_ERR", "An attempt was made to write to a file or directory which could not be modified due to the state of the underlying filesystem." },
    { "INVALID_STATE_ERR", "An operation that depends on state cached in an interface object was made but the state had changed since it was read from disk." },
    { "SYNTAX_ERR", "An invalid or unsupported argument was given, like an invalid line ending specifier." },
    { "INVALID_MODIFICATION_ERR", "The modification request was illegal." },
    { "QUOTA_EXCEEDED_ERR", "The operation failed because it would cause the application to exceed its storage quota." },
    { "TYPE_MISMATCH_ERR", "The path supplied exists, but was not an entry of requested type." },
    { "PATH_EXISTS_ERR", "An attempt was made to create a file or directory where an element already exists." }
};

bool FileException::initializeDescription(ExceptionCode ec, ExceptionCodeDescription* description)
{
    if (ec < FileExceptionOffset || ec > FileExceptionMax)
        return false;

    description->typeName = "DOM File";
    description->code = ec - FileExceptionOffset;
    description->type = FileExceptionType;

    size_t tableSize = WTF_ARRAY_LENGTH(fileExceptions);
    size_t tableIndex = ec - NOT_FOUND_ERR;

    description->name = tableIndex < tableSize ? fileExceptions[tableIndex].name : 0;
    description->description = tableIndex < tableSize ? fileExceptions[tableIndex].description : 0;

    return true;
}

} // namespace WebCore

#endif // ENABLE(BLOB)
