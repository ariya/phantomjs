/*
 * Copyright (C) 2011 Igalia S.L.
 * Copyright (C) 2011 Apple Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PluginProcessProxy.h"

#if ENABLE(PLUGIN_PROCESS)

#include "PluginProcessCreationParameters.h"
#include "ProcessExecutablePath.h"
#include <WebCore/FileSystem.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>
#if PLATFORM(GTK) || PLATFORM(EFL)
#include <glib.h>
#include <wtf/gobject/GOwnPtr.h>
#endif

using namespace WebCore;

namespace WebKit {

void PluginProcessProxy::platformGetLaunchOptions(ProcessLauncher::LaunchOptions& launchOptions, const PluginProcessAttributes& pluginProcessAttributes)
{
#if PLATFORM(EFL) && !defined(NDEBUG)
    const char* commandPrefix = getenv("PLUGIN_PROCESS_COMMAND_PREFIX");
    if (commandPrefix && *commandPrefix)
        launchOptions.processCmdPrefix = String::fromUTF8(commandPrefix);
#endif

    launchOptions.extraInitializationData.add("plugin-path", pluginProcessAttributes.moduleInfo.path);
}

void PluginProcessProxy::platformInitializePluginProcess(PluginProcessCreationParameters&)
{
}

bool PluginProcessProxy::scanPlugin(const String& pluginPath, RawPluginMetaData& result)
{
#if PLATFORM(GTK) || PLATFORM(EFL)
    CString binaryPath = fileSystemRepresentation(executablePathOfPluginProcess());
    CString pluginPathCString = fileSystemRepresentation(pluginPath);
    char* argv[4];
    argv[0] = const_cast<char*>(binaryPath.data());
    argv[1] = const_cast<char*>("-scanPlugin");
    argv[2] = const_cast<char*>(pluginPathCString.data());
    argv[3] = 0;

    int status;
    GOwnPtr<char> stdOut;

    // If the disposition of SIGCLD signal is set to SIG_IGN (default)
    // then the signal will be ignored and g_spawn_sync() will not be
    // able to return the status.
    // As a consequence, we make sure that the disposition is set to
    // SIG_DFL before calling g_spawn_sync().
    struct sigaction action;
    sigaction(SIGCLD, 0, &action);
    if (action.sa_handler == SIG_IGN) {
        action.sa_handler = SIG_DFL;
        sigaction(SIGCLD, &action, 0);
    }

    if (!g_spawn_sync(0, argv, 0, G_SPAWN_STDERR_TO_DEV_NULL, 0, 0, &stdOut.outPtr(), 0, &status, 0))
        return false;

    if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS || !stdOut)
        return false;

    String stdOutString(reinterpret_cast<const UChar*>(stdOut.get()));

    Vector<String> lines;
    stdOutString.split(UChar('\n'), true, lines);

    if (lines.size() < 3)
        return false;

    result.name.swap(lines[0]);
    result.description.swap(lines[1]);
    result.mimeDescription.swap(lines[2]);
    return !result.mimeDescription.isEmpty();
#else // PLATFORM(GTK) || PLATFORM(EFL)
    return false;
#endif // PLATFORM(GTK) || PLATFORM(EFL)
}

} // namespace WebKit

#endif // ENABLE(PLUGIN_PROCESS)
