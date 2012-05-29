/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
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
 *
 */

#ifndef Document_h
#define Document_h

#include "CheckedRadioButtons.h"
#include "CollectionCache.h"
#include "CollectionType.h"
#include "Color.h"
#include "DOMTimeStamp.h"
#include "DocumentTiming.h"
#include "IconURL.h"
#include "QualifiedName.h"
#include "ScriptExecutionContext.h"
#include "StringWithDirection.h"
#include "Timer.h"
#include "TreeScope.h"
#include "ViewportArguments.h"
#include <wtf/Deque.h>
#include <wtf/FixedArray.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {

class AXObjectCache;
class Attr;
class CDATASection;
class CSSPrimitiveValueCache;
class CSSStyleDeclaration;
class CSSStyleSelector;
class CSSStyleSheet;
class CachedCSSStyleSheet;
class CachedResourceLoader;
class CachedScript;
class CanvasRenderingContext;
class CharacterData;
class Comment;
class ContentSecurityPolicy;
class DOMImplementation;
class DOMSelection;
class DOMWindow;
class Database;
class DatabaseThread;
class DocumentFragment;
class DocumentLoader;
class DocumentMarkerController;
class DocumentType;
class DocumentWeakReference;
class EditingText;
class Element;
class EntityReference;
class Event;
class EventListener;
class EventQueue;
class FormAssociatedElement;
class Frame;
class FrameView;
class HTMLCanvasElement;
class HTMLCollection;
class HTMLAllCollection;
class HTMLDocument;
class HTMLElement;
class HTMLFormElement;
class HTMLFrameOwnerElement;
class HTMLHeadElement;
class HTMLInputElement;
class HTMLMapElement;
class HitTestRequest;
class HitTestResult;
class IntPoint;
class DOMWrapperWorld;
class JSNode;
class MediaCanStartListener;
class MediaQueryList;
class MediaQueryMatcher;
class MouseEventWithHitTestResults;
class NodeFilter;
class NodeIterator;
class Page;
class PlatformMouseEvent;
class ProcessingInstruction;
class Range;
class RegisteredEventListener;
class RenderArena;
class RenderView;
class RenderFullScreen;
class ScriptableDocumentParser;
class ScriptElementData;
class ScriptRunner;
class SecurityOrigin;
class SerializedScriptValue;
class SegmentedString;
class Settings;
class StyleSheet;
class StyleSheetList;
class Text;
class TextResourceDecoder;
class DocumentParser;
class TreeWalker;
class XMLHttpRequest;

#if ENABLE(SVG)
class SVGDocumentExtensions;
#endif

#if ENABLE(XSLT)
class TransformSource;
#endif

#if ENABLE(XPATH)
class XPathEvaluator;
class XPathExpression;
class XPathNSResolver;
class XPathResult;
#endif

#if ENABLE(DASHBOARD_SUPPORT)
struct DashboardRegionValue;
#endif

#if ENABLE(TOUCH_EVENTS)
class Touch;
class TouchList;
#endif

#if ENABLE(REQUEST_ANIMATION_FRAME)
class RequestAnimationFrameCallback;
class ScriptedAnimationController;
#endif

typedef int ExceptionCode;

class FormElementKey {
public:
    FormElementKey(AtomicStringImpl* = 0, AtomicStringImpl* = 0);
    ~FormElementKey();
    FormElementKey(const FormElementKey&);
    FormElementKey& operator=(const FormElementKey&);

    AtomicStringImpl* name() const { return m_name; }
    AtomicStringImpl* type() const { return m_type; }

    // Hash table deleted values, which are only constructed and never copied or destroyed.
    FormElementKey(WTF::HashTableDeletedValueType) : m_name(hashTableDeletedValue()) { }
    bool isHashTableDeletedValue() const { return m_name == hashTableDeletedValue(); }

private:
    void ref() const;
    void deref() const;

    static AtomicStringImpl* hashTableDeletedValue() { return reinterpret_cast<AtomicStringImpl*>(-1); }

    AtomicStringImpl* m_name;
    AtomicStringImpl* m_type;
};

inline bool operator==(const FormElementKey& a, const FormElementKey& b)
{
    return a.name() == b.name() && a.type() == b.type();
}

struct FormElementKeyHash {
    static unsigned hash(const FormElementKey&);
    static bool equal(const FormElementKey& a, const FormElementKey& b) { return a == b; }
    static const bool safeToCompareToEmptyOrDeleted = true;
};

struct FormElementKeyHashTraits : WTF::GenericHashTraits<FormElementKey> {
    static void constructDeletedValue(FormElementKey& slot) { new (&slot) FormElementKey(WTF::HashTableDeletedValue); }
    static bool isDeletedValue(const FormElementKey& value) { return value.isHashTableDeletedValue(); }
};

enum PageshowEventPersistence {
    PageshowEventNotPersisted = 0,
    PageshowEventPersisted = 1
};

enum StyleSelectorUpdateFlag { RecalcStyleImmediately, DeferRecalcStyle };

class Document : public TreeScope, public ScriptExecutionContext {
public:
    static PassRefPtr<Document> create(Frame* frame, const KURL& url)
    {
        return adoptRef(new Document(frame, url, false, false));
    }
    static PassRefPtr<Document> createXHTML(Frame* frame, const KURL& url)
    {
        return adoptRef(new Document(frame, url, true, false));
    }
    virtual ~Document();

    MediaQueryMatcher* mediaQueryMatcher();

    using TreeScope::ref;
    using TreeScope::deref;

    // Nodes belonging to this document hold guard references -
    // these are enough to keep the document from being destroyed, but
    // not enough to keep it from removing its children. This allows a
    // node that outlives its document to still have a valid document
    // pointer without introducing reference cycles.
    void guardRef()
    {
        ASSERT(!m_deletionHasBegun);
        ++m_guardRefCount;
    }

    void guardDeref()
    {
        ASSERT(!m_deletionHasBegun);
        --m_guardRefCount;
        if (!m_guardRefCount && !refCount()) {
#ifndef NDEBUG
            m_deletionHasBegun = true;
#endif
            delete this;
        }
    }

    virtual void removedLastRef();

    Element* getElementById(const AtomicString& id) const;

    Element* getElementByAccessKey(const String& key);
    void invalidateAccessKeyMap();

    // DOM methods & attributes for Document

    DEFINE_ATTRIBUTE_EVENT_LISTENER(abort);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(change);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(click);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(contextmenu);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(dblclick);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(dragenter);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(dragover);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(dragleave);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(drop);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(dragstart);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(drag);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(dragend);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(input);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(invalid);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(keydown);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(keypress);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(keyup);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(mousedown);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(mousemove);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(mouseout);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(mouseover);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(mouseup);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(mousewheel);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(scroll);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(select);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(submit);

    DEFINE_ATTRIBUTE_EVENT_LISTENER(blur);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(error);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(focus);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(load);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(readystatechange);

    // WebKit extensions
    DEFINE_ATTRIBUTE_EVENT_LISTENER(beforecut);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(cut);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(beforecopy);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(copy);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(beforepaste);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(paste);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(reset);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(search);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(selectstart);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(selectionchange);
#if ENABLE(TOUCH_EVENTS)
    DEFINE_ATTRIBUTE_EVENT_LISTENER(touchstart);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(touchmove);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(touchend);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(touchcancel);
#endif
#if ENABLE(FULLSCREEN_API)
    DEFINE_ATTRIBUTE_EVENT_LISTENER(webkitfullscreenchange);
#endif

    ViewportArguments viewportArguments() const { return m_viewportArguments; }

    DocumentType* doctype() const { return m_docType.get(); }

    DOMImplementation* implementation();
    
    Element* documentElement() const
    {
        if (!m_documentElement)
            cacheDocumentElement();
        return m_documentElement.get();
    }
    
    virtual PassRefPtr<Element> createElement(const AtomicString& tagName, ExceptionCode&);
    PassRefPtr<DocumentFragment> createDocumentFragment();
    PassRefPtr<Text> createTextNode(const String& data);
    PassRefPtr<Comment> createComment(const String& data);
    PassRefPtr<CDATASection> createCDATASection(const String& data, ExceptionCode&);
    PassRefPtr<ProcessingInstruction> createProcessingInstruction(const String& target, const String& data, ExceptionCode&);
    PassRefPtr<Attr> createAttribute(const String& name, ExceptionCode&);
    PassRefPtr<Attr> createAttributeNS(const String& namespaceURI, const String& qualifiedName, ExceptionCode&, bool shouldIgnoreNamespaceChecks = false);
    PassRefPtr<EntityReference> createEntityReference(const String& name, ExceptionCode&);
    PassRefPtr<Node> importNode(Node* importedNode, bool deep, ExceptionCode&);
    virtual PassRefPtr<Element> createElementNS(const String& namespaceURI, const String& qualifiedName, ExceptionCode&);
    PassRefPtr<Element> createElement(const QualifiedName&, bool createdByParser);

    /**
     * Retrieve all nodes that intersect a rect in the window's document, until it is fully enclosed by
     * the boundaries of a node.
     *
     * @param centerX x reference for the rectangle in CSS pixels
     * @param centerY y reference for the rectangle in CSS pixels
     * @param topPadding How much to expand the top of the rectangle
     * @param rightPadding How much to expand the right of the rectangle
     * @param bottomPadding How much to expand the bottom of the rectangle
     * @param leftPadding How much to expand the left of the rectangle
     * @param ignoreClipping whether or not to ignore the root scroll frame when retrieving the element.
     *        If false, this method returns null for coordinates outside of the viewport.
     */
    PassRefPtr<NodeList> nodesFromRect(int centerX, int centerY, unsigned topPadding, unsigned rightPadding,
                                       unsigned bottomPadding, unsigned leftPadding, bool ignoreClipping) const;
    Element* elementFromPoint(int x, int y) const;
    PassRefPtr<Range> caretRangeFromPoint(int x, int y);

    String readyState() const;

    String defaultCharset() const;
    
    String inputEncoding() const { return Document::encoding(); }
    String charset() const { return Document::encoding(); }
    String characterSet() const { return Document::encoding(); }

    void setCharset(const String&);

    void setContent(const String&);

    String suggestedMIMEType() const;

    String contentLanguage() const { return m_contentLanguage; }
    void setContentLanguage(const String& lang) { m_contentLanguage = lang; }

    String xmlEncoding() const { return m_xmlEncoding; }
    String xmlVersion() const { return m_xmlVersion; }
    bool xmlStandalone() const { return m_xmlStandalone; }

    void setXMLEncoding(const String& encoding) { m_xmlEncoding = encoding; } // read-only property, only to be set from XMLDocumentParser
    void setXMLVersion(const String&, ExceptionCode&);
    void setXMLStandalone(bool, ExceptionCode&);

    String documentURI() const { return m_documentURI; }
    void setDocumentURI(const String&);

    virtual KURL baseURI() const;

    PassRefPtr<Node> adoptNode(PassRefPtr<Node> source, ExceptionCode&);

    PassRefPtr<HTMLCollection> images();
    PassRefPtr<HTMLCollection> embeds();
    PassRefPtr<HTMLCollection> plugins(); // an alias for embeds() required for the JS DOM bindings.
    PassRefPtr<HTMLCollection> applets();
    PassRefPtr<HTMLCollection> links();
    PassRefPtr<HTMLCollection> forms();
    PassRefPtr<HTMLCollection> anchors();
    PassRefPtr<HTMLCollection> objects();
    PassRefPtr<HTMLCollection> scripts();
    PassRefPtr<HTMLCollection> windowNamedItems(const String& name);
    PassRefPtr<HTMLCollection> documentNamedItems(const String& name);

    PassRefPtr<HTMLAllCollection> all();

    CollectionCache* collectionInfo(CollectionType type)
    {
        ASSERT(type >= FirstUnnamedDocumentCachedType);
        unsigned index = type - FirstUnnamedDocumentCachedType;
        ASSERT(index < NumUnnamedDocumentCachedTypes);
        m_collectionInfo[index].checkConsistency();
        return &m_collectionInfo[index]; 
    }

    CollectionCache* nameCollectionInfo(CollectionType, const AtomicString& name);

    // Other methods (not part of DOM)
    bool isHTMLDocument() const { return m_isHTML; }
    bool isXHTMLDocument() const { return m_isXHTML; }
    virtual bool isImageDocument() const { return false; }
#if ENABLE(SVG)
    virtual bool isSVGDocument() const { return false; }
    bool hasSVGRootNode() const;
#else
    static bool isSVGDocument() { return false; }
    static bool hasSVGRootNode() { return false; }
#endif
    virtual bool isPluginDocument() const { return false; }
    virtual bool isMediaDocument() const { return false; }
#if ENABLE(XHTMLMP)
    bool isXHTMLMPDocument() const; 
    bool shouldProcessNoscriptElement() const { return m_shouldProcessNoScriptElement; }
    void setShouldProcessNoscriptElement(bool shouldDo) { m_shouldProcessNoScriptElement = shouldDo; }
#endif
    virtual bool isFrameSet() const { return false; }
    
    PassRefPtr<CSSPrimitiveValueCache> cssPrimitiveValueCache() const;
    
    CSSStyleSelector* styleSelectorIfExists() const { return m_styleSelector.get(); }

    bool usesViewSourceStyles() const { return m_usesViewSourceStyles; }
    void setUsesViewSourceStyles(bool usesViewSourceStyles) { m_usesViewSourceStyles = usesViewSourceStyles; }

    bool sawElementsInKnownNamespaces() const { return m_sawElementsInKnownNamespaces; }

    CSSStyleSelector* styleSelector()
    { 
        if (!m_styleSelector)
            createStyleSelector();
        return m_styleSelector.get();
    }

    /**
     * Updates the pending sheet count and then calls updateStyleSelector.
     */
    void removePendingSheet();

    /**
     * This method returns true if all top-level stylesheets have loaded (including
     * any @imports that they may be loading).
     */
    bool haveStylesheetsLoaded() const
    {
        return m_pendingStylesheets <= 0 || m_ignorePendingStylesheets;
    }

    /**
     * Increments the number of pending sheets.  The <link> elements
     * invoke this to add themselves to the loading list.
     */
    void addPendingSheet() { m_pendingStylesheets++; }

    void addStyleSheetCandidateNode(Node*, bool createdByParser);
    void removeStyleSheetCandidateNode(Node*);

    bool gotoAnchorNeededAfterStylesheetsLoad() { return m_gotoAnchorNeededAfterStylesheetsLoad; }
    void setGotoAnchorNeededAfterStylesheetsLoad(bool b) { m_gotoAnchorNeededAfterStylesheetsLoad = b; }

    /**
     * Called when one or more stylesheets in the document may have been added, removed or changed.
     *
     * Creates a new style selector and assign it to this document. This is done by iterating through all nodes in
     * document (or those before <BODY> in a HTML document), searching for stylesheets. Stylesheets can be contained in
     * <LINK>, <STYLE> or <BODY> elements, as well as processing instructions (XML documents only). A list is
     * constructed from these which is used to create the a new style selector which collates all of the stylesheets
     * found and is used to calculate the derived styles for all rendering objects.
     */
    void styleSelectorChanged(StyleSelectorUpdateFlag);
    void recalcStyleSelector();

    bool usesSiblingRules() const { return m_usesSiblingRules || m_usesSiblingRulesOverride; }
    void setUsesSiblingRules(bool b) { m_usesSiblingRulesOverride = b; }
    bool usesFirstLineRules() const { return m_usesFirstLineRules; }
    bool usesFirstLetterRules() const { return m_usesFirstLetterRules; }
    void setUsesFirstLetterRules(bool b) { m_usesFirstLetterRules = b; }
    bool usesBeforeAfterRules() const { return m_usesBeforeAfterRules || m_usesBeforeAfterRulesOverride; }
    void setUsesBeforeAfterRules(bool b) { m_usesBeforeAfterRulesOverride = b; }
    bool usesRemUnits() const { return m_usesRemUnits; }
    void setUsesRemUnits(bool b) { m_usesRemUnits = b; }
    bool usesLinkRules() const { return linkColor() != visitedLinkColor() || m_usesLinkRules; }
    void setUsesLinkRules(bool b) { m_usesLinkRules = b; }

    // Machinery for saving and restoring state when you leave and then go back to a page.
    void registerFormElementWithState(Element* e) { m_formElementsWithState.add(e); }
    void unregisterFormElementWithState(Element* e) { m_formElementsWithState.remove(e); }
    Vector<String> formElementsState() const;
    void setStateForNewFormElements(const Vector<String>&);
    bool hasStateForNewFormElements() const;
    bool takeStateForFormElement(AtomicStringImpl* name, AtomicStringImpl* type, String& state);

    void registerFormElementWithFormAttribute(FormAssociatedElement*);
    void unregisterFormElementWithFormAttribute(FormAssociatedElement*);
    void resetFormElementsOwner(HTMLFormElement*);

    FrameView* view() const; // can be NULL
    Frame* frame() const { return m_frame; } // can be NULL
    Page* page() const; // can be NULL
    Settings* settings() const; // can be NULL

    PassRefPtr<Range> createRange();

    PassRefPtr<NodeIterator> createNodeIterator(Node* root, unsigned whatToShow,
        PassRefPtr<NodeFilter>, bool expandEntityReferences, ExceptionCode&);

    PassRefPtr<TreeWalker> createTreeWalker(Node* root, unsigned whatToShow, 
        PassRefPtr<NodeFilter>, bool expandEntityReferences, ExceptionCode&);

    // Special support for editing
    PassRefPtr<CSSStyleDeclaration> createCSSStyleDeclaration();
    PassRefPtr<EditingText> createEditingTextNode(const String&);

    virtual void recalcStyle(StyleChange = NoChange);
    bool childNeedsAndNotInStyleRecalc();
    virtual void updateStyleIfNeeded();
    void updateLayout();
    void updateLayoutIgnorePendingStylesheets();
    PassRefPtr<RenderStyle> styleForElementIgnoringPendingStylesheets(Element*);
    PassRefPtr<RenderStyle> styleForPage(int pageIndex);

    // Returns true if page box (margin boxes and page borders) is visible.
    bool isPageBoxVisible(int pageIndex);

    // Returns the preferred page size and margins in pixels, assuming 96
    // pixels per inch. pageSize, marginTop, marginRight, marginBottom,
    // marginLeft must be initialized to the default values that are used if
    // auto is specified.
    void pageSizeAndMarginsInPixels(int pageIndex, IntSize& pageSize, int& marginTop, int& marginRight, int& marginBottom, int& marginLeft);

    static void updateStyleForAllDocuments(); // FIXME: Try to reduce the # of calls to this function.
    CachedResourceLoader* cachedResourceLoader() { return m_cachedResourceLoader.get(); }

    virtual void attach();
    virtual void detach();

    RenderArena* renderArena() { return m_renderArena.get(); }

    RenderView* renderView() const;

    void clearAXObjectCache();
    AXObjectCache* axObjectCache() const;
    bool axObjectCacheExists() const;
    
    // to get visually ordered hebrew and arabic pages right
    void setVisuallyOrdered();
    bool visuallyOrdered() const { return m_visuallyOrdered; }
    
    void setDocumentLoader(DocumentLoader* documentLoader) { m_documentLoader = documentLoader; }
    DocumentLoader* loader() const { return m_documentLoader; }

    void open(Document* ownerDocument = 0);
    void implicitOpen();

    // close() is the DOM API document.close()
    void close();
    // In some situations (see the code), we ignore document.close().
    // explicitClose() bypass these checks and actually tries to close the
    // input stream.
    void explicitClose();
    // implicitClose() actually does the work of closing the input stream.
    void implicitClose();

    void cancelParsing();

    void write(const SegmentedString& text, Document* ownerDocument = 0);
    void write(const String& text, Document* ownerDocument = 0);
    void writeln(const String& text, Document* ownerDocument = 0);
    void finishParsing();

    bool wellFormed() const { return m_wellFormed; }

    const KURL& url() const { return m_url; }
    void setURL(const KURL&);

    const KURL& baseURL() const { return m_baseURL; }
    const String& baseTarget() const { return m_baseTarget; }
    void processBaseElement();

    KURL completeURL(const String&) const;

    virtual String userAgent(const KURL&) const;

    CSSStyleSheet* pageUserSheet();
    void clearPageUserSheet();
    void updatePageUserSheet();

    const Vector<RefPtr<CSSStyleSheet> >* pageGroupUserSheets() const;
    void clearPageGroupUserSheets();
    void updatePageGroupUserSheets();

    CSSStyleSheet* elementSheet();
    CSSStyleSheet* mappedElementSheet();
    
    virtual PassRefPtr<DocumentParser> createParser();
    DocumentParser* parser() const { return m_parser.get(); }
    ScriptableDocumentParser* scriptableDocumentParser() const;
    
    bool printing() const { return m_printing; }
    void setPrinting(bool p) { m_printing = p; }

    bool paginatedForScreen() const { return m_paginatedForScreen; }
    void setPaginatedForScreen(bool p) { m_paginatedForScreen = p; }
    
    bool paginated() const { return printing() || paginatedForScreen(); }

    enum CompatibilityMode { QuirksMode, LimitedQuirksMode, NoQuirksMode };

    virtual void setCompatibilityModeFromDoctype() { }
    void setCompatibilityMode(CompatibilityMode m);
    void lockCompatibilityMode() { m_compatibilityModeLocked = true; }
    CompatibilityMode compatibilityMode() const { return m_compatibilityMode; }

    String compatMode() const;

    bool inQuirksMode() const { return m_compatibilityMode == QuirksMode; }
    bool inLimitedQuirksMode() const { return m_compatibilityMode == LimitedQuirksMode; }
    bool inNoQuirksMode() const { return m_compatibilityMode == NoQuirksMode; }

    enum ReadyState {
        Loading,
        Interactive,
        Complete
    };
    void setReadyState(ReadyState);
    void setParsing(bool);
    bool parsing() const { return m_bParsing; }
    int minimumLayoutDelay();

    // This method is used by Android.
    void setExtraLayoutDelay(int delay) { m_extraLayoutDelay = delay; }

    bool shouldScheduleLayout();
    bool isLayoutTimerActive();
    int elapsedTime() const;
    
    void setTextColor(const Color& color) { m_textColor = color; }
    Color textColor() const { return m_textColor; }

    const Color& linkColor() const { return m_linkColor; }
    const Color& visitedLinkColor() const { return m_visitedLinkColor; }
    const Color& activeLinkColor() const { return m_activeLinkColor; }
    void setLinkColor(const Color& c) { m_linkColor = c; }
    void setVisitedLinkColor(const Color& c) { m_visitedLinkColor = c; }
    void setActiveLinkColor(const Color& c) { m_activeLinkColor = c; }
    void resetLinkColor();
    void resetVisitedLinkColor();
    void resetActiveLinkColor();
    
    MouseEventWithHitTestResults prepareMouseEvent(const HitTestRequest&, const IntPoint&, const PlatformMouseEvent&);

    StyleSheetList* styleSheets();

    /* Newly proposed CSS3 mechanism for selecting alternate
       stylesheets using the DOM. May be subject to change as
       spec matures. - dwh
    */
    String preferredStylesheetSet() const;
    String selectedStylesheetSet() const;
    void setSelectedStylesheetSet(const String&);

    bool setFocusedNode(PassRefPtr<Node>);
    Node* focusedNode() const { return m_focusedNode.get(); }

    void getFocusableNodes(Vector<RefPtr<Node> >&);
    
    // The m_ignoreAutofocus flag specifies whether or not the document has been changed by the user enough 
    // for WebCore to ignore the autofocus attribute on any form controls
    bool ignoreAutofocus() const { return m_ignoreAutofocus; };
    void setIgnoreAutofocus(bool shouldIgnore = true) { m_ignoreAutofocus = shouldIgnore; };

    void setHoverNode(PassRefPtr<Node>);
    Node* hoverNode() const { return m_hoverNode.get(); }

    void setActiveNode(PassRefPtr<Node>);
    Node* activeNode() const { return m_activeNode.get(); }

    void focusedNodeRemoved();
    void removeFocusedNodeOfSubtree(Node*, bool amongChildrenOnly = false);
    void hoveredNodeDetached(Node*);
    void activeChainNodeDetached(Node*);

    // Updates for :target (CSS3 selector).
    void setCSSTarget(Element*);
    Element* cssTarget() const { return m_cssTarget; }
    
    void scheduleForcedStyleRecalc();
    void scheduleStyleRecalc();
    void unscheduleStyleRecalc();
    bool isPendingStyleRecalc() const;
    void styleRecalcTimerFired(Timer<Document>*);

    void attachNodeIterator(NodeIterator*);
    void detachNodeIterator(NodeIterator*);
    void moveNodeIteratorsToNewDocument(Node*, Document*);

    void attachRange(Range*);
    void detachRange(Range*);

    void nodeChildrenChanged(ContainerNode*);
    // nodeChildrenWillBeRemoved is used when removing all node children at once.
    void nodeChildrenWillBeRemoved(ContainerNode*);
    // nodeWillBeRemoved is only safe when removing one node at a time.
    void nodeWillBeRemoved(Node*);

    void textInserted(Node*, unsigned offset, unsigned length);
    void textRemoved(Node*, unsigned offset, unsigned length);
    void textNodesMerged(Text* oldNode, unsigned offset);
    void textNodeSplit(Text* oldNode);

    DOMWindow* defaultView() const { return domWindow(); } 
    DOMWindow* domWindow() const;

    // Helper functions for forwarding DOMWindow event related tasks to the DOMWindow if it exists.
    void setWindowAttributeEventListener(const AtomicString& eventType, PassRefPtr<EventListener>);
    EventListener* getWindowAttributeEventListener(const AtomicString& eventType);
    void dispatchWindowEvent(PassRefPtr<Event>, PassRefPtr<EventTarget> = 0);
    void dispatchWindowLoadEvent();

    PassRefPtr<Event> createEvent(const String& eventType, ExceptionCode&);

    // keep track of what types of event listeners are registered, so we don't
    // dispatch events unnecessarily
    enum ListenerType {
        DOMSUBTREEMODIFIED_LISTENER          = 0x01,
        DOMNODEINSERTED_LISTENER             = 0x02,
        DOMNODEREMOVED_LISTENER              = 0x04,
        DOMNODEREMOVEDFROMDOCUMENT_LISTENER  = 0x08,
        DOMNODEINSERTEDINTODOCUMENT_LISTENER = 0x10,
        DOMATTRMODIFIED_LISTENER             = 0x20,
        DOMCHARACTERDATAMODIFIED_LISTENER    = 0x40,
        OVERFLOWCHANGED_LISTENER             = 0x80,
        ANIMATIONEND_LISTENER                = 0x100,
        ANIMATIONSTART_LISTENER              = 0x200,
        ANIMATIONITERATION_LISTENER          = 0x400,
        TRANSITIONEND_LISTENER               = 0x800,
        BEFORELOAD_LISTENER                  = 0x1000,
        TOUCH_LISTENER                       = 0x2000,
        BEFOREPROCESS_LISTENER               = 0x4000
    };

    bool hasListenerType(ListenerType listenerType) const { return (m_listenerTypes & listenerType); }
    void addListenerType(ListenerType listenerType) { m_listenerTypes = m_listenerTypes | listenerType; }
    void addListenerTypeIfNeeded(const AtomicString& eventType);

    CSSStyleDeclaration* getOverrideStyle(Element*, const String& pseudoElt);

    /**
     * Searches through the document, starting from fromNode, for the next selectable element that comes after fromNode.
     * The order followed is as specified in section 17.11.1 of the HTML4 spec, which is elements with tab indexes
     * first (from lowest to highest), and then elements without tab indexes (in document order).
     *
     * @param fromNode The node from which to start searching. The node after this will be focused. May be null.
     *
     * @return The focus node that comes after fromNode
     *
     * See http://www.w3.org/TR/html4/interact/forms.html#h-17.11.1
     */
    Node* nextFocusableNode(Node* start, KeyboardEvent*);

    /**
     * Searches through the document, starting from fromNode, for the previous selectable element (that comes _before_)
     * fromNode. The order followed is as specified in section 17.11.1 of the HTML4 spec, which is elements with tab
     * indexes first (from lowest to highest), and then elements without tab indexes (in document order).
     *
     * @param fromNode The node from which to start searching. The node before this will be focused. May be null.
     *
     * @return The focus node that comes before fromNode
     *
     * See http://www.w3.org/TR/html4/interact/forms.html#h-17.11.1
     */
    Node* previousFocusableNode(Node* start, KeyboardEvent*);

    int nodeAbsIndex(Node*);
    Node* nodeWithAbsIndex(int absIndex);

    /**
     * Handles a HTTP header equivalent set by a meta tag using <meta http-equiv="..." content="...">. This is called
     * when a meta tag is encountered during document parsing, and also when a script dynamically changes or adds a meta
     * tag. This enables scripts to use meta tags to perform refreshes and set expiry dates in addition to them being
     * specified in a HTML file.
     *
     * @param equiv The http header name (value of the meta tag's "equiv" attribute)
     * @param content The header value (value of the meta tag's "content" attribute)
     */
    void processHttpEquiv(const String& equiv, const String& content);
    void processViewport(const String& features);

    // Returns the owning element in the parent document.
    // Returns 0 if this is the top level document.
    HTMLFrameOwnerElement* ownerElement() const;

    // Used by DOM bindings; no direction known.
    String title() const { return m_title.string(); }
    void setTitle(const String&);

    void setTitleElement(const StringWithDirection&, Element* titleElement);
    void removeTitle(Element* titleElement);

    String cookie(ExceptionCode&) const;
    void setCookie(const String&, ExceptionCode&);

    String referrer() const;

    String domain() const;
    void setDomain(const String& newDomain, ExceptionCode&);

    String lastModified() const;

    // The cookieURL is used to query the cookie database for this document's
    // cookies. For example, if the cookie URL is http://example.com, we'll
    // use the non-Secure cookies for example.com when computing
    // document.cookie.
    //
    // Q: How is the cookieURL different from the document's URL?
    // A: The two URLs are the same almost all the time.  However, if one
    //    document inherits the security context of another document, it
    //    inherits its cookieURL but not its URL.
    //
    const KURL& cookieURL() const { return m_cookieURL; }

    // The firstPartyForCookies is used to compute whether this document
    // appears in a "third-party" context for the purpose of third-party
    // cookie blocking.  The document is in a third-party context if the
    // cookieURL and the firstPartyForCookies are from different hosts.
    //
    // Note: Some ports (including possibly Apple's) only consider the
    //       document in a third-party context if the cookieURL and the
    //       firstPartyForCookies have a different registry-controlled
    //       domain.
    //
    const KURL& firstPartyForCookies() const { return m_firstPartyForCookies; }
    void setFirstPartyForCookies(const KURL& url) { m_firstPartyForCookies = url; }
    
    // The following implements the rule from HTML 4 for what valid names are.
    // To get this right for all the XML cases, we probably have to improve this or move it
    // and make it sensitive to the type of document.
    static bool isValidName(const String&);

    // The following breaks a qualified name into a prefix and a local name.
    // It also does a validity check, and returns false if the qualified name
    // is invalid.  It also sets ExceptionCode when name is invalid.
    static bool parseQualifiedName(const String& qualifiedName, String& prefix, String& localName, ExceptionCode&);
    
    // Checks to make sure prefix and namespace do not conflict (per DOM Core 3)
    static bool hasPrefixNamespaceMismatch(const QualifiedName&);
    
    HTMLElement* body() const;
    void setBody(PassRefPtr<HTMLElement>, ExceptionCode&);

    HTMLHeadElement* head();

    DocumentMarkerController* markers() const { return m_markers.get(); }

    bool directionSetOnDocumentElement() const { return m_directionSetOnDocumentElement; }
    bool writingModeSetOnDocumentElement() const { return m_writingModeSetOnDocumentElement; }
    void setDirectionSetOnDocumentElement(bool b) { m_directionSetOnDocumentElement = b; }
    void setWritingModeSetOnDocumentElement(bool b) { m_writingModeSetOnDocumentElement = b; }

    bool execCommand(const String& command, bool userInterface = false, const String& value = String());
    bool queryCommandEnabled(const String& command);
    bool queryCommandIndeterm(const String& command);
    bool queryCommandState(const String& command);
    bool queryCommandSupported(const String& command);
    String queryCommandValue(const String& command);
    
    // designMode support
    enum InheritedBool { off = false, on = true, inherit };    
    void setDesignMode(InheritedBool value);
    InheritedBool getDesignMode() const;
    bool inDesignMode() const;

    Document* parentDocument() const;
    Document* topDocument() const;

    int docID() const { return m_docID; }
    
    ScriptRunner* scriptRunner() { return m_scriptRunner.get(); }

#if ENABLE(XSLT)
    void applyXSLTransform(ProcessingInstruction* pi);
    PassRefPtr<Document> transformSourceDocument() { return m_transformSourceDocument; }
    void setTransformSourceDocument(Document* doc) { m_transformSourceDocument = doc; }

    void setTransformSource(PassOwnPtr<TransformSource>);
    TransformSource* transformSource() const { return m_transformSource.get(); }
#endif

    void incDOMTreeVersion() { m_domTreeVersion = ++s_globalTreeVersion; }
    uint64_t domTreeVersion() const { return m_domTreeVersion; }

    void setDocType(PassRefPtr<DocumentType>);

#if ENABLE(XPATH)
    // XPathEvaluator methods
    PassRefPtr<XPathExpression> createExpression(const String& expression,
                                                 XPathNSResolver* resolver,
                                                 ExceptionCode& ec);
    PassRefPtr<XPathNSResolver> createNSResolver(Node *nodeResolver);
    PassRefPtr<XPathResult> evaluate(const String& expression,
                                     Node* contextNode,
                                     XPathNSResolver* resolver,
                                     unsigned short type,
                                     XPathResult* result,
                                     ExceptionCode& ec);
#endif // ENABLE(XPATH)
    
    enum PendingSheetLayout { NoLayoutWithPendingSheets, DidLayoutWithPendingSheets, IgnoreLayoutWithPendingSheets };

    bool didLayoutWithPendingStylesheets() const { return m_pendingSheetLayout == DidLayoutWithPendingSheets; }
    
    void setHasNodesWithPlaceholderStyle() { m_hasNodesWithPlaceholderStyle = true; }

    IconURL iconURL(IconType) const;
    void setIconURL(const String&, const String&, IconType);
    void setIconURL(const IconURL&);

    void setUseSecureKeyboardEntryWhenActive(bool);
    bool useSecureKeyboardEntryWhenActive() const;

    void updateFocusAppearanceSoon(bool restorePreviousSelection);
    void cancelFocusAppearanceUpdate();
        
    // FF method for accessing the selection added for compatibility.
    DOMSelection* getSelection() const;
    
    // Extension for manipulating canvas drawing contexts for use in CSS
    CanvasRenderingContext* getCSSCanvasContext(const String& type, const String& name, int width, int height);
    HTMLCanvasElement* getCSSCanvasElement(const String& name);

    bool isDNSPrefetchEnabled() const { return m_isDNSPrefetchEnabled; }
    void parseDNSPrefetchControlHeader(const String&);

    virtual void addMessage(MessageSource, MessageType, MessageLevel, const String& message, unsigned lineNumber, const String& sourceURL, PassRefPtr<ScriptCallStack>);
    virtual void postTask(PassOwnPtr<Task>); // Executes the task on context's thread asynchronously.

    virtual void suspendScriptedAnimationControllerCallbacks();
    virtual void resumeScriptedAnimationControllerCallbacks();

    virtual void finishedParsing();

    bool inPageCache() const { return m_inPageCache; }
    void setInPageCache(bool flag);
    
    // Elements can register themselves for the "documentWillBecomeInactive()" and  
    // "documentDidBecomeActive()" callbacks
    void registerForDocumentActivationCallbacks(Element*);
    void unregisterForDocumentActivationCallbacks(Element*);
    void documentWillBecomeInactive();
    void documentDidBecomeActive();

    void registerForMediaVolumeCallbacks(Element*);
    void unregisterForMediaVolumeCallbacks(Element*);
    void mediaVolumeDidChange();

    void registerForPrivateBrowsingStateChangedCallbacks(Element*);
    void unregisterForPrivateBrowsingStateChangedCallbacks(Element*);
    void privateBrowsingStateDidChange();

    void setShouldCreateRenderers(bool);
    bool shouldCreateRenderers();

    void setDecoder(PassRefPtr<TextResourceDecoder>);
    TextResourceDecoder* decoder() const { return m_decoder.get(); }

    String displayStringModifiedByEncoding(const String&) const;
    PassRefPtr<StringImpl> displayStringModifiedByEncoding(PassRefPtr<StringImpl>) const;
    void displayBufferModifiedByEncoding(UChar* buffer, unsigned len) const;

    // Quirk for the benefit of Apple's Dictionary application.
    void setFrameElementsShouldIgnoreScrolling(bool ignore) { m_frameElementsShouldIgnoreScrolling = ignore; }
    bool frameElementsShouldIgnoreScrolling() const { return m_frameElementsShouldIgnoreScrolling; }

#if ENABLE(DASHBOARD_SUPPORT)
    void setDashboardRegionsDirty(bool f) { m_dashboardRegionsDirty = f; }
    bool dashboardRegionsDirty() const { return m_dashboardRegionsDirty; }
    bool hasDashboardRegions () const { return m_hasDashboardRegions; }
    void setHasDashboardRegions(bool f) { m_hasDashboardRegions = f; }
    const Vector<DashboardRegionValue>& dashboardRegions() const;
    void setDashboardRegions(const Vector<DashboardRegionValue>&);
#endif

    virtual void removeAllEventListeners();

    CheckedRadioButtons& checkedRadioButtons() { return m_checkedRadioButtons; }
    
#if ENABLE(SVG)
    const SVGDocumentExtensions* svgExtensions();
    SVGDocumentExtensions* accessSVGExtensions();
#endif

    void initSecurityContext();

    // Explicitly override the security origin for this document.
    // Note: It is dangerous to change the security origin of a document
    //       that already contains content.
    void setSecurityOrigin(SecurityOrigin*);

    void updateURLForPushOrReplaceState(const KURL&);
    void statePopped(SerializedScriptValue*);

    bool processingLoadEvent() const { return m_processingLoadEvent; }

#if ENABLE(DATABASE)
    virtual bool allowDatabaseAccess() const;
    virtual void databaseExceededQuota(const String& name);
#endif

    virtual bool isContextThread() const;
    virtual bool isJSExecutionForbidden() const { return false; }

    void setUsingGeolocation(bool f) { m_usingGeolocation = f; }
    bool usingGeolocation() const { return m_usingGeolocation; };

    bool containsValidityStyleRules() const { return m_containsValidityStyleRules; }
    void setContainsValidityStyleRules() { m_containsValidityStyleRules = true; }

    void enqueueWindowEvent(PassRefPtr<Event>);
    void enqueueDocumentEvent(PassRefPtr<Event>);
    void enqueuePageshowEvent(PageshowEventPersistence);
    void enqueueHashchangeEvent(const String& oldURL, const String& newURL);
    void enqueuePopstateEvent(PassRefPtr<SerializedScriptValue> stateObject);
    EventQueue* eventQueue() const { return m_eventQueue.get(); }

    void addMediaCanStartListener(MediaCanStartListener*);
    void removeMediaCanStartListener(MediaCanStartListener*);
    MediaCanStartListener* takeAnyMediaCanStartListener();

    const QualifiedName& idAttributeName() const { return m_idAttributeName; }
    
#if ENABLE(FULLSCREEN_API)
    bool webkitIsFullScreen() const { return m_fullScreenElement.get(); }
    bool webkitFullScreenKeyboardInputAllowed() const { return m_fullScreenElement.get() && m_areKeysEnabledInFullScreen; }
    Element* webkitCurrentFullScreenElement() const { return m_fullScreenElement.get(); }
    
    enum FullScreenCheckType {
        EnforceIFrameAllowFulScreenRequirement,
        ExemptIFrameAllowFulScreenRequirement,
    };

    void requestFullScreenForElement(Element*, unsigned short flags, FullScreenCheckType);
    void webkitCancelFullScreen();
    
    void webkitWillEnterFullScreenForElement(Element*);
    void webkitDidEnterFullScreenForElement(Element*);
    void webkitWillExitFullScreenForElement(Element*);
    void webkitDidExitFullScreenForElement(Element*);
    
    void setFullScreenRenderer(RenderFullScreen*);
    RenderFullScreen* fullScreenRenderer() const { return m_fullScreenRenderer; }
    
    void setFullScreenRendererSize(const IntSize&);
    void setFullScreenRendererBackgroundColor(Color);
    
    void fullScreenChangeDelayTimerFired(Timer<Document>*);
    bool fullScreenIsAllowedForElement(Element*) const;
    void fullScreenElementRemoved();
    void removeFullScreenElementOfSubtree(Node*, bool amongChildrenOnly = false);
#endif

    // Used to allow element that loads data without going through a FrameLoader to delay the 'load' event.
    void incrementLoadEventDelayCount() { ++m_loadEventDelayCount; }
    void decrementLoadEventDelayCount();
    bool isDelayingLoadEvent() const { return m_loadEventDelayCount; }

#if ENABLE(TOUCH_EVENTS)
    PassRefPtr<Touch> createTouch(DOMWindow*, EventTarget*, int identifier, int pageX, int pageY, int screenX, int screenY, ExceptionCode&) const;
    PassRefPtr<TouchList> createTouchList(ExceptionCode&) const;
#endif

    const DocumentTiming* timing() const { return &m_documentTiming; }

#if ENABLE(REQUEST_ANIMATION_FRAME)
    int webkitRequestAnimationFrame(PassRefPtr<RequestAnimationFrameCallback>, Element*);
    void webkitCancelRequestAnimationFrame(int id);
    void serviceScriptedAnimations(DOMTimeStamp);
#endif

    virtual EventTarget* errorEventTarget();
    virtual void logExceptionToConsole(const String& errorMessage, int lineNumber, const String& sourceURL, PassRefPtr<ScriptCallStack>);

    void initDNSPrefetch();

    ContentSecurityPolicy* contentSecurityPolicy() { return m_contentSecurityPolicy.get(); }

protected:
    Document(Frame*, const KURL&, bool isXHTML, bool isHTML);

    void clearXMLVersion() { m_xmlVersion = String(); }

private:
    friend class IgnoreDestructiveWriteCountIncrementer;

    void detachParser();

    typedef void (*ArgumentsCallback)(const String& keyString, const String& valueString, Document*, void* data);
    void processArguments(const String& features, void* data, ArgumentsCallback);

    virtual bool isDocument() const { return true; }

    virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);

    virtual String nodeName() const;
    virtual NodeType nodeType() const;
    virtual bool childTypeAllowed(NodeType) const;
    virtual PassRefPtr<Node> cloneNode(bool deep);
    virtual bool canReplaceChild(Node* newChild, Node* oldChild);

    virtual void refScriptExecutionContext() { ref(); }
    virtual void derefScriptExecutionContext() { deref(); }

    virtual const KURL& virtualURL() const; // Same as url(), but needed for ScriptExecutionContext to implement it without a performance loss for direct calls.
    virtual KURL virtualCompleteURL(const String&) const; // Same as completeURL() for the same reason as above.

    virtual double minimumTimerInterval() const;

    String encoding() const;

    void updateTitle(const StringWithDirection&);
    void updateFocusAppearanceTimerFired(Timer<Document>*);
    void updateBaseURL();

    void cacheDocumentElement() const;

    void buildAccessKeyMap(ContainerNode* root);

    void createStyleSelector();

    PassRefPtr<NodeList> handleZeroPadding(const HitTestRequest&, HitTestResult&) const;

    void loadEventDelayTimerFired(Timer<Document>*);

    int m_guardRefCount;

    OwnPtr<CSSStyleSelector> m_styleSelector;
    bool m_didCalculateStyleSelector;
    bool m_hasDirtyStyleSelector;
    
    mutable RefPtr<CSSPrimitiveValueCache> m_cssPrimitiveValueCache;

    Frame* m_frame;
    DocumentLoader* m_documentLoader;
    OwnPtr<CachedResourceLoader> m_cachedResourceLoader;
    RefPtr<DocumentParser> m_parser;
    bool m_wellFormed;

    // Document URLs.
    KURL m_url; // Document.URL: The URL from which this document was retrieved.
    KURL m_baseURL; // Node.baseURI: The URL to use when resolving relative URLs.
    KURL m_baseElementURL; // The URL set by the <base> element.
    KURL m_cookieURL; // The URL to use for cookie access.
    KURL m_firstPartyForCookies; // The policy URL for third-party cookie blocking.

    // Document.documentURI:
    // Although URL-like, Document.documentURI can actually be set to any
    // string by content.  Document.documentURI affects m_baseURL unless the
    // document contains a <base> element, in which case the <base> element
    // takes precedence.
    String m_documentURI;

    String m_baseTarget;

    RefPtr<DocumentType> m_docType;
    OwnPtr<DOMImplementation> m_implementation;

    // Track the number of currently loading top-level stylesheets needed for rendering.
    // Sheets loaded using the @import directive are not included in this count.
    // We use this count of pending sheets to detect when we can begin attaching
    // elements and when it is safe to execute scripts.
    int m_pendingStylesheets;

    // But sometimes you need to ignore pending stylesheet count to
    // force an immediate layout when requested by JS.
    bool m_ignorePendingStylesheets;

    // If we do ignore the pending stylesheet count, then we need to add a boolean
    // to track that this happened so that we can do a full repaint when the stylesheets
    // do eventually load.
    PendingSheetLayout m_pendingSheetLayout;
    
    bool m_hasNodesWithPlaceholderStyle;

    RefPtr<CSSStyleSheet> m_elemSheet;
    RefPtr<CSSStyleSheet> m_mappedElementSheet;
    RefPtr<CSSStyleSheet> m_pageUserSheet;
    mutable OwnPtr<Vector<RefPtr<CSSStyleSheet> > > m_pageGroupUserSheets;
    mutable bool m_pageGroupUserSheetCacheValid;

    bool m_printing;
    bool m_paginatedForScreen;

    bool m_ignoreAutofocus;

    CompatibilityMode m_compatibilityMode;
    bool m_compatibilityModeLocked; // This is cheaper than making setCompatibilityMode virtual.

    Color m_textColor;

    RefPtr<Node> m_focusedNode;
    RefPtr<Node> m_hoverNode;
    RefPtr<Node> m_activeNode;
    mutable RefPtr<Element> m_documentElement;

    uint64_t m_domTreeVersion;
    static uint64_t s_globalTreeVersion;
    
    HashSet<NodeIterator*> m_nodeIterators;
    HashSet<Range*> m_ranges;

    unsigned short m_listenerTypes;

    RefPtr<StyleSheetList> m_styleSheets; // All of the stylesheets that are currently in effect for our media type and stylesheet set.
    
    typedef ListHashSet<Node*, 32> StyleSheetCandidateListHashSet;
    StyleSheetCandidateListHashSet m_styleSheetCandidateNodes; // All of the nodes that could potentially provide stylesheets to the document (<link>, <style>, <?xml-stylesheet>)

    typedef ListHashSet<Element*, 64> FormElementListHashSet;
    FormElementListHashSet m_formElementsWithState;
    typedef ListHashSet<RefPtr<FormAssociatedElement>, 32> FormAssociatedElementListHashSet;
    FormAssociatedElementListHashSet m_formElementsWithFormAttribute;

    typedef HashMap<FormElementKey, Vector<String>, FormElementKeyHash, FormElementKeyHashTraits> FormElementStateMap;
    FormElementStateMap m_stateForNewFormElements;
    
    Color m_linkColor;
    Color m_visitedLinkColor;
    Color m_activeLinkColor;

    String m_preferredStylesheetSet;
    String m_selectedStylesheetSet;

    bool m_loadingSheet;
    bool m_visuallyOrdered;
    ReadyState m_readyState;
    bool m_bParsing;
    
    Timer<Document> m_styleRecalcTimer;
    bool m_pendingStyleRecalcShouldForce;
    bool m_inStyleRecalc;
    bool m_closeAfterStyleRecalc;

    bool m_usesSiblingRules;
    bool m_usesSiblingRulesOverride;
    bool m_usesFirstLineRules;
    bool m_usesFirstLetterRules;
    bool m_usesBeforeAfterRules;
    bool m_usesBeforeAfterRulesOverride;
    bool m_usesRemUnits;
    bool m_usesLinkRules;
    bool m_gotoAnchorNeededAfterStylesheetsLoad;
    bool m_isDNSPrefetchEnabled;
    bool m_haveExplicitlyDisabledDNSPrefetch;
    bool m_frameElementsShouldIgnoreScrolling;
    bool m_containsValidityStyleRules;
    bool m_updateFocusAppearanceRestoresSelection;

    // http://www.whatwg.org/specs/web-apps/current-work/#ignore-destructive-writes-counter
    unsigned m_ignoreDestructiveWriteCount;

    StringWithDirection m_title;
    StringWithDirection m_rawTitle;
    bool m_titleSetExplicitly;
    RefPtr<Element> m_titleElement;

    OwnPtr<RenderArena> m_renderArena;

    mutable AXObjectCache* m_axObjectCache;
    OwnPtr<DocumentMarkerController> m_markers;
    
    Timer<Document> m_updateFocusAppearanceTimer;

    Element* m_cssTarget;
    
    bool m_processingLoadEvent;
    RefPtr<SerializedScriptValue> m_pendingStateObject;
    double m_startTime;
    bool m_overMinimumLayoutThreshold;
    // This is used to increase the minimum delay between re-layouts. It is set
    // using setExtraLayoutDelay to modify the minimum delay used at different
    // points during the lifetime of the Document.
    int m_extraLayoutDelay;
    
    OwnPtr<ScriptRunner> m_scriptRunner;

#if ENABLE(XSLT)
    OwnPtr<TransformSource> m_transformSource;
    RefPtr<Document> m_transformSourceDocument;
#endif

    int m_docID; // A unique document identifier used for things like document-specific mapped attributes.

    String m_xmlEncoding;
    String m_xmlVersion;
    bool m_xmlStandalone;

    String m_contentLanguage;

#if ENABLE(XHTMLMP)
    bool m_shouldProcessNoScriptElement;
#endif

    RenderObject* m_savedRenderer;
    
    RefPtr<TextResourceDecoder> m_decoder;

    InheritedBool m_designMode;
    
    CheckedRadioButtons m_checkedRadioButtons;

    typedef HashMap<AtomicStringImpl*, CollectionCache*> NamedCollectionMap;
    FixedArray<CollectionCache, NumUnnamedDocumentCachedTypes> m_collectionInfo;
    FixedArray<NamedCollectionMap, NumNamedDocumentCachedTypes> m_nameCollectionInfo;

#if ENABLE(XPATH)
    RefPtr<XPathEvaluator> m_xpathEvaluator;
#endif
    
#if ENABLE(SVG)
    OwnPtr<SVGDocumentExtensions> m_svgExtensions;
#endif
    
#if ENABLE(DASHBOARD_SUPPORT)
    Vector<DashboardRegionValue> m_dashboardRegions;
    bool m_hasDashboardRegions;
    bool m_dashboardRegionsDirty;
#endif

    HashMap<String, RefPtr<HTMLCanvasElement> > m_cssCanvasElements;

    bool m_createRenderers;
    bool m_inPageCache;
    IconURL m_iconURLs[ICON_COUNT];

    HashSet<Element*> m_documentActivationCallbackElements;
    HashSet<Element*> m_mediaVolumeCallbackElements;
    HashSet<Element*> m_privateBrowsingStateChangedElements;

    HashMap<StringImpl*, Element*, CaseFoldingHash> m_elementsByAccessKey;    
    bool m_accessKeyMapValid;

    bool m_useSecureKeyboardEntryWhenActive;

    bool m_isXHTML;
    bool m_isHTML;

    bool m_usesViewSourceStyles;
    bool m_sawElementsInKnownNamespaces;

    bool m_usingGeolocation;

    RefPtr<EventQueue> m_eventQueue;

    RefPtr<DocumentWeakReference> m_weakReference;

    HashSet<MediaCanStartListener*> m_mediaCanStartListeners;

    QualifiedName m_idAttributeName;

#if ENABLE(FULLSCREEN_API)
    bool m_areKeysEnabledInFullScreen;
    RefPtr<Element> m_fullScreenElement;
    RenderFullScreen* m_fullScreenRenderer;
    Timer<Document> m_fullScreenChangeDelayTimer;
    Deque<RefPtr<Element> > m_fullScreenChangeEventTargetQueue;
#endif

    int m_loadEventDelayCount;
    Timer<Document> m_loadEventDelayTimer;

    ViewportArguments m_viewportArguments;

    bool m_directionSetOnDocumentElement;
    bool m_writingModeSetOnDocumentElement;

    DocumentTiming m_documentTiming;
    RefPtr<MediaQueryMatcher> m_mediaQueryMatcher;
    bool m_writeRecursionIsTooDeep;
    unsigned m_writeRecursionDepth;

#if ENABLE(REQUEST_ANIMATION_FRAME)
    OwnPtr<ScriptedAnimationController> m_scriptedAnimationController;
#endif

    RefPtr<ContentSecurityPolicy> m_contentSecurityPolicy;
};

// Put these methods here, because they require the Document definition, but we really want to inline them.

inline bool Node::isDocumentNode() const
{
    return this == m_document;
}

inline Node::Node(Document* document, ConstructionType type)
    : m_document(document)
    , m_previous(0)
    , m_next(0)
    , m_renderer(0)
    , m_nodeFlags(type)
{
    if (m_document)
        m_document->guardRef();
#if !defined(NDEBUG) || (defined(DUMP_NODE_STATISTICS) && DUMP_NODE_STATISTICS)
    trackForDebugging();
#endif
}

} // namespace WebCore

#endif // Document_h
