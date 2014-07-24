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
#include "VibrationClientEfl.h"

#if ENABLE(VIBRATION)

#include "EwkView.h"
#include "WKAPICast.h"
#include "WKVibration.h"

using namespace WebKit;
using namespace EwkViewCallbacks;

static inline VibrationClientEfl* toVibrationClient(const void* clientInfo)
{
    return static_cast<VibrationClientEfl*>(const_cast<void*>(clientInfo));
}

void VibrationClientEfl::vibrateCallback(WKVibrationRef, uint32_t vibrationTime, const void* clientInfo)
{
    toVibrationClient(clientInfo)->m_view->smartCallback<Vibrate>().call(&vibrationTime);
}

void VibrationClientEfl::cancelVibrationCallback(WKVibrationRef, const void* clientInfo)
{
    toVibrationClient(clientInfo)->m_view->smartCallback<CancelVibration>().call();
}

PassOwnPtr<VibrationClientEfl> VibrationClientEfl::create(EwkView* viewImpl)
{
    return adoptPtr(new VibrationClientEfl(viewImpl));
}

VibrationClientEfl::VibrationClientEfl(EwkView* view)
    : m_view(view)
{
    ASSERT(m_view);

    WKPageRef pageRef = m_view->wkPage();
    ASSERT(pageRef);

    WKVibrationRef wkVibration = WKPageGetVibration(pageRef);
    ASSERT(wkVibration);

    WKVibrationProvider wkVibrationProvider = {
        kWKVibrationProviderCurrentVersion,
        this, // clientInfo
        vibrateCallback,
        cancelVibrationCallback
    };
    WKVibrationSetProvider(wkVibration, &wkVibrationProvider);
}

VibrationClientEfl::~VibrationClientEfl()
{
    WKPageRef pageRef = m_view->wkPage();
    ASSERT(pageRef);

    WKVibrationRef wkVibration = WKPageGetVibration(pageRef);
    ASSERT(wkVibration);

    WKVibrationSetProvider(wkVibration, 0);
}

#endif // ENABLE(VIBRATION)
