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
#include "SeccompFiltersWebProcessEfl.h"

#if ENABLE(SECCOMP_FILTERS)

#include "WebProcessCreationParameters.h"
#include <WebKit2/SeccompBroker.h>
#include <sys/types.h>
#include <unistd.h>

namespace WebKit {

SeccompFiltersWebProcessEfl::SeccompFiltersWebProcessEfl(const WebProcessCreationParameters& parameters)
    : SeccompFilters(Allow)
{
    m_policy.addDefaultWebProcessPolicy(parameters);
}

void SeccompFiltersWebProcessEfl::platformInitialize()
{
    // TODO: We should block all the syscalls and whitelist
    // what we need + trap what should be handled by the broker.
    addRule("open", Trap);
    addRule("openat", Trap);
    addRule("creat", Trap);

    // Needed by Eeze on NetworkStateNotifierEfl.
    m_policy.addDirectoryPermission(ASCIILiteral("/sys/bus"), SyscallPolicy::Read);
    m_policy.addDirectoryPermission(ASCIILiteral("/sys/class"), SyscallPolicy::Read);
    m_policy.addDirectoryPermission(ASCIILiteral("/sys/devices"), SyscallPolicy::Read);
    m_policy.addFilePermission(ASCIILiteral("/etc/udev/udev.conf"), SyscallPolicy::Read);

#ifdef SOURCE_DIR
    // Developers using build-webkit --efl expect some libraries to be loaded
    // from the build root directory and they also need access to layout test
    // files. The constant is defined only when jhbuild is detected, which is
    // an indication of a development build.
    char* sourceDir = canonicalize_file_name(SOURCE_DIR);
    if (sourceDir) {
        m_policy.addDirectoryPermission(String::fromUTF8(sourceDir), SyscallPolicy::ReadAndWrite);
        free(sourceDir);
    }
#endif

    // Place where the theme and icons are installed.
    char* dataDir = canonicalize_file_name(DATA_DIR);
    if (dataDir) {
        m_policy.addDirectoryPermission(String::fromUTF8(dataDir), SyscallPolicy::Read);
        free(dataDir);
    }

#if USE(GSTREAMER)
    // Video playback requires access to the root of the user cache dir which
    // is not right. We need to check with these directories on gstreamer
    // can be configured.
    char* homeDir = getenv("HOME");
    if (homeDir)
        m_policy.addDirectoryPermission(String::fromUTF8(homeDir) + "/.cache", SyscallPolicy::ReadAndWrite);
#endif

    SeccompBroker::launchProcess(this, m_policy);
}

} // namespace WebKit

#endif // ENABLE(SECCOMP_FILTERS)
