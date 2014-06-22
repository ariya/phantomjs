/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "SandboxExtension.h"

#if ENABLE(WEB_PROCESS_SANDBOX)

#import "ArgumentDecoder.h"
#import "ArgumentEncoder.h"
#import "DataReference.h"
#import "WebKitSystemInterface.h"
#import <WebCore/FileSystem.h>
#import <sys/stat.h>
#import <wtf/text/CString.h>

using namespace WebCore;

namespace WebKit {

SandboxExtension::Handle::Handle()
    : m_sandboxExtension(0)
{
}
    
SandboxExtension::Handle::~Handle()
{
    if (m_sandboxExtension) {
        WKSandboxExtensionInvalidate(m_sandboxExtension);
        WKSandboxExtensionDestroy(m_sandboxExtension);
    }
}

void SandboxExtension::Handle::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    if (!m_sandboxExtension) {
        encoder << CoreIPC::DataReference();
        return;
    }

    size_t length = 0;
    const char *serializedFormat = WKSandboxExtensionGetSerializedFormat(m_sandboxExtension, &length);
    ASSERT(serializedFormat);

    encoder << CoreIPC::DataReference(reinterpret_cast<const uint8_t*>(serializedFormat), length);

    // Encoding will destroy the sandbox extension locally.
    WKSandboxExtensionDestroy(m_sandboxExtension);
    m_sandboxExtension = 0;
}

bool SandboxExtension::Handle::decode(CoreIPC::ArgumentDecoder& decoder, Handle& result)
{
    ASSERT(!result.m_sandboxExtension);

    CoreIPC::DataReference dataReference;
    if (!decoder.decode(dataReference))
        return false;

    if (dataReference.isEmpty())
        return true;

    result.m_sandboxExtension = WKSandboxExtensionCreateFromSerializedFormat(reinterpret_cast<const char*>(dataReference.data()), dataReference.size());
    return true;
}

SandboxExtension::HandleArray::HandleArray()
    : m_data(0)
    , m_size(0)
{
}

SandboxExtension::HandleArray::~HandleArray()
{
    if (m_data)
        delete[] m_data;
}

void SandboxExtension::HandleArray::allocate(size_t size)
{
    if (!size)
        return;

    ASSERT(!m_data);

    m_data = new SandboxExtension::Handle[size];
    m_size = size;
}

SandboxExtension::Handle& SandboxExtension::HandleArray::operator[](size_t i)
{
    ASSERT_WITH_SECURITY_IMPLICATION(i < m_size); 
    return m_data[i];
}

const SandboxExtension::Handle& SandboxExtension::HandleArray::operator[](size_t i) const
{
    ASSERT_WITH_SECURITY_IMPLICATION(i < m_size);
    return m_data[i];
}

size_t SandboxExtension::HandleArray::size() const
{
    return m_size;
}

void SandboxExtension::HandleArray::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << static_cast<uint64_t>(size());
    for (size_t i = 0; i < m_size; ++i)
        encoder << m_data[i];
    
}

bool SandboxExtension::HandleArray::decode(CoreIPC::ArgumentDecoder& decoder, SandboxExtension::HandleArray& handles)
{
    uint64_t size;
    if (!decoder.decode(size))
        return false;
    handles.allocate(size);
    for (size_t i = 0; i < size; i++) {
        if (!decoder.decode(handles[i]))
            return false;
    }
    return true;
}

PassRefPtr<SandboxExtension> SandboxExtension::create(const Handle& handle)
{
    if (!handle.m_sandboxExtension)
        return 0;

    return adoptRef(new SandboxExtension(handle));
}

static WKSandboxExtensionType wkSandboxExtensionType(SandboxExtension::Type type)
{
    switch (type) {
    case SandboxExtension::ReadOnly:
        return WKSandboxExtensionTypeReadOnly;
    case SandboxExtension::ReadWrite:
        return WKSandboxExtensionTypeReadWrite;
    }

    CRASH();
}

static CString resolveSymlinksInPath(const CString& path)
{
    struct stat statBuf;

    // Check if this file exists.
    if (!stat(path.data(), &statBuf)) {
        char resolvedName[PATH_MAX];

        return realpath(path.data(), resolvedName);
    }

    char* slashPtr = strrchr(path.data(), '/');
    if (slashPtr == path.data())
        return path;

    size_t parentDirectoryLength = slashPtr - path.data();
    if (parentDirectoryLength >= PATH_MAX)
        return CString();

    // Get the parent directory.
    char parentDirectory[PATH_MAX];
    memcpy(parentDirectory, path.data(), parentDirectoryLength);
    parentDirectory[parentDirectoryLength] = '\0';

    // Resolve it.
    CString resolvedParentDirectory = resolveSymlinksInPath(CString(parentDirectory));
    if (resolvedParentDirectory.isNull())
        return CString();

    size_t lastPathComponentLength = path.length() - parentDirectoryLength;
    size_t resolvedPathLength = resolvedParentDirectory.length() + lastPathComponentLength;
    if (resolvedPathLength >= PATH_MAX)
        return CString();

    // Combine the resolved parent directory with the last path component.
    char* resolvedPathBuffer;
    CString resolvedPath = CString::newUninitialized(resolvedPathLength, resolvedPathBuffer);
    memcpy(resolvedPathBuffer, resolvedParentDirectory.data(), resolvedParentDirectory.length());
    memcpy(resolvedPathBuffer + resolvedParentDirectory.length(), slashPtr, lastPathComponentLength);

    return resolvedPath;
}

void SandboxExtension::createHandle(const String& path, Type type, Handle& handle)
{
    ASSERT(!handle.m_sandboxExtension);

    // FIXME: Do we need both resolveSymlinksInPath() and -stringByStandardizingPath?
    CString standardizedPath = resolveSymlinksInPath(fileSystemRepresentation([(NSString *)path stringByStandardizingPath]));
    handle.m_sandboxExtension = WKSandboxExtensionCreate(standardizedPath.data(), wkSandboxExtensionType(type));
    if (!handle.m_sandboxExtension)
        WTFLogAlways("Could not create a sandbox extension for '%s'", path.utf8().data());
}

void SandboxExtension::createHandleForReadWriteDirectory(const String& path, SandboxExtension::Handle& handle)
{
    NSError *error = nil;
    NSString *nsPath = path;

    if (![[NSFileManager defaultManager] createDirectoryAtPath:nsPath withIntermediateDirectories:YES attributes:nil error:&error]) {
        NSLog(@"could not create \"%@\", error %@", nsPath, error);
        return;
    }

    SandboxExtension::createHandle(path, SandboxExtension::ReadWrite, handle);
}

String SandboxExtension::createHandleForTemporaryFile(const String& prefix, Type type, Handle& handle)
{
    ASSERT(!handle.m_sandboxExtension);
    
    Vector<char> path(PATH_MAX);
    if (!confstr(_CS_DARWIN_USER_TEMP_DIR, path.data(), path.size()))
        return String();
    
    // Shrink the vector.   
    path.shrink(strlen(path.data()));
    ASSERT(path.last() == '/');
    
    // Append the file name.    
    path.append(prefix.utf8().data(), prefix.length());
    path.append('\0');
    
    handle.m_sandboxExtension = WKSandboxExtensionCreate(fileSystemRepresentation(path.data()).data(), wkSandboxExtensionType(type));

    if (!handle.m_sandboxExtension) {
        WTFLogAlways("Could not create a sandbox extension for temporary file '%s'", path.data());
        return String();
    }
    return String(path.data());
}

SandboxExtension::SandboxExtension(const Handle& handle)
    : m_sandboxExtension(handle.m_sandboxExtension)
    , m_useCount(0)
{
    handle.m_sandboxExtension = 0;
}

SandboxExtension::~SandboxExtension()
{
    if (!m_sandboxExtension)
        return;

    ASSERT(!m_useCount);
    WKSandboxExtensionDestroy(m_sandboxExtension);
}

bool SandboxExtension::revoke()
{
    ASSERT(m_sandboxExtension);
    ASSERT(m_useCount);
    
    if (--m_useCount)
        return true;

    return WKSandboxExtensionInvalidate(m_sandboxExtension);
}

bool SandboxExtension::consume()
{
    ASSERT(m_sandboxExtension);

    if (m_useCount++)
        return true;

    return WKSandboxExtensionConsume(m_sandboxExtension);
}

bool SandboxExtension::consumePermanently()
{
    ASSERT(m_sandboxExtension);

    bool result = WKSandboxExtensionConsume(m_sandboxExtension);

    // Destroy the extension without invalidating it.
    WKSandboxExtensionDestroy(m_sandboxExtension);
    m_sandboxExtension = 0;

    return result;
}

bool SandboxExtension::consumePermanently(const Handle& handle)
{
    if (!handle.m_sandboxExtension)
        return false;

    bool result = WKSandboxExtensionConsume(handle.m_sandboxExtension);
    
    // Destroy the extension without invalidating it.
    WKSandboxExtensionDestroy(handle.m_sandboxExtension);
    handle.m_sandboxExtension = 0;

    return result;
}

} // namespace WebKit

#endif // ENABLE(WEB_PROCESS_SANDBOX)
