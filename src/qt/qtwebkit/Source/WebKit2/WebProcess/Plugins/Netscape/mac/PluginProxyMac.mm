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
#import "PluginProxy.h"

#if ENABLE(PLUGIN_PROCESS) && ENABLE(NETSCAPE_PLUGIN_API)

#import "PluginController.h"
#import "PluginControllerProxyMessages.h"
#import "PluginProcessConnection.h"
#import <QuartzCore/QuartzCore.h>
#import <WebKitSystemInterface.h>

const static double fadeInDuration = 0.5;

namespace WebKit {

static void makeRenderLayer(CALayer *pluginLayer, uint32_t layerHostingContextID)
{
    CALayer *renderLayer = WKMakeRenderLayer(layerHostingContextID);
    [renderLayer setFrame:[pluginLayer bounds]];
    [renderLayer setAutoresizingMask:kCALayerWidthSizable | kCALayerHeightSizable];
    [pluginLayer setSublayers:[NSArray arrayWithObject:renderLayer]];
}

PlatformLayer* PluginProxy::pluginLayer()
{
    if (!m_pluginLayer && m_remoteLayerClientID) {
        // Create a layer with flipped geometry and add the real plug-in layer as a sublayer
        // so the coordinate system will match the event coordinate system.
        m_pluginLayer = adoptNS([[CALayer alloc] init]);
        [m_pluginLayer.get() setGeometryFlipped:YES];

        if (m_isRestartedProcess) {
            CABasicAnimation *fadeInAnimation = [CABasicAnimation animationWithKeyPath:@"opacity"];
            fadeInAnimation.fromValue = [NSNumber numberWithFloat:0];
            fadeInAnimation.toValue = [NSNumber numberWithFloat:1];
            fadeInAnimation.duration = fadeInDuration;
            fadeInAnimation.removedOnCompletion = NO;
            [m_pluginLayer.get() addAnimation:fadeInAnimation forKey:@"restarted-plugin-fade-in"];
        }

        makeRenderLayer(m_pluginLayer.get(), m_remoteLayerClientID);
    }

    return m_pluginLayer.get();
}

bool PluginProxy::needsBackingStore() const
{
    return !m_remoteLayerClientID;
}

void PluginProxy::pluginFocusOrWindowFocusChanged(bool pluginHasFocusAndWindowHasFocus)
{
    controller()->pluginFocusOrWindowFocusChanged(pluginHasFocusAndWindowHasFocus);
}

void PluginProxy::setComplexTextInputState(uint64_t complexTextInputState)
{
    controller()->setComplexTextInputState(static_cast<PluginComplexTextInputState>(complexTextInputState));
}

void PluginProxy::setLayerHostingMode(LayerHostingMode layerHostingMode)
{
    m_connection->connection()->send(Messages::PluginControllerProxy::SetLayerHostingMode(layerHostingMode), m_pluginInstanceID);
}

void PluginProxy::setLayerHostingContextID(uint32_t layerHostingContextID)
{
    ASSERT(m_pluginLayer.get());

    m_remoteLayerClientID = layerHostingContextID;
    makeRenderLayer(m_pluginLayer.get(), m_remoteLayerClientID);
}

} // namespace WebKit

#endif // ENABLE(PLUGIN_PROCESS) && ENABLE(NETSCAPE_PLUGIN_API)
