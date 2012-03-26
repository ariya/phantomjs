/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2008, 2009 Google Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
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
#include "Document.h"

#include "AXObjectCache.h"
#include "AnimationController.h"
#include "Attr.h"
#include "Attribute.h"
#include "CDATASection.h"
#include "CSSPrimitiveValueCache.h"
#include "CSSStyleSelector.h"
#include "CSSStyleSheet.h"
#include "CSSValueKeywords.h"
#include "CachedCSSStyleSheet.h"
#include "CachedResourceLoader.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "Comment.h"
#include "Console.h"
#include "ContentSecurityPolicy.h"
#include "CookieJar.h"
#include "CustomEvent.h"
#include "DateComponents.h"
#include "DOMImplementation.h"
#include "DOMWindow.h"
#include "DeviceMotionEvent.h"
#include "DeviceOrientationEvent.h"
#include "DocumentFragment.h"
#include "DocumentLoader.h"
#include "DocumentMarkerController.h"
#include "DocumentType.h"
#include "EditingText.h"
#include "Editor.h"
#include "Element.h"
#include "EntityReference.h"
#include "Event.h"
#include "EventHandler.h"
#include "EventListener.h"
#include "EventNames.h"
#include "EventQueue.h"
#include "ExceptionCode.h"
#include "FocusController.h"
#include "FormAssociatedElement.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "HashChangeEvent.h"
#include "HTMLAllCollection.h"
#include "HTMLAnchorElement.h"
#include "HTMLBodyElement.h"
#include "HTMLCanvasElement.h"
#include "HTMLCollection.h"
#include "HTMLDocument.h"
#include "HTMLElementFactory.h"
#include "HTMLFrameOwnerElement.h"
#include "HTMLHeadElement.h"
#include "HTMLIFrameElement.h"
#include "HTMLInputElement.h"
#include "HTMLLinkElement.h"
#include "HTMLMapElement.h"
#include "HTMLNameCollection.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "HTMLStyleElement.h"
#include "HTMLTitleElement.h"
#include "HTTPParsers.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "ImageLoader.h"
#include "InspectorInstrumentation.h"
#include "KeyboardEvent.h"
#include "Logging.h"
#include "MediaQueryList.h"
#include "MediaQueryMatcher.h"
#include "MessageEvent.h"
#include "MouseEvent.h"
#include "MouseEventWithHitTestResults.h"
#include "MutationEvent.h"
#include "NameNodeList.h"
#include "NestingLevelIncrementer.h"
#include "NodeFilter.h"
#include "NodeIterator.h"
#include "NodeWithIndex.h"
#include "OverflowEvent.h"
#include "Page.h"
#include "PageGroup.h"
#include "PageTransitionEvent.h"
#include "PlatformKeyboardEvent.h"
#include "PopStateEvent.h"
#include "ProcessingInstruction.h"
#include "ProgressEvent.h"
#include "RegisteredEventListener.h"
#include "RenderArena.h"
#include "RenderLayer.h"
#include "RenderLayerBacking.h"
#include "RenderTextControl.h"
#include "RenderView.h"
#include "RenderWidget.h"
#include "ScopedEventQueue.h"
#include "ScriptCallStack.h"
#include "ScriptController.h"
#include "ScriptElement.h"
#include "ScriptEventListener.h"
#include "ScriptRunner.h"
#include "SecurityOrigin.h"
#include "SegmentedString.h"
#include "SelectionController.h"
#include "Settings.h"
#include "ShadowRoot.h"
#include "StaticHashSetNodeList.h"
#include "StyleSheetList.h"
#include "TextEvent.h"
#include "TextResourceDecoder.h"
#include "Timer.h"
#include "TransformSource.h"
#include "TreeWalker.h"
#include "UIEvent.h"
#include "UserContentURLPattern.h"
#include "WebKitAnimationEvent.h"
#include "WebKitTransitionEvent.h"
#include "WheelEvent.h"
#include "XMLDocumentParser.h"
#include "XMLHttpRequest.h"
#include "XMLNSNames.h"
#include "XMLNames.h"
#include "htmlediting.h"
#include <wtf/CurrentTime.h>
#include <wtf/HashFunctions.h>
#include <wtf/MainThread.h>
#include <wtf/PassRefPtr.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringBuffer.h>

#if ENABLE(SHARED_WORKERS)
#include "SharedWorkerRepository.h"
#endif

#if ENABLE(DOM_STORAGE)
#include "StorageEvent.h"
#endif

#if ENABLE(XPATH)
#include "XPathEvaluator.h"
#include "XPathExpression.h"
#include "XPathNSResolver.h"
#include "XPathResult.h"
#endif

#if ENABLE(XSLT)
#include "XSLTProcessor.h"
#endif

#if ENABLE(SVG)
#include "SVGDocumentExtensions.h"
#include "SVGElementFactory.h"
#include "SVGNames.h"
#include "SVGStyleElement.h"
#include "SVGZoomEvent.h"
#endif

#if ENABLE(TOUCH_EVENTS)
#if USE(V8)
#include "RuntimeEnabledFeatures.h"
#endif
#include "TouchEvent.h"
#endif

#if ENABLE(MATHML)
#include "MathMLElement.h"
#include "MathMLElementFactory.h"
#include "MathMLNames.h"
#endif

#if ENABLE(XHTMLMP)
#include "HTMLNoScriptElement.h"
#endif

#if ENABLE(FULLSCREEN_API)
#include "RenderFullScreen.h"
#endif

#if ENABLE(REQUEST_ANIMATION_FRAME)
#include "RequestAnimationFrameCallback.h"
#include "ScriptedAnimationController.h"
#endif

using namespace std;
using namespace WTF;
using namespace Unicode;

namespace WebCore {

using namespace HTMLNames;

// #define INSTRUMENT_LAYOUT_SCHEDULING 1

static const unsigned cMaxWriteRecursionDepth = 21;

// This amount of time must have elapsed before we will even consider scheduling a layout without a delay.
// FIXME: For faster machines this value can really be lowered to 200.  250 is adequate, but a little high
// for dual G5s. :)
static const int cLayoutScheduleThreshold = 250;

// DOM Level 2 says (letters added):
//
// a) Name start characters must have one of the categories Ll, Lu, Lo, Lt, Nl.
// b) Name characters other than Name-start characters must have one of the categories Mc, Me, Mn, Lm, or Nd.
// c) Characters in the compatibility area (i.e. with character code greater than #xF900 and less than #xFFFE) are not allowed in XML names.
// d) Characters which have a font or compatibility decomposition (i.e. those with a "compatibility formatting tag" in field 5 of the database -- marked by field 5 beginning with a "<") are not allowed.
// e) The following characters are treated as name-start characters rather than name characters, because the property file classifies them as Alphabetic: [#x02BB-#x02C1], #x0559, #x06E5, #x06E6.
// f) Characters #x20DD-#x20E0 are excluded (in accordance with Unicode, section 5.14).
// g) Character #x00B7 is classified as an extender, because the property list so identifies it.
// h) Character #x0387 is added as a name character, because #x00B7 is its canonical equivalent.
// i) Characters ':' and '_' are allowed as name-start characters.
// j) Characters '-' and '.' are allowed as name characters.
//
// It also contains complete tables. If we decide it's better, we could include those instead of the following code.

static inline bool isValidNameStart(UChar32 c)
{
    // rule (e) above
    if ((c >= 0x02BB && c <= 0x02C1) || c == 0x559 || c == 0x6E5 || c == 0x6E6)
        return true;

    // rule (i) above
    if (c == ':' || c == '_')
        return true;

    // rules (a) and (f) above
    const uint32_t nameStartMask = Letter_Lowercase | Letter_Uppercase | Letter_Other | Letter_Titlecase | Number_Letter;
    if (!(Unicode::category(c) & nameStartMask))
        return false;

    // rule (c) above
    if (c >= 0xF900 && c < 0xFFFE)
        return false;

    // rule (d) above
    DecompositionType decompType = decompositionType(c);
    if (decompType == DecompositionFont || decompType == DecompositionCompat)
        return false;

    return true;
}

static inline bool isValidNamePart(UChar32 c)
{
    // rules (a), (e), and (i) above
    if (isValidNameStart(c))
        return true;

    // rules (g) and (h) above
    if (c == 0x00B7 || c == 0x0387)
        return true;

    // rule (j) above
    if (c == '-' || c == '.')
        return true;

    // rules (b) and (f) above
    const uint32_t otherNamePartMask = Mark_NonSpacing | Mark_Enclosing | Mark_SpacingCombining | Letter_Modifier | Number_DecimalDigit;
    if (!(Unicode::category(c) & otherNamePartMask))
        return false;

    // rule (c) above
    if (c >= 0xF900 && c < 0xFFFE)
        return false;

    // rule (d) above
    DecompositionType decompType = decompositionType(c);
    if (decompType == DecompositionFont || decompType == DecompositionCompat)
        return false;

    return true;
}

static Widget* widgetForNode(Node* focusedNode)
{
    if (!focusedNode)
        return 0;
    RenderObject* renderer = focusedNode->renderer();
    if (!renderer || !renderer->isWidget())
        return 0;
    return toRenderWidget(renderer)->widget();
}

static bool acceptsEditingFocus(Node* node)
{
    ASSERT(node);
    ASSERT(node->rendererIsEditable());

    Node* root = node->rootEditableElement();
    Frame* frame = node->document()->frame();
    if (!frame || !root)
        return false;

    return frame->editor()->shouldBeginEditing(rangeOfContents(root).get());
}

static bool disableRangeMutation(Page* page)
{
    // This check is made on super-hot code paths, so we only want this on Leopard.
#ifdef TARGETING_LEOPARD
    // Disable Range mutation on document modifications in Leopard Mail.
    // See <rdar://problem/5865171>
    return page && (page->settings()->needsLeopardMailQuirks() || page->settings()->needsTigerMailQuirks());
#else
    UNUSED_PARAM(page);
    return false;
#endif
}

static HashSet<Document*>* documentsThatNeedStyleRecalc = 0;

class DocumentWeakReference : public ThreadSafeRefCounted<DocumentWeakReference> {
public:
    static PassRefPtr<DocumentWeakReference> create(Document* document)
    {
        return adoptRef(new DocumentWeakReference(document));
    }

    Document* document()
    {
        ASSERT(isMainThread());
        return m_document;
    }

    void clear()
    {
        ASSERT(isMainThread());
        m_document = 0;
    }

private:
    DocumentWeakReference(Document* document)
        : m_document(document)
    {
        ASSERT(isMainThread());
    }

    Document* m_document;
};

uint64_t Document::s_globalTreeVersion = 0;

Document::Document(Frame* frame, const KURL& url, bool isXHTML, bool isHTML)
    : TreeScope(0)
    , m_guardRefCount(0)
    , m_compatibilityMode(NoQuirksMode)
    , m_compatibilityModeLocked(false)
    , m_domTreeVersion(++s_globalTreeVersion)
    , m_styleSheets(StyleSheetList::create(this))
    , m_readyState(Complete)
    , m_styleRecalcTimer(this, &Document::styleRecalcTimerFired)
    , m_pendingStyleRecalcShouldForce(false)
    , m_frameElementsShouldIgnoreScrolling(false)
    , m_containsValidityStyleRules(false)
    , m_updateFocusAppearanceRestoresSelection(false)
    , m_ignoreDestructiveWriteCount(0)
    , m_titleSetExplicitly(false)
    , m_updateFocusAppearanceTimer(this, &Document::updateFocusAppearanceTimerFired)
    , m_startTime(currentTime())
    , m_overMinimumLayoutThreshold(false)
    , m_extraLayoutDelay(0)
    , m_scriptRunner(ScriptRunner::create(this))
    , m_xmlVersion("1.0")
    , m_xmlStandalone(false)
    , m_savedRenderer(0)
    , m_designMode(inherit)
#if ENABLE(DASHBOARD_SUPPORT)
    , m_hasDashboardRegions(false)
    , m_dashboardRegionsDirty(false)
#endif
    , m_createRenderers(true)
    , m_inPageCache(false)
    , m_accessKeyMapValid(false)
    , m_useSecureKeyboardEntryWhenActive(false)
    , m_isXHTML(isXHTML)
    , m_isHTML(isHTML)
    , m_usesViewSourceStyles(false)
    , m_sawElementsInKnownNamespaces(false)
    , m_usingGeolocation(false)
    , m_eventQueue(EventQueue::create(this))
    , m_weakReference(DocumentWeakReference::create(this))
    , m_idAttributeName(idAttr)
#if ENABLE(FULLSCREEN_API)
    , m_areKeysEnabledInFullScreen(0)
    , m_fullScreenRenderer(0)
    , m_fullScreenChangeDelayTimer(this, &Document::fullScreenChangeDelayTimerFired)
#endif
    , m_loadEventDelayCount(0)
    , m_loadEventDelayTimer(this, &Document::loadEventDelayTimerFired)
    , m_directionSetOnDocumentElement(false)
    , m_writingModeSetOnDocumentElement(false)
    , m_writeRecursionIsTooDeep(false)
    , m_writeRecursionDepth(0)
{
    m_document = this;

    m_pageGroupUserSheetCacheValid = false;

    m_printing = false;
    m_paginatedForScreen = false;

    m_ignoreAutofocus = false;

    m_frame = frame;
    m_documentLoader = frame ? frame->loader()->activeDocumentLoader() : 0;

    // We depend on the url getting immediately set in subframes, but we
    // also depend on the url NOT getting immediately set in opened windows.
    // See fast/dom/early-frame-url.html
    // and fast/dom/location-new-window-no-crash.html, respectively.
    // FIXME: Can/should we unify this behavior?
    if ((frame && frame->ownerElement()) || !url.isEmpty())
        setURL(url);

    m_axObjectCache = 0;

    m_markers = adoptPtr(new DocumentMarkerController);

    m_cachedResourceLoader = adoptPtr(new CachedResourceLoader(this));

    m_visuallyOrdered = false;
    m_bParsing = false;
    m_wellFormed = false;

    m_textColor = Color::black;
    m_listenerTypes = 0;
    setInDocument();
    m_inStyleRecalc = false;
    m_closeAfterStyleRecalc = false;

    m_usesSiblingRules = false;
    m_usesSiblingRulesOverride = false;
    m_usesFirstLineRules = false;
    m_usesFirstLetterRules = false;
    m_usesBeforeAfterRules = false;
    m_usesBeforeAfterRulesOverride = false;
    m_usesRemUnits = false;
    m_usesLinkRules = false;

    m_gotoAnchorNeededAfterStylesheetsLoad = false;

    m_didCalculateStyleSelector = false;
    m_hasDirtyStyleSelector = false;
    m_pendingStylesheets = 0;
    m_ignorePendingStylesheets = false;
    m_hasNodesWithPlaceholderStyle = false;
    m_pendingSheetLayout = NoLayoutWithPendingSheets;

    m_cssTarget = 0;

    resetLinkColor();
    resetVisitedLinkColor();
    resetActiveLinkColor();

    m_processingLoadEvent = false;
    
    initSecurityContext();
    initDNSPrefetch();

    static int docID = 0;
    m_docID = docID++;
#if ENABLE(XHTMLMP)
    m_shouldProcessNoScriptElement = !(m_frame && m_frame->script()->canExecuteScripts(NotAboutToExecuteScript));
#endif
}

Document::~Document()
{
    ASSERT(!renderer());
    ASSERT(!m_inPageCache);
    ASSERT(!m_savedRenderer);
    ASSERT(m_ranges.isEmpty());
    ASSERT(!m_styleRecalcTimer.isActive());
    ASSERT(!m_parentTreeScope);
    ASSERT(!m_guardRefCount);

    m_scriptRunner.clear();

    removeAllEventListeners();

    // Currently we believe that Document can never outlive the parser.
    // Although the Document may be replaced synchronously, DocumentParsers
    // generally keep at least one reference to an Element which would in turn
    // has a reference to the Document.  If you hit this ASSERT, then that
    // assumption is wrong.  DocumentParser::detach() should ensure that even
    // if the DocumentParser outlives the Document it won't cause badness.
    ASSERT(!m_parser || m_parser->refCount() == 1);
    detachParser();
    m_document = 0;
    m_cachedResourceLoader.clear();

    m_renderArena.clear();

    clearAXObjectCache();

    m_decoder = 0;

    for (size_t i = 0; i < m_nameCollectionInfo.size(); ++i)
        deleteAllValues(m_nameCollectionInfo[i]);

    if (m_styleSheets)
        m_styleSheets->documentDestroyed();

    if (m_elemSheet)
        m_elemSheet->clearOwnerNode();
    if (m_mappedElementSheet)
        m_mappedElementSheet->clearOwnerNode();
    if (m_pageUserSheet)
        m_pageUserSheet->clearOwnerNode();
    if (m_pageGroupUserSheets) {
        for (size_t i = 0; i < m_pageGroupUserSheets->size(); ++i)
            (*m_pageGroupUserSheets)[i]->clearOwnerNode();
    }

    m_weakReference->clear();

    if (m_mediaQueryMatcher)
        m_mediaQueryMatcher->documentDestroyed();
}

void Document::removedLastRef()
{
    ASSERT(!m_deletionHasBegun);
    if (m_guardRefCount) {
        // If removing a child removes the last self-only ref, we don't
        // want the scope to be destructed until after
        // removeAllChildren returns, so we guard ourselves with an
        // extra self-only ref.
        guardRef();

        // We must make sure not to be retaining any of our children through
        // these extra pointers or we will create a reference cycle.
        m_docType = 0;
        m_focusedNode = 0;
        m_hoverNode = 0;
        m_activeNode = 0;
        m_titleElement = 0;
        m_documentElement = 0;
#if ENABLE(FULLSCREEN_API)
        m_fullScreenElement = 0;
#endif

        // removeAllChildren() doesn't always unregister IDs,
        // so tear down scope information upfront to avoid having stale references in the map.
        destroyTreeScopeData();
        removeAllChildren();

        m_markers->detach();

        detachParser();

        m_cssCanvasElements.clear();

#if ENABLE(REQUEST_ANIMATION_FRAME)
        // FIXME: consider using ActiveDOMObject.
        m_scriptedAnimationController = 0;
#endif

#ifndef NDEBUG
        m_inRemovedLastRefFunction = false;
#endif

        guardDeref();
    } else {
#ifndef NDEBUG
        m_deletionHasBegun = true;
#endif
        delete this;
    }
}

Element* Document::getElementById(const AtomicString& id) const
{
    return TreeScope::getElementById(id);
}

Element* Document::getElementByAccessKey(const String& key)
{
    if (key.isEmpty())
        return 0;
    if (!m_accessKeyMapValid) {
        buildAccessKeyMap(this);
        m_accessKeyMapValid = true;
    }
    return m_elementsByAccessKey.get(key.impl());
}

void Document::buildAccessKeyMap(ContainerNode* root)
{
     for (Node* n = root; n; n = n->traverseNextNode(root)) {
        if (!n->isElementNode())
            continue;
        Element* element = static_cast<Element*>(n);
        const AtomicString& accessKey = element->getAttribute(accesskeyAttr);
        if (!accessKey.isEmpty())
            m_elementsByAccessKey.set(accessKey.impl(), element);
        buildAccessKeyMap(element->shadowRoot());
    }
}

void Document::invalidateAccessKeyMap()
{
    m_accessKeyMapValid = false;
    m_elementsByAccessKey.clear();
}

MediaQueryMatcher* Document::mediaQueryMatcher()
{
    if (!m_mediaQueryMatcher)
        m_mediaQueryMatcher = MediaQueryMatcher::create(this);
    return m_mediaQueryMatcher.get();
}

void Document::setCompatibilityMode(CompatibilityMode mode)
{
    if (m_compatibilityModeLocked || mode == m_compatibilityMode)
        return;
    ASSERT(!m_styleSheets->length());
    bool wasInQuirksMode = inQuirksMode();
    m_compatibilityMode = mode;
    if (inQuirksMode() != wasInQuirksMode) {
        // All user stylesheets have to reparse using the different mode.
        clearPageUserSheet();
        clearPageGroupUserSheets();
    }
}

String Document::compatMode() const
{
    return inQuirksMode() ? "BackCompat" : "CSS1Compat";
}

void Document::resetLinkColor()
{
    m_linkColor = Color(0, 0, 238);
}

void Document::resetVisitedLinkColor()
{
    m_visitedLinkColor = Color(85, 26, 139);    
}

void Document::resetActiveLinkColor()
{
    m_activeLinkColor.setNamedColor("red");
}

void Document::setDocType(PassRefPtr<DocumentType> docType)
{
    // This should never be called more than once.
    ASSERT(!m_docType || !docType);
    m_docType = docType;
    if (m_docType)
        m_docType->setTreeScopeRecursively(this);
}

DOMImplementation* Document::implementation()
{
    if (!m_implementation)
        m_implementation = DOMImplementation::create(this);
    return m_implementation.get();
}

void Document::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    TreeScope::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);
    
    // Invalidate the document element we have cached in case it was replaced.
    m_documentElement = 0;
}

void Document::cacheDocumentElement() const
{
    ASSERT(!m_documentElement);
    m_documentElement = firstElementChild(this);
}

PassRefPtr<Element> Document::createElement(const AtomicString& name, ExceptionCode& ec)
{
    if (!isValidName(name)) {
        ec = INVALID_CHARACTER_ERR;
        return 0;
    }

    if (m_isXHTML)
        return HTMLElementFactory::createHTMLElement(QualifiedName(nullAtom, name, xhtmlNamespaceURI), this, 0, false);

    return createElement(QualifiedName(nullAtom, name, nullAtom), false);
}

PassRefPtr<DocumentFragment> Document::createDocumentFragment()
{
    return DocumentFragment::create(document());
}

PassRefPtr<Text> Document::createTextNode(const String& data)
{
    return Text::create(this, data);
}

PassRefPtr<Comment> Document::createComment(const String& data)
{
    return Comment::create(this, data);
}

PassRefPtr<CDATASection> Document::createCDATASection(const String& data, ExceptionCode& ec)
{
    if (isHTMLDocument()) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }
    return CDATASection::create(this, data);
}

PassRefPtr<ProcessingInstruction> Document::createProcessingInstruction(const String& target, const String& data, ExceptionCode& ec)
{
    if (!isValidName(target)) {
        ec = INVALID_CHARACTER_ERR;
        return 0;
    }
    if (isHTMLDocument()) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }
    return ProcessingInstruction::create(this, target, data);
}

PassRefPtr<EntityReference> Document::createEntityReference(const String& name, ExceptionCode& ec)
{
    if (!isValidName(name)) {
        ec = INVALID_CHARACTER_ERR;
        return 0;
    }
    if (isHTMLDocument()) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }
    return EntityReference::create(this, name);
}

PassRefPtr<EditingText> Document::createEditingTextNode(const String& text)
{
    return EditingText::create(this, text);
}

PassRefPtr<CSSStyleDeclaration> Document::createCSSStyleDeclaration()
{
    return CSSMutableStyleDeclaration::create();
}

PassRefPtr<Node> Document::importNode(Node* importedNode, bool deep, ExceptionCode& ec)
{
    ec = 0;
    
    if (!importedNode
#if ENABLE(SVG) && ENABLE(DASHBOARD_SUPPORT)
        || (importedNode->isSVGElement() && page() && page()->settings()->usesDashboardBackwardCompatibilityMode())
#endif
        ) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    switch (importedNode->nodeType()) {
    case TEXT_NODE:
        return createTextNode(importedNode->nodeValue());
    case CDATA_SECTION_NODE:
        return createCDATASection(importedNode->nodeValue(), ec);
    case ENTITY_REFERENCE_NODE:
        return createEntityReference(importedNode->nodeName(), ec);
    case PROCESSING_INSTRUCTION_NODE:
        return createProcessingInstruction(importedNode->nodeName(), importedNode->nodeValue(), ec);
    case COMMENT_NODE:
        return createComment(importedNode->nodeValue());
    case ELEMENT_NODE: {
        Element* oldElement = static_cast<Element*>(importedNode);
        RefPtr<Element> newElement = createElementNS(oldElement->namespaceURI(), oldElement->tagQName().toString(), ec);
                    
        if (ec)
            return 0;

        NamedNodeMap* attrs = oldElement->attributes(true);
        if (attrs) {
            unsigned length = attrs->length();
            for (unsigned i = 0; i < length; i++) {
                Attribute* attr = attrs->attributeItem(i);
                newElement->setAttribute(attr->name(), attr->value().impl(), ec);
                if (ec)
                    return 0;
            }
        }

        newElement->copyNonAttributeProperties(oldElement);

        if (deep) {
            for (Node* oldChild = oldElement->firstChild(); oldChild; oldChild = oldChild->nextSibling()) {
                RefPtr<Node> newChild = importNode(oldChild, true, ec);
                if (ec)
                    return 0;
                newElement->appendChild(newChild.release(), ec);
                if (ec)
                    return 0;
            }
        }

        return newElement.release();
    }
    case ATTRIBUTE_NODE:
        return Attr::create(0, this, static_cast<Attr*>(importedNode)->attr()->clone());
    case DOCUMENT_FRAGMENT_NODE: {
        DocumentFragment* oldFragment = static_cast<DocumentFragment*>(importedNode);
        RefPtr<DocumentFragment> newFragment = createDocumentFragment();
        if (deep) {
            for (Node* oldChild = oldFragment->firstChild(); oldChild; oldChild = oldChild->nextSibling()) {
                RefPtr<Node> newChild = importNode(oldChild, true, ec);
                if (ec)
                    return 0;
                newFragment->appendChild(newChild.release(), ec);
                if (ec)
                    return 0;
            }
        }
        
        return newFragment.release();
    }
    case ENTITY_NODE:
    case NOTATION_NODE:
        // FIXME: It should be possible to import these node types, however in DOM3 the DocumentType is readonly, so there isn't much sense in doing that.
        // Ability to add these imported nodes to a DocumentType will be considered for addition to a future release of the DOM.
    case DOCUMENT_NODE:
    case DOCUMENT_TYPE_NODE:
    case XPATH_NAMESPACE_NODE:
    case SHADOW_ROOT_NODE:
        // ShadowRoot nodes should not be explicitly importable.
        // Either they are imported along with their host node, or created implicitly.
        break;
    }
    ec = NOT_SUPPORTED_ERR;
    return 0;
}


PassRefPtr<Node> Document::adoptNode(PassRefPtr<Node> source, ExceptionCode& ec)
{
    if (!source) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    if (source->isReadOnlyNode()) {
        ec = NO_MODIFICATION_ALLOWED_ERR;
        return 0;
    }

    EventQueueScope scope;

    switch (source->nodeType()) {
    case ENTITY_NODE:
    case NOTATION_NODE:
    case DOCUMENT_NODE:
    case DOCUMENT_TYPE_NODE:
    case XPATH_NAMESPACE_NODE:
        ec = NOT_SUPPORTED_ERR;
        return 0;            
    case ATTRIBUTE_NODE: {                   
        Attr* attr = static_cast<Attr*>(source.get());
        if (attr->ownerElement())
            attr->ownerElement()->removeAttributeNode(attr, ec);
        attr->setSpecified(true);
        break;
    }       
    default:
        if (source->hasTagName(iframeTag)) {
            HTMLIFrameElement* iframe = static_cast<HTMLIFrameElement*>(source.get());
            if (frame() && frame()->tree()->isDescendantOf(iframe->contentFrame())) {
                ec = HIERARCHY_REQUEST_ERR;
                return 0;
            }
            iframe->setRemainsAliveOnRemovalFromTree(attached() && source->attached());
        }

        if (source->parentNode())
            source->parentNode()->removeChild(source.get(), ec);
    }

    source->setTreeScopeRecursively(this);

    return source;
}

bool Document::hasPrefixNamespaceMismatch(const QualifiedName& qName)
{
    // These checks are from DOM Core Level 2, createElementNS
    // http://www.w3.org/TR/DOM-Level-2-Core/core.html#ID-DocCrElNS
    if (!qName.prefix().isEmpty() && qName.namespaceURI().isNull()) // createElementNS(null, "html:div")
        return true;
    if (qName.prefix() == xmlAtom && qName.namespaceURI() != XMLNames::xmlNamespaceURI) // createElementNS("http://www.example.com", "xml:lang")
        return true;

    // Required by DOM Level 3 Core and unspecified by DOM Level 2 Core:
    // http://www.w3.org/TR/2004/REC-DOM-Level-3-Core-20040407/core.html#ID-DocCrElNS
    // createElementNS("http://www.w3.org/2000/xmlns/", "foo:bar"), createElementNS(null, "xmlns:bar")
    if ((qName.prefix() == xmlnsAtom && qName.namespaceURI() != XMLNSNames::xmlnsNamespaceURI) || (qName.prefix() != xmlnsAtom && qName.namespaceURI() == XMLNSNames::xmlnsNamespaceURI))
        return true;

    return false;
}

// FIXME: This should really be in a possible ElementFactory class
PassRefPtr<Element> Document::createElement(const QualifiedName& qName, bool createdByParser)
{
    RefPtr<Element> e;

    // FIXME: Use registered namespaces and look up in a hash to find the right factory.
    if (qName.namespaceURI() == xhtmlNamespaceURI)
        e = HTMLElementFactory::createHTMLElement(qName, this, 0, createdByParser);
#if ENABLE(SVG)
    else if (qName.namespaceURI() == SVGNames::svgNamespaceURI)
        e = SVGElementFactory::createSVGElement(qName, this, createdByParser);
#endif
#if ENABLE(MATHML)
    else if (qName.namespaceURI() == MathMLNames::mathmlNamespaceURI)
        e = MathMLElementFactory::createMathMLElement(qName, this, createdByParser);
#endif

    if (e)
        m_sawElementsInKnownNamespaces = true;
    else
        e = Element::create(qName, document());

    // <image> uses imgTag so we need a special rule.
    ASSERT((qName.matches(imageTag) && e->tagQName().matches(imgTag) && e->tagQName().prefix() == qName.prefix()) || qName == e->tagQName());

    return e.release();
}

PassRefPtr<Element> Document::createElementNS(const String& namespaceURI, const String& qualifiedName, ExceptionCode& ec)
{
    String prefix, localName;
    if (!parseQualifiedName(qualifiedName, prefix, localName, ec))
        return 0;

    QualifiedName qName(prefix, localName, namespaceURI);
    if (hasPrefixNamespaceMismatch(qName)) {
        ec = NAMESPACE_ERR;
        return 0;
    }

    return createElement(qName, false);
}

String Document::readyState() const
{
    DEFINE_STATIC_LOCAL(const String, loading, ("loading"));
    DEFINE_STATIC_LOCAL(const String, interactive, ("interactive"));
    DEFINE_STATIC_LOCAL(const String, complete, ("complete"));

    switch (m_readyState) {
    case Loading:
        return loading;
    case Interactive:
        return interactive;
    case Complete:
        return complete;
    }

    ASSERT_NOT_REACHED();
    return String();
}

void Document::setReadyState(ReadyState readyState)
{
    if (readyState == m_readyState)
        return;

    switch (readyState) {
    case Loading:
        if (!m_documentTiming.domLoading)
            m_documentTiming.domLoading = currentTime();
        break;
    case Interactive:
        if (!m_documentTiming.domInteractive)
            m_documentTiming.domInteractive = currentTime();
        break;
    case Complete:
        if (!m_documentTiming.domComplete)
            m_documentTiming.domComplete = currentTime();
        break;
    }

    m_readyState = readyState;
    dispatchEvent(Event::create(eventNames().readystatechangeEvent, false, false));
}

String Document::encoding() const
{
    if (TextResourceDecoder* d = decoder())
        return d->encoding().domName();
    return String();
}

String Document::defaultCharset() const
{
    if (Settings* settings = this->settings())
        return settings->defaultTextEncodingName();
    return String();
}

void Document::setCharset(const String& charset)
{
    if (!decoder())
        return;
    decoder()->setEncoding(charset, TextResourceDecoder::UserChosenEncoding);
}

void Document::setXMLVersion(const String& version, ExceptionCode& ec)
{
    if (!implementation()->hasFeature("XML", String())) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }

    if (!XMLDocumentParser::supportsXMLVersion(version)) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }

    m_xmlVersion = version;
}

void Document::setXMLStandalone(bool standalone, ExceptionCode& ec)
{
    if (!implementation()->hasFeature("XML", String())) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }

    m_xmlStandalone = standalone;
}

void Document::setDocumentURI(const String& uri)
{
    m_documentURI = uri;
    updateBaseURL();
}

KURL Document::baseURI() const
{
    return m_baseURL;
}

void Document::setContent(const String& content)
{
    open();
    m_parser->append(content);
    close();
}

String Document::suggestedMIMEType() const
{
    if (m_document->isXHTMLDocument())
        return "application/xhtml+xml";
    if (m_document->isSVGDocument())
        return "image/svg+xml";
    if (m_document->xmlStandalone())
        return "text/xml";
    if (m_document->isHTMLDocument())
        return "text/html";

    return m_documentLoader->responseMIMEType();
}

// FIXME: We need to discuss the DOM API here at some point. Ideas:
// * making it receive a rect as parameter, i.e. nodesFromRect(x, y, w, h);
// * making it receive the expading size of each direction separately,
//   i.e. nodesFromRect(x, y, topSize, rightSize, bottomSize, leftSize);
PassRefPtr<NodeList> Document::nodesFromRect(int centerX, int centerY, unsigned topPadding, unsigned rightPadding, unsigned bottomPadding, unsigned leftPadding, bool ignoreClipping) const
{
    // FIXME: Share code between this, elementFromPoint and caretRangeFromPoint.
    if (!renderer())
        return 0;
    Frame* frame = this->frame();
    if (!frame)
        return 0;
    FrameView* frameView = frame->view();
    if (!frameView)
        return 0;

    float zoomFactor = frame->pageZoomFactor();
    IntPoint point = roundedIntPoint(FloatPoint(centerX * zoomFactor + view()->scrollX(), centerY * zoomFactor + view()->scrollY()));

    int type = HitTestRequest::ReadOnly | HitTestRequest::Active;

    // When ignoreClipping is false, this method returns null for coordinates outside of the viewport.
    if (ignoreClipping)
        type |= HitTestRequest::IgnoreClipping;
    else if (!frameView->visibleContentRect().intersects(HitTestResult::rectForPoint(point, topPadding, rightPadding, bottomPadding, leftPadding)))
        return 0;

    HitTestRequest request(type);

    // Passing a zero padding will trigger a rect hit test, however for the purposes of nodesFromRect,
    // we special handle this case in order to return a valid NodeList.
    if (!topPadding && !rightPadding && !bottomPadding && !leftPadding) {
        HitTestResult result(point);
        return handleZeroPadding(request, result);
    }

    HitTestResult result(point, topPadding, rightPadding, bottomPadding, leftPadding);
    renderView()->layer()->hitTest(request, result);

    return StaticHashSetNodeList::adopt(result.rectBasedTestResult());
}

PassRefPtr<NodeList> Document::handleZeroPadding(const HitTestRequest& request, HitTestResult& result) const
{
    renderView()->layer()->hitTest(request, result);

    Node* node = result.innerNode();
    if (!node)
        return 0;

    node = node->shadowAncestorNode();
    ListHashSet<RefPtr<Node> > list;
    list.add(node);
    return StaticHashSetNodeList::adopt(list);
}

static Node* nodeFromPoint(Frame* frame, RenderView* renderView, int x, int y, IntPoint* localPoint = 0)
{
    if (!frame)
        return 0;
    FrameView* frameView = frame->view();
    if (!frameView)
        return 0;

    float zoomFactor = frame->pageZoomFactor();
    IntPoint point = roundedIntPoint(FloatPoint(x * zoomFactor  + frameView->scrollX(), y * zoomFactor + frameView->scrollY()));

    if (!frameView->visibleContentRect().contains(point))
        return 0;

    HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::Active);
    HitTestResult result(point);
    renderView->layer()->hitTest(request, result);

    if (localPoint)
        *localPoint = result.localPoint();

    return result.innerNode();
}

Element* Document::elementFromPoint(int x, int y) const
{
    if (!renderer())
        return 0;
    Node* node = nodeFromPoint(frame(), renderView(), x, y);
    while (node && !node->isElementNode())
        node = node->parentNode();
    if (node)
        node = node->shadowAncestorNode();
    return static_cast<Element*>(node);
}

PassRefPtr<Range> Document::caretRangeFromPoint(int x, int y)
{
    if (!renderer())
        return 0;
    IntPoint localPoint;
    Node* node = nodeFromPoint(frame(), renderView(), x, y, &localPoint);
    if (!node)
        return 0;

    Node* shadowAncestorNode = node->shadowAncestorNode();
    if (shadowAncestorNode != node) {
        unsigned offset = shadowAncestorNode->nodeIndex();
        ContainerNode* container = shadowAncestorNode->parentNode();
        return Range::create(this, container, offset, container, offset);
    }

    RenderObject* renderer = node->renderer();
    if (!renderer)
        return 0;
    VisiblePosition visiblePosition = renderer->positionForPoint(localPoint);
    if (visiblePosition.isNull())
        return 0;

    Position rangeCompliantPosition = visiblePosition.deepEquivalent().parentAnchoredEquivalent();
    return Range::create(this, rangeCompliantPosition, rangeCompliantPosition);
}

/*
 * Performs three operations:
 *  1. Convert control characters to spaces
 *  2. Trim leading and trailing spaces
 *  3. Collapse internal whitespace.
 */
static inline StringWithDirection canonicalizedTitle(Document* document, const StringWithDirection& titleWithDirection)
{
    const String& title = titleWithDirection.string();
    const UChar* characters = title.characters();
    unsigned length = title.length();
    unsigned i;

    StringBuffer buffer(length);
    unsigned builderIndex = 0;

    // Skip leading spaces and leading characters that would convert to spaces
    for (i = 0; i < length; ++i) {
        UChar c = characters[i];
        if (!(c <= 0x20 || c == 0x7F))
            break;
    }

    if (i == length)
        return StringWithDirection();

    // Replace control characters with spaces, and backslashes with currency symbols, and collapse whitespace.
    bool previousCharWasWS = false;
    for (; i < length; ++i) {
        UChar c = characters[i];
        if (c <= 0x20 || c == 0x7F || (WTF::Unicode::category(c) & (WTF::Unicode::Separator_Line | WTF::Unicode::Separator_Paragraph))) {
            if (previousCharWasWS)
                continue;
            buffer[builderIndex++] = ' ';
            previousCharWasWS = true;
        } else {
            buffer[builderIndex++] = c;
            previousCharWasWS = false;
        }
    }

    // Strip trailing spaces
    while (builderIndex > 0) {
        --builderIndex;
        if (buffer[builderIndex] != ' ')
            break;
    }

    if (!builderIndex && buffer[builderIndex] == ' ')
        return StringWithDirection();

    buffer.shrink(builderIndex + 1);

    // Replace the backslashes with currency symbols if the encoding requires it.
    document->displayBufferModifiedByEncoding(buffer.characters(), buffer.length());
    
    return StringWithDirection(String::adopt(buffer), titleWithDirection.direction());
}

void Document::updateTitle(const StringWithDirection& title)
{
    if (m_rawTitle == title)
        return;

    m_rawTitle = title;
    m_title = canonicalizedTitle(this, m_rawTitle);
    if (Frame* f = frame())
        f->loader()->setTitle(m_title);
}

void Document::setTitle(const String& title)
{
    // Title set by JavaScript -- overrides any title elements.
    m_titleSetExplicitly = true;
    if (!isHTMLDocument())
        m_titleElement = 0;
    else if (!m_titleElement) {
        if (HTMLElement* headElement = head()) {
            m_titleElement = createElement(titleTag, false);
            ExceptionCode ec = 0;
            headElement->appendChild(m_titleElement, ec);
            ASSERT(!ec);
        }
    }

    // The DOM API has no method of specifying direction, so assume LTR.
    updateTitle(StringWithDirection(title, LTR));

    if (m_titleElement) {
        ASSERT(m_titleElement->hasTagName(titleTag));
        if (m_titleElement->hasTagName(titleTag))
            static_cast<HTMLTitleElement*>(m_titleElement.get())->setText(title);
    }
}

void Document::setTitleElement(const StringWithDirection& title, Element* titleElement)
{
    if (titleElement != m_titleElement) {
        if (m_titleElement || m_titleSetExplicitly)
            // Only allow the first title element to change the title -- others have no effect.
            return;
        m_titleElement = titleElement;
    }

    updateTitle(title);
}

void Document::removeTitle(Element* titleElement)
{
    if (m_titleElement != titleElement)
        return;

    m_titleElement = 0;
    m_titleSetExplicitly = false;

    // Update title based on first title element in the head, if one exists.
    if (HTMLElement* headElement = head()) {
        for (Node* e = headElement->firstChild(); e; e = e->nextSibling())
            if (e->hasTagName(titleTag)) {
                HTMLTitleElement* titleElement = static_cast<HTMLTitleElement*>(e);
                setTitleElement(titleElement->textWithDirection(), titleElement);
                break;
            }
    }

    if (!m_titleElement)
        updateTitle(StringWithDirection());
}

String Document::nodeName() const
{
    return "#document";
}

Node::NodeType Document::nodeType() const
{
    return DOCUMENT_NODE;
}

FrameView* Document::view() const
{
    return m_frame ? m_frame->view() : 0;
}

Page* Document::page() const
{
    return m_frame ? m_frame->page() : 0;    
}

Settings* Document::settings() const
{
    return m_frame ? m_frame->settings() : 0;
}

PassRefPtr<Range> Document::createRange()
{
    return Range::create(this);
}

PassRefPtr<NodeIterator> Document::createNodeIterator(Node* root, unsigned whatToShow, 
    PassRefPtr<NodeFilter> filter, bool expandEntityReferences, ExceptionCode& ec)
{
    if (!root) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }
    return NodeIterator::create(root, whatToShow, filter, expandEntityReferences);
}

PassRefPtr<TreeWalker> Document::createTreeWalker(Node* root, unsigned whatToShow, 
    PassRefPtr<NodeFilter> filter, bool expandEntityReferences, ExceptionCode& ec)
{
    if (!root) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }
    return TreeWalker::create(root, whatToShow, filter, expandEntityReferences);
}

void Document::scheduleForcedStyleRecalc()
{
    m_pendingStyleRecalcShouldForce = true;
    scheduleStyleRecalc();
}

void Document::scheduleStyleRecalc()
{
    if (m_styleRecalcTimer.isActive() || inPageCache())
        return;

    ASSERT(childNeedsStyleRecalc() || m_pendingStyleRecalcShouldForce);

    if (!documentsThatNeedStyleRecalc)
        documentsThatNeedStyleRecalc = new HashSet<Document*>;
    documentsThatNeedStyleRecalc->add(this);
    
    // FIXME: Why on earth is this here? This is clearly misplaced.
    invalidateAccessKeyMap();
    
    m_styleRecalcTimer.startOneShot(0);
}

void Document::unscheduleStyleRecalc()
{
    ASSERT(!childNeedsStyleRecalc());

    if (documentsThatNeedStyleRecalc)
        documentsThatNeedStyleRecalc->remove(this);

    m_styleRecalcTimer.stop();
    m_pendingStyleRecalcShouldForce = false;
}

bool Document::isPendingStyleRecalc() const
{
    return m_styleRecalcTimer.isActive() && !m_inStyleRecalc;
}

void Document::styleRecalcTimerFired(Timer<Document>*)
{
    updateStyleIfNeeded();
}

bool Document::childNeedsAndNotInStyleRecalc()
{
    return childNeedsStyleRecalc() && !m_inStyleRecalc;
}

void Document::recalcStyle(StyleChange change)
{
    // we should not enter style recalc while painting
    if (view() && view()->isPainting()) {
        ASSERT(!view()->isPainting());
        return;
    }
    
    if (m_inStyleRecalc)
        return; // Guard against re-entrancy. -dwh
    
    if (m_hasDirtyStyleSelector)
        recalcStyleSelector();

    InspectorInstrumentationCookie cookie = InspectorInstrumentation::willRecalculateStyle(this);

    m_inStyleRecalc = true;
    suspendPostAttachCallbacks();
    RenderWidget::suspendWidgetHierarchyUpdates();
    
    RefPtr<FrameView> frameView = view();
    if (frameView) {
        frameView->pauseScheduledEvents();
        frameView->beginDeferredRepaints();
    }

    ASSERT(!renderer() || renderArena());
    if (!renderer() || !renderArena())
        goto bail_out;

    if (m_pendingStyleRecalcShouldForce)
        change = Force;

    if (change == Force) {
        // style selector may set this again during recalc
        m_hasNodesWithPlaceholderStyle = false;
        
        RefPtr<RenderStyle> documentStyle = CSSStyleSelector::styleForDocument(this);
        StyleChange ch = diff(documentStyle.get(), renderer()->style());
        if (renderer() && ch != NoChange)
            renderer()->setStyle(documentStyle.release());
    }

    for (Node* n = firstChild(); n; n = n->nextSibling())
        if (change >= Inherit || n->childNeedsStyleRecalc() || n->needsStyleRecalc())
            n->recalcStyle(change);

#if USE(ACCELERATED_COMPOSITING)
    if (view()) {
        bool layoutPending = view()->layoutPending() || renderer()->needsLayout();
        // If we didn't update compositing layers because of layout(), we need to do so here.
        if (!layoutPending)
            view()->updateCompositingLayers();
    }
#endif

bail_out:
    clearNeedsStyleRecalc();
    clearChildNeedsStyleRecalc();
    unscheduleStyleRecalc();

    m_inStyleRecalc = false;
    
    // Pseudo element removal and similar may only work with these flags still set. Reset them after the style recalc.
    if (m_styleSelector) {
        m_usesSiblingRules = m_styleSelector->usesSiblingRules();
        m_usesFirstLineRules = m_styleSelector->usesFirstLineRules();
        m_usesBeforeAfterRules = m_styleSelector->usesBeforeAfterRules();
        m_usesLinkRules = m_styleSelector->usesLinkRules();
    }

    if (frameView) {
        frameView->resumeScheduledEvents();
        frameView->endDeferredRepaints();
    }
    RenderWidget::resumeWidgetHierarchyUpdates();
    resumePostAttachCallbacks();

    // If we wanted to call implicitClose() during recalcStyle, do so now that we're finished.
    if (m_closeAfterStyleRecalc) {
        m_closeAfterStyleRecalc = false;
        implicitClose();
    }

    InspectorInstrumentation::didRecalculateStyle(cookie);
}

void Document::updateStyleIfNeeded()
{
    ASSERT(isMainThread());
    ASSERT(!view() || (!view()->isInLayout() && !view()->isPainting()));
    
    if ((!m_pendingStyleRecalcShouldForce && !childNeedsStyleRecalc()) || inPageCache())
        return;

    if (m_frame)
        m_frame->animation()->beginAnimationUpdate();
        
    recalcStyle(NoChange);

    // Tell the animation controller that updateStyleIfNeeded is finished and it can do any post-processing
    if (m_frame)
        m_frame->animation()->endAnimationUpdate();
}

void Document::updateStyleForAllDocuments()
{
    ASSERT(isMainThread());
    if (!documentsThatNeedStyleRecalc)
        return;

    while (documentsThatNeedStyleRecalc->size()) {
        HashSet<Document*>::iterator it = documentsThatNeedStyleRecalc->begin();
        Document* doc = *it;
        documentsThatNeedStyleRecalc->remove(doc);
        doc->updateStyleIfNeeded();
    }
}

void Document::updateLayout()
{
    ASSERT(isMainThread());
    if (Element* oe = ownerElement())
        oe->document()->updateLayout();

    updateStyleIfNeeded();

    // Only do a layout if changes have occurred that make it necessary.      
    FrameView* v = view();
    if (v && renderer() && (v->layoutPending() || renderer()->needsLayout()))
        v->layout();
}

// FIXME: This is a bad idea and needs to be removed eventually.
// Other browsers load stylesheets before they continue parsing the web page.
// Since we don't, we can run JavaScript code that needs answers before the
// stylesheets are loaded. Doing a layout ignoring the pending stylesheets
// lets us get reasonable answers. The long term solution to this problem is
// to instead suspend JavaScript execution.
void Document::updateLayoutIgnorePendingStylesheets()
{
    bool oldIgnore = m_ignorePendingStylesheets;
    
    if (!haveStylesheetsLoaded()) {
        m_ignorePendingStylesheets = true;
        // FIXME: We are willing to attempt to suppress painting with outdated style info only once.  Our assumption is that it would be
        // dangerous to try to stop it a second time, after page content has already been loaded and displayed
        // with accurate style information.  (Our suppression involves blanking the whole page at the
        // moment.  If it were more refined, we might be able to do something better.)
        // It's worth noting though that this entire method is a hack, since what we really want to do is
        // suspend JS instead of doing a layout with inaccurate information.
        if (body() && !body()->renderer() && m_pendingSheetLayout == NoLayoutWithPendingSheets) {
            m_pendingSheetLayout = DidLayoutWithPendingSheets;
            styleSelectorChanged(RecalcStyleImmediately);
        } else if (m_hasNodesWithPlaceholderStyle)
            // If new nodes have been added or style recalc has been done with style sheets still pending, some nodes 
            // may not have had their real style calculated yet. Normally this gets cleaned when style sheets arrive 
            // but here we need up-to-date style immediately.
            recalcStyle(Force);
    }

    updateLayout();

    m_ignorePendingStylesheets = oldIgnore;
}

PassRefPtr<RenderStyle> Document::styleForElementIgnoringPendingStylesheets(Element* element)
{
    ASSERT_ARG(element, element->document() == this);

    bool oldIgnore = m_ignorePendingStylesheets;
    m_ignorePendingStylesheets = true;
    RefPtr<RenderStyle> style = styleSelector()->styleForElement(element, element->parentNode() ? element->parentNode()->computedStyle() : 0);
    m_ignorePendingStylesheets = oldIgnore;
    return style.release();
}

PassRefPtr<RenderStyle> Document::styleForPage(int pageIndex)
{
    RefPtr<RenderStyle> style = styleSelector()->styleForPage(pageIndex);
    return style.release();
}

bool Document::isPageBoxVisible(int pageIndex)
{
    RefPtr<RenderStyle> style = styleForPage(pageIndex);
    return style->visibility() != HIDDEN; // display property doesn't apply to @page.
}

void Document::pageSizeAndMarginsInPixels(int pageIndex, IntSize& pageSize, int& marginTop, int& marginRight, int& marginBottom, int& marginLeft)
{
    RefPtr<RenderStyle> style = styleForPage(pageIndex);

    int width = pageSize.width();
    int height = pageSize.height();
    switch (style->pageSizeType()) {
    case PAGE_SIZE_AUTO:
        break;
    case PAGE_SIZE_AUTO_LANDSCAPE:
        if (width < height)
            std::swap(width, height);
        break;
    case PAGE_SIZE_AUTO_PORTRAIT:
        if (width > height)
            std::swap(width, height);
        break;
    case PAGE_SIZE_RESOLVED: {
        LengthSize size = style->pageSize();
        ASSERT(size.width().isFixed());
        ASSERT(size.height().isFixed());
        width = size.width().calcValue(0);
        height = size.height().calcValue(0);
        break;
    }
    default:
        ASSERT_NOT_REACHED();
    }
    pageSize = IntSize(width, height);

    // The percentage is calculated with respect to the width even for margin top and bottom.
    // http://www.w3.org/TR/CSS2/box.html#margin-properties
    marginTop = style->marginTop().isAuto() ? marginTop : style->marginTop().calcValue(width);
    marginRight = style->marginRight().isAuto() ? marginRight : style->marginRight().calcValue(width);
    marginBottom = style->marginBottom().isAuto() ? marginBottom : style->marginBottom().calcValue(width);
    marginLeft = style->marginLeft().isAuto() ? marginLeft : style->marginLeft().calcValue(width);
}

PassRefPtr<CSSPrimitiveValueCache> Document::cssPrimitiveValueCache() const
{
    if (!m_cssPrimitiveValueCache)
        m_cssPrimitiveValueCache = CSSPrimitiveValueCache::create();
    return m_cssPrimitiveValueCache;
}

void Document::createStyleSelector()
{
    bool matchAuthorAndUserStyles = true;
    if (Settings* docSettings = settings())
        matchAuthorAndUserStyles = docSettings->authorAndUserStylesEnabled();
    m_styleSelector = adoptPtr(new CSSStyleSelector(this, m_styleSheets.get(), m_mappedElementSheet.get(), pageUserSheet(), pageGroupUserSheets(), 
                                                    !inQuirksMode(), matchAuthorAndUserStyles));
    // Delay resetting the flags until after next style recalc since unapplying the style may not work without these set (this is true at least with before/after).
    m_usesSiblingRules = m_usesSiblingRules || m_styleSelector->usesSiblingRules();
    m_usesFirstLineRules = m_usesFirstLineRules || m_styleSelector->usesFirstLineRules();
    m_usesBeforeAfterRules = m_usesBeforeAfterRules || m_styleSelector->usesBeforeAfterRules();
    m_usesLinkRules = m_usesLinkRules || m_styleSelector->usesLinkRules();
}

void Document::attach()
{
    ASSERT(!attached());
    ASSERT(!m_inPageCache);
    ASSERT(!m_axObjectCache);

    if (!m_renderArena)
        m_renderArena = adoptPtr(new RenderArena);
    
    // Create the rendering tree
    setRenderer(new (m_renderArena.get()) RenderView(this, view()));
#if USE(ACCELERATED_COMPOSITING)
    renderView()->didMoveOnscreen();
#endif

    recalcStyle(Force);

    RenderObject* render = renderer();
    setRenderer(0);

    TreeScope::attach();

    setRenderer(render);
}

void Document::detach()
{
    ASSERT(attached());
    ASSERT(!m_inPageCache);

    clearAXObjectCache();
    stopActiveDOMObjects();
    m_eventQueue->cancelQueuedEvents();

#if ENABLE(REQUEST_ANIMATION_FRAME)
    // FIXME: consider using ActiveDOMObject.
    m_scriptedAnimationController = 0;
#endif

    RenderObject* render = renderer();

    // Send out documentWillBecomeInactive() notifications to registered elements,
    // in order to stop media elements
    documentWillBecomeInactive();

#if ENABLE(SHARED_WORKERS)
    SharedWorkerRepository::documentDetached(this);
#endif

    if (m_frame) {
        FrameView* view = m_frame->view();
        if (view)
            view->detachCustomScrollbars();

#if ENABLE(TOUCH_EVENTS)
        Page* ownerPage = page();
        if (ownerPage && (m_frame == ownerPage->mainFrame())) {
            // Inform the Chrome Client that it no longer needs to
            // foward touch events to WebCore as the document is being
            // destroyed. It may start again if a subsequent page
            // registers a touch event listener.
            ownerPage->chrome()->client()->needTouchEvents(false);
        }
#endif
    }

    // indicate destruction mode,  i.e. attached() but renderer == 0
    setRenderer(0);
    
#if ENABLE(FULLSCREEN_API)
    if (m_fullScreenRenderer)
        setFullScreenRenderer(0);
#endif

    m_hoverNode = 0;
    m_focusedNode = 0;
    m_activeNode = 0;

    TreeScope::detach();

    unscheduleStyleRecalc();

    if (render)
        render->destroy();
    
    // This is required, as our Frame might delete itself as soon as it detaches
    // us. However, this violates Node::detach() semantics, as it's never
    // possible to re-attach. Eventually Document::detach() should be renamed,
    // or this setting of the frame to 0 could be made explicit in each of the
    // callers of Document::detach().
    m_frame = 0;
    m_renderArena.clear();
}

void Document::removeAllEventListeners()
{
    EventTarget::removeAllEventListeners();

    if (DOMWindow* domWindow = this->domWindow())
        domWindow->removeAllEventListeners();
    for (Node* node = firstChild(); node; node = node->traverseNextNode())
        node->removeAllEventListeners();
}

RenderView* Document::renderView() const
{
    return toRenderView(renderer());
}

void Document::clearAXObjectCache()
{
    // clear cache in top document
    if (m_axObjectCache) {
        // Clear the cache member variable before calling delete because attempts
        // are made to access it during destruction.
        AXObjectCache* axObjectCache = m_axObjectCache;
        m_axObjectCache = 0;
        delete axObjectCache;
        return;
    }
    
    // ask the top-level document to clear its cache
    Document* doc = topDocument();
    if (doc != this)
        doc->clearAXObjectCache();
}

bool Document::axObjectCacheExists() const
{
    if (m_axObjectCache)
        return true;
    
    Document* doc = topDocument();
    if (doc != this)
        return doc->axObjectCacheExists();
    
    return false;
}
    
AXObjectCache* Document::axObjectCache() const
{
    // The only document that actually has a AXObjectCache is the top-level
    // document.  This is because we need to be able to get from any WebCoreAXObject
    // to any other WebCoreAXObject on the same page.  Using a single cache allows
    // lookups across nested webareas (i.e. multiple documents).
    
    if (m_axObjectCache) {
        // return already known top-level cache
        if (!ownerElement())
            return m_axObjectCache;
        
        // In some pages with frames, the cache is created before the sub-webarea is
        // inserted into the tree.  Here, we catch that case and just toss the old
        // cache and start over.
        // NOTE: This recovery may no longer be needed. I have been unable to trigger
        // it again. See rdar://5794454
        // FIXME: Can this be fixed when inserting the subframe instead of now?
        // FIXME: If this function was called to get the cache in order to remove
        // an AXObject, we are now deleting the cache as a whole and returning a
        // new empty cache that does not contain the AXObject! That should actually
        // be OK. I am concerned about other cases like this where accessing the
        // cache blows away the AXObject being operated on.
        delete m_axObjectCache;
        m_axObjectCache = 0;
    }

    // ask the top-level document for its cache
    Document* doc = topDocument();
    if (doc != this)
        return doc->axObjectCache();
    
    // this is the top-level document, so install a new cache
    m_axObjectCache = new AXObjectCache(this);
    return m_axObjectCache;
}

void Document::setVisuallyOrdered()
{
    m_visuallyOrdered = true;
    if (renderer())
        renderer()->style()->setVisuallyOrdered(true);
}

PassRefPtr<DocumentParser> Document::createParser()
{
    // FIXME: this should probably pass the frame instead
    return XMLDocumentParser::create(this, view());
}

ScriptableDocumentParser* Document::scriptableDocumentParser() const
{
    return parser() ? parser()->asScriptableDocumentParser() : 0;
}

void Document::open(Document* ownerDocument)
{
    if (ownerDocument) {
        setURL(ownerDocument->url());
        m_cookieURL = ownerDocument->cookieURL();
        ScriptExecutionContext::setSecurityOrigin(ownerDocument->securityOrigin());
    }

    if (m_frame) {
        if (ScriptableDocumentParser* parser = scriptableDocumentParser()) {
            if (parser->isParsing()) {
                // FIXME: HTML5 doesn't tell us to check this, it might not be correct.
                if (parser->isExecutingScript())
                    return;

                if (!parser->wasCreatedByScript() && parser->hasInsertionPoint())
                    return;
            }
        }

        if (m_frame->loader()->state() == FrameStateProvisional)
            m_frame->loader()->stopAllLoaders();
    }

    removeAllEventListeners();
    implicitOpen();
    if (ScriptableDocumentParser* parser = scriptableDocumentParser())
        parser->setWasCreatedByScript(true);

    if (DOMWindow* domWindow = this->domWindow())
        domWindow->removeAllEventListeners();

    if (m_frame)
        m_frame->loader()->didExplicitOpen();
}

void Document::detachParser()
{
    if (!m_parser)
        return;
    m_parser->detach();
    m_parser.clear();
}

void Document::cancelParsing()
{
    if (!m_parser)
        return;

    // We have to clear the parser to avoid possibly triggering
    // the onload handler when closing as a side effect of a cancel-style
    // change, such as opening a new document or closing the window while
    // still parsing
    detachParser();
    explicitClose();
}

void Document::implicitOpen()
{
    cancelParsing();

    removeChildren();

    setCompatibilityMode(NoQuirksMode);

    m_parser = createParser();
    setParsing(true);
    setReadyState(Loading);

    // If we reload, the animation controller sticks around and has
    // a stale animation time. We need to update it here.
    if (m_frame && m_frame->animation())
        m_frame->animation()->beginAnimationUpdate();
}

HTMLElement* Document::body() const
{
    Node* de = documentElement();
    if (!de)
        return 0;
    
    // try to prefer a FRAMESET element over BODY
    Node* body = 0;
    for (Node* i = de->firstChild(); i; i = i->nextSibling()) {
        if (i->hasTagName(framesetTag))
            return toHTMLElement(i);
        
        if (i->hasTagName(bodyTag) && !body)
            body = i;
    }
    return toHTMLElement(body);
}

void Document::setBody(PassRefPtr<HTMLElement> newBody, ExceptionCode& ec)
{
    if (!newBody || !documentElement() || !newBody->hasTagName(bodyTag)) { 
        ec = HIERARCHY_REQUEST_ERR;
        return;
    }

    if (newBody->document() && newBody->document() != this) {
        ec = 0;
        RefPtr<Node> node = importNode(newBody.get(), true, ec);
        if (ec)
            return;
        
        newBody = toHTMLElement(node.get());
    }

    HTMLElement* b = body();
    if (!b)
        documentElement()->appendChild(newBody, ec);
    else
        documentElement()->replaceChild(newBody, b, ec);
}

HTMLHeadElement* Document::head()
{
    Node* de = documentElement();
    if (!de)
        return 0;

    for (Node* e = de->firstChild(); e; e = e->nextSibling())
        if (e->hasTagName(headTag))
            return static_cast<HTMLHeadElement*>(e);

    return 0;
}

void Document::close()
{
    // FIXME: We should follow the specification more closely:
    //        http://www.whatwg.org/specs/web-apps/current-work/#dom-document-close

    if (!scriptableDocumentParser() || !scriptableDocumentParser()->wasCreatedByScript() || !scriptableDocumentParser()->isParsing())
        return;

    explicitClose();
}

void Document::explicitClose()
{
    if (!m_frame) {
        // Because we have no frame, we don't know if all loading has completed,
        // so we just call implicitClose() immediately. FIXME: This might fire
        // the load event prematurely <http://bugs.webkit.org/show_bug.cgi?id=14568>.
        if (m_parser)
            m_parser->finish();
        implicitClose();
        return;
    }

    // This code calls implicitClose() if all loading has completed.
    loader()->writer()->endIfNotLoadingMainResource();
    if (m_frame)
        m_frame->loader()->checkCompleted();
}

void Document::implicitClose()
{
    // If we're in the middle of recalcStyle, we need to defer the close until the style information is accurate and all elements are re-attached.
    if (m_inStyleRecalc) {
        m_closeAfterStyleRecalc = true;
        return;
    }

    bool wasLocationChangePending = frame() && frame()->navigationScheduler()->locationChangePending();
    bool doload = !parsing() && m_parser && !m_processingLoadEvent && !wasLocationChangePending;
    
    if (!doload)
        return;

    m_processingLoadEvent = true;

    ScriptableDocumentParser* parser = scriptableDocumentParser();
    m_wellFormed = parser && parser->wellFormed();

    // We have to clear the parser, in case someone document.write()s from the
    // onLoad event handler, as in Radar 3206524.
    detachParser();

    // Parser should have picked up all preloads by now
    m_cachedResourceLoader->clearPreloads();

    // FIXME: We kick off the icon loader when the Document is done parsing.
    // There are earlier opportunities we could start it:
    //  -When the <head> finishes parsing
    //  -When any new HTMLLinkElement is inserted into the document
    // But those add a dynamic component to the favicon that has UI 
    // ramifications, and we need to decide what is the Right Thing To Do(tm)
    Frame* f = frame();
    if (f)
        f->loader()->startIconLoader();

    // Resume the animations (or start them)
    if (f)
        f->animation()->resumeAnimationsForDocument(this);

    ImageLoader::dispatchPendingBeforeLoadEvents();
    ImageLoader::dispatchPendingLoadEvents();
    dispatchWindowLoadEvent();
    enqueuePageshowEvent(PageshowEventNotPersisted);
    enqueuePopstateEvent(m_pendingStateObject ? m_pendingStateObject.release() : SerializedScriptValue::nullValue());
    
    if (f)
        f->loader()->handledOnloadEvents();
#ifdef INSTRUMENT_LAYOUT_SCHEDULING
    if (!ownerElement())
        printf("onload fired at %d\n", elapsedTime());
#endif

    m_processingLoadEvent = false;

    // An event handler may have removed the frame
    if (!frame())
        return;

    // Make sure both the initial layout and reflow happen after the onload
    // fires. This will improve onload scores, and other browsers do it.
    // If they wanna cheat, we can too. -dwh

    if (frame()->navigationScheduler()->locationChangePending() && elapsedTime() < cLayoutScheduleThreshold) {
        // Just bail out. Before or during the onload we were shifted to another page.
        // The old i-Bench suite does this. When this happens don't bother painting or laying out.        
        view()->unscheduleRelayout();
        return;
    }

    frame()->loader()->checkCallImplicitClose();
    RenderObject* renderObject = renderer();
    
    // We used to force a synchronous display and flush here.  This really isn't
    // necessary and can in fact be actively harmful if pages are loading at a rate of > 60fps
    // (if your platform is syncing flushes and limiting them to 60fps).
    m_overMinimumLayoutThreshold = true;
    if (!ownerElement() || (ownerElement()->renderer() && !ownerElement()->renderer()->needsLayout())) {
        updateStyleIfNeeded();
        
        // Always do a layout after loading if needed.
        if (view() && renderObject && (!renderObject->firstChild() || renderObject->needsLayout()))
            view()->layout();
    }

#if PLATFORM(MAC) || PLATFORM(CHROMIUM)
    if (f && renderObject && this == topDocument() && AXObjectCache::accessibilityEnabled()) {
        // The AX cache may have been cleared at this point, but we need to make sure it contains an
        // AX object to send the notification to. getOrCreate will make sure that an valid AX object
        // exists in the cache (we ignore the return value because we don't need it here). This is 
        // only safe to call when a layout is not in progress, so it can not be used in postNotification.    
        axObjectCache()->getOrCreate(renderObject);
        axObjectCache()->postNotification(renderObject, AXObjectCache::AXLoadComplete, true);
    }
#endif

#if ENABLE(SVG)
    // FIXME: Officially, time 0 is when the outermost <svg> receives its
    // SVGLoad event, but we don't implement those yet.  This is close enough
    // for now.  In some cases we should have fired earlier.
    if (svgExtensions())
        accessSVGExtensions()->startAnimations();
#endif
}

void Document::setParsing(bool b)
{
    m_bParsing = b;
    if (!m_bParsing && view())
        view()->scheduleRelayout();

#ifdef INSTRUMENT_LAYOUT_SCHEDULING
    if (!ownerElement() && !m_bParsing)
        printf("Parsing finished at %d\n", elapsedTime());
#endif
}

bool Document::shouldScheduleLayout()
{
    // This function will only be called when FrameView thinks a layout is needed.
    // This enforces a couple extra rules.
    //
    //    (a) Only schedule a layout once the stylesheets are loaded.
    //    (b) Only schedule layout once we have a body element.

    return (haveStylesheetsLoaded() && body())
        || (documentElement() && !documentElement()->hasTagName(htmlTag));
}
    
bool Document::isLayoutTimerActive()
{
    if (!view() || !view()->layoutPending())
        return false;
    bool isPendingLayoutImmediate = minimumLayoutDelay() == m_extraLayoutDelay;
    return isPendingLayoutImmediate;
}

int Document::minimumLayoutDelay()
{
    if (m_overMinimumLayoutThreshold)
        return m_extraLayoutDelay;
    
    int elapsed = elapsedTime();
    m_overMinimumLayoutThreshold = elapsed > cLayoutScheduleThreshold;
    
    // We'll want to schedule the timer to fire at the minimum layout threshold.
    return max(0, cLayoutScheduleThreshold - elapsed) + m_extraLayoutDelay;
}

int Document::elapsedTime() const
{
    return static_cast<int>((currentTime() - m_startTime) * 1000);
}

void Document::write(const SegmentedString& text, Document* ownerDocument)
{
    NestingLevelIncrementer nestingLevelIncrementer(m_writeRecursionDepth);

    m_writeRecursionIsTooDeep = (m_writeRecursionDepth > 1) && m_writeRecursionIsTooDeep;
    m_writeRecursionIsTooDeep = (m_writeRecursionDepth > cMaxWriteRecursionDepth) || m_writeRecursionIsTooDeep;

    if (m_writeRecursionIsTooDeep)
       return;

#ifdef INSTRUMENT_LAYOUT_SCHEDULING
    if (!ownerElement())
        printf("Beginning a document.write at %d\n", elapsedTime());
#endif

    bool hasInsertionPoint = m_parser && m_parser->hasInsertionPoint();
    if (!hasInsertionPoint && m_ignoreDestructiveWriteCount)
        return;

    if (!hasInsertionPoint)
        open(ownerDocument);

    ASSERT(m_parser);
    m_parser->insert(text);

#ifdef INSTRUMENT_LAYOUT_SCHEDULING
    if (!ownerElement())
        printf("Ending a document.write at %d\n", elapsedTime());
#endif    
}

void Document::write(const String& text, Document* ownerDocument)
{
    write(SegmentedString(text), ownerDocument);
}

void Document::writeln(const String& text, Document* ownerDocument)
{
    write(text, ownerDocument);
    write("\n", ownerDocument);
}

void Document::finishParsing()
{
#ifdef INSTRUMENT_LAYOUT_SCHEDULING
    if (!ownerElement())
        printf("Received all data at %d\n", elapsedTime());
#endif
    
    // Let the parser go through as much data as it can.  There will be three possible outcomes after
    // finish() is called:
    // (1) All remaining data is parsed, document isn't loaded yet
    // (2) All remaining data is parsed, document is loaded, parser gets deleted
    // (3) Data is still remaining to be parsed.
    if (m_parser)
        m_parser->finish();
}

const KURL& Document::virtualURL() const
{
    return m_url;
}

KURL Document::virtualCompleteURL(const String& url) const
{
    return completeURL(url);
}

double Document::minimumTimerInterval() const
{
    Page* p = page();
    if (!p)
        return ScriptExecutionContext::minimumTimerInterval();
    return p->settings()->minDOMTimerInterval();
}

EventTarget* Document::errorEventTarget()
{
    return domWindow();
}

void Document::logExceptionToConsole(const String& errorMessage, int lineNumber, const String& sourceURL, PassRefPtr<ScriptCallStack> callStack)
{
    MessageType messageType = callStack ? UncaughtExceptionMessageType : LogMessageType;
    addMessage(JSMessageSource, messageType, ErrorMessageLevel, errorMessage, lineNumber, sourceURL, callStack);
}

void Document::setURL(const KURL& url)
{
    const KURL& newURL = url.isEmpty() ? blankURL() : url;
    if (newURL == m_url)
        return;

    m_url = newURL;
    m_documentURI = m_url.string();
    updateBaseURL();
}

void Document::updateBaseURL()
{
    // DOM 3 Core: When the Document supports the feature "HTML" [DOM Level 2 HTML], the base URI is computed using
    // first the value of the href attribute of the HTML BASE element if any, and the value of the documentURI attribute
    // from the Document interface otherwise.
    if (m_baseElementURL.isEmpty()) {
        // The documentURI attribute is an arbitrary string. DOM 3 Core does not specify how it should be resolved,
        // so we use a null base URL.
        m_baseURL = KURL(KURL(), documentURI());
    } else
        m_baseURL = m_baseElementURL;
    if (!m_baseURL.isValid())
        m_baseURL = KURL();

    if (m_elemSheet)
        m_elemSheet->setFinalURL(m_baseURL);
    if (m_mappedElementSheet)
        m_mappedElementSheet->setFinalURL(m_baseURL);
}

void Document::processBaseElement()
{
    // Find the first href attribute in a base element and the first target attribute in a base element.
    const AtomicString* href = 0;
    const AtomicString* target = 0;
    for (Node* node = document()->firstChild(); node && (!href || !target); node = node->traverseNextNode()) {
        if (node->hasTagName(baseTag)) {
            if (!href) {
                const AtomicString& value = static_cast<Element*>(node)->fastGetAttribute(hrefAttr);
                if (!value.isNull())
                    href = &value;
            }
            if (!target) {
                const AtomicString& value = static_cast<Element*>(node)->fastGetAttribute(targetAttr);
                if (!value.isNull())
                    target = &value;
            }
        }
    }

    // FIXME: Since this doesn't share code with completeURL it may not handle encodings correctly.
    KURL baseElementURL;
    if (href) {
        String strippedHref = stripLeadingAndTrailingHTMLSpaces(*href);
        if (!strippedHref.isEmpty())
            baseElementURL = KURL(url(), strippedHref);
    }
    if (m_baseElementURL != baseElementURL) {
        m_baseElementURL = baseElementURL;
        updateBaseURL();
    }

    m_baseTarget = target ? *target : nullAtom;
}

String Document::userAgent(const KURL& url) const
{
    return frame() ? frame()->loader()->userAgent(url) : String();
}

CSSStyleSheet* Document::pageUserSheet()
{
    if (m_pageUserSheet)
        return m_pageUserSheet.get();
    
    Page* owningPage = page();
    if (!owningPage)
        return 0;
    
    String userSheetText = owningPage->userStyleSheet();
    if (userSheetText.isEmpty())
        return 0;
    
    // Parse the sheet and cache it.
    m_pageUserSheet = CSSStyleSheet::createInline(this, settings()->userStyleSheetLocation());
    m_pageUserSheet->setIsUserStyleSheet(true);
    m_pageUserSheet->parseString(userSheetText, !inQuirksMode());
    return m_pageUserSheet.get();
}

void Document::clearPageUserSheet()
{
    if (m_pageUserSheet) {
        m_pageUserSheet = 0;
        styleSelectorChanged(DeferRecalcStyle);
    }
}

void Document::updatePageUserSheet()
{
    clearPageUserSheet();
    if (pageUserSheet())
        styleSelectorChanged(RecalcStyleImmediately);
}

const Vector<RefPtr<CSSStyleSheet> >* Document::pageGroupUserSheets() const
{
    if (m_pageGroupUserSheetCacheValid)
        return m_pageGroupUserSheets.get();
    
    m_pageGroupUserSheetCacheValid = true;
    
    Page* owningPage = page();
    if (!owningPage)
        return 0;
        
    const PageGroup& pageGroup = owningPage->group();
    const UserStyleSheetMap* sheetsMap = pageGroup.userStyleSheets();
    if (!sheetsMap)
        return 0;

    UserStyleSheetMap::const_iterator end = sheetsMap->end();
    for (UserStyleSheetMap::const_iterator it = sheetsMap->begin(); it != end; ++it) {
        const UserStyleSheetVector* sheets = it->second;
        for (unsigned i = 0; i < sheets->size(); ++i) {
            const UserStyleSheet* sheet = sheets->at(i).get();
            if (sheet->injectedFrames() == InjectInTopFrameOnly && ownerElement())
                continue;
            if (!UserContentURLPattern::matchesPatterns(url(), sheet->whitelist(), sheet->blacklist()))
                continue;
            RefPtr<CSSStyleSheet> parsedSheet = CSSStyleSheet::createInline(const_cast<Document*>(this), sheet->url());
            parsedSheet->setIsUserStyleSheet(sheet->level() == UserStyleUserLevel);
            parsedSheet->parseString(sheet->source(), !inQuirksMode());
            if (!m_pageGroupUserSheets)
                m_pageGroupUserSheets = adoptPtr(new Vector<RefPtr<CSSStyleSheet> >);
            m_pageGroupUserSheets->append(parsedSheet.release());
        }
    }

    return m_pageGroupUserSheets.get();
}

void Document::clearPageGroupUserSheets()
{
    m_pageGroupUserSheetCacheValid = false;
    if (m_pageGroupUserSheets && m_pageGroupUserSheets->size()) {
        m_pageGroupUserSheets->clear();
        styleSelectorChanged(DeferRecalcStyle);
    }
}

void Document::updatePageGroupUserSheets()
{
    clearPageGroupUserSheets();
    if (pageGroupUserSheets() && pageGroupUserSheets()->size())
        styleSelectorChanged(RecalcStyleImmediately);
}

CSSStyleSheet* Document::elementSheet()
{
    if (!m_elemSheet)
        m_elemSheet = CSSStyleSheet::createInline(this, m_baseURL);
    return m_elemSheet.get();
}

CSSStyleSheet* Document::mappedElementSheet()
{
    if (!m_mappedElementSheet)
        m_mappedElementSheet = CSSStyleSheet::createInline(this, m_baseURL);
    return m_mappedElementSheet.get();
}

static Node* nextNodeWithExactTabIndex(Node* start, int tabIndex, KeyboardEvent* event)
{
    // Search is inclusive of start
    for (Node* n = start; n; n = n->traverseNextNode())
        if (n->isKeyboardFocusable(event) && n->tabIndex() == tabIndex)
            return n;
    
    return 0;
}

static Node* previousNodeWithExactTabIndex(Node* start, int tabIndex, KeyboardEvent* event)
{
    // Search is inclusive of start
    for (Node* n = start; n; n = n->traversePreviousNode())
        if (n->isKeyboardFocusable(event) && n->tabIndex() == tabIndex)
            return n;
    
    return 0;
}

static Node* nextNodeWithGreaterTabIndex(Node* start, int tabIndex, KeyboardEvent* event)
{
    // Search is inclusive of start
    int winningTabIndex = SHRT_MAX + 1;
    Node* winner = 0;
    for (Node* n = start; n; n = n->traverseNextNode())
        if (n->isKeyboardFocusable(event) && n->tabIndex() > tabIndex && n->tabIndex() < winningTabIndex) {
            winner = n;
            winningTabIndex = n->tabIndex();
        }
    
    return winner;
}

static Node* previousNodeWithLowerTabIndex(Node* start, int tabIndex, KeyboardEvent* event)
{
    // Search is inclusive of start
    int winningTabIndex = 0;
    Node* winner = 0;
    for (Node* n = start; n; n = n->traversePreviousNode())
        if (n->isKeyboardFocusable(event) && n->tabIndex() < tabIndex && n->tabIndex() > winningTabIndex) {
            winner = n;
            winningTabIndex = n->tabIndex();
        }
    
    return winner;
}

Node* Document::nextFocusableNode(Node* start, KeyboardEvent* event)
{
    if (start) {
        // If a node is excluded from the normal tabbing cycle, the next focusable node is determined by tree order
        if (start->tabIndex() < 0) {
            for (Node* n = start->traverseNextNode(); n; n = n->traverseNextNode())
                if (n->isKeyboardFocusable(event) && n->tabIndex() >= 0)
                    return n;
        }
    
        // First try to find a node with the same tabindex as start that comes after start in the document.
        if (Node* winner = nextNodeWithExactTabIndex(start->traverseNextNode(), start->tabIndex(), event))
            return winner;

        if (!start->tabIndex())
            // We've reached the last node in the document with a tabindex of 0. This is the end of the tabbing order.
            return 0;
    }

    // Look for the first node in the document that:
    // 1) has the lowest tabindex that is higher than start's tabindex (or 0, if start is null), and
    // 2) comes first in the document, if there's a tie.
    if (Node* winner = nextNodeWithGreaterTabIndex(this, start ? start->tabIndex() : 0, event))
        return winner;

    // There are no nodes with a tabindex greater than start's tabindex,
    // so find the first node with a tabindex of 0.
    return nextNodeWithExactTabIndex(this, 0, event);
}

Node* Document::previousFocusableNode(Node* start, KeyboardEvent* event)
{
    Node* last;
    for (last = this; last->lastChild(); last = last->lastChild()) { }

    // First try to find the last node in the document that comes before start and has the same tabindex as start.
    // If start is null, find the last node in the document with a tabindex of 0.
    Node* startingNode;
    int startingTabIndex;
    if (start) {
        startingNode = start->traversePreviousNode();
        startingTabIndex = start->tabIndex();
    } else {
        startingNode = last;
        startingTabIndex = 0;
    }
    
    // However, if a node is excluded from the normal tabbing cycle, the previous focusable node is determined by tree order
    if (startingTabIndex < 0) {
        for (Node* n = startingNode; n; n = n->traversePreviousNode())
            if (n->isKeyboardFocusable(event) && n->tabIndex() >= 0)
                return n;        
    }

    if (Node* winner = previousNodeWithExactTabIndex(startingNode, startingTabIndex, event))
        return winner;

    // There are no nodes before start with the same tabindex as start, so look for a node that:
    // 1) has the highest non-zero tabindex (that is less than start's tabindex), and
    // 2) comes last in the document, if there's a tie.
    startingTabIndex = (start && start->tabIndex()) ? start->tabIndex() : SHRT_MAX;
    return previousNodeWithLowerTabIndex(last, startingTabIndex, event);
}

int Document::nodeAbsIndex(Node *node)
{
    ASSERT(node->document() == this);

    int absIndex = 0;
    for (Node* n = node; n && n != this; n = n->traversePreviousNode())
        absIndex++;
    return absIndex;
}

Node* Document::nodeWithAbsIndex(int absIndex)
{
    Node* n = this;
    for (int i = 0; n && (i < absIndex); i++)
        n = n->traverseNextNode();
    return n;
}

void Document::processHttpEquiv(const String& equiv, const String& content)
{
    ASSERT(!equiv.isNull() && !content.isNull());

    Frame* frame = this->frame();

    if (equalIgnoringCase(equiv, "default-style")) {
        // The preferred style set has been overridden as per section 
        // 14.3.2 of the HTML4.0 specification.  We need to update the
        // sheet used variable and then update our style selector. 
        // For more info, see the test at:
        // http://www.hixie.ch/tests/evil/css/import/main/preferred.html
        // -dwh
        m_selectedStylesheetSet = content;
        m_preferredStylesheetSet = content;
        styleSelectorChanged(DeferRecalcStyle);
    } else if (equalIgnoringCase(equiv, "refresh")) {
        double delay;
        String url;
        if (frame && parseHTTPRefresh(content, true, delay, url)) {
            if (url.isEmpty())
                url = m_url.string();
            else
                url = completeURL(url).string();
            frame->navigationScheduler()->scheduleRedirect(delay, url);
        }
    } else if (equalIgnoringCase(equiv, "set-cookie")) {
        // FIXME: make setCookie work on XML documents too; e.g. in case of <html:meta .....>
        if (isHTMLDocument()) {
            ExceptionCode ec; // Exception (for sandboxed documents) ignored.
            static_cast<HTMLDocument*>(this)->setCookie(content, ec);
        }
    } else if (equalIgnoringCase(equiv, "content-language"))
        setContentLanguage(content);
    else if (equalIgnoringCase(equiv, "x-dns-prefetch-control"))
        parseDNSPrefetchControlHeader(content);
    else if (equalIgnoringCase(equiv, "x-frame-options")) {
        if (frame) {
            FrameLoader* frameLoader = frame->loader();
            if (frameLoader->shouldInterruptLoadForXFrameOptions(content, url())) {
                frameLoader->stopAllLoaders();
                frame->navigationScheduler()->scheduleLocationChange(securityOrigin(), blankURL(), String());

                DEFINE_STATIC_LOCAL(String, consoleMessage, ("Refused to display document because display forbidden by X-Frame-Options.\n"));
                frame->domWindow()->console()->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, consoleMessage, 1, String());
            }
        }
    } else if (equalIgnoringCase(equiv, "x-webkit-csp"))
        contentSecurityPolicy()->didReceiveHeader(content);
}

// Though isspace() considers \t and \v to be whitespace, Win IE doesn't.
static bool isSeparator(UChar c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '=' || c == ',' || c == '\0';
}

void Document::processArguments(const String& features, void* data, ArgumentsCallback callback)
{
    // Tread lightly in this code -- it was specifically designed to mimic Win IE's parsing behavior.
    int keyBegin, keyEnd;
    int valueBegin, valueEnd;

    int i = 0;
    int length = features.length();
    String buffer = features.lower();
    while (i < length) {
        // skip to first non-separator, but don't skip past the end of the string
        while (isSeparator(buffer[i])) {
            if (i >= length)
                break;
            i++;
        }
        keyBegin = i;

        // skip to first separator
        while (!isSeparator(buffer[i]))
            i++;
        keyEnd = i;

        // skip to first '=', but don't skip past a ',' or the end of the string
        while (buffer[i] != '=') {
            if (buffer[i] == ',' || i >= length)
                break;
            i++;
        }

        // skip to first non-separator, but don't skip past a ',' or the end of the string
        while (isSeparator(buffer[i])) {
            if (buffer[i] == ',' || i >= length)
                break;
            i++;
        }
        valueBegin = i;

        // skip to first separator
        while (!isSeparator(buffer[i]))
            i++;
        valueEnd = i;

        ASSERT(i <= length);

        String keyString = buffer.substring(keyBegin, keyEnd - keyBegin);
        String valueString = buffer.substring(valueBegin, valueEnd - valueBegin);
        callback(keyString, valueString, this, data);
    }
}

void Document::processViewport(const String& features)
{
    ASSERT(!features.isNull());

    m_viewportArguments = ViewportArguments();
    processArguments(features, (void*)&m_viewportArguments, &setViewportFeature);

    Frame* frame = this->frame();
    if (!frame || !frame->page())
        return;

    frame->page()->updateViewportArguments();
}

MouseEventWithHitTestResults Document::prepareMouseEvent(const HitTestRequest& request, const IntPoint& documentPoint, const PlatformMouseEvent& event)
{
    ASSERT(!renderer() || renderer()->isRenderView());

    if (!renderer())
        return MouseEventWithHitTestResults(event, HitTestResult(IntPoint()));

    HitTestResult result(documentPoint);
    renderView()->layer()->hitTest(request, result);

    if (!request.readOnly())
        updateStyleIfNeeded();

    return MouseEventWithHitTestResults(event, result);
}

// DOM Section 1.1.1
bool Document::childTypeAllowed(NodeType type) const
{
    switch (type) {
    case ATTRIBUTE_NODE:
    case CDATA_SECTION_NODE:
    case DOCUMENT_FRAGMENT_NODE:
    case DOCUMENT_NODE:
    case ENTITY_NODE:
    case ENTITY_REFERENCE_NODE:
    case NOTATION_NODE:
    case TEXT_NODE:
    case XPATH_NAMESPACE_NODE:
    case SHADOW_ROOT_NODE:
        return false;
    case COMMENT_NODE:
    case PROCESSING_INSTRUCTION_NODE:
        return true;
    case DOCUMENT_TYPE_NODE:
    case ELEMENT_NODE:
        // Documents may contain no more than one of each of these.
        // (One Element and one DocumentType.)
        for (Node* c = firstChild(); c; c = c->nextSibling())
            if (c->nodeType() == type)
                return false;
        return true;
    }
    return false;
}

bool Document::canReplaceChild(Node* newChild, Node* oldChild)
{
    if (!oldChild)
        // ContainerNode::replaceChild will raise a NOT_FOUND_ERR.
        return true;

    if (oldChild->nodeType() == newChild->nodeType())
        return true;

    int numDoctypes = 0;
    int numElements = 0;

    // First, check how many doctypes and elements we have, not counting
    // the child we're about to remove.
    for (Node* c = firstChild(); c; c = c->nextSibling()) {
        if (c == oldChild)
            continue;
        
        switch (c->nodeType()) {
        case DOCUMENT_TYPE_NODE:
            numDoctypes++;
            break;
        case ELEMENT_NODE:
            numElements++;
            break;
        default:
            break;
        }
    }
    
    // Then, see how many doctypes and elements might be added by the new child.
    if (newChild->nodeType() == DOCUMENT_FRAGMENT_NODE) {
        for (Node* c = firstChild(); c; c = c->nextSibling()) {
            switch (c->nodeType()) {
            case ATTRIBUTE_NODE:
            case CDATA_SECTION_NODE:
            case DOCUMENT_FRAGMENT_NODE:
            case DOCUMENT_NODE:
            case ENTITY_NODE:
            case ENTITY_REFERENCE_NODE:
            case NOTATION_NODE:
            case TEXT_NODE:
            case XPATH_NAMESPACE_NODE:
                return false;
            case COMMENT_NODE:
            case PROCESSING_INSTRUCTION_NODE:
                break;
            case DOCUMENT_TYPE_NODE:
                numDoctypes++;
                break;
            case ELEMENT_NODE:
                numElements++;
                break;
            case SHADOW_ROOT_NODE:
                ASSERT_NOT_REACHED();
                return false;
            }
        }
    } else {
        switch (newChild->nodeType()) {
        case ATTRIBUTE_NODE:
        case CDATA_SECTION_NODE:
        case DOCUMENT_FRAGMENT_NODE:
        case DOCUMENT_NODE:
        case ENTITY_NODE:
        case ENTITY_REFERENCE_NODE:
        case NOTATION_NODE:
        case TEXT_NODE:
        case XPATH_NAMESPACE_NODE:
        case SHADOW_ROOT_NODE:
            return false;
        case COMMENT_NODE:
        case PROCESSING_INSTRUCTION_NODE:
            return true;
        case DOCUMENT_TYPE_NODE:
            numDoctypes++;
            break;
        case ELEMENT_NODE:
            numElements++;
            break;
        }                
    }
        
    if (numElements > 1 || numDoctypes > 1)
        return false;
    
    return true;
}

PassRefPtr<Node> Document::cloneNode(bool /*deep*/)
{
    // Spec says cloning Document nodes is "implementation dependent"
    // so we do not support it...
    return 0;
}

StyleSheetList* Document::styleSheets()
{
    return m_styleSheets.get();
}

String Document::preferredStylesheetSet() const
{
    return m_preferredStylesheetSet;
}

String Document::selectedStylesheetSet() const
{
    return m_selectedStylesheetSet;
}

void Document::setSelectedStylesheetSet(const String& aString)
{
    m_selectedStylesheetSet = aString;
    styleSelectorChanged(DeferRecalcStyle);
}

// This method is called whenever a top-level stylesheet has finished loading.
void Document::removePendingSheet()
{
    // Make sure we knew this sheet was pending, and that our count isn't out of sync.
    ASSERT(m_pendingStylesheets > 0);

    m_pendingStylesheets--;
    
#ifdef INSTRUMENT_LAYOUT_SCHEDULING
    if (!ownerElement())
        printf("Stylesheet loaded at time %d. %d stylesheets still remain.\n", elapsedTime(), m_pendingStylesheets);
#endif

    if (m_pendingStylesheets)
        return;

    styleSelectorChanged(RecalcStyleImmediately);

    if (ScriptableDocumentParser* parser = scriptableDocumentParser())
        parser->executeScriptsWaitingForStylesheets();

    if (m_gotoAnchorNeededAfterStylesheetsLoad && view())
        view()->scrollToFragment(m_url);
}

void Document::styleSelectorChanged(StyleSelectorUpdateFlag updateFlag)
{
    // Don't bother updating, since we haven't loaded all our style info yet
    // and haven't calculated the style selector for the first time.
    if (!attached() || (!m_didCalculateStyleSelector && !haveStylesheetsLoaded()))
        return;

#ifdef INSTRUMENT_LAYOUT_SCHEDULING
    if (!ownerElement())
        printf("Beginning update of style selector at time %d.\n", elapsedTime());
#endif

    recalcStyleSelector();
    
    if (updateFlag == DeferRecalcStyle) {
        scheduleForcedStyleRecalc();
        return;
    }
    
    if (didLayoutWithPendingStylesheets() && m_pendingStylesheets <= 0) {
        m_pendingSheetLayout = IgnoreLayoutWithPendingSheets;
        if (renderer())
            renderer()->repaint();
    }

    // This recalcStyle initiates a new recalc cycle. We need to bracket it to
    // make sure animations get the correct update time
    if (m_frame)
        m_frame->animation()->beginAnimationUpdate();
    recalcStyle(Force);
    if (m_frame)
        m_frame->animation()->endAnimationUpdate();

#ifdef INSTRUMENT_LAYOUT_SCHEDULING
    if (!ownerElement())
        printf("Finished update of style selector at time %d\n", elapsedTime());
#endif

    if (renderer()) {
        renderer()->setNeedsLayoutAndPrefWidthsRecalc();
        if (view())
            view()->scheduleRelayout();
    }

    if (m_mediaQueryMatcher)
        m_mediaQueryMatcher->styleSelectorChanged();
}

void Document::addStyleSheetCandidateNode(Node* node, bool createdByParser)
{
    if (!node->inDocument())
        return;
    
    // Until the <body> exists, we have no choice but to compare document positions,
    // since styles outside of the body and head continue to be shunted into the head
    // (and thus can shift to end up before dynamically added DOM content that is also
    // outside the body).
    if ((createdByParser && body()) || m_styleSheetCandidateNodes.isEmpty()) {
        m_styleSheetCandidateNodes.add(node);
        return;
    }

    // Determine an appropriate insertion point.
    StyleSheetCandidateListHashSet::iterator begin = m_styleSheetCandidateNodes.begin();
    StyleSheetCandidateListHashSet::iterator end = m_styleSheetCandidateNodes.end();
    StyleSheetCandidateListHashSet::iterator it = end;
    Node* followingNode = 0;
    do {
        --it;
        Node* n = *it;
        unsigned short position = n->compareDocumentPosition(node);
        if (position == DOCUMENT_POSITION_FOLLOWING) {
            m_styleSheetCandidateNodes.insertBefore(followingNode, node);
            return;
        }
        followingNode = n;
    } while (it != begin);
    
    m_styleSheetCandidateNodes.insertBefore(followingNode, node);
}

void Document::removeStyleSheetCandidateNode(Node* node)
{
    m_styleSheetCandidateNodes.remove(node);
}

void Document::recalcStyleSelector()
{
    if (m_inStyleRecalc) {
        // SVG <use> element may manage to invalidate style selector in the middle of a style recalc.
        // https://bugs.webkit.org/show_bug.cgi?id=54344
        // FIXME: This should be fixed in SVG and this code replaced with ASSERT(!m_inStyleRecalc).
        m_hasDirtyStyleSelector = true;
        scheduleForcedStyleRecalc();
        return;
    }
    if (!renderer() || !attached())
        return;

    StyleSheetVector sheets;

    bool matchAuthorAndUserStyles = true;
    if (Settings* settings = this->settings())
        matchAuthorAndUserStyles = settings->authorAndUserStylesEnabled();

    StyleSheetCandidateListHashSet::iterator begin = m_styleSheetCandidateNodes.begin();
    StyleSheetCandidateListHashSet::iterator end = m_styleSheetCandidateNodes.end();
    if (!matchAuthorAndUserStyles)
        end = begin;
    for (StyleSheetCandidateListHashSet::iterator it = begin; it != end; ++it) {
        Node* n = *it;

        StyleSheet* sheet = 0;

        if (n->nodeType() == PROCESSING_INSTRUCTION_NODE) {
            // Processing instruction (XML documents only).
            // We don't support linking to embedded CSS stylesheets, see <https://bugs.webkit.org/show_bug.cgi?id=49281> for discussion.
            ProcessingInstruction* pi = static_cast<ProcessingInstruction*>(n);
            sheet = pi->sheet();
#if ENABLE(XSLT)
            // Don't apply XSL transforms to already transformed documents -- <rdar://problem/4132806>
            if (pi->isXSL() && !transformSourceDocument()) {
                // Don't apply XSL transforms until loading is finished.
                if (!parsing())
                    applyXSLTransform(pi);
                return;
            }
#endif
        } else if ((n->isHTMLElement() && (n->hasTagName(linkTag) || n->hasTagName(styleTag)))
#if ENABLE(SVG)
            ||  (n->isSVGElement() && n->hasTagName(SVGNames::styleTag))
#endif
        ) {
            Element* e = static_cast<Element*>(n);
            AtomicString title = e->getAttribute(titleAttr);
            bool enabledViaScript = false;
            if (e->hasLocalName(linkTag)) {
                // <LINK> element
                HTMLLinkElement* linkElement = static_cast<HTMLLinkElement*>(n);
                if (linkElement->disabled())
                    continue;
                enabledViaScript = linkElement->isEnabledViaScript();
                if (linkElement->isLoading()) {
                    // it is loading but we should still decide which style sheet set to use
                    if (!enabledViaScript && !title.isEmpty() && m_preferredStylesheetSet.isEmpty()) {
                        const AtomicString& rel = e->getAttribute(relAttr);
                        if (!rel.contains("alternate")) {
                            m_preferredStylesheetSet = title;
                            m_selectedStylesheetSet = title;
                        }
                    }
                    continue;
                }
                if (!linkElement->sheet())
                    title = nullAtom;
            }

            // Get the current preferred styleset.  This is the
            // set of sheets that will be enabled.
#if ENABLE(SVG)
            if (n->isSVGElement() && n->hasTagName(SVGNames::styleTag))
                sheet = static_cast<SVGStyleElement*>(n)->sheet();
            else
#endif
            if (e->hasLocalName(linkTag))
                sheet = static_cast<HTMLLinkElement*>(n)->sheet();
            else
                // <STYLE> element
                sheet = static_cast<HTMLStyleElement*>(n)->sheet();

            // Check to see if this sheet belongs to a styleset
            // (thus making it PREFERRED or ALTERNATE rather than
            // PERSISTENT).
            if (!enabledViaScript && !title.isEmpty()) {
                // Yes, we have a title.
                if (m_preferredStylesheetSet.isEmpty()) {
                    // No preferred set has been established.  If
                    // we are NOT an alternate sheet, then establish
                    // us as the preferred set.  Otherwise, just ignore
                    // this sheet.
                    AtomicString rel = e->getAttribute(relAttr);
                    if (e->hasLocalName(styleTag) || !rel.contains("alternate"))
                        m_preferredStylesheetSet = m_selectedStylesheetSet = title;
                }

                if (title != m_preferredStylesheetSet)
                    sheet = 0;
            }
        }

        if (sheet)
            sheets.append(sheet);
    }

    m_styleSheets->swap(sheets);

    m_styleSelector.clear();
    m_didCalculateStyleSelector = true;
    m_hasDirtyStyleSelector = false;
}

void Document::setHoverNode(PassRefPtr<Node> newHoverNode)
{
    m_hoverNode = newHoverNode;
}

void Document::setActiveNode(PassRefPtr<Node> newActiveNode)
{
    m_activeNode = newActiveNode;
}

void Document::focusedNodeRemoved()
{
    setFocusedNode(0);
}

void Document::removeFocusedNodeOfSubtree(Node* node, bool amongChildrenOnly)
{
    if (!m_focusedNode || this->inPageCache()) // If the document is in the page cache, then we don't need to clear out the focused node.
        return;
        
    bool nodeInSubtree = false;
    if (amongChildrenOnly)
        nodeInSubtree = m_focusedNode->isDescendantOf(node);
    else
        nodeInSubtree = (m_focusedNode == node) || m_focusedNode->isDescendantOf(node);
    
    if (nodeInSubtree)
        document()->focusedNodeRemoved();
}

void Document::hoveredNodeDetached(Node* node)
{
    if (!m_hoverNode || (node != m_hoverNode && (!m_hoverNode->isTextNode() || node != m_hoverNode->parentNode())))
        return;

    m_hoverNode = node->parentNode();
    while (m_hoverNode && !m_hoverNode->renderer())
        m_hoverNode = m_hoverNode->parentNode();
    if (frame())
        frame()->eventHandler()->scheduleHoverStateUpdate();
}

void Document::activeChainNodeDetached(Node* node)
{
    if (!m_activeNode || (node != m_activeNode && (!m_activeNode->isTextNode() || node != m_activeNode->parentNode())))
        return;

    m_activeNode = node->parentNode();
    while (m_activeNode && !m_activeNode->renderer())
        m_activeNode = m_activeNode->parentNode();
}

#if ENABLE(DASHBOARD_SUPPORT)
const Vector<DashboardRegionValue>& Document::dashboardRegions() const
{
    return m_dashboardRegions;
}

void Document::setDashboardRegions(const Vector<DashboardRegionValue>& regions)
{
    m_dashboardRegions = regions;
    setDashboardRegionsDirty(false);
}
#endif

bool Document::setFocusedNode(PassRefPtr<Node> newFocusedNode)
{    
    // Make sure newFocusedNode is actually in this document
    if (newFocusedNode && (newFocusedNode->document() != this))
        return true;

    if (m_focusedNode == newFocusedNode)
        return true;

    if (m_inPageCache)
        return false;

    bool focusChangeBlocked = false;
    RefPtr<Node> oldFocusedNode = m_focusedNode;
    m_focusedNode = 0;

    // Remove focus from the existing focus node (if any)
    if (oldFocusedNode && !oldFocusedNode->inDetach()) { 
        if (oldFocusedNode->active())
            oldFocusedNode->setActive(false);

        oldFocusedNode->setFocus(false);

        // Dispatch a change event for text fields or textareas that have been edited
        if (oldFocusedNode->isElementNode()) {
            Element* element = static_cast<Element*>(oldFocusedNode.get());
            if (element->wasChangedSinceLastFormControlChangeEvent())
                element->dispatchFormControlChangeEvent();
        }

        // Dispatch the blur event and let the node do any other blur related activities (important for text fields)
        oldFocusedNode->dispatchBlurEvent();

        if (m_focusedNode) {
            // handler shifted focus
            focusChangeBlocked = true;
            newFocusedNode = 0;
        }
        
        oldFocusedNode->dispatchUIEvent(eventNames().focusoutEvent, 0, 0); // DOM level 3 name for the bubbling blur event.
        // FIXME: We should remove firing DOMFocusOutEvent event when we are sure no content depends
        // on it, probably when <rdar://problem/8503958> is resolved.
        oldFocusedNode->dispatchUIEvent(eventNames().DOMFocusOutEvent, 0, 0); // DOM level 2 name for compatibility.

        if (m_focusedNode) {
            // handler shifted focus
            focusChangeBlocked = true;
            newFocusedNode = 0;
        }
        if (oldFocusedNode == this && oldFocusedNode->hasOneRef())
            return true;
            
        if (oldFocusedNode == oldFocusedNode->rootEditableElement())
            frame()->editor()->didEndEditing();

        if (view()) {
            Widget* oldWidget = widgetForNode(oldFocusedNode.get());
            if (oldWidget)
                oldWidget->setFocus(false);
            else
                view()->setFocus(false);
        }
    }

    if (newFocusedNode) {
        if (newFocusedNode == newFocusedNode->rootEditableElement() && !acceptsEditingFocus(newFocusedNode.get())) {
            // delegate blocks focus change
            focusChangeBlocked = true;
            goto SetFocusedNodeDone;
        }
        // Set focus on the new node
        m_focusedNode = newFocusedNode.get();

        // Dispatch the focus event and let the node do any other focus related activities (important for text fields)
        m_focusedNode->dispatchFocusEvent();

        if (m_focusedNode != newFocusedNode) {
            // handler shifted focus
            focusChangeBlocked = true;
            goto SetFocusedNodeDone;
        }

        m_focusedNode->dispatchUIEvent(eventNames().focusinEvent, 0, 0); // DOM level 3 bubbling focus event.
        // FIXME: We should remove firing DOMFocusInEvent event when we are sure no content depends
        // on it, probably when <rdar://problem/8503958> is resolved.
        m_focusedNode->dispatchUIEvent(eventNames().DOMFocusInEvent, 0, 0); // DOM level 2 for compatibility.

        if (m_focusedNode != newFocusedNode) { 
            // handler shifted focus
            focusChangeBlocked = true;
            goto SetFocusedNodeDone;
        }
        m_focusedNode->setFocus(true);

        if (m_focusedNode == m_focusedNode->rootEditableElement())
            frame()->editor()->didBeginEditing();

        // eww, I suck. set the qt focus correctly
        // ### find a better place in the code for this
        if (view()) {
            Widget* focusWidget = widgetForNode(m_focusedNode.get());
            if (focusWidget) {
                // Make sure a widget has the right size before giving it focus.
                // Otherwise, we are testing edge cases of the Widget code.
                // Specifically, in WebCore this does not work well for text fields.
                updateLayout();
                // Re-get the widget in case updating the layout changed things.
                focusWidget = widgetForNode(m_focusedNode.get());
            }
            if (focusWidget)
                focusWidget->setFocus(true);
            else
                view()->setFocus(true);
        }
    }

#if PLATFORM(MAC) || PLATFORM(WIN) || PLATFORM(GTK) || PLATFORM(CHROMIUM)
    if (!focusChangeBlocked && m_focusedNode && AXObjectCache::accessibilityEnabled()) {
        RenderObject* oldFocusedRenderer = 0;
        RenderObject* newFocusedRenderer = 0;

        if (oldFocusedNode)
            oldFocusedRenderer = oldFocusedNode->renderer();
        if (newFocusedNode)
            newFocusedRenderer = newFocusedNode->renderer();

        axObjectCache()->handleFocusedUIElementChanged(oldFocusedRenderer, newFocusedRenderer);
    }
#endif
    if (!focusChangeBlocked)
        page()->chrome()->focusedNodeChanged(m_focusedNode.get());

SetFocusedNodeDone:
    updateStyleIfNeeded();
    return !focusChangeBlocked;
}
    
void Document::getFocusableNodes(Vector<RefPtr<Node> >& nodes)
{
    updateLayout();

    for (Node* node = firstChild(); node; node = node->traverseNextNode()) {
        if (node->isFocusable())
            nodes.append(node);
    }
}
  
void Document::setCSSTarget(Element* n)
{
    if (m_cssTarget)
        m_cssTarget->setNeedsStyleRecalc();
    m_cssTarget = n;
    if (n)
        n->setNeedsStyleRecalc();
}

void Document::attachNodeIterator(NodeIterator* ni)
{
    m_nodeIterators.add(ni);
}

void Document::detachNodeIterator(NodeIterator* ni)
{
    // The node iterator can be detached without having been attached if its root node didn't have a document
    // when the iterator was created, but has it now.
    m_nodeIterators.remove(ni);
}

void Document::moveNodeIteratorsToNewDocument(Node* node, Document* newDocument)
{
    HashSet<NodeIterator*> nodeIteratorsList = m_nodeIterators;
    HashSet<NodeIterator*>::const_iterator nodeIteratorsEnd = nodeIteratorsList.end();
    for (HashSet<NodeIterator*>::const_iterator it = nodeIteratorsList.begin(); it != nodeIteratorsEnd; ++it) {
        if ((*it)->root() == node) {
            detachNodeIterator(*it);
            newDocument->attachNodeIterator(*it);
        }
    }
}

void Document::nodeChildrenChanged(ContainerNode* container)
{
    if (!disableRangeMutation(page()) && !m_ranges.isEmpty()) {
        HashSet<Range*>::const_iterator end = m_ranges.end();
        for (HashSet<Range*>::const_iterator it = m_ranges.begin(); it != end; ++it)
            (*it)->nodeChildrenChanged(container);
    }
}

void Document::nodeChildrenWillBeRemoved(ContainerNode* container)
{
    if (!disableRangeMutation(page()) && !m_ranges.isEmpty()) {
        HashSet<Range*>::const_iterator end = m_ranges.end();
        for (HashSet<Range*>::const_iterator it = m_ranges.begin(); it != end; ++it)
            (*it)->nodeChildrenWillBeRemoved(container);
    }

    HashSet<NodeIterator*>::const_iterator nodeIteratorsEnd = m_nodeIterators.end();
    for (HashSet<NodeIterator*>::const_iterator it = m_nodeIterators.begin(); it != nodeIteratorsEnd; ++it) {
        for (Node* n = container->firstChild(); n; n = n->nextSibling())
            (*it)->nodeWillBeRemoved(n);
    }

    if (Frame* frame = this->frame()) {
        for (Node* n = container->firstChild(); n; n = n->nextSibling()) {
            frame->selection()->nodeWillBeRemoved(n);
            frame->page()->dragCaretController()->nodeWillBeRemoved(n);
        }
    }
}

void Document::nodeWillBeRemoved(Node* n)
{
    HashSet<NodeIterator*>::const_iterator nodeIteratorsEnd = m_nodeIterators.end();
    for (HashSet<NodeIterator*>::const_iterator it = m_nodeIterators.begin(); it != nodeIteratorsEnd; ++it)
        (*it)->nodeWillBeRemoved(n);

    if (!disableRangeMutation(page()) && !m_ranges.isEmpty()) {
        HashSet<Range*>::const_iterator rangesEnd = m_ranges.end();
        for (HashSet<Range*>::const_iterator it = m_ranges.begin(); it != rangesEnd; ++it)
            (*it)->nodeWillBeRemoved(n);
    }

    if (Frame* frame = this->frame()) {
        frame->selection()->nodeWillBeRemoved(n);
        frame->page()->dragCaretController()->nodeWillBeRemoved(n);
    }
}

void Document::textInserted(Node* text, unsigned offset, unsigned length)
{
    if (!disableRangeMutation(page()) && !m_ranges.isEmpty()) {
        HashSet<Range*>::const_iterator end = m_ranges.end();
        for (HashSet<Range*>::const_iterator it = m_ranges.begin(); it != end; ++it)
            (*it)->textInserted(text, offset, length);
    }

    // Update the markers for spelling and grammar checking.
    m_markers->shiftMarkers(text, offset, length);
}

void Document::textRemoved(Node* text, unsigned offset, unsigned length)
{
    if (!disableRangeMutation(page()) && !m_ranges.isEmpty()) {
        HashSet<Range*>::const_iterator end = m_ranges.end();
        for (HashSet<Range*>::const_iterator it = m_ranges.begin(); it != end; ++it)
            (*it)->textRemoved(text, offset, length);
    }

    // Update the markers for spelling and grammar checking.
    m_markers->removeMarkers(text, offset, length);
    m_markers->shiftMarkers(text, offset + length, 0 - length);
}

void Document::textNodesMerged(Text* oldNode, unsigned offset)
{
    if (!disableRangeMutation(page()) && !m_ranges.isEmpty()) {
        NodeWithIndex oldNodeWithIndex(oldNode);
        HashSet<Range*>::const_iterator end = m_ranges.end();
        for (HashSet<Range*>::const_iterator it = m_ranges.begin(); it != end; ++it)
            (*it)->textNodesMerged(oldNodeWithIndex, offset);
    }

    // FIXME: This should update markers for spelling and grammar checking.
}

void Document::textNodeSplit(Text* oldNode)
{
    if (!disableRangeMutation(page()) && !m_ranges.isEmpty()) {
        HashSet<Range*>::const_iterator end = m_ranges.end();
        for (HashSet<Range*>::const_iterator it = m_ranges.begin(); it != end; ++it)
            (*it)->textNodeSplit(oldNode);
    }

    // FIXME: This should update markers for spelling and grammar checking.
}

// FIXME: eventually, this should return a DOMWindow stored in the document.
DOMWindow* Document::domWindow() const
{
    if (!frame())
        return 0;

    // The m_frame pointer is not (not always?) zeroed out when the document is put into b/f cache, so the frame can hold an unrelated document/window pair.
    // FIXME: We should always zero out the frame pointer on navigation to avoid accidentally accessing the new frame content.
    if (m_frame->document() != this)
        return 0;

    return frame()->domWindow();
}

void Document::setWindowAttributeEventListener(const AtomicString& eventType, PassRefPtr<EventListener> listener)
{
    DOMWindow* domWindow = this->domWindow();
    if (!domWindow)
        return;
    domWindow->setAttributeEventListener(eventType, listener);
}

EventListener* Document::getWindowAttributeEventListener(const AtomicString& eventType)
{
    DOMWindow* domWindow = this->domWindow();
    if (!domWindow)
        return 0;
    return domWindow->getAttributeEventListener(eventType);
}

void Document::dispatchWindowEvent(PassRefPtr<Event> event,  PassRefPtr<EventTarget> target)
{
    ASSERT(!eventDispatchForbidden());
    DOMWindow* domWindow = this->domWindow();
    if (!domWindow)
        return;
    domWindow->dispatchEvent(event, target);
}

void Document::dispatchWindowLoadEvent()
{
    ASSERT(!eventDispatchForbidden());
    DOMWindow* domWindow = this->domWindow();
    if (!domWindow)
        return;
    domWindow->dispatchLoadEvent();
}

void Document::enqueueWindowEvent(PassRefPtr<Event> event)
{
    event->setTarget(domWindow());
    m_eventQueue->enqueueEvent(event);
}

void Document::enqueueDocumentEvent(PassRefPtr<Event> event)
{
    event->setTarget(this);
    m_eventQueue->enqueueEvent(event);
}

PassRefPtr<Event> Document::createEvent(const String& eventType, ExceptionCode& ec)
{
    RefPtr<Event> event;
    if (eventType == "Event" || eventType == "Events" || eventType == "HTMLEvents")
        event = Event::create();
    else if (eventType == "CustomEvent")
        event = CustomEvent::create();
    else if (eventType == "KeyboardEvent" || eventType == "KeyboardEvents")
        event = KeyboardEvent::create();
    else if (eventType == "MessageEvent")
        event = MessageEvent::create();
    else if (eventType == "MouseEvent" || eventType == "MouseEvents")
        event = MouseEvent::create();
    else if (eventType == "MutationEvent" || eventType == "MutationEvents")
        event = MutationEvent::create();
    else if (eventType == "OverflowEvent")
        event = OverflowEvent::create();
    else if (eventType == "PageTransitionEvent")
        event = PageTransitionEvent::create();
    else if (eventType == "ProgressEvent")
        event = ProgressEvent::create();
#if ENABLE(DOM_STORAGE)
    else if (eventType == "StorageEvent")
        event = StorageEvent::create();
#endif
    else if (eventType == "TextEvent")
        event = TextEvent::create();
    else if (eventType == "UIEvent" || eventType == "UIEvents")
        event = UIEvent::create();
    else if (eventType == "WebKitAnimationEvent")
        event = WebKitAnimationEvent::create();
    else if (eventType == "WebKitTransitionEvent")
        event = WebKitTransitionEvent::create();
    else if (eventType == "WheelEvent")
        event = WheelEvent::create();
#if ENABLE(SVG)
    else if (eventType == "SVGEvents")
        event = Event::create();
    else if (eventType == "SVGZoomEvents")
        event = SVGZoomEvent::create();
#endif
#if ENABLE(TOUCH_EVENTS)
#if USE(V8)
    else if (eventType == "TouchEvent" && RuntimeEnabledFeatures::touchEnabled())
#else
    else if (eventType == "TouchEvent")
#endif
        event = TouchEvent::create();
#endif
#if ENABLE(DEVICE_ORIENTATION)
    else if (eventType == "DeviceMotionEvent")
        event = DeviceMotionEvent::create();
    else if (eventType == "DeviceOrientationEvent")
        event = DeviceOrientationEvent::create();
#endif
#if ENABLE(ORIENTATION_EVENTS)
    else if (eventType == "OrientationEvent")
        event = Event::create();
#endif
    if (event)
        return event.release();

    ec = NOT_SUPPORTED_ERR;
    return 0;
}

void Document::addListenerTypeIfNeeded(const AtomicString& eventType)
{
    if (eventType == eventNames().DOMSubtreeModifiedEvent)
        addListenerType(DOMSUBTREEMODIFIED_LISTENER);
    else if (eventType == eventNames().DOMNodeInsertedEvent)
        addListenerType(DOMNODEINSERTED_LISTENER);
    else if (eventType == eventNames().DOMNodeRemovedEvent)
        addListenerType(DOMNODEREMOVED_LISTENER);
    else if (eventType == eventNames().DOMNodeRemovedFromDocumentEvent)
        addListenerType(DOMNODEREMOVEDFROMDOCUMENT_LISTENER);
    else if (eventType == eventNames().DOMNodeInsertedIntoDocumentEvent)
        addListenerType(DOMNODEINSERTEDINTODOCUMENT_LISTENER);
    else if (eventType == eventNames().DOMAttrModifiedEvent)
        addListenerType(DOMATTRMODIFIED_LISTENER);
    else if (eventType == eventNames().DOMCharacterDataModifiedEvent)
        addListenerType(DOMCHARACTERDATAMODIFIED_LISTENER);
    else if (eventType == eventNames().overflowchangedEvent)
        addListenerType(OVERFLOWCHANGED_LISTENER);
    else if (eventType == eventNames().webkitAnimationStartEvent)
        addListenerType(ANIMATIONSTART_LISTENER);
    else if (eventType == eventNames().webkitAnimationEndEvent)
        addListenerType(ANIMATIONEND_LISTENER);
    else if (eventType == eventNames().webkitAnimationIterationEvent)
        addListenerType(ANIMATIONITERATION_LISTENER);
    else if (eventType == eventNames().webkitTransitionEndEvent)
        addListenerType(TRANSITIONEND_LISTENER);
    else if (eventType == eventNames().beforeloadEvent)
        addListenerType(BEFORELOAD_LISTENER);
    else if (eventType == eventNames().beforeprocessEvent)
        addListenerType(BEFOREPROCESS_LISTENER);
#if ENABLE(TOUCH_EVENTS)
    else if (eventType == eventNames().touchstartEvent
             || eventType == eventNames().touchmoveEvent
             || eventType == eventNames().touchendEvent
             || eventType == eventNames().touchcancelEvent) {
        addListenerType(TOUCH_LISTENER);
        if (Page* page = this->page())
            page->chrome()->client()->needTouchEvents(true);
    }
#endif
}

CSSStyleDeclaration* Document::getOverrideStyle(Element*, const String&)
{
    return 0;
}

HTMLFrameOwnerElement* Document::ownerElement() const
{
    if (!frame())
        return 0;
    return frame()->ownerElement();
}

String Document::cookie(ExceptionCode& ec) const
{
    if (page() && !page()->cookieEnabled())
        return String();

    // FIXME: The HTML5 DOM spec states that this attribute can raise an
    // INVALID_STATE_ERR exception on getting if the Document has no
    // browsing context.

    if (!securityOrigin()->canAccessCookies()) {
        ec = SECURITY_ERR;
        return String();
    }

    KURL cookieURL = this->cookieURL();
    if (cookieURL.isEmpty())
        return String();

    return cookies(this, cookieURL);
}

void Document::setCookie(const String& value, ExceptionCode& ec)
{
    if (page() && !page()->cookieEnabled())
        return;

    // FIXME: The HTML5 DOM spec states that this attribute can raise an
    // INVALID_STATE_ERR exception on setting if the Document has no
    // browsing context.

    if (!securityOrigin()->canAccessCookies()) {
        ec = SECURITY_ERR;
        return;
    }

    KURL cookieURL = this->cookieURL();
    if (cookieURL.isEmpty())
        return;

    setCookies(this, cookieURL, value);
}

String Document::referrer() const
{
    if (frame())
        return frame()->loader()->referrer();
    return String();
}

String Document::domain() const
{
    return securityOrigin()->domain();
}

void Document::setDomain(const String& newDomain, ExceptionCode& ec)
{
    if (SecurityOrigin::isDomainRelaxationForbiddenForURLScheme(securityOrigin()->protocol())) {
        ec = SECURITY_ERR;
        return;
    }

    // Both NS and IE specify that changing the domain is only allowed when
    // the new domain is a suffix of the old domain.

    // FIXME: We should add logging indicating why a domain was not allowed.

    // If the new domain is the same as the old domain, still call
    // securityOrigin()->setDomainForDOM. This will change the
    // security check behavior. For example, if a page loaded on port 8000
    // assigns its current domain using document.domain, the page will
    // allow other pages loaded on different ports in the same domain that
    // have also assigned to access this page.
    if (equalIgnoringCase(domain(), newDomain)) {
        securityOrigin()->setDomainFromDOM(newDomain);
        if (m_frame)
            m_frame->script()->updateSecurityOrigin();
        return;
    }

    int oldLength = domain().length();
    int newLength = newDomain.length();
    // e.g. newDomain = webkit.org (10) and domain() = www.webkit.org (14)
    if (newLength >= oldLength) {
        ec = SECURITY_ERR;
        return;
    }

    String test = domain();
    // Check that it's a subdomain, not e.g. "ebkit.org"
    if (test[oldLength - newLength - 1] != '.') {
        ec = SECURITY_ERR;
        return;
    }

    // Now test is "webkit.org" from domain()
    // and we check that it's the same thing as newDomain
    test.remove(0, oldLength - newLength);
    if (test != newDomain) {
        ec = SECURITY_ERR;
        return;
    }

    securityOrigin()->setDomainFromDOM(newDomain);
    if (m_frame)
        m_frame->script()->updateSecurityOrigin();
}

// http://www.whatwg.org/specs/web-apps/current-work/#dom-document-lastmodified
String Document::lastModified() const
{
    DateComponents date;
    bool foundDate = false;
    if (m_frame) {
        String httpLastModified = m_documentLoader->response().httpHeaderField("Last-Modified");
        if (!httpLastModified.isEmpty()) {
            date.setMillisecondsSinceEpochForDateTime(parseDate(httpLastModified));
            foundDate = true;
        }
    }
    // FIXME: If this document came from the file system, the HTML5
    // specificiation tells us to read the last modification date from the file
    // system.
    if (!foundDate)
        date.setMillisecondsSinceEpochForDateTime(currentTimeMS());
    return String::format("%02d/%02d/%04d %02d:%02d:%02d", date.month() + 1, date.monthDay(), date.fullYear(), date.hour(), date.minute(), date.second());
}

static bool isValidNameNonASCII(const UChar* characters, unsigned length)
{
    unsigned i = 0;

    UChar32 c;
    U16_NEXT(characters, i, length, c)
    if (!isValidNameStart(c))
        return false;

    while (i < length) {
        U16_NEXT(characters, i, length, c)
        if (!isValidNamePart(c))
            return false;
    }

    return true;
}

static inline bool isValidNameASCII(const UChar* characters, unsigned length)
{
    UChar c = characters[0];
    if (!(isASCIIAlpha(c) || c == ':' || c == '_'))
        return false;

    for (unsigned i = 1; i < length; ++i) {
        c = characters[i];
        if (!(isASCIIAlphanumeric(c) || c == ':' || c == '_' || c == '-' || c == '.'))
            return false;
    }

    return true;
}

bool Document::isValidName(const String& name)
{
    unsigned length = name.length();
    if (!length)
        return false;

    const UChar* characters = name.characters();
    return isValidNameASCII(characters, length) || isValidNameNonASCII(characters, length);
}

bool Document::parseQualifiedName(const String& qualifiedName, String& prefix, String& localName, ExceptionCode& ec)
{
    unsigned length = qualifiedName.length();

    if (!length) {
        ec = INVALID_CHARACTER_ERR;
        return false;
    }

    bool nameStart = true;
    bool sawColon = false;
    int colonPos = 0;

    const UChar* s = qualifiedName.characters();
    for (unsigned i = 0; i < length;) {
        UChar32 c;
        U16_NEXT(s, i, length, c)
        if (c == ':') {
            if (sawColon) {
                ec = NAMESPACE_ERR;
                return false; // multiple colons: not allowed
            }
            nameStart = true;
            sawColon = true;
            colonPos = i - 1;
        } else if (nameStart) {
            if (!isValidNameStart(c)) {
                ec = INVALID_CHARACTER_ERR;
                return false;
            }
            nameStart = false;
        } else {
            if (!isValidNamePart(c)) {
                ec = INVALID_CHARACTER_ERR;
                return false;
            }
        }
    }

    if (!sawColon) {
        prefix = String();
        localName = qualifiedName;
    } else {
        prefix = qualifiedName.substring(0, colonPos);
        if (prefix.isEmpty()) {
            ec = NAMESPACE_ERR;
            return false;
        }
        localName = qualifiedName.substring(colonPos + 1);
    }

    if (localName.isEmpty()) {
        ec = NAMESPACE_ERR;
        return false;
    }

    return true;
}

void Document::setDecoder(PassRefPtr<TextResourceDecoder> decoder)
{
    m_decoder = decoder;
}

KURL Document::completeURL(const String& url) const
{
    // Always return a null URL when passed a null string.
    // FIXME: Should we change the KURL constructor to have this behavior?
    // See also [CSS]StyleSheet::completeURL(const String&)
    if (url.isNull())
        return KURL();
    const KURL& baseURL = ((m_baseURL.isEmpty() || m_baseURL == blankURL()) && parentDocument()) ? parentDocument()->baseURL() : m_baseURL;
    if (!m_decoder)
        return KURL(baseURL, url);
    return KURL(baseURL, url, m_decoder->encoding());
}

void Document::setInPageCache(bool flag)
{
    if (m_inPageCache == flag)
        return;

    m_inPageCache = flag;
    if (flag) {
        ASSERT(!m_savedRenderer);
        m_savedRenderer = renderer();
        if (FrameView* v = view()) {
            v->cacheCurrentScrollPosition();
            if (page() && page()->mainFrame() == m_frame)
                v->resetScrollbarsAndClearContentsSize();
            else
                v->resetScrollbars();
        }
        m_styleRecalcTimer.stop();
    } else {
        ASSERT(!renderer() || renderer() == m_savedRenderer);
        ASSERT(m_renderArena);
        setRenderer(m_savedRenderer);
        m_savedRenderer = 0;

        if (frame() && frame()->page())
            frame()->page()->updateViewportArguments();

        if (childNeedsStyleRecalc())
            scheduleStyleRecalc();
    }
}

void Document::documentWillBecomeInactive() 
{
#if USE(ACCELERATED_COMPOSITING)
    if (renderer())
        renderView()->willMoveOffscreen();
#endif

    HashSet<Element*>::iterator end = m_documentActivationCallbackElements.end();
    for (HashSet<Element*>::iterator i = m_documentActivationCallbackElements.begin(); i != end; ++i)
        (*i)->documentWillBecomeInactive();
}

void Document::documentDidBecomeActive() 
{
    HashSet<Element*>::iterator end = m_documentActivationCallbackElements.end();
    for (HashSet<Element*>::iterator i = m_documentActivationCallbackElements.begin(); i != end; ++i)
        (*i)->documentDidBecomeActive();

#if USE(ACCELERATED_COMPOSITING)
    if (renderer())
        renderView()->didMoveOnscreen();
#endif

    ASSERT(m_frame);
    m_frame->loader()->client()->dispatchDidBecomeFrameset(isFrameSet());
}

void Document::registerForDocumentActivationCallbacks(Element* e)
{
    m_documentActivationCallbackElements.add(e);
}

void Document::unregisterForDocumentActivationCallbacks(Element* e)
{
    m_documentActivationCallbackElements.remove(e);
}

void Document::mediaVolumeDidChange() 
{
    HashSet<Element*>::iterator end = m_mediaVolumeCallbackElements.end();
    for (HashSet<Element*>::iterator i = m_mediaVolumeCallbackElements.begin(); i != end; ++i)
        (*i)->mediaVolumeDidChange();
}

void Document::registerForMediaVolumeCallbacks(Element* e)
{
    m_mediaVolumeCallbackElements.add(e);
}

void Document::unregisterForMediaVolumeCallbacks(Element* e)
{
    m_mediaVolumeCallbackElements.remove(e);
}

void Document::privateBrowsingStateDidChange() 
{
    HashSet<Element*>::iterator end = m_privateBrowsingStateChangedElements.end();
    for (HashSet<Element*>::iterator it = m_privateBrowsingStateChangedElements.begin(); it != end; ++it)
        (*it)->privateBrowsingStateDidChange();
}

void Document::registerForPrivateBrowsingStateChangedCallbacks(Element* e)
{
    m_privateBrowsingStateChangedElements.add(e);
}

void Document::unregisterForPrivateBrowsingStateChangedCallbacks(Element* e)
{
    m_privateBrowsingStateChangedElements.remove(e);
}

void Document::setShouldCreateRenderers(bool f)
{
    m_createRenderers = f;
}

bool Document::shouldCreateRenderers()
{
    return m_createRenderers;
}

// Support for Javascript execCommand, and related methods

static Editor::Command command(Document* document, const String& commandName, bool userInterface = false)
{
    Frame* frame = document->frame();
    if (!frame || frame->document() != document)
        return Editor::Command();

    document->updateStyleIfNeeded();

    return frame->editor()->command(commandName,
        userInterface ? CommandFromDOMWithUserInterface : CommandFromDOM);
}

bool Document::execCommand(const String& commandName, bool userInterface, const String& value)
{
    return command(this, commandName, userInterface).execute(value);
}

bool Document::queryCommandEnabled(const String& commandName)
{
    return command(this, commandName).isEnabled();
}

bool Document::queryCommandIndeterm(const String& commandName)
{
    return command(this, commandName).state() == MixedTriState;
}

bool Document::queryCommandState(const String& commandName)
{
    return command(this, commandName).state() == TrueTriState;
}

bool Document::queryCommandSupported(const String& commandName)
{
    return command(this, commandName).isSupported();
}

String Document::queryCommandValue(const String& commandName)
{
    return command(this, commandName).value();
}

#if ENABLE(XSLT)

void Document::applyXSLTransform(ProcessingInstruction* pi)
{
    RefPtr<XSLTProcessor> processor = XSLTProcessor::create();
    processor->setXSLStyleSheet(static_cast<XSLStyleSheet*>(pi->sheet()));
    String resultMIMEType;
    String newSource;
    String resultEncoding;
    if (!processor->transformToString(this, resultMIMEType, newSource, resultEncoding))
        return;
    // FIXME: If the transform failed we should probably report an error (like Mozilla does).
    processor->createDocumentFromSource(newSource, resultEncoding, resultMIMEType, this, frame());
}

void Document::setTransformSource(PassOwnPtr<TransformSource> source)
{
    m_transformSource = source;
}

#endif

void Document::setDesignMode(InheritedBool value)
{
    m_designMode = value;
    for (Frame* frame = m_frame; frame && frame->document(); frame = frame->tree()->traverseNext(m_frame))
        frame->document()->scheduleForcedStyleRecalc();
}

Document::InheritedBool Document::getDesignMode() const
{
    return m_designMode;
}

bool Document::inDesignMode() const
{
    for (const Document* d = this; d; d = d->parentDocument()) {
        if (d->m_designMode != inherit)
            return d->m_designMode;
    }
    return false;
}

Document* Document::parentDocument() const
{
    if (!m_frame)
        return 0;
    Frame* parent = m_frame->tree()->parent();
    if (!parent)
        return 0;
    return parent->document();
}

Document* Document::topDocument() const
{
    Document* doc = const_cast<Document *>(this);
    Element* element;
    while ((element = doc->ownerElement()))
        doc = element->document();
    
    return doc;
}

PassRefPtr<Attr> Document::createAttribute(const String& name, ExceptionCode& ec)
{
    return createAttributeNS(String(), name, ec, true);
}

PassRefPtr<Attr> Document::createAttributeNS(const String& namespaceURI, const String& qualifiedName, ExceptionCode& ec, bool shouldIgnoreNamespaceChecks)
{
    String prefix, localName;
    if (!parseQualifiedName(qualifiedName, prefix, localName, ec))
        return 0;

    QualifiedName qName(prefix, localName, namespaceURI);
    if (!shouldIgnoreNamespaceChecks && hasPrefixNamespaceMismatch(qName)) {
        ec = NAMESPACE_ERR;
        return 0;
    }

    // Spec: DOM Level 2 Core: http://www.w3.org/TR/DOM-Level-2-Core/core.html#ID-DocCrAttrNS
    if (!shouldIgnoreNamespaceChecks && qName.localName() == xmlnsAtom && qName.namespaceURI() != XMLNSNames::xmlnsNamespaceURI) {
        ec = NAMESPACE_ERR;
        return 0;
    }

    // FIXME: Assume this is a mapped attribute, since createAttribute isn't namespace-aware.  There's no harm to XML
    // documents if we're wrong.
    return Attr::create(0, this, Attribute::createMapped(qName, StringImpl::empty()));
}

#if ENABLE(SVG)
const SVGDocumentExtensions* Document::svgExtensions()
{
    return m_svgExtensions.get();
}

SVGDocumentExtensions* Document::accessSVGExtensions()
{
    if (!m_svgExtensions)
        m_svgExtensions = adoptPtr(new SVGDocumentExtensions(this));
    return m_svgExtensions.get();
}

bool Document::hasSVGRootNode() const
{
    return documentElement() && documentElement()->hasTagName(SVGNames::svgTag);
}
#endif

PassRefPtr<HTMLCollection> Document::images()
{
    return HTMLCollection::create(this, DocImages);
}

PassRefPtr<HTMLCollection> Document::applets()
{
    return HTMLCollection::create(this, DocApplets);
}

PassRefPtr<HTMLCollection> Document::embeds()
{
    return HTMLCollection::create(this, DocEmbeds);
}

PassRefPtr<HTMLCollection> Document::plugins()
{
    // This is an alias for embeds() required for the JS DOM bindings.
    return HTMLCollection::create(this, DocEmbeds);
}

PassRefPtr<HTMLCollection> Document::objects()
{
    return HTMLCollection::create(this, DocObjects);
}

PassRefPtr<HTMLCollection> Document::scripts()
{
    return HTMLCollection::create(this, DocScripts);
}

PassRefPtr<HTMLCollection> Document::links()
{
    return HTMLCollection::create(this, DocLinks);
}

PassRefPtr<HTMLCollection> Document::forms()
{
    return HTMLCollection::create(this, DocForms);
}

PassRefPtr<HTMLCollection> Document::anchors()
{
    return HTMLCollection::create(this, DocAnchors);
}

PassRefPtr<HTMLAllCollection> Document::all()
{
    return HTMLAllCollection::create(this);
}

PassRefPtr<HTMLCollection> Document::windowNamedItems(const String &name)
{
    return HTMLNameCollection::create(this, WindowNamedItems, name);
}

PassRefPtr<HTMLCollection> Document::documentNamedItems(const String &name)
{
    return HTMLNameCollection::create(this, DocumentNamedItems, name);
}

CollectionCache* Document::nameCollectionInfo(CollectionType type, const AtomicString& name)
{
    ASSERT(type >= FirstNamedDocumentCachedType);
    unsigned index = type - FirstNamedDocumentCachedType;
    ASSERT(index < NumNamedDocumentCachedTypes);

    NamedCollectionMap& map = m_nameCollectionInfo[index];
    NamedCollectionMap::iterator iter = map.find(name.impl());
    if (iter == map.end())
        iter = map.add(name.impl(), new CollectionCache).first;
    iter->second->checkConsistency();
    return iter->second;
}

void Document::finishedParsing()
{
    ASSERT(!scriptableDocumentParser() || !m_parser->isParsing());
    ASSERT(!scriptableDocumentParser() || m_readyState != Loading);
    setParsing(false);
    if (!m_documentTiming.domContentLoadedEventStart)
        m_documentTiming.domContentLoadedEventStart = currentTime();
    dispatchEvent(Event::create(eventNames().DOMContentLoadedEvent, true, false));
    if (!m_documentTiming.domContentLoadedEventEnd)
        m_documentTiming.domContentLoadedEventEnd = currentTime();

    if (RefPtr<Frame> f = frame()) {
        // FrameLoader::finishedParsing() might end up calling Document::implicitClose() if all
        // resource loads are complete. HTMLObjectElements can start loading their resources from
        // post attach callbacks triggered by recalcStyle().  This means if we parse out an <object>
        // tag and then reach the end of the document without updating styles, we might not have yet
        // started the resource load and might fire the window load event too early.  To avoid this
        // we force the styles to be up to date before calling FrameLoader::finishedParsing().
        // See https://bugs.webkit.org/show_bug.cgi?id=36864 starting around comment 35.
        updateStyleIfNeeded();

        f->loader()->finishedParsing();

        InspectorInstrumentation::domContentLoadedEventFired(f.get(), url());
    }
}

Vector<String> Document::formElementsState() const
{
    Vector<String> stateVector;
    stateVector.reserveInitialCapacity(m_formElementsWithState.size() * 3);
    typedef FormElementListHashSet::const_iterator Iterator;
    Iterator end = m_formElementsWithState.end();
    for (Iterator it = m_formElementsWithState.begin(); it != end; ++it) {
        Element* elementWithState = *it;
        String value;
        if (!elementWithState->shouldSaveAndRestoreFormControlState())
            continue;
        if (!elementWithState->saveFormControlState(value))
            continue;
        stateVector.append(elementWithState->formControlName().string());
        stateVector.append(elementWithState->formControlType().string());
        stateVector.append(value);
    }
    return stateVector;
}

#if ENABLE(XPATH)

PassRefPtr<XPathExpression> Document::createExpression(const String& expression,
                                                       XPathNSResolver* resolver,
                                                       ExceptionCode& ec)
{
    if (!m_xpathEvaluator)
        m_xpathEvaluator = XPathEvaluator::create();
    return m_xpathEvaluator->createExpression(expression, resolver, ec);
}

PassRefPtr<XPathNSResolver> Document::createNSResolver(Node* nodeResolver)
{
    if (!m_xpathEvaluator)
        m_xpathEvaluator = XPathEvaluator::create();
    return m_xpathEvaluator->createNSResolver(nodeResolver);
}

PassRefPtr<XPathResult> Document::evaluate(const String& expression,
                                           Node* contextNode,
                                           XPathNSResolver* resolver,
                                           unsigned short type,
                                           XPathResult* result,
                                           ExceptionCode& ec)
{
    if (!m_xpathEvaluator)
        m_xpathEvaluator = XPathEvaluator::create();
    return m_xpathEvaluator->evaluate(expression, contextNode, resolver, type, result, ec);
}

#endif // ENABLE(XPATH)

void Document::setStateForNewFormElements(const Vector<String>& stateVector)
{
    // Walk the state vector backwards so that the value to use for each
    // name/type pair first is the one at the end of each individual vector
    // in the FormElementStateMap. We're using them like stacks.
    typedef FormElementStateMap::iterator Iterator;
    m_formElementsWithState.clear();
    for (size_t i = stateVector.size() / 3 * 3; i; i -= 3) {
        AtomicString a = stateVector[i - 3];
        AtomicString b = stateVector[i - 2];
        const String& c = stateVector[i - 1];
        FormElementKey key(a.impl(), b.impl());
        Iterator it = m_stateForNewFormElements.find(key);
        if (it != m_stateForNewFormElements.end())
            it->second.append(c);
        else {
            Vector<String> v(1);
            v[0] = c;
            m_stateForNewFormElements.set(key, v);
        }
    }
}

bool Document::hasStateForNewFormElements() const
{
    return !m_stateForNewFormElements.isEmpty();
}

bool Document::takeStateForFormElement(AtomicStringImpl* name, AtomicStringImpl* type, String& state)
{
    typedef FormElementStateMap::iterator Iterator;
    Iterator it = m_stateForNewFormElements.find(FormElementKey(name, type));
    if (it == m_stateForNewFormElements.end())
        return false;
    ASSERT(it->second.size());
    state = it->second.last();
    if (it->second.size() > 1)
        it->second.removeLast();
    else
        m_stateForNewFormElements.remove(it);
    return true;
}

FormElementKey::FormElementKey(AtomicStringImpl* name, AtomicStringImpl* type)
    : m_name(name), m_type(type)
{
    ref();
}

FormElementKey::~FormElementKey()
{
    deref();
}

FormElementKey::FormElementKey(const FormElementKey& other)
    : m_name(other.name()), m_type(other.type())
{
    ref();
}

FormElementKey& FormElementKey::operator=(const FormElementKey& other)
{
    other.ref();
    deref();
    m_name = other.name();
    m_type = other.type();
    return *this;
}

void FormElementKey::ref() const
{
    if (name())
        name()->ref();
    if (type())
        type()->ref();
}

void FormElementKey::deref() const
{
    if (name())
        name()->deref();
    if (type())
        type()->deref();
}

unsigned FormElementKeyHash::hash(const FormElementKey& key)
{
    return StringHasher::hashMemory<sizeof(FormElementKey)>(&key);
}

IconURL Document::iconURL(IconType iconType) const
{
    return m_iconURLs[toIconIndex(iconType)];
}

void Document::setIconURL(const String& url, const String& mimeType, IconType iconType)
{
    // FIXME - <rdar://problem/4727645> - At some point in the future, we might actually honor the "mimeType"
    IconURL newURL(KURL(ParsedURLString, url), iconType);
    if (!iconURL(iconType).m_iconURL.isEmpty())
        setIconURL(newURL);
    else if (!mimeType.isEmpty())
        setIconURL(newURL);
    if (Frame* f = frame())
        f->loader()->setIconURL(newURL);
}

void Document::setIconURL(const IconURL& iconURL)
{
    m_iconURLs[toIconIndex(iconURL.m_iconType)] = iconURL;
}

void Document::registerFormElementWithFormAttribute(FormAssociatedElement* element)
{
    ASSERT(toHTMLElement(element)->fastHasAttribute(formAttr));
    m_formElementsWithFormAttribute.add(element);
}

void Document::unregisterFormElementWithFormAttribute(FormAssociatedElement* element)
{
    m_formElementsWithFormAttribute.remove(element);
}

void Document::resetFormElementsOwner(HTMLFormElement* form)
{
    typedef FormAssociatedElementListHashSet::iterator Iterator;
    Iterator end = m_formElementsWithFormAttribute.end();
    for (Iterator it = m_formElementsWithFormAttribute.begin(); it != end; ++it)
        (*it)->resetFormOwner(form);
}

void Document::setUseSecureKeyboardEntryWhenActive(bool usesSecureKeyboard)
{
    if (m_useSecureKeyboardEntryWhenActive == usesSecureKeyboard)
        return;
        
    m_useSecureKeyboardEntryWhenActive = usesSecureKeyboard;
    m_frame->selection()->updateSecureKeyboardEntryIfActive();
}

bool Document::useSecureKeyboardEntryWhenActive() const
{
    return m_useSecureKeyboardEntryWhenActive;
}

void Document::initSecurityContext()
{
    if (securityOrigin() && !securityOrigin()->isEmpty())
        return; // m_securityOrigin has already been initialized.

    if (!m_frame) {
        // No source for a security context.
        // This can occur via document.implementation.createDocument().
        m_cookieURL = KURL(ParsedURLString, "");
        ScriptExecutionContext::setSecurityOrigin(SecurityOrigin::createEmpty());
        m_contentSecurityPolicy = ContentSecurityPolicy::create(this);
        return;
    }

    // In the common case, create the security context from the currently
    // loading URL with a fresh content security policy.
    m_cookieURL = m_url;
    ScriptExecutionContext::setSecurityOrigin(SecurityOrigin::create(m_url, m_frame->loader()->sandboxFlags()));
    m_contentSecurityPolicy = ContentSecurityPolicy::create(this);

    if (SecurityOrigin::allowSubstituteDataAccessToLocal()) {
        // If this document was loaded with substituteData, then the document can
        // load local resources.  See https://bugs.webkit.org/show_bug.cgi?id=16756
        // and https://bugs.webkit.org/show_bug.cgi?id=19760 for further
        // discussion.
        if (m_documentLoader->substituteData().isValid())
            securityOrigin()->grantLoadLocalResources();
    }

    if (Settings* settings = this->settings()) {
        if (!settings->isWebSecurityEnabled()) {
          // Web security is turned off.  We should let this document access every
          // other document.  This is used primary by testing harnesses for web
          // sites.
          securityOrigin()->grantUniversalAccess();

        } else if (settings->allowUniversalAccessFromFileURLs() && securityOrigin()->isLocal()) {
          // Some clients want file:// URLs to have universal access, but that
          // setting is dangerous for other clients.
          securityOrigin()->grantUniversalAccess();
        } else if (!settings->allowFileAccessFromFileURLs() && securityOrigin()->isLocal()) {
          // Some clients want file:// URLs to have even tighter restrictions by
          // default, and not be able to access other local files.
          securityOrigin()->enforceFilePathSeparation();
        }
    }

    if (!securityOrigin()->isEmpty())
        return;

    // If we do not obtain a meaningful origin from the URL, then we try to
    // find one via the frame hierarchy.

    Frame* ownerFrame = m_frame->tree()->parent();
    if (!ownerFrame)
        ownerFrame = m_frame->loader()->opener();

    if (ownerFrame) {
        m_cookieURL = ownerFrame->document()->cookieURL();
        // We alias the SecurityOrigins to match Firefox, see Bug 15313
        // https://bugs.webkit.org/show_bug.cgi?id=15313
        ScriptExecutionContext::setSecurityOrigin(ownerFrame->document()->securityOrigin());
        // FIXME: Consider moving m_contentSecurityPolicy into SecurityOrigin.
        m_contentSecurityPolicy = ownerFrame->document()->contentSecurityPolicy();
    }
}

void Document::setSecurityOrigin(SecurityOrigin* securityOrigin)
{
    ScriptExecutionContext::setSecurityOrigin(securityOrigin);
    // FIXME: Find a better place to enable DNS prefetch, which is a loader concept,
    // not applicable to arbitrary documents.
    initDNSPrefetch();
}

#if ENABLE(DATABASE)

bool Document::allowDatabaseAccess() const
{
    if (!page() || page()->settings()->privateBrowsingEnabled())
        return false;
    return true;
}

void Document::databaseExceededQuota(const String& name)
{
    Page* currentPage = page();
    if (currentPage)
        currentPage->chrome()->client()->exceededDatabaseQuota(document()->frame(), name);
}

#endif

bool Document::isContextThread() const
{
    return isMainThread();
}

void Document::updateURLForPushOrReplaceState(const KURL& url)
{
    Frame* f = frame();
    if (!f)
        return;

    setURL(url);
    f->loader()->setOutgoingReferrer(url);
    m_documentLoader->replaceRequestURLForSameDocumentNavigation(url);
}

void Document::statePopped(SerializedScriptValue* stateObject)
{
    if (!frame())
        return;
    
    // Per step 11 of section 6.5.9 (history traversal) of the HTML5 spec, we 
    // defer firing of popstate until we're in the complete state.
    if (m_readyState == Complete)
        enqueuePopstateEvent(stateObject);
    else
        m_pendingStateObject = stateObject;
}

void Document::updateFocusAppearanceSoon(bool restorePreviousSelection)
{
    m_updateFocusAppearanceRestoresSelection = restorePreviousSelection;
    if (!m_updateFocusAppearanceTimer.isActive())
        m_updateFocusAppearanceTimer.startOneShot(0);
}

void Document::cancelFocusAppearanceUpdate()
{
    m_updateFocusAppearanceTimer.stop();
}

void Document::updateFocusAppearanceTimerFired(Timer<Document>*)
{
    Node* node = focusedNode();
    if (!node)
        return;
    if (!node->isElementNode())
        return;

    updateLayout();

    Element* element = static_cast<Element*>(node);
    if (element->isFocusable())
        element->updateFocusAppearance(m_updateFocusAppearanceRestoresSelection);
}

// FF method for accessing the selection added for compatibility.
DOMSelection* Document::getSelection() const
{
    return frame() ? frame()->domWindow()->getSelection() : 0;
}

void Document::attachRange(Range* range)
{
    ASSERT(!m_ranges.contains(range));
    m_ranges.add(range);
}

void Document::detachRange(Range* range)
{
    // We don't ASSERT m_ranges.contains(range) to allow us to call this
    // unconditionally to fix: https://bugs.webkit.org/show_bug.cgi?id=26044
    m_ranges.remove(range);
}

CanvasRenderingContext* Document::getCSSCanvasContext(const String& type, const String& name, int width, int height)
{
    HTMLCanvasElement* result = getCSSCanvasElement(name);
    if (!result)
        return 0;
    result->setSize(IntSize(width, height));
    return result->getContext(type);
}

HTMLCanvasElement* Document::getCSSCanvasElement(const String& name)
{
    RefPtr<HTMLCanvasElement> result = m_cssCanvasElements.get(name).get();
    if (!result) {
        result = HTMLCanvasElement::create(this);
        m_cssCanvasElements.set(name, result);
    }
    return result.get();
}

void Document::initDNSPrefetch()
{
    Settings* settings = this->settings();

    m_haveExplicitlyDisabledDNSPrefetch = false;
    m_isDNSPrefetchEnabled = settings && settings->dnsPrefetchingEnabled() && securityOrigin()->protocol() == "http";

    // Inherit DNS prefetch opt-out from parent frame    
    if (Document* parent = parentDocument()) {
        if (!parent->isDNSPrefetchEnabled())
            m_isDNSPrefetchEnabled = false;
    }
}

void Document::parseDNSPrefetchControlHeader(const String& dnsPrefetchControl)
{
    if (equalIgnoringCase(dnsPrefetchControl, "on") && !m_haveExplicitlyDisabledDNSPrefetch) {
        m_isDNSPrefetchEnabled = true;
        return;
    }

    m_isDNSPrefetchEnabled = false;
    m_haveExplicitlyDisabledDNSPrefetch = true;
}

void Document::addMessage(MessageSource source, MessageType type, MessageLevel level, const String& message, unsigned lineNumber, const String& sourceURL, PassRefPtr<ScriptCallStack> callStack)
{
    if (DOMWindow* window = domWindow())
        window->console()->addMessage(source, type, level, message, lineNumber, sourceURL, callStack);
}

struct PerformTaskContext {
    WTF_MAKE_NONCOPYABLE(PerformTaskContext); WTF_MAKE_FAST_ALLOCATED;
public:
    PerformTaskContext(PassRefPtr<DocumentWeakReference> documentReference, PassOwnPtr<ScriptExecutionContext::Task> task)
        : documentReference(documentReference)
        , task(task)
    {
    }

    RefPtr<DocumentWeakReference> documentReference;
    OwnPtr<ScriptExecutionContext::Task> task;
};

static void performTask(void* ctx)
{
    ASSERT(isMainThread());

    PerformTaskContext* context = reinterpret_cast<PerformTaskContext*>(ctx);
    ASSERT(context);

    if (Document* document = context->documentReference->document())
        context->task->performTask(document);

    delete context;
}

void Document::postTask(PassOwnPtr<Task> task)
{
    callOnMainThread(performTask, new PerformTaskContext(m_weakReference, task));
}

void Document::suspendScriptedAnimationControllerCallbacks()
{
#if ENABLE(REQUEST_ANIMATION_FRAME)
    if (m_scriptedAnimationController)
        m_scriptedAnimationController->suspend();
#endif
}

void Document::resumeScriptedAnimationControllerCallbacks()
{
#if ENABLE(REQUEST_ANIMATION_FRAME)
    if (m_scriptedAnimationController)
        m_scriptedAnimationController->resume();
#endif
}

String Document::displayStringModifiedByEncoding(const String& str) const
{
    if (m_decoder)
        return m_decoder->encoding().displayString(str.impl());
    return str;
}

PassRefPtr<StringImpl> Document::displayStringModifiedByEncoding(PassRefPtr<StringImpl> str) const
{
    if (m_decoder)
        return m_decoder->encoding().displayString(str);
    return str;
}

void Document::displayBufferModifiedByEncoding(UChar* buffer, unsigned len) const
{
    if (m_decoder)
        m_decoder->encoding().displayBuffer(buffer, len);
}

void Document::enqueuePageshowEvent(PageshowEventPersistence persisted)
{
    // FIXME: https://bugs.webkit.org/show_bug.cgi?id=36334 Pageshow event needs to fire asynchronously.
    dispatchWindowEvent(PageTransitionEvent::create(eventNames().pageshowEvent, persisted), this);
}

void Document::enqueueHashchangeEvent(const String& oldURL, const String& newURL)
{
    enqueueWindowEvent(HashChangeEvent::create(oldURL, newURL));
}

void Document::enqueuePopstateEvent(PassRefPtr<SerializedScriptValue> stateObject)
{
    // FIXME: https://bugs.webkit.org/show_bug.cgi?id=36202 Popstate event needs to fire asynchronously
    dispatchWindowEvent(PopStateEvent::create(stateObject));
}

void Document::addMediaCanStartListener(MediaCanStartListener* listener)
{
    ASSERT(!m_mediaCanStartListeners.contains(listener));
    m_mediaCanStartListeners.add(listener);
}

void Document::removeMediaCanStartListener(MediaCanStartListener* listener)
{
    ASSERT(m_mediaCanStartListeners.contains(listener));
    m_mediaCanStartListeners.remove(listener);
}

MediaCanStartListener* Document::takeAnyMediaCanStartListener()
{
    HashSet<MediaCanStartListener*>::iterator slot = m_mediaCanStartListeners.begin();
    if (slot == m_mediaCanStartListeners.end())
        return 0;
    MediaCanStartListener* listener = *slot;
    m_mediaCanStartListeners.remove(slot);
    return listener;
}

#if ENABLE(XHTMLMP)
bool Document::isXHTMLMPDocument() const
{
    if (!frame() || !frame()->loader())
        return false;
    // As per section 7.2 of OMA-WAP-XHTMLMP-V1_1-20061020-A.pdf, a conforming user agent
    // MUST accept XHTMLMP document identified as "application/vnd.wap.xhtml+xml"
    // and SHOULD accept it identified as "application/xhtml+xml" , "application/xhtml+xml" is a 
    // general MIME type for all XHTML documents, not only for XHTMLMP
    return loader()->writer()->mimeType() == "application/vnd.wap.xhtml+xml";
}
#endif

#if ENABLE(FULLSCREEN_API)
bool Document::fullScreenIsAllowedForElement(Element* element) const
{
    ASSERT(element);
    while (HTMLFrameOwnerElement* ownerElement = element->document()->ownerElement()) {
        if (!ownerElement->isFrameElementBase())
            continue;

        if (!static_cast<HTMLFrameElementBase*>(ownerElement)->allowFullScreen())
            return false;
        element = ownerElement;
    }
    return true;
}

void Document::requestFullScreenForElement(Element* element, unsigned short flags, FullScreenCheckType checkType)
{
    if (!page() || !page()->settings()->fullScreenEnabled())
        return;

    if (!element)
        element = documentElement();
    
    if (checkType == EnforceIFrameAllowFulScreenRequirement && !fullScreenIsAllowedForElement(element))
        return;
    
    if (!ScriptController::processingUserGesture())
        return;
    
    if (!page()->chrome()->client()->supportsFullScreenForElement(element, flags & Element::ALLOW_KEYBOARD_INPUT))
        return;
    
    m_areKeysEnabledInFullScreen = flags & Element::ALLOW_KEYBOARD_INPUT;
    page()->chrome()->client()->enterFullScreenForElement(element);
}

void Document::webkitCancelFullScreen()
{
    if (!page() || !m_fullScreenElement)
        return;
    
    page()->chrome()->client()->exitFullScreenForElement(m_fullScreenElement.get());
}

static void setContainsFullScreenElementRecursively(Element* element, bool contains)
{
    if (!element)
        return;

    do {
        if (!element->isFrameElementBase())
            continue;
        
        static_cast<HTMLFrameElementBase*>(element)->setContainsFullScreenElement(contains);
    } while ((element = element->document()->ownerElement()));
}

void Document::webkitWillEnterFullScreenForElement(Element* element)
{
    ASSERT(element);
    ASSERT(page() && page()->settings()->fullScreenEnabled());

    m_fullScreenElement = element;
    
    if (m_fullScreenElement != documentElement())
        m_fullScreenElement->detach();

    setContainsFullScreenElementRecursively(ownerElement(), true);
    
    recalcStyle(Force);
    
    if (m_fullScreenRenderer) {
        m_fullScreenRenderer->setAnimating(true);
#if USE(ACCELERATED_COMPOSITING)
        view()->updateCompositingLayers();
        if (m_fullScreenRenderer->layer()->isComposited())
            page()->chrome()->client()->setRootFullScreenLayer(m_fullScreenRenderer->layer()->backing()->graphicsLayer());
#endif
    }
}
    
void Document::webkitDidEnterFullScreenForElement(Element*)
{
    if (m_fullScreenRenderer) {
#if USE(ACCELERATED_COMPOSITING)
        page()->chrome()->client()->setRootFullScreenLayer(0);
#endif
        m_fullScreenRenderer->setAnimating(false);
#if USE(ACCELERATED_COMPOSITING)
        view()->updateCompositingLayers();
#endif
    }
    m_fullScreenChangeEventTargetQueue.append(m_fullScreenElement);
    m_fullScreenChangeDelayTimer.startOneShot(0);
}

void Document::webkitWillExitFullScreenForElement(Element*)
{
    setContainsFullScreenElementRecursively(ownerElement(), false);
    
    if (m_fullScreenRenderer) {
        m_fullScreenRenderer->setAnimating(true);
#if USE(ACCELERATED_COMPOSITING)
        view()->updateCompositingLayers();
        if (m_fullScreenRenderer->layer()->isComposited())
            page()->chrome()->client()->setRootFullScreenLayer(m_fullScreenRenderer->layer()->backing()->graphicsLayer());
#endif
    }
}

void Document::webkitDidExitFullScreenForElement(Element*)
{
    m_areKeysEnabledInFullScreen = false;

    if (m_fullScreenRenderer)
        m_fullScreenRenderer->remove();
    
    if (m_fullScreenElement != documentElement())
        m_fullScreenElement->detach();

    m_fullScreenChangeEventTargetQueue.append(m_fullScreenElement.release());
    setFullScreenRenderer(0);
#if USE(ACCELERATED_COMPOSITING)
    page()->chrome()->client()->setRootFullScreenLayer(0);
#endif
    recalcStyle(Force);
    
    m_fullScreenChangeDelayTimer.startOneShot(0);
}
    
void Document::setFullScreenRenderer(RenderFullScreen* renderer)
{
    if (renderer == m_fullScreenRenderer)
        return;

    if (m_fullScreenRenderer)
        m_fullScreenRenderer->destroy();
    m_fullScreenRenderer = renderer;
    
    // This notification can come in after the page has been destroyed.
    if (page())
        page()->chrome()->client()->fullScreenRendererChanged(m_fullScreenRenderer);
}
    
void Document::setFullScreenRendererSize(const IntSize& size)
{
    ASSERT(m_fullScreenRenderer);
    if (!m_fullScreenRenderer)
        return;
    
    if (m_fullScreenRenderer) {
        RefPtr<RenderStyle> newStyle = RenderStyle::clone(m_fullScreenRenderer->style());
        newStyle->setWidth(Length(size.width(), WebCore::Fixed));
        newStyle->setHeight(Length(size.height(), WebCore::Fixed));
        newStyle->setTop(Length(0, WebCore::Fixed));
        newStyle->setLeft(Length(0, WebCore::Fixed));
        m_fullScreenRenderer->setStyle(newStyle);
        updateLayout();
    }
}
    
void Document::setFullScreenRendererBackgroundColor(Color backgroundColor)
{
    if (!m_fullScreenRenderer)
        return;
    
    RefPtr<RenderStyle> newStyle = RenderStyle::clone(m_fullScreenRenderer->style());
    newStyle->setBackgroundColor(backgroundColor);
    m_fullScreenRenderer->setStyle(newStyle);
}
    
void Document::fullScreenChangeDelayTimerFired(Timer<Document>*)
{
    while (!m_fullScreenChangeEventTargetQueue.isEmpty()) {
        RefPtr<Element> element = m_fullScreenChangeEventTargetQueue.takeFirst();
        if (!element)
            element = documentElement();
        
        element->dispatchEvent(Event::create(eventNames().webkitfullscreenchangeEvent, true, false));
    }
}

void Document::fullScreenElementRemoved()
{
    // If the current full screen element or any of its ancestors is removed, set the current
    // full screen element to the document root, and fire a fullscreenchange event to inform 
    // clients of the DOM.
    if (m_fullScreenRenderer)
        m_fullScreenRenderer->remove();
    setFullScreenRenderer(0);

    m_fullScreenChangeEventTargetQueue.append(m_fullScreenElement.release());
    m_fullScreenElement = documentElement();
    recalcStyle(Force);
    
    // Dispatch this event manually, before the element is actually removed from the DOM
    // so that the message cascades as expected.
    fullScreenChangeDelayTimerFired(&m_fullScreenChangeDelayTimer);
    m_fullScreenChangeDelayTimer.stop();
}

void Document::removeFullScreenElementOfSubtree(Node* node, bool amongChildrenOnly)
{
    if (!m_fullScreenElement)
        return;
    
    bool elementInSubtree = false;
    if (amongChildrenOnly)
        elementInSubtree = m_fullScreenElement->isDescendantOf(node);
    else
        elementInSubtree = (m_fullScreenElement == node) || m_fullScreenElement->isDescendantOf(node);
    
    if (elementInSubtree)
        fullScreenElementRemoved();
}
#endif

void Document::decrementLoadEventDelayCount()
{
    ASSERT(m_loadEventDelayCount);
    --m_loadEventDelayCount;

    if (frame() && !m_loadEventDelayCount && !m_loadEventDelayTimer.isActive())
        m_loadEventDelayTimer.startOneShot(0);
}

void Document::loadEventDelayTimerFired(Timer<Document>*)
{
    if (frame())
        frame()->loader()->checkCompleted();
}

#if ENABLE(REQUEST_ANIMATION_FRAME)
int Document::webkitRequestAnimationFrame(PassRefPtr<RequestAnimationFrameCallback> callback, Element* animationElement)
{
    if (!m_scriptedAnimationController)
        m_scriptedAnimationController = ScriptedAnimationController::create(this);

    return m_scriptedAnimationController->registerCallback(callback, animationElement);
}

void Document::webkitCancelRequestAnimationFrame(int id)
{
    if (!m_scriptedAnimationController)
        return;
    m_scriptedAnimationController->cancelCallback(id);
}

void Document::serviceScriptedAnimations(DOMTimeStamp time)
{
    if (!m_scriptedAnimationController)
        return;
    m_scriptedAnimationController->serviceScriptedAnimations(time);
}
#endif

#if ENABLE(TOUCH_EVENTS)
PassRefPtr<Touch> Document::createTouch(DOMWindow* window, EventTarget* target, int identifier, int pageX, int pageY, int screenX, int screenY, ExceptionCode&) const
{
    // FIXME: It's not clear from the documentation at
    // http://developer.apple.com/library/safari/#documentation/UserExperience/Reference/DocumentAdditionsReference/DocumentAdditions/DocumentAdditions.html
    // when this method should throw and nor is it by inspection of iOS behavior. It would be nice to verify any cases where it throws under iOS
    // and implement them here. See https://bugs.webkit.org/show_bug.cgi?id=47819
    // Ditto for the createTouchList method below.
    Frame* frame = window ? window->frame() : this->frame();
    return Touch::create(frame, target, identifier, screenX, screenY, pageX, pageY);
}

PassRefPtr<TouchList> Document::createTouchList(ExceptionCode&) const
{
    return TouchList::create();
}
#endif

} // namespace WebCore
