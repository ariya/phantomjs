/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * (C) 2002-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002, 2006, 2008 Apple Inc. All rights reserved.
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

#ifndef CSSFontFaceRule_h
#define CSSFontFaceRule_h

#include "CSSRule.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class CSSMutableStyleDeclaration;

class CSSFontFaceRule : public CSSRule {
public:
    static PassRefPtr<CSSFontFaceRule> create()
    {
        return adoptRef(new CSSFontFaceRule(0));
    }
    static PassRefPtr<CSSFontFaceRule> create(CSSStyleSheet* parent)
    {
        return adoptRef(new CSSFontFaceRule(parent));
    }

    virtual ~CSSFontFaceRule();

    CSSMutableStyleDeclaration* style() const { return m_style.get(); }

    virtual String cssText() const;

    void setDeclaration(PassRefPtr<CSSMutableStyleDeclaration>);

    virtual void addSubresourceStyleURLs(ListHashSet<KURL>& urls);

private:
    CSSFontFaceRule(CSSStyleSheet* parent);

    virtual bool isFontFaceRule() { return true; }

    // Inherited from CSSRule
    virtual unsigned short type() const { return FONT_FACE_RULE; }

    RefPtr<CSSMutableStyleDeclaration> m_style;
};

} // namespace WebCore

#endif // CSSFontFaceRule_h
