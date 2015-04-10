/*
 * Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Samuel Weinig <sam.weinig@gmail.com>
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

#ifndef DOMImplementationFront_h
#define DOMImplementationFront_h

// FIXME: This source file exists to work around a problem that occurs trying
// to mix DOMImplementation and WebCore::DOMImplementation in DOM.mm.
// It seems to affect only older versions of gcc. Once the buildbot is upgraded,
// we should consider increasing the minimum required version of gcc to build
// WebCore, and rolling the change that introduced this file back.

#include <wtf/Forward.h>

namespace WebCore {

class CSSStyleSheet;
class Document;
class DocumentType;
class HTMLDocument;
class JSDOMImplementation;

typedef int ExceptionCode;

class DOMImplementationFront {
public:
    void ref();
    void deref();
    bool hasFeature(const String& feature, const String& version) const;
    PassRefPtr<DocumentType> createDocumentType(const String& qualifiedName, const String& publicId, const String& systemId, ExceptionCode&);
    PassRefPtr<Document> createDocument(const String& namespaceURI, const String& qualifiedName, DocumentType*, ExceptionCode&);
    DOMImplementationFront* getInterface(const String& feature);
    PassRefPtr<CSSStyleSheet> createCSSStyleSheet(const String& title, const String& media, ExceptionCode&);
    PassRefPtr<HTMLDocument> createHTMLDocument(const String& title);
};

DOMImplementationFront* implementationFront(Document*);
DOMImplementationFront* implementationFront(JSDOMImplementation*);

} // namespace WebCore

#endif // DOMImplementationFront_h
