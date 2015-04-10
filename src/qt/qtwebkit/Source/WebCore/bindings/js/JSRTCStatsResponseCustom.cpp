/*
 * Copyright (C) Research In Motion Limited 2013. All rights reserved.
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
#if ENABLE(MEDIA_STREAM)
#include "JSRTCStatsResponse.h"

#include "JSRTCStatsReport.h"
#include "RTCStatsResponse.h"

using namespace JSC;

namespace WebCore {

bool JSRTCStatsResponse::canGetItemsForName(ExecState*, RTCStatsResponse* impl, PropertyName propertyName)
{
    return impl->namedItem(propertyNameToString(propertyName));
}

JSValue JSRTCStatsResponse::nameGetter(ExecState* exec, JSValue slotBase, PropertyName propertyName)
{
    JSRTCStatsResponse* thisObj = jsCast<JSRTCStatsResponse*>(asObject(slotBase));
    return toJS(exec, thisObj->globalObject(), thisObj->impl()->namedItem(propertyNameToString(propertyName)));
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
