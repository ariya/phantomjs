/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2009 Torch Mobile, Inc. http://www.torchmobile.com/
 * Copyright (C) 2010 Google Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "HTMLPreloadScanner.h"

#include "CachedResourceLoader.h"
#include "Document.h"
#include "InputType.h"
#include "HTMLDocumentParser.h"
#include "HTMLTokenizer.h"
#include "HTMLLinkElement.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "MediaList.h"
#include "MediaQueryEvaluator.h"

namespace WebCore {

using namespace HTMLNames;

namespace {

class PreloadTask {
public:
    PreloadTask(const HTMLToken& token)
        : m_tagName(token.name().data(), token.name().size())
        , m_linkIsStyleSheet(false)
        , m_linkMediaAttributeIsScreen(true)
        , m_inputIsImage(false)
    {
        processAttributes(token.attributes());
    }

    void processAttributes(const HTMLToken::AttributeList& attributes)
    {
        if (m_tagName != imgTag
            && m_tagName != inputTag
            && m_tagName != linkTag
            && m_tagName != scriptTag)
            return;

        for (HTMLToken::AttributeList::const_iterator iter = attributes.begin();
             iter != attributes.end(); ++iter) {
            AtomicString attributeName(iter->m_name.data(), iter->m_name.size());
            String attributeValue(iter->m_value.data(), iter->m_value.size());

            if (attributeName == charsetAttr)
                m_charset = attributeValue;

            if (m_tagName == scriptTag || m_tagName == imgTag) {
                if (attributeName == srcAttr)
                    setUrlToLoad(attributeValue);
            } else if (m_tagName == linkTag) {
                if (attributeName == hrefAttr)
                    setUrlToLoad(attributeValue);
                else if (attributeName == relAttr)
                    m_linkIsStyleSheet = relAttributeIsStyleSheet(attributeValue);
                else if (attributeName == mediaAttr)
                    m_linkMediaAttributeIsScreen = linkMediaAttributeIsScreen(attributeValue);
            } else if (m_tagName == inputTag) {
                if (attributeName == srcAttr)
                    setUrlToLoad(attributeValue);
                else if (attributeName == typeAttr)
                    m_inputIsImage = equalIgnoringCase(attributeValue, InputTypeNames::image());
            }
        }
    }

    static bool relAttributeIsStyleSheet(const String& attributeValue)
    {
        HTMLLinkElement::RelAttribute rel;
        HTMLLinkElement::tokenizeRelAttribute(attributeValue, rel);
        return rel.m_isStyleSheet && !rel.m_isAlternate && rel.m_iconType == InvalidIcon && !rel.m_isDNSPrefetch;
    }

    static bool linkMediaAttributeIsScreen(const String& attributeValue)
    {
        if (attributeValue.isEmpty())
            return true;
        RefPtr<MediaList> mediaList = MediaList::createAllowingDescriptionSyntax(attributeValue);
    
        // Only preload screen media stylesheets. Used this way, the evaluator evaluates to true for any 
        // rules containing complex queries (full evaluation is possible but it requires a frame and a style selector which
        // may be problematic here).
        MediaQueryEvaluator mediaQueryEvaluator("screen");
        return mediaQueryEvaluator.eval(mediaList.get());
    }

    void setUrlToLoad(const String& attributeValue)
    {
        // We only respect the first src/href, per HTML5:
        // http://www.whatwg.org/specs/web-apps/current-work/multipage/tokenization.html#attribute-name-state
        if (!m_urlToLoad.isEmpty())
            return;
        m_urlToLoad = stripLeadingAndTrailingHTMLSpaces(attributeValue);
    }

    void preload(Document* document, bool scanningBody)
    {
        if (m_urlToLoad.isEmpty())
            return;

        CachedResourceLoader* cachedResourceLoader = document->cachedResourceLoader();
        if (m_tagName == scriptTag)
            cachedResourceLoader->preload(CachedResource::Script, m_urlToLoad, m_charset, scanningBody);
        else if (m_tagName == imgTag || (m_tagName == inputTag && m_inputIsImage))
            cachedResourceLoader->preload(CachedResource::ImageResource, m_urlToLoad, String(), scanningBody);
        else if (m_tagName == linkTag && m_linkIsStyleSheet && m_linkMediaAttributeIsScreen) 
            cachedResourceLoader->preload(CachedResource::CSSStyleSheet, m_urlToLoad, m_charset, scanningBody);
    }

    const AtomicString& tagName() const { return m_tagName; }

private:
    AtomicString m_tagName;
    String m_urlToLoad;
    String m_charset;
    bool m_linkIsStyleSheet;
    bool m_linkMediaAttributeIsScreen;
    bool m_inputIsImage;
};

} // namespace

HTMLPreloadScanner::HTMLPreloadScanner(Document* document)
    : m_document(document)
    , m_cssScanner(document)
    , m_tokenizer(HTMLTokenizer::create(HTMLDocumentParser::usePreHTML5ParserQuirks(document)))
    , m_bodySeen(false)
    , m_inStyle(false)
{
}

void HTMLPreloadScanner::appendToEnd(const SegmentedString& source)
{
    m_source.append(source);
}

void HTMLPreloadScanner::scan()
{
    // FIXME: We should save and re-use these tokens in HTMLDocumentParser if
    // the pending script doesn't end up calling document.write.
    while (m_tokenizer->nextToken(m_source, m_token)) {
        processToken();
        m_token.clear();
    }
}

void HTMLPreloadScanner::processToken()
{
    if (m_inStyle) {
        if (m_token.type() == HTMLToken::Character)
            m_cssScanner.scan(m_token, scanningBody());
        else if (m_token.type() == HTMLToken::EndTag) {
            m_inStyle = false;
            m_cssScanner.reset();
        }
    }

    if (m_token.type() != HTMLToken::StartTag)
        return;

    PreloadTask task(m_token);
    m_tokenizer->updateStateFor(task.tagName(), m_document->frame());

    if (task.tagName() == bodyTag)
        m_bodySeen = true;

    if (task.tagName() == styleTag)
        m_inStyle = true;

    task.preload(m_document, scanningBody());
}

bool HTMLPreloadScanner::scanningBody() const
{
    return m_document->body() || m_bodySeen;
}

}
