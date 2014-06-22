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
#include "BlobData.h"

#include "Blob.h"
#include "BlobURL.h"
#include "ThreadableBlobRegistry.h"

#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

const long long BlobDataItem::toEndOfFile = -1;

RawData::RawData()
{
}

void RawData::detachFromCurrentThread()
{
}

void BlobDataItem::detachFromCurrentThread()
{
    data->detachFromCurrentThread();
    path = path.isolatedCopy();
    url = url.copy();
}

PassOwnPtr<BlobData> BlobData::create()
{
    return adoptPtr(new BlobData());
}

void BlobData::detachFromCurrentThread()
{
    m_contentType = m_contentType.isolatedCopy();
    m_contentDisposition = m_contentDisposition.isolatedCopy();
    for (size_t i = 0; i < m_items.size(); ++i)
        m_items.at(i).detachFromCurrentThread();
}

void BlobData::setContentType(const String& contentType)
{
    ASSERT(Blob::isNormalizedContentType(contentType));
    m_contentType = contentType;
}

void BlobData::appendData(PassRefPtr<RawData> data, long long offset, long long length)
{
    m_items.append(BlobDataItem(data, offset, length));
}

void BlobData::appendFile(const String& path)
{
    m_items.append(BlobDataItem(path));
}

void BlobData::appendFile(const String& path, long long offset, long long length, double expectedModificationTime)
{
    m_items.append(BlobDataItem(path, offset, length, expectedModificationTime));
}

void BlobData::appendBlob(const KURL& url, long long offset, long long length)
{
    m_items.append(BlobDataItem(url, offset, length));
}

#if ENABLE(FILE_SYSTEM)
void BlobData::appendURL(const KURL& url, long long offset, long long length, double expectedModificationTime)
{
    m_items.append(BlobDataItem(url, offset, length, expectedModificationTime));
}
#endif

void BlobData::swapItems(BlobDataItemList& items)
{
    m_items.swap(items);
}


BlobDataHandle::BlobDataHandle(PassOwnPtr<BlobData> data, long long size)
{
    UNUSED_PARAM(size);
    m_internalURL = BlobURL::createInternalURL();
    ThreadableBlobRegistry::registerBlobURL(m_internalURL, data);
}

BlobDataHandle::~BlobDataHandle()
{
    ThreadableBlobRegistry::unregisterBlobURL(m_internalURL);
}

} // namespace WebCore
