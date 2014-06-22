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

#include "config.h"
#include "WebProcessCreationParameters.h"

#include "ArgumentCoders.h"

namespace WebKit {

WebProcessCreationParameters::WebProcessCreationParameters()
    : shouldTrackVisitedLinks(false)
    , shouldAlwaysUseComplexTextCodePath(false)
    , shouldUseFontSmoothing(true)
    , defaultRequestTimeoutInterval(INT_MAX)
#if PLATFORM(MAC)
    , nsURLCacheMemoryCapacity(0)
    , nsURLCacheDiskCapacity(0)
    , shouldForceScreenFontSubstitution(false)
    , shouldEnableKerningAndLigaturesByDefault(false)
#endif
#if ENABLE(NETWORK_PROCESS)
    , usesNetworkProcess(false)
#endif
{
}

void WebProcessCreationParameters::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << injectedBundlePath;
    encoder << injectedBundlePathExtensionHandle;
    encoder << applicationCacheDirectory;
    encoder << applicationCacheDirectoryExtensionHandle;
    encoder << databaseDirectory;
    encoder << databaseDirectoryExtensionHandle;
    encoder << localStorageDirectory;
    encoder << localStorageDirectoryExtensionHandle;
    encoder << diskCacheDirectory;
    encoder << diskCacheDirectoryExtensionHandle;
    encoder << cookieStorageDirectory;
    encoder << cookieStorageDirectoryExtensionHandle;
    encoder << urlSchemesRegistererdAsEmptyDocument;
    encoder << urlSchemesRegisteredAsSecure;
    encoder << urlSchemesForWhichDomainRelaxationIsForbidden;
    encoder << urlSchemesRegisteredAsLocal;
    encoder << urlSchemesRegisteredAsNoAccess;
    encoder << urlSchemesRegisteredAsDisplayIsolated;
    encoder << urlSchemesRegisteredAsCORSEnabled;
#if ENABLE(CUSTOM_PROTOCOLS)
    encoder << urlSchemesRegisteredForCustomProtocols;
#endif
#if USE(SOUP)
    encoder << urlSchemesRegistered;
    encoder << cookiePersistentStoragePath;
    encoder << cookiePersistentStorageType;
    encoder.encodeEnum(cookieAcceptPolicy);
    encoder << ignoreTLSErrors;
#endif
    encoder.encodeEnum(cacheModel);
    encoder << shouldTrackVisitedLinks;
    encoder << shouldAlwaysUseComplexTextCodePath;
    encoder << shouldUseFontSmoothing;
    encoder << iconDatabaseEnabled;
    encoder << terminationTimeout;
    encoder << languages;
    encoder << textCheckerState;
    encoder << fullKeyboardAccessEnabled;
    encoder << defaultRequestTimeoutInterval;
#if PLATFORM(MAC) || USE(CFNETWORK)
    encoder << uiProcessBundleIdentifier;
#endif
#if PLATFORM(MAC)
    encoder << presenterApplicationPid;
    encoder << nsURLCacheMemoryCapacity;
    encoder << nsURLCacheDiskCapacity;
    encoder << acceleratedCompositingPort;
    encoder << uiProcessBundleResourcePath;
    encoder << uiProcessBundleResourcePathExtensionHandle;
    encoder << shouldForceScreenFontSubstitution;
    encoder << shouldEnableKerningAndLigaturesByDefault;
#endif

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    encoder << notificationPermissions;
#endif

#if ENABLE(NETWORK_PROCESS)
    encoder << usesNetworkProcess;
#endif

    encoder << plugInAutoStartOriginHashes;
    encoder << plugInAutoStartOrigins;
}

bool WebProcessCreationParameters::decode(CoreIPC::ArgumentDecoder& decoder, WebProcessCreationParameters& parameters)
{
    if (!decoder.decode(parameters.injectedBundlePath))
        return false;
    if (!decoder.decode(parameters.injectedBundlePathExtensionHandle))
        return false;
    if (!decoder.decode(parameters.applicationCacheDirectory))
        return false;
    if (!decoder.decode(parameters.applicationCacheDirectoryExtensionHandle))
        return false;
    if (!decoder.decode(parameters.databaseDirectory))
        return false;
    if (!decoder.decode(parameters.databaseDirectoryExtensionHandle))
        return false;
    if (!decoder.decode(parameters.localStorageDirectory))
        return false;
    if (!decoder.decode(parameters.localStorageDirectoryExtensionHandle))
        return false;
    if (!decoder.decode(parameters.diskCacheDirectory))
        return false;
    if (!decoder.decode(parameters.diskCacheDirectoryExtensionHandle))
        return false;
    if (!decoder.decode(parameters.cookieStorageDirectory))
        return false;
    if (!decoder.decode(parameters.cookieStorageDirectoryExtensionHandle))
        return false;
    if (!decoder.decode(parameters.urlSchemesRegistererdAsEmptyDocument))
        return false;
    if (!decoder.decode(parameters.urlSchemesRegisteredAsSecure))
        return false;
    if (!decoder.decode(parameters.urlSchemesForWhichDomainRelaxationIsForbidden))
        return false;
    if (!decoder.decode(parameters.urlSchemesRegisteredAsLocal))
        return false;
    if (!decoder.decode(parameters.urlSchemesRegisteredAsNoAccess))
        return false;
    if (!decoder.decode(parameters.urlSchemesRegisteredAsDisplayIsolated))
        return false;
    if (!decoder.decode(parameters.urlSchemesRegisteredAsCORSEnabled))
        return false;
#if ENABLE(CUSTOM_PROTOCOLS)
    if (!decoder.decode(parameters.urlSchemesRegisteredForCustomProtocols))
        return false;
#endif
#if USE(SOUP)
    if (!decoder.decode(parameters.urlSchemesRegistered))
        return false;
    if (!decoder.decode(parameters.cookiePersistentStoragePath))
        return false;
    if (!decoder.decode(parameters.cookiePersistentStorageType))
        return false;
    if (!decoder.decodeEnum(parameters.cookieAcceptPolicy))
        return false;
    if (!decoder.decode(parameters.ignoreTLSErrors))
        return false;
#endif
    if (!decoder.decodeEnum(parameters.cacheModel))
        return false;
    if (!decoder.decode(parameters.shouldTrackVisitedLinks))
        return false;
    if (!decoder.decode(parameters.shouldAlwaysUseComplexTextCodePath))
        return false;
    if (!decoder.decode(parameters.shouldUseFontSmoothing))
        return false;
    if (!decoder.decode(parameters.iconDatabaseEnabled))
        return false;
    if (!decoder.decode(parameters.terminationTimeout))
        return false;
    if (!decoder.decode(parameters.languages))
        return false;
    if (!decoder.decode(parameters.textCheckerState))
        return false;
    if (!decoder.decode(parameters.fullKeyboardAccessEnabled))
        return false;
    if (!decoder.decode(parameters.defaultRequestTimeoutInterval))
        return false;
#if PLATFORM(MAC) || USE(CFNETWORK)
    if (!decoder.decode(parameters.uiProcessBundleIdentifier))
        return false;
#endif

#if PLATFORM(MAC)
    if (!decoder.decode(parameters.presenterApplicationPid))
        return false;
    if (!decoder.decode(parameters.nsURLCacheMemoryCapacity))
        return false;
    if (!decoder.decode(parameters.nsURLCacheDiskCapacity))
        return false;
    if (!decoder.decode(parameters.acceleratedCompositingPort))
        return false;
    if (!decoder.decode(parameters.uiProcessBundleResourcePath))
        return false;
    if (!decoder.decode(parameters.uiProcessBundleResourcePathExtensionHandle))
        return false;
    if (!decoder.decode(parameters.shouldForceScreenFontSubstitution))
        return false;
    if (!decoder.decode(parameters.shouldEnableKerningAndLigaturesByDefault))
        return false;
#endif

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    if (!decoder.decode(parameters.notificationPermissions))
        return false;
#endif

#if ENABLE(NETWORK_PROCESS)
    if (!decoder.decode(parameters.usesNetworkProcess))
        return false;
#endif

    if (!decoder.decode(parameters.plugInAutoStartOriginHashes))
        return false;
    if (!decoder.decode(parameters.plugInAutoStartOrigins))
        return false;

    return true;
}

} // namespace WebKit
