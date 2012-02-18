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

#ifndef FileReaderSync_h
#define FileReaderSync_h

#if ENABLE(BLOB)

#include "ExceptionCode.h"
#include <wtf/Forward.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class ArrayBuffer;
class Blob;
class FileReaderLoader;
class ScriptExecutionContext;

class FileReaderSync : public RefCounted<FileReaderSync> {
public:
    static PassRefPtr<FileReaderSync> create()
    {
        return adoptRef(new FileReaderSync());
    }

    virtual ~FileReaderSync() { }

    PassRefPtr<ArrayBuffer> readAsArrayBuffer(ScriptExecutionContext*, Blob*, ExceptionCode&);
    String readAsBinaryString(ScriptExecutionContext*, Blob*, ExceptionCode&);
    String readAsText(ScriptExecutionContext* scriptExecutionContext, Blob* blob, ExceptionCode& ec)
    {
        return readAsText(scriptExecutionContext, blob, "", ec);
    }
    String readAsText(ScriptExecutionContext*, Blob*, const String& encoding, ExceptionCode&);
    String readAsDataURL(ScriptExecutionContext*, Blob*, ExceptionCode&);

private:
    FileReaderSync();

    void startLoading(ScriptExecutionContext*, FileReaderLoader&, Blob*, ExceptionCode&);
};

} // namespace WebCore

#endif // ENABLE(BLOB)

#endif // FileReaderSync_h
