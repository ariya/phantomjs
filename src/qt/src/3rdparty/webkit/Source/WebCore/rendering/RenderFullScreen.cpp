/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(FULLSCREEN_API)

#include "RenderFullScreen.h"

#include "RenderLayer.h"

#if USE(ACCELERATED_COMPOSITING)
#include "RenderLayerCompositor.h"
#endif

using namespace WebCore;

void RenderFullScreen::setAnimating(bool animating)
{
    m_isAnimating = animating;
#if USE(ACCELERATED_COMPOSITING)
    if (layer()) {
        layer()->contentChanged(RenderLayer::FullScreenChanged);
        // Clearing the layer's backing will force the compositor to reparent
        // the layer the next time layers are synchronized.
        layer()->clearBacking();
    }
#endif
}

PassRefPtr<RenderStyle> RenderFullScreen::createFullScreenStyle()
{
    RefPtr<RenderStyle> fullscreenStyle = RenderStyle::createDefaultStyle();

    // Create a stacking context:
    fullscreenStyle->setZIndex(INT_MAX);

    fullscreenStyle->setFontDescription(FontDescription());
    fullscreenStyle->font().update(0);

    fullscreenStyle->setDisplay(BOX);
    fullscreenStyle->setBoxPack(BCENTER);
    fullscreenStyle->setBoxAlign(BCENTER);
    fullscreenStyle->setBoxOrient(VERTICAL);
    
    fullscreenStyle->setPosition(FixedPosition);
    fullscreenStyle->setWidth(Length(100.0, Percent));
    fullscreenStyle->setHeight(Length(100.0, Percent));
    fullscreenStyle->setLeft(Length(0, Fixed));
    fullscreenStyle->setTop(Length(0, Fixed));
    
    fullscreenStyle->setBackgroundColor(Color::black);
    
    return fullscreenStyle.release();
}

#endif
