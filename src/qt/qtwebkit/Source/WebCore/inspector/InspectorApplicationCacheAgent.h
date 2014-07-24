/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef InspectorApplicationCacheAgent_h
#define InspectorApplicationCacheAgent_h

#if ENABLE(INSPECTOR)

#include "ApplicationCacheHost.h"
#include "InspectorBaseAgent.h"
#include "InspectorFrontend.h"
#include <wtf/Noncopyable.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class Frame;
class InspectorArray;
class InspectorAgent;
class InspectorFrontend;
class InspectorObject;
class InspectorPageAgent;
class InspectorValue;
class InspectorState;
class InstrumentingAgents;
class Page;
class ResourceResponse;

typedef String ErrorString;

class InspectorApplicationCacheAgent : public InspectorBaseAgent<InspectorApplicationCacheAgent>, public InspectorBackendDispatcher::ApplicationCacheCommandHandler {
    WTF_MAKE_NONCOPYABLE(InspectorApplicationCacheAgent); WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<InspectorApplicationCacheAgent> create(InstrumentingAgents* instrumentingAgents, InspectorCompositeState* state, InspectorPageAgent* pageAgent)
    {
        return adoptPtr(new InspectorApplicationCacheAgent(instrumentingAgents, state, pageAgent));
    }
    ~InspectorApplicationCacheAgent() { }

    // InspectorBaseAgent
    virtual void setFrontend(InspectorFrontend*);
    virtual void clearFrontend();
    virtual void restore();

    // InspectorInstrumentation API
    void updateApplicationCacheStatus(Frame*);
    void networkStateChanged();

    // ApplicationCache API for InspectorFrontend
    virtual void enable(ErrorString*);
    virtual void getFramesWithManifests(ErrorString*, RefPtr<TypeBuilder::Array<TypeBuilder::ApplicationCache::FrameWithManifest> >& result);
    virtual void getManifestForFrame(ErrorString*, const String& frameId, String* manifestURL);
    virtual void getApplicationCacheForFrame(ErrorString*, const String& frameId, RefPtr<TypeBuilder::ApplicationCache::ApplicationCache>&);

private:
    InspectorApplicationCacheAgent(InstrumentingAgents*, InspectorCompositeState*, InspectorPageAgent*);
    PassRefPtr<TypeBuilder::ApplicationCache::ApplicationCache> buildObjectForApplicationCache(const ApplicationCacheHost::ResourceInfoList&, const ApplicationCacheHost::CacheInfo&);
    PassRefPtr<TypeBuilder::Array<TypeBuilder::ApplicationCache::ApplicationCacheResource> > buildArrayForApplicationCacheResources(const ApplicationCacheHost::ResourceInfoList&);
    PassRefPtr<TypeBuilder::ApplicationCache::ApplicationCacheResource> buildObjectForApplicationCacheResource(const ApplicationCacheHost::ResourceInfo&);

    DocumentLoader* assertFrameWithDocumentLoader(ErrorString*, String frameId);

    InspectorPageAgent* m_pageAgent;
    InspectorFrontend::ApplicationCache* m_frontend;
};

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
#endif // InspectorApplicationCacheAgent_h
