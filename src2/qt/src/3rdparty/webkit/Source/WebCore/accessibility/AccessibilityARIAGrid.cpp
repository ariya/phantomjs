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
#include "AccessibilityARIAGrid.h"

#include "AXObjectCache.h"
#include "AccessibilityTableCell.h"
#include "AccessibilityTableColumn.h"
#include "AccessibilityTableHeaderContainer.h"
#include "AccessibilityTableRow.h"
#include "RenderObject.h"

using namespace std;

namespace WebCore {

AccessibilityARIAGrid::AccessibilityARIAGrid(RenderObject* renderer)
    : AccessibilityTable(renderer)
{
#if ACCESSIBILITY_TABLES
    m_isAccessibilityTable = true;
#else
    m_isAccessibilityTable = false;
#endif
}

AccessibilityARIAGrid::~AccessibilityARIAGrid()
{
}

PassRefPtr<AccessibilityARIAGrid> AccessibilityARIAGrid::create(RenderObject* renderer)
{
    return adoptRef(new AccessibilityARIAGrid(renderer));
}

void AccessibilityARIAGrid::addChild(AccessibilityObject* child, HashSet<AccessibilityObject*>& appendedRows, unsigned& columnCount)
{
    if (!child || !child->isTableRow() || child->ariaRoleAttribute() != RowRole)
        return;
        
    AccessibilityTableRow* row = static_cast<AccessibilityTableRow*>(child);
    if (appendedRows.contains(row))
        return;
        
    // store the maximum number of columns
    unsigned rowCellCount = row->children().size();
    if (rowCellCount > columnCount)
        columnCount = rowCellCount;
    
    row->setRowIndex((int)m_rows.size());        
    m_rows.append(row);

    // Try adding the row if it's not ignoring accessibility,
    // otherwise add its children (the cells) as the grid's children.
    if (!row->accessibilityIsIgnored())
        m_children.append(row);
    else
        m_children.append(row->children());

    appendedRows.add(row);
}
    
void AccessibilityARIAGrid::addChildren()
{
    ASSERT(!m_haveChildren); 
    
    if (!isAccessibilityTable()) {
        AccessibilityRenderObject::addChildren();
        return;
    }
    
    m_haveChildren = true;
    if (!m_renderer)
        return;
    
    AXObjectCache* axCache = m_renderer->document()->axObjectCache();
    
    // add only rows that are labeled as aria rows
    HashSet<AccessibilityObject*> appendedRows;
    unsigned columnCount = 0;
    for (RefPtr<AccessibilityObject> child = firstChild(); child; child = child->nextSibling()) {

        if (child->isTableRow() || child->ariaRoleAttribute() == RowRole)
            addChild(child.get(), appendedRows, columnCount);
        else {
            // in case the render tree doesn't match the expected ARIA hierarchy, look at the children
            if (!child->hasChildren())
                child->addChildren();

            // Do not navigate children through the Accessibility
            // children vector to let addChild() check the result
            // of accessibilityIsIgnored() and make the proper
            // decision (add the objects or their children).
            AccessibilityObject* grandChild = 0;
            for (grandChild = child->firstChild(); grandChild; grandChild = grandChild->nextSibling())
                addChild(grandChild, appendedRows, columnCount);
        }
    }
    
    // make the columns based on the number of columns in the first body
    for (unsigned i = 0; i < columnCount; ++i) {
        AccessibilityTableColumn* column = static_cast<AccessibilityTableColumn*>(axCache->getOrCreate(ColumnRole));
        column->setColumnIndex((int)i);
        column->setParentTable(this);
        m_columns.append(column);
        if (!column->accessibilityIsIgnored())
            m_children.append(column);
    }
    
    AccessibilityObject* headerContainerObject = headerContainer();
    if (headerContainerObject && !headerContainerObject->accessibilityIsIgnored())
        m_children.append(headerContainerObject);
}
    
AccessibilityTableCell* AccessibilityARIAGrid::cellForColumnAndRow(unsigned column, unsigned row)
{
    if (!m_renderer)
        return 0;
    
    updateChildrenIfNecessary();
    
    if (column >= columnCount() || row >= rowCount())
        return 0;
    
    int intRow = (int)row;
    int intColumn = (int)column;

    pair<int, int> columnRange;
    pair<int, int> rowRange;
    
    // Iterate backwards through the rows in case the desired cell has a rowspan and exists
    // in a previous row.
    for (; intRow >= 0; --intRow) {
        AccessibilityObject* tableRow = m_rows[intRow].get();
        if (!tableRow)
            continue;
        
        AccessibilityChildrenVector children = tableRow->children();
        unsigned childrenLength = children.size();
        
        // Since some cells may have colspans, we have to check the actual range of each
        // cell to determine which is the right one.
        for (unsigned k = 0; k < childrenLength; ++k) {
            AccessibilityObject* child = children[k].get();
            if (!child->isTableCell()) 
                continue;
            
            AccessibilityTableCell* tableCellChild = static_cast<AccessibilityTableCell*>(child);
            tableCellChild->columnIndexRange(columnRange);
            tableCellChild->rowIndexRange(rowRange);
            
            if ((intColumn >= columnRange.first && intColumn < (columnRange.first + columnRange.second))
                && (intRow >= rowRange.first && intRow < (rowRange.first + rowRange.second)))
                return tableCellChild;
        }
    }

    return 0;
}
    
} // namespace WebCore
