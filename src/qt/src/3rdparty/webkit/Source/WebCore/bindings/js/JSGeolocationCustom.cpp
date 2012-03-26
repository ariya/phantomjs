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

#include "config.h"
#include "JSGeolocation.h"

#if ENABLE(GEOLOCATION)

#include "CallbackFunction.h"
#include "DOMWindow.h"
#include "Geolocation.h"
#include "JSCustomPositionCallback.h"
#include "JSCustomPositionErrorCallback.h"
#include "JSDOMWindow.h"
#include "PositionOptions.h"

#if !ENABLE(CLIENT_BASED_GEOLOCATION)
#include "GeolocationService.h"
#endif

using namespace JSC;
using namespace std;

namespace WebCore {

static PassRefPtr<PositionOptions> createPositionOptions(ExecState* exec, JSValue value)
{
    // Create default options.
    RefPtr<PositionOptions> options = PositionOptions::create();

    // Argument is optional (hence undefined is allowed), and null is allowed.
    if (value.isUndefinedOrNull()) {
        // Use default options.
        return options.release();
    }

    // Given the above test, this will always yield an object.
    JSObject* object = value.toObject(exec);

    // For all three properties, we apply the following ...
    // - If the getter or the property's valueOf method throws an exception, we
    //   quit so as not to risk overwriting the exception.
    // - If the value is absent or undefined, we don't override the default.
    JSValue enableHighAccuracyValue = object->get(exec, Identifier(exec, "enableHighAccuracy"));
    if (exec->hadException())
        return 0;
    if (!enableHighAccuracyValue.isUndefined()) {
        options->setEnableHighAccuracy(enableHighAccuracyValue.toBoolean(exec));
        if (exec->hadException())
            return 0;
    }

    JSValue timeoutValue = object->get(exec, Identifier(exec, "timeout"));
    if (exec->hadException())
        return 0;
    if (!timeoutValue.isUndefined()) {
        double timeoutNumber = timeoutValue.toNumber(exec);
        if (exec->hadException())
            return 0;
        // If the value is positive infinity, there's nothing to do.
        if (!(isinf(timeoutNumber) && (timeoutNumber > 0))) {
            // Wrap to int32 and force non-negative to match behavior of window.setTimeout.
            options->setTimeout(max(0, timeoutValue.toInt32(exec)));
            if (exec->hadException())
                return 0;
        }
    }

    JSValue maximumAgeValue = object->get(exec, Identifier(exec, "maximumAge"));
    if (exec->hadException())
        return 0;
    if (!maximumAgeValue.isUndefined()) {
        double maximumAgeNumber = maximumAgeValue.toNumber(exec);
        if (exec->hadException())
            return 0;
        if (isinf(maximumAgeNumber) && (maximumAgeNumber > 0)) {
            // If the value is positive infinity, clear maximumAge.
            options->clearMaximumAge();
        } else {
            // Wrap to int32 and force non-negative to match behavior of window.setTimeout.
            options->setMaximumAge(max(0, maximumAgeValue.toInt32(exec)));
            if (exec->hadException())
                return 0;
        }
    }

    return options.release();
}

JSValue JSGeolocation::getCurrentPosition(ExecState* exec)
{
    // Arguments: PositionCallback, (optional)PositionErrorCallback, (optional)PositionOptions

    RefPtr<PositionCallback> positionCallback = createFunctionOnlyCallback<JSCustomPositionCallback>(exec, static_cast<JSDOMGlobalObject*>(exec->lexicalGlobalObject()), exec->argument(0));
    if (exec->hadException())
        return jsUndefined();
    ASSERT(positionCallback);

    RefPtr<PositionErrorCallback> positionErrorCallback = createFunctionOnlyCallback<JSCustomPositionErrorCallback>(exec, static_cast<JSDOMGlobalObject*>(exec->lexicalGlobalObject()), exec->argument(1), CallbackAllowUndefined | CallbackAllowNull);
    if (exec->hadException())
        return jsUndefined();

    RefPtr<PositionOptions> positionOptions = createPositionOptions(exec, exec->argument(2));
    if (exec->hadException())
        return jsUndefined();
    ASSERT(positionOptions);

    m_impl->getCurrentPosition(positionCallback.release(), positionErrorCallback.release(), positionOptions.release());
    return jsUndefined();
}

JSValue JSGeolocation::watchPosition(ExecState* exec)
{
    // Arguments: PositionCallback, (optional)PositionErrorCallback, (optional)PositionOptions

    RefPtr<PositionCallback> positionCallback = createFunctionOnlyCallback<JSCustomPositionCallback>(exec, static_cast<JSDOMGlobalObject*>(exec->lexicalGlobalObject()), exec->argument(0));
    if (exec->hadException())
        return jsUndefined();
    ASSERT(positionCallback);

    RefPtr<PositionErrorCallback> positionErrorCallback = createFunctionOnlyCallback<JSCustomPositionErrorCallback>(exec, static_cast<JSDOMGlobalObject*>(exec->lexicalGlobalObject()), exec->argument(1), CallbackAllowUndefined | CallbackAllowNull);
    if (exec->hadException())
        return jsUndefined();

    RefPtr<PositionOptions> positionOptions = createPositionOptions(exec, exec->argument(2));
    if (exec->hadException())
        return jsUndefined();
    ASSERT(positionOptions);

    int watchID = m_impl->watchPosition(positionCallback.release(), positionErrorCallback.release(), positionOptions.release());
    return jsNumber(watchID);
}

} // namespace WebCore

#endif // ENABLE(GEOLOCATION)
