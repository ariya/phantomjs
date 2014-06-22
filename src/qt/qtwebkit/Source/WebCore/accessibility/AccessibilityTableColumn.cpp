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
#include "AccessibilityTableColumn.h"

#include "AXObjectCache.h"
#include "AccessibilityTableCell.h"
#include "HTMLNames.h"
#include "RenderTable.h"
#include "RenderTableCell.h"
#include "RenderTableSection.h"

using namespace std;

namespace WebCore {
    
using namespace HTMLNames;

AccessibilityTableColumn::AccessibilityTableColumn()
{
}

AccessibilityTableColumn::~AccessibilityTableColumn()
{
}    

PassRefPtr<AccessibilityTableColumn> AccessibilityTableColumn::create()
{
    return adoptRef(new AccessibilityTableColumn());
}

void AccessibilityTableColumn::setParent(AccessibilityObject* parent)
{
    AccessibilityMockObject::setParent(parent);
    
    clearChildren();
}
    
LayoutRect AccessibilityTableColumn::elementRect() const
{
    // this will be filled in when addChildren is called
    return m_columnRect;
}

AccessibilityObject* AccessibilityTableColumn::headerObject()
{
    if (!m_parent)
        return 0;
    
    RenderObject* renderer = m_parent->renderer();
    if (!renderer)
        return 0;
    
    if (!m_parent->isAccessibilityTable())
        return 0;
    
    AccessibilityTable* parentTable = toAccessibilityTable(m_parent);
    if (parentTable->isAriaTable()) {
        AccessibilityChildrenVector rowChildren = children();
        unsigned childrenCount = rowChildren.size();
        for (unsigned i = 0; i < childrenCount; ++i) {
            AccessibilityObject* cell = rowChildren[i].get();
            if (cell->ariaRoleAttribute() == ColumnHeaderRole)
                return cell;
        }
        
        return 0;
    }

    if (!renderer->isTable())
        return 0;
    
    RenderTable* table = toRenderTable(renderer);
    
    AccessibilityObject* headerObject = 0;
    
    // try the <thead> section first. this doesn't require th tags
    headerObject = headerObjectForSection(table->header(), false);

    if (headerObject)
        return headerObject;
    
    // now try for <th> tags in the first body
    headerObject = headerObjectForSection(table->firstBody(), true);

    return headerObject;
}

AccessibilityObject* AccessibilityTableColumn::headerObjectForSection(RenderTableSection* section, bool thTagRequired)
{
    if (!section)
        return 0;
    
    unsigned numCols = section->numColumns();
    if (m_columnIndex >= numCols)
        return 0;
    
    if (!section->numRows())
        return 0;
    
    RenderTableCell* cell = 0;
    // also account for cells that have a span
    for (int testCol = m_columnIndex; testCol >= 0; --testCol) {
        RenderTableCell* testCell = section->primaryCellAt(0, testCol);
        if (!testCell)
            continue;
        
        // we've reached a cell that doesn't even overlap our column 
        // it can't be our header
        if ((testCell->col() + (testCell->colSpan()-1)) < m_columnIndex)
            break;
        
        Node* node = testCell->node();
        if (!node)
            continue;
        
        if (thTagRequired && !node->hasTagName(thTag))
            continue;
        
        cell = testCell;
    }
    
    if (!cell)
        return 0;

    return axObjectCache()->getOrCreate(cell);
}
    
bool AccessibilityTableColumn::computeAccessibilityIsIgnored() const
{
    if (!m_parent)
        return true;
    
#if PLATFORM(GTK)
    return true;
#endif
    
    return m_parent->accessibilityIsIgnored();
}
    
void AccessibilityTableColumn::addChildren()
{
    ASSERT(!m_haveChildren); 
    
    m_haveChildren = true;
    if (!m_parent || !m_parent->isAccessibilityTable())
        return;
    
    AccessibilityTable* parentTable = toAccessibilityTable(m_parent);
    int numRows = parentTable->rowCount();
    
    for (int i = 0; i < numRows; i++) {
        AccessibilityTableCell* cell = parentTable->cellForColumnAndRow(m_columnIndex, i);
        if (!cell)
            continue;
        
        // make sure the last one isn't the same as this one (rowspan cells)
        if (m_children.size() > 0 && m_children.last() == cell)
            continue;
            
        m_children.append(cell);
        m_columnRect.unite(cell->elementRect());
    }
}
    
} // namespace WebCore
