/*
 * Copyright (C) 2013 Google, Inc. All Rights Reserved.
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

#if ENABLE(THREADED_HTML_PARSER)

#include "HTMLTreeBuilderSimulator.h"

#include "HTMLDocumentParser.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "HTMLTokenizer.h"
#include "HTMLTreeBuilder.h"
#include "MathMLNames.h"
#include "SVGNames.h"

namespace WebCore {

using namespace HTMLNames;

static bool tokenExitsForeignContent(const CompactHTMLToken& token)
{
    // FIXME: This is copied from HTMLTreeBuilder::processTokenInForeignContent and changed to use threadSafeHTMLNamesMatch.
    const HTMLIdentifier& tagName = token.data();
    return threadSafeHTMLNamesMatch(tagName, bTag)
        || threadSafeHTMLNamesMatch(tagName, bigTag)
        || threadSafeHTMLNamesMatch(tagName, blockquoteTag)
        || threadSafeHTMLNamesMatch(tagName, bodyTag)
        || threadSafeHTMLNamesMatch(tagName, brTag)
        || threadSafeHTMLNamesMatch(tagName, centerTag)
        || threadSafeHTMLNamesMatch(tagName, codeTag)
        || threadSafeHTMLNamesMatch(tagName, ddTag)
        || threadSafeHTMLNamesMatch(tagName, divTag)
        || threadSafeHTMLNamesMatch(tagName, dlTag)
        || threadSafeHTMLNamesMatch(tagName, dtTag)
        || threadSafeHTMLNamesMatch(tagName, emTag)
        || threadSafeHTMLNamesMatch(tagName, embedTag)
        || threadSafeHTMLNamesMatch(tagName, h1Tag)
        || threadSafeHTMLNamesMatch(tagName, h2Tag)
        || threadSafeHTMLNamesMatch(tagName, h3Tag)
        || threadSafeHTMLNamesMatch(tagName, h4Tag)
        || threadSafeHTMLNamesMatch(tagName, h5Tag)
        || threadSafeHTMLNamesMatch(tagName, h6Tag)
        || threadSafeHTMLNamesMatch(tagName, headTag)
        || threadSafeHTMLNamesMatch(tagName, hrTag)
        || threadSafeHTMLNamesMatch(tagName, iTag)
        || threadSafeHTMLNamesMatch(tagName, imgTag)
        || threadSafeHTMLNamesMatch(tagName, liTag)
        || threadSafeHTMLNamesMatch(tagName, listingTag)
        || threadSafeHTMLNamesMatch(tagName, menuTag)
        || threadSafeHTMLNamesMatch(tagName, metaTag)
        || threadSafeHTMLNamesMatch(tagName, nobrTag)
        || threadSafeHTMLNamesMatch(tagName, olTag)
        || threadSafeHTMLNamesMatch(tagName, pTag)
        || threadSafeHTMLNamesMatch(tagName, preTag)
        || threadSafeHTMLNamesMatch(tagName, rubyTag)
        || threadSafeHTMLNamesMatch(tagName, sTag)
        || threadSafeHTMLNamesMatch(tagName, smallTag)
        || threadSafeHTMLNamesMatch(tagName, spanTag)
        || threadSafeHTMLNamesMatch(tagName, strongTag)
        || threadSafeHTMLNamesMatch(tagName, strikeTag)
        || threadSafeHTMLNamesMatch(tagName, subTag)
        || threadSafeHTMLNamesMatch(tagName, supTag)
        || threadSafeHTMLNamesMatch(tagName, tableTag)
        || threadSafeHTMLNamesMatch(tagName, ttTag)
        || threadSafeHTMLNamesMatch(tagName, uTag)
        || threadSafeHTMLNamesMatch(tagName, ulTag)
        || threadSafeHTMLNamesMatch(tagName, varTag)
        || (threadSafeHTMLNamesMatch(tagName, fontTag) && (token.getAttributeItem(colorAttr) || token.getAttributeItem(faceAttr) || token.getAttributeItem(sizeAttr)));
}

static bool tokenExitsSVG(const CompactHTMLToken& token)
{
    // FIXME: It's very fragile that we special case foreignObject here to be case-insensitive.
    return equalIgnoringCaseNonNull(token.data().asStringImpl(), SVGNames::foreignObjectTag.localName().impl());
}

static bool tokenExitsMath(const CompactHTMLToken& token)
{
    // FIXME: This is copied from HTMLElementStack::isMathMLTextIntegrationPoint and changed to use threadSafeMatch.
    const HTMLIdentifier& tagName = token.data();
    return threadSafeMatch(tagName, MathMLNames::miTag)
        || threadSafeMatch(tagName, MathMLNames::moTag)
        || threadSafeMatch(tagName, MathMLNames::mnTag)
        || threadSafeMatch(tagName, MathMLNames::msTag)
        || threadSafeMatch(tagName, MathMLNames::mtextTag);
}

HTMLTreeBuilderSimulator::HTMLTreeBuilderSimulator(const HTMLParserOptions& options)
    : m_options(options)
{
    m_namespaceStack.append(HTML);
}

HTMLTreeBuilderSimulator::State HTMLTreeBuilderSimulator::stateFor(HTMLTreeBuilder* treeBuilder)
{
    ASSERT(isMainThread());
    State namespaceStack;
    for (HTMLElementStack::ElementRecord* record = treeBuilder->openElements()->topRecord(); record; record = record->next()) {
        Namespace currentNamespace = HTML;
        if (record->namespaceURI() == SVGNames::svgNamespaceURI)
            currentNamespace = SVG;
        else if (record->namespaceURI() == MathMLNames::mathmlNamespaceURI)
            currentNamespace = MathML;

        if (namespaceStack.isEmpty() || namespaceStack.last() != currentNamespace)
            namespaceStack.append(currentNamespace);
    }
    namespaceStack.reverse();
    return namespaceStack;
}

bool HTMLTreeBuilderSimulator::simulate(const CompactHTMLToken& token, HTMLTokenizer* tokenizer)
{
    if (token.type() == HTMLToken::StartTag) {
        const HTMLIdentifier& tagName = token.data();
        if (threadSafeMatch(tagName, SVGNames::svgTag))
            m_namespaceStack.append(SVG);
        if (threadSafeMatch(tagName, MathMLNames::mathTag))
            m_namespaceStack.append(MathML);
        if (inForeignContent() && tokenExitsForeignContent(token))
            m_namespaceStack.removeLast();
        if ((m_namespaceStack.last() == SVG && tokenExitsSVG(token))
            || (m_namespaceStack.last() == MathML && tokenExitsMath(token)))
            m_namespaceStack.append(HTML);
        if (!inForeignContent()) {
            // FIXME: This is just a copy of Tokenizer::updateStateFor which uses threadSafeMatches.
            if (threadSafeHTMLNamesMatch(tagName, textareaTag) || threadSafeHTMLNamesMatch(tagName, titleTag))
                tokenizer->setState(HTMLTokenizer::RCDATAState);
            else if (threadSafeHTMLNamesMatch(tagName, plaintextTag))
                tokenizer->setState(HTMLTokenizer::PLAINTEXTState);
            else if (threadSafeHTMLNamesMatch(tagName, scriptTag))
                tokenizer->setState(HTMLTokenizer::ScriptDataState);
            else if (threadSafeHTMLNamesMatch(tagName, styleTag)
                || threadSafeHTMLNamesMatch(tagName, iframeTag)
                || threadSafeHTMLNamesMatch(tagName, xmpTag)
                || (threadSafeHTMLNamesMatch(tagName, noembedTag) && m_options.pluginsEnabled)
                || threadSafeHTMLNamesMatch(tagName, noframesTag)
                || (threadSafeHTMLNamesMatch(tagName, noscriptTag) && m_options.scriptEnabled))
                tokenizer->setState(HTMLTokenizer::RAWTEXTState);
        }
    }

    if (token.type() == HTMLToken::EndTag) {
        const HTMLIdentifier& tagName = token.data();
        if ((m_namespaceStack.last() == SVG && threadSafeMatch(tagName, SVGNames::svgTag))
            || (m_namespaceStack.last() == MathML && threadSafeMatch(tagName, MathMLNames::mathTag))
            || (m_namespaceStack.contains(SVG) && m_namespaceStack.last() == HTML && tokenExitsSVG(token))
            || (m_namespaceStack.contains(MathML) && m_namespaceStack.last() == HTML && tokenExitsMath(token)))
            m_namespaceStack.removeLast();
        if (threadSafeHTMLNamesMatch(tagName, scriptTag)) {
            if (!inForeignContent())
                tokenizer->setState(HTMLTokenizer::DataState);
            return false;
        }
    }

    // FIXME: Also setForceNullCharacterReplacement when in text mode.
    tokenizer->setForceNullCharacterReplacement(inForeignContent());
    tokenizer->setShouldAllowCDATA(inForeignContent());
    return true;
}

}

#endif // ENABLE(THREADED_HTML_PARSER)
