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
#include "DOMFileSystemBase.h"

#if ENABLE(FILE_SYSTEM)

#include "DOMFilePath.h"
#include "DirectoryEntry.h"
#include "DirectoryReaderBase.h"
#include "EntriesCallback.h"
#include "EntryArray.h"
#include "EntryBase.h"
#include "EntryCallback.h"
#include "ErrorCallback.h"
#include "FileError.h"
#include "FileSystemCallbacks.h"
#include "MetadataCallback.h"
#include "ScriptExecutionContext.h"
#include "VoidCallback.h"
#include <wtf/OwnPtr.h>

namespace WebCore {

const char DOMFileSystemBase::persistentPathPrefix[] = "persistent";
const size_t DOMFileSystemBase::persistentPathPrefixLength = sizeof(DOMFileSystemBase::persistentPathPrefix) - 1;
const char DOMFileSystemBase::temporaryPathPrefix[] = "temporary";
const size_t DOMFileSystemBase::temporaryPathPrefixLength = sizeof(DOMFileSystemBase::temporaryPathPrefix) - 1;
const char DOMFileSystemBase::isolatedPathPrefix[] = "isolated";
const size_t DOMFileSystemBase::isolatedPathPrefixLength = sizeof(DOMFileSystemBase::isolatedPathPrefix) - 1;

DOMFileSystemBase::DOMFileSystemBase(ScriptExecutionContext* context, const String& name, FileSystemType type, const KURL& rootURL, PassOwnPtr<AsyncFileSystem> asyncFileSystem)
    : m_context(context)
    , m_name(name)
    , m_type(type)
    , m_filesystemRootURL(rootURL)
    , m_clonable(false)
    , m_asyncFileSystem(asyncFileSystem)
{
}

DOMFileSystemBase::~DOMFileSystemBase()
{
}

#if !PLATFORM(BLACKBERRY)
// static
bool DOMFileSystemBase::isValidType(FileSystemType type)
{
    return type == FileSystemTypeTemporary || type == FileSystemTypePersistent;
}

// static
bool DOMFileSystemBase::crackFileSystemURL(const KURL& url, FileSystemType& type, String& filePath)
{
    if (!url.protocolIs("filesystem"))
        return false;

    if (!url.innerURL())
        return false;

    String typeString = url.innerURL()->path().substring(1);
    if (typeString == temporaryPathPrefix)
        type = FileSystemTypeTemporary;
    else if (typeString == persistentPathPrefix)
        type = FileSystemTypePersistent;
    else
        return false;

    filePath = decodeURLEscapeSequences(url.path());
    return true;
}

bool DOMFileSystemBase::supportsToURL() const
{
    ASSERT(isValidType(m_type));
    return true;
}

KURL DOMFileSystemBase::createFileSystemURL(const String& fullPath) const
{
    ASSERT(DOMFilePath::isAbsolute(fullPath));
    KURL url = m_filesystemRootURL;
    // Remove the extra leading slash.
    url.setPath(url.path() + encodeWithURLEscapeSequences(fullPath.substring(1)));
    return url;
}
#endif

SecurityOrigin* DOMFileSystemBase::securityOrigin() const
{
    return m_context->securityOrigin();
}

KURL DOMFileSystemBase::createFileSystemURL(const EntryBase* entry) const
{
    return createFileSystemURL(entry->fullPath());
}

bool DOMFileSystemBase::getMetadata(const EntryBase* entry, PassRefPtr<MetadataCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
{
    m_asyncFileSystem->readMetadata(createFileSystemURL(entry), MetadataCallbacks::create(successCallback, errorCallback));
    return true;
}

static bool verifyAndGetDestinationPathForCopyOrMove(const EntryBase* source, EntryBase* parent, const String& newName, String& destinationPath)
{
    ASSERT(source);

    if (!parent || !parent->isDirectory())
        return false;

    if (!newName.isEmpty() && !DOMFilePath::isValidName(newName))
        return false;

    const bool isSameFileSystem = (*source->filesystem() == *parent->filesystem());

    // It is an error to try to copy or move an entry inside itself at any depth if it is a directory.
    if (source->isDirectory() && isSameFileSystem && DOMFilePath::isParentOf(source->fullPath(), parent->fullPath()))
        return false;

    // It is an error to copy or move an entry into its parent if a name different from its current one isn't provided.
    if (isSameFileSystem && (newName.isEmpty() || source->name() == newName) && DOMFilePath::getDirectory(source->fullPath()) == parent->fullPath())
        return false;

    destinationPath = parent->fullPath();
    if (!newName.isEmpty())
        destinationPath = DOMFilePath::append(destinationPath, newName);
    else
        destinationPath = DOMFilePath::append(destinationPath, source->name());

    return true;
}

static bool pathToAbsolutePath(FileSystemType type, const EntryBase* base, String path, String& absolutePath)
{
    ASSERT(base);

    if (!DOMFilePath::isAbsolute(path))
        path = DOMFilePath::append(base->fullPath(), path);
    absolutePath = DOMFilePath::removeExtraParentReferences(path);

    if ((type == FileSystemTypeTemporary || type == FileSystemTypePersistent) && !DOMFilePath::isValidPath(absolutePath))
        return false;
    return true;
}

bool DOMFileSystemBase::move(const EntryBase* source, EntryBase* parent, const String& newName, PassRefPtr<EntryCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
{
    String destinationPath;
    if (!verifyAndGetDestinationPathForCopyOrMove(source, parent, newName, destinationPath))
        return false;

    m_asyncFileSystem->move(createFileSystemURL(source), parent->filesystem()->createFileSystemURL(destinationPath), EntryCallbacks::create(successCallback, errorCallback, parent->filesystem(), destinationPath, source->isDirectory()));
    return true;
}

bool DOMFileSystemBase::copy(const EntryBase* source, EntryBase* parent, const String& newName, PassRefPtr<EntryCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
{
    String destinationPath;
    if (!verifyAndGetDestinationPathForCopyOrMove(source, parent, newName, destinationPath))
        return false;

    m_asyncFileSystem->copy(createFileSystemURL(source), parent->filesystem()->createFileSystemURL(destinationPath), EntryCallbacks::create(successCallback, errorCallback, parent->filesystem(), destinationPath, source->isDirectory()));
    return true;
}

bool DOMFileSystemBase::remove(const EntryBase* entry, PassRefPtr<VoidCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
{
    ASSERT(entry);
    // We don't allow calling remove() on the root directory.
    if (entry->fullPath() == String(DOMFilePath::root))
        return false;
    m_asyncFileSystem->remove(createFileSystemURL(entry), VoidCallbacks::create(successCallback, errorCallback));
    return true;
}

bool DOMFileSystemBase::removeRecursively(const EntryBase* entry, PassRefPtr<VoidCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
{
    ASSERT(entry && entry->isDirectory());
    // We don't allow calling remove() on the root directory.
    if (entry->fullPath() == String(DOMFilePath::root))
        return false;
    m_asyncFileSystem->removeRecursively(createFileSystemURL(entry), VoidCallbacks::create(successCallback, errorCallback));
    return true;
}

bool DOMFileSystemBase::getParent(const EntryBase* entry, PassRefPtr<EntryCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
{
    ASSERT(entry);
    String path = DOMFilePath::getDirectory(entry->fullPath());

    m_asyncFileSystem->directoryExists(createFileSystemURL(path), EntryCallbacks::create(successCallback, errorCallback, this, path, true));
    return true;
}

bool DOMFileSystemBase::getFile(const EntryBase* entry, const String& path, const FileSystemFlags& flags, PassRefPtr<EntryCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
{
    String absolutePath;
    if (!pathToAbsolutePath(m_type, entry, path, absolutePath))
        return false;

    OwnPtr<EntryCallbacks> callbacks = EntryCallbacks::create(successCallback, errorCallback, this, absolutePath, false);
    if (flags.create)
        m_asyncFileSystem->createFile(createFileSystemURL(absolutePath), flags.exclusive, callbacks.release());
    else
        m_asyncFileSystem->fileExists(createFileSystemURL(absolutePath), callbacks.release());
    return true;
}

bool DOMFileSystemBase::getDirectory(const EntryBase* entry, const String& path, const FileSystemFlags& flags, PassRefPtr<EntryCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
{
    String absolutePath;
    if (!pathToAbsolutePath(m_type, entry, path, absolutePath))
        return false;

    OwnPtr<EntryCallbacks> callbacks = EntryCallbacks::create(successCallback, errorCallback, this, absolutePath, true);
    if (flags.create)
        m_asyncFileSystem->createDirectory(createFileSystemURL(absolutePath), flags.exclusive, callbacks.release());
    else
        m_asyncFileSystem->directoryExists(createFileSystemURL(absolutePath), callbacks.release());
    return true;
}

bool DOMFileSystemBase::readDirectory(PassRefPtr<DirectoryReaderBase> reader, const String& path, PassRefPtr<EntriesCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
{
    ASSERT(DOMFilePath::isAbsolute(path));
    m_asyncFileSystem->readDirectory(createFileSystemURL(path), EntriesCallbacks::create(successCallback, errorCallback, reader, path));
    return true;
}

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)
