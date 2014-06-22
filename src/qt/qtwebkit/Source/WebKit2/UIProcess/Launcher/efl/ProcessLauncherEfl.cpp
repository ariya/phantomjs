/*
    Copyright (C) 2012 Samsung Electronics
    Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies).

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "ProcessLauncher.h"

#include "Connection.h"
#include "ProcessExecutablePath.h"
#include <WebCore/AuthenticationChallenge.h>
#include <WebCore/FileSystem.h>
#include <WebCore/NetworkingContext.h>
#include <WebCore/ResourceHandle.h>
#include <WebCore/RunLoop.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

using namespace WebCore;

namespace WebKit {

static Vector<OwnArrayPtr<char>> createArgsArray(const String& prefix, const String& executablePath, const String& socket, const String& pluginPath)
{
    ASSERT(!executablePath.isEmpty());
    ASSERT(!socket.isEmpty());

    Vector<String> splitArgs;
    prefix.split(' ', splitArgs);

    splitArgs.append(executablePath);
    splitArgs.append(socket);
    if (!pluginPath.isEmpty())
        splitArgs.append(pluginPath);

    Vector<OwnArrayPtr<char>> args;
    args.resize(splitArgs.size() + 1); // Extra room for null.

    size_t numArgs = splitArgs.size();
    for (size_t i = 0; i < numArgs; ++i) {
        CString param = splitArgs[i].utf8();
        args[i] = adoptArrayPtr(new char[param.length() + 1]); // Room for the terminating null coming from the CString.
        strncpy(args[i].get(), param.data(), param.length() + 1); // +1 here so that strncpy copies the ending null.
    }
    // execvp() needs the pointers' array to be null-terminated.
    args[numArgs] = nullptr;

    return args;
}

void ProcessLauncher::launchProcess()
{
    int sockets[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
        ASSERT_NOT_REACHED();
        return;
    }

    String processCmdPrefix, executablePath, pluginPath;
    switch (m_launchOptions.processType) {
    case WebProcess:
        executablePath = executablePathOfWebProcess();
        break;
#if ENABLE(PLUGIN_PROCESS)
    case PluginProcess:
        executablePath = executablePathOfPluginProcess();
        pluginPath = m_launchOptions.extraInitializationData.get("plugin-path");
        break;
#endif
    default:
        ASSERT_NOT_REACHED();
        return;
    }

#ifndef NDEBUG
    if (!m_launchOptions.processCmdPrefix.isEmpty())
        processCmdPrefix = m_launchOptions.processCmdPrefix;
#endif
    Vector<OwnArrayPtr<char>> args = createArgsArray(processCmdPrefix, executablePath, String::number(sockets[0]), pluginPath);

    // Do not perform memory allocation in the middle of the fork()
    // exec() below. FastMalloc can potentially deadlock because
    // the fork() doesn't inherit the running threads.
    pid_t pid = fork();
    if (!pid) { // Child process.
        close(sockets[1]);
        execvp(args.data()[0].get(), reinterpret_cast<char* const*>(args.data()));
    } else if (pid > 0) { // parent process;
        close(sockets[0]);
        m_processIdentifier = pid;
        // We've finished launching the process, message back to the main run loop.
        RunLoop::main()->dispatch(bind(&ProcessLauncher::didFinishLaunchingProcess, this, pid, sockets[1]));
    } else {
        ASSERT_NOT_REACHED();
        return;
    }
}

void ProcessLauncher::terminateProcess()
{
    if (m_isLaunching) {
        invalidate();
        return;
    }

    if (!m_processIdentifier)
        return;
    kill(m_processIdentifier, SIGKILL);
    m_processIdentifier = 0;
}

void ProcessLauncher::platformInvalidate()
{
}

} // namespace WebKit
