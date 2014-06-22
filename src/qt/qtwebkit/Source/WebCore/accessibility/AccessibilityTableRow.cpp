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

AccessibilityRole AccessibilityTableRow::determineAccessibilityRole()
{
    if (!isTableRow())
        return AccessibilityRenderObject::determineAccessibilityRole();

    m_ariaRole = determineAriaRoleAttribute();

    AccessibilityRole ariaRole = ariaRoleAttribute();
    if (ariaRole != UnknownRole)
        return ariaRole;

    return RowRole;
}

bool AccessibilityTableRow::isTableRow() const
{
    AccessibilityObject* table = parentTable();
    if (!table || !table->isAccessibilityTable())
        return false;
    
    return true;
}
    
AccessibilityObject* AccessibilityTableRow::observableObject() const
{
    // This allows the table to be the one who sends notifications about tables.
    return parentTable();
}
    
bool AccessibilityTableRow::computeAccessibilityIsIgnored() const
{    
    AccessibilityObjectInclusion decision = defaultObjectInclusion();
    if (decision == IncludeObject)
        return false;
    if (decision == IgnoreObject)
        return true;
    
    if (!isTableRow())
        return AccessibilityRenderObject::computeAccessibilityIsIgnored();

    return false;
}
    
AccessibilityObject* AccessibilityTableRow::parentTable() const
{
    // The parent table might not be the direct ancestor of the row unfortunately. ARIA states that role="grid" should
    // only have "row" elements, but if not, we still should handle it gracefully by finding the right table.
    for (AccessibilityObject* parent = parentObject(); parent; parent = parent->parentObject()) {
        // If this is a table object, but not an accessibility table, we should stop because we don't want to
        // choose another ancestor table as this row's table.
        if (parent->isTable())
            return parent->isAccessibilityTable() ? parent : 0;
    }
    
    return 0;
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
