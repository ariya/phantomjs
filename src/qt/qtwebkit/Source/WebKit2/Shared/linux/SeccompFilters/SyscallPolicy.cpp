/*
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
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

#include "config.h"
#include "SyscallPolicy.h"

#if ENABLE(SECCOMP_FILTERS)

#include "WebProcessCreationParameters.h"
#include <libgen.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wtf/OwnPtr.h>

namespace WebKit {

static String removeTrailingSlash(const String& path)
{
    if (path.endsWith('/'))
        return path.left(path.length() - 1);

    return path;
}

bool SyscallPolicy::hasPermissionForPath(const char* path, Permission permission) const
{
    // The root directory policy needs to be set because it is the
    // ultimate fallback when rewinding directories.
    ASSERT(m_directoryPermission.contains("/"));

    if (permission == NotAllowed)
        return false;

    char* basePath = strdup(path);
    char* canonicalPath = canonicalize_file_name(basePath);

    while (canonicalPath) {
        struct stat pathStat;
        if (stat(canonicalPath, &pathStat) == -1) {
            free(basePath);
            free(canonicalPath);
            return false;
        }

        if (S_ISDIR(pathStat.st_mode))
            break;

        PermissionMap::const_iterator policy = m_filePermission.find(String(canonicalPath));
        if (policy != m_filePermission.end()) {
            free(basePath);
            free(canonicalPath);
            return (permission & policy->value) == permission;
        }

        // If not a directory neither a file with a policy defined,
        // we set canonicalPath to zero to force a rewind to the parent
        // directory.
        free(canonicalPath);
        canonicalPath = 0;
    }

    while (!canonicalPath) {
        char* currentBaseDirectory = dirname(basePath);
        canonicalPath = canonicalize_file_name(currentBaseDirectory);
    }

    PermissionMap::const_iterator policy = m_directoryPermission.find(String(canonicalPath));
    while (policy == m_directoryPermission.end()) {
        char* currentBaseDirectory = dirname(canonicalPath);
        policy = m_directoryPermission.find(String(currentBaseDirectory));
    }

    free(basePath);
    free(canonicalPath);

    return (permission & policy->value) == permission;
}

void SyscallPolicy::addFilePermission(const String& path, Permission permission)
{
    ASSERT(!path.isEmpty() && path.startsWith('/')  && !path.endsWith('/') && !path.contains("//"));

    m_filePermission.set(path, permission);
}

void SyscallPolicy::addDirectoryPermission(const String& path, Permission permission)
{
    ASSERT(path.startsWith('/') && !path.contains("//") && (path.length() == 1 || !path.endsWith('/')));

    m_directoryPermission.set(path, permission);
}

void SyscallPolicy::addDefaultWebProcessPolicy(const WebProcessCreationParameters& parameters)
{
    // Directories settings coming from the UIProcess.
    if (!parameters.applicationCacheDirectory.isEmpty())
        addDirectoryPermission(removeTrailingSlash(parameters.applicationCacheDirectory), ReadAndWrite);
    if (!parameters.databaseDirectory.isEmpty())
        addDirectoryPermission(removeTrailingSlash(parameters.databaseDirectory), ReadAndWrite);
    if (!parameters.localStorageDirectory.isEmpty())
        addDirectoryPermission(removeTrailingSlash(parameters.localStorageDirectory), ReadAndWrite);
    if (!parameters.diskCacheDirectory.isEmpty())
        addDirectoryPermission(removeTrailingSlash(parameters.diskCacheDirectory), ReadAndWrite);
    if (!parameters.cookieStorageDirectory.isEmpty())
        addDirectoryPermission(removeTrailingSlash(parameters.cookieStorageDirectory), ReadAndWrite);
#if USE(SOUP)
    if (!parameters.cookiePersistentStoragePath.isEmpty())
        addDirectoryPermission(removeTrailingSlash(parameters.cookiePersistentStoragePath), ReadAndWrite);
#endif

    // The root policy will block access to any directory or
    // file unless white listed bellow or by platform.
    addDirectoryPermission(ASCIILiteral("/"), NotAllowed);

    // Shared libraries, plugins and fonts.
    addDirectoryPermission(ASCIILiteral("/lib"), Read);
    addDirectoryPermission(ASCIILiteral("/usr/lib"), Read);
    addDirectoryPermission(ASCIILiteral("/usr/share"), Read);

    // SSL Certificates.
    addDirectoryPermission(ASCIILiteral("/etc/ssl/certs"), Read);

    // Fontconfig cache.
    addDirectoryPermission(ASCIILiteral("/etc/fonts"), Read);
    addDirectoryPermission(ASCIILiteral("/var/cache/fontconfig"), Read);

    // Audio devices, random number generators, etc.
    addDirectoryPermission(ASCIILiteral("/dev"), ReadAndWrite);

    // Temporary files and process self information.
    addDirectoryPermission(ASCIILiteral("/tmp"), ReadAndWrite);
    addDirectoryPermission(ASCIILiteral("/proc/") + String::number(getpid()), ReadAndWrite);

    // In some distros /dev/shm is a symbolic link to /run/shm, and in
    // this case, the canonical path resolver will follow the link. If
    // inside /dev, the policy is already set.
    addDirectoryPermission(ASCIILiteral("/run/shm"), ReadAndWrite);

    // Needed by glibc for networking and locale.
    addFilePermission(ASCIILiteral("/etc/gai.conf"), Read);
    addFilePermission(ASCIILiteral("/etc/host.conf"), Read);
    addFilePermission(ASCIILiteral("/etc/hosts"), Read);
    addFilePermission(ASCIILiteral("/etc/localtime"), Read);
    addFilePermission(ASCIILiteral("/etc/nsswitch.conf"), Read);

    // Needed for DNS resoltion. In some distros, the resolv.conf inside
    // /etc is just a symbolic link.
    addFilePermission(ASCIILiteral("/etc/resolv.conf"), Read);
    addFilePermission(ASCIILiteral("/run/resolvconf/resolv.conf"), Read);

    // Needed to convert uid and gid into names.
    addFilePermission(ASCIILiteral("/etc/group"), Read);
    addFilePermission(ASCIILiteral("/etc/passwd"), Read);

    // Needed by the loader.
    addFilePermission(ASCIILiteral("/etc/ld.so.cache"), Read);

    // Needed by various, including toolkits, for optimizations based
    // on the current amount of free system memory.
    addFilePermission(ASCIILiteral("/proc/cpuinfo"), Read);
    addFilePermission(ASCIILiteral("/proc/filesystems"), Read);
    addFilePermission(ASCIILiteral("/proc/meminfo"), Read);
    addFilePermission(ASCIILiteral("/proc/stat"), Read);

    // Needed by D-Bus.
    addFilePermission(ASCIILiteral("/var/lib/dbus/machine-id"), Read);

    char* homeDir = getenv("HOME");
    if (homeDir) {
        // X11 connection token.
        addFilePermission(String::fromUTF8(homeDir) + "/.Xauthority", Read);
        // MIME type resolution.
        addDirectoryPermission(String::fromUTF8(homeDir) +  "/.local/share/mime", Read);
    }
}

} // namespace WebKit

#endif // ENABLE(SECCOMP_FILTERS)
