//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Image9.h: Defines the rx::Image9 class, which acts as the interface to
// the actual underlying surfaces of a Texture.

#ifndef LIBGLESV2_RENDERER_IMAGE9_H_
#define LIBGLESV2_RENDERER_IMAGE9_H_

#include "libGLESv2/renderer/Image.h"
#include "common/debug.h"

namespace gl
{
class Framebuffer;
}

namespace rx
{
class Renderer;
class Renderer9;
class TextureStorageInterface2D;
class TextureStorageInterfaceCube;

class Image9 : public Image
{
  public:
    Image9();
    ~Image9();

    static Image9 *makeImage9(Image *img);

    static void generateMipmap(Image9 *dest, Image9 *source);
    static void generateMip(IDirect3DSurface9 *destSurface, IDirect3DSurface9 *sourceSurface);
    static void copyLockableSurfaces(IDirect3DSurface9 *dest, IDirect3DSurface9 *source);

    virtual bool redefine(Renderer *renderer, GLint internalformat, GLsizei width, GLsizei height, bool forceRelease);

    virtual bool isRenderableFormat() const;
    D3DFORMAT getD3DFormat() const;

    virtual bool isDirty() const {return mSurface && mDirty;}
    IDirect3DSurface9 *getSurface();

    virtual void setManagedSurface(TextureStorageInterface2D *storage, int level);
    virtual void setManagedSurface(TextureStorageInterfaceCube *storage, int face, int level);
    virtual bool updateSurface(TextureStorageInterface2D *storage, int level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height);
    virtual bool updateSurface(TextureStorageInterfaceCube *storage, int face, int level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height);

    virtual void loadData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                  GLint unpackAlignment, const void *input);
    virtual void loadCompressedData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                    const void *input);

    virtual void copy(GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source);

  private:
    DISALLOW_COPY_AND_ASSIGN(Image9);

    void createSurface();
    void setManagedSurface(IDirect3DSurface9 *surface);
    bool updateSurface(IDirect3DSurface9 *dest, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height);

    HRESULT lock(D3DLOCKED_RECT *lockedRect, const RECT *rect);
    void unlock();
    
    Renderer9 *mRenderer;

    D3DPOOL mD3DPool;   // can only be D3DPOOL_SYSTEMMEM or D3DPOOL_MANAGED since it needs to be lockable.
    D3DFORMAT mD3DFormat;

    IDirect3DSurface9 *mSurface;
};
}

#endif   // LIBGLESV2_RENDERER_IMAGE9_H_
