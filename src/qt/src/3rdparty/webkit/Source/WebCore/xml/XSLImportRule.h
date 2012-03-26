/*
 * This file is part of the XSL implementation.
 *
 * Copyright (C) 2004, 2006, 2008 Apple Inc. All rights reserved.
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

#ifndef XSLImportRule_h
#define XSLImportRule_h

#if ENABLE(XSLT)

#include "CachedResourceClient.h"
#include "CachedResourceHandle.h"
#include "StyleBase.h"
#include "XSLStyleSheet.h"

namespace WebCore {

class CachedXSLStyleSheet;

class XSLImportRule : public StyleBase, private CachedResourceClient {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassRefPtr<XSLImportRule> create(XSLStyleSheet* parentSheet, const String& href)
    {
        return adoptRef(new XSLImportRule(parentSheet, href));
    }

    virtual ~XSLImportRule();
    
    const String& href() const { return m_strHref; }
    XSLStyleSheet* styleSheet() const { return m_styleSheet.get(); }

    XSLStyleSheet* parentStyleSheet() const;

    bool isLoading();
    void loadSheet();
    
private:
    XSLImportRule(XSLStyleSheet* parentSheet, const String& href);

    virtual bool isImportRule() { return true; }

    // from CachedResourceClient
    virtual void setXSLStyleSheet(const String& href, const KURL& baseURL, const String& sheet);
    
    String m_strHref;
    RefPtr<XSLStyleSheet> m_styleSheet;
    CachedResourceHandle<CachedXSLStyleSheet> m_cachedSheet;
    bool m_loading;
};

} // namespace WebCore

#endif // ENABLE(XSLT)

#endif // XSLImportRule_h
