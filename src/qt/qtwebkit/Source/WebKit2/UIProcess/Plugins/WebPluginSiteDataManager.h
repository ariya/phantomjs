/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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

#ifndef WebPluginSiteDataManagerh
#define WebPluginSiteDataManager_h

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "APIObject.h"
#include "Arguments.h"
#include "GenericCallback.h"
#include <wtf/HashMap.h>
#include <wtf/PassRefPtr.h>

namespace WebKit {

class WebContext;
class WebProcessProxy;

typedef GenericCallback<WKArrayRef> ArrayCallback;

class WebPluginSiteDataManager : public TypedAPIObject<APIObject::TypePluginSiteDataManager> {
public:
    static PassRefPtr<WebPluginSiteDataManager> create(WebContext*);
    virtual ~WebPluginSiteDataManager();

    void invalidate();
    void clearContext() { m_webContext = 0; }

    void getSitesWithData(PassRefPtr<ArrayCallback>);
    void didGetSitesWithData(const Vector<String>& sites, uint64_t callbackID);

    void clearSiteData(ImmutableArray* sites, uint64_t flags, uint64_t maxAgeInSeconds, PassRefPtr<VoidCallback>);
    void didClearSiteData(uint64_t callbackID);

#if ENABLE(PLUGIN_PROCESS)
    void didGetSitesWithDataForSinglePlugin(const Vector<String>& sites, uint64_t callbackID);
    void didClearSiteDataForSinglePlugin(uint64_t callbackID);    
#endif

    bool shouldTerminate(WebProcessProxy*) const;

private:
    explicit WebPluginSiteDataManager(WebContext*);

    WebContext* m_webContext;
    HashMap<uint64_t, RefPtr<ArrayCallback> > m_arrayCallbacks;
    HashMap<uint64_t, RefPtr<VoidCallback> > m_voidCallbacks;

#if ENABLE(PLUGIN_PROCESS)
    void didGetSitesWithDataForAllPlugins(const Vector<String>& sites, uint64_t callbackID);
    void didClearSiteDataForAllPlugins(uint64_t callbackID);

    class GetSitesWithDataState;
    HashMap<uint64_t, OwnPtr<GetSitesWithDataState> > m_pendingGetSitesWithData;

    class ClearSiteDataState;
    HashMap<uint64_t, OwnPtr<ClearSiteDataState> > m_pendingClearSiteData;
#endif
};

} // namespace WebKit

#endif // ENABLE(NETSCAPE_PLUGIN_API)

#endif // WebPluginSiteDataManager_h
