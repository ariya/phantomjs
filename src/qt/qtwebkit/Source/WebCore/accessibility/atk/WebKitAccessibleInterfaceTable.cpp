/*
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2009 Jan Alonzo
 * Copyright (C) 2009, 2010, 2011, 2012 Igalia S.L.
 *
 * Portions from Mozilla a11y, copyright as follows:
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Sun Microsystems, Inc.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
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
#include "WebKitAccessibleInterfaceTable.h"

#if HAVE(ACCESSIBILITY)

#include "AccessibilityListBox.h"
#include "AccessibilityObject.h"
#include "AccessibilityTable.h"
#include "AccessibilityTableCell.h"
#include "HTMLSelectElement.h"
#include "HTMLTableCaptionElement.h"
#include "HTMLTableElement.h"
#include "RenderObject.h"
#include "WebKitAccessibleInterfaceText.h"
#include "WebKitAccessibleWrapperAtk.h"

using namespace WebCore;

static AccessibilityObject* core(AtkTable* table)
{
    if (!WEBKIT_IS_ACCESSIBLE(table))
        return 0;

    return webkitAccessibleGetAccessibilityObject(WEBKIT_ACCESSIBLE(table));
}

static AccessibilityTableCell* cell(AtkTable* table, guint row, guint column)
{
    AccessibilityObject* accTable = core(table);
    if (accTable->isAccessibilityRenderObject())
        return static_cast<AccessibilityTable*>(accTable)->cellForColumnAndRow(column, row);
    return 0;
}

static gint cellIndex(AccessibilityTableCell* axCell, AccessibilityTable* axTable)
{
    // Calculate the cell's index as if we had a traditional Gtk+ table in
    // which cells are all direct children of the table, arranged row-first.
    AccessibilityObject::AccessibilityChildrenVector allCells;
    axTable->cells(allCells);
    AccessibilityObject::AccessibilityChildrenVector::iterator position;
    position = std::find(allCells.begin(), allCells.end(), axCell);
    if (position == allCells.end())
        return -1;
    return position - allCells.begin();
}

static AccessibilityTableCell* cellAtIndex(AtkTable* table, gint index)
{
    AccessibilityObject* accTable = core(table);
    if (accTable->isAccessibilityRenderObject()) {
        AccessibilityObject::AccessibilityChildrenVector allCells;
        static_cast<AccessibilityTable*>(accTable)->cells(allCells);
        if (0 <= index && static_cast<unsigned>(index) < allCells.size()) {
            AccessibilityObject* accCell = allCells.at(index).get();
            return static_cast<AccessibilityTableCell*>(accCell);
        }
    }
    return 0;
}

static AtkObject* webkitAccessibleTableRefAt(AtkTable* table, gint row, gint column)
{
    AccessibilityTableCell* axCell = cell(table, row, column);
    if (!axCell)
        return 0;

    AtkObject* cell = axCell->wrapper();
    if (!cell)
        return 0;

    // This method transfers full ownership over the returned
    // AtkObject, so an extra reference is needed here.
    return ATK_OBJECT(g_object_ref(cell));
}

static gint webkitAccessibleTableGetIndexAt(AtkTable* table, gint row, gint column)
{
    AccessibilityTableCell* axCell = cell(table, row, column);
    AccessibilityTable* axTable = static_cast<AccessibilityTable*>(core(table));
    return cellIndex(axCell, axTable);
}

static gint webkitAccessibleTableGetColumnAtIndex(AtkTable* table, gint index)
{
    AccessibilityTableCell* axCell = cellAtIndex(table, index);
    if (axCell) {
        pair<unsigned, unsigned> columnRange;
        axCell->columnIndexRange(columnRange);
        return columnRange.first;
    }
    return -1;
}

static gint webkitAccessibleTableGetRowAtIndex(AtkTable* table, gint index)
{
    AccessibilityTableCell* axCell = cellAtIndex(table, index);
    if (axCell) {
        pair<unsigned, unsigned> rowRange;
        axCell->rowIndexRange(rowRange);
        return rowRange.first;
    }
    return -1;
}

static gint webkitAccessibleTableGetNColumns(AtkTable* table)
{
    AccessibilityObject* accTable = core(table);
    if (accTable->isAccessibilityRenderObject())
        return static_cast<AccessibilityTable*>(accTable)->columnCount();
    return 0;
}

static gint webkitAccessibleTableGetNRows(AtkTable* table)
{
    AccessibilityObject* accTable = core(table);
    if (accTable->isAccessibilityRenderObject())
        return static_cast<AccessibilityTable*>(accTable)->rowCount();
    return 0;
}

static gint webkitAccessibleTableGetColumnExtentAt(AtkTable* table, gint row, gint column)
{
    AccessibilityTableCell* axCell = cell(table, row, column);
    if (axCell) {
        pair<unsigned, unsigned> columnRange;
        axCell->columnIndexRange(columnRange);
        return columnRange.second;
    }
    return 0;
}

static gint webkitAccessibleTableGetRowExtentAt(AtkTable* table, gint row, gint column)
{
    AccessibilityTableCell* axCell = cell(table, row, column);
    if (axCell) {
        pair<unsigned, unsigned> rowRange;
        axCell->rowIndexRange(rowRange);
        return rowRange.second;
    }
    return 0;
}

static AtkObject* webkitAccessibleTableGetColumnHeader(AtkTable* table, gint column)
{
    AccessibilityObject* accTable = core(table);
    if (accTable->isAccessibilityRenderObject()) {
        AccessibilityObject::AccessibilityChildrenVector allColumnHeaders;
        static_cast<AccessibilityTable*>(accTable)->columnHeaders(allColumnHeaders);
        unsigned columnCount = allColumnHeaders.size();
        for (unsigned k = 0; k < columnCount; ++k) {
            pair<unsigned, unsigned> columnRange;
            AccessibilityTableCell* cell = static_cast<AccessibilityTableCell*>(allColumnHeaders.at(k).get());
            cell->columnIndexRange(columnRange);
            if (columnRange.first <= static_cast<unsigned>(column) && static_cast<unsigned>(column) < columnRange.first + columnRange.second)
                return allColumnHeaders[k]->wrapper();
        }
    }
    return 0;
}

static AtkObject* webkitAccessibleTableGetRowHeader(AtkTable* table, gint row)
{
    AccessibilityObject* accTable = core(table);
    if (accTable->isAccessibilityRenderObject()) {
        AccessibilityObject::AccessibilityChildrenVector allRowHeaders;
        static_cast<AccessibilityTable*>(accTable)->rowHeaders(allRowHeaders);
        unsigned rowCount = allRowHeaders.size();
        for (unsigned k = 0; k < rowCount; ++k) {
            pair<unsigned, unsigned> rowRange;
            AccessibilityTableCell* cell = static_cast<AccessibilityTableCell*>(allRowHeaders.at(k).get());
            cell->rowIndexRange(rowRange);
            if (rowRange.first <= static_cast<unsigned>(row) && static_cast<unsigned>(row) < rowRange.first + rowRange.second)
                return allRowHeaders[k]->wrapper();
        }
    }
    return 0;
}

static AtkObject* webkitAccessibleTableGetCaption(AtkTable* table)
{
    AccessibilityObject* accTable = core(table);
    if (accTable->isAccessibilityRenderObject()) {
        Node* node = accTable->node();
        if (node && isHTMLTableElement(node)) {
            HTMLTableCaptionElement* caption = toHTMLTableElement(node)->caption();
            if (caption)
                return AccessibilityObject::firstAccessibleObjectFromNode(caption->renderer()->node())->wrapper();
        }
    }
    return 0;
}

static const gchar* webkitAccessibleTableGetColumnDescription(AtkTable* table, gint column)
{
    AtkObject* columnHeader = atk_table_get_column_header(table, column);
    if (columnHeader && ATK_IS_TEXT(columnHeader))
        return atk_text_get_text(ATK_TEXT(columnHeader), 0, -1);

    return 0;
}

static const gchar* webkitAccessibleTableGetRowDescription(AtkTable* table, gint row)
{
    AtkObject* rowHeader = atk_table_get_row_header(table, row);
    if (rowHeader && ATK_IS_TEXT(rowHeader))
        return atk_text_get_text(ATK_TEXT(rowHeader), 0, -1);

    return 0;
}

void webkitAccessibleTableInterfaceInit(AtkTableIface* iface)
{
    iface->ref_at = webkitAccessibleTableRefAt;
    iface->get_index_at = webkitAccessibleTableGetIndexAt;
    iface->get_column_at_index = webkitAccessibleTableGetColumnAtIndex;
    iface->get_row_at_index = webkitAccessibleTableGetRowAtIndex;
    iface->get_n_columns = webkitAccessibleTableGetNColumns;
    iface->get_n_rows = webkitAccessibleTableGetNRows;
    iface->get_column_extent_at = webkitAccessibleTableGetColumnExtentAt;
    iface->get_row_extent_at = webkitAccessibleTableGetRowExtentAt;
    iface->get_column_header = webkitAccessibleTableGetColumnHeader;
    iface->get_row_header = webkitAccessibleTableGetRowHeader;
    iface->get_caption = webkitAccessibleTableGetCaption;
    iface->get_column_description = webkitAccessibleTableGetColumnDescription;
    iface->get_row_description = webkitAccessibleTableGetRowDescription;
}

#endif
