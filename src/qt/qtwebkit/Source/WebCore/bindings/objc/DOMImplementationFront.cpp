/*
 * Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
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

#include "config.h"
#include "DOMImplementationFront.h"

#include "CSSStyleSheet.h"
#include "DocumentType.h"
#include "DOMImplementation.h"
#include "HTMLDocument.h"
#include "JSDOMImplementation.h"

namespace WebCore {

DOMImplementationFront* implementationFront(Document* document)
{
    return reinterpret_cast<DOMImplementationFront*>(document->implementation());
}

DOMImplementationFront* implementationFront(JSDOMImplementation* wrapper)
{
    return reinterpret_cast<DOMImplementationFront*>(wrapper->impl());
}

void DOMImplementationFront::ref()
{
    reinterpret_cast<DOMImplementation*>(this)->ref();
}

void DOMImplementationFront::deref()
{
    reinterpret_cast<DOMImplementation*>(this)->deref();
}

bool DOMImplementationFront::hasFeature(const String& feature, const String& version) const
{
    return reinterpret_cast<const DOMImplementation*>(this)->hasFeature(feature, version);
}

PassRefPtr<DocumentType> DOMImplementationFront::createDocumentType(const String& qualifiedName, const String& publicId, const String& systemId, ExceptionCode& ec)
{
    return reinterpret_cast<DOMImplementation*>(this)->createDocumentType(qualifiedName, publicId, systemId, ec);
}

PassRefPtr<Document> DOMImplementationFront::createDocument(const String& namespaceURI, const String& qualifiedName, DocumentType* type, ExceptionCode& ec)
{
    return reinterpret_cast<DOMImplementation*>(this)->createDocument(namespaceURI, qualifiedName, type, ec);
}

DOMImplementationFront* DOMImplementationFront::getInterface(const String& feature)
{
    return reinterpret_cast<DOMImplementationFront*>(reinterpret_cast<DOMImplementation*>(this)->getInterface(feature));
}

PassRefPtr<CSSStyleSheet> DOMImplementationFront::createCSSStyleSheet(const String& title, const String& media, ExceptionCode& ec)
{
    return reinterpret_cast<DOMImplementation*>(this)->createCSSStyleSheet(title, media, ec);
}

PassRefPtr<HTMLDocument> DOMImplementationFront::createHTMLDocument(const String& title)
{
    return reinterpret_cast<DOMImplementation*>(this)->createHTMLDocument(title);
}

} //namespace
