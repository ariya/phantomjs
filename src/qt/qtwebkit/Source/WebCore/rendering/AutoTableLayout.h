/*
 * Copyright (C) 2002 Lars Knoll (knoll@kde.org)
 *           (C) 2002 Dirk Mueller (mueller@kde.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License.
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

#ifndef AutoTableLayout_h
#define AutoTableLayout_h

#include "LayoutUnit.h"
#include "Length.h"
#include "TableLayout.h"
#include <wtf/Vector.h>

namespace WebCore {

class RenderTable;
class RenderTableCell;

class AutoTableLayout : public TableLayout {
public:
    AutoTableLayout(RenderTable*);
    ~AutoTableLayout();

    virtual void computeIntrinsicLogicalWidths(LayoutUnit& minWidth, LayoutUnit& maxWidth) OVERRIDE;
    virtual void applyPreferredLogicalWidthQuirks(LayoutUnit& minWidth, LayoutUnit& maxWidth) const OVERRIDE;
    virtual void layout();

private:
    void fullRecalc();
    void recalcColumn(unsigned effCol);

    int calcEffectiveLogicalWidth();

    void insertSpanCell(RenderTableCell*);

    struct Layout {
        Layout()
            : minLogicalWidth(0)
            , maxLogicalWidth(0)
            , effectiveMinLogicalWidth(0)
            , effectiveMaxLogicalWidth(0)
            , computedLogicalWidth(0)
            , emptyCellsOnly(true)
        {
        }

        Length logicalWidth;
        Length effectiveLogicalWidth;
        int minLogicalWidth;
        int maxLogicalWidth;
        int effectiveMinLogicalWidth;
        int effectiveMaxLogicalWidth;
        int computedLogicalWidth;
        bool emptyCellsOnly;
    };

    Vector<Layout, 4> m_layoutStruct;
    Vector<RenderTableCell*, 4> m_spanCells;
    bool m_hasPercent : 1;
    mutable bool m_effectiveLogicalWidthDirty : 1;
};

} // namespace WebCore

#endif // AutoTableLayout_h
