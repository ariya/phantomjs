/*
 * Copyright (C) 2011 Adam Barth. All Rights Reserved.
 * Copyright (C) 2011 Daniel Bates (dbates@intudata.com).
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
#include "XSSAuditor.h"

#include "ContentSecurityPolicy.h"
#include "DecodeEscapeSequences.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "FormData.h"
#include "Frame.h"
#include "HTMLDocumentParser.h"
#include "HTMLNames.h"
#include "HTMLParamElement.h"
#include "HTMLParserIdioms.h"
#include "InspectorValues.h"
#include "KURL.h"
#include "Settings.h"
#include "TextEncoding.h"
#include "TextResourceDecoder.h"
#include "XLinkNames.h"
#include "XSSAuditorDelegate.h"

#if ENABLE(SVG)
#include "SVGNames.h"
#endif

#include <wtf/MainThread.h>

namespace WebCore {

using namespace HTMLNames;

static bool isNonCanonicalCharacter(UChar c)
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

static String canonicalize(const String& string)
{
    return string.removeCharacters(&isNonCanonicalCharacter);
}

static bool isRequiredForInjection(UChar c)
{
    return (c == '\'' || c == '"' || c == '<' || c == '>');
}

static bool isTerminatingCharacter(UChar c)
{
    return (c == '&' || c == '/' || c == '"' || c == '\'' || c == '<' || c == '>' || c == ',');
}

static bool isHTMLQuote(UChar c)
{
    return (c == '"' || c == '\'');
}

static bool isJSNewline(UChar c)
{
    // Per ecma-262 section 7.3 Line Terminators.
    return (c == '\n' || c == '\r' || c == 0x2028 || c == 0x2029);
}

static bool startsHTMLCommentAt(const String& string, size_t start)
{
    return (start + 3 < string.length() && string[start] == '<' && string[start+1] == '!' && string[start+2] == '-' && string[start+3] == '-');
}

static bool startsSingleLineCommentAt(const String& string, size_t start)
{
    return (start + 1 < string.length() && string[start] == '/' && string[start+1] == '/');
}

static bool startsMultiLineCommentAt(const String& string, size_t start)
{
    return (start + 1 < string.length() && string[start] == '/' && string[start+1] == '*');
}

// If other files need this, we should move this to HTMLParserIdioms.h
template<size_t inlineCapacity>
bool threadSafeMatch(const Vector<UChar, inlineCapacity>& vector, const QualifiedName& qname)
{
    return equalIgnoringNullity(vector, qname.localName().impl());
}

static bool hasName(const HTMLToken& token, const QualifiedName& name)
{
    return threadSafeMatch(token.name(), name);
}

static bool findAttributeWithName(const HTMLToken& token, const QualifiedName& name, size_t& indexOfMatchingAttribute)
{
    // Notice that we're careful not to ref the StringImpl here because we might be on a background thread.
    const String& attrName = name.namespaceURI() == XLinkNames::xlinkNamespaceURI ? "xlink:" + name.localName().string() : name.localName().string();

    for (size_t i = 0; i < token.attributes().size(); ++i) {
        if (equalIgnoringNullity(token.attributes().at(i).name, attrName)) {
            indexOfMatchingAttribute = i;
            return true;
        }
    }
    return false;
}

static bool isNameOfInlineEventHandler(const Vector<UChar, 32>& name)
{
    const size_t lengthOfShortestInlineEventHandlerName = 5; // To wit: oncut.
    if (name.size() < lengthOfShortestInlineEventHandlerName)
        return false;
    return name[0] == 'o' && name[1] == 'n';
}

static bool isDangerousHTTPEquiv(const String& value)
{
    String equiv = value.stripWhiteSpace();
    return equalIgnoringCase(equiv, "refresh") || equalIgnoringCase(equiv, "set-cookie");
}

static inline String decode16BitUnicodeEscapeSequences(const String& string)
{
    // Note, the encoding is ignored since each %u-escape sequence represents a UTF-16 code unit.
    return decodeEscapeSequences<Unicode16BitEscapeSequence>(string, UTF8Encoding());
}

static inline String decodeStandardURLEscapeSequences(const String& string, const TextEncoding& encoding)
{
    // We use decodeEscapeSequences() instead of decodeURLEscapeSequences() (declared in KURL.h) to
    // avoid platform-specific URL decoding differences (e.g. KURLGoogle).
    return decodeEscapeSequences<URLEscapeSequence>(string, encoding);
}

static String fullyDecodeString(const String& string, const TextEncoding& encoding)
{
    size_t oldWorkingStringLength;
    String workingString = string;
    do {
        oldWorkingStringLength = workingString.length();
        workingString = decode16BitUnicodeEscapeSequences(decodeStandardURLEscapeSequences(workingString, encoding));
    } while (workingString.length() < oldWorkingStringLength);
    workingString.replace('+', ' ');
    workingString = canonicalize(workingString);
    return workingString;
}

static ContentSecurityPolicy::ReflectedXSSDisposition combineXSSProtectionHeaderAndCSP(ContentSecurityPolicy::ReflectedXSSDisposition xssProtection, ContentSecurityPolicy::ReflectedXSSDisposition reflectedXSS)
{
    ContentSecurityPolicy::ReflectedXSSDisposition result = std::max(xssProtection, reflectedXSS);

    if (result == ContentSecurityPolicy::ReflectedXSSInvalid || result == ContentSecurityPolicy::FilterReflectedXSS || result == ContentSecurityPolicy::ReflectedXSSUnset)
        return ContentSecurityPolicy::FilterReflectedXSS;

    return result;
}

static bool isSemicolonSeparatedAttribute(const HTMLToken::Attribute& attribute)
{
#if ENABLE(SVG)
    return threadSafeMatch(attribute.name, SVGNames::valuesAttr);
#else
    return false;
#endif
}

static bool semicolonSeparatedValueContainsJavaScriptURL(const String& value)
{
    Vector<String> valueList;
    value.split(';', valueList);
    for (size_t i = 0; i < valueList.size(); ++i) {
        if (protocolIsJavaScript(valueList[i]))
            return true;
    }
    return false;
}

XSSAuditor::XSSAuditor()
    : m_isEnabled(false)
    , m_xssProtection(ContentSecurityPolicy::FilterReflectedXSS)
    , m_didSendValidCSPHeader(false)
    , m_didSendValidXSSProtectionHeader(false)
    , m_state(Uninitialized)
    , m_scriptTagNestingLevel(0)
    , m_encoding(UTF8Encoding())
{
    // Although tempting to call init() at this point, the various objects
    // we want to reference might not all have been constructed yet.
}

void XSSAuditor::initForFragment()
{
    ASSERT(isMainThread());
    ASSERT(m_state == Uninitialized);
    m_state = Initialized;
    // When parsing a fragment, we don't enable the XSS auditor because it's
    // too much overhead.
    ASSERT(!m_isEnabled);
}

void XSSAuditor::init(Document* document, XSSAuditorDelegate* auditorDelegate)
{
    const size_t miniumLengthForSuffixTree = 512; // FIXME: Tune this parameter.
    const int suffixTreeDepth = 5;

    ASSERT(isMainThread());
    if (m_state == Initialized)
        return;
    ASSERT(m_state == Uninitialized);
    m_state = Initialized;

    if (Frame* frame = document->frame())
        if (Settings* settings = frame->settings())
            m_isEnabled = settings->xssAuditorEnabled();

    if (!m_isEnabled)
        return;

    m_documentURL = document->url().copy();

    // In theory, the Document could have detached from the Frame after the
    // XSSAuditor was constructed.
    if (!document->frame()) {
        m_isEnabled = false;
        return;
    }

    if (m_documentURL.isEmpty()) {
        // The URL can be empty when opening a new browser window or calling window.open("").
        m_isEnabled = false;
        return;
    }

    if (m_documentURL.protocolIsData()) {
        m_isEnabled = false;
        return;
    }

    if (document->decoder())
        m_encoding = document->decoder()->encoding();

    m_decodedURL = fullyDecodeString(m_documentURL.string(), m_encoding);
    if (m_decodedURL.find(isRequiredForInjection) == notFound)
        m_decodedURL = String();

    String httpBodyAsString;
    if (DocumentLoader* documentLoader = document->frame()->loader()->documentLoader()) {
        DEFINE_STATIC_LOCAL(String, XSSProtectionHeader, (ASCIILiteral("X-XSS-Protection")));
        String headerValue = documentLoader->response().httpHeaderField(XSSProtectionHeader);
        String errorDetails;
        unsigned errorPosition = 0;
        String reportURL;
        KURL xssProtectionReportURL;

        // Process the X-XSS-Protection header, then mix in the CSP header's value.
        ContentSecurityPolicy::ReflectedXSSDisposition xssProtectionHeader = parseXSSProtectionHeader(headerValue, errorDetails, errorPosition, reportURL);
        m_didSendValidXSSProtectionHeader = xssProtectionHeader != ContentSecurityPolicy::ReflectedXSSUnset && xssProtectionHeader != ContentSecurityPolicy::ReflectedXSSInvalid;
        if ((xssProtectionHeader == ContentSecurityPolicy::FilterReflectedXSS || xssProtectionHeader == ContentSecurityPolicy::BlockReflectedXSS) && !reportURL.isEmpty()) {
            xssProtectionReportURL = document->completeURL(reportURL);
            if (MixedContentChecker::isMixedContent(document->securityOrigin(), xssProtectionReportURL)) {
                errorDetails = "insecure reporting URL for secure page";
                xssProtectionHeader = ContentSecurityPolicy::ReflectedXSSInvalid;
                xssProtectionReportURL = KURL();
            }
        }
        if (xssProtectionHeader == ContentSecurityPolicy::ReflectedXSSInvalid)
            document->addConsoleMessage(SecurityMessageSource, ErrorMessageLevel, "Error parsing header X-XSS-Protection: " + headerValue + ": "  + errorDetails + " at character position " + String::format("%u", errorPosition) + ". The default protections will be applied.");

        ContentSecurityPolicy::ReflectedXSSDisposition cspHeader = document->contentSecurityPolicy()->reflectedXSSDisposition();
        m_didSendValidCSPHeader = cspHeader != ContentSecurityPolicy::ReflectedXSSUnset && cspHeader != ContentSecurityPolicy::ReflectedXSSInvalid;

        m_xssProtection = combineXSSProtectionHeaderAndCSP(xssProtectionHeader, cspHeader);
        // FIXME: Combine the two report URLs in some reasonable way.
        if (auditorDelegate)
            auditorDelegate->setReportURL(xssProtectionReportURL.copy());
        FormData* httpBody = documentLoader->originalRequest().httpBody();
        if (httpBody && !httpBody->isEmpty()) {
            httpBodyAsString = httpBody->flattenToString();
            if (!httpBodyAsString.isEmpty()) {
                m_decodedHTTPBody = fullyDecodeString(httpBodyAsString, m_encoding);
                if (m_decodedHTTPBody.find(isRequiredForInjection) == notFound)
                    m_decodedHTTPBody = String();
                if (m_decodedHTTPBody.length() >= miniumLengthForSuffixTree)
                    m_decodedHTTPBodySuffixTree = adoptPtr(new SuffixTree<ASCIICodebook>(m_decodedHTTPBody, suffixTreeDepth));
            }
        }
    }

    if (m_decodedURL.isEmpty() && m_decodedHTTPBody.isEmpty()) {
        m_isEnabled = false;
        return;
    }
}

PassOwnPtr<XSSInfo> XSSAuditor::filterToken(const FilterTokenRequest& request)
{
    ASSERT(m_state == Initialized);
    if (!m_isEnabled || m_xssProtection == ContentSecurityPolicy::AllowReflectedXSS)
        return nullptr;

    bool didBlockScript = false;
    if (request.token.type() == HTMLToken::StartTag)
        didBlockScript = filterStartToken(request);
    else if (m_scriptTagNestingLevel) {
        if (request.token.type() == HTMLToken::Character)
            didBlockScript = filterCharacterToken(request);
        else if (request.token.type() == HTMLToken::EndTag)
            filterEndToken(request);
    }

    if (didBlockScript) {
        bool didBlockEntirePage = (m_xssProtection == ContentSecurityPolicy::BlockReflectedXSS);
        OwnPtr<XSSInfo> xssInfo = XSSInfo::create(didBlockEntirePage, m_didSendValidXSSProtectionHeader, m_didSendValidCSPHeader);
        return xssInfo.release();
    }
    return nullptr;
}

bool XSSAuditor::filterStartToken(const FilterTokenRequest& request)
{
    bool didBlockScript = eraseDangerousAttributesIfInjected(request);

    if (hasName(request.token, scriptTag)) {
        didBlockScript |= filterScriptToken(request);
        ASSERT(request.shouldAllowCDATA || !m_scriptTagNestingLevel);
        m_scriptTagNestingLevel++;
    } else if (hasName(request.token, objectTag))
        didBlockScript |= filterObjectToken(request);
    else if (hasName(request.token, paramTag))
        didBlockScript |= filterParamToken(request);
    else if (hasName(request.token, embedTag))
        didBlockScript |= filterEmbedToken(request);
    else if (hasName(request.token, appletTag))
        didBlockScript |= filterAppletToken(request);
    else if (hasName(request.token, iframeTag))
        didBlockScript |= filterIframeToken(request);
    else if (hasName(request.token, metaTag))
        didBlockScript |= filterMetaToken(request);
    else if (hasName(request.token, baseTag))
        didBlockScript |= filterBaseToken(request);
    else if (hasName(request.token, formTag))
        didBlockScript |= filterFormToken(request);
    else if (hasName(request.token, inputTag))
        didBlockScript |= filterInputToken(request);
    else if (hasName(request.token, buttonTag))
        didBlockScript |= filterButtonToken(request);

    return didBlockScript;
}

void XSSAuditor::filterEndToken(const FilterTokenRequest& request)
{
    ASSERT(m_scriptTagNestingLevel);
    if (hasName(request.token, scriptTag)) {
        m_scriptTagNestingLevel--;
        ASSERT(request.shouldAllowCDATA || !m_scriptTagNestingLevel);
    }
}

bool XSSAuditor::filterCharacterToken(const FilterTokenRequest& request)
{
    ASSERT(m_scriptTagNestingLevel);
    if (isContainedInRequest(m_cachedDecodedSnippet) && isContainedInRequest(decodedSnippetForJavaScript(request))) {
        request.token.eraseCharacters();
        request.token.appendToCharacter(' '); // Technically, character tokens can't be empty.
        return true;
    }
    return false;
}

bool XSSAuditor::filterScriptToken(const FilterTokenRequest& request)
{
    ASSERT(request.token.type() == HTMLToken::StartTag);
    ASSERT(hasName(request.token, scriptTag));

    m_cachedDecodedSnippet = decodedSnippetForName(request);

    bool didBlockScript = false;
    if (isContainedInRequest(decodedSnippetForName(request))) {
        didBlockScript |= eraseAttributeIfInjected(request, srcAttr, blankURL().string(), SrcLikeAttribute);
        didBlockScript |= eraseAttributeIfInjected(request, XLinkNames::hrefAttr, blankURL().string(), SrcLikeAttribute);
    }

    return didBlockScript;
}

bool XSSAuditor::filterObjectToken(const FilterTokenRequest& request)
{
    ASSERT(request.token.type() == HTMLToken::StartTag);
    ASSERT(hasName(request.token, objectTag));

    bool didBlockScript = false;
    if (isContainedInRequest(decodedSnippetForName(request))) {
        didBlockScript |= eraseAttributeIfInjected(request, dataAttr, blankURL().string(), SrcLikeAttribute);
        didBlockScript |= eraseAttributeIfInjected(request, typeAttr);
        didBlockScript |= eraseAttributeIfInjected(request, classidAttr);
    }
    return didBlockScript;
}

bool XSSAuditor::filterParamToken(const FilterTokenRequest& request)
{
    ASSERT(request.token.type() == HTMLToken::StartTag);
    ASSERT(hasName(request.token, paramTag));

    size_t indexOfNameAttribute;
    if (!findAttributeWithName(request.token, nameAttr, indexOfNameAttribute))
        return false;

    const HTMLToken::Attribute& nameAttribute = request.token.attributes().at(indexOfNameAttribute);
    if (!HTMLParamElement::isURLParameter(String(nameAttribute.value)))
        return false;

    return eraseAttributeIfInjected(request, valueAttr, blankURL().string(), SrcLikeAttribute);
}

bool XSSAuditor::filterEmbedToken(const FilterTokenRequest& request)
{
    ASSERT(request.token.type() == HTMLToken::StartTag);
    ASSERT(hasName(request.token, embedTag));

    bool didBlockScript = false;
    if (isContainedInRequest(decodedSnippetForName(request))) {
        didBlockScript |= eraseAttributeIfInjected(request, codeAttr, String(), SrcLikeAttribute);
        didBlockScript |= eraseAttributeIfInjected(request, srcAttr, blankURL().string(), SrcLikeAttribute);
        didBlockScript |= eraseAttributeIfInjected(request, typeAttr);
    }
    return didBlockScript;
}

bool XSSAuditor::filterAppletToken(const FilterTokenRequest& request)
{
    ASSERT(request.token.type() == HTMLToken::StartTag);
    ASSERT(hasName(request.token, appletTag));

    bool didBlockScript = false;
    if (isContainedInRequest(decodedSnippetForName(request))) {
        didBlockScript |= eraseAttributeIfInjected(request, codeAttr, String(), SrcLikeAttribute);
        didBlockScript |= eraseAttributeIfInjected(request, objectAttr);
    }
    return didBlockScript;
}

bool XSSAuditor::filterIframeToken(const FilterTokenRequest& request)
{
    ASSERT(request.token.type() == HTMLToken::StartTag);
    ASSERT(hasName(request.token, iframeTag));

    bool didBlockScript = false;
    if (isContainedInRequest(decodedSnippetForName(request))) {
        didBlockScript |= eraseAttributeIfInjected(request, srcAttr, String(), SrcLikeAttribute);
        didBlockScript |= eraseAttributeIfInjected(request, srcdocAttr, String(), ScriptLikeAttribute);
    }
    return didBlockScript;
}

bool XSSAuditor::filterMetaToken(const FilterTokenRequest& request)
{
    ASSERT(request.token.type() == HTMLToken::StartTag);
    ASSERT(hasName(request.token, metaTag));

    return eraseAttributeIfInjected(request, http_equivAttr);
}

bool XSSAuditor::filterBaseToken(const FilterTokenRequest& request)
{
    ASSERT(request.token.type() == HTMLToken::StartTag);
    ASSERT(hasName(request.token, baseTag));

    return eraseAttributeIfInjected(request, hrefAttr);
}

bool XSSAuditor::filterFormToken(const FilterTokenRequest& request)
{
    ASSERT(request.token.type() == HTMLToken::StartTag);
    ASSERT(hasName(request.token, formTag));

    return eraseAttributeIfInjected(request, actionAttr, blankURL().string());
}

bool XSSAuditor::filterInputToken(const FilterTokenRequest& request)
{
    ASSERT(request.token.type() == HTMLToken::StartTag);
    ASSERT(hasName(request.token, inputTag));

    return eraseAttributeIfInjected(request, formactionAttr, blankURL().string(), SrcLikeAttribute);
}

bool XSSAuditor::filterButtonToken(const FilterTokenRequest& request)
{
    ASSERT(request.token.type() == HTMLToken::StartTag);
    ASSERT(hasName(request.token, buttonTag));

    return eraseAttributeIfInjected(request, formactionAttr, blankURL().string(), SrcLikeAttribute);
}

bool XSSAuditor::eraseDangerousAttributesIfInjected(const FilterTokenRequest& request)
{
    DEFINE_STATIC_LOCAL(String, safeJavaScriptURL, (ASCIILiteral("javascript:void(0)")));

    bool didBlockScript = false;
    for (size_t i = 0; i < request.token.attributes().size(); ++i) {
        const HTMLToken::Attribute& attribute = request.token.attributes().at(i);
        bool isInlineEventHandler = isNameOfInlineEventHandler(attribute.name);
        // FIXME: It would be better if we didn't create a new String for every attribute in the document.
        String strippedValue = stripLeadingAndTrailingHTMLSpaces(String(attribute.value));
        bool valueContainsJavaScriptURL = (!isInlineEventHandler && protocolIsJavaScript(strippedValue)) || (isSemicolonSeparatedAttribute(attribute) && semicolonSeparatedValueContainsJavaScriptURL(strippedValue));
        if (!isInlineEventHandler && !valueContainsJavaScriptURL)
            continue;
        if (!isContainedInRequest(decodedSnippetForAttribute(request, attribute, ScriptLikeAttribute)))
            continue;
        request.token.eraseValueOfAttribute(i);
        if (valueContainsJavaScriptURL)
            request.token.appendToAttributeValue(i, safeJavaScriptURL);
        didBlockScript = true;
    }
    return didBlockScript;
}

bool XSSAuditor::eraseAttributeIfInjected(const FilterTokenRequest& request, const QualifiedName& attributeName, const String& replacementValue, AttributeKind treatment)
{
    size_t indexOfAttribute = 0;
    if (findAttributeWithName(request.token, attributeName, indexOfAttribute)) {
        const HTMLToken::Attribute& attribute = request.token.attributes().at(indexOfAttribute);
        if (isContainedInRequest(decodedSnippetForAttribute(request, attribute, treatment))) {
            if (threadSafeMatch(attributeName, srcAttr) && isLikelySafeResource(String(attribute.value)))
                return false;
            if (threadSafeMatch(attributeName, http_equivAttr) && !isDangerousHTTPEquiv(String(attribute.value)))
                return false;
            request.token.eraseValueOfAttribute(indexOfAttribute);
            if (!replacementValue.isEmpty())
                request.token.appendToAttributeValue(indexOfAttribute, replacementValue);
            return true;
        }
    }
    return false;
}

String XSSAuditor::decodedSnippetForName(const FilterTokenRequest& request)
{
    // Grab a fixed number of characters equal to the length of the token's name plus one (to account for the "<").
    return fullyDecodeString(request.sourceTracker.sourceForToken(request.token), m_encoding).substring(0, request.token.name().size() + 1);
}

String XSSAuditor::decodedSnippetForAttribute(const FilterTokenRequest& request, const HTMLToken::Attribute& attribute, AttributeKind treatment)
{
    // The range doesn't inlcude the character which terminates the value. So,
    // for an input of |name="value"|, the snippet is |name="value|. For an
    // unquoted input of |name=value |, the snippet is |name=value|.
    // FIXME: We should grab one character before the name also.
    int start = attribute.nameRange.start - request.token.startIndex();
    int end = attribute.valueRange.end - request.token.startIndex();
    String decodedSnippet = fullyDecodeString(request.sourceTracker.sourceForToken(request.token).substring(start, end - start), m_encoding);
    decodedSnippet.truncate(kMaximumFragmentLengthTarget);
    if (treatment == SrcLikeAttribute) {
        int slashCount = 0;
        bool commaSeen = false;
        // In HTTP URLs, characters following the first ?, #, or third slash may come from 
        // the page itself and can be merely ignored by an attacker's server when a remote
        // script or script-like resource is requested. In DATA URLS, the payload starts at
        // the first comma, and the the first /*, //, or <!-- may introduce a comment. Characters
        // following this may come from the page itself and may be ignored when the script is
        // executed. For simplicity, we don't differentiate based on URL scheme, and stop at
        // the first # or ?, the third slash, or the first slash or < once a comma is seen.
        for (size_t currentLength = 0; currentLength < decodedSnippet.length(); ++currentLength) {
            UChar currentChar = decodedSnippet[currentLength];
            if (currentChar == '?'
                || currentChar == '#'
                || ((currentChar == '/' || currentChar == '\\') && (commaSeen || ++slashCount > 2))
                || (currentChar == '<' && commaSeen)) {
                decodedSnippet.truncate(currentLength);
                break;
            }
            if (currentChar == ',')
                commaSeen = true;
        }
    } else if (treatment == ScriptLikeAttribute) {
        // Beware of trailing characters which came from the page itself, not the 
        // injected vector. Excluding the terminating character covers common cases
        // where the page immediately ends the attribute, but doesn't cover more
        // complex cases where there is other page data following the injection. 
        // Generally, these won't parse as javascript, so the injected vector
        // typically excludes them from consideration via a single-line comment or
        // by enclosing them in a string literal terminated later by the page's own
        // closing punctuation. Since the snippet has not been parsed, the vector
        // may also try to introduce these via entities. As a result, we'd like to
        // stop before the first "//", the first <!--, the first entity, or the first
        // quote not immediately following the first equals sign (taking whitespace
        // into consideration). To keep things simpler, we don't try to distinguish
        // between entity-introducing amperands vs. other uses, nor do we bother to
        // check for a second slash for a comment, nor do we bother to check for
        // !-- following a less-than sign. We stop instead on any ampersand
        // slash, or less-than sign.
        size_t position = 0;
        if ((position = decodedSnippet.find("=")) != notFound
            && (position = decodedSnippet.find(isNotHTMLSpace, position + 1)) != notFound
            && (position = decodedSnippet.find(isTerminatingCharacter, isHTMLQuote(decodedSnippet[position]) ? position + 1 : position)) != notFound) {
            decodedSnippet.truncate(position);
        }
    }
    return decodedSnippet;
}

String XSSAuditor::decodedSnippetForJavaScript(const FilterTokenRequest& request)
{
    String string = request.sourceTracker.sourceForToken(request.token);
    size_t startPosition = 0;
    size_t endPosition = string.length();
    size_t foundPosition = notFound;

    // Skip over initial comments to find start of code.
    while (startPosition < endPosition) {
        while (startPosition < endPosition && isHTMLSpace(string[startPosition]))
            startPosition++;

        // Under SVG/XML rules, only HTML comment syntax matters and the parser returns
        // these as a separate comment tokens. Having consumed whitespace, we need not look
        // further for these.
        if (request.shouldAllowCDATA)
            break;

        // Under HTML rules, both the HTML and JS comment synatx matters, and the HTML
        // comment ends at the end of the line, not with -->.
        if (startsHTMLCommentAt(string, startPosition) || startsSingleLineCommentAt(string, startPosition)) {
            while (startPosition < endPosition && !isJSNewline(string[startPosition]))
                startPosition++;
        } else if (startsMultiLineCommentAt(string, startPosition)) {
            if (startPosition + 2 < endPosition && (foundPosition = string.find("*/", startPosition + 2)) != notFound)
                startPosition = foundPosition + 2;
            else
                startPosition = endPosition;
        } else
            break;
    }

    String result;
    while (startPosition < endPosition && !result.length()) {
        // Stop at next comment (using the same rules as above for SVG/XML vs HTML), when we 
        // encounter a comma, or when we  exceed the maximum length target. The comma rule
        // covers a common parameter concatenation case performed by some webservers.
        // After hitting the length target, we can only stop at a point where we know we are
        // not in the middle of a %-escape sequence. For the sake of simplicity, approximate
        // not stopping inside a (possibly multiply encoded) %-esacpe sequence by breaking on
        // whitespace only. We should have enough text in these cases to avoid false positives.
        for (foundPosition = startPosition; foundPosition < endPosition; foundPosition++) {
            if (!request.shouldAllowCDATA) {
                if (startsSingleLineCommentAt(string, foundPosition) || startsMultiLineCommentAt(string, foundPosition)) {
                    foundPosition += 2;
                    break;
                }
                if (startsHTMLCommentAt(string, foundPosition)) {
                    foundPosition += 4;
                    break;
                }
            }
            if (string[foundPosition] == ',' || (foundPosition > startPosition + kMaximumFragmentLengthTarget && isHTMLSpace(string[foundPosition]))) {
                break;
            }
        }

        result = fullyDecodeString(string.substring(startPosition, foundPosition - startPosition), m_encoding);
        startPosition = foundPosition + 1;
    }
    return result;
}

bool XSSAuditor::isContainedInRequest(const String& decodedSnippet)
{
    if (decodedSnippet.isEmpty())
        return false;
    if (m_decodedURL.find(decodedSnippet, 0, false) != notFound)
        return true;
    if (m_decodedHTTPBodySuffixTree && !m_decodedHTTPBodySuffixTree->mightContain(decodedSnippet))
        return false;
    return m_decodedHTTPBody.find(decodedSnippet, 0, false) != notFound;
}

bool XSSAuditor::isLikelySafeResource(const String& url)
{
    // Give empty URLs and about:blank a pass. Making a resourceURL from an
    // empty string below will likely later fail the "no query args test" as
    // it inherits the document's query args.
    if (url.isEmpty() || url == blankURL().string())
        return true;

    // If the resource is loaded from the same host as the enclosing page, it's
    // probably not an XSS attack, so we reduce false positives by allowing the
    // request, ignoring scheme and port considerations. If the resource has a
    // query string, we're more suspicious, however, because that's pretty rare
    // and the attacker might be able to trick a server-side script into doing
    // something dangerous with the query string.  
    if (m_documentURL.host().isEmpty())
        return false;

    KURL resourceURL(m_documentURL, url);
    return (m_documentURL.host() == resourceURL.host() && resourceURL.query().isEmpty());
}

bool XSSAuditor::isSafeToSendToAnotherThread() const
{
    return m_documentURL.isSafeToSendToAnotherThread()
        && m_decodedURL.isSafeToSendToAnotherThread()
        && m_decodedHTTPBody.isSafeToSendToAnotherThread()
        && m_cachedDecodedSnippet.isSafeToSendToAnotherThread();
}

} // namespace WebCore
