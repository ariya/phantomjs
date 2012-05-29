/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * (C) 2002-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002, 2006, 2007 Apple Inc. All rights reserved.
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

#include "CSSStyleSheet.h"
#include "KURLHash.h"
#include <wtf/ListHashSet.h>

namespace WebCore {

typedef int ExceptionCode;

enum CSSRuleFilter {
    AllCSSRules,
    SameOriginCSSRulesOnly
}; 

class CSSRule : public StyleBase {
public:
    // FIXME: Change name to Type.
    enum CSSRuleType {
        UNKNOWN_RULE,
        STYLE_RULE,
        CHARSET_RULE,
        IMPORT_RULE,
        MEDIA_RULE,
        FONT_FACE_RULE,
        PAGE_RULE,
        // 7 used to be VARIABLES_RULE
        WEBKIT_KEYFRAMES_RULE = 8,
        WEBKIT_KEYFRAME_RULE
    };

    // FIXME: Change to return CSSRuleType.
    virtual unsigned short type() const = 0;

    CSSStyleSheet* parentStyleSheet() const;
    CSSRule* parentRule() const;

    virtual String cssText() const = 0;
    void setCssText(const String&, ExceptionCode&);

    virtual void addSubresourceStyleURLs(ListHashSet<KURL>&) { }

protected:
    CSSRule(CSSStyleSheet* parent)
        : StyleBase(parent)
    {
    }

private:
    virtual bool isRule() { return true; }
};

} // namespace WebCore

#endif // CSSRule_h
