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

#include "config.h"
#include "ProxyServer.h"

#include "KURL.h"
#include "Logging.h"
#include <wtf/RetainPtr.h>
#include <wtf/text/CString.h>

#if PLATFORM(WIN)
#include <CFNetwork/CFNetwork.h>
#endif

namespace WebCore {

static void processProxyServers(Vector<ProxyServer>& proxyServers, CFArrayRef proxies, CFURLRef url);

static void proxyAutoConfigurationResultCallback(void *context, CFArrayRef proxies, CFErrorRef error)
{
    // We only expect a single result callback per invocation. Stop our runloop to unblock our caller.
    CFRunLoopStop(CFRunLoopGetCurrent());

    Vector<ProxyServer>* proxyServers = (Vector<ProxyServer>*)context;
    if (!proxies) {
        ASSERT(error);
        RetainPtr<CFStringRef> errorDescriptionCF = adoptCF(CFErrorCopyDescription(error));
        String errorDescription(errorDescriptionCF.get());
        LOG(Network, "Failed to process proxy auto-configuration file with error: %s", errorDescription.utf8().data());
        return;
    }

    processProxyServers(*proxyServers, proxies, 0);
}

static void processProxyServers(Vector<ProxyServer>& proxyServers, CFArrayRef proxies, CFURLRef url)
{
    CFIndex numProxies = CFArrayGetCount(proxies);
    for (CFIndex i = 0; i < numProxies; ++i) {
        CFDictionaryRef proxyDictionary = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(proxies, i));

        ProxyServer::Type type = ProxyServer::Direct;
        CFStringRef typeString = static_cast<CFStringRef>(CFDictionaryGetValue(proxyDictionary, kCFProxyTypeKey));

        if (!url) {
            // If we have no URL then we're processing an auto-configuration response.
            // It isn't sensible to receive another auto-configured proxy in such a response.
            ASSERT(!CFEqual(typeString, kCFProxyTypeAutoConfigurationURL));
        }

        if (CFEqual(typeString, kCFProxyTypeAutoConfigurationURL)) {
            if (!url)
                continue;

            // FIXME: We should restructure to allow this to happen asynchronously.
            CFURLRef scriptURL = static_cast<CFURLRef>(CFDictionaryGetValue(proxyDictionary, kCFProxyAutoConfigurationURLKey));
            if (!scriptURL || CFGetTypeID(scriptURL) != CFURLGetTypeID())
                continue;

            CFStreamClientContext context = { 0, (void*)&proxyServers, 0, 0, 0 };
            RetainPtr<CFRunLoopSourceRef> runLoopSource = adoptCF(CFNetworkExecuteProxyAutoConfigurationURL(scriptURL, url, proxyAutoConfigurationResultCallback, &context));

            CFStringRef privateRunLoopMode = CFSTR("com.apple.WebKit.ProxyAutoConfiguration");
            CFTimeInterval timeout = 5;
            CFRunLoopAddSource(CFRunLoopGetCurrent(), runLoopSource.get(), privateRunLoopMode);
            CFRunLoopRunInMode(privateRunLoopMode, timeout, 0);
            CFRunLoopRemoveSource(CFRunLoopGetCurrent(), runLoopSource.get(), privateRunLoopMode);
            CFRunLoopSourceInvalidate(runLoopSource.get());

            // The proxyAutoConfigurationResultCallback has added any relevant ProxyServers to proxyServers.
            continue;
        }

        if (CFEqual(typeString, kCFProxyTypeNone)) {
            proxyServers.append(ProxyServer(ProxyServer::Direct, String(), -1));
            continue;
        }

        if (CFEqual(typeString, kCFProxyTypeHTTP))
            type = ProxyServer::HTTP;
        else if (CFEqual(typeString, kCFProxyTypeHTTPS))
            type = ProxyServer::HTTPS;
        else if (CFEqual(typeString, kCFProxyTypeSOCKS))
            type = ProxyServer::SOCKS;
        else {
            // We don't know how to handle this type.
            continue;
        }

        CFStringRef host = static_cast<CFStringRef>(CFDictionaryGetValue(proxyDictionary, kCFProxyHostNameKey));
        CFNumberRef port = static_cast<CFNumberRef>(CFDictionaryGetValue(proxyDictionary, kCFProxyPortNumberKey));
        SInt32 portValue;
        CFNumberGetValue(port, kCFNumberSInt32Type, &portValue);

        proxyServers.append(ProxyServer(type, host, portValue));
    }
}

static void addProxyServersForURL(Vector<ProxyServer>& proxyServers, const KURL& url)
{
    RetainPtr<CFDictionaryRef> proxySettings = adoptCF(CFNetworkCopySystemProxySettings());
    if (!proxySettings)
        return;

    RetainPtr<CFURLRef> cfURL = url.createCFURL();
    RetainPtr<CFArrayRef> proxiesForURL = adoptCF(CFNetworkCopyProxiesForURL(cfURL.get(), proxySettings.get()));
    if (!proxiesForURL)
        return;

    processProxyServers(proxyServers, proxiesForURL.get(), cfURL.get());
}

Vector<ProxyServer> proxyServersForURL(const KURL& url, const NetworkingContext*)
{
    Vector<ProxyServer> proxyServers;
    
    addProxyServersForURL(proxyServers, url);
    return proxyServers;
    
}

} // namespace WebCore
