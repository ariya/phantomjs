/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#import "ChildProcess.h"

#import "SandboxInitializationParameters.h"
#import "WebKitSystemInterface.h"
#import <WebCore/FileSystem.h>
#import <WebCore/SystemVersionMac.h>
#import <mach/task.h>
#import <pwd.h>
#import <stdlib.h>
#import <sysexits.h>

// We have to #undef __APPLE_API_PRIVATE to prevent sandbox.h from looking for a header file that does not exist (<rdar://problem/9679211>). 
#undef __APPLE_API_PRIVATE
#import <sandbox.h>

#define SANDBOX_NAMED_EXTERNAL 0x0003
extern "C" int sandbox_init_with_parameters(const char *profile, uint64_t flags, const char *const parameters[], char **errorbuf);

#ifdef __has_include
#if __has_include(<HIServices/ProcessesPriv.h>)
#include <HIServices/ProcessesPriv.h>
#endif
#endif

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
typedef bool (^LSServerConnectionAllowedBlock) ( CFDictionaryRef optionsRef );
extern "C" void _LSSetApplicationLaunchServicesServerConnectionStatus(uint64_t flags, LSServerConnectionAllowedBlock block);
extern "C" CFDictionaryRef _LSApplicationCheckIn(int sessionID, CFDictionaryRef applicationInfo);
#endif

extern "C" OSStatus SetApplicationIsDaemon(Boolean isDaemon);

using namespace WebCore;

namespace WebKit {

static const double kSuspensionHysteresisSeconds = 5.0;

void ChildProcess::setProcessSuppressionEnabledInternal(bool processSuppressionEnabled)
{
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    if (this->processSuppressionEnabled() == processSuppressionEnabled)
        return;

    if (processSuppressionEnabled) {
        ASSERT(!m_activeTaskCount);
        [[NSProcessInfo processInfo] endActivity:m_processSuppressionAssertion.get()];
        m_processSuppressionAssertion.clear();
    } else {
        NSActivityOptions options = (NSActivityUserInitiatedAllowingIdleSystemSleep | NSActivityLatencyCritical) & ~(NSActivitySuddenTerminationDisabled | NSActivityAutomaticTerminationDisabled);
        m_processSuppressionAssertion = [[NSProcessInfo processInfo] beginActivityWithOptions:options reason:@"Process Suppression Disabled"];
    }
#else
    UNUSED_PARAM(processSuppressionEnabled);
#endif
}

void ChildProcess::setProcessSuppressionEnabled(bool processSuppressionEnabled)
{
    if (this->processSuppressionEnabled() == processSuppressionEnabled)
        return;
    if (m_shouldSuspend == processSuppressionEnabled)
        return;
    m_shouldSuspend = processSuppressionEnabled;
    if (m_shouldSuspend) {
        if (!m_activeTaskCount)
            m_suspensionHysteresisTimer.startOneShot(kSuspensionHysteresisSeconds);
        return;
    }
    setProcessSuppressionEnabledInternal(false);
}

void ChildProcess::incrementActiveTaskCount()
{
    m_activeTaskCount++;
    if (m_suspensionHysteresisTimer.isActive())
        m_suspensionHysteresisTimer.stop();
    if (m_activeTaskCount)
        setProcessSuppressionEnabledInternal(false);
}

void ChildProcess::decrementActiveTaskCount()
{
    ASSERT(m_activeTaskCount);
    m_activeTaskCount--;
    if (m_activeTaskCount)
        return;
    if (m_shouldSuspend)
        m_suspensionHysteresisTimer.startOneShot(kSuspensionHysteresisSeconds);
}

void ChildProcess::suspensionHysteresisTimerFired()
{
    ASSERT(!m_activeTaskCount);
    ASSERT(m_shouldSuspend);
    setProcessSuppressionEnabledInternal(true);
}

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
static void initializeTimerCoalescingPolicy()
{
    // Set task_latency and task_throughput QOS tiers as appropriate for a visible application.
    struct task_qos_policy qosinfo = { LATENCY_QOS_TIER_0, THROUGHPUT_QOS_TIER_0 };
    kern_return_t kr = task_policy_set(mach_task_self(), TASK_BASE_QOS_POLICY, (task_policy_t)&qosinfo, TASK_QOS_POLICY_COUNT);
    ASSERT_UNUSED(kr, kr == KERN_SUCCESS);
}
#endif

void ChildProcess::setApplicationIsDaemon()
{
    OSStatus error = SetApplicationIsDaemon(true);
    ASSERT_UNUSED(error, error == noErr);

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    _LSSetApplicationLaunchServicesServerConnectionStatus(0, 0);
    RetainPtr<CFDictionaryRef> unused = _LSApplicationCheckIn(-2, CFBundleGetInfoDictionary(CFBundleGetMainBundle()));
#endif
}

void ChildProcess::platformInitialize()
{
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    initializeTimerCoalescingPolicy();
#endif
    // Starting with process suppression disabled.  The proxy for this process will
    // enable if appropriate from didFinishLaunching().
    // We use setProcessSuppressionEnabledInternal to avoid any short circuit logic
    // that would prevent us from taking the suppression assertion.
    setProcessSuppressionEnabledInternal(false);

    [[NSFileManager defaultManager] changeCurrentDirectoryPath:[[NSBundle mainBundle] bundlePath]];
}

void ChildProcess::initializeSandbox(const ChildProcessInitializationParameters& parameters, SandboxInitializationParameters& sandboxParameters)
{
    NSBundle *webkit2Bundle = [NSBundle bundleForClass:NSClassFromString(@"WKView")];
    String defaultProfilePath = [webkit2Bundle pathForResource:[[NSBundle mainBundle] bundleIdentifier] ofType:@"sb"];

    if (sandboxParameters.systemDirectorySuffix().isNull()) {
        String defaultSystemDirectorySuffix = String([[NSBundle mainBundle] bundleIdentifier]) + "+" + parameters.clientIdentifier;
        sandboxParameters.setSystemDirectorySuffix(defaultSystemDirectorySuffix);
    }

    Vector<String> osVersionParts;
    String osSystemMarketingVersion = String(systemMarketingVersion());
    osSystemMarketingVersion.split('.', false, osVersionParts);
    if (osVersionParts.size() < 2) {
        WTFLogAlways("%s: Couldn't find OS Version\n", getprogname());
        exit(EX_NOPERM);
    }
    String osVersion = osVersionParts[0];
    osVersion.append('.');
    osVersion.append(osVersionParts[1]);
    sandboxParameters.addParameter("_OS_VERSION", osVersion.utf8().data());

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
    // Use private temporary and cache directories.
    setenv("DIRHELPER_USER_DIR_SUFFIX", fileSystemRepresentation(sandboxParameters.systemDirectorySuffix()).data(), 0);
    char temporaryDirectory[PATH_MAX];
    if (!confstr(_CS_DARWIN_USER_TEMP_DIR, temporaryDirectory, sizeof(temporaryDirectory))) {
        WTFLogAlways("%s: couldn't retrieve private temporary directory path: %d\n", getprogname(), errno);
        exit(EX_NOPERM);
    }
    setenv("TMPDIR", temporaryDirectory, 1);
#endif

    sandboxParameters.addPathParameter("WEBKIT2_FRAMEWORK_DIR", [[webkit2Bundle bundlePath] stringByDeletingLastPathComponent]);
    sandboxParameters.addConfDirectoryParameter("DARWIN_USER_TEMP_DIR", _CS_DARWIN_USER_TEMP_DIR);
    sandboxParameters.addConfDirectoryParameter("DARWIN_USER_CACHE_DIR", _CS_DARWIN_USER_CACHE_DIR);

    char buffer[4096];
    int bufferSize = sizeof(buffer);
    struct passwd pwd;
    struct passwd* result = 0;
    if (getpwuid_r(getuid(), &pwd, buffer, bufferSize, &result) || !result) {
        WTFLogAlways("%s: Couldn't find home directory\n", getprogname());
        exit(EX_NOPERM);
    }

    sandboxParameters.addPathParameter("HOME_DIR", pwd.pw_dir);

    String path = String::fromUTF8(pwd.pw_dir);
    path.append("/Library");

    sandboxParameters.addPathParameter("HOME_LIBRARY_DIR", fileSystemRepresentation(path).data());

    path.append("/Preferences");

    sandboxParameters.addPathParameter("HOME_LIBRARY_PREFERENCES_DIR", fileSystemRepresentation(path).data());

    switch (sandboxParameters.mode()) {
    case SandboxInitializationParameters::UseDefaultSandboxProfilePath:
    case SandboxInitializationParameters::UseOverrideSandboxProfilePath: {
        String sandboxProfilePath = sandboxParameters.mode() == SandboxInitializationParameters::UseDefaultSandboxProfilePath ? defaultProfilePath : sandboxParameters.overrideSandboxProfilePath();
        if (!sandboxProfilePath.isEmpty()) {
            CString profilePath = fileSystemRepresentation(sandboxProfilePath);
            char* errorBuf;
            if (sandbox_init_with_parameters(profilePath.data(), SANDBOX_NAMED_EXTERNAL, sandboxParameters.namedParameterArray(), &errorBuf)) {
                WTFLogAlways("%s: Couldn't initialize sandbox profile [%s], error '%s'\n", getprogname(), profilePath.data(), errorBuf);
                for (size_t i = 0, count = sandboxParameters.count(); i != count; ++i)
                    WTFLogAlways("%s=%s\n", sandboxParameters.name(i), sandboxParameters.value(i));
                exit(EX_NOPERM);
            }
        }

        break;
    }
    case SandboxInitializationParameters::UseSandboxProfile: {
        char* errorBuf;
        if (sandbox_init_with_parameters(sandboxParameters.sandboxProfile().utf8().data(), 0, sandboxParameters.namedParameterArray(), &errorBuf)) {
            WTFLogAlways("%s: Couldn't initialize sandbox profile, error '%s'\n", getprogname(), errorBuf);
            for (size_t i = 0, count = sandboxParameters.count(); i != count; ++i)
                WTFLogAlways("%s=%s\n", sandboxParameters.name(i), sandboxParameters.value(i));
            exit(EX_NOPERM);
        }

        break;
    }
    }

    // This will override LSFileQuarantineEnabled from Info.plist unless sandbox quarantine is globally disabled.
    OSStatus error = WKEnableSandboxStyleFileQuarantine();
    if (error) {
        WTFLogAlways("%s: Couldn't enable sandbox style file quarantine: %ld\n", getprogname(), (long)error);
        exit(EX_NOPERM);
    }
}

}
