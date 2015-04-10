/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
#include "EnvironmentUtilities.h"

#include <wtf/text/CString.h>

namespace WebKit {

namespace EnvironmentUtilities {

void stripValuesEndingWithString(const char* environmentVariable, const char* searchValue)
{
    ASSERT(environmentVariable);
    ASSERT(searchValue);
    
    // Grab the current value of the environment variable.
    char* environmentValue = getenv(environmentVariable);
        
    if (!environmentValue || environmentValue[0] == '\0')
        return;

    // Set up the strings we'll be searching for.
    size_t searchLength = strlen(searchValue);
    if (!searchLength)
        return;

    Vector<char> searchValueWithColonVector;
    searchValueWithColonVector.grow(searchLength + 2);
    char* searchValueWithColon = searchValueWithColonVector.data();
    size_t searchLengthWithColon = searchLength + 1;

    memcpy(searchValueWithColon, searchValue, searchLength);
    searchValueWithColon[searchLength] = ':';
    searchValueWithColon[searchLengthWithColon] = '\0';
    
    // Loop over environmentValueBuffer, removing any components that match the search value ending with a colon.
    char* componentStart = environmentValue;
    char* match = strstr(componentStart, searchValueWithColon);
    bool foundAnyMatches = match != NULL;
    while (match != NULL) {
        // Update componentStart to point to the colon immediately preceding the match.
        char* nextColon = strstr(componentStart, ":");
        while (nextColon && nextColon < match) {
            componentStart = nextColon;
            nextColon = strstr(componentStart + 1, ":");
        }
                
        // Copy over everything right of the match to the current component start, and search from there again.
        if (componentStart[0] == ':') {
            // If componentStart points to a colon, go ahead and copy the colon over.
            strcpy(componentStart, match + searchLength);
        } else {
            // Otherwise, componentStart still points to the beginning of environmentValueBuffer, so don't copy over the colon.
            // The edge case is if the colon is the last character in the string, so "match + searchLengthWithoutColon + 1" is the
            // null terminator of the original input, in which case this is still safe.
            strcpy(componentStart, match + searchLengthWithColon);
        }
        
        match = strstr(componentStart, searchValueWithColon);
    }
    
    // Search for the value without a trailing colon, seeing if the original input ends with it.
    match = strstr(componentStart, searchValue);
    while (match != NULL) {
        if (match[searchLength] == '\0')
            break;
        match = strstr(match + 1, searchValue);
    }
    
    // Since the original input ends with the search, strip out the last component.
    if (match) {
        // Update componentStart to point to the colon immediately preceding the match.
        char* nextColon = strstr(componentStart, ":");
        while (nextColon && nextColon < match) {
            componentStart = nextColon;
            nextColon = strstr(componentStart + 1, ":");
        }
        
        // Whether componentStart points to the original string or the last colon, putting the null terminator there will get us the desired result.
        componentStart[0] = '\0';

        foundAnyMatches = true;
    }

    // If we found no matches, don't change anything.
    if (!foundAnyMatches)
        return;

    // If we have nothing left, just unset the variable
    if (environmentValue[0] == '\0') {
        unsetenv(environmentVariable);
        return;
    }
    
    setenv(environmentVariable, environmentValue, 1);
}

} // namespace EnvironmentUtilities

} // namespace WebKit
