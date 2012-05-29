/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "AccessibilityTableRow.h"

#include "AXObjectCache.h"
#include "AccessibilityTableCell.h"
#include "HTMLNames.h"
#include "HTMLTableRowElement.h"
#include "RenderObject.h"
#include "RenderTableCell.h"
#include "RenderTableRow.h"

using namespace std;

namespace WebCore {
    
using namespace HTMLNames;
    
AccessibilityTableRow::AccessibilityTableRow(RenderObject* renderer)
    : AccessibilityRenderObject(renderer)
{
}

AccessibilityTableRow::~AccessibilityTableRow()
{
}

PassRefPtr<AccessibilityTableRow> AccessibilityTableRow::create(RenderObject* renderer)
{
    return adoptRef(new AccessibilityTableRow(renderer));
}

AccessibilityRole AccessibilityTableRow::roleValue() const
{
    if (!isTableRow())
        return AccessibilityRenderObject::roleValue();
    
    return RowRole;
}

bool AccessibilityTableRow::isTableRow() const
{
    AccessibilityObject* table = parentTable();
    if (!table || !table->isAccessibilityTable())
        return false;
    
    return true;
}
    
bool AccessibilityTableRow::accessibilityIsIgnored() const
{    
    AccessibilityObjectInclusion decision = accessibilityIsIgnoredBase();
    if (decision == IncludeObject)
        return false;
    if (decision == IgnoreObject)
        return true;
    
    if (!isTableRow())
        return AccessibilityRenderObject::accessibilityIsIgnored();

    return false;
}
    
AccessibilityObject* AccessibilityTableRow::parentTable() const
{
    if (!m_renderer || !m_renderer->isTableRow())
        return 0;
    
    // Do not use getOrCreate. parentTable() can be called while the render tree is being modified.
    return axObjectCache()->get(toRenderTableRow(m_renderer)->table());
}
    
AccessibilityObject* AccessibilityTableRow::headerObject()
{
    if (!m_renderer || !m_renderer->isTableRow())
        return 0;
    
    AccessibilityChildrenVector rowChildren = children();
    if (!rowChildren.size())
        return 0;
    
    // check the first element in the row to see if it is a TH element
    AccessibilityObject* cell = rowChildren[0].get();
    if (!cell->isTableCell())
        return 0;
    
    RenderObject* cellRenderer = static_cast<AccessibilityTableCell*>(cell)->renderer();
    if (!cellRenderer)
        return 0;
    
    Node* cellNode = cellRenderer->node();
    if (!cellNode || !cellNode->hasTagName(thTag))
        return 0;
    
    return cell;
}
    
} // namespace WebCore
