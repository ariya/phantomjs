/*
 * Copyright (C) 2012 Samsung Electronics
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
#include "ProcessExecutablePath.h"

#include "FileSystem.h"
#include <libgen.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/CString.h>

namespace WebKit {

static String findProcessPath(const char* processName)
{
#if OS(UNIX)
    char readLinkBuffer[PATH_MAX] = {0};

#if OS(LINUX)
    ssize_t result = readlink("/proc/self/exe", readLinkBuffer, PATH_MAX);
#else
    ssize_t result = readlink("/proc/curproc/file", readLinkBuffer, PATH_MAX);
#endif
    if (result > 0) {
        char* executablePathPtr = dirname(readLinkBuffer);
        String executablePath = WebCore::pathByAppendingComponent(String(executablePathPtr), processName);

        // Checks whether process exist on the current path.
        struct stat fileStat;
        if (!stat(executablePath.utf8().data(), &fileStat))
            return executablePath;
    }
#endif

    return WebCore::pathByAppendingComponent(String(LIBEXECDIR), processName);
}

String executablePathOfWebProcess()
{
    DEFINE_STATIC_LOCAL(const String, webKitWebProcessName, (findProcessPath(WEBPROCESSNAME)));

    return webKitWebProcessName;
}

String executablePathOfPluginProcess()
{
    DEFINE_STATIC_LOCAL(const String, webKitPluginProcessName, (findProcessPath(PLUGINPROCESSNAME)));

    return webKitPluginProcessName;
}

} // namespace WebKit
