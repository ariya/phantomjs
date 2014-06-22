/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef FileChooser_h
#define FileChooser_h

#include <wtf/RefCounted.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class FileChooser;

struct FileChooserFileInfo {
    FileChooserFileInfo(const String& path, const String& displayName = String())
        : path(path)
        , displayName(displayName)
    {
    }

    const String path;
    const String displayName;
};

struct FileChooserSettings {
    bool allowsMultipleFiles;
#if ENABLE(DIRECTORY_UPLOAD)
    bool allowsDirectoryUpload;
#endif
    Vector<String> acceptMIMETypes;
    Vector<String> acceptFileExtensions;
    Vector<String> selectedFiles;
#if ENABLE(MEDIA_CAPTURE)
    String capture;
#endif

    // Returns a combined vector of acceptMIMETypes and acceptFileExtensions.
    Vector<String> acceptTypes() const;
};

class FileChooserClient {
public:
    virtual ~FileChooserClient() { }

    virtual void filesChosen(const Vector<FileChooserFileInfo>&) = 0;
};

class FileChooser : public RefCounted<FileChooser> {
public:
    static PassRefPtr<FileChooser> create(FileChooserClient*, const FileChooserSettings&);
    ~FileChooser();

    void invalidate();

    void chooseFile(const String& path);
    void chooseFiles(const Vector<String>& paths);

    // FIXME: We should probably just pass file paths that could be virtual paths with proper display names rather than passing structs.
    void chooseFiles(const Vector<FileChooserFileInfo>& files);

    const FileChooserSettings& settings() const { return m_settings; }

private:
    FileChooser(FileChooserClient*, const FileChooserSettings&);

    FileChooserClient* m_client;
    FileChooserSettings m_settings;
};

} // namespace WebCore

#endif // FileChooser_h
