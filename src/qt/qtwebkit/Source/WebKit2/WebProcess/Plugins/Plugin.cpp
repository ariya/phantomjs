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
#include "Plugin.h"

#include "WebCoreArgumentCoders.h"
#include <WebCore/IntPoint.h>

using namespace WebCore;

namespace WebKit {

void Plugin::Parameters::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << url.string();
    encoder << names;
    encoder << values;
    encoder << mimeType;
    encoder << isFullFramePlugin;
    encoder << shouldUseManualLoader;
#if PLATFORM(MAC)
    encoder.encodeEnum(layerHostingMode);
#endif
}

bool Plugin::Parameters::decode(CoreIPC::ArgumentDecoder& decoder, Parameters& parameters)
{
    String urlString;
    if (!decoder.decode(urlString))
        return false;
    // FIXME: We can't assume that the url passed in here is valid.
    parameters.url = KURL(ParsedURLString, urlString);

    if (!decoder.decode(parameters.names))
        return false;
    if (!decoder.decode(parameters.values))
        return false;
    if (!decoder.decode(parameters.mimeType))
        return false;
    if (!decoder.decode(parameters.isFullFramePlugin))
        return false;
    if (!decoder.decode(parameters.shouldUseManualLoader))
        return false;
#if PLATFORM(MAC)
    if (!decoder.decodeEnum(parameters.layerHostingMode))
        return false;
#endif
    if (parameters.names.size() != parameters.values.size()) {
        decoder.markInvalid();
        return false;
    }

    return true;
}

Plugin::Plugin()
    : m_pluginController(0)
{
}

Plugin::~Plugin()
{
}

bool Plugin::initialize(PluginController* pluginController, const Parameters& parameters)
{
    ASSERT(!m_pluginController);
    ASSERT(pluginController);

    m_pluginController = pluginController;

    return initialize(parameters);
}

void Plugin::destroyPlugin()
{
    destroy();

    m_pluginController = 0;
}

void Plugin::updateControlTints(GraphicsContext*)
{
}

IntPoint Plugin::convertToRootView(const IntPoint&) const
{
    ASSERT_NOT_REACHED();
    return IntPoint();
}

} // namespace WebKit
