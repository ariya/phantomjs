/*
 * Copyright (C) 2010, 2011 Nokia Corporation and/or its subsidiary(-ies)
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
 *
 */

#include "config.h"
#include "RenderDetails.h"

#if ENABLE(DETAILS)

#include "CSSStyleSelector.h"
#include "HTMLDetailsElement.h"
#include "HTMLNames.h"

namespace WebCore {

using namespace HTMLNames;

RenderDetails::RenderDetails(Node* node)
    : RenderBlock(node)
{
}

void RenderDetails::addChild(RenderObject* newChild, RenderObject* beforeChild)
{
    if (static_cast<HTMLDetailsElement*>(node())->mainSummary() == newChild->node())
        RenderBlock::addChild(newChild, firstChild());
    else
        RenderBlock::addChild(newChild, beforeChild);
}

void RenderDetails::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderBlock::styleDidChange(diff, oldStyle);
    // Ensure that if we ended up being inline that we set our replaced flag
    // so that we're treated like an inline-block.
    setReplaced(isInline());
}

bool RenderDetails::isOpen() const
{
    return node() && node()->isElementNode() ? !static_cast<Element*>(node())->getAttribute(openAttr).isNull() : false;
}

} // namespace WebCore

#endif
