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

#include "config.h"
#include "EnvironmentVariables.h"

#include <crt_externs.h>

namespace WebKit {

EnvironmentVariables::EnvironmentVariables()
    : m_environmentPointer(*_NSGetEnviron())
{
}

EnvironmentVariables::~EnvironmentVariables()
{
    size_t size = m_allocatedStrings.size();
    for (size_t i = 0; i < size; ++i)
        fastFree(m_allocatedStrings[i]);
}

void EnvironmentVariables::set(const char* name, const char* value)
{
    // Check if we need to copy the environment.
    if (m_environmentPointer == *_NSGetEnviron())
        copyEnvironmentVariables();

    // Allocate a string for the name and value.
    const char* nameAndValue = createStringForVariable(name, value);

    for (size_t i = 0; i < m_environmentVariables.size() - 1; ++i) {
        if (valueIfVariableHasName(m_environmentVariables[i], name)) {
            // Just replace the environment variable.
            m_environmentVariables[i] = const_cast<char*>(nameAndValue);
            return;
        }
    }

    // Append the new string.
    ASSERT(!m_environmentVariables.last());
    m_environmentVariables.last() = const_cast<char*>(nameAndValue);
    m_environmentVariables.append(static_cast<char*>(0));

    m_environmentPointer = m_environmentVariables.data();
}

const char* EnvironmentVariables::get(const char* name) const
{
    for (size_t i = 0; m_environmentPointer[i]; ++i) {
        if (const char* value = valueIfVariableHasName(m_environmentPointer[i], name))
            return value;
    }
    return 0;
}

void EnvironmentVariables::appendValue(const char* name, const char* value, char separator)
{
    const char* existingValue = get(name);
    if (!existingValue) {
        set(name, value);
        return;
    }

    Vector<char, 128> newValue;
    newValue.append(existingValue, strlen(existingValue));
    newValue.append(separator);
    newValue.append(value, strlen(value) + 1);

    set(name, newValue.data());
}

const char* EnvironmentVariables::valueIfVariableHasName(const char* environmentVariable, const char* name) const
{
    // Find the environment variable name.
    const char* equalsLocation = strchr(environmentVariable, '=');
    ASSERT(equalsLocation);

    size_t nameLength = equalsLocation - environmentVariable;
    if (strlen(name) != nameLength)
        return 0;
    if (memcmp(environmentVariable, name, nameLength))
        return 0;

    return equalsLocation + 1;
}

const char* EnvironmentVariables::createStringForVariable(const char* name, const char* value)
{
    int nameLength = strlen(name);
    int valueLength = strlen(value);

    // Allocate enough room to hold 'name=value' and the null character.
    char* string = static_cast<char*>(fastMalloc(nameLength + 1 + valueLength + 1));
    memcpy(string, name, nameLength);
    string[nameLength] = '=';
    memcpy(string + nameLength + 1, value, valueLength);
    string[nameLength + 1 + valueLength] = '\0';

    m_allocatedStrings.append(string);

    return string;
}

void EnvironmentVariables::copyEnvironmentVariables()
{
    for (size_t i = 0; (*_NSGetEnviron())[i]; i++)
        m_environmentVariables.append((*_NSGetEnviron())[i]);

    // Null-terminate the array.
    m_environmentVariables.append(static_cast<char*>(0));

    // Update the environment pointer.
    m_environmentPointer = m_environmentVariables.data();
}

const char* EnvironmentVariables::preexistingProcessServiceNameKey()
{
    return "WEBKIT_PREEXISTING_PROCESS_SERVICE_NAME";
}

const char* EnvironmentVariables::preexistingProcessTypeKey()
{
    return "WEBKIT_PREEXISTING_PROCESS_TYPE";
}

void EnvironmentVariables::dump()
{
    for (size_t i = 0; (*_NSGetEnviron())[i]; i++)
        printf("%s\n", (*_NSGetEnviron())[i]);

    printf("\n\n\n");
}

} // namespace WebKit
