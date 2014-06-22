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

#import "config.h"

#if ENABLE(PLUGIN_PROCESS)

#import "ChildProcessEntryPoint.h"
#import "EnvironmentUtilities.h"
#import "NetscapePluginModule.h"
#import "PluginProcess.h"
#import "WKBase.h"
#import <WebCore/RunLoop.h>

#if USE(APPKIT)
@interface NSApplication (WebNSApplicationDetails)
-(void)_installAutoreleasePoolsOnCurrentThreadIfNecessary;
@end
#endif

using namespace WebCore;

namespace WebKit {

class PluginProcessMainDelegate : public ChildProcessMainDelegate {
public:
    PluginProcessMainDelegate(const CommandLine& commandLine)
        : ChildProcessMainDelegate(commandLine)
    {
    }

    virtual void doPreInitializationWork()
    {
        // Remove the PluginProcess shim from the DYLD_INSERT_LIBRARIES environment variable so any processes
        // spawned by the PluginProcess don't try to insert the shim and crash.
        EnvironmentUtilities::stripValuesEndingWithString("DYLD_INSERT_LIBRARIES", "/PluginProcessShim.dylib");

#if USE(APPKIT)
        RunLoop::setUseApplicationRunLoopOnMainRunLoop();

        // Initialize AppKit.
        [NSApplication sharedApplication];

        // Installs autorelease pools on the current runloop which prevents memory from accumulating between user events.
        // FIXME: Remove when <rdar://problem/8929426> is fixed.
        [NSApp _installAutoreleasePoolsOnCurrentThreadIfNecessary];
#endif

        // Check if we're being spawned to write a MIME type preferences file.
        String pluginPath = m_commandLine["createPluginMIMETypesPreferences"];
        if (!pluginPath.isEmpty()) {
            // We are never going to get to the actual initialization, so initialize WebKit2 now.
            InitializeWebKit2();

            if (!NetscapePluginModule::createPluginMIMETypesPreferences(pluginPath))
                exit(EXIT_FAILURE);
            exit(EXIT_SUCCESS);
        }
    }

    virtual bool getExtraInitializationData(HashMap<String, String>& extraInitializationData)
    {
        String pluginPath = m_commandLine["plugin-path"];
        if (pluginPath.isEmpty())
            return false;
        extraInitializationData.add("plugin-path", pluginPath);

        String disableSandbox = m_commandLine["disable-sandbox"];
        if (!disableSandbox.isEmpty())
            extraInitializationData.add("disable-sandbox", disableSandbox);

        return true;
    }

    virtual void doPostRunWork()
    {
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
        // If we have private temporary and cache directories, clean them up.
        if (getenv("DIRHELPER_USER_DIR_SUFFIX")) {
            char darwinDirectory[PATH_MAX];
            if (confstr(_CS_DARWIN_USER_TEMP_DIR, darwinDirectory, sizeof(darwinDirectory)))
                [[NSFileManager defaultManager] removeItemAtPath:[[NSFileManager defaultManager] stringWithFileSystemRepresentation:darwinDirectory length:strlen(darwinDirectory)] error:nil];
            if (confstr(_CS_DARWIN_USER_CACHE_DIR, darwinDirectory, sizeof(darwinDirectory)))
                [[NSFileManager defaultManager] removeItemAtPath:[[NSFileManager defaultManager] stringWithFileSystemRepresentation:darwinDirectory length:strlen(darwinDirectory)] error:nil];
        }
#endif
    }
};

} // namespace WebKit

using namespace WebKit;

extern "C" WK_EXPORT int PluginProcessMain(int argc, char** argv);

int PluginProcessMain(int argc, char** argv)
{
    return ChildProcessMain<PluginProcess, PluginProcessMainDelegate>(argc, argv);
}

#endif // ENABLE(PLUGIN_PROCESS)
