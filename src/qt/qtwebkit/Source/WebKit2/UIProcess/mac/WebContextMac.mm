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
#import "WebContext.h"

#import "PluginProcessManager.h"
#import "SharedWorkerProcessManager.h"
#import "TextChecker.h"
#import "WKBrowsingContextControllerInternal.h"
#import "WKBrowsingContextControllerInternal.h"
#import "WebKitSystemInterface.h"
#import "WebProcessCreationParameters.h"
#import "WebProcessMessages.h"
#import <QuartzCore/CARemoteLayerServer.h>
#import <WebCore/Color.h>
#import <WebCore/FileSystem.h>
#import <WebCore/NotImplemented.h>
#import <WebCore/PlatformPasteboard.h>
#import <sys/param.h>

#if ENABLE(NETWORK_PROCESS)
#import "NetworkProcessCreationParameters.h"
#import "NetworkProcessProxy.h"
#endif


#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090

#if __has_include(<CFNetwork/CFURLProtocolPriv.h>)
#include <CFNetwork/CFURLProtocolPriv.h>
#else
extern "C" Boolean _CFNetworkIsKnownHSTSHostWithSession(CFURLRef url, CFURLStorageSessionRef session);
extern "C" void _CFNetworkResetHSTSHostsWithSession(CFURLStorageSessionRef session);
#endif

#endif

using namespace WebCore;

NSString *WebDatabaseDirectoryDefaultsKey = @"WebDatabaseDirectory";
NSString *WebKitLocalCacheDefaultsKey = @"WebKitLocalCache";
NSString *WebStorageDirectoryDefaultsKey = @"WebKitLocalStorageDatabasePathPreferenceKey";
NSString *WebKitKerningAndLigaturesEnabledByDefaultDefaultsKey = @"WebKitKerningAndLigaturesEnabledByDefault";

static NSString *WebKitApplicationDidChangeAccessibilityEnhancedUserInterfaceNotification = @"NSApplicationDidChangeAccessibilityEnhancedUserInterfaceNotification";

// FIXME: <rdar://problem/9138817> - After this "backwards compatibility" radar is removed, this code should be removed to only return an empty String.
NSString *WebIconDatabaseDirectoryDefaultsKey = @"WebIconDatabaseDirectoryDefaultsKey";

static NSString * const WebKit2HTTPProxyDefaultsKey = @"WebKit2HTTPProxy";
static NSString * const WebKit2HTTPSProxyDefaultsKey = @"WebKit2HTTPSProxy";

namespace WebKit {

NSString *SchemeForCustomProtocolRegisteredNotificationName = @"WebKitSchemeForCustomProtocolRegisteredNotification";
NSString *SchemeForCustomProtocolUnregisteredNotificationName = @"WebKitSchemeForCustomProtocolUnregisteredNotification";

static bool s_applicationIsOccluded = false;
static bool s_applicationWindowModificationsHaveStopped = false;
static bool s_occlusionNotificationHandlersRegistered = false;
static bool s_processSuppressionEnabledForAllContexts = true;

static void registerUserDefaultsIfNeeded()
{
    static bool didRegister;
    if (didRegister)
        return;

    didRegister = true;
    NSMutableDictionary *registrationDictionary = [NSMutableDictionary dictionary];
    
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    [registrationDictionary setObject:[NSNumber numberWithBool:YES] forKey:WebKitKerningAndLigaturesEnabledByDefaultDefaultsKey];
#endif

    [[NSUserDefaults standardUserDefaults] registerDefaults:registrationDictionary];
}

static void updateProcessSuppressionStateOfGlobalChildProcesses()
{
    // The plan is to have all child processes become context specific.  This function
    // can be removed once that is complete.
#if ENABLE(PLUGIN_PROCESS) || ENABLE(SHARED_WORKER_PROCESS)
    bool canEnable = WebContext::canEnableProcessSuppressionForGlobalChildProcesses();
#endif
#if ENABLE(PLUGIN_PROCESS)
    PluginProcessManager::shared().setProcessSuppressionEnabled(canEnable);
#endif
#if ENABLE(SHARED_WORKER_PROCESS)
    SharedWorkerProcessManager::shared().setProcessSuppressionEnabled(canEnable);
#endif
}

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
static void applicationOcclusionStateChanged()
{
    const Vector<WebContext*>& contexts = WebContext::allContexts();
    for (size_t i = 0, count = contexts.size(); i < count; ++i) {
        if (contexts[i]->processSuppressionEnabled())
            contexts[i]->updateProcessSuppressionStateOfChildProcesses();
    }

    if (s_processSuppressionEnabledForAllContexts)
        updateProcessSuppressionStateOfGlobalChildProcesses();
}

static void applicationBecameVisible(uint32_t, void*, uint32_t, void*, uint32_t)
{
    if (!s_applicationIsOccluded)
        return;
    s_applicationIsOccluded = false;
    applicationOcclusionStateChanged();
}

static void applicationBecameOccluded(uint32_t, void*, uint32_t, void*, uint32_t)
{
    if (s_applicationIsOccluded)
        return;
    s_applicationIsOccluded = true;
    applicationOcclusionStateChanged();
}

static void applicationWindowModificationsStarted(uint32_t, void*, uint32_t, void*, uint32_t)
{
    if (!s_applicationWindowModificationsHaveStopped)
        return;
    s_applicationWindowModificationsHaveStopped = false;
    applicationOcclusionStateChanged();
}

static void applicationWindowModificationsStopped(uint32_t, void*, uint32_t, void*, uint32_t)
{
    if (s_applicationWindowModificationsHaveStopped)
        return;
    s_applicationWindowModificationsHaveStopped = true;
    applicationOcclusionStateChanged();
}

struct OcclusionNotificationHandler {
    WKOcclusionNotificationType notificationType;
    WKOcclusionNotificationHandler handler;
    const char *name;
};

static const OcclusionNotificationHandler occlusionNotificationHandlers[] = {
    { WKOcclusionNotificationTypeApplicationBecameVisible, applicationBecameVisible, "Application Became Visible" },
    { WKOcclusionNotificationTypeApplicationBecameOccluded, applicationBecameOccluded, "Application Became Occluded" },
    { WKOcclusionNotificationTypeApplicationWindowModificationsStarted, applicationWindowModificationsStarted, "Application Window Modifications Started" },
    { WKOcclusionNotificationTypeApplicationWindowModificationsStopped, applicationWindowModificationsStopped, "Application Window Modifications Stopped" },
};

#endif

static void registerOcclusionNotificationHandlers()
{
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    for (const OcclusionNotificationHandler& occlusionNotificationHandler : occlusionNotificationHandlers) {
        bool result = WKRegisterOcclusionNotificationHandler(occlusionNotificationHandler.notificationType, occlusionNotificationHandler.handler);
        UNUSED_PARAM(result);
        ASSERT_WITH_MESSAGE(result, "Registration of \"%s\" notification handler failed.\n", occlusionNotificationHandler.name);
    }
#endif
}

static void unregisterOcclusionNotificationHandlers()
{
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    for (const OcclusionNotificationHandler& occlusionNotificationHandler : occlusionNotificationHandlers) {
        bool result = WKUnregisterOcclusionNotificationHandler(occlusionNotificationHandler.notificationType, occlusionNotificationHandler.handler);
        UNUSED_PARAM(result);
        ASSERT_WITH_MESSAGE(result, "Unregistration of \"%s\" notification handler failed.\n", occlusionNotificationHandler.name);
    }
#endif
}

static void enableOcclusionNotifications()
{
    if (s_occlusionNotificationHandlersRegistered)
        return;

    s_occlusionNotificationHandlersRegistered = true;
    registerOcclusionNotificationHandlers();
}

static void disableOcclusionNotifications()
{
    if (!s_occlusionNotificationHandlersRegistered)
        return;

    s_occlusionNotificationHandlersRegistered = false;
    unregisterOcclusionNotificationHandlers();
}

static bool processSuppressionIsEnabledForAnyContext()
{
    bool result = false;
    const Vector<WebContext*>& contexts = WebContext::allContexts();
    for (size_t i = 0, count = contexts.size(); i < count; ++i) {
        if (contexts[i]->processSuppressionEnabled()) {
            result = true;
            break;
        }
    }
    return result;
}

static bool processSuppressionIsEnabledForAllContexts()
{
    bool result = true;
    const Vector<WebContext*>& contexts = WebContext::allContexts();
    for (size_t i = 0, count = contexts.size(); i < count; ++i) {
        if (!contexts[i]->processSuppressionEnabled()) {
            result = false;
            break;
        }
    }
    return result;
}

static bool omitProcessSuppression()
{
    static bool result = [[NSUserDefaults standardUserDefaults] boolForKey:@"WebKit2OmitProcessSuppression"];
    return result;
}

void WebContext::platformInitialize()
{
    registerUserDefaultsIfNeeded();
    registerNotificationObservers();
    ASSERT(m_processSuppressionEnabled);
    enableOcclusionNotifications();
}

String WebContext::platformDefaultApplicationCacheDirectory() const
{
    NSString *appName = [[NSBundle mainBundle] bundleIdentifier];
    if (!appName)
        appName = [[NSProcessInfo processInfo] processName];

    ASSERT(appName);

    char cacheDirectory[MAXPATHLEN];
    size_t cacheDirectoryLen = confstr(_CS_DARWIN_USER_CACHE_DIR, cacheDirectory, MAXPATHLEN);
    if (!cacheDirectoryLen)
        return String();

    NSString *cacheDir = [[NSFileManager defaultManager] stringWithFileSystemRepresentation:cacheDirectory length:cacheDirectoryLen - 1];
    return [cacheDir stringByAppendingPathComponent:appName];
}

void WebContext::platformInitializeWebProcess(WebProcessCreationParameters& parameters)
{
    parameters.presenterApplicationPid = getpid();

    NSURLCache *urlCache = [NSURLCache sharedURLCache];
    parameters.nsURLCacheMemoryCapacity = [urlCache memoryCapacity];
    parameters.nsURLCacheDiskCapacity = [urlCache diskCapacity];

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    parameters.shouldForceScreenFontSubstitution = [[NSUserDefaults standardUserDefaults] boolForKey:@"NSFontDefaultScreenFontSubstitutionEnabled"];
#endif
    parameters.shouldEnableKerningAndLigaturesByDefault = [[NSUserDefaults standardUserDefaults] boolForKey:WebKitKerningAndLigaturesEnabledByDefaultDefaultsKey];

#if USE(ACCELERATED_COMPOSITING) && HAVE(HOSTED_CORE_ANIMATION)
    mach_port_t renderServerPort = [[CARemoteLayerServer sharedServer] serverPort];
    if (renderServerPort != MACH_PORT_NULL)
        parameters.acceleratedCompositingPort = CoreIPC::MachPort(renderServerPort, MACH_MSG_TYPE_COPY_SEND);
#endif

    // FIXME: This should really be configurable; we shouldn't just blindly allow read access to the UI process bundle.
    parameters.uiProcessBundleResourcePath = [[NSBundle mainBundle] resourcePath];
    SandboxExtension::createHandle(parameters.uiProcessBundleResourcePath, SandboxExtension::ReadOnly, parameters.uiProcessBundleResourcePathExtensionHandle);

    parameters.uiProcessBundleIdentifier = String([[NSBundle mainBundle] bundleIdentifier]);

#if ENABLE(NETWORK_PROCESS)
    if (!m_usesNetworkProcess) {
#endif
        for (NSString *scheme in [WKBrowsingContextController customSchemes])
            parameters.urlSchemesRegisteredForCustomProtocols.append(scheme);
#if ENABLE(NETWORK_PROCESS)
    }
#endif
}

#if ENABLE(NETWORK_PROCESS)
void WebContext::platformInitializeNetworkProcess(NetworkProcessCreationParameters& parameters)
{
    NSURLCache *urlCache = [NSURLCache sharedURLCache];
    parameters.nsURLCacheMemoryCapacity = [urlCache memoryCapacity];
    parameters.nsURLCacheDiskCapacity = [urlCache diskCapacity];

    parameters.parentProcessName = [[NSProcessInfo processInfo] processName];
    parameters.uiProcessBundleIdentifier = [[NSBundle mainBundle] bundleIdentifier];

    for (NSString *scheme in [WKBrowsingContextController customSchemes])
        parameters.urlSchemesRegisteredForCustomProtocols.append(scheme);

    parameters.httpProxy = [[NSUserDefaults standardUserDefaults] stringForKey:WebKit2HTTPProxyDefaultsKey];
    parameters.httpsProxy = [[NSUserDefaults standardUserDefaults] stringForKey:WebKit2HTTPSProxyDefaultsKey];
}
#endif

void WebContext::platformInvalidateContext()
{
    unregisterNotificationObservers();
}

String WebContext::platformDefaultDiskCacheDirectory() const
{
    RetainPtr<NSString> cachePath = adoptNS((NSString *)WKCopyFoundationCacheDirectory());
    if (!cachePath)
        cachePath = @"~/Library/Caches/com.apple.WebKit2.WebProcess";

    return [cachePath.get() stringByStandardizingPath];
}

String WebContext::platformDefaultCookieStorageDirectory() const
{
    notImplemented();
    return [@"" stringByStandardizingPath];
}

String WebContext::platformDefaultDatabaseDirectory() const
{
    NSString *databasesDirectory = [[NSUserDefaults standardUserDefaults] objectForKey:WebDatabaseDirectoryDefaultsKey];
    if (!databasesDirectory || ![databasesDirectory isKindOfClass:[NSString class]])
        databasesDirectory = @"~/Library/WebKit/Databases";
    return [databasesDirectory stringByStandardizingPath];
}

String WebContext::platformDefaultIconDatabasePath() const
{
    // FIXME: <rdar://problem/9138817> - After this "backwards compatibility" radar is removed, this code should be removed to only return an empty String.
    NSString *databasesDirectory = [[NSUserDefaults standardUserDefaults] objectForKey:WebIconDatabaseDirectoryDefaultsKey];
    if (!databasesDirectory || ![databasesDirectory isKindOfClass:[NSString class]])
        databasesDirectory = @"~/Library/Icons/WebpageIcons.db";
    return [databasesDirectory stringByStandardizingPath];
}

String WebContext::platformDefaultLocalStorageDirectory() const
{
    NSString *localStorageDirectory = [[NSUserDefaults standardUserDefaults] objectForKey:WebStorageDirectoryDefaultsKey];
    if (!localStorageDirectory || ![localStorageDirectory isKindOfClass:[NSString class]])
        localStorageDirectory = @"~/Library/WebKit/LocalStorage";
    return [localStorageDirectory stringByStandardizingPath];
}

bool WebContext::omitPDFSupport()
{
    // Since this is a "secret default" we don't bother registering it.
    return [[NSUserDefaults standardUserDefaults] boolForKey:@"WebKitOmitPDFSupport"];
}

void WebContext::getPasteboardTypes(const String& pasteboardName, Vector<String>& pasteboardTypes)
{
    PlatformPasteboard(pasteboardName).getTypes(pasteboardTypes);
}

void WebContext::getPasteboardPathnamesForType(const String& pasteboardName, const String& pasteboardType, Vector<String>& pathnames)
{
    PlatformPasteboard(pasteboardName).getPathnamesForType(pathnames, pasteboardType);
}

void WebContext::getPasteboardStringForType(const String& pasteboardName, const String& pasteboardType, String& string)
{
    string = PlatformPasteboard(pasteboardName).stringForType(pasteboardType);
}

void WebContext::getPasteboardBufferForType(const String& pasteboardName, const String& pasteboardType, SharedMemory::Handle& handle, uint64_t& size)
{
    RefPtr<SharedBuffer> buffer = PlatformPasteboard(pasteboardName).bufferForType(pasteboardType);
    if (!buffer)
        return;
    size = buffer->size();
    RefPtr<SharedMemory> sharedMemoryBuffer = SharedMemory::create(size);
    memcpy(sharedMemoryBuffer->data(), buffer->data(), size);
    sharedMemoryBuffer->createHandle(handle, SharedMemory::ReadOnly);
}

void WebContext::pasteboardCopy(const String& fromPasteboard, const String& toPasteboard)
{
    PlatformPasteboard(toPasteboard).copy(fromPasteboard);
}

void WebContext::getPasteboardChangeCount(const String& pasteboardName, uint64_t& changeCount)
{
    changeCount = PlatformPasteboard(pasteboardName).changeCount();
}

void WebContext::getPasteboardUniqueName(String& pasteboardName)
{
    pasteboardName = PlatformPasteboard::uniqueName();
}

void WebContext::getPasteboardColor(const String& pasteboardName, WebCore::Color& color)
{
    color = PlatformPasteboard(pasteboardName).color();    
}

void WebContext::getPasteboardURL(const String& pasteboardName, WTF::String& urlString)
{
    urlString = PlatformPasteboard(pasteboardName).url().string();
}

void WebContext::addPasteboardTypes(const String& pasteboardName, const Vector<String>& pasteboardTypes)
{
    PlatformPasteboard(pasteboardName).addTypes(pasteboardTypes);
}

void WebContext::setPasteboardTypes(const String& pasteboardName, const Vector<String>& pasteboardTypes)
{
    PlatformPasteboard(pasteboardName).setTypes(pasteboardTypes);
}

void WebContext::setPasteboardPathnamesForType(const String& pasteboardName, const String& pasteboardType, const Vector<String>& pathnames)
{
    PlatformPasteboard(pasteboardName).setPathnamesForType(pathnames, pasteboardType);
}

void WebContext::setPasteboardStringForType(const String& pasteboardName, const String& pasteboardType, const String& string)
{
    PlatformPasteboard(pasteboardName).setStringForType(string, pasteboardType);    
}

void WebContext::setPasteboardBufferForType(const String& pasteboardName, const String& pasteboardType, const SharedMemory::Handle& handle, uint64_t size)
{
    if (handle.isNull()) {
        PlatformPasteboard(pasteboardName).setBufferForType(0, pasteboardType);
        return;
    }
    RefPtr<SharedMemory> sharedMemoryBuffer = SharedMemory::create(handle, SharedMemory::ReadOnly);
    RefPtr<SharedBuffer> buffer = SharedBuffer::create(static_cast<unsigned char *>(sharedMemoryBuffer->data()), size);
    PlatformPasteboard(pasteboardName).setBufferForType(buffer, pasteboardType);
}

void WebContext::setProcessSuppressionEnabled(bool enabled)
{
    if (m_processSuppressionEnabled == enabled)
        return;
    m_processSuppressionEnabled = enabled;
    processSuppressionEnabledChanged();
}

void WebContext::updateProcessSuppressionStateOfChildProcesses()
{
#if ENABLE(NETWORK_PROCESS)
    bool canEnable = canEnableProcessSuppressionForNetworkProcess();
    if (usesNetworkProcess() && networkProcess())
        networkProcess()->setProcessSuppressionEnabled(canEnable);
#endif
    size_t processCount = m_processes.size();
    for (size_t i = 0; i < processCount; ++i) {
        WebProcessProxy* process = m_processes[i].get();
        process->updateProcessSuppressionState();
    }
}

bool WebContext::canEnableProcessSuppressionForNetworkProcess() const
{
    return (s_applicationIsOccluded || s_applicationWindowModificationsHaveStopped) && m_processSuppressionEnabled && !omitProcessSuppression();
}

bool WebContext::canEnableProcessSuppressionForWebProcess(const WebKit::WebProcessProxy *webProcess) const
{
    return (s_applicationIsOccluded || s_applicationWindowModificationsHaveStopped || webProcess->allPagesAreProcessSuppressible())
           && m_processSuppressionEnabled && !omitProcessSuppression();
}

bool WebContext::canEnableProcessSuppressionForGlobalChildProcesses()
{
    return (s_applicationIsOccluded || s_applicationWindowModificationsHaveStopped) && s_processSuppressionEnabledForAllContexts && !omitProcessSuppression();
}

void WebContext::processSuppressionEnabledChanged()
{
    updateProcessSuppressionStateOfChildProcesses();

    if (processSuppressionIsEnabledForAnyContext())
        enableOcclusionNotifications();
    else
        disableOcclusionNotifications();

    bool newProcessSuppressionEnabledForAllContexts = processSuppressionIsEnabledForAllContexts();
    if (s_processSuppressionEnabledForAllContexts != newProcessSuppressionEnabledForAllContexts) {
        s_processSuppressionEnabledForAllContexts = newProcessSuppressionEnabledForAllContexts;
        updateProcessSuppressionStateOfGlobalChildProcesses();
    }
}

void WebContext::registerNotificationObservers()
{
    m_customSchemeRegisteredObserver = [[NSNotificationCenter defaultCenter] addObserverForName:WebKit::SchemeForCustomProtocolRegisteredNotificationName object:nil queue:[NSOperationQueue currentQueue] usingBlock:^(NSNotification *notification) {
        NSString *scheme = [notification object];
        ASSERT([scheme isKindOfClass:[NSString class]]);
        registerSchemeForCustomProtocol(scheme);
    }];

    m_customSchemeUnregisteredObserver = [[NSNotificationCenter defaultCenter] addObserverForName:WebKit::SchemeForCustomProtocolUnregisteredNotificationName object:nil queue:[NSOperationQueue currentQueue] usingBlock:^(NSNotification *notification) {
        NSString *scheme = [notification object];
        ASSERT([scheme isKindOfClass:[NSString class]]);
        unregisterSchemeForCustomProtocol(scheme);
    }];

    // Listen for enhanced accessibility changes and propagate them to the WebProcess.
    m_enhancedAccessibilityObserver = [[NSNotificationCenter defaultCenter] addObserverForName:WebKitApplicationDidChangeAccessibilityEnhancedUserInterfaceNotification object:nil queue:[NSOperationQueue currentQueue] usingBlock:^(NSNotification *note) {
        setEnhancedAccessibility([[[note userInfo] objectForKey:@"AXEnhancedUserInterface"] boolValue]);
    }];

    m_automaticTextReplacementNotificationObserver = [[NSNotificationCenter defaultCenter] addObserverForName:NSSpellCheckerDidChangeAutomaticTextReplacementNotification object:nil queue:[NSOperationQueue currentQueue] usingBlock:^(NSNotification *notification) {
        TextChecker::didChangeAutomaticTextReplacementEnabled();
        textCheckerStateChanged();
    }];
    
    m_automaticSpellingCorrectionNotificationObserver = [[NSNotificationCenter defaultCenter] addObserverForName:NSSpellCheckerDidChangeAutomaticSpellingCorrectionNotification object:nil queue:[NSOperationQueue currentQueue] usingBlock:^(NSNotification *notification) {
        TextChecker::didChangeAutomaticSpellingCorrectionEnabled();
        textCheckerStateChanged();
    }];

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    m_automaticQuoteSubstitutionNotificationObserver = [[NSNotificationCenter defaultCenter] addObserverForName:NSSpellCheckerDidChangeAutomaticQuoteSubstitutionNotification object:nil queue:[NSOperationQueue currentQueue] usingBlock:^(NSNotification *notification) {
        TextChecker::didChangeAutomaticQuoteSubstitutionEnabled();
        textCheckerStateChanged();
    }];

    m_automaticDashSubstitutionNotificationObserver = [[NSNotificationCenter defaultCenter] addObserverForName:NSSpellCheckerDidChangeAutomaticDashSubstitutionNotification object:nil queue:[NSOperationQueue currentQueue] usingBlock:^(NSNotification *notification) {
        TextChecker::didChangeAutomaticDashSubstitutionEnabled();
        textCheckerStateChanged();
    }];
#endif
}

void WebContext::unregisterNotificationObservers()
{
    [[NSNotificationCenter defaultCenter] removeObserver:m_customSchemeRegisteredObserver.get()];
    [[NSNotificationCenter defaultCenter] removeObserver:m_customSchemeUnregisteredObserver.get()];
    [[NSNotificationCenter defaultCenter] removeObserver:m_enhancedAccessibilityObserver.get()];
    
    [[NSNotificationCenter defaultCenter] removeObserver:m_automaticTextReplacementNotificationObserver.get()];
    [[NSNotificationCenter defaultCenter] removeObserver:m_automaticSpellingCorrectionNotificationObserver.get()];
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    [[NSNotificationCenter defaultCenter] removeObserver:m_automaticQuoteSubstitutionNotificationObserver.get()];
    [[NSNotificationCenter defaultCenter] removeObserver:m_automaticDashSubstitutionNotificationObserver.get()];
#endif
}

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
static CFURLStorageSessionRef privateBrowsingSession()
{
    static CFURLStorageSessionRef session;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        NSString *identifier = [NSString stringWithFormat:@"%@.PrivateBrowsing", [[NSBundle mainBundle] bundleIdentifier]];

        session = WKCreatePrivateStorageSession((CFStringRef)identifier);
    });

    return session;
}
#endif

bool WebContext::isURLKnownHSTSHost(const String& urlString, bool privateBrowsingEnabled) const
{
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    RetainPtr<CFURLRef> url = KURL(KURL(), urlString).createCFURL();

    return _CFNetworkIsKnownHSTSHostWithSession(url.get(), privateBrowsingEnabled ? privateBrowsingSession() : nullptr);
#else
    return false;
#endif
}

void WebContext::resetHSTSHosts()
{
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    _CFNetworkResetHSTSHostsWithSession(nullptr);
    _CFNetworkResetHSTSHostsWithSession(privateBrowsingSession());
#endif
}

} // namespace WebKit
