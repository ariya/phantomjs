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

#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "HTMLParserOptions.h"
#include "HTMLTokenizer.h"
#include "InputTypeNames.h"
#include "LinkRelAttribute.h"
#include "MediaList.h"
#include "MediaQueryEvaluator.h"
#include <wtf/MainThread.h>

namespace WebCore {

using namespace HTMLNames;

TokenPreloadScanner::TagId TokenPreloadScanner::tagIdFor(const HTMLToken::DataVector& data)
{
    AtomicString tagName(data);
    if (tagName == imgTag)
        return ImgTagId;
    if (tagName == inputTag)
        return InputTagId;
    if (tagName == linkTag)
        return LinkTagId;
    if (tagName == scriptTag)
        return ScriptTagId;
    if (tagName == styleTag)
        return StyleTagId;
    if (tagName == baseTag)
        return BaseTagId;
    if (tagName == templateTag)
        return TemplateTagId;
    return UnknownTagId;
}

#if ENABLE(THREADED_HTML_PARSER)
TokenPreloadScanner::TagId TokenPreloadScanner::tagIdFor(const HTMLIdentifier& tagName)
{
    if (threadSafeHTMLNamesMatch(tagName, imgTag))
        return ImgTagId;
    if (threadSafeHTMLNamesMatch(tagName, inputTag))
        return InputTagId;
    if (threadSafeHTMLNamesMatch(tagName, linkTag))
        return LinkTagId;
    if (threadSafeHTMLNamesMatch(tagName, scriptTag))
        return ScriptTagId;
    if (threadSafeHTMLNamesMatch(tagName, styleTag))
        return StyleTagId;
    if (threadSafeHTMLNamesMatch(tagName, baseTag))
        return BaseTagId;
    if (threadSafeHTMLNamesMatch(tagName, templateTag))
        return TemplateTagId;
    return UnknownTagId;
}
#endif

String TokenPreloadScanner::initiatorFor(TagId tagId)
{
    switch (tagId) {
    case ImgTagId:
        return "img";
    case InputTagId:
        return "input";
    case LinkTagId:
        return "link";
    case ScriptTagId:
        return "script";
    case UnknownTagId:
    case StyleTagId:
    case BaseTagId:
    case TemplateTagId:
        ASSERT_NOT_REACHED();
        return "unknown";
    }
    ASSERT_NOT_REACHED();
    return "unknown";
}

class TokenPreloadScanner::StartTagScanner {
public:
    explicit StartTagScanner(TagId tagId)
        : m_tagId(tagId)
        , m_linkIsStyleSheet(false)
        , m_linkMediaAttributeIsScreen(true)
        , m_inputIsImage(false)
    {
    }

    void processAttributes(const HTMLToken::AttributeList& attributes)
    {
        ASSERT(isMainThread());
        if (m_tagId >= UnknownTagId)
            return;
        for (HTMLToken::AttributeList::const_iterator iter = attributes.begin(); iter != attributes.end(); ++iter) {
            AtomicString attributeName(iter->name);
            String attributeValue = StringImpl::create8BitIfPossible(iter->value);
            processAttribute(attributeName, attributeValue);
        }
    }

#if ENABLE(THREADED_HTML_PARSER)
    void processAttributes(const Vector<CompactHTMLToken::Attribute>& attributes)
    {
        if (m_tagId >= UnknownTagId)
            return;
        for (Vector<CompactHTMLToken::Attribute>::const_iterator iter = attributes.begin(); iter != attributes.end(); ++iter)
            processAttribute(iter->name, iter->value);
    }
#endif

    PassOwnPtr<PreloadRequest> createPreloadRequest(const KURL& predictedBaseURL)
    {
        if (!shouldPreload())
            return nullptr;

        OwnPtr<PreloadRequest> request = PreloadRequest::create(initiatorFor(m_tagId), m_urlToLoad, predictedBaseURL, resourceType());
        request->setCrossOriginModeAllowsCookies(crossOriginModeAllowsCookies());
        request->setCharset(charset());
        return request.release();
    }

static bool match(const AtomicString& name, const QualifiedName& qName)
{
    ASSERT(isMainThread());
    return qName.localName() == name;
}

#if ENABLE(THREADED_HTML_PARSER)
static bool match(const HTMLIdentifier& name, const QualifiedName& qName)
{
    return threadSafeHTMLNamesMatch(name, qName);
}
#endif

private:
    template<typename NameType>
    void processAttribute(const NameType& attributeName, const String& attributeValue)
    {
        if (match(attributeName, charsetAttr))
            m_charset = attributeValue;

        if (m_tagId == ScriptTagId || m_tagId == ImgTagId) {
            if (match(attributeName, srcAttr))
                setUrlToLoad(attributeValue);
            else if (match(attributeName, crossoriginAttr) && !attributeValue.isNull())
                m_crossOriginMode = stripLeadingAndTrailingHTMLSpaces(attributeValue);
        } else if (m_tagId == LinkTagId) {
            if (match(attributeName, hrefAttr))
                setUrlToLoad(attributeValue);
            else if (match(attributeName, relAttr))
                m_linkIsStyleSheet = relAttributeIsStyleSheet(attributeValue);
            else if (match(attributeName, mediaAttr))
                m_linkMediaAttributeIsScreen = linkMediaAttributeIsScreen(attributeValue);
        } else if (m_tagId == InputTagId) {
            if (match(attributeName, srcAttr))
                setUrlToLoad(attributeValue);
            else if (match(attributeName, typeAttr))
                m_inputIsImage = equalIgnoringCase(attributeValue, InputTypeNames::image());
        }
    }

    static bool relAttributeIsStyleSheet(const String& attributeValue)
    {
        LinkRelAttribute rel(attributeValue);
        return rel.m_isStyleSheet && !rel.m_isAlternate && rel.m_iconType == InvalidIcon && !rel.m_isDNSPrefetch;
    }

    static bool linkMediaAttributeIsScreen(const String& attributeValue)
    {
        if (attributeValue.isEmpty())
            return true;
        RefPtr<MediaQuerySet> mediaQueries = MediaQuerySet::createAllowingDescriptionSyntax(attributeValue);
    
        // Only preload screen media stylesheets. Used this way, the evaluator evaluates to true for any 
        // rules containing complex queries (full evaluation is possible but it requires a frame and a style selector which
        // may be problematic here).
        MediaQueryEvaluator mediaQueryEvaluator("screen");
        return mediaQueryEvaluator.eval(mediaQueries.get());
    }

    void setUrlToLoad(const String& attributeValue)
    {
        // We only respect the first src/href, per HTML5:
        // http://www.whatwg.org/specs/web-apps/current-work/multipage/tokenization.html#attribute-name-state
        if (!m_urlToLoad.isEmpty())
            return;
        m_urlToLoad = stripLeadingAndTrailingHTMLSpaces(attributeValue);
    }

    const String& charset() const
    {
        // FIXME: Its not clear that this if is needed, the loader probably ignores charset for image requests anyway.
        if (m_tagId == ImgTagId)
            return emptyString();
        return m_charset;
    }

    CachedResource::Type resourceType() const
    {
        if (m_tagId == ScriptTagId)
            return CachedResource::Script;
        if (m_tagId == ImgTagId || (m_tagId == InputTagId && m_inputIsImage))
            return CachedResource::ImageResource;
        if (m_tagId == LinkTagId && m_linkIsStyleSheet && m_linkMediaAttributeIsScreen)
            return CachedResource::CSSStyleSheet;
        ASSERT_NOT_REACHED();
        return CachedResource::RawResource;
    }

    bool shouldPreload()
    {
        if (m_urlToLoad.isEmpty())
            return false;

        if (m_tagId == LinkTagId && (!m_linkIsStyleSheet || !m_linkMediaAttributeIsScreen))
            return false;

        if (m_tagId == InputTagId && !m_inputIsImage)
            return false;

        return true;
    }

    bool crossOriginModeAllowsCookies()
    {
        return m_crossOriginMode.isNull() || equalIgnoringCase(m_crossOriginMode, "use-credentials");
    }

    TagId m_tagId;
    String m_urlToLoad;
    String m_charset;
    String m_crossOriginMode;
    bool m_linkIsStyleSheet;
    bool m_linkMediaAttributeIsScreen;
    bool m_inputIsImage;
};

TokenPreloadScanner::TokenPreloadScanner(const KURL& documentURL)
    : m_documentURL(documentURL)
    , m_inStyle(false)
#if ENABLE(TEMPLATE_ELEMENT)
    , m_templateCount(0)
#endif
{
}

TokenPreloadScanner::~TokenPreloadScanner()
{
}

TokenPreloadScannerCheckpoint TokenPreloadScanner::createCheckpoint()
{
    TokenPreloadScannerCheckpoint checkpoint = m_checkpoints.size();
    m_checkpoints.append(Checkpoint(m_predictedBaseElementURL, m_inStyle
#if ENABLE(TEMPLATE_ELEMENT)
                                    , m_templateCount
#endif
                                    ));
    return checkpoint;
}

void TokenPreloadScanner::rewindTo(TokenPreloadScannerCheckpoint checkpointIndex)
{
    ASSERT(checkpointIndex < m_checkpoints.size()); // If this ASSERT fires, checkpointIndex is invalid.
    const Checkpoint& checkpoint = m_checkpoints[checkpointIndex];
    m_predictedBaseElementURL = checkpoint.predictedBaseElementURL;
    m_inStyle = checkpoint.inStyle;
#if ENABLE(TEMPLATE_ELEMENT)
    m_templateCount = checkpoint.templateCount;
#endif
    m_cssScanner.reset();
    m_checkpoints.clear();
}

void TokenPreloadScanner::scan(const HTMLToken& token, Vector<OwnPtr<PreloadRequest> >& requests)
{
    scanCommon(token, requests);
}

#if ENABLE(THREADED_HTML_PARSER)
void TokenPreloadScanner::scan(const CompactHTMLToken& token, Vector<OwnPtr<PreloadRequest> >& requests)
{
    scanCommon(token, requests);
}
#endif

template<typename Token>
void TokenPreloadScanner::scanCommon(const Token& token, Vector<OwnPtr<PreloadRequest> >& requests)
{
    switch (token.type()) {
    case HTMLToken::Character: {
        if (!m_inStyle)
            return;
        m_cssScanner.scan(token.data(), requests);
        return;
    }
    case HTMLToken::EndTag: {
        TagId tagId = tagIdFor(token.data());
#if ENABLE(TEMPLATE_ELEMENT)
        if (tagId == TemplateTagId) {
            if (m_templateCount)
                --m_templateCount;
            return;
        }
#endif
        if (tagId == StyleTagId) {
            if (m_inStyle)
                m_cssScanner.reset();
            m_inStyle = false;
        }
        return;
    }
    case HTMLToken::StartTag: {
#if ENABLE(TEMPLATE_ELEMENT)
        if (m_templateCount)
            return;
#endif
        TagId tagId = tagIdFor(token.data());
#if ENABLE(TEMPLATE_ELEMENT)
        if (tagId == TemplateTagId) {
            ++m_templateCount;
            return;
        }
#endif
        if (tagId == StyleTagId) {
            m_inStyle = true;
            return;
        }
        if (tagId == BaseTagId) {
            // The first <base> element is the one that wins.
            if (!m_predictedBaseElementURL.isEmpty())
                return;
            updatePredictedBaseURL(token);
            return;
        }

        StartTagScanner scanner(tagId);
        scanner.processAttributes(token.attributes());
        OwnPtr<PreloadRequest> request = scanner.createPreloadRequest(m_predictedBaseElementURL);
        if (request)
            requests.append(request.release());
        return;
    }
    default: {
        return;
    }
    }
}

template<typename Token>
void TokenPreloadScanner::updatePredictedBaseURL(const Token& token)
{
    ASSERT(m_predictedBaseElementURL.isEmpty());
    if (const typename Token::Attribute* hrefAttribute = token.getAttributeItem(hrefAttr))
        m_predictedBaseElementURL = KURL(m_documentURL, stripLeadingAndTrailingHTMLSpaces(hrefAttribute->value)).copy();
}

HTMLPreloadScanner::HTMLPreloadScanner(const HTMLParserOptions& options, const KURL& documentURL)
    : m_scanner(documentURL)
    , m_tokenizer(HTMLTokenizer::create(options))
{
}

HTMLPreloadScanner::~HTMLPreloadScanner()
{
}

void HTMLPreloadScanner::appendToEnd(const SegmentedString& source)
{
    m_source.append(source);
}

void HTMLPreloadScanner::scan(HTMLResourcePreloader* preloader, const KURL& startingBaseElementURL)
{
    ASSERT(isMainThread()); // HTMLTokenizer::updateStateFor only works on the main thread.

    // When we start scanning, our best prediction of the baseElementURL is the real one!
    if (!startingBaseElementURL.isEmpty())
        m_scanner.setPredictedBaseElementURL(startingBaseElementURL);

    PreloadRequestStream requests;

    while (m_tokenizer->nextToken(m_source, m_token)) {
        if (m_token.type() == HTMLToken::StartTag)
            m_tokenizer->updateStateFor(AtomicString(m_token.name()));
        m_scanner.scan(m_token, requests);
        m_token.clear();
    }

    preloader->takeAndPreload(requests);
}

}
