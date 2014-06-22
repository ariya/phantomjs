//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// IndexRangeCache.h: Defines the rx::IndexRangeCache class which stores information about
// ranges of indices.

#ifndef LIBGLESV2_RENDERER_INDEXRANGECACHE_H_
#define LIBGLESV2_RENDERER_INDEXRANGECACHE_H_

#include "common/angleutils.h"

namespace rx
{

class IndexRangeCache
{
  public:
    void addRange(GLenum type, unsigned int offset, GLsizei count, unsigned int minIdx, unsigned int maxIdx,
                  unsigned int streamOffset);
    bool findRange(GLenum type, unsigned int offset, GLsizei count, unsigned int *outMinIndex,
                   unsigned int *outMaxIndex, unsigned int *outStreamOffset) const;

    void invalidateRange(unsigned int offset, unsigned int size);
    void clear();

  private:
    struct IndexRange
    {
        GLenum type;
        unsigned int offset;
        GLsizei count;

        IndexRange();
        IndexRange(GLenum type, intptr_t offset, GLsizei count);

        bool operator<(const IndexRange& rhs) const;
    };

    struct IndexBounds
    {
        unsigned int minIndex;
        unsigned int maxIndex;
        unsigned int streamOffset;

        IndexBounds();
        IndexBounds(unsigned int minIdx, unsigned int maxIdx, unsigned int offset);
    };

    typedef std::map<IndexRange, IndexBounds> IndexRangeMap;
    IndexRangeMap mIndexRangeCache;
};

}

#endif LIBGLESV2_RENDERER_INDEXRANGECACHE_H
