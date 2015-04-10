/*
 * Copyright (C) 2012, 2013 Research In Motion Limited. All rights reserved.
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

#include "WebAnimation.h"

#if USE(ACCELERATED_COMPOSITING)
#include "GraphicsLayer.h"
#include "LayerCompositingThread.h"
#include "LayerWebKitThread.h"
#include "ScaleTransformOperation.h"
#include "WebAnimation_p.h"

#include <BlackBerryPlatformMessageClient.h>
#include <BlackBerryPlatformString.h>

namespace BlackBerry {
namespace WebKit {

using namespace WebCore;

WebAnimation WebAnimation::fadeAnimation(const BlackBerry::Platform::String& name, float from, float to, double duration)
{
    WebAnimation tmp;
    tmp.d->name = name;
    tmp.d->animation = Animation::create();
    tmp.d->animation->setDuration(duration);
    tmp.d->keyframes = KeyframeValueList(AnimatedPropertyOpacity);
    tmp.d->keyframes.insert(new FloatAnimationValue(0, from));
    tmp.d->keyframes.insert(new FloatAnimationValue(1.0, to));

    return tmp;
}

WebAnimation WebAnimation::shrinkAnimation(const BlackBerry::Platform::String& name, float from, float to, double duration)
{
    WebAnimation tmp;
    tmp.d->name = name;
    tmp.d->animation = Animation::create();
    tmp.d->animation->setDuration(duration);
    tmp.d->keyframes = KeyframeValueList(AnimatedPropertyWebkitTransform);

    TransformOperations startOperation;
    startOperation.operations().append(ScaleTransformOperation::create(from, from, TransformOperation::SCALE));
    tmp.d->keyframes.insert(new TransformAnimationValue(0, &startOperation));

    TransformOperations endOperation;
    endOperation.operations().append(ScaleTransformOperation::create(to, to, TransformOperation::SCALE));
    tmp.d->keyframes.insert(new TransformAnimationValue(1.0, &endOperation));

    return tmp;
}

BlackBerry::Platform::String WebAnimation::name() const
{
    return d->name;
}

WebAnimation::WebAnimation()
    : d(new WebAnimationPrivate)
{
}

WebAnimation::WebAnimation(const WebAnimation& o)
: d(new WebAnimationPrivate)
{
    *d = *o.d;
}

WebAnimation::~WebAnimation()
{
    delete d;
}

WebAnimation& WebAnimation::operator=(const WebAnimation& o)
{
    if (&o != this)
        *d = *o.d;
    return *this;
}

} // namespace WebKit
} // namespace BlackBerry

#endif // USE(ACCELERATED_COMPOSITING)
