/**
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2006 Apple Computer, Inc.
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
#include "RenderBR.h"

#include "Document.h"
#include "InlineTextBox.h"
#include "VisiblePosition.h"

namespace WebCore {

RenderBR::RenderBR(Node* node)
    : RenderText(node, StringImpl::create("\n"))
    , m_lineHeight(-1)
{
}

RenderBR::~RenderBR()
{
}

int RenderBR::lineHeight(bool firstLine) const
{
    if (firstLine && document()->usesFirstLineRules()) {
        RenderStyle* s = style(firstLine);
        if (s != style())
            return s->computedLineHeight();
    }
    
    if (m_lineHeight == -1)
        m_lineHeight = style()->computedLineHeight();
    
    return m_lineHeight;
}

void RenderBR::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderText::styleDidChange(diff, oldStyle);
    m_lineHeight = -1;
}

int RenderBR::caretMinOffset() const 
{ 
    return 0;
}

int RenderBR::caretMaxOffset() const 
{ 
    return 1;
}

unsigned RenderBR::caretMaxRenderedOffset() const
{
    return 1;
}

VisiblePosition RenderBR::positionForPoint(const IntPoint&)
{
    return createVisiblePosition(0, DOWNSTREAM);
}

} // namespace WebCore
