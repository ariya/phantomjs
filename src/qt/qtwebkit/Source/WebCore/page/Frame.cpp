/*
 * Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 *                     1999 Lars Knoll <knoll@kde.org>
 *                     1999 Antti Koivisto <koivisto@kde.org>
 *                     2000 Simon Hausmann <hausmann@kde.org>
 *                     2000 Stefan Schimanski <1Stein@gmx.de>
 *                     2001 George Staikos <staikos@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2005 Alexey Proskuryakov <ap@nypop.com>
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008 Google Inc.
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
#include "Frame.h"

#include "AnimationController.h"
#include "ApplyStyleCommand.h"
#include "BackForwardController.h"
#include "CSSComputedStyleDeclaration.h"
#include "CSSPropertyNames.h"
#include "CachedCSSStyleSheet.h"
#include "CachedResourceLoader.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "DOMWindow.h"
#include "DocumentType.h"
#include "Editor.h"
#include "EditorClient.h"
#include "Event.h"
#include "EventHandler.h"
#include "EventNames.h"
#include "FloatQuad.h"
#include "FocusController.h"
#include "FrameDestructionObserver.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameSelection.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "GraphicsLayer.h"
#include "HTMLDocument.h"
#include "HTMLFormControlElement.h"
#include "HTMLFormElement.h"
#include "HTMLFrameElementBase.h"
#include "HTMLNames.h"
#include "HTMLTableCellElement.h"
#include "HitTestResult.h"
#include "ImageBuffer.h"
#include "InspectorInstrumentation.h"
#include "JSDOMWindowShell.h"
#include "Logging.h"
#include "MathMLNames.h"
#include "MediaFeatureNames.h"
#include "Navigator.h"
#include "NodeList.h"
#include "NodeTraversal.h"
#include "Page.h"
#include "PageCache.h"
#include "PageGroup.h"
#include "RegularExpression.h"
#include "RenderPart.h"
#include "RenderTableCell.h"
#include "RenderTextControl.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "RuntimeEnabledFeatures.h"
#include "SVGNames.h"
#include "ScriptController.h"
#include "ScriptSourceCode.h"
#include "ScriptValue.h"
#include "ScrollingCoordinator.h"
#include "Settings.h"
#include "StylePropertySet.h"
#include "TextIterator.h"
#include "TextResourceDecoder.h"
#include "UserContentURLPattern.h"
#include "UserTypingGestureIndicator.h"
#include "VisibleUnits.h"
#include "WebKitFontFamilyNames.h"
#include "XLinkNames.h"
#include "XMLNSNames.h"
#include "XMLNames.h"
#include "htmlediting.h"
#include "markup.h"
#include "npruntime_impl.h"
#include "runtime_root.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/RefCountedLeakCounter.h>
#include <wtf/StdLibExtras.h>

#if USE(ACCELERATED_COMPOSITING)
#include "RenderLayerCompositor.h"
#endif

#if ENABLE(SVG)
#include "SVGDocument.h"
#include "SVGDocumentExtensions.h"
#endif

#if USE(TILED_BACKING_STORE)
#include "TiledBackingStore.h"
#endif

using namespace std;

namespace WebCore {

using namespace HTMLNames;

DEFINE_DEBUG_ONLY_GLOBAL(WTF::RefCountedLeakCounter, frameCounter, ("Frame"));

static inline Frame* parentFromOwnerElement(HTMLFrameOwnerElement* ownerElement)
{
    if (!ownerElement)
        return 0;
    return ownerElement->document()->frame();
}

static inline float parentPageZoomFactor(Frame* frame)
{
    Frame* parent = frame->tree()->parent();
    if (!parent)
        return 1;
    return parent->pageZoomFactor();
}

static inline float parentTextZoomFactor(Frame* frame)
{
    Frame* parent = frame->tree()->parent();
    if (!parent)
        return 1;
    return parent->textZoomFactor();
}

inline Frame::Frame(Page* page, HTMLFrameOwnerElement* ownerElement, FrameLoaderClient* frameLoaderClient)
    : m_page(page)
    , m_treeNode(this, parentFromOwnerElement(ownerElement))
    , m_loader(this, frameLoaderClient)
    , m_navigationScheduler(this)
    , m_ownerElement(ownerElement)
    , m_script(adoptPtr(new ScriptController(this)))
    , m_editor(adoptPtr(new Editor(this)))
    , m_selection(adoptPtr(new FrameSelection(this)))
    , m_eventHandler(adoptPtr(new EventHandler(this)))
    , m_animationController(adoptPtr(new AnimationController(this)))
    , m_pageZoomFactor(parentPageZoomFactor(this))
    , m_textZoomFactor(parentTextZoomFactor(this))
#if ENABLE(ORIENTATION_EVENTS)
    , m_orientation(0)
#endif
    , m_inViewSourceMode(false)
    , m_activeDOMObjectsAndAnimationsSuspendedCount(0)
{
    ASSERT(page);
    AtomicString::init();
    HTMLNames::init();
    QualifiedName::init();
    MediaFeatureNames::init();
    SVGNames::init();
    XLinkNames::init();
    MathMLNames::init();
    XMLNSNames::init();
    XMLNames::init();
    WebKitFontFamilyNames::init();

    if (!ownerElement) {
#if USE(TILED_BACKING_STORE)
        // Top level frame only for now.
        setTiledBackingStoreEnabled(page->settings()->tiledBackingStoreEnabled());
#endif
    } else {
        page->incrementSubframeCount();
        ownerElement->setContentFrame(this);
    }

#ifndef NDEBUG
    frameCounter.increment();
#endif

    // Pause future ActiveDOMObjects if this frame is being created while the page is in a paused state.
    Frame* parent = parentFromOwnerElement(ownerElement);
    if (parent && parent->activeDOMObjectsAndAnimationsSuspended())
        suspendActiveDOMObjectsAndAnimations();
}

PassRefPtr<Frame> Frame::create(Page* page, HTMLFrameOwnerElement* ownerElement, FrameLoaderClient* client)
{
    RefPtr<Frame> frame = adoptRef(new Frame(page, ownerElement, client));
    if (!ownerElement)
        page->setMainFrame(frame);
    return frame.release();
}

Frame::~Frame()
{
    setView(0);
    loader()->cancelAndClear();

    // FIXME: We should not be doing all this work inside the destructor

#ifndef NDEBUG
    frameCounter.decrement();
#endif

    disconnectOwnerElement();

    HashSet<FrameDestructionObserver*>::iterator stop = m_destructionObservers.end();
    for (HashSet<FrameDestructionObserver*>::iterator it = m_destructionObservers.begin(); it != stop; ++it)
        (*it)->frameDestroyed();
}

bool Frame::inScope(TreeScope* scope) const
{
    ASSERT(scope);
    Document* doc = document();
    if (!doc)
        return false;
    HTMLFrameOwnerElement* owner = doc->ownerElement();
    if (!owner)
        return false;
    return owner->treeScope() == scope;
}

void Frame::addDestructionObserver(FrameDestructionObserver* observer)
{
    m_destructionObservers.add(observer);
}

void Frame::removeDestructionObserver(FrameDestructionObserver* observer)
{
    m_destructionObservers.remove(observer);
}

void Frame::setView(PassRefPtr<FrameView> view)
{
    // We the custom scroll bars as early as possible to prevent m_doc->detach()
    // from messing with the view such that its scroll bars won't be torn down.
    // FIXME: We should revisit this.
    if (m_view)
        m_view->prepareForDetach();

    // Prepare for destruction now, so any unload event handlers get run and the DOMWindow is
    // notified. If we wait until the view is destroyed, then things won't be hooked up enough for
    // these calls to work.
    if (!view && m_doc && m_doc->attached() && !m_doc->inPageCache()) {
        // FIXME: We don't call willRemove here. Why is that OK?
        m_doc->prepareForDestruction();
    }
    
    if (m_view)
        m_view->unscheduleRelayout();
    
    eventHandler()->clear();

    m_view = view;

    // Only one form submission is allowed per view of a part.
    // Since this part may be getting reused as a result of being
    // pulled from the back/forward cache, reset this flag.
    loader()->resetMultipleFormSubmissionProtection();
    
#if USE(TILED_BACKING_STORE)
    if (m_view && tiledBackingStore())
        m_view->setPaintsEntireContents(true);
#endif
}

void Frame::setDocument(PassRefPtr<Document> newDoc)
{
    ASSERT(!newDoc || newDoc->frame() == this);
    if (m_doc && m_doc->attached() && !m_doc->inPageCache()) {
        // FIXME: We don't call willRemove here. Why is that OK?
        m_doc->detach();
    }

    m_doc = newDoc;
    ASSERT(!m_doc || m_doc->domWindow());
    ASSERT(!m_doc || m_doc->domWindow()->frame() == this);

    if (m_doc && !m_doc->attached())
        m_doc->attach();

    if (m_doc) {
        m_script->updateDocument();
        m_doc->updateViewportArguments();
    }

    if (m_page && m_page->mainFrame() == this) {
        notifyChromeClientWheelEventHandlerCountChanged();
#if ENABLE(TOUCH_EVENTS)
        if (m_doc && m_doc->hasTouchEventHandlers())
            m_page->chrome().client()->needTouchEvents(true);
#endif
    }

    // Suspend document if this frame was created in suspended state.
    if (m_doc && activeDOMObjectsAndAnimationsSuspended()) {
        m_doc->suspendScriptedAnimationControllerCallbacks();
        m_animationController->suspendAnimationsForDocument(m_doc.get());
        m_doc->suspendActiveDOMObjects(ActiveDOMObject::PageWillBeSuspended);
    }
}

#if ENABLE(ORIENTATION_EVENTS)
void Frame::sendOrientationChangeEvent(int orientation)
{
    m_orientation = orientation;
    if (Document* doc = document())
        doc->dispatchWindowEvent(Event::create(eventNames().orientationchangeEvent, false, false));
}
#endif // ENABLE(ORIENTATION_EVENTS)
    
Settings* Frame::settings() const
{
    return m_page ? m_page->settings() : 0;
}

static PassOwnPtr<RegularExpression> createRegExpForLabels(const Vector<String>& labels)
{
    // REVIEW- version of this call in FrameMac.mm caches based on the NSArray ptrs being
    // the same across calls.  We can't do that.

    DEFINE_STATIC_LOCAL(RegularExpression, wordRegExp, ("\\w", TextCaseSensitive));
    String pattern("(");
    unsigned int numLabels = labels.size();
    unsigned int i;
    for (i = 0; i < numLabels; i++) {
        String label = labels[i];

        bool startsWithWordChar = false;
        bool endsWithWordChar = false;
        if (label.length()) {
            startsWithWordChar = wordRegExp.match(label.substring(0, 1)) >= 0;
            endsWithWordChar = wordRegExp.match(label.substring(label.length() - 1, 1)) >= 0;
        }

        if (i)
            pattern.append("|");
        // Search for word boundaries only if label starts/ends with "word characters".
        // If we always searched for word boundaries, this wouldn't work for languages
        // such as Japanese.
        if (startsWithWordChar)
            pattern.append("\\b");
        pattern.append(label);
        if (endsWithWordChar)
            pattern.append("\\b");
    }
    pattern.append(")");
    return adoptPtr(new RegularExpression(pattern, TextCaseInsensitive));
}

String Frame::searchForLabelsAboveCell(RegularExpression* regExp, HTMLTableCellElement* cell, size_t* resultDistanceFromStartOfCell)
{
    HTMLTableCellElement* aboveCell = cell->cellAbove();
    if (aboveCell) {
        // search within the above cell we found for a match
        size_t lengthSearched = 0;    
        for (Node* n = aboveCell->firstChild(); n; n = NodeTraversal::next(n, aboveCell)) {
            if (n->isTextNode() && n->renderer() && n->renderer()->style()->visibility() == VISIBLE) {
                // For each text chunk, run the regexp
                String nodeString = n->nodeValue();
                int pos = regExp->searchRev(nodeString);
                if (pos >= 0) {
                    if (resultDistanceFromStartOfCell)
                        *resultDistanceFromStartOfCell = lengthSearched;
                    return nodeString.substring(pos, regExp->matchedLength());
                }
                lengthSearched += nodeString.length();
            }
        }
    }

    // Any reason in practice to search all cells in that are above cell?
    if (resultDistanceFromStartOfCell)
        *resultDistanceFromStartOfCell = notFound;
    return String();
}

String Frame::searchForLabelsBeforeElement(const Vector<String>& labels, Element* element, size_t* resultDistance, bool* resultIsInCellAbove)
{
    OwnPtr<RegularExpression> regExp(createRegExpForLabels(labels));
    // We stop searching after we've seen this many chars
    const unsigned int charsSearchedThreshold = 500;
    // This is the absolute max we search.  We allow a little more slop than
    // charsSearchedThreshold, to make it more likely that we'll search whole nodes.
    const unsigned int maxCharsSearched = 600;
    // If the starting element is within a table, the cell that contains it
    HTMLTableCellElement* startingTableCell = 0;
    bool searchedCellAbove = false;

    if (resultDistance)
        *resultDistance = notFound;
    if (resultIsInCellAbove)
        *resultIsInCellAbove = false;
    
    // walk backwards in the node tree, until another element, or form, or end of tree
    int unsigned lengthSearched = 0;
    Node* n;
    for (n = NodeTraversal::previous(element); n && lengthSearched < charsSearchedThreshold; n = NodeTraversal::previous(n)) {
        // We hit another form element or the start of the form - bail out
        if (isHTMLFormElement(n) || (n->isHTMLElement() && toElement(n)->isFormControlElement()))
            break;

        if (n->hasTagName(tdTag) && !startingTableCell) {
            startingTableCell = static_cast<HTMLTableCellElement*>(n);
        } else if (n->hasTagName(trTag) && startingTableCell) {
            String result = searchForLabelsAboveCell(regExp.get(), startingTableCell, resultDistance);
            if (!result.isEmpty()) {
                if (resultIsInCellAbove)
                    *resultIsInCellAbove = true;
                return result;
            }
            searchedCellAbove = true;
        } else if (n->isTextNode() && n->renderer() && n->renderer()->style()->visibility() == VISIBLE) {
            // For each text chunk, run the regexp
            String nodeString = n->nodeValue();
            // add 100 for slop, to make it more likely that we'll search whole nodes
            if (lengthSearched + nodeString.length() > maxCharsSearched)
                nodeString = nodeString.right(charsSearchedThreshold - lengthSearched);
            int pos = regExp->searchRev(nodeString);
            if (pos >= 0) {
                if (resultDistance)
                    *resultDistance = lengthSearched;
                return nodeString.substring(pos, regExp->matchedLength());
            }
            lengthSearched += nodeString.length();
        }
    }

    // If we started in a cell, but bailed because we found the start of the form or the
    // previous element, we still might need to search the row above us for a label.
    if (startingTableCell && !searchedCellAbove) {
         String result = searchForLabelsAboveCell(regExp.get(), startingTableCell, resultDistance);
        if (!result.isEmpty()) {
            if (resultIsInCellAbove)
                *resultIsInCellAbove = true;
            return result;
        }
    }
    return String();
}

static String matchLabelsAgainstString(const Vector<String>& labels, const String& stringToMatch)
{
    if (stringToMatch.isEmpty())
        return String();

    String mutableStringToMatch = stringToMatch;

    // Make numbers and _'s in field names behave like word boundaries, e.g., "address2"
    replace(mutableStringToMatch, RegularExpression("\\d", TextCaseSensitive), " ");
    mutableStringToMatch.replace('_', ' ');
    
    OwnPtr<RegularExpression> regExp(createRegExpForLabels(labels));
    // Use the largest match we can find in the whole string
    int pos;
    int length;
    int bestPos = -1;
    int bestLength = -1;
    int start = 0;
    do {
        pos = regExp->match(mutableStringToMatch, start);
        if (pos != -1) {
            length = regExp->matchedLength();
            if (length >= bestLength) {
                bestPos = pos;
                bestLength = length;
            }
            start = pos + 1;
        }
    } while (pos != -1);
    
    if (bestPos != -1)
        return mutableStringToMatch.substring(bestPos, bestLength);
    return String();
}
    
String Frame::matchLabelsAgainstElement(const Vector<String>& labels, Element* element)
{
    // Match against the name element, then against the id element if no match is found for the name element.
    // See 7538330 for one popular site that benefits from the id element check.
    // FIXME: This code is mirrored in FrameMac.mm. It would be nice to make the Mac code call the platform-agnostic
    // code, which would require converting the NSArray of NSStrings to a Vector of Strings somewhere along the way.
    String resultFromNameAttribute = matchLabelsAgainstString(labels, element->getNameAttribute());
    if (!resultFromNameAttribute.isEmpty())
        return resultFromNameAttribute;
    
    return matchLabelsAgainstString(labels, element->getAttribute(idAttr));
}

void Frame::setPrinting(bool printing, const FloatSize& pageSize, const FloatSize& originalPageSize, float maximumShrinkRatio, AdjustViewSizeOrNot shouldAdjustViewSize)
{
    m_pageResets.clear();

    // In setting printing, we should not validate resources already cached for the document.
    // See https://bugs.webkit.org/show_bug.cgi?id=43704
    ResourceCacheValidationSuppressor validationSuppressor(m_doc->cachedResourceLoader());

    m_doc->setPrinting(printing);
    view()->adjustMediaTypeForPrinting(printing);

    m_doc->styleResolverChanged(RecalcStyleImmediately);
    if (shouldUsePrintingLayout()) {
        view()->forceLayoutForPagination(pageSize, originalPageSize, maximumShrinkRatio, shouldAdjustViewSize);
    } else {
        view()->forceLayout();
        if (shouldAdjustViewSize == AdjustViewSize)
            view()->adjustViewSize();
    }

    // Subframes of the one we're printing don't lay out to the page size.
    for (RefPtr<Frame> child = tree()->firstChild(); child; child = child->tree()->nextSibling())
        child->setPrinting(printing, FloatSize(), FloatSize(), 0, shouldAdjustViewSize);
}

bool Frame::shouldUsePrintingLayout() const
{
    // Only top frame being printed should be fit to page size.
    // Subframes should be constrained by parents only.
    return m_doc->printing() && (!tree()->parent() || !tree()->parent()->m_doc->printing());
}

FloatSize Frame::resizePageRectsKeepingRatio(const FloatSize& originalSize, const FloatSize& expectedSize)
{
    FloatSize resultSize;
    if (!contentRenderer())
        return FloatSize();

    if (contentRenderer()->style()->isHorizontalWritingMode()) {
        ASSERT(fabs(originalSize.width()) > numeric_limits<float>::epsilon());
        float ratio = originalSize.height() / originalSize.width();
        resultSize.setWidth(floorf(expectedSize.width()));
        resultSize.setHeight(floorf(resultSize.width() * ratio));
    } else {
        ASSERT(fabs(originalSize.height()) > numeric_limits<float>::epsilon());
        float ratio = originalSize.width() / originalSize.height();
        resultSize.setHeight(floorf(expectedSize.height()));
        resultSize.setWidth(floorf(resultSize.height() * ratio));
    }
    return resultSize;
}

void Frame::addResetPage(int page)
{
    m_pageResets.append(page);
}

void Frame::getPagination(int page, int pages, int& logicalPage, int& logicalPages) const
{
    logicalPage = page;
    logicalPages = pages;
    int last_j = 0;
    int j = 0;
    for(size_t i = 0; i < m_pageResets.size(); ++i) {
        j = m_pageResets.at(i);
        if (j >= page) {
            break;
        }
        last_j = j;
    }
    if (page > last_j) {
        logicalPage = page - last_j;
    }
    if (last_j) {
        if (j > last_j) {
            logicalPages = j - last_j;
        } else {
            logicalPages = pages - last_j;
        }
    } else if (j >= page && j < pages) {
        logicalPages = j;
    }
}

void Frame::injectUserScripts(UserScriptInjectionTime injectionTime)
{
    if (!m_page)
        return;

    if (loader()->stateMachine()->creatingInitialEmptyDocument() && !settings()->shouldInjectUserScriptsInInitialEmptyDocument())
        return;

    // Walk the hashtable. Inject by world.
    const UserScriptMap* userScripts = m_page->group().userScripts();
    if (!userScripts)
        return;
    UserScriptMap::const_iterator end = userScripts->end();
    for (UserScriptMap::const_iterator it = userScripts->begin(); it != end; ++it)
        injectUserScriptsForWorld(it->key.get(), *it->value, injectionTime);
}

void Frame::injectUserScriptsForWorld(DOMWrapperWorld* world, const UserScriptVector& userScripts, UserScriptInjectionTime injectionTime)
{
    if (userScripts.isEmpty())
        return;

    Document* doc = document();
    if (!doc)
        return;

    Vector<ScriptSourceCode> sourceCode;
    unsigned count = userScripts.size();
    for (unsigned i = 0; i < count; ++i) {
        UserScript* script = userScripts[i].get();
        if (script->injectedFrames() == InjectInTopFrameOnly && ownerElement())
            continue;

        if (script->injectionTime() == injectionTime && UserContentURLPattern::matchesPatterns(doc->url(), script->whitelist(), script->blacklist()))
            m_script->evaluateInWorld(ScriptSourceCode(script->source(), script->url()), world);
    }
}

RenderView* Frame::contentRenderer() const
{
    return document() ? document()->renderView() : 0;
}

RenderPart* Frame::ownerRenderer() const
{
    HTMLFrameOwnerElement* ownerElement = m_ownerElement;
    if (!ownerElement)
        return 0;
    RenderObject* object = ownerElement->renderer();
    if (!object)
        return 0;
    // FIXME: If <object> is ever fixed to disassociate itself from frames
    // that it has started but canceled, then this can turn into an ASSERT
    // since m_ownerElement would be 0 when the load is canceled.
    // https://bugs.webkit.org/show_bug.cgi?id=18585
    if (!object->isRenderPart())
        return 0;
    return toRenderPart(object);
}

Frame* Frame::frameForWidget(const Widget* widget)
{
    ASSERT_ARG(widget, widget);

    if (RenderWidget* renderer = RenderWidget::find(widget))
        if (Node* node = renderer->node())
            return node->document()->frame();

    // Assume all widgets are either a FrameView or owned by a RenderWidget.
    // FIXME: That assumption is not right for scroll bars!
    ASSERT_WITH_SECURITY_IMPLICATION(widget->isFrameView());
    return toFrameView(widget)->frame();
}

void Frame::clearTimers(FrameView *view, Document *document)
{
    if (view) {
        view->unscheduleRelayout();
        if (view->frame()) {
            view->frame()->animation()->suspendAnimationsForDocument(document);
            view->frame()->eventHandler()->stopAutoscrollTimer();
        }
    }
}

void Frame::clearTimers()
{
    clearTimers(m_view.get(), document());
}

#if ENABLE(PAGE_VISIBILITY_API)
void Frame::dispatchVisibilityStateChangeEvent()
{
    if (m_doc)
        m_doc->dispatchVisibilityStateChangeEvent();

    Vector<RefPtr<Frame> > childFrames;
    for (Frame* child = tree()->firstChild(); child; child = child->tree()->nextSibling())
        childFrames.append(child);

    for (size_t i = 0; i < childFrames.size(); ++i)
        childFrames[i]->dispatchVisibilityStateChangeEvent();
}
#endif

void Frame::willDetachPage()
{
    if (Frame* parent = tree()->parent())
        parent->loader()->checkLoadComplete();

    HashSet<FrameDestructionObserver*>::iterator stop = m_destructionObservers.end();
    for (HashSet<FrameDestructionObserver*>::iterator it = m_destructionObservers.begin(); it != stop; ++it)
        (*it)->willDetachPage();

    // FIXME: It's unclear as to why this is called more than once, but it is,
    // so page() could be NULL.
    if (page() && page()->focusController()->focusedFrame() == this)
        page()->focusController()->setFocusedFrame(0);

    if (page() && page()->scrollingCoordinator() && m_view)
        page()->scrollingCoordinator()->willDestroyScrollableArea(m_view.get());

    script()->clearScriptObjects();
    script()->updatePlatformScriptObjects();
}

void Frame::disconnectOwnerElement()
{
    if (m_ownerElement) {
        if (Document* doc = document())
            doc->topDocument()->clearAXObjectCache();
        m_ownerElement->clearContentFrame();
        if (m_page)
            m_page->decrementSubframeCount();
    }
    m_ownerElement = 0;
}

String Frame::documentTypeString() const
{
    if (DocumentType* doctype = document()->doctype())
        return createMarkup(doctype);

    return String();
}

String Frame::displayStringModifiedByEncoding(const String& str) const
{
    return document() ? document()->displayStringModifiedByEncoding(str) : str;
}

VisiblePosition Frame::visiblePositionForPoint(const IntPoint& framePoint)
{
    HitTestResult result = eventHandler()->hitTestResultAtPoint(framePoint, HitTestRequest::ReadOnly | HitTestRequest::Active);
    Node* node = result.innerNonSharedNode();
    if (!node)
        return VisiblePosition();
    RenderObject* renderer = node->renderer();
    if (!renderer)
        return VisiblePosition();
    VisiblePosition visiblePos = renderer->positionForPoint(result.localPoint());
    if (visiblePos.isNull())
        visiblePos = firstPositionInOrBeforeNode(node);
    return visiblePos;
}

Document* Frame::documentAtPoint(const IntPoint& point)
{
    if (!view())
        return 0;

    IntPoint pt = view()->windowToContents(point);
    HitTestResult result = HitTestResult(pt);

    if (contentRenderer())
        result = eventHandler()->hitTestResultAtPoint(pt);
    return result.innerNode() ? result.innerNode()->document() : 0;
}

PassRefPtr<Range> Frame::rangeForPoint(const IntPoint& framePoint)
{
    VisiblePosition position = visiblePositionForPoint(framePoint);
    if (position.isNull())
        return 0;

    VisiblePosition previous = position.previous();
    if (previous.isNotNull()) {
        RefPtr<Range> previousCharacterRange = makeRange(previous, position);
        LayoutRect rect = editor().firstRectForRange(previousCharacterRange.get());
        if (rect.contains(framePoint))
            return previousCharacterRange.release();
    }

    VisiblePosition next = position.next();
    if (RefPtr<Range> nextCharacterRange = makeRange(position, next)) {
        LayoutRect rect = editor().firstRectForRange(nextCharacterRange.get());
        if (rect.contains(framePoint))
            return nextCharacterRange.release();
    }

    return 0;
}

void Frame::createView(const IntSize& viewportSize, const Color& backgroundColor, bool transparent,
    const IntSize& fixedLayoutSize, const IntRect& fixedVisibleContentRect ,
    bool useFixedLayout, ScrollbarMode horizontalScrollbarMode, bool horizontalLock,
    ScrollbarMode verticalScrollbarMode, bool verticalLock)
{
    ASSERT(this);
    ASSERT(m_page);

    bool isMainFrame = this == m_page->mainFrame();

    if (isMainFrame && view())
        view()->setParentVisible(false);

    setView(0);

    RefPtr<FrameView> frameView;
    if (isMainFrame) {
        frameView = FrameView::create(this, viewportSize);
        frameView->setFixedLayoutSize(fixedLayoutSize);
        frameView->setFixedVisibleContentRect(fixedVisibleContentRect);
        frameView->setUseFixedLayout(useFixedLayout);
    } else
        frameView = FrameView::create(this);

    frameView->setScrollbarModes(horizontalScrollbarMode, verticalScrollbarMode, horizontalLock, verticalLock);

    setView(frameView);

    if (backgroundColor.isValid())
        frameView->updateBackgroundRecursively(backgroundColor, transparent);

    if (isMainFrame)
        frameView->setParentVisible(true);

    if (ownerRenderer())
        ownerRenderer()->setWidget(frameView);

    if (HTMLFrameOwnerElement* owner = ownerElement())
        view()->setCanHaveScrollbars(owner->scrollingMode() != ScrollbarAlwaysOff);
}

#if USE(TILED_BACKING_STORE)
void Frame::setTiledBackingStoreEnabled(bool enabled)
{
    if (!enabled) {
        m_tiledBackingStore.clear();
        return;
    }
    if (m_tiledBackingStore)
        return;
    m_tiledBackingStore = adoptPtr(new TiledBackingStore(this));
    m_tiledBackingStore->setCommitTileUpdatesOnIdleEventLoop(true);
    if (m_view)
        m_view->setPaintsEntireContents(true);
}

void Frame::tiledBackingStorePaintBegin()
{
    if (!m_view)
        return;
    m_view->updateLayoutAndStyleIfNeededRecursive();
    m_view->flushDeferredRepaints();
}

void Frame::tiledBackingStorePaint(GraphicsContext* context, const IntRect& rect)
{
    if (!m_view)
        return;
    m_view->paintContents(context, rect);
}

void Frame::tiledBackingStorePaintEnd(const Vector<IntRect>& paintedArea)
{
    if (!m_page || !m_view)
        return;
    unsigned size = paintedArea.size();
    // Request repaint from the system
    for (unsigned n = 0; n < size; ++n)
        m_page->chrome().invalidateContentsAndRootView(m_view->contentsToRootView(paintedArea[n]), false);
}

IntRect Frame::tiledBackingStoreContentsRect()
{
    if (!m_view)
        return IntRect();
    return IntRect(IntPoint(), m_view->contentsSize());
}

IntRect Frame::tiledBackingStoreVisibleRect()
{
    if (!m_page)
        return IntRect();
    return m_page->chrome().client()->visibleRectForTiledBackingStore();
}

Color Frame::tiledBackingStoreBackgroundColor() const
{
    if (!m_view)
        return Color();
    return m_view->baseBackgroundColor();
}
#endif

String Frame::layerTreeAsText(LayerTreeFlags flags) const
{
#if USE(ACCELERATED_COMPOSITING)
    document()->updateLayout();

    if (!contentRenderer())
        return String();

    return contentRenderer()->compositor()->layerTreeAsText(flags);
#else
    UNUSED_PARAM(flags);
    return String();
#endif
}

String Frame::trackedRepaintRectsAsText() const
{
    if (!m_view)
        return String();
    return m_view->trackedRepaintRectsAsText();
}

void Frame::setPageZoomFactor(float factor)
{
    setPageAndTextZoomFactors(factor, m_textZoomFactor);
}

void Frame::setTextZoomFactor(float factor)
{
    setPageAndTextZoomFactors(m_pageZoomFactor, factor);
}

void Frame::setPageAndTextZoomFactors(float pageZoomFactor, float textZoomFactor)
{
    if (m_pageZoomFactor == pageZoomFactor && m_textZoomFactor == textZoomFactor)
        return;

    Page* page = this->page();
    if (!page)
        return;

    Document* document = this->document();
    if (!document)
        return;

    m_editor->dismissCorrectionPanelAsIgnored();

#if ENABLE(SVG)
    // Respect SVGs zoomAndPan="disabled" property in standalone SVG documents.
    // FIXME: How to handle compound documents + zoomAndPan="disabled"? Needs SVG WG clarification.
    if (document->isSVGDocument()) {
        if (!toSVGDocument(document)->zoomAndPanEnabled())
            return;
    }
#endif

    if (m_pageZoomFactor != pageZoomFactor) {
        if (FrameView* view = this->view()) {
            // Update the scroll position when doing a full page zoom, so the content stays in relatively the same position.
            LayoutPoint scrollPosition = view->scrollPosition();
            float percentDifference = (pageZoomFactor / m_pageZoomFactor);
            view->setScrollPosition(IntPoint(scrollPosition.x() * percentDifference, scrollPosition.y() * percentDifference));
        }
    }

    m_pageZoomFactor = pageZoomFactor;
    m_textZoomFactor = textZoomFactor;

    document->recalcStyle(Node::Force);

    for (RefPtr<Frame> child = tree()->firstChild(); child; child = child->tree()->nextSibling())
        child->setPageAndTextZoomFactors(m_pageZoomFactor, m_textZoomFactor);

    if (FrameView* view = this->view()) {
        if (document->renderer() && document->renderer()->needsLayout() && view->didFirstLayout())
            view->layout();
    }

    if (page->mainFrame() == this)
        pageCache()->markPagesForFullStyleRecalc(page);
}

float Frame::frameScaleFactor() const
{
    Page* page = this->page();

    // Main frame is scaled with respect to he container but inner frames are not scaled with respect to the main frame.
    if (!page || page->mainFrame() != this || page->settings()->applyPageScaleFactorInCompositor())
        return 1;

    return page->pageScaleFactor();
}

void Frame::suspendActiveDOMObjectsAndAnimations()
{
    bool wasSuspended = activeDOMObjectsAndAnimationsSuspended();

    m_activeDOMObjectsAndAnimationsSuspendedCount++;

    if (wasSuspended)
        return;

    if (document()) {
        document()->suspendScriptedAnimationControllerCallbacks();
        animation()->suspendAnimationsForDocument(document());
        document()->suspendActiveDOMObjects(ActiveDOMObject::PageWillBeSuspended);
    }
}

void Frame::resumeActiveDOMObjectsAndAnimations()
{
    ASSERT(activeDOMObjectsAndAnimationsSuspended());

    m_activeDOMObjectsAndAnimationsSuspendedCount--;

    if (activeDOMObjectsAndAnimationsSuspended())
        return;

    if (document()) {
        document()->resumeActiveDOMObjects(ActiveDOMObject::PageWillBeSuspended);
        animation()->resumeAnimationsForDocument(document());
        document()->resumeScriptedAnimationControllerCallbacks();
    }

    if (m_view)
        m_view->resumeAnimatingImages();
}

#if USE(ACCELERATED_COMPOSITING)
void Frame::deviceOrPageScaleFactorChanged()
{
    for (RefPtr<Frame> child = tree()->firstChild(); child; child = child->tree()->nextSibling())
        child->deviceOrPageScaleFactorChanged();

    RenderView* root = contentRenderer();
    if (root && root->compositor())
        root->compositor()->deviceOrPageScaleFactorChanged();
}
#endif
void Frame::notifyChromeClientWheelEventHandlerCountChanged() const
{
    // Ensure that this method is being called on the main frame of the page.
    ASSERT(m_page && m_page->mainFrame() == this);

    unsigned count = 0;
    for (const Frame* frame = this; frame; frame = frame->tree()->traverseNext()) {
        if (frame->document())
            count += frame->document()->wheelEventHandlerCount();
    }

    m_page->chrome().client()->numWheelEventHandlersChanged(count);
}

bool Frame::isURLAllowed(const KURL& url) const
{
    // We allow one level of self-reference because some sites depend on that,
    // but we don't allow more than one.
    if (m_page->subframeCount() >= Page::maxNumberOfFrames)
        return false;
    bool foundSelfReference = false;
    for (const Frame* frame = this; frame; frame = frame->tree()->parent()) {
        if (equalIgnoringFragmentIdentifier(frame->document()->url(), url)) {
            if (foundSelfReference)
                return false;
            foundSelfReference = true;
        }
    }
    return true;
}

#if !PLATFORM(MAC) && !PLATFORM(WIN)
struct ScopedFramePaintingState {
    ScopedFramePaintingState(Frame* frame, Node* node)
        : frame(frame)
        , node(node)
        , paintBehavior(frame->view()->paintBehavior())
        , backgroundColor(frame->view()->baseBackgroundColor())
    {
        ASSERT(!node || node->renderer());
        if (node)
            node->renderer()->updateDragState(true);
    }

    ~ScopedFramePaintingState()
    {
        if (node && node->renderer())
            node->renderer()->updateDragState(false);
        frame->view()->setPaintBehavior(paintBehavior);
        frame->view()->setBaseBackgroundColor(backgroundColor);
        frame->view()->setNodeToDraw(0);
    }

    Frame* frame;
    Node* node;
    PaintBehavior paintBehavior;
    Color backgroundColor;
};

DragImageRef Frame::nodeImage(Node* node)
{
    if (!node->renderer())
        return 0;

    const ScopedFramePaintingState state(this, node);

    m_view->setPaintBehavior(state.paintBehavior | PaintBehaviorFlattenCompositingLayers);

    // When generating the drag image for an element, ignore the document background.
    m_view->setBaseBackgroundColor(Color::transparent);
    m_doc->updateLayout();
    m_view->setNodeToDraw(node); // Enable special sub-tree drawing mode.

    // Document::updateLayout may have blown away the original RenderObject.
    RenderObject* renderer = node->renderer();
    if (!renderer)
      return 0;

    LayoutRect topLevelRect;
    IntRect paintingRect = pixelSnappedIntRect(renderer->paintingRootRect(topLevelRect));

    float deviceScaleFactor = 1;
    if (m_page)
        deviceScaleFactor = m_page->deviceScaleFactor();
    paintingRect.setWidth(paintingRect.width() * deviceScaleFactor);
    paintingRect.setHeight(paintingRect.height() * deviceScaleFactor);

    OwnPtr<ImageBuffer> buffer(ImageBuffer::create(paintingRect.size(), deviceScaleFactor, ColorSpaceDeviceRGB));
    if (!buffer)
        return 0;
    buffer->context()->translate(-paintingRect.x(), -paintingRect.y());
    buffer->context()->clip(FloatRect(0, 0, paintingRect.maxX(), paintingRect.maxY()));

    m_view->paintContents(buffer->context(), paintingRect);

    RefPtr<Image> image = buffer->copyImage();
    return createDragImageFromImage(image.get(), renderer->shouldRespectImageOrientation());
}

DragImageRef Frame::dragImageForSelection()
{
    if (!selection()->isRange())
        return 0;

    const ScopedFramePaintingState state(this, 0);
    m_view->setPaintBehavior(PaintBehaviorSelectionOnly);
    m_doc->updateLayout();

    IntRect paintingRect = enclosingIntRect(selection()->bounds());

    float deviceScaleFactor = 1;
    if (m_page)
        deviceScaleFactor = m_page->deviceScaleFactor();
    paintingRect.setWidth(paintingRect.width() * deviceScaleFactor);
    paintingRect.setHeight(paintingRect.height() * deviceScaleFactor);

    OwnPtr<ImageBuffer> buffer(ImageBuffer::create(paintingRect.size(), deviceScaleFactor, ColorSpaceDeviceRGB));
    if (!buffer)
        return 0;
    buffer->context()->translate(-paintingRect.x(), -paintingRect.y());
    buffer->context()->clip(FloatRect(0, 0, paintingRect.maxX(), paintingRect.maxY()));

    m_view->paintContents(buffer->context(), paintingRect);

    RefPtr<Image> image = buffer->copyImage();
    return createDragImageFromImage(image.get());
}

#endif

} // namespace WebCore
