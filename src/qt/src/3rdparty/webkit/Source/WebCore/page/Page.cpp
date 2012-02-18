/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All Rights Reserved.
 * Copyright (C) 2008 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "Page.h"

#include "BackForwardController.h"
#include "BackForwardList.h"
#include "Base64.h"
#include "CSSStyleSelector.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "ContextMenuClient.h"
#include "ContextMenuController.h"
#include "DOMWindow.h"
#include "DeviceMotionController.h"
#include "DeviceOrientationController.h"
#include "DocumentMarkerController.h"
#include "DragController.h"
#include "EditorClient.h"
#include "Event.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "FileSystem.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "HTMLElement.h"
#include "HistoryItem.h"
#include "InspectorController.h"
#include "InspectorInstrumentation.h"
#include "Logging.h"
#include "MediaCanStartListener.h"
#include "MediaStreamClient.h"
#include "MediaStreamController.h"
#include "Navigator.h"
#include "NetworkStateNotifier.h"
#include "PageGroup.h"
#include "PluginData.h"
#include "PluginHalter.h"
#include "PluginView.h"
#include "PluginViewBase.h"
#include "ProgressTracker.h"
#include "RenderTheme.h"
#include "RenderWidget.h"
#include "RuntimeEnabledFeatures.h"
#include "SelectionController.h"
#include "Settings.h"
#include "SharedBuffer.h"
#include "SpeechInput.h"
#include "SpeechInputClient.h"
#include "TextResourceDecoder.h"
#include "Widget.h"
#include <wtf/HashMap.h>
#include <wtf/RefCountedLeakCounter.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringHash.h>

#if ENABLE(ACCELERATED_2D_CANVAS)
#include "SharedGraphicsContext3D.h"
#endif

#if ENABLE(DOM_STORAGE)
#include "StorageArea.h"
#include "StorageNamespace.h"
#endif

#if ENABLE(CLIENT_BASED_GEOLOCATION)
#include "GeolocationController.h"
#endif

namespace WebCore {

static HashSet<Page*>* allPages;

#ifndef NDEBUG
static WTF::RefCountedLeakCounter pageCounter("Page");
#endif

static void networkStateChanged()
{
    Vector<RefPtr<Frame> > frames;
    
    // Get all the frames of all the pages in all the page groups
    HashSet<Page*>::iterator end = allPages->end();
    for (HashSet<Page*>::iterator it = allPages->begin(); it != end; ++it) {
        for (Frame* frame = (*it)->mainFrame(); frame; frame = frame->tree()->traverseNext())
            frames.append(frame);
        InspectorInstrumentation::networkStateChanged(*it);
    }

    AtomicString eventName = networkStateNotifier().onLine() ? eventNames().onlineEvent : eventNames().offlineEvent;
    for (unsigned i = 0; i < frames.size(); i++)
        frames[i]->document()->dispatchWindowEvent(Event::create(eventName, false, false));
}

Page::Page(PageClients& pageClients)
    : m_chrome(adoptPtr(new Chrome(this, pageClients.chromeClient)))
    , m_dragCaretController(adoptPtr(new SelectionController(0, true)))
#if ENABLE(DRAG_SUPPORT)
    , m_dragController(adoptPtr(new DragController(this, pageClients.dragClient)))
#endif
    , m_focusController(adoptPtr(new FocusController(this)))
#if ENABLE(CONTEXT_MENUS)
    , m_contextMenuController(adoptPtr(new ContextMenuController(this, pageClients.contextMenuClient)))
#endif
#if ENABLE(INSPECTOR)
    , m_inspectorController(adoptPtr(new InspectorController(this, pageClients.inspectorClient)))
#endif
#if ENABLE(CLIENT_BASED_GEOLOCATION)
    , m_geolocationController(adoptPtr(new GeolocationController(this, pageClients.geolocationClient)))
#endif
#if ENABLE(DEVICE_ORIENTATION)
    , m_deviceMotionController(RuntimeEnabledFeatures::deviceMotionEnabled() ? adoptPtr(new DeviceMotionController(pageClients.deviceMotionClient)) : nullptr)
    , m_deviceOrientationController(RuntimeEnabledFeatures::deviceOrientationEnabled() ? adoptPtr(new DeviceOrientationController(this, pageClients.deviceOrientationClient)) : nullptr)
#endif
#if ENABLE(MEDIA_STREAM)
    , m_mediaStreamController(RuntimeEnabledFeatures::mediaStreamEnabled() ? adoptPtr(new MediaStreamController(pageClients.mediaStreamClient)) : PassOwnPtr<MediaStreamController>())
#endif
#if ENABLE(INPUT_SPEECH)
    , m_speechInputClient(pageClients.speechInputClient)
#endif
    , m_settings(adoptPtr(new Settings(this)))
    , m_progress(adoptPtr(new ProgressTracker))
    , m_backForwardController(adoptPtr(new BackForwardController(this, pageClients.backForwardClient)))
    , m_theme(RenderTheme::themeForPage(this))
    , m_editorClient(pageClients.editorClient)
    , m_frameCount(0)
    , m_openedByDOM(false)
    , m_tabKeyCyclesThroughElements(true)
    , m_defersLoading(false)
    , m_inLowQualityInterpolationMode(false)
    , m_cookieEnabled(true)
    , m_areMemoryCacheClientCallsEnabled(true)
    , m_mediaVolume(1)
    , m_javaScriptURLsAreAllowed(true)
    , m_didLoadUserStyleSheet(false)
    , m_userStyleSheetModificationTime(0)
    , m_group(0)
    , m_debugger(0)
    , m_customHTMLTokenizerTimeDelay(-1)
    , m_customHTMLTokenizerChunkSize(-1)
    , m_canStartMedia(true)
    , m_viewMode(ViewModeWindowed)
    , m_minimumTimerInterval(Settings::defaultMinDOMTimerInterval())
    , m_isEditable(false)
{
    if (!allPages) {
        allPages = new HashSet<Page*>;
        
        networkStateNotifier().setNetworkStateChangedFunction(networkStateChanged);
    }

    ASSERT(!allPages->contains(this));
    allPages->add(this);

    if (pageClients.pluginHalterClient) {
        m_pluginHalter = adoptPtr(new PluginHalter(pageClients.pluginHalterClient.release()));
        m_pluginHalter->setPluginAllowedRunTime(m_settings->pluginAllowedRunTime());
    }

#ifndef NDEBUG
    pageCounter.increment();
#endif
}

Page::~Page()
{
    m_mainFrame->setView(0);
    setGroupName(String());
    allPages->remove(this);
    
    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext())
        frame->pageDestroyed();

    if (m_scrollableAreaSet) {
        ScrollableAreaSet::const_iterator end = m_scrollableAreaSet->end(); 
        for (ScrollableAreaSet::const_iterator it = m_scrollableAreaSet->begin(); it != end; ++it)
            (*it)->disconnectFromPage();
    }

    m_editorClient->pageDestroyed();

    InspectorInstrumentation::inspectedPageDestroyed(this);

    backForward()->close();

#ifndef NDEBUG
    pageCounter.decrement();

    // Cancel keepAlive timers, to ensure we release all Frames before exiting.
    // It's safe to do this because we prohibit closing a Page while JavaScript
    // is executing.
    Frame::cancelAllKeepAlive();
#endif
}

struct ViewModeInfo {
    const char* name;
    Page::ViewMode type;
};
static const int viewModeMapSize = 5;
static ViewModeInfo viewModeMap[viewModeMapSize] = {
    {"windowed", Page::ViewModeWindowed},
    {"floating", Page::ViewModeFloating},
    {"fullscreen", Page::ViewModeFullscreen},
    {"maximized", Page::ViewModeMaximized},
    {"minimized", Page::ViewModeMinimized}
};

Page::ViewMode Page::stringToViewMode(const String& text)
{
    for (int i = 0; i < viewModeMapSize; ++i) {
        if (text == viewModeMap[i].name)
            return viewModeMap[i].type;
    }
    return Page::ViewModeInvalid;
}

void Page::setViewMode(ViewMode viewMode)
{
    if (viewMode == m_viewMode || viewMode == ViewModeInvalid)
        return;

    m_viewMode = viewMode;

    if (!m_mainFrame)
        return;

    if (m_mainFrame->view())
        m_mainFrame->view()->forceLayout();

    if (m_mainFrame->document())
        m_mainFrame->document()->styleSelectorChanged(RecalcStyleImmediately);
}

void Page::setMainFrame(PassRefPtr<Frame> mainFrame)
{
    ASSERT(!m_mainFrame); // Should only be called during initialization
    m_mainFrame = mainFrame;
}

bool Page::openedByDOM() const
{
    return m_openedByDOM;
}

void Page::setOpenedByDOM()
{
    m_openedByDOM = true;
}

BackForwardList* Page::backForwardList() const
{
    return m_backForwardController->client();
}

bool Page::goBack()
{
    HistoryItem* item = backForward()->backItem();
    
    if (item) {
        goToItem(item, FrameLoadTypeBack);
        return true;
    }
    return false;
}

bool Page::goForward()
{
    HistoryItem* item = backForward()->forwardItem();
    
    if (item) {
        goToItem(item, FrameLoadTypeForward);
        return true;
    }
    return false;
}

bool Page::canGoBackOrForward(int distance) const
{
    if (distance == 0)
        return true;
    if (distance > 0 && distance <= backForward()->forwardCount())
        return true;
    if (distance < 0 && -distance <= backForward()->backCount())
        return true;
    return false;
}

void Page::goBackOrForward(int distance)
{
    if (distance == 0)
        return;

    HistoryItem* item = backForward()->itemAtIndex(distance);
    if (!item) {
        if (distance > 0) {
            if (int forwardCount = backForward()->forwardCount()) 
                item = backForward()->itemAtIndex(forwardCount);
        } else {
            if (int backCount = backForward()->backCount())
                item = backForward()->itemAtIndex(-backCount);
        }
    }

    ASSERT(item);
    if (!item)
        return;

    goToItem(item, FrameLoadTypeIndexedBackForward);
}

void Page::goToItem(HistoryItem* item, FrameLoadType type)
{
    if (defersLoading())
        return;

    // stopAllLoaders may end up running onload handlers, which could cause further history traversals that may lead to the passed in HistoryItem
    // being deref()-ed. Make sure we can still use it with HistoryController::goToItem later.
    RefPtr<HistoryItem> protector(item);

    if (m_mainFrame->loader()->history()->shouldStopLoadingForHistoryItem(item))
        m_mainFrame->loader()->stopAllLoaders();

    m_mainFrame->loader()->history()->goToItem(item, type);
}

int Page::getHistoryLength()
{
    return backForward()->backCount() + 1 + backForward()->forwardCount();
}

void Page::setGroupName(const String& name)
{
    if (m_group && !m_group->name().isEmpty()) {
        ASSERT(m_group != m_singlePageGroup.get());
        ASSERT(!m_singlePageGroup);
        m_group->removePage(this);
    }

    if (name.isEmpty())
        m_group = m_singlePageGroup.get();
    else {
        m_singlePageGroup.clear();
        m_group = PageGroup::pageGroup(name);
        m_group->addPage(this);
    }
}

const String& Page::groupName() const
{
    DEFINE_STATIC_LOCAL(String, nullString, ());
    return m_group ? m_group->name() : nullString;
}

void Page::initGroup()
{
    ASSERT(!m_singlePageGroup);
    ASSERT(!m_group);
    m_singlePageGroup = adoptPtr(new PageGroup(this));
    m_group = m_singlePageGroup.get();
}

void Page::scheduleForcedStyleRecalcForAllPages()
{
    if (!allPages)
        return;
    HashSet<Page*>::iterator end = allPages->end();
    for (HashSet<Page*>::iterator it = allPages->begin(); it != end; ++it)
        for (Frame* frame = (*it)->mainFrame(); frame; frame = frame->tree()->traverseNext())
            frame->document()->scheduleForcedStyleRecalc();
}

void Page::setNeedsRecalcStyleInAllFrames()
{
    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext())
        frame->document()->styleSelectorChanged(DeferRecalcStyle);
}

void Page::updateViewportArguments()
{
    if (!mainFrame() || !mainFrame()->document() || mainFrame()->document()->viewportArguments() == m_viewportArguments)
        return;

    m_viewportArguments = mainFrame()->document()->viewportArguments();
    chrome()->dispatchViewportDataDidChange(m_viewportArguments);
}

void Page::refreshPlugins(bool reload)
{
    if (!allPages)
        return;

    PluginData::refresh();

    Vector<RefPtr<Frame> > framesNeedingReload;

    HashSet<Page*>::iterator end = allPages->end();
    for (HashSet<Page*>::iterator it = allPages->begin(); it != end; ++it) {
        Page* page = *it;
        
        // Clear out the page's plug-in data.
        if (page->m_pluginData)
            page->m_pluginData = 0;

        if (!reload)
            continue;
        
        for (Frame* frame = (*it)->mainFrame(); frame; frame = frame->tree()->traverseNext()) {
            if (frame->loader()->subframeLoader()->containsPlugins())
                framesNeedingReload.append(frame);
        }
    }

    for (size_t i = 0; i < framesNeedingReload.size(); ++i)
        framesNeedingReload[i]->loader()->reload();
}

PluginData* Page::pluginData() const
{
    if (!mainFrame()->loader()->subframeLoader()->allowPlugins(NotAboutToInstantiatePlugin))
        return 0;
    if (!m_pluginData)
        m_pluginData = PluginData::create(this);
    return m_pluginData.get();
}

inline MediaCanStartListener* Page::takeAnyMediaCanStartListener()
{
    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        if (MediaCanStartListener* listener = frame->document()->takeAnyMediaCanStartListener())
            return listener;
    }
    return 0;
}

void Page::setCanStartMedia(bool canStartMedia)
{
    if (m_canStartMedia == canStartMedia)
        return;

    m_canStartMedia = canStartMedia;

    while (m_canStartMedia) {
        MediaCanStartListener* listener = takeAnyMediaCanStartListener();
        if (!listener)
            break;
        listener->mediaCanStart();
    }
}

static Frame* incrementFrame(Frame* curr, bool forward, bool wrapFlag)
{
    return forward
        ? curr->tree()->traverseNextWithWrap(wrapFlag)
        : curr->tree()->traversePreviousWithWrap(wrapFlag);
}

bool Page::findString(const String& target, TextCaseSensitivity caseSensitivity, FindDirection direction, bool shouldWrap)
{
    return findString(target, (caseSensitivity == TextCaseInsensitive ? CaseInsensitive : 0) | (direction == FindDirectionBackward ? Backwards : 0) | (shouldWrap ? WrapAround : 0));
}

bool Page::findString(const String& target, FindOptions options)
{
    if (target.isEmpty() || !mainFrame())
        return false;

    bool shouldWrap = options & WrapAround;
    Frame* frame = focusController()->focusedOrMainFrame();
    Frame* startFrame = frame;
    do {
        if (frame->editor()->findString(target, (options & ~WrapAround) | StartInSelection)) {
            if (frame != startFrame)
                startFrame->selection()->clear();
            focusController()->setFocusedFrame(frame);
            return true;
        }
        frame = incrementFrame(frame, !(options & Backwards), shouldWrap);
    } while (frame && frame != startFrame);

    // Search contents of startFrame, on the other side of the selection that we did earlier.
    // We cheat a bit and just research with wrap on
    if (shouldWrap && !startFrame->selection()->isNone()) {
        bool found = startFrame->editor()->findString(target, options | WrapAround | StartInSelection);
        focusController()->setFocusedFrame(frame);
        return found;
    }

    return false;
}

unsigned int Page::markAllMatchesForText(const String& target, TextCaseSensitivity caseSensitivity, bool shouldHighlight, unsigned limit)
{
    return markAllMatchesForText(target, caseSensitivity == TextCaseInsensitive ? CaseInsensitive : 0, shouldHighlight, limit);
}

unsigned int Page::markAllMatchesForText(const String& target, FindOptions options, bool shouldHighlight, unsigned limit)
{
    if (target.isEmpty() || !mainFrame())
        return 0;

    unsigned matches = 0;

    Frame* frame = mainFrame();
    do {
        frame->editor()->setMarkedTextMatchesAreHighlighted(shouldHighlight);
        matches += frame->editor()->countMatchesForText(target, options, limit ? (limit - matches) : 0, true);
        frame = incrementFrame(frame, true, false);
    } while (frame);

    return matches;
}

void Page::unmarkAllTextMatches()
{
    if (!mainFrame())
        return;

    Frame* frame = mainFrame();
    do {
        frame->document()->markers()->removeMarkers(DocumentMarker::TextMatch);
        frame = incrementFrame(frame, true, false);
    } while (frame);
}

const VisibleSelection& Page::selection() const
{
    return focusController()->focusedOrMainFrame()->selection()->selection();
}

void Page::setDefersLoading(bool defers)
{
    if (!m_settings->loadDeferringEnabled())
        return;

    if (defers == m_defersLoading)
        return;

    m_defersLoading = defers;
    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext())
        frame->loader()->setDefersLoading(defers);
}

void Page::clearUndoRedoOperations()
{
    m_editorClient->clearUndoRedoOperations();
}

bool Page::inLowQualityImageInterpolationMode() const
{
    return m_inLowQualityInterpolationMode;
}

void Page::setInLowQualityImageInterpolationMode(bool mode)
{
    m_inLowQualityInterpolationMode = mode;
}

void Page::setMediaVolume(float volume)
{
    if (volume < 0 || volume > 1)
        return;

    if (m_mediaVolume == volume)
        return;

    m_mediaVolume = volume;
    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        frame->document()->mediaVolumeDidChange();
    }
}

void Page::didMoveOnscreen()
{
    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        if (frame->view())
            frame->view()->didMoveOnscreen();
    }
}

void Page::willMoveOffscreen()
{
    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        if (frame->view())
            frame->view()->willMoveOffscreen();
    }
}

void Page::userStyleSheetLocationChanged()
{
    // FIXME: Eventually we will move to a model of just being handed the sheet
    // text instead of loading the URL ourselves.
    KURL url = m_settings->userStyleSheetLocation();
    if (url.isLocalFile())
        m_userStyleSheetPath = url.fileSystemPath();
    else
        m_userStyleSheetPath = String();

    m_didLoadUserStyleSheet = false;
    m_userStyleSheet = String();
    m_userStyleSheetModificationTime = 0;

    // Data URLs with base64-encoded UTF-8 style sheets are common. We can process them
    // synchronously and avoid using a loader. 
    if (url.protocolIsData() && url.string().startsWith("data:text/css;charset=utf-8;base64,")) {
        m_didLoadUserStyleSheet = true;

        Vector<char> styleSheetAsUTF8;
        if (base64Decode(decodeURLEscapeSequences(url.string().substring(35)), styleSheetAsUTF8, IgnoreWhitespace))
            m_userStyleSheet = String::fromUTF8(styleSheetAsUTF8.data(), styleSheetAsUTF8.size());
    }

    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        if (frame->document())
            frame->document()->updatePageUserSheet();
    }
}

const String& Page::userStyleSheet() const
{
    if (m_userStyleSheetPath.isEmpty())
        return m_userStyleSheet;

    time_t modTime;
    if (!getFileModificationTime(m_userStyleSheetPath, modTime)) {
        // The stylesheet either doesn't exist, was just deleted, or is
        // otherwise unreadable. If we've read the stylesheet before, we should
        // throw away that data now as it no longer represents what's on disk.
        m_userStyleSheet = String();
        return m_userStyleSheet;
    }

    // If the stylesheet hasn't changed since the last time we read it, we can
    // just return the old data.
    if (m_didLoadUserStyleSheet && modTime <= m_userStyleSheetModificationTime)
        return m_userStyleSheet;

    m_didLoadUserStyleSheet = true;
    m_userStyleSheet = String();
    m_userStyleSheetModificationTime = modTime;

    // FIXME: It would be better to load this asynchronously to avoid blocking
    // the process, but we will first need to create an asynchronous loading
    // mechanism that is not tied to a particular Frame. We will also have to
    // determine what our behavior should be before the stylesheet is loaded
    // and what should happen when it finishes loading, especially with respect
    // to when the load event fires, when Document::close is called, and when
    // layout/paint are allowed to happen.
    RefPtr<SharedBuffer> data = SharedBuffer::createWithContentsOfFile(m_userStyleSheetPath);
    if (!data)
        return m_userStyleSheet;

    RefPtr<TextResourceDecoder> decoder = TextResourceDecoder::create("text/css");
    m_userStyleSheet = decoder->decode(data->data(), data->size());
    m_userStyleSheet += decoder->flush();

    return m_userStyleSheet;
}

void Page::removeAllVisitedLinks()
{
    if (!allPages)
        return;
    HashSet<PageGroup*> groups;
    HashSet<Page*>::iterator pagesEnd = allPages->end();
    for (HashSet<Page*>::iterator it = allPages->begin(); it != pagesEnd; ++it) {
        if (PageGroup* group = (*it)->groupPtr())
            groups.add(group);
    }
    HashSet<PageGroup*>::iterator groupsEnd = groups.end();
    for (HashSet<PageGroup*>::iterator it = groups.begin(); it != groupsEnd; ++it)
        (*it)->removeVisitedLinks();
}

void Page::allVisitedStateChanged(PageGroup* group)
{
    ASSERT(group);
    if (!allPages)
        return;

    HashSet<Page*>::iterator pagesEnd = allPages->end();
    for (HashSet<Page*>::iterator it = allPages->begin(); it != pagesEnd; ++it) {
        Page* page = *it;
        if (page->m_group != group)
            continue;
        for (Frame* frame = page->m_mainFrame.get(); frame; frame = frame->tree()->traverseNext()) {
            if (CSSStyleSelector* styleSelector = frame->document()->styleSelector())
                styleSelector->allVisitedStateChanged();
        }
    }
}

void Page::visitedStateChanged(PageGroup* group, LinkHash visitedLinkHash)
{
    ASSERT(group);
    if (!allPages)
        return;

    HashSet<Page*>::iterator pagesEnd = allPages->end();
    for (HashSet<Page*>::iterator it = allPages->begin(); it != pagesEnd; ++it) {
        Page* page = *it;
        if (page->m_group != group)
            continue;
        for (Frame* frame = page->m_mainFrame.get(); frame; frame = frame->tree()->traverseNext()) {
            if (CSSStyleSelector* styleSelector = frame->document()->styleSelector())
                styleSelector->visitedStateChanged(visitedLinkHash);
        }
    }
}

void Page::setDebuggerForAllPages(JSC::Debugger* debugger)
{
    ASSERT(allPages);

    HashSet<Page*>::iterator end = allPages->end();
    for (HashSet<Page*>::iterator it = allPages->begin(); it != end; ++it)
        (*it)->setDebugger(debugger);
}

void Page::setDebugger(JSC::Debugger* debugger)
{
    if (m_debugger == debugger)
        return;

    m_debugger = debugger;

    for (Frame* frame = m_mainFrame.get(); frame; frame = frame->tree()->traverseNext())
        frame->script()->attachDebugger(m_debugger);
}

SharedGraphicsContext3D* Page::sharedGraphicsContext3D()
{
#if ENABLE(ACCELERATED_2D_CANVAS)
    if (!m_sharedGraphicsContext3D)
#if USE(SKIA)
        // Temporary code to postpone massive test rebaselining
        m_sharedGraphicsContext3D = SharedGraphicsContext3D::create(chrome(), 0 /*settings()->legacyAccelerated2dCanvasEnabled() ? 0 : SharedGraphicsContext3D::UseSkiaGPU*/);
#else
        m_sharedGraphicsContext3D = SharedGraphicsContext3D::create(chrome(), 0);
#endif
    return m_sharedGraphicsContext3D.get();
#else // !ENABLE(ACCELERATED_2D_CANVAS)
    return 0;
#endif
}

#if ENABLE(DOM_STORAGE)
StorageNamespace* Page::sessionStorage(bool optionalCreate)
{
    if (!m_sessionStorage && optionalCreate)
        m_sessionStorage = StorageNamespace::sessionStorageNamespace(this, m_settings->sessionStorageQuota());

    return m_sessionStorage.get();
}

void Page::setSessionStorage(PassRefPtr<StorageNamespace> newStorage)
{
    m_sessionStorage = newStorage;
}
#endif

void Page::setCustomHTMLTokenizerTimeDelay(double customHTMLTokenizerTimeDelay)
{
    if (customHTMLTokenizerTimeDelay < 0) {
        m_customHTMLTokenizerTimeDelay = -1;
        return;
    }
    m_customHTMLTokenizerTimeDelay = customHTMLTokenizerTimeDelay;
}

void Page::setCustomHTMLTokenizerChunkSize(int customHTMLTokenizerChunkSize)
{
    if (customHTMLTokenizerChunkSize < 0) {
        m_customHTMLTokenizerChunkSize = -1;
        return;
    }
    m_customHTMLTokenizerChunkSize = customHTMLTokenizerChunkSize;
}

void Page::setMemoryCacheClientCallsEnabled(bool enabled)
{
    if (m_areMemoryCacheClientCallsEnabled == enabled)
        return;

    m_areMemoryCacheClientCallsEnabled = enabled;
    if (!enabled)
        return;

    for (RefPtr<Frame> frame = mainFrame(); frame; frame = frame->tree()->traverseNext())
        frame->loader()->tellClientAboutPastMemoryCacheLoads();
}

void Page::setJavaScriptURLsAreAllowed(bool areAllowed)
{
    m_javaScriptURLsAreAllowed = areAllowed;
}

bool Page::javaScriptURLsAreAllowed() const
{
    return m_javaScriptURLsAreAllowed;
}

void Page::setMinimumTimerInterval(double minimumTimerInterval)
{
    double oldTimerInterval = m_minimumTimerInterval;
    m_minimumTimerInterval = minimumTimerInterval;
    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNextWithWrap(false)) {
        if (frame->document())
            frame->document()->adjustMinimumTimerInterval(oldTimerInterval);
    }
}

double Page::minimumTimerInterval() const
{
    return m_minimumTimerInterval;
}

#if ENABLE(INPUT_SPEECH)
SpeechInput* Page::speechInput()
{
    ASSERT(m_speechInputClient);
    if (!m_speechInput.get())
        m_speechInput = adoptPtr(new SpeechInput(m_speechInputClient));
    return m_speechInput.get();
}
#endif

void Page::dnsPrefetchingStateChanged()
{
    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext())
        frame->document()->initDNSPrefetch();
}

void Page::privateBrowsingStateChanged()
{
    bool privateBrowsingEnabled = m_settings->privateBrowsingEnabled();

    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext())
        frame->document()->privateBrowsingStateDidChange();

    // Collect the PluginViews in to a vector to ensure that action the plug-in takes
    // from below privateBrowsingStateChanged does not affect their lifetime.
    Vector<RefPtr<PluginViewBase>, 32> pluginViewBases;
    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        FrameView* view = frame->view();
        if (!view)
            return;

        const HashSet<RefPtr<Widget> >* children = view->children();
        ASSERT(children);

        HashSet<RefPtr<Widget> >::const_iterator end = children->end();
        for (HashSet<RefPtr<Widget> >::const_iterator it = children->begin(); it != end; ++it) {
            Widget* widget = (*it).get();
            if (widget->isPluginViewBase())
                pluginViewBases.append(static_cast<PluginViewBase*>(widget));
        }
    }

    for (size_t i = 0; i < pluginViewBases.size(); ++i)
        pluginViewBases[i]->privateBrowsingStateChanged(privateBrowsingEnabled);
}

void Page::pluginAllowedRunTimeChanged()
{
    if (m_pluginHalter)
        m_pluginHalter->setPluginAllowedRunTime(m_settings->pluginAllowedRunTime());
}

void Page::didStartPlugin(HaltablePlugin* obj)
{
    if (m_pluginHalter)
        m_pluginHalter->didStartPlugin(obj);
}

void Page::didStopPlugin(HaltablePlugin* obj)
{
    if (m_pluginHalter)
        m_pluginHalter->didStopPlugin(obj);
}

void Page::addScrollableArea(ScrollableArea* scrollableArea)
{
    if (!m_scrollableAreaSet)
        m_scrollableAreaSet = adoptPtr(new ScrollableAreaSet);
    m_scrollableAreaSet->add(scrollableArea);
}

void Page::removeScrollableArea(ScrollableArea* scrollableArea)
{
    if (!m_scrollableAreaSet)
        return;
    m_scrollableAreaSet->remove(scrollableArea);
}

bool Page::containsScrollableArea(ScrollableArea* scrollableArea) const
{
    if (!m_scrollableAreaSet)
        return false;
    return m_scrollableAreaSet->contains(scrollableArea);
}

#if !ASSERT_DISABLED
void Page::checkFrameCountConsistency() const
{
    ASSERT(m_frameCount >= 0);

    int frameCount = 0;
    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext())
        ++frameCount;

    ASSERT(m_frameCount + 1 == frameCount);
}
#endif

Page::PageClients::PageClients()
    : chromeClient(0)
    , contextMenuClient(0)
    , editorClient(0)
    , dragClient(0)
    , inspectorClient(0)
    , geolocationClient(0)
    , deviceMotionClient(0)
    , deviceOrientationClient(0)
    , speechInputClient(0)
    , mediaStreamClient(0)
{
}

Page::PageClients::~PageClients()
{
}

} // namespace WebCore
