//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef _VARIABLEPACKER_INCLUDED_
#define _VARIABLEPACKER_INCLUDED_

#include <vector>
#include "compiler/ShHandle.h"

class VariablePacker {
 public:
    // Returns true if the passed in variables pack in maxVectors following
    // the packing rules from the GLSL 1.017 spec, Appendix A, section 7.
    bool CheckVariablesWithinPackingLimits(
        int maxVectors,
        const TVariableInfoList& in_variables);

    // Gets how many components in a row a data type takes.
    static int GetNumComponentsPerRow(ShDataType type);

    // Gets how many rows a data type takes.
    static int GetNumRows(ShDataType type);

 private:
    static const int kNumColumns = 4;
    static const unsigned kColumnMask = (1 << kNumColumns) - 1;

    unsigned makeColumnFlags(int column, int numComponentsPerRow);
    void fillColumns(int topRow, int numRows, int column, int numComponentsPerRow);
    bool searchColumn(int column, int numRows, int* destRow, int* destSize);

    int topNonFullRow_;
    int bottomNonFullRow_;
    int maxRows_;
    std::vector<unsigned> rows_;
};

#endif // _VARIABLEPACKER_INCLUDED_
