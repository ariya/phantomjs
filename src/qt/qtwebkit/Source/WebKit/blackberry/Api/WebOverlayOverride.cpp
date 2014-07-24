/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#include "WebOverlayOverride.h"

#if USE(ACCELERATED_COMPOSITING)
#include "WebAnimation.h"
#include "WebAnimation_p.h"
#include "WebOverlay_p.h"

#include <BlackBerryPlatformMessageClient.h>
#include <BlackBerryPlatformString.h>
#include <wtf/CurrentTime.h>

namespace BlackBerry {
namespace WebKit {

using namespace WebCore;

WebOverlayOverride::WebOverlayOverride(WebOverlayPrivate* d)
    : d(d)
{
}

WebOverlayOverride::~WebOverlayOverride()
{
}

void WebOverlayOverride::setPosition(const Platform::FloatPoint& position)
{
    d->layerCompositingThread()->override()->setPosition(position);
    d->scheduleCompositingRun();
}

void WebOverlayOverride::setAnchorPoint(const Platform::FloatPoint& anchor)
{
    d->layerCompositingThread()->override()->setAnchorPoint(anchor);
    d->scheduleCompositingRun();
}

void WebOverlayOverride::setSize(const Platform::FloatSize& size)
{
    d->layerCompositingThread()->override()->setBounds(IntSize(size.width(), size.height()));
    d->scheduleCompositingRun();
}

void WebOverlayOverride::setTransform(const Platform::TransformationMatrix& transform)
{
    d->layerCompositingThread()->override()->setTransform(reinterpret_cast<const TransformationMatrix&>(transform));
    d->scheduleCompositingRun();
}

void WebOverlayOverride::setOpacity(float opacity)
{
    d->layerCompositingThread()->override()->setOpacity(opacity);
    d->scheduleCompositingRun();
}

void WebOverlayOverride::addAnimation(const WebAnimation& animation)
{
    LayerCompositingThread* layerCompositingThread = d->layerCompositingThread();
    LayerOverride* override = layerCompositingThread->override();

    IntSize boxSize = override->isBoundsSet() ? override->bounds() : layerCompositingThread->bounds();
    RefPtr<LayerAnimation> layerAnimation = LayerAnimation::create(animation.d->keyframes, boxSize, animation.d->animation.get(), animation.d->name, 0.0);
    layerAnimation->setStartTime(currentTime());

    override->addAnimation(layerAnimation);
    d->scheduleCompositingRun();
}

void WebOverlayOverride::removeAnimation(const BlackBerry::Platform::String& name)
{
    d->layerCompositingThread()->override()->removeAnimation(name);
    d->scheduleCompositingRun();
}

}
}
#else // USE(ACCELERATED_COMPOSITING)
namespace BlackBerry {
namespace WebKit {

WebOverlayOverride::WebOverlayOverride(WebOverlayPrivate*, bool)
{
}

WebOverlayOverride::~WebOverlayOverride()
{
}

void WebOverlayOverride::setPosition(const Platform::FloatPoint&)
{
}

void WebOverlayOverride::setAnchorPoint(const Platform::FloatPoint&)
{
}

void WebOverlayOverride::setSize(const Platform::FloatSize&)
{
}

void WebOverlayOverride::setTransform(const Platform::TransformationMatrix&)
{
}

void WebOverlayOverride::setOpacity(float)
{
}

void WebOverlayOverride::addAnimation(const WebAnimation&)
{
}

void WebOverlayOverride::removeAnimation(const BlackBerry::Platform::String&)
{
}

}
}
#endif // USE(ACCELERATED_COMPOSITING)
