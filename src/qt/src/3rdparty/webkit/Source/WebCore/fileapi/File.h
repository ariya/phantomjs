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
#include "PlatformString.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class KURL;

class File : public Blob {
public:
    static PassRefPtr<File> create(const String& path)
    {
        return adoptRef(new File(path));
    }

    // For deserialization.
    static PassRefPtr<File> create(const String& path, const KURL& srcURL, const String& type)
    {
        return adoptRef(new File(path, srcURL, type));
    }

#if ENABLE(DIRECTORY_UPLOAD)
    static PassRefPtr<File> create(const String& relativePath, const String& path)
    {
        return adoptRef(new File(relativePath, path));
    }
#endif

    virtual unsigned long long size() const;
    virtual bool isFile() const { return true; }

    const String& path() const { return m_path; }
    const String& name() const { return m_name; }
    double lastModifiedDate() const;
#if ENABLE(DIRECTORY_UPLOAD)
    // Returns the relative path of this file in the context of a directory selection.
    const String& webkitRelativePath() const { return m_relativePath; }
#endif

    // Note that this involves synchronous file operation. Think twice before calling this function.
    void captureSnapshot(long long& snapshotSize, double& snapshotModificationTime) const;

    // FIXME: obsolete attributes. To be removed.
    const String& fileName() const { return name(); }
    unsigned long long fileSize() const { return size(); }

private:
    File(const String& path);
    
    // For deserialization.
    File(const String& path, const KURL& srcURL, const String& type);

#if ENABLE(DIRECTORY_UPLOAD)
    File(const String& relativePath, const String& path);
#endif

    String m_path;
    String m_name;
#if ENABLE(DIRECTORY_UPLOAD)
    String m_relativePath;
#endif
};

} // namespace WebCore

#endif // File_h
