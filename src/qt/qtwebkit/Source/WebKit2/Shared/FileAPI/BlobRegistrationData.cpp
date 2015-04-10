/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "BlobRegistrationData.h"

#if ENABLE(BLOB) && ENABLE(NETWORK_PROCESS)

#include "ArgumentCoders.h"
#include "DataReference.h"
#include "WebCoreArgumentCoders.h"
#include <WebCore/BlobData.h>

using namespace WebCore;

namespace WebKit {

BlobRegistrationData::BlobRegistrationData()
{
}

BlobRegistrationData::BlobRegistrationData(PassOwnPtr<BlobData> data)
    : m_data(data)
{
    const BlobDataItemList& items = m_data->items();
    size_t fileCount = 0;
    for (size_t i = 0, count = items.size(); i < count; ++i) {
        // File path can be empty when submitting a form file input without a file, see bug 111778.
        if (items[i].type == BlobDataItem::File && !items[i].path.isEmpty())
            ++fileCount;
    }

    m_sandboxExtensions.allocate(fileCount);
    size_t extensionIndex = 0;
    for (size_t i = 0, count = items.size(); i < count; ++i) {
        const BlobDataItem& item = items[i];
        if (item.type == BlobDataItem::File && !items[i].path.isEmpty())
            SandboxExtension::createHandle(item.path, SandboxExtension::ReadOnly, m_sandboxExtensions[extensionIndex++]);
    }
}

BlobRegistrationData::~BlobRegistrationData()
{
}

PassOwnPtr<BlobData> BlobRegistrationData::releaseData() const
{
    return m_data.release();
}

void BlobRegistrationData::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << m_data->contentType();
    encoder << m_data->contentDisposition();

    const BlobDataItemList& items = m_data->items();
    encoder << static_cast<uint64_t>(items.size());
    for (size_t i = 0, count = items.size(); i < count; ++i) {
        const BlobDataItem& item = items[i];
        encoder << static_cast<uint32_t>(item.type);
        switch (item.type) {
        case BlobDataItem::Data:
            // There is no way to create a partial data item.
            ASSERT(!item.offset);
            ASSERT(item.length == BlobDataItem::toEndOfFile);
            encoder << CoreIPC::DataReference(reinterpret_cast<const uint8_t*>(item.data->data()), item.data->length());
            break;
        case BlobDataItem::File:
            encoder << static_cast<int64_t>(item.offset);
            encoder << static_cast<int64_t>(item.length);
            encoder << item.expectedModificationTime;
            encoder << item.path;
            break;
        case BlobDataItem::Blob:
            encoder << static_cast<int64_t>(item.offset);
            encoder << static_cast<int64_t>(item.length);
            encoder << item.url;
            break;
        }
    }

    encoder << m_sandboxExtensions;
}

bool BlobRegistrationData::decode(CoreIPC::ArgumentDecoder& decoder, BlobRegistrationData& result)
{
    ASSERT(!result.m_data);
    result.m_data = BlobData::create();

    String contentType;
    if (!decoder.decode(contentType))
        return false;
    result.m_data->setContentType(contentType);

    String contentDisposition;
    if (!decoder.decode(contentDisposition))
        return false;
    result.m_data->setContentDisposition(contentDisposition);

    uint64_t itemCount;
    if (!decoder.decode(itemCount))
        return false;

    for (uint64_t i = 0; i < itemCount; ++i) {
        uint32_t type;
        if (!decoder.decode(type))
            return false;
        switch (type) {
        case BlobDataItem::Data: {
            CoreIPC::DataReference data;
            if (!decoder.decode(data))
                return false;
            RefPtr<RawData> rawData = RawData::create();
            rawData->mutableData()->append(data.data(), data.size());
            result.m_data->appendData(rawData.release(), 0, BlobDataItem::toEndOfFile);
            break;
        }
        case BlobDataItem::File: {
            int64_t offset;
            if (!decoder.decode(offset))
                return false;
            int64_t length;
            if (!decoder.decode(length))
                return false;
            double expectedModificationTime;
            if (!decoder.decode(expectedModificationTime))
                return false;
            String path;
            if (!decoder.decode(path))
                return false;
            result.m_data->appendFile(path, offset, length, expectedModificationTime);
            break;
        }
        case BlobDataItem::Blob: {
            int64_t offset;
            if (!decoder.decode(offset))
                return false;
            int64_t length;
            if (!decoder.decode(length))
                return false;
            String url;
            if (!decoder.decode(url))
                return false;
            result.m_data->appendBlob(KURL(KURL(), url), offset, length);
            break;
        }
        default:
            return false;
        }
    }

    if (!decoder.decode(result.m_sandboxExtensions))
        return false;

    return true;
}

}

#endif // ENABLE(BLOB) && ENABLE(NETWORK_PROCESS)
