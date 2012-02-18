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

#ifndef XSLStyleSheet_h
#define XSLStyleSheet_h

#if ENABLE(XSLT)

#include "ProcessingInstruction.h"
#include "StyleSheet.h"

#if !USE(QXMLQUERY)
#include <libxml/parser.h>
#include <libxslt/transform.h>
#endif

#include <wtf/PassRefPtr.h>

namespace WebCore {

class XSLImportRule;
    
class XSLStyleSheet : public StyleSheet {
public:
#if !USE(QXMLQUERY)
    static PassRefPtr<XSLStyleSheet> create(XSLImportRule* parentImport, const String& originalURL, const KURL& finalURL)
    {
        return adoptRef(new XSLStyleSheet(parentImport, originalURL, finalURL));
    }
#endif
    static PassRefPtr<XSLStyleSheet> create(ProcessingInstruction* parentNode, const String& originalURL, const KURL& finalURL)
    {
        return adoptRef(new XSLStyleSheet(parentNode, originalURL, finalURL, false));
    }
    static PassRefPtr<XSLStyleSheet> createEmbedded(ProcessingInstruction* parentNode, const KURL& finalURL)
    {
        return adoptRef(new XSLStyleSheet(parentNode, finalURL.string(), finalURL, true));
    }

    // Taking an arbitrary node is unsafe, because owner node pointer can become stale.
    // XSLTProcessor ensures that the stylesheet doesn't outlive its parent, in part by not exposing it to JavaScript.
    static PassRefPtr<XSLStyleSheet> createForXSLTProcessor(Node* parentNode, const String& originalURL, const KURL& finalURL)
    {
        return adoptRef(new XSLStyleSheet(parentNode, originalURL, finalURL, false));
    }

    virtual ~XSLStyleSheet();
    
    virtual bool isXSLStyleSheet() const { return true; }

    virtual String type() const { return "text/xml"; }

    virtual bool parseString(const String &string, bool strict = true);
    
    virtual bool isLoading();
    virtual void checkLoaded();

    void loadChildSheets();
    void loadChildSheet(const String& href);

    CachedResourceLoader* cachedResourceLoader();

    Document* ownerDocument();
    XSLStyleSheet* parentStyleSheet() const { return m_parentStyleSheet; }
    void setParentStyleSheet(XSLStyleSheet* parent);

#if USE(QXMLQUERY)
    String sheetString() const { return m_sheetString; }
#else
    xmlDocPtr document();
    xsltStylesheetPtr compileStyleSheet();
    xmlDocPtr locateStylesheetSubResource(xmlDocPtr parentDoc, const xmlChar* uri);
#endif

    void clearDocuments();

    void markAsProcessed();
    bool processed() const { return m_processed; }

private:
    XSLStyleSheet(Node* parentNode, const String& originalURL, const KURL& finalURL, bool embedded);
#if !USE(QXMLQUERY)
    XSLStyleSheet(XSLImportRule* parentImport, const String& originalURL, const KURL& finalURL);
#endif

    Document* m_ownerDocument;
    bool m_embedded;
    bool m_processed;

#if USE(QXMLQUERY)
    String m_sheetString;
#else
    xmlDocPtr m_stylesheetDoc;
    bool m_stylesheetDocTaken;
#endif
    
    XSLStyleSheet* m_parentStyleSheet;
};

} // namespace WebCore

#endif // ENABLE(XSLT)

#endif // XSLStyleSheet_h
