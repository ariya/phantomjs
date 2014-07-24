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
#include "DirectoryEntry.h"

#if ENABLE(FILE_SYSTEM)

#include "DirectoryReader.h"
#include "EntryCallback.h"
#include "ErrorCallback.h"
#include "FileError.h"
#include "VoidCallback.h"

namespace WebCore {

DirectoryEntry::DirectoryEntry(PassRefPtr<DOMFileSystemBase> fileSystem, const String& fullPath)
    : Entry(fileSystem, fullPath)
{
}

PassRefPtr<DirectoryReader> DirectoryEntry::createReader()
{
    return DirectoryReader::create(m_fileSystem, m_fullPath);
}

void DirectoryEntry::getFile(const String& path, const Dictionary& options, PassRefPtr<EntryCallback> successCallback, PassRefPtr<ErrorCallback> errorCallbackRef)
{
    FileSystemFlags flags(options);
    RefPtr<ErrorCallback> errorCallback(errorCallbackRef);
    if (!m_fileSystem->getFile(this, path, flags, successCallback, errorCallback))
        filesystem()->scheduleCallback(errorCallback.release(), FileError::create(FileError::INVALID_MODIFICATION_ERR));
}

void DirectoryEntry::getDirectory(const String& path, const Dictionary& options, PassRefPtr<EntryCallback> successCallback, PassRefPtr<ErrorCallback> errorCallbackRef)
{
    FileSystemFlags flags(options);
    RefPtr<ErrorCallback> errorCallback(errorCallbackRef);
    if (!m_fileSystem->getDirectory(this, path, flags, successCallback, errorCallback))
        filesystem()->scheduleCallback(errorCallback.release(), FileError::create(FileError::INVALID_MODIFICATION_ERR));
}

void DirectoryEntry::removeRecursively(PassRefPtr<VoidCallback> successCallback, PassRefPtr<ErrorCallback> errorCallbackRef) const
{
    RefPtr<ErrorCallback> errorCallback(errorCallbackRef);
    if (!m_fileSystem->removeRecursively(this, successCallback, errorCallback))
        filesystem()->scheduleCallback(errorCallback.release(), FileError::create(FileError::INVALID_MODIFICATION_ERR));
}

}

#endif // ENABLE(FILE_SYSTEM)
