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

#ifndef PluginInformation_h
#define PluginInformation_h

#include "ImmutableDictionary.h"
#include <wtf/Forward.h>

namespace WebKit {

struct PluginModuleInfo;

// Plug-in module information keys
String pluginInformationBundleIdentifierKey();
String pluginInformationBundleVersionKey();
String pluginInformationBundleShortVersionKey();
String pluginInformationPathKey();
String pluginInformationDisplayNameKey();
String pluginInformationDefaultLoadPolicyKey();
String pluginInformationUpdatePastLastBlockedVersionIsKnownAvailableKey();
String pluginInformationHasSandboxProfileKey();

// Plug-in load specific information keys
String pluginInformationFrameURLKey();
String pluginInformationMIMETypeKey();
String pluginInformationPageURLKey();
String pluginInformationPluginspageAttributeURLKey();
String pluginInformationPluginURLKey();
String plugInInformationReplacementObscuredKey();

PassRefPtr<ImmutableDictionary> createPluginInformationDictionary(const PluginModuleInfo&);
PassRefPtr<ImmutableDictionary> createPluginInformationDictionary(const PluginModuleInfo&, const String& frameURLString, const String& mimeType, const String& pageURLString, const String& pluginspageAttributeURLString, const String& pluginURLString, bool replacementObscured = false);
PassRefPtr<ImmutableDictionary> createPluginInformationDictionary(const String& mimeType, const String& frameURLString, const String& pageURLString);

void getPluginModuleInformation(const PluginModuleInfo&, ImmutableDictionary::MapType&);
void getPlatformPluginModuleInformation(const PluginModuleInfo&, ImmutableDictionary::MapType&);

} // namespace WebKit

#endif // PluginInformation_h
