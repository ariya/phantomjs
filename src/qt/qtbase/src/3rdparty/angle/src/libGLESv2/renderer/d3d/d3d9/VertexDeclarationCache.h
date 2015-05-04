//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexDeclarationCache.h: Defines a helper class to construct and cache vertex declarations.

#ifndef LIBGLESV2_RENDERER_VERTEXDECLARATIONCACHE_H_
#define LIBGLESV2_RENDERER_VERTEXDECLARATIONCACHE_H_

#include "libGLESv2/Error.h"
#include "libGLESv2/renderer/d3d/VertexDataManager.h"

namespace gl
{
class VertexDataManager;
}

namespace rx
{

class VertexDeclarationCache
{
  public:
    VertexDeclarationCache();
    ~VertexDeclarationCache();

    gl::Error applyDeclaration(IDirect3DDevice9 *device, TranslatedAttribute attributes[], gl::ProgramBinary *programBinary, GLsizei instances, GLsizei *repeatDraw);

    void markStateDirty();

  private:
    UINT mMaxLru;

    enum { NUM_VERTEX_DECL_CACHE_ENTRIES = 32 };

    struct VBData
    {
        unsigned int serial;
        unsigned int stride;
        unsigned int offset;
    };

    VBData mAppliedVBs[gl::MAX_VERTEX_ATTRIBS];
    IDirect3DVertexDeclaration9 *mLastSetVDecl;
    bool mInstancingEnabled;

    struct VertexDeclCacheEntry
    {
        D3DVERTEXELEMENT9 cachedElements[gl::MAX_VERTEX_ATTRIBS + 1];
        UINT lruCount;
        IDirect3DVertexDeclaration9 *vertexDeclaration;
    } mVertexDeclCache[NUM_VERTEX_DECL_CACHE_ENTRIES];
};

}

#endif // LIBGLESV2_RENDERER_VERTEXDECLARATIONCACHE_H_
