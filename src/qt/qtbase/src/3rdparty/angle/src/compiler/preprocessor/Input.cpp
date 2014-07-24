//
// Copyright (c) 2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "Input.h"

#include <algorithm>
#include <cassert>
#include <cstring>

namespace pp
{

Input::Input() : mCount(0), mString(0)
{
}

Input::Input(size_t count, const char* const string[], const int length[]) :
    mCount(count),
    mString(string)
{
    mLength.reserve(mCount);
    for (size_t i = 0; i < mCount; ++i)
    {
        int len = length ? length[i] : -1;
        mLength.push_back(len < 0 ? std::strlen(mString[i]) : len);
    }
}

size_t Input::read(char* buf, size_t maxSize)
{
    size_t nRead = 0;
    while ((nRead < maxSize) && (mReadLoc.sIndex < mCount))
    {
        size_t size = mLength[mReadLoc.sIndex] - mReadLoc.cIndex;
        size = std::min(size, maxSize);
        std::memcpy(buf + nRead, mString[mReadLoc.sIndex] + mReadLoc.cIndex, size);
        nRead += size;
        mReadLoc.cIndex += size;

        // Advance string if we reached the end of current string.
        if (mReadLoc.cIndex == mLength[mReadLoc.sIndex])
        {
            ++mReadLoc.sIndex;
            mReadLoc.cIndex = 0;
        }
    }
    return nRead;
}

}  // namespace pp

