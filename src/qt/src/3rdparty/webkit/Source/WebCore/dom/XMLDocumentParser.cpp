/*
 * Copyright (C) 2000 Peter Kelly (pmk@post.com)
 * Copyright (C) 2005, 2006, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2007 Samuel Weinig (sam@webkit.org)
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008 Holger Hans Peter Freyther
 * Copyright (C) 2008 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
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
#include "XMLDocumentParser.h"

#include "CDATASection.h"
#include "CachedScript.h"
#include "Comment.h"
#include "CachedResourceLoader.h"
#include "Document.h"
#include "DocumentFragment.h"
#include "DocumentType.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameView.h"
#include "HTMLLinkElement.h"
#include "HTMLNames.h"
#include "HTMLStyleElement.h"
#include "ImageLoader.h"
#include "ProcessingInstruction.h"
#include "ResourceError.h"
#include "ResourceHandle.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "ScriptElement.h"
#include "ScriptSourceCode.h"
#include "ScriptValue.h"
#include "TextResourceDecoder.h"
#include "TreeDepthLimit.h"
#include <wtf/text/StringConcatenate.h>
#include <wtf/StringExtras.h>
#include <wtf/Threading.h>
#include <wtf/Vector.h>

#if ENABLE(SVG)
#include "SVGNames.h"
#include "SVGStyleElement.h"
#endif

using namespace std;

namespace WebCore {

using namespace HTMLNames;

const int maxErrors = 25;

void XMLDocumentParser::pushCurrentNode(Node* n)
{
    ASSERT(n);
    ASSERT(m_currentNode);
    if (n != document())
        n->ref();
    m_currentNodeStack.append(m_currentNode);
    m_currentNode = n;
    if (m_currentNodeStack.size() > maxDOMTreeDepth)
        handleError(fatal, "Excessive node nesting.", lineNumber(), columnNumber());
}

void XMLDocumentParser::popCurrentNode()
{
    if (!m_currentNode)
        return;
    ASSERT(m_currentNodeStack.size());

    if (m_currentNode != document())
        m_currentNode->deref();

    m_currentNode = m_currentNodeStack.last();
    m_currentNodeStack.removeLast();
}

void XMLDocumentParser::clearCurrentNodeStack()
{
    if (m_currentNode && m_currentNode != document())
        m_currentNode->deref();
    m_currentNode = 0;

    if (m_currentNodeStack.size()) { // Aborted parsing.
        for (size_t i = m_currentNodeStack.size() - 1; i != 0; --i)
            m_currentNodeStack[i]->deref();
        if (m_currentNodeStack[0] && m_currentNodeStack[0] != document())
            m_currentNodeStack[0]->deref();
        m_currentNodeStack.clear();
    }
}

void XMLDocumentParser::insert(const SegmentedString&)
{
    ASSERT_NOT_REACHED();
}

void XMLDocumentParser::append(const SegmentedString& s)
{
    String parseString = s.toString();

    if (m_sawXSLTransform || !m_sawFirstElement)
        m_originalSourceForTransform += parseString;

    if (isStopped() || m_sawXSLTransform)
        return;

    if (m_parserPaused) {
        m_pendingSrc.append(s);
        return;
    }

    doWrite(s.toString());

    // After parsing, go ahead and dispatch image beforeload events.
    ImageLoader::dispatchPendingBeforeLoadEvents();
}

void XMLDocumentParser::handleError(ErrorType type, const char* m, int lineNumber, int columnNumber)
{
    handleError(type, m, TextPosition1(WTF::OneBasedNumber::fromOneBasedInt(lineNumber), WTF::OneBasedNumber::fromOneBasedInt(columnNumber)));
}

void XMLDocumentParser::handleError(ErrorType type, const char* m, TextPosition1 position)
{
    if (type == fatal || (m_errorCount < maxErrors && m_lastErrorPosition.m_line != position.m_line && m_lastErrorPosition.m_column != position.m_column)) {
        switch (type) {
            case warning:
                m_errorMessages += makeString("warning on line ", String::number(position.m_line.oneBasedInt()), " at column ", String::number(position.m_column.oneBasedInt()), ": ", m);
                break;
            case fatal:
            case nonFatal:
                m_errorMessages += makeString("error on line ", String::number(position.m_line.oneBasedInt()), " at column ", String::number(position.m_column.oneBasedInt()), ": ", m);
        }

        m_lastErrorPosition = position;
        ++m_errorCount;
    }

    if (type != warning)
        m_sawError = true;

    if (type == fatal)
        stopParsing();
}

void XMLDocumentParser::enterText()
{
#if !USE(QXMLSTREAM)
    ASSERT(m_bufferedText.size() == 0);
#endif
    RefPtr<Node> newNode = Text::create(document(), "");
    m_currentNode->deprecatedParserAddChild(newNode.get());
    pushCurrentNode(newNode.get());
}

#if !USE(QXMLSTREAM)
static inline String toString(const xmlChar* string, size_t size) 
{ 
    return String::fromUTF8(reinterpret_cast<const char*>(string), size); 
}
#endif


void XMLDocumentParser::exitText()
{
    if (isStopped())
        return;

    if (!m_currentNode || !m_currentNode->isTextNode())
        return;

#if !USE(QXMLSTREAM)
    ExceptionCode ec = 0;
    static_cast<Text*>(m_currentNode)->appendData(toString(m_bufferedText.data(), m_bufferedText.size()), ec);
    Vector<xmlChar> empty;
    m_bufferedText.swap(empty);
#endif

    if (m_view && m_currentNode && !m_currentNode->attached())
        m_currentNode->attach();

    popCurrentNode();
}

void XMLDocumentParser::detach()
{
    clearCurrentNodeStack();
    ScriptableDocumentParser::detach();
}

void XMLDocumentParser::end()
{
    // XMLDocumentParserLibxml2 will do bad things to the document if doEnd() is called.
    // I don't believe XMLDocumentParserQt needs doEnd called in the fragment case.
    ASSERT(!m_parsingFragment);

    doEnd();

    // doEnd() could process a script tag, thus pausing parsing.
    if (m_parserPaused)
        return;

    if (m_sawError)
        insertErrorMessageBlock();
    else {
        exitText();
        document()->styleSelectorChanged(RecalcStyleImmediately);
    }

    if (isParsing())
        prepareToStopParsing();
    document()->setReadyState(Document::Interactive);
    clearCurrentNodeStack();
    document()->finishedParsing();
}

void XMLDocumentParser::finish()
{
    // FIXME: We should ASSERT(!m_parserStopped) here, since it does not
    // makes sense to call any methods on DocumentParser once it's been stopped.
    // However, FrameLoader::stop calls Document::finishParsing unconditionally
    // which in turn calls m_parser->finish().

    if (m_parserPaused)
        m_finishCalled = true;
    else
        end();
}

bool XMLDocumentParser::finishWasCalled()
{
    return m_finishCalled;
}

static inline RefPtr<Element> createXHTMLParserErrorHeader(Document* doc, const String& errorMessages)
{
    RefPtr<Element> reportElement = doc->createElement(QualifiedName(nullAtom, "parsererror", xhtmlNamespaceURI), false);
    reportElement->setAttribute(styleAttr, "display: block; white-space: pre; border: 2px solid #c77; padding: 0 1em 0 1em; margin: 1em; background-color: #fdd; color: black");

    ExceptionCode ec = 0;
    RefPtr<Element> h3 = doc->createElement(h3Tag, false);
    reportElement->appendChild(h3.get(), ec);
    h3->appendChild(doc->createTextNode("This page contains the following errors:"), ec);

    RefPtr<Element> fixed = doc->createElement(divTag, false);
    reportElement->appendChild(fixed.get(), ec);
    fixed->setAttribute(styleAttr, "font-family:monospace;font-size:12px");
    fixed->appendChild(doc->createTextNode(errorMessages), ec);

    h3 = doc->createElement(h3Tag, false);
    reportElement->appendChild(h3.get(), ec);
    h3->appendChild(doc->createTextNode("Below is a rendering of the page up to the first error."), ec);

    return reportElement;
}

void XMLDocumentParser::insertErrorMessageBlock()
{
#if USE(QXMLSTREAM)
    if (m_parsingFragment)
        return;
#endif
    // One or more errors occurred during parsing of the code. Display an error block to the user above
    // the normal content (the DOM tree is created manually and includes line/col info regarding
    // where the errors are located)

    // Create elements for display
    ExceptionCode ec = 0;
    Document* document = this->document();
    RefPtr<Element> documentElement = document->documentElement();
    if (!documentElement) {
        RefPtr<Element> rootElement = document->createElement(htmlTag, false);
        document->appendChild(rootElement, ec);
        RefPtr<Element> body = document->createElement(bodyTag, false);
        rootElement->appendChild(body, ec);
        documentElement = body.get();
    }
#if ENABLE(SVG)
    else if (documentElement->namespaceURI() == SVGNames::svgNamespaceURI) {
        RefPtr<Element> rootElement = document->createElement(htmlTag, false);
        RefPtr<Element> body = document->createElement(bodyTag, false);
        rootElement->appendChild(body, ec);
        body->appendChild(documentElement, ec);
        document->appendChild(rootElement.get(), ec);
        documentElement = body.get();
    }
#endif
    RefPtr<Element> reportElement = createXHTMLParserErrorHeader(document, m_errorMessages);
    documentElement->insertBefore(reportElement, documentElement->firstChild(), ec);
#if ENABLE(XSLT)
    if (document->transformSourceDocument()) {
        RefPtr<Element> paragraph = document->createElement(pTag, false);
        paragraph->setAttribute(styleAttr, "white-space: normal");
        paragraph->appendChild(document->createTextNode("This document was created as the result of an XSL transformation. The line and column numbers given are from the transformed result."), ec);
        reportElement->appendChild(paragraph.release(), ec);
    }
#endif
    document->updateStyleIfNeeded();
}

void XMLDocumentParser::notifyFinished(CachedResource* unusedResource)
{
    ASSERT_UNUSED(unusedResource, unusedResource == m_pendingScript);
    ASSERT(m_pendingScript->accessCount() > 0);

    ScriptSourceCode sourceCode(m_pendingScript.get());
    bool errorOccurred = m_pendingScript->errorOccurred();
    bool wasCanceled = m_pendingScript->wasCanceled();

    m_pendingScript->removeClient(this);
    m_pendingScript = 0;

    RefPtr<Element> e = m_scriptElement;
    m_scriptElement = 0;

    ScriptElement* scriptElement = toScriptElement(e.get());
    ASSERT(scriptElement);

    // JavaScript can detach this parser, make sure it's kept alive even if detached.
    RefPtr<XMLDocumentParser> protect(this);
    
    if (errorOccurred)
        scriptElement->dispatchErrorEvent();
    else if (!wasCanceled) {
        scriptElement->executeScript(sourceCode);
        scriptElement->dispatchLoadEvent();
    }

    m_scriptElement = 0;

    if (!isDetached() && !m_requestingScript)
        resumeParsing();
}

bool XMLDocumentParser::isWaitingForScripts() const
{
    return m_pendingScript;
}

void XMLDocumentParser::pauseParsing()
{
    if (m_parsingFragment)
        return;

    m_parserPaused = true;
}

bool XMLDocumentParser::parseDocumentFragment(const String& chunk, DocumentFragment* fragment, Element* contextElement, FragmentScriptingPermission scriptingPermission)
{
    if (!chunk.length())
        return true;

    // FIXME: We need to implement the HTML5 XML Fragment parsing algorithm:
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/the-xhtml-syntax.html#xml-fragment-parsing-algorithm
    // For now we have a hack for script/style innerHTML support:
    if (contextElement && (contextElement->hasLocalName(HTMLNames::scriptTag) || contextElement->hasLocalName(HTMLNames::styleTag))) {
        fragment->parserAddChild(fragment->document()->createTextNode(chunk));
        return true;
    }

    RefPtr<XMLDocumentParser> parser = XMLDocumentParser::create(fragment, contextElement, scriptingPermission);
    bool wellFormed = parser->appendFragmentSource(chunk);
    // Do not call finish().  Current finish() and doEnd() implementations touch the main Document/loader
    // and can cause crashes in the fragment case.
    parser->detach(); // Allows ~DocumentParser to assert it was detached before destruction.
    return wellFormed; // appendFragmentSource()'s wellFormed is more permissive than wellFormed().
}

} // namespace WebCore
