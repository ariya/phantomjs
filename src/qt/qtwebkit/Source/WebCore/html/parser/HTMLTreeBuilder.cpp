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

#include "AtomicHTMLToken.h"
#include "DocumentFragment.h"
#include "HTMLDocument.h"
#include "HTMLDocumentParser.h"
#include "HTMLFormElement.h"
#include "HTMLNames.h"
#include "HTMLOptGroupElement.h"
#include "HTMLOptionElement.h"
#include "HTMLParserIdioms.h"
#include "HTMLStackItem.h"
#include "HTMLTableElement.h"
#include "HTMLTemplateElement.h"
#include "HTMLToken.h"
#include "HTMLTokenizer.h"
#include "LocalizedStrings.h"
#include "MathMLNames.h"
#include "NotImplemented.h"
#include "SVGNames.h"
#include "XLinkNames.h"
#include "XMLNSNames.h"
#include "XMLNames.h"
#include <wtf/MainThread.h>
#include <wtf/unicode/CharacterNames.h>

namespace WebCore {

using namespace HTMLNames;

namespace {

inline bool isHTMLSpaceOrReplacementCharacter(UChar character)
{
    return isHTMLSpace(character) || character == replacementCharacter;
}

}

static TextPosition uninitializedPositionValue1()
{
    return TextPosition(OrdinalNumber::fromOneBasedInt(-1), OrdinalNumber::first());
}

static inline bool isAllWhitespace(const String& string)
{
    return string.isAllSpecialCharacters<isHTMLSpace>();
}

static inline bool isAllWhitespaceOrReplacementCharacters(const String& string)
{
    return string.isAllSpecialCharacters<isHTMLSpaceOrReplacementCharacter>();
}

static bool isNumberedHeaderTag(const AtomicString& tagName)
{
    return tagName == h1Tag
        || tagName == h2Tag
        || tagName == h3Tag
        || tagName == h4Tag
        || tagName == h5Tag
        || tagName == h6Tag;
}

static bool isCaptionColOrColgroupTag(const AtomicString& tagName)
{
    return tagName == captionTag
        || tagName == colTag
        || tagName == colgroupTag;
}

static bool isTableCellContextTag(const AtomicString& tagName)
{
    return tagName == thTag || tagName == tdTag;
}

static bool isTableBodyContextTag(const AtomicString& tagName)
{
    return tagName == tbodyTag
        || tagName == tfootTag
        || tagName == theadTag;
}

static bool isNonAnchorNonNobrFormattingTag(const AtomicString& tagName)
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

static bool isNonAnchorFormattingTag(const AtomicString& tagName)
{
    return tagName == nobrTag
        || isNonAnchorNonNobrFormattingTag(tagName);
}

// http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#formatting
static bool isFormattingTag(const AtomicString& tagName)
{
    return tagName == aTag || isNonAnchorFormattingTag(tagName);
}

static HTMLFormElement* closestFormAncestor(Element* element)
{
    ASSERT(isMainThread());
    while (element) {
        if (isHTMLFormElement(element))
            return toHTMLFormElement(element);
        ContainerNode* parent = element->parentNode();
        if (!parent || !parent->isElementNode())
            return 0;
        element = toElement(parent);
    }
    return 0;
}

class HTMLTreeBuilder::ExternalCharacterTokenBuffer {
    WTF_MAKE_NONCOPYABLE(ExternalCharacterTokenBuffer);
public:
    explicit ExternalCharacterTokenBuffer(AtomicHTMLToken* token)
        : m_current(token->characters())
        , m_end(m_current + token->charactersLength())
        , m_isAll8BitData(token->isAll8BitData())
    {
        ASSERT(!isEmpty());
    }

    explicit ExternalCharacterTokenBuffer(const String& string)
        : m_current(string.characters())
        , m_end(m_current + string.length())
        , m_isAll8BitData(string.length() && string.is8Bit())
    {
        ASSERT(!isEmpty());
    }

    ~ExternalCharacterTokenBuffer()
    {
        ASSERT(isEmpty());
    }

    bool isEmpty() const { return m_current == m_end; }

    bool isAll8BitData() const { return m_isAll8BitData; }

    void skipAtMostOneLeadingNewline()
    {
        ASSERT(!isEmpty());
        if (*m_current == '\n')
            ++m_current;
    }

    void skipLeadingWhitespace()
    {
        skipLeading<isHTMLSpace>();
    }

    String takeLeadingWhitespace()
    {
        return takeLeading<isHTMLSpace>();
    }

    void skipLeadingNonWhitespace()
    {
        skipLeading<isNotHTMLSpace>();
    }

    String takeRemaining()
    {
        ASSERT(!isEmpty());
        const UChar* start = m_current;
        m_current = m_end;
        size_t length = m_current - start;

        if (isAll8BitData())
            return String::make8BitFrom16BitSource(start, length);

        return String(start, length);
    }

    void giveRemainingTo(StringBuilder& recipient)
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
        if (isAll8BitData())
            return String::make8BitFrom16BitSource(start, m_current - start);
        return String(start, m_current - start);
    }

    const UChar* m_current;
    const UChar* m_end;
    bool m_isAll8BitData;
};


HTMLTreeBuilder::HTMLTreeBuilder(HTMLDocumentParser* parser, HTMLDocument* document, ParserContentPolicy parserContentPolicy, bool, const HTMLParserOptions& options)
    : m_framesetOk(true)
#ifndef NDEBUG
    , m_isAttached(true)
#endif
    , m_tree(document, parserContentPolicy, options.maximumDOMTreeDepth)
    , m_insertionMode(InitialMode)
    , m_originalInsertionMode(InitialMode)
    , m_shouldSkipLeadingNewline(false)
    , m_parser(parser)
    , m_scriptToProcessStartPosition(uninitializedPositionValue1())
    , m_options(options)
{
}

// FIXME: Member variables should be grouped into self-initializing structs to
// minimize code duplication between these constructors.
HTMLTreeBuilder::HTMLTreeBuilder(HTMLDocumentParser* parser, DocumentFragment* fragment, Element* contextElement, ParserContentPolicy parserContentPolicy, const HTMLParserOptions& options)
    : m_framesetOk(true)
#ifndef NDEBUG
    , m_isAttached(true)
#endif
    , m_fragmentContext(fragment, contextElement)
    , m_tree(fragment, parserContentPolicy, options.maximumDOMTreeDepth)
    , m_insertionMode(InitialMode)
    , m_originalInsertionMode(InitialMode)
    , m_shouldSkipLeadingNewline(false)
    , m_parser(parser)
    , m_scriptToProcessStartPosition(uninitializedPositionValue1())
    , m_options(options)
{
    ASSERT(isMainThread());
    // FIXME: This assertion will become invalid if <http://webkit.org/b/60316> is fixed.
    ASSERT(contextElement);
    if (contextElement) {
        // Steps 4.2-4.6 of the HTML5 Fragment Case parsing algorithm:
        // http://www.whatwg.org/specs/web-apps/current-work/multipage/the-end.html#fragment-case
        // For efficiency, we skip step 4.2 ("Let root be a new html element with no attributes")
        // and instead use the DocumentFragment as a root node.
        m_tree.openElements()->pushRootNode(HTMLStackItem::create(fragment, HTMLStackItem::ItemForDocumentFragmentNode));

#if ENABLE(TEMPLATE_ELEMENT)
        if (contextElement->hasTagName(templateTag))
            m_templateInsertionModes.append(TemplateContentsMode);
#endif

        resetInsertionModeAppropriately();
        m_tree.setForm(closestFormAncestor(contextElement));
    }
}

HTMLTreeBuilder::~HTMLTreeBuilder()
{
}

void HTMLTreeBuilder::detach()
{
#ifndef NDEBUG
    // This call makes little sense in fragment mode, but for consistency
    // DocumentParser expects detach() to always be called before it's destroyed.
    m_isAttached = false;
#endif
    // HTMLConstructionSite might be on the callstack when detach() is called
    // otherwise we'd just call m_tree.clear() here instead.
    m_tree.detach();
}

HTMLTreeBuilder::FragmentParsingContext::FragmentParsingContext()
    : m_fragment(0)
    , m_contextElement(0)
{
}

HTMLTreeBuilder::FragmentParsingContext::FragmentParsingContext(DocumentFragment* fragment, Element* contextElement)
    : m_fragment(fragment)
    , m_contextElement(contextElement)
{
    ASSERT(!fragment->hasChildNodes());
}

HTMLTreeBuilder::FragmentParsingContext::~FragmentParsingContext()
{
}

PassRefPtr<Element> HTMLTreeBuilder::takeScriptToProcess(TextPosition& scriptStartPosition)
{
    ASSERT(m_scriptToProcess);
    // Unpause ourselves, callers may pause us again when processing the script.
    // The HTML5 spec is written as though scripts are executed inside the tree
    // builder.  We pause the parser to exit the tree builder, and then resume
    // before running scripts.
    scriptStartPosition = m_scriptToProcessStartPosition;
    m_scriptToProcessStartPosition = uninitializedPositionValue1();
    return m_scriptToProcess.release();
}

void HTMLTreeBuilder::constructTree(AtomicHTMLToken* token)
{
    if (shouldProcessTokenInForeignContent(token))
        processTokenInForeignContent(token);
    else
        processToken(token);

    if (m_parser->tokenizer()) {
        bool inForeignContent = !m_tree.isEmpty()
            && !m_tree.currentStackItem()->isInHTMLNamespace()
            && !HTMLElementStack::isHTMLIntegrationPoint(m_tree.currentStackItem())
            && !HTMLElementStack::isMathMLTextIntegrationPoint(m_tree.currentStackItem());

        m_parser->tokenizer()->setForceNullCharacterReplacement(m_insertionMode == TextMode || inForeignContent);
        m_parser->tokenizer()->setShouldAllowCDATA(inForeignContent);
    }

    m_tree.executeQueuedTasks();
    // We might be detached now.
}

void HTMLTreeBuilder::processToken(AtomicHTMLToken* token)
{
    switch (token->type()) {
    case HTMLToken::Uninitialized:
        ASSERT_NOT_REACHED();
        break;
    case HTMLToken::DOCTYPE:
        m_shouldSkipLeadingNewline = false;
        processDoctypeToken(token);
        break;
    case HTMLToken::StartTag:
        m_shouldSkipLeadingNewline = false;
        processStartTag(token);
        break;
    case HTMLToken::EndTag:
        m_shouldSkipLeadingNewline = false;
        processEndTag(token);
        break;
    case HTMLToken::Comment:
        m_shouldSkipLeadingNewline = false;
        processComment(token);
        return;
    case HTMLToken::Character:
        processCharacter(token);
        break;
    case HTMLToken::EndOfFile:
        m_shouldSkipLeadingNewline = false;
        processEndOfFile(token);
        break;
    }
}

void HTMLTreeBuilder::processDoctypeToken(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::DOCTYPE);
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

void HTMLTreeBuilder::processFakeStartTag(const QualifiedName& tagName, const Vector<Attribute>& attributes)
{
    // FIXME: We'll need a fancier conversion than just "localName" for SVG/MathML tags.
    AtomicHTMLToken fakeToken(HTMLToken::StartTag, tagName.localName(), attributes);
    processStartTag(&fakeToken);
}

void HTMLTreeBuilder::processFakeEndTag(const AtomicString& tagName)
{
    AtomicHTMLToken fakeToken(HTMLToken::EndTag, tagName);
    processEndTag(&fakeToken);
}

void HTMLTreeBuilder::processFakeEndTag(const QualifiedName& tagName)
{
    // FIXME: We'll need a fancier conversion than just "localName" for SVG/MathML tags.
    processFakeEndTag(tagName.localName());
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
    processEndTag(&endP);
}

Vector<Attribute> HTMLTreeBuilder::attributesForIsindexInput(AtomicHTMLToken* token)
{
    Vector<Attribute> attributes = token->attributes();
    for (int i = attributes.size() - 1; i >= 0; --i) {
        const QualifiedName& name = attributes.at(i).name();
        if (name.matches(nameAttr) || name.matches(actionAttr) || name.matches(promptAttr))
            attributes.remove(i);
    }

    attributes.append(Attribute(nameAttr, isindexTag.localName()));
    return attributes;
}

void HTMLTreeBuilder::processIsindexStartTagForInBody(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::StartTag);
    ASSERT(token->name() == isindexTag);
    parseError(token);
    if (m_tree.form())
        return;
    notImplemented(); // Acknowledge self-closing flag
    processFakeStartTag(formTag);
    Attribute* actionAttribute = token->getAttributeItem(actionAttr);
    if (actionAttribute)
        m_tree.form()->setAttribute(actionAttr, actionAttribute->value());
    processFakeStartTag(hrTag);
    processFakeStartTag(labelTag);
    Attribute* promptAttribute = token->getAttributeItem(promptAttr);
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

bool isLi(const HTMLStackItem* item)
{
    return item->hasTagName(liTag);
}

bool isDdOrDt(const HTMLStackItem* item)
{
    return item->hasTagName(ddTag)
        || item->hasTagName(dtTag);
}

}

template <bool shouldClose(const HTMLStackItem*)>
void HTMLTreeBuilder::processCloseWhenNestedTag(AtomicHTMLToken* token)
{
    m_framesetOk = false;
    HTMLElementStack::ElementRecord* nodeRecord = m_tree.openElements()->topRecord();
    while (1) {
        RefPtr<HTMLStackItem> item = nodeRecord->stackItem();
        if (shouldClose(item.get())) {
            ASSERT(item->isElementNode());
            processFakeEndTag(item->localName());
            break;
        }
        if (item->isSpecialNode() && !item->hasTagName(addressTag) && !item->hasTagName(divTag) && !item->hasTagName(pTag))
            break;
        nodeRecord = nodeRecord->next();
    }
    processFakePEndTagIfPInButtonScope();
    m_tree.insertHTMLElement(token);
}

typedef HashMap<AtomicString, QualifiedName> PrefixedNameToQualifiedNameMap;

static void mapLoweredLocalNameToName(PrefixedNameToQualifiedNameMap* map, QualifiedName** names, size_t length)
{
    for (size_t i = 0; i < length; ++i) {
        const QualifiedName& name = *names[i];
        const AtomicString& localName = name.localName();
        AtomicString loweredLocalName = localName.lower();
        if (loweredLocalName != localName)
            map->add(loweredLocalName, name);
    }
}

static void adjustSVGTagNameCase(AtomicHTMLToken* token)
{
    static PrefixedNameToQualifiedNameMap* caseMap = 0;
    if (!caseMap) {
        caseMap = new PrefixedNameToQualifiedNameMap;
        QualifiedName** svgTags = SVGNames::getSVGTags();
        mapLoweredLocalNameToName(caseMap, svgTags, SVGNames::SVGTagsCount);
    }

    const QualifiedName& casedName = caseMap->get(token->name());
    if (casedName.localName().isNull())
        return;
    token->setName(casedName.localName());
}

template<QualifiedName** getAttrs(), unsigned length>
static void adjustAttributes(AtomicHTMLToken* token)
{
    static PrefixedNameToQualifiedNameMap* caseMap = 0;
    if (!caseMap) {
        caseMap = new PrefixedNameToQualifiedNameMap;
        QualifiedName** attrs = getAttrs();
        mapLoweredLocalNameToName(caseMap, attrs, length);
    }

    for (unsigned i = 0; i < token->attributes().size(); ++i) {
        Attribute& tokenAttribute = token->attributes().at(i);
        const QualifiedName& casedName = caseMap->get(tokenAttribute.localName());
        if (!casedName.localName().isNull())
            tokenAttribute.parserSetName(casedName);
    }
}

static void adjustSVGAttributes(AtomicHTMLToken* token)
{
    adjustAttributes<SVGNames::getSVGAttrs, SVGNames::SVGAttrsCount>(token);
}

static void adjustMathMLAttributes(AtomicHTMLToken* token)
{
    adjustAttributes<MathMLNames::getMathMLAttrs, MathMLNames::MathMLAttrsCount>(token);
}

static void addNamesWithPrefix(PrefixedNameToQualifiedNameMap* map, const AtomicString& prefix, QualifiedName** names, size_t length)
{
    for (size_t i = 0; i < length; ++i) {
        QualifiedName* name = names[i];
        const AtomicString& localName = name->localName();
        AtomicString prefixColonLocalName = prefix + ':' + localName;
        QualifiedName nameWithPrefix(prefix, localName, name->namespaceURI());
        map->add(prefixColonLocalName, nameWithPrefix);
    }
}

static void adjustForeignAttributes(AtomicHTMLToken* token)
{
    static PrefixedNameToQualifiedNameMap* map = 0;
    if (!map) {
        map = new PrefixedNameToQualifiedNameMap;

        QualifiedName** attrs = XLinkNames::getXLinkAttrs();
        addNamesWithPrefix(map, xlinkAtom, attrs, XLinkNames::XLinkAttrsCount);

        attrs = XMLNames::getXMLAttrs();
        addNamesWithPrefix(map, xmlAtom, attrs, XMLNames::XMLAttrsCount);

        map->add(WTF::xmlnsAtom, XMLNSNames::xmlnsAttr);
        map->add("xmlns:xlink", QualifiedName(xmlnsAtom, xlinkAtom, XMLNSNames::xmlnsNamespaceURI));
    }

    for (unsigned i = 0; i < token->attributes().size(); ++i) {
        Attribute& tokenAttribute = token->attributes().at(i);
        const QualifiedName& name = map->get(tokenAttribute.localName());
        if (!name.localName().isNull())
            tokenAttribute.parserSetName(name);
    }
}

void HTMLTreeBuilder::processStartTagForInBody(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::StartTag);
    if (token->name() == htmlTag) {
        processHtmlStartTagForInBody(token);
        return;
    }
    if (token->name() == baseTag
        || token->name() == basefontTag
        || token->name() == bgsoundTag
        || token->name() == commandTag
        || token->name() == linkTag
        || token->name() == metaTag
        || token->name() == noframesTag
        || token->name() == scriptTag
        || token->name() == styleTag
        || token->name() == titleTag) {
        bool didProcess = processStartTagForInHead(token);
        ASSERT_UNUSED(didProcess, didProcess);
        return;
    }
    if (token->name() == bodyTag) {
        parseError(token);
        bool fragmentOrTemplateCase = !m_tree.openElements()->secondElementIsHTMLBodyElement() || m_tree.openElements()->hasOnlyOneElement();
#if ENABLE(TEMPLATE_ELEMENT)
        fragmentOrTemplateCase = fragmentOrTemplateCase || m_tree.openElements()->hasTemplateInHTMLScope();
#endif
        if (fragmentOrTemplateCase) {
            ASSERT(isParsingFragmentOrTemplateContents());
            return;
        }
        m_framesetOk = false;
        m_tree.insertHTMLBodyStartTagInBody(token);
        return;
    }
    if (token->name() == framesetTag) {
        parseError(token);
        if (!m_tree.openElements()->secondElementIsHTMLBodyElement() || m_tree.openElements()->hasOnlyOneElement()) {
            ASSERT(isParsingFragmentOrTemplateContents());
            return;
        }
        if (!m_framesetOk)
            return;
        m_tree.openElements()->bodyElement()->remove(ASSERT_NO_EXCEPTION);
        m_tree.openElements()->popUntil(m_tree.openElements()->bodyElement());
        m_tree.openElements()->popHTMLBodyElement();
        ASSERT(m_tree.openElements()->top() == m_tree.openElements()->htmlElement());
        m_tree.insertHTMLElement(token);
        setInsertionMode(InFramesetMode);
        return;
    }
    if (token->name() == addressTag
        || token->name() == articleTag
        || token->name() == asideTag
        || token->name() == blockquoteTag
        || token->name() == centerTag
        || token->name() == detailsTag
        || token->name() == dirTag
        || token->name() == divTag
        || token->name() == dlTag
        || token->name() == fieldsetTag
        || token->name() == figcaptionTag
        || token->name() == figureTag
        || token->name() == footerTag
        || token->name() == headerTag
        || token->name() == hgroupTag
        || token->name() == mainTag
        || token->name() == menuTag
        || token->name() == navTag
        || token->name() == olTag
        || token->name() == pTag
        || token->name() == sectionTag
        || token->name() == summaryTag
        || token->name() == ulTag) {
        processFakePEndTagIfPInButtonScope();
        m_tree.insertHTMLElement(token);
        return;
    }
    if (isNumberedHeaderTag(token->name())) {
        processFakePEndTagIfPInButtonScope();
        if (m_tree.currentStackItem()->isNumberedHeaderElement()) {
            parseError(token);
            m_tree.openElements()->pop();
        }
        m_tree.insertHTMLElement(token);
        return;
    }
    if (token->name() == preTag || token->name() == listingTag) {
        processFakePEndTagIfPInButtonScope();
        m_tree.insertHTMLElement(token);
        m_shouldSkipLeadingNewline = true;
        m_framesetOk = false;
        return;
    }
    if (token->name() == formTag) {
        if (m_tree.form()) {
            parseError(token);
            return;
        }
        processFakePEndTagIfPInButtonScope();
        m_tree.insertHTMLFormElement(token);
        return;
    }
    if (token->name() == liTag) {
        processCloseWhenNestedTag<isLi>(token);
        return;
    }
    if (token->name() == ddTag || token->name() == dtTag) {
        processCloseWhenNestedTag<isDdOrDt>(token);
        return;
    }
    if (token->name() == plaintextTag) {
        processFakePEndTagIfPInButtonScope();
        m_tree.insertHTMLElement(token);
        if (m_parser->tokenizer())
            m_parser->tokenizer()->setState(HTMLTokenizer::PLAINTEXTState);
        return;
    }
    if (token->name() == buttonTag) {
        if (m_tree.openElements()->inScope(buttonTag)) {
            parseError(token);
            processFakeEndTag(buttonTag);
            processStartTag(token); // FIXME: Could we just fall through here?
            return;
        }
        m_tree.reconstructTheActiveFormattingElements();
        m_tree.insertHTMLElement(token);
        m_framesetOk = false;
        return;
    }
    if (token->name() == aTag) {
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
    if (isNonAnchorNonNobrFormattingTag(token->name())) {
        m_tree.reconstructTheActiveFormattingElements();
        m_tree.insertFormattingElement(token);
        return;
    }
    if (token->name() == nobrTag) {
        m_tree.reconstructTheActiveFormattingElements();
        if (m_tree.openElements()->inScope(nobrTag)) {
            parseError(token);
            processFakeEndTag(nobrTag);
            m_tree.reconstructTheActiveFormattingElements();
        }
        m_tree.insertFormattingElement(token);
        return;
    }
    if (token->name() == appletTag
        || token->name() == embedTag
        || token->name() == objectTag) {
        if (!pluginContentIsAllowed(m_tree.parserContentPolicy()))
            return;
    }
    if (token->name() == appletTag
        || token->name() == marqueeTag
        || token->name() == objectTag) {
        m_tree.reconstructTheActiveFormattingElements();
        m_tree.insertHTMLElement(token);
        m_tree.activeFormattingElements()->appendMarker();
        m_framesetOk = false;
        return;
    }
    if (token->name() == tableTag) {
        if (!m_tree.inQuirksMode() && m_tree.openElements()->inButtonScope(pTag))
            processFakeEndTag(pTag);
        m_tree.insertHTMLElement(token);
        m_framesetOk = false;
        setInsertionMode(InTableMode);
        return;
    }
    if (token->name() == imageTag) {
        parseError(token);
        // Apparently we're not supposed to ask.
        token->setName(imgTag.localName());
        // Note the fall through to the imgTag handling below!
    }
    if (token->name() == areaTag
        || token->name() == brTag
        || token->name() == embedTag
        || token->name() == imgTag
        || token->name() == keygenTag
        || token->name() == wbrTag) {
        m_tree.reconstructTheActiveFormattingElements();
        m_tree.insertSelfClosingHTMLElement(token);
        m_framesetOk = false;
        return;
    }
    if (token->name() == inputTag) {
        Attribute* typeAttribute = token->getAttributeItem(typeAttr);
        m_tree.reconstructTheActiveFormattingElements();
        m_tree.insertSelfClosingHTMLElement(token);
        if (!typeAttribute || !equalIgnoringCase(typeAttribute->value(), "hidden"))
            m_framesetOk = false;
        return;
    }
    if (token->name() == paramTag
        || token->name() == sourceTag
        || token->name() == trackTag) {
        m_tree.insertSelfClosingHTMLElement(token);
        return;
    }
    if (token->name() == hrTag) {
        processFakePEndTagIfPInButtonScope();
        m_tree.insertSelfClosingHTMLElement(token);
        m_framesetOk = false;
        return;
    }
    if (token->name() == isindexTag) {
        processIsindexStartTagForInBody(token);
        return;
    }
    if (token->name() == textareaTag) {
        m_tree.insertHTMLElement(token);
        m_shouldSkipLeadingNewline = true;
        if (m_parser->tokenizer())
            m_parser->tokenizer()->setState(HTMLTokenizer::RCDATAState);
        m_originalInsertionMode = m_insertionMode;
        m_framesetOk = false;
        setInsertionMode(TextMode);
        return;
    }
    if (token->name() == xmpTag) {
        processFakePEndTagIfPInButtonScope();
        m_tree.reconstructTheActiveFormattingElements();
        m_framesetOk = false;
        processGenericRawTextStartTag(token);
        return;
    }
    if (token->name() == iframeTag) {
        m_framesetOk = false;
        processGenericRawTextStartTag(token);
        return;
    }
    if (token->name() == noembedTag && m_options.pluginsEnabled) {
        processGenericRawTextStartTag(token);
        return;
    }
    if (token->name() == noscriptTag && m_options.scriptEnabled) {
        processGenericRawTextStartTag(token);
        return;
    }
    if (token->name() == selectTag) {
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
    if (token->name() == optgroupTag || token->name() == optionTag) {
        if (isHTMLOptionElement(m_tree.currentStackItem()->node())) {
            AtomicHTMLToken endOption(HTMLToken::EndTag, optionTag.localName());
            processEndTag(&endOption);
        }
        m_tree.reconstructTheActiveFormattingElements();
        m_tree.insertHTMLElement(token);
        return;
    }
    if (token->name() == rpTag || token->name() == rtTag) {
        if (m_tree.openElements()->inScope(rubyTag.localName())) {
            m_tree.generateImpliedEndTags();
            if (!m_tree.currentStackItem()->hasTagName(rubyTag))
                parseError(token);
        }
        m_tree.insertHTMLElement(token);
        return;
    }
    if (token->name() == MathMLNames::mathTag.localName()) {
        m_tree.reconstructTheActiveFormattingElements();
        adjustMathMLAttributes(token);
        adjustForeignAttributes(token);
        m_tree.insertForeignElement(token, MathMLNames::mathmlNamespaceURI);
        return;
    }
    if (token->name() == SVGNames::svgTag.localName()) {
        m_tree.reconstructTheActiveFormattingElements();
        adjustSVGAttributes(token);
        adjustForeignAttributes(token);
        m_tree.insertForeignElement(token, SVGNames::svgNamespaceURI);
        return;
    }
    if (isCaptionColOrColgroupTag(token->name())
        || token->name() == frameTag
        || token->name() == headTag
        || isTableBodyContextTag(token->name())
        || isTableCellContextTag(token->name())
        || token->name() == trTag) {
        parseError(token);
        return;
    }
#if ENABLE(TEMPLATE_ELEMENT)
    if (token->name() == templateTag) {
        processTemplateStartTag(token);
        return;
    }
#endif
    m_tree.reconstructTheActiveFormattingElements();
    m_tree.insertHTMLElement(token);
}

#if ENABLE(TEMPLATE_ELEMENT)
void HTMLTreeBuilder::processTemplateStartTag(AtomicHTMLToken* token)
{
    m_tree.activeFormattingElements()->appendMarker();
    m_tree.insertHTMLElement(token);
    m_templateInsertionModes.append(TemplateContentsMode);
    setInsertionMode(TemplateContentsMode);
}

bool HTMLTreeBuilder::processTemplateEndTag(AtomicHTMLToken* token)
{
    ASSERT(token->name() == templateTag.localName());
    if (!m_tree.openElements()->hasTemplateInHTMLScope()) {
        ASSERT(m_templateInsertionModes.isEmpty() || (m_templateInsertionModes.size() == 1 && m_fragmentContext.contextElement()->hasTagName(templateTag)));
        parseError(token);
        return false;
    }
    m_tree.generateImpliedEndTags();
    if (!m_tree.currentStackItem()->hasTagName(templateTag))
        parseError(token);
    m_tree.openElements()->popUntilPopped(templateTag);
    m_tree.activeFormattingElements()->clearToLastMarker();
    m_templateInsertionModes.removeLast();
    resetInsertionModeAppropriately();
    return true;
}

bool HTMLTreeBuilder::processEndOfFileForInTemplateContents(AtomicHTMLToken* token)
{
    AtomicHTMLToken endTemplate(HTMLToken::EndTag, templateTag.localName());
    if (!processTemplateEndTag(&endTemplate))
        return false;

    processEndOfFile(token);
    return true;
}
#endif

bool HTMLTreeBuilder::processColgroupEndTagForInColumnGroup()
{
    bool ignoreFakeEndTag = m_tree.currentIsRootNode();
#if ENABLE(TEMPLATE_ELEMENT)
    ignoreFakeEndTag = ignoreFakeEndTag || m_tree.currentNode()->hasTagName(templateTag);
#endif

    if (ignoreFakeEndTag) {
        ASSERT(isParsingFragmentOrTemplateContents());
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

void HTMLTreeBuilder::processStartTagForInTable(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::StartTag);
    if (token->name() == captionTag) {
        m_tree.openElements()->popUntilTableScopeMarker();
        m_tree.activeFormattingElements()->appendMarker();
        m_tree.insertHTMLElement(token);
        setInsertionMode(InCaptionMode);
        return;
    }
    if (token->name() == colgroupTag) {
        m_tree.openElements()->popUntilTableScopeMarker();
        m_tree.insertHTMLElement(token);
        setInsertionMode(InColumnGroupMode);
        return;
    }
    if (token->name() == colTag) {
        processFakeStartTag(colgroupTag);
        ASSERT(InColumnGroupMode);
        processStartTag(token);
        return;
    }
    if (isTableBodyContextTag(token->name())) {
        m_tree.openElements()->popUntilTableScopeMarker();
        m_tree.insertHTMLElement(token);
        setInsertionMode(InTableBodyMode);
        return;
    }
    if (isTableCellContextTag(token->name())
        || token->name() == trTag) {
        processFakeStartTag(tbodyTag);
        ASSERT(insertionMode() == InTableBodyMode);
        processStartTag(token);
        return;
    }
    if (token->name() == tableTag) {
        parseError(token);
        if (!processTableEndTagForInTable()) {
            ASSERT(isParsingFragmentOrTemplateContents());
            return;
        }
        processStartTag(token);
        return;
    }
    if (token->name() == styleTag || token->name() == scriptTag) {
        processStartTagForInHead(token);
        return;
    }
    if (token->name() == inputTag) {
        Attribute* typeAttribute = token->getAttributeItem(typeAttr);
        if (typeAttribute && equalIgnoringCase(typeAttribute->value(), "hidden")) {
            parseError(token);
            m_tree.insertSelfClosingHTMLElement(token);
            return;
        }
        // Fall through to "anything else" case.
    }
    if (token->name() == formTag) {
        parseError(token);
        if (m_tree.form())
            return;
        m_tree.insertHTMLFormElement(token, true);
        m_tree.openElements()->pop();
        return;
    }
#if ENABLE(TEMPLATE_ELEMENT)
    if (token->name() == templateTag) {
        processTemplateStartTag(token);
        return;
    }
#endif
    parseError(token);
    HTMLConstructionSite::RedirectToFosterParentGuard redirecter(m_tree);
    processStartTagForInBody(token);
}

void HTMLTreeBuilder::processStartTag(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::StartTag);
    switch (insertionMode()) {
    case InitialMode:
        ASSERT(insertionMode() == InitialMode);
        defaultForInitial();
        // Fall through.
    case BeforeHTMLMode:
        ASSERT(insertionMode() == BeforeHTMLMode);
        if (token->name() == htmlTag) {
            m_tree.insertHTMLHtmlStartTagBeforeHTML(token);
            setInsertionMode(BeforeHeadMode);
            return;
        }
        defaultForBeforeHTML();
        // Fall through.
    case BeforeHeadMode:
        ASSERT(insertionMode() == BeforeHeadMode);
        if (token->name() == htmlTag) {
            processHtmlStartTagForInBody(token);
            return;
        }
        if (token->name() == headTag) {
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
        if (token->name() == htmlTag) {
            processHtmlStartTagForInBody(token);
            return;
        }
        if (token->name() == bodyTag) {
            m_framesetOk = false;
            m_tree.insertHTMLBodyElement(token);
            setInsertionMode(InBodyMode);
            return;
        }
        if (token->name() == framesetTag) {
            m_tree.insertHTMLElement(token);
            setInsertionMode(InFramesetMode);
            return;
        }
        if (token->name() == baseTag
            || token->name() == basefontTag
            || token->name() == bgsoundTag
            || token->name() == linkTag
            || token->name() == metaTag
            || token->name() == noframesTag
            || token->name() == scriptTag
            || token->name() == styleTag
            || token->name() == titleTag) {
            parseError(token);
            ASSERT(m_tree.head());
            m_tree.openElements()->pushHTMLHeadElement(m_tree.headStackItem());
            processStartTagForInHead(token);
            m_tree.openElements()->removeHTMLHeadElement(m_tree.head());
            return;
        }
        if (token->name() == headTag) {
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
        if (isCaptionColOrColgroupTag(token->name())
            || isTableBodyContextTag(token->name())
            || isTableCellContextTag(token->name())
            || token->name() == trTag) {
            parseError(token);
            if (!processCaptionEndTagForInCaption()) {
                ASSERT(isParsingFragment());
                return;
            }
            processStartTag(token);
            return;
        }
        processStartTagForInBody(token);
        break;
    case InColumnGroupMode:
        ASSERT(insertionMode() == InColumnGroupMode);
        if (token->name() == htmlTag) {
            processHtmlStartTagForInBody(token);
            return;
        }
        if (token->name() == colTag) {
            m_tree.insertSelfClosingHTMLElement(token);
            return;
        }
#if ENABLE(TEMPLATE_ELEMENT)
        if (token->name() == templateTag) {
            processTemplateStartTag(token);
            return;
        }
#endif
        if (!processColgroupEndTagForInColumnGroup()) {
            ASSERT(isParsingFragmentOrTemplateContents());
            return;
        }
        processStartTag(token);
        break;
    case InTableBodyMode:
        ASSERT(insertionMode() == InTableBodyMode);
        if (token->name() == trTag) {
            m_tree.openElements()->popUntilTableBodyScopeMarker(); // How is there ever anything to pop?
            m_tree.insertHTMLElement(token);
            setInsertionMode(InRowMode);
            return;
        }
        if (isTableCellContextTag(token->name())) {
            parseError(token);
            processFakeStartTag(trTag);
            ASSERT(insertionMode() == InRowMode);
            processStartTag(token);
            return;
        }
        if (isCaptionColOrColgroupTag(token->name()) || isTableBodyContextTag(token->name())) {
            // FIXME: This is slow.
            if (!m_tree.openElements()->inTableScope(tbodyTag) && !m_tree.openElements()->inTableScope(theadTag) && !m_tree.openElements()->inTableScope(tfootTag)) {
                ASSERT(isParsingFragmentOrTemplateContents());
                parseError(token);
                return;
            }
            m_tree.openElements()->popUntilTableBodyScopeMarker();
            ASSERT(isTableBodyContextTag(m_tree.currentStackItem()->localName()));
            processFakeEndTag(m_tree.currentStackItem()->localName());
            processStartTag(token);
            return;
        }
        processStartTagForInTable(token);
        break;
    case InRowMode:
        ASSERT(insertionMode() == InRowMode);
        if (isTableCellContextTag(token->name())) {
            m_tree.openElements()->popUntilTableRowScopeMarker();
            m_tree.insertHTMLElement(token);
            setInsertionMode(InCellMode);
            m_tree.activeFormattingElements()->appendMarker();
            return;
        }
        if (token->name() == trTag
            || isCaptionColOrColgroupTag(token->name())
            || isTableBodyContextTag(token->name())) {
            if (!processTrEndTagForInRow()) {
                ASSERT(isParsingFragmentOrTemplateContents());
                return;
            }
            ASSERT(insertionMode() == InTableBodyMode);
            processStartTag(token);
            return;
        }
        processStartTagForInTable(token);
        break;
    case InCellMode:
        ASSERT(insertionMode() == InCellMode);
        if (isCaptionColOrColgroupTag(token->name())
            || isTableCellContextTag(token->name())
            || token->name() == trTag
            || isTableBodyContextTag(token->name())) {
            // FIXME: This could be more efficient.
            if (!m_tree.openElements()->inTableScope(tdTag) && !m_tree.openElements()->inTableScope(thTag)) {
                ASSERT(isParsingFragment());
                parseError(token);
                return;
            }
            closeTheCell();
            processStartTag(token);
            return;
        }
        processStartTagForInBody(token);
        break;
    case AfterBodyMode:
    case AfterAfterBodyMode:
        ASSERT(insertionMode() == AfterBodyMode || insertionMode() == AfterAfterBodyMode);
        if (token->name() == htmlTag) {
            processHtmlStartTagForInBody(token);
            return;
        }
        setInsertionMode(InBodyMode);
        processStartTag(token);
        break;
    case InHeadNoscriptMode:
        ASSERT(insertionMode() == InHeadNoscriptMode);
        if (token->name() == htmlTag) {
            processHtmlStartTagForInBody(token);
            return;
        }
        if (token->name() == basefontTag
            || token->name() == bgsoundTag
            || token->name() == linkTag
            || token->name() == metaTag
            || token->name() == noframesTag
            || token->name() == styleTag) {
            bool didProcess = processStartTagForInHead(token);
            ASSERT_UNUSED(didProcess, didProcess);
            return;
        }
        if (token->name() == htmlTag || token->name() == noscriptTag) {
            parseError(token);
            return;
        }
        defaultForInHeadNoscript();
        processToken(token);
        break;
    case InFramesetMode:
        ASSERT(insertionMode() == InFramesetMode);
        if (token->name() == htmlTag) {
            processHtmlStartTagForInBody(token);
            return;
        }
        if (token->name() == framesetTag) {
            m_tree.insertHTMLElement(token);
            return;
        }
        if (token->name() == frameTag) {
            m_tree.insertSelfClosingHTMLElement(token);
            return;
        }
        if (token->name() == noframesTag) {
            processStartTagForInHead(token);
            return;
        }
#if ENABLE(TEMPLATE_ELEMENT)
        if (token->name() == templateTag) {
            processTemplateStartTag(token);
            return;
        }
#endif
        parseError(token);
        break;
    case AfterFramesetMode:
    case AfterAfterFramesetMode:
        ASSERT(insertionMode() == AfterFramesetMode || insertionMode() == AfterAfterFramesetMode);
        if (token->name() == htmlTag) {
            processHtmlStartTagForInBody(token);
            return;
        }
        if (token->name() == noframesTag) {
            processStartTagForInHead(token);
            return;
        }
        parseError(token);
        break;
    case InSelectInTableMode:
        ASSERT(insertionMode() == InSelectInTableMode);
        if (token->name() == captionTag
            || token->name() == tableTag
            || isTableBodyContextTag(token->name())
            || token->name() == trTag
            || isTableCellContextTag(token->name())) {
            parseError(token);
            AtomicHTMLToken endSelect(HTMLToken::EndTag, selectTag.localName());
            processEndTag(&endSelect);
            processStartTag(token);
            return;
        }
        // Fall through
    case InSelectMode:
        ASSERT(insertionMode() == InSelectMode || insertionMode() == InSelectInTableMode);
        if (token->name() == htmlTag) {
            processHtmlStartTagForInBody(token);
            return;
        }
        if (token->name() == optionTag) {
            if (isHTMLOptionElement(m_tree.currentStackItem()->node())) {
                AtomicHTMLToken endOption(HTMLToken::EndTag, optionTag.localName());
                processEndTag(&endOption);
            }
            m_tree.insertHTMLElement(token);
            return;
        }
        if (token->name() == optgroupTag) {
            if (isHTMLOptionElement(m_tree.currentStackItem()->node())) {
                AtomicHTMLToken endOption(HTMLToken::EndTag, optionTag.localName());
                processEndTag(&endOption);
            }
            if (isHTMLOptGroupElement(m_tree.currentStackItem()->node())) {
                AtomicHTMLToken endOptgroup(HTMLToken::EndTag, optgroupTag.localName());
                processEndTag(&endOptgroup);
            }
            m_tree.insertHTMLElement(token);
            return;
        }
        if (token->name() == selectTag) {
            parseError(token);
            AtomicHTMLToken endSelect(HTMLToken::EndTag, selectTag.localName());
            processEndTag(&endSelect);
            return;
        }
        if (token->name() == inputTag
            || token->name() == keygenTag
            || token->name() == textareaTag) {
            parseError(token);
            if (!m_tree.openElements()->inSelectScope(selectTag)) {
                ASSERT(isParsingFragment());
                return;
            }
            AtomicHTMLToken endSelect(HTMLToken::EndTag, selectTag.localName());
            processEndTag(&endSelect);
            processStartTag(token);
            return;
        }
        if (token->name() == scriptTag) {
            bool didProcess = processStartTagForInHead(token);
            ASSERT_UNUSED(didProcess, didProcess);
            return;
        }
#if ENABLE(TEMPLATE_ELEMENT)
        if (token->name() == templateTag) {
            processTemplateStartTag(token);
            return;
        }
#endif
        break;
    case InTableTextMode:
        defaultForInTableText();
        processStartTag(token);
        break;
    case TextMode:
        ASSERT_NOT_REACHED();
        break;
    case TemplateContentsMode:
#if ENABLE(TEMPLATE_ELEMENT)
        if (token->name() == templateTag) {
            processTemplateStartTag(token);
            return;
        }

        if (token->name() == linkTag
            || token->name() == scriptTag
            || token->name() == styleTag
            || token->name() == metaTag) {
            processStartTagForInHead(token);
            return;
        }

        InsertionMode insertionMode = TemplateContentsMode;
        if (token->name() == frameTag)
            insertionMode = InFramesetMode;
        else if (token->name() == colTag)
            insertionMode = InColumnGroupMode;
        else if (isCaptionColOrColgroupTag(token->name()) || isTableBodyContextTag(token->name()))
            insertionMode = InTableMode;
        else if (token->name() == trTag)
            insertionMode = InTableBodyMode;
        else if (isTableCellContextTag(token->name()))
            insertionMode = InRowMode;
        else
            insertionMode = InBodyMode;

        ASSERT(insertionMode != TemplateContentsMode);
        ASSERT(m_templateInsertionModes.last() == TemplateContentsMode);
        m_templateInsertionModes.last() = insertionMode;
        setInsertionMode(insertionMode);

        processStartTag(token);
#else
        ASSERT_NOT_REACHED();
#endif
        break;
    }
}

void HTMLTreeBuilder::processHtmlStartTagForInBody(AtomicHTMLToken* token)
{
    parseError(token);
#if ENABLE(TEMPLATE_ELEMENT)
    if (m_tree.openElements()->hasTemplateInHTMLScope()) {
        ASSERT(isParsingTemplateContents());
        return;
    }
#endif
    m_tree.insertHTMLHtmlStartTagInBody(token);
}

bool HTMLTreeBuilder::processBodyEndTagForInBody(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::EndTag);
    ASSERT(token->name() == bodyTag);
    if (!m_tree.openElements()->inScope(bodyTag.localName())) {
        parseError(token);
        return false;
    }
    notImplemented(); // Emit a more specific parse error based on stack contents.
    setInsertionMode(AfterBodyMode);
    return true;
}

void HTMLTreeBuilder::processAnyOtherEndTagForInBody(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::EndTag);
    HTMLElementStack::ElementRecord* record = m_tree.openElements()->topRecord();
    while (1) {
        RefPtr<HTMLStackItem> item = record->stackItem();
        if (item->matchesHTMLTag(token->name())) {
            m_tree.generateImpliedEndTagsWithExclusion(token->name());
            if (!m_tree.currentStackItem()->matchesHTMLTag(token->name()))
                parseError(token);
            m_tree.openElements()->popUntilPopped(item->element());
            return;
        }
        if (item->isSpecialNode()) {
            parseError(token);
            return;
        }
        record = record->next();
    }
}

// http://www.whatwg.org/specs/web-apps/current-work/multipage/tokenization.html#parsing-main-inbody
void HTMLTreeBuilder::callTheAdoptionAgency(AtomicHTMLToken* token)
{
    // The adoption agency algorithm is N^2. We limit the number of iterations
    // to stop from hanging the whole browser. This limit is specified in the
    // adoption agency algorithm: 
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/tree-construction.html#parsing-main-inbody
    static const int outerIterationLimit = 8;
    static const int innerIterationLimit = 3;

    // 1, 2, 3 and 16 are covered by the for() loop.
    for (int i = 0; i < outerIterationLimit; ++i) {
        // 4.
        Element* formattingElement = m_tree.activeFormattingElements()->closestElementInScopeWithName(token->name());
        // 4.a
        if (!formattingElement)
            return processAnyOtherEndTagForInBody(token);
        // 4.c
        if ((m_tree.openElements()->contains(formattingElement)) && !m_tree.openElements()->inScope(formattingElement)) {
            parseError(token);
            notImplemented(); // Check the stack of open elements for a more specific parse error.
            return;
        }
        // 4.b
        HTMLElementStack::ElementRecord* formattingElementRecord = m_tree.openElements()->find(formattingElement);
        if (!formattingElementRecord) {
            parseError(token);
            m_tree.activeFormattingElements()->remove(formattingElement);
            return;
        }
        // 4.d
        if (formattingElement != m_tree.currentElement())
            parseError(token);
        // 5.
        HTMLElementStack::ElementRecord* furthestBlock = m_tree.openElements()->furthestBlockForFormattingElement(formattingElement);
        // 6.
        if (!furthestBlock) {
            m_tree.openElements()->popUntilPopped(formattingElement);
            m_tree.activeFormattingElements()->remove(formattingElement);
            return;
        }
        // 7.
        ASSERT(furthestBlock->isAbove(formattingElementRecord));
        RefPtr<HTMLStackItem> commonAncestor = formattingElementRecord->next()->stackItem();
        // 8.
        HTMLFormattingElementList::Bookmark bookmark = m_tree.activeFormattingElements()->bookmarkFor(formattingElement);
        // 9.
        HTMLElementStack::ElementRecord* node = furthestBlock;
        HTMLElementStack::ElementRecord* nextNode = node->next();
        HTMLElementStack::ElementRecord* lastNode = furthestBlock;
        // 9.1, 9.2, 9.3 and 9.11 are covered by the for() loop.
        for (int i = 0; i < innerIterationLimit; ++i) {
            // 9.4
            node = nextNode;
            ASSERT(node);
            nextNode = node->next(); // Save node->next() for the next iteration in case node is deleted in 9.5.
            // 9.5
            if (!m_tree.activeFormattingElements()->contains(node->element())) {
                m_tree.openElements()->remove(node->element());
                node = 0;
                continue;
            }
            // 9.6
            if (node == formattingElementRecord)
                break;
            // 9.7
            RefPtr<HTMLStackItem> newItem = m_tree.createElementFromSavedToken(node->stackItem().get());

            HTMLFormattingElementList::Entry* nodeEntry = m_tree.activeFormattingElements()->find(node->element());
            nodeEntry->replaceElement(newItem);
            node->replaceElement(newItem.release());

            // 9.8
            if (lastNode == furthestBlock)
                bookmark.moveToAfter(nodeEntry);
            // 9.9
            if (ContainerNode* parent = lastNode->element()->parentNode())
                parent->parserRemoveChild(lastNode->element());
            node->element()->parserAppendChild(lastNode->element());
            if (lastNode->element()->parentElement()->attached() && !lastNode->element()->attached())
                lastNode->element()->lazyAttach();
            // 9.10
            lastNode = node;
        }
        // 10.
        if (ContainerNode* parent = lastNode->element()->parentNode())
            parent->parserRemoveChild(lastNode->element());
        if (commonAncestor->causesFosterParenting())
            m_tree.fosterParent(lastNode->element());
        else {
#if ENABLE(TEMPLATE_ELEMENT)
            if (commonAncestor->hasTagName(templateTag))
                toHTMLTemplateElement(commonAncestor->node())->content()->parserAppendChild(lastNode->element());
            else
                commonAncestor->node()->parserAppendChild(lastNode->element());
#else
            commonAncestor->node()->parserAppendChild(lastNode->element());
#endif
            ASSERT(lastNode->stackItem()->isElementNode());
            ASSERT(lastNode->element()->parentNode());
            if (lastNode->element()->parentNode()->attached() && !lastNode->element()->attached())
                lastNode->element()->lazyAttach();
        }
        // 11.
        RefPtr<HTMLStackItem> newItem = m_tree.createElementFromSavedToken(formattingElementRecord->stackItem().get());
        // 12.
        newItem->element()->takeAllChildrenFrom(furthestBlock->element());
        // 13.
        Element* furthestBlockElement = furthestBlock->element();
        // FIXME: All this creation / parserAppendChild / attach business should
        //        be in HTMLConstructionSite. My guess is that steps 11--15
        //        should all be in some HTMLConstructionSite function.
        furthestBlockElement->parserAppendChild(newItem->element());
        // FIXME: Why is this attach logic necessary? Style resolve should attach us if needed.
        if (furthestBlockElement->attached() && !newItem->element()->attached()) {
            // Notice that newItem->element() might already be attached if, for example, one of the reparented
            // children is a style element, which attaches itself automatically.
            newItem->element()->attach();
        }
        // 14.
        m_tree.activeFormattingElements()->swapTo(formattingElement, newItem, bookmark);
        // 15.
        m_tree.openElements()->remove(formattingElement);
        m_tree.openElements()->insertAbove(newItem, furthestBlock);
    }
}

void HTMLTreeBuilder::resetInsertionModeAppropriately()
{
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#reset-the-insertion-mode-appropriately
    bool last = false;
    HTMLElementStack::ElementRecord* nodeRecord = m_tree.openElements()->topRecord();
    while (1) {
        RefPtr<HTMLStackItem> item = nodeRecord->stackItem();
        if (item->node() == m_tree.openElements()->rootNode()) {
            ASSERT(isParsingFragment());
            last = true;
            item = HTMLStackItem::create(m_fragmentContext.contextElement(), HTMLStackItem::ItemForContextElement);
        }
#if ENABLE(TEMPLATE_ELEMENT)
        if (item->hasTagName(templateTag))
            return setInsertionMode(m_templateInsertionModes.last());
#endif
        if (item->hasTagName(selectTag)) {
            return setInsertionMode(InSelectMode);
        }
        if (item->hasTagName(tdTag) || item->hasTagName(thTag))
            return setInsertionMode(InCellMode);
        if (item->hasTagName(trTag))
            return setInsertionMode(InRowMode);
        if (item->hasTagName(tbodyTag) || item->hasTagName(theadTag) || item->hasTagName(tfootTag))
            return setInsertionMode(InTableBodyMode);
        if (item->hasTagName(captionTag))
            return setInsertionMode(InCaptionMode);
        if (item->hasTagName(colgroupTag)) {
            return setInsertionMode(InColumnGroupMode);
        }
        if (isHTMLTableElement(item->node()))
            return setInsertionMode(InTableMode);
        if (item->hasTagName(headTag)) {
#if ENABLE(TEMPLATE_ELEMENT)
            if (!m_fragmentContext.fragment() || m_fragmentContext.contextElement() != item->node())
                return setInsertionMode(InHeadMode);
#endif
            return setInsertionMode(InBodyMode);
        }
        if (item->hasTagName(bodyTag))
            return setInsertionMode(InBodyMode);
        if (item->hasTagName(framesetTag)) {
            return setInsertionMode(InFramesetMode);
        }
        if (item->hasTagName(htmlTag)) {
            ASSERT(isParsingFragment());
            return setInsertionMode(BeforeHeadMode);
        }
        if (last) {
            ASSERT(isParsingFragment());
            return setInsertionMode(InBodyMode);
        }
        nodeRecord = nodeRecord->next();
    }
}

void HTMLTreeBuilder::processEndTagForInTableBody(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::EndTag);
    if (isTableBodyContextTag(token->name())) {
        if (!m_tree.openElements()->inTableScope(token->name())) {
            parseError(token);
            return;
        }
        m_tree.openElements()->popUntilTableBodyScopeMarker();
        m_tree.openElements()->pop();
        setInsertionMode(InTableMode);
        return;
    }
    if (token->name() == tableTag) {
        // FIXME: This is slow.
        if (!m_tree.openElements()->inTableScope(tbodyTag) && !m_tree.openElements()->inTableScope(theadTag) && !m_tree.openElements()->inTableScope(tfootTag)) {
            ASSERT(isParsingFragmentOrTemplateContents());
            parseError(token);
            return;
        }
        m_tree.openElements()->popUntilTableBodyScopeMarker();
        ASSERT(isTableBodyContextTag(m_tree.currentStackItem()->localName()));
        processFakeEndTag(m_tree.currentStackItem()->localName());
        processEndTag(token);
        return;
    }
    if (token->name() == bodyTag
        || isCaptionColOrColgroupTag(token->name())
        || token->name() == htmlTag
        || isTableCellContextTag(token->name())
        || token->name() == trTag) {
        parseError(token);
        return;
    }
    processEndTagForInTable(token);
}

void HTMLTreeBuilder::processEndTagForInRow(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::EndTag);
    if (token->name() == trTag) {
        processTrEndTagForInRow();
        return;
    }
    if (token->name() == tableTag) {
        if (!processTrEndTagForInRow()) {
            ASSERT(isParsingFragmentOrTemplateContents());
            return;
        }
        ASSERT(insertionMode() == InTableBodyMode);
        processEndTag(token);
        return;
    }
    if (isTableBodyContextTag(token->name())) {
        if (!m_tree.openElements()->inTableScope(token->name())) {
            parseError(token);
            return;
        }
        processFakeEndTag(trTag);
        ASSERT(insertionMode() == InTableBodyMode);
        processEndTag(token);
        return;
    }
    if (token->name() == bodyTag
        || isCaptionColOrColgroupTag(token->name())
        || token->name() == htmlTag
        || isTableCellContextTag(token->name())) {
        parseError(token);
        return;
    }
    processEndTagForInTable(token);
}

void HTMLTreeBuilder::processEndTagForInCell(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::EndTag);
    if (isTableCellContextTag(token->name())) {
        if (!m_tree.openElements()->inTableScope(token->name())) {
            parseError(token);
            return;
        }
        m_tree.generateImpliedEndTags();
        if (!m_tree.currentStackItem()->matchesHTMLTag(token->name()))
            parseError(token);
        m_tree.openElements()->popUntilPopped(token->name());
        m_tree.activeFormattingElements()->clearToLastMarker();
        setInsertionMode(InRowMode);
        return;
    }
    if (token->name() == bodyTag
        || isCaptionColOrColgroupTag(token->name())
        || token->name() == htmlTag) {
        parseError(token);
        return;
    }
    if (token->name() == tableTag
        || token->name() == trTag
        || isTableBodyContextTag(token->name())) {
        if (!m_tree.openElements()->inTableScope(token->name())) {
#if ENABLE(TEMPLATE_ELEMENT)
            ASSERT(isTableBodyContextTag(token->name()) || m_tree.openElements()->inTableScope(templateTag) || isParsingFragment());
#else
            ASSERT(isTableBodyContextTag(token->name()) || isParsingFragment());
#endif
            parseError(token);
            return;
        }
        closeTheCell();
        processEndTag(token);
        return;
    }
    processEndTagForInBody(token);
}

void HTMLTreeBuilder::processEndTagForInBody(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::EndTag);
    if (token->name() == bodyTag) {
        processBodyEndTagForInBody(token);
        return;
    }
    if (token->name() == htmlTag) {
        AtomicHTMLToken endBody(HTMLToken::EndTag, bodyTag.localName());
        if (processBodyEndTagForInBody(&endBody))
            processEndTag(token);
        return;
    }
    if (token->name() == addressTag
        || token->name() == articleTag
        || token->name() == asideTag
        || token->name() == blockquoteTag
        || token->name() == buttonTag
        || token->name() == centerTag
        || token->name() == detailsTag
        || token->name() == dirTag
        || token->name() == divTag
        || token->name() == dlTag
        || token->name() == fieldsetTag
        || token->name() == figcaptionTag
        || token->name() == figureTag
        || token->name() == footerTag
        || token->name() == headerTag
        || token->name() == hgroupTag
        || token->name() == listingTag
        || token->name() == mainTag
        || token->name() == menuTag
        || token->name() == navTag
        || token->name() == olTag
        || token->name() == preTag
        || token->name() == sectionTag
        || token->name() == summaryTag
        || token->name() == ulTag) {
        if (!m_tree.openElements()->inScope(token->name())) {
            parseError(token);
            return;
        }
        m_tree.generateImpliedEndTags();
        if (!m_tree.currentStackItem()->matchesHTMLTag(token->name()))
            parseError(token);
        m_tree.openElements()->popUntilPopped(token->name());
        return;
    }
    if (token->name() == formTag) {
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
    if (token->name() == pTag) {
        if (!m_tree.openElements()->inButtonScope(token->name())) {
            parseError(token);
            processFakeStartTag(pTag);
            ASSERT(m_tree.openElements()->inScope(token->name()));
            processEndTag(token);
            return;
        }
        m_tree.generateImpliedEndTagsWithExclusion(token->name());
        if (!m_tree.currentStackItem()->matchesHTMLTag(token->name()))
            parseError(token);
        m_tree.openElements()->popUntilPopped(token->name());
        return;
    }
    if (token->name() == liTag) {
        if (!m_tree.openElements()->inListItemScope(token->name())) {
            parseError(token);
            return;
        }
        m_tree.generateImpliedEndTagsWithExclusion(token->name());
        if (!m_tree.currentStackItem()->matchesHTMLTag(token->name()))
            parseError(token);
        m_tree.openElements()->popUntilPopped(token->name());
        return;
    }
    if (token->name() == ddTag
        || token->name() == dtTag) {
        if (!m_tree.openElements()->inScope(token->name())) {
            parseError(token);
            return;
        }
        m_tree.generateImpliedEndTagsWithExclusion(token->name());
        if (!m_tree.currentStackItem()->matchesHTMLTag(token->name()))
            parseError(token);
        m_tree.openElements()->popUntilPopped(token->name());
        return;
    }
    if (isNumberedHeaderTag(token->name())) {
        if (!m_tree.openElements()->hasNumberedHeaderElementInScope()) {
            parseError(token);
            return;
        }
        m_tree.generateImpliedEndTags();
        if (!m_tree.currentStackItem()->matchesHTMLTag(token->name()))
            parseError(token);
        m_tree.openElements()->popUntilNumberedHeaderElementPopped();
        return;
    }
    if (isFormattingTag(token->name())) {
        callTheAdoptionAgency(token);
        return;
    }
    if (token->name() == appletTag
        || token->name() == marqueeTag
        || token->name() == objectTag) {
        if (!m_tree.openElements()->inScope(token->name())) {
            parseError(token);
            return;
        }
        m_tree.generateImpliedEndTags();
        if (!m_tree.currentStackItem()->matchesHTMLTag(token->name()))
            parseError(token);
        m_tree.openElements()->popUntilPopped(token->name());
        m_tree.activeFormattingElements()->clearToLastMarker();
        return;
    }
    if (token->name() == brTag) {
        parseError(token);
        processFakeStartTag(brTag);
        return;
    }
#if ENABLE(TEMPLATE_ELEMENT)
    if (token->name() == templateTag) {
        processTemplateEndTag(token);
        return;
    }
#endif
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
    // FIXME: parse error if (!m_tree.currentStackItem()->hasTagName(captionTag))
    m_tree.openElements()->popUntilPopped(captionTag.localName());
    m_tree.activeFormattingElements()->clearToLastMarker();
    setInsertionMode(InTableMode);
    return true;
}

bool HTMLTreeBuilder::processTrEndTagForInRow()
{
    if (!m_tree.openElements()->inTableScope(trTag)) {
        ASSERT(isParsingFragmentOrTemplateContents());
        // FIXME: parse error
        return false;
    }
    m_tree.openElements()->popUntilTableRowScopeMarker();
    ASSERT(m_tree.currentStackItem()->hasTagName(trTag));
    m_tree.openElements()->pop();
    setInsertionMode(InTableBodyMode);
    return true;
}

bool HTMLTreeBuilder::processTableEndTagForInTable()
{
    if (!m_tree.openElements()->inTableScope(tableTag)) {
        ASSERT(isParsingFragmentOrTemplateContents());
        // FIXME: parse error.
        return false;
    }
    m_tree.openElements()->popUntilPopped(tableTag.localName());
    resetInsertionModeAppropriately();
    return true;
}

void HTMLTreeBuilder::processEndTagForInTable(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::EndTag);
    if (token->name() == tableTag) {
        processTableEndTagForInTable();
        return;
    }
    if (token->name() == bodyTag
        || isCaptionColOrColgroupTag(token->name())
        || token->name() == htmlTag
        || isTableBodyContextTag(token->name())
        || isTableCellContextTag(token->name())
        || token->name() == trTag) {
        parseError(token);
        return;
    }
    parseError(token);
    // Is this redirection necessary here?
    HTMLConstructionSite::RedirectToFosterParentGuard redirecter(m_tree);
    processEndTagForInBody(token);
}

void HTMLTreeBuilder::processEndTag(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::EndTag);
    switch (insertionMode()) {
    case InitialMode:
        ASSERT(insertionMode() == InitialMode);
        defaultForInitial();
        // Fall through.
    case BeforeHTMLMode:
        ASSERT(insertionMode() == BeforeHTMLMode);
        if (token->name() != headTag && token->name() != bodyTag && token->name() != htmlTag && token->name() != brTag) {
            parseError(token);
            return;
        }
        defaultForBeforeHTML();
        // Fall through.
    case BeforeHeadMode:
        ASSERT(insertionMode() == BeforeHeadMode);
        if (token->name() != headTag && token->name() != bodyTag && token->name() != htmlTag && token->name() != brTag) {
            parseError(token);
            return;
        }
        defaultForBeforeHead();
        // Fall through.
    case InHeadMode:
        ASSERT(insertionMode() == InHeadMode);
        // FIXME: This case should be broken out into processEndTagForInHead,
        // because other end tag cases now refer to it ("process the token for using the rules of the "in head" insertion mode").
        // but because the logic falls through to AfterHeadMode, that gets a little messy.
#if ENABLE(TEMPLATE_ELEMENT)
        if (token->name() == templateTag) {
            processTemplateEndTag(token);
            return;
        }
#endif
        if (token->name() == headTag) {
            m_tree.openElements()->popHTMLHeadElement();
            setInsertionMode(AfterHeadMode);
            return;
        }
        if (token->name() != bodyTag && token->name() != htmlTag && token->name() != brTag) {
            parseError(token);
            return;
        }
        defaultForInHead();
        // Fall through.
    case AfterHeadMode:
        ASSERT(insertionMode() == AfterHeadMode);
        if (token->name() != bodyTag && token->name() != htmlTag && token->name() != brTag) {
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
        if (token->name() == captionTag) {
            processCaptionEndTagForInCaption();
            return;
        }
        if (token->name() == tableTag) {
            parseError(token);
            if (!processCaptionEndTagForInCaption()) {
                ASSERT(isParsingFragment());
                return;
            }
            processEndTag(token);
            return;
        }
        if (token->name() == bodyTag
            || token->name() == colTag
            || token->name() == colgroupTag
            || token->name() == htmlTag
            || isTableBodyContextTag(token->name())
            || isTableCellContextTag(token->name())
            || token->name() == trTag) {
            parseError(token);
            return;
        }
        processEndTagForInBody(token);
        break;
    case InColumnGroupMode:
        ASSERT(insertionMode() == InColumnGroupMode);
        if (token->name() == colgroupTag) {
            processColgroupEndTagForInColumnGroup();
            return;
        }
        if (token->name() == colTag) {
            parseError(token);
            return;
        }
#if ENABLE(TEMPLATE_ELEMENT)
        if (token->name() == templateTag) {
            processTemplateEndTag(token);
            return;
        }
#endif
        if (!processColgroupEndTagForInColumnGroup()) {
            ASSERT(isParsingFragmentOrTemplateContents());
            return;
        }
        processEndTag(token);
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
        if (token->name() == htmlTag) {
            if (isParsingFragment()) {
                parseError(token);
                return;
            }
            setInsertionMode(AfterAfterBodyMode);
            return;
        }
        // Fall through.
    case AfterAfterBodyMode:
        ASSERT(insertionMode() == AfterBodyMode || insertionMode() == AfterAfterBodyMode);
        parseError(token);
        setInsertionMode(InBodyMode);
        processEndTag(token);
        break;
    case InHeadNoscriptMode:
        ASSERT(insertionMode() == InHeadNoscriptMode);
        if (token->name() == noscriptTag) {
            ASSERT(m_tree.currentStackItem()->hasTagName(noscriptTag));
            m_tree.openElements()->pop();
            ASSERT(m_tree.currentStackItem()->hasTagName(headTag));
            setInsertionMode(InHeadMode);
            return;
        }
        if (token->name() != brTag) {
            parseError(token);
            return;
        }
        defaultForInHeadNoscript();
        processToken(token);
        break;
    case TextMode:
        if (token->name() == scriptTag) {
            // Pause ourselves so that parsing stops until the script can be processed by the caller.
            ASSERT(m_tree.currentStackItem()->hasTagName(scriptTag));
            if (scriptingContentIsAllowed(m_tree.parserContentPolicy()))
                m_scriptToProcess = m_tree.currentElement();
            m_tree.openElements()->pop();
            setInsertionMode(m_originalInsertionMode);

            if (m_parser->tokenizer()) {
                // This token will not have been created by the tokenizer if a
                // self-closing script tag was encountered and pre-HTML5 parser
                // quirks are enabled. We must set the tokenizer's state to
                // DataState explicitly if the tokenizer didn't have a chance to.
                ASSERT(m_parser->tokenizer()->state() == HTMLTokenizer::DataState || m_options.usePreHTML5ParserQuirks || m_options.useThreading);
                m_parser->tokenizer()->setState(HTMLTokenizer::DataState);
            }
            return;
        }
        m_tree.openElements()->pop();
        setInsertionMode(m_originalInsertionMode);
        break;
    case InFramesetMode:
        ASSERT(insertionMode() == InFramesetMode);
        if (token->name() == framesetTag) {
            bool ignoreFramesetForFragmentParsing  = m_tree.currentIsRootNode();
#if ENABLE(TEMPLATE_ELEMENT)
            ignoreFramesetForFragmentParsing = ignoreFramesetForFragmentParsing || m_tree.openElements()->hasTemplateInHTMLScope();
#endif
            if (ignoreFramesetForFragmentParsing) {
                ASSERT(isParsingFragmentOrTemplateContents());
                parseError(token);
                return;
            }
            m_tree.openElements()->pop();
            if (!isParsingFragment() && !m_tree.currentStackItem()->hasTagName(framesetTag))
                setInsertionMode(AfterFramesetMode);
            return;
        }
#if ENABLE(TEMPLATE_ELEMENT)
        if (token->name() == templateTag) {
            processTemplateEndTag(token);
            return;
        }
#endif
        break;
    case AfterFramesetMode:
        ASSERT(insertionMode() == AfterFramesetMode);
        if (token->name() == htmlTag) {
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
        if (token->name() == captionTag
            || token->name() == tableTag
            || isTableBodyContextTag(token->name())
            || token->name() == trTag
            || isTableCellContextTag(token->name())) {
            parseError(token);
            if (m_tree.openElements()->inTableScope(token->name())) {
                AtomicHTMLToken endSelect(HTMLToken::EndTag, selectTag.localName());
                processEndTag(&endSelect);
                processEndTag(token);
            }
            return;
        }
        // Fall through.
    case InSelectMode:
        ASSERT(insertionMode() == InSelectMode || insertionMode() == InSelectInTableMode);
        if (token->name() == optgroupTag) {
            if (isHTMLOptionElement(m_tree.currentStackItem()->node()) && m_tree.oneBelowTop() && isHTMLOptGroupElement(m_tree.oneBelowTop()->node()))
                processFakeEndTag(optionTag);
            if (isHTMLOptGroupElement(m_tree.currentStackItem()->node())) {
                m_tree.openElements()->pop();
                return;
            }
            parseError(token);
            return;
        }
        if (token->name() == optionTag) {
            if (isHTMLOptionElement(m_tree.currentStackItem()->node())) {
                m_tree.openElements()->pop();
                return;
            }
            parseError(token);
            return;
        }
        if (token->name() == selectTag) {
            if (!m_tree.openElements()->inSelectScope(token->name())) {
                ASSERT(isParsingFragment());
                parseError(token);
                return;
            }
            m_tree.openElements()->popUntilPopped(selectTag.localName());
            resetInsertionModeAppropriately();
            return;
        }
#if ENABLE(TEMPLATE_ELEMENT)
        if (token->name() == templateTag) {
            processTemplateEndTag(token);
            return;
        }
#endif
        break;
    case InTableTextMode:
        defaultForInTableText();
        processEndTag(token);
        break;
    case TemplateContentsMode:
#if ENABLE(TEMPLATE_ELEMENT)
        if (token->name() == templateTag) {
            processTemplateEndTag(token);
            return;
        }

        break;
#else
        ASSERT_NOT_REACHED();
#endif
        break;
    }
}

void HTMLTreeBuilder::processComment(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::Comment);
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

void HTMLTreeBuilder::processCharacter(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::Character);
    ExternalCharacterTokenBuffer buffer(token);
    processCharacterBuffer(buffer);
}

void HTMLTreeBuilder::processCharacterBuffer(ExternalCharacterTokenBuffer& buffer)
{
ReprocessBuffer:
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/tokenization.html#parsing-main-inbody
    // Note that this logic is different than the generic \r\n collapsing
    // handled in the input stream preprocessor. This logic is here as an
    // "authoring convenience" so folks can write:
    //
    // <pre>
    // lorem ipsum
    // lorem ipsum
    // </pre>
    //
    // without getting an extra newline at the start of their <pre> element.
    if (m_shouldSkipLeadingNewline) {
        m_shouldSkipLeadingNewline = false;
        buffer.skipAtMostOneLeadingNewline();
        if (buffer.isEmpty())
            return;
    }

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
            m_tree.insertTextNode(leadingWhitespace, AllWhitespace);
        if (buffer.isEmpty())
            return;
        defaultForInHead();
        // Fall through.
    }
    case AfterHeadMode: {
        ASSERT(insertionMode() == AfterHeadMode);
        String leadingWhitespace = buffer.takeLeadingWhitespace();
        if (!leadingWhitespace.isEmpty())
            m_tree.insertTextNode(leadingWhitespace, AllWhitespace);
        if (buffer.isEmpty())
            return;
        defaultForAfterHead();
        // Fall through.
    }
    case InBodyMode:
    case InCaptionMode:
    case TemplateContentsMode:
    case InCellMode: {
#if ENABLE(TEMPLATE_ELEMENT)
        ASSERT(insertionMode() == InBodyMode || insertionMode() == InCaptionMode || insertionMode() == InCellMode || insertionMode() == TemplateContentsMode);
#else
        ASSERT(insertionMode() != TemplateContentsMode);
        ASSERT(insertionMode() == InBodyMode || insertionMode() == InCaptionMode || insertionMode() == InCellMode);
#endif
        processCharacterBufferForInBody(buffer);
        break;
    }
    case InTableMode:
    case InTableBodyMode:
    case InRowMode: {
        ASSERT(insertionMode() == InTableMode || insertionMode() == InTableBodyMode || insertionMode() == InRowMode);
        ASSERT(m_pendingTableCharacters.isEmpty());
        if (m_tree.currentStackItem()->isElementNode()
            && (isHTMLTableElement(m_tree.currentStackItem()->node())
                || m_tree.currentStackItem()->hasTagName(HTMLNames::tbodyTag)
                || m_tree.currentStackItem()->hasTagName(HTMLNames::tfootTag)
                || m_tree.currentStackItem()->hasTagName(HTMLNames::theadTag)
                || m_tree.currentStackItem()->hasTagName(HTMLNames::trTag))) {
            m_originalInsertionMode = m_insertionMode;
            setInsertionMode(InTableTextMode);
            // Note that we fall through to the InTableTextMode case below.
        } else {
            HTMLConstructionSite::RedirectToFosterParentGuard redirecter(m_tree);
            processCharacterBufferForInBody(buffer);
            break;
        }
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
            m_tree.insertTextNode(leadingWhitespace, AllWhitespace);
        if (buffer.isEmpty())
            return;
        if (!processColgroupEndTagForInColumnGroup()) {
            ASSERT(isParsingFragmentOrTemplateContents());
            // The spec tells us to drop these characters on the floor.
            buffer.skipLeadingNonWhitespace();
            if (buffer.isEmpty())
                return;
        }
        goto ReprocessBuffer;
    }
    case AfterBodyMode:
    case AfterAfterBodyMode: {
        ASSERT(insertionMode() == AfterBodyMode || insertionMode() == AfterAfterBodyMode);
        // FIXME: parse error
        setInsertionMode(InBodyMode);
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
            m_tree.insertTextNode(leadingWhitespace, AllWhitespace);
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
            m_tree.insertTextNode(leadingWhitespace, AllWhitespace);
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
    case AfterAfterFramesetMode: {
        String leadingWhitespace = buffer.takeRemainingWhitespace();
        if (!leadingWhitespace.isEmpty()) {
            m_tree.reconstructTheActiveFormattingElements();
            m_tree.insertTextNode(leadingWhitespace, AllWhitespace);
        }
        // FIXME: We should generate a parse error if we skipped over any
        // non-whitespace characters.
        break;
    }
    }
}

void HTMLTreeBuilder::processCharacterBufferForInBody(ExternalCharacterTokenBuffer& buffer)
{
    m_tree.reconstructTheActiveFormattingElements();
    String characters = buffer.takeRemaining();
    m_tree.insertTextNode(characters);
    if (m_framesetOk && !isAllWhitespaceOrReplacementCharacters(characters))
        m_framesetOk = false;
}

void HTMLTreeBuilder::processEndOfFile(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::EndOfFile);
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
#if ENABLE(TEMPLATE_ELEMENT)
        ASSERT(insertionMode() == InBodyMode || insertionMode() == InCellMode || insertionMode() == InCaptionMode || insertionMode() == InRowMode || insertionMode() == TemplateContentsMode);
#else
        ASSERT(insertionMode() != TemplateContentsMode);
        ASSERT(insertionMode() == InBodyMode || insertionMode() == InCellMode || insertionMode() == InCaptionMode || insertionMode() == InRowMode);
#endif
        notImplemented(); // Emit parse error based on what elements are still open.
#if ENABLE(TEMPLATE_ELEMENT)
        if (!m_templateInsertionModes.isEmpty())
            if (processEndOfFileForInTemplateContents(token))
                return;
#endif
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
    case InColumnGroupMode:
        if (m_tree.currentIsRootNode()) {
            ASSERT(isParsingFragment());
            return; // FIXME: Should we break here instead of returning?
        }
#if ENABLE(TEMPLATE_ELEMENT)
        ASSERT(m_tree.currentNode()->hasTagName(colgroupTag) || m_tree.currentNode()->hasTagName(templateTag));
#else
        ASSERT(m_tree.currentNode()->hasTagName(colgroupTag));
#endif
        processColgroupEndTagForInColumnGroup();
        // Fall through
    case InFramesetMode:
    case InTableMode:
    case InTableBodyMode:
    case InSelectInTableMode:
    case InSelectMode:
        ASSERT(insertionMode() == InSelectMode || insertionMode() == InSelectInTableMode || insertionMode() == InTableMode || insertionMode() == InFramesetMode || insertionMode() == InTableBodyMode || insertionMode() == InColumnGroupMode);
        if (m_tree.currentNode() != m_tree.openElements()->rootNode())
            parseError(token);

#if ENABLE(TEMPLATE_ELEMENT)
        if (!m_templateInsertionModes.isEmpty())
            if (processEndOfFileForInTemplateContents(token))
                return;
#endif
        break;
    case InTableTextMode:
        defaultForInTableText();
        processEndOfFile(token);
        return;
    case TextMode:
        parseError(token);
        if (m_tree.currentStackItem()->hasTagName(scriptTag))
            notImplemented(); // mark the script element as "already started".
        m_tree.openElements()->pop();
        ASSERT(m_originalInsertionMode != TextMode);
        setInsertionMode(m_originalInsertionMode);
        processEndOfFile(token);
        return;
    case TemplateContentsMode:
#if ENABLE(TEMPLATE_ELEMENT)
        if (processEndOfFileForInTemplateContents(token))
            return;
        break;
#else
        ASSERT_NOT_REACHED();
#endif
    }
    ASSERT(m_tree.currentNode());
    m_tree.openElements()->popAll();
}

void HTMLTreeBuilder::defaultForInitial()
{
    notImplemented();
    m_tree.setDefaultCompatibilityMode();
    // FIXME: parse error
    setInsertionMode(BeforeHTMLMode);
}

void HTMLTreeBuilder::defaultForBeforeHTML()
{
    AtomicHTMLToken startHTML(HTMLToken::StartTag, htmlTag.localName());
    m_tree.insertHTMLHtmlStartTagBeforeHTML(&startHTML);
    setInsertionMode(BeforeHeadMode);
}

void HTMLTreeBuilder::defaultForBeforeHead()
{
    AtomicHTMLToken startHead(HTMLToken::StartTag, headTag.localName());
    processStartTag(&startHead);
}

void HTMLTreeBuilder::defaultForInHead()
{
    AtomicHTMLToken endHead(HTMLToken::EndTag, headTag.localName());
    processEndTag(&endHead);
}

void HTMLTreeBuilder::defaultForInHeadNoscript()
{
    AtomicHTMLToken endNoscript(HTMLToken::EndTag, noscriptTag.localName());
    processEndTag(&endNoscript);
}

void HTMLTreeBuilder::defaultForAfterHead()
{
    AtomicHTMLToken startBody(HTMLToken::StartTag, bodyTag.localName());
    processStartTag(&startBody);
    m_framesetOk = true;
}

void HTMLTreeBuilder::defaultForInTableText()
{
    String characters = m_pendingTableCharacters.toString();
    m_pendingTableCharacters.clear();
    if (!isAllWhitespace(characters)) {
        // FIXME: parse error
        HTMLConstructionSite::RedirectToFosterParentGuard redirecter(m_tree);
        m_tree.reconstructTheActiveFormattingElements();
        m_tree.insertTextNode(characters, NotAllWhitespace);
        m_framesetOk = false;
        setInsertionMode(m_originalInsertionMode);
        return;
    }
    m_tree.insertTextNode(characters);
    setInsertionMode(m_originalInsertionMode);
}

bool HTMLTreeBuilder::processStartTagForInHead(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::StartTag);
    if (token->name() == htmlTag) {
        processHtmlStartTagForInBody(token);
        return true;
    }
    if (token->name() == baseTag
        || token->name() == basefontTag
        || token->name() == bgsoundTag
        || token->name() == commandTag
        || token->name() == linkTag
        || token->name() == metaTag) {
        m_tree.insertSelfClosingHTMLElement(token);
        // Note: The custom processing for the <meta> tag is done in HTMLMetaElement::process().
        return true;
    }
    if (token->name() == titleTag) {
        processGenericRCDATAStartTag(token);
        return true;
    }
    if (token->name() == noscriptTag) {
        if (m_options.scriptEnabled) {
            processGenericRawTextStartTag(token);
            return true;
        }
        m_tree.insertHTMLElement(token);
        setInsertionMode(InHeadNoscriptMode);
        return true;
    }
    if (token->name() == noframesTag || token->name() == styleTag) {
        processGenericRawTextStartTag(token);
        return true;
    }
    if (token->name() == scriptTag) {
        processScriptStartTag(token);
        if (m_options.usePreHTML5ParserQuirks && token->selfClosing())
            processFakeEndTag(scriptTag);
        return true;
    }
#if ENABLE(TEMPLATE_ELEMENT)
    if (token->name() == templateTag) {
        processTemplateStartTag(token);
        return true;
    }
#endif
    if (token->name() == headTag) {
        parseError(token);
        return true;
    }
    return false;
}

void HTMLTreeBuilder::processGenericRCDATAStartTag(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::StartTag);
    m_tree.insertHTMLElement(token);
    if (m_parser->tokenizer())
        m_parser->tokenizer()->setState(HTMLTokenizer::RCDATAState);
    m_originalInsertionMode = m_insertionMode;
    setInsertionMode(TextMode);
}

void HTMLTreeBuilder::processGenericRawTextStartTag(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::StartTag);
    m_tree.insertHTMLElement(token);
    if (m_parser->tokenizer())
        m_parser->tokenizer()->setState(HTMLTokenizer::RAWTEXTState);
    m_originalInsertionMode = m_insertionMode;
    setInsertionMode(TextMode);
}

void HTMLTreeBuilder::processScriptStartTag(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::StartTag);
    m_tree.insertScriptElement(token);
    if (m_parser->tokenizer())
        m_parser->tokenizer()->setState(HTMLTokenizer::ScriptDataState);
    m_originalInsertionMode = m_insertionMode;

    TextPosition position = m_parser->textPosition();

    m_scriptToProcessStartPosition = position;

    setInsertionMode(TextMode);
}

// http://www.whatwg.org/specs/web-apps/current-work/multipage/tree-construction.html#tree-construction
bool HTMLTreeBuilder::shouldProcessTokenInForeignContent(AtomicHTMLToken* token)
{
    if (m_tree.isEmpty())
        return false;
    HTMLStackItem* item = m_tree.currentStackItem();
    if (item->isInHTMLNamespace())
        return false;
    if (HTMLElementStack::isMathMLTextIntegrationPoint(item)) {
        if (token->type() == HTMLToken::StartTag
            && token->name() != MathMLNames::mglyphTag
            && token->name() != MathMLNames::malignmarkTag)
            return false;
        if (token->type() == HTMLToken::Character)
            return false;
    }
    if (item->hasTagName(MathMLNames::annotation_xmlTag)
        && token->type() == HTMLToken::StartTag
        && token->name() == SVGNames::svgTag)
        return false;
    if (HTMLElementStack::isHTMLIntegrationPoint(item)) {
        if (token->type() == HTMLToken::StartTag)
            return false;
        if (token->type() == HTMLToken::Character)
            return false;
    }
    if (token->type() == HTMLToken::EndOfFile)
        return false;
    return true;
}

void HTMLTreeBuilder::processTokenInForeignContent(AtomicHTMLToken* token)
{
    switch (token->type()) {
    case HTMLToken::Uninitialized:
        ASSERT_NOT_REACHED();
        break;
    case HTMLToken::DOCTYPE:
        parseError(token);
        break;
    case HTMLToken::StartTag: {
        if (token->name() == bTag
            || token->name() == bigTag
            || token->name() == blockquoteTag
            || token->name() == bodyTag
            || token->name() == brTag
            || token->name() == centerTag
            || token->name() == codeTag
            || token->name() == ddTag
            || token->name() == divTag
            || token->name() == dlTag
            || token->name() == dtTag
            || token->name() == emTag
            || token->name() == embedTag
            || isNumberedHeaderTag(token->name())
            || token->name() == headTag
            || token->name() == hrTag
            || token->name() == iTag
            || token->name() == imgTag
            || token->name() == liTag
            || token->name() == listingTag
            || token->name() == menuTag
            || token->name() == metaTag
            || token->name() == nobrTag
            || token->name() == olTag
            || token->name() == pTag
            || token->name() == preTag
            || token->name() == rubyTag
            || token->name() == sTag
            || token->name() == smallTag
            || token->name() == spanTag
            || token->name() == strongTag
            || token->name() == strikeTag
            || token->name() == subTag
            || token->name() == supTag
            || token->name() == tableTag
            || token->name() == ttTag
            || token->name() == uTag
            || token->name() == ulTag
            || token->name() == varTag
            || (token->name() == fontTag && (token->getAttributeItem(colorAttr) || token->getAttributeItem(faceAttr) || token->getAttributeItem(sizeAttr)))) {
            parseError(token);
            m_tree.openElements()->popUntilForeignContentScopeMarker();
            processStartTag(token);
            return;
        }
        const AtomicString& currentNamespace = m_tree.currentStackItem()->namespaceURI();
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
    case HTMLToken::EndTag: {
        if (m_tree.currentStackItem()->namespaceURI() == SVGNames::svgNamespaceURI)
            adjustSVGTagNameCase(token);

        if (token->name() == SVGNames::scriptTag && m_tree.currentStackItem()->hasTagName(SVGNames::scriptTag)) {
            if (scriptingContentIsAllowed(m_tree.parserContentPolicy()))
                m_scriptToProcess = m_tree.currentElement();
            m_tree.openElements()->pop();
            return;
        }
        if (!m_tree.currentStackItem()->isInHTMLNamespace()) {
            // FIXME: This code just wants an Element* iterator, instead of an ElementRecord*
            HTMLElementStack::ElementRecord* nodeRecord = m_tree.openElements()->topRecord();
            if (!nodeRecord->stackItem()->hasLocalName(token->name()))
                parseError(token);
            while (1) {
                if (nodeRecord->stackItem()->hasLocalName(token->name())) {
                    m_tree.openElements()->popUntilPopped(nodeRecord->element());
                    return;
                }
                nodeRecord = nodeRecord->next();

                if (nodeRecord->stackItem()->isInHTMLNamespace())
                    break;
            }
        }
        // Otherwise, process the token according to the rules given in the section corresponding to the current insertion mode in HTML content.
        processEndTag(token);
        break;
    }
    case HTMLToken::Comment:
        m_tree.insertComment(token);
        return;
    case HTMLToken::Character: {
        String characters = String(token->characters(), token->charactersLength());
        m_tree.insertTextNode(characters);
        if (m_framesetOk && !isAllWhitespaceOrReplacementCharacters(characters))
            m_framesetOk = false;
        break;
    }
    case HTMLToken::EndOfFile:
        ASSERT_NOT_REACHED();
        break;
    }
}

void HTMLTreeBuilder::finished()
{
    if (isParsingFragment())
        return;

#if ENABLE(TEMPLATE_ELEMENT)
    ASSERT(m_templateInsertionModes.isEmpty());
#endif

    ASSERT(m_isAttached);
    // Warning, this may detach the parser. Do not do anything else after this.
    m_tree.finishedParsing();
}

void HTMLTreeBuilder::parseError(AtomicHTMLToken*)
{
}

}
