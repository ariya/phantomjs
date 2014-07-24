/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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
#include "WKBatteryManager.h"

#include "WKAPICast.h"
#include <wtf/text/AtomicString.h>

#if ENABLE(BATTERY_STATUS)
#include "WebBatteryManagerProxy.h"
#endif

using namespace WebKit;

WKTypeID WKBatteryManagerGetTypeID()
{
#if ENABLE(BATTERY_STATUS)
    return toAPI(WebBatteryManagerProxy::APIType);
#else
    return 0;
#endif
}

void WKBatteryManagerSetProvider(WKBatteryManagerRef batteryManager, const WKBatteryProvider* provider)
{
#if ENABLE(BATTERY_STATUS)
    toImpl(batteryManager)->initializeProvider(provider);
#endif
}

void WKBatteryManagerProviderDidChangeBatteryStatus(WKBatteryManagerRef batteryManager, WKStringRef eventType, WKBatteryStatusRef status)
{
#if ENABLE(BATTERY_STATUS)
    toImpl(batteryManager)->providerDidChangeBatteryStatus(AtomicString(toImpl(eventType)->string()), toImpl(status));
#endif
}

void WKBatteryManagerProviderUpdateBatteryStatus(WKBatteryManagerRef batteryManager, WKBatteryStatusRef status)
{
#if ENABLE(BATTERY_STATUS)
    toImpl(batteryManager)->providerUpdateBatteryStatus(toImpl(status));
#endif
}
