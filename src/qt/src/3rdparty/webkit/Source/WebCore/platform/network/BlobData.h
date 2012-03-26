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

#ifndef BlobData_h
#define BlobData_h

#include "KURL.h"
#include "PlatformString.h"
#include <wtf/Forward.h>
#include <wtf/ThreadSafeRefCounted.h>

namespace WebCore {

class RawData : public ThreadSafeRefCounted<RawData> {
public:
    static PassRefPtr<RawData> create()
    {
        return adoptRef(new RawData());
    }

    void detachFromCurrentThread();

    const char* data() const { return m_data.data(); }
    size_t length() const { return m_data.size(); }
    Vector<char>* mutableData() { return &m_data; }

private:
    RawData();

    Vector<char> m_data;
};

struct BlobDataItem {
    static const long long toEndOfFile;
    static const double doNotCheckFileChange;

    // Default constructor.
    BlobDataItem()
        : type(Data)
        , offset(0)
        , length(toEndOfFile)
        , expectedModificationTime(doNotCheckFileChange)
    {
    }

    // Constructor for String type (complete string).
    explicit BlobDataItem(PassRefPtr<RawData> data)
        : type(Data)
        , data(data)
        , offset(0)
        , length(toEndOfFile)
        , expectedModificationTime(doNotCheckFileChange)
    {
    }

    // Constructor for File type (complete file).
    explicit BlobDataItem(const String& path)
        : type(File)
        , path(path)
        , offset(0)
        , length(toEndOfFile)
        , expectedModificationTime(doNotCheckFileChange)
    {
    }

    // Constructor for File type (partial file).
    BlobDataItem(const String& path, long long offset, long long length, double expectedModificationTime)
        : type(File)
        , path(path)
        , offset(offset)
        , length(length)
        , expectedModificationTime(expectedModificationTime)
    {
    }
    
    // Constructor for Blob type.
    BlobDataItem(const KURL& url, long long offset, long long length)
        : type(Blob)
        , url(url)
        , offset(offset)
        , length(length)
        , expectedModificationTime(doNotCheckFileChange)
    {
    }

    // Detaches from current thread so that it can be passed to another thread.
    void detachFromCurrentThread();

    enum { Data, File, Blob } type;
    
    // For Data type.
    RefPtr<RawData> data;

    // For File type.
    String path;

    // For Blob type.
    KURL url;

    long long offset;
    long long length;
    double expectedModificationTime;

private:
    friend class BlobData;

    // Constructor for String type (partial string).
    BlobDataItem(PassRefPtr<RawData> data, long long offset, long long length)
        : type(Data)
        , data(data)
        , offset(offset)
        , length(length)
        , expectedModificationTime(doNotCheckFileChange)
    {
    }
};

typedef Vector<BlobDataItem> BlobDataItemList;

class BlobData {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<BlobData> create();

    // Detaches from current thread so that it can be passed to another thread.
    void detachFromCurrentThread();

    const String& contentType() const { return m_contentType; }
    void setContentType(const String& contentType) { m_contentType = contentType; }

    const String& contentDisposition() const { return m_contentDisposition; }
    void setContentDisposition(const String& contentDisposition) { m_contentDisposition = contentDisposition; }

    const BlobDataItemList& items() const { return m_items; }
    void swapItems(BlobDataItemList&);

    void appendData(PassRefPtr<RawData>, long long offset, long long length);
    void appendFile(const String& path);
    void appendFile(const String& path, long long offset, long long length, double expectedModificationTime);
    void appendBlob(const KURL&, long long offset, long long length);

private:
    friend class BlobRegistryImpl;
    friend class BlobStorageData;

    BlobData() { }

    // This is only exposed to BlobStorageData.
    void appendData(const RawData&, long long offset, long long length);

    String m_contentType;
    String m_contentDisposition;
    BlobDataItemList m_items;
};

} // namespace WebCore

#endif // BlobData_h
