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
#include "WebBatteryStatus.h"

#if ENABLE(BATTERY_STATUS)

#include "ArgumentCoders.h"
#include "Arguments.h"

namespace WebKit {

WebBatteryStatus::WebBatteryStatus(bool isCharging, double chargingTime, double dischargingTime, double level)
{
    m_data.isCharging = isCharging;
    m_data.chargingTime = chargingTime;
    m_data.dischargingTime = dischargingTime;
    m_data.level = level;
}

WebBatteryStatus::~WebBatteryStatus()
{
}

void WebBatteryStatus::Data::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << isCharging;
    encoder << chargingTime;
    encoder << dischargingTime;
    encoder << level;
}

bool WebBatteryStatus::Data::decode(CoreIPC::ArgumentDecoder& decoder, Data& result)
{
    if (!decoder.decode(result.isCharging))
        return false;
    if (!decoder.decode(result.chargingTime))
        return false;
    if (!decoder.decode(result.dischargingTime))
        return false;
    if (!decoder.decode(result.level))
        return false;

    return true;
}

} // namespace WebKit

#endif // ENABLE(BATTERY_STATUS)
