/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#ifndef BlobResourceHandle_h
#define BlobResourceHandle_h

#if ENABLE(BLOB)

#include "FileStreamClient.h"
#include "ResourceHandle.h"
#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class AsyncFileStream;
class BlobStorageData;
class FileStream;
class ResourceHandleClient;
class ResourceRequest;
struct BlobDataItem;

class BlobResourceHandle : public FileStreamClient, public ResourceHandle  {
public:
    static PassRefPtr<BlobResourceHandle> createAsync(BlobStorageData*, const ResourceRequest&, ResourceHandleClient*);

    static void loadResourceSynchronously(BlobStorageData* blobData, const ResourceRequest& request, ResourceError& error, ResourceResponse& response, Vector<char>& data);

    // FileStreamClient methods.
    virtual void didGetSize(long long) OVERRIDE;
    virtual void didOpen(bool) OVERRIDE;
    virtual void didRead(int) OVERRIDE;

    // ResourceHandle methods.
    virtual void cancel() OVERRIDE;

    void start();
    int readSync(char*, int);

    bool aborted() const { return m_aborted; }

private:
    friend void delayedStartBlobResourceHandle(void*);

    BlobResourceHandle(PassRefPtr<BlobStorageData>, const ResourceRequest&, ResourceHandleClient*, bool async);
    virtual ~BlobResourceHandle();

    void doStart();
    void getSizeForNext();
    void seek();
    void consumeData(const char* data, int bytesRead);
    void failed(int errorCode);

    void readAsync();
    void readDataAsync(const BlobDataItem&);
    void readFileAsync(const BlobDataItem&);

    int readDataSync(const BlobDataItem&, char*, int);
    int readFileSync(const BlobDataItem&, char*, int);

    void notifyResponse();
    void notifyResponseOnSuccess();
    void notifyResponseOnError();
    void notifyReceiveData(const char*, int);
    void notifyFail(int errorCode);
    void notifyFinish();

    RefPtr<BlobStorageData> m_blobData;
    bool m_async;
    RefPtr<AsyncFileStream> m_asyncStream; // For asynchronous loading.
    RefPtr<FileStream> m_stream; // For synchronous loading.
    Vector<char> m_buffer;
    Vector<long long> m_itemLengthList;
    int m_errorCode;
    bool m_aborted;
    long long m_rangeOffset;
    long long m_rangeEnd;
    long long m_rangeSuffixLength;
    long long m_totalRemainingSize;
    long long m_currentItemReadSize;
    unsigned m_sizeItemCount;
    unsigned m_readItemCount;
    bool m_fileOpened;
};

} // namespace WebCore

#endif // ENABLE(BLOB)

#endif // BlobResourceHandle_h
