/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#include "SandboxInitializationParameters.h"

namespace WebKit {

SandboxInitializationParameters::SandboxInitializationParameters()
    : m_profileSelectionMode(UseDefaultSandboxProfilePath)
{
}

SandboxInitializationParameters::~SandboxInitializationParameters()
{
    for (size_t i = 0; i + 1 < m_namedParameters.size(); i += 2)
        fastFree(const_cast<char*>(m_namedParameters[i + 1]));
}

void SandboxInitializationParameters::appendPathInternal(const char* name, const char* path)
{
    char normalizedPath[PATH_MAX];
    if (!realpath(path, normalizedPath))
        normalizedPath[0] = '\0';

    ASSERT(!(m_namedParameters.size() % 2));

    m_namedParameters.append(name);
    m_namedParameters.append(fastStrDup(normalizedPath));
}

void SandboxInitializationParameters::addConfDirectoryParameter(const char* name, int confID)
{
    char path[PATH_MAX];
    if (confstr(confID, path, PATH_MAX) <= 0)
        path[0] = '\0';

    appendPathInternal(name, path);
}

void SandboxInitializationParameters::addPathParameter(const char* name, NSString *path)
{
    appendPathInternal(name, [path length] ? [(NSString *)path fileSystemRepresentation] : "");
}

void SandboxInitializationParameters::addPathParameter(const char* name, const char* path)
{
    appendPathInternal(name, path);
}

void SandboxInitializationParameters::addParameter(const char* name, const char* value)
{
    m_namedParameters.append(name);
    m_namedParameters.append(fastStrDup(value));
}

const char* const* SandboxInitializationParameters::namedParameterArray() const
{
    if (!(m_namedParameters.size() % 2))
        m_namedParameters.append(static_cast<const char*>(0));

    return m_namedParameters.data();
}

size_t SandboxInitializationParameters::count() const
{
    return m_namedParameters.size() / 2;
}

const char* SandboxInitializationParameters::name(size_t index) const
{
    ASSERT(index != m_namedParameters.size());
    return m_namedParameters[index * 2];
}

const char* SandboxInitializationParameters::value(size_t index) const
{
    return m_namedParameters[index * 2 + 1];
}

} // namespace WebKit
