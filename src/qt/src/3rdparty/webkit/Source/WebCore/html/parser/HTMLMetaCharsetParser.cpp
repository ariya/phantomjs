/*
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
#include "HTMLMetaCharsetParser.h"

#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "HTMLTokenizer.h"
#include "PlatformString.h"
#include "TextCodec.h"
#include "TextEncodingRegistry.h"

using namespace WTF;

namespace WebCore {

using namespace HTMLNames;

HTMLMetaCharsetParser::HTMLMetaCharsetParser()
    : m_tokenizer(HTMLTokenizer::create(false)) // No pre-HTML5 parser quirks.
    , m_assumedCodec(newTextCodec(Latin1Encoding()))
    , m_inHeadSection(true)
    , m_doneChecking(false)
{
}

HTMLMetaCharsetParser::~HTMLMetaCharsetParser()
{
}

static const char charsetString[] = "charset";
static const size_t charsetLength = sizeof("charset") - 1;

String HTMLMetaCharsetParser::extractCharset(const String& value)
{
    size_t pos = 0;
    unsigned length = value.length();

    while (pos < length) {
        pos = value.find(charsetString, pos, false);
        if (pos == notFound)
            break;

        pos += charsetLength;

        // Skip whitespace.
        while (pos < length && value[pos] <= ' ')
            ++pos;

        if (value[pos] != '=')
            continue;

        ++pos;

        while (pos < length && value[pos] <= ' ')
            ++pos;

        char quoteMark = 0;
        if (pos < length && (value[pos] == '"' || value[pos] == '\'')) {
            quoteMark = static_cast<char>(value[pos++]);
            ASSERT(!(quoteMark & 0x80));
        }
            
        if (pos == length)
            break;

        unsigned end = pos;
        while (end < length && ((quoteMark && value[end] != quoteMark) || (!quoteMark && value[end] > ' ' && value[end] != '"' && value[end] != '\'' && value[end] != ';')))
            ++end;

        if (quoteMark && (end == length))
            break; // Close quote not found.

        return value.substring(pos, end - pos);
    }

    return "";
}

bool HTMLMetaCharsetParser::processMeta()
{
    const HTMLToken::AttributeList& tokenAttributes = m_token.attributes();
    AttributeList attributes;
    for (HTMLToken::AttributeList::const_iterator iter = tokenAttributes.begin(); iter != tokenAttributes.end(); ++iter) {
        String attributeName(iter->m_name.data(), iter->m_name.size());
        String attributeValue(iter->m_value.data(), iter->m_value.size());
        attributes.append(make_pair(attributeName, attributeValue));
    }

    m_encoding = encodingFromMetaAttributes(attributes);
    return m_encoding.isValid();
}

TextEncoding HTMLMetaCharsetParser::encodingFromMetaAttributes(const AttributeList& attributes)
{
    bool gotPragma = false;
    Mode mode = None;
    String charset;

    for (AttributeList::const_iterator iter = attributes.begin(); iter != attributes.end(); ++iter) {
        const AtomicString& attributeName = iter->first;
        const String& attributeValue = iter->second;

        if (attributeName == http_equivAttr) {
            if (equalIgnoringCase(attributeValue, "content-type"))
                gotPragma = true;
        } else if (charset.isEmpty()) {
            if (attributeName == charsetAttr) {
                charset = attributeValue;
                mode = Charset;
            } else if (attributeName == contentAttr) {
                charset = extractCharset(attributeValue);
                if (charset.length())
                    mode = Pragma;
            }
        }
    }

    if (mode == Charset || (mode == Pragma && gotPragma))
        return TextEncoding(stripLeadingAndTrailingHTMLSpaces(charset));

    return TextEncoding();
}

static const int bytesToCheckUnconditionally = 1024; // That many input bytes will be checked for meta charset even if <head> section is over.

bool HTMLMetaCharsetParser::checkForMetaCharset(const char* data, size_t length)
{
    if (m_doneChecking)
        return true;

    ASSERT(!m_encoding.isValid());

    // We still don't have an encoding, and are in the head.
    // The following tags are allowed in <head>:
    // SCRIPT|STYLE|META|LINK|OBJECT|TITLE|BASE

    // We stop scanning when a tag that is not permitted in <head>
    // is seen, rather when </head> is seen, because that more closely
    // matches behavior in other browsers; more details in
    // <http://bugs.webkit.org/show_bug.cgi?id=3590>.

    // Additionally, we ignore things that looks like tags in <title>, <script>
    // and <noscript>; see <http://bugs.webkit.org/show_bug.cgi?id=4560>,
    // <http://bugs.webkit.org/show_bug.cgi?id=12165> and
    // <http://bugs.webkit.org/show_bug.cgi?id=12389>.

    // Since many sites have charset declarations after <body> or other tags
    // that are disallowed in <head>, we don't bail out until we've checked at
    // least bytesToCheckUnconditionally bytes of input.

    m_input.append(SegmentedString(m_assumedCodec->decode(data, length)));

    while (m_tokenizer->nextToken(m_input, m_token)) {
        bool end = m_token.type() == HTMLToken::EndTag;
        if (end || m_token.type() == HTMLToken::StartTag) {
            AtomicString tagName(m_token.name().data(), m_token.name().size());
            if (!end) {
                m_tokenizer->updateStateFor(tagName, 0);
                if (tagName == metaTag && processMeta()) {
                    m_doneChecking = true;
                    return true;
                }
            }

            if (tagName != scriptTag && tagName != noscriptTag
                && tagName != styleTag && tagName != linkTag
                && tagName != metaTag && tagName != objectTag
                && tagName != titleTag && tagName != baseTag
                && (end || tagName != htmlTag) && (end || tagName != headTag)) {
                m_inHeadSection = false;
            }
        }

        if (!m_inHeadSection && m_input.numberOfCharactersConsumed() >= bytesToCheckUnconditionally) {
            m_doneChecking = true;
            return true;
        }

        m_token.clear();
    }

    return false;
}

}
