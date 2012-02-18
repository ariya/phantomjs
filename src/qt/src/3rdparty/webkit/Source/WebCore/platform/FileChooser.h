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

#include "PlatformString.h"
#include <wtf/Vector.h>

namespace WebCore {

class FileChooser;
class Font;
class Icon;

class FileChooserClient {
public:
    virtual void valueChanged() = 0;
    virtual void repaint() = 0;
    virtual bool allowsMultipleFiles() = 0;
#if ENABLE(DIRECTORY_UPLOAD)
    virtual bool allowsDirectoryUpload() = 0;
#endif
    virtual String acceptTypes() = 0;
    virtual void chooseIconForFiles(FileChooser*, const Vector<String>&) = 0;
    virtual ~FileChooserClient();
};

class FileChooser : public RefCounted<FileChooser> {
public:
    static PassRefPtr<FileChooser> create(FileChooserClient*, const Vector<String>& initialFilenames);
    ~FileChooser();

    void disconnectClient() { m_client = 0; }
    bool disconnected() { return !m_client; }

    const Vector<String>& filenames() const { return m_filenames; }
    String basenameForWidth(const Font&, int width) const;

    Icon* icon() const { return m_icon.get(); }

    void clear(); // for use by client; does not call valueChanged

    void chooseFile(const String& path);
    void chooseFiles(const Vector<String>& paths);
    // Called when FileChooserClient finishes to load an icon requested by iconForFiles().
    void iconLoaded(PassRefPtr<Icon>);

    bool allowsMultipleFiles() const { return m_client ? m_client->allowsMultipleFiles() : false; }
#if ENABLE(DIRECTORY_UPLOAD)
    bool allowsDirectoryUpload() const { return m_client ? m_client->allowsDirectoryUpload() : false; }
#endif
    // Acceptable MIME types.  It's an 'accept' attribute value of the corresponding INPUT element.
    String acceptTypes() const { return m_client ? m_client->acceptTypes() : String(); }

private:
    FileChooser(FileChooserClient*, const Vector<String>& initialFilenames);
    void initialize();
    void loadIcon();

    FileChooserClient* m_client;
    Vector<String> m_filenames;
    RefPtr<Icon> m_icon;
    bool m_isInitializing;
};

} // namespace WebCore

#endif // FileChooser_h
