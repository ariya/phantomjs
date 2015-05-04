//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include <algorithm>

#include "angle_gl.h"

#include "compiler/translator/VariablePacker.h"
#include "common/utilities.h"

int VariablePacker::GetNumComponentsPerRow(sh::GLenum type)
{
    switch (type)
    {
      case GL_FLOAT_MAT4:
      case GL_FLOAT_MAT2:
      case GL_FLOAT_MAT2x4:
      case GL_FLOAT_MAT3x4:
      case GL_FLOAT_MAT4x2:
      case GL_FLOAT_MAT4x3:
      case GL_FLOAT_VEC4:
      case GL_INT_VEC4:
      case GL_BOOL_VEC4:
      case GL_UNSIGNED_INT_VEC4:
        return 4;
      case GL_FLOAT_MAT3:
      case GL_FLOAT_MAT2x3:
      case GL_FLOAT_MAT3x2:
      case GL_FLOAT_VEC3:
      case GL_INT_VEC3:
      case GL_BOOL_VEC3:
      case GL_UNSIGNED_INT_VEC3:
        return 3;
      case GL_FLOAT_VEC2:
      case GL_INT_VEC2:
      case GL_BOOL_VEC2:
      case GL_UNSIGNED_INT_VEC2:
        return 2;
      default:
        ASSERT(gl::VariableComponentCount(type) == 1);
        return 1;
    }
}

int VariablePacker::GetNumRows(sh::GLenum type)
{
    switch (type)
    {
      case GL_FLOAT_MAT4:
      case GL_FLOAT_MAT2x4:
      case GL_FLOAT_MAT3x4:
      case GL_FLOAT_MAT4x3:
      case GL_FLOAT_MAT4x2:
        return 4;
      case GL_FLOAT_MAT3:
      case GL_FLOAT_MAT2x3:
      case GL_FLOAT_MAT3x2:
        return 3;
      case GL_FLOAT_MAT2:
        return 2;
      default:
        ASSERT(gl::VariableRowCount(type) == 1);
        return 1;
    }
}

struct TVariableInfoComparer
{
    bool operator()(const sh::ShaderVariable &lhs, const sh::ShaderVariable &rhs) const
    {
        int lhsSortOrder = gl::VariableSortOrder(lhs.type);
        int rhsSortOrder = gl::VariableSortOrder(rhs.type);
        if (lhsSortOrder != rhsSortOrder) {
            return lhsSortOrder < rhsSortOrder;
        }
        // Sort by largest first.
        return lhs.arraySize > rhs.arraySize;
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

template <typename VarT>
bool VariablePacker::CheckVariablesWithinPackingLimits(unsigned int maxVectors,
                                                       const std::vector<VarT> &in_variables)
{
    ASSERT(maxVectors > 0);
    maxRows_ = maxVectors;
    topNonFullRow_ = 0;
    bottomNonFullRow_ = maxRows_ - 1;
    std::vector<VarT> variables(in_variables);

    // Check whether each variable fits in the available vectors.
    for (size_t i = 0; i < variables.size(); i++) {
        const sh::ShaderVariable &variable = variables[i];
        if (variable.elementCount() > maxVectors / GetNumRows(variable.type)) {
            return false;
        }
    }

    // As per GLSL 1.017 Appendix A, Section 7 variables are packed in specific
    // order by type, then by size of array, largest first.
    std::sort(variables.begin(), variables.end(), TVariableInfoComparer());
    rows_.clear();
    rows_.resize(maxVectors, 0);

    // Packs the 4 column variables.
    size_t ii = 0;
    for (; ii < variables.size(); ++ii) {
        const sh::ShaderVariable &variable = variables[ii];
        if (GetNumComponentsPerRow(variable.type) != 4) {
            break;
        }
        topNonFullRow_ += GetNumRows(variable.type) * variable.elementCount();
    }

    if (topNonFullRow_ > maxRows_) {
        return false;
    }

    // Packs the 3 column variables.
    int num3ColumnRows = 0;
    for (; ii < variables.size(); ++ii) {
        const sh::ShaderVariable &variable = variables[ii];
        if (GetNumComponentsPerRow(variable.type) != 3) {
            break;
        }
        num3ColumnRows += GetNumRows(variable.type) * variable.elementCount();
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
        const sh::ShaderVariable &variable = variables[ii];
        if (GetNumComponentsPerRow(variable.type) != 2) {
            break;
        }
        int numRows = GetNumRows(variable.type) * variable.elementCount();
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
        const sh::ShaderVariable &variable = variables[ii];
        ASSERT(1 == GetNumComponentsPerRow(variable.type));
        int numRows = GetNumRows(variable.type) * variable.elementCount();
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

// Instantiate all possible variable packings
template bool VariablePacker::CheckVariablesWithinPackingLimits(unsigned int, const std::vector<sh::ShaderVariable> &);
template bool VariablePacker::CheckVariablesWithinPackingLimits(unsigned int, const std::vector<sh::Attribute> &);
template bool VariablePacker::CheckVariablesWithinPackingLimits(unsigned int, const std::vector<sh::Uniform> &);
template bool VariablePacker::CheckVariablesWithinPackingLimits(unsigned int, const std::vector<sh::Varying> &);
