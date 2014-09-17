/*
 * Copyright (C) 2006, 2008, 2009, 2010 Apple Inc. All rights reserved.
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
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "HTMLViewSourceDocument.h"

#include "Attribute.h"
#include "DOMImplementation.h"
#include "HTMLAnchorElement.h"
#include "HTMLBaseElement.h"
#include "HTMLBodyElement.h"
#include "HTMLDivElement.h"
#include "HTMLHtmlElement.h"
#include "HTMLNames.h"
#include "HTMLTableCellElement.h"
#include "HTMLTableElement.h"
#include "HTMLTableRowElement.h"
#include "HTMLTableSectionElement.h"
#include "HTMLToken.h"
#include "HTMLViewSourceParser.h"
#include "SegmentedString.h"
#include "Text.h"
#include "TextViewSourceParser.h"

namespace WebCore {

using namespace HTMLNames;

HTMLViewSourceDocument::HTMLViewSourceDocument(Frame* frame, const KURL& url, const String& mimeType)
    : HTMLDocument(frame, url)
    , m_type(mimeType)
{
    setUsesBeforeAfterRules(true);
    setUsesViewSourceStyles(true);

    setCompatibilityMode(QuirksMode);
    lockCompatibilityMode();
}

PassRefPtr<DocumentParser> HTMLViewSourceDocument::createParser()
{
    if (m_type == "text/html" || m_type == "application/xhtml+xml" || m_type == "image/svg+xml" || DOMImplementation::isXMLMIMEType(m_type)
#if ENABLE(XHTMLMP)
        || m_type == "application/vnd.wap.xhtml+xml"
#endif
        )
        return HTMLViewSourceParser::create(this);

    return TextViewSourceParser::create(this);
}

void HTMLViewSourceDocument::createContainingTable()
{
    RefPtr<HTMLHtmlElement> html = HTMLHtmlElement::create(this);
    parserAddChild(html);
    html->attach();
    RefPtr<HTMLBodyElement> body = HTMLBodyElement::create(this);
    html->parserAddChild(body);
    body->attach();

    // Create a line gutter div that can be used to make sure the gutter extends down the height of the whole
    // document.
    RefPtr<HTMLDivElement> div = HTMLDivElement::create(this);
    RefPtr<NamedNodeMap> attrs = NamedNodeMap::create();
    attrs->addAttribute(Attribute::createMapped(classAttr, "webkit-line-gutter-backdrop"));
    div->setAttributeMap(attrs.release());
    body->parserAddChild(div);
    div->attach();

    RefPtr<HTMLTableElement> table = HTMLTableElement::create(this);
    body->parserAddChild(table);
    table->attach();
    m_tbody = HTMLTableSectionElement::create(tbodyTag, this);
    table->parserAddChild(m_tbody);
    m_tbody->attach();
    m_current = m_tbody;
}

void HTMLViewSourceDocument::addSource(const String& source, HTMLToken& token)
{
    if (!m_current)
        createContainingTable();

    switch (token.type()) {
    case HTMLToken::Uninitialized:
        ASSERT_NOT_REACHED();
        break;
    case HTMLToken::DOCTYPE:
        processDoctypeToken(source, token);
        break;
    case HTMLToken::EndOfFile:
        break;
    case HTMLToken::StartTag:
    case HTMLToken::EndTag:
        processTagToken(source, token);
        break;
    case HTMLToken::Comment:
        processCommentToken(source, token);
        break;
    case HTMLToken::Character:
        processCharacterToken(source, token);
        break;
    }
}

void HTMLViewSourceDocument::processDoctypeToken(const String& source, HTMLToken&)
{
    if (!m_current)
        createContainingTable();
    m_current = addSpanWithClassName("webkit-html-doctype");
    addText(source, "webkit-html-doctype");
    m_current = m_td;
}

void HTMLViewSourceDocument::processTagToken(const String& source, HTMLToken& token)
{
    m_current = addSpanWithClassName("webkit-html-tag");

    AtomicString tagName(token.name().data(), token.name().size());

    unsigned index = 0;
    HTMLToken::AttributeList::const_iterator iter = token.attributes().begin();
    while (index < source.length()) {
        if (iter == token.attributes().end()) {
            // We want to show the remaining characters in the token.
            index = addRange(source, index, source.length(), "");
            ASSERT(index == source.length());
            break;
        }

        AtomicString name(iter->m_name.data(), iter->m_name.size());
        String value(iter->m_value.data(), iter->m_value.size()); 

        index = addRange(source, index, iter->m_nameRange.m_start - token.startIndex(), "");
        index = addRange(source, index, iter->m_nameRange.m_end - token.startIndex(), "webkit-html-attribute-name");

        if (tagName == baseTag && name == hrefAttr)
            m_current = addBase(value);

        index = addRange(source, index, iter->m_valueRange.m_start - token.startIndex(), "");

        bool isLink = name == srcAttr || name == hrefAttr;
        index = addRange(source, index, iter->m_valueRange.m_end - token.startIndex(), "webkit-html-attribute-value", isLink, tagName == aTag);

        ++iter;
    }
    m_current = m_td;
}

void HTMLViewSourceDocument::processCommentToken(const String& source, HTMLToken&)
{
    m_current = addSpanWithClassName("webkit-html-comment");
    addText(source, "webkit-html-comment");
    m_current = m_td;
}

void HTMLViewSourceDocument::processCharacterToken(const String& source, HTMLToken&)
{
    addText(source, "");
}

PassRefPtr<Element> HTMLViewSourceDocument::addSpanWithClassName(const AtomicString& className)
{
    if (m_current == m_tbody) {
        addLine(className);
        return m_current;
    }

    RefPtr<HTMLElement> span = HTMLElement::create(spanTag, this);
    RefPtr<NamedNodeMap> attrs = NamedNodeMap::create();
    attrs->addAttribute(Attribute::createMapped(classAttr, className));
    span->setAttributeMap(attrs.release());
    m_current->parserAddChild(span);
    span->attach();
    return span.release();
}

void HTMLViewSourceDocument::addLine(const AtomicString& className)
{
    // Create a table row.
    RefPtr<HTMLTableRowElement> trow = HTMLTableRowElement::create(this);
    m_tbody->parserAddChild(trow);
    trow->attach();

    // Create a cell that will hold the line number (it is generated in the stylesheet using counters).
    RefPtr<HTMLTableCellElement> td = HTMLTableCellElement::create(tdTag, this);
    RefPtr<NamedNodeMap> attrs = NamedNodeMap::create();
    attrs->addAttribute(Attribute::createMapped(classAttr, "webkit-line-number"));
    td->setAttributeMap(attrs.release());
    trow->parserAddChild(td);
    td->attach();

    // Create a second cell for the line contents
    td = HTMLTableCellElement::create(tdTag, this);
    attrs = NamedNodeMap::create();
    attrs->addAttribute(Attribute::createMapped(classAttr, "webkit-line-content"));
    td->setAttributeMap(attrs.release());
    trow->parserAddChild(td);
    td->attach();
    m_current = m_td = td;

#ifdef DEBUG_LINE_NUMBERS
    RefPtr<Text> lineNumberText = Text::create(this, String::number(parser()->lineNumber() + 1) + " ");
    td->addChild(lineNumberText);
    lineNumberText->attach();
#endif

    // Open up the needed spans.
    if (!className.isEmpty()) {
        if (className == "webkit-html-attribute-name" || className == "webkit-html-attribute-value")
            m_current = addSpanWithClassName("webkit-html-tag");
        m_current = addSpanWithClassName(className);
    }
}

void HTMLViewSourceDocument::addText(const String& text, const AtomicString& className)
{
    if (text.isEmpty())
        return;

    // Add in the content, splitting on newlines.
    Vector<String> lines;
    text.split('\n', true, lines);
    unsigned size = lines.size();
    for (unsigned i = 0; i < size; i++) {
        String substring = lines[i];
        if (substring.isEmpty()) {
            if (i == size - 1)
                break;
            substring = " ";
        }
        if (m_current == m_tbody)
            addLine(className);
        RefPtr<Text> t = Text::create(this, substring);
        m_current->parserAddChild(t);
        t->attach();
        if (i < size - 1)
            m_current = m_tbody;
    }

    // Set current to m_tbody if the last character was a newline.
    if (text[text.length() - 1] == '\n')
        m_current = m_tbody;
}

int HTMLViewSourceDocument::addRange(const String& source, int start, int end, const String& className, bool isLink, bool isAnchor)
{
    ASSERT(start <= end);
    if (start == end)
        return start;

    String text = source.substring(start, end - start);
    if (!className.isEmpty()) {
        if (isLink)
            m_current = addLink(text, isAnchor);
        else
            m_current = addSpanWithClassName(className);
    }
    addText(text, className);
    if (!className.isEmpty() && m_current != m_tbody)
        m_current = static_cast<Element*>(m_current->parentNode());
    return end;
}

PassRefPtr<Element> HTMLViewSourceDocument::addBase(const AtomicString& href)
{
    RefPtr<HTMLBaseElement> base = HTMLBaseElement::create(baseTag, this);
    RefPtr<NamedNodeMap> attributeMap = NamedNodeMap::create();
    attributeMap->addAttribute(Attribute::createMapped(hrefAttr, href));
    base->setAttributeMap(attributeMap.release());
    m_current->parserAddChild(base);
    base->attach();
    return base.release();
}

PassRefPtr<Element> HTMLViewSourceDocument::addLink(const AtomicString& url, bool isAnchor)
{
    if (m_current == m_tbody)
        addLine("webkit-html-tag");

    // Now create a link for the attribute value instead of a span.
    RefPtr<HTMLAnchorElement> anchor = HTMLAnchorElement::create(this);
    RefPtr<NamedNodeMap> attrs = NamedNodeMap::create();
    const char* classValue;
    if (isAnchor)
        classValue = "webkit-html-attribute-value webkit-html-external-link";
    else
        classValue = "webkit-html-attribute-value webkit-html-resource-link";
    attrs->addAttribute(Attribute::createMapped(classAttr, classValue));
    attrs->addAttribute(Attribute::createMapped(targetAttr, "_blank"));
    attrs->addAttribute(Attribute::createMapped(hrefAttr, url));
    anchor->setAttributeMap(attrs.release());
    m_current->parserAddChild(anchor);
    anchor->attach();
    return anchor.release();
}

}
