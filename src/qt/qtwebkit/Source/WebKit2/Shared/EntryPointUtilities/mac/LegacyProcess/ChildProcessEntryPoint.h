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

#ifndef ChildProcessEntryPoint_h
#define ChildProcessEntryPoint_h

#import "ChildProcess.h"
#import "CommandLine.h"
#import "WebKit2Initialize.h"
#import <WebCore/RunLoop.h>
#import <WebKitSystemInterface.h>
#import <sysexits.h>

namespace WebKit {

class ChildProcessMainDelegate {
public:
    ChildProcessMainDelegate(const CommandLine& commandLine)
        : m_commandLine(commandLine)
    {
    }

    virtual ~ChildProcessMainDelegate();

    virtual void installSignalHandlers();
    virtual void doPreInitializationWork();

    virtual bool getConnectionIdentifier(CoreIPC::Connection::Identifier& identifier);
    virtual bool getClientIdentifier(String& clientIdentifier);
    virtual bool getClientProcessName(String& clientProcessName);
    virtual bool getExtraInitializationData(HashMap<String, String>& extraInitializationData);

    virtual void doPostRunWork();

protected:
    const CommandLine& m_commandLine;
};

template<typename ChildProcessType, typename ChildProcessMainDelegateType>
int ChildProcessMain(int argc, char** argv)
{
    CommandLine commandLine;
    if (!commandLine.parse(argc, argv))
        return EXIT_FAILURE;

    ChildProcessMainDelegateType delegate(commandLine);

    @autoreleasepool {
        delegate.installSignalHandlers();
        delegate.doPreInitializationWork();

        InitializeWebKit2();

        ChildProcessInitializationParameters parameters;
        if (!delegate.getConnectionIdentifier(parameters.connectionIdentifier))
            return EXIT_FAILURE;

        if (!delegate.getClientIdentifier(parameters.clientIdentifier))
            return EXIT_FAILURE;

        if (!delegate.getClientProcessName(parameters.uiProcessName))
            return EXIT_FAILURE;

        if (!delegate.getExtraInitializationData(parameters.extraInitializationData))
            return EXIT_FAILURE;

        // FIXME: This should be moved to ChildProcessMac if it is still necessary.
        String localization = commandLine["localization"];
        RetainPtr<CFStringRef> cfLocalization = adoptCF(CFStringCreateWithCharacters(0, reinterpret_cast<const UniChar*>(localization.characters()), localization.length()));
        if (cfLocalization)
            WKSetDefaultLocalization(cfLocalization.get());

        ChildProcessType::shared().initialize(parameters);
    }

    WebCore::RunLoop::run();

    @autoreleasepool {
        delegate.doPostRunWork();
    }

    return 0;
}

} // namespace WebKit

#endif // ChildProcessEntryPoint_h
