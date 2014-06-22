/*
 * This file is part of the XSL implementation.
 *
 * Copyright (C) 2004, 2005, 2006, 2008 Apple Inc. All rights reserved.
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
#include "XSLImportRule.h"

#if ENABLE(XSLT)

#include "CachedXSLStyleSheet.h"
#include "CachedResourceLoader.h"
#include "CachedResourceRequest.h"
#include "Document.h"
#include "XSLStyleSheet.h"

namespace WebCore {

XSLImportRule::XSLImportRule(XSLStyleSheet* parent, const String& href)
    : m_parentStyleSheet(parent)
    , m_strHref(href)
    , m_cachedSheet(0)
    , m_loading(false)
{
}

XSLImportRule::~XSLImportRule()
{
    if (m_styleSheet)
        m_styleSheet->setParentStyleSheet(0);
    
    if (m_cachedSheet)
        m_cachedSheet->removeClient(this);
}

void XSLImportRule::setXSLStyleSheet(const String& href, const KURL& baseURL, const String& sheet)
{
    if (m_styleSheet)
        m_styleSheet->setParentStyleSheet(0);

    m_styleSheet = XSLStyleSheet::create(this, href, baseURL);

    XSLStyleSheet* parent = parentStyleSheet();
    if (parent)
        m_styleSheet->setParentStyleSheet(parent);

    m_styleSheet->parseString(sheet);
    m_loading = false;
    
    if (parent)
        parent->checkLoaded();
}

bool XSLImportRule::isLoading()
{
    return (m_loading || (m_styleSheet && m_styleSheet->isLoading()));
}

void XSLImportRule::loadSheet()
{
    CachedResourceLoader* cachedResourceLoader = 0;

    XSLStyleSheet* rootSheet = parentStyleSheet();

    if (rootSheet) {
        while (XSLStyleSheet* parentSheet = rootSheet->parentStyleSheet())
            rootSheet = parentSheet;
    }

    if (rootSheet)
        cachedResourceLoader = rootSheet->cachedResourceLoader();
    
    String absHref = m_strHref;
    XSLStyleSheet* parentSheet = parentStyleSheet();
    if (!parentSheet->baseURL().isNull())
        // use parent styleheet's URL as the base URL
        absHref = KURL(parentSheet->baseURL(), m_strHref).string();
    
    // Check for a cycle in our import chain.  If we encounter a stylesheet
    // in our parent chain with the same URL, then just bail.
    for (XSLStyleSheet* parentSheet = parentStyleSheet(); parentSheet; parentSheet = parentSheet->parentStyleSheet()) {
        if (absHref == parentSheet->baseURL().string())
            return;
    }
    
    CachedResourceRequest request(ResourceRequest(cachedResourceLoader->document()->completeURL(absHref)));
    m_cachedSheet = cachedResourceLoader->requestXSLStyleSheet(request);
    
    if (m_cachedSheet) {
        m_cachedSheet->addClient(this);
        
        // If the imported sheet is in the cache, then setXSLStyleSheet gets called,
        // and the sheet even gets parsed (via parseString).  In this case we have
        // loaded (even if our subresources haven't), so if we have a stylesheet after
        // checking the cache, then we've clearly loaded.
        if (!m_styleSheet)
            m_loading = true;
    }
}

} // namespace WebCore

#endif // ENABLE(XSLT)
