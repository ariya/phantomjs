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
#ifndef PlatformBlob_h
#define PlatformBlob_h

#if ENABLE(FILE_SYSTEM)
#include "Assertions.h"
#include "Blob.h"
#include "BlobRegistryImpl.h"
#include "BlobStorageData.h"

#include <BlackBerryPlatformWebFileSystemFileWriter.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class Blob;

class PlatformBlob: public BlackBerry::Platform::WebBlob {
public:
    PlatformBlob(Blob* blob)
        : m_currentItem(0)
    {
        ASSERT(blob);
        m_blobData = static_cast<BlobRegistryImpl&>(blobRegistry()).getBlobDataFromURL(blob->url());
    }

    virtual BlackBerry::Platform::WebBlobDataItem* nextDataItem();

private:
    RefPtr<BlobStorageData> m_blobData;
    BlackBerry::Platform::WebBlobDataItem m_platformBlobItem;
    unsigned long long m_currentItem;
};

} // namespace WebCore
#endif
#endif
