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

#ifndef PluginComplexTextInputState_h
#define PluginComplexTextInputState_h

namespace WebKit {

enum PluginComplexTextInputState {
    // Complex text input is disabled.
    PluginComplexTextInputDisabled,

    // Complex text input is enabled using the updated Cocoa event model text
    // input handling specification described at https://wiki.mozilla.org/NPAPI:CocoaEventModel#Text_Input
    PluginComplexTextInputEnabled,

    // Complex text input is enabled using the legacy Cocoa/Carbon event model text
    // input handling that matches WebKit1.
    PluginComplexTextInputEnabledLegacy
};

static inline bool isValidPluginComplexTextInputState(uint64_t value)
{
    switch (value) {
    case PluginComplexTextInputDisabled:
    case PluginComplexTextInputEnabled:
    case PluginComplexTextInputEnabledLegacy:
        return true;
    }

    return false;
}

} // namespace WebKit

#endif // PluginComplexTextInputState_h
