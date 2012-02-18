/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * (C) 2002-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002, 2005, 2006, 2008, 2009, 2010 Apple Inc. All rights reserved.
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
#include "CSSImportRule.h"

#include "CachedCSSStyleSheet.h"
#include "CachedResourceLoader.h"
#include "Document.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include <wtf/StdLibExtras.h>

namespace WebCore {

CSSImportRule::CSSImportRule(CSSStyleSheet* parent, const String& href, PassRefPtr<MediaList> media)
    : CSSRule(parent)
    , m_strHref(href)
    , m_lstMedia(media)
    , m_cachedSheet(0)
    , m_loading(false)
{
    if (m_lstMedia)
        m_lstMedia->setParent(this);
    else
        m_lstMedia = MediaList::create(this, String());
}

CSSImportRule::~CSSImportRule()
{
    if (m_lstMedia)
        m_lstMedia->setParent(0);
    if (m_styleSheet)
        m_styleSheet->setParent(0);
    if (m_cachedSheet)
        m_cachedSheet->removeClient(this);
}

void CSSImportRule::setCSSStyleSheet(const String& href, const KURL& baseURL, const String& charset, const CachedCSSStyleSheet* sheet)
{
    if (m_styleSheet)
        m_styleSheet->setParent(0);
    m_styleSheet = CSSStyleSheet::create(this, href, baseURL, charset);

    bool crossOriginCSS = false;
    bool validMIMEType = false;
    CSSStyleSheet* parent = parentStyleSheet();
    bool strict = !parent || parent->useStrictParsing();
    bool enforceMIMEType = strict;
    bool needsSiteSpecificQuirks = parent && parent->document() && parent->document()->settings() && parent->document()->settings()->needsSiteSpecificQuirks();

#ifdef BUILDING_ON_LEOPARD
    if (enforceMIMEType && needsSiteSpecificQuirks) {
        // Covers both http and https, with or without "www."
        if (baseURL.string().contains("mcafee.com/japan/", false))
            enforceMIMEType = false;
    }
#endif

    String sheetText = sheet->sheetText(enforceMIMEType, &validMIMEType);
    m_styleSheet->parseString(sheetText, strict);

    if (!parent || !parent->document() || !parent->document()->securityOrigin()->canRequest(baseURL))
        crossOriginCSS = true;

    if (crossOriginCSS && !validMIMEType && !m_styleSheet->hasSyntacticallyValidCSSHeader())
        m_styleSheet = CSSStyleSheet::create(this, href, baseURL, charset);

    if (strict && needsSiteSpecificQuirks) {
        // Work around <https://bugs.webkit.org/show_bug.cgi?id=28350>.
        DEFINE_STATIC_LOCAL(const String, slashKHTMLFixesDotCss, ("/KHTMLFixes.css"));
        DEFINE_STATIC_LOCAL(const String, mediaWikiKHTMLFixesStyleSheet, ("/* KHTML fix stylesheet */\n/* work around the horizontal scrollbars */\n#column-content { margin-left: 0; }\n\n"));
        // There are two variants of KHTMLFixes.css. One is equal to mediaWikiKHTMLFixesStyleSheet,
        // while the other lacks the second trailing newline.
        if (baseURL.string().endsWith(slashKHTMLFixesDotCss) && !sheetText.isNull() && mediaWikiKHTMLFixesStyleSheet.startsWith(sheetText)
                && sheetText.length() >= mediaWikiKHTMLFixesStyleSheet.length() - 1) {
            ASSERT(m_styleSheet->length() == 1);
            ExceptionCode ec;
            m_styleSheet->deleteRule(0, ec);
        }
    }

    m_loading = false;

    if (parent)
        parent->checkLoaded();
}

bool CSSImportRule::isLoading() const
{
    return m_loading || (m_styleSheet && m_styleSheet->isLoading());
}

void CSSImportRule::insertedIntoParent()
{
    CSSStyleSheet* parentSheet = parentStyleSheet();
    if (!parentSheet || !parentSheet->document())
        return;

    CachedResourceLoader* cachedResourceLoader = parentSheet->document()->cachedResourceLoader();
    if (!cachedResourceLoader)
        return;

    String absHref = m_strHref;
    if (!parentSheet->finalURL().isNull())
        // use parent styleheet's URL as the base URL
        absHref = KURL(parentSheet->finalURL(), m_strHref).string();

    // Check for a cycle in our import chain.  If we encounter a stylesheet
    // in our parent chain with the same URL, then just bail.
    StyleBase* root = this;
    for (StyleBase* curr = parent(); curr; curr = curr->parent()) {
        // FIXME: This is wrong if the finalURL was updated via document::updateBaseURL. 
        if (curr->isCSSStyleSheet() && absHref == static_cast<CSSStyleSheet*>(curr)->finalURL().string())
            return;
        root = curr;
    }

    if (parentSheet->isUserStyleSheet())
        m_cachedSheet = cachedResourceLoader->requestUserCSSStyleSheet(absHref, parentSheet->charset());
    else
        m_cachedSheet = cachedResourceLoader->requestCSSStyleSheet(absHref, parentSheet->charset());
    if (m_cachedSheet) {
        // if the import rule is issued dynamically, the sheet may be
        // removed from the pending sheet count, so let the doc know
        // the sheet being imported is pending.
        if (parentSheet && parentSheet->loadCompleted() && root == parentSheet)
            parentSheet->document()->addPendingSheet();
        m_loading = true;
        m_cachedSheet->addClient(this);
    }
}

String CSSImportRule::cssText() const
{
    String result = "@import url(\"";
    result += m_strHref;
    result += "\")";

    if (m_lstMedia) {
        result += " ";
        result += m_lstMedia->mediaText();
    }
    result += ";";

    return result;
}

void CSSImportRule::addSubresourceStyleURLs(ListHashSet<KURL>& urls)
{
    if (m_styleSheet)
        addSubresourceURL(urls, m_styleSheet->baseURL());
}

} // namespace WebCore
