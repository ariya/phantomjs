/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#import "PluginProcessProxy.h"

#if ENABLE(PLUGIN_PROCESS)

#import "DynamicLinkerEnvironmentExtractor.h"
#import "EnvironmentVariables.h"
#import "PluginProcessCreationParameters.h"
#import "PluginProcessMessages.h"
#import "WebKitSystemInterface.h"
#import <WebCore/FileSystem.h>
#import <WebCore/KURL.h>
#import <WebCore/RuntimeApplicationChecks.h>
#import <crt_externs.h>
#import <mach-o/dyld.h>
#import <spawn.h>
#import <wtf/text/CString.h>

#import <QuartzCore/CARemoteLayerServer.h>

@interface WKPlaceholderModalWindow : NSWindow 
@end

@implementation WKPlaceholderModalWindow

// Prevent NSApp from calling requestUserAttention: when the window is shown 
// modally, even if the app is inactive. See 6823049.
- (BOOL)_wantsUserAttention
{
    return NO;   
}

@end

using namespace WebCore;

namespace WebKit {
    
bool PluginProcessProxy::pluginNeedsExecutableHeap(const PluginModuleInfo& pluginInfo)
{
    static bool forceNonexecutableHeapForPlugins = [[NSUserDefaults standardUserDefaults] boolForKey:@"ForceNonexecutableHeapForPlugins"];
    if (forceNonexecutableHeapForPlugins)
        return false;
    
    if (pluginInfo.bundleIdentifier == "com.apple.QuickTime Plugin.plugin")
        return false;
    
    return true;
}

bool PluginProcessProxy::createPropertyListFile(const PluginModuleInfo& plugin)
{
    NSBundle *webKit2Bundle = [NSBundle bundleWithIdentifier:@"com.apple.WebKit2"];
    NSString *frameworksPath = [[webKit2Bundle bundlePath] stringByDeletingLastPathComponent];
    const char* frameworkExecutablePath = [[webKit2Bundle executablePath] fileSystemRepresentation];
    
    NSString *processPath = [webKit2Bundle pathForAuxiliaryExecutable:@"PluginProcess.app"];
    NSString *processAppExecutablePath = [[NSBundle bundleWithPath:processPath] executablePath];

    CString pluginPathString = fileSystemRepresentation(plugin.path);

    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);

    cpu_type_t cpuTypes[] = { plugin.pluginArchitecture };    
    size_t outCount = 0;
    posix_spawnattr_setbinpref_np(&attr, 1, cpuTypes, &outCount);

    EnvironmentVariables environmentVariables;

    DynamicLinkerEnvironmentExtractor environmentExtractor([[NSBundle mainBundle] executablePath], _NSGetMachExecuteHeader()->cputype);
    environmentExtractor.getExtractedEnvironmentVariables(environmentVariables);
    
    // To make engineering builds work, if the path is outside of /System set up
    // DYLD_FRAMEWORK_PATH to pick up other frameworks, but don't do it for the
    // production configuration because it involves extra file system access.
    if (![frameworksPath hasPrefix:@"/System/"])
        environmentVariables.appendValue("DYLD_FRAMEWORK_PATH", [frameworksPath fileSystemRepresentation], ':');

    const char* args[] = { [processAppExecutablePath fileSystemRepresentation], frameworkExecutablePath, "-type", "pluginprocess", "-createPluginMIMETypesPreferences", pluginPathString.data(), 0 };

    pid_t pid;
    int result = posix_spawn(&pid, args[0], 0, &attr, const_cast<char* const*>(args), environmentVariables.environmentPointer());
    posix_spawnattr_destroy(&attr);

    if (result)
        return false;
    int status;
    if (waitpid(pid, &status, 0) < 0)
        return false;

    if (!WIFEXITED(status))
        return false;

    if (WEXITSTATUS(status) != EXIT_SUCCESS)
        return false;

    return true;
}

#if HAVE(XPC)
static bool shouldUseXPC()
{
    if (id value = [[NSUserDefaults standardUserDefaults] objectForKey:@"WebKit2UseXPCServiceForWebProcess"])
        return [value boolValue];

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    // FIXME: Temporary workaround for <rdar://problem/13236883>
    if (applicationIsSafari())
        return false;

    return true;
#else
    return false;
#endif
}
#endif

void PluginProcessProxy::platformGetLaunchOptions(ProcessLauncher::LaunchOptions& launchOptions, const PluginProcessAttributes& pluginProcessAttributes)
{
    launchOptions.architecture = pluginProcessAttributes.moduleInfo.pluginArchitecture;
    launchOptions.executableHeap = PluginProcessProxy::pluginNeedsExecutableHeap(pluginProcessAttributes.moduleInfo);
    launchOptions.extraInitializationData.add("plugin-path", pluginProcessAttributes.moduleInfo.path);

    // FIXME: Don't allow this if the UI process is sandboxed.
    if (pluginProcessAttributes.sandboxPolicy == PluginProcessSandboxPolicyUnsandboxed)
        launchOptions.extraInitializationData.add("disable-sandbox", "1");

#if HAVE(XPC)
    launchOptions.useXPC = shouldUseXPC();
#endif
}

void PluginProcessProxy::platformInitializePluginProcess(PluginProcessCreationParameters& parameters)
{
    // For now only Flash is known to behave with asynchronous plug-in initialization.
    parameters.supportsAsynchronousPluginInitialization = m_pluginProcessAttributes.moduleInfo.bundleIdentifier == "com.macromedia.Flash Player.plugin";

#if USE(ACCELERATED_COMPOSITING) && HAVE(HOSTED_CORE_ANIMATION)
    mach_port_t renderServerPort = [[CARemoteLayerServer sharedServer] serverPort];
    if (renderServerPort != MACH_PORT_NULL)
        parameters.acceleratedCompositingPort = CoreIPC::MachPort(renderServerPort, MACH_MSG_TYPE_COPY_SEND);
#endif
}

bool PluginProcessProxy::getPluginProcessSerialNumber(ProcessSerialNumber& pluginProcessSerialNumber)
{
    pid_t pluginProcessPID = processIdentifier();
#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
    return GetProcessForPID(pluginProcessPID, &pluginProcessSerialNumber) == noErr;
#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif
}

void PluginProcessProxy::makePluginProcessTheFrontProcess()
{
    ProcessSerialNumber pluginProcessSerialNumber;
    if (!getPluginProcessSerialNumber(pluginProcessSerialNumber))
        return;

#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
    SetFrontProcess(&pluginProcessSerialNumber);
#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif
}

void PluginProcessProxy::makeUIProcessTheFrontProcess()
{
    ProcessSerialNumber processSerialNumber;
#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
    GetCurrentProcess(&processSerialNumber);
    SetFrontProcess(&processSerialNumber);            
#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif
}

void PluginProcessProxy::setFullscreenWindowIsShowing(bool fullscreenWindowIsShowing)
{
    if (m_fullscreenWindowIsShowing == fullscreenWindowIsShowing)
        return;

    m_fullscreenWindowIsShowing = fullscreenWindowIsShowing;
    if (m_fullscreenWindowIsShowing)
        enterFullscreen();
    else
        exitFullscreen();
}

void PluginProcessProxy::enterFullscreen()
{
    // Get the current presentation options.
    m_preFullscreenAppPresentationOptions = [NSApp presentationOptions];

    // Figure out which presentation options to use.
    unsigned presentationOptions = m_preFullscreenAppPresentationOptions & ~(NSApplicationPresentationAutoHideDock | NSApplicationPresentationAutoHideMenuBar);
    presentationOptions |= NSApplicationPresentationHideDock | NSApplicationPresentationHideMenuBar;

    [NSApp setPresentationOptions:presentationOptions];
    makePluginProcessTheFrontProcess();
}

void PluginProcessProxy::exitFullscreen()
{
    // If the plug-in host is the current application then we should bring ourselves to the front when it exits full-screen mode.
    ProcessSerialNumber frontProcessSerialNumber;
#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
    GetFrontProcess(&frontProcessSerialNumber);
#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif

    // The UI process must be the front process in order to change the presentation mode.
    makeUIProcessTheFrontProcess();
    [NSApp setPresentationOptions:m_preFullscreenAppPresentationOptions];

    ProcessSerialNumber pluginProcessSerialNumber;
    if (!getPluginProcessSerialNumber(pluginProcessSerialNumber))
        return;

    // If the plug-in process was not the front process, switch back to the previous front process.
    // (Otherwise we'll keep the UI process as the front process).
    Boolean isPluginProcessFrontProcess;
#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
    SameProcess(&frontProcessSerialNumber, &pluginProcessSerialNumber, &isPluginProcessFrontProcess);
#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif
    if (!isPluginProcessFrontProcess) {
#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
        SetFrontProcess(&frontProcessSerialNumber);
#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif
    }
}

void PluginProcessProxy::setModalWindowIsShowing(bool modalWindowIsShowing)
{
    if (modalWindowIsShowing == m_modalWindowIsShowing)
        return;
    
    m_modalWindowIsShowing = modalWindowIsShowing;
    
    if (m_modalWindowIsShowing)
        beginModal();
    else
        endModal();
}

void PluginProcessProxy::beginModal()
{
    ASSERT(!m_placeholderWindow);
    ASSERT(!m_activationObserver);
    
    m_placeholderWindow = adoptNS([[WKPlaceholderModalWindow alloc] initWithContentRect:NSMakeRect(0, 0, 1, 1) styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:YES]);
    [m_placeholderWindow.get() setReleasedWhenClosed:NO];
    
    m_activationObserver = [[NSNotificationCenter defaultCenter] addObserverForName:NSApplicationWillBecomeActiveNotification object:NSApp queue:nil
                                                                         usingBlock:^(NSNotification *){ applicationDidBecomeActive(); }];

    // The call to -[NSApp runModalForWindow:] below will run a nested run loop, and if the plug-in process
    // crashes the PluginProcessProxy object can be destroyed. Protect against this here.
    RefPtr<PluginProcessProxy> protect(this);

    [NSApp runModalForWindow:m_placeholderWindow.get()];
    
    [m_placeholderWindow.get() orderOut:nil];
    m_placeholderWindow = nullptr;
}

void PluginProcessProxy::endModal()
{
    ASSERT(m_placeholderWindow);
    ASSERT(m_activationObserver);
    
    [[NSNotificationCenter defaultCenter] removeObserver:m_activationObserver.get()];
    m_activationObserver = nullptr;
    
    [NSApp stopModal];

    makeUIProcessTheFrontProcess();
}
    
void PluginProcessProxy::applicationDidBecomeActive()
{
    makePluginProcessTheFrontProcess();
}

void PluginProcessProxy::setProcessSuppressionEnabled(bool processSuppressionEnabled)
{
    if (!isValid())
        return;

    m_connection->send(Messages::PluginProcess::SetProcessSuppressionEnabled(processSuppressionEnabled), 0);
}

void PluginProcessProxy::openPluginPreferencePane()
{
    if (!m_pluginProcessAttributes.moduleInfo.preferencePanePath)
        return;

    NSURL *preferenceURL = [NSURL fileURLWithPath:m_pluginProcessAttributes.moduleInfo.preferencePanePath];
    if (!preferenceURL) {
        LOG_ERROR("Creating URL for preference pane path \"%@\" failed.", (NSString *)m_pluginProcessAttributes.moduleInfo.preferencePanePath);
        return;
    }

    NSArray *preferenceURLs = [NSArray arrayWithObject:preferenceURL];

    LSLaunchURLSpec prefSpec;
    prefSpec.appURL = 0;
    prefSpec.itemURLs = reinterpret_cast<CFArrayRef>(preferenceURLs);
    prefSpec.passThruParams = 0;
    prefSpec.launchFlags = kLSLaunchAsync | kLSLaunchDontAddToRecents;
    prefSpec.asyncRefCon = 0;

    OSStatus error = LSOpenFromURLSpec(&prefSpec, 0);
    if (error != noErr)
        LOG_ERROR("LSOpenFromURLSpec to open \"%@\" failed with error %d.", (NSString *)m_pluginProcessAttributes.moduleInfo.preferencePanePath, error);
}

static bool isFlashUpdater(const String& launchPath, const Vector<String>& arguments)
{
    if (launchPath != "/Applications/Utilities/Adobe Flash Player Install Manager.app/Contents/MacOS/Adobe Flash Player Install Manager")
        return false;

    if (arguments.size() != 1)
        return false;

    if (arguments[0] != "-update")
        return false;

    return true;
}

static bool shouldLaunchProcess(const PluginProcessAttributes& pluginProcessAttributes, const String& launchPath, const Vector<String>& arguments)
{
    if (pluginProcessAttributes.moduleInfo.bundleIdentifier == "com.macromedia.Flash Player.plugin")
        return isFlashUpdater(launchPath, arguments);

    return false;
}

void PluginProcessProxy::launchProcess(const String& launchPath, const Vector<String>& arguments, bool& result)
{
    if (!shouldLaunchProcess(m_pluginProcessAttributes, launchPath, arguments)) {
        result = false;
        return;
    }

    result = true;

    RetainPtr<NSMutableArray> argumentsArray = adoptNS([[NSMutableArray alloc] initWithCapacity:arguments.size()]);
    for (size_t i = 0; i < arguments.size(); ++i)
        [argumentsArray addObject:(NSString *)arguments[i]];

    [NSTask launchedTaskWithLaunchPath:launchPath arguments:argumentsArray.get()];
}

static bool isJavaUpdaterURL(const PluginProcessAttributes& pluginProcessAttributes, const String& urlString)
{
    NSURL *url = [NSURL URLWithString:urlString];
    if (![url isFileURL])
        return false;

    NSString *javaUpdaterPath = [NSString pathWithComponents:[NSArray arrayWithObjects:(NSString *)pluginProcessAttributes.moduleInfo.path, @"Contents/Resources/Java Updater.app", nil]];
    return [url.path isEqualToString:javaUpdaterPath];
}

static bool shouldLaunchApplicationAtURL(const PluginProcessAttributes& pluginProcessAttributes, const String& urlString)
{
    if (pluginProcessAttributes.moduleInfo.bundleIdentifier == "com.oracle.java.JavaAppletPlugin")
        return isJavaUpdaterURL(pluginProcessAttributes, urlString);

    return false;
}

void PluginProcessProxy::launchApplicationAtURL(const String& urlString, const Vector<String>& arguments, bool& result)
{
    if (!shouldLaunchApplicationAtURL(m_pluginProcessAttributes, urlString)) {
        result = false;
        return;
    }

    result = true;

    RetainPtr<NSMutableArray> argumentsArray = adoptNS([[NSMutableArray alloc] initWithCapacity:arguments.size()]);
    for (size_t i = 0; i < arguments.size(); ++i)
        [argumentsArray addObject:(NSString *)arguments[i]];

    NSDictionary *configuration = [NSDictionary dictionaryWithObject:argumentsArray.get() forKey:NSWorkspaceLaunchConfigurationArguments];
    [[NSWorkspace sharedWorkspace] launchApplicationAtURL:[NSURL URLWithString:urlString] options:NSWorkspaceLaunchAsync configuration:configuration error:nullptr];
}

static bool isSilverlightPreferencesURL(const PluginProcessAttributes& pluginProcessAttributes, const String& urlString)
{
    NSURL *silverlightPreferencesURL = [NSURL fileURLWithPathComponents:[NSArray arrayWithObjects:(NSString *)pluginProcessAttributes.moduleInfo.path, @"Contents/Resources/Silverlight Preferences.app", nil]];

    return [[NSURL URLWithString:urlString] isEqual:silverlightPreferencesURL];
}

static bool shouldOpenURL(const PluginProcessAttributes& pluginProcessAttributes, const String& urlString)
{
    if (pluginProcessAttributes.moduleInfo.bundleIdentifier == "com.microsoft.SilverlightPlugin")
        return isSilverlightPreferencesURL(pluginProcessAttributes, urlString);

    return false;
}

void PluginProcessProxy::openURL(const String& urlString, bool& result, int32_t& status, String& launchedURLString)
{
    if (!shouldOpenURL(m_pluginProcessAttributes, urlString)) {
        result = false;
        return;
    }

    result = true;
    CFURLRef launchedURL;
    status = LSOpenCFURLRef(KURL(ParsedURLString, urlString).createCFURL().get(), &launchedURL);

    if (launchedURL) {
        launchedURLString = KURL(launchedURL).string();
        CFRelease(launchedURL);
    }

    result = false;
}

} // namespace WebKit

#endif // ENABLE(PLUGIN_PROCESS)
