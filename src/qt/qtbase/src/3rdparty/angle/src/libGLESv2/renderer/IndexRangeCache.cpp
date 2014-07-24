#include "precompiled.h"
//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// IndexRangeCache.cpp: Defines the rx::IndexRangeCache class which stores information about
// ranges of indices.

#include "libGLESv2/renderer/IndexRangeCache.h"
#include "common/debug.h"
#include "libGLESv2/utilities.h"
#include <tuple>

namespace rx
{

void IndexRangeCache::addRange(GLenum type, unsigned int offset, GLsizei count, unsigned int minIdx, unsigned int maxIdx,
                               unsigned int streamOffset)
{
    mIndexRangeCache[IndexRange(type, offset, count)] = IndexBounds(minIdx, maxIdx, streamOffset);
}

void IndexRangeCache::invalidateRange(unsigned int offset, unsigned int size)
{
    unsigned int invalidateStart = offset;
    unsigned int invalidateEnd = offset + size;

    IndexRangeMap::iterator i = mIndexRangeCache.begin();
    while (i != mIndexRangeCache.end())
    {
        unsigned int rangeStart = i->second.streamOffset;
        unsigned int rangeEnd = i->second.streamOffset + (gl::ComputeTypeSize(i->first.type) * i->first.count);

        if (invalidateEnd < rangeStart || invalidateStart > rangeEnd)
        {
            ++i;
        }
        else
        {
            i = mIndexRangeCache.erase(i);
        }
    }
}

bool IndexRangeCache::findRange(GLenum type, unsigned int offset, GLsizei count, unsigned int *outMinIndex,
                                unsigned int *outMaxIndex, unsigned int *outStreamOffset) const
{
    IndexRangeMap::const_iterator i = mIndexRangeCache.find(IndexRange(type, offset, count));
    if (i != mIndexRangeCache.end())
    {
        if (outMinIndex)     *outMinIndex = i->second.minIndex;
        if (outMaxIndex)     *outMaxIndex = i->second.maxIndex;
        if (outStreamOffset) *outStreamOffset = i->second.streamOffset;
        return true;
    }
    else
    {
        if (outMinIndex)     *outMinIndex = 0;
        if (outMaxIndex)     *outMaxIndex = 0;
        if (outStreamOffset) *outStreamOffset = 0;
        return false;
    }
}

void IndexRangeCache::clear()
{
    mIndexRangeCache.clear();
}

IndexRangeCache::IndexRange::IndexRange()
    : type(GL_NONE), offset(0), count(0)
{
}

IndexRangeCache::IndexRange::IndexRange(GLenum typ, intptr_t off, GLsizei c)
    : type(typ), offset(off), count(c)
{
}

bool IndexRangeCache::IndexRange::operator<(const IndexRange& rhs) const
{
#if defined(_MSC_VER) && _MSC_VER < 1600
    return std::tr1::make_tuple(type, offset, count) < std::tr1::make_tuple(rhs.type, rhs.offset, rhs.count);
#else
    return std::make_tuple(type, offset, count) < std::make_tuple(rhs.type, rhs.offset, rhs.count);
#endif
}

IndexRangeCache::IndexBounds::IndexBounds()
    : minIndex(0), maxIndex(0), streamOffset(0)
{
}

IndexRangeCache::IndexBounds::IndexBounds(unsigned int minIdx, unsigned int maxIdx, unsigned int offset)
    : minIndex(minIdx), maxIndex(maxIdx), streamOffset(offset)
{
}

}
