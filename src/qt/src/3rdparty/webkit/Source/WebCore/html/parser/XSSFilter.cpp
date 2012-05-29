/*
 * Copyright (C) 2011 Adam Barth. All Rights Reserved.
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
#include "XSSFilter.h"

#include "DOMWindow.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "HTMLDocumentParser.h"
#include "HTMLNames.h"
#include "HTMLParamElement.h"
#include "HTMLParserIdioms.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include "TextEncoding.h"
#include "TextResourceDecoder.h"
#include <wtf/text/CString.h>

namespace WebCore {

using namespace HTMLNames;

namespace {

bool isNonCanonicalCharacter(UChar c)
{
    // We remove all non-ASCII characters, including non-printable ASCII characters.
    //
    // Note, we don't remove backslashes like PHP stripslashes(), which among other things converts "\\0" to the \0 character.
    // Instead, we remove backslashes and zeros (since the string "\\0" =(remove backslashes)=> "0"). However, this has the 
    // adverse effect that we remove any legitimate zeros from a string.
    //
    // For instance: new String("http://localhost:8000") => new String("http://localhost:8").
    return (c == '\\' || c == '0' || c == '\0' || c >= 127);
}

String canonicalize(const String& string)
{
    return string.removeCharacters(&isNonCanonicalCharacter);
}

bool isRequiredForInjection(UChar c)
{
    return (c == '\'' || c == '"' || c == '<' || c == '>');
}

bool hasName(const HTMLToken& token, const QualifiedName& name)
{
    return equalIgnoringNullity(token.name(), static_cast<const String&>(name.localName()));
}

bool findAttributeWithName(const HTMLToken& token, const QualifiedName& name, size_t& indexOfMatchingAttribute)
{
    for (size_t i = 0; i < token.attributes().size(); ++i) {
        if (equalIgnoringNullity(token.attributes().at(i).m_name, name.localName())) {
            indexOfMatchingAttribute = i;
            return true;
        }
    }
    return false;
}

bool isNameOfInlineEventHandler(const Vector<UChar, 32>& name)
{
    const size_t lengthOfShortestInlineEventHandlerName = 5; // To wit: oncut.
    if (name.size() < lengthOfShortestInlineEventHandlerName)
        return false;
    return name[0] == 'o' && name[1] == 'n';
}

bool containsJavaScriptURL(const Vector<UChar, 32>& value)
{
    static const char javaScriptScheme[] = "javascript:";
    static const size_t lengthOfJavaScriptScheme = sizeof(javaScriptScheme) - 1;

    size_t i;
    for (i = 0; i < value.size(); ++i) {
        if (!isHTMLSpace(value[i]))
            break;
    }

    if (value.size() - i < lengthOfJavaScriptScheme)
        return false;

    return equalIgnoringCase(value.data() + i, javaScriptScheme, lengthOfJavaScriptScheme);
}

String decodeURL(const String& string, const TextEncoding& encoding)
{
    String workingString = string;
    workingString.replace('+', ' ');
    workingString = decodeURLEscapeSequences(workingString);
    CString workingStringUTF8 = workingString.utf8();
    String decodedString = encoding.decode(workingStringUTF8.data(), workingStringUTF8.length());
    // FIXME: Is this check necessary?
    if (decodedString.isEmpty())
        return canonicalize(workingString);
    return canonicalize(decodedString);
}

}

XSSFilter::XSSFilter(HTMLDocumentParser* parser)
    : m_parser(parser)
    , m_isEnabled(false)
    , m_xssProtection(XSSProtectionEnabled)
    , m_state(Uninitialized)
{
    ASSERT(m_parser);
    if (Frame* frame = parser->document()->frame()) {
        if (Settings* settings = frame->settings())
            m_isEnabled = settings->xssAuditorEnabled();
    }
    // Although tempting to call init() at this point, the various objects
    // we want to reference might not all have been constructed yet.
}

void XSSFilter::init()
{
    const size_t miniumLengthForSuffixTree = 512; // FIXME: Tune this parameter.
    const int suffixTreeDepth = 5;

    ASSERT(m_state == Uninitialized);
    m_state = Initial;

    if (!m_isEnabled)
        return;
    
    // In theory, the Document could have detached from the Frame after the
    // XSSFilter was constructed.
    if (!m_parser->document()->frame()) {
        m_isEnabled = false;
        return;
    }

    const KURL& url = m_parser->document()->url();

    if (url.protocolIsData()) {
        m_isEnabled = false;
        return;
    }

    TextResourceDecoder* decoder = m_parser->document()->decoder();
    m_decodedURL = decoder ? decodeURL(url.string(), decoder->encoding()) : url.string();
    if (m_decodedURL.find(isRequiredForInjection, 0) == notFound)
        m_decodedURL = String();

    if (DocumentLoader* documentLoader = m_parser->document()->frame()->loader()->documentLoader()) {
        DEFINE_STATIC_LOCAL(String, XSSProtectionHeader, ("X-XSS-Protection"));
        m_xssProtection = parseXSSProtectionHeader(documentLoader->response().httpHeaderField(XSSProtectionHeader));

        FormData* httpBody = documentLoader->originalRequest().httpBody();
        if (httpBody && !httpBody->isEmpty()) {
            String httpBodyAsString = httpBody->flattenToString();
            m_decodedHTTPBody = decoder ? decodeURL(httpBodyAsString, decoder->encoding()) : httpBodyAsString;
            if (m_decodedHTTPBody.find(isRequiredForInjection, 0) == notFound)
                m_decodedHTTPBody = String();
            if (m_decodedHTTPBody.length() >= miniumLengthForSuffixTree)
                m_decodedHTTPBodySuffixTree = adoptPtr(new SuffixTree<ASCIICodebook>(m_decodedHTTPBody, suffixTreeDepth));
        }
    }

    if (m_decodedURL.isEmpty() && m_decodedHTTPBody.isEmpty())
        m_isEnabled = false;
}

void XSSFilter::filterToken(HTMLToken& token)
{
    if (m_state == Uninitialized) {
        init();
        ASSERT(m_state == Initial);
    }

    if (!m_isEnabled || m_xssProtection == XSSProtectionDisabled)
        return;

    bool didBlockScript = false;

    switch (m_state) {
    case Uninitialized:
        ASSERT_NOT_REACHED();
        break;
    case Initial: 
        didBlockScript = filterTokenInitial(token);
        break;
    case AfterScriptStartTag:
        didBlockScript = filterTokenAfterScriptStartTag(token);
        ASSERT(m_state == Initial);
        m_cachedSnippet = String();
        break;
    }

    if (didBlockScript) {
        // FIXME: Consider using a more helpful console message.
        DEFINE_STATIC_LOCAL(String, consoleMessage, ("Refused to execute a JavaScript script. Source code of script found within request.\n"));
        // FIXME: We should add the real line number to the console.
        m_parser->document()->domWindow()->console()->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, consoleMessage, 1, String());

        if (m_xssProtection == XSSProtectionBlockEnabled) {
            m_parser->document()->frame()->loader()->stopAllLoaders();
            m_parser->document()->frame()->navigationScheduler()->scheduleLocationChange(m_parser->document()->securityOrigin(), blankURL(), String());
        }
    }
}

bool XSSFilter::filterTokenInitial(HTMLToken& token)
{
    ASSERT(m_state == Initial);

    if (token.type() != HTMLToken::StartTag)
        return false;

    bool didBlockScript = eraseDangerousAttributesIfInjected(token);

    if (hasName(token, scriptTag))
        didBlockScript |= filterScriptToken(token);
    else if (hasName(token, objectTag))
        didBlockScript |= filterObjectToken(token);
    else if (hasName(token, paramTag))
        didBlockScript |= filterParamToken(token);
    else if (hasName(token, embedTag))
        didBlockScript |= filterEmbedToken(token);
    else if (hasName(token, appletTag))
        didBlockScript |= filterAppletToken(token);
    else if (hasName(token, iframeTag))
        didBlockScript |= filterIframeToken(token);
    else if (hasName(token, metaTag))
        didBlockScript |= filterMetaToken(token);
    else if (hasName(token, baseTag))
        didBlockScript |= filterBaseToken(token);
    else if (hasName(token, formTag))
        didBlockScript |= filterFormToken(token);

    return didBlockScript;
}

bool XSSFilter::filterTokenAfterScriptStartTag(HTMLToken& token)
{
    ASSERT(m_state == AfterScriptStartTag);
    m_state = Initial;

    if (token.type() != HTMLToken::Character) {
        ASSERT(token.type() == HTMLToken::EndTag || token.type() == HTMLToken::EndOfFile);
        return false;
    }

    int start = 0;
    // FIXME: We probably want to grab only the first few characters of the
    //        contents of the script element.
    int end = token.endIndex() - token.startIndex();
    if (isContainedInRequest(m_cachedSnippet + snippetForRange(token, start, end))) {
        token.eraseCharacters();
        token.appendToCharacter(' '); // Technically, character tokens can't be empty.
        return true;
    }
    return false;
}

bool XSSFilter::filterScriptToken(HTMLToken& token)
{
    ASSERT(m_state == Initial);
    ASSERT(token.type() == HTMLToken::StartTag);
    ASSERT(hasName(token, scriptTag));

    if (eraseAttributeIfInjected(token, srcAttr, blankURL().string()))
        return true;

    m_state = AfterScriptStartTag;
    m_cachedSnippet = m_parser->sourceForToken(token);
    return false;
}

bool XSSFilter::filterObjectToken(HTMLToken& token)
{
    ASSERT(m_state == Initial);
    ASSERT(token.type() == HTMLToken::StartTag);
    ASSERT(hasName(token, objectTag));

    bool didBlockScript = false;

    didBlockScript |= eraseAttributeIfInjected(token, dataAttr, blankURL().string());
    didBlockScript |= eraseAttributeIfInjected(token, typeAttr);
    didBlockScript |= eraseAttributeIfInjected(token, classidAttr);

    return didBlockScript;
}

bool XSSFilter::filterParamToken(HTMLToken& token)
{
    ASSERT(m_state == Initial);
    ASSERT(token.type() == HTMLToken::StartTag);
    ASSERT(hasName(token, paramTag));

    size_t indexOfNameAttribute;
    if (!findAttributeWithName(token, nameAttr, indexOfNameAttribute))
        return false;

    const HTMLToken::Attribute& nameAttribute = token.attributes().at(indexOfNameAttribute);
    String name = String(nameAttribute.m_value.data(), nameAttribute.m_value.size());

    if (!HTMLParamElement::isURLParameter(name))
        return false;

    return eraseAttributeIfInjected(token, valueAttr, blankURL().string());
}

bool XSSFilter::filterEmbedToken(HTMLToken& token)
{
    ASSERT(m_state == Initial);
    ASSERT(token.type() == HTMLToken::StartTag);
    ASSERT(hasName(token, embedTag));

    bool didBlockScript = false;

    didBlockScript |= eraseAttributeIfInjected(token, srcAttr, blankURL().string());
    didBlockScript |= eraseAttributeIfInjected(token, typeAttr);

    return didBlockScript;
}

bool XSSFilter::filterAppletToken(HTMLToken& token)
{
    ASSERT(m_state == Initial);
    ASSERT(token.type() == HTMLToken::StartTag);
    ASSERT(hasName(token, appletTag));

    bool didBlockScript = false;

    didBlockScript |= eraseAttributeIfInjected(token, codeAttr);
    didBlockScript |= eraseAttributeIfInjected(token, objectAttr);

    return didBlockScript;
}

bool XSSFilter::filterIframeToken(HTMLToken& token)
{
    ASSERT(m_state == Initial);
    ASSERT(token.type() == HTMLToken::StartTag);
    ASSERT(hasName(token, iframeTag));

    return eraseAttributeIfInjected(token, srcAttr);
}

bool XSSFilter::filterMetaToken(HTMLToken& token)
{
    ASSERT(m_state == Initial);
    ASSERT(token.type() == HTMLToken::StartTag);
    ASSERT(hasName(token, metaTag));

    return eraseAttributeIfInjected(token, http_equivAttr);
}

bool XSSFilter::filterBaseToken(HTMLToken& token)
{
    ASSERT(m_state == Initial);
    ASSERT(token.type() == HTMLToken::StartTag);
    ASSERT(hasName(token, baseTag));

    return eraseAttributeIfInjected(token, hrefAttr);
}

bool XSSFilter::filterFormToken(HTMLToken& token)
{
    ASSERT(m_state == Initial);
    ASSERT(token.type() == HTMLToken::StartTag);
    ASSERT(hasName(token, formTag));

    return eraseAttributeIfInjected(token, actionAttr);
}

bool XSSFilter::eraseDangerousAttributesIfInjected(HTMLToken& token)
{
    DEFINE_STATIC_LOCAL(String, safeJavaScriptURL, ("javascript:void(0)"));

    bool didBlockScript = false;
    for (size_t i = 0; i < token.attributes().size(); ++i) {
        const HTMLToken::Attribute& attribute = token.attributes().at(i);
        bool isInlineEventHandler = isNameOfInlineEventHandler(attribute.m_name);
        bool valueContainsJavaScriptURL = isInlineEventHandler ? false : containsJavaScriptURL(attribute.m_value);
        if (!isInlineEventHandler && !valueContainsJavaScriptURL)
            continue;
        if (!isContainedInRequest(snippetForAttribute(token, attribute)))
            continue;
        token.eraseValueOfAttribute(i);
        if (valueContainsJavaScriptURL)
            token.appendToAttributeValue(i, safeJavaScriptURL);
        didBlockScript = true;
    }
    return didBlockScript;
}

bool XSSFilter::eraseAttributeIfInjected(HTMLToken& token, const QualifiedName& attributeName, const String& replacementValue)
{
    size_t indexOfAttribute;
    if (findAttributeWithName(token, attributeName, indexOfAttribute)) {
        const HTMLToken::Attribute& attribute = token.attributes().at(indexOfAttribute);
        if (isContainedInRequest(snippetForAttribute(token, attribute))) {
            if (attributeName == srcAttr && isSameOriginResource(String(attribute.m_value.data(), attribute.m_value.size())))
                return false;
            token.eraseValueOfAttribute(indexOfAttribute);
            if (!replacementValue.isEmpty())
                token.appendToAttributeValue(indexOfAttribute, replacementValue);
            return true;
        }
    }
    return false;
}

String XSSFilter::snippetForRange(const HTMLToken& token, int start, int end)
{
    // FIXME: There's an extra allocation here that we could save by
    //        passing the range to the parser.
    return m_parser->sourceForToken(token).substring(start, end - start);
}

String XSSFilter::snippetForAttribute(const HTMLToken& token, const HTMLToken::Attribute& attribute)
{
    // FIXME: We should grab one character before the name also.
    int start = attribute.m_nameRange.m_start - token.startIndex();
    // FIXME: We probably want to grab only the first few characters of the attribute value.
    int end = attribute.m_valueRange.m_end - token.startIndex();
    return snippetForRange(token, start, end);
}

bool XSSFilter::isContainedInRequest(const String& snippet)
{
    ASSERT(!snippet.isEmpty());
    String canonicalizedSnippet = canonicalize(snippet);
    ASSERT(!canonicalizedSnippet.isEmpty());
    if (m_decodedURL.find(canonicalizedSnippet, 0, false) != notFound)
        return true;
    if (m_decodedHTTPBodySuffixTree && !m_decodedHTTPBodySuffixTree->mightContain(canonicalizedSnippet))
        return false;
    return m_decodedHTTPBody.find(canonicalizedSnippet, 0, false) != notFound;
}

bool XSSFilter::isSameOriginResource(const String& url)
{
    // If the resource is loaded from the same URL as the enclosing page, it's
    // probably not an XSS attack, so we reduce false positives by allowing the
    // request. If the resource has a query string, we're more suspicious,
    // however, because that's pretty rare and the attacker might be able to
    // trick a server-side script into doing something dangerous with the query
    // string.
    KURL resourceURL(m_parser->document()->url(), url);
    return (m_parser->document()->url().host() == resourceURL.host() && resourceURL.query().isEmpty());
}

}
