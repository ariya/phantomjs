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

#ifndef WKVibration_h
#define WKVibration_h

#include <WebKit2/WKBase.h>

#ifdef __cplusplus
extern "C" {
#endif

// Provider.
typedef void (*WKVibrationProviderVibrateCallback)(WKVibrationRef vibrationRef, uint32_t vibrationTime, const void* clientInfo);
typedef void (*WKVibrationProviderCancelVibrationCallback)(WKVibrationRef vibrationRef, const void* clientInfo);

struct WKVibrationProvider {
    int                                                                 version;
    const void *                                                        clientInfo;
    WKVibrationProviderVibrateCallback                                  vibrate;
    WKVibrationProviderCancelVibrationCallback                          cancelVibration;
};
typedef struct WKVibrationProvider WKVibrationProvider;

enum { kWKVibrationProviderCurrentVersion = 0 };

WK_EXPORT WKTypeID WKVibrationGetTypeID();

WK_EXPORT void WKVibrationSetProvider(WKVibrationRef vibrationRef, const WKVibrationProvider* provider);

#ifdef __cplusplus
}
#endif

#endif // WKVibration_h
