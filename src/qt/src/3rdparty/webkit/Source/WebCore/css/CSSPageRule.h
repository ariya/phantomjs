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

#ifndef CSSPageRule_h
#define CSSPageRule_h

#include "CSSStyleRule.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class CSSMutableStyleDeclaration;
class CSSSelector;
class CSSSelectorList;

class CSSPageRule : public CSSStyleRule {
public:
    static PassRefPtr<CSSPageRule> create(CSSStyleSheet* parent, int sourceLine)
    {
        return adoptRef(new CSSPageRule(parent, sourceLine));
    }

    virtual ~CSSPageRule();

    virtual String selectorText() const;

private:
    CSSPageRule(CSSStyleSheet* parent, int sourceLine);

    virtual bool isPageRule() { return true; }

    // Inherited from CSSRule
    virtual unsigned short type() const { return PAGE_RULE; }
};

} // namespace WebCore

#endif // CSSPageRule_h
