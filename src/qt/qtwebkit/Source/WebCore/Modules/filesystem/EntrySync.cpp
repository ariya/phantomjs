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
#include "EntrySync.h"

#if ENABLE(FILE_SYSTEM)

#include "DOMFilePath.h"
#include "DOMFileSystemSync.h"
#include "DirectoryEntry.h"
#include "DirectoryEntrySync.h"
#include "FileEntrySync.h"
#include "FileException.h"
#include "Metadata.h"
#include "SyncCallbackHelper.h"

namespace WebCore {

PassRefPtr<EntrySync> EntrySync::create(EntryBase* entry)
{
    if (entry->isFile())
        return adoptRef(new FileEntrySync(entry->m_fileSystem, entry->m_fullPath));
    return adoptRef(new DirectoryEntrySync(entry->m_fileSystem, entry->m_fullPath));
}

PassRefPtr<Metadata> EntrySync::getMetadata(ExceptionCode& ec)
{
    ec = 0;
    MetadataSyncCallbackHelper helper(m_fileSystem->asyncFileSystem());
    if (!m_fileSystem->getMetadata(this, helper.successCallback(), helper.errorCallback())) {
        ec = FileException::INVALID_MODIFICATION_ERR;
        return 0;
    }
    return helper.getResult(ec);
}

PassRefPtr<EntrySync> EntrySync::moveTo(PassRefPtr<DirectoryEntrySync> parent, const String& name, ExceptionCode& ec) const
{
    ec = 0;
    EntrySyncCallbackHelper helper(m_fileSystem->asyncFileSystem());
    if (!m_fileSystem->move(this, parent.get(), name, helper.successCallback(), helper.errorCallback())) {
        ec = FileException::INVALID_MODIFICATION_ERR;
        return 0;
    }
    return helper.getResult(ec);
}

PassRefPtr<EntrySync> EntrySync::copyTo(PassRefPtr<DirectoryEntrySync> parent, const String& name, ExceptionCode& ec) const
{
    ec = 0;
    EntrySyncCallbackHelper helper(m_fileSystem->asyncFileSystem());
    if (!m_fileSystem->copy(this, parent.get(), name, helper.successCallback(), helper.errorCallback())) {
        ec = FileException::INVALID_MODIFICATION_ERR;
        return 0;
    }
    return helper.getResult(ec);
}

void EntrySync::remove(ExceptionCode& ec) const
{
    ec = 0;
    VoidSyncCallbackHelper helper(m_fileSystem->asyncFileSystem());
    if (!m_fileSystem->remove(this, helper.successCallback(), helper.errorCallback())) {
        ec = FileException::INVALID_MODIFICATION_ERR;
        return;
    }
    helper.getResult(ec);
}

PassRefPtr<EntrySync> EntrySync::getParent() const
{
    // Sync verion of getParent doesn't throw exceptions.
    String parentPath = DOMFilePath::getDirectory(fullPath());
    return DirectoryEntrySync::create(m_fileSystem, parentPath);
}

EntrySync::EntrySync(PassRefPtr<DOMFileSystemBase> fileSystem, const String& fullPath)
    : EntryBase(fileSystem, fullPath)
{
}

}

#endif // ENABLE(FILE_SYSTEM)
