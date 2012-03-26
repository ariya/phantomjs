/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "InspectorApplicationCacheAgent.h"

#if ENABLE(INSPECTOR) && ENABLE(OFFLINE_WEB_APPLICATIONS)

#include "ApplicationCacheHost.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "InspectorAgent.h"
#include "InspectorFrontend.h"
#include "InspectorValues.h"
#include "InstrumentingAgents.h"
#include "NetworkStateNotifier.h"
#include "Page.h"
#include "ResourceResponse.h"

namespace WebCore {

InspectorApplicationCacheAgent::InspectorApplicationCacheAgent(InstrumentingAgents* instrumentingAgents, Page* inspectedPage)
    : m_instrumentingAgents(instrumentingAgents)
    , m_inspectedPage(inspectedPage)
    , m_frontend(0)
{
}

void InspectorApplicationCacheAgent::setFrontend(InspectorFrontend* frontend)
{
    m_frontend = frontend->applicationcache();
    m_instrumentingAgents->setInspectorApplicationCacheAgent(this);
}

void InspectorApplicationCacheAgent::clearFrontend()
{
    m_instrumentingAgents->setInspectorApplicationCacheAgent(0);
    m_frontend = 0;
}

void InspectorApplicationCacheAgent::updateApplicationCacheStatus(Frame* frame)
{
    ApplicationCacheHost::Status status = frame->loader()->documentLoader()->applicationCacheHost()->status();
    m_frontend->updateApplicationCacheStatus(status);
}

void InspectorApplicationCacheAgent::networkStateChanged()
{
    bool isNowOnline = networkStateNotifier().onLine();
    m_frontend->updateNetworkState(isNowOnline);
}

void InspectorApplicationCacheAgent::getApplicationCaches(ErrorString*, RefPtr<InspectorObject>* applicationCaches)
{
    DocumentLoader* documentLoader = m_inspectedPage->mainFrame()->loader()->documentLoader();
    if (!documentLoader)
        return;
    ApplicationCacheHost* host = documentLoader->applicationCacheHost();
    ApplicationCacheHost::CacheInfo info = host->applicationCacheInfo();

    ApplicationCacheHost::ResourceInfoList resources;
    host->fillResourceList(&resources);
    *applicationCaches = buildObjectForApplicationCache(resources, info);
}

PassRefPtr<InspectorObject> InspectorApplicationCacheAgent::buildObjectForApplicationCache(const ApplicationCacheHost::ResourceInfoList& applicationCacheResources, const ApplicationCacheHost::CacheInfo& applicationCacheInfo)
{
    RefPtr<InspectorObject> value = InspectorObject::create();
    value->setNumber("size", applicationCacheInfo.m_size);
    value->setString("manifest", applicationCacheInfo.m_manifest.string());
    value->setString("lastPathComponent", applicationCacheInfo.m_manifest.lastPathComponent());
    value->setNumber("creationTime", applicationCacheInfo.m_creationTime);
    value->setNumber("updateTime", applicationCacheInfo.m_updateTime);
    value->setArray("resources", buildArrayForApplicationCacheResources(applicationCacheResources));
    return value;
}

PassRefPtr<InspectorArray> InspectorApplicationCacheAgent::buildArrayForApplicationCacheResources(const ApplicationCacheHost::ResourceInfoList& applicationCacheResources)
{
    RefPtr<InspectorArray> resources = InspectorArray::create();

    ApplicationCacheHost::ResourceInfoList::const_iterator end = applicationCacheResources.end();
    ApplicationCacheHost::ResourceInfoList::const_iterator it = applicationCacheResources.begin();
    for (int i = 0; it != end; ++it, i++)
        resources->pushObject(buildObjectForApplicationCacheResource(*it));

    return resources;
}

PassRefPtr<InspectorObject> InspectorApplicationCacheAgent::buildObjectForApplicationCacheResource(const ApplicationCacheHost::ResourceInfo& resourceInfo)
{
    RefPtr<InspectorObject> value = InspectorObject::create();
    value->setString("name", resourceInfo.m_resource.string());
    value->setNumber("size", resourceInfo.m_size);

    String types;
    if (resourceInfo.m_isMaster)
        types.append("Master ");

    if (resourceInfo.m_isManifest)
        types.append("Manifest ");

    if (resourceInfo.m_isFallback)
        types.append("Fallback ");

    if (resourceInfo.m_isForeign)
        types.append("Foreign ");

    if (resourceInfo.m_isExplicit)
        types.append("Explicit ");

    value->setString("type", types);
    return value;
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR) && ENABLE(OFFLINE_WEB_APPLICATIONS)
