/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(DEVICE_ORIENTATION)

#include "JSDeviceOrientationEvent.h"

#include "DeviceOrientationData.h"
#include "DeviceOrientationEvent.h"

using namespace JSC;

namespace WebCore {

JSValue JSDeviceOrientationEvent::alpha(ExecState*) const
{
    DeviceOrientationEvent* imp = static_cast<DeviceOrientationEvent*>(impl());
    if (!imp->orientation()->canProvideAlpha())
        return jsNull();
    return jsNumber(imp->orientation()->alpha());
}

JSValue JSDeviceOrientationEvent::beta(ExecState*) const
{
    DeviceOrientationEvent* imp = static_cast<DeviceOrientationEvent*>(impl());
    if (!imp->orientation()->canProvideBeta())
        return jsNull();
    return jsNumber(imp->orientation()->beta());
}

JSValue JSDeviceOrientationEvent::gamma(ExecState*) const
{
    DeviceOrientationEvent* imp = static_cast<DeviceOrientationEvent*>(impl());
    if (!imp->orientation()->canProvideGamma())
        return jsNull();
    return jsNumber(imp->orientation()->gamma());
}

JSValue JSDeviceOrientationEvent::absolute(ExecState*) const
{
    DeviceOrientationEvent* imp = static_cast<DeviceOrientationEvent*>(impl());
    if (!imp->orientation()->canProvideAbsolute())
        return jsNull();
    return jsBoolean(imp->orientation()->absolute());
}

JSValue JSDeviceOrientationEvent::initDeviceOrientationEvent(ExecState* exec)
{
    const String type = exec->argument(0).toString(exec)->value(exec);
    bool bubbles = exec->argument(1).toBoolean(exec);
    bool cancelable = exec->argument(2).toBoolean(exec);
    // If alpha, beta or gamma are null or undefined, mark them as not provided.
    // Otherwise, use the standard JavaScript conversion.
    bool alphaProvided = !exec->argument(3).isUndefinedOrNull();
    double alpha = exec->argument(3).toNumber(exec);
    bool betaProvided = !exec->argument(4).isUndefinedOrNull();
    double beta = exec->argument(4).toNumber(exec);
    bool gammaProvided = !exec->argument(5).isUndefinedOrNull();
    double gamma = exec->argument(5).toNumber(exec);
    bool absoluteProvided = !exec->argument(6).isUndefinedOrNull();
    bool absolute = exec->argument(6).toBoolean(exec);
    RefPtr<DeviceOrientationData> orientation = DeviceOrientationData::create(alphaProvided, alpha, betaProvided, beta, gammaProvided, gamma, absoluteProvided, absolute);
    DeviceOrientationEvent* imp = static_cast<DeviceOrientationEvent*>(impl());
    imp->initDeviceOrientationEvent(type, bubbles, cancelable, orientation.get());
    return jsUndefined();
}

} // namespace WebCore

#endif // ENABLE(DEVICE_ORIENTATION)
