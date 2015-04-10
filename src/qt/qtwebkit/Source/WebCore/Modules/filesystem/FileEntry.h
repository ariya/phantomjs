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

#ifndef FileEntry_h
#define FileEntry_h

#if ENABLE(FILE_SYSTEM)

#include "Entry.h"

namespace WebCore {

class DOMFileSystemBase;
class FileCallback;
class FileWriterCallback;

class FileEntry : public Entry {
public:
    static PassRefPtr<FileEntry> create(PassRefPtr<DOMFileSystemBase> fileSystem, const String& fullPath)
    {
        return adoptRef(new FileEntry(fileSystem, fullPath));
    }

    void createWriter(PassRefPtr<FileWriterCallback>, PassRefPtr<ErrorCallback> = 0);
    void file(PassRefPtr<FileCallback>, PassRefPtr<ErrorCallback> = 0);

    virtual bool isFile() const { return true; }

private:
    FileEntry(PassRefPtr<DOMFileSystemBase>, const String& fullPath);
};

} // namespace

#endif // ENABLE(FILE_SYSTEM)

#endif // FileEntry_h
