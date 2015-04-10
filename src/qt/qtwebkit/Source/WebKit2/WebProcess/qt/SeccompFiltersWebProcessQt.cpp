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
#include "SeccompFiltersWebProcessQt.h"

#if ENABLE(SECCOMP_FILTERS)

#include <QFileInfo>
#include <QLibraryInfo>
#include <WebKit2/SeccompBroker.h>
#include <WebKit2/SyscallPolicy.h>

namespace WebKit {

SeccompFiltersWebProcessQt::SeccompFiltersWebProcessQt(const WebProcessCreationParameters& parameters)
    : SeccompFilters(Allow)
{
    m_policy.addDefaultWebProcessPolicy(parameters);
}

void SeccompFiltersWebProcessQt::platformInitialize()
{
    // TODO: We should block all the syscalls and whitelist
    // what we need + trap what should be handled by the broker.
    addRule("open", Trap);
    addRule("openat", Trap);
    addRule("creat", Trap);

    // Qt directories.
    m_policy.addDirectoryPermission(QLibraryInfo::location(QLibraryInfo::LibrariesPath), SyscallPolicy::Read);
    m_policy.addDirectoryPermission(QLibraryInfo::location(QLibraryInfo::PluginsPath), SyscallPolicy::Read);

    // WEBKIT_TESTFONTS is set when running layout tests. We also need to add
    // the root of the WebKit tree to the sandbox so we can read the tests and
    // write back the results.
    const char* path = getenv("WEBKIT_TESTFONTS");
    if (path) {
        QFileInfo fontDir(QString::fromUtf8(path));
        m_policy.addDirectoryPermission(fontDir.canonicalFilePath(), SyscallPolicy::Read);

        QFileInfo sourceDir(QString::fromUtf8(SOURCE_DIR));
        m_policy.addDirectoryPermission(sourceDir.canonicalFilePath(), SyscallPolicy::ReadAndWrite);
    }

    SeccompBroker::launchProcess(this, m_policy);
}

} // namespace WebKit

#endif // ENABLE(SECCOMP_FILTERS)
