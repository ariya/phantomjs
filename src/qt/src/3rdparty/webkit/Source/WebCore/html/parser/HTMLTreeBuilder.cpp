/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#include "config.h"
#include "HTMLTreeBuilder.h"

#include "Comment.h"
#include "DOMWindow.h"
#include "DocumentFragment.h"
#include "DocumentType.h"
#include "Frame.h"
#include "HTMLDocument.h"
#include "HTMLDocumentParser.h"
#include "HTMLElementFactory.h"
#include "HTMLFormElement.h"
#include "HTMLHtmlElement.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "HTMLScriptElement.h"
#include "HTMLToken.h"
#include "HTMLTokenizer.h"
#include "LocalizedStrings.h"
#include "MathMLNames.h"
#include "NotImplemented.h"
#include "SVGNames.h"
#include "Text.h"
#include "XLinkNames.h"
#include "XMLNSNames.h"
#include "XMLNames.h"
#include <wtf/unicode/CharacterNames.h>

namespace WebCore {

using namespace HTMLNames;

static const int uninitializedLineNumberValue = -1;

static TextPosition1 uninitializedPositionValue1()
{
    return TextPosition1(WTF::OneBasedNumber::fromOneBasedInt(-1), WTF::OneBasedNumber::base());
}

namespace {

inline bool isHTMLSpaceOrReplacementCharacter(UChar character)
{
    return isHTMLSpace(character) || character == replacementCharacter;
}

inline bool isAllWhitespace(const String& string)
{
    return string.isAllSpecialCharacters<isHTMLSpace>();
}

inline bool isAllWhitespaceOrReplacementCharacters(const String& string)
{
    return string.isAllSpecialCharacters<isHTMLSpaceOrReplacementCharacter>();
}

bool isNumberedHeaderTag(const AtomicString& tagName)
{
    return tagName == h1Tag
        || tagName == h2Tag
        || tagName == h3Tag
        || tagName == h4Tag
        || tagName == h5Tag
        || tagName == h6Tag;
}

bool isCaptionColOrColgroupTag(const AtomicString& tagName)
{
    return tagName == captionTag
        || tagName == colTag
        || tagName == colgroupTag;
}

bool isTableCellContextTag(const AtomicString& tagName)
{
    return tagName == thTag || tagName == tdTag;
}

bool isTableBodyContextTag(const AtomicString& tagName)
{
    return tagName == tbodyTag
        || tagName == tfootTag
        || tagName == theadTag;
}

// http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#special
bool isSpecialNode(Node* node)
{
    if (node->hasTagName(MathMLNames::miTag)
        || node->hasTagName(MathMLNames::moTag)
        || node->hasTagName(MathMLNames::mnTag)
        || node->hasTagName(MathMLNames::msTag)
        || node->hasTagName(MathMLNames::mtextTag)
        || node->hasTagName(MathMLNames::annotation_xmlTag)
        || node->hasTagName(SVGNames::foreignObjectTag)
        || node->hasTagName(SVGNames::descTag)
        || node->hasTagName(SVGNames::titleTag))
        return true;
    if (node->nodeType() == Node::DOCUMENT_FRAGMENT_NODE)
        return true;
    if (!isInHTMLNamespace(node))
        return false;
    const AtomicString& tagName = node->localName();
    return tagName == addressTag
        || tagName == appletTag
        || tagName == areaTag
        || tagName == articleTag
        || tagName == asideTag
        || tagName == baseTag
        || tagName == basefontTag
        || tagName == bgsoundTag
        || tagName == blockquoteTag
        || tagName == bodyTag
        || tagName == brTag
        || tagName == buttonTag
        || tagName == captionTag
        || tagName == centerTag
        || tagName == colTag
        || tagName == colgroupTag
        || tagName == commandTag
        || tagName == ddTag
        || tagName == detailsTag
        || tagName == dirTag
        || tagName == divTag
        || tagName == dlTag
        || tagName == dtTag
        || tagName == embedTag
        || tagName == fieldsetTag
        || tagName == figcaptionTag
        || tagName == figureTag
        || tagName == footerTag
        || tagName == formTag
        || tagName == frameTag
        || tagName == framesetTag
        || isNumberedHeaderTag(tagName)
        || tagName == headTag
        || tagName == headerTag
        || tagName == hgroupTag
        || tagName == hrTag
        || tagName == htmlTag
        || tagName == iframeTag
        || tagName == imgTag
        || tagName == inputTag
        || tagName == isindexTag
        || tagName == liTag
        || tagName == linkTag
        || tagName == listingTag
        || tagName == marqueeTag
        || tagName == menuTag
        || tagName == metaTag
        || tagName == navTag
        || tagName == noembedTag
        || tagName == noframesTag
        || tagName == noscriptTag
        || tagName == objectTag
        || tagName == olTag
        || tagName == pTag
        || tagName == paramTag
        || tagName == plaintextTag
        || tagName == preTag
        || tagName == scriptTag
        || tagName == sectionTag
        || tagName == selectTag
        || tagName == styleTag
        || tagName == summaryTag
        || tagName == tableTag
        || isTableBodyContextTag(tagName)
        || tagName == tdTag
        || tagName == textareaTag
        || tagName == thTag
        || tagName == titleTag
        || tagName == trTag
        || tagName == ulTag
        || tagName == wbrTag
        || tagName == xmpTag;
}

bool isNonAnchorNonNobrFormattingTag(const AtomicString& tagName)
{
    return tagName == bTag
        || tagName == bigTag
        || tagName == codeTag
        || tagName == emTag
        || tagName == fontTag
        || tagName == iTag
        || tagName == sTag
        || tagName == smallTag
        || tagName == strikeTag
        || tagName == strongTag
        || tagName == ttTag
        || tagName == uTag;
}

bool isNonAnchorFormattingTag(const AtomicString& tagName)
{
    return tagName == nobrTag
        || isNonAnchorNonNobrFormattingTag(tagName);
}

// http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#formatting
bool isFormattingTag(const AtomicString& tagName)
{
    return tagName == aTag || isNonAnchorFormattingTag(tagName);
}

HTMLFormElement* closestFormAncestor(Element* element)
{
    while (element) {
        if (element->hasTagName(formTag))
            return static_cast<HTMLFormElement*>(element);
        ContainerNode* parent = element->parentNode();
        if (!parent || !parent->isElementNode())
            return 0;
        element = static_cast<Element*>(parent);
    }
    return 0;
}

} // namespace

class HTMLTreeBuilder::ExternalCharacterTokenBuffer {
    WTF_MAKE_NONCOPYABLE(ExternalCharacterTokenBuffer);
public:
    explicit ExternalCharacterTokenBuffer(AtomicHTMLToken& token)
        : m_current(token.characters().data())
        , m_end(m_current + token.characters().size())
    {
        ASSERT(!isEmpty());
    }

    explicit ExternalCharacterTokenBuffer(const String& string)
        : m_current(string.characters())
        , m_end(m_current + string.length())
    {
        ASSERT(!isEmpty());
    }

    ~ExternalCharacterTokenBuffer()
    {
        ASSERT(isEmpty());
    }

    bool isEmpty() const { return m_current == m_end; }

    void skipLeadingWhitespace()
    {
        skipLeading<isHTMLSpace>();
    }

    String takeLeadingWhitespace()
    {
        return takeLeading<isHTMLSpace>();
    }

    String takeLeadingNonWhitespace()
    {
        return takeLeading<isNotHTMLSpace>();
    }

    String takeRemaining()
    {
        ASSERT(!isEmpty());
        const UChar* start = m_current;
        m_current = m_end;
        return String(start, m_current - start);
    }

    void giveRemainingTo(Vector<UChar>& recipient)
    {
        recipient.append(m_current, m_end - m_current);
        m_current = m_end;
    }

    String takeRemainingWhitespace()
    {
        ASSERT(!isEmpty());
        Vector<UChar> whitespace;
        do {
            UChar cc = *m_current++;
            if (isHTMLSpace(cc))
                whitespace.append(cc);
        } while (m_current < m_end);
        // Returning the null string when there aren't any whitespace
        // characters is slightly cleaner semantically because we don't want
        // to insert a text node (as opposed to inserting an empty text node).
        if (whitespace.isEmpty())
            return String();
        return String::adopt(whitespace);
    }

private:
    template<bool characterPredicate(UChar)>
    void skipLeading()
    {
        ASSERT(!isEmpty());
        while (characterPredicate(*m_current)) {
            if (++m_current == m_end)
                return;
        }
    }

    template<bool characterPredicate(UChar)>
    String takeLeading()
    {
        ASSERT(!isEmpty());
        const UChar* start = m_current;
        skipLeading<characterPredicate>();
        if (start == m_current)
            return String();
        return String(start, m_current - start);
    }

    const UChar* m_current;
    const UChar* m_end;
};


HTMLTreeBuilder::HTMLTreeBuilder(HTMLDocumentParser* parser, HTMLDocument* document, bool reportErrors, bool usePreHTML5ParserQuirks)
    : m_framesetOk(true)
    , m_document(document)
    , m_tree(document)
    , m_reportErrors(reportErrors)
    , m_isPaused(false)
    , m_insertionMode(InitialMode)
    , m_originalInsertionMode(InitialMode)
    , m_parser(parser)
    , m_scriptToProcessStartPosition(uninitializedPositionValue1())
    , m_lastScriptElementStartPosition(TextPosition0::belowRangePosition())
    , m_usePreHTML5ParserQuirks(usePreHTML5ParserQuirks)
    , m_hasPendingForeignInsertionModeSteps(false)
{
}

// FIXME: Member variables should be grouped into self-initializing structs to
// minimize code duplication between these constructors.
HTMLTreeBuilder::HTMLTreeBuilder(HTMLDocumentParser* parser, DocumentFragment* fragment, Element* contextElement, FragmentScriptingPermission scriptingPermission, bool usePreHTML5ParserQuirks)
    : m_framesetOk(true)
    , m_fragmentContext(fragment, contextElement, scriptingPermission)
    , m_document(fragment->document())
    , m_tree(fragment, scriptingPermission)
    , m_reportErrors(false) // FIXME: Why not report errors in fragments?
    , m_isPaused(false)
    , m_insertionMode(InitialMode)
    , m_originalInsertionMode(InitialMode)
    , m_parser(parser)
    , m_scriptToProcessStartPosition(uninitializedPositionValue1())
    , m_lastScriptElementStartPosition(TextPosition0::belowRangePosition())
    , m_usePreHTML5ParserQuirks(usePreHTML5ParserQuirks)
    , m_hasPendingForeignInsertionModeSteps(false)
{
    if (contextElement) {
        // Steps 4.2-4.6 of the HTML5 Fragment Case parsing algorithm:
        // http://www.whatwg.org/specs/web-apps/current-work/multipage/the-end.html#fragment-case
        // For efficiency, we skip step 4.2 ("Let root be a new html element with no attributes")
        // and instead use the DocumentFragment as a root node.
        m_tree.openElements()->pushRootNode(fragment);
        resetInsertionModeAppropriately();
        m_tree.setForm(closestFormAncestor(contextElement));
    }
}

HTMLTreeBuilder::~HTMLTreeBuilder()
{
}

void HTMLTreeBuilder::detach()
{
    // This call makes little sense in fragment mode, but for consistency
    // DocumentParser expects detach() to always be called before it's destroyed.
    m_document = 0;
    // HTMLConstructionSite might be on the callstack when detach() is called
    // otherwise we'd just call m_tree.clear() here instead.
    m_tree.detach();
}

HTMLTreeBuilder::FragmentParsingContext::FragmentParsingContext()
    : m_fragment(0)
    , m_contextElement(0)
    , m_scriptingPermission(FragmentScriptingAllowed)
{
}

HTMLTreeBuilder::FragmentParsingContext::FragmentParsingContext(DocumentFragment* fragment, Element* contextElement, FragmentScriptingPermission scriptingPermission)
    : m_fragment(fragment)
    , m_contextElement(contextElement)
    , m_scriptingPermission(scriptingPermission)
{
    ASSERT(!fragment->hasChildNodes());
}

HTMLTreeBuilder::FragmentParsingContext::~FragmentParsingContext()
{
}

PassRefPtr<Element> HTMLTreeBuilder::takeScriptToProcess(TextPosition1& scriptStartPosition)
{
    // Unpause ourselves, callers may pause us again when processing the script.
    // The HTML5 spec is written as though scripts are executed inside the tree
    // builder.  We pause the parser to exit the tree builder, and then resume
    // before running scripts.
    m_isPaused = false;
    scriptStartPosition = m_scriptToProcessStartPosition;
    m_scriptToProcessStartPosition = uninitializedPositionValue1();
    return m_scriptToProcess.release();
}

void HTMLTreeBuilder::constructTreeFromToken(HTMLToken& rawToken)
{
    AtomicHTMLToken token(rawToken);

    // We clear the rawToken in case constructTreeFromAtomicToken
    // synchronously re-enters the parser. We don't clear the token immedately
    // for Character tokens because the AtomicHTMLToken avoids copying the
    // characters by keeping a pointer to the underlying buffer in the
    // HTMLToken. Fortuantely, Character tokens can't cause use to re-enter
    // the parser.
    //
    // FIXME: Top clearing the rawToken once we start running the parser off
    // the main thread or once we stop allowing synchronous JavaScript
    // execution from parseMappedAttribute.
    if (rawToken.type() != HTMLToken::Character)
        rawToken.clear();

    constructTreeFromAtomicToken(token);

    if (!rawToken.isUninitialized()) {
        ASSERT(rawToken.type() == HTMLToken::Character);
        rawToken.clear();
    }
}

void HTMLTreeBuilder::constructTreeFromAtomicToken(AtomicHTMLToken& token)
{
    processToken(token);

    // Swallowing U+0000 characters isn't in the HTML5 spec, but turning all
    // the U+0000 characters into replacement characters has compatibility
    // problems.
    m_parser->tokenizer()->setForceNullCharacterReplacement(m_insertionMode == TextMode || m_insertionMode == InForeignContentMode);
    m_parser->tokenizer()->setShouldAllowCDATA(m_insertionMode == InForeignContentMode && !isInHTMLNamespace(m_tree.currentNode()));
}

void HTMLTreeBuilder::processToken(AtomicHTMLToken& token)
{
    switch (token.type()) {
    case HTMLToken::Uninitialized:
        ASSERT_NOT_REACHED();
        break;
    case HTMLToken::DOCTYPE:
        processDoctypeToken(token);
        break;
    case HTMLToken::StartTag:
        processStartTag(token);
        break;
    case HTMLToken::EndTag:
        processEndTag(token);
        break;
    case HTMLToken::Comment:
        processComment(token);
        return;
    case HTMLToken::Character:
        processCharacter(token);
        break;
    case HTMLToken::EndOfFile:
        processEndOfFile(token);
        break;
    }
}

void HTMLTreeBuilder::processDoctypeToken(AtomicHTMLToken& token)
{
    ASSERT(token.type() == HTMLToken::DOCTYPE);
    if (m_insertionMode == InitialMode) {
        m_tree.insertDoctype(token);
        setInsertionMode(BeforeHTMLMode);
        return;
    }
    if (m_insertionMode == InTableTextMode) {
        defaultForInTableText();
        processDoctypeToken(token);
        return;
    }
    parseError(token);
}

void HTMLTreeBuilder::processFakeStartTag(const QualifiedName& tagName, PassRefPtr<NamedNodeMap> attributes)
{
    // FIXME: We'll need a fancier conversion than just "localName" for SVG/MathML tags.
    AtomicHTMLToken fakeToken(HTMLToken::StartTag, tagName.localName(), attributes);
    processStartTag(fakeToken);
}

void HTMLTreeBuilder::processFakeEndTag(const QualifiedName& tagName)
{
    // FIXME: We'll need a fancier conversion than just "localName" for SVG/MathML tags.
    AtomicHTMLToken fakeToken(HTMLToken::EndTag, tagName.localName());
    processEndTag(fakeToken);
}

void HTMLTreeBuilder::processFakeCharacters(const String& characters)
{
    ASSERT(!characters.isEmpty());
    ExternalCharacterTokenBuffer buffer(characters);
    processCharacterBuffer(buffer);
}

void HTMLTreeBuilder::processFakePEndTagIfPInButtonScope()
{
    if (!m_tree.openElements()->inButtonScope(pTag.localName()))
        return;
    AtomicHTMLToken endP(HTMLToken::EndTag, pTag.localName());
    processEndTag(endP);
}

PassRefPtr<NamedNodeMap> HTMLTreeBuilder::attributesForIsindexInput(AtomicHTMLToken& token)
{
    RefPtr<NamedNodeMap> attributes = token.takeAtributes();
    if (!attributes)
        attributes = NamedNodeMap::create();
    else {
        attributes->removeAttribute(nameAttr);
        attributes->removeAttribute(actionAttr);
        attributes->removeAttribute(promptAttr);
    }

    RefPtr<Attribute> mappedAttribute = Attribute::createMapped(nameAttr, isindexTag.localName());
    attributes->insertAttribute(mappedAttribute.release(), false);
    return attributes.release();
}

void HTMLTreeBuilder::processIsindexStartTagForInBody(AtomicHTMLToken& token)
{
    ASSERT(token.type() == HTMLToken::StartTag);
    ASSERT(token.name() == isindexTag);
    parseError(token);
    if (m_tree.form())
        return;
    notImplemented(); // Acknowledge self-closing flag
    processFakeStartTag(formTag);
    RefPtr<Attribute> actionAttribute = token.getAttributeItem(actionAttr);
    if (actionAttribute) {
        ASSERT(m_tree.currentElement()->hasTagName(formTag));
        m_tree.currentElement()->setAttribute(actionAttr, actionAttribute->value());
    }
    processFakeStartTag(hrTag);
    processFakeStartTag(labelTag);
    RefPtr<Attribute> promptAttribute = token.getAttributeItem(promptAttr);
    if (promptAttribute)
        processFakeCharacters(promptAttribute->value());
    else
        processFakeCharacters(searchableIndexIntroduction());
    processFakeStartTag(inputTag, attributesForIsindexInput(token));
    notImplemented(); // This second set of characters may be needed by non-english locales.
    processFakeEndTag(labelTag);
    processFakeStartTag(hrTag);
    processFakeEndTag(formTag);
}

namespace {

bool isLi(const ContainerNode* element)
{
    return element->hasTagName(liTag);
}

bool isDdOrDt(const ContainerNode* element)
{
    return element->hasTagName(ddTag)
        || element->hasTagName(dtTag);
}

}

template <bool shouldClose(const ContainerNode*)>
void HTMLTreeBuilder::processCloseWhenNestedTag(AtomicHTMLToken& token)
{
    m_framesetOk = false;
    HTMLElementStack::ElementRecord* nodeRecord = m_tree.openElements()->topRecord();
    while (1) {
        RefPtr<ContainerNode> node = nodeRecord->node();
        if (shouldClose(node.get())) {
            ASSERT(node->isElementNode());
            processFakeEndTag(toElement(node.get())->tagQName());
            break;
        }
        if (isSpecialNode(node.get()) && !node->hasTagName(addressTag) && !node->hasTagName(divTag) && !node->hasTagName(pTag))
            break;
        nodeRecord = nodeRecord->next();
    }
    processFakePEndTagIfPInButtonScope();
    m_tree.insertHTMLElement(token);
}

namespace {

typedef HashMap<AtomicString, QualifiedName> PrefixedNameToQualifiedNameMap;

void mapLoweredLocalNameToName(PrefixedNameToQualifiedNameMap* map, QualifiedName** names, size_t length)
{
    for (size_t i = 0; i < length; ++i) {
        const QualifiedName& name = *names[i];
        const AtomicString& localName = name.localName();
        AtomicString loweredLocalName = localName.lower();
        if (loweredLocalName != localName)
            map->add(loweredLocalName, name);
    }
}

void adjustSVGTagNameCase(AtomicHTMLToken& token)
{
    static PrefixedNameToQualifiedNameMap* caseMap = 0;
    if (!caseMap) {
        caseMap = new PrefixedNameToQualifiedNameMap;
        size_t length = 0;
        QualifiedName** svgTags = SVGNames::getSVGTags(&length);
        mapLoweredLocalNameToName(caseMap, svgTags, length);
    }

    const QualifiedName& casedName = caseMap->get(token.name());
    if (casedName.localName().isNull())
        return;
    token.setName(casedName.localName());
}

template<QualifiedName** getAttrs(size_t* length)>
void adjustAttributes(AtomicHTMLToken& token)
{
    static PrefixedNameToQualifiedNameMap* caseMap = 0;
    if (!caseMap) {
        caseMap = new PrefixedNameToQualifiedNameMap;
        size_t length = 0;
        QualifiedName** attrs = getAttrs(&length);
        mapLoweredLocalNameToName(caseMap, attrs, length);
    }

    NamedNodeMap* attributes = token.attributes();
    if (!attributes)
        return;

    for (unsigned x = 0; x < attributes->length(); ++x) {
        Attribute* attribute = attributes->attributeItem(x);
        const QualifiedName& casedName = caseMap->get(attribute->localName());
        if (!casedName.localName().isNull())
            attribute->parserSetName(casedName);
    }
}

void adjustSVGAttributes(AtomicHTMLToken& token)
{
    adjustAttributes<SVGNames::getSVGAttrs>(token);
}

void adjustMathMLAttributes(AtomicHTMLToken& token)
{
    adjustAttributes<MathMLNames::getMathMLAttrs>(token);
}

void addNamesWithPrefix(PrefixedNameToQualifiedNameMap* map, const AtomicString& prefix, QualifiedName** names, size_t length)
{
    for (size_t i = 0; i < length; ++i) {
        QualifiedName* name = names[i];
        const AtomicString& localName = name->localName();
        AtomicString prefixColonLocalName(prefix + ":" + localName);
        QualifiedName nameWithPrefix(prefix, localName, name->namespaceURI());
        map->add(prefixColonLocalName, nameWithPrefix);
    }
}

void adjustForeignAttributes(AtomicHTMLToken& token)
{
    static PrefixedNameToQualifiedNameMap* map = 0;
    if (!map) {
        map = new PrefixedNameToQualifiedNameMap;
        size_t length = 0;
        QualifiedName** attrs = XLinkNames::getXLinkAttrs(&length);
        addNamesWithPrefix(map, "xlink", attrs, length);

        attrs = XMLNames::getXMLAttrs(&length);
        addNamesWithPrefix(map, "xml", attrs, length);

        map->add("xmlns", XMLNSNames::xmlnsAttr);
        map->add("xmlns:xlink", QualifiedName("xmlns", "xlink", XMLNSNames::xmlnsNamespaceURI));
    }

    NamedNodeMap* attributes = token.attributes();
    if (!attributes)
        return;

    for (unsigned x = 0; x < attributes->length(); ++x) {
        Attribute* attribute = attributes->attributeItem(x);
        const QualifiedName& name = map->get(attribute->localName());
        if (!name.localName().isNull())
            attribute->parserSetName(name);
    }
}

}

void HTMLTreeBuilder::processStartTagForInBody(AtomicHTMLToken& token)
{
    ASSERT(token.type() == HTMLToken::StartTag);
    if (token.name() == htmlTag) {
        m_tree.insertHTMLHtmlStartTagInBody(token);
        return;
    }
    if (token.name() == baseTag
        || token.name() == basefontTag
        || token.name() == bgsoundTag
        || token.name() == commandTag
        || token.name() == linkTag
        || token.name() == metaTag
        || token.name() == noframesTag
        || token.name() == scriptTag
        || token.name() == styleTag
        || token.name() == titleTag) {
        bool didProcess = processStartTagForInHead(token);
        ASSERT_UNUSED(didProcess, didProcess);
        return;
    }
    if (token.name() == bodyTag) {
        if (!m_tree.openElements()->secondElementIsHTMLBodyElement() || m_tree.openElements()->hasOnlyOneElement()) {
            ASSERT(isParsingFragment());
            return;
        }
        m_tree.insertHTMLBodyStartTagInBody(token);
        return;
    }
    if (token.name() == framesetTag) {
        parseError(token);
        if (!m_tree.openElements()->secondElementIsHTMLBodyElement() || m_tree.openElements()->hasOnlyOneElement()) {
            ASSERT(isParsingFragment());
            return;
        }
        if (!m_framesetOk)
            return;
        ExceptionCode ec = 0;
        m_tree.openElements()->bodyElement()->remove(ec);
        ASSERT(!ec);
        m_tree.openElements()->popUntil(m_tree.openElements()->bodyElement());
        m_tree.openElements()->popHTMLBodyElement();
        ASSERT(m_tree.openElements()->top() == m_tree.openElements()->htmlElement());
        m_tree.insertHTMLElement(token);
        setInsertionMode(InFramesetMode);
        return;
    }
    if (token.name() == addressTag
        || token.name() == articleTag
        || token.name() == asideTag
        || token.name() == blockquoteTag
        || token.name() == centerTag
        || token.name() == detailsTag
        || token.name() == dirTag
        || token.name() == divTag
        || token.name() == dlTag
        || token.name() == fieldsetTag
        || token.name() == figcaptionTag
        || token.name() == figureTag
        || token.name() == footerTag
        || token.name() == headerTag
        || token.name() == hgroupTag
        || token.name() == menuTag
        || token.name() == navTag
        || token.name() == olTag
        || token.name() == pTag
        || token.name() == sectionTag
        || token.name() == summaryTag
        || token.name() == ulTag) {
        processFakePEndTagIfPInButtonScope();
        m_tree.insertHTMLElement(token);
        return;
    }
    if (isNumberedHeaderTag(token.name())) {
        processFakePEndTagIfPInButtonScope();
        if (isNumberedHeaderTag(m_tree.currentNode()->localName())) {
            parseError(token);
            m_tree.openElements()->pop();
        }
        m_tree.insertHTMLElement(token);
        return;
    }
    if (token.name() == preTag || token.name() == listingTag) {
        processFakePEndTagIfPInButtonScope();
        m_tree.insertHTMLElement(token);
        m_parser->tokenizer()->setSkipLeadingNewLineForListing(true);
        m_framesetOk = false;
        return;
    }
    if (token.name() == formTag) {
        if (m_tree.form()) {
            parseError(token);
            return;
        }
        processFakePEndTagIfPInButtonScope();
        m_tree.insertHTMLFormElement(token);
        return;
    }
    if (token.name() == liTag) {
        processCloseWhenNestedTag<isLi>(token);
        return;
    }
    if (token.name() == ddTag || token.name() == dtTag) {
        processCloseWhenNestedTag<isDdOrDt>(token);
        return;
    }
    if (token.name() == plaintextTag) {
        processFakePEndTagIfPInButtonScope();
        m_tree.insertHTMLElement(token);
        m_parser->tokenizer()->setState(HTMLTokenizer::PLAINTEXTState);
        return;
    }
    if (token.name() == buttonTag) {
        if (m_tree.openElements()->inScope(buttonTag)) {
            parseError(token);
            processFakeEndTag(buttonTag);
            reprocessStartTag(token); // FIXME: Could we just fall through here?
            return;
        }
        m_tree.reconstructTheActiveFormattingElements();
        m_tree.insertHTMLElement(token);
        m_framesetOk = false;
        return;
    }
    if (token.name() == aTag) {
        Element* activeATag = m_tree.activeFormattingElements()->closestElementInScopeWithName(aTag.localName());
        if (activeATag) {
            parseError(token);
            processFakeEndTag(aTag);
            m_tree.activeFormattingElements()->remove(activeATag);
            if (m_tree.openElements()->contains(activeATag))
                m_tree.openElements()->remove(activeATag);
        }
        m_tree.reconstructTheActiveFormattingElements();
        m_tree.insertFormattingElement(token);
        return;
    }
    if (isNonAnchorNonNobrFormattingTag(token.name())) {
        m_tree.reconstructTheActiveFormattingElements();
        m_tree.insertFormattingElement(token);
        return;
    }
    if (token.name() == nobrTag) {
        m_tree.reconstructTheActiveFormattingElements();
        if (m_tree.openElements()->inScope(nobrTag)) {
            parseError(token);
            processFakeEndTag(nobrTag);
            m_tree.reconstructTheActiveFormattingElements();
        }
        m_tree.insertFormattingElement(token);
        return;
    }
    if (token.name() == appletTag
        || token.name() == marqueeTag
        || token.name() == objectTag) {
        m_tree.reconstructTheActiveFormattingElements();
        m_tree.insertHTMLElement(token);
        m_tree.activeFormattingElements()->appendMarker();
        m_framesetOk = false;
        return;
    }
    if (token.name() == tableTag) {
        if (!m_document->inQuirksMode() && m_tree.openElements()->inButtonScope(pTag))
            processFakeEndTag(pTag);
        m_tree.insertHTMLElement(token);
        m_framesetOk = false;
        setInsertionMode(InTableMode);
        return;
    }
    if (token.name() == imageTag) {
        parseError(token);
        // Apparently we're not supposed to ask.
        token.setName(imgTag.localName());
        prepareToReprocessToken();
        // Note the fall through to the imgTag handling below!
    }
    if (token.name() == areaTag
        || token.name() == brTag
        || token.name() == embedTag
        || token.name() == imgTag
        || token.name() == keygenTag
        || token.name() == wbrTag) {
        m_tree.reconstructTheActiveFormattingElements();
        m_tree.insertSelfClosingHTMLElement(token);
        m_framesetOk = false;
        return;
    }
    if (token.name() == inputTag) {
        RefPtr<Attribute> typeAttribute = token.getAttributeItem(typeAttr);
        m_tree.reconstructTheActiveFormattingElements();
        m_tree.insertSelfClosingHTMLElement(token);
        if (!typeAttribute || !equalIgnoringCase(typeAttribute->value(), "hidden"))
            m_framesetOk = false;
        return;
    }
    if (token.name() == paramTag
        || token.name() == sourceTag
        || token.name() == trackTag) {
        m_tree.insertSelfClosingHTMLElement(token);
        return;
    }
    if (token.name() == hrTag) {
        processFakePEndTagIfPInButtonScope();
        m_tree.insertSelfClosingHTMLElement(token);
        m_framesetOk = false;
        return;
    }
    if (token.name() == isindexTag) {
        processIsindexStartTagForInBody(token);
        return;
    }
    if (token.name() == textareaTag) {
        m_tree.insertHTMLElement(token);
        m_parser->tokenizer()->setSkipLeadingNewLineForListing(true);
        m_parser->tokenizer()->setState(HTMLTokenizer::RCDATAState);
        m_originalInsertionMode = m_insertionMode;
        m_framesetOk = false;
        setInsertionMode(TextMode);
        return;
    }
    if (token.name() == xmpTag) {
        processFakePEndTagIfPInButtonScope();
        m_tree.reconstructTheActiveFormattingElements();
        m_framesetOk = false;
        processGenericRawTextStartTag(token);
        return;
    }
    if (token.name() == iframeTag) {
        m_framesetOk = false;
        processGenericRawTextStartTag(token);
        return;
    }
    if (token.name() == noembedTag && pluginsEnabled(m_document->frame())) {
        processGenericRawTextStartTag(token);
        return;
    }
    if (token.name() == noscriptTag && scriptEnabled(m_document->frame())) {
        processGenericRawTextStartTag(token);
        return;
    }
    if (token.name() == selectTag) {
        m_tree.reconstructTheActiveFormattingElements();
        m_tree.insertHTMLElement(token);
        m_framesetOk = false;
        if (m_insertionMode == InTableMode
             || m_insertionMode == InCaptionMode
             || m_insertionMode == InColumnGroupMode
             || m_insertionMode == InTableBodyMode
             || m_insertionMode == InRowMode
             || m_insertionMode == InCellMode)
            setInsertionMode(InSelectInTableMode);
        else
            setInsertionMode(InSelectMode);
        return;
    }
    if (token.name() == optgroupTag || token.name() == optionTag) {
        if (m_tree.openElements()->inScope(optionTag.localName())) {
            AtomicHTMLToken endOption(HTMLToken::EndTag, optionTag.localName());
            processEndTag(endOption);
        }
        m_tree.reconstructTheActiveFormattingElements();
        m_tree.insertHTMLElement(token);
        return;
    }
    if (token.name() == rpTag || token.name() == rtTag) {
        if (m_tree.openElements()->inScope(rubyTag.localName())) {
            m_tree.generateImpliedEndTags();
            if (!m_tree.currentNode()->hasTagName(rubyTag)) {
                parseError(token);
                m_tree.openElements()->popUntil(rubyTag.localName());
            }
        }
        m_tree.insertHTMLElement(token);
        return;
    }
    if (token.name() == MathMLNames::mathTag.localName()) {
        m_tree.reconstructTheActiveFormattingElements();
        adjustMathMLAttributes(token);
        adjustForeignAttributes(token);
        m_tree.insertForeignElement(token, MathMLNames::mathmlNamespaceURI);
        if (m_insertionMode != InForeignContentMode && !token.selfClosing())
            setInsertionMode(InForeignContentMode);
        return;
    }
    if (token.name() == SVGNames::svgTag.localName()) {
        m_tree.reconstructTheActiveFormattingElements();
        adjustSVGAttributes(token);
        adjustForeignAttributes(token);
        m_tree.insertForeignElement(token, SVGNames::svgNamespaceURI);
        if (m_insertionMode != InForeignContentMode && !token.selfClosing())
            setInsertionMode(InForeignContentMode);
        return;
    }
    if (isCaptionColOrColgroupTag(token.name())
        || token.name() == frameTag
        || token.name() == headTag
        || isTableBodyContextTag(token.name())
        || isTableCellContextTag(token.name())
        || token.name() == trTag) {
        parseError(token);
        return;
    }
    m_tree.reconstructTheActiveFormattingElements();
    m_tree.insertHTMLElement(token);
}

bool HTMLTreeBuilder::processColgroupEndTagForInColumnGroup()
{
    if (m_tree.currentNode() == m_tree.openElements()->rootNode()) {
        ASSERT(isParsingFragment());
        // FIXME: parse error
        return false;
    }
    m_tree.openElements()->pop();
    setInsertionMode(InTableMode);
    return true;
}

// http://www.whatwg.org/specs/web-apps/current-work/multipage/tokenization.html#close-the-cell
void HTMLTreeBuilder::closeTheCell()
{
    ASSERT(insertionMode() == InCellMode);
    if (m_tree.openElements()->inTableScope(tdTag)) {
        ASSERT(!m_tree.openElements()->inTableScope(thTag));
        processFakeEndTag(tdTag);
        return;
    }
    ASSERT(m_tree.openElements()->inTableScope(thTag));
    processFakeEndTag(thTag);
    ASSERT(insertionMode() == InRowMode);
}

void HTMLTreeBuilder::processStartTagForInTable(AtomicHTMLToken& token)
{
    ASSERT(token.type() == HTMLToken::StartTag);
    if (token.name() == captionTag) {
        m_tree.openElements()->popUntilTableScopeMarker();
        m_tree.activeFormattingElements()->appendMarker();
        m_tree.insertHTMLElement(token);
        setInsertionMode(InCaptionMode);
        return;
    }
    if (token.name() == colgroupTag) {
        m_tree.openElements()->popUntilTableScopeMarker();
        m_tree.insertHTMLElement(token);
        setInsertionMode(InColumnGroupMode);
        return;
    }
    if (token.name() == colTag) {
        processFakeStartTag(colgroupTag);
        ASSERT(InColumnGroupMode);
        reprocessStartTag(token);
        return;
    }
    if (isTableBodyContextTag(token.name())) {
        m_tree.openElements()->popUntilTableScopeMarker();
        m_tree.insertHTMLElement(token);
        setInsertionMode(InTableBodyMode);
        return;
    }
    if (isTableCellContextTag(token.name())
        || token.name() == trTag) {
        processFakeStartTag(tbodyTag);
        ASSERT(insertionMode() == InTableBodyMode);
        reprocessStartTag(token);
        return;
    }
    if (token.name() == tableTag) {
        parseError(token);
        if (!processTableEndTagForInTable()) {
            ASSERT(isParsingFragment());
            return;
        }
        reprocessStartTag(token);
        return;
    }
    if (token.name() == styleTag || token.name() == scriptTag) {
        processStartTagForInHead(token);
        return;
    }
    if (token.name() == inputTag) {
        Attribute* typeAttribute = token.getAttributeItem(typeAttr);
        if (typeAttribute && equalIgnoringCase(typeAttribute->value(), "hidden")) {
            parseError(token);
            m_tree.insertSelfClosingHTMLElement(token);
            return;
        }
        // Fall through to "anything else" case.
    }
    if (token.name() == formTag) {
        parseError(token);
        if (m_tree.form())
            return;
        m_tree.insertHTMLFormElement(token, true);
        m_tree.openElements()->pop();
        return;
    }
    parseError(token);
    HTMLConstructionSite::RedirectToFosterParentGuard redirecter(m_tree);
    processStartTagForInBody(token);
}

namespace {

bool shouldProcessForeignContentUsingInBodyInsertionMode(AtomicHTMLToken& token, ContainerNode* currentElement)
{
    ASSERT(token.type() == HTMLToken::StartTag);
    if (currentElement->hasTagName(MathMLNames::miTag)
        || currentElement->hasTagName(MathMLNames::moTag)
        || currentElement->hasTagName(MathMLNames::mnTag)
        || currentElement->hasTagName(MathMLNames::msTag)
        || currentElement->hasTagName(MathMLNames::mtextTag)) {
        return token.name() != MathMLNames::mglyphTag
            && token.name() != MathMLNames::malignmarkTag;
    }
    if (currentElement->hasTagName(MathMLNames::annotation_xmlTag))
        return token.name() == SVGNames::svgTag;
    if (currentElement->hasTagName(SVGNames::foreignObjectTag)
        || currentElement->hasTagName(SVGNames::descTag)
        || currentElement->hasTagName(SVGNames::titleTag))
        return true;
    return isInHTMLNamespace(currentElement);
}

}

void HTMLTreeBuilder::processStartTag(AtomicHTMLToken& token)
{
    ASSERT(token.type() == HTMLToken::StartTag);
    switch (insertionMode()) {
    case InitialMode:
        ASSERT(insertionMode() == InitialMode);
        defaultForInitial();
        // Fall through.
    case BeforeHTMLMode:
        ASSERT(insertionMode() == BeforeHTMLMode);
        if (token.name() == htmlTag) {
            m_tree.insertHTMLHtmlStartTagBeforeHTML(token);
            setInsertionMode(BeforeHeadMode);
            return;
        }
        defaultForBeforeHTML();
        // Fall through.
    case BeforeHeadMode:
        ASSERT(insertionMode() == BeforeHeadMode);
        if (token.name() == htmlTag) {
            m_tree.insertHTMLHtmlStartTagInBody(token);
            return;
        }
        if (token.name() == headTag) {
            m_tree.insertHTMLHeadElement(token);
            setInsertionMode(InHeadMode);
            return;
        }
        defaultForBeforeHead();
        // Fall through.
    case InHeadMode:
        ASSERT(insertionMode() == InHeadMode);
        if (processStartTagForInHead(token))
            return;
        defaultForInHead();
        // Fall through.
    case AfterHeadMode:
        ASSERT(insertionMode() == AfterHeadMode);
        if (token.name() == htmlTag) {
            m_tree.insertHTMLHtmlStartTagInBody(token);
            return;
        }
        if (token.name() == bodyTag) {
            m_framesetOk = false;
            m_tree.insertHTMLBodyElement(token);
            setInsertionMode(InBodyMode);
            return;
        }
        if (token.name() == framesetTag) {
            m_tree.insertHTMLElement(token);
            setInsertionMode(InFramesetMode);
            return;
        }
        if (token.name() == baseTag
            || token.name() == basefontTag
            || token.name() == bgsoundTag
            || token.name() == linkTag
            || token.name() == metaTag
            || token.name() == noframesTag
            || token.name() == scriptTag
            || token.name() == styleTag
            || token.name() == titleTag) {
            parseError(token);
            ASSERT(m_tree.head());
            m_tree.openElements()->pushHTMLHeadElement(m_tree.head());
            processStartTagForInHead(token);
            m_tree.openElements()->removeHTMLHeadElement(m_tree.head());
            return;
        }
        if (token.name() == headTag) {
            parseError(token);
            return;
        }
        defaultForAfterHead();
        // Fall through
    case InBodyMode:
        ASSERT(insertionMode() == InBodyMode);
        processStartTagForInBody(token);
        break;
    case InTableMode:
        ASSERT(insertionMode() == InTableMode);
        processStartTagForInTable(token);
        break;
    case InCaptionMode:
        ASSERT(insertionMode() == InCaptionMode);
        if (isCaptionColOrColgroupTag(token.name())
            || isTableBodyContextTag(token.name())
            || isTableCellContextTag(token.name())
            || token.name() == trTag) {
            parseError(token);
            if (!processCaptionEndTagForInCaption()) {
                ASSERT(isParsingFragment());
                return;
            }
            reprocessStartTag(token);
            return;
        }
        processStartTagForInBody(token);
        break;
    case InColumnGroupMode:
        ASSERT(insertionMode() == InColumnGroupMode);
        if (token.name() == htmlTag) {
            m_tree.insertHTMLHtmlStartTagInBody(token);
            return;
        }
        if (token.name() == colTag) {
            m_tree.insertSelfClosingHTMLElement(token);
            return;
        }
        if (!processColgroupEndTagForInColumnGroup()) {
            ASSERT(isParsingFragment());
            return;
        }
        reprocessStartTag(token);
        break;
    case InTableBodyMode:
        ASSERT(insertionMode() == InTableBodyMode);
        if (token.name() == trTag) {
            m_tree.openElements()->popUntilTableBodyScopeMarker(); // How is there ever anything to pop?
            m_tree.insertHTMLElement(token);
            setInsertionMode(InRowMode);
            return;
        }
        if (isTableCellContextTag(token.name())) {
            parseError(token);
            processFakeStartTag(trTag);
            ASSERT(insertionMode() == InRowMode);
            reprocessStartTag(token);
            return;
        }
        if (isCaptionColOrColgroupTag(token.name()) || isTableBodyContextTag(token.name())) {
            // FIXME: This is slow.
            if (!m_tree.openElements()->inTableScope(tbodyTag.localName()) && !m_tree.openElements()->inTableScope(theadTag.localName()) && !m_tree.openElements()->inTableScope(tfootTag.localName())) {
                ASSERT(isParsingFragment());
                parseError(token);
                return;
            }
            m_tree.openElements()->popUntilTableBodyScopeMarker();
            ASSERT(isTableBodyContextTag(m_tree.currentElement()->localName()));
            processFakeEndTag(m_tree.currentElement()->tagQName());
            reprocessStartTag(token);
            return;
        }
        processStartTagForInTable(token);
        break;
    case InRowMode:
        ASSERT(insertionMode() == InRowMode);
        if (isTableCellContextTag(token.name())) {
            m_tree.openElements()->popUntilTableRowScopeMarker();
            m_tree.insertHTMLElement(token);
            setInsertionMode(InCellMode);
            m_tree.activeFormattingElements()->appendMarker();
            return;
        }
        if (token.name() == trTag
            || isCaptionColOrColgroupTag(token.name())
            || isTableBodyContextTag(token.name())) {
            if (!processTrEndTagForInRow()) {
                ASSERT(isParsingFragment());
                return;
            }
            ASSERT(insertionMode() == InTableBodyMode);
            reprocessStartTag(token);
            return;
        }
        processStartTagForInTable(token);
        break;
    case InCellMode:
        ASSERT(insertionMode() == InCellMode);
        if (isCaptionColOrColgroupTag(token.name())
            || isTableCellContextTag(token.name())
            || token.name() == trTag
            || isTableBodyContextTag(token.name())) {
            // FIXME: This could be more efficient.
            if (!m_tree.openElements()->inTableScope(tdTag) && !m_tree.openElements()->inTableScope(thTag)) {
                ASSERT(isParsingFragment());
                parseError(token);
                return;
            }
            closeTheCell();
            reprocessStartTag(token);
            return;
        }
        processStartTagForInBody(token);
        break;
    case AfterBodyMode:
    case AfterAfterBodyMode:
        ASSERT(insertionMode() == AfterBodyMode || insertionMode() == AfterAfterBodyMode);
        if (token.name() == htmlTag) {
            m_tree.insertHTMLHtmlStartTagInBody(token);
            return;
        }
        setInsertionMode(InBodyMode);
        reprocessStartTag(token);
        break;
    case InHeadNoscriptMode:
        ASSERT(insertionMode() == InHeadNoscriptMode);
        if (token.name() == htmlTag) {
            m_tree.insertHTMLHtmlStartTagInBody(token);
            return;
        }
        if (token.name() == basefontTag
            || token.name() == bgsoundTag
            || token.name() == linkTag
            || token.name() == metaTag
            || token.name() == noframesTag
            || token.name() == styleTag) {
            bool didProcess = processStartTagForInHead(token);
            ASSERT_UNUSED(didProcess, didProcess);
            return;
        }
        if (token.name() == htmlTag || token.name() == noscriptTag) {
            parseError(token);
            return;
        }
        defaultForInHeadNoscript();
        processToken(token);
        break;
    case InFramesetMode:
        ASSERT(insertionMode() == InFramesetMode);
        if (token.name() == htmlTag) {
            m_tree.insertHTMLHtmlStartTagInBody(token);
            return;
        }
        if (token.name() == framesetTag) {
            m_tree.insertHTMLElement(token);
            return;
        }
        if (token.name() == frameTag) {
            m_tree.insertSelfClosingHTMLElement(token);
            return;
        }
        if (token.name() == noframesTag) {
            processStartTagForInHead(token);
            return;
        }
        parseError(token);
        break;
    case AfterFramesetMode:
    case AfterAfterFramesetMode:
        ASSERT(insertionMode() == AfterFramesetMode || insertionMode() == AfterAfterFramesetMode);
        if (token.name() == htmlTag) {
            m_tree.insertHTMLHtmlStartTagInBody(token);
            return;
        }
        if (token.name() == noframesTag) {
            processStartTagForInHead(token);
            return;
        }
        parseError(token);
        break;
    case InSelectInTableMode:
        ASSERT(insertionMode() == InSelectInTableMode);
        if (token.name() == captionTag
            || token.name() == tableTag
            || isTableBodyContextTag(token.name())
            || token.name() == trTag
            || isTableCellContextTag(token.name())) {
            parseError(token);
            AtomicHTMLToken endSelect(HTMLToken::EndTag, selectTag.localName());
            processEndTag(endSelect);
            reprocessStartTag(token);
            return;
        }
        // Fall through
    case InSelectMode:
        ASSERT(insertionMode() == InSelectMode || insertionMode() == InSelectInTableMode);
        if (token.name() == htmlTag) {
            m_tree.insertHTMLHtmlStartTagInBody(token);
            return;
        }
        if (token.name() == optionTag) {
            if (m_tree.currentNode()->hasTagName(optionTag)) {
                AtomicHTMLToken endOption(HTMLToken::EndTag, optionTag.localName());
                processEndTag(endOption);
            }
            m_tree.insertHTMLElement(token);
            return;
        }
        if (token.name() == optgroupTag) {
            if (m_tree.currentNode()->hasTagName(optionTag)) {
                AtomicHTMLToken endOption(HTMLToken::EndTag, optionTag.localName());
                processEndTag(endOption);
            }
            if (m_tree.currentNode()->hasTagName(optgroupTag)) {
                AtomicHTMLToken endOptgroup(HTMLToken::EndTag, optgroupTag.localName());
                processEndTag(endOptgroup);
            }
            m_tree.insertHTMLElement(token);
            return;
        }
        if (token.name() == selectTag) {
            parseError(token);
            AtomicHTMLToken endSelect(HTMLToken::EndTag, selectTag.localName());
            processEndTag(endSelect);
            return;
        }
        if (token.name() == inputTag
            || token.name() == keygenTag
            || token.name() == textareaTag) {
            parseError(token);
            if (!m_tree.openElements()->inSelectScope(selectTag)) {
                ASSERT(isParsingFragment());
                return;
            }
            AtomicHTMLToken endSelect(HTMLToken::EndTag, selectTag.localName());
            processEndTag(endSelect);
            reprocessStartTag(token);
            return;
        }
        if (token.name() == scriptTag) {
            bool didProcess = processStartTagForInHead(token);
            ASSERT_UNUSED(didProcess, didProcess);
            return;
        }
        break;
    case InTableTextMode:
        defaultForInTableText();
        processStartTag(token);
        break;
    case InForeignContentMode: {
        if (shouldProcessForeignContentUsingInBodyInsertionMode(token, m_tree.currentNode())) {
            processForeignContentUsingInBodyModeAndResetMode(token);
            return;
        }
        if (token.name() == bTag
            || token.name() == bigTag
            || token.name() == blockquoteTag
            || token.name() == bodyTag
            || token.name() == brTag
            || token.name() == centerTag
            || token.name() == codeTag
            || token.name() == ddTag
            || token.name() == divTag
            || token.name() == dlTag
            || token.name() == dtTag
            || token.name() == emTag
            || token.name() == embedTag
            || isNumberedHeaderTag(token.name())
            || token.name() == headTag
            || token.name() == hrTag
            || token.name() == iTag
            || token.name() == imgTag
            || token.name() == liTag
            || token.name() == listingTag
            || token.name() == menuTag
            || token.name() == metaTag
            || token.name() == nobrTag
            || token.name() == olTag
            || token.name() == pTag
            || token.name() == preTag
            || token.name() == rubyTag
            || token.name() == sTag
            || token.name() == smallTag
            || token.name() == spanTag
            || token.name() == strongTag
            || token.name() == strikeTag
            || token.name() == subTag
            || token.name() == supTag
            || token.name() == tableTag
            || token.name() == ttTag
            || token.name() == uTag
            || token.name() == ulTag
            || token.name() == varTag
            || (token.name() == fontTag && (token.getAttributeItem(colorAttr) || token.getAttributeItem(faceAttr) || token.getAttributeItem(sizeAttr)))) {
            parseError(token);
            m_tree.openElements()->popUntilForeignContentScopeMarker();
            resetInsertionModeAppropriately();
            reprocessStartTag(token);
            return;
        }
        const AtomicString& currentNamespace = m_tree.currentElement()->namespaceURI();
        if (currentNamespace == MathMLNames::mathmlNamespaceURI)
            adjustMathMLAttributes(token);
        if (currentNamespace == SVGNames::svgNamespaceURI) {
            adjustSVGTagNameCase(token);
            adjustSVGAttributes(token);
        }
        adjustForeignAttributes(token);
        m_tree.insertForeignElement(token, currentNamespace);
        break;
    }
    case TextMode:
        ASSERT_NOT_REACHED();
        break;
    }
}

bool HTMLTreeBuilder::processBodyEndTagForInBody(AtomicHTMLToken& token)
{
    ASSERT(token.type() == HTMLToken::EndTag);
    ASSERT(token.name() == bodyTag);
    if (!m_tree.openElements()->inScope(bodyTag.localName())) {
        parseError(token);
        return false;
    }
    notImplemented(); // Emit a more specific parse error based on stack contents.
    setInsertionMode(AfterBodyMode);
    return true;
}

void HTMLTreeBuilder::processAnyOtherEndTagForInBody(AtomicHTMLToken& token)
{
    ASSERT(token.type() == HTMLToken::EndTag);
    HTMLElementStack::ElementRecord* record = m_tree.openElements()->topRecord();
    while (1) {
        RefPtr<ContainerNode> node = record->node();
        if (node->hasLocalName(token.name())) {
            m_tree.generateImpliedEndTags();
            // FIXME: The ElementRecord pointed to by record might be deleted by
            // the preceding call. Perhaps we should hold a RefPtr so that it
            // stays alive for the duration of record's scope.
            record = 0;
            if (!m_tree.currentNode()->hasLocalName(token.name())) {
                parseError(token);
                // FIXME: This is either a bug in the spec, or a bug in our
                // implementation.  Filed a bug with HTML5:
                // http://www.w3.org/Bugs/Public/show_bug.cgi?id=10080
                // We might have already popped the node for the token in
                // generateImpliedEndTags, just abort.
                if (!m_tree.openElements()->contains(toElement(node.get())))
                    return;
            }
            m_tree.openElements()->popUntilPopped(toElement(node.get()));
            return;
        }
        if (isSpecialNode(node.get())) {
            parseError(token);
            return;
        }
        record = record->next();
    }
}

// FIXME: This probably belongs on HTMLElementStack.
HTMLElementStack::ElementRecord* HTMLTreeBuilder::furthestBlockForFormattingElement(Element* formattingElement)
{
    HTMLElementStack::ElementRecord* furthestBlock = 0;
    HTMLElementStack::ElementRecord* record = m_tree.openElements()->topRecord();
    for (; record; record = record->next()) {
        if (record->element() == formattingElement)
            return furthestBlock;
        if (isSpecialNode(record->element()))
            furthestBlock = record;
    }
    ASSERT_NOT_REACHED();
    return 0;
}

// http://www.whatwg.org/specs/web-apps/current-work/multipage/tokenization.html#parsing-main-inbody
void HTMLTreeBuilder::callTheAdoptionAgency(AtomicHTMLToken& token)
{
    // The adoption agency algorithm is N^2.  We limit the number of iterations
    // to stop from hanging the whole browser.  This limit is copied from the
    // legacy tree builder and might need to be tweaked in the future.
    static const int adoptionAgencyIterationLimit = 10;

    for (int i = 0; i < adoptionAgencyIterationLimit; ++i) {
        // 1.
        Element* formattingElement = m_tree.activeFormattingElements()->closestElementInScopeWithName(token.name());
        if (!formattingElement || ((m_tree.openElements()->contains(formattingElement)) && !m_tree.openElements()->inScope(formattingElement))) {
            parseError(token);
            notImplemented(); // Check the stack of open elements for a more specific parse error.
            return;
        }
        HTMLElementStack::ElementRecord* formattingElementRecord = m_tree.openElements()->find(formattingElement);
        if (!formattingElementRecord) {
            parseError(token);
            m_tree.activeFormattingElements()->remove(formattingElement);
            return;
        }
        if (formattingElement != m_tree.currentElement())
            parseError(token);
        // 2.
        HTMLElementStack::ElementRecord* furthestBlock = furthestBlockForFormattingElement(formattingElement);
        // 3.
        if (!furthestBlock) {
            m_tree.openElements()->popUntilPopped(formattingElement);
            m_tree.activeFormattingElements()->remove(formattingElement);
            return;
        }
        // 4.
        ASSERT(furthestBlock->isAbove(formattingElementRecord));
        RefPtr<ContainerNode> commonAncestor = formattingElementRecord->next()->node();
        // 5.
        HTMLFormattingElementList::Bookmark bookmark = m_tree.activeFormattingElements()->bookmarkFor(formattingElement);
        // 6.
        HTMLElementStack::ElementRecord* node = furthestBlock;
        HTMLElementStack::ElementRecord* nextNode = node->next();
        HTMLElementStack::ElementRecord* lastNode = furthestBlock;
        for (int i = 0; i < adoptionAgencyIterationLimit; ++i) {
            // 6.1
            node = nextNode;
            ASSERT(node);
            nextNode = node->next(); // Save node->next() for the next iteration in case node is deleted in 6.2.
            // 6.2
            if (!m_tree.activeFormattingElements()->contains(node->element())) {
                m_tree.openElements()->remove(node->element());
                node = 0;
                continue;
            }
            // 6.3
            if (node == formattingElementRecord)
                break;
            // 6.5
            RefPtr<Element> newElement = m_tree.createHTMLElementFromElementRecord(node);
            HTMLFormattingElementList::Entry* nodeEntry = m_tree.activeFormattingElements()->find(node->element());
            nodeEntry->replaceElement(newElement.get());
            node->replaceElement(newElement.release());
            // 6.4 -- Intentionally out of order to handle the case where node
            // was replaced in 6.5.
            // http://www.w3.org/Bugs/Public/show_bug.cgi?id=10096
            if (lastNode == furthestBlock)
                bookmark.moveToAfter(nodeEntry);
            // 6.6
            if (Element* parent = lastNode->element()->parentElement())
                parent->parserRemoveChild(lastNode->element());
            node->element()->parserAddChild(lastNode->element());
            if (lastNode->element()->parentElement()->attached() && !lastNode->element()->attached())
                lastNode->element()->lazyAttach();
            // 6.7
            lastNode = node;
        }
        // 7
        const AtomicString& commonAncestorTag = commonAncestor->localName();
        if (Element* parent = lastNode->element()->parentElement())
            parent->parserRemoveChild(lastNode->element());
        // FIXME: If this moves to HTMLConstructionSite, this check should use
        // causesFosterParenting(tagName) instead.
        if (commonAncestorTag == tableTag
            || commonAncestorTag == trTag
            || isTableBodyContextTag(commonAncestorTag))
            m_tree.fosterParent(lastNode->element());
        else {
            commonAncestor->parserAddChild(lastNode->element());
            ASSERT(lastNode->node()->isElementNode());
            ASSERT(lastNode->element()->parentNode());
            if (lastNode->element()->parentNode()->attached() && !lastNode->element()->attached())
                lastNode->element()->lazyAttach();
        }
        // 8
        RefPtr<Element> newElement = m_tree.createHTMLElementFromElementRecord(formattingElementRecord);
        // 9
        newElement->takeAllChildrenFrom(furthestBlock->element());
        // 10
        Element* furthestBlockElement = furthestBlock->element();
        // FIXME: All this creation / parserAddChild / attach business should
        //        be in HTMLConstructionSite.  My guess is that steps 8--12
        //        should all be in some HTMLConstructionSite function.
        furthestBlockElement->parserAddChild(newElement);
        if (furthestBlockElement->attached() && !newElement->attached()) {
            // Notice that newElement might already be attached if, for example, one of the reparented
            // children is a style element, which attaches itself automatically.
            newElement->attach();
        }
        // 11
        m_tree.activeFormattingElements()->swapTo(formattingElement, newElement.get(), bookmark);
        // 12
        m_tree.openElements()->remove(formattingElement);
        m_tree.openElements()->insertAbove(newElement, furthestBlock);
    }
}

void HTMLTreeBuilder::resetInsertionModeAppropriately()
{
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#reset-the-insertion-mode-appropriately
    bool last = false;
    HTMLElementStack::ElementRecord* nodeRecord = m_tree.openElements()->topRecord();
    while (1) {
        ContainerNode* node = nodeRecord->node();
        if (node == m_tree.openElements()->rootNode()) {
            ASSERT(isParsingFragment());
            last = true;
            node = m_fragmentContext.contextElement();
        }
        if (node->hasTagName(selectTag)) {
            ASSERT(isParsingFragment());
            return setInsertionMode(InSelectMode);
        }
        if (node->hasTagName(tdTag) || node->hasTagName(thTag))
            return setInsertionMode(InCellMode);
        if (node->hasTagName(trTag))
            return setInsertionMode(InRowMode);
        if (node->hasTagName(tbodyTag) || node->hasTagName(theadTag) || node->hasTagName(tfootTag))
            return setInsertionMode(InTableBodyMode);
        if (node->hasTagName(captionTag))
            return setInsertionMode(InCaptionMode);
        if (node->hasTagName(colgroupTag)) {
            ASSERT(isParsingFragment());
            return setInsertionMode(InColumnGroupMode);
        }
        if (node->hasTagName(tableTag))
            return setInsertionMode(InTableMode);
        if (node->hasTagName(headTag)) {
            ASSERT(isParsingFragment());
            return setInsertionMode(InBodyMode);
        }
        if (node->hasTagName(bodyTag))
            return setInsertionMode(InBodyMode);
        if (node->hasTagName(framesetTag)) {
            ASSERT(isParsingFragment());
            return setInsertionMode(InFramesetMode);
        }
        if (node->hasTagName(htmlTag)) {
            ASSERT(isParsingFragment());
            return setInsertionMode(BeforeHeadMode);
        }
        if (node->namespaceURI() == SVGNames::svgNamespaceURI
            || node->namespaceURI() == MathMLNames::mathmlNamespaceURI)
            return setInsertionMode(InForeignContentMode);
        if (last) {
            ASSERT(isParsingFragment());
            return setInsertionMode(InBodyMode);
        }
        nodeRecord = nodeRecord->next();
    }
}

void HTMLTreeBuilder::processEndTagForInTableBody(AtomicHTMLToken& token)
{
    ASSERT(token.type() == HTMLToken::EndTag);
    if (isTableBodyContextTag(token.name())) {
        if (!m_tree.openElements()->inTableScope(token.name())) {
            parseError(token);
            return;
        }
        m_tree.openElements()->popUntilTableBodyScopeMarker();
        m_tree.openElements()->pop();
        setInsertionMode(InTableMode);
        return;
    }
    if (token.name() == tableTag) {
        // FIXME: This is slow.
        if (!m_tree.openElements()->inTableScope(tbodyTag.localName()) && !m_tree.openElements()->inTableScope(theadTag.localName()) && !m_tree.openElements()->inTableScope(tfootTag.localName())) {
            ASSERT(isParsingFragment());
            parseError(token);
            return;
        }
        m_tree.openElements()->popUntilTableBodyScopeMarker();
        ASSERT(isTableBodyContextTag(m_tree.currentElement()->localName()));
        processFakeEndTag(m_tree.currentElement()->tagQName());
        reprocessEndTag(token);
        return;
    }
    if (token.name() == bodyTag
        || isCaptionColOrColgroupTag(token.name())
        || token.name() == htmlTag
        || isTableCellContextTag(token.name())
        || token.name() == trTag) {
        parseError(token);
        return;
    }
    processEndTagForInTable(token);
}

void HTMLTreeBuilder::processEndTagForInRow(AtomicHTMLToken& token)
{
    ASSERT(token.type() == HTMLToken::EndTag);
    if (token.name() == trTag) {
        processTrEndTagForInRow();
        return;
    }
    if (token.name() == tableTag) {
        if (!processTrEndTagForInRow()) {
            ASSERT(isParsingFragment());
            return;
        }
        ASSERT(insertionMode() == InTableBodyMode);
        reprocessEndTag(token);
        return;
    }
    if (isTableBodyContextTag(token.name())) {
        if (!m_tree.openElements()->inTableScope(token.name())) {
            parseError(token);
            return;
        }
        processFakeEndTag(trTag);
        ASSERT(insertionMode() == InTableBodyMode);
        reprocessEndTag(token);
        return;
    }
    if (token.name() == bodyTag
        || isCaptionColOrColgroupTag(token.name())
        || token.name() == htmlTag
        || isTableCellContextTag(token.name())) {
        parseError(token);
        return;
    }
    processEndTagForInTable(token);
}

void HTMLTreeBuilder::processEndTagForInCell(AtomicHTMLToken& token)
{
    ASSERT(token.type() == HTMLToken::EndTag);
    if (isTableCellContextTag(token.name())) {
        if (!m_tree.openElements()->inTableScope(token.name())) {
            parseError(token);
            return;
        }
        m_tree.generateImpliedEndTags();
        if (!m_tree.currentNode()->hasLocalName(token.name()))
            parseError(token);
        m_tree.openElements()->popUntilPopped(token.name());
        m_tree.activeFormattingElements()->clearToLastMarker();
        setInsertionMode(InRowMode);
        return;
    }
    if (token.name() == bodyTag
        || isCaptionColOrColgroupTag(token.name())
        || token.name() == htmlTag) {
        parseError(token);
        return;
    }
    if (token.name() == tableTag
        || token.name() == trTag
        || isTableBodyContextTag(token.name())) {
        if (!m_tree.openElements()->inTableScope(token.name())) {
            ASSERT(isTableBodyContextTag(token.name()) || isParsingFragment());
            parseError(token);
            return;
        }
        closeTheCell();
        reprocessEndTag(token);
        return;
    }
    processEndTagForInBody(token);
}

void HTMLTreeBuilder::processEndTagForInBody(AtomicHTMLToken& token)
{
    ASSERT(token.type() == HTMLToken::EndTag);
    if (token.name() == bodyTag) {
        processBodyEndTagForInBody(token);
        return;
    }
    if (token.name() == htmlTag) {
        AtomicHTMLToken endBody(HTMLToken::EndTag, bodyTag.localName());
        if (processBodyEndTagForInBody(endBody))
            reprocessEndTag(token);
        return;
    }
    if (token.name() == addressTag
        || token.name() == articleTag
        || token.name() == asideTag
        || token.name() == blockquoteTag
        || token.name() == buttonTag
        || token.name() == centerTag
        || token.name() == detailsTag
        || token.name() == dirTag
        || token.name() == divTag
        || token.name() == dlTag
        || token.name() == fieldsetTag
        || token.name() == figcaptionTag
        || token.name() == figureTag
        || token.name() == footerTag
        || token.name() == headerTag
        || token.name() == hgroupTag
        || token.name() == listingTag
        || token.name() == menuTag
        || token.name() == navTag
        || token.name() == olTag
        || token.name() == preTag
        || token.name() == sectionTag
        || token.name() == summaryTag
        || token.name() == ulTag) {
        if (!m_tree.openElements()->inScope(token.name())) {
            parseError(token);
            return;
        }
        m_tree.generateImpliedEndTags();
        if (!m_tree.currentNode()->hasLocalName(token.name()))
            parseError(token);
        m_tree.openElements()->popUntilPopped(token.name());
        return;
    }
    if (token.name() == formTag) {
        RefPtr<Element> node = m_tree.takeForm();
        if (!node || !m_tree.openElements()->inScope(node.get())) {
            parseError(token);
            return;
        }
        m_tree.generateImpliedEndTags();
        if (m_tree.currentElement() != node.get())
            parseError(token);
        m_tree.openElements()->remove(node.get());
    }
    if (token.name() == pTag) {
        if (!m_tree.openElements()->inButtonScope(token.name())) {
            parseError(token);
            processFakeStartTag(pTag);
            ASSERT(m_tree.openElements()->inScope(token.name()));
            reprocessEndTag(token);
            return;
        }
        m_tree.generateImpliedEndTagsWithExclusion(token.name());
        if (!m_tree.currentNode()->hasLocalName(token.name()))
            parseError(token);
        m_tree.openElements()->popUntilPopped(token.name());
        return;
    }
    if (token.name() == liTag) {
        if (!m_tree.openElements()->inListItemScope(token.name())) {
            parseError(token);
            return;
        }
        m_tree.generateImpliedEndTagsWithExclusion(token.name());
        if (!m_tree.currentNode()->hasLocalName(token.name()))
            parseError(token);
        m_tree.openElements()->popUntilPopped(token.name());
        return;
    }
    if (token.name() == ddTag
        || token.name() == dtTag) {
        if (!m_tree.openElements()->inScope(token.name())) {
            parseError(token);
            return;
        }
        m_tree.generateImpliedEndTagsWithExclusion(token.name());
        if (!m_tree.currentNode()->hasLocalName(token.name()))
            parseError(token);
        m_tree.openElements()->popUntilPopped(token.name());
        return;
    }
    if (isNumberedHeaderTag(token.name())) {
        if (!m_tree.openElements()->hasNumberedHeaderElementInScope()) {
            parseError(token);
            return;
        }
        m_tree.generateImpliedEndTags();
        if (!m_tree.currentNode()->hasLocalName(token.name()))
            parseError(token);
        m_tree.openElements()->popUntilNumberedHeaderElementPopped();
        return;
    }
    if (isFormattingTag(token.name())) {
        callTheAdoptionAgency(token);
        return;
    }
    if (token.name() == appletTag
        || token.name() == marqueeTag
        || token.name() == objectTag) {
        if (!m_tree.openElements()->inScope(token.name())) {
            parseError(token);
            return;
        }
        m_tree.generateImpliedEndTags();
        if (!m_tree.currentNode()->hasLocalName(token.name()))
            parseError(token);
        m_tree.openElements()->popUntilPopped(token.name());
        m_tree.activeFormattingElements()->clearToLastMarker();
        return;
    }
    if (token.name() == brTag) {
        parseError(token);
        processFakeStartTag(brTag);
        return;
    }
    processAnyOtherEndTagForInBody(token);
}

bool HTMLTreeBuilder::processCaptionEndTagForInCaption()
{
    if (!m_tree.openElements()->inTableScope(captionTag.localName())) {
        ASSERT(isParsingFragment());
        // FIXME: parse error
        return false;
    }
    m_tree.generateImpliedEndTags();
    // FIXME: parse error if (!m_tree.currentElement()->hasTagName(captionTag))
    m_tree.openElements()->popUntilPopped(captionTag.localName());
    m_tree.activeFormattingElements()->clearToLastMarker();
    setInsertionMode(InTableMode);
    return true;
}

bool HTMLTreeBuilder::processTrEndTagForInRow()
{
    if (!m_tree.openElements()->inTableScope(trTag.localName())) {
        ASSERT(isParsingFragment());
        // FIXME: parse error
        return false;
    }
    m_tree.openElements()->popUntilTableRowScopeMarker();
    ASSERT(m_tree.currentElement()->hasTagName(trTag));
    m_tree.openElements()->pop();
    setInsertionMode(InTableBodyMode);
    return true;
}

bool HTMLTreeBuilder::processTableEndTagForInTable()
{
    if (!m_tree.openElements()->inTableScope(tableTag)) {
        ASSERT(isParsingFragment());
        // FIXME: parse error.
        return false;
    }
    m_tree.openElements()->popUntilPopped(tableTag.localName());
    resetInsertionModeAppropriately();
    return true;
}

void HTMLTreeBuilder::processEndTagForInTable(AtomicHTMLToken& token)
{
    ASSERT(token.type() == HTMLToken::EndTag);
    if (token.name() == tableTag) {
        processTableEndTagForInTable();
        return;
    }
    if (token.name() == bodyTag
        || isCaptionColOrColgroupTag(token.name())
        || token.name() == htmlTag
        || isTableBodyContextTag(token.name())
        || isTableCellContextTag(token.name())
        || token.name() == trTag) {
        parseError(token);
        return;
    }
    // Is this redirection necessary here?
    HTMLConstructionSite::RedirectToFosterParentGuard redirecter(m_tree);
    processEndTagForInBody(token);
}

void HTMLTreeBuilder::processEndTag(AtomicHTMLToken& token)
{
    ASSERT(token.type() == HTMLToken::EndTag);
    switch (insertionMode()) {
    case InitialMode:
        ASSERT(insertionMode() == InitialMode);
        defaultForInitial();
        // Fall through.
    case BeforeHTMLMode:
        ASSERT(insertionMode() == BeforeHTMLMode);
        if (token.name() != headTag && token.name() != bodyTag && token.name() != htmlTag && token.name() != brTag) {
            parseError(token);
            return;
        }
        defaultForBeforeHTML();
        // Fall through.
    case BeforeHeadMode:
        ASSERT(insertionMode() == BeforeHeadMode);
        if (token.name() != headTag && token.name() != bodyTag && token.name() != htmlTag && token.name() != brTag) {
            parseError(token);
            return;
        }
        defaultForBeforeHead();
        // Fall through.
    case InHeadMode:
        ASSERT(insertionMode() == InHeadMode);
        if (token.name() == headTag) {
            m_tree.openElements()->popHTMLHeadElement();
            setInsertionMode(AfterHeadMode);
            return;
        }
        if (token.name() != bodyTag && token.name() != htmlTag && token.name() != brTag) {
            parseError(token);
            return;
        }
        defaultForInHead();
        // Fall through.
    case AfterHeadMode:
        ASSERT(insertionMode() == AfterHeadMode);
        if (token.name() != bodyTag && token.name() != htmlTag && token.name() != brTag) {
            parseError(token);
            return;
        }
        defaultForAfterHead();
        // Fall through
    case InBodyMode:
        ASSERT(insertionMode() == InBodyMode);
        processEndTagForInBody(token);
        break;
    case InTableMode:
        ASSERT(insertionMode() == InTableMode);
        processEndTagForInTable(token);
        break;
    case InCaptionMode:
        ASSERT(insertionMode() == InCaptionMode);
        if (token.name() == captionTag) {
            processCaptionEndTagForInCaption();
            return;
        }
        if (token.name() == tableTag) {
            parseError(token);
            if (!processCaptionEndTagForInCaption()) {
                ASSERT(isParsingFragment());
                return;
            }
            reprocessEndTag(token);
            return;
        }
        if (token.name() == bodyTag
            || token.name() == colTag
            || token.name() == colgroupTag
            || token.name() == htmlTag
            || isTableBodyContextTag(token.name())
            || isTableCellContextTag(token.name())
            || token.name() == trTag) {
            parseError(token);
            return;
        }
        processEndTagForInBody(token);
        break;
    case InColumnGroupMode:
        ASSERT(insertionMode() == InColumnGroupMode);
        if (token.name() == colgroupTag) {
            processColgroupEndTagForInColumnGroup();
            return;
        }
        if (token.name() == colTag) {
            parseError(token);
            return;
        }
        if (!processColgroupEndTagForInColumnGroup()) {
            ASSERT(isParsingFragment());
            return;
        }
        reprocessEndTag(token);
        break;
    case InRowMode:
        ASSERT(insertionMode() == InRowMode);
        processEndTagForInRow(token);
        break;
    case InCellMode:
        ASSERT(insertionMode() == InCellMode);
        processEndTagForInCell(token);
        break;
    case InTableBodyMode:
        ASSERT(insertionMode() == InTableBodyMode);
        processEndTagForInTableBody(token);
        break;
    case AfterBodyMode:
        ASSERT(insertionMode() == AfterBodyMode);
        if (token.name() == htmlTag) {
            if (isParsingFragment()) {
                parseError(token);
                return;
            }
            setInsertionMode(AfterAfterBodyMode);
            return;
        }
        prepareToReprocessToken();
        // Fall through.
    case AfterAfterBodyMode:
        ASSERT(insertionMode() == AfterBodyMode || insertionMode() == AfterAfterBodyMode);
        parseError(token);
        setInsertionMode(InBodyMode);
        reprocessEndTag(token);
        break;
    case InHeadNoscriptMode:
        ASSERT(insertionMode() == InHeadNoscriptMode);
        if (token.name() == noscriptTag) {
            ASSERT(m_tree.currentElement()->hasTagName(noscriptTag));
            m_tree.openElements()->pop();
            ASSERT(m_tree.currentElement()->hasTagName(headTag));
            setInsertionMode(InHeadMode);
            return;
        }
        if (token.name() != brTag) {
            parseError(token);
            return;
        }
        defaultForInHeadNoscript();
        processToken(token);
        break;
    case TextMode:
        if (token.name() == scriptTag) {
            // Pause ourselves so that parsing stops until the script can be processed by the caller.
            m_isPaused = true;
            ASSERT(m_tree.currentElement()->hasTagName(scriptTag));
            m_scriptToProcess = m_tree.currentElement();
            m_scriptToProcessStartPosition = WTF::toOneBasedTextPosition(m_lastScriptElementStartPosition);
            m_tree.openElements()->pop();
            if (isParsingFragment() && m_fragmentContext.scriptingPermission() == FragmentScriptingNotAllowed)
                m_scriptToProcess->removeAllChildren();
            setInsertionMode(m_originalInsertionMode);

            // This token will not have been created by the tokenizer if a
            // self-closing script tag was encountered and pre-HTML5 parser
            // quirks are enabled. We must set the tokenizer's state to
            // DataState explicitly if the tokenizer didn't have a chance to.
            ASSERT(m_parser->tokenizer()->state() == HTMLTokenizer::DataState || m_usePreHTML5ParserQuirks);
            m_parser->tokenizer()->setState(HTMLTokenizer::DataState);
            return;
        }
        m_tree.openElements()->pop();
        setInsertionMode(m_originalInsertionMode);
        break;
    case InFramesetMode:
        ASSERT(insertionMode() == InFramesetMode);
        if (token.name() == framesetTag) {
            if (m_tree.currentNode() == m_tree.openElements()->rootNode()) {
                parseError(token);
                return;
            }
            m_tree.openElements()->pop();
            if (!isParsingFragment() && !m_tree.currentElement()->hasTagName(framesetTag))
                setInsertionMode(AfterFramesetMode);
            return;
        }
        break;
    case AfterFramesetMode:
        ASSERT(insertionMode() == AfterFramesetMode);
        if (token.name() == htmlTag) {
            setInsertionMode(AfterAfterFramesetMode);
            return;
        }
        // Fall through.
    case AfterAfterFramesetMode:
        ASSERT(insertionMode() == AfterFramesetMode || insertionMode() == AfterAfterFramesetMode);
        parseError(token);
        break;
    case InSelectInTableMode:
        ASSERT(insertionMode() == InSelectInTableMode);
        if (token.name() == captionTag
            || token.name() == tableTag
            || isTableBodyContextTag(token.name())
            || token.name() == trTag
            || isTableCellContextTag(token.name())) {
            parseError(token);
            if (m_tree.openElements()->inTableScope(token.name())) {
                AtomicHTMLToken endSelect(HTMLToken::EndTag, selectTag.localName());
                processEndTag(endSelect);
                reprocessEndTag(token);
            }
            return;
        }
        // Fall through.
    case InSelectMode:
        ASSERT(insertionMode() == InSelectMode || insertionMode() == InSelectInTableMode);
        if (token.name() == optgroupTag) {
            if (m_tree.currentNode()->hasTagName(optionTag) && m_tree.oneBelowTop()->hasTagName(optgroupTag))
                processFakeEndTag(optionTag);
            if (m_tree.currentNode()->hasTagName(optgroupTag)) {
                m_tree.openElements()->pop();
                return;
            }
            parseError(token);
            return;
        }
        if (token.name() == optionTag) {
            if (m_tree.currentNode()->hasTagName(optionTag)) {
                m_tree.openElements()->pop();
                return;
            }
            parseError(token);
            return;
        }
        if (token.name() == selectTag) {
            if (!m_tree.openElements()->inSelectScope(token.name())) {
                ASSERT(isParsingFragment());
                parseError(token);
                return;
            }
            m_tree.openElements()->popUntilPopped(selectTag.localName());
            resetInsertionModeAppropriately();
            return;
        }
        break;
    case InTableTextMode:
        defaultForInTableText();
        processEndTag(token);
        break;
    case InForeignContentMode:
        if (token.name() == SVGNames::scriptTag && m_tree.currentNode()->hasTagName(SVGNames::scriptTag)) {
            notImplemented();
            return;
        }
        if (!isInHTMLNamespace(m_tree.currentNode())) {
            // FIXME: This code just wants an Element* iterator, instead of an ElementRecord*
            HTMLElementStack::ElementRecord* nodeRecord = m_tree.openElements()->topRecord();
            if (!nodeRecord->node()->hasLocalName(token.name()))
                parseError(token);
            while (1) {
                if (nodeRecord->node()->hasLocalName(token.name())) {
                    m_tree.openElements()->popUntilPopped(nodeRecord->element());
                    resetForeignInsertionMode();
                    return;
                }
                nodeRecord = nodeRecord->next();
                
                if (isInHTMLNamespace(nodeRecord->node()))
                    break;
            }
        }
        // Any other end tag (also the last two steps of "An end tag, if the current node is not an element in the HTML namespace."
        processForeignContentUsingInBodyModeAndResetMode(token);
        break;
    }
}

void HTMLTreeBuilder::prepareToReprocessToken()
{
    if (m_hasPendingForeignInsertionModeSteps) {
        resetForeignInsertionMode();
        m_hasPendingForeignInsertionModeSteps = false;
    }
}

void HTMLTreeBuilder::reprocessStartTag(AtomicHTMLToken& token)
{
    prepareToReprocessToken();
    processStartTag(token);
}

void HTMLTreeBuilder::reprocessEndTag(AtomicHTMLToken& token)
{
    prepareToReprocessToken();
    processEndTag(token);
}

class HTMLTreeBuilder::FakeInsertionMode {
    WTF_MAKE_NONCOPYABLE(FakeInsertionMode);
public:
    FakeInsertionMode(HTMLTreeBuilder* treeBuilder, InsertionMode mode)
        : m_treeBuilder(treeBuilder)
        , m_originalMode(treeBuilder->insertionMode())
    {
        m_treeBuilder->setFakeInsertionMode(mode);
    }

    ~FakeInsertionMode()
    {
        if (m_treeBuilder->isFakeInsertionMode())
            m_treeBuilder->setInsertionMode(m_originalMode);
    }

private:
    HTMLTreeBuilder* m_treeBuilder;
    InsertionMode m_originalMode;
};

void HTMLTreeBuilder::processForeignContentUsingInBodyModeAndResetMode(AtomicHTMLToken& token)
{
    m_hasPendingForeignInsertionModeSteps = true;
    {
        FakeInsertionMode fakeMode(this, InBodyMode);
        processToken(token);
    }
    if (m_hasPendingForeignInsertionModeSteps)
        resetForeignInsertionMode();
}

void HTMLTreeBuilder::resetForeignInsertionMode()
{
    if (insertionMode() == InForeignContentMode)
        resetInsertionModeAppropriately();
}

void HTMLTreeBuilder::processComment(AtomicHTMLToken& token)
{
    ASSERT(token.type() == HTMLToken::Comment);
    if (m_insertionMode == InitialMode
        || m_insertionMode == BeforeHTMLMode
        || m_insertionMode == AfterAfterBodyMode
        || m_insertionMode == AfterAfterFramesetMode) {
        m_tree.insertCommentOnDocument(token);
        return;
    }
    if (m_insertionMode == AfterBodyMode) {
        m_tree.insertCommentOnHTMLHtmlElement(token);
        return;
    }
    if (m_insertionMode == InTableTextMode) {
        defaultForInTableText();
        processComment(token);
        return;
    }
    m_tree.insertComment(token);
}

void HTMLTreeBuilder::processCharacter(AtomicHTMLToken& token)
{
    ASSERT(token.type() == HTMLToken::Character);
    ExternalCharacterTokenBuffer buffer(token);
    processCharacterBuffer(buffer);
}

void HTMLTreeBuilder::processCharacterBuffer(ExternalCharacterTokenBuffer& buffer)
{
ReprocessBuffer:
    switch (insertionMode()) {
    case InitialMode: {
        ASSERT(insertionMode() == InitialMode);
        buffer.skipLeadingWhitespace();
        if (buffer.isEmpty())
            return;
        defaultForInitial();
        // Fall through.
    }
    case BeforeHTMLMode: {
        ASSERT(insertionMode() == BeforeHTMLMode);
        buffer.skipLeadingWhitespace();
        if (buffer.isEmpty())
            return;
        defaultForBeforeHTML();
        // Fall through.
    }
    case BeforeHeadMode: {
        ASSERT(insertionMode() == BeforeHeadMode);
        buffer.skipLeadingWhitespace();
        if (buffer.isEmpty())
            return;
        defaultForBeforeHead();
        // Fall through.
    }
    case InHeadMode: {
        ASSERT(insertionMode() == InHeadMode);
        String leadingWhitespace = buffer.takeLeadingWhitespace();
        if (!leadingWhitespace.isEmpty())
            m_tree.insertTextNode(leadingWhitespace);
        if (buffer.isEmpty())
            return;
        defaultForInHead();
        // Fall through.
    }
    case AfterHeadMode: {
        ASSERT(insertionMode() == AfterHeadMode);
        String leadingWhitespace = buffer.takeLeadingWhitespace();
        if (!leadingWhitespace.isEmpty())
            m_tree.insertTextNode(leadingWhitespace);
        if (buffer.isEmpty())
            return;
        defaultForAfterHead();
        // Fall through.
    }
    case InBodyMode:
    case InCaptionMode:
    case InCellMode: {
        ASSERT(insertionMode() == InBodyMode || insertionMode() == InCaptionMode || insertionMode() == InCellMode);
        m_tree.reconstructTheActiveFormattingElements();
        String characters = buffer.takeRemaining();
        m_tree.insertTextNode(characters);
        if (m_framesetOk && !isAllWhitespaceOrReplacementCharacters(characters))
            m_framesetOk = false;
        break;
    }
    case InTableMode:
    case InTableBodyMode:
    case InRowMode: {
        ASSERT(insertionMode() == InTableMode || insertionMode() == InTableBodyMode || insertionMode() == InRowMode);
        ASSERT(m_pendingTableCharacters.isEmpty());
        m_originalInsertionMode = m_insertionMode;
        setInsertionMode(InTableTextMode);
        prepareToReprocessToken();
        // Fall through.
    }
    case InTableTextMode: {
        buffer.giveRemainingTo(m_pendingTableCharacters);
        break;
    }
    case InColumnGroupMode: {
        ASSERT(insertionMode() == InColumnGroupMode);
        String leadingWhitespace = buffer.takeLeadingWhitespace();
        if (!leadingWhitespace.isEmpty())
            m_tree.insertTextNode(leadingWhitespace);
        if (buffer.isEmpty())
            return;
        if (!processColgroupEndTagForInColumnGroup()) {
            ASSERT(isParsingFragment());
            // The spec tells us to drop these characters on the floor.
            buffer.takeLeadingNonWhitespace();
            if (buffer.isEmpty())
                return;
        }
        prepareToReprocessToken();
        goto ReprocessBuffer;
    }
    case AfterBodyMode:
    case AfterAfterBodyMode: {
        ASSERT(insertionMode() == AfterBodyMode || insertionMode() == AfterAfterBodyMode);
        // FIXME: parse error
        setInsertionMode(InBodyMode);
        prepareToReprocessToken();
        goto ReprocessBuffer;
        break;
    }
    case TextMode: {
        ASSERT(insertionMode() == TextMode);
        m_tree.insertTextNode(buffer.takeRemaining());
        break;
    }
    case InHeadNoscriptMode: {
        ASSERT(insertionMode() == InHeadNoscriptMode);
        String leadingWhitespace = buffer.takeLeadingWhitespace();
        if (!leadingWhitespace.isEmpty())
            m_tree.insertTextNode(leadingWhitespace);
        if (buffer.isEmpty())
            return;
        defaultForInHeadNoscript();
        goto ReprocessBuffer;
        break;
    }
    case InFramesetMode:
    case AfterFramesetMode: {
        ASSERT(insertionMode() == InFramesetMode || insertionMode() == AfterFramesetMode || insertionMode() == AfterAfterFramesetMode);
        String leadingWhitespace = buffer.takeRemainingWhitespace();
        if (!leadingWhitespace.isEmpty())
            m_tree.insertTextNode(leadingWhitespace);
        // FIXME: We should generate a parse error if we skipped over any
        // non-whitespace characters.
        break;
    }
    case InSelectInTableMode:
    case InSelectMode: {
        ASSERT(insertionMode() == InSelectMode || insertionMode() == InSelectInTableMode);
        m_tree.insertTextNode(buffer.takeRemaining());
        break;
    }
    case InForeignContentMode: {
        ASSERT(insertionMode() == InForeignContentMode);
        String characters = buffer.takeRemaining();
        m_tree.insertTextNode(characters);
        if (m_framesetOk && !isAllWhitespace(characters))
            m_framesetOk = false;
        break;
    }
    case AfterAfterFramesetMode: {
        String leadingWhitespace = buffer.takeRemainingWhitespace();
        if (!leadingWhitespace.isEmpty()) {
            m_tree.reconstructTheActiveFormattingElements();
            m_tree.insertTextNode(leadingWhitespace);
        }
        // FIXME: We should generate a parse error if we skipped over any
        // non-whitespace characters.
        break;
    }
    }
}

void HTMLTreeBuilder::processEndOfFile(AtomicHTMLToken& token)
{
    ASSERT(token.type() == HTMLToken::EndOfFile);
    switch (insertionMode()) {
    case InitialMode:
        ASSERT(insertionMode() == InitialMode);
        defaultForInitial();
        // Fall through.
    case BeforeHTMLMode:
        ASSERT(insertionMode() == BeforeHTMLMode);
        defaultForBeforeHTML();
        // Fall through.
    case BeforeHeadMode:
        ASSERT(insertionMode() == BeforeHeadMode);
        defaultForBeforeHead();
        // Fall through.
    case InHeadMode:
        ASSERT(insertionMode() == InHeadMode);
        defaultForInHead();
        // Fall through.
    case AfterHeadMode:
        ASSERT(insertionMode() == AfterHeadMode);
        defaultForAfterHead();
        // Fall through
    case InBodyMode:
    case InCellMode:
    case InCaptionMode:
    case InRowMode:
        ASSERT(insertionMode() == InBodyMode || insertionMode() == InCellMode || insertionMode() == InCaptionMode || insertionMode() == InRowMode);
        notImplemented(); // Emit parse error based on what elements are still open.
        break;
    case AfterBodyMode:
    case AfterAfterBodyMode:
        ASSERT(insertionMode() == AfterBodyMode || insertionMode() == AfterAfterBodyMode);
        break;
    case InHeadNoscriptMode:
        ASSERT(insertionMode() == InHeadNoscriptMode);
        defaultForInHeadNoscript();
        processEndOfFile(token);
        return;
    case AfterFramesetMode:
    case AfterAfterFramesetMode:
        ASSERT(insertionMode() == AfterFramesetMode || insertionMode() == AfterAfterFramesetMode);
        break;
    case InFramesetMode:
    case InTableMode:
    case InTableBodyMode:
    case InSelectInTableMode:
    case InSelectMode:
        ASSERT(insertionMode() == InSelectMode || insertionMode() == InSelectInTableMode || insertionMode() == InTableMode || insertionMode() == InFramesetMode || insertionMode() == InTableBodyMode);
        if (m_tree.currentNode() != m_tree.openElements()->rootNode())
            parseError(token);
        break;
    case InColumnGroupMode:
        if (m_tree.currentNode() == m_tree.openElements()->rootNode()) {
            ASSERT(isParsingFragment());
            return; // FIXME: Should we break here instead of returning?
        }
        if (!processColgroupEndTagForInColumnGroup()) {
            ASSERT(isParsingFragment());
            return; // FIXME: Should we break here instead of returning?
        }
        prepareToReprocessToken();
        processEndOfFile(token);
        return;
    case InForeignContentMode:
        setInsertionMode(InBodyMode);
        processEndOfFile(token);
        return;
    case InTableTextMode:
        defaultForInTableText();
        processEndOfFile(token);
        return;
    case TextMode:
        parseError(token);
        if (m_tree.currentNode()->hasTagName(scriptTag))
            notImplemented(); // mark the script element as "already started".
        m_tree.openElements()->pop();
        setInsertionMode(m_originalInsertionMode);
        prepareToReprocessToken();
        processEndOfFile(token);
        return;
    }
    ASSERT(m_tree.currentNode());
    m_tree.openElements()->popAll();
}

void HTMLTreeBuilder::defaultForInitial()
{
    notImplemented();
    if (!m_fragmentContext.fragment())
        m_document->setCompatibilityMode(Document::QuirksMode);
    // FIXME: parse error
    setInsertionMode(BeforeHTMLMode);
    prepareToReprocessToken();
}

void HTMLTreeBuilder::defaultForBeforeHTML()
{
    AtomicHTMLToken startHTML(HTMLToken::StartTag, htmlTag.localName());
    m_tree.insertHTMLHtmlStartTagBeforeHTML(startHTML);
    setInsertionMode(BeforeHeadMode);
    prepareToReprocessToken();
}

void HTMLTreeBuilder::defaultForBeforeHead()
{
    AtomicHTMLToken startHead(HTMLToken::StartTag, headTag.localName());
    processStartTag(startHead);
    prepareToReprocessToken();
}

void HTMLTreeBuilder::defaultForInHead()
{
    AtomicHTMLToken endHead(HTMLToken::EndTag, headTag.localName());
    processEndTag(endHead);
    prepareToReprocessToken();
}

void HTMLTreeBuilder::defaultForInHeadNoscript()
{
    AtomicHTMLToken endNoscript(HTMLToken::EndTag, noscriptTag.localName());
    processEndTag(endNoscript);
    prepareToReprocessToken();
}

void HTMLTreeBuilder::defaultForAfterHead()
{
    AtomicHTMLToken startBody(HTMLToken::StartTag, bodyTag.localName());
    processStartTag(startBody);
    m_framesetOk = true;
    prepareToReprocessToken();
}

void HTMLTreeBuilder::defaultForInTableText()
{
    String characters = String::adopt(m_pendingTableCharacters);
    if (!isAllWhitespace(characters)) {
        // FIXME: parse error
        HTMLConstructionSite::RedirectToFosterParentGuard redirecter(m_tree);
        m_tree.reconstructTheActiveFormattingElements();
        m_tree.insertTextNode(characters);
        m_framesetOk = false;
        setInsertionMode(m_originalInsertionMode);
        prepareToReprocessToken();
        return;
    }
    m_tree.insertTextNode(characters);
    setInsertionMode(m_originalInsertionMode);
    prepareToReprocessToken();
}

bool HTMLTreeBuilder::processStartTagForInHead(AtomicHTMLToken& token)
{
    ASSERT(token.type() == HTMLToken::StartTag);
    if (token.name() == htmlTag) {
        m_tree.insertHTMLHtmlStartTagInBody(token);
        return true;
    }
    if (token.name() == baseTag
        || token.name() == basefontTag
        || token.name() == bgsoundTag
        || token.name() == commandTag
        || token.name() == linkTag
        || token.name() == metaTag) {
        m_tree.insertSelfClosingHTMLElement(token);
        // Note: The custom processing for the <meta> tag is done in HTMLMetaElement::process().
        return true;
    }
    if (token.name() == titleTag) {
        processGenericRCDATAStartTag(token);
        return true;
    }
    if (token.name() == noscriptTag) {
        if (scriptEnabled(m_document->frame())) {
            processGenericRawTextStartTag(token);
            return true;
        }
        m_tree.insertHTMLElement(token);
        setInsertionMode(InHeadNoscriptMode);
        return true;
    }
    if (token.name() == noframesTag || token.name() == styleTag) {
        processGenericRawTextStartTag(token);
        return true;
    }
    if (token.name() == scriptTag) {
        processScriptStartTag(token);
        if (m_usePreHTML5ParserQuirks && token.selfClosing())
            processFakeEndTag(scriptTag);
        return true;
    }
    if (token.name() == headTag) {
        parseError(token);
        return true;
    }
    return false;
}

void HTMLTreeBuilder::processGenericRCDATAStartTag(AtomicHTMLToken& token)
{
    ASSERT(token.type() == HTMLToken::StartTag);
    m_tree.insertHTMLElement(token);
    m_parser->tokenizer()->setState(HTMLTokenizer::RCDATAState);
    m_originalInsertionMode = m_insertionMode;
    setInsertionMode(TextMode);
}

void HTMLTreeBuilder::processGenericRawTextStartTag(AtomicHTMLToken& token)
{
    ASSERT(token.type() == HTMLToken::StartTag);
    m_tree.insertHTMLElement(token);
    m_parser->tokenizer()->setState(HTMLTokenizer::RAWTEXTState);
    m_originalInsertionMode = m_insertionMode;
    setInsertionMode(TextMode);
}

void HTMLTreeBuilder::processScriptStartTag(AtomicHTMLToken& token)
{
    ASSERT(token.type() == HTMLToken::StartTag);
    m_tree.insertScriptElement(token);
    m_parser->tokenizer()->setState(HTMLTokenizer::ScriptDataState);
    m_originalInsertionMode = m_insertionMode;

    TextPosition0 position = m_parser->textPosition();

    ASSERT(position.m_line.zeroBasedInt() == m_parser->tokenizer()->lineNumber());

    m_lastScriptElementStartPosition = position;

    setInsertionMode(TextMode);
}

void HTMLTreeBuilder::finished()
{
    if (isParsingFragment())
        return;
    
    ASSERT(m_document);
    // Warning, this may detach the parser. Do not do anything else after this.
    m_document->finishedParsing();
}

void HTMLTreeBuilder::parseError(AtomicHTMLToken&)
{
}

bool HTMLTreeBuilder::scriptEnabled(Frame* frame)
{
    if (!frame)
        return false;
    return frame->script()->canExecuteScripts(NotAboutToExecuteScript);
}

bool HTMLTreeBuilder::pluginsEnabled(Frame* frame)
{
    if (!frame)
        return false;
    return frame->loader()->subframeLoader()->allowPlugins(NotAboutToInstantiatePlugin);
}

}
