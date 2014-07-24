/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * (C) 2002-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002, 2006, 2008, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Samuel Weinig (sam@webkit.org)
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

#ifndef CSSHostRule_h
#define CSSHostRule_h

#include "CSSGroupingRule.h"

namespace WebCore {

#if ENABLE(SHADOW_DOM)

class CSSHostRule : public CSSGroupingRule {
public:
    static PassRefPtr<CSSHostRule> create(StyleRuleHost* rule, CSSStyleSheet* sheet) { return adoptRef(new CSSHostRule(rule, sheet)); }

    virtual CSSRule::Type type() const OVERRIDE { return CSSRule::HOST_RULE; }
    virtual String cssText() const OVERRIDE;

private:
    CSSHostRule(StyleRuleHost*, CSSStyleSheet*);
};

#endif // ENABLE(SHADOW_DOM)

} // namespace WebCore

#endif // CSSHostRule_h
