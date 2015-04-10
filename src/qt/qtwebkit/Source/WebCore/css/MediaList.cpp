/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2006, 2010, 2012 Apple Inc. All rights reserved.
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
#include "MediaList.h"

#include "CSSImportRule.h"
#include "CSSParser.h"
#include "CSSStyleSheet.h"
#include "Console.h"
#include "DOMWindow.h"
#include "Document.h"
#include "ExceptionCode.h"
#include "MediaFeatureNames.h"
#include "MediaQuery.h"
#include "MediaQueryExp.h"
#include "ScriptableDocumentParser.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

/* MediaList is used to store 3 types of media related entities which mean the same:
 * Media Queries, Media Types and Media Descriptors.
 * Currently MediaList always tries to parse media queries and if parsing fails,
 * tries to fallback to Media Descriptors if m_fallbackToDescriptor flag is set.
 * Slight problem with syntax error handling:
 * CSS 2.1 Spec (http://www.w3.org/TR/CSS21/media.html)
 * specifies that failing media type parsing is a syntax error
 * CSS 3 Media Queries Spec (http://www.w3.org/TR/css3-mediaqueries/)
 * specifies that failing media query is a syntax error
 * HTML 4.01 spec (http://www.w3.org/TR/REC-html40/present/styles.html#adef-media)
 * specifies that Media Descriptors should be parsed with forward-compatible syntax
 * DOM Level 2 Style Sheet spec (http://www.w3.org/TR/DOM-Level-2-Style/)
 * talks about MediaList.mediaText and refers
 *   -  to Media Descriptors of HTML 4.0 in context of StyleSheet
 *   -  to Media Types of CSS 2.0 in context of CSSMediaRule and CSSImportRule
 *
 * These facts create situation where same (illegal) media specification may result in
 * different parses depending on whether it is media attr of style element or part of
 * css @media rule.
 * <style media="screen and resolution > 40dpi"> ..</style> will be enabled on screen devices where as
 * @media screen and resolution > 40dpi {..} will not.
 * This gets more counter-intuitive in JavaScript:
 * document.styleSheets[0].media.mediaText = "screen and resolution > 40dpi" will be ok and
 * enabled, while
 * document.styleSheets[0].cssRules[0].media.mediaText = "screen and resolution > 40dpi" will
 * throw SYNTAX_ERR exception.
 */
    
MediaQuerySet::MediaQuerySet()
    : m_fallbackToDescriptor(false)
    , m_lastLine(0)
{
}

MediaQuerySet::MediaQuerySet(const String& mediaString, bool fallbackToDescriptor)
    : m_fallbackToDescriptor(fallbackToDescriptor)
    , m_lastLine(0)
{
    bool success = parse(mediaString);
    // FIXME: parsing can fail. The problem with failing constructor is that
    // we would need additional flag saying MediaList is not valid
    // Parse can fail only when fallbackToDescriptor == false, i.e when HTML4 media descriptor
    // forward-compatible syntax is not in use.
    // DOMImplementationCSS seems to mandate that media descriptors are used
    // for both html and svg, even though svg:style doesn't use media descriptors
    // Currently the only places where parsing can fail are
    // creating <svg:style>, creating css media / import rules from js
    
    // FIXME: This doesn't make much sense.
    if (!success)
        parse("invalid");
}

MediaQuerySet::MediaQuerySet(const MediaQuerySet& o)
    : RefCounted<MediaQuerySet>()
    , m_fallbackToDescriptor(o.m_fallbackToDescriptor)
    , m_lastLine(o.m_lastLine)
    , m_queries(o.m_queries.size())
{
    for (unsigned i = 0; i < m_queries.size(); ++i)
        m_queries[i] = o.m_queries[i]->copy();
}

MediaQuerySet::~MediaQuerySet()
{
}

static String parseMediaDescriptor(const String& string)
{
    // http://www.w3.org/TR/REC-html40/types.html#type-media-descriptors
    // "Each entry is truncated just before the first character that isn't a
    // US ASCII letter [a-zA-Z] (ISO 10646 hex 41-5a, 61-7a), digit [0-9] (hex 30-39),
    // or hyphen (hex 2d)."
    unsigned length = string.length();
    unsigned i = 0;
    for (; i < length; ++i) {
        unsigned short c = string[i];
        if (! ((c >= 'a' && c <= 'z')
               || (c >= 'A' && c <= 'Z')
               || (c >= '1' && c <= '9')
               || (c == '-')))
            break;
    }
    return string.left(i);
}

bool MediaQuerySet::parse(const String& mediaString)
{
    CSSParser parser(CSSStrictMode);
    
    Vector<OwnPtr<MediaQuery> > result;
    Vector<String> list;
    mediaString.split(',', list);
    for (unsigned i = 0; i < list.size(); ++i) {
        String medium = list[i].stripWhiteSpace();
        if (medium.isEmpty()) {
            if (!m_fallbackToDescriptor)
                return false;
            continue;
        }
        OwnPtr<MediaQuery> mediaQuery = parser.parseMediaQuery(medium);
        if (!mediaQuery) {
            if (!m_fallbackToDescriptor)
                return false;
            String mediaDescriptor = parseMediaDescriptor(medium);
            if (mediaDescriptor.isNull())
                continue;
            mediaQuery = adoptPtr(new MediaQuery(MediaQuery::None, mediaDescriptor, nullptr));
        }
        result.append(mediaQuery.release());
    }
    // ",,,," falls straight through, but is not valid unless fallback
    if (!m_fallbackToDescriptor && list.isEmpty()) {
        String strippedMediaString = mediaString.stripWhiteSpace();
        if (!strippedMediaString.isEmpty())
            return false;
    }
    m_queries.swap(result);
    return true;
}

bool MediaQuerySet::add(const String& queryString)
{
    CSSParser parser(CSSStrictMode);

    OwnPtr<MediaQuery> parsedQuery = parser.parseMediaQuery(queryString);
    if (!parsedQuery && m_fallbackToDescriptor) {
        String medium = parseMediaDescriptor(queryString);
        if (!medium.isNull())
            parsedQuery = adoptPtr(new MediaQuery(MediaQuery::None, medium, nullptr));
    }
    if (!parsedQuery)
        return false;

    m_queries.append(parsedQuery.release());
    return true;
}

bool MediaQuerySet::remove(const String& queryStringToRemove)
{
    CSSParser parser(CSSStrictMode);

    OwnPtr<MediaQuery> parsedQuery = parser.parseMediaQuery(queryStringToRemove);
    if (!parsedQuery && m_fallbackToDescriptor) {
        String medium = parseMediaDescriptor(queryStringToRemove);
        if (!medium.isNull())
            parsedQuery = adoptPtr(new MediaQuery(MediaQuery::None, medium, nullptr));
    }
    if (!parsedQuery)
        return false;
    
    for (size_t i = 0; i < m_queries.size(); ++i) {
        MediaQuery* query = m_queries[i].get();
        if (*query == *parsedQuery) {
            m_queries.remove(i);
            return true;
        }
    }
    return false;
}

void MediaQuerySet::addMediaQuery(PassOwnPtr<MediaQuery> mediaQuery)
{
    m_queries.append(mediaQuery);
}

String MediaQuerySet::mediaText() const
{
    StringBuilder text;
    
    bool first = true;
    for (size_t i = 0; i < m_queries.size(); ++i) {
        if (!first)
            text.appendLiteral(", ");
        else
            first = false;
        text.append(m_queries[i]->cssText());
    }
    return text.toString();
}

MediaList::MediaList(MediaQuerySet* mediaQueries, CSSStyleSheet* parentSheet)
    : m_mediaQueries(mediaQueries)
    , m_parentStyleSheet(parentSheet)
    , m_parentRule(0)
{
}

MediaList::MediaList(MediaQuerySet* mediaQueries, CSSRule* parentRule)
    : m_mediaQueries(mediaQueries)
    , m_parentStyleSheet(0)
    , m_parentRule(parentRule)
{
}

MediaList::~MediaList()
{
}

void MediaList::setMediaText(const String& value, ExceptionCode& ec)
{
    CSSStyleSheet::RuleMutationScope mutationScope(m_parentRule);

    bool success = m_mediaQueries->parse(value);
    if (!success) {
        ec = SYNTAX_ERR;
        return;
    }
    if (m_parentStyleSheet)
        m_parentStyleSheet->didMutate();
}

String MediaList::item(unsigned index) const
{
    const Vector<OwnPtr<MediaQuery> >& queries = m_mediaQueries->queryVector();
    if (index < queries.size())
        return queries[index]->cssText();
    return String();
}

void MediaList::deleteMedium(const String& medium, ExceptionCode& ec)
{
    CSSStyleSheet::RuleMutationScope mutationScope(m_parentRule);

    bool success = m_mediaQueries->remove(medium);
    if (!success) {
        ec = NOT_FOUND_ERR;
        return;
    }
    if (m_parentStyleSheet)
        m_parentStyleSheet->didMutate();
}

void MediaList::appendMedium(const String& medium, ExceptionCode& ec)
{
    CSSStyleSheet::RuleMutationScope mutationScope(m_parentRule);

    bool success = m_mediaQueries->add(medium);
    if (!success) {
        // FIXME: Should this really be INVALID_CHARACTER_ERR?
        ec = INVALID_CHARACTER_ERR;
        return;
    }
    if (m_parentStyleSheet)
        m_parentStyleSheet->didMutate();
}

void MediaList::reattach(MediaQuerySet* mediaQueries)
{
    ASSERT(mediaQueries);
    m_mediaQueries = mediaQueries;
}

#if ENABLE(RESOLUTION_MEDIA_QUERY)
static void addResolutionWarningMessageToConsole(Document* document, const String& serializedExpression, const CSSPrimitiveValue* value)
{
    ASSERT(document);
    ASSERT(value);

    DEFINE_STATIC_LOCAL(String, mediaQueryMessage, (ASCIILiteral("Consider using 'dppx' units instead of '%replacementUnits%', as in CSS '%replacementUnits%' means dots-per-CSS-%lengthUnit%, not dots-per-physical-%lengthUnit%, so does not correspond to the actual '%replacementUnits%' of a screen. In media query expression: ")));
    DEFINE_STATIC_LOCAL(String, mediaValueDPI, (ASCIILiteral("dpi")));
    DEFINE_STATIC_LOCAL(String, mediaValueDPCM, (ASCIILiteral("dpcm")));
    DEFINE_STATIC_LOCAL(String, lengthUnitInch, (ASCIILiteral("inch")));
    DEFINE_STATIC_LOCAL(String, lengthUnitCentimeter, (ASCIILiteral("centimeter")));

    String message;
    if (value->isDotsPerInch())
        message = String(mediaQueryMessage).replace("%replacementUnits%", mediaValueDPI).replace("%lengthUnit%", lengthUnitInch);
    else if (value->isDotsPerCentimeter())
        message = String(mediaQueryMessage).replace("%replacementUnits%", mediaValueDPCM).replace("%lengthUnit%", lengthUnitCentimeter);
    else
        ASSERT_NOT_REACHED();

    message.append(serializedExpression);

    document->addConsoleMessage(CSSMessageSource, DebugMessageLevel, message);
}

void reportMediaQueryWarningIfNeeded(Document* document, const MediaQuerySet* mediaQuerySet)
{
    if (!mediaQuerySet || !document)
        return;

    const Vector<OwnPtr<MediaQuery> >& mediaQueries = mediaQuerySet->queryVector();
    const size_t queryCount = mediaQueries.size();

    if (!queryCount)
        return;

    for (size_t i = 0; i < queryCount; ++i) {
        const MediaQuery* query = mediaQueries[i].get();
        String mediaType = query->mediaType();
        if (!query->ignored() && !equalIgnoringCase(mediaType, "print")) {
            const Vector<OwnPtr<MediaQueryExp> >* exps = query->expressions();
            for (size_t j = 0; j < exps->size(); ++j) {
                const MediaQueryExp* exp = exps->at(j).get();
                if (exp->mediaFeature() == MediaFeatureNames::resolutionMediaFeature || exp->mediaFeature() == MediaFeatureNames::max_resolutionMediaFeature || exp->mediaFeature() == MediaFeatureNames::min_resolutionMediaFeature) {
                    CSSValue* cssValue =  exp->value();
                    if (cssValue && cssValue->isPrimitiveValue()) {
                        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(cssValue);
                        if (primitiveValue->isDotsPerInch() || primitiveValue->isDotsPerCentimeter())
                            addResolutionWarningMessageToConsole(document, mediaQuerySet->mediaText(), primitiveValue);
                    }
                }
            }
        }
    }
}
#endif

}
