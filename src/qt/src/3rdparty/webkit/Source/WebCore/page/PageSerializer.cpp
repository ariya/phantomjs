/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PageSerializer.h"

#include "CSSImageValue.h"
#include "CSSImportRule.h"
#include "CSSStyleRule.h"
#include "CachedImage.h"
#include "Document.h"
#include "Element.h"
#include "Frame.h"
#include "HTMLFrameOwnerElement.h"
#include "HTMLHeadElement.h"
#include "HTMLImageElement.h"
#include "HTMLLinkElement.h"
#include "HTMLMetaCharsetParser.h"
#include "HTMLNames.h"
#include "HTMLStyleElement.h"
#include "HTTPParsers.h"
#include "Image.h"
#include "MIMETypeRegistry.h"
#include "MarkupAccumulator.h"
#include "Page.h"
#include "StyleCachedImage.h"
#include "StyleImage.h"
#include "Text.h"
#include "TextEncoding.h"
#include <wtf/text/StringBuilder.h>
#include <wtf/text/StringConcatenate.h>

namespace WebCore {

static bool isCharsetSpecifyingNode(Node* node)
{
    if (!node->isHTMLElement())
        return false;

    HTMLElement* element = toHTMLElement(node);
    if (!element->hasTagName(HTMLNames::metaTag))
        return false;
    HTMLMetaCharsetParser::AttributeList attributes;
    const NamedNodeMap* attributesMap = element->attributes(true);
    for (unsigned i = 0; i < attributesMap->length(); ++i) {
        Attribute* item = attributesMap->attributeItem(i);
        // FIXME: We should deal appropriately with the attribute if they have a namespace.
        attributes.append(make_pair(item->name().toString(), item->value().string()));
    }
    TextEncoding textEncoding = HTMLMetaCharsetParser::encodingFromMetaAttributes(attributes);
    return textEncoding.isValid();
}

static bool shouldIgnoreElement(Element* element)
{
    return element->hasTagName(HTMLNames::scriptTag) || element->hasTagName(HTMLNames::noscriptTag) || isCharsetSpecifyingNode(element);
}

static const QualifiedName& frameOwnerURLAttributeName(const HTMLFrameOwnerElement& frameOwner)
{
    // FIXME: We should support all frame owners including applets.
    return frameOwner.hasTagName(HTMLNames::objectTag) ? HTMLNames::dataAttr : HTMLNames::srcAttr;
}

class SerializerMarkupAccumulator : public WebCore::MarkupAccumulator {
public:
    SerializerMarkupAccumulator(PageSerializer*, Document*, Vector<Node*>*);
    virtual ~SerializerMarkupAccumulator();

protected:
    virtual void appendText(Vector<UChar>& out, Text*);
    virtual void appendElement(Vector<UChar>& out, Element*, Namespaces*);
    virtual void appendCustomAttributes(Vector<UChar>& out, Element*, Namespaces*);
    virtual void appendEndTag(Node*);

private:
    PageSerializer* m_serializer;
    Document* m_document;
};

SerializerMarkupAccumulator::SerializerMarkupAccumulator(PageSerializer* serializer, Document* document, Vector<Node*>* nodes)
    : MarkupAccumulator(nodes, AbsoluteURLs)
    , m_serializer(serializer)
    , m_document(document)
{
    // MarkupAccumulator does not serialize the <?xml ... line, so we add it explicitely to ensure the right encoding is specified.
    if (m_document->isXHTMLDocument() || m_document->xmlStandalone() || m_document->isSVGDocument())
        appendString(makeString("<?xml version=\"", m_document->xmlVersion(), "\" encoding=\"", m_document->charset(), "\"?>"));
}

SerializerMarkupAccumulator::~SerializerMarkupAccumulator()
{
}

void SerializerMarkupAccumulator::appendText(Vector<UChar>& out, Text* text)
{
    Element* parent = text->parentElement();
    if (parent && !shouldIgnoreElement(parent))
        MarkupAccumulator::appendText(out, text);
}

void SerializerMarkupAccumulator::appendElement(Vector<UChar>& out, Element* element, Namespaces* namespaces)
{
    if (!shouldIgnoreElement(element))
        MarkupAccumulator::appendElement(out, element, namespaces);

    if (element->hasTagName(HTMLNames::headTag)) {
        String meta = makeString("<meta charset=\"", m_document->charset(), "\">");
        out.append(meta.characters(), meta.length());
    }

    // FIXME: For object (plugins) tags and video tag we could replace them by an image of their current contents.
}

void SerializerMarkupAccumulator::appendCustomAttributes(Vector<UChar>& out, Element* element, Namespaces* namespaces)
{
    if (!element->isFrameOwnerElement())
        return;

    HTMLFrameOwnerElement* frameOwner = static_cast<HTMLFrameOwnerElement*>(element);
    Frame* frame = frameOwner->contentFrame();
    if (!frame)
        return;

    KURL url = frame->document()->url();
    if (url.isValid() && !url.protocolIs("about"))
        return;

    // We need to give a fake location to blank frames so they can be referenced by the serialized frame.
    url = m_serializer->urlForBlankFrame(frame);
    RefPtr<Attribute> attribute = Attribute::create(frameOwnerURLAttributeName(*frameOwner), url.string());
    appendAttribute(out, element, *attribute, namespaces);
}

void SerializerMarkupAccumulator::appendEndTag(Node* node)
{
    if (node->isElementNode() && !shouldIgnoreElement(toElement(node)))
        MarkupAccumulator::appendEndTag(node);
}

PageSerializer::Resource::Resource()
{
}

PageSerializer::Resource::Resource(const KURL& url, const String& mimeType, PassRefPtr<SharedBuffer> data)
    : url(url)
    , mimeType(mimeType)
    , data(data)
{
}

PageSerializer::PageSerializer(Vector<PageSerializer::Resource>* resources)
    : m_resources(resources)
    , m_blankFrameCounter(0)
{
}

void PageSerializer::serialize(Page* page)
{
    serializeFrame(page->mainFrame());
}

void PageSerializer::serializeFrame(Frame* frame)
{
    Document* document = frame->document();
    KURL url = document->url();
    if (!url.isValid() || url.protocolIs("about")) {
        // For blank frames we generate a fake URL so they can be referenced by their containing frame.
        url = urlForBlankFrame(frame);
    }

    if (m_resourceURLs.contains(url)) {
        // FIXME: We could have 2 frame with the same URL but which were dynamically changed and have now
        // different content. So we should serialize both and somehow rename the frame src in the containing
        // frame. Arg!
        return;
    }

    Vector<Node*> nodes;
    SerializerMarkupAccumulator accumulator(this, document, &nodes);
    TextEncoding textEncoding(TextEncoding(document->charset()));
    ASSERT(textEncoding.isValid());
    String text = accumulator.serializeNodes(document->documentElement(), 0, IncludeNode);
    CString frameHTML = textEncoding.encode(text.characters(), text.length(), EntitiesForUnencodables);
    m_resources->append(Resource(url, document->suggestedMIMEType(), SharedBuffer::create(frameHTML.data(), frameHTML.length())));
    m_resourceURLs.add(url);

    for (Vector<Node*>::iterator iter = nodes.begin(); iter != nodes.end(); ++iter) {
        Node* node = *iter;
        if (!node->isElementNode())
            continue;

        Element* element = toElement(node);
        // We have to process in-line style as it might contain some resources (typically background images).
        retrieveResourcesForCSSDeclaration(element->style());

        if (element->hasTagName(HTMLNames::imgTag)) {
            HTMLImageElement* imageElement = static_cast<HTMLImageElement*>(element);
            KURL url = document->completeURL(imageElement->getAttribute(HTMLNames::srcAttr));
            CachedImage* cachedImage = imageElement->cachedImage();
            addImageToResources(cachedImage, url);
        } else if (element->hasTagName(HTMLNames::linkTag)) {
            HTMLLinkElement* linkElement = static_cast<HTMLLinkElement*>(element);
            StyleSheet* sheet = linkElement->sheet();
            if (sheet && sheet->isCSSStyleSheet()) {
                KURL url = document->completeURL(linkElement->getAttribute(HTMLNames::hrefAttr));
                serializeCSSStyleSheet(static_cast<CSSStyleSheet*>(sheet), url);
                ASSERT(m_resourceURLs.contains(url));
            }
        } else if (element->hasTagName(HTMLNames::styleTag)) {
            HTMLStyleElement* styleElement = static_cast<HTMLStyleElement*>(element);
            StyleSheet* sheet = styleElement->sheet();
            if (sheet && sheet->isCSSStyleSheet())
                serializeCSSStyleSheet(static_cast<CSSStyleSheet*>(sheet), KURL());
        }
    }

    for (Frame* childFrame = frame->tree()->firstChild(); childFrame; childFrame = childFrame->tree()->nextSibling())
        serializeFrame(childFrame);
}

void PageSerializer::serializeCSSStyleSheet(CSSStyleSheet* styleSheet, const KURL& url)
{
    StringBuilder cssText;
    for (unsigned i = 0; i < styleSheet->length(); ++i) {
        StyleBase* item = styleSheet->item(i);
        String itemText = item->cssText();
        if (!itemText.isEmpty()) {
            cssText.append(itemText);
            if (i < styleSheet->length() - 1)
                cssText.append("\n\n");
        }
        // Some rules have resources associated with them that we need to retrieve.
        if (item->isImportRule()) {
            CSSImportRule* importRule = static_cast<CSSImportRule*>(item);
            KURL importURL = styleSheet->document()->completeURL(importRule->href());
            if (m_resourceURLs.contains(importURL))
                continue;
            serializeCSSStyleSheet(importRule->styleSheet(), importURL);
        } else if (item->isFontFaceRule()) {
            // FIXME: Add support for font face rule. It is not clear to me at this point if the actual otf/eot file can
            // be retrieved from the CSSFontFaceRule object.
        } else if (item->isStyleRule())
            retrieveResourcesForCSSRule(static_cast<CSSStyleRule*>(item));
    }

    if (url.isValid() && !m_resourceURLs.contains(url)) {
        // FIXME: We should check whether a charset has been specified and if none was found add one.
        TextEncoding textEncoding = TextEncoding(styleSheet->charset());
        ASSERT(textEncoding.isValid());
        String textString = cssText.toString();
        CString text = textEncoding.encode(textString.characters(), textString.length(), EntitiesForUnencodables);
        m_resources->append(Resource(url, String("text/css"), SharedBuffer::create(text.data(), text.length())));
        m_resourceURLs.add(url);
    }
}

void PageSerializer::addImageToResources(CachedImage* image, const KURL& url)
{
    if (!url.isValid() || m_resourceURLs.contains(url))
        return;

    if (!image || image->image() == Image::nullImage())
        return;

    String mimeType = image->response().mimeType();
    m_resources->append(Resource(url, mimeType, image->image()->data()));
    m_resourceURLs.add(url);
}

void PageSerializer::retrieveResourcesForCSSRule(CSSStyleRule* rule)
{
    retrieveResourcesForCSSDeclaration(rule->style());
}

void PageSerializer::retrieveResourcesForCSSDeclaration(CSSStyleDeclaration* styleDeclaration)
{
    if (!styleDeclaration)
        return;

    if (!styleDeclaration->stylesheet()->isCSSStyleSheet())
        return;

    CSSStyleSheet* cssStyleSheet = static_cast<CSSStyleSheet*>(styleDeclaration->stylesheet());

    // The background-image and list-style-image (for ul or ol) are the CSS properties
    // that make use of images. We iterate to make sure we include any other
    // image properties there might be.
    for (unsigned i = 0; i < styleDeclaration->length(); ++i) {
        // FIXME: It's kind of ridiculous to get the property name and then get
        // the value out of the name. Ideally we would get the value out of the
        // property ID, but CSSStyleDeclaration only gives access to property
        // names, not IDs.
        RefPtr<CSSValue> cssValue = styleDeclaration->getPropertyCSSValue(styleDeclaration->item(i));
        if (!cssValue->isImageValue())
            continue;

        CSSImageValue* imageValue = static_cast<CSSImageValue*>(cssValue.get());
        StyleImage* styleImage = imageValue->cachedOrPendingImage();
        // Non cached-images are just place-holders and do not contain data.
        if (!styleImage || !styleImage->isCachedImage())
            continue;

        CachedImage* image = static_cast<StyleCachedImage*>(styleImage)->cachedImage();

        KURL url = cssStyleSheet->document()->completeURL(image->url());
        addImageToResources(image, url);
    }
}

KURL PageSerializer::urlForBlankFrame(Frame* frame)
{
    HashMap<Frame*, KURL>::iterator iter = m_blankFrameURLs.find(frame);
    if (iter != m_blankFrameURLs.end())
        return iter->second;
    String url = makeString("wyciwyg://frame/", String::number(m_blankFrameCounter++));
    KURL fakeURL(ParsedURLString, url);
    m_blankFrameURLs.add(frame, fakeURL);

    return fakeURL;
}

}
