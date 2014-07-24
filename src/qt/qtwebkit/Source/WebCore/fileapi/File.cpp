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

#include "FileMetadata.h"
#include "FileSystem.h"
#include "MIMETypeRegistry.h"
#include <wtf/CurrentTime.h>
#include <wtf/DateMath.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

static String getContentTypeFromFileName(const String& name, File::ContentTypeLookupPolicy policy)
{
    String type;
    int index = name.reverseFind('.');
    if (index != -1) {
        if (policy == File::WellKnownContentTypes)
            type = MIMETypeRegistry::getWellKnownMIMETypeForExtension(name.substring(index + 1));
        else {
            ASSERT(policy == File::AllContentTypes);
            type = MIMETypeRegistry::getMIMETypeForExtension(name.substring(index + 1));
        }
    }
    return type;
}

static PassOwnPtr<BlobData> createBlobDataForFileWithType(const String& path, const String& contentType)
{
    OwnPtr<BlobData> blobData = BlobData::create();
    ASSERT(Blob::isNormalizedContentType(contentType));
    blobData->setContentType(contentType);
    blobData->appendFile(path);
    return blobData.release();
}

static PassOwnPtr<BlobData> createBlobDataForFile(const String& path, File::ContentTypeLookupPolicy policy)
{
    return createBlobDataForFileWithType(path, getContentTypeFromFileName(path, policy));
}

static PassOwnPtr<BlobData> createBlobDataForFileWithName(const String& path, const String& fileSystemName, File::ContentTypeLookupPolicy policy)
{
    return createBlobDataForFileWithType(path, getContentTypeFromFileName(fileSystemName, policy));
}

#if ENABLE(FILE_SYSTEM)
static PassOwnPtr<BlobData> createBlobDataForFileWithMetadata(const String& fileSystemName, const FileMetadata& metadata)
{
    OwnPtr<BlobData> blobData = BlobData::create();
    blobData->setContentType(getContentTypeFromFileName(fileSystemName, File::WellKnownContentTypes));
    blobData->appendFile(metadata.platformPath, 0, metadata.length, metadata.modificationTime);
    return blobData.release();
}

static PassOwnPtr<BlobData> createBlobDataForFileSystemURL(const KURL& fileSystemURL, const FileMetadata& metadata)
{
    OwnPtr<BlobData> blobData = BlobData::create();
    blobData->setContentType(getContentTypeFromFileName(fileSystemURL.path(), File::WellKnownContentTypes));
    blobData->appendURL(fileSystemURL, 0, metadata.length, metadata.modificationTime);
    return blobData.release();
}
#endif

#if ENABLE(DIRECTORY_UPLOAD)
PassRefPtr<File> File::createWithRelativePath(const String& path, const String& relativePath)
{
    RefPtr<File> file = adoptRef(new File(path, AllContentTypes));
    file->m_relativePath = relativePath;
    return file.release();
}
#endif

File::File(const String& path, ContentTypeLookupPolicy policy)
    : Blob(createBlobDataForFile(path, policy), -1)
    , m_path(path)
    , m_name(pathGetFileName(path))
#if ENABLE(FILE_SYSTEM)
    , m_snapshotSize(-1)
    , m_snapshotModificationTime(invalidFileTime())
#endif
{
}

File::File(const String& path, const KURL& url, const String& type)
    : Blob(url, type, -1)
    , m_path(path)
#if ENABLE(FILE_SYSTEM)
    , m_snapshotSize(-1)
    , m_snapshotModificationTime(invalidFileTime())
#endif
{
    m_name = pathGetFileName(path);
    // FIXME: File object serialization/deserialization does not include
    // newer file object data members: m_name and m_relativePath.
    // See SerializedScriptValue.cpp
}

File::File(const String& path, const String& name, ContentTypeLookupPolicy policy)
    : Blob(createBlobDataForFileWithName(path, name, policy), -1)
    , m_path(path)
    , m_name(name)
#if ENABLE(FILE_SYSTEM)
    , m_snapshotSize(-1)
    , m_snapshotModificationTime(invalidFileTime())
#endif
{
}

#if ENABLE(FILE_SYSTEM)
File::File(const String& name, const FileMetadata& metadata)
    : Blob(createBlobDataForFileWithMetadata(name, metadata), metadata.length)
    , m_path(metadata.platformPath)
    , m_name(name)
    , m_snapshotSize(metadata.length)
    , m_snapshotModificationTime(metadata.modificationTime)
{
}

File::File(const KURL& fileSystemURL, const FileMetadata& metadata)
    : Blob(createBlobDataForFileSystemURL(fileSystemURL, metadata), metadata.length)
    , m_fileSystemURL(fileSystemURL)
    , m_snapshotSize(metadata.length)
    , m_snapshotModificationTime(metadata.modificationTime)
{
}
#endif

double File::lastModifiedDate() const
{
#if ENABLE(FILE_SYSTEM)
    if (hasValidSnapshotMetadata() && isValidFileTime(m_snapshotModificationTime))
        return m_snapshotModificationTime * msPerSecond;
#endif

    time_t modificationTime;
    if (getFileModificationTime(m_path, modificationTime) && isValidFileTime(modificationTime))
        return modificationTime * msPerSecond;

    return currentTime() * msPerSecond;
}

unsigned long long File::size() const
{
#if ENABLE(FILE_SYSTEM)
    if (hasValidSnapshotMetadata())
        return m_snapshotSize;
#endif

    // FIXME: JavaScript cannot represent sizes as large as unsigned long long, we need to
    // come up with an exception to throw if file size is not representable.
    long long size;
    if (!getFileSize(m_path, size))
        return 0;
    return static_cast<unsigned long long>(size);
}

void File::captureSnapshot(long long& snapshotSize, double& snapshotModificationTime) const
{
#if ENABLE(FILE_SYSTEM)
    if (hasValidSnapshotMetadata()) {
        snapshotSize = m_snapshotSize;
        snapshotModificationTime = m_snapshotModificationTime;
        return;
    }
#endif

    // Obtains a snapshot of the file by capturing its current size and modification time. This is used when we slice a file for the first time.
    // If we fail to retrieve the size or modification time, probably due to that the file has been deleted, 0 size is returned.
    FileMetadata metadata;
    if (!getFileMetadata(m_path, metadata)) {
        snapshotSize = 0;
        snapshotModificationTime = invalidFileTime();
        return;
    }

    snapshotSize = metadata.length;
    snapshotModificationTime = metadata.modificationTime;
}

} // namespace WebCore
