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

#include "WebKitBlobBuilder.h"

#include "ArrayBuffer.h"
#include "Blob.h"
#include "ExceptionCode.h"
#include "File.h"
#include "LineEnding.h"
#include "TextEncoding.h"
#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/AtomicString.h>
#include <wtf/text/CString.h>

namespace WebCore {

WebKitBlobBuilder::WebKitBlobBuilder()
    : m_size(0)
{
}

Vector<char>& WebKitBlobBuilder::getBuffer()
{
    // If the last item is not a data item, create one. Otherwise, we simply append the new string to the last data item.
    if (m_items.isEmpty() || m_items[m_items.size() - 1].type != BlobDataItem::Data)
        m_items.append(BlobDataItem(RawData::create()));

    return *m_items[m_items.size() - 1].data->mutableData();
}

void WebKitBlobBuilder::append(const String& text, const String& endingType, ExceptionCode& ec)
{
    bool isEndingTypeTransparent = endingType == "transparent";
    bool isEndingTypeNative = endingType == "native";
    if (!endingType.isEmpty() && !isEndingTypeTransparent && !isEndingTypeNative) {
        ec = SYNTAX_ERR;
        return;
    }

    CString utf8Text = UTF8Encoding().encode(text.characters(), text.length(), EntitiesForUnencodables);

    Vector<char>& buffer = getBuffer();
    size_t oldSize = buffer.size();

    if (isEndingTypeNative)
        normalizeLineEndingsToNative(utf8Text, buffer);
    else
        buffer.append(utf8Text.data(), utf8Text.length());
    m_size += buffer.size() - oldSize;
}

void WebKitBlobBuilder::append(const String& text, ExceptionCode& ec)
{
    append(text, String(), ec);
}

#if ENABLE(BLOB)
void WebKitBlobBuilder::append(ArrayBuffer* arrayBuffer)
{
    if (!arrayBuffer)
        return;
    Vector<char>& buffer = getBuffer();
    size_t oldSize = buffer.size();
    buffer.append(static_cast<const char*>(arrayBuffer->data()), arrayBuffer->byteLength());
    m_size += buffer.size() - oldSize;
}
#endif

void WebKitBlobBuilder::append(Blob* blob)
{
    if (!blob)
        return;
    if (blob->isFile()) {
        // If the blob is file that is not snapshoted, capture the snapshot now.
        // FIXME: This involves synchronous file operation. We need to figure out how to make it asynchronous.
        File* file = static_cast<File*>(blob);
        long long snapshotSize;
        double snapshotModificationTime;
        file->captureSnapshot(snapshotSize, snapshotModificationTime);

        m_size += snapshotSize;
        m_items.append(BlobDataItem(file->path(), 0, snapshotSize, snapshotModificationTime));
    } else {
        long long blobSize = static_cast<long long>(blob->size());
        m_size += blobSize;
        m_items.append(BlobDataItem(blob->url(), 0, blobSize));
    }
}

PassRefPtr<Blob> WebKitBlobBuilder::getBlob(const String& contentType)
{
    OwnPtr<BlobData> blobData = BlobData::create();
    blobData->setContentType(contentType);
    blobData->swapItems(m_items);

    RefPtr<Blob> blob = Blob::create(blobData.release(), m_size);

    // After creating a blob from the current blob data, we do not need to keep the data around any more. Instead, we only
    // need to keep a reference to the URL of the blob just created.
    m_items.append(BlobDataItem(blob->url(), 0, m_size));

    return blob;
}

} // namespace WebCore
