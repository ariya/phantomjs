/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
#include "AccessibilityARIAGridRow.h"

#include "AccessibilityObject.h"
#include "AccessibilityTable.h"
#include "RenderObject.h"

using namespace std;

namespace WebCore {
    
AccessibilityARIAGridRow::AccessibilityARIAGridRow(RenderObject* renderer)
    : AccessibilityTableRow(renderer)
{
}

AccessibilityARIAGridRow::~AccessibilityARIAGridRow()
{
}

PassRefPtr<AccessibilityARIAGridRow> AccessibilityARIAGridRow::create(RenderObject* renderer)
{
    return adoptRef(new AccessibilityARIAGridRow(renderer));
}

bool AccessibilityARIAGridRow::isARIATreeGridRow() const
{
    AccessibilityObject* parent = parentTable();
    if (!parent)
        return false;
    
    return parent->ariaRoleAttribute() == TreeGridRole;
}
    
void AccessibilityARIAGridRow::disclosedRows(AccessibilityChildrenVector& disclosedRows)
{
    // The contiguous disclosed rows will be the rows in the table that 
    // have an aria-level of plus 1 from this row.
    AccessibilityObject* parent = parentObjectUnignored();
    if (!parent || !parent->isAccessibilityTable())
        return;
    
    // Search for rows that match the correct level. 
    // Only take the subsequent rows from this one that are +1 from this row's level.
    int index = rowIndex();
    if (index < 0)
        return;
    
    unsigned level = hierarchicalLevel();
    AccessibilityChildrenVector& allRows = static_cast<AccessibilityTable*>(parent)->rows();
    int rowCount = allRows.size();
    for (int k = index + 1; k < rowCount; ++k) {
        AccessibilityObject* row = allRows[k].get();
        // Stop at the first row that doesn't match the correct level.
        if (row->hierarchicalLevel() != level + 1)
            break;

        disclosedRows.append(row);
    }
}
    
AccessibilityObject* AccessibilityARIAGridRow::disclosedByRow() const
{
    // The row that discloses this one is the row in the table
    // that is aria-level subtract 1 from this row.
    AccessibilityObject* parent = parentObjectUnignored();
    if (!parent || !parent->isAccessibilityTable())
        return 0;
    
    // If the level is 1 or less, than nothing discloses this row.
    unsigned level = hierarchicalLevel();
    if (level <= 1)
        return 0;
    
    // Search for the previous row that matches the correct level.
    int index = rowIndex();
    AccessibilityChildrenVector& allRows = static_cast<AccessibilityTable*>(parent)->rows();
    int rowCount = allRows.size();
    if (index >= rowCount)
        return 0;
    
    for (int k = index - 1; k >= 0; --k) {
        AccessibilityObject* row = allRows[k].get();
        if (row->hierarchicalLevel() == level - 1)
            return row;
    }
    
    return 0;
}
    
AccessibilityObject* AccessibilityARIAGridRow::parentTable() const
{
    // The parent table might not be the direct ancestor of the row unfortunately. ARIA states that role="grid" should
    // only have "row" elements, but if not, we still should handle it gracefully by finding the right table.
    for (AccessibilityObject* parent = parentObject(); parent; parent = parent->parentObject()) {
        // The parent table for an ARIA grid row should be an ARIA table.
        if (parent->isTable() && parent->isAccessibilityTable() && toAccessibilityTable(parent)->isAriaTable())
            return parent;
    }
    
    return 0;
}

AccessibilityObject* AccessibilityARIAGridRow::headerObject()
{
    AccessibilityChildrenVector rowChildren = children();
    unsigned childrenCount = rowChildren.size();
    for (unsigned i = 0; i < childrenCount; ++i) {
        AccessibilityObject* cell = rowChildren[i].get();
        if (cell->ariaRoleAttribute() == RowHeaderRole)
            return cell;
    }
    
    return 0;
}

} // namespace WebCore
