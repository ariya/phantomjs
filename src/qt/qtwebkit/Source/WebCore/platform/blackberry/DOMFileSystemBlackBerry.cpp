/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 */

#include "config.h"
#if ENABLE(FILE_SYSTEM)
#include "DOMFileSystem.h"

#include "DOMFilePath.h"
#include "ScriptExecutionContext.h"

#include <wtf/text/WTFString.h>

namespace WebCore {

KURL DOMFileSystemBase::createFileSystemURL(const String& fullPath) const
{
    ASSERT(DOMFilePath::isAbsolute(fullPath));

    // For regular types we can just append the entry's fullPath to the m_filesystemRootURL that should look like 'filesystem:<origin>/<typePrefix>'.
    ASSERT(!m_filesystemRootURL.isEmpty());
    return m_context->completeURL(m_filesystemRootURL.string() + encodeWithURLEscapeSequences(fullPath));
}

bool DOMFileSystemBase::crackFileSystemURL(const KURL& url, FileSystemType& type, String& filePath)
{
    if (!url.protocolIs("filesystem"))
        return false;

    if (url.innerURL()) {
        String typeString = url.innerURL()->path().substring(1);
        if (typeString == temporaryPathPrefix)
            type = FileSystemTypeTemporary;
        else if (typeString == persistentPathPrefix)
            type = FileSystemTypePersistent;
        else
            return false;

        filePath = decodeURLEscapeSequences(url.path());
    } else {
        KURL originURL(ParsedURLString, url.path());
        String path = decodeURLEscapeSequences(originURL.path());
        if (path.isEmpty() || path[0] != '/')
            return false;
        path = path.substring(1);

        if (path.startsWith(temporaryPathPrefix)) {
            type = FileSystemTypeTemporary;
            path = path.substring(temporaryPathPrefixLength);
        } else if (path.startsWith(persistentPathPrefix)) {
            type = FileSystemTypePersistent;
            path = path.substring(persistentPathPrefixLength);
        } else
            return false;

        if (path.isEmpty() || path[0] != '/')
            return false;

        filePath.swap(path);
    }
    return true;
}

bool DOMFileSystemBase::supportsToURL() const
{
    return true;
}

bool DOMFileSystemBase::isValidType(FileSystemType type)
{
    return type == FileSystemTypeTemporary || type == FileSystemTypePersistent;
}

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)
