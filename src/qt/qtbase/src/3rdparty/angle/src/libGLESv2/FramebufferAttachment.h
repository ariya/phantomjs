//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// FramebufferAttachment.h: Defines the wrapper class gl::FramebufferAttachment, as well as the
// objects and related functionality. [OpenGL ES 2.0.24] section 4.4.3 page 108.

#ifndef LIBGLESV2_FRAMEBUFFERATTACHMENT_H_
#define LIBGLESV2_FRAMEBUFFERATTACHMENT_H_

#include "common/angleutils.h"
#include "common/RefCountObject.h"
#include "Texture.h"

#include "angle_gl.h"

namespace gl
{
class Renderbuffer;

// FramebufferAttachment implements a GL framebuffer attachment.
// Attachments are "light" containers, which store pointers to ref-counted GL objects.
// We support GL texture (2D/3D/Cube/2D array) and renderbuffer object attachments.
// Note: Our old naming scheme used the term "Renderbuffer" for both GL renderbuffers and for
// framebuffer attachments, which confused their usage.

class FramebufferAttachment
{
  public:
    explicit FramebufferAttachment(GLenum binding);
    virtual ~FramebufferAttachment();

    // Helper methods
    GLuint getRedSize() const;
    GLuint getGreenSize() const;
    GLuint getBlueSize() const;
    GLuint getAlphaSize() const;
    GLuint getDepthSize() const;
    GLuint getStencilSize() const;
    GLenum getComponentType() const;
    GLenum getColorEncoding() const;
    bool isTexture() const;

    bool isTextureWithId(GLuint textureId) const { return isTexture() && id() == textureId; }
    bool isRenderbufferWithId(GLuint renderbufferId) const { return !isTexture() && id() == renderbufferId; }

    GLenum getBinding() const { return mBinding; }

    // Child class interface
    virtual GLsizei getWidth() const = 0;
    virtual GLsizei getHeight() const = 0;
    virtual GLenum getInternalFormat() const = 0;
    virtual GLenum getActualFormat() const = 0;
    virtual GLsizei getSamples() const = 0;

    virtual GLuint id() const = 0;
    virtual GLenum type() const = 0;
    virtual GLint mipLevel() const = 0;
    virtual GLint layer() const = 0;

    virtual Texture *getTexture() = 0;
    virtual const ImageIndex *getTextureImageIndex() const = 0;
    virtual Renderbuffer *getRenderbuffer() = 0;

  private:
    DISALLOW_COPY_AND_ASSIGN(FramebufferAttachment);

    GLenum mBinding;
};

class TextureAttachment : public FramebufferAttachment
{
  public:
    TextureAttachment(GLenum binding, Texture *texture, const ImageIndex &index);
    virtual ~TextureAttachment();

    virtual GLsizei getSamples() const;
    virtual GLuint id() const;

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLenum getActualFormat() const;

    virtual GLenum type() const;
    virtual GLint mipLevel() const;
    virtual GLint layer() const;

    virtual Texture *getTexture();
    virtual const ImageIndex *getTextureImageIndex() const;
    virtual Renderbuffer *getRenderbuffer();

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureAttachment);

    BindingPointer<Texture> mTexture;
    ImageIndex mIndex;
};

class RenderbufferAttachment : public FramebufferAttachment
{
  public:
    RenderbufferAttachment(GLenum binding, Renderbuffer *renderbuffer);

    virtual ~RenderbufferAttachment();

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLenum getActualFormat() const;
    virtual GLsizei getSamples() const;

    virtual GLuint id() const;
    virtual GLenum type() const;
    virtual GLint mipLevel() const;
    virtual GLint layer() const;

    virtual Texture *getTexture();
    virtual const ImageIndex *getTextureImageIndex() const;
    virtual Renderbuffer *getRenderbuffer();

  private:
    DISALLOW_COPY_AND_ASSIGN(RenderbufferAttachment);

    BindingPointer<Renderbuffer> mRenderbuffer;
};

}

#endif // LIBGLESV2_FRAMEBUFFERATTACHMENT_H_
