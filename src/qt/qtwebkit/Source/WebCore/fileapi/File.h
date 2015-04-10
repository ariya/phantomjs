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

#ifndef File_h
#define File_h

#include "Blob.h"
#include <wtf/PassRefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

struct FileMetadata;
class KURL;

class File : public Blob {
public:
    // AllContentTypes should only be used when the full path/name are trusted; otherwise, it could
    // allow arbitrary pages to determine what applications an user has installed.
    enum ContentTypeLookupPolicy {
        WellKnownContentTypes,
        AllContentTypes,
    };

    static PassRefPtr<File> create(const String& path, ContentTypeLookupPolicy policy = WellKnownContentTypes)
    {
        return adoptRef(new File(path, policy));
    }

    // For deserialization.
    static PassRefPtr<File> create(const String& path, const KURL& srcURL, const String& type)
    {
        return adoptRef(new File(path, srcURL, type));
    }

#if ENABLE(DIRECTORY_UPLOAD)
    static PassRefPtr<File> createWithRelativePath(const String& path, const String& relativePath);
#endif

#if ENABLE(FILE_SYSTEM)
    // If filesystem files live in the remote filesystem, the port might pass the valid metadata (whose length field is non-negative) and cache in the File object.
    //
    // Otherwise calling size(), lastModifiedTime() and slice() will synchronously query the file metadata.
    static PassRefPtr<File> createForFileSystemFile(const String& name, const FileMetadata& metadata)
    {
        return adoptRef(new File(name, metadata));
    }

    static PassRefPtr<File> createForFileSystemFile(const KURL& url, const FileMetadata& metadata)
    {
        return adoptRef(new File(url, metadata));
    }

    KURL fileSystemURL() const { return m_fileSystemURL; }
#endif

    // Create a file with a name exposed to the author (via File.name and associated DOM properties) that differs from the one provided in the path.
    static PassRefPtr<File> createWithName(const String& path, const String& name, ContentTypeLookupPolicy policy = WellKnownContentTypes)
    {
        if (name.isEmpty())
            return adoptRef(new File(path, policy));
        return adoptRef(new File(path, name, policy));
    }

    virtual unsigned long long size() const;
    virtual bool isFile() const { return true; }

    const String& path() const { return m_path; }
    const String& name() const { return m_name; }

    // This returns the current date and time if the file's last modifiecation date is not known (per spec: http://www.w3.org/TR/FileAPI/#dfn-lastModifiedDate).
    double lastModifiedDate() const;

#if ENABLE(DIRECTORY_UPLOAD)
    // Returns the relative path of this file in the context of a directory selection.
    const String& webkitRelativePath() const { return m_relativePath; }
#endif

    // Note that this involves synchronous file operation. Think twice before calling this function.
    void captureSnapshot(long long& snapshotSize, double& snapshotModificationTime) const;

private:
    File(const String& path, ContentTypeLookupPolicy);

    // For deserialization.
    File(const String& path, const KURL& srcURL, const String& type);
    File(const String& path, const String& name, ContentTypeLookupPolicy);

# if ENABLE(FILE_SYSTEM)
    File(const String& name, const FileMetadata&);
    File(const KURL& fileSystemURL, const FileMetadata&);

    // Returns true if this has a valid snapshot metadata (i.e. m_snapshotSize >= 0).
    bool hasValidSnapshotMetadata() const { return m_snapshotSize >= 0; }
#endif

    String m_path;
    String m_name;

#if ENABLE(FILE_SYSTEM)
    KURL m_fileSystemURL;

    // If m_snapshotSize is negative (initialized to -1 by default), the snapshot metadata is invalid and we retrieve the latest metadata synchronously in size(), lastModifiedTime() and slice().
    // Otherwise, the snapshot metadata are used directly in those methods.
    const long long m_snapshotSize;
    const double m_snapshotModificationTime;
#endif

#if ENABLE(DIRECTORY_UPLOAD)
    String m_relativePath;
#endif
};

inline File* toFile(Blob* blob)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!blob || blob->isFile());
    return static_cast<File*>(blob);
}

inline const File* toFile(const Blob* blob)
{
    ASSERT_WITH_SECURITY_IMPLICATION(!blob || blob->isFile());
    return static_cast<const File*>(blob);
}

} // namespace WebCore

#endif // File_h
