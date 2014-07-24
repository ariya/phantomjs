/*
 * Copyright (C) 2012 Company 100, Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GOOGLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef HTMLStackItem_h
#define HTMLStackItem_h

#include "AtomicHTMLToken.h"
#include "Element.h"
#include "HTMLNames.h"
#include "MathMLNames.h"
#include "SVGNames.h"

#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {

class ContainerNode;

class HTMLStackItem : public RefCounted<HTMLStackItem> {
public:
    enum ItemType {
        ItemForContextElement,
        ItemForDocumentFragmentNode
    };

    // Used by document fragment node and context element.
    static PassRefPtr<HTMLStackItem> create(PassRefPtr<ContainerNode> node, ItemType type)
    {
        return adoptRef(new HTMLStackItem(node, type));
    }

    // Used by HTMLElementStack and HTMLFormattingElementList.
    static PassRefPtr<HTMLStackItem> create(PassRefPtr<ContainerNode> node, AtomicHTMLToken* token, const AtomicString& namespaceURI = HTMLNames::xhtmlNamespaceURI)
    {
        return adoptRef(new HTMLStackItem(node, token, namespaceURI));
    }

    Element* element() const { return toElement(m_node.get()); }
    ContainerNode* node() const { return m_node.get(); }

    bool isDocumentFragmentNode() const { return m_isDocumentFragmentNode; }
    bool isElementNode() const { return !m_isDocumentFragmentNode; }

    const AtomicString& namespaceURI() const { return m_namespaceURI; }
    const AtomicString& localName() const { return m_tokenLocalName; }

    const Vector<Attribute>& attributes() const { ASSERT(m_tokenLocalName); return m_tokenAttributes; }
    Attribute* getAttributeItem(const QualifiedName& attributeName)
    {
        ASSERT(m_tokenLocalName);
        return findAttributeInVector(m_tokenAttributes, attributeName);
    }

    bool hasLocalName(const AtomicString& name) const { return m_tokenLocalName == name; }
    bool hasTagName(const QualifiedName& name) const { return m_tokenLocalName == name.localName() && m_namespaceURI == name.namespaceURI(); }

    bool matchesHTMLTag(const AtomicString& name) const { return m_tokenLocalName == name && m_namespaceURI == HTMLNames::xhtmlNamespaceURI; }
    bool matchesHTMLTag(const QualifiedName& name) const { return m_tokenLocalName == name && m_namespaceURI == HTMLNames::xhtmlNamespaceURI; }

    bool causesFosterParenting()
    {
        return hasTagName(HTMLNames::tableTag)
            || hasTagName(HTMLNames::tbodyTag)
            || hasTagName(HTMLNames::tfootTag)
            || hasTagName(HTMLNames::theadTag)
            || hasTagName(HTMLNames::trTag);
    }

    bool isInHTMLNamespace() const
    {
        // A DocumentFragment takes the place of the document element when parsing
        // fragments and should be considered in the HTML namespace.
        return namespaceURI() == HTMLNames::xhtmlNamespaceURI
            || isDocumentFragmentNode(); // FIXME: Does this also apply to ShadowRoot?
    }

    bool isNumberedHeaderElement() const
    {
        return hasTagName(HTMLNames::h1Tag)
            || hasTagName(HTMLNames::h2Tag)
            || hasTagName(HTMLNames::h3Tag)
            || hasTagName(HTMLNames::h4Tag)
            || hasTagName(HTMLNames::h5Tag)
            || hasTagName(HTMLNames::h6Tag);
    }

    bool isTableBodyContextElement() const
    {
        return hasTagName(HTMLNames::tbodyTag)
            || hasTagName(HTMLNames::tfootTag)
            || hasTagName(HTMLNames::theadTag);
    }

    // http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#special
    bool isSpecialNode() const
    {
        if (hasTagName(MathMLNames::miTag)
            || hasTagName(MathMLNames::moTag)
            || hasTagName(MathMLNames::mnTag)
            || hasTagName(MathMLNames::msTag)
            || hasTagName(MathMLNames::mtextTag)
            || hasTagName(MathMLNames::annotation_xmlTag)
            || hasTagName(SVGNames::foreignObjectTag)
            || hasTagName(SVGNames::descTag)
            || hasTagName(SVGNames::titleTag))
            return true;
        if (isDocumentFragmentNode())
            return true;
        if (!isInHTMLNamespace())
            return false;
        const AtomicString& tagName = localName();
        return tagName == HTMLNames::addressTag
            || tagName == HTMLNames::appletTag
            || tagName == HTMLNames::areaTag
            || tagName == HTMLNames::articleTag
            || tagName == HTMLNames::asideTag
            || tagName == HTMLNames::baseTag
            || tagName == HTMLNames::basefontTag
            || tagName == HTMLNames::bgsoundTag
            || tagName == HTMLNames::blockquoteTag
            || tagName == HTMLNames::bodyTag
            || tagName == HTMLNames::brTag
            || tagName == HTMLNames::buttonTag
            || tagName == HTMLNames::captionTag
            || tagName == HTMLNames::centerTag
            || tagName == HTMLNames::colTag
            || tagName == HTMLNames::colgroupTag
            || tagName == HTMLNames::commandTag
            || tagName == HTMLNames::ddTag
            || tagName == HTMLNames::detailsTag
            || tagName == HTMLNames::dirTag
            || tagName == HTMLNames::divTag
            || tagName == HTMLNames::dlTag
            || tagName == HTMLNames::dtTag
            || tagName == HTMLNames::embedTag
            || tagName == HTMLNames::fieldsetTag
            || tagName == HTMLNames::figcaptionTag
            || tagName == HTMLNames::figureTag
            || tagName == HTMLNames::footerTag
            || tagName == HTMLNames::formTag
            || tagName == HTMLNames::frameTag
            || tagName == HTMLNames::framesetTag
            || isNumberedHeaderElement()
            || tagName == HTMLNames::headTag
            || tagName == HTMLNames::headerTag
            || tagName == HTMLNames::hgroupTag
            || tagName == HTMLNames::hrTag
            || tagName == HTMLNames::htmlTag
            || tagName == HTMLNames::iframeTag
            || tagName == HTMLNames::imgTag
            || tagName == HTMLNames::inputTag
            || tagName == HTMLNames::isindexTag
            || tagName == HTMLNames::liTag
            || tagName == HTMLNames::linkTag
            || tagName == HTMLNames::listingTag
            || tagName == HTMLNames::mainTag
            || tagName == HTMLNames::marqueeTag
            || tagName == HTMLNames::menuTag
            || tagName == HTMLNames::metaTag
            || tagName == HTMLNames::navTag
            || tagName == HTMLNames::noembedTag
            || tagName == HTMLNames::noframesTag
            || tagName == HTMLNames::noscriptTag
            || tagName == HTMLNames::objectTag
            || tagName == HTMLNames::olTag
            || tagName == HTMLNames::pTag
            || tagName == HTMLNames::paramTag
            || tagName == HTMLNames::plaintextTag
            || tagName == HTMLNames::preTag
            || tagName == HTMLNames::scriptTag
            || tagName == HTMLNames::sectionTag
            || tagName == HTMLNames::selectTag
            || tagName == HTMLNames::styleTag
            || tagName == HTMLNames::summaryTag
            || tagName == HTMLNames::tableTag
            || isTableBodyContextElement()
            || tagName == HTMLNames::tdTag
#if ENABLE(TEMPLATE_ELEMENT)
            || tagName == HTMLNames::templateTag
#endif
            || tagName == HTMLNames::textareaTag
            || tagName == HTMLNames::thTag
            || tagName == HTMLNames::titleTag
            || tagName == HTMLNames::trTag
            || tagName == HTMLNames::ulTag
            || tagName == HTMLNames::wbrTag
            || tagName == HTMLNames::xmpTag;
    }

private:
    HTMLStackItem(PassRefPtr<ContainerNode> node, ItemType type)
        : m_node(node)
    {
        switch (type) {
        case ItemForDocumentFragmentNode:
            m_isDocumentFragmentNode = true;
            break;
        case ItemForContextElement:
            m_tokenLocalName = m_node->localName();
            m_namespaceURI = m_node->namespaceURI();
            m_isDocumentFragmentNode = false;
            break;
        }
    }

    HTMLStackItem(PassRefPtr<ContainerNode> node, AtomicHTMLToken* token, const AtomicString& namespaceURI = HTMLNames::xhtmlNamespaceURI)
        : m_node(node)
        , m_tokenLocalName(token->name())
        , m_tokenAttributes(token->attributes())
        , m_namespaceURI(namespaceURI)
        , m_isDocumentFragmentNode(false)
    {
    }

    RefPtr<ContainerNode> m_node;

    AtomicString m_tokenLocalName;
    Vector<Attribute> m_tokenAttributes;
    AtomicString m_namespaceURI;
    bool m_isDocumentFragmentNode;
};

} // namespace WebCore

#endif // HTMLStackItem_h
