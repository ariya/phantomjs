/*
 * Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 *                     1999 Lars Knoll <knoll@kde.org>
 *                     1999 Antti Koivisto <koivisto@kde.org>
 *                     2000 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 *           (C) 2006 Graham Dennis (graham.dennis@gmail.com)
 *           (C) 2006 Alexey Proskuryakov (ap@nypop.com)
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
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
#include "FrameView.h"

#include "AXObjectCache.h"
#include "CSSStyleSelector.h"
#include "CachedResourceLoader.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "DocumentMarkerController.h"
#include "EventHandler.h"
#include "FloatRect.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameActionScheduler.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameTree.h"
#include "GraphicsContext.h"
#include "HTMLDocument.h"
#include "HTMLFrameElement.h"
#include "HTMLFrameSetElement.h"
#include "HTMLNames.h"
#include "HTMLPlugInImageElement.h"
#include "InspectorInstrumentation.h"
#include "OverflowEvent.h"
#include "RenderEmbeddedObject.h"
#include "RenderFullScreen.h"
#include "RenderLayer.h"
#include "RenderPart.h"
#include "RenderScrollbar.h"
#include "RenderScrollbarPart.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "ScrollAnimator.h"
#include "Settings.h"
#include "TextResourceDecoder.h"
#include <wtf/CurrentTime.h>

#if USE(ACCELERATED_COMPOSITING)
#include "RenderLayerCompositor.h"
#endif

#if ENABLE(SVG)
#include "SVGDocument.h"
#include "SVGLocatable.h"
#include "SVGNames.h"
#include "SVGPreserveAspectRatio.h"
#include "SVGSVGElement.h"
#include "SVGViewElement.h"
#include "SVGViewSpec.h"
#endif

#if ENABLE(TILED_BACKING_STORE)
#include "TiledBackingStore.h"
#endif

namespace WebCore {

using namespace HTMLNames;

double FrameView::sCurrentPaintTimeStamp = 0.0;

// REPAINT_THROTTLING now chooses default values for throttling parameters.
// Should be removed when applications start using runtime configuration.
#if ENABLE(REPAINT_THROTTLING)
// Normal delay
double FrameView::s_deferredRepaintDelay = 0.025;
// Negative value would mean that first few repaints happen without a delay
double FrameView::s_initialDeferredRepaintDelayDuringLoading = 0;
// The delay grows on each repaint to this maximum value
double FrameView::s_maxDeferredRepaintDelayDuringLoading = 2.5;
// On each repaint the delay increses by this amount
double FrameView::s_deferredRepaintDelayIncrementDuringLoading = 0.5;
#else
// FIXME: Repaint throttling could be good to have on all platform.
// The balance between CPU use and repaint frequency will need some tuning for desktop.
// More hooks may be needed to reset the delay on things like GIF and CSS animations.
double FrameView::s_deferredRepaintDelay = 0;
double FrameView::s_initialDeferredRepaintDelayDuringLoading = 0;
double FrameView::s_maxDeferredRepaintDelayDuringLoading = 0;
double FrameView::s_deferredRepaintDelayIncrementDuringLoading = 0;
#endif

// The maximum number of updateWidgets iterations that should be done before returning.
static const unsigned maxUpdateWidgetsIterations = 2;

FrameView::FrameView(Frame* frame)
    : m_frame(frame)
    , m_canHaveScrollbars(true)
    , m_slowRepaintObjectCount(0)
    , m_fixedObjectCount(0)
    , m_layoutTimer(this, &FrameView::layoutTimerFired)
    , m_layoutRoot(0)
    , m_hasPendingPostLayoutTasks(false)
    , m_inSynchronousPostLayout(false)
    , m_postLayoutTasksTimer(this, &FrameView::postLayoutTimerFired)
    , m_isTransparent(false)
    , m_baseBackgroundColor(Color::white)
    , m_mediaType("screen")
    , m_actionScheduler(adoptPtr(new FrameActionScheduler))
    , m_overflowStatusDirty(true)
    , m_viewportRenderer(0)
    , m_wasScrolledByUser(false)
    , m_inProgrammaticScroll(false)
    , m_deferredRepaintTimer(this, &FrameView::deferredRepaintTimerFired)
    , m_shouldUpdateWhileOffscreen(true)
    , m_deferSetNeedsLayouts(0)
    , m_setNeedsLayoutWasDeferred(false)
    , m_scrollCorner(0)
{
    init();

    if (m_frame) {
        if (Page* page = m_frame->page()) {
            m_page = page;
            m_page->addScrollableArea(this);

            if (m_frame == m_page->mainFrame()) {
                ScrollableArea::setVerticalScrollElasticity(ScrollElasticityAllowed);
                ScrollableArea::setHorizontalScrollElasticity(ScrollElasticityAllowed);
            }
        }
    }
}

PassRefPtr<FrameView> FrameView::create(Frame* frame)
{
    RefPtr<FrameView> view = adoptRef(new FrameView(frame));
    view->show();
    return view.release();
}

PassRefPtr<FrameView> FrameView::create(Frame* frame, const IntSize& initialSize)
{
    RefPtr<FrameView> view = adoptRef(new FrameView(frame));
    view->Widget::setFrameRect(IntRect(view->pos(), initialSize));
    view->setInitialBoundsSize(initialSize);
    view->show();
    return view.release();
}

FrameView::~FrameView()
{
    if (m_hasPendingPostLayoutTasks) {
        m_postLayoutTasksTimer.stop();
        m_actionScheduler->clear();
    }
    
    if (AXObjectCache::accessibilityEnabled() && axObjectCache())
        axObjectCache()->remove(this);
    
    resetScrollbars();

    // Custom scrollbars should already be destroyed at this point
    ASSERT(!horizontalScrollbar() || !horizontalScrollbar()->isCustomScrollbar());
    ASSERT(!verticalScrollbar() || !verticalScrollbar()->isCustomScrollbar());

    setHasHorizontalScrollbar(false); // Remove native scrollbars now before we lose the connection to the HostWindow.
    setHasVerticalScrollbar(false);
    
    ASSERT(!m_scrollCorner);
    ASSERT(m_actionScheduler->isEmpty());

    if (m_page)
        m_page->removeScrollableArea(this);

    if (m_frame) {
        ASSERT(m_frame->view() != this || !m_frame->contentRenderer());
        RenderPart* renderer = m_frame->ownerRenderer();
        if (renderer && renderer->widget() == this)
            renderer->setWidget(0);
    }
}

void FrameView::reset()
{
    m_useSlowRepaints = false;
    m_isOverlapped = false;
    m_contentIsOpaque = false;
    m_borderX = 30;
    m_borderY = 30;
    m_layoutTimer.stop();
    m_layoutRoot = 0;
    m_delayedLayout = false;
    m_doFullRepaint = true;
    m_layoutSchedulingEnabled = true;
    m_inLayout = false;
    m_inSynchronousPostLayout = false;
    m_hasPendingPostLayoutTasks = false;
    m_layoutCount = 0;
    m_nestedLayoutCount = 0;
    m_postLayoutTasksTimer.stop();
    m_firstLayout = true;
    m_firstLayoutCallbackPending = false;
    m_wasScrolledByUser = false;
    m_lastLayoutSize = IntSize();
    m_lastZoomFactor = 1.0f;
    m_deferringRepaints = 0;
    m_repaintCount = 0;
    m_repaintRects.clear();
    m_deferredRepaintDelay = s_initialDeferredRepaintDelayDuringLoading;
    m_deferredRepaintTimer.stop();
    m_lastPaintTime = 0;
    m_paintBehavior = PaintBehaviorNormal;
    m_isPainting = false;
    m_isVisuallyNonEmpty = false;
    m_firstVisuallyNonEmptyLayoutCallbackPending = true;
    m_maintainScrollPositionAnchor = 0;
}

bool FrameView::isFrameView() const 
{ 
    return true; 
}

void FrameView::clearFrame()
{
    m_frame = 0;
}

void FrameView::resetScrollbars()
{
    // Reset the document's scrollbars back to our defaults before we yield the floor.
    m_firstLayout = true;
    setScrollbarsSuppressed(true);
    if (m_canHaveScrollbars)
        setScrollbarModes(ScrollbarAuto, ScrollbarAuto);
    else
        setScrollbarModes(ScrollbarAlwaysOff, ScrollbarAlwaysOff);
    setScrollbarsSuppressed(false);
}

void FrameView::resetScrollbarsAndClearContentsSize()
{
    resetScrollbars();

    setScrollbarsSuppressed(true);
    setContentsSize(IntSize());
    setScrollbarsSuppressed(false);
}

void FrameView::init()
{
    reset();

    m_margins = IntSize(-1, -1); // undefined
    m_size = IntSize();

    // Propagate the marginwidth/height and scrolling modes to the view.
    Element* ownerElement = m_frame ? m_frame->ownerElement() : 0;
    if (ownerElement && (ownerElement->hasTagName(frameTag) || ownerElement->hasTagName(iframeTag))) {
        HTMLFrameElement* frameElt = static_cast<HTMLFrameElement*>(ownerElement);
        if (frameElt->scrollingMode() == ScrollbarAlwaysOff)
            setCanHaveScrollbars(false);
        int marginWidth = frameElt->marginWidth();
        int marginHeight = frameElt->marginHeight();
        if (marginWidth != -1)
            setMarginWidth(marginWidth);
        if (marginHeight != -1)
            setMarginHeight(marginHeight);
    }
}

void FrameView::detachCustomScrollbars()
{
    if (!m_frame)
        return;

    Scrollbar* horizontalBar = horizontalScrollbar();
    if (horizontalBar && horizontalBar->isCustomScrollbar())
        setHasHorizontalScrollbar(false);

    Scrollbar* verticalBar = verticalScrollbar();
    if (verticalBar && verticalBar->isCustomScrollbar())
        setHasVerticalScrollbar(false);

    if (m_scrollCorner) {
        m_scrollCorner->destroy();
        m_scrollCorner = 0;
    }
}

ScrollbarOverlayStyle FrameView::recommendedScrollbarOverlayStyle() const
{
    Color bgColor = m_frame->getDocumentBackgroundColor();
    if (!bgColor.isValid())
        return ScrollbarOverlayStyleDefault;
    
    // Reduce the background color from RGB to a lightness value
    // and determine which scrollbar style to use based on a lightness
    // heuristic.
    double hue, saturation, lightness;
    bgColor.getHSL(hue, saturation, lightness);
    if (lightness > .5)
        return ScrollbarOverlayStyleDefault;
    
    return ScrollbarOverlayStyleLight;
}

void FrameView::clear()
{
    setCanBlitOnScroll(true);
    
    reset();

    if (m_frame) {
        if (RenderPart* renderer = m_frame->ownerRenderer())
            renderer->viewCleared();
    }

    setScrollbarsSuppressed(true);
}

bool FrameView::didFirstLayout() const
{
    return !m_firstLayout;
}

void FrameView::invalidateRect(const IntRect& rect)
{
    if (!parent()) {
        if (hostWindow())
            hostWindow()->invalidateContentsAndWindow(rect, false /*immediate*/);
        return;
    }

    if (!m_frame)
        return;

    RenderPart* renderer = m_frame->ownerRenderer();
    if (!renderer)
        return;

    IntRect repaintRect = rect;
    repaintRect.move(renderer->borderLeft() + renderer->paddingLeft(),
                     renderer->borderTop() + renderer->paddingTop());
    renderer->repaintRectangle(repaintRect);
}

void FrameView::setFrameRect(const IntRect& newRect)
{
    IntRect oldRect = frameRect();
    if (newRect == oldRect)
        return;

    ScrollView::setFrameRect(newRect);

#if USE(ACCELERATED_COMPOSITING)
    if (RenderView* root = m_frame->contentRenderer()) {
        if (root->usesCompositing())
            root->compositor()->frameViewDidChangeSize();
    }
#endif
}

#if ENABLE(REQUEST_ANIMATION_FRAME)
void FrameView::scheduleAnimation()
{
    if (hostWindow())
        hostWindow()->scheduleAnimation();
}
#endif

void FrameView::setMarginWidth(int w)
{
    // make it update the rendering area when set
    m_margins.setWidth(w);
}

void FrameView::setMarginHeight(int h)
{
    // make it update the rendering area when set
    m_margins.setHeight(h);
}

bool FrameView::avoidScrollbarCreation() const
{
    ASSERT(m_frame);

    // with frame flattening no subframe can have scrollbars
    // but we also cannot turn scrollbars of as we determine
    // our flattening policy using that.

    if (!m_frame->ownerElement())
        return false;

    if (!m_frame->settings() || m_frame->settings()->frameFlatteningEnabled())
        return true;

    return false;
}

void FrameView::setCanHaveScrollbars(bool canHaveScrollbars)
{
    m_canHaveScrollbars = canHaveScrollbars;
    ScrollView::setCanHaveScrollbars(canHaveScrollbars);
}

void FrameView::updateCanHaveScrollbars()
{
    ScrollbarMode hMode;
    ScrollbarMode vMode;
    scrollbarModes(hMode, vMode);
    if (hMode == ScrollbarAlwaysOff && vMode == ScrollbarAlwaysOff)
        setCanHaveScrollbars(false);
    else
        setCanHaveScrollbars(true);
}

PassRefPtr<Scrollbar> FrameView::createScrollbar(ScrollbarOrientation orientation)
{
    // FIXME: We need to update the scrollbar dynamically as documents change (or as doc elements and bodies get discovered that have custom styles).
    Document* doc = m_frame->document();

    // Try the <body> element first as a scrollbar source.
    Element* body = doc ? doc->body() : 0;
    if (body && body->renderer() && body->renderer()->style()->hasPseudoStyle(SCROLLBAR))
        return RenderScrollbar::createCustomScrollbar(this, orientation, body->renderer()->enclosingBox());
    
    // If the <body> didn't have a custom style, then the root element might.
    Element* docElement = doc ? doc->documentElement() : 0;
    if (docElement && docElement->renderer() && docElement->renderer()->style()->hasPseudoStyle(SCROLLBAR))
        return RenderScrollbar::createCustomScrollbar(this, orientation, docElement->renderBox());
        
    // If we have an owning iframe/frame element, then it can set the custom scrollbar also.
    RenderPart* frameRenderer = m_frame->ownerRenderer();
    if (frameRenderer && frameRenderer->style()->hasPseudoStyle(SCROLLBAR))
        return RenderScrollbar::createCustomScrollbar(this, orientation, 0, m_frame.get());
    
    // Nobody set a custom style, so we just use a native scrollbar.
    return ScrollView::createScrollbar(orientation);
}

void FrameView::setContentsSize(const IntSize& size)
{
    if (size == contentsSize())
        return;

    m_deferSetNeedsLayouts++;

    ScrollView::setContentsSize(size);
    scrollAnimator()->contentsResized();
    
    Page* page = frame() ? frame()->page() : 0;
    if (!page)
        return;

    page->chrome()->contentsSizeChanged(frame(), size); //notify only

    m_deferSetNeedsLayouts--;
    
    if (!m_deferSetNeedsLayouts)
        m_setNeedsLayoutWasDeferred = false; // FIXME: Find a way to make the deferred layout actually happen.
}

void FrameView::adjustViewSize()
{
    ASSERT(m_frame->view() == this);
    RenderView* root = m_frame->contentRenderer();
    if (!root)
        return;

    IntSize size = IntSize(root->docWidth(), root->docHeight());

    ScrollView::setScrollOrigin(IntPoint(-root->docLeft(), -root->docTop()), !m_frame->document()->printing(), size == contentsSize());
    
    setContentsSize(size);
}

void FrameView::applyOverflowToViewport(RenderObject* o, ScrollbarMode& hMode, ScrollbarMode& vMode)
{
    // Handle the overflow:hidden/scroll case for the body/html elements.  WinIE treats
    // overflow:hidden and overflow:scroll on <body> as applying to the document's
    // scrollbars.  The CSS2.1 draft states that HTML UAs should use the <html> or <body> element and XML/XHTML UAs should
    // use the root element.
    switch (o->style()->overflowX()) {
        case OHIDDEN:
            hMode = ScrollbarAlwaysOff;
            break;
        case OSCROLL:
            hMode = ScrollbarAlwaysOn;
            break;
        case OAUTO:
            hMode = ScrollbarAuto;
            break;
        default:
            // Don't set it at all.
            ;
    }
    
     switch (o->style()->overflowY()) {
        case OHIDDEN:
            vMode = ScrollbarAlwaysOff;
            break;
        case OSCROLL:
            vMode = ScrollbarAlwaysOn;
            break;
        case OAUTO:
            vMode = ScrollbarAuto;
            break;
        default:
            // Don't set it at all.
            ;
    }

    m_viewportRenderer = o;
}

void FrameView::calculateScrollbarModesForLayout(ScrollbarMode& hMode, ScrollbarMode& vMode)
{
    m_viewportRenderer = 0;

    const HTMLFrameOwnerElement* owner = m_frame->ownerElement();
    if (owner && (owner->scrollingMode() == ScrollbarAlwaysOff)) {
        hMode = ScrollbarAlwaysOff;
        vMode = ScrollbarAlwaysOff;
        return;
    }  
    
    if (m_canHaveScrollbars) {
        hMode = ScrollbarAuto;
        vMode = ScrollbarAuto;
    } else {
        hMode = ScrollbarAlwaysOff;
        vMode = ScrollbarAlwaysOff;
    }
    
    if (!m_layoutRoot) {
        Document* document = m_frame->document();
        Node* documentElement = document->documentElement();
        RenderObject* rootRenderer = documentElement ? documentElement->renderer() : 0;
        Node* body = document->body();
        if (body && body->renderer()) {
            if (body->hasTagName(framesetTag) && m_frame->settings() && !m_frame->settings()->frameFlatteningEnabled()) {
                vMode = ScrollbarAlwaysOff;
                hMode = ScrollbarAlwaysOff;
            } else if (body->hasTagName(bodyTag)) {
                // It's sufficient to just check the X overflow,
                // since it's illegal to have visible in only one direction.
                RenderObject* o = rootRenderer->style()->overflowX() == OVISIBLE && document->documentElement()->hasTagName(htmlTag) ? body->renderer() : rootRenderer;
                applyOverflowToViewport(o, hMode, vMode);
            }
        } else if (rootRenderer) {
#if ENABLE(SVG)
            if (!documentElement->isSVGElement())
                applyOverflowToViewport(rootRenderer, hMode, vMode);
#else
            applyOverflowToViewport(rootRenderer, hMode, vMode);
#endif
        }
    }    
}

#if ENABLE(FULLSCREEN_API) && USE(ACCELERATED_COMPOSITING)
static bool isDocumentRunningFullScreenAnimation(Document* document)
{
    return document->webkitIsFullScreen() && document->fullScreenRenderer() && document->fullScreenRenderer()->isAnimating();
}
#endif
    
#if USE(ACCELERATED_COMPOSITING)
void FrameView::updateCompositingLayers()
{
    RenderView* view = m_frame->contentRenderer();
    if (!view)
        return;

    // This call will make sure the cached hasAcceleratedCompositing is updated from the pref
    view->compositor()->cacheAcceleratedCompositingFlags();
    view->compositor()->updateCompositingLayers(CompositingUpdateAfterLayoutOrStyleChange);
    
#if ENABLE(FULLSCREEN_API)
    Document* document = m_frame->document();
    if (isDocumentRunningFullScreenAnimation(document))
        view->compositor()->updateCompositingLayers(CompositingUpdateAfterLayoutOrStyleChange, document->fullScreenRenderer()->layer());
#endif
}

GraphicsLayer* FrameView::layerForHorizontalScrollbar() const
{
    RenderView* view = m_frame->contentRenderer();
    if (!view)
        return 0;
    return view->compositor()->layerForHorizontalScrollbar();
}

GraphicsLayer* FrameView::layerForVerticalScrollbar() const
{
    RenderView* view = m_frame->contentRenderer();
    if (!view)
        return 0;
    return view->compositor()->layerForVerticalScrollbar();
}

GraphicsLayer* FrameView::layerForScrollCorner() const
{
    RenderView* view = m_frame->contentRenderer();
    if (!view)
        return 0;
    return view->compositor()->layerForScrollCorner();
}

bool FrameView::syncCompositingStateForThisFrame()
{
    ASSERT(m_frame->view() == this);
    RenderView* view = m_frame->contentRenderer();
    if (!view)
        return true; // We don't want to keep trying to update layers if we have no renderer.

    // If we sync compositing layers when a layout is pending, we may cause painting of compositing
    // layer content to occur before layout has happened, which will cause paintContents() to bail.
    if (needsLayout())
        return false;

    if (GraphicsLayer* graphicsLayer = view->compositor()->layerForHorizontalScrollbar())
        graphicsLayer->syncCompositingStateForThisLayerOnly();
    if (GraphicsLayer* graphicsLayer = view->compositor()->layerForVerticalScrollbar())
        graphicsLayer->syncCompositingStateForThisLayerOnly();
    if (GraphicsLayer* graphicsLayer = view->compositor()->layerForScrollCorner())
        graphicsLayer->syncCompositingStateForThisLayerOnly();

    view->compositor()->flushPendingLayerChanges();

#if ENABLE(FULLSCREEN_API)
    // The fullScreenRenderer's graphicsLayer  has been re-parented, and the above recursive syncCompositingState
    // call will not cause the subtree under it to repaint.  Explicitly call the syncCompositingState on 
    // the fullScreenRenderer's graphicsLayer here:
    Document* document = m_frame->document();
    if (isDocumentRunningFullScreenAnimation(document)) {
        RenderLayerBacking* backing = document->fullScreenRenderer()->layer()->backing();
        if (GraphicsLayer* fullScreenLayer = backing->graphicsLayer())
            fullScreenLayer->syncCompositingState();
    }
#endif
    return true;
}

void FrameView::setNeedsOneShotDrawingSynchronization()
{
    Page* page = frame() ? frame()->page() : 0;
    if (page)
        page->chrome()->client()->setNeedsOneShotDrawingSynchronization();
}

#endif // USE(ACCELERATED_COMPOSITING)

bool FrameView::hasCompositedContent() const
{
#if USE(ACCELERATED_COMPOSITING)
    if (RenderView* view = m_frame->contentRenderer())
        return view->compositor()->inCompositingMode();
#endif
    return false;
}

bool FrameView::hasCompositedContentIncludingDescendants() const
{
#if USE(ACCELERATED_COMPOSITING)
    for (Frame* frame = m_frame.get(); frame; frame = frame->tree()->traverseNext(m_frame.get())) {
        RenderView* renderView = frame->contentRenderer();
        RenderLayerCompositor* compositor = renderView ? renderView->compositor() : 0;
        if (compositor) {
            if (compositor->inCompositingMode())
                return true;

            if (!RenderLayerCompositor::allowsIndependentlyCompositedFrames(this))
                break;
        }
    }
#endif
    return false;
}

bool FrameView::hasCompositingAncestor() const
{
#if USE(ACCELERATED_COMPOSITING)
    for (Frame* frame = m_frame->tree()->parent(); frame; frame = frame->tree()->parent()) {
        if (FrameView* view = frame->view()) {
            if (view->hasCompositedContent())
                return true;
        }
    }
#endif
    return false;
}

// Sometimes (for plug-ins) we need to eagerly go into compositing mode.
void FrameView::enterCompositingMode()
{
#if USE(ACCELERATED_COMPOSITING)
    if (RenderView* view = m_frame->contentRenderer()) {
        view->compositor()->enableCompositingMode();
        if (!needsLayout())
            view->compositor()->scheduleCompositingLayerUpdate();
    }
#endif
}

bool FrameView::isEnclosedInCompositingLayer() const
{
#if USE(ACCELERATED_COMPOSITING)
    RenderObject* frameOwnerRenderer = m_frame->ownerRenderer();
    if (frameOwnerRenderer && frameOwnerRenderer->containerForRepaint())
        return true;

    if (FrameView* parentView = parentFrameView())
        return parentView->isEnclosedInCompositingLayer();
#endif
    return false;
}
    
bool FrameView::syncCompositingStateIncludingSubframes()
{
#if USE(ACCELERATED_COMPOSITING)
    bool allFramesSynced = syncCompositingStateForThisFrame();
    
    for (Frame* child = m_frame->tree()->firstChild(); child; child = child->tree()->traverseNext(m_frame.get())) {
        bool synced = child->view()->syncCompositingStateForThisFrame();
        allFramesSynced &= synced;
    }
    return allFramesSynced;
#else // USE(ACCELERATED_COMPOSITING)
    return true;
#endif
}

bool FrameView::isSoftwareRenderable() const
{
#if USE(ACCELERATED_COMPOSITING)
    RenderView* view = m_frame->contentRenderer();
    if (!view)
        return true;

    return !view->compositor()->has3DContent();
#else
    return true;
#endif
}

void FrameView::didMoveOnscreen()
{
    RenderView* view = m_frame->contentRenderer();
    if (view)
        view->didMoveOnscreen();
    scrollAnimator()->contentAreaDidShow();
}

void FrameView::willMoveOffscreen()
{
    RenderView* view = m_frame->contentRenderer();
    if (view)
        view->willMoveOffscreen();
    scrollAnimator()->contentAreaDidHide();
}

RenderObject* FrameView::layoutRoot(bool onlyDuringLayout) const
{
    return onlyDuringLayout && layoutPending() ? 0 : m_layoutRoot;
}

void FrameView::layout(bool allowSubtree)
{
    if (m_inLayout)
        return;

    bool inSubframeLayoutWithFrameFlattening = parent() && m_frame->settings() && m_frame->settings()->frameFlatteningEnabled();

    if (inSubframeLayoutWithFrameFlattening) {
        if (parent()->isFrameView()) {
            FrameView* parentView =   static_cast<FrameView*>(parent());
            if (!parentView->m_nestedLayoutCount) {
                while (parentView->parent() && parentView->parent()->isFrameView())
                    parentView = static_cast<FrameView*>(parentView->parent());
                parentView->layout(allowSubtree);
                return;
            }
        }
    }

    m_layoutTimer.stop();
    m_delayedLayout = false;
    m_setNeedsLayoutWasDeferred = false;

    // Protect the view from being deleted during layout (in recalcStyle)
    RefPtr<FrameView> protector(this);

    if (!m_frame) {
        // FIXME: Do we need to set m_size.width here?
        // FIXME: Should we set m_size.height here too?
        m_size.setWidth(layoutWidth());
        return;
    }
    
    // we shouldn't enter layout() while painting
    ASSERT(!isPainting());
    if (isPainting())
        return;

    InspectorInstrumentationCookie cookie = InspectorInstrumentation::willLayout(m_frame.get());

    if (!allowSubtree && m_layoutRoot) {
        m_layoutRoot->markContainingBlocksForLayout(false);
        m_layoutRoot = 0;
    }

    ASSERT(m_frame->view() == this);

    Document* document = m_frame->document();

    m_layoutSchedulingEnabled = false;

    if (!m_nestedLayoutCount && !m_inSynchronousPostLayout && m_hasPendingPostLayoutTasks && !inSubframeLayoutWithFrameFlattening) {
        // This is a new top-level layout. If there are any remaining tasks from the previous
        // layout, finish them now.
        m_inSynchronousPostLayout = true;
        m_postLayoutTasksTimer.stop();
        performPostLayoutTasks();
        m_inSynchronousPostLayout = false;
    }

    // Viewport-dependent media queries may cause us to need completely different style information.
    // Check that here.
    if (document->styleSelector()->affectedByViewportChange())
        document->styleSelectorChanged(RecalcStyleImmediately);

    // Always ensure our style info is up-to-date.  This can happen in situations where
    // the layout beats any sort of style recalc update that needs to occur.
    document->updateStyleIfNeeded();
    
    bool subtree = m_layoutRoot;

    // If there is only one ref to this view left, then its going to be destroyed as soon as we exit, 
    // so there's no point to continuing to layout
    if (protector->hasOneRef())
        return;

    RenderObject* root = subtree ? m_layoutRoot : document->renderer();
    if (!root) {
        // FIXME: Do we need to set m_size here?
        m_layoutSchedulingEnabled = true;
        return;
    }

    m_nestedLayoutCount++;

    if (!m_layoutRoot) {
        Document* document = m_frame->document();
        Node* documentElement = document->documentElement();
        RenderObject* rootRenderer = documentElement ? documentElement->renderer() : 0;
        Node* body = document->body();
        if (body && body->renderer()) {
            if (body->hasTagName(framesetTag) && m_frame->settings() && !m_frame->settings()->frameFlatteningEnabled()) {
                body->renderer()->setChildNeedsLayout(true);
            } else if (body->hasTagName(bodyTag)) {
                if (!m_firstLayout && m_size.height() != layoutHeight() && body->renderer()->enclosingBox()->stretchesToViewport())
                    body->renderer()->setChildNeedsLayout(true);
            }
        } else if (rootRenderer) {
#if ENABLE(SVG)
            if (documentElement->isSVGElement()) {
                if (!m_firstLayout && (m_size.width() != layoutWidth() || m_size.height() != layoutHeight()))
                    rootRenderer->setChildNeedsLayout(true);
            }
#endif
        }
        
#ifdef INSTRUMENT_LAYOUT_SCHEDULING
        if (m_firstLayout && !m_frame->ownerElement())
            printf("Elapsed time before first layout: %d\n", document->elapsedTime());
#endif        
    }

    ScrollbarMode hMode;
    ScrollbarMode vMode;    
    calculateScrollbarModesForLayout(hMode, vMode);

    m_doFullRepaint = !subtree && (m_firstLayout || toRenderView(root)->printing());

    if (!subtree) {
        // Now set our scrollbar state for the layout.
        ScrollbarMode currentHMode = horizontalScrollbarMode();
        ScrollbarMode currentVMode = verticalScrollbarMode();

        if (m_firstLayout || (hMode != currentHMode || vMode != currentVMode)) {
            if (m_firstLayout) {
                setScrollbarsSuppressed(true);

                m_firstLayout = false;
                m_firstLayoutCallbackPending = true;
                m_lastLayoutSize = IntSize(width(), height());
                m_lastZoomFactor = root->style()->zoom();

                // Set the initial vMode to AlwaysOn if we're auto.
                if (vMode == ScrollbarAuto)
                    setVerticalScrollbarMode(ScrollbarAlwaysOn); // This causes a vertical scrollbar to appear.
                // Set the initial hMode to AlwaysOff if we're auto.
                if (hMode == ScrollbarAuto)
                    setHorizontalScrollbarMode(ScrollbarAlwaysOff); // This causes a horizontal scrollbar to disappear.

                setScrollbarModes(hMode, vMode);
                setScrollbarsSuppressed(false, true);
            } else
                setScrollbarModes(hMode, vMode);
        }

        IntSize oldSize = m_size;

        m_size = IntSize(layoutWidth(), layoutHeight());

        if (oldSize != m_size) {
            m_doFullRepaint = true;
            if (!m_firstLayout) {
                RenderBox* rootRenderer = document->documentElement() ? document->documentElement()->renderBox() : 0;
                RenderBox* bodyRenderer = rootRenderer && document->body() ? document->body()->renderBox() : 0;
                if (bodyRenderer && bodyRenderer->stretchesToViewport())
                    bodyRenderer->setChildNeedsLayout(true);
                else if (rootRenderer && rootRenderer->stretchesToViewport())
                    rootRenderer->setChildNeedsLayout(true);
            }
        }
    }

    RenderLayer* layer = root->enclosingLayer();

    m_actionScheduler->pause();

    bool disableLayoutState = false;
    if (subtree) {
        RenderView* view = root->view();
        disableLayoutState = view->shouldDisableLayoutStateForSubtree(root);
        view->pushLayoutState(root);
        if (disableLayoutState)
            view->disableLayoutState();
    }
        
    m_inLayout = true;
    beginDeferredRepaints();
    root->layout();
    endDeferredRepaints();
    m_inLayout = false;

    if (subtree) {
        RenderView* view = root->view();
        view->popLayoutState(root);
        if (disableLayoutState)
            view->enableLayoutState();
    }
    m_layoutRoot = 0;

    m_layoutSchedulingEnabled = true;

    if (!subtree && !toRenderView(root)->printing())
        adjustViewSize();

    // Now update the positions of all layers.
    beginDeferredRepaints();
    IntPoint cachedOffset;
    if (m_doFullRepaint)
        root->view()->repaint(); // FIXME: This isn't really right, since the RenderView doesn't fully encompass the visibleContentRect(). It just happens
                                 // to work out most of the time, since first layouts and printing don't have you scrolled anywhere.
    layer->updateLayerPositions((m_doFullRepaint ? 0 : RenderLayer::CheckForRepaint)
                                | RenderLayer::IsCompositingUpdateRoot
                                | RenderLayer::UpdateCompositingLayers,
                                subtree ? 0 : &cachedOffset);
    endDeferredRepaints();

#if USE(ACCELERATED_COMPOSITING)
    updateCompositingLayers();
#endif
    
    m_layoutCount++;

#if PLATFORM(MAC) || PLATFORM(CHROMIUM)
    if (AXObjectCache::accessibilityEnabled())
        root->document()->axObjectCache()->postNotification(root, AXObjectCache::AXLayoutComplete, true);
#endif
#if ENABLE(DASHBOARD_SUPPORT)
    updateDashboardRegions();
#endif

    ASSERT(!root->needsLayout());

    updateCanBlitOnScrollRecursively();

    if (document->hasListenerType(Document::OVERFLOWCHANGED_LISTENER))
        updateOverflowStatus(layoutWidth() < contentsWidth(),
                             layoutHeight() < contentsHeight());

    if (!m_hasPendingPostLayoutTasks) {
        if (!m_inSynchronousPostLayout) {
            if (inSubframeLayoutWithFrameFlattening)
                m_frame->contentRenderer()->updateWidgetPositions();
            else {
                m_inSynchronousPostLayout = true;
                // Calls resumeScheduledEvents()
                performPostLayoutTasks();
                m_inSynchronousPostLayout = false;
            }
        }
        
        if (!m_hasPendingPostLayoutTasks && (needsLayout() || m_inSynchronousPostLayout || inSubframeLayoutWithFrameFlattening)) {
            // If we need layout or are already in a synchronous call to postLayoutTasks(), 
            // defer widget updates and event dispatch until after we return. postLayoutTasks()
            // can make us need to update again, and we can get stuck in a nasty cycle unless
            // we call it through the timer here.
            m_hasPendingPostLayoutTasks = true;
            m_postLayoutTasksTimer.startOneShot(0);
            if (needsLayout()) {
                m_actionScheduler->pause();
                layout();
            }
        }
    } else {
        m_actionScheduler->resume();
    }

    InspectorInstrumentation::didLayout(cookie);

    m_nestedLayoutCount--;
}

void FrameView::addWidgetToUpdate(RenderEmbeddedObject* object)
{
    if (!m_widgetUpdateSet)
        m_widgetUpdateSet = adoptPtr(new RenderEmbeddedObjectSet);

    m_widgetUpdateSet->add(object);
}

void FrameView::removeWidgetToUpdate(RenderEmbeddedObject* object)
{
    if (!m_widgetUpdateSet)
        return;

    m_widgetUpdateSet->remove(object);
}

void FrameView::setMediaType(const String& mediaType)
{
    m_mediaType = mediaType;
}

String FrameView::mediaType() const
{
    // See if we have an override type.
    String overrideType = m_frame->loader()->client()->overrideMediaType();
    if (!overrideType.isNull())
        return overrideType;
    return m_mediaType;
}

void FrameView::adjustMediaTypeForPrinting(bool printing)
{
    if (printing) {
        if (m_mediaTypeWhenNotPrinting.isNull())
            m_mediaTypeWhenNotPrinting = mediaType();
            setMediaType("print");
    } else {
        if (!m_mediaTypeWhenNotPrinting.isNull())
            setMediaType(m_mediaTypeWhenNotPrinting);
        m_mediaTypeWhenNotPrinting = String();
    }
}

bool FrameView::useSlowRepaints() const
{
    if (m_useSlowRepaints || m_slowRepaintObjectCount > 0 || (platformWidget() && m_fixedObjectCount > 0) || m_isOverlapped || !m_contentIsOpaque)
        return true;

    if (FrameView* parentView = parentFrameView())
        return parentView->useSlowRepaints();

    return false;
}

bool FrameView::useSlowRepaintsIfNotOverlapped() const
{
    if (m_useSlowRepaints || m_slowRepaintObjectCount > 0 || (platformWidget() && m_fixedObjectCount > 0) || !m_contentIsOpaque)
        return true;

    if (FrameView* parentView = parentFrameView())
        return parentView->useSlowRepaintsIfNotOverlapped();

    return false;
}

void FrameView::updateCanBlitOnScrollRecursively()
{
    for (Frame* frame = m_frame.get(); frame; frame = frame->tree()->traverseNext(m_frame.get())) {
        if (FrameView* view = frame->view())
            view->setCanBlitOnScroll(!view->useSlowRepaints());
    }
}

void FrameView::setUseSlowRepaints()
{
    m_useSlowRepaints = true;
    updateCanBlitOnScrollRecursively();
}

void FrameView::addSlowRepaintObject()
{
    if (!m_slowRepaintObjectCount)
        updateCanBlitOnScrollRecursively();
    m_slowRepaintObjectCount++;
}

void FrameView::removeSlowRepaintObject()
{
    ASSERT(m_slowRepaintObjectCount > 0);
    m_slowRepaintObjectCount--;
    if (!m_slowRepaintObjectCount)
        updateCanBlitOnScrollRecursively();
}

void FrameView::addFixedObject()
{
    if (!m_fixedObjectCount && platformWidget())
        updateCanBlitOnScrollRecursively();
    ++m_fixedObjectCount;
}

void FrameView::removeFixedObject()
{
    ASSERT(m_fixedObjectCount > 0);
    --m_fixedObjectCount;
    if (!m_fixedObjectCount)
        updateCanBlitOnScrollRecursively();
}

int FrameView::scrollXForFixedPosition() const
{
    int visibleContentWidth = visibleContentRect().width();
    int maxX = contentsWidth() - visibleContentWidth;

    if (maxX == 0)
        return 0;

    int x = scrollX();

    if (!ScrollView::scrollOrigin().x()) {
        if (x < 0)
            x = 0;
        else if (x > maxX)
            x = maxX;
    } else {
        if (x > 0)
            x = 0;
        else if (x < -maxX)
            x = -maxX;
    }

    if (!m_frame)
        return x;

    float pageScaleFactor = m_frame->pageScaleFactor();

    // When the page is scaled, the scaled "viewport" with respect to which fixed object are positioned
    // doesn't move as fast as the content view, so that when the content is scrolled all the way to the
    // end, the bottom of the scaled "viewport" touches the bottom of the real viewport.
    float dragFactor = (contentsWidth() - visibleContentWidth * pageScaleFactor) / maxX;

    return x * dragFactor / pageScaleFactor;
}

int FrameView::scrollYForFixedPosition() const
{
    int visibleContentHeight = visibleContentRect().height();

    int maxY = contentsHeight() - visibleContentHeight;
    if (maxY == 0)
        return 0;

    int y = scrollY();

    if (!ScrollView::scrollOrigin().y()) {
        if (y < 0)
            y = 0;
        else if (y > maxY)
            y = maxY;
    } else {
        if (y > 0)
            y = 0;
        else if (y < -maxY)
            y = -maxY;
    }

    if (!m_frame)
        return y;

    float pageScaleFactor = m_frame->pageScaleFactor();
    float dragFactor = (contentsHeight() - visibleContentHeight * pageScaleFactor) / maxY;

    return y * dragFactor / pageScaleFactor;
}

IntSize FrameView::scrollOffsetForFixedPosition() const
{
    return IntSize(scrollXForFixedPosition(), scrollYForFixedPosition());
}

IntPoint FrameView::currentMousePosition() const
{
    return m_frame ? m_frame->eventHandler()->currentMousePosition() : IntPoint();
}

bool FrameView::scrollContentsFastPath(const IntSize& scrollDelta, const IntRect& rectToScroll, const IntRect& clipRect)
{
    const size_t fixedObjectThreshold = 5;

    RenderBlock::PositionedObjectsListHashSet* positionedObjects = 0;
    if (RenderView* root = m_frame->contentRenderer())
        positionedObjects = root->positionedObjects();

    if (!positionedObjects || positionedObjects->isEmpty()) {
        hostWindow()->scroll(scrollDelta, rectToScroll, clipRect);
        return true;
    }

    // Get the rects of the fixed objects visible in the rectToScroll
    Vector<IntRect, fixedObjectThreshold> subRectToUpdate;
    bool updateInvalidatedSubRect = true;
    RenderBlock::PositionedObjectsListHashSet::const_iterator end = positionedObjects->end();
    for (RenderBlock::PositionedObjectsListHashSet::const_iterator it = positionedObjects->begin(); it != end; ++it) {
        RenderBox* renderBox = *it;
        if (renderBox->style()->position() != FixedPosition)
            continue;
        IntRect updateRect = renderBox->layer()->repaintRectIncludingDescendants();
        updateRect = contentsToWindow(updateRect);
        if (clipsRepaints())
            updateRect.intersect(rectToScroll);
        if (!updateRect.isEmpty()) {
            if (subRectToUpdate.size() >= fixedObjectThreshold) {
                updateInvalidatedSubRect = false;
                break;
            }
            subRectToUpdate.append(updateRect);
        }
    }

    // Scroll the view
    if (updateInvalidatedSubRect) {
        // 1) scroll
        hostWindow()->scroll(scrollDelta, rectToScroll, clipRect);

        // 2) update the area of fixed objects that has been invalidated
        size_t fixObjectsCount = subRectToUpdate.size();
        for (size_t i = 0; i < fixObjectsCount; ++i) {
            IntRect updateRect = subRectToUpdate[i];
            IntRect scrolledRect = updateRect;
            scrolledRect.move(scrollDelta);
            updateRect.unite(scrolledRect);
            if (clipsRepaints())
                updateRect.intersect(rectToScroll);
            hostWindow()->invalidateContentsAndWindow(updateRect, false);
        }
        return true;
    }

    // the number of fixed objects exceed the threshold, we cannot use the fast path
    return false;
}

void FrameView::scrollContentsSlowPath(const IntRect& updateRect)
{
#if USE(ACCELERATED_COMPOSITING)
    if (RenderPart* frameRenderer = m_frame->ownerRenderer()) {
        if (frameRenderer->containerForRepaint()) {
            IntRect rect(frameRenderer->borderLeft() + frameRenderer->paddingLeft(),
                         frameRenderer->borderTop() + frameRenderer->paddingTop(),
                         visibleWidth(), visibleHeight());
            frameRenderer->repaintRectangle(rect);
            return;
        }
    }
#endif

    ScrollView::scrollContentsSlowPath(updateRect);
}

// Note that this gets called at painting time.
void FrameView::setIsOverlapped(bool isOverlapped)
{
    if (isOverlapped == m_isOverlapped)
        return;

    m_isOverlapped = isOverlapped;
    updateCanBlitOnScrollRecursively();
    
#if USE(ACCELERATED_COMPOSITING)
    if (hasCompositedContentIncludingDescendants()) {
        // Overlap can affect compositing tests, so if it changes, we need to trigger
        // a layer update in the parent document.
        if (Frame* parentFrame = m_frame->tree()->parent()) {
            if (RenderView* parentView = parentFrame->contentRenderer()) {
                RenderLayerCompositor* compositor = parentView->compositor();
                compositor->setCompositingLayersNeedRebuild();
                compositor->scheduleCompositingLayerUpdate();
            }
        }

        if (RenderLayerCompositor::allowsIndependentlyCompositedFrames(this)) {
            // We also need to trigger reevaluation for this and all descendant frames,
            // since a frame uses compositing if any ancestor is compositing.
            for (Frame* frame = m_frame.get(); frame; frame = frame->tree()->traverseNext(m_frame.get())) {
                if (RenderView* view = frame->contentRenderer()) {
                    RenderLayerCompositor* compositor = view->compositor();
                    compositor->setCompositingLayersNeedRebuild();
                    compositor->scheduleCompositingLayerUpdate();
                }
            }
        }
    }
#endif
}

bool FrameView::isOverlappedIncludingAncestors() const
{
    if (isOverlapped())
        return true;

    if (FrameView* parentView = parentFrameView()) {
        if (parentView->isOverlapped())
            return true;
    }

    return false;
}

void FrameView::setContentIsOpaque(bool contentIsOpaque)
{
    if (contentIsOpaque == m_contentIsOpaque)
        return;

    m_contentIsOpaque = contentIsOpaque;
    updateCanBlitOnScrollRecursively();
}

void FrameView::restoreScrollbar()
{
    setScrollbarsSuppressed(false);
}

bool FrameView::scrollToFragment(const KURL& url)
{
    // If our URL has no ref, then we have no place we need to jump to.
    // OTOH If CSS target was set previously, we want to set it to 0, recalc
    // and possibly repaint because :target pseudo class may have been
    // set (see bug 11321).
    if (!url.hasFragmentIdentifier() && !m_frame->document()->cssTarget())
        return false;

    String fragmentIdentifier = url.fragmentIdentifier();
    if (scrollToAnchor(fragmentIdentifier))
        return true;

    // Try again after decoding the ref, based on the document's encoding.
    if (TextResourceDecoder* decoder = m_frame->document()->decoder())
        return scrollToAnchor(decodeURLEscapeSequences(fragmentIdentifier, decoder->encoding()));

    return false;
}

bool FrameView::scrollToAnchor(const String& name)
{
    ASSERT(m_frame->document());

    if (!m_frame->document()->haveStylesheetsLoaded()) {
        m_frame->document()->setGotoAnchorNeededAfterStylesheetsLoad(true);
        return false;
    }

    m_frame->document()->setGotoAnchorNeededAfterStylesheetsLoad(false);

    Element* anchorNode = m_frame->document()->findAnchor(name);

#if ENABLE(SVG)
    if (m_frame->document()->isSVGDocument()) {
        if (name.startsWith("xpointer(")) {
            // We need to parse the xpointer reference here
        } else if (name.startsWith("svgView(")) {
            RefPtr<SVGSVGElement> svg = static_cast<SVGDocument*>(m_frame->document())->rootElement();
            if (!svg->currentView()->parseViewSpec(name))
                return false;
            svg->setUseCurrentView(true);
        } else {
            if (anchorNode && anchorNode->hasTagName(SVGNames::viewTag)) {
                RefPtr<SVGViewElement> viewElement = anchorNode->hasTagName(SVGNames::viewTag) ? static_cast<SVGViewElement*>(anchorNode) : 0;
                if (viewElement.get()) {
                    SVGElement* element = SVGLocatable::nearestViewportElement(viewElement.get());
                    if (element->hasTagName(SVGNames::svgTag)) {
                        RefPtr<SVGSVGElement> svg = static_cast<SVGSVGElement*>(element);
                        svg->inheritViewAttributes(viewElement.get());
                    }
                }
            }
        }
        // FIXME: need to decide which <svg> to focus on, and zoom to that one
        // FIXME: need to actually "highlight" the viewTarget(s)
    }
#endif

    m_frame->document()->setCSSTarget(anchorNode); // Setting to null will clear the current target.
  
    // Implement the rule that "" and "top" both mean top of page as in other browsers.
    if (!anchorNode && !(name.isEmpty() || equalIgnoringCase(name, "top")))
        return false;

    maintainScrollPositionAtAnchor(anchorNode ? static_cast<Node*>(anchorNode) : m_frame->document());
    return true;
}

void FrameView::maintainScrollPositionAtAnchor(Node* anchorNode)
{
    m_maintainScrollPositionAnchor = anchorNode;
    if (!m_maintainScrollPositionAnchor)
        return;

    // We need to update the layout before scrolling, otherwise we could
    // really mess things up if an anchor scroll comes at a bad moment.
    m_frame->document()->updateStyleIfNeeded();
    // Only do a layout if changes have occurred that make it necessary.
    if (m_frame->contentRenderer() && m_frame->contentRenderer()->needsLayout())
        layout();
    else
        scrollToAnchor();
}

void FrameView::setScrollPosition(const IntPoint& scrollPoint)
{
    bool wasInProgrammaticScroll = m_inProgrammaticScroll;
    m_inProgrammaticScroll = true;
    m_maintainScrollPositionAnchor = 0;
    ScrollView::setScrollPosition(scrollPoint);
    m_inProgrammaticScroll = wasInProgrammaticScroll;
}

void FrameView::scrollPositionChangedViaPlatformWidget()
{
    repaintFixedElementsAfterScrolling();
    scrollPositionChanged();
}

void FrameView::scrollPositionChanged()
{
    frame()->eventHandler()->sendScrollEvent();

#if USE(ACCELERATED_COMPOSITING)
    if (RenderView* root = m_frame->contentRenderer()) {
        if (root->usesCompositing())
            root->compositor()->frameViewDidScroll(scrollPosition());
    }
#endif
}

void FrameView::repaintFixedElementsAfterScrolling()
{
    // For fixed position elements, update widget positions and compositing layers after scrolling,
    // but only if we're not inside of layout.
    if (!m_nestedLayoutCount && hasFixedObjects()) {
        if (RenderView* root = m_frame->contentRenderer()) {
            root->updateWidgetPositions();
            root->layer()->updateRepaintRectsAfterScroll();
#if USE(ACCELERATED_COMPOSITING)
            root->compositor()->updateCompositingLayers(CompositingUpdateOnScroll);
#endif
        }
    }
}

HostWindow* FrameView::hostWindow() const
{
    Page* page = frame() ? frame()->page() : 0;
    if (!page)
        return 0;
    return page->chrome();
}

const unsigned cRepaintRectUnionThreshold = 25;

void FrameView::repaintContentRectangle(const IntRect& r, bool immediate)
{
    ASSERT(!m_frame->ownerElement());

    double delay = m_deferringRepaints ? 0 : adjustedDeferredRepaintDelay();
    if ((m_deferringRepaints || m_deferredRepaintTimer.isActive() || delay) && !immediate) {
        IntRect paintRect = r;
        if (clipsRepaints() && !paintsEntireContents())
            paintRect.intersect(visibleContentRect());
        if (paintRect.isEmpty())
            return;
        if (m_repaintCount == cRepaintRectUnionThreshold) {
            IntRect unionedRect;
            for (unsigned i = 0; i < cRepaintRectUnionThreshold; ++i)
                unionedRect.unite(m_repaintRects[i]);
            m_repaintRects.clear();
            m_repaintRects.append(unionedRect);
        }
        if (m_repaintCount < cRepaintRectUnionThreshold)
            m_repaintRects.append(paintRect);
        else
            m_repaintRects[0].unite(paintRect);
        m_repaintCount++;
    
        if (!m_deferringRepaints && !m_deferredRepaintTimer.isActive())
             m_deferredRepaintTimer.startOneShot(delay);
        return;
    }
    
    if (!shouldUpdate(immediate))
        return;

#if ENABLE(TILED_BACKING_STORE)
    if (frame()->tiledBackingStore()) {
        frame()->tiledBackingStore()->invalidate(r);
        return;
    }
#endif
    ScrollView::repaintContentRectangle(r, immediate);
}

void FrameView::contentsResized()
{
    scrollAnimator()->contentsResized();
    setNeedsLayout();
}

void FrameView::visibleContentsResized()
{
    // We check to make sure the view is attached to a frame() as this method can
    // be triggered before the view is attached by Frame::createView(...) setting
    // various values such as setScrollBarModes(...) for example.  An ASSERT is
    // triggered when a view is layout before being attached to a frame().
    if (!frame()->view())
        return;

    if (needsLayout())
        layout();

#if USE(ACCELERATED_COMPOSITING)
    if (RenderView* root = m_frame->contentRenderer()) {
        if (root->usesCompositing())
            root->compositor()->frameViewDidChangeSize();
    }
#endif
}

void FrameView::beginDeferredRepaints()
{
    Page* page = m_frame->page();
    if (page->mainFrame() != m_frame)
        return page->mainFrame()->view()->beginDeferredRepaints();

    m_deferringRepaints++;
}


void FrameView::endDeferredRepaints()
{
    Page* page = m_frame->page();
    if (page->mainFrame() != m_frame)
        return page->mainFrame()->view()->endDeferredRepaints();

    ASSERT(m_deferringRepaints > 0);

    if (--m_deferringRepaints)
        return;
    
    if (m_deferredRepaintTimer.isActive())
        return;

    if (double delay = adjustedDeferredRepaintDelay()) {
        m_deferredRepaintTimer.startOneShot(delay);
        return;
    }
    
    doDeferredRepaints();
}

void FrameView::checkStopDelayingDeferredRepaints()
{
    if (!m_deferredRepaintTimer.isActive())
        return;

    Document* document = m_frame->document();
    if (document && (document->parsing() || document->cachedResourceLoader()->requestCount()))
        return;
    
    m_deferredRepaintTimer.stop();

    doDeferredRepaints();
}
    
void FrameView::doDeferredRepaints()
{
    ASSERT(!m_deferringRepaints);
    if (!shouldUpdate()) {
        m_repaintRects.clear();
        m_repaintCount = 0;
        return;
    }
    unsigned size = m_repaintRects.size();
    for (unsigned i = 0; i < size; i++) {
#if ENABLE(TILED_BACKING_STORE)
        if (frame()->tiledBackingStore()) {
            frame()->tiledBackingStore()->invalidate(m_repaintRects[i]);
            continue;
        }
#endif
        ScrollView::repaintContentRectangle(m_repaintRects[i], false);
    }
    m_repaintRects.clear();
    m_repaintCount = 0;
    
    updateDeferredRepaintDelay();
}

void FrameView::updateDeferredRepaintDelay()
{
    Document* document = m_frame->document();
    if (!document || (!document->parsing() && !document->cachedResourceLoader()->requestCount())) {
        m_deferredRepaintDelay = s_deferredRepaintDelay;
        return;
    }
    if (m_deferredRepaintDelay < s_maxDeferredRepaintDelayDuringLoading) {
        m_deferredRepaintDelay += s_deferredRepaintDelayIncrementDuringLoading;
        if (m_deferredRepaintDelay > s_maxDeferredRepaintDelayDuringLoading)
            m_deferredRepaintDelay = s_maxDeferredRepaintDelayDuringLoading;
    }
}

void FrameView::resetDeferredRepaintDelay()
{
    m_deferredRepaintDelay = 0;
    if (m_deferredRepaintTimer.isActive()) {
        m_deferredRepaintTimer.stop();
        if (!m_deferringRepaints)
            doDeferredRepaints();
    }
}

double FrameView::adjustedDeferredRepaintDelay() const
{
    ASSERT(!m_deferringRepaints);
    if (!m_deferredRepaintDelay)
        return 0;
    double timeSinceLastPaint = currentTime() - m_lastPaintTime;
    return max(0., m_deferredRepaintDelay - timeSinceLastPaint);
}
    
void FrameView::deferredRepaintTimerFired(Timer<FrameView>*)
{
    doDeferredRepaints();
}    

void FrameView::layoutTimerFired(Timer<FrameView>*)
{
#ifdef INSTRUMENT_LAYOUT_SCHEDULING
    if (!m_frame->document()->ownerElement())
        printf("Layout timer fired at %d\n", m_frame->document()->elapsedTime());
#endif
    layout();
}

void FrameView::scheduleRelayout()
{
    // FIXME: We should assert the page is not in the page cache, but that is causing
    // too many false assertions.  See <rdar://problem/7218118>.
    ASSERT(m_frame->view() == this);

    if (m_layoutRoot) {
        m_layoutRoot->markContainingBlocksForLayout(false);
        m_layoutRoot = 0;
    }
    if (!m_layoutSchedulingEnabled)
        return;
    if (!needsLayout())
        return;
    if (!m_frame->document()->shouldScheduleLayout())
        return;

    // When frame flattening is enabled, the contents of the frame affects layout of the parent frames.
    // Also invalidate parent frame starting from the owner element of this frame.
    if (m_frame->settings() && m_frame->settings()->frameFlatteningEnabled() && m_frame->ownerRenderer()) {
        if (m_frame->ownerElement()->hasTagName(iframeTag) || m_frame->ownerElement()->hasTagName(frameTag))
            m_frame->ownerRenderer()->setNeedsLayout(true, true);
    }

    int delay = m_frame->document()->minimumLayoutDelay();
    if (m_layoutTimer.isActive() && m_delayedLayout && !delay)
        unscheduleRelayout();
    if (m_layoutTimer.isActive())
        return;

    m_delayedLayout = delay != 0;

#ifdef INSTRUMENT_LAYOUT_SCHEDULING
    if (!m_frame->document()->ownerElement())
        printf("Scheduling layout for %d\n", delay);
#endif

    m_layoutTimer.startOneShot(delay * 0.001);
}

static bool isObjectAncestorContainerOf(RenderObject* ancestor, RenderObject* descendant)
{
    for (RenderObject* r = descendant; r; r = r->container()) {
        if (r == ancestor)
            return true;
    }
    return false;
}

void FrameView::scheduleRelayoutOfSubtree(RenderObject* relayoutRoot)
{
    ASSERT(m_frame->view() == this);

    if (m_frame->contentRenderer() && m_frame->contentRenderer()->needsLayout()) {
        if (relayoutRoot)
            relayoutRoot->markContainingBlocksForLayout(false);
        return;
    }

    if (layoutPending() || !m_layoutSchedulingEnabled) {
        if (m_layoutRoot != relayoutRoot) {
            if (isObjectAncestorContainerOf(m_layoutRoot, relayoutRoot)) {
                // Keep the current root
                relayoutRoot->markContainingBlocksForLayout(false, m_layoutRoot);
                ASSERT(!m_layoutRoot->container() || !m_layoutRoot->container()->needsLayout());
            } else if (m_layoutRoot && isObjectAncestorContainerOf(relayoutRoot, m_layoutRoot)) {
                // Re-root at relayoutRoot
                m_layoutRoot->markContainingBlocksForLayout(false, relayoutRoot);
                m_layoutRoot = relayoutRoot;
                ASSERT(!m_layoutRoot->container() || !m_layoutRoot->container()->needsLayout());
            } else {
                // Just do a full relayout
                if (m_layoutRoot)
                    m_layoutRoot->markContainingBlocksForLayout(false);
                m_layoutRoot = 0;
                relayoutRoot->markContainingBlocksForLayout(false);
            }
        }
    } else if (m_layoutSchedulingEnabled) {
        int delay = m_frame->document()->minimumLayoutDelay();
        m_layoutRoot = relayoutRoot;
        ASSERT(!m_layoutRoot->container() || !m_layoutRoot->container()->needsLayout());
        m_delayedLayout = delay != 0;
        m_layoutTimer.startOneShot(delay * 0.001);
    }
}

bool FrameView::layoutPending() const
{
    return m_layoutTimer.isActive();
}

bool FrameView::needsLayout() const
{
    // This can return true in cases where the document does not have a body yet.
    // Document::shouldScheduleLayout takes care of preventing us from scheduling
    // layout in that case.
    if (!m_frame)
        return false;
    RenderView* root = m_frame->contentRenderer();
    return layoutPending()
        || (root && root->needsLayout())
        || m_layoutRoot
        || (m_deferSetNeedsLayouts && m_setNeedsLayoutWasDeferred);
}

void FrameView::setNeedsLayout()
{
    if (m_deferSetNeedsLayouts) {
        m_setNeedsLayoutWasDeferred = true;
        return;
    }
    RenderView* root = m_frame->contentRenderer();
    if (root)
        root->setNeedsLayout(true);
}

void FrameView::unscheduleRelayout()
{
    m_postLayoutTasksTimer.stop();

    if (!m_layoutTimer.isActive())
        return;

#ifdef INSTRUMENT_LAYOUT_SCHEDULING
    if (!m_frame->document()->ownerElement())
        printf("Layout timer unscheduled at %d\n", m_frame->document()->elapsedTime());
#endif
    
    m_layoutTimer.stop();
    m_delayedLayout = false;
}

#if ENABLE(REQUEST_ANIMATION_FRAME)
void FrameView::serviceScriptedAnimations(DOMTimeStamp time)
{
    for (Frame* frame = m_frame.get(); frame; frame = frame->tree()->traverseNext())
        frame->document()->serviceScriptedAnimations(time);
}
#endif

bool FrameView::isTransparent() const
{
    return m_isTransparent;
}

void FrameView::setTransparent(bool isTransparent)
{
    m_isTransparent = isTransparent;
}

Color FrameView::baseBackgroundColor() const
{
    return m_baseBackgroundColor;
}

void FrameView::setBaseBackgroundColor(const Color& backgroundColor)
{
    if (!backgroundColor.isValid())
        m_baseBackgroundColor = Color::white;
    else
        m_baseBackgroundColor = backgroundColor;
}

void FrameView::updateBackgroundRecursively(const Color& backgroundColor, bool transparent)
{
    for (Frame* frame = m_frame.get(); frame; frame = frame->tree()->traverseNext(m_frame.get())) {
        if (FrameView* view = frame->view()) {
            view->setTransparent(transparent);
            view->setBaseBackgroundColor(backgroundColor);
        }
    }
}

bool FrameView::shouldUpdateWhileOffscreen() const
{
    return m_shouldUpdateWhileOffscreen;
}

void FrameView::setShouldUpdateWhileOffscreen(bool shouldUpdateWhileOffscreen)
{
    m_shouldUpdateWhileOffscreen = shouldUpdateWhileOffscreen;
}

bool FrameView::shouldUpdate(bool immediateRequested) const
{
    if (!immediateRequested && isOffscreen() && !shouldUpdateWhileOffscreen())
        return false;
    return true;
}

void FrameView::scheduleEvent(PassRefPtr<Event> event, PassRefPtr<Node> eventTarget)
{
    m_actionScheduler->scheduleEvent(event, eventTarget);
}

void FrameView::pauseScheduledEvents()
{
    m_actionScheduler->pause();
}

void FrameView::resumeScheduledEvents()
{
    m_actionScheduler->resume();
}

void FrameView::scrollToAnchor()
{
    RefPtr<Node> anchorNode = m_maintainScrollPositionAnchor;
    if (!anchorNode)
        return;

    if (!anchorNode->renderer())
        return;

    IntRect rect;
    if (anchorNode != m_frame->document())
        rect = anchorNode->getRect();

    // Scroll nested layers and frames to reveal the anchor.
    // Align to the top and to the closest side (this matches other browsers).
    anchorNode->renderer()->enclosingLayer()->scrollRectToVisible(rect, true, ScrollAlignment::alignToEdgeIfNeeded, ScrollAlignment::alignTopAlways);

    if (AXObjectCache::accessibilityEnabled())
        m_frame->document()->axObjectCache()->handleScrolledToAnchor(anchorNode.get());

    // scrollRectToVisible can call into setScrollPosition(), which resets m_maintainScrollPositionAnchor.
    m_maintainScrollPositionAnchor = anchorNode;
}

void FrameView::updateWidget(RenderEmbeddedObject* object)
{
    ASSERT(!object->node() || object->node()->isElementNode());
    Element* ownerElement = static_cast<Element*>(object->node());
    // The object may have already been destroyed (thus node cleared),
    // but FrameView holds a manual ref, so it won't have been deleted.
    ASSERT(m_widgetUpdateSet->contains(object));
    if (!ownerElement)
        return;

    // No need to update if it's already crashed or known to be missing.
    if (object->pluginCrashedOrWasMissing())
        return;

    // FIXME: This could turn into a real virtual dispatch if we defined
    // updateWidget(bool) on HTMLElement.
    if (ownerElement->hasTagName(objectTag) || ownerElement->hasTagName(embedTag))
        static_cast<HTMLPlugInImageElement*>(ownerElement)->updateWidget(CreateAnyWidgetType);
    // FIXME: It is not clear that Media elements need or want this updateWidget() call.
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    else if (ownerElement->isMediaElement())
        static_cast<HTMLMediaElement*>(ownerElement)->updateWidget(CreateAnyWidgetType);
#endif
    else
        ASSERT_NOT_REACHED();

    // Caution: it's possible the object was destroyed again, since loading a
    // plugin may run any arbitrary javascript.
    object->updateWidgetPosition();
}

bool FrameView::updateWidgets()
{
    if (m_nestedLayoutCount > 1 || !m_widgetUpdateSet || m_widgetUpdateSet->isEmpty())
        return true;
    
    size_t size = m_widgetUpdateSet->size();

    Vector<RenderEmbeddedObject*> objects;
    objects.reserveCapacity(size);

    RenderEmbeddedObjectSet::const_iterator end = m_widgetUpdateSet->end();
    for (RenderEmbeddedObjectSet::const_iterator it = m_widgetUpdateSet->begin(); it != end; ++it) {
        objects.uncheckedAppend(*it);
        (*it)->ref();
    }

    for (size_t i = 0; i < size; ++i) {
        RenderEmbeddedObject* object = objects[i];
        updateWidget(object);
        m_widgetUpdateSet->remove(object);
    }

    RenderArena* arena = m_frame->document()->renderArena();
    for (size_t i = 0; i < size; ++i)
        objects[i]->deref(arena);
    
    return m_widgetUpdateSet->isEmpty();
}

void FrameView::flushAnyPendingPostLayoutTasks()
{
    if (!m_hasPendingPostLayoutTasks)
        return;

    m_postLayoutTasksTimer.stop();
    performPostLayoutTasks();
}

void FrameView::performPostLayoutTasks()
{
    m_hasPendingPostLayoutTasks = false;

    m_frame->selection()->setCaretRectNeedsUpdate();
    m_frame->selection()->updateAppearance();

    if (m_nestedLayoutCount <= 1) {
        if (m_firstLayoutCallbackPending) {
            m_firstLayoutCallbackPending = false;
            m_frame->loader()->didFirstLayout();
        }
        
        if (m_isVisuallyNonEmpty && m_firstVisuallyNonEmptyLayoutCallbackPending) {
            m_firstVisuallyNonEmptyLayoutCallbackPending = false;
            m_frame->loader()->didFirstVisuallyNonEmptyLayout();
        }
    }

    RenderView* root = m_frame->contentRenderer();

    root->updateWidgetPositions();
    
    for (unsigned i = 0; i < maxUpdateWidgetsIterations; i++) {
        if (updateWidgets())
            break;
    }

    scrollToAnchor();

    m_actionScheduler->resume();

    if (!root->printing()) {
        IntSize currentSize = IntSize(width(), height());
        float currentZoomFactor = root->style()->zoom();
        bool resized = !m_firstLayout && (currentSize != m_lastLayoutSize || currentZoomFactor != m_lastZoomFactor);
        m_lastLayoutSize = currentSize;
        m_lastZoomFactor = currentZoomFactor;
        if (resized)
            m_frame->eventHandler()->sendResizeEvent();
    }
}

void FrameView::postLayoutTimerFired(Timer<FrameView>*)
{
    performPostLayoutTasks();
}

void FrameView::updateOverflowStatus(bool horizontalOverflow, bool verticalOverflow)
{
    if (!m_viewportRenderer)
        return;
    
    if (m_overflowStatusDirty) {
        m_horizontalOverflow = horizontalOverflow;
        m_verticalOverflow = verticalOverflow;
        m_overflowStatusDirty = false;
        return;
    }
    
    bool horizontalOverflowChanged = (m_horizontalOverflow != horizontalOverflow);
    bool verticalOverflowChanged = (m_verticalOverflow != verticalOverflow);
    
    if (horizontalOverflowChanged || verticalOverflowChanged) {
        m_horizontalOverflow = horizontalOverflow;
        m_verticalOverflow = verticalOverflow;
        
        m_actionScheduler->scheduleEvent(OverflowEvent::create(horizontalOverflowChanged, horizontalOverflow,
            verticalOverflowChanged, verticalOverflow),
            m_viewportRenderer->node());
    }
    
}

IntRect FrameView::windowClipRect(bool clipToContents) const
{
    ASSERT(m_frame->view() == this);

    if (paintsEntireContents())
        return IntRect(IntPoint(0, 0), contentsSize());

    // Set our clip rect to be our contents.
    IntRect clipRect = contentsToWindow(visibleContentRect(!clipToContents));
    if (!m_frame || !m_frame->ownerElement())
        return clipRect;

    // Take our owner element and get the clip rect from the enclosing layer.
    Element* elt = m_frame->ownerElement();
    // The renderer can sometimes be null when style="display:none" interacts
    // with external content and plugins.
    RenderLayer* layer = elt->renderer() ? elt->renderer()->enclosingLayer() : 0;
    if (!layer)
        return clipRect;
    FrameView* parentView = elt->document()->view();
    clipRect.intersect(parentView->windowClipRectForLayer(layer, true));
    return clipRect;
}

IntRect FrameView::windowClipRectForLayer(const RenderLayer* layer, bool clipToLayerContents) const
{
    // If we have no layer, just return our window clip rect.
    if (!layer)
        return windowClipRect();

    // Apply the clip from the layer.
    IntRect clipRect;
    if (clipToLayerContents)
        clipRect = layer->childrenClipRect();
    else
        clipRect = layer->selfClipRect();
    clipRect = contentsToWindow(clipRect); 
    return intersection(clipRect, windowClipRect());
}

bool FrameView::isActive() const
{
    Page* page = frame()->page();
    return page && page->focusController()->isActive();
}

void FrameView::scrollTo(const IntSize& newOffset)
{
    IntSize offset = scrollOffset();
    ScrollView::scrollTo(newOffset);
    if (offset != scrollOffset())
        scrollPositionChanged();
    frame()->loader()->client()->didChangeScrollOffset();
}

void FrameView::invalidateScrollbarRect(Scrollbar* scrollbar, const IntRect& rect)
{
    // Add in our offset within the FrameView.
    IntRect dirtyRect = rect;
    dirtyRect.move(scrollbar->x(), scrollbar->y());
    invalidateRect(dirtyRect);
}

void FrameView::getTickmarks(Vector<IntRect>& tickmarks) const
{
    tickmarks = frame()->document()->markers()->renderedRectsForMarkers(DocumentMarker::TextMatch);
}

IntRect FrameView::windowResizerRect() const
{
    Page* page = frame() ? frame()->page() : 0;
    if (!page)
        return IntRect();
    return page->chrome()->windowResizerRect();
}

void FrameView::didCompleteRubberBand(const IntSize& initialOverhang) const
{
    Page* page = m_frame->page();
    if (!page)
        return;
    if (page->mainFrame() != m_frame)
        return;
    return page->chrome()->client()->didCompleteRubberBandForMainFrame(initialOverhang);
}

void FrameView::scrollbarStyleChanged()
{
    Page* page = m_frame->page();
    if (!page)
        return;
    page->setNeedsRecalcStyleInAllFrames();
}

void FrameView::setVisibleScrollerThumbRect(const IntRect& scrollerThumb)
{
    Page* page = m_frame->page();
    if (!page)
        return;
    if (page->mainFrame() != m_frame)
        return;
    return page->chrome()->client()->notifyScrollerThumbIsVisibleInRect(scrollerThumb);
}

bool FrameView::shouldSuspendScrollAnimations() const
{
    return m_frame->loader()->state() != FrameStateComplete;
}

void FrameView::notifyPageThatContentAreaWillPaint() const
{
    Page* page = m_frame->page();
    const HashSet<ScrollableArea*>* scrollableAreas = page->scrollableAreaSet();
    if (!scrollableAreas)
        return;

    HashSet<ScrollableArea*>::const_iterator end = scrollableAreas->end(); 
    for (HashSet<ScrollableArea*>::const_iterator it = scrollableAreas->begin(); it != end; ++it)
        (*it)->scrollAnimator()->contentAreaWillPaint();
}

#if ENABLE(DASHBOARD_SUPPORT)
void FrameView::updateDashboardRegions()
{
    Document* document = m_frame->document();
    if (!document->hasDashboardRegions())
        return;
    Vector<DashboardRegionValue> newRegions;
    document->renderBox()->collectDashboardRegions(newRegions);
    if (newRegions == document->dashboardRegions())
        return;
    document->setDashboardRegions(newRegions);
    Page* page = m_frame->page();
    if (!page)
        return;
    page->chrome()->client()->dashboardRegionsChanged();
}
#endif

void FrameView::updateScrollCorner()
{
    RenderObject* renderer = 0;
    RefPtr<RenderStyle> cornerStyle;
    
    if (!scrollCornerRect().isEmpty()) {
        // Try the <body> element first as a scroll corner source.
        Document* doc = m_frame->document();
        Element* body = doc ? doc->body() : 0;
        if (body && body->renderer()) {
            renderer = body->renderer();
            cornerStyle = renderer->getUncachedPseudoStyle(SCROLLBAR_CORNER, renderer->style());
        }
        
        if (!cornerStyle) {
            // If the <body> didn't have a custom style, then the root element might.
            Element* docElement = doc ? doc->documentElement() : 0;
            if (docElement && docElement->renderer()) {
                renderer = docElement->renderer();
                cornerStyle = renderer->getUncachedPseudoStyle(SCROLLBAR_CORNER, renderer->style());
            }
        }
        
        if (!cornerStyle) {
            // If we have an owning iframe/frame element, then it can set the custom scrollbar also.
            if (RenderPart* renderer = m_frame->ownerRenderer())
                cornerStyle = renderer->getUncachedPseudoStyle(SCROLLBAR_CORNER, renderer->style());
        }
    }

    if (cornerStyle) {
        if (!m_scrollCorner)
            m_scrollCorner = new (renderer->renderArena()) RenderScrollbarPart(renderer->document());
        m_scrollCorner->setStyle(cornerStyle.release());
        invalidateScrollCorner();
    } else if (m_scrollCorner) {
        m_scrollCorner->destroy();
        m_scrollCorner = 0;
    }

    ScrollView::updateScrollCorner();
}

void FrameView::paintScrollCorner(GraphicsContext* context, const IntRect& cornerRect)
{
    if (context->updatingControlTints()) {
        updateScrollCorner();
        return;
    }

    if (m_scrollCorner) {
        m_scrollCorner->paintIntoRect(context, cornerRect.x(), cornerRect.y(), cornerRect);
        return;
    }

    ScrollView::paintScrollCorner(context, cornerRect);
}

bool FrameView::hasCustomScrollbars() const
{
    const HashSet<RefPtr<Widget> >* viewChildren = children();
    HashSet<RefPtr<Widget> >::const_iterator end = viewChildren->end();
    for (HashSet<RefPtr<Widget> >::const_iterator current = viewChildren->begin(); current != end; ++current) {
        Widget* widget = current->get();
        if (widget->isFrameView()) {
            if (static_cast<FrameView*>(widget)->hasCustomScrollbars())
                return true;
        } else if (widget->isScrollbar()) {
            Scrollbar* scrollbar = static_cast<Scrollbar*>(widget);
            if (scrollbar->isCustomScrollbar())
                return true;
        }
    }

    return false;
}

void FrameView::clearOwningRendererForCustomScrollbars(RenderBox* box)
{
    const HashSet<RefPtr<Widget> >* viewChildren = children();
    HashSet<RefPtr<Widget> >::const_iterator end = viewChildren->end();
    for (HashSet<RefPtr<Widget> >::const_iterator current = viewChildren->begin(); current != end; ++current) {
        Widget* widget = current->get();
        if (widget->isScrollbar()) {
            Scrollbar* scrollbar = static_cast<Scrollbar*>(widget);
            if (scrollbar->isCustomScrollbar()) {
                RenderScrollbar* customScrollbar = toRenderScrollbar(scrollbar);
                if (customScrollbar->owningRenderer() == box)
                    customScrollbar->clearOwningRenderer();
            }
        }
    }
}

FrameView* FrameView::parentFrameView() const
{
    if (Widget* parentView = parent()) {
        if (parentView->isFrameView())
            return static_cast<FrameView*>(parentView);
    }
    return 0;
}

void FrameView::updateControlTints()
{
    // This is called when control tints are changed from aqua/graphite to clear and vice versa.
    // We do a "fake" paint, and when the theme gets a paint call, it can then do an invalidate.
    // This is only done if the theme supports control tinting. It's up to the theme and platform
    // to define when controls get the tint and to call this function when that changes.
    
    // Optimize the common case where we bring a window to the front while it's still empty.
    if (!m_frame || m_frame->document()->url().isEmpty())
        return;

    if ((m_frame->contentRenderer() && m_frame->contentRenderer()->theme()->supportsControlTints()) || hasCustomScrollbars())  {
        if (needsLayout())
            layout();
        PlatformGraphicsContext* const noContext = 0;
        GraphicsContext context(noContext);
        context.setUpdatingControlTints(true);
        if (platformWidget())
            paintContents(&context, visibleContentRect());
        else
            paint(&context, frameRect());
    }
}

bool FrameView::wasScrolledByUser() const
{
    return m_wasScrolledByUser;
}

void FrameView::setWasScrolledByUser(bool wasScrolledByUser)
{
    if (m_inProgrammaticScroll)
        return;
    m_maintainScrollPositionAnchor = 0;
    m_wasScrolledByUser = wasScrolledByUser;
}

void FrameView::paintContents(GraphicsContext* p, const IntRect& rect)
{
    if (!frame())
        return;

    InspectorInstrumentationCookie cookie = InspectorInstrumentation::willPaint(m_frame.get(), rect);

    Document* document = m_frame->document();

#ifndef NDEBUG
    bool fillWithRed;
    if (document->printing())
        fillWithRed = false; // Printing, don't fill with red (can't remember why).
    else if (m_frame->ownerElement())
        fillWithRed = false; // Subframe, don't fill with red.
    else if (isTransparent())
        fillWithRed = false; // Transparent, don't fill with red.
    else if (m_paintBehavior & PaintBehaviorSelectionOnly)
        fillWithRed = false; // Selections are transparent, don't fill with red.
    else if (m_nodeToDraw)
        fillWithRed = false; // Element images are transparent, don't fill with red.
    else
        fillWithRed = true;
    
    if (fillWithRed)
        p->fillRect(rect, Color(0xFF, 0, 0), ColorSpaceDeviceRGB);
#endif

    bool isTopLevelPainter = !sCurrentPaintTimeStamp;
    if (isTopLevelPainter)
        sCurrentPaintTimeStamp = currentTime();
    
    RenderView* contentRenderer = frame()->contentRenderer();
    if (!contentRenderer) {
        LOG_ERROR("called FrameView::paint with nil renderer");
        return;
    }

    ASSERT(!needsLayout());
    if (needsLayout())
        return;

#if USE(ACCELERATED_COMPOSITING)
    if (!p->paintingDisabled())
        syncCompositingStateForThisFrame();
#endif

    PaintBehavior oldPaintBehavior = m_paintBehavior;
    
    if (FrameView* parentView = parentFrameView()) {
        if (parentView->paintBehavior() & PaintBehaviorFlattenCompositingLayers)
            m_paintBehavior |= PaintBehaviorFlattenCompositingLayers;
    }
    
    if (m_paintBehavior == PaintBehaviorNormal)
        document->markers()->invalidateRenderedRectsForMarkersInRect(rect);

    if (document->printing())
        m_paintBehavior |= PaintBehaviorFlattenCompositingLayers;

    bool flatteningPaint = m_paintBehavior & PaintBehaviorFlattenCompositingLayers;
    bool isRootFrame = !m_frame->ownerElement();
    if (flatteningPaint && isRootFrame)
        notifyWidgetsInAllFrames(WillPaintFlattened);

    ASSERT(!m_isPainting);
    m_isPainting = true;

    // m_nodeToDraw is used to draw only one element (and its descendants)
    RenderObject* eltRenderer = m_nodeToDraw ? m_nodeToDraw->renderer() : 0;
    RenderLayer* rootLayer = contentRenderer->layer();

    rootLayer->paint(p, rect, m_paintBehavior, eltRenderer);

    if (rootLayer->containsDirtyOverlayScrollbars())
        rootLayer->paintOverlayScrollbars(p, rect, m_paintBehavior, eltRenderer);

    m_isPainting = false;

    if (flatteningPaint && isRootFrame)
        notifyWidgetsInAllFrames(DidPaintFlattened);

    m_paintBehavior = oldPaintBehavior;
    m_lastPaintTime = currentTime();

#if ENABLE(DASHBOARD_SUPPORT)
    // Regions may have changed as a result of the visibility/z-index of element changing.
    if (document->dashboardRegionsDirty())
        updateDashboardRegions();
#endif

    if (isTopLevelPainter)
        sCurrentPaintTimeStamp = 0;

    InspectorInstrumentation::didPaint(cookie);
}

void FrameView::setPaintBehavior(PaintBehavior behavior)
{
    m_paintBehavior = behavior;
}

PaintBehavior FrameView::paintBehavior() const
{
    return m_paintBehavior;
}

bool FrameView::isPainting() const
{
    return m_isPainting;
}

void FrameView::setNodeToDraw(Node* node)
{
    m_nodeToDraw = node;
}

void FrameView::paintOverhangAreas(GraphicsContext* context, const IntRect& horizontalOverhangArea, const IntRect& verticalOverhangArea, const IntRect& dirtyRect)
{
    if (context->paintingDisabled())
        return;

    if (m_frame->document()->printing())
        return;

    Page* page = m_frame->page();
    if (page->mainFrame() == m_frame) {
        if (page->chrome()->client()->paintCustomOverhangArea(context, horizontalOverhangArea, verticalOverhangArea, dirtyRect))
            return;
    }

    return ScrollView::paintOverhangAreas(context, horizontalOverhangArea, verticalOverhangArea, dirtyRect);
}

void FrameView::updateLayoutAndStyleIfNeededRecursive()
{
    // We have to crawl our entire tree looking for any FrameViews that need
    // layout and make sure they are up to date.
    // Mac actually tests for intersection with the dirty region and tries not to
    // update layout for frames that are outside the dirty region.  Not only does this seem
    // pointless (since those frames will have set a zero timer to layout anyway), but
    // it is also incorrect, since if two frames overlap, the first could be excluded from the dirty
    // region but then become included later by the second frame adding rects to the dirty region
    // when it lays out.

    m_frame->document()->updateStyleIfNeeded();

    if (needsLayout())
        layout();

    const HashSet<RefPtr<Widget> >* viewChildren = children();
    HashSet<RefPtr<Widget> >::const_iterator end = viewChildren->end();
    for (HashSet<RefPtr<Widget> >::const_iterator current = viewChildren->begin(); current != end; ++current) {
        Widget* widget = (*current).get();
        if (widget->isFrameView())
            static_cast<FrameView*>(widget)->updateLayoutAndStyleIfNeededRecursive();
    }

    // updateLayoutAndStyleIfNeededRecursive is called when we need to make sure style and layout are up-to-date before
    // painting, so we need to flush out any deferred repaints too.
    flushDeferredRepaints();
}
    
void FrameView::flushDeferredRepaints()
{
    if (!m_deferredRepaintTimer.isActive())
        return;
    m_deferredRepaintTimer.stop();
    doDeferredRepaints();
}

void FrameView::forceLayout(bool allowSubtree)
{
    layout(allowSubtree);
}

void FrameView::forceLayoutForPagination(const FloatSize& pageSize, float maximumShrinkFactor, Frame::AdjustViewSizeOrNot shouldAdjustViewSize)
{
    // Dumping externalRepresentation(m_frame->renderer()).ascii() is a good trick to see
    // the state of things before and after the layout
    RenderView *root = toRenderView(m_frame->document()->renderer());
    if (root) {
        float pageLogicalWidth = root->style()->isHorizontalWritingMode() ? pageSize.width() : pageSize.height();
        float pageLogicalHeight = root->style()->isHorizontalWritingMode() ? pageSize.height() : pageSize.width();

        int flooredPageLogicalWidth = static_cast<int>(pageLogicalWidth);
        root->setLogicalWidth(flooredPageLogicalWidth);
        root->setPageLogicalHeight(pageLogicalHeight);
        root->setNeedsLayoutAndPrefWidthsRecalc();
        forceLayout();
        
        // If we don't fit in the given page width, we'll lay out again. If we don't fit in the
        // page width when shrunk, we will lay out at maximum shrink and clip extra content.
        // FIXME: We are assuming a shrink-to-fit printing implementation.  A cropping
        // implementation should not do this!
        int docLogicalWidth = root->style()->isHorizontalWritingMode() ? root->docWidth() : root->docHeight();
        if (docLogicalWidth > pageLogicalWidth) {
            flooredPageLogicalWidth = std::min<int>(docLogicalWidth, pageLogicalWidth * maximumShrinkFactor);
            if (pageLogicalHeight)
                root->setPageLogicalHeight(flooredPageLogicalWidth / pageSize.width() * pageSize.height());
            root->setLogicalWidth(flooredPageLogicalWidth);
            root->setNeedsLayoutAndPrefWidthsRecalc();
            forceLayout();
            int docLogicalHeight = root->style()->isHorizontalWritingMode() ? root->docHeight() : root->docWidth();
            int docLogicalTop = root->style()->isHorizontalWritingMode() ? root->docTop() : root->docLeft();
            int docLogicalRight = root->style()->isHorizontalWritingMode() ? root->docRight() : root->docBottom();
            int clippedLogicalLeft = 0;
            if (!root->style()->isLeftToRightDirection())
                clippedLogicalLeft = docLogicalRight - flooredPageLogicalWidth;
            IntRect overflow(clippedLogicalLeft, docLogicalTop, flooredPageLogicalWidth, docLogicalHeight);
            if (!root->style()->isHorizontalWritingMode())
                overflow = overflow.transposedRect();
            root->clearLayoutOverflow();
            root->addLayoutOverflow(overflow); // This is how we clip in case we overflow again.
        }
    }

    if (shouldAdjustViewSize)
        adjustViewSize();
}

void FrameView::adjustPageHeightDeprecated(float *newBottom, float oldTop, float oldBottom, float /*bottomLimit*/)
{
    RenderView* root = m_frame->contentRenderer();
    if (root) {
        // Use a context with painting disabled.
        GraphicsContext context((PlatformGraphicsContext*)0);
        root->setTruncatedAt((int)floorf(oldBottom));
        IntRect dirtyRect(0, (int)floorf(oldTop), root->maxXLayoutOverflow(), (int)ceilf(oldBottom - oldTop));
        root->setPrintRect(dirtyRect);
        root->layer()->paint(&context, dirtyRect);
        *newBottom = root->bestTruncatedAt();
        if (*newBottom == 0)
            *newBottom = oldBottom;
        root->setPrintRect(IntRect());
    } else
        *newBottom = oldBottom;
}

IntRect FrameView::convertFromRenderer(const RenderObject* renderer, const IntRect& rendererRect) const
{
    IntRect rect = renderer->localToAbsoluteQuad(FloatRect(rendererRect)).enclosingBoundingBox();

    // Convert from page ("absolute") to FrameView coordinates.
    rect.move(-scrollX(), -scrollY());

    return rect;
}

IntRect FrameView::convertToRenderer(const RenderObject* renderer, const IntRect& viewRect) const
{
    IntRect rect = viewRect;
    
    // Convert from FrameView coords into page ("absolute") coordinates.
    rect.move(scrollX(), scrollY());

    // FIXME: we don't have a way to map an absolute rect down to a local quad, so just
    // move the rect for now.
    rect.setLocation(roundedIntPoint(renderer->absoluteToLocal(rect.location(), false, true /* use transforms */)));
    return rect;
}

IntPoint FrameView::convertFromRenderer(const RenderObject* renderer, const IntPoint& rendererPoint) const
{
    IntPoint point = roundedIntPoint(renderer->localToAbsolute(rendererPoint, false, true /* use transforms */));

    // Convert from page ("absolute") to FrameView coordinates.
    point.move(-scrollX(), -scrollY());
    return point;
}

IntPoint FrameView::convertToRenderer(const RenderObject* renderer, const IntPoint& viewPoint) const
{
    IntPoint point = viewPoint;
    
    // Convert from FrameView coords into page ("absolute") coordinates.
    point += IntSize(scrollX(), scrollY());

    return roundedIntPoint(renderer->absoluteToLocal(point, false, true /* use transforms */));
}

IntRect FrameView::convertToContainingView(const IntRect& localRect) const
{
    if (const ScrollView* parentScrollView = parent()) {
        if (parentScrollView->isFrameView()) {
            const FrameView* parentView = static_cast<const FrameView*>(parentScrollView);
            // Get our renderer in the parent view
            RenderPart* renderer = m_frame->ownerRenderer();
            if (!renderer)
                return localRect;
                
            IntRect rect(localRect);
            // Add borders and padding??
            rect.move(renderer->borderLeft() + renderer->paddingLeft(),
                      renderer->borderTop() + renderer->paddingTop());
            return parentView->convertFromRenderer(renderer, rect);
        }
        
        return Widget::convertToContainingView(localRect);
    }
    
    return localRect;
}

IntRect FrameView::convertFromContainingView(const IntRect& parentRect) const
{
    if (const ScrollView* parentScrollView = parent()) {
        if (parentScrollView->isFrameView()) {
            const FrameView* parentView = static_cast<const FrameView*>(parentScrollView);

            // Get our renderer in the parent view
            RenderPart* renderer = m_frame->ownerRenderer();
            if (!renderer)
                return parentRect;

            IntRect rect = parentView->convertToRenderer(renderer, parentRect);
            // Subtract borders and padding
            rect.move(-renderer->borderLeft() - renderer->paddingLeft(),
                      -renderer->borderTop() - renderer->paddingTop());
            return rect;
        }
        
        return Widget::convertFromContainingView(parentRect);
    }
    
    return parentRect;
}

IntPoint FrameView::convertToContainingView(const IntPoint& localPoint) const
{
    if (const ScrollView* parentScrollView = parent()) {
        if (parentScrollView->isFrameView()) {
            const FrameView* parentView = static_cast<const FrameView*>(parentScrollView);

            // Get our renderer in the parent view
            RenderPart* renderer = m_frame->ownerRenderer();
            if (!renderer)
                return localPoint;
                
            IntPoint point(localPoint);

            // Add borders and padding
            point.move(renderer->borderLeft() + renderer->paddingLeft(),
                       renderer->borderTop() + renderer->paddingTop());
            return parentView->convertFromRenderer(renderer, point);
        }
        
        return Widget::convertToContainingView(localPoint);
    }
    
    return localPoint;
}

IntPoint FrameView::convertFromContainingView(const IntPoint& parentPoint) const
{
    if (const ScrollView* parentScrollView = parent()) {
        if (parentScrollView->isFrameView()) {
            const FrameView* parentView = static_cast<const FrameView*>(parentScrollView);

            // Get our renderer in the parent view
            RenderPart* renderer = m_frame->ownerRenderer();
            if (!renderer)
                return parentPoint;

            IntPoint point = parentView->convertToRenderer(renderer, parentPoint);
            // Subtract borders and padding
            point.move(-renderer->borderLeft() - renderer->paddingLeft(),
                       -renderer->borderTop() - renderer->paddingTop());
            return point;
        }
        
        return Widget::convertFromContainingView(parentPoint);
    }
    
    return parentPoint;
}

// Normal delay
void FrameView::setRepaintThrottlingDeferredRepaintDelay(double p)
{
    s_deferredRepaintDelay = p;
}

// Negative value would mean that first few repaints happen without a delay
void FrameView::setRepaintThrottlingnInitialDeferredRepaintDelayDuringLoading(double p)
{
    s_initialDeferredRepaintDelayDuringLoading = p;
}

// The delay grows on each repaint to this maximum value
void FrameView::setRepaintThrottlingMaxDeferredRepaintDelayDuringLoading(double p)
{
    s_maxDeferredRepaintDelayDuringLoading = p;
}

// On each repaint the delay increases by this amount
void FrameView::setRepaintThrottlingDeferredRepaintDelayIncrementDuringLoading(double p)
{
    s_deferredRepaintDelayIncrementDuringLoading = p;
}

bool FrameView::isVerticalDocument() const
{
    if (!m_frame)
        return true;
    Document* doc = m_frame->document();
    if (!doc)
        return true;
    RenderObject* renderView = doc->renderer();
    if (!renderView)
        return true;
    return renderView->style()->isHorizontalWritingMode();
}

bool FrameView::isFlippedDocument() const
{
    if (!m_frame)
        return false;
    Document* doc = m_frame->document();
    if (!doc)
        return false;
    RenderObject* renderView = doc->renderer();
    if (!renderView)
        return false;
    return renderView->style()->isFlippedBlocksWritingMode();
}

void FrameView::notifyWidgetsInAllFrames(WidgetNotification notification)
{
    for (Frame* frame = m_frame.get(); frame; frame = frame->tree()->traverseNext(m_frame.get())) {
        if (RenderView* root = frame->contentRenderer())
            root->notifyWidgets(notification);
    }
}
    
AXObjectCache* FrameView::axObjectCache() const
{
    if (frame() && frame()->document() && frame()->document()->axObjectCacheExists())
        return frame()->document()->axObjectCache();
    return 0;
}
    
} // namespace WebCore
