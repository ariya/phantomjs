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

#import "WebBaseNetscapePluginView.h"
#import "WebKitSystemInterface.h"

#import <wtf/RefPtr.h>

namespace WebKit {
    class HostedNetscapePluginStream;
    class NetscapePluginInstanceProxy;
}

@interface WebHostedNetscapePluginView : WebBaseNetscapePluginView<WebPluginManualLoader, WebPluginContainerCheckController>
{
    RetainPtr<NSArray> _attributeKeys;
    RetainPtr<NSArray> _attributeValues;
    
    RetainPtr<CALayer> _pluginLayer;
    WKSoftwareCARendererRef _softwareRenderer;
    
    NSSize _previousSize;
    RefPtr<WebKit::NetscapePluginInstanceProxy> _proxy;
}

- (id)initWithFrame:(NSRect)r
      pluginPackage:(WebNetscapePluginPackage *)thePluginPackage
                URL:(NSURL *)URL
            baseURL:(NSURL *)baseURL
           MIMEType:(NSString *)MIME
      attributeKeys:(NSArray *)keys
    attributeValues:(NSArray *)values
       loadManually:(BOOL)loadManually
            element:(PassRefPtr<WebCore::HTMLPlugInElement>)element;

- (void)pluginHostDied;
- (CALayer *)pluginLayer;
- (BOOL)getFormValue:(NSString **)value;
- (void)webFrame:(WebFrame *)webFrame didFinishLoadWithReason:(NPReason)reason;

@end

#endif // USE(PLUGIN_HOST_PROCESS)

