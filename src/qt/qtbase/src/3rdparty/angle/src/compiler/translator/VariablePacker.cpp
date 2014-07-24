//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
#include "compiler/translator/VariablePacker.h"

#include <algorithm>
#include "compiler/translator/ShHandle.h"

namespace {
int GetSortOrder(ShDataType type)
{
    switch (type) {
        case SH_FLOAT_MAT4:
            return 0;
        case SH_FLOAT_MAT2:
            return 1;
        case SH_FLOAT_VEC4:
        case SH_INT_VEC4:
        case SH_BOOL_VEC4:
            return 2;
        case SH_FLOAT_MAT3:
            return 3;
        case SH_FLOAT_VEC3:
        case SH_INT_VEC3:
        case SH_BOOL_VEC3:
            return 4;
        case SH_FLOAT_VEC2:
        case SH_INT_VEC2:
        case SH_BOOL_VEC2:
            return 5;
        case SH_FLOAT:
        case SH_INT:
        case SH_BOOL:
        case SH_SAMPLER_2D:
        case SH_SAMPLER_CUBE:
        case SH_SAMPLER_EXTERNAL_OES:
        case SH_SAMPLER_2D_RECT_ARB:
            return 6;
        default:
            ASSERT(false);
            return 7;
    }
}
}    // namespace

int VariablePacker::GetNumComponentsPerRow(ShDataType type)
{
    switch (type) {
        case SH_FLOAT_MAT4:
        case SH_FLOAT_MAT2:
        case SH_FLOAT_VEC4:
        case SH_INT_VEC4:
        case SH_BOOL_VEC4:
            return 4;
        case SH_FLOAT_MAT3:
        case SH_FLOAT_VEC3:
        case SH_INT_VEC3:
        case SH_BOOL_VEC3:
            return 3;
        case SH_FLOAT_VEC2:
        case SH_INT_VEC2:
        case SH_BOOL_VEC2:
            return 2;
        case SH_FLOAT:
        case SH_INT:
        case SH_BOOL:
        case SH_SAMPLER_2D:
        case SH_SAMPLER_CUBE:
        case SH_SAMPLER_EXTERNAL_OES:
        case SH_SAMPLER_2D_RECT_ARB:
            return 1;
        default:
            ASSERT(false);
            return 5;
    }
}

int VariablePacker::GetNumRows(ShDataType type)
{
    switch (type) {
        case SH_FLOAT_MAT4:
            return 4;
        case SH_FLOAT_MAT3:
            return 3;
        case SH_FLOAT_MAT2:
            return 2;
        case SH_FLOAT_VEC4:
        case SH_INT_VEC4:
        case SH_BOOL_VEC4:
        case SH_FLOAT_VEC3:
        case SH_INT_VEC3:
        case SH_BOOL_VEC3:
        case SH_FLOAT_VEC2:
        case SH_INT_VEC2:
        case SH_BOOL_VEC2:
        case SH_FLOAT:
        case SH_INT:
        case SH_BOOL:
        case SH_SAMPLER_2D:
        case SH_SAMPLER_CUBE:
        case SH_SAMPLER_EXTERNAL_OES:
        case SH_SAMPLER_2D_RECT_ARB:
            return 1;
        default:
            ASSERT(false);
            return 100000;
    }
}

struct TVariableInfoComparer {
    bool operator()(const TVariableInfo& lhs, const TVariableInfo& rhs) const
    {
        int lhsSortOrder = GetSortOrder(lhs.type);
        int rhsSortOrder = GetSortOrder(rhs.type);
        if (lhsSortOrder != rhsSortOrder) {
            return lhsSortOrder < rhsSortOrder;
        }
        // Sort by largest first.
        return lhs.size > rhs.size;
    }
};

unsigned VariablePacker::makeColumnFlags(int column, int numComponentsPerRow)
{
    return ((kColumnMask << (kNumColumns - numComponentsPerRow)) &
                    kColumnMask) >> column;
}

void VariablePacker::fillColumns(int topRow, int numRows, int column, int numComponentsPerRow)
{
    unsigned columnFlags = makeColumnFlags(column, numComponentsPerRow);
    for (int r = 0; r < numRows; ++r) {
        int row = topRow + r;
        ASSERT((rows_[row] & columnFlags) == 0);
        rows_[row] |= columnFlags;
    }
}

bool VariablePacker::searchColumn(int column, int numRows, int* destRow, int* destSize)
{
    ASSERT(destRow);

    for (; topNonFullRow_ < maxRows_ && rows_[topNonFullRow_] == kColumnMask;
         ++topNonFullRow_) {
    }

    for (; bottomNonFullRow_ >= 0 && rows_[bottomNonFullRow_] == kColumnMask;
         --bottomNonFullRow_) {
    }

    if (bottomNonFullRow_ - topNonFullRow_ + 1 < numRows) {
        return false;
    }

    unsigned columnFlags = makeColumnFlags(column, 1);
    int topGoodRow = 0;
    int smallestGoodTop = -1;
    int smallestGoodSize = maxRows_ + 1;
    int bottomRow = bottomNonFullRow_ + 1;
    bool found = false;
    for (int row = topNonFullRow_; row <= bottomRow; ++row) {
        bool rowEmpty = row < bottomRow ? ((rows_[row] & columnFlags) == 0) : false;
        if (rowEmpty) {
            if (!found) {
                topGoodRow = row;
                found = true;
            }
        } else {
            if (found) {
                int size = row - topGoodRow;
                if (size >= numRows && size < smallestGoodSize) {
                    smallestGoodSize = size;
                    smallestGoodTop = topGoodRow;
                }
            }
            found = false;
        }
    }
    if (smallestGoodTop < 0) {
        return false;
    }

    *destRow = smallestGoodTop;
    if (destSize) {
        *destSize = smallestGoodSize;
    }
    return true;
}

bool VariablePacker::CheckVariablesWithinPackingLimits(int maxVectors, const TVariableInfoList& in_variables)
{
    ASSERT(maxVectors > 0);
    maxRows_ = maxVectors;
    topNonFullRow_ = 0;
    bottomNonFullRow_ = maxRows_ - 1;
    TVariableInfoList variables(in_variables);

    // As per GLSL 1.017 Appendix A, Section 7 variables are packed in specific
    // order by type, then by size of array, largest first.
    std::sort(variables.begin(), variables.end(), TVariableInfoComparer());
    rows_.clear();
    rows_.resize(maxVectors, 0);

    // Packs the 4 column variables.
    size_t ii = 0;
    for (; ii < variables.size(); ++ii) {
        const TVariableInfo& variable = variables[ii];
        if (GetNumComponentsPerRow(variable.type) != 4) {
            break;
        }
        topNonFullRow_ += GetNumRows(variable.type) * variable.size;
    }

    if (topNonFullRow_ > maxRows_) {
        return false;
    }

    // Packs the 3 column variables.
    int num3ColumnRows = 0;
    for (; ii < variables.size(); ++ii) {
        const TVariableInfo& variable = variables[ii];
        if (GetNumComponentsPerRow(variable.type) != 3) {
            break;
        }
        num3ColumnRows += GetNumRows(variable.type) * variable.size;
    }

    if (topNonFullRow_ + num3ColumnRows > maxRows_) {
        return false;
    }

    fillColumns(topNonFullRow_, num3ColumnRows, 0, 3);

    // Packs the 2 column variables.
    int top2ColumnRow = topNonFullRow_ + num3ColumnRows;
    int twoColumnRowsAvailable = maxRows_ - top2ColumnRow;
    int rowsAvailableInColumns01 = twoColumnRowsAvailable;
    int rowsAvailableInColumns23 = twoColumnRowsAvailable;
    for (; ii < variables.size(); ++ii) {
        const TVariableInfo& variable = variables[ii];
        if (GetNumComponentsPerRow(variable.type) != 2) {
            break;
        }
        int numRows = GetNumRows(variable.type) * variable.size;
        if (numRows <= rowsAvailableInColumns01) {
            rowsAvailableInColumns01 -= numRows;
        } else if (numRows <= rowsAvailableInColumns23) {
            rowsAvailableInColumns23 -= numRows;
        } else {
            return false;
        }
    }

    int numRowsUsedInColumns01 =
        twoColumnRowsAvailable - rowsAvailableInColumns01;
    int numRowsUsedInColumns23 =
        twoColumnRowsAvailable - rowsAvailableInColumns23;
    fillColumns(top2ColumnRow, numRowsUsedInColumns01, 0, 2);
    fillColumns(maxRows_ - numRowsUsedInColumns23, numRowsUsedInColumns23,
                2, 2);

    // Packs the 1 column variables.
    for (; ii < variables.size(); ++ii) {
        const TVariableInfo& variable = variables[ii];
        ASSERT(1 == GetNumComponentsPerRow(variable.type));
        int numRows = GetNumRows(variable.type) * variable.size;
        int smallestColumn = -1;
        int smallestSize = maxRows_ + 1;
        int topRow = -1;
        for (int column = 0; column < kNumColumns; ++column) {
            int row = 0;
            int size = 0;
            if (searchColumn(column, numRows, &row, &size)) {
                if (size < smallestSize) {
                    smallestSize = size;
                    smallestColumn = column;
                    topRow = row;
                }
            }
        }

        if (smallestColumn < 0) {
            return false;
        }

        fillColumns(topRow, numRows, smallestColumn, 1);
    }

    ASSERT(variables.size() == ii);

    return true;
}



