/*
 * Copyright (C) 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#if ENABLE(VIDEO)
#include "HTMLMediaElement.h"

#include "Attribute.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "ClientRect.h"
#include "ClientRectList.h"
#include "ContentSecurityPolicy.h"
#include "ContentType.h"
#include "CSSPropertyNames.h"
#include "CSSValueKeywords.h"
#include "Event.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameView.h"
#include "HTMLDocument.h"
#include "HTMLNames.h"
#include "HTMLSourceElement.h"
#include "HTMLVideoElement.h"
#include "Logging.h"
#include "MediaControls.h"
#include "MediaDocument.h"
#include "MediaError.h"
#include "MediaList.h"
#include "MediaPlayer.h"
#include "MediaQueryEvaluator.h"
#include "MouseEvent.h"
#include "MIMETypeRegistry.h"
#include "Page.h"
#include "RenderVideo.h"
#include "RenderView.h"
#include "ScriptEventListener.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include "ShadowRoot.h"
#include "TimeRanges.h"
#include <limits>
#include <wtf/CurrentTime.h>
#include <wtf/MathExtras.h>
#include <wtf/text/CString.h>

#if USE(ACCELERATED_COMPOSITING)
#include "RenderView.h"
#include "RenderLayerCompositor.h"
#endif

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
#include "RenderEmbeddedObject.h"
#include "Widget.h"
#endif

using namespace std;

namespace WebCore {

#if !LOG_DISABLED
static String urlForLogging(const String& url)
{
    static const unsigned maximumURLLengthForLogging = 128;

    if (url.length() < maximumURLLengthForLogging)
        return url;
    return url.substring(0, maximumURLLengthForLogging) + "...";
}

static const char *boolString(bool val)
{
    return val ? "true" : "false";
}
#endif

#ifndef LOG_MEDIA_EVENTS
// Default to not logging events because so many are generated they can overwhelm the rest of 
// the logging.
#define LOG_MEDIA_EVENTS 0
#endif

#ifndef LOG_CACHED_TIME_WARNINGS
// Default to not logging warnings about excessive drift in the cached media time because it adds a
// fair amount of overhead and logging.
#define LOG_CACHED_TIME_WARNINGS 0
#endif

static const float invalidMediaTime = -1;

using namespace HTMLNames;

HTMLMediaElement::HTMLMediaElement(const QualifiedName& tagName, Document* document)
    : HTMLElement(tagName, document)
    , ActiveDOMObject(document, this)
    , m_loadTimer(this, &HTMLMediaElement::loadTimerFired)
    , m_asyncEventTimer(this, &HTMLMediaElement::asyncEventTimerFired)
    , m_progressEventTimer(this, &HTMLMediaElement::progressEventTimerFired)
    , m_playbackProgressTimer(this, &HTMLMediaElement::playbackProgressTimerFired)
    , m_playedTimeRanges()
    , m_playbackRate(1.0f)
    , m_defaultPlaybackRate(1.0f)
    , m_webkitPreservesPitch(true)
    , m_networkState(NETWORK_EMPTY)
    , m_readyState(HAVE_NOTHING)
    , m_readyStateMaximum(HAVE_NOTHING)
    , m_volume(1.0f)
    , m_lastSeekTime(0)
    , m_previousProgress(0)
    , m_previousProgressTime(numeric_limits<double>::max())
    , m_lastTimeUpdateEventWallTime(0)
    , m_lastTimeUpdateEventMovieTime(numeric_limits<float>::max())
    , m_loadState(WaitingForSource)
    , m_currentSourceNode(0)
    , m_nextChildNodeToConsider(0)
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    , m_proxyWidget(0)
#endif
    , m_restrictions(RequireUserGestureForFullScreenRestriction)
    , m_preload(MediaPlayer::Auto)
    , m_displayMode(Unknown)
    , m_processingMediaPlayerCallback(0)
    , m_cachedTime(invalidMediaTime)
    , m_cachedTimeWallClockUpdateTime(0)
    , m_minimumWallClockTimeToCacheMediaTime(0)
    , m_playing(false)
    , m_isWaitingUntilMediaCanStart(false)
    , m_shouldDelayLoadEvent(false)
    , m_haveFiredLoadedData(false)
    , m_inActiveDocument(true)
    , m_autoplaying(true)
    , m_muted(false)
    , m_paused(true)
    , m_seeking(false)
    , m_sentStalledEvent(false)
    , m_sentEndEvent(false)
    , m_pausedInternal(false)
    , m_sendProgressEvents(true)
    , m_isFullscreen(false)
    , m_closedCaptionsVisible(false)
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    , m_needWidgetUpdate(false)
#endif
    , m_dispatchingCanPlayEvent(false)
    , m_loadInitiatedByUserGesture(false)
    , m_completelyLoaded(false)
    , m_havePreparedToPlay(false)
{
    LOG(Media, "HTMLMediaElement::HTMLMediaElement");
    document->registerForDocumentActivationCallbacks(this);
    document->registerForMediaVolumeCallbacks(this);
    document->registerForPrivateBrowsingStateChangedCallbacks(this);
}

HTMLMediaElement::~HTMLMediaElement()
{
    LOG(Media, "HTMLMediaElement::~HTMLMediaElement");
    if (m_isWaitingUntilMediaCanStart)
        document()->removeMediaCanStartListener(this);
    setShouldDelayLoadEvent(false);
    document()->unregisterForDocumentActivationCallbacks(this);
    document()->unregisterForMediaVolumeCallbacks(this);
    document()->unregisterForPrivateBrowsingStateChangedCallbacks(this);
}

void HTMLMediaElement::willMoveToNewOwnerDocument()
{
    if (m_isWaitingUntilMediaCanStart)
        document()->removeMediaCanStartListener(this);
    setShouldDelayLoadEvent(false);
    document()->unregisterForDocumentActivationCallbacks(this);
    document()->unregisterForMediaVolumeCallbacks(this);
    HTMLElement::willMoveToNewOwnerDocument();
}

void HTMLMediaElement::didMoveToNewOwnerDocument()
{
    if (m_isWaitingUntilMediaCanStart)
        document()->addMediaCanStartListener(this);
    if (m_readyState < HAVE_CURRENT_DATA)
        setShouldDelayLoadEvent(true);
    document()->registerForDocumentActivationCallbacks(this);
    document()->registerForMediaVolumeCallbacks(this);
    HTMLElement::didMoveToNewOwnerDocument();
}

void HTMLMediaElement::attributeChanged(Attribute* attr, bool preserveDecls)
{
    HTMLElement::attributeChanged(attr, preserveDecls);

    const QualifiedName& attrName = attr->name();
    if (attrName == srcAttr) {
        // Trigger a reload, as long as the 'src' attribute is present.
        if (!getAttribute(srcAttr).isEmpty())
            scheduleLoad();
    }
    else if (attrName == controlsAttr) {
#if !ENABLE(PLUGIN_PROXY_FOR_VIDEO)
        if (controls()) {
            if (!hasMediaControls()) {
                ensureMediaControls();
                mediaControls()->reset();
            }
            mediaControls()->show();
        } else if (hasMediaControls())
            mediaControls()->hide();
#else
        if (m_player)
            m_player->setControls(controls());
#endif
    }
}

void HTMLMediaElement::parseMappedAttribute(Attribute* attr)
{
    const QualifiedName& attrName = attr->name();

    if (attrName == preloadAttr) {
        String value = attr->value();

        if (equalIgnoringCase(value, "none"))
            m_preload = MediaPlayer::None;
        else if (equalIgnoringCase(value, "metadata"))
            m_preload = MediaPlayer::MetaData;
        else {
            // The spec does not define an "invalid value default" but "auto" is suggested as the
            // "missing value default", so use it for everything except "none" and "metadata"
            m_preload = MediaPlayer::Auto;
        }

        // The attribute must be ignored if the autoplay attribute is present
        if (!autoplay() && m_player)
            m_player->setPreload(m_preload);

    } else if (attrName == onabortAttr)
        setAttributeEventListener(eventNames().abortEvent, createAttributeEventListener(this, attr));
    else if (attrName == onbeforeloadAttr)
        setAttributeEventListener(eventNames().beforeloadEvent, createAttributeEventListener(this, attr));
    else if (attrName == oncanplayAttr)
        setAttributeEventListener(eventNames().canplayEvent, createAttributeEventListener(this, attr));
    else if (attrName == oncanplaythroughAttr)
        setAttributeEventListener(eventNames().canplaythroughEvent, createAttributeEventListener(this, attr));
    else if (attrName == ondurationchangeAttr)
        setAttributeEventListener(eventNames().durationchangeEvent, createAttributeEventListener(this, attr));
    else if (attrName == onemptiedAttr)
        setAttributeEventListener(eventNames().emptiedEvent, createAttributeEventListener(this, attr));
    else if (attrName == onendedAttr)
        setAttributeEventListener(eventNames().endedEvent, createAttributeEventListener(this, attr));
    else if (attrName == onerrorAttr)
        setAttributeEventListener(eventNames().errorEvent, createAttributeEventListener(this, attr));
    else if (attrName == onloadeddataAttr)
        setAttributeEventListener(eventNames().loadeddataEvent, createAttributeEventListener(this, attr));
    else if (attrName == onloadedmetadataAttr)
        setAttributeEventListener(eventNames().loadedmetadataEvent, createAttributeEventListener(this, attr));
    else if (attrName == onloadstartAttr)
        setAttributeEventListener(eventNames().loadstartEvent, createAttributeEventListener(this, attr));
    else if (attrName == onpauseAttr)
        setAttributeEventListener(eventNames().pauseEvent, createAttributeEventListener(this, attr));
    else if (attrName == onplayAttr)
        setAttributeEventListener(eventNames().playEvent, createAttributeEventListener(this, attr));
    else if (attrName == onplayingAttr)
        setAttributeEventListener(eventNames().playingEvent, createAttributeEventListener(this, attr));
    else if (attrName == onprogressAttr)
        setAttributeEventListener(eventNames().progressEvent, createAttributeEventListener(this, attr));
    else if (attrName == onratechangeAttr)
        setAttributeEventListener(eventNames().ratechangeEvent, createAttributeEventListener(this, attr));
    else if (attrName == onseekedAttr)
        setAttributeEventListener(eventNames().seekedEvent, createAttributeEventListener(this, attr));
    else if (attrName == onseekingAttr)
        setAttributeEventListener(eventNames().seekingEvent, createAttributeEventListener(this, attr));
    else if (attrName == onstalledAttr)
        setAttributeEventListener(eventNames().stalledEvent, createAttributeEventListener(this, attr));
    else if (attrName == onsuspendAttr)
        setAttributeEventListener(eventNames().suspendEvent, createAttributeEventListener(this, attr));
    else if (attrName == ontimeupdateAttr)
        setAttributeEventListener(eventNames().timeupdateEvent, createAttributeEventListener(this, attr));
    else if (attrName == onvolumechangeAttr)
        setAttributeEventListener(eventNames().volumechangeEvent, createAttributeEventListener(this, attr));
    else if (attrName == onwaitingAttr)
        setAttributeEventListener(eventNames().waitingEvent, createAttributeEventListener(this, attr));
    else if (attrName == onwebkitbeginfullscreenAttr)
        setAttributeEventListener(eventNames().webkitbeginfullscreenEvent, createAttributeEventListener(this, attr));
    else if (attrName == onwebkitendfullscreenAttr)
        setAttributeEventListener(eventNames().webkitendfullscreenEvent, createAttributeEventListener(this, attr));
    else
        HTMLElement::parseMappedAttribute(attr);
}

bool HTMLMediaElement::rendererIsNeeded(RenderStyle* style)
{
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    UNUSED_PARAM(style);
    Frame* frame = document()->frame();
    if (!frame)
        return false;

    return true;
#else
    return controls() ? HTMLElement::rendererIsNeeded(style) : false;
#endif
}

RenderObject* HTMLMediaElement::createRenderer(RenderArena* arena, RenderStyle*)
{
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    // Setup the renderer if we already have a proxy widget.
    RenderEmbeddedObject* mediaRenderer = new (arena) RenderEmbeddedObject(this);
    if (m_proxyWidget) {
        mediaRenderer->setWidget(m_proxyWidget);

        Frame* frame = document()->frame();
        FrameLoader* loader = frame ? frame->loader() : 0;
        if (loader)
            loader->showMediaPlayerProxyPlugin(m_proxyWidget.get());
    }
    return mediaRenderer;
#else
    return new (arena) RenderMedia(this);
#endif
}
 
void HTMLMediaElement::insertedIntoDocument()
{
    LOG(Media, "HTMLMediaElement::removedFromDocument");
    HTMLElement::insertedIntoDocument();
    if (!getAttribute(srcAttr).isEmpty() && m_networkState == NETWORK_EMPTY)
        scheduleLoad();
}

void HTMLMediaElement::removedFromDocument()
{
    LOG(Media, "HTMLMediaElement::removedFromDocument");
    if (m_networkState > NETWORK_EMPTY)
        pause(processingUserGesture());
    if (m_isFullscreen)
        exitFullscreen();
    HTMLElement::removedFromDocument();
}

void HTMLMediaElement::attach()
{
    ASSERT(!attached());

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    m_needWidgetUpdate = true;
#endif

    HTMLElement::attach();

    if (renderer())
        renderer()->updateFromElement();
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    else if (m_proxyWidget) {
        Frame* frame = document()->frame();
        FrameLoader* loader = frame ? frame->loader() : 0;
        if (loader)
            loader->hideMediaPlayerProxyPlugin(m_proxyWidget.get());
    }
#endif
}

void HTMLMediaElement::recalcStyle(StyleChange change)
{
    HTMLElement::recalcStyle(change);

    if (renderer())
        renderer()->updateFromElement();
}

void HTMLMediaElement::scheduleLoad()
{
    LOG(Media, "HTMLMediaElement::scheduleLoad");
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    createMediaPlayerProxy();
#endif

    if (m_loadTimer.isActive())
        return;
    prepareForLoad();
    m_loadTimer.startOneShot(0);
}

void HTMLMediaElement::scheduleNextSourceChild()
{
    // Schedule the timer to try the next <source> element WITHOUT resetting state ala prepareForLoad.
    m_loadTimer.startOneShot(0);
}

void HTMLMediaElement::scheduleEvent(const AtomicString& eventName)
{
#if LOG_MEDIA_EVENTS
    LOG(Media, "HTMLMediaElement::scheduleEvent - scheduling '%s'", eventName.string().ascii().data());
#endif
    m_pendingEvents.append(Event::create(eventName, false, true));
    if (!m_asyncEventTimer.isActive())
        m_asyncEventTimer.startOneShot(0);
}

void HTMLMediaElement::asyncEventTimerFired(Timer<HTMLMediaElement>*)
{
    Vector<RefPtr<Event> > pendingEvents;
    ExceptionCode ec = 0;

    m_pendingEvents.swap(pendingEvents);
    unsigned count = pendingEvents.size();
    for (unsigned ndx = 0; ndx < count; ++ndx) {
#if LOG_MEDIA_EVENTS
        LOG(Media, "HTMLMediaElement::asyncEventTimerFired - dispatching '%s'", pendingEvents[ndx]->type().string().ascii().data());
#endif
        if (pendingEvents[ndx]->type() == eventNames().canplayEvent) {
            m_dispatchingCanPlayEvent = true;
            dispatchEvent(pendingEvents[ndx].release(), ec);
            m_dispatchingCanPlayEvent = false;
        } else
            dispatchEvent(pendingEvents[ndx].release(), ec);
    }
}

void HTMLMediaElement::loadTimerFired(Timer<HTMLMediaElement>*)
{
    if (m_loadState == LoadingFromSourceElement)
        loadNextSourceChild();
    else
        loadInternal();
}

PassRefPtr<MediaError> HTMLMediaElement::error() const 
{
    return m_error;
}

void HTMLMediaElement::setSrc(const String& url)
{
    setAttribute(srcAttr, url);
}

String HTMLMediaElement::currentSrc() const
{
    return m_currentSrc;
}

HTMLMediaElement::NetworkState HTMLMediaElement::networkState() const
{
    return m_networkState;
}

String HTMLMediaElement::canPlayType(const String& mimeType) const
{
    MediaPlayer::SupportsType support = MediaPlayer::supportsType(ContentType(mimeType));
    String canPlay;

    // 4.8.10.3
    switch (support)
    {
        case MediaPlayer::IsNotSupported:
            canPlay = "";
            break;
        case MediaPlayer::MayBeSupported:
            canPlay = "maybe";
            break;
        case MediaPlayer::IsSupported:
            canPlay = "probably";
            break;
    }
    
    LOG(Media, "HTMLMediaElement::canPlayType(%s) -> %s", mimeType.utf8().data(), canPlay.utf8().data());

    return canPlay;
}

void HTMLMediaElement::load(bool isUserGesture, ExceptionCode& ec)
{
    LOG(Media, "HTMLMediaElement::load(isUserGesture : %s)", boolString(isUserGesture));

    if (m_restrictions & RequireUserGestureForLoadRestriction && !isUserGesture)
        ec = INVALID_STATE_ERR;
    else {
        m_loadInitiatedByUserGesture = isUserGesture;
        prepareForLoad();
        loadInternal();
    }
}

void HTMLMediaElement::prepareForLoad()
{
    LOG(Media, "HTMLMediaElement::prepareForLoad");

    // Perform the cleanup required for the resource load algorithm to run.
    stopPeriodicTimers();
    m_loadTimer.stop();
    m_sentEndEvent = false;
    m_sentStalledEvent = false;
    m_haveFiredLoadedData = false;
    m_completelyLoaded = false;
    m_havePreparedToPlay = false;
    m_displayMode = Unknown;

    // 1 - Abort any already-running instance of the resource selection algorithm for this element.
    m_loadState = WaitingForSource;
    m_currentSourceNode = 0;

    // 2 - If there are any tasks from the media element's media element event task source in 
    // one of the task queues, then remove those tasks.
    cancelPendingEventsAndCallbacks();

    // 3 - If the media element's networkState is set to NETWORK_LOADING or NETWORK_IDLE, queue
    // a task to fire a simple event named abort at the media element.
    if (m_networkState == NETWORK_LOADING || m_networkState == NETWORK_IDLE)
        scheduleEvent(eventNames().abortEvent);

#if !ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    m_player = MediaPlayer::create(this);
#else
    if (m_player)
        m_player->cancelLoad();
    else
        createMediaPlayerProxy();
#endif

    // 4 - If the media element's networkState is not set to NETWORK_EMPTY, then run these substeps
    if (m_networkState != NETWORK_EMPTY) {
        m_networkState = NETWORK_EMPTY;
        m_readyState = HAVE_NOTHING;
        m_readyStateMaximum = HAVE_NOTHING;
        refreshCachedTime();
        m_paused = true;
        m_seeking = false;
        invalidateCachedTime();
        scheduleEvent(eventNames().emptiedEvent);
    }

    // 5 - Set the playbackRate attribute to the value of the defaultPlaybackRate attribute.
    setPlaybackRate(defaultPlaybackRate());

    // 6 - Set the error attribute to null and the autoplaying flag to true.
    m_error = 0;
    m_autoplaying = true;

    // 7 - Invoke the media element's resource selection algorithm.

    // 8 - Note: Playback of any previously playing media resource for this element stops.

    // The resource selection algorithm
    // 1 - Set the networkState to NETWORK_NO_SOURCE
    m_networkState = NETWORK_NO_SOURCE;

    // 2 - Asynchronously await a stable state.

    m_playedTimeRanges = TimeRanges::create();
    m_lastSeekTime = 0;
    m_closedCaptionsVisible = false;

    // The spec doesn't say to block the load event until we actually run the asynchronous section
    // algorithm, but do it now because we won't start that until after the timer fires and the 
    // event may have already fired by then.
    setShouldDelayLoadEvent(true);
}

void HTMLMediaElement::loadInternal()
{
    // If we can't start a load right away, start it later.
    Page* page = document()->page();
    if (page && !page->canStartMedia()) {
        if (m_isWaitingUntilMediaCanStart)
            return;
        document()->addMediaCanStartListener(this);
        m_isWaitingUntilMediaCanStart = true;
        return;
    }

    selectMediaResource();
}

void HTMLMediaElement::selectMediaResource()
{
    LOG(Media, "HTMLMediaElement::selectMediaResource");

    enum Mode { attribute, children };
    Mode mode = attribute;

    // 3 - ... the media element has neither a src attribute ...
    if (!hasAttribute(srcAttr)) {
        // ... nor a source element child: ...
        Node* node;
        for (node = firstChild(); node; node = node->nextSibling()) {
            if (node->hasTagName(sourceTag))
                break;
        }

        if (!node) {
            m_loadState = WaitingForSource;
            setShouldDelayLoadEvent(false);

            // ... set the networkState to NETWORK_EMPTY, and abort these steps
            m_networkState = NETWORK_EMPTY;

            LOG(Media, "HTMLMediaElement::selectMediaResource, nothing to load");
            return;
        }

        mode = children;
    }

    // 4 - Set the media element's delaying-the-load-event flag to true (this delays the load event), 
    // and set its networkState to NETWORK_LOADING.
    setShouldDelayLoadEvent(true);
    m_networkState = NETWORK_LOADING;

    // 5
    scheduleEvent(eventNames().loadstartEvent);

    // 6 - If mode is attribute, then run these substeps
    if (mode == attribute) {
        // If the src attribute's value is the empty string ... jump down to the failed step below
        KURL mediaURL = getNonEmptyURLAttribute(srcAttr);
        if (mediaURL.isEmpty()) {
            noneSupported();
            LOG(Media, "HTMLMediaElement::selectMediaResource, empty 'src'");
            return;
        }

        if (isSafeToLoadURL(mediaURL, Complain) && dispatchBeforeLoadEvent(mediaURL.string())) {
            ContentType contentType("");
            m_loadState = LoadingFromSrcAttr;
            loadResource(mediaURL, contentType);
        } else 
            noneSupported();

        LOG(Media, "HTMLMediaElement::selectMediaResource, 'src' not used");
        return;
    }

    // Otherwise, the source elements will be used
    m_currentSourceNode = 0;
    loadNextSourceChild();
}

void HTMLMediaElement::loadNextSourceChild()
{
    ContentType contentType("");
    KURL mediaURL = selectNextSourceChild(&contentType, Complain);
    if (!mediaURL.isValid()) {
        waitForSourceChange();
        return;
    }

#if !ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    // Recreate the media player for the new url
    m_player = MediaPlayer::create(this);
#endif

    m_loadState = LoadingFromSourceElement;
    loadResource(mediaURL, contentType);
}

void HTMLMediaElement::loadResource(const KURL& initialURL, ContentType& contentType)
{
    ASSERT(isSafeToLoadURL(initialURL, Complain));

    LOG(Media, "HTMLMediaElement::loadResource(%s, %s)", urlForLogging(initialURL.string()).utf8().data(), contentType.raw().utf8().data());

    Frame* frame = document()->frame();
    if (!frame)
        return;
    FrameLoader* loader = frame->loader();
    if (!loader)
        return;

    KURL url(initialURL);
    if (!loader->willLoadMediaElementURL(url))
        return;

    // The resource fetch algorithm 
    m_networkState = NETWORK_LOADING;

    m_currentSrc = url;

    LOG(Media, "HTMLMediaElement::loadResource - m_currentSrc -> %s", urlForLogging(m_currentSrc).utf8().data());

    if (m_sendProgressEvents) 
        startProgressEventTimer();

    Settings* settings = document()->settings();
    bool privateMode = !settings || settings->privateBrowsingEnabled();
    m_player->setPrivateBrowsingMode(privateMode);

    // Reset display mode to force a recalculation of what to show because we are resetting the player.
    setDisplayMode(Unknown);

    if (!autoplay())
        m_player->setPreload(m_preload);
    m_player->setPreservesPitch(m_webkitPreservesPitch);
    updateVolume();

    m_player->load(m_currentSrc, contentType);

    // If there is no poster to display, allow the media engine to render video frames as soon as
    // they are available.
    updateDisplayState();

    if (renderer())
        renderer()->updateFromElement();
}

bool HTMLMediaElement::isSafeToLoadURL(const KURL& url, InvalidSourceAction actionIfInvalid)
{
    if (!url.isValid()) {
        LOG(Media, "HTMLMediaElement::isSafeToLoadURL(%s) -> FALSE because url is invalid", urlForLogging(url.string()).utf8().data());
        return false;
    }

    Frame* frame = document()->frame();
    if (!frame || !document()->securityOrigin()->canDisplay(url)) {
        if (actionIfInvalid == Complain)
            FrameLoader::reportLocalLoadFailed(frame, url.string());
        LOG(Media, "HTMLMediaElement::isSafeToLoadURL(%s) -> FALSE rejected by SecurityOrigin", urlForLogging(url.string()).utf8().data());
        return false;
    }

    if (!document()->contentSecurityPolicy()->allowMediaFromSource(url))
        return false;

    return true;
}

void HTMLMediaElement::startProgressEventTimer()
{
    if (m_progressEventTimer.isActive())
        return;

    m_previousProgressTime = WTF::currentTime();
    m_previousProgress = 0;
    // 350ms is not magic, it is in the spec!
    m_progressEventTimer.startRepeating(0.350);
}

void HTMLMediaElement::waitForSourceChange()
{
    LOG(Media, "HTMLMediaElement::waitForSourceChange");

    stopPeriodicTimers();
    m_loadState = WaitingForSource;

    // 6.17 - Waiting: Set the element's networkState attribute to the NETWORK_NO_SOURCE value
    m_networkState = NETWORK_NO_SOURCE;

    // 6.18 - Set the element's delaying-the-load-event flag to false. This stops delaying the load event.
    setShouldDelayLoadEvent(false);

    updateDisplayState();

    if (renderer())
        renderer()->updateFromElement();
}

void HTMLMediaElement::noneSupported()
{
    LOG(Media, "HTMLMediaElement::noneSupported");

    stopPeriodicTimers();
    m_loadState = WaitingForSource;
    m_currentSourceNode = 0;

    // 5 - Reaching this step indicates that either the URL failed to resolve, or the media
    // resource failed to load. Set the error attribute to a new MediaError object whose
    // code attribute is set to MEDIA_ERR_SRC_NOT_SUPPORTED.
    m_error = MediaError::create(MediaError::MEDIA_ERR_SRC_NOT_SUPPORTED);

    // 6 - Set the element's networkState attribute to the NETWORK_NO_SOURCE value.
    m_networkState = NETWORK_NO_SOURCE;

    // 7 - Queue a task to fire a progress event called error at the media element, in
    // the context of the fetching process that was used to try to obtain the media
    // resource in the resource fetch algorithm.
    scheduleEvent(eventNames().errorEvent);

    // 8 - Set the element's delaying-the-load-event flag to false. This stops delaying the load event.
    setShouldDelayLoadEvent(false);

    // 9 -Abort these steps. Until the load() method is invoked, the element won't attempt to load another resource.

    updateDisplayState();

    if (renderer())
        renderer()->updateFromElement();
}

void HTMLMediaElement::mediaEngineError(PassRefPtr<MediaError> err)
{
    LOG(Media, "HTMLMediaElement::mediaEngineError(%d)", static_cast<int>(err->code()));

    // 1 - The user agent should cancel the fetching process.
    stopPeriodicTimers();
    m_loadState = WaitingForSource;

    // 2 - Set the error attribute to a new MediaError object whose code attribute is 
    // set to MEDIA_ERR_NETWORK/MEDIA_ERR_DECODE.
    m_error = err;

    // 3 - Queue a task to fire a simple event named error at the media element.
    scheduleEvent(eventNames().errorEvent);

    // 4 - Set the element's networkState attribute to the NETWORK_EMPTY value and queue a
    // task to fire a simple event called emptied at the element.
    m_networkState = NETWORK_EMPTY;
    scheduleEvent(eventNames().emptiedEvent);

    // 5 - Set the element's delaying-the-load-event flag to false. This stops delaying the load event.
    setShouldDelayLoadEvent(false);

    // 6 - Abort the overall resource selection algorithm.
    m_currentSourceNode = 0;
}

void HTMLMediaElement::cancelPendingEventsAndCallbacks()
{
    LOG(Media, "HTMLMediaElement::cancelPendingEventsAndCallbacks");

    m_pendingEvents.clear();

    for (Node* node = firstChild(); node; node = node->nextSibling()) {
        if (node->hasTagName(sourceTag))
            static_cast<HTMLSourceElement*>(node)->cancelPendingErrorEvent();
    }
}

Document* HTMLMediaElement::mediaPlayerOwningDocument()
{
    Document* d = document();
    
    if (!d)
        d = ownerDocument();
    
    return d;
}

void HTMLMediaElement::mediaPlayerNetworkStateChanged(MediaPlayer*)
{
    beginProcessingMediaPlayerCallback();
    setNetworkState(m_player->networkState());
    endProcessingMediaPlayerCallback();
}

void HTMLMediaElement::setNetworkState(MediaPlayer::NetworkState state)
{
    LOG(Media, "HTMLMediaElement::setNetworkState(%d) - current state is %d", static_cast<int>(state), static_cast<int>(m_networkState));

    if (state == MediaPlayer::Empty) {
        // Just update the cached state and leave, we can't do anything.
        m_networkState = NETWORK_EMPTY;
        return;
    }

    if (state == MediaPlayer::FormatError || state == MediaPlayer::NetworkError || state == MediaPlayer::DecodeError) {
        stopPeriodicTimers();

        // If we failed while trying to load a <source> element, the movie was never parsed, and there are more
        // <source> children, schedule the next one
        if (m_readyState < HAVE_METADATA && m_loadState == LoadingFromSourceElement) {

            if (m_currentSourceNode)
                m_currentSourceNode->scheduleErrorEvent();
            else
                LOG(Media, "HTMLMediaElement::setNetworkState - error event not sent, <source> was removed");

            if (havePotentialSourceChild()) {
                LOG(Media, "HTMLMediaElement::setNetworkState - scheduling next <source>");
                scheduleNextSourceChild();
            } else {
                LOG(Media, "HTMLMediaElement::setNetworkState - no more <source> elements, waiting");
                waitForSourceChange();
            }

            return;
        }

        if (state == MediaPlayer::NetworkError)
            mediaEngineError(MediaError::create(MediaError::MEDIA_ERR_NETWORK));
        else if (state == MediaPlayer::DecodeError)
            mediaEngineError(MediaError::create(MediaError::MEDIA_ERR_DECODE));
        else if (state == MediaPlayer::FormatError && m_loadState == LoadingFromSrcAttr)
            noneSupported();

        updateDisplayState();
        if (hasMediaControls())
            mediaControls()->reportedError();
        return;
    }

    if (state == MediaPlayer::Idle) {
        if (m_networkState > NETWORK_IDLE) {
            m_progressEventTimer.stop();
            scheduleEvent(eventNames().suspendEvent);
            setShouldDelayLoadEvent(false);
        }
        m_networkState = NETWORK_IDLE;
    }

    if (state == MediaPlayer::Loading) {
        if (m_networkState < NETWORK_LOADING || m_networkState == NETWORK_NO_SOURCE)
            startProgressEventTimer();
        m_networkState = NETWORK_LOADING;
    }

    if (state == MediaPlayer::Loaded) {
        if (m_networkState != NETWORK_IDLE) {
            m_progressEventTimer.stop();

            // Schedule one last progress event so we guarantee that at least one is fired
            // for files that load very quickly.
            scheduleEvent(eventNames().progressEvent);
        }
        m_networkState = NETWORK_IDLE;
        m_completelyLoaded = true;
    }

    if (hasMediaControls())
        mediaControls()->updateStatusDisplay();
}

void HTMLMediaElement::mediaPlayerReadyStateChanged(MediaPlayer*)
{
    beginProcessingMediaPlayerCallback();

    setReadyState(m_player->readyState());

    endProcessingMediaPlayerCallback();
}

void HTMLMediaElement::setReadyState(MediaPlayer::ReadyState state)
{
    LOG(Media, "HTMLMediaElement::setReadyState(%d) - current state is %d,", static_cast<int>(state), static_cast<int>(m_readyState));

    // Set "wasPotentiallyPlaying" BEFORE updating m_readyState, potentiallyPlaying() uses it
    bool wasPotentiallyPlaying = potentiallyPlaying();

    ReadyState oldState = m_readyState;
    m_readyState = static_cast<ReadyState>(state);

    if (m_readyState == oldState)
        return;
    
    if (oldState > m_readyStateMaximum)
        m_readyStateMaximum = oldState;

    if (m_networkState == NETWORK_EMPTY)
        return;

    if (m_seeking) {
        // 4.8.10.9, step 11
        if (wasPotentiallyPlaying && m_readyState < HAVE_FUTURE_DATA)
            scheduleEvent(eventNames().waitingEvent);

        // 4.8.10.10 step 14 & 15.
        if (m_readyState >= HAVE_CURRENT_DATA)
            finishSeek();
    } else {
        if (wasPotentiallyPlaying && m_readyState < HAVE_FUTURE_DATA) {
            // 4.8.10.8
            scheduleTimeupdateEvent(false);
            scheduleEvent(eventNames().waitingEvent);
        }
    }

    if (m_readyState >= HAVE_METADATA && oldState < HAVE_METADATA) {
        scheduleEvent(eventNames().durationchangeEvent);
        scheduleEvent(eventNames().loadedmetadataEvent);
        if (hasMediaControls())
            mediaControls()->loadedMetadata();
        if (renderer())
            renderer()->updateFromElement();
    }

    bool shouldUpdateDisplayState = false;

    if (m_readyState >= HAVE_CURRENT_DATA && oldState < HAVE_CURRENT_DATA && !m_haveFiredLoadedData) {
        m_haveFiredLoadedData = true;
        shouldUpdateDisplayState = true;
        scheduleEvent(eventNames().loadeddataEvent);
        setShouldDelayLoadEvent(false);
    }

    bool isPotentiallyPlaying = potentiallyPlaying();
    if (m_readyState == HAVE_FUTURE_DATA && oldState <= HAVE_CURRENT_DATA) {
        scheduleEvent(eventNames().canplayEvent);
        if (isPotentiallyPlaying)
            scheduleEvent(eventNames().playingEvent);
        shouldUpdateDisplayState = true;
    }

    if (m_readyState == HAVE_ENOUGH_DATA && oldState < HAVE_ENOUGH_DATA) {
        if (oldState <= HAVE_CURRENT_DATA)
            scheduleEvent(eventNames().canplayEvent);

        scheduleEvent(eventNames().canplaythroughEvent);

        if (isPotentiallyPlaying && oldState <= HAVE_CURRENT_DATA)
            scheduleEvent(eventNames().playingEvent);

        if (m_autoplaying && m_paused && autoplay()) {
            m_paused = false;
            invalidateCachedTime();
            scheduleEvent(eventNames().playEvent);
            scheduleEvent(eventNames().playingEvent);
        }

        shouldUpdateDisplayState = true;
    }

    if (shouldUpdateDisplayState) {
        updateDisplayState();
        if (hasMediaControls())
            mediaControls()->updateStatusDisplay();
    }

    updatePlayState();
}

void HTMLMediaElement::progressEventTimerFired(Timer<HTMLMediaElement>*)
{
    ASSERT(m_player);
    if (m_networkState != NETWORK_LOADING)
        return;

    unsigned progress = m_player->bytesLoaded();
    double time = WTF::currentTime();
    double timedelta = time - m_previousProgressTime;

    if (progress == m_previousProgress) {
        if (timedelta > 3.0 && !m_sentStalledEvent) {
            scheduleEvent(eventNames().stalledEvent);
            m_sentStalledEvent = true;
            setShouldDelayLoadEvent(false);
        }
    } else {
        scheduleEvent(eventNames().progressEvent);
        m_previousProgress = progress;
        m_previousProgressTime = time;
        m_sentStalledEvent = false;
        if (renderer())
            renderer()->updateFromElement();
    }
}

void HTMLMediaElement::rewind(float timeDelta)
{
    LOG(Media, "HTMLMediaElement::rewind(%f)", timeDelta);

    ExceptionCode e;
    setCurrentTime(max(currentTime() - timeDelta, minTimeSeekable()), e);
}

void HTMLMediaElement::returnToRealtime()
{
    LOG(Media, "HTMLMediaElement::returnToRealtime");
    ExceptionCode e;
    setCurrentTime(maxTimeSeekable(), e);
}  

void HTMLMediaElement::addPlayedRange(float start, float end)
{
    LOG(Media, "HTMLMediaElement::addPlayedRange(%f, %f)", start, end);
    if (!m_playedTimeRanges)
        m_playedTimeRanges = TimeRanges::create();
    m_playedTimeRanges->add(start, end);
}  

bool HTMLMediaElement::supportsSave() const
{
    return m_player ? m_player->supportsSave() : false;
}

void HTMLMediaElement::prepareToPlay()
{
    if (m_havePreparedToPlay)
        return;
    m_havePreparedToPlay = true;
    m_player->prepareToPlay();
}

void HTMLMediaElement::seek(float time, ExceptionCode& ec)
{
    LOG(Media, "HTMLMediaElement::seek(%f)", time);

    // 4.8.9.9 Seeking

    // 1 - If the media element's readyState is HAVE_NOTHING, then raise an INVALID_STATE_ERR exception.
    if (m_readyState == HAVE_NOTHING || !m_player) {
        ec = INVALID_STATE_ERR;
        return;
    }

    // If the media engine has been told to postpone loading data, let it go ahead now.
    if (m_preload < MediaPlayer::Auto && m_readyState < HAVE_FUTURE_DATA)
        prepareToPlay();

    // Get the current time before setting m_seeking, m_lastSeekTime is returned once it is set.
    refreshCachedTime();
    float now = currentTime();

    // 2 - If the element's seeking IDL attribute is true, then another instance of this algorithm is
    // already running. Abort that other instance of the algorithm without waiting for the step that
    // it is running to complete.
    // Nothing specific to be done here.

    // 3 - Set the seeking IDL attribute to true.
    // The flag will be cleared when the engine tells us the time has actually changed.
    m_seeking = true;

    // 5 - If the new playback position is later than the end of the media resource, then let it be the end 
    // of the media resource instead.
    time = min(time, duration());

    // 6 - If the new playback position is less than the earliest possible position, let it be that position instead.
    float earliestTime = m_player->startTime();
    time = max(time, earliestTime);

    // Ask the media engine for the time value in the movie's time scale before comparing with current time. This
    // is necessary because if the seek time is not equal to currentTime but the delta is less than the movie's
    // time scale, we will ask the media engine to "seek" to the current movie time, which may be a noop and
    // not generate a timechanged callback. This means m_seeking will never be cleared and we will never 
    // fire a 'seeked' event.
#if !LOG_DISABLED
    float mediaTime = m_player->mediaTimeForTimeValue(time);
    if (time != mediaTime)
        LOG(Media, "HTMLMediaElement::seek(%f) - media timeline equivalent is %f", time, mediaTime);
#endif
    time = m_player->mediaTimeForTimeValue(time);

    // 7 - If the (possibly now changed) new playback position is not in one of the ranges given in the 
    // seekable attribute, then let it be the position in one of the ranges given in the seekable attribute 
    // that is the nearest to the new playback position. ... If there are no ranges given in the seekable
    // attribute then set the seeking IDL attribute to false and abort these steps.
    RefPtr<TimeRanges> seekableRanges = seekable();

    // Short circuit seeking to the current time by just firing the events if no seek is required.
    // Don't skip calling the media engine if we are in poster mode because a seek should always 
    // cancel poster display.
    bool noSeekRequired = !seekableRanges->length() || (time == now && displayMode() != Poster);
    if (noSeekRequired) {
        if (time == now) {
            scheduleEvent(eventNames().seekingEvent);
            scheduleTimeupdateEvent(false);
            scheduleEvent(eventNames().seekedEvent);
        }
        m_seeking = false;
        return;
    }
    time = seekableRanges->nearest(time);

    if (m_playing) {
        if (m_lastSeekTime < now)
            addPlayedRange(m_lastSeekTime, now);
    }
    m_lastSeekTime = time;
    m_sentEndEvent = false;

    // 8 - Set the current playback position to the given new playback position
    m_player->seek(time);

    // 9 - Queue a task to fire a simple event named seeking at the element.
    scheduleEvent(eventNames().seekingEvent);

    // 10 - Queue a task to fire a simple event named timeupdate at the element.
    scheduleTimeupdateEvent(false);

    // 11-15 are handled, if necessary, when the engine signals a readystate change.
}

void HTMLMediaElement::finishSeek()
{
    LOG(Media, "HTMLMediaElement::finishSeek");

    // 4.8.10.9 Seeking step 14
    m_seeking = false;

    // 4.8.10.9 Seeking step 15
    scheduleEvent(eventNames().seekedEvent);

    setDisplayMode(Video);
}

HTMLMediaElement::ReadyState HTMLMediaElement::readyState() const
{
    return m_readyState;
}

MediaPlayer::MovieLoadType HTMLMediaElement::movieLoadType() const
{
    return m_player ? m_player->movieLoadType() : MediaPlayer::Unknown;
}

bool HTMLMediaElement::hasAudio() const
{
    return m_player ? m_player->hasAudio() : false;
}

bool HTMLMediaElement::seeking() const
{
    return m_seeking;
}

void HTMLMediaElement::refreshCachedTime() const
{
    m_cachedTime = m_player->currentTime();
    m_cachedTimeWallClockUpdateTime = WTF::currentTime();
}

void HTMLMediaElement::invalidateCachedTime()
{
    LOG(Media, "HTMLMediaElement::invalidateCachedTime");

    // Don't try to cache movie time when playback first starts as the time reported by the engine
    // sometimes fluctuates for a short amount of time, so the cached time will be off if we take it
    // too early.
    static const double minimumTimePlayingBeforeCacheSnapshot = 0.5;

    m_minimumWallClockTimeToCacheMediaTime = WTF::currentTime() + minimumTimePlayingBeforeCacheSnapshot;
    m_cachedTime = invalidMediaTime;
}

// playback state
float HTMLMediaElement::currentTime() const
{
#if LOG_CACHED_TIME_WARNINGS
    static const double minCachedDeltaForWarning = 0.01;
#endif

    if (!m_player)
        return 0;

    if (m_seeking) {
        LOG(Media, "HTMLMediaElement::currentTime - seeking, returning %f", m_lastSeekTime);
        return m_lastSeekTime;
    }

    if (m_cachedTime != invalidMediaTime && m_paused) {
#if LOG_CACHED_TIME_WARNINGS
        float delta = m_cachedTime - m_player->currentTime();
        if (delta > minCachedDeltaForWarning)
            LOG(Media, "HTMLMediaElement::currentTime - WARNING, cached time is %f seconds off of media time when paused", delta);
#endif
        return m_cachedTime;
    }

    // Is it too soon use a cached time?
    double now = WTF::currentTime();
    double maximumDurationToCacheMediaTime = m_player->maximumDurationToCacheMediaTime();

    if (maximumDurationToCacheMediaTime && m_cachedTime != invalidMediaTime && !m_paused && now > m_minimumWallClockTimeToCacheMediaTime) {
        double wallClockDelta = now - m_cachedTimeWallClockUpdateTime;

        // Not too soon, use the cached time only if it hasn't expired.
        if (wallClockDelta < maximumDurationToCacheMediaTime) {
            float adjustedCacheTime = static_cast<float>(m_cachedTime + (m_playbackRate * wallClockDelta));

#if LOG_CACHED_TIME_WARNINGS
            float delta = adjustedCacheTime - m_player->currentTime();
            if (delta > minCachedDeltaForWarning)
                LOG(Media, "HTMLMediaElement::currentTime - WARNING, cached time is %f seconds off of media time when playing", delta);
#endif
            return adjustedCacheTime;
        }
    }

#if LOG_CACHED_TIME_WARNINGS
    if (maximumDurationToCacheMediaTime && now > m_minimumWallClockTimeToCacheMediaTime && m_cachedTime != invalidMediaTime) {
        double wallClockDelta = now - m_cachedTimeWallClockUpdateTime;
        float delta = m_cachedTime + (m_playbackRate * wallClockDelta) - m_player->currentTime();
        LOG(Media, "HTMLMediaElement::currentTime - cached time was %f seconds off of media time when it expired", delta);
    }
#endif

    refreshCachedTime();

    return m_cachedTime;
}

void HTMLMediaElement::setCurrentTime(float time, ExceptionCode& ec)
{
    seek(time, ec);
}

float HTMLMediaElement::startTime() const
{
    if (!m_player)
        return 0;
    return m_player->startTime();
}

float HTMLMediaElement::duration() const
{
    if (m_player && m_readyState >= HAVE_METADATA)
        return m_player->duration();

    return numeric_limits<float>::quiet_NaN();
}

bool HTMLMediaElement::paused() const
{
    return m_paused;
}

float HTMLMediaElement::defaultPlaybackRate() const
{
    return m_defaultPlaybackRate;
}

void HTMLMediaElement::setDefaultPlaybackRate(float rate)
{
    if (m_defaultPlaybackRate != rate) {
        m_defaultPlaybackRate = rate;
        scheduleEvent(eventNames().ratechangeEvent);
    }
}

float HTMLMediaElement::playbackRate() const
{
    return m_playbackRate;
}

void HTMLMediaElement::setPlaybackRate(float rate)
{
    LOG(Media, "HTMLMediaElement::setPlaybackRate(%f)", rate);

    if (m_playbackRate != rate) {
        m_playbackRate = rate;
        invalidateCachedTime();
        scheduleEvent(eventNames().ratechangeEvent);
    }
    if (m_player && potentiallyPlaying() && m_player->rate() != rate)
        m_player->setRate(rate);
}

bool HTMLMediaElement::webkitPreservesPitch() const
{
    return m_webkitPreservesPitch;
}

void HTMLMediaElement::setWebkitPreservesPitch(bool preservesPitch)
{
    LOG(Media, "HTMLMediaElement::setWebkitPreservesPitch(%s)", boolString(preservesPitch));

    m_webkitPreservesPitch = preservesPitch;
    
    if (!m_player)
        return;

    m_player->setPreservesPitch(preservesPitch);
}

bool HTMLMediaElement::ended() const
{
    // 4.8.10.8 Playing the media resource
    // The ended attribute must return true if the media element has ended 
    // playback and the direction of playback is forwards, and false otherwise.
    return endedPlayback() && m_playbackRate > 0;
}

bool HTMLMediaElement::autoplay() const
{
    return hasAttribute(autoplayAttr);
}

void HTMLMediaElement::setAutoplay(bool b)
{
    LOG(Media, "HTMLMediaElement::setAutoplay(%s)", boolString(b));
    setBooleanAttribute(autoplayAttr, b);
}

String HTMLMediaElement::preload() const
{
    switch (m_preload) {
    case MediaPlayer::None:
        return "none";
        break;
    case MediaPlayer::MetaData:
        return "metadata";
        break;
    case MediaPlayer::Auto:
        return "auto";
        break;
    }

    ASSERT_NOT_REACHED();
    return String();
}

void HTMLMediaElement::setPreload(const String& preload)
{
    LOG(Media, "HTMLMediaElement::setPreload(%s)", preload.utf8().data());
    setAttribute(preloadAttr, preload);
}

void HTMLMediaElement::play(bool isUserGesture)
{
    LOG(Media, "HTMLMediaElement::play(isUserGesture : %s)", boolString(isUserGesture));

    if (m_restrictions & RequireUserGestureForRateChangeRestriction && !isUserGesture)
        return;

    Document* doc = document();
    Settings* settings = doc->settings();
    if (settings && settings->needsSiteSpecificQuirks() && m_dispatchingCanPlayEvent && !m_loadInitiatedByUserGesture) {
        // It should be impossible to be processing the canplay event while handling a user gesture
        // since it is dispatched asynchronously.
        ASSERT(!isUserGesture);
        String host = doc->baseURL().host();
        if (host.endsWith(".npr.org", false) || equalIgnoringCase(host, "npr.org"))
            return;
    }
    
    playInternal();
}

void HTMLMediaElement::playInternal()
{
    LOG(Media, "HTMLMediaElement::playInternal");

    // 4.8.10.9. Playing the media resource
    if (!m_player || m_networkState == NETWORK_EMPTY)
        scheduleLoad();

    if (endedPlayback()) {
        ExceptionCode unused;
        seek(0, unused);
    }
    
    if (m_paused) {
        m_paused = false;
        invalidateCachedTime();
        scheduleEvent(eventNames().playEvent);

        if (m_readyState <= HAVE_CURRENT_DATA)
            scheduleEvent(eventNames().waitingEvent);
        else if (m_readyState >= HAVE_FUTURE_DATA)
            scheduleEvent(eventNames().playingEvent);
    }
    m_autoplaying = false;

    updatePlayState();
}

void HTMLMediaElement::pause(bool isUserGesture)
{
    LOG(Media, "HTMLMediaElement::pause(isUserGesture : %s)", boolString(isUserGesture));

    if (m_restrictions & RequireUserGestureForRateChangeRestriction && !isUserGesture)
        return;

    pauseInternal();
}


void HTMLMediaElement::pauseInternal()
{
    LOG(Media, "HTMLMediaElement::pauseInternal");

    // 4.8.10.9. Playing the media resource
    if (!m_player || m_networkState == NETWORK_EMPTY)
        scheduleLoad();

    m_autoplaying = false;
    
    if (!m_paused) {
        m_paused = true;
        scheduleTimeupdateEvent(false);
        scheduleEvent(eventNames().pauseEvent);
    }

    updatePlayState();
}

bool HTMLMediaElement::loop() const
{
    return hasAttribute(loopAttr);
}

void HTMLMediaElement::setLoop(bool b)
{
    LOG(Media, "HTMLMediaElement::setLoop(%s)", boolString(b));
    setBooleanAttribute(loopAttr, b);
}

bool HTMLMediaElement::controls() const
{
    Frame* frame = document()->frame();

    // always show controls when scripting is disabled
    if (frame && !frame->script()->canExecuteScripts(NotAboutToExecuteScript))
        return true;

    // always show controls for video when fullscreen playback is required.
    if (isVideo() && document()->page() && document()->page()->chrome()->requiresFullscreenForVideoPlayback())
        return true;

    // Always show controls when in full screen mode.
    if (isFullscreen())
        return true;

    return hasAttribute(controlsAttr);
}

void HTMLMediaElement::setControls(bool b)
{
    LOG(Media, "HTMLMediaElement::setControls(%s)", boolString(b));
    setBooleanAttribute(controlsAttr, b);
}

float HTMLMediaElement::volume() const
{
    return m_volume;
}

void HTMLMediaElement::setVolume(float vol, ExceptionCode& ec)
{
    LOG(Media, "HTMLMediaElement::setVolume(%f)", vol);

    if (vol < 0.0f || vol > 1.0f) {
        ec = INDEX_SIZE_ERR;
        return;
    }
    
    if (m_volume != vol) {
        m_volume = vol;
        updateVolume();
        scheduleEvent(eventNames().volumechangeEvent);
    }
}

bool HTMLMediaElement::muted() const
{
    return m_muted;
}

void HTMLMediaElement::setMuted(bool muted)
{
    LOG(Media, "HTMLMediaElement::setMuted(%s)", boolString(muted));

    if (m_muted != muted) {
        m_muted = muted;
        // Avoid recursion when the player reports volume changes.
        if (!processingMediaPlayerCallback()) {
            if (m_player) {
                m_player->setMuted(m_muted);
                if (hasMediaControls())
                    mediaControls()->changedMute();
            }
        }
        scheduleEvent(eventNames().volumechangeEvent);
    }
}

void HTMLMediaElement::togglePlayState()
{
    LOG(Media, "HTMLMediaElement::togglePlayState - canPlay() is %s", boolString(canPlay()));

    // We can safely call the internal play/pause methods, which don't check restrictions, because
    // this method is only called from the built-in media controller
    if (canPlay()) {
        setPlaybackRate(defaultPlaybackRate());
        playInternal();
    } else 
        pauseInternal();
}

void HTMLMediaElement::beginScrubbing()
{
    LOG(Media, "HTMLMediaElement::beginScrubbing - paused() is %s", boolString(paused()));

    if (!paused()) {
        if (ended()) {
            // Because a media element stays in non-paused state when it reaches end, playback resumes 
            // when the slider is dragged from the end to another position unless we pause first. Do 
            // a "hard pause" so an event is generated, since we want to stay paused after scrubbing finishes.
            pause(processingUserGesture());
        } else {
            // Not at the end but we still want to pause playback so the media engine doesn't try to
            // continue playing during scrubbing. Pause without generating an event as we will 
            // unpause after scrubbing finishes.
            setPausedInternal(true);
        }
    }
}

void HTMLMediaElement::endScrubbing()
{
    LOG(Media, "HTMLMediaElement::endScrubbing - m_pausedInternal is %s", boolString(m_pausedInternal));

    if (m_pausedInternal)
        setPausedInternal(false);
}

// The spec says to fire periodic timeupdate events (those sent while playing) every
// "15 to 250ms", we choose the slowest frequency
static const double maxTimeupdateEventFrequency = 0.25;

static const double timeWithoutMouseMovementBeforeHidingControls = 3;

void HTMLMediaElement::startPlaybackProgressTimer()
{
    if (m_playbackProgressTimer.isActive())
        return;

    m_previousProgressTime = WTF::currentTime();
    m_previousProgress = 0;
    m_playbackProgressTimer.startRepeating(maxTimeupdateEventFrequency);
}

void HTMLMediaElement::playbackProgressTimerFired(Timer<HTMLMediaElement>*)
{
    ASSERT(m_player);
    if (!m_playbackRate)
        return;

    scheduleTimeupdateEvent(true);
    if (hasMediaControls())
        mediaControls()->playbackProgressed();
    // FIXME: deal with cue ranges here
}

void HTMLMediaElement::scheduleTimeupdateEvent(bool periodicEvent)
{
    double now = WTF::currentTime();
    double timedelta = now - m_lastTimeUpdateEventWallTime;

    // throttle the periodic events
    if (periodicEvent && timedelta < maxTimeupdateEventFrequency)
        return;

    // Some media engines make multiple "time changed" callbacks at the same time, but we only want one
    // event at a given time so filter here
    float movieTime = currentTime();
    if (movieTime != m_lastTimeUpdateEventMovieTime) {
        scheduleEvent(eventNames().timeupdateEvent);
        m_lastTimeUpdateEventWallTime = now;
        m_lastTimeUpdateEventMovieTime = movieTime;
    }
}

bool HTMLMediaElement::canPlay() const
{
    return paused() || ended() || m_readyState < HAVE_METADATA;
}

float HTMLMediaElement::percentLoaded() const
{
    if (!m_player)
        return 0;
    float duration = m_player->duration();

    if (!duration || isinf(duration))
        return 0;

    float buffered = 0;
    RefPtr<TimeRanges> timeRanges = m_player->buffered();
    for (unsigned i = 0; i < timeRanges->length(); ++i) {
        ExceptionCode ignoredException;
        float start = timeRanges->start(i, ignoredException);
        float end = timeRanges->end(i, ignoredException);
        buffered += end - start;
    }
    return buffered / duration;
}

bool HTMLMediaElement::havePotentialSourceChild()
{
    // Stash the current <source> node and next nodes so we can restore them after checking
    // to see there is another potential.
    HTMLSourceElement* currentSourceNode = m_currentSourceNode;
    Node* nextNode = m_nextChildNodeToConsider;

    KURL nextURL = selectNextSourceChild(0, DoNothing);

    m_currentSourceNode = currentSourceNode;
    m_nextChildNodeToConsider = nextNode;

    return nextURL.isValid();
}

KURL HTMLMediaElement::selectNextSourceChild(ContentType *contentType, InvalidSourceAction actionIfInvalid)
{
#if !LOG_DISABLED
    // Don't log if this was just called to find out if there are any valid <source> elements.
    bool shouldLog = actionIfInvalid != DoNothing;
    if (shouldLog)
        LOG(Media, "HTMLMediaElement::selectNextSourceChild(contentType : \"%s\")", contentType ? contentType->raw().utf8().data() : "");
#endif

    if (m_nextChildNodeToConsider == sourceChildEndOfListValue()) {
#if !LOG_DISABLED
        if (shouldLog)
            LOG(Media, "HTMLMediaElement::selectNextSourceChild -> 0x0000, \"\"");
#endif
        return KURL();
    }

    KURL mediaURL;
    Node* node;
    HTMLSourceElement* source = 0;
    bool lookingForStartNode = m_nextChildNodeToConsider;
    bool canUse = false;

    for (node = firstChild(); !canUse && node; node = node->nextSibling()) {
        if (lookingForStartNode && m_nextChildNodeToConsider != node)
            continue;
        lookingForStartNode = false;
        
        if (!node->hasTagName(sourceTag))
            continue;

        source = static_cast<HTMLSourceElement*>(node);

        // If candidate does not have a src attribute, or if its src attribute's value is the empty string ... jump down to the failed step below
        mediaURL = source->getNonEmptyURLAttribute(srcAttr);
#if !LOG_DISABLED
        if (shouldLog)
            LOG(Media, "HTMLMediaElement::selectNextSourceChild - 'src' is %s", urlForLogging(mediaURL).utf8().data());
#endif
        if (mediaURL.isEmpty())
            goto check_again;
        
        if (source->hasAttribute(mediaAttr)) {
            MediaQueryEvaluator screenEval("screen", document()->frame(), renderer() ? renderer()->style() : 0);
            RefPtr<MediaList> media = MediaList::createAllowingDescriptionSyntax(source->media());
#if !LOG_DISABLED
            if (shouldLog)
                LOG(Media, "HTMLMediaElement::selectNextSourceChild - 'media' is %s", source->media().utf8().data());
#endif
            if (!screenEval.eval(media.get())) 
                goto check_again;
        }

        if (source->hasAttribute(typeAttr)) {
#if !LOG_DISABLED
            if (shouldLog)
                LOG(Media, "HTMLMediaElement::selectNextSourceChild - 'type' is %s", source->type().utf8().data());
#endif
            if (!MediaPlayer::supportsType(ContentType(source->type())))
                goto check_again;
        }

        // Is it safe to load this url?
        if (!isSafeToLoadURL(mediaURL, actionIfInvalid) || !dispatchBeforeLoadEvent(mediaURL.string()))
            goto check_again;

        // Making it this far means the <source> looks reasonable.
        canUse = true;

check_again:
        if (!canUse && actionIfInvalid == Complain)
            source->scheduleErrorEvent();
    }

    if (canUse) {
        if (contentType)
            *contentType = ContentType(source->type());
        m_currentSourceNode = source;
        m_nextChildNodeToConsider = source->nextSibling();
        if (!m_nextChildNodeToConsider)
            m_nextChildNodeToConsider = sourceChildEndOfListValue();
    } else {
        m_currentSourceNode = 0;
        m_nextChildNodeToConsider = sourceChildEndOfListValue();
    }

#if !LOG_DISABLED
    if (shouldLog)
        LOG(Media, "HTMLMediaElement::selectNextSourceChild -> %p, %s", m_currentSourceNode, canUse ? urlForLogging(mediaURL.string()).utf8().data() : "");
#endif
    return canUse ? mediaURL : KURL();
}

void HTMLMediaElement::sourceWasAdded(HTMLSourceElement* source)
{
    LOG(Media, "HTMLMediaElement::sourceWasAdded(%p)", source);

#if !LOG_DISABLED
    if (source->hasTagName(sourceTag)) {
        KURL url = source->getNonEmptyURLAttribute(srcAttr);
        LOG(Media, "HTMLMediaElement::sourceWasAdded - 'src' is %s", urlForLogging(url).utf8().data());
    }
#endif
    
    // We should only consider a <source> element when there is not src attribute at all.
    if (hasAttribute(srcAttr))
        return;

    // 4.8.8 - If a source element is inserted as a child of a media element that has no src 
    // attribute and whose networkState has the value NETWORK_EMPTY, the user agent must invoke 
    // the media element's resource selection algorithm.
    if (networkState() == HTMLMediaElement::NETWORK_EMPTY) {
        scheduleLoad();
        return;
    }

    if (m_currentSourceNode && source == m_currentSourceNode->nextSibling()) {
        LOG(Media, "HTMLMediaElement::sourceWasAdded - <source> inserted immediately after current source");
        m_nextChildNodeToConsider = source;
        return;
    }

    if (m_nextChildNodeToConsider != sourceChildEndOfListValue())
        return;
    
    // 4.8.9.5, resource selection algorithm, source elements section:
    // 20 - Wait until the node after pointer is a node other than the end of the list. (This step might wait forever.)
    // 21 - Asynchronously await a stable state...
    // 22 - Set the element's delaying-the-load-event flag back to true (this delays the load event again, in case 
    // it hasn't been fired yet).
    setShouldDelayLoadEvent(true);

    // 23 - Set the networkState back to NETWORK_LOADING.
    m_networkState = NETWORK_LOADING;
    
    // 24 - Jump back to the find next candidate step above.
    m_nextChildNodeToConsider = source;
    scheduleNextSourceChild();
}

void HTMLMediaElement::sourceWillBeRemoved(HTMLSourceElement* source)
{
    LOG(Media, "HTMLMediaElement::sourceWillBeRemoved(%p)", source);

#if !LOG_DISABLED
    if (source->hasTagName(sourceTag)) {
        KURL url = source->getNonEmptyURLAttribute(srcAttr);
        LOG(Media, "HTMLMediaElement::sourceWillBeRemoved - 'src' is %s", urlForLogging(url).utf8().data());
    }
#endif

    if (source != m_currentSourceNode && source != m_nextChildNodeToConsider)
        return;

    if (source == m_nextChildNodeToConsider) {
        m_nextChildNodeToConsider = m_nextChildNodeToConsider->nextSibling();
        if (!m_nextChildNodeToConsider)
            m_nextChildNodeToConsider = sourceChildEndOfListValue();
        LOG(Media, "HTMLMediaElement::sourceRemoved - m_nextChildNodeToConsider set to %p", m_nextChildNodeToConsider);
    } else if (source == m_currentSourceNode) {
        // Clear the current source node pointer, but don't change the movie as the spec says:
        // 4.8.8 - Dynamically modifying a source element and its attribute when the element is already 
        // inserted in a video or audio element will have no effect.
        m_currentSourceNode = 0;
        LOG(Media, "HTMLMediaElement::sourceRemoved - m_currentSourceNode set to 0");
    }
}

void HTMLMediaElement::mediaPlayerTimeChanged(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerTimeChanged");

    beginProcessingMediaPlayerCallback();

    invalidateCachedTime();

    // 4.8.10.9 step 14 & 15.  Needed if no ReadyState change is associated with the seek.
    if (m_seeking && m_readyState >= HAVE_CURRENT_DATA)
        finishSeek();
    
    // Always call scheduleTimeupdateEvent when the media engine reports a time discontinuity, 
    // it will only queue a 'timeupdate' event if we haven't already posted one at the current
    // movie time.
    scheduleTimeupdateEvent(false);

    float now = currentTime();
    float dur = duration();
    if (!isnan(dur) && dur && now >= dur) {
        if (loop()) {
            ExceptionCode ignoredException;
            m_sentEndEvent = false;
            seek(0, ignoredException);
        } else {
            if (!m_sentEndEvent) {
                m_sentEndEvent = true;
                scheduleEvent(eventNames().endedEvent);
            }
        }
    }
    else
        m_sentEndEvent = false;

    updatePlayState();
    endProcessingMediaPlayerCallback();
}

void HTMLMediaElement::mediaPlayerVolumeChanged(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerVolumeChanged");

    beginProcessingMediaPlayerCallback();
    if (m_player) {
        float vol = m_player->volume();
        if (vol != m_volume) {
            m_volume = vol;
            updateVolume();
            scheduleEvent(eventNames().volumechangeEvent);
        }
    }
    endProcessingMediaPlayerCallback();
}

void HTMLMediaElement::mediaPlayerMuteChanged(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerMuteChanged");

    beginProcessingMediaPlayerCallback();
    if (m_player)
        setMuted(m_player->muted());
    endProcessingMediaPlayerCallback();
}

void HTMLMediaElement::mediaPlayerDurationChanged(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerDurationChanged");

    beginProcessingMediaPlayerCallback();
    scheduleEvent(eventNames().durationchangeEvent);
    if (renderer())
        renderer()->updateFromElement();
    endProcessingMediaPlayerCallback();
}

void HTMLMediaElement::mediaPlayerRateChanged(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerRateChanged");

    beginProcessingMediaPlayerCallback();

    invalidateCachedTime();

    // Stash the rate in case the one we tried to set isn't what the engine is
    // using (eg. it can't handle the rate we set)
    m_playbackRate = m_player->rate();
    invalidateCachedTime();
    endProcessingMediaPlayerCallback();
}

void HTMLMediaElement::mediaPlayerPlaybackStateChanged(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerPlaybackStateChanged");

    if (!m_player || m_pausedInternal)
        return;

    beginProcessingMediaPlayerCallback();
    if (m_player->paused())
        pauseInternal();
    else
        playInternal();
    endProcessingMediaPlayerCallback();
}

void HTMLMediaElement::mediaPlayerSawUnsupportedTracks(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerSawUnsupportedTracks");

    // The MediaPlayer came across content it cannot completely handle.
    // This is normally acceptable except when we are in a standalone
    // MediaDocument. If so, tell the document what has happened.
    if (ownerDocument()->isMediaDocument()) {
        MediaDocument* mediaDocument = static_cast<MediaDocument*>(ownerDocument());
        mediaDocument->mediaElementSawUnsupportedTracks();
    }
}

// MediaPlayerPresentation methods
void HTMLMediaElement::mediaPlayerRepaint(MediaPlayer*)
{
    beginProcessingMediaPlayerCallback();
    updateDisplayState();
    if (renderer())
        renderer()->repaint();
    endProcessingMediaPlayerCallback();
}

void HTMLMediaElement::mediaPlayerSizeChanged(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerSizeChanged");

    beginProcessingMediaPlayerCallback();
    if (renderer())
        renderer()->updateFromElement();
    endProcessingMediaPlayerCallback();
}

#if USE(ACCELERATED_COMPOSITING)
bool HTMLMediaElement::mediaPlayerRenderingCanBeAccelerated(MediaPlayer*)
{
    if (renderer() && renderer()->isVideo()) {
        ASSERT(renderer()->view());
        return renderer()->view()->compositor()->canAccelerateVideoRendering(toRenderVideo(renderer()));
    }
    return false;
}

void HTMLMediaElement::mediaPlayerRenderingModeChanged(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerRenderingModeChanged");

    // Kick off a fake recalcStyle that will update the compositing tree.
    setNeedsStyleRecalc(SyntheticStyleChange);
}
#endif

void HTMLMediaElement::mediaPlayerEngineUpdated(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerEngineUpdated");
    beginProcessingMediaPlayerCallback();
    if (renderer())
        renderer()->updateFromElement();
    endProcessingMediaPlayerCallback();
}

void HTMLMediaElement::mediaPlayerFirstVideoFrameAvailable(MediaPlayer*)
{
    LOG(Media, "HTMLMediaElement::mediaPlayerFirstVideoFrameAvailable");
    beginProcessingMediaPlayerCallback();
    if (displayMode() == PosterWaitingForVideo) {
        setDisplayMode(Video);
#if USE(ACCELERATED_COMPOSITING)
        mediaPlayerRenderingModeChanged(m_player.get());
#endif
    }
    endProcessingMediaPlayerCallback();
}

PassRefPtr<TimeRanges> HTMLMediaElement::buffered() const
{
    if (!m_player)
        return TimeRanges::create();
    return m_player->buffered();
}

PassRefPtr<TimeRanges> HTMLMediaElement::played()
{
    if (m_playing) {
        float time = currentTime();
        if (time > m_lastSeekTime)
            addPlayedRange(m_lastSeekTime, time);
    }

    if (!m_playedTimeRanges)
        m_playedTimeRanges = TimeRanges::create();

    return m_playedTimeRanges->copy();
}

PassRefPtr<TimeRanges> HTMLMediaElement::seekable() const
{
    // FIXME real ranges support
    if (!maxTimeSeekable())
        return TimeRanges::create();
    return TimeRanges::create(minTimeSeekable(), maxTimeSeekable());
}

bool HTMLMediaElement::potentiallyPlaying() const
{
    // "pausedToBuffer" means the media engine's rate is 0, but only because it had to stop playing
    // when it ran out of buffered data. A movie is this state is "potentially playing", modulo the
    // checks in couldPlayIfEnoughData().
    bool pausedToBuffer = m_readyStateMaximum >= HAVE_FUTURE_DATA && m_readyState < HAVE_FUTURE_DATA;
    return (pausedToBuffer || m_readyState >= HAVE_FUTURE_DATA) && couldPlayIfEnoughData();
}

bool HTMLMediaElement::couldPlayIfEnoughData() const
{
    return !paused() && !endedPlayback() && !stoppedDueToErrors() && !pausedForUserInteraction();
}

bool HTMLMediaElement::endedPlayback() const
{
    float dur = duration();
    if (!m_player || isnan(dur))
        return false;

    // 4.8.10.8 Playing the media resource

    // A media element is said to have ended playback when the element's 
    // readyState attribute is HAVE_METADATA or greater, 
    if (m_readyState < HAVE_METADATA)
        return false;

    // and the current playback position is the end of the media resource and the direction
    // of playback is forwards and the media element does not have a loop attribute specified,
    float now = currentTime();
    if (m_playbackRate > 0)
        return dur > 0 && now >= dur && !loop();

    // or the current playback position is the earliest possible position and the direction 
    // of playback is backwards
    if (m_playbackRate < 0)
        return now <= 0;

    return false;
}

bool HTMLMediaElement::stoppedDueToErrors() const
{
    if (m_readyState >= HAVE_METADATA && m_error) {
        RefPtr<TimeRanges> seekableRanges = seekable();
        if (!seekableRanges->contain(currentTime()))
            return true;
    }
    
    return false;
}

bool HTMLMediaElement::pausedForUserInteraction() const
{
//    return !paused() && m_readyState >= HAVE_FUTURE_DATA && [UA requires a decitions from the user]
    return false;
}

float HTMLMediaElement::minTimeSeekable() const
{
    return 0;
}

float HTMLMediaElement::maxTimeSeekable() const
{
    return m_player ? m_player->maxTimeSeekable() : 0;
}
    
void HTMLMediaElement::updateVolume()
{
    if (!m_player)
        return;

    // Avoid recursion when the player reports volume changes.
    if (!processingMediaPlayerCallback()) {
        Page* page = document()->page();
        float volumeMultiplier = page ? page->mediaVolume() : 1;
    
        m_player->setMuted(m_muted);
        m_player->setVolume(m_volume * volumeMultiplier);
    }

    if (hasMediaControls())
        mediaControls()->changedVolume();
}

void HTMLMediaElement::updatePlayState()
{
    if (!m_player)
        return;

    if (m_pausedInternal) {
        if (!m_player->paused())
            m_player->pause();
        refreshCachedTime();
        m_playbackProgressTimer.stop();
        if (hasMediaControls())
            mediaControls()->playbackStopped();
        return;
    }
    
    bool shouldBePlaying = potentiallyPlaying();
    bool playerPaused = m_player->paused();

    LOG(Media, "HTMLMediaElement::updatePlayState - shouldBePlaying = %s, playerPaused = %s", 
        boolString(shouldBePlaying), boolString(playerPaused));

    if (shouldBePlaying) {
        setDisplayMode(Video);
        invalidateCachedTime();

        if (playerPaused) {
            if (!m_isFullscreen && isVideo() && document() && document()->page() && document()->page()->chrome()->requiresFullscreenForVideoPlayback())
                enterFullscreen();

            // Set rate, muted before calling play in case they were set before the media engine was setup.
            // The media engine should just stash the rate and muted values since it isn't already playing.
            m_player->setRate(m_playbackRate);
            m_player->setMuted(m_muted);

            m_player->play();
        }

        if (hasMediaControls())
            mediaControls()->playbackStarted();
        startPlaybackProgressTimer();
        m_playing = true;

    } else { // Should not be playing right now
        if (!playerPaused)
            m_player->pause();
        refreshCachedTime();

        m_playbackProgressTimer.stop();
        m_playing = false;
        float time = currentTime();
        if (time > m_lastSeekTime)
            addPlayedRange(m_lastSeekTime, time);

        if (couldPlayIfEnoughData())
            prepareToPlay();

        if (hasMediaControls())
            mediaControls()->playbackStopped();
    }
    
    if (renderer())
        renderer()->updateFromElement();
}
    
void HTMLMediaElement::setPausedInternal(bool b)
{
    m_pausedInternal = b;
    updatePlayState();
}

void HTMLMediaElement::stopPeriodicTimers()
{
    m_progressEventTimer.stop();
    m_playbackProgressTimer.stop();
}

void HTMLMediaElement::userCancelledLoad()
{
    LOG(Media, "HTMLMediaElement::userCancelledLoad");

    if (m_networkState == NETWORK_EMPTY || m_completelyLoaded)
        return;

    // If the media data fetching process is aborted by the user:

    // 1 - The user agent should cancel the fetching process.
#if !ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    m_player.clear();
#endif
    stopPeriodicTimers();
    m_loadTimer.stop();
    m_loadState = WaitingForSource;

    // 2 - Set the error attribute to a new MediaError object whose code attribute is set to MEDIA_ERR_ABORTED.
    m_error = MediaError::create(MediaError::MEDIA_ERR_ABORTED);

    // 3 - Queue a task to fire a simple event named error at the media element.
    scheduleEvent(eventNames().abortEvent);

    // 4 - If the media element's readyState attribute has a value equal to HAVE_NOTHING, set the 
    // element's networkState attribute to the NETWORK_EMPTY value and queue a task to fire a 
    // simple event named emptied at the element. Otherwise, set the element's networkState 
    // attribute to the NETWORK_IDLE value.
    if (m_readyState == HAVE_NOTHING) {
        m_networkState = NETWORK_EMPTY;
        scheduleEvent(eventNames().emptiedEvent);
    }
    else
        m_networkState = NETWORK_IDLE;

    // 5 - Set the element's delaying-the-load-event flag to false. This stops delaying the load event.
    setShouldDelayLoadEvent(false);

    // 6 - Abort the overall resource selection algorithm.
    m_currentSourceNode = 0;

    // Reset m_readyState since m_player is gone.
    m_readyState = HAVE_NOTHING;
}

bool HTMLMediaElement::canSuspend() const
{
    return true; 
}

void HTMLMediaElement::stop()
{
    LOG(Media, "HTMLMediaElement::stop");
    if (m_isFullscreen)
        exitFullscreen();
    
    m_inActiveDocument = false;
    userCancelledLoad();
    
    // Stop the playback without generating events
    setPausedInternal(true);
    
    if (renderer())
        renderer()->updateFromElement();
    
    stopPeriodicTimers();
    cancelPendingEventsAndCallbacks();
}

void HTMLMediaElement::suspend(ReasonForSuspension why)
{
    LOG(Media, "HTMLMediaElement::suspend");
    
    switch (why)
    {
        case DocumentWillBecomeInactive:
            stop();
            break;
        case JavaScriptDebuggerPaused:
        case WillShowDialog:
            // Do nothing, we don't pause media playback in these cases.
            break;
    }
}

void HTMLMediaElement::resume()
{
    LOG(Media, "HTMLMediaElement::resume");

    m_inActiveDocument = true;
    setPausedInternal(false);

    if (m_error && m_error->code() == MediaError::MEDIA_ERR_ABORTED) {
        // Restart the load if it was aborted in the middle by moving the document to the page cache.
        // m_error is only left at MEDIA_ERR_ABORTED when the document becomes inactive (it is set to
        //  MEDIA_ERR_ABORTED while the abortEvent is being sent, but cleared immediately afterwards).
        // This behavior is not specified but it seems like a sensible thing to do.
        ExceptionCode ec;
        load(processingUserGesture(), ec);
    }

    if (renderer())
        renderer()->updateFromElement();
}

bool HTMLMediaElement::hasPendingActivity() const
{
    // Return true when we have pending events so we can't fire events after the JS 
    // object gets collected.
    bool pending = m_pendingEvents.size();
    LOG(Media, "HTMLMediaElement::hasPendingActivity -> %s", boolString(pending));
    return pending;
}

void HTMLMediaElement::mediaVolumeDidChange()
{
    LOG(Media, "HTMLMediaElement::mediaVolumeDidChange");
    updateVolume();
}

void HTMLMediaElement::defaultEventHandler(Event* event)
{
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    RenderObject* r = renderer();
    if (!r || !r->isWidget())
        return;

    Widget* widget = toRenderWidget(r)->widget();
    if (widget)
        widget->handleEvent(event);
#else
    HTMLElement::defaultEventHandler(event);
#endif
}

bool HTMLMediaElement::processingUserGesture() const
{
    Frame* frame = document()->frame();
    FrameLoader* loader = frame ? frame->loader() : 0;

    // return 'true' for safety if we don't know the answer 
    return loader ? loader->isProcessingUserGesture() : true;
}

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)

void HTMLMediaElement::ensureMediaPlayer()
{
    if (!m_player)
        m_player = MediaPlayer::create(this);
}

void HTMLMediaElement::deliverNotification(MediaPlayerProxyNotificationType notification)
{
    if (notification == MediaPlayerNotificationPlayPauseButtonPressed) {
        togglePlayState();
        return;
    }

    if (m_player)
        m_player->deliverNotification(notification);
}

void HTMLMediaElement::setMediaPlayerProxy(WebMediaPlayerProxy* proxy)
{
    ensureMediaPlayer();
    m_player->setMediaPlayerProxy(proxy);
}

void HTMLMediaElement::getPluginProxyParams(KURL& url, Vector<String>& names, Vector<String>& values)
{
    Frame* frame = document()->frame();
    FrameLoader* loader = frame ? frame->loader() : 0;

    if (isVideo()) {
        KURL posterURL = getNonEmptyURLAttribute(posterAttr);
        if (!posterURL.isEmpty() && loader && loader->willLoadMediaElementURL(posterURL)) {
            names.append("_media_element_poster_");
            values.append(posterURL.string());
        }
    }

    if (controls()) {
        names.append("_media_element_controls_");
        values.append("true");
    }

    url = src();
    if (!isSafeToLoadURL(url, Complain))
        url = selectNextSourceChild(0, DoNothing);

    m_currentSrc = url.string();
    if (url.isValid() && loader && loader->willLoadMediaElementURL(url)) {
        names.append("_media_element_src_");
        values.append(m_currentSrc);
    }
}

void HTMLMediaElement::finishParsingChildren()
{
    HTMLElement::finishParsingChildren();
    document()->updateStyleIfNeeded();
    createMediaPlayerProxy();
}

void HTMLMediaElement::createMediaPlayerProxy()
{
    ensureMediaPlayer();

    if (m_proxyWidget || (inDocument() && !m_needWidgetUpdate))
        return;

    Frame* frame = document()->frame();
    FrameLoader* loader = frame ? frame->loader() : 0;
    if (!loader)
        return;

    LOG(Media, "HTMLMediaElement::createMediaPlayerProxy");

    KURL url;
    Vector<String> paramNames;
    Vector<String> paramValues;

    getPluginProxyParams(url, paramNames, paramValues);
    
    // Hang onto the proxy widget so it won't be destroyed if the plug-in is set to
    // display:none
    m_proxyWidget = loader->subframeLoader()->loadMediaPlayerProxyPlugin(this, url, paramNames, paramValues);
    if (m_proxyWidget)
        m_needWidgetUpdate = false;
}

void HTMLMediaElement::updateWidget(PluginCreationOption)
{
    mediaElement->setNeedWidgetUpdate(false);

    Vector<String> paramNames;
    Vector<String> paramValues;
    KURL kurl;
    
    mediaElement->getPluginProxyParams(kurl, paramNames, paramValues);
    SubframeLoader* loader = document()->frame()->loader()->subframeLoader();
    loader->loadMediaPlayerProxyPlugin(mediaElement, kurl, paramNames, paramValues);
}

#endif // ENABLE(PLUGIN_PROXY_FOR_VIDEO)

bool HTMLMediaElement::isFullscreen() const
{
    if (m_isFullscreen)
        return true;
    
#if ENABLE(FULLSCREEN_API)
    if (document()->webkitIsFullScreen() && document()->webkitCurrentFullScreenElement() == this)
        return true;
#endif
    
    return false;
}

void HTMLMediaElement::enterFullscreen()
{
    LOG(Media, "HTMLMediaElement::enterFullscreen");

#if ENABLE(FULLSCREEN_API)
    if (document() && document()->settings() && document()->settings()->fullScreenEnabled()) {
        document()->requestFullScreenForElement(this, 0, Document::ExemptIFrameAllowFulScreenRequirement);
        return;
    }
#endif
    ASSERT(!m_isFullscreen);
    m_isFullscreen = true;
    if (hasMediaControls())
        mediaControls()->enteredFullscreen();
    if (document() && document()->page()) {
        document()->page()->chrome()->client()->enterFullscreenForNode(this);
        scheduleEvent(eventNames().webkitbeginfullscreenEvent);
    }
}

void HTMLMediaElement::exitFullscreen()
{
    LOG(Media, "HTMLMediaElement::exitFullscreen");

#if ENABLE(FULLSCREEN_API)
    if (document() && document()->settings() && document()->settings()->fullScreenEnabled()) {
        if (document()->webkitIsFullScreen() && document()->webkitCurrentFullScreenElement() == this)
            document()->webkitCancelFullScreen();
        return;
    }
#endif
    ASSERT(m_isFullscreen);
    m_isFullscreen = false;
    if (hasMediaControls())
        mediaControls()->exitedFullscreen();
    if (document() && document()->page()) {
        if (document()->page()->chrome()->requiresFullscreenForVideoPlayback())
            pauseInternal();
        document()->page()->chrome()->client()->exitFullscreenForNode(this);
        scheduleEvent(eventNames().webkitendfullscreenEvent);
    }
}

PlatformMedia HTMLMediaElement::platformMedia() const
{
    return m_player ? m_player->platformMedia() : NoPlatformMedia;
}

#if USE(ACCELERATED_COMPOSITING)
PlatformLayer* HTMLMediaElement::platformLayer() const
{
    return m_player ? m_player->platformLayer() : 0;
}
#endif

bool HTMLMediaElement::hasClosedCaptions() const
{
    return m_player && m_player->hasClosedCaptions();
}

bool HTMLMediaElement::closedCaptionsVisible() const
{
    return m_closedCaptionsVisible;
}

void HTMLMediaElement::setClosedCaptionsVisible(bool closedCaptionVisible)
{
    LOG(Media, "HTMLMediaElement::setClosedCaptionsVisible(%s)", boolString(closedCaptionVisible));

    if (!m_player ||!hasClosedCaptions())
        return;

    m_closedCaptionsVisible = closedCaptionVisible;
    m_player->setClosedCaptionsVisible(closedCaptionVisible);
    if (hasMediaControls())
        mediaControls()->changedClosedCaptionsVisibility();
}

void HTMLMediaElement::setWebkitClosedCaptionsVisible(bool visible)
{
    setClosedCaptionsVisible(visible);
}

bool HTMLMediaElement::webkitClosedCaptionsVisible() const
{
    return closedCaptionsVisible();
}


bool HTMLMediaElement::webkitHasClosedCaptions() const
{
    return hasClosedCaptions();
}

#if ENABLE(MEDIA_STATISTICS)
unsigned HTMLMediaElement::webkitAudioDecodedByteCount() const
{
    if (!m_player)
        return 0;
    return m_player->audioDecodedByteCount();
}

unsigned HTMLMediaElement::webkitVideoDecodedByteCount() const
{
    if (!m_player)
        return 0;
    return m_player->videoDecodedByteCount();
}
#endif

void HTMLMediaElement::mediaCanStart()
{
    LOG(Media, "HTMLMediaElement::mediaCanStart");

    ASSERT(m_isWaitingUntilMediaCanStart);
    m_isWaitingUntilMediaCanStart = false;
    loadInternal();
}

bool HTMLMediaElement::isURLAttribute(Attribute* attribute) const
{
    return attribute->name() == srcAttr;
}

void HTMLMediaElement::setShouldDelayLoadEvent(bool shouldDelay)
{
    if (m_shouldDelayLoadEvent == shouldDelay)
        return;

    LOG(Media, "HTMLMediaElement::setShouldDelayLoadEvent(%s)", boolString(shouldDelay));

    m_shouldDelayLoadEvent = shouldDelay;
    if (shouldDelay)
        document()->incrementLoadEventDelayCount();
    else
        document()->decrementLoadEventDelayCount();
}
    

void HTMLMediaElement::getSitesInMediaCache(Vector<String>& sites)
{
    MediaPlayer::getSitesInMediaCache(sites);
}

void HTMLMediaElement::clearMediaCache()
{
    MediaPlayer::clearMediaCache();
}

void HTMLMediaElement::clearMediaCacheForSite(const String& site)
{
    MediaPlayer::clearMediaCacheForSite(site);
}

void HTMLMediaElement::privateBrowsingStateDidChange()
{
    if (!m_player)
        return;

    Settings* settings = document()->settings();
    bool privateMode = !settings || settings->privateBrowsingEnabled();
    LOG(Media, "HTMLMediaElement::privateBrowsingStateDidChange(%s)", boolString(privateMode));
    m_player->setPrivateBrowsingMode(privateMode);
}

MediaControls* HTMLMediaElement::mediaControls()
{
    return toMediaControls(shadowRoot()->firstChild());
}

bool HTMLMediaElement::hasMediaControls()
{
    if (!shadowRoot())
        return false;

    Node* node = shadowRoot()->firstChild();
    return node && node->isMediaControls();
}

void HTMLMediaElement::ensureMediaControls()
{
    if (hasMediaControls())
        return;

    ExceptionCode ec;
    ensureShadowRoot()->appendChild(MediaControls::create(this), ec);
}

void* HTMLMediaElement::preDispatchEventHandler(Event* event)
{
    if (event && event->type() == eventNames().webkitfullscreenchangeEvent) {
        if (controls()) {
            if (!hasMediaControls()) {
                ensureMediaControls();
                mediaControls()->reset();
            }
            mediaControls()->show();
        } else if (hasMediaControls())
            mediaControls()->hide();
    }
    return 0;
}


}

#endif
