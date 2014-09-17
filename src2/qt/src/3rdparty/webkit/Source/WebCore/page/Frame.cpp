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

#include "ApplyStyleCommand.h"
#include "CSSComputedStyleDeclaration.h"
#include "CSSMutableStyleDeclaration.h"
#include "CSSProperty.h"
#include "CSSPropertyNames.h"
#include "CachedCSSStyleSheet.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "DOMWindow.h"
#include "CachedResourceLoader.h"
#include "DocumentType.h"
#include "EditingText.h"
#include "EditorClient.h"
#include "EventNames.h"
#include "FloatQuad.h"
#include "FocusController.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
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
#include "Logging.h"
#include "MediaFeatureNames.h"
#include "MediaStreamFrameController.h"
#include "Navigator.h"
#include "NodeList.h"
#include "Page.h"
#include "PageGroup.h"
#include "RegularExpression.h"
#include "RenderLayer.h"
#include "RenderPart.h"
#include "RenderTableCell.h"
#include "RenderTextControl.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "RuntimeEnabledFeatures.h"
#include "ScriptController.h"
#include "ScriptSourceCode.h"
#include "ScriptValue.h"
#include "Settings.h"
#include "TextIterator.h"
#include "TextResourceDecoder.h"
#include "UserContentURLPattern.h"
#include "UserTypingGestureIndicator.h"
#include "XMLNSNames.h"
#include "XMLNames.h"
#include "htmlediting.h"
#include "markup.h"
#include "npruntime_impl.h"
#include "visible_units.h"
#include <wtf/RefCountedLeakCounter.h>
#include <wtf/StdLibExtras.h>

#if USE(ACCELERATED_COMPOSITING)
#include "RenderLayerCompositor.h"
#endif

#if USE(JSC)
#include "JSDOMWindowShell.h"
#include "runtime_root.h"
#endif

#include "MathMLNames.h"
#include "SVGNames.h"
#include "XLinkNames.h"

#if ENABLE(SVG)
#include "SVGDocument.h"
#include "SVGDocumentExtensions.h"
#endif

#if ENABLE(TILED_BACKING_STORE)
#include "TiledBackingStore.h"
#endif

using namespace std;

namespace WebCore {

using namespace HTMLNames;

#ifndef NDEBUG
static WTF::RefCountedLeakCounter frameCounter("Frame");
#endif

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
    , m_script(this)
    , m_editor(this)
    , m_selectionController(this)
    , m_eventHandler(this)
    , m_animationController(this)
    , m_lifeSupportTimer(this, &Frame::lifeSupportTimerFired)
    , m_pageZoomFactor(parentPageZoomFactor(this))
    , m_textZoomFactor(parentTextZoomFactor(this))
    , m_pageScaleFactor(1)
#if ENABLE(ORIENTATION_EVENTS)
    , m_orientation(0)
#endif
    , m_inViewSourceMode(false)
    , m_isDisconnected(false)
    , m_excludeFromTextSearch(false)
#if ENABLE(MEDIA_STREAM)
    , m_mediaStreamFrameController(RuntimeEnabledFeatures::mediaStreamEnabled() ? adoptPtr(new MediaStreamFrameController(this)) : PassOwnPtr<MediaStreamFrameController>())
#endif
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

    if (!ownerElement) {
#if ENABLE(TILED_BACKING_STORE)
        // Top level frame only for now.
        setTiledBackingStoreEnabled(page->settings()->tiledBackingStoreEnabled());
#endif
    } else {
        page->incrementFrameCount();

        // Make sure we will not end up with two frames referencing the same owner element.
        Frame*& contentFrameSlot = ownerElement->m_contentFrame;
        ASSERT(!contentFrameSlot || contentFrameSlot->ownerElement() != ownerElement);
        contentFrameSlot = this;
    }

#ifndef NDEBUG
    frameCounter.increment();
#endif
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

    ASSERT(!m_lifeSupportTimer.isActive());

#ifndef NDEBUG
    frameCounter.decrement();
#endif

    disconnectOwnerElement();

    if (m_domWindow)
        m_domWindow->disconnectFrame();

#if ENABLE(MEDIA_STREAM)
    if (m_mediaStreamFrameController)
        m_mediaStreamFrameController->disconnectFrame();
#endif

    HashSet<DOMWindow*>::iterator end = m_liveFormerWindows.end();
    for (HashSet<DOMWindow*>::iterator it = m_liveFormerWindows.begin(); it != end; ++it)
        (*it)->disconnectFrame();

    HashSet<FrameDestructionObserver*>::iterator stop = m_destructionObservers.end();
    for (HashSet<FrameDestructionObserver*>::iterator it = m_destructionObservers.begin(); it != stop; ++it)
        (*it)->frameDestroyed();

    if (m_view) {
        m_view->hide();
        m_view->clearFrame();
    }

    ASSERT(!m_lifeSupportTimer.isActive());
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
        m_view->detachCustomScrollbars();

    // Detach the document now, so any onUnload handlers get run - if
    // we wait until the view is destroyed, then things won't be
    // hooked up enough for some JavaScript calls to work.
    if (!view && m_doc && m_doc->attached() && !m_doc->inPageCache()) {
        // FIXME: We don't call willRemove here. Why is that OK?
        m_doc->detach();
    }
    
    if (m_view)
        m_view->unscheduleRelayout();
    
    eventHandler()->clear();

    m_view = view;

    // Only one form submission is allowed per view of a part.
    // Since this part may be getting reused as a result of being
    // pulled from the back/forward cache, reset this flag.
    loader()->resetMultipleFormSubmissionProtection();
    
#if ENABLE(TILED_BACKING_STORE)
    if (m_view && tiledBackingStore())
        m_view->setPaintsEntireContents(true);
#endif
}

void Frame::setDocument(PassRefPtr<Document> newDoc)
{
    ASSERT(!newDoc || newDoc->frame());
    if (m_doc && m_doc->attached() && !m_doc->inPageCache()) {
        // FIXME: We don't call willRemove here. Why is that OK?
        m_doc->detach();
    }

    m_doc = newDoc;
    selection()->updateSecureKeyboardEntryIfActive();

    if (m_doc && !m_doc->attached())
        m_doc->attach();

    // Update the cached 'document' property, which is now stale.
    m_script.updateDocument();

    if (m_page)
        m_page->updateViewportArguments();
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
        for (Node* n = aboveCell->firstChild(); n; n = n->traverseNextNode(aboveCell)) {
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
    for (n = element->traversePreviousNode();
         n && lengthSearched < charsSearchedThreshold;
         n = n->traversePreviousNode())
    {
        if (n->hasTagName(formTag)
            || (n->isHTMLElement() && static_cast<Element*>(n)->isFormControlElement()))
        {
            // We hit another form element or the start of the form - bail out
            break;
        } else if (n->hasTagName(tdTag) && !startingTableCell) {
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
    String resultFromNameAttribute = matchLabelsAgainstString(labels, element->getAttribute(nameAttr));
    if (!resultFromNameAttribute.isEmpty())
        return resultFromNameAttribute;
    
    return matchLabelsAgainstString(labels, element->getAttribute(idAttr));
}
    
Color Frame::getDocumentBackgroundColor() const
{
    // <https://bugs.webkit.org/show_bug.cgi?id=59540> We blend the background color of
    // the document and the body against the base background color of the frame view.
    // Background images are unfortunately impractical to include.

    // Return invalid Color objects whenever there is insufficient information.
    if (!m_doc)
        return Color();

    Element* htmlElement = m_doc->documentElement();
    Element* bodyElement = m_doc->body();

    // start as invalid colors
    Color htmlBackgroundColor;
    Color bodyBackgroundColor;
    if (htmlElement && htmlElement->renderer())
        htmlBackgroundColor = htmlElement->renderer()->style()->visitedDependentColor(CSSPropertyBackgroundColor);
    if (bodyElement && bodyElement->renderer())
        bodyBackgroundColor = bodyElement->renderer()->style()->visitedDependentColor(CSSPropertyBackgroundColor);
    
    if (!bodyBackgroundColor.isValid()) {
        if (!htmlBackgroundColor.isValid())
            return Color();
        return view()->baseBackgroundColor().blend(htmlBackgroundColor);
    }
    
    if (!htmlBackgroundColor.isValid())
        return view()->baseBackgroundColor().blend(bodyBackgroundColor);
    
    // We take the aggregate of the base background color
    // the <html> background color, and the <body>
    // background color to find the document color. The
    // addition of the base background color is not
    // technically part of the document background, but it
    // otherwise poses problems when the aggregate is not
    // fully opaque.
    return view()->baseBackgroundColor().blend(htmlBackgroundColor).blend(bodyBackgroundColor);
}

void Frame::setPrinting(bool printing, const FloatSize& pageSize, float maximumShrinkRatio, AdjustViewSizeOrNot shouldAdjustViewSize)
{
    m_pageResets.clear();

    m_doc->setPrinting(printing);
    view()->adjustMediaTypeForPrinting(printing);

    m_doc->styleSelectorChanged(RecalcStyleImmediately);
    if (printing)
        view()->forceLayoutForPagination(pageSize, maximumShrinkRatio, shouldAdjustViewSize);
    else {
        view()->forceLayout();
        if (shouldAdjustViewSize == AdjustViewSize)
            view()->adjustViewSize();
    }

    // Subframes of the one we're printing don't lay out to the page size.
    for (Frame* child = tree()->firstChild(); child; child = child->tree()->nextSibling())
        child->setPrinting(printing, IntSize(), 0, shouldAdjustViewSize);
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
        injectUserScriptsForWorld(it->first.get(), *it->second, injectionTime);
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
            m_script.evaluateInWorld(ScriptSourceCode(script->source(), script->url()), world);
    }
}

#ifndef NDEBUG
static HashSet<Frame*>& keepAliveSet()
{
    DEFINE_STATIC_LOCAL(HashSet<Frame*>, staticKeepAliveSet, ());
    return staticKeepAliveSet;
}
#endif

void Frame::keepAlive()
{
    if (m_lifeSupportTimer.isActive())
        return;
#ifndef NDEBUG
    keepAliveSet().add(this);
#endif
    ref();
    m_lifeSupportTimer.startOneShot(0);
}

#ifndef NDEBUG
void Frame::cancelAllKeepAlive()
{
    HashSet<Frame*>::iterator end = keepAliveSet().end();
    for (HashSet<Frame*>::iterator it = keepAliveSet().begin(); it != end; ++it) {
        Frame* frame = *it;
        frame->m_lifeSupportTimer.stop();
        frame->deref();
    }
    keepAliveSet().clear();
}
#endif

void Frame::lifeSupportTimerFired(Timer<Frame>*)
{
#ifndef NDEBUG
    keepAliveSet().remove(this);
#endif
    deref();
}

void Frame::clearDOMWindow()
{
    if (m_domWindow) {
        m_liveFormerWindows.add(m_domWindow.get());
        m_domWindow->clear();
    }
    m_domWindow = 0;
}

RenderView* Frame::contentRenderer() const
{
    Document* doc = document();
    if (!doc)
        return 0;
    RenderObject* object = doc->renderer();
    if (!object)
        return 0;
    ASSERT(object->isRenderView());
    return toRenderView(object);
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
    ASSERT(widget->isFrameView());
    return static_cast<const FrameView*>(widget)->frame();
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

void Frame::setDOMWindow(DOMWindow* domWindow)
{
    if (m_domWindow) {
        m_liveFormerWindows.add(m_domWindow.get());
        m_domWindow->clear();
    }
    m_domWindow = domWindow;
}

DOMWindow* Frame::domWindow() const
{
    if (!m_domWindow)
        m_domWindow = DOMWindow::create(const_cast<Frame*>(this));

    return m_domWindow.get();
}

void Frame::clearFormerDOMWindow(DOMWindow* window)
{
    m_liveFormerWindows.remove(window);
}

void Frame::pageDestroyed()
{
    if (Frame* parent = tree()->parent())
        parent->loader()->checkLoadComplete();

    if (m_domWindow) {
        m_domWindow->resetGeolocation();
        m_domWindow->pageDestroyed();
    }

#if ENABLE(MEDIA_STREAM)
    if (m_mediaStreamFrameController)
        m_mediaStreamFrameController->disconnectPage();
#endif

    // FIXME: It's unclear as to why this is called more than once, but it is,
    // so page() could be NULL.
    if (page() && page()->focusController()->focusedFrame() == this)
        page()->focusController()->setFocusedFrame(0);

    script()->clearWindowShell();
    script()->clearScriptObjects();
    script()->updatePlatformScriptObjects();

    detachFromPage();
}

void Frame::disconnectOwnerElement()
{
    if (m_ownerElement) {
        if (Document* doc = document())
            doc->clearAXObjectCache();
        m_ownerElement->m_contentFrame = 0;
        if (m_page)
            m_page->decrementFrameCount();
    }
    m_ownerElement = 0;
}

// The frame is moved in DOM, potentially to another page.
void Frame::transferChildFrameToNewDocument()
{
    ASSERT(m_ownerElement);
    Frame* newParent = m_ownerElement->document()->frame();
    ASSERT(newParent);
    bool didTransfer = false;

    // Switch page.
    Page* newPage = newParent->page();
    Page* oldPage = m_page;
    if (m_page != newPage) {
        if (m_page) {
            if (m_page->focusController()->focusedFrame() == this)
                m_page->focusController()->setFocusedFrame(0);

             m_page->decrementFrameCount();
        }

        // FIXME: We should ideally allow existing Geolocation activities to continue
        // when the Geolocation's iframe is reparented.
        // See https://bugs.webkit.org/show_bug.cgi?id=55577
        // and https://bugs.webkit.org/show_bug.cgi?id=52877
        if (m_domWindow)
            m_domWindow->resetGeolocation();

#if ENABLE(MEDIA_STREAM)
        if (m_mediaStreamFrameController)
            m_mediaStreamFrameController->transferToNewPage(newPage);
#endif
        m_page = newPage;

        if (newPage)
            newPage->incrementFrameCount();

        didTransfer = true;
    }

    // Update the frame tree.
    didTransfer = newParent->tree()->transferChild(this) || didTransfer;

    // Avoid unnecessary calls to client and frame subtree if the frame ended
    // up on the same page and under the same parent frame.
    if (didTransfer) {
        // Let external clients update themselves.
        loader()->client()->didTransferChildFrameToNewDocument(oldPage);

        // Update resource tracking now that frame could be in a different page.
        if (oldPage != newPage)
            loader()->transferLoadingResourcesFromPage(oldPage);

        // Do the same for all the children.
        for (Frame* child = tree()->firstChild(); child; child = child->tree()->nextSibling())
            child->transferChildFrameToNewDocument();
    }
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
    HitTestResult result = eventHandler()->hitTestResultAtPoint(framePoint, true);
    Node* node = result.innerNode();
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
        result = eventHandler()->hitTestResultAtPoint(pt, false);
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
        IntRect rect = editor()->firstRectForRange(previousCharacterRange.get());
        if (rect.contains(framePoint))
            return previousCharacterRange.release();
    }

    VisiblePosition next = position.next();
    if (RefPtr<Range> nextCharacterRange = makeRange(position, next)) {
        IntRect rect = editor()->firstRectForRange(nextCharacterRange.get());
        if (rect.contains(framePoint))
            return nextCharacterRange.release();
    }

    return 0;
}

void Frame::createView(const IntSize& viewportSize,
                       const Color& backgroundColor, bool transparent,
                       const IntSize& fixedLayoutSize, bool useFixedLayout,
                       ScrollbarMode horizontalScrollbarMode, bool horizontalLock,
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

#if ENABLE(TILED_BACKING_STORE)
void Frame::setTiledBackingStoreEnabled(bool enabled)
{
    if (!enabled) {
        m_tiledBackingStore.clear();
        return;
    }
    if (m_tiledBackingStore)
        return;
    m_tiledBackingStore = adoptPtr(new TiledBackingStore(this));
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
    for (int n = 0; n < size; ++n)
        m_page->chrome()->invalidateContentsAndWindow(m_view->contentsToWindow(paintedArea[n]), false);
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
    return m_page->chrome()->client()->visibleRectForTiledBackingStore();
}

Color Frame::tiledBackingStoreBackgroundColor() const
{
    if (!m_view)
        return Color();
    return m_view->baseBackgroundColor();
}
#endif

String Frame::layerTreeAsText(bool showDebugInfo) const
{
#if USE(ACCELERATED_COMPOSITING)
    document()->updateLayout();

    if (!contentRenderer())
        return String();

    return contentRenderer()->compositor()->layerTreeAsText(showDebugInfo);
#else
    return String();
#endif
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

    m_editor.dismissCorrectionPanelAsIgnored();

#if ENABLE(SVG)
    // Respect SVGs zoomAndPan="disabled" property in standalone SVG documents.
    // FIXME: How to handle compound documents + zoomAndPan="disabled"? Needs SVG WG clarification.
    if (document->isSVGDocument()) {
        if (!static_cast<SVGDocument*>(document)->zoomAndPanEnabled())
            return;
        if (document->renderer())
            document->renderer()->setNeedsLayout(true);
    }
#endif

    if (m_pageZoomFactor != pageZoomFactor) {
        if (FrameView* view = this->view()) {
            // Update the scroll position when doing a full page zoom, so the content stays in relatively the same position.
            IntPoint scrollPosition = view->scrollPosition();
            float percentDifference = (pageZoomFactor / m_pageZoomFactor);
            view->setScrollPosition(IntPoint(scrollPosition.x() * percentDifference, scrollPosition.y() * percentDifference));
        }
    }

    m_pageZoomFactor = pageZoomFactor;
    m_textZoomFactor = textZoomFactor;

    document->recalcStyle(Node::Force);

    for (Frame* child = tree()->firstChild(); child; child = child->tree()->nextSibling())
        child->setPageAndTextZoomFactors(m_pageZoomFactor, m_textZoomFactor);

    if (FrameView* view = this->view()) {
        if (document->renderer() && document->renderer()->needsLayout() && view->didFirstLayout())
            view->layout();
    }
}

#if USE(ACCELERATED_COMPOSITING)
void Frame::updateContentsScale(float scale)
{
    for (Frame* child = tree()->firstChild(); child; child = child->tree()->nextSibling())
        child->updateContentsScale(scale);

    RenderView* root = contentRenderer();
    if (root && root->compositor())
        root->compositor()->updateContentsScale(scale);
}
#endif

void Frame::scalePage(float scale, const IntPoint& origin)
{
    Document* document = this->document();
    if (!document)
        return;

    if (scale != m_pageScaleFactor) {
        m_pageScaleFactor = scale;

        if (document->renderer())
            document->renderer()->setNeedsLayout(true);

        document->recalcStyle(Node::Force);

#if USE(ACCELERATED_COMPOSITING)
        updateContentsScale(scale);
#endif
    }

    if (FrameView* view = this->view()) {
        if (document->renderer() && document->renderer()->needsLayout() && view->didFirstLayout())
            view->layout();
        view->setScrollPosition(origin);
    }
}

} // namespace WebCore
