/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#if USE(PLUGIN_HOST_PROCESS)

#ifndef NetscapePluginHostManager_h
#define NetscapePluginHostManager_h

#import <wtf/HashMap.h>
#import <wtf/PassRefPtr.h>
#import <wtf/text/StringHash.h>
#import <wtf/text/WTFString.h>

@class WebHostedNetscapePluginView;

namespace WebKit {

class NetscapePluginInstanceProxy;
class NetscapePluginHostProxy;

class NetscapePluginHostManager {
public:
    static NetscapePluginHostManager& shared();
    
    PassRefPtr<NetscapePluginInstanceProxy> instantiatePlugin(const String& pluginPath, cpu_type_t pluginArchitecture, const String& bundleIdentifier, WebHostedNetscapePluginView *, NSString *mimeType, NSArray *attributeKeys, NSArray *attributeValues, NSString *userAgent, NSURL *sourceURL, bool fullFrame, bool isPrivateBrowsingEnabled, bool isAcceleratedCompositingEnabled);

    void pluginHostDied(NetscapePluginHostProxy*);

    void createPropertyListFile(const String& pluginPath, cpu_type_t pluginArchitecture, const String& bundleIdentifier);
    
    void didCreateWindow();
    
private:
    NetscapePluginHostProxy* hostForPlugin(const String& pluginPath, cpu_type_t pluginArchitecture, const String& bundleIdentifier);

    NetscapePluginHostManager();
    ~NetscapePluginHostManager();
    
    bool spawnPluginHost(const String& pluginPath, cpu_type_t pluginArchitecture, mach_port_t clientPort, mach_port_t& pluginHostPort, ProcessSerialNumber& pluginHostPSN);
    
    bool initializeVendorPort();
    
    mach_port_t m_pluginVendorPort;
    
    // FIXME: This should really be a HashMap of RetainPtrs, but that doesn't work right now.
    typedef HashMap<String, NetscapePluginHostProxy*> PluginHostMap;
    PluginHostMap m_pluginHosts;
};
    
} // namespace WebKit

#endif // NetscapePluginHostManager_h
#endif // USE(PLUGIN_HOST_PROCESS)
