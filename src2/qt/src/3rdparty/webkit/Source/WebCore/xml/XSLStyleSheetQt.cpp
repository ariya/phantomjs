/*
 * This file is part of the XSL implementation.
 *
 * Copyright (C) 2009 Jakub Wieczorek <faw217@gmail.com>
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
#include "XSLStyleSheet.h"

#if ENABLE(XSLT)

#include "DOMWindow.h"
#include "Document.h"
#include "Node.h"
#include "NotImplemented.h"
#include "XSLTProcessor.h"

namespace WebCore {

XSLStyleSheet::XSLStyleSheet(Node* parentNode, const String& originalURL, const KURL& finalURL,  bool embedded)
    : StyleSheet(parentNode, originalURL, finalURL)
    , m_embedded(embedded)
{
}

XSLStyleSheet::~XSLStyleSheet()
{
}

bool XSLStyleSheet::isLoading()
{
    notImplemented();
    return false;
}

void XSLStyleSheet::checkLoaded()
{
    if (ownerNode())
        ownerNode()->sheetLoaded();
}

void XSLStyleSheet::clearDocuments()
{
    notImplemented();
}

CachedResourceLoader* XSLStyleSheet::cachedResourceLoader()
{
    Document* document = ownerDocument();
    if (!document)
        return 0;
    return document->cachedResourceLoader();
}

bool XSLStyleSheet::parseString(const String& string, bool)
{
    // FIXME: Fix QXmlQuery so that it allows compiling the stylesheet before setting the document
    // to be transformed. This way we could not only check if the stylesheet is correct before using it
    // but also turn XSLStyleSheet::sheetString() into XSLStyleSheet::query() that returns a QXmlQuery.

    m_sheetString = string;
    return !m_sheetString.isEmpty();
}

void XSLStyleSheet::loadChildSheets()
{
    notImplemented();
}

void XSLStyleSheet::loadChildSheet(const String&)
{
    notImplemented();
}

Document* XSLStyleSheet::ownerDocument()
{
    Node* node = ownerNode();
    return node ? node->document() : 0;
}

void XSLStyleSheet::setParentStyleSheet(XSLStyleSheet*)
{
    notImplemented();
}

void XSLStyleSheet::markAsProcessed()
{
    notImplemented();
}

} // namespace WebCore

#endif // ENABLE(XSLT)
