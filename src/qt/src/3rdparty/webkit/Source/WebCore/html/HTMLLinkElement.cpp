/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Rob Buis (rwlbuis@gmail.com)
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
#include "HTMLLinkElement.h"

#include "Attribute.h"
#include "CachedCSSStyleSheet.h"
#include "CachedResource.h"
#include "CachedResourceLoader.h"
#include "CSSStyleSelector.h"
#include "Document.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "MediaList.h"
#include "MediaQueryEvaluator.h"
#include "Page.h"
#include "ResourceHandle.h"
#include "ScriptEventListener.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include <wtf/StdLibExtras.h>

namespace WebCore {

using namespace HTMLNames;

inline HTMLLinkElement::HTMLLinkElement(const QualifiedName& tagName, Document* document, bool createdByParser)
    : HTMLElement(tagName, document)
#if ENABLE(LINK_PREFETCH)
    , m_onloadTimer(this, &HTMLLinkElement::onloadTimerFired)
#endif
    , m_loading(false)
    , m_isEnabledViaScript(false)
    , m_createdByParser(createdByParser)
    , m_isInShadowTree(false)
    , m_pendingSheetType(None)
{
    ASSERT(hasTagName(linkTag));
}

PassRefPtr<HTMLLinkElement> HTMLLinkElement::create(const QualifiedName& tagName, Document* document, bool createdByParser)
{
    return adoptRef(new HTMLLinkElement(tagName, document, createdByParser));
}

HTMLLinkElement::~HTMLLinkElement()
{
    if (m_sheet)
        m_sheet->clearOwnerNode();

    if (m_cachedSheet) {
        m_cachedSheet->removeClient(this);    
        removePendingSheet();
    }
    
#if ENABLE(LINK_PREFETCH)
    if (m_cachedLinkResource)
        m_cachedLinkResource->removeClient(this);
#endif

    if (inDocument())
        document()->removeStyleSheetCandidateNode(this);
}

void HTMLLinkElement::setDisabled(bool disabled)
{
    if (!m_sheet)
        return;

    bool wasDisabled = m_sheet->disabled();
    if (wasDisabled == disabled)
        return;

    m_sheet->setDisabled(disabled);
    m_isEnabledViaScript = !disabled;

    // If we change the disabled state while the sheet is still loading, then we have to
    // perform three checks:
    if (isLoading()) {
        // Check #1: The sheet becomes disabled while loading.
        if (disabled)
            removePendingSheet();

        // Check #2: An alternate sheet becomes enabled while it is still loading.
        if (m_relAttribute.m_isAlternate && !disabled)
            addPendingSheet(Blocking);

        // Check #3: A main sheet becomes enabled while it was still loading and
        // after it was disabled via script. It takes really terrible code to make this
        // happen (a double toggle for no reason essentially). This happens on
        // virtualplastic.net, which manages to do about 12 enable/disables on only 3
        // sheets. :)
        if (!m_relAttribute.m_isAlternate && !disabled && wasDisabled)
            addPendingSheet(Blocking);

        // If the sheet is already loading just bail.
        return;
    }

    if (!disabled)
        process();
}

StyleSheet* HTMLLinkElement::sheet() const
{
    return m_sheet.get();
}

void HTMLLinkElement::parseMappedAttribute(Attribute* attr)
{
    if (attr->name() == relAttr) {
        tokenizeRelAttribute(attr->value(), m_relAttribute);
        process();
    } else if (attr->name() == hrefAttr) {
        m_url = document()->completeURL(stripLeadingAndTrailingHTMLSpaces(attr->value()));
        process();
    } else if (attr->name() == typeAttr) {
        m_type = attr->value();
        process();
    } else if (attr->name() == mediaAttr) {
        m_media = attr->value().string().lower();
        process();
    } else if (attr->name() == onbeforeloadAttr)
        setAttributeEventListener(eventNames().beforeloadEvent, createAttributeEventListener(this, attr));
#if ENABLE(LINK_PREFETCH)
    else if (attr->name() == onloadAttr)
        setAttributeEventListener(eventNames().loadEvent, createAttributeEventListener(this, attr));
    else if (attr->name() == onerrorAttr)
        setAttributeEventListener(eventNames().errorEvent, createAttributeEventListener(this, attr));
#endif
    else {
        if (attr->name() == titleAttr && m_sheet)
            m_sheet->setTitle(attr->value());
        HTMLElement::parseMappedAttribute(attr);
    }
}

void HTMLLinkElement::tokenizeRelAttribute(const AtomicString& rel, RelAttribute& relAttribute)
{
    relAttribute.m_isStyleSheet = false;
    relAttribute.m_iconType = InvalidIcon;
    relAttribute.m_isAlternate = false;
    relAttribute.m_isDNSPrefetch = false;
#if ENABLE(LINK_PREFETCH)
    relAttribute.m_isLinkPrefetch = false;
    relAttribute.m_isLinkSubresource = false;
#endif
    if (equalIgnoringCase(rel, "stylesheet"))
        relAttribute.m_isStyleSheet = true;
    else if (equalIgnoringCase(rel, "icon") || equalIgnoringCase(rel, "shortcut icon"))
        relAttribute.m_iconType = Favicon;
#if ENABLE(TOUCH_ICON_LOADING)
    else if (equalIgnoringCase(rel, "apple-touch-icon"))
        relAttribute.m_iconType = TouchIcon;
    else if (equalIgnoringCase(rel, "apple-touch-icon-precomposed"))
        relAttribute.m_iconType = TouchPrecomposedIcon;
#endif
    else if (equalIgnoringCase(rel, "dns-prefetch"))
        relAttribute.m_isDNSPrefetch = true;
    else if (equalIgnoringCase(rel, "alternate stylesheet") || equalIgnoringCase(rel, "stylesheet alternate")) {
        relAttribute.m_isStyleSheet = true;
        relAttribute.m_isAlternate = true;
    } else {
        // Tokenize the rel attribute and set bits based on specific keywords that we find.
        String relString = rel.string();
        relString.replace('\n', ' ');
        Vector<String> list;
        relString.split(' ', list);
        Vector<String>::const_iterator end = list.end();
        for (Vector<String>::const_iterator it = list.begin(); it != end; ++it) {
            if (equalIgnoringCase(*it, "stylesheet"))
                relAttribute.m_isStyleSheet = true;
            else if (equalIgnoringCase(*it, "alternate"))
                relAttribute.m_isAlternate = true;
            else if (equalIgnoringCase(*it, "icon"))
                relAttribute.m_iconType = Favicon;
#if ENABLE(TOUCH_ICON_LOADING)
            else if (equalIgnoringCase(*it, "apple-touch-icon"))
                relAttribute.m_iconType = TouchIcon;
            else if (equalIgnoringCase(*it, "apple-touch-icon-precomposed"))
                relAttribute.m_iconType = TouchPrecomposedIcon;
#endif
#if ENABLE(LINK_PREFETCH)
            else if (equalIgnoringCase(*it, "prefetch"))
              relAttribute.m_isLinkPrefetch = true;
            else if (equalIgnoringCase(*it, "subresource"))
              relAttribute.m_isLinkSubresource = true;
#endif
        }
    }
}

bool HTMLLinkElement::checkBeforeLoadEvent()
{
    RefPtr<Document> originalDocument = document();
    if (!dispatchBeforeLoadEvent(m_url))
        return false;
    // A beforeload handler might have removed us from the document or changed the document.
    if (!inDocument() || document() != originalDocument)
        return false;
    return true;
}

void HTMLLinkElement::process()
{
    if (!inDocument() || m_isInShadowTree) {
        ASSERT(!m_sheet);
        return;
    }

    String type = m_type.lower();

    // IE extension: location of small icon for locationbar / bookmarks
    // We'll record this URL per document, even if we later only use it in top level frames
    if (m_relAttribute.m_iconType != InvalidIcon && m_url.isValid() && !m_url.isEmpty()) {
        if (!checkBeforeLoadEvent()) 
            return;
        document()->setIconURL(m_url.string(), type, m_relAttribute.m_iconType);
    }

    if (m_relAttribute.m_isDNSPrefetch) {
        Settings* settings = document()->settings();
        // FIXME: The href attribute of the link element can be in "//hostname" form, and we shouldn't attempt
        // to complete that as URL <https://bugs.webkit.org/show_bug.cgi?id=48857>.
        if (settings && settings->dnsPrefetchingEnabled() && m_url.isValid() && !m_url.isEmpty())
            ResourceHandle::prepareForURL(m_url);
    }

#if ENABLE(LINK_PREFETCH)
    if ((m_relAttribute.m_isLinkPrefetch || m_relAttribute.m_isLinkSubresource) && m_url.isValid() && document()->frame()) {
        if (!checkBeforeLoadEvent())
            return;
        ResourceLoadPriority priority = ResourceLoadPriorityUnresolved;
        if (m_relAttribute.m_isLinkSubresource)
            priority = ResourceLoadPriorityLow;

        m_cachedLinkResource = document()->cachedResourceLoader()->requestLinkResource(m_url, priority);
        if (m_cachedLinkResource)
            m_cachedLinkResource->addClient(this);
    }
#endif

    bool acceptIfTypeContainsTextCSS = document()->page() && document()->page()->settings() && document()->page()->settings()->treatsAnyTextCSSLinkAsStylesheet();
    
    if (!disabled() && (m_relAttribute.m_isStyleSheet || (acceptIfTypeContainsTextCSS && type.contains("text/css")))
        && document()->frame() && m_url.isValid()) {
        
        String charset = getAttribute(charsetAttr);
        if (charset.isEmpty() && document()->frame())
            charset = document()->charset();
        
        if (m_cachedSheet) {
            removePendingSheet();
            m_cachedSheet->removeClient(this);
            m_cachedSheet = 0;
        }

        if (!checkBeforeLoadEvent())
            return;

        m_loading = true;

        bool mediaQueryMatches = true;
        if (!m_media.isEmpty()) {
            RefPtr<RenderStyle> documentStyle = CSSStyleSelector::styleForDocument(document());
            RefPtr<MediaList> media = MediaList::createAllowingDescriptionSyntax(m_media);
            MediaQueryEvaluator evaluator(document()->frame()->view()->mediaType(), document()->frame(), documentStyle.get());
            mediaQueryMatches = evaluator.eval(media.get());
        }

        // Don't hold up render tree construction and script execution on stylesheets
        // that are not needed for the rendering at the moment.
        bool blocking = mediaQueryMatches && !isAlternate();
        addPendingSheet(blocking ? Blocking : NonBlocking);

        // Load stylesheets that are not needed for the rendering immediately with low priority.
        ResourceLoadPriority priority = blocking ? ResourceLoadPriorityUnresolved : ResourceLoadPriorityVeryLow;
        m_cachedSheet = document()->cachedResourceLoader()->requestCSSStyleSheet(m_url, charset, priority);
        
        if (m_cachedSheet)
            m_cachedSheet->addClient(this);
        else {
            // The request may have been denied if (for example) the stylesheet is local and the document is remote.
            m_loading = false;
            removePendingSheet();
        }
    } else if (m_sheet) {
        // we no longer contain a stylesheet, e.g. perhaps rel or type was changed
        m_sheet = 0;
        document()->styleSelectorChanged(DeferRecalcStyle);
    }
}

void HTMLLinkElement::insertedIntoDocument()
{
    HTMLElement::insertedIntoDocument();

    m_isInShadowTree = isInShadowTree();
    if (m_isInShadowTree)
        return;

    document()->addStyleSheetCandidateNode(this, m_createdByParser);

    process();
}

void HTMLLinkElement::removedFromDocument()
{
    HTMLElement::removedFromDocument();

    if (m_isInShadowTree) {
        ASSERT(!m_sheet);
        return;
    }
    document()->removeStyleSheetCandidateNode(this);

    if (m_sheet) {
        ASSERT(m_sheet->ownerNode() == this);
        m_sheet->clearOwnerNode();
        m_sheet = 0;
    }

    if (document()->renderer())
        document()->styleSelectorChanged(DeferRecalcStyle);
}

void HTMLLinkElement::finishParsingChildren()
{
    m_createdByParser = false;
    HTMLElement::finishParsingChildren();
}

void HTMLLinkElement::setCSSStyleSheet(const String& href, const KURL& baseURL, const String& charset, const CachedCSSStyleSheet* sheet)
{
    if (!inDocument()) {
        ASSERT(!m_sheet);
        return;
    }

    m_sheet = CSSStyleSheet::create(this, href, baseURL, charset);

    bool strictParsing = !document()->inQuirksMode();
    bool enforceMIMEType = strictParsing;
    bool crossOriginCSS = false;
    bool validMIMEType = false;
    bool needsSiteSpecificQuirks = document()->page() && document()->page()->settings()->needsSiteSpecificQuirks();

    // Check to see if we should enforce the MIME type of the CSS resource in strict mode.
    // Running in iWeb 2 is one example of where we don't want to - <rdar://problem/6099748>
    if (enforceMIMEType && document()->page() && !document()->page()->settings()->enforceCSSMIMETypeInNoQuirksMode())
        enforceMIMEType = false;

#ifdef BUILDING_ON_LEOPARD
    if (enforceMIMEType && needsSiteSpecificQuirks) {
        // Covers both http and https, with or without "www."
        if (baseURL.string().contains("mcafee.com/japan/", false))
            enforceMIMEType = false;
    }
#endif

    String sheetText = sheet->sheetText(enforceMIMEType, &validMIMEType);
    m_sheet->parseString(sheetText, strictParsing);

    // If we're loading a stylesheet cross-origin, and the MIME type is not
    // standard, require the CSS to at least start with a syntactically
    // valid CSS rule.
    // This prevents an attacker playing games by injecting CSS strings into
    // HTML, XML, JSON, etc. etc.
    if (!document()->securityOrigin()->canRequest(baseURL))
        crossOriginCSS = true;

    if (crossOriginCSS && !validMIMEType && !m_sheet->hasSyntacticallyValidCSSHeader())
        m_sheet = CSSStyleSheet::create(this, href, baseURL, charset);

    if (strictParsing && needsSiteSpecificQuirks) {
        // Work around <https://bugs.webkit.org/show_bug.cgi?id=28350>.
        DEFINE_STATIC_LOCAL(const String, slashKHTMLFixesDotCss, ("/KHTMLFixes.css"));
        DEFINE_STATIC_LOCAL(const String, mediaWikiKHTMLFixesStyleSheet, ("/* KHTML fix stylesheet */\n/* work around the horizontal scrollbars */\n#column-content { margin-left: 0; }\n\n"));
        // There are two variants of KHTMLFixes.css. One is equal to mediaWikiKHTMLFixesStyleSheet,
        // while the other lacks the second trailing newline.
        if (baseURL.string().endsWith(slashKHTMLFixesDotCss) && !sheetText.isNull() && mediaWikiKHTMLFixesStyleSheet.startsWith(sheetText)
                && sheetText.length() >= mediaWikiKHTMLFixesStyleSheet.length() - 1) {
            ASSERT(m_sheet->length() == 1);
            ExceptionCode ec;
            m_sheet->deleteRule(0, ec);
        }
    }

    m_sheet->setTitle(title());

    RefPtr<MediaList> media = MediaList::createAllowingDescriptionSyntax(m_media);
    m_sheet->setMedia(media.get());

    m_loading = false;
    m_sheet->checkLoaded();
}

bool HTMLLinkElement::isLoading() const
{
    if (m_loading)
        return true;
    if (!m_sheet)
        return false;
    return static_cast<CSSStyleSheet *>(m_sheet.get())->isLoading();
}

#if ENABLE(LINK_PREFETCH)
void HTMLLinkElement::onloadTimerFired(Timer<HTMLLinkElement>* timer)
{
    ASSERT_UNUSED(timer, timer == &m_onloadTimer);
    if (m_cachedLinkResource->errorOccurred())
        dispatchEvent(Event::create(eventNames().errorEvent, false, false));
    else if (!m_cachedLinkResource->wasCanceled())
        dispatchEvent(Event::create(eventNames().loadEvent, false, false));

    m_cachedLinkResource->removeClient(this);
    m_cachedLinkResource = 0;
}

void HTMLLinkElement::notifyFinished(CachedResource* resource)
{
    m_onloadTimer.startOneShot(0);
    ASSERT(m_cachedLinkResource.get() == resource);
}
#endif

bool HTMLLinkElement::sheetLoaded()
{
    if (!isLoading()) {
        removePendingSheet();
        return true;
    }
    return false;
}

bool HTMLLinkElement::isURLAttribute(Attribute *attr) const
{
    return attr->name() == hrefAttr;
}

KURL HTMLLinkElement::href() const
{
    return document()->completeURL(getAttribute(hrefAttr));
}

String HTMLLinkElement::rel() const
{
    return getAttribute(relAttr);
}

String HTMLLinkElement::target() const
{
    return getAttribute(targetAttr);
}

String HTMLLinkElement::type() const
{
    return getAttribute(typeAttr);
}

void HTMLLinkElement::addSubresourceAttributeURLs(ListHashSet<KURL>& urls) const
{
    HTMLElement::addSubresourceAttributeURLs(urls);

    // Favicons are handled by a special case in LegacyWebArchive::create()
    if (m_relAttribute.m_iconType != InvalidIcon)
        return;

    if (!m_relAttribute.m_isStyleSheet)
        return;
    
    // Append the URL of this link element.
    addSubresourceURL(urls, href());
    
    // Walk the URLs linked by the linked-to stylesheet.
    if (StyleSheet* styleSheet = const_cast<HTMLLinkElement*>(this)->sheet())
        styleSheet->addSubresourceStyleURLs(urls);
}

void HTMLLinkElement::addPendingSheet(PendingSheetType type)
{
    if (type <= m_pendingSheetType)
        return;
    m_pendingSheetType = type;

    if (m_pendingSheetType == NonBlocking)
        return;
    document()->addPendingSheet();
}

void HTMLLinkElement::removePendingSheet()
{
    PendingSheetType type = m_pendingSheetType;
    m_pendingSheetType = None;

    if (type == None)
        return;
    if (type == NonBlocking) {
        // Document::removePendingSheet() triggers the style selector recalc for blocking sheets.
        document()->styleSelectorChanged(RecalcStyleImmediately);
        return;
    }
    document()->removePendingSheet();
}

bool HTMLLinkElement::disabled() const
{
    return m_sheet && m_sheet->disabled();
}

}
