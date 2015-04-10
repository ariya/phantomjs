/*
 * Copyright (C) 2013 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(VIDEO_TRACK)

#include "WebVTTElement.h"

#include "HTMLElementFactory.h"
#include "TextTrack.h"

namespace WebCore {

static const QualifiedName& nodeTypeToTagName(WebVTTNodeType nodeType)
{
    DEFINE_STATIC_LOCAL(QualifiedName, cTag, (nullAtom, "c", nullAtom));
    DEFINE_STATIC_LOCAL(QualifiedName, vTag, (nullAtom, "v", nullAtom));
    DEFINE_STATIC_LOCAL(QualifiedName, langTag, (nullAtom, "lang", nullAtom));
    DEFINE_STATIC_LOCAL(QualifiedName, bTag, (nullAtom, "b", nullAtom));
    DEFINE_STATIC_LOCAL(QualifiedName, uTag, (nullAtom, "u", nullAtom));
    DEFINE_STATIC_LOCAL(QualifiedName, iTag, (nullAtom, "i", nullAtom));
    DEFINE_STATIC_LOCAL(QualifiedName, rubyTag, (nullAtom, "ruby", nullAtom));
    DEFINE_STATIC_LOCAL(QualifiedName, rtTag, (nullAtom, "rt", nullAtom));
    switch (nodeType) {
    case WebVTTNodeTypeClass:
        return cTag;
    case WebVTTNodeTypeItalic:
        return iTag;
    case WebVTTNodeTypeLanguage:
        return langTag;
    case WebVTTNodeTypeBold:
        return bTag;
    case WebVTTNodeTypeUnderline:
        return uTag;
    case WebVTTNodeTypeRuby:
        return rubyTag;
    case WebVTTNodeTypeRubyText:
        return rtTag;
    case WebVTTNodeTypeVoice:
        return vTag;
    case WebVTTNodeTypeNone:
    default:
        ASSERT_NOT_REACHED();
        return cTag; // Make the compiler happy.
    }
}

WebVTTElement::WebVTTElement(WebVTTNodeType nodeType, Document* document)
    : Element(nodeTypeToTagName(nodeType), document, CreateElement)
    , m_isPastNode(0)
    , m_webVTTNodeType(nodeType)
{
}

PassRefPtr<WebVTTElement> WebVTTElement::create(WebVTTNodeType nodeType, Document* document)
{
    return adoptRef(new WebVTTElement(nodeType, document));
}

PassRefPtr<Element> WebVTTElement::cloneElementWithoutAttributesAndChildren()
{
    RefPtr<WebVTTElement> clone = create(static_cast<WebVTTNodeType>(m_webVTTNodeType), document());
    clone->setLanguage(m_language);
    return clone;
}

PassRefPtr<HTMLElement> WebVTTElement::createEquivalentHTMLElement(Document* document)
{
    RefPtr<HTMLElement> htmlElement;
    switch (m_webVTTNodeType) {
    case WebVTTNodeTypeClass:
    case WebVTTNodeTypeLanguage:
    case WebVTTNodeTypeVoice:
        htmlElement = HTMLElementFactory::createHTMLElement(HTMLNames::spanTag, document);
        htmlElement.get()->setAttribute(HTMLNames::titleAttr, getAttribute(voiceAttributeName()));
        htmlElement.get()->setAttribute(HTMLNames::langAttr, getAttribute(langAttributeName()));
        break;
    case WebVTTNodeTypeItalic:
        htmlElement = HTMLElementFactory::createHTMLElement(HTMLNames::iTag, document);
        break;
    case WebVTTNodeTypeBold:
        htmlElement = HTMLElementFactory::createHTMLElement(HTMLNames::bTag, document);
        break;
    case WebVTTNodeTypeUnderline:
        htmlElement = HTMLElementFactory::createHTMLElement(HTMLNames::uTag, document);
        break;
    case WebVTTNodeTypeRuby:
        htmlElement = HTMLElementFactory::createHTMLElement(HTMLNames::rubyTag, document);
        break;
    case WebVTTNodeTypeRubyText:
        htmlElement = HTMLElementFactory::createHTMLElement(HTMLNames::rtTag, document);
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    htmlElement.get()->setAttribute(HTMLNames::classAttr, getAttribute(HTMLNames::classAttr));
    return htmlElement;
}

} // namespace WebCore

#endif
