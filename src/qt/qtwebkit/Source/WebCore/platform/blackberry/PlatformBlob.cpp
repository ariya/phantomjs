/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 */

#include "config.h"
#if ENABLE(FILE_SYSTEM)
#include "PlatformBlob.h"

namespace WebCore {

BlackBerry::Platform::WebBlobDataItem* PlatformBlob::nextDataItem()
{
    if (!m_blobData || m_currentItem >= m_blobData->items().size())
        return 0;

    const BlobDataItem& item = m_blobData->items()[m_currentItem++];
    ASSERT(item.type == BlobDataItem::Data || item.type == BlobDataItem::File);
    m_platformBlobItem.m_isData = item.type == BlobDataItem::Data;
    m_platformBlobItem.m_buffer = m_platformBlobItem.m_isData ? item.data->data() + static_cast<int>(item.offset) : 0;
    m_platformBlobItem.m_bufferLength = m_platformBlobItem.m_isData ? static_cast<size_t>(item.length) : 0;
    m_platformBlobItem.m_fileName = m_platformBlobItem.m_isData ? emptyString() : item.path;
    m_platformBlobItem.m_fileStart = m_platformBlobItem.m_isData ? 0 : item.offset;
    m_platformBlobItem.m_fileLength = m_platformBlobItem.m_isData ? 0 : item.length;
    m_platformBlobItem.m_expectedFileModificationTime = m_platformBlobItem.m_isData ? BlackBerry::Platform::WebBlobDataItem::DONOTCHECKFILECHANGE : item.expectedModificationTime;

    return &m_platformBlobItem;
}

} // namespace WebCore
#endif
