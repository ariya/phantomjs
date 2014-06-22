/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef ExpressionRangeInfo_h
#define ExpressionRangeInfo_h

#include <wtf/StdLibExtras.h>

namespace JSC {

struct ExpressionRangeInfo {
    // Line and column values are encoded in 1 of 3 modes depending on the size
    // of their values. These modes are:
    //
    //   1. FatLine: 22-bit line, 8-bit column.
    //   2. FatColumn: 8-bit line, 22-bit column.
    //   3. FatLineAndColumn: 32-bit line, 32-bit column.
    //
    // For the first 2 modes, the line and column will be encoded in the 30-bit
    // position field in the ExpressionRangeInfo. For the FatLineAndColumn mode,
    // the position field will hold an index into a FatPosition vector which
    // holds the FatPosition records with the full 32-bit line and column values.

    enum {
        FatLineMode,
        FatColumnMode,
        FatLineAndColumnMode
    };

    struct FatPosition {
        uint32_t line;
        uint32_t column;
    };

    enum {
        FatLineModeLineShift = 8,
        FatLineModeLineMask = (1 << 22) - 1,
        FatLineModeColumnMask = (1 << 8) - 1,
        FatColumnModeLineShift = 22,
        FatColumnModeLineMask = (1 << 8) - 1,
        FatColumnModeColumnMask = (1 << 22) - 1
    };

    enum {
        MaxOffset = (1 << 7) - 1, 
        MaxDivot = (1 << 25) - 1,
        MaxFatLineModeLine = (1 << 22) - 1,
        MaxFatLineModeColumn = (1 << 8) - 1,
        MaxFatColumnModeLine = (1 << 8) - 1,
        MaxFatColumnModeColumn = (1 << 22) - 1
    };

    void encodeFatLineMode(unsigned line, unsigned column)
    {
        ASSERT(line <= MaxFatLineModeLine);
        ASSERT(column <= MaxFatLineModeColumn);
        position = ((line & FatLineModeLineMask) << FatLineModeLineShift | (column & FatLineModeColumnMask));
    }

    void encodeFatColumnMode(unsigned line, unsigned column)
    {
        ASSERT(line <= MaxFatColumnModeLine);
        ASSERT(column <= MaxFatColumnModeColumn);
        position = ((line & FatColumnModeLineMask) << FatColumnModeLineShift | (column & FatColumnModeColumnMask));
    }

    void decodeFatLineMode(unsigned& line, unsigned& column)
    {
        line = (position >> FatLineModeLineShift) & FatLineModeLineMask;
        column = position & FatLineModeColumnMask;
    }

    void decodeFatColumnMode(unsigned& line, unsigned& column)
    {
        line = (position >> FatColumnModeLineShift) & FatColumnModeLineMask;
        column = position & FatColumnModeColumnMask;
    }

    uint32_t instructionOffset : 25;
    uint32_t startOffset : 7;
    uint32_t divotPoint : 25;
    uint32_t endOffset : 7;
    uint32_t mode : 2;
    uint32_t position : 30;
};

} // namespace JSC

#endif // ExpressionRangeInfo_h

