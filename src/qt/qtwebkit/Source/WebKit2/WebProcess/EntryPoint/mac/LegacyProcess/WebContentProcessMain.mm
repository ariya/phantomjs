/*
 * Copyright (C) 2010, 2011, 2012 Apple Inc. All rights reserved.
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

#import "ChildProcessEntryPoint.h"
#import "EnvironmentUtilities.h"
#import "EnvironmentVariables.h"
#import "StringUtilities.h"
#import "WKBase.h"
#import "WebProcess.h"
#import <WebCore/RunLoop.h>
#import <mach/mach_error.h>
#import <servers/bootstrap.h>
#import <spawn.h>
#import <stdio.h>
#import <wtf/RetainPtr.h>
#import <wtf/text/CString.h>
#import <wtf/text/WTFString.h>

#if USE(APPKIT)
@interface NSApplication (WebNSApplicationDetails)
-(void)_installAutoreleasePoolsOnCurrentThreadIfNecessary;
@end
#endif

extern "C" kern_return_t bootstrap_register2(mach_port_t, name_t, mach_port_t, uint64_t);

using namespace WebCore;

namespace WebKit {

class WebContentProcessMainDelegate : public ChildProcessMainDelegate {
public:
    WebContentProcessMainDelegate(const CommandLine& commandLine)
        : ChildProcessMainDelegate(commandLine)
    {
    }

    virtual void doPreInitializationWork()
    {
        // Remove the WebProcess shim from the DYLD_INSERT_LIBRARIES environment variable so any processes spawned by
        // the WebProcess don't try to insert the shim and crash.
        EnvironmentUtilities::stripValuesEndingWithString("DYLD_INSERT_LIBRARIES", "/WebProcessShim.dylib");
    
#if USE(APPKIT)
        RunLoop::setUseApplicationRunLoopOnMainRunLoop();

        // Initialize AppKit.
        [NSApplication sharedApplication];

        // Installs autorelease pools on the current runloop which prevents memory from accumulating between user events.
        // FIXME: Remove when <rdar://problem/8929426> is fixed.
        [NSApp _installAutoreleasePoolsOnCurrentThreadIfNecessary];
#endif
    }

    virtual bool getConnectionIdentifier(CoreIPC::Connection::Identifier& identifier)
    {
        String clientExecutable = m_commandLine["client-executable"];
        if (clientExecutable.isEmpty())
            return ChildProcessMainDelegate::getConnectionIdentifier(identifier);

        mach_port_name_t publishedService;
        mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &publishedService);
        mach_port_insert_right(mach_task_self(), publishedService, publishedService, MACH_MSG_TYPE_MAKE_SEND);
        // Make it possible to look up.
        String serviceName = String::format("com.apple.WebKit.WebProcess-%d", getpid());
        if (kern_return_t kr = bootstrap_register2(bootstrap_port, const_cast<char*>(serviceName.utf8().data()), publishedService, 0)) {
            WTFLogAlways("Failed to register service name \"%s\". %s (%x)\n", serviceName.utf8().data(), mach_error_string(kr), kr);
            return false;
        }

        CString command = clientExecutable.utf8();
        const char* args[] = { command.data(), 0 };

        EnvironmentVariables environmentVariables;
        environmentVariables.set(EnvironmentVariables::preexistingProcessServiceNameKey(), serviceName.utf8().data());
        environmentVariables.set(EnvironmentVariables::preexistingProcessTypeKey(), m_commandLine["type"].utf8().data());

        posix_spawn_file_actions_t fileActions;
        posix_spawn_file_actions_init(&fileActions);
        posix_spawn_file_actions_addinherit_np(&fileActions, STDIN_FILENO);
        posix_spawn_file_actions_addinherit_np(&fileActions, STDOUT_FILENO);
        posix_spawn_file_actions_addinherit_np(&fileActions, STDERR_FILENO);

        posix_spawnattr_t attributes;
        posix_spawnattr_init(&attributes);
        posix_spawnattr_setflags(&attributes, POSIX_SPAWN_CLOEXEC_DEFAULT | POSIX_SPAWN_SETPGROUP);

        int spawnResult = posix_spawn(0, command.data(), &fileActions, &attributes, const_cast<char**>(args), environmentVariables.environmentPointer());

        posix_spawnattr_destroy(&attributes);
        posix_spawn_file_actions_destroy(&fileActions);

        if (spawnResult)
            return false;

        mach_msg_empty_rcv_t message;
        if (kern_return_t kr = mach_msg(&message.header, MACH_RCV_MSG, 0, sizeof(message), publishedService, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL)) {
            WTFLogAlways("Failed to receive port from the UI process. %s (%x)\n", mach_error_string(kr), kr);
            return false;
        }

        mach_port_mod_refs(mach_task_self(), publishedService, MACH_PORT_RIGHT_RECEIVE, -1);
        mach_port_t serverPort = message.header.msgh_remote_port;
        mach_port_type_t portType;
        kern_return_t kr = mach_port_type(mach_task_self(), serverPort, &portType);
        if (kr || !(portType & MACH_PORT_TYPE_SEND)) {
            WTFLogAlways("Failed to obtain send right for port received from the UI process.\n");
            return false;
        }

        identifier = serverPort;
        return true;
    }

    virtual bool getClientIdentifier(String& clientIdentifier)
    {
        String clientExecutable = m_commandLine["client-executable"];
        if (clientExecutable.isEmpty())
            return ChildProcessMainDelegate::getClientIdentifier(clientIdentifier);

        RetainPtr<NSURL> clientExecutableURL = adoptNS([[NSURL alloc] initFileURLWithPath:nsStringFromWebCoreString(clientExecutable)]);
        RetainPtr<CFURLRef> clientBundleURL = adoptCF(WKCopyBundleURLForExecutableURL((CFURLRef)clientExecutableURL.get()));
        RetainPtr<NSBundle> clientBundle = adoptNS([[NSBundle alloc] initWithURL:(NSURL *)clientBundleURL.get()]);
        clientIdentifier = [clientBundle.get() bundleIdentifier];
        if (clientIdentifier.isEmpty())
            return false;
        return true;
    }

    virtual bool getClientProcessName(String& clientProcessName)
    {
        String clientExecutable = m_commandLine["client-executable"];
        if (clientExecutable.isEmpty())
            return ChildProcessMainDelegate::getClientProcessName(clientProcessName);

        // Conjure up a process name by using everything after the last slash from the client-executable,
        // e.g. /Applications/Safari.app/Contents/MacOS/Safari becomes Safari.
        size_t lastSlash = clientExecutable.reverseFind('/');
        clientProcessName = clientExecutable.substring(lastSlash + 1);
        if (clientProcessName.isEmpty())
            return false;
        return true;
    }
};

} // namespace WebKit

using namespace WebKit;

extern "C" WK_EXPORT int WebContentProcessMain(int argc, char** argv);

int WebContentProcessMain(int argc, char** argv)
{
    return ChildProcessMain<WebProcess, WebContentProcessMainDelegate>(argc, argv);
}
