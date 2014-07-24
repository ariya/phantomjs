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
#include "Comment.h"
#include "DocumentFragment.h"
#include "DocumentType.h"
#include "Element.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "HTMLElementFactory.h"
#include "HTMLFormElement.h"
#include "HTMLHtmlElement.h"
#include "HTMLNames.h"
#include "HTMLOptGroupElement.h"
#include "HTMLOptionElement.h"
#include "HTMLParserIdioms.h"
#include "HTMLScriptElement.h"
#include "HTMLStackItem.h"
#include "HTMLTemplateElement.h"
#include "HTMLToken.h"
#include "NotImplemented.h"
#include "Text.h"

namespace WebCore {

using namespace HTMLNames;

static inline void setAttributes(Element* element, AtomicHTMLToken* token, ParserContentPolicy parserContentPolicy)
{
    if (!scriptingContentIsAllowed(parserContentPolicy))
        element->stripScriptingAttributes(token->attributes());
    element->parserSetAttributes(token->attributes());
}

static bool hasImpliedEndTag(const HTMLStackItem* item)
{
    return item->hasTagName(ddTag)
        || item->hasTagName(dtTag)
        || item->hasTagName(liTag)
        || isHTMLOptionElement(item->node())
        || isHTMLOptGroupElement(item->node())
        || item->hasTagName(pTag)
        || item->hasTagName(rpTag)
        || item->hasTagName(rtTag);
}

static bool shouldUseLengthLimit(const ContainerNode* node)
{
    return !node->hasTagName(scriptTag)
        && !node->hasTagName(styleTag)
        && !node->hasTagName(SVGNames::scriptTag);
}

static inline bool isAllWhitespace(const String& string)
{
    return string.isAllSpecialCharacters<isHTMLSpace>();
}

static inline void executeTask(HTMLConstructionSiteTask& task)
{
#if ENABLE(TEMPLATE_ELEMENT)
    if (task.parent->hasTagName(templateTag))
        task.parent = toHTMLTemplateElement(task.parent.get())->content();
#endif

    if (task.nextChild)
        task.parent->parserInsertBefore(task.child.get(), task.nextChild.get());
    else
        task.parent->parserAppendChild(task.child.get());

    // JavaScript run from beforeload (or DOM Mutation or event handlers)
    // might have removed the child, in which case we should not attach it.

    if (task.child->parentNode() && task.parent->attached() && !task.child->attached())
        task.child->attach();

    task.child->beginParsingChildren();

    if (task.selfClosing)
        task.child->finishParsingChildren();
}

void HTMLConstructionSite::attachLater(ContainerNode* parent, PassRefPtr<Node> prpChild, bool selfClosing)
{
    ASSERT(scriptingContentIsAllowed(m_parserContentPolicy) || !prpChild.get()->isElementNode() || !toScriptElementIfPossible(toElement(prpChild.get())));
    ASSERT(pluginContentIsAllowed(m_parserContentPolicy) || !prpChild->isPluginElement());

    HTMLConstructionSiteTask task;
    task.parent = parent;
    task.child = prpChild;
    task.selfClosing = selfClosing;

    if (shouldFosterParent()) {
        fosterParent(task.child);
        return;
    }

    // Add as a sibling of the parent if we have reached the maximum depth allowed.
    if (m_openElements.stackDepth() > m_maximumDOMTreeDepth && task.parent->parentNode())
        task.parent = task.parent->parentNode();

    ASSERT(task.parent);
    m_attachmentQueue.append(task);
}

void HTMLConstructionSite::executeQueuedTasks()
{
    const size_t size = m_attachmentQueue.size();
    if (!size)
        return;

    // Copy the task queue into a local variable in case executeTask
    // re-enters the parser.
    AttachmentQueue queue;
    queue.swap(m_attachmentQueue);

    for (size_t i = 0; i < size; ++i)
        executeTask(queue[i]);

    // We might be detached now.
}

HTMLConstructionSite::HTMLConstructionSite(Document* document, ParserContentPolicy parserContentPolicy, unsigned maximumDOMTreeDepth)
    : m_document(document)
    , m_attachmentRoot(document)
    , m_parserContentPolicy(parserContentPolicy)
    , m_isParsingFragment(false)
    , m_redirectAttachToFosterParent(false)
    , m_maximumDOMTreeDepth(maximumDOMTreeDepth)
    , m_inQuirksMode(document->inQuirksMode())
{
    ASSERT(m_document->isHTMLDocument() || m_document->isXHTMLDocument());
}

HTMLConstructionSite::HTMLConstructionSite(DocumentFragment* fragment, ParserContentPolicy parserContentPolicy, unsigned maximumDOMTreeDepth)
    : m_document(fragment->document())
    , m_attachmentRoot(fragment)
    , m_parserContentPolicy(parserContentPolicy)
    , m_isParsingFragment(true)
    , m_redirectAttachToFosterParent(false)
    , m_maximumDOMTreeDepth(maximumDOMTreeDepth)
    , m_inQuirksMode(fragment->document()->inQuirksMode())
{
    ASSERT(m_document->isHTMLDocument() || m_document->isXHTMLDocument());
}

HTMLConstructionSite::~HTMLConstructionSite()
{
}

void HTMLConstructionSite::detach()
{
    m_document = 0;
    m_attachmentRoot = 0;
}

void HTMLConstructionSite::setForm(HTMLFormElement* form)
{
    // This method should only be needed for HTMLTreeBuilder in the fragment case.
    ASSERT(!m_form);
    m_form = form;
}

PassRefPtr<HTMLFormElement> HTMLConstructionSite::takeForm()
{
    return m_form.release();
}

void HTMLConstructionSite::dispatchDocumentElementAvailableIfNeeded()
{
    ASSERT(m_document);
    if (m_document->frame() && !m_isParsingFragment)
        m_document->frame()->loader()->dispatchDocumentElementAvailable();
}

void HTMLConstructionSite::insertHTMLHtmlStartTagBeforeHTML(AtomicHTMLToken* token)
{
    RefPtr<HTMLHtmlElement> element = HTMLHtmlElement::create(m_document);
    setAttributes(element.get(), token, m_parserContentPolicy);
    attachLater(m_attachmentRoot, element);
    m_openElements.pushHTMLHtmlElement(HTMLStackItem::create(element, token));

    executeQueuedTasks();
    element->insertedByParser();
    dispatchDocumentElementAvailableIfNeeded();
}

void HTMLConstructionSite::mergeAttributesFromTokenIntoElement(AtomicHTMLToken* token, Element* element)
{
    if (token->attributes().isEmpty())
        return;

    for (unsigned i = 0; i < token->attributes().size(); ++i) {
        const Attribute& tokenAttribute = token->attributes().at(i);
        if (!element->elementData() || !element->getAttributeItem(tokenAttribute.name()))
            element->setAttribute(tokenAttribute.name(), tokenAttribute.value());
    }
}

void HTMLConstructionSite::insertHTMLHtmlStartTagInBody(AtomicHTMLToken* token)
{
    // Fragments do not have a root HTML element, so any additional HTML elements
    // encountered during fragment parsing should be ignored.
    if (m_isParsingFragment)
        return;

    mergeAttributesFromTokenIntoElement(token, m_openElements.htmlElement());
}

void HTMLConstructionSite::insertHTMLBodyStartTagInBody(AtomicHTMLToken* token)
{
    mergeAttributesFromTokenIntoElement(token, m_openElements.bodyElement());
}

void HTMLConstructionSite::setDefaultCompatibilityMode()
{
    if (m_isParsingFragment)
        return;
    if (m_document->isSrcdocDocument())
        return;
    setCompatibilityMode(Document::QuirksMode);
}

void HTMLConstructionSite::setCompatibilityMode(Document::CompatibilityMode mode)
{
    m_inQuirksMode = (mode == Document::QuirksMode);
    m_document->setCompatibilityMode(mode);
}

void HTMLConstructionSite::setCompatibilityModeFromDoctype(const String& name, const String& publicId, const String& systemId)
{
    // There are three possible compatibility modes:
    // Quirks - quirks mode emulates WinIE and NS4. CSS parsing is also relaxed in this mode, e.g., unit types can
    // be omitted from numbers.
    // Limited Quirks - This mode is identical to no-quirks mode except for its treatment of line-height in the inline box model.  
    // No Quirks - no quirks apply. Web pages will obey the specifications to the letter.

    // Check for Quirks Mode.
    if (name != "html"
        || publicId.startsWith("+//Silmaril//dtd html Pro v0r11 19970101//", false)
        || publicId.startsWith("-//AdvaSoft Ltd//DTD HTML 3.0 asWedit + extensions//", false)
        || publicId.startsWith("-//AS//DTD HTML 3.0 asWedit + extensions//", false)
        || publicId.startsWith("-//IETF//DTD HTML 2.0 Level 1//", false)
        || publicId.startsWith("-//IETF//DTD HTML 2.0 Level 2//", false)
        || publicId.startsWith("-//IETF//DTD HTML 2.0 Strict Level 1//", false)
        || publicId.startsWith("-//IETF//DTD HTML 2.0 Strict Level 2//", false)
        || publicId.startsWith("-//IETF//DTD HTML 2.0 Strict//", false)
        || publicId.startsWith("-//IETF//DTD HTML 2.0//", false)
        || publicId.startsWith("-//IETF//DTD HTML 2.1E//", false)
        || publicId.startsWith("-//IETF//DTD HTML 3.0//", false)
        || publicId.startsWith("-//IETF//DTD HTML 3.2 Final//", false)
        || publicId.startsWith("-//IETF//DTD HTML 3.2//", false)
        || publicId.startsWith("-//IETF//DTD HTML 3//", false)
        || publicId.startsWith("-//IETF//DTD HTML Level 0//", false)
        || publicId.startsWith("-//IETF//DTD HTML Level 1//", false)
        || publicId.startsWith("-//IETF//DTD HTML Level 2//", false)
        || publicId.startsWith("-//IETF//DTD HTML Level 3//", false)
        || publicId.startsWith("-//IETF//DTD HTML Strict Level 0//", false)
        || publicId.startsWith("-//IETF//DTD HTML Strict Level 1//", false)
        || publicId.startsWith("-//IETF//DTD HTML Strict Level 2//", false)
        || publicId.startsWith("-//IETF//DTD HTML Strict Level 3//", false)
        || publicId.startsWith("-//IETF//DTD HTML Strict//", false)
        || publicId.startsWith("-//IETF//DTD HTML//", false)
        || publicId.startsWith("-//Metrius//DTD Metrius Presentational//", false)
        || publicId.startsWith("-//Microsoft//DTD Internet Explorer 2.0 HTML Strict//", false)
        || publicId.startsWith("-//Microsoft//DTD Internet Explorer 2.0 HTML//", false)
        || publicId.startsWith("-//Microsoft//DTD Internet Explorer 2.0 Tables//", false)
        || publicId.startsWith("-//Microsoft//DTD Internet Explorer 3.0 HTML Strict//", false)
        || publicId.startsWith("-//Microsoft//DTD Internet Explorer 3.0 HTML//", false)
        || publicId.startsWith("-//Microsoft//DTD Internet Explorer 3.0 Tables//", false)
        || publicId.startsWith("-//Netscape Comm. Corp.//DTD HTML//", false)
        || publicId.startsWith("-//Netscape Comm. Corp.//DTD Strict HTML//", false)
        || publicId.startsWith("-//O'Reilly and Associates//DTD HTML 2.0//", false)
        || publicId.startsWith("-//O'Reilly and Associates//DTD HTML Extended 1.0//", false)
        || publicId.startsWith("-//O'Reilly and Associates//DTD HTML Extended Relaxed 1.0//", false)
        || publicId.startsWith("-//SoftQuad Software//DTD HoTMetaL PRO 6.0::19990601::extensions to HTML 4.0//", false)
        || publicId.startsWith("-//SoftQuad//DTD HoTMetaL PRO 4.0::19971010::extensions to HTML 4.0//", false)
        || publicId.startsWith("-//Spyglass//DTD HTML 2.0 Extended//", false)
        || publicId.startsWith("-//SQ//DTD HTML 2.0 HoTMetaL + extensions//", false)
        || publicId.startsWith("-//Sun Microsystems Corp.//DTD HotJava HTML//", false)
        || publicId.startsWith("-//Sun Microsystems Corp.//DTD HotJava Strict HTML//", false)
        || publicId.startsWith("-//W3C//DTD HTML 3 1995-03-24//", false)
        || publicId.startsWith("-//W3C//DTD HTML 3.2 Draft//", false)
        || publicId.startsWith("-//W3C//DTD HTML 3.2 Final//", false)
        || publicId.startsWith("-//W3C//DTD HTML 3.2//", false)
        || publicId.startsWith("-//W3C//DTD HTML 3.2S Draft//", false)
        || publicId.startsWith("-//W3C//DTD HTML 4.0 Frameset//", false)
        || publicId.startsWith("-//W3C//DTD HTML 4.0 Transitional//", false)
        || publicId.startsWith("-//W3C//DTD HTML Experimental 19960712//", false)
        || publicId.startsWith("-//W3C//DTD HTML Experimental 970421//", false)
        || publicId.startsWith("-//W3C//DTD W3 HTML//", false)
        || publicId.startsWith("-//W3O//DTD W3 HTML 3.0//", false)
        || equalIgnoringCase(publicId, "-//W3O//DTD W3 HTML Strict 3.0//EN//")
        || publicId.startsWith("-//WebTechs//DTD Mozilla HTML 2.0//", false)
        || publicId.startsWith("-//WebTechs//DTD Mozilla HTML//", false)
        || equalIgnoringCase(publicId, "-/W3C/DTD HTML 4.0 Transitional/EN")
        || equalIgnoringCase(publicId, "HTML")
        || equalIgnoringCase(systemId, "http://www.ibm.com/data/dtd/v11/ibmxhtml1-transitional.dtd")
        || (systemId.isEmpty() && publicId.startsWith("-//W3C//DTD HTML 4.01 Frameset//", false))
        || (systemId.isEmpty() && publicId.startsWith("-//W3C//DTD HTML 4.01 Transitional//", false))) {
        setCompatibilityMode(Document::QuirksMode);
        return;
    }

    // Check for Limited Quirks Mode.
    if (publicId.startsWith("-//W3C//DTD XHTML 1.0 Frameset//", false)
        || publicId.startsWith("-//W3C//DTD XHTML 1.0 Transitional//", false)
        || (!systemId.isEmpty() && publicId.startsWith("-//W3C//DTD HTML 4.01 Frameset//", false))
        || (!systemId.isEmpty() && publicId.startsWith("-//W3C//DTD HTML 4.01 Transitional//", false))) {
        setCompatibilityMode(Document::LimitedQuirksMode);
        return;
    }

    // Otherwise we are No Quirks Mode.
    setCompatibilityMode(Document::NoQuirksMode);
}

void HTMLConstructionSite::finishedParsing()
{
    m_document->finishedParsing();
}

void HTMLConstructionSite::insertDoctype(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::DOCTYPE);

    const String& publicId = StringImpl::create8BitIfPossible(token->publicIdentifier());
    const String& systemId = StringImpl::create8BitIfPossible(token->systemIdentifier());
    RefPtr<DocumentType> doctype = DocumentType::create(m_document, token->name(), publicId, systemId);
    attachLater(m_attachmentRoot, doctype.release());

    // DOCTYPE nodes are only processed when parsing fragments w/o contextElements, which
    // never occurs.  However, if we ever chose to support such, this code is subtly wrong,
    // because context-less fragments can determine their own quirks mode, and thus change
    // parsing rules (like <p> inside <table>).  For now we ASSERT that we never hit this code
    // in a fragment, as changing the owning document's compatibility mode would be wrong.
    ASSERT(!m_isParsingFragment);
    if (m_isParsingFragment)
        return;

    if (token->forceQuirks())
        setCompatibilityMode(Document::QuirksMode);
    else {
        setCompatibilityModeFromDoctype(token->name(), publicId, systemId);
    }
}

void HTMLConstructionSite::insertComment(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::Comment);
    attachLater(currentNode(), Comment::create(ownerDocumentForCurrentNode(), token->comment()));
}

void HTMLConstructionSite::insertCommentOnDocument(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::Comment);
    attachLater(m_attachmentRoot, Comment::create(m_document, token->comment()));
}

void HTMLConstructionSite::insertCommentOnHTMLHtmlElement(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::Comment);
    ContainerNode* parent = m_openElements.rootNode();
    attachLater(parent, Comment::create(parent->document(), token->comment()));
}

void HTMLConstructionSite::insertHTMLHeadElement(AtomicHTMLToken* token)
{
    ASSERT(!shouldFosterParent());
    m_head = HTMLStackItem::create(createHTMLElement(token), token);
    attachLater(currentNode(), m_head->element());
    m_openElements.pushHTMLHeadElement(m_head);
}

void HTMLConstructionSite::insertHTMLBodyElement(AtomicHTMLToken* token)
{
    ASSERT(!shouldFosterParent());
    RefPtr<Element> body = createHTMLElement(token);
    attachLater(currentNode(), body);
    m_openElements.pushHTMLBodyElement(HTMLStackItem::create(body.release(), token));
    if (Frame* frame = m_document->frame())
        frame->loader()->client()->dispatchWillInsertBody();
}

void HTMLConstructionSite::insertHTMLFormElement(AtomicHTMLToken* token, bool isDemoted)
{
    RefPtr<Element> element = createHTMLElement(token);
    ASSERT(isHTMLFormElement(element.get()));
    m_form = static_pointer_cast<HTMLFormElement>(element.release());
    m_form->setDemoted(isDemoted);
    attachLater(currentNode(), m_form);
    m_openElements.push(HTMLStackItem::create(m_form, token));
}

void HTMLConstructionSite::insertHTMLElement(AtomicHTMLToken* token)
{
    RefPtr<Element> element = createHTMLElement(token);
    attachLater(currentNode(), element);
    m_openElements.push(HTMLStackItem::create(element.release(), token));
}

void HTMLConstructionSite::insertSelfClosingHTMLElement(AtomicHTMLToken* token)
{
    ASSERT(token->type() == HTMLToken::StartTag);
    // Normally HTMLElementStack is responsible for calling finishParsingChildren,
    // but self-closing elements are never in the element stack so the stack
    // doesn't get a chance to tell them that we're done parsing their children.
    attachLater(currentNode(), createHTMLElement(token), true);
    // FIXME: Do we want to acknowledge the token's self-closing flag?
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/tokenization.html#acknowledge-self-closing-flag
}

void HTMLConstructionSite::insertFormattingElement(AtomicHTMLToken* token)
{
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/parsing.html#the-stack-of-open-elements
    // Possible active formatting elements include:
    // a, b, big, code, em, font, i, nobr, s, small, strike, strong, tt, and u.
    insertHTMLElement(token);
    m_activeFormattingElements.append(currentElementRecord()->stackItem());
}

void HTMLConstructionSite::insertScriptElement(AtomicHTMLToken* token)
{
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/scripting-1.html#already-started
    // http://html5.org/specs/dom-parsing.html#dom-range-createcontextualfragment
    // For createContextualFragment, the specifications say to mark it parser-inserted and already-started and later unmark them.
    // However, we short circuit that logic to avoid the subtree traversal to find script elements since scripts can never see
    // those flags or effects thereof.
    const bool parserInserted = m_parserContentPolicy != AllowScriptingContentAndDoNotMarkAlreadyStarted;
    const bool alreadyStarted = m_isParsingFragment && parserInserted;
    RefPtr<HTMLScriptElement> element = HTMLScriptElement::create(scriptTag, ownerDocumentForCurrentNode(), parserInserted, alreadyStarted);
    setAttributes(element.get(), token, m_parserContentPolicy);
    if (scriptingContentIsAllowed(m_parserContentPolicy))
        attachLater(currentNode(), element);
    m_openElements.push(HTMLStackItem::create(element.release(), token));
}

void HTMLConstructionSite::insertForeignElement(AtomicHTMLToken* token, const AtomicString& namespaceURI)
{
    ASSERT(token->type() == HTMLToken::StartTag);
    notImplemented(); // parseError when xmlns or xmlns:xlink are wrong.

    RefPtr<Element> element = createElement(token, namespaceURI);
    if (scriptingContentIsAllowed(m_parserContentPolicy) || !toScriptElementIfPossible(element.get()))
        attachLater(currentNode(), element, token->selfClosing());
    if (!token->selfClosing())
        m_openElements.push(HTMLStackItem::create(element.release(), token, namespaceURI));
}

void HTMLConstructionSite::insertTextNode(const String& characters, WhitespaceMode whitespaceMode)
{
    HTMLConstructionSiteTask task;
    task.parent = currentNode();

    if (shouldFosterParent())
        findFosterSite(task);

#if ENABLE(TEMPLATE_ELEMENT)
    if (task.parent->hasTagName(templateTag))
        task.parent = toHTMLTemplateElement(task.parent.get())->content();
#endif

    // Strings composed entirely of whitespace are likely to be repeated.
    // Turn them into AtomicString so we share a single string for each.
    bool shouldUseAtomicString = whitespaceMode == AllWhitespace
        || (whitespaceMode == WhitespaceUnknown && isAllWhitespace(characters));

    unsigned currentPosition = 0;
    unsigned lengthLimit = shouldUseLengthLimit(task.parent.get()) ? Text::defaultLengthLimit : std::numeric_limits<unsigned>::max();

    // FIXME: Splitting text nodes into smaller chunks contradicts HTML5 spec, but is currently necessary
    // for performance, see <https://bugs.webkit.org/show_bug.cgi?id=55898>.

    Node* previousChild = task.nextChild ? task.nextChild->previousSibling() : task.parent->lastChild();
    if (previousChild && previousChild->isTextNode()) {
        // FIXME: We're only supposed to append to this text node if it
        // was the last text node inserted by the parser.
        CharacterData* textNode = static_cast<CharacterData*>(previousChild);
        currentPosition = textNode->parserAppendData(characters, 0, lengthLimit);
    }

    while (currentPosition < characters.length()) {
        RefPtr<Text> textNode = Text::createWithLengthLimit(task.parent->document(), shouldUseAtomicString ? AtomicString(characters).string() : characters, currentPosition, lengthLimit);
        // If we have a whole string of unbreakable characters the above could lead to an infinite loop. Exceeding the length limit is the lesser evil.
        if (!textNode->length()) {
            String substring = characters.substring(currentPosition);
            textNode = Text::create(task.parent->document(), shouldUseAtomicString ? AtomicString(substring).string() : substring);
        }

        currentPosition += textNode->length();
        ASSERT(currentPosition <= characters.length());
        task.child = textNode.release();

        executeTask(task);
    }
}

PassRefPtr<Element> HTMLConstructionSite::createElement(AtomicHTMLToken* token, const AtomicString& namespaceURI)
{
    QualifiedName tagName(nullAtom, token->name(), namespaceURI);
    RefPtr<Element> element = ownerDocumentForCurrentNode()->createElement(tagName, true);
    setAttributes(element.get(), token, m_parserContentPolicy);
    return element.release();
}

inline Document* HTMLConstructionSite::ownerDocumentForCurrentNode()
{
#if ENABLE(TEMPLATE_ELEMENT)
    if (currentNode()->hasTagName(templateTag))
        return toHTMLTemplateElement(currentElement())->content()->document();
#endif
    return currentNode()->document();
}

PassRefPtr<Element> HTMLConstructionSite::createHTMLElement(AtomicHTMLToken* token)
{
    QualifiedName tagName(nullAtom, token->name(), xhtmlNamespaceURI);
    // FIXME: This can't use HTMLConstructionSite::createElement because we
    // have to pass the current form element.  We should rework form association
    // to occur after construction to allow better code sharing here.
    RefPtr<Element> element = HTMLElementFactory::createHTMLElement(tagName, ownerDocumentForCurrentNode(), form(), true);
    setAttributes(element.get(), token, m_parserContentPolicy);
    ASSERT(element->isHTMLElement());
    return element.release();
}

PassRefPtr<HTMLStackItem> HTMLConstructionSite::createElementFromSavedToken(HTMLStackItem* item)
{
    RefPtr<Element> element;
    // NOTE: Moving from item -> token -> item copies the Attribute vector twice!
    AtomicHTMLToken fakeToken(HTMLToken::StartTag, item->localName(), item->attributes());
    if (item->namespaceURI() == HTMLNames::xhtmlNamespaceURI)
        element = createHTMLElement(&fakeToken);
    else
        element = createElement(&fakeToken, item->namespaceURI());
    return HTMLStackItem::create(element.release(), &fakeToken, item->namespaceURI());
}

bool HTMLConstructionSite::indexOfFirstUnopenFormattingElement(unsigned& firstUnopenElementIndex) const
{
    if (m_activeFormattingElements.isEmpty())
        return false;
    unsigned index = m_activeFormattingElements.size();
    do {
        --index;
        const HTMLFormattingElementList::Entry& entry = m_activeFormattingElements.at(index);
        if (entry.isMarker() || m_openElements.contains(entry.element())) {
            firstUnopenElementIndex = index + 1;
            return firstUnopenElementIndex < m_activeFormattingElements.size();
        }
    } while (index);
    firstUnopenElementIndex = index;
    return true;
}

void HTMLConstructionSite::reconstructTheActiveFormattingElements()
{
    unsigned firstUnopenElementIndex;
    if (!indexOfFirstUnopenFormattingElement(firstUnopenElementIndex))
        return;

    unsigned unopenEntryIndex = firstUnopenElementIndex;
    ASSERT(unopenEntryIndex < m_activeFormattingElements.size());
    for (; unopenEntryIndex < m_activeFormattingElements.size(); ++unopenEntryIndex) {
        HTMLFormattingElementList::Entry& unopenedEntry = m_activeFormattingElements.at(unopenEntryIndex);
        RefPtr<HTMLStackItem> reconstructed = createElementFromSavedToken(unopenedEntry.stackItem().get());
        attachLater(currentNode(), reconstructed->node());
        m_openElements.push(reconstructed);
        unopenedEntry.replaceElement(reconstructed.release());
    }
}

void HTMLConstructionSite::generateImpliedEndTagsWithExclusion(const AtomicString& tagName)
{
    while (hasImpliedEndTag(currentStackItem()) && !currentStackItem()->matchesHTMLTag(tagName))
        m_openElements.pop();
}

void HTMLConstructionSite::generateImpliedEndTags()
{
    while (hasImpliedEndTag(currentStackItem()))
        m_openElements.pop();
}

bool HTMLConstructionSite::inQuirksMode()
{
    return m_inQuirksMode;
}

void HTMLConstructionSite::findFosterSite(HTMLConstructionSiteTask& task)
{
#if ENABLE(TEMPLATE_ELEMENT)
    // When a node is to be foster parented, the last template element with no table element is below it in the stack of open elements is the foster parent element (NOT the template's parent!)
    HTMLElementStack::ElementRecord* lastTemplateElement = m_openElements.topmost(templateTag.localName());
    if (lastTemplateElement && !m_openElements.inTableScope(tableTag)) {
        task.parent = lastTemplateElement->element();
        return;
    }

#endif

    HTMLElementStack::ElementRecord* lastTableElementRecord = m_openElements.topmost(tableTag.localName());
    if (lastTableElementRecord) {
        Element* lastTableElement = lastTableElementRecord->element();
        ContainerNode* parent = lastTableElement->parentNode();
        // When parsing HTML fragments, we skip step 4.2 ("Let root be a new html element with no attributes") for efficiency,
        // and instead use the DocumentFragment as a root node. So we must treat the root node (DocumentFragment) as if it is a html element here.
        bool parentCanBeFosterParent = parent && (parent->isElementNode() || (m_isParsingFragment && parent == m_openElements.rootNode()));
#if ENABLE(TEMPLATE_ELEMENT)
        parentCanBeFosterParent = parentCanBeFosterParent || (parent && parent->isDocumentFragment() && static_cast<DocumentFragment*>(parent)->isTemplateContent());
#endif
        if (parentCanBeFosterParent) {
            task.parent = parent;
            task.nextChild = lastTableElement;
            return;
        }
        task.parent = lastTableElementRecord->next()->element();
        return;
    }
    // Fragment case
    task.parent = m_openElements.rootNode(); // DocumentFragment
}

bool HTMLConstructionSite::shouldFosterParent() const
{
    return m_redirectAttachToFosterParent
        && currentStackItem()->isElementNode()
        && currentStackItem()->causesFosterParenting();
}

void HTMLConstructionSite::fosterParent(PassRefPtr<Node> node)
{
    HTMLConstructionSiteTask task;
    findFosterSite(task);
    task.child = node;
    ASSERT(task.parent);

    m_attachmentQueue.append(task);
}

}
