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
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "ScriptElement.h"
#include "ScriptSourceCode.h"
#include "ScriptValue.h"
#include "TextResourceDecoder.h"
#include "TreeDepthLimit.h"
#include "XMLErrors.h"
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

void XMLDocumentParser::pushCurrentNode(ContainerNode* n)
{
    ASSERT(n);
    ASSERT(m_currentNode);
    if (n != document())
        n->ref();
    m_currentNodeStack.append(m_currentNode);
    m_currentNode = n;
    if (m_currentNodeStack.size() > maxDOMTreeDepth)
        handleError(XMLErrors::fatal, "Excessive node nesting.", textPosition());
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
    m_leafTextNode = 0;

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

void XMLDocumentParser::append(PassRefPtr<StringImpl> inputSource)
{
    SegmentedString source(inputSource);
    if (m_sawXSLTransform || !m_sawFirstElement)
        m_originalSourceForTransform.append(source);

    if (isStopped() || m_sawXSLTransform)
        return;

    if (m_parserPaused) {
        m_pendingSrc.append(source);
        return;
    }

    doWrite(source.toString());

    // After parsing, go ahead and dispatch image beforeload events.
    ImageLoader::dispatchPendingBeforeLoadEvents();
}

void XMLDocumentParser::handleError(XMLErrors::ErrorType type, const char* m, TextPosition position)
{
    m_xmlErrors.handleError(type, m, position);
    if (type != XMLErrors::warning)
        m_sawError = true;
    if (type == XMLErrors::fatal)
        stopParsing();
}

void XMLDocumentParser::enterText()
{
#if !USE(QXMLSTREAM)
    ASSERT(m_bufferedText.size() == 0);
#endif
    ASSERT(!m_leafTextNode);
    m_leafTextNode = Text::create(m_currentNode->document(), "");
    m_currentNode->parserAppendChild(m_leafTextNode.get());
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

    if (!m_leafTextNode)
        return;

#if !USE(QXMLSTREAM)
    m_leafTextNode->appendData(toString(m_bufferedText.data(), m_bufferedText.size()), IGNORE_EXCEPTION);
    Vector<xmlChar> empty;
    m_bufferedText.swap(empty);
#endif

    if (m_view && m_leafTextNode->parentNode() && m_leafTextNode->parentNode()->attached()
        && !m_leafTextNode->attached())
        m_leafTextNode->attach();

    m_leafTextNode = 0;
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

    // doEnd() call above can detach the parser and null out its document.
    // In that case, we just bail out.
    if (isDetached())
        return;

    // doEnd() could process a script tag, thus pausing parsing.
    if (m_parserPaused)
        return;

    if (m_sawError)
        insertErrorMessageBlock();
    else {
        exitText();
        document()->styleResolverChanged(RecalcStyleImmediately);
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
    // However, FrameLoader::stop calls DocumentParser::finish unconditionally.

    if (m_parserPaused)
        m_finishCalled = true;
    else
        end();
}

void XMLDocumentParser::insertErrorMessageBlock()
{
#if USE(QXMLSTREAM)
    if (m_parsingFragment)
        return;
#endif

    m_xmlErrors.insertErrorMessageBlock();
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

    ScriptElement* scriptElement = toScriptElementIfPossible(e.get());
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

bool XMLDocumentParser::parseDocumentFragment(const String& chunk, DocumentFragment* fragment, Element* contextElement, ParserContentPolicy parserContentPolicy)
{
    if (!chunk.length())
        return true;

    // FIXME: We need to implement the HTML5 XML Fragment parsing algorithm:
    // http://www.whatwg.org/specs/web-apps/current-work/multipage/the-xhtml-syntax.html#xml-fragment-parsing-algorithm
    // For now we have a hack for script/style innerHTML support:
    if (contextElement && (contextElement->hasLocalName(HTMLNames::scriptTag) || contextElement->hasLocalName(HTMLNames::styleTag))) {
        fragment->parserAppendChild(fragment->document()->createTextNode(chunk));
        return true;
    }

    RefPtr<XMLDocumentParser> parser = XMLDocumentParser::create(fragment, contextElement, parserContentPolicy);
    bool wellFormed = parser->appendFragmentSource(chunk);
    // Do not call finish().  Current finish() and doEnd() implementations touch the main Document/loader
    // and can cause crashes in the fragment case.
    parser->detach(); // Allows ~DocumentParser to assert it was detached before destruction.
    return wellFormed; // appendFragmentSource()'s wellFormed is more permissive than wellFormed().
}

} // namespace WebCore
