/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "File.h"

#include "FileSystem.h"
#include "MIMETypeRegistry.h"
#include <wtf/CurrentTime.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

static PassOwnPtr<BlobData> createBlobDataForFile(const String& path)
{
    String type;
    int index = path.reverseFind('.');
    if (index != -1)
        type = MIMETypeRegistry::getMIMETypeForExtension(path.substring(index + 1));

    OwnPtr<BlobData> blobData = BlobData::create();
    blobData->setContentType(type);
    blobData->appendFile(path);
    return blobData.release();
}

File::File(const String& path)
    : Blob(createBlobDataForFile(path), -1)
    , m_path(path)
    , m_name(pathGetFileName(path))
{
}

File::File(const String& path, const KURL& url, const String& type)
    : Blob(url, type, -1)
    , m_path(path)
{
    m_name = pathGetFileName(path);
}

#if ENABLE(DIRECTORY_UPLOAD)
File::File(const String& relativePath, const String& path)
    : Blob(createBlobDataForFile(path), -1)
    , m_path(path)
    , m_relativePath(relativePath)
{
    m_name = pathGetFileName(path);
}
#endif

double File::lastModifiedDate() const
{
    time_t modificationTime;
    if (!getFileModificationTime(m_path, modificationTime))
        return 0;

    // Needs to return epoch time in milliseconds for Date.
    return modificationTime * 1000.0;
}

unsigned long long File::size() const
{
    // FIXME: JavaScript cannot represent sizes as large as unsigned long long, we need to
    // come up with an exception to throw if file size is not representable.
    long long size;
    if (!getFileSize(m_path, size))
        return 0;
    return static_cast<unsigned long long>(size);
}

void File::captureSnapshot(long long& snapshotSize, double& snapshotModificationTime) const
{
    // Obtains a snapshot of the file by capturing its current size and modification time. This is used when we slice a file for the first time.
    // If we fail to retrieve the size or modification time, probably due to that the file has been deleted, 0 size is returned.
    // FIXME: Combine getFileSize and getFileModificationTime into one file system call.
    time_t modificationTime;
    if (!getFileSize(m_path, snapshotSize) || !getFileModificationTime(m_path, modificationTime)) {
        snapshotSize = 0;
        snapshotModificationTime = 0;
    } else
        snapshotModificationTime = modificationTime;
}

} // namespace WebCore
