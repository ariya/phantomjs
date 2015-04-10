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

#ifndef FileReaderLoader_h
#define FileReaderLoader_h

#if ENABLE(BLOB)

#include "FileError.h"
#include "KURL.h"
#include "TextEncoding.h"
#include "ThreadableLoaderClient.h"
#include <wtf/Forward.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class Blob;
class FileReaderLoaderClient;
class ScriptExecutionContext;
class TextResourceDecoder;
class ThreadableLoader;

class FileReaderLoader : public ThreadableLoaderClient {
public:
    enum ReadType {
        ReadAsArrayBuffer,
        ReadAsBinaryString,
        ReadAsBlob,
        ReadAsText,
        ReadAsDataURL
    };

    // If client is given, do the loading asynchronously. Otherwise, load synchronously.
    FileReaderLoader(ReadType, FileReaderLoaderClient*);
    ~FileReaderLoader();

    void start(ScriptExecutionContext*, Blob*);
    void cancel();

    // ThreadableLoaderClient
    virtual void didReceiveResponse(unsigned long, const ResourceResponse&);
    virtual void didReceiveData(const char*, int);
    virtual void didFinishLoading(unsigned long, double);
    virtual void didFail(const ResourceError&);

    String stringResult();
    PassRefPtr<ArrayBuffer> arrayBufferResult() const;
#if ENABLE(STREAM)
    PassRefPtr<Blob> blobResult();
#endif // ENABLE(STREAM)
    unsigned bytesLoaded() const { return m_bytesLoaded; }
    unsigned totalBytes() const { return m_totalBytes; }
    int errorCode() const { return m_errorCode; }

    void setEncoding(const String&);
    void setDataType(const String& dataType) { m_dataType = dataType; }
#if ENABLE(STREAM)
    void setRange(unsigned, unsigned);
#endif // ENABLE(STREAM)

private:
    void terminate();
    void cleanup();
    void failed(int errorCode);
    void convertToText();
    void convertToDataURL();

    bool isCompleted() const;

    static FileError::ErrorCode httpStatusCodeToErrorCode(int);

    ReadType m_readType;
    FileReaderLoaderClient* m_client;
    TextEncoding m_encoding;
    String m_dataType;

    KURL m_urlForReading;
    RefPtr<ThreadableLoader> m_loader;

    RefPtr<ArrayBuffer> m_rawData;
    bool m_isRawDataConverted;

    String m_stringResult;
    RefPtr<Blob> m_blobResult;

    // The decoder used to decode the text data.
    RefPtr<TextResourceDecoder> m_decoder;

    bool m_variableLength;
    unsigned m_bytesLoaded;
    unsigned m_totalBytes;

    bool m_hasRange;
    unsigned m_rangeStart;
    unsigned m_rangeEnd;

    int m_errorCode;
};

} // namespace WebCore

#endif // ENABLE(BLOB)

#endif // FileReaderLoader_h
