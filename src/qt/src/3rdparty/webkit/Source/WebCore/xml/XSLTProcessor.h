/*
 * This file is part of the XSL implementation.
 *
 * Copyright (C) 2004, 2007, 2008 Apple, Inc. All rights reserved.
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

#ifndef XSLTProcessor_h
#define XSLTProcessor_h

#if ENABLE(XSLT)

#include "Node.h"
#include "XSLStyleSheet.h"
#include <wtf/HashMap.h>
#include <wtf/text/StringHash.h>

#if !USE(QXMLQUERY)
#include <libxml/parserInternals.h>
#include <libxslt/documents.h>
#endif

namespace WebCore {

class Frame;
class Document;
class DocumentFragment;

class XSLTProcessor : public RefCounted<XSLTProcessor> {
public:
    static PassRefPtr<XSLTProcessor> create() { return adoptRef(new XSLTProcessor); }
    ~XSLTProcessor();

    void setXSLStyleSheet(PassRefPtr<XSLStyleSheet> styleSheet) { m_stylesheet = styleSheet; }
    bool transformToString(Node* source, String& resultMIMEType, String& resultString, String& resultEncoding);
    PassRefPtr<Document> createDocumentFromSource(const String& source, const String& sourceEncoding, const String& sourceMIMEType, Node* sourceNode, Frame* frame);
    
    // DOM methods
    void importStylesheet(PassRefPtr<Node> style) { m_stylesheetRootNode = style; }
    PassRefPtr<DocumentFragment> transformToFragment(Node* source, Document* ouputDoc);
    PassRefPtr<Document> transformToDocument(Node* source);
    
    void setParameter(const String& namespaceURI, const String& localName, const String& value);
    String getParameter(const String& namespaceURI, const String& localName) const;
    void removeParameter(const String& namespaceURI, const String& localName);
    void clearParameters() { m_parameters.clear(); }

    void reset();

#if !USE(QXMLQUERY)
    static void parseErrorFunc(void* userData, xmlError*);
    static void genericErrorFunc(void* userData, const char* msg, ...);
    
    // Only for libXSLT callbacks
    XSLStyleSheet* xslStylesheet() const { return m_stylesheet.get(); }
#endif

    typedef HashMap<String, String> ParameterMap;

private:
    XSLTProcessor() { }

    RefPtr<XSLStyleSheet> m_stylesheet;
    RefPtr<Node> m_stylesheetRootNode;
    ParameterMap m_parameters;
};

}

#endif
#endif
