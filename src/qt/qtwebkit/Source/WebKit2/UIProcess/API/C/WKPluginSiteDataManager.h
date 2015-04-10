/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef WKPluginSiteDataManager_h
#define WKPluginSiteDataManager_h

#include <WebKit2/WKBase.h>

#ifdef __cplusplus
extern "C" {
#endif

WK_EXPORT WKTypeID WKPluginSiteDataManagerGetTypeID();

typedef void (*WKPluginSiteDataManagerGetSitesWithDataFunction)(WKArrayRef, WKErrorRef, void*);
WK_EXPORT void WKPluginSiteDataManagerGetSitesWithData(WKPluginSiteDataManagerRef manager, void* context, WKPluginSiteDataManagerGetSitesWithDataFunction function);

enum {
    kWKClearSiteDataFlagsClearAll = 0,
    kWKClearSiteDataFlagsClearCache = 1 << 0,
};
typedef uint64_t WKClearSiteDataFlags;

typedef void (*WKPluginSiteDataManagerClearSiteDataFunction)(WKErrorRef, void*);

WK_EXPORT void WKPluginSiteDataManagerClearSiteData(WKPluginSiteDataManagerRef manager, WKArrayRef sites, WKClearSiteDataFlags flags, uint64_t maxAgeInSeconds, void* context, WKPluginSiteDataManagerClearSiteDataFunction function);
WK_EXPORT void WKPluginSiteDataManagerClearAllSiteData(WKPluginSiteDataManagerRef manager, void* context, WKPluginSiteDataManagerClearSiteDataFunction function);

#ifdef __cplusplus
}
#endif

#endif // WKPluginSiteDataManager_h
