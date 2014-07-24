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

#ifndef WebProcessCreationParameters_h
#define WebProcessCreationParameters_h

#include "CacheModel.h"
#include "SandboxExtension.h"
#include "TextCheckerState.h"
#include <wtf/RetainPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

#if PLATFORM(MAC)
#include "MachPort.h"
#endif

#if USE(SOUP)
#include "HTTPCookieAcceptPolicy.h"
#endif

namespace CoreIPC {
    class ArgumentDecoder;
    class ArgumentEncoder;
}

namespace WebKit {

struct WebProcessCreationParameters {
    WebProcessCreationParameters();

    void encode(CoreIPC::ArgumentEncoder&) const;
    static bool decode(CoreIPC::ArgumentDecoder&, WebProcessCreationParameters&);

    String injectedBundlePath;
    SandboxExtension::Handle injectedBundlePathExtensionHandle;

    String applicationCacheDirectory;    
    SandboxExtension::Handle applicationCacheDirectoryExtensionHandle;
    String databaseDirectory;
    SandboxExtension::Handle databaseDirectoryExtensionHandle;
    String localStorageDirectory;
    SandboxExtension::Handle localStorageDirectoryExtensionHandle;
    String diskCacheDirectory;
    SandboxExtension::Handle diskCacheDirectoryExtensionHandle;
    String cookieStorageDirectory;
    SandboxExtension::Handle cookieStorageDirectoryExtensionHandle;

    Vector<String> urlSchemesRegistererdAsEmptyDocument;
    Vector<String> urlSchemesRegisteredAsSecure;
    Vector<String> urlSchemesForWhichDomainRelaxationIsForbidden;
    Vector<String> urlSchemesRegisteredAsLocal;
    Vector<String> urlSchemesRegisteredAsNoAccess;
    Vector<String> urlSchemesRegisteredAsDisplayIsolated;
    Vector<String> urlSchemesRegisteredAsCORSEnabled;
#if ENABLE(CUSTOM_PROTOCOLS)
    Vector<String> urlSchemesRegisteredForCustomProtocols;
#endif
#if USE(SOUP)
    Vector<String> urlSchemesRegistered;
    String cookiePersistentStoragePath;
    uint32_t cookiePersistentStorageType;
    HTTPCookieAcceptPolicy cookieAcceptPolicy;
    bool ignoreTLSErrors;
#endif

    CacheModel cacheModel;
    bool shouldTrackVisitedLinks;

    bool shouldAlwaysUseComplexTextCodePath;
    bool shouldUseFontSmoothing;

    bool iconDatabaseEnabled;

    double terminationTimeout;

    Vector<String> languages;

    TextCheckerState textCheckerState;

    bool fullKeyboardAccessEnabled;

    double defaultRequestTimeoutInterval;

#if PLATFORM(MAC) || USE(CFNETWORK)
    String uiProcessBundleIdentifier;
#endif

#if PLATFORM(MAC)
    pid_t presenterApplicationPid;

    uint64_t nsURLCacheMemoryCapacity;
    uint64_t nsURLCacheDiskCapacity;

    CoreIPC::MachPort acceleratedCompositingPort;

    String uiProcessBundleResourcePath;
    SandboxExtension::Handle uiProcessBundleResourcePathExtensionHandle;

    bool shouldForceScreenFontSubstitution;
    bool shouldEnableKerningAndLigaturesByDefault;
#endif // PLATFORM(MAC)

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    HashMap<String, bool> notificationPermissions;
#endif

#if ENABLE(NETWORK_PROCESS)
    bool usesNetworkProcess;
#endif

    HashMap<unsigned, double> plugInAutoStartOriginHashes;
    Vector<String> plugInAutoStartOrigins;
};

} // namespace WebKit

#endif // WebProcessCreationParameters_h
