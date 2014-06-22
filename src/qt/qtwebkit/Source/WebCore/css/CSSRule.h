/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * (C) 2002-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002, 2006, 2007, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Andreas Kling (kling@webkit.org)
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

#ifndef CSSRule_h
#define CSSRule_h

#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class CSSStyleSheet;
class StyleRuleBase;
struct CSSParserContext;
typedef int ExceptionCode;

class CSSRule : public RefCounted<CSSRule> {
public:
    virtual ~CSSRule() { }

    enum Type {
        UNKNOWN_RULE,
        STYLE_RULE,
        CHARSET_RULE,
        IMPORT_RULE,
        MEDIA_RULE,
        FONT_FACE_RULE,
        PAGE_RULE,
        // 7 was VARIABLES_RULE; we now match other browsers with 7 as
        // KEYFRAMES_RULE:
        // <https://bugs.webkit.org/show_bug.cgi?id=71293>.
        WEBKIT_KEYFRAMES_RULE,
        WEBKIT_KEYFRAME_RULE,
#if ENABLE(CSS3_CONDITIONAL_RULES)
        SUPPORTS_RULE = 12,
#endif
#if ENABLE(CSS_DEVICE_ADAPTATION)
        WEBKIT_VIEWPORT_RULE = 15,
#endif
#if ENABLE(CSS_REGIONS)
        WEBKIT_REGION_RULE = 16,
#endif
#if ENABLE(CSS_SHADERS)
        WEBKIT_FILTER_RULE = 17,
#endif
#if ENABLE(SHADOW_DOM)
        HOST_RULE = 1001,
#endif
    };

    virtual Type type() const = 0;
    virtual String cssText() const = 0;
    virtual void reattach(StyleRuleBase*) = 0;

    void setParentStyleSheet(CSSStyleSheet* styleSheet)
    {
        m_parentIsRule = false;
        m_parentStyleSheet = styleSheet;
    }

    void setParentRule(CSSRule* rule)
    {
        m_parentIsRule = true;
        m_parentRule = rule;
    }

    CSSStyleSheet* parentStyleSheet() const
    {
        if (m_parentIsRule)
            return m_parentRule ? m_parentRule->parentStyleSheet() : 0;
        return m_parentStyleSheet;
    }

    CSSRule* parentRule() const { return m_parentIsRule ? m_parentRule : 0; }

    // NOTE: Just calls notImplemented().
    void setCssText(const String&, ExceptionCode&);

protected:
    CSSRule(CSSStyleSheet* parent)
        : m_hasCachedSelectorText(false)
        , m_parentIsRule(false)
        , m_parentStyleSheet(parent)
    {
    }

    bool hasCachedSelectorText() const { return m_hasCachedSelectorText; }
    void setHasCachedSelectorText(bool hasCachedSelectorText) const { m_hasCachedSelectorText = hasCachedSelectorText; }

    const CSSParserContext& parserContext() const;

private:
    mutable unsigned char m_hasCachedSelectorText : 1;
    unsigned char m_parentIsRule : 1;

    union {
        CSSRule* m_parentRule;
        CSSStyleSheet* m_parentStyleSheet;
    };
};

} // namespace WebCore

#endif // CSSRule_h
