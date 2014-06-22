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

#if ENABLE(INSPECTOR)

#include "InspectorApplicationCacheAgent.h"

#include "ApplicationCacheHost.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "InspectorAgent.h"
#include "InspectorFrontend.h"
#include "InspectorPageAgent.h"
#include "InspectorState.h"
#include "InspectorValues.h"
#include "InstrumentingAgents.h"
#include "NetworkStateNotifier.h"
#include "Page.h"
#include "ResourceResponse.h"

namespace WebCore {

namespace ApplicationCacheAgentState {
static const char applicationCacheAgentEnabled[] = "applicationCacheAgentEnabled";
}

InspectorApplicationCacheAgent::InspectorApplicationCacheAgent(InstrumentingAgents* instrumentingAgents, InspectorCompositeState* state, InspectorPageAgent* pageAgent)
    : InspectorBaseAgent<InspectorApplicationCacheAgent>(ASCIILiteral("ApplicationCache"), instrumentingAgents, state)
    , m_pageAgent(pageAgent)
    , m_frontend(0)
{
}

void InspectorApplicationCacheAgent::setFrontend(InspectorFrontend* frontend)
{
    m_frontend = frontend->applicationcache();
}

void InspectorApplicationCacheAgent::clearFrontend()
{
    m_instrumentingAgents->setInspectorApplicationCacheAgent(0);
    m_frontend = 0;
}

void InspectorApplicationCacheAgent::restore()
{
    if (m_state->getBoolean(ApplicationCacheAgentState::applicationCacheAgentEnabled)) {
        ErrorString error;
        enable(&error);
    }
}

void InspectorApplicationCacheAgent::enable(ErrorString*)
{
    m_state->setBoolean(ApplicationCacheAgentState::applicationCacheAgentEnabled, true);
    m_instrumentingAgents->setInspectorApplicationCacheAgent(this);

    // We need to pass initial navigator.onOnline.
    networkStateChanged();
}

void InspectorApplicationCacheAgent::updateApplicationCacheStatus(Frame* frame)
{
    DocumentLoader* documentLoader = frame->loader()->documentLoader();
    if (!documentLoader)
        return;

    ApplicationCacheHost* host = documentLoader->applicationCacheHost();
    ApplicationCacheHost::Status status = host->status();
    ApplicationCacheHost::CacheInfo info = host->applicationCacheInfo();

    String manifestURL = info.m_manifest.string();
    m_frontend->applicationCacheStatusUpdated(m_pageAgent->frameId(frame), manifestURL, static_cast<int>(status));
}

void InspectorApplicationCacheAgent::networkStateChanged()
{
    bool isNowOnline = networkStateNotifier().onLine();
    m_frontend->networkStateUpdated(isNowOnline);
}

void InspectorApplicationCacheAgent::getFramesWithManifests(ErrorString*, RefPtr<TypeBuilder::Array<TypeBuilder::ApplicationCache::FrameWithManifest> >& result)
{
    result = TypeBuilder::Array<TypeBuilder::ApplicationCache::FrameWithManifest>::create();

    Frame* mainFrame = m_pageAgent->mainFrame();
    for (Frame* frame = mainFrame; frame; frame = frame->tree()->traverseNext(mainFrame)) {
        DocumentLoader* documentLoader = frame->loader()->documentLoader();
        if (!documentLoader)
            continue;

        ApplicationCacheHost* host = documentLoader->applicationCacheHost();
        ApplicationCacheHost::CacheInfo info = host->applicationCacheInfo();
        String manifestURL = info.m_manifest.string();
        if (!manifestURL.isEmpty()) {
            RefPtr<TypeBuilder::ApplicationCache::FrameWithManifest> value = TypeBuilder::ApplicationCache::FrameWithManifest::create()
                .setFrameId(m_pageAgent->frameId(frame))
                .setManifestURL(manifestURL)
                .setStatus(static_cast<int>(host->status()));
            result->addItem(value);
        }
    }
}

DocumentLoader* InspectorApplicationCacheAgent::assertFrameWithDocumentLoader(ErrorString* errorString, String frameId)
{
    Frame* frame = m_pageAgent->assertFrame(errorString, frameId);
    if (!frame)
        return 0;

    return InspectorPageAgent::assertDocumentLoader(errorString, frame);
}

void InspectorApplicationCacheAgent::getManifestForFrame(ErrorString* errorString, const String& frameId, String* manifestURL)
{
    DocumentLoader* documentLoader = assertFrameWithDocumentLoader(errorString, frameId);
    if (!documentLoader)
        return;

    ApplicationCacheHost::CacheInfo info = documentLoader->applicationCacheHost()->applicationCacheInfo();
    *manifestURL = info.m_manifest.string();
}

void InspectorApplicationCacheAgent::getApplicationCacheForFrame(ErrorString* errorString, const String& frameId, RefPtr<TypeBuilder::ApplicationCache::ApplicationCache>& applicationCache)
{
    DocumentLoader* documentLoader = assertFrameWithDocumentLoader(errorString, frameId);
    if (!documentLoader)
        return;

    ApplicationCacheHost* host = documentLoader->applicationCacheHost();
    ApplicationCacheHost::CacheInfo info = host->applicationCacheInfo();

    ApplicationCacheHost::ResourceInfoList resources;
    host->fillResourceList(&resources);

    applicationCache = buildObjectForApplicationCache(resources, info);
}

PassRefPtr<TypeBuilder::ApplicationCache::ApplicationCache> InspectorApplicationCacheAgent::buildObjectForApplicationCache(const ApplicationCacheHost::ResourceInfoList& applicationCacheResources, const ApplicationCacheHost::CacheInfo& applicationCacheInfo)
{
    return TypeBuilder::ApplicationCache::ApplicationCache::create()
        .setManifestURL(applicationCacheInfo.m_manifest.string())
        .setSize(applicationCacheInfo.m_size)
        .setCreationTime(applicationCacheInfo.m_creationTime)
        .setUpdateTime(applicationCacheInfo.m_updateTime)
        .setResources(buildArrayForApplicationCacheResources(applicationCacheResources))
        .release();
}

PassRefPtr<TypeBuilder::Array<TypeBuilder::ApplicationCache::ApplicationCacheResource> > InspectorApplicationCacheAgent::buildArrayForApplicationCacheResources(const ApplicationCacheHost::ResourceInfoList& applicationCacheResources)
{
    RefPtr<TypeBuilder::Array<TypeBuilder::ApplicationCache::ApplicationCacheResource> > resources = TypeBuilder::Array<TypeBuilder::ApplicationCache::ApplicationCacheResource>::create();

    ApplicationCacheHost::ResourceInfoList::const_iterator end = applicationCacheResources.end();
    ApplicationCacheHost::ResourceInfoList::const_iterator it = applicationCacheResources.begin();
    for (int i = 0; it != end; ++it, i++)
        resources->addItem(buildObjectForApplicationCacheResource(*it));

    return resources;
}

PassRefPtr<TypeBuilder::ApplicationCache::ApplicationCacheResource> InspectorApplicationCacheAgent::buildObjectForApplicationCacheResource(const ApplicationCacheHost::ResourceInfo& resourceInfo)
{
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

    RefPtr<TypeBuilder::ApplicationCache::ApplicationCacheResource> value = TypeBuilder::ApplicationCache::ApplicationCacheResource::create()
        .setUrl(resourceInfo.m_resource.string())
        .setSize(static_cast<int>(resourceInfo.m_size))
        .setType(types);
    return value;
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
