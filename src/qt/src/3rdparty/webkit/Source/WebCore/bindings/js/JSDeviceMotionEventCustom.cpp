/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include "JSDeviceMotionEvent.h"

#if ENABLE(DEVICE_ORIENTATION)

#include "DeviceMotionData.h"
#include "DeviceMotionEvent.h"

using namespace JSC;

namespace WebCore {

static PassRefPtr<DeviceMotionData::Acceleration> readAccelerationArgument(JSValue value, ExecState* exec)
{
    if (value.isUndefinedOrNull())
        return 0;

    // Given the above test, this will always yield an object.
    JSObject* object = value.toObject(exec);

    JSValue xValue = object->get(exec, Identifier(exec, "x"));
    if (exec->hadException())
        return 0;
    bool canProvideX = !xValue.isUndefinedOrNull();
    double x = xValue.toNumber(exec);
    if (exec->hadException())
        return 0;

    JSValue yValue = object->get(exec, Identifier(exec, "y"));
    if (exec->hadException())
        return 0;
    bool canProvideY = !yValue.isUndefinedOrNull();
    double y = yValue.toNumber(exec);
    if (exec->hadException())
        return 0;

    JSValue zValue = object->get(exec, Identifier(exec, "z"));
    if (exec->hadException())
        return 0;
    bool canProvideZ = !zValue.isUndefinedOrNull();
    double z = zValue.toNumber(exec);
    if (exec->hadException())
        return 0;

    if (!canProvideX && !canProvideY && !canProvideZ)
        return 0;

    return DeviceMotionData::Acceleration::create(canProvideX, x, canProvideY, y, canProvideZ, z);
}

static PassRefPtr<DeviceMotionData::RotationRate> readRotationRateArgument(JSValue value, ExecState* exec)
{
    if (value.isUndefinedOrNull())
        return 0;

    // Given the above test, this will always yield an object.
    JSObject* object = value.toObject(exec);

    JSValue alphaValue = object->get(exec, Identifier(exec, "alpha"));
    if (exec->hadException())
        return 0;
    bool canProvideAlpha = !alphaValue.isUndefinedOrNull();
    double alpha = alphaValue.toNumber(exec);
    if (exec->hadException())
        return 0;

    JSValue betaValue = object->get(exec, Identifier(exec, "beta"));
    if (exec->hadException())
        return 0;
    bool canProvideBeta = !betaValue.isUndefinedOrNull();
    double beta = betaValue.toNumber(exec);
    if (exec->hadException())
        return 0;

    JSValue gammaValue = object->get(exec, Identifier(exec, "gamma"));
    if (exec->hadException())
        return 0;
    bool canProvideGamma = !gammaValue.isUndefinedOrNull();
    double gamma = gammaValue.toNumber(exec);
    if (exec->hadException())
        return 0;

    if (!canProvideAlpha && !canProvideBeta && !canProvideGamma)
        return 0;

    return DeviceMotionData::RotationRate::create(canProvideAlpha, alpha, canProvideBeta, beta, canProvideGamma, gamma);
}

static JSObject* createAccelerationObject(const DeviceMotionData::Acceleration* acceleration, ExecState* exec)
{
    JSObject* object = constructEmptyObject(exec);
    object->putDirect(exec->globalData(), Identifier(exec, "x"), acceleration->canProvideX() ? jsNumber(acceleration->x()) : jsNull());
    object->putDirect(exec->globalData(), Identifier(exec, "y"), acceleration->canProvideY() ? jsNumber(acceleration->y()) : jsNull());
    object->putDirect(exec->globalData(), Identifier(exec, "z"), acceleration->canProvideZ() ? jsNumber(acceleration->z()) : jsNull());
    return object;
}

static JSObject* createRotationRateObject(const DeviceMotionData::RotationRate* rotationRate, ExecState* exec)
{
    JSObject* object = constructEmptyObject(exec);
    object->putDirect(exec->globalData(), Identifier(exec, "alpha"), rotationRate->canProvideAlpha() ? jsNumber(rotationRate->alpha()) : jsNull());
    object->putDirect(exec->globalData(), Identifier(exec, "beta"),  rotationRate->canProvideBeta()  ? jsNumber(rotationRate->beta())  : jsNull());
    object->putDirect(exec->globalData(), Identifier(exec, "gamma"), rotationRate->canProvideGamma() ? jsNumber(rotationRate->gamma()) : jsNull());
    return object;
}

JSValue JSDeviceMotionEvent::acceleration(ExecState* exec) const
{
    DeviceMotionEvent* imp = static_cast<DeviceMotionEvent*>(impl());
    if (!imp->deviceMotionData()->acceleration())
        return jsNull();
    return createAccelerationObject(imp->deviceMotionData()->acceleration(), exec);
}

JSValue JSDeviceMotionEvent::accelerationIncludingGravity(ExecState* exec) const
{
    DeviceMotionEvent* imp = static_cast<DeviceMotionEvent*>(impl());
    if (!imp->deviceMotionData()->accelerationIncludingGravity())
        return jsNull();
    return createAccelerationObject(imp->deviceMotionData()->accelerationIncludingGravity(), exec);
}

JSValue JSDeviceMotionEvent::rotationRate(ExecState* exec) const
{
    DeviceMotionEvent* imp = static_cast<DeviceMotionEvent*>(impl());
    if (!imp->deviceMotionData()->rotationRate())
        return jsNull();
    return createRotationRateObject(imp->deviceMotionData()->rotationRate(), exec);
}

JSValue JSDeviceMotionEvent::interval(ExecState*) const
{
    DeviceMotionEvent* imp = static_cast<DeviceMotionEvent*>(impl());
    if (!imp->deviceMotionData()->canProvideInterval())
        return jsNull();
    return jsNumber(imp->deviceMotionData()->interval());
}

JSValue JSDeviceMotionEvent::initDeviceMotionEvent(ExecState* exec)
{
    const String& type = ustringToString(exec->argument(0).toString(exec));
    bool bubbles = exec->argument(1).toBoolean(exec);
    bool cancelable = exec->argument(2).toBoolean(exec);

    // If any of the parameters are null or undefined, mark them as not provided.
    // Otherwise, use the standard JavaScript conversion.
    RefPtr<DeviceMotionData::Acceleration> acceleration = readAccelerationArgument(exec->argument(3), exec);
    if (exec->hadException())
        return jsUndefined();

    RefPtr<DeviceMotionData::Acceleration> accelerationIncludingGravity = readAccelerationArgument(exec->argument(4), exec);
    if (exec->hadException())
        return jsUndefined();

    RefPtr<DeviceMotionData::RotationRate> rotationRate = readRotationRateArgument(exec->argument(5), exec);
    if (exec->hadException())
        return jsUndefined();

    bool intervalProvided = !exec->argument(6).isUndefinedOrNull();
    double interval = exec->argument(6).toNumber(exec);
    RefPtr<DeviceMotionData> deviceMotionData = DeviceMotionData::create(acceleration, accelerationIncludingGravity, rotationRate, intervalProvided, interval);
    DeviceMotionEvent* imp = static_cast<DeviceMotionEvent*>(impl());
    imp->initDeviceMotionEvent(type, bubbles, cancelable, deviceMotionData.get());
    return jsUndefined();
}

} // namespace WebCore

#endif // ENABLE(DEVICE_ORIENTATION)
