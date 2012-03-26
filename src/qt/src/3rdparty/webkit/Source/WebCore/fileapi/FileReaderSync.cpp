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

#include "config.h"

#if ENABLE(BLOB)

#include "FileReaderSync.h"

#include "ArrayBuffer.h"
#include "Blob.h"
#include "BlobURL.h"
#include "FileException.h"
#include "FileReaderLoader.h"
#include <wtf/PassRefPtr.h>

namespace WebCore {

FileReaderSync::FileReaderSync()
{
}

PassRefPtr<ArrayBuffer> FileReaderSync::readAsArrayBuffer(ScriptExecutionContext* scriptExecutionContext, Blob* blob, ExceptionCode& ec)
{
    if (!blob)
        return 0;

    FileReaderLoader loader(FileReaderLoader::ReadAsArrayBuffer, 0);
    startLoading(scriptExecutionContext, loader, blob, ec);

    return loader.arrayBufferResult();
}

String FileReaderSync::readAsBinaryString(ScriptExecutionContext* scriptExecutionContext, Blob* blob, ExceptionCode& ec)
{
    if (!blob)
        return String();

    FileReaderLoader loader(FileReaderLoader::ReadAsBinaryString, 0);
    startLoading(scriptExecutionContext, loader, blob, ec);
    return loader.stringResult();
}

String FileReaderSync::readAsText(ScriptExecutionContext* scriptExecutionContext, Blob* blob, const String& encoding, ExceptionCode& ec)
{
    if (!blob)
        return String();

    FileReaderLoader loader(FileReaderLoader::ReadAsText, 0);
    loader.setEncoding(encoding);
    startLoading(scriptExecutionContext, loader, blob, ec);
    return loader.stringResult();
}

String FileReaderSync::readAsDataURL(ScriptExecutionContext* scriptExecutionContext, Blob* blob, ExceptionCode& ec)
{
    if (!blob)
        return String();

    FileReaderLoader loader(FileReaderLoader::ReadAsDataURL, 0);
    loader.setDataType(blob->type());
    startLoading(scriptExecutionContext, loader, blob, ec);
    return loader.stringResult();
}

void FileReaderSync::startLoading(ScriptExecutionContext* scriptExecutionContext, FileReaderLoader& loader, Blob* blob, ExceptionCode& ec)
{
    loader.start(scriptExecutionContext, blob);
    ec = FileException::ErrorCodeToExceptionCode(loader.errorCode());
}

} // namespace WebCore
 
#endif // ENABLE(BLOB)
