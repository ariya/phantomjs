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
#include "PluginSandboxProfile.h"

#include <wtf/RetainPtr.h>
#include <wtf/text/StringConcatenate.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

static NSString *pluginSandboxProfileDefaultDirectory()
{
    return [[[NSBundle bundleForClass:NSClassFromString(@"WKView")] resourcePath] stringByAppendingPathComponent:@"PlugInSandboxProfiles"];
}

static NSArray *pluginSandboxProfileDirectories()
{
    return @[
        // First look in the WebKit2 bundle.
        pluginSandboxProfileDefaultDirectory(),

        // Then try /System/Library/Sandbox/Profiles/.
        @"/System/Library/Sandbox/Profiles/"
    ];
}

static NSString *pluginSandboxProfileName(const String& bundleIdentifier)
{
    // Fold all / characters to : to prevent the plugin bundle-id from trying to escape the profile directory
    String sanitizedBundleIdentifier = bundleIdentifier;
    sanitizedBundleIdentifier.replace('/', ':');

    return [NSString stringWithFormat:@"%@.sb", (NSString *)sanitizedBundleIdentifier];
}

static String pluginSandboxCommonProfile()
{
    NSString *profilePath = [pluginSandboxProfileDefaultDirectory() stringByAppendingPathComponent:@"com.apple.WebKit.plugin-common.sb"];
    return [NSString stringWithContentsOfFile:profilePath encoding:NSUTF8StringEncoding error:NULL];
}

static String pluginSandboxProfileForDirectory(NSString *profileName, NSString *sandboxProfileDirectoryPath)
{
    NSString *profilePath = [sandboxProfileDirectoryPath stringByAppendingPathComponent:profileName];
    NSString *profileString = [NSString stringWithContentsOfFile:profilePath encoding:NSUTF8StringEncoding error:NULL];
    if (!profileString)
        return String();

    return makeString(pluginSandboxCommonProfile(), String(profileString));
}

String pluginSandboxProfile(const String& bundleIdentifier)
{
    if (bundleIdentifier.isEmpty())
        return String();

    NSString *profileName = pluginSandboxProfileName(bundleIdentifier);

    for (NSString *directory in pluginSandboxProfileDirectories()) {
        String sandboxProfile = pluginSandboxProfileForDirectory(profileName, directory);
        if (!sandboxProfile.isEmpty())
            return sandboxProfile;
    }

    return String();
}

static bool pluginHasSandboxProfileForDirectory(NSString *profileName, NSString *sandboxProfileDirectoryPath)
{
    NSString *profilePath = [sandboxProfileDirectoryPath stringByAppendingPathComponent:profileName];
    return [[NSFileManager defaultManager] fileExistsAtPath:profilePath];
}

bool pluginHasSandboxProfile(const String& bundleIdentifier)
{
    if (bundleIdentifier.isEmpty())
        return false;

    NSString *profileName = pluginSandboxProfileName(bundleIdentifier);

    for (NSString *directory in pluginSandboxProfileDirectories()) {
        if (pluginHasSandboxProfileForDirectory(profileName, directory))
            return true;
    }
    
    return false;
}

} // namespace WebKit
