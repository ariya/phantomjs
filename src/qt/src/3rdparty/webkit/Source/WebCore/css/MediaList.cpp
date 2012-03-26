/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2006, 2010 Apple Inc. All rights reserved.
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
#include "ExceptionCode.h"
#include "MediaQuery.h"
#include "MediaQueryExp.h"

namespace WebCore {

/* MediaList is used to store 3 types of media related entities which mean the same:
 * Media Queries, Media Types and Media Descriptors.
 * Currently MediaList always tries to parse media queries and if parsing fails,
 * tries to fallback to Media Descriptors if m_fallback flag is set.
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

MediaList::MediaList(CSSStyleSheet* parentSheet, bool fallbackToDescriptor)
    : StyleBase(parentSheet)
    , m_fallback(fallbackToDescriptor)
{
}

MediaList::MediaList(CSSStyleSheet* parentSheet, const String& media, bool fallbackToDescriptor)
    : StyleBase(parentSheet)
    , m_fallback(fallbackToDescriptor)
{
    ExceptionCode ec = 0;
    setMediaText(media, ec);
    // FIXME: parsing can fail. The problem with failing constructor is that
    // we would need additional flag saying MediaList is not valid
    // Parse can fail only when fallbackToDescriptor == false, i.e when HTML4 media descriptor
    // forward-compatible syntax is not in use. 
    // DOMImplementationCSS seems to mandate that media descriptors are used
    // for both html and svg, even though svg:style doesn't use media descriptors
    // Currently the only places where parsing can fail are
    // creating <svg:style>, creating css media / import rules from js
    if (ec)
        setMediaText("invalid", ec);
}

MediaList::MediaList(CSSImportRule* parentRule, const String& media)
    : StyleBase(parentRule)
    , m_fallback(false)
{
    ExceptionCode ec = 0;
    setMediaText(media, ec);
    if (ec)
        setMediaText("invalid", ec);
}

MediaList::~MediaList()
{
    deleteAllValues(m_queries);
}

static String parseMediaDescriptor(const String& s)
{
    int len = s.length();

    // http://www.w3.org/TR/REC-html40/types.html#type-media-descriptors
    // "Each entry is truncated just before the first character that isn't a
    // US ASCII letter [a-zA-Z] (ISO 10646 hex 41-5a, 61-7a), digit [0-9] (hex 30-39),
    // or hyphen (hex 2d)."
    int i;
    unsigned short c;
    for (i = 0; i < len; ++i) {
        c = s[i];
        if (! ((c >= 'a' && c <= 'z')
            || (c >= 'A' && c <= 'Z')
            || (c >= '1' && c <= '9')
            || (c == '-')))
            break;
    }
    return s.left(i);
}

void MediaList::deleteMedium(const String& oldMedium, ExceptionCode& ec)
{
    RefPtr<MediaList> tempMediaList = MediaList::create();
    CSSParser p(true);

    MediaQuery* oldQuery = 0;
    OwnPtr<MediaQuery> createdQuery;

    if (p.parseMediaQuery(tempMediaList.get(), oldMedium)) {
        if (tempMediaList->m_queries.size() > 0)
            oldQuery = tempMediaList->m_queries[0];
    } else if (m_fallback) {
        String medium = parseMediaDescriptor(oldMedium);
        if (!medium.isNull()) {
            createdQuery = adoptPtr(new MediaQuery(MediaQuery::None, medium, nullptr));
            oldQuery = createdQuery.get();
        }
    }

    // DOM Style Sheets spec doesn't allow SYNTAX_ERR to be thrown in deleteMedium
    ec = NOT_FOUND_ERR;

    if (oldQuery) {
        for (size_t i = 0; i < m_queries.size(); ++i) {
            MediaQuery* a = m_queries[i];
            if (*a == *oldQuery) {
                m_queries.remove(i);
                delete a;
                ec = 0;
                break;
            }
        }
    }
    
    if (!ec)
        notifyChanged();
}

String MediaList::mediaText() const
{
    String text("");

    bool first = true;
    for (size_t i = 0; i < m_queries.size(); ++i) {
        if (!first)
            text += ", ";
        else
            first = false;
        text += m_queries[i]->cssText();
    }

    return text;
}

void MediaList::setMediaText(const String& value, ExceptionCode& ec)
{
    RefPtr<MediaList> tempMediaList = MediaList::create();
    CSSParser p(true);

    Vector<String> list;
    value.split(',', list);
    Vector<String>::const_iterator end = list.end();
    for (Vector<String>::const_iterator it = list.begin(); it != end; ++it) {
        String medium = (*it).stripWhiteSpace();
        if (!medium.isEmpty()) {
            if (!p.parseMediaQuery(tempMediaList.get(), medium)) {
                if (m_fallback) {
                    String mediaDescriptor = parseMediaDescriptor(medium);
                    if (!mediaDescriptor.isNull())
                        tempMediaList->m_queries.append(new MediaQuery(MediaQuery::None, mediaDescriptor, nullptr));
                } else {
                    ec = SYNTAX_ERR;
                    return;
                }
            }          
        } else if (!m_fallback) {
            ec = SYNTAX_ERR;
            return;
        }            
    }
    // ",,,," falls straight through, but is not valid unless fallback
    if (!m_fallback && list.begin() == list.end()) {
        String s = value.stripWhiteSpace();
        if (!s.isEmpty()) {
            ec = SYNTAX_ERR;
            return;
        }
    }
    
    ec = 0;
    deleteAllValues(m_queries);
    m_queries = tempMediaList->m_queries;
    tempMediaList->m_queries.clear();
    notifyChanged();
}

String MediaList::item(unsigned index) const
{
    if (index < m_queries.size()) {
        MediaQuery* query = m_queries[index];
        return query->cssText();
    }

    return String();
}

void MediaList::appendMedium(const String& newMedium, ExceptionCode& ec)
{
    ec = INVALID_CHARACTER_ERR;
    CSSParser p(true);
    if (p.parseMediaQuery(this, newMedium)) {
        ec = 0;
    } else if (m_fallback) {
        String medium = parseMediaDescriptor(newMedium);
        if (!medium.isNull()) {
            m_queries.append(new MediaQuery(MediaQuery::None, medium, nullptr));
            ec = 0;
        }
    }
    
    if (!ec)
        notifyChanged();
}

void MediaList::appendMediaQuery(PassOwnPtr<MediaQuery> mediaQuery)
{
    m_queries.append(mediaQuery.leakPtr());
}

void MediaList::notifyChanged()
{
    for (StyleBase* p = parent(); p; p = p->parent()) {
        if (p->isCSSStyleSheet())
            return static_cast<CSSStyleSheet*>(p)->styleSheetChanged();
    }
}

}
