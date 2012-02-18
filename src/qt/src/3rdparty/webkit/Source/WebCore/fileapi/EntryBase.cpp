/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
#include "EntryBase.h"

#if ENABLE(FILE_SYSTEM)

#include "AsyncFileSystem.h"
#include "DOMFilePath.h"
#include "DOMFileSystemBase.h"
#include "PlatformString.h"
#include "SecurityOrigin.h"
#include <wtf/PassRefPtr.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

EntryBase::EntryBase(PassRefPtr<DOMFileSystemBase> fileSystem, const String& fullPath)
    : m_fileSystem(fileSystem)
    , m_fullPath(fullPath)
    , m_name(DOMFilePath::getName(fullPath))
{
}

EntryBase::~EntryBase()
{
}

String EntryBase::toURL()
{
    String originString = m_fileSystem->securityOrigin()->toString();
    ASSERT(!originString.isEmpty());
    if (originString == "null")
        return String();
    StringBuilder result;
    result.append("filesystem:");
    result.append(originString);
    result.append("/");
    switch (m_fileSystem->asyncFileSystem()->type()) {
    case AsyncFileSystem::Temporary:
        result.append(DOMFileSystemBase::kTemporaryPathPrefix);
        break;
    case AsyncFileSystem::Persistent:
        result.append(DOMFileSystemBase::kPersistentPathPrefix);
        break;
    case AsyncFileSystem::External:
        result.append(DOMFileSystemBase::kExternalPathPrefix);
        break;
    }
    result.append(m_fullPath);
    return result.toString();
}

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)
