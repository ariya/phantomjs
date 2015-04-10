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

#ifndef BatteryProvider_h
#define BatteryProvider_h

#if ENABLE(BATTERY_STATUS)

#include "BatteryProviderEfl.h"
#include "BatteryProviderEflClient.h"
#include "BatteryStatus.h"
#include <WebKit2/WKBase.h>
#include <WebKit2/WKRetainPtr.h>
#include <wtf/PassRefPtr.h>

namespace WebKit {

class BatteryProvider : public RefCounted<BatteryProvider>, public WebCore::BatteryProviderEflClient {
public:
    virtual ~BatteryProvider();
    static PassRefPtr<BatteryProvider> create(WKContextRef);

    void startUpdating();
    void stopUpdating();

private:
    explicit BatteryProvider(WKContextRef);

    // BatteryProviderEflClient interface.
    virtual void didChangeBatteryStatus(const AtomicString& eventType, PassRefPtr<WebCore::BatteryStatus>);

    WKRetainPtr<WKBatteryManagerRef> m_batteryManager;
    WebCore::BatteryProviderEfl m_provider;
};

} // namespace WebKit

#endif // ENABLE(BATTERY_STATUS)

#endif // BatteryProvider_h
