/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * (C) 2002-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002, 2005, 2006, 2008 Apple Inc. All rights reserved.
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
#include "CSSPageRule.h"

#include "CSSMutableStyleDeclaration.h"
#include <wtf/Vector.h>

namespace WebCore {

CSSPageRule::CSSPageRule(CSSStyleSheet* parent, int sourceLine)
    : CSSStyleRule(parent, sourceLine)
{
}

CSSPageRule::~CSSPageRule()
{
}

String CSSPageRule::selectorText() const
{
    String text = "@page";
    CSSSelector* selector = selectorList().first();
    if (selector) {
        String pageSpecification = selector->selectorText();
        if (!pageSpecification.isEmpty() && pageSpecification != starAtom)
            text += " " + pageSpecification;
    }
    return text;
}

} // namespace WebCore
