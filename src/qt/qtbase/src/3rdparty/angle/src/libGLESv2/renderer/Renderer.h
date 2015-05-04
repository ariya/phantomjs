//
// Copyright (c) 2012-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Renderer.h: Defines a back-end specific class that hides the details of the
// implementation-specific renderer.

#ifndef LIBGLESV2_RENDERER_RENDERER_H_
#define LIBGLESV2_RENDERER_RENDERER_H_

#include "libGLESv2/Caps.h"
#include "libGLESv2/Error.h"
#include "libGLESv2/Uniform.h"
#include "libGLESv2/angletypes.h"
#include "libGLESv2/renderer/Workarounds.h"
#include "common/NativeWindow.h"
#include "common/mathutil.h"

#include <cstdint>

#include <EGL/egl.h>

#if !defined(ANGLE_COMPILE_OPTIMIZATION_LEVEL)
// WARNING: D3DCOMPILE_OPTIMIZATION_LEVEL3 may lead to a DX9 shader compiler hang.
// It should only be used selectively to work around specific bugs.
#define ANGLE_COMPILE_OPTIMIZATION_LEVEL D3DCOMPILE_OPTIMIZATION_LEVEL1
#endif

namespace egl
{
class Display;
}

namespace gl
{
class Buffer;
class Framebuffer;
struct Data;
}

namespace rx
{
class QueryImpl;
class FenceNVImpl;
class FenceSyncImpl;
class BufferImpl;
class VertexArrayImpl;
class ShaderImpl;
class ProgramImpl;
class TextureImpl;
class TransformFeedbackImpl;
class RenderbufferImpl;
struct TranslatedIndexData;
struct Workarounds;
class SwapChain;

struct ConfigDesc
{
    GLenum  renderTargetFormat;
    GLenum  depthStencilFormat;
    GLint   multiSample;
    bool    fastConfig;
    bool    es3Capable;
};

class Renderer
{
  public:
    Renderer();
    virtual ~Renderer();

    virtual EGLint initialize() = 0;
    virtual bool resetDevice() = 0;

    virtual int generateConfigs(ConfigDesc **configDescList) = 0;
    virtual void deleteConfigs(ConfigDesc *configDescList) = 0;

    virtual gl::Error sync(bool block) = 0;

    virtual gl::Error drawArrays(const gl::Data &data, GLenum mode,
                                 GLint first, GLsizei count, GLsizei instances) = 0;
    virtual gl::Error drawElements(const gl::Data &data, GLenum mode, GLsizei count, GLenum type,
                                   const GLvoid *indices, GLsizei instances,
                                   const RangeUI &indexRange) = 0;

    virtual gl::Error clear(const gl::Data &data, GLbitfield mask) = 0;
    virtual gl::Error clearBufferfv(const gl::Data &data, GLenum buffer, GLint drawbuffer, const GLfloat *values) = 0;
    virtual gl::Error clearBufferuiv(const gl::Data &data, GLenum buffer, GLint drawbuffer, const GLuint *values) = 0;
    virtual gl::Error clearBufferiv(const gl::Data &data, GLenum buffer, GLint drawbuffer, const GLint *values) = 0;
    virtual gl::Error clearBufferfi(const gl::Data &data, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) = 0;

    virtual gl::Error readPixels(const gl::Data &data, GLint x, GLint y, GLsizei width, GLsizei height,
                                 GLenum format, GLenum type, GLsizei *bufSize, void* pixels) = 0;

    virtual gl::Error blitFramebuffer(const gl::Data &data,
                                      GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                                      GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                                      GLbitfield mask, GLenum filter) = 0;

    // TODO(jmadill): caps? and virtual for egl::Display
    virtual bool getShareHandleSupport() const = 0;
    virtual bool getPostSubBufferSupport() const = 0;

    // Shader creation
    virtual ShaderImpl *createShader(const gl::Data &data, GLenum type) = 0;
    virtual ProgramImpl *createProgram() = 0;

    // Shader operations
    virtual void releaseShaderCompiler() = 0;

    // Texture creation
    virtual TextureImpl *createTexture(GLenum target) = 0;

    // Renderbuffer creation
    virtual RenderbufferImpl *createRenderbuffer() = 0;
    virtual RenderbufferImpl *createRenderbuffer(SwapChain *swapChain, bool depth) = 0;

    // Buffer creation
    virtual BufferImpl *createBuffer() = 0;

    // Vertex Array creation
    virtual VertexArrayImpl *createVertexArray() = 0;

    // Query and Fence creation
    virtual QueryImpl *createQuery(GLenum type) = 0;
    virtual FenceNVImpl *createFenceNV() = 0;
    virtual FenceSyncImpl *createFenceSync() = 0;

    // Transform Feedback creation
    virtual TransformFeedbackImpl *createTransformFeedback() = 0;

    // lost device
    //TODO(jmadill): investigate if this stuff is necessary in GL
    virtual void notifyDeviceLost() = 0;
    virtual bool isDeviceLost() = 0;
    virtual bool testDeviceLost(bool notify) = 0;
    virtual bool testDeviceResettable() = 0;

    virtual DWORD getAdapterVendor() const = 0;
    virtual std::string getRendererDescription() const = 0;
    virtual GUID getAdapterIdentifier() const = 0;

    // Renderer capabilities (virtual because of egl::Display)
    virtual const gl::Caps &getRendererCaps() const;
    const gl::TextureCapsMap &getRendererTextureCaps() const;
    virtual const gl::Extensions &getRendererExtensions() const;
    const Workarounds &getWorkarounds() const;

    // TODO(jmadill): needed by egl::Display, probably should be removed
    virtual int getMajorShaderModel() const = 0;
    virtual int getMinSwapInterval() const = 0;
    virtual int getMaxSwapInterval() const = 0;
    virtual bool getLUID(LUID *adapterLuid) const = 0;

  private:
    DISALLOW_COPY_AND_ASSIGN(Renderer);

    virtual void generateCaps(gl::Caps *outCaps, gl::TextureCapsMap* outTextureCaps, gl::Extensions *outExtensions) const = 0;
    virtual Workarounds generateWorkarounds() const = 0;

    mutable bool mCapsInitialized;
    mutable gl::Caps mCaps;
    mutable gl::TextureCapsMap mTextureCaps;
    mutable gl::Extensions mExtensions;

    mutable bool mWorkaroundsInitialized;
    mutable Workarounds mWorkarounds;
};

struct dx_VertexConstants
{
    float depthRange[4];
    float viewAdjust[4];
};

struct dx_PixelConstants
{
    float depthRange[4];
    float viewCoords[4];
    float depthFront[4];
};

enum ShaderType
{
    SHADER_VERTEX,
    SHADER_PIXEL,
    SHADER_GEOMETRY
};

}
#endif // LIBGLESV2_RENDERER_RENDERER_H_
