/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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

#ifndef EnvironmentVariables_h
#define EnvironmentVariables_h

#include <wtf/Noncopyable.h>
#include <wtf/Vector.h>

namespace WebKit {

class EnvironmentVariables {
    WTF_MAKE_NONCOPYABLE(EnvironmentVariables);

public:
    EnvironmentVariables();
    ~EnvironmentVariables();

    void set(const char* name, const char* value);
    const char* get(const char* name) const;

    // Will append the value with the given separator if the environment variable already exists.
    void appendValue(const char* name, const char* value, char separator);

    char** environmentPointer() const { return m_environmentPointer; }

    static const char* preexistingProcessServiceNameKey();
    static const char* preexistingProcessTypeKey();

    static void dump();

private:
    const char* valueIfVariableHasName(const char* environmentVariable, const char* name) const;
    const char* createStringForVariable(const char* name, const char* value);
    void copyEnvironmentVariables();

    char** m_environmentPointer;
    Vector<char*> m_environmentVariables;

    // These allocated strings will be freed in the destructor.
    Vector<char*> m_allocatedStrings;
};

} // namespace WebKit

#endif // EnvironmentVariables_h
