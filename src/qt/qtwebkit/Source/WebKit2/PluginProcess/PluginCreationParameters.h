/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef PluginCreationParameters_h
#define PluginCreationParameters_h

#if ENABLE(PLUGIN_PROCESS)

#include "Plugin.h"

namespace CoreIPC {
    class ArgumentDecoder;
    class ArgumentEncoder;
}

namespace WebKit {

struct PluginCreationParameters {
    PluginCreationParameters();

    void encode(CoreIPC::ArgumentEncoder&) const;
    static bool decode(CoreIPC::ArgumentDecoder&, PluginCreationParameters&);

    // The unique ID of this plug-in instance.
    uint64_t pluginInstanceID;

    // The ID of the window NPObject.
    uint64_t windowNPObjectID;

    // The parameters passed to the plug-in.
    Plugin::Parameters parameters;

    // The browser user agent.
    String userAgent;

    // The current contents scale factor that this plug-in should have.
    float contentsScaleFactor;

    // Whether private browsing is enabled at the time of instantiation.
    bool isPrivateBrowsingEnabled;
    
    // If requesting synchronous initialization, whether this plugin had previously been requested asynchronously
    bool asynchronousCreationIncomplete;

    // Simulated initialization delay test asynchronous plugin initialization
    bool artificialPluginInitializationDelayEnabled;

#if USE(ACCELERATED_COMPOSITING)
    // Whether accelerated compositing is enabled.
    bool isAcceleratedCompositingEnabled;
#endif
};

} // namespace WebKit

#endif // ENABLE(PLUGIN_PROCESS)

#endif // PluginCreationParameters_h
