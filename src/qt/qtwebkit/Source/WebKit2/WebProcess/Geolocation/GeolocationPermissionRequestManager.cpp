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

#include "config.h"
#include "GeolocationPermissionRequestManager.h"

#if ENABLE(GEOLOCATION)

#include "WebCoreArgumentCoders.h"
#include "WebFrame.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include <WebCore/Frame.h>
#include <WebCore/FrameLoader.h>
#include <WebCore/Geolocation.h>
#include <WebCore/SecurityOrigin.h>

using namespace WebCore;

namespace WebKit {

static uint64_t generateGeolocationID()
{
    static uint64_t uniqueGeolocationID = 1;
    return uniqueGeolocationID++;
}

GeolocationPermissionRequestManager::GeolocationPermissionRequestManager(WebPage* page)
    : m_page(page)
{
}

void GeolocationPermissionRequestManager::startRequestForGeolocation(Geolocation* geolocation)
{
    uint64_t geolocationID = generateGeolocationID();

    m_geolocationToIDMap.set(geolocation, geolocationID);
    m_idToGeolocationMap.set(geolocationID, geolocation);


    Frame* frame = geolocation->frame();

    WebFrameLoaderClient* webFrameLoaderClient = toWebFrameLoaderClient(frame->loader()->client());
    WebFrame* webFrame = webFrameLoaderClient ? webFrameLoaderClient->webFrame() : 0;
    ASSERT(webFrame);

    SecurityOrigin* origin = frame->document()->securityOrigin();

    m_page->send(Messages::WebPageProxy::RequestGeolocationPermissionForFrame(geolocationID, webFrame->frameID(), origin->databaseIdentifier()));
}

void GeolocationPermissionRequestManager::cancelRequestForGeolocation(Geolocation* geolocation)
{
    GeolocationToIDMap::iterator it = m_geolocationToIDMap.find(geolocation);
    if (it == m_geolocationToIDMap.end())
        return;

    uint64_t geolocationID = it->value;
    m_geolocationToIDMap.remove(it);
    m_idToGeolocationMap.remove(geolocationID);
}

void GeolocationPermissionRequestManager::didReceiveGeolocationPermissionDecision(uint64_t geolocationID, bool allowed)
{
    IDToGeolocationMap::iterator it = m_idToGeolocationMap.find(geolocationID);
    if (it == m_idToGeolocationMap.end())
        return;

    Geolocation* geolocation = it->value;
    geolocation->setIsAllowed(allowed);

    m_idToGeolocationMap.remove(it);
    m_geolocationToIDMap.remove(geolocation);
}

} // namespace WebKit

#endif // ENABLE(GEOLOCATION)
