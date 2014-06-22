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

#include "config.h"
#include "Blob.h"

#include "BlobURL.h"
#include "File.h"
#include "HistogramSupport.h"
#include "ScriptCallStack.h"
#include "ScriptExecutionContext.h"
#include "ThreadableBlobRegistry.h"
#include <wtf/text/CString.h>

namespace WebCore {

namespace {

// Used in histograms to see when we can actually deprecate the prefixed slice.
enum SliceHistogramEnum {
    SliceWithoutPrefix,
    SliceWithPrefix,
    SliceHistogramEnumMax,
};

} // namespace

Blob::Blob()
    : m_size(0)
{
    OwnPtr<BlobData> blobData = BlobData::create();

    // Create a new internal URL and register it with the provided blob data.
    m_internalURL = BlobURL::createInternalURL();
    ThreadableBlobRegistry::registerBlobURL(m_internalURL, blobData.release());
}

Blob::Blob(PassOwnPtr<BlobData> blobData, long long size)
    : m_type(blobData->contentType())
    , m_size(size)
{
    ASSERT(blobData);

    // Create a new internal URL and register it with the provided blob data.
    m_internalURL = BlobURL::createInternalURL();
    ThreadableBlobRegistry::registerBlobURL(m_internalURL, blobData);
}

Blob::Blob(const KURL& srcURL, const String& type, long long size)
    : m_type(Blob::normalizedContentType(type))
    , m_size(size)
{
    // Create a new internal URL and register it with the same blob data as the source URL.
    m_internalURL = BlobURL::createInternalURL();
    ThreadableBlobRegistry::registerBlobURL(0, m_internalURL, srcURL);
}

Blob::~Blob()
{
    ThreadableBlobRegistry::unregisterBlobURL(m_internalURL);
}

bool Blob::isValidContentType(const String& contentType)
{
    if (contentType.isNull())
        return true;

    size_t length = contentType.length();
    if (contentType.is8Bit()) {
        const LChar* characters = contentType.characters8();
        for (size_t i = 0; i < length; ++i) {
            if (characters[i] < 0x20 || characters[i] > 0x7e)
                return false;
        }
    } else {
        const UChar* characters = contentType.characters16();
        for (size_t i = 0; i < length; ++i) {
            if (characters[i] < 0x20 || characters[i] > 0x7e)
                return false;
        }
    }
    return true;
}

String Blob::normalizedContentType(const String& contentType)
{
    if (Blob::isValidContentType(contentType))
        return contentType.lower();
    return emptyString();
}

bool Blob::isNormalizedContentType(const String& contentType)
{
    if (contentType.isNull())
        return true;

    size_t length = contentType.length();
    if (contentType.is8Bit()) {
        const LChar* characters = contentType.characters8();
        for (size_t i = 0; i < length; ++i) {
            if (characters[i] < 0x20 || characters[i] > 0x7e)
                return false;
            if (characters[i] >= 'A' && characters[i] <= 'Z')
                return false;
        }
    } else {
        const UChar* characters = contentType.characters16();
        for (size_t i = 0; i < length; ++i) {
            if (characters[i] < 0x20 || characters[i] > 0x7e)
                return false;
            if (characters[i] >= 'A' && characters[i] <= 'Z')
                return false;
        }
    }
    return true;
}

bool Blob::isNormalizedContentType(const CString& contentType)
{
    size_t length = contentType.length();
    const char* characters = contentType.data();
    for (size_t i = 0; i < length; ++i) {
        if (characters[i] < 0x20 || characters[i] > 0x7e)
            return false;
        if (characters[i] >= 'A' && characters[i] <= 'Z')
            return false;
    }
    return true;
}

#if ENABLE(BLOB)
PassRefPtr<Blob> Blob::slice(long long start, long long end, const String& contentType) const
{
    // When we slice a file for the first time, we obtain a snapshot of the file by capturing its current size and modification time.
    // The modification time will be used to verify if the file has been changed or not, when the underlying data are accessed.
    long long size;
    double modificationTime;
    if (isFile()) {
        // FIXME: This involves synchronous file operation. We need to figure out how to make it asynchronous.
        toFile(this)->captureSnapshot(size, modificationTime);
    } else {
        ASSERT(m_size != -1);
        size = m_size;
    }

    // Convert the negative value that is used to select from the end.
    if (start < 0)
        start = start + size;
    if (end < 0)
        end = end + size;

    // Clamp the range if it exceeds the size limit.
    if (start < 0)
        start = 0;
    if (end < 0)
        end = 0;
    if (start >= size) {
        start = 0;
        end = 0;
    } else if (end < start)
        end = start;
    else if (end > size)
        end = size;

    long long length = end - start;
    OwnPtr<BlobData> blobData = BlobData::create();
    blobData->setContentType(Blob::normalizedContentType(contentType));
    if (isFile()) {
#if ENABLE(FILE_SYSTEM)
        if (!toFile(this)->fileSystemURL().isEmpty())
            blobData->appendURL(toFile(this)->fileSystemURL(), start, length, modificationTime);
        else
#endif
        blobData->appendFile(toFile(this)->path(), start, length, modificationTime);
    } else
        blobData->appendBlob(m_internalURL, start, length);

    return Blob::create(blobData.release(), length);
}
#endif

} // namespace WebCore
