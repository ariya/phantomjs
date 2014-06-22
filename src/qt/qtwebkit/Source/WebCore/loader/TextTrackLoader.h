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

#ifndef TextTrackLoader_h
#define TextTrackLoader_h

#if ENABLE(VIDEO_TRACK)

#include "CachedResourceClient.h"
#include "CachedResourceHandle.h"
#include "Timer.h"
#include "WebVTTParser.h"
#include <wtf/OwnPtr.h>

namespace WebCore {

class CachedTextTrack;
class Document;
class TextTrackLoader;
class ScriptExecutionContext;

class TextTrackLoaderClient {
public:
    virtual ~TextTrackLoaderClient() { }
    
    virtual bool shouldLoadCues(TextTrackLoader*) = 0;
    virtual void newCuesAvailable(TextTrackLoader*) = 0;
    virtual void cueLoadingStarted(TextTrackLoader*) = 0;
    virtual void cueLoadingCompleted(TextTrackLoader*, bool loadingFailed) = 0;
#if ENABLE(WEBVTT_REGIONS)
    virtual void newRegionsAvailable(TextTrackLoader*) = 0;
#endif
};

class TextTrackLoader : public CachedResourceClient, private WebVTTParserClient {
    WTF_MAKE_NONCOPYABLE(TextTrackLoader); 
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<TextTrackLoader> create(TextTrackLoaderClient* client, ScriptExecutionContext* context)
    {
        return adoptPtr(new TextTrackLoader(client, context));
    }
    virtual ~TextTrackLoader();
    
    bool load(const KURL&, const String& crossOriginMode);
    void cancelLoad();
    void getNewCues(Vector<RefPtr<TextTrackCue> >& outputCues);
#if ENABLE(WEBVTT_REGIONS)
    void getNewRegions(Vector<RefPtr<TextTrackRegion> >& outputRegions);
#endif
private:

    // CachedResourceClient
    virtual void notifyFinished(CachedResource*);
    virtual void deprecatedDidReceiveCachedResource(CachedResource*);
    
    // WebVTTParserClient
    virtual void newCuesParsed();
#if ENABLE(WEBVTT_REGIONS)
    virtual void newRegionsParsed();
#endif
    virtual void fileFailedToParse();
    
    TextTrackLoader(TextTrackLoaderClient*, ScriptExecutionContext*);
    
    void processNewCueData(CachedResource*);
    void cueLoadTimerFired(Timer<TextTrackLoader>*);
    void corsPolicyPreventedLoad();

    enum State { Idle, Loading, Finished, Failed };
    
    TextTrackLoaderClient* m_client;
    OwnPtr<WebVTTParser> m_cueParser;
    CachedResourceHandle<CachedTextTrack> m_cachedCueData;
    ScriptExecutionContext* m_scriptExecutionContext;
    Timer<TextTrackLoader> m_cueLoadTimer;
    String m_crossOriginMode;
    State m_state;
    unsigned m_parseOffset;
    bool m_newCuesAvailable;
};

} // namespace WebCore

#endif
#endif
