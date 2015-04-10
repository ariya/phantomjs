/*
 * Copyright (C) 2012 Igalia S.L.
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

#ifndef WKInspectorClientGtk_h
#define WKInspectorClientGtk_h

#include <WebKit2/WKBase.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*WKInspectorClientGtkInspectorCallback)(WKInspectorRef inspector, const void* clientInfo);
typedef void (*WKInspectorClientGtkInspectorDidCloseCallback)(WKInspectorRef inspector, const void* clientInfo);
typedef void (*WKInspectorClientGtkInspectedURLChangedCallback)(WKInspectorRef inspector, WKStringRef url, const void* clientInfo);
typedef void (*WKInspectorClientGtkDidChangeAttachedHeightCallback)(WKInspectorRef inspector, unsigned height, const void* clientInfo);

struct WKInspectorClientGtk {
    int                                                 version;
    const void*                                         clientInfo;
    WKInspectorClientGtkInspectorCallback               openWindow;
    WKInspectorClientGtkInspectorDidCloseCallback       didClose;
    WKInspectorClientGtkInspectorCallback               bringToFront;
    WKInspectorClientGtkInspectedURLChangedCallback     inspectedURLChanged;
    WKInspectorClientGtkInspectorCallback               attach;
    WKInspectorClientGtkInspectorCallback               detach;
    WKInspectorClientGtkDidChangeAttachedHeightCallback didChangeAttachedHeight;
};
typedef struct WKInspectorClientGtk WKInspectorClientGtk;

enum { kWKInspectorClientGtkCurrentVersion = 0 };

WK_EXPORT void WKInspectorSetInspectorClientGtk(WKInspectorRef inspectorRef, const WKInspectorClientGtk* client);

#ifdef __cplusplus
}
#endif

#endif /* WKInspectorClientGtk_h */
