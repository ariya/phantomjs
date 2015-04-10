/*
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#if ENABLE(SVG_FONTS)
#include "SVGFontFaceUriElement.h"

#include "Attribute.h"
#include "CSSFontFaceSrcValue.h"
#include "CachedFont.h"
#include "CachedResourceLoader.h"
#include "CachedResourceRequest.h"
#include "Document.h"
#include "SVGFontFaceElement.h"
#include "SVGNames.h"
#include "XLinkNames.h"

namespace WebCore {
    
using namespace SVGNames;
    
inline SVGFontFaceUriElement::SVGFontFaceUriElement(const QualifiedName& tagName, Document* document)
    : SVGElement(tagName, document)
{
    ASSERT(hasTagName(font_face_uriTag));
}

PassRefPtr<SVGFontFaceUriElement> SVGFontFaceUriElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGFontFaceUriElement(tagName, document));
}

SVGFontFaceUriElement::~SVGFontFaceUriElement()
{
    if (m_cachedFont)
        m_cachedFont->removeClient(this);
}

PassRefPtr<CSSFontFaceSrcValue> SVGFontFaceUriElement::srcValue() const
{
    RefPtr<CSSFontFaceSrcValue> src = CSSFontFaceSrcValue::create(getAttribute(XLinkNames::hrefAttr));
    AtomicString value(fastGetAttribute(formatAttr));
    src->setFormat(value.isEmpty() ? "svg" : value); // Default format
    return src.release();
}

void SVGFontFaceUriElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == XLinkNames::hrefAttr)
        loadFont();
    else
        SVGElement::parseAttribute(name, value);
}

void SVGFontFaceUriElement::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    SVGElement::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);

    if (!parentNode() || !parentNode()->hasTagName(font_face_srcTag))
        return;
    
    ContainerNode* grandparent = parentNode()->parentNode();
    if (grandparent && grandparent->hasTagName(font_faceTag))
        static_cast<SVGFontFaceElement*>(grandparent)->rebuildFontFace();
}

Node::InsertionNotificationRequest SVGFontFaceUriElement::insertedInto(ContainerNode* rootParent)
{
    loadFont();
    return SVGElement::insertedInto(rootParent);
}

void SVGFontFaceUriElement::loadFont()
{
    if (m_cachedFont)
        m_cachedFont->removeClient(this);

    const AtomicString& href = getAttribute(XLinkNames::hrefAttr);
    if (!href.isNull()) {
        CachedResourceLoader* cachedResourceLoader = document()->cachedResourceLoader();
        CachedResourceRequest request(ResourceRequest(document()->completeURL(href)));
        request.setInitiator(this);
        m_cachedFont = cachedResourceLoader->requestFont(request);
        if (m_cachedFont) {
            m_cachedFont->addClient(this);
            m_cachedFont->beginLoadIfNeeded(cachedResourceLoader);
        }
    } else
        m_cachedFont = 0;
}

}

#endif // ENABLE(SVG_FONTS)
