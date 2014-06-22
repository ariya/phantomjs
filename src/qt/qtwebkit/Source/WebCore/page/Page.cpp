/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013 Apple Inc. All Rights Reserved.
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

#include "AlternativeTextClient.h"
#include "AnimationController.h"
#include "BackForwardController.h"
#include "BackForwardList.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "ClientRectList.h"
#include "ContextMenuClient.h"
#include "ContextMenuController.h"
#include "DOMWindow.h"
#include "DocumentMarkerController.h"
#include "DocumentStyleSheetCollection.h"
#include "DragController.h"
#include "Editor.h"
#include "EditorClient.h"
#include "Event.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "ExceptionCodePlaceholder.h"
#include "FileSystem.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameSelection.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "HTMLElement.h"
#include "HistoryController.h"
#include "HistoryItem.h"
#include "InspectorController.h"
#include "InspectorInstrumentation.h"
#include "Logging.h"
#include "MediaCanStartListener.h"
#include "Navigator.h"
#include "NetworkStateNotifier.h"
#include "PageActivityAssertionToken.h"
#include "PageCache.h"
#include "PageConsole.h"
#include "PageGroup.h"
#include "PageThrottler.h"
#include "PlugInClient.h"
#include "PluginData.h"
#include "PluginView.h"
#include "PointerLockController.h"
#include "ProgressTracker.h"
#include "RenderArena.h"
#include "RenderLayerCompositor.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "RenderWidget.h"
#include "RuntimeEnabledFeatures.h"
#include "SchemeRegistry.h"
#include "ScriptController.h"
#include "ScrollingCoordinator.h"
#include "Settings.h"
#include "SharedBuffer.h"
#include "StorageArea.h"
#include "StorageNamespace.h"
#include "StyleResolver.h"
#include "TextResourceDecoder.h"
#include "VisitedLinkState.h"
#include "VoidCallback.h"
#include "Widget.h"
#include <wtf/HashMap.h>
#include <wtf/RefCountedLeakCounter.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/Base64.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

static HashSet<Page*>* allPages;

DEFINE_DEBUG_ONLY_GLOBAL(WTF::RefCountedLeakCounter, pageCounter, ("Page"));

static void networkStateChanged(bool isOnLine)
{
    Vector<RefPtr<Frame> > frames;
    
    // Get all the frames of all the pages in all the page groups
    HashSet<Page*>::iterator end = allPages->end();
    for (HashSet<Page*>::iterator it = allPages->begin(); it != end; ++it) {
        for (Frame* frame = (*it)->mainFrame(); frame; frame = frame->tree()->traverseNext())
            frames.append(frame);
        InspectorInstrumentation::networkStateChanged(*it);
    }

    AtomicString eventName = isOnLine ? eventNames().onlineEvent : eventNames().offlineEvent;
    for (unsigned i = 0; i < frames.size(); i++)
        frames[i]->document()->dispatchWindowEvent(Event::create(eventName, false, false));
}

float deviceScaleFactor(Frame* frame)
{
    if (!frame)
        return 1;
    Page* page = frame->page();
    if (!page)
        return 1;
    return page->deviceScaleFactor();
}

Page::Page(PageClients& pageClients)
    : m_chrome(Chrome::create(this, pageClients.chromeClient))
    , m_dragCaretController(DragCaretController::create())
#if ENABLE(DRAG_SUPPORT)
    , m_dragController(DragController::create(this, pageClients.dragClient))
#endif
    , m_focusController(FocusController::create(this))
#if ENABLE(CONTEXT_MENUS)
    , m_contextMenuController(ContextMenuController::create(this, pageClients.contextMenuClient))
#endif
#if ENABLE(INSPECTOR)
    , m_inspectorController(InspectorController::create(this, pageClients.inspectorClient))
#endif
#if ENABLE(POINTER_LOCK)
    , m_pointerLockController(PointerLockController::create(this))
#endif
    , m_settings(Settings::create(this))
    , m_progress(ProgressTracker::create())
    , m_backForwardController(BackForwardController::create(this, pageClients.backForwardClient))
    , m_theme(RenderTheme::themeForPage(this))
    , m_editorClient(pageClients.editorClient)
    , m_plugInClient(pageClients.plugInClient)
    , m_validationMessageClient(pageClients.validationMessageClient)
    , m_subframeCount(0)
    , m_openedByDOM(false)
    , m_tabKeyCyclesThroughElements(true)
    , m_defersLoading(false)
    , m_defersLoadingCallCount(0)
    , m_inLowQualityInterpolationMode(false)
    , m_cookieEnabled(true)
    , m_areMemoryCacheClientCallsEnabled(true)
    , m_mediaVolume(1)
    , m_pageScaleFactor(1)
    , m_deviceScaleFactor(1)
    , m_suppressScrollbarAnimations(false)
    , m_didLoadUserStyleSheet(false)
    , m_userStyleSheetModificationTime(0)
    , m_group(0)
    , m_debugger(0)
    , m_customHTMLTokenizerTimeDelay(-1)
    , m_customHTMLTokenizerChunkSize(-1)
    , m_canStartMedia(true)
#if ENABLE(VIEW_MODE_CSS_MEDIA)
    , m_viewMode(ViewModeWindowed)
#endif // ENABLE(VIEW_MODE_CSS_MEDIA)
    , m_minimumTimerInterval(Settings::defaultMinDOMTimerInterval())
    , m_timerAlignmentInterval(Settings::defaultDOMTimerAlignmentInterval())
    , m_isEditable(false)
    , m_isOnscreen(true)
    , m_isInWindow(true)
#if ENABLE(PAGE_VISIBILITY_API)
    , m_visibilityState(PageVisibilityStateVisible)
#endif
    , m_requestedLayoutMilestones(0)
    , m_headerHeight(0)
    , m_footerHeight(0)
    , m_isCountingRelevantRepaintedObjects(false)
#ifndef NDEBUG
    , m_isPainting(false)
#endif
    , m_alternativeTextClient(pageClients.alternativeTextClient)
    , m_scriptedAnimationsSuspended(false)
    , m_pageThrottler(PageThrottler::create(this))
    , m_console(PageConsole::create(this))
    , m_framesHandlingBeforeUnloadEvent(0)
{
    ASSERT(m_editorClient);

    if (!allPages) {
        allPages = new HashSet<Page*>;
        
        networkStateNotifier().addNetworkStateChangeListener(networkStateChanged);
    }

    ASSERT(!allPages->contains(this));
    allPages->add(this);

#ifndef NDEBUG
    pageCounter.increment();
#endif
}

Page::~Page()
{
    m_mainFrame->setView(0);
    setGroupName(String());
    allPages->remove(this);
    
    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        frame->willDetachPage();
        frame->detachFromPage();
    }

    m_editorClient->pageDestroyed();
    if (m_plugInClient)
        m_plugInClient->pageDestroyed();
    if (m_alternativeTextClient)
        m_alternativeTextClient->pageDestroyed();

#if ENABLE(INSPECTOR)
    m_inspectorController->inspectedPageDestroyed();
#endif

    if (m_scrollingCoordinator)
        m_scrollingCoordinator->pageDestroyed();

    backForward()->close();

#ifndef NDEBUG
    pageCounter.decrement();
#endif

    m_pageThrottler.clear();
}

ArenaSize Page::renderTreeSize() const
{
    ArenaSize total(0, 0);
    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        if (!frame->document())
            continue;
        if (RenderArena* arena = frame->document()->renderArena()) {
            total.treeSize += arena->totalRenderArenaSize();
            total.allocated += arena->totalRenderArenaAllocatedBytes();
        }
    }
    return total;
}

ViewportArguments Page::viewportArguments() const
{
    return mainFrame() && mainFrame()->document() ? mainFrame()->document()->viewportArguments() : ViewportArguments();
}

ScrollingCoordinator* Page::scrollingCoordinator()
{
    if (!m_scrollingCoordinator && m_settings->scrollingCoordinatorEnabled())
        m_scrollingCoordinator = ScrollingCoordinator::create(this);

    return m_scrollingCoordinator.get();
}

String Page::scrollingStateTreeAsText()
{
    if (Document* document = m_mainFrame->document())
        document->updateLayout();

    if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
        return scrollingCoordinator->scrollingStateTreeAsText();

    return String();
}

String Page::mainThreadScrollingReasonsAsText()
{
    if (Document* document = m_mainFrame->document())
        document->updateLayout();

    if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
        return scrollingCoordinator->mainThreadScrollingReasonsAsText();

    return String();
}

PassRefPtr<ClientRectList> Page::nonFastScrollableRects(const Frame* frame)
{
    if (Document* document = m_mainFrame->document())
        document->updateLayout();

    Vector<IntRect> rects;
    if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
        rects = scrollingCoordinator->computeNonFastScrollableRegion(frame, IntPoint()).rects();

    Vector<FloatQuad> quads(rects.size());
    for (size_t i = 0; i < rects.size(); ++i)
        quads[i] = FloatRect(rects[i]);
    return ClientRectList::create(quads);
}

#if ENABLE(VIEW_MODE_CSS_MEDIA)
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
        m_mainFrame->document()->styleResolverChanged(RecalcStyleImmediately);
}
#endif // ENABLE(VIEW_MODE_CSS_MEDIA)

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

    if (!item)
        return;

    goToItem(item, FrameLoadTypeIndexedBackForward);
}

void Page::goToItem(HistoryItem* item, FrameLoadType type)
{
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
    return m_group ? m_group->name() : nullAtom.string();
}

void Page::initGroup()
{
    ASSERT(!m_singlePageGroup);
    ASSERT(!m_group);
    m_singlePageGroup = PageGroup::create(this);
    m_group = m_singlePageGroup.get();
}

void Page::updateStyleForAllPagesAfterGlobalChangeInEnvironment()
{
    if (!allPages)
        return;
    HashSet<Page*>::iterator end = allPages->end();
    for (HashSet<Page*>::iterator it = allPages->begin(); it != end; ++it)
        for (Frame* frame = (*it)->mainFrame(); frame; frame = frame->tree()->traverseNext()) {
            // If a change in the global environment has occurred, we need to
            // make sure all the properties a recomputed, therefore we invalidate
            // the properties cache.
            if (StyleResolver* styleResolver = frame->document()->styleResolverIfExists())
                styleResolver->invalidateMatchedPropertiesCache();
            frame->document()->scheduleForcedStyleRecalc();
        }
}

void Page::setNeedsRecalcStyleInAllFrames()
{
    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext())
        frame->document()->styleResolverChanged(DeferRecalcStyle);
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
        if (frame->editor().findString(target, (options & ~WrapAround) | StartInSelection)) {
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
        bool found = startFrame->editor().findString(target, options | WrapAround | StartInSelection);
        focusController()->setFocusedFrame(frame);
        return found;
    }

    return false;
}

void Page::findStringMatchingRanges(const String& target, FindOptions options, int limit, Vector<RefPtr<Range> >* matchRanges, int& indexForSelection)
{
    indexForSelection = 0;
    if (!mainFrame())
        return;

    Frame* frame = mainFrame();
    Frame* frameWithSelection = 0;
    do {
        frame->editor().countMatchesForText(target, 0, options, limit ? (limit - matchRanges->size()) : 0, true, matchRanges);
        if (frame->selection()->isRange())
            frameWithSelection = frame;
        frame = incrementFrame(frame, true, false);
    } while (frame);

    if (matchRanges->isEmpty())
        return;

    if (frameWithSelection) {
        indexForSelection = NoMatchAfterUserSelection;
        RefPtr<Range> selectedRange = frameWithSelection->selection()->selection().firstRange();
        if (options & Backwards) {
            for (size_t i = matchRanges->size(); i > 0; --i) {
                if (selectedRange->compareBoundaryPoints(Range::END_TO_START, matchRanges->at(i - 1).get(), IGNORE_EXCEPTION) > 0) {
                    indexForSelection = i - 1;
                    break;
                }
            }
        } else {
            for (size_t i = 0; i < matchRanges->size(); ++i) {
                if (selectedRange->compareBoundaryPoints(Range::START_TO_END, matchRanges->at(i).get(), IGNORE_EXCEPTION) < 0) {
                    indexForSelection = i;
                    break;
                }
            }
        }
    } else {
        if (options & Backwards)
            indexForSelection = matchRanges->size() - 1;
        else
            indexForSelection = 0;
    }
}

PassRefPtr<Range> Page::rangeOfString(const String& target, Range* referenceRange, FindOptions options)
{
    if (target.isEmpty() || !mainFrame())
        return 0;

    if (referenceRange && referenceRange->ownerDocument()->page() != this)
        return 0;

    bool shouldWrap = options & WrapAround;
    Frame* frame = referenceRange ? referenceRange->ownerDocument()->frame() : mainFrame();
    Frame* startFrame = frame;
    do {
        if (RefPtr<Range> resultRange = frame->editor().rangeOfString(target, frame == startFrame ? referenceRange : 0, options & ~WrapAround))
            return resultRange.release();

        frame = incrementFrame(frame, !(options & Backwards), shouldWrap);
    } while (frame && frame != startFrame);

    // Search contents of startFrame, on the other side of the reference range that we did earlier.
    // We cheat a bit and just search again with wrap on.
    if (shouldWrap && referenceRange) {
        if (RefPtr<Range> resultRange = startFrame->editor().rangeOfString(target, referenceRange, options | WrapAround | StartInSelection))
            return resultRange.release();
    }

    return 0;
}

unsigned Page::findMatchesForText(const String& target, FindOptions options, unsigned maxMatchCount, ShouldHighlightMatches shouldHighlightMatches, ShouldMarkMatches shouldMarkMatches)
{
    if (target.isEmpty() || !mainFrame())
        return 0;

    unsigned matchCount = 0;

    Frame* frame = mainFrame();
    do {
        if (shouldMarkMatches == MarkMatches)
            frame->editor().setMarkedTextMatchesAreHighlighted(shouldHighlightMatches == HighlightMatches);
        matchCount += frame->editor().countMatchesForText(target, 0, options, maxMatchCount ? (maxMatchCount - matchCount) : 0, shouldMarkMatches == MarkMatches, 0);
        frame = incrementFrame(frame, true, false);
    } while (frame);

    return matchCount;
}

unsigned Page::markAllMatchesForText(const String& target, FindOptions options, bool shouldHighlight, unsigned maxMatchCount)
{
    return findMatchesForText(target, options, maxMatchCount, shouldHighlight ? HighlightMatches : DoNotHighlightMatches, MarkMatches);
}

unsigned Page::countFindMatches(const String& target, FindOptions options, unsigned maxMatchCount)
{
    return findMatchesForText(target, options, maxMatchCount, DoNotHighlightMatches, DoNotMarkMatches);
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

    if (m_settings->wantsBalancedSetDefersLoadingBehavior()) {
        ASSERT(defers || m_defersLoadingCallCount);
        if (defers && ++m_defersLoadingCallCount > 1)
            return;
        if (!defers && --m_defersLoadingCallCount)
            return;
    } else {
        ASSERT(!m_defersLoadingCallCount);
        if (defers == m_defersLoading)
            return;
    }

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

void Page::setPageScaleFactor(float scale, const IntPoint& origin)
{
    Document* document = mainFrame()->document();
    FrameView* view = document->view();

    if (scale == m_pageScaleFactor) {
        if (view && (view->scrollPosition() != origin || view->delegatesScrolling())) {
            if (!m_settings->applyPageScaleFactorInCompositor())
                document->updateLayoutIgnorePendingStylesheets();
            view->setScrollPosition(origin);
        }
        return;
    }

    m_pageScaleFactor = scale;

    if (!m_settings->applyPageScaleFactorInCompositor()) {
        if (document->renderer())
            document->renderer()->setNeedsLayout(true);

        document->recalcStyle(Node::Force);

        // Transform change on RenderView doesn't trigger repaint on non-composited contents.
        mainFrame()->view()->invalidateRect(IntRect(LayoutRect::infiniteRect()));
    }

#if USE(ACCELERATED_COMPOSITING)
    mainFrame()->deviceOrPageScaleFactorChanged();
#endif

    if (view && view->fixedElementsLayoutRelativeToFrame())
        view->setViewportConstrainedObjectsNeedLayout();

    if (view && view->scrollPosition() != origin) {
        if (!m_settings->applyPageScaleFactorInCompositor() && document->renderer() && document->renderer()->needsLayout() && view->didFirstLayout())
            view->layout();
        view->setScrollPosition(origin);
    }
}


void Page::setDeviceScaleFactor(float scaleFactor)
{
    if (m_deviceScaleFactor == scaleFactor)
        return;

    m_deviceScaleFactor = scaleFactor;
    setNeedsRecalcStyleInAllFrames();

#if USE(ACCELERATED_COMPOSITING)
    if (mainFrame())
        mainFrame()->deviceOrPageScaleFactorChanged();

    pageCache()->markPagesForDeviceScaleChanged(this);
#endif

    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext())
        frame->editor().deviceScaleFactorChanged();

    pageCache()->markPagesForFullStyleRecalc(this);
}

void Page::setShouldSuppressScrollbarAnimations(bool suppressAnimations)
{
    if (suppressAnimations == m_suppressScrollbarAnimations)
        return;

    if (!suppressAnimations) {
        // If animations are not going to be suppressed anymore, then there is nothing to do here but
        // change the cached value.
        m_suppressScrollbarAnimations = suppressAnimations;
        return;
    }

    // On the other hand, if we are going to start suppressing animations, then we need to make sure we
    // finish any current scroll animations first.
    FrameView* view = mainFrame()->view();
    if (!view)
        return;

    view->finishCurrentScrollAnimations();
    
    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        FrameView* frameView = frame->view();
        if (!frameView)
            continue;

        const HashSet<ScrollableArea*>* scrollableAreas = frameView->scrollableAreas();
        if (!scrollableAreas)
            continue;

        for (HashSet<ScrollableArea*>::const_iterator it = scrollableAreas->begin(), end = scrollableAreas->end(); it != end; ++it) {
            ScrollableArea* scrollableArea = *it;
            ASSERT(scrollableArea->scrollbarsCanBeActive());

            scrollableArea->finishCurrentScrollAnimations();
        }
    }

    m_suppressScrollbarAnimations = suppressAnimations;
}

bool Page::rubberBandsAtBottom()
{
    if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
        return scrollingCoordinator->rubberBandsAtBottom();

    return false;
}

void Page::setRubberBandsAtBottom(bool rubberBandsAtBottom)
{
    if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
        scrollingCoordinator->setRubberBandsAtBottom(rubberBandsAtBottom);
}

bool Page::rubberBandsAtTop()
{
    if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
        return scrollingCoordinator->rubberBandsAtTop();

    return false;
}

void Page::setRubberBandsAtTop(bool rubberBandsAtTop)
{
    if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
        scrollingCoordinator->setRubberBandsAtTop(rubberBandsAtTop);
}

void Page::setPagination(const Pagination& pagination)
{
    if (m_pagination == pagination)
        return;

    m_pagination = pagination;

    setNeedsRecalcStyleInAllFrames();
    pageCache()->markPagesForFullStyleRecalc(this);
}

unsigned Page::pageCount() const
{
    if (m_pagination.mode == Pagination::Unpaginated)
        return 0;

    if (Document* document = mainFrame()->document())
        document->updateLayoutIgnorePendingStylesheets();

    RenderView* contentRenderer = mainFrame()->contentRenderer();
    return contentRenderer ? contentRenderer->columnCount(contentRenderer->columnInfo()) : 0;
}

void Page::didMoveOnscreen()
{
    m_isOnscreen = true;

    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        if (FrameView* frameView = frame->view())
            frameView->didMoveOnscreen();
    }
    
    resumeScriptedAnimations();
}

void Page::willMoveOffscreen()
{
    m_isOnscreen = false;

    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        if (FrameView* frameView = frame->view())
            frameView->willMoveOffscreen();
    }
    
    suspendScriptedAnimations();
}

void Page::setIsInWindow(bool isInWindow)
{
    if (m_isInWindow == isInWindow)
        return;

    m_isInWindow = isInWindow;

    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        if (FrameView* frameView = frame->view())
            frameView->setIsInWindow(isInWindow);
    }
}

void Page::suspendScriptedAnimations()
{
    m_scriptedAnimationsSuspended = true;
    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        if (frame->document())
            frame->document()->suspendScriptedAnimationControllerCallbacks();
    }
}

void Page::resumeScriptedAnimations()
{
    m_scriptedAnimationsSuspended = false;
    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        if (frame->document())
            frame->document()->resumeScriptedAnimationControllerCallbacks();
    }
}

void Page::setThrottled(bool throttled)
{
    m_pageThrottler->setThrottled(throttled);
}

void Page::userStyleSheetLocationChanged()
{
    // FIXME: Eventually we will move to a model of just being handed the sheet
    // text instead of loading the URL ourselves.
    KURL url = m_settings->userStyleSheetLocation();
    
    // Allow any local file URL scheme to be loaded.
    if (SchemeRegistry::shouldTreatURLSchemeAsLocal(url.protocol()))
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
        if (base64Decode(decodeURLEscapeSequences(url.string().substring(35)), styleSheetAsUTF8, Base64IgnoreWhitespace))
            m_userStyleSheet = String::fromUTF8(styleSheetAsUTF8.data(), styleSheetAsUTF8.size());
    }

    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        if (frame->document())
            frame->document()->styleSheetCollection()->updatePageUserSheet();
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
    m_userStyleSheet.append(decoder->flush());

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
        for (Frame* frame = page->m_mainFrame.get(); frame; frame = frame->tree()->traverseNext())
            frame->document()->visitedLinkState()->invalidateStyleForAllLinks();
    }
}

void Page::visitedStateChanged(PageGroup* group, LinkHash linkHash)
{
    ASSERT(group);
    if (!allPages)
        return;

    HashSet<Page*>::iterator pagesEnd = allPages->end();
    for (HashSet<Page*>::iterator it = allPages->begin(); it != pagesEnd; ++it) {
        Page* page = *it;
        if (page->m_group != group)
            continue;
        for (Frame* frame = page->m_mainFrame.get(); frame; frame = frame->tree()->traverseNext())
            frame->document()->visitedLinkState()->invalidateStyleForLink(linkHash);
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

StorageNamespace* Page::sessionStorage(bool optionalCreate)
{
    if (!m_sessionStorage && optionalCreate)
        m_sessionStorage = StorageNamespace::sessionStorageNamespace(this);

    return m_sessionStorage.get();
}

void Page::setSessionStorage(PassRefPtr<StorageNamespace> newStorage)
{
    m_sessionStorage = newStorage;
}

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

void Page::setTimerAlignmentInterval(double interval)
{
    if (interval == m_timerAlignmentInterval)
        return;

    m_timerAlignmentInterval = interval;
    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNextWithWrap(false)) {
        if (frame->document())
            frame->document()->didChangeTimerAlignmentInterval();
    }
}

double Page::timerAlignmentInterval() const
{
    return m_timerAlignmentInterval;
}

void Page::dnsPrefetchingStateChanged()
{
    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext())
        frame->document()->initDNSPrefetch();
}

void Page::collectPluginViews(Vector<RefPtr<PluginViewBase>, 32>& pluginViewBases)
{
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
                pluginViewBases.append(toPluginViewBase(widget));
        }
    }
}

void Page::storageBlockingStateChanged()
{
    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext())
        frame->document()->storageBlockingStateDidChange();

    // Collect the PluginViews in to a vector to ensure that action the plug-in takes
    // from below storageBlockingStateChanged does not affect their lifetime.
    Vector<RefPtr<PluginViewBase>, 32> pluginViewBases;
    collectPluginViews(pluginViewBases);

    for (size_t i = 0; i < pluginViewBases.size(); ++i)
        pluginViewBases[i]->storageBlockingStateChanged();
}

void Page::privateBrowsingStateChanged()
{
    bool privateBrowsingEnabled = m_settings->privateBrowsingEnabled();

    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext())
        frame->document()->privateBrowsingStateDidChange();

    // Collect the PluginViews in to a vector to ensure that action the plug-in takes
    // from below privateBrowsingStateChanged does not affect their lifetime.
    Vector<RefPtr<PluginViewBase>, 32> pluginViewBases;
    collectPluginViews(pluginViewBases);

    for (size_t i = 0; i < pluginViewBases.size(); ++i)
        pluginViewBases[i]->privateBrowsingStateChanged(privateBrowsingEnabled);
}

#if !ASSERT_DISABLED
void Page::checkSubframeCountConsistency() const
{
    ASSERT(m_subframeCount >= 0);

    int subframeCount = 0;
    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext())
        ++subframeCount;

    ASSERT(m_subframeCount + 1 == subframeCount);
}
#endif

void Page::throttleTimers()
{
#if ENABLE(HIDDEN_PAGE_DOM_TIMER_THROTTLING)
    if (m_settings->hiddenPageDOMTimerThrottlingEnabled())
        setTimerAlignmentInterval(Settings::hiddenPageDOMTimerAlignmentInterval());
#endif
}

void Page::unthrottleTimers()
{
#if ENABLE(HIDDEN_PAGE_DOM_TIMER_THROTTLING)
    if (m_settings->hiddenPageDOMTimerThrottlingEnabled())
        setTimerAlignmentInterval(Settings::defaultDOMTimerAlignmentInterval());
#endif
}

#if ENABLE(PAGE_VISIBILITY_API) || ENABLE(HIDDEN_PAGE_DOM_TIMER_THROTTLING)
void Page::setVisibilityState(PageVisibilityState visibilityState, bool isInitialState)
{
    // FIXME: the visibility state needs to be stored on the top-level document
    // https://bugs.webkit.org/show_bug.cgi?id=116769
#if ENABLE(PAGE_VISIBILITY_API)
    if (m_visibilityState == visibilityState)
        return;
    m_visibilityState = visibilityState;

    if (!isInitialState && m_mainFrame)
        m_mainFrame->dispatchVisibilityStateChangeEvent();
#endif

    if (visibilityState == WebCore::PageVisibilityStateHidden) {
        if (m_pageThrottler->shouldThrottleTimers())
            throttleTimers();
        if (m_settings->hiddenPageCSSAnimationSuspensionEnabled())
            mainFrame()->animation()->suspendAnimations();
    } else {
        unthrottleTimers();
        if (m_settings->hiddenPageCSSAnimationSuspensionEnabled())
            mainFrame()->animation()->resumeAnimations();
    }
#if !ENABLE(PAGE_VISIBILITY_API)
    UNUSED_PARAM(isInitialState);
#endif
}
#endif // ENABLE(PAGE_VISIBILITY_API) || ENABLE(HIDDEN_PAGE_DOM_TIMER_THROTTLING)

#if ENABLE(PAGE_VISIBILITY_API)
PageVisibilityState Page::visibilityState() const
{
    return m_visibilityState;
}
#endif

#if ENABLE(RUBBER_BANDING)
void Page::addHeaderWithHeight(int headerHeight)
{
    m_headerHeight = headerHeight;

    FrameView* frameView = mainFrame() ? mainFrame()->view() : 0;
    if (!frameView)
        return;

    RenderView* renderView = frameView->renderView();
    if (!renderView)
        return;

    frameView->setHeaderHeight(m_headerHeight);
    renderView->compositor()->updateLayerForHeader(m_headerHeight);
}

void Page::addFooterWithHeight(int footerHeight)
{
    m_footerHeight = footerHeight;

    FrameView* frameView = mainFrame() ? mainFrame()->view() : 0;
    if (!frameView)
        return;

    RenderView* renderView = frameView->renderView();
    if (!renderView)
        return;

    frameView->setFooterHeight(m_footerHeight);
    renderView->compositor()->updateLayerForFooter(m_footerHeight);
}
#endif

void Page::addLayoutMilestones(LayoutMilestones milestones)
{
    // In the future, we may want a function that replaces m_layoutMilestones instead of just adding to it.
    m_requestedLayoutMilestones |= milestones;
}

void Page::removeLayoutMilestones(LayoutMilestones milestones)
{
    m_requestedLayoutMilestones &= ~milestones;
}

// These are magical constants that might be tweaked over time.
static double gMinimumPaintedAreaRatio = 0.1;
static double gMaximumUnpaintedAreaRatio = 0.04;

bool Page::isCountingRelevantRepaintedObjects() const
{
    return m_isCountingRelevantRepaintedObjects && (m_requestedLayoutMilestones & DidHitRelevantRepaintedObjectsAreaThreshold);
}

void Page::startCountingRelevantRepaintedObjects()
{
    // Reset everything in case we didn't hit the threshold last time.
    resetRelevantPaintedObjectCounter();

    m_isCountingRelevantRepaintedObjects = true;
}

void Page::resetRelevantPaintedObjectCounter()
{
    m_isCountingRelevantRepaintedObjects = false;
    m_relevantUnpaintedRenderObjects.clear();
    m_topRelevantPaintedRegion = Region();
    m_bottomRelevantPaintedRegion = Region();
    m_relevantUnpaintedRegion = Region();
}

static LayoutRect relevantViewRect(RenderView* view)
{
    // DidHitRelevantRepaintedObjectsAreaThreshold is a LayoutMilestone intended to indicate that
    // a certain relevant amount of content has been drawn to the screen. This is the rect that
    // has been determined to be relevant in the context of this goal. We may choose to tweak
    // the rect over time, much like we may choose to tweak gMinimumPaintedAreaRatio and
    // gMaximumUnpaintedAreaRatio. But this seems to work well right now.
    LayoutRect relevantViewRect = LayoutRect(0, 0, 980, 1300);

    LayoutRect viewRect = view->viewRect();
    // If the viewRect is wider than the relevantViewRect, center the relevantViewRect.
    if (viewRect.width() > relevantViewRect.width())
        relevantViewRect.setX((viewRect.width() - relevantViewRect.width()) / 2);

    return relevantViewRect;
}

void Page::addRelevantRepaintedObject(RenderObject* object, const LayoutRect& objectPaintRect)
{
    if (!isCountingRelevantRepaintedObjects())
        return;

    // Objects inside sub-frames are not considered to be relevant.
    if (object->document()->frame() != mainFrame())
        return;

    RenderView* view = object->view();
    if (!view)
        return;

    LayoutRect relevantRect = relevantViewRect(view);

    // The objects are only relevant if they are being painted within the viewRect().
    if (!objectPaintRect.intersects(pixelSnappedIntRect(relevantRect)))
        return;

    IntRect snappedPaintRect = pixelSnappedIntRect(objectPaintRect);

    // If this object was previously counted as an unpainted object, remove it from that HashSet
    // and corresponding Region. FIXME: This doesn't do the right thing if the objects overlap.
    HashSet<RenderObject*>::iterator it = m_relevantUnpaintedRenderObjects.find(object);
    if (it != m_relevantUnpaintedRenderObjects.end()) {
        m_relevantUnpaintedRenderObjects.remove(it);
        m_relevantUnpaintedRegion.subtract(snappedPaintRect);
    }

    // Split the relevantRect into a top half and a bottom half. Making sure we have coverage in
    // both halves helps to prevent cases where we have a fully loaded menu bar or masthead with
    // no content beneath that.
    LayoutRect topRelevantRect = relevantRect;
    topRelevantRect.contract(LayoutSize(0, relevantRect.height() / 2));
    LayoutRect bottomRelevantRect = topRelevantRect;
    bottomRelevantRect.setY(relevantRect.height() / 2);

    // If the rect straddles both Regions, split it appropriately.
    if (topRelevantRect.intersects(snappedPaintRect) && bottomRelevantRect.intersects(snappedPaintRect)) {
        IntRect topIntersection = snappedPaintRect;
        topIntersection.intersect(pixelSnappedIntRect(topRelevantRect));
        m_topRelevantPaintedRegion.unite(topIntersection);

        IntRect bottomIntersection = snappedPaintRect;
        bottomIntersection.intersect(pixelSnappedIntRect(bottomRelevantRect));
        m_bottomRelevantPaintedRegion.unite(bottomIntersection);
    } else if (topRelevantRect.intersects(snappedPaintRect))
        m_topRelevantPaintedRegion.unite(snappedPaintRect);
    else
        m_bottomRelevantPaintedRegion.unite(snappedPaintRect);

    float topPaintedArea = m_topRelevantPaintedRegion.totalArea();
    float bottomPaintedArea = m_bottomRelevantPaintedRegion.totalArea();
    float viewArea = relevantRect.width() * relevantRect.height();

    float ratioThatIsPaintedOnTop = topPaintedArea / viewArea;
    float ratioThatIsPaintedOnBottom = bottomPaintedArea / viewArea;
    float ratioOfViewThatIsUnpainted = m_relevantUnpaintedRegion.totalArea() / viewArea;

    if (ratioThatIsPaintedOnTop > (gMinimumPaintedAreaRatio / 2) && ratioThatIsPaintedOnBottom > (gMinimumPaintedAreaRatio / 2)
        && ratioOfViewThatIsUnpainted < gMaximumUnpaintedAreaRatio) {
        m_isCountingRelevantRepaintedObjects = false;
        resetRelevantPaintedObjectCounter();
        if (Frame* frame = mainFrame())
            frame->loader()->didLayout(DidHitRelevantRepaintedObjectsAreaThreshold);
    }
}

void Page::addRelevantUnpaintedObject(RenderObject* object, const LayoutRect& objectPaintRect)
{
    if (!isCountingRelevantRepaintedObjects())
        return;

    // The objects are only relevant if they are being painted within the relevantViewRect().
    if (RenderView* view = object->view()) {
        if (!objectPaintRect.intersects(pixelSnappedIntRect(relevantViewRect(view))))
            return;
    }

    m_relevantUnpaintedRenderObjects.add(object);
    m_relevantUnpaintedRegion.unite(pixelSnappedIntRect(objectPaintRect));
}

void Page::suspendActiveDOMObjectsAndAnimations()
{
    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext())
        frame->suspendActiveDOMObjectsAndAnimations();
}

void Page::resumeActiveDOMObjectsAndAnimations()
{
    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext())
        frame->resumeActiveDOMObjectsAndAnimations();
}

bool Page::hasSeenAnyPlugin() const
{
    return !m_seenPlugins.isEmpty();
}

bool Page::hasSeenPlugin(const String& serviceType) const
{
    return m_seenPlugins.contains(serviceType);
}

void Page::sawPlugin(const String& serviceType)
{
    m_seenPlugins.add(serviceType);
}

void Page::resetSeenPlugins()
{
    m_seenPlugins.clear();
}

bool Page::hasSeenAnyMediaEngine() const
{
    return !m_seenMediaEngines.isEmpty();
}

bool Page::hasSeenMediaEngine(const String& engineDescription) const
{
    return m_seenMediaEngines.contains(engineDescription);
}

void Page::sawMediaEngine(const String& engineDescription)
{
    m_seenMediaEngines.add(engineDescription);
}

void Page::resetSeenMediaEngines()
{
    m_seenMediaEngines.clear();
}

PassOwnPtr<PageActivityAssertionToken> Page::createActivityToken()
{
    return adoptPtr(new PageActivityAssertionToken(m_pageThrottler.get()));
}

#if ENABLE(HIDDEN_PAGE_DOM_TIMER_THROTTLING)
void Page::hiddenPageDOMTimerThrottlingStateChanged()
{
    if (m_settings->hiddenPageDOMTimerThrottlingEnabled()) {
#if ENABLE(PAGE_VISIBILITY_API)
        if (m_pageThrottler->shouldThrottleTimers())
            setTimerAlignmentInterval(Settings::hiddenPageDOMTimerAlignmentInterval());
#endif
    } else
        setTimerAlignmentInterval(Settings::defaultDOMTimerAlignmentInterval());
}
#endif

#if (ENABLE_PAGE_VISIBILITY_API)
void Page::hiddenPageCSSAnimationSuspensionStateChanged()
{
    if (m_visibilityState == WebCore::PageVisibilityStateHidden) {
        if (m_settings->hiddenPageCSSAnimationSuspensionEnabled())
            mainFrame()->animation()->suspendAnimations();
        else
            mainFrame()->animation()->resumeAnimations();
    }
}
#endif

#if ENABLE(VIDEO_TRACK)
void Page::captionPreferencesChanged()
{
    for (Frame* frame = mainFrame(); frame; frame = frame->tree()->traverseNext())
        frame->document()->captionPreferencesChanged();
}
#endif

void Page::incrementFrameHandlingBeforeUnloadEventCount()
{
    ++m_framesHandlingBeforeUnloadEvent;
}

void Page::decrementFrameHandlingBeforeUnloadEventCount()
{
    ASSERT(m_framesHandlingBeforeUnloadEvent);
    --m_framesHandlingBeforeUnloadEvent;
}

bool Page::isAnyFrameHandlingBeforeUnloadEvent()
{
    return m_framesHandlingBeforeUnloadEvent;
}

Page::PageClients::PageClients()
    : alternativeTextClient(0)
    , chromeClient(0)
#if ENABLE(CONTEXT_MENUS)
    , contextMenuClient(0)
#endif
    , editorClient(0)
    , dragClient(0)
    , inspectorClient(0)
    , plugInClient(0)
    , validationMessageClient(0)
{
}

Page::PageClients::~PageClients()
{
}

} // namespace WebCore
