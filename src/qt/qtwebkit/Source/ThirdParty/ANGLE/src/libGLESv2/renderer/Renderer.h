//
// Copyright (c) 2012-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Renderer.h: Defines a back-end specific class that hides the details of the
// implementation-specific renderer.

#ifndef LIBGLESV2_RENDERER_RENDERER_H_
#define LIBGLESV2_RENDERER_RENDERER_H_

#include "libGLESv2/Uniform.h"
#include "libGLESv2/angletypes.h"

#if !defined(ANGLE_COMPILE_OPTIMIZATION_LEVEL)
#define ANGLE_COMPILE_OPTIMIZATION_LEVEL D3DCOMPILE_OPTIMIZATION_LEVEL3
#endif

const int versionWindowsVista = MAKEWORD(0x00, 0x06);
const int versionWindows7 = MAKEWORD(0x01, 0x06);

// Return the version of the operating system in a format suitable for ordering
// comparison.
inline int getComparableOSVersion()
{
    DWORD version = GetVersion();
    int majorVersion = LOBYTE(LOWORD(version));
    int minorVersion = HIBYTE(LOWORD(version));
    return MAKEWORD(minorVersion, majorVersion);
}

namespace egl
{
class Display;
}

namespace gl
{
class InfoLog;
class ProgramBinary;
class VertexAttribute;
class Buffer;
class Texture;
class Framebuffer;
}

namespace rx
{
class TextureStorageInterface2D;
class TextureStorageInterfaceCube;
class VertexBuffer;
class IndexBuffer;
class QueryImpl;
class FenceImpl;
class BufferStorage;
class Blit;
struct TranslatedIndexData;
class ShaderExecutable;
class SwapChain;
class RenderTarget;
class Image;
class TextureStorage;

typedef void * ShaderBlob;
typedef void (*pCompileFunc)();

struct ConfigDesc
{
    GLenum  renderTargetFormat;
    GLenum  depthStencilFormat;
    GLint   multiSample;
    bool    fastConfig;
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

class Renderer
{
  public:
    explicit Renderer(egl::Display *display);
    virtual ~Renderer();

    virtual EGLint initialize() = 0;
    virtual bool resetDevice() = 0;

    virtual int generateConfigs(ConfigDesc **configDescList) = 0;
    virtual void deleteConfigs(ConfigDesc *configDescList) = 0;

    virtual void sync(bool block) = 0;

    virtual SwapChain *createSwapChain(HWND window, HANDLE shareHandle, GLenum backBufferFormat, GLenum depthBufferFormat) = 0;

    virtual void setSamplerState(gl::SamplerType type, int index, const gl::SamplerState &sampler) = 0;
    virtual void setTexture(gl::SamplerType type, int index, gl::Texture *texture) = 0;

    virtual void setRasterizerState(const gl::RasterizerState &rasterState) = 0;
    virtual void setBlendState(const gl::BlendState &blendState, const gl::Color &blendColor,
                               unsigned int sampleMask) = 0;
    virtual void setDepthStencilState(const gl::DepthStencilState &depthStencilState, int stencilRef,
                                      int stencilBackRef, bool frontFaceCCW) = 0;

    virtual void setScissorRectangle(const gl::Rectangle &scissor, bool enabled) = 0;
    virtual bool setViewport(const gl::Rectangle &viewport, float zNear, float zFar, GLenum drawMode, GLenum frontFace,
                             bool ignoreViewport) = 0;

    virtual bool applyRenderTarget(gl::Framebuffer *frameBuffer) = 0;
    virtual void applyShaders(gl::ProgramBinary *programBinary) = 0;
    virtual void applyUniforms(gl::ProgramBinary *programBinary, gl::UniformArray *uniformArray) = 0;
    virtual bool applyPrimitiveType(GLenum primitiveType, GLsizei elementCount) = 0;
    virtual GLenum applyVertexBuffer(gl::ProgramBinary *programBinary, gl::VertexAttribute vertexAttributes[], GLint first, GLsizei count, GLsizei instances) = 0;
    virtual GLenum applyIndexBuffer(const GLvoid *indices, gl::Buffer *elementArrayBuffer, GLsizei count, GLenum mode, GLenum type, TranslatedIndexData *indexInfo) = 0;

    virtual void drawArrays(GLenum mode, GLsizei count, GLsizei instances) = 0;
    virtual void drawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, gl::Buffer *elementArrayBuffer, const TranslatedIndexData &indexInfo, GLsizei instances) = 0;

    virtual void clear(const gl::ClearParameters &clearParams, gl::Framebuffer *frameBuffer) = 0;

    virtual void markAllStateDirty() = 0;

    // lost device
    virtual void notifyDeviceLost() = 0;
    virtual bool isDeviceLost() = 0;
    virtual bool testDeviceLost(bool notify) = 0;
    virtual bool testDeviceResettable() = 0;

    // Renderer capabilities
    virtual DWORD getAdapterVendor() const = 0;
    virtual std::string getRendererDescription() const = 0;
    virtual GUID getAdapterIdentifier() const = 0;

    virtual bool getBGRATextureSupport() const = 0;
    virtual bool getDXT1TextureSupport() = 0;
    virtual bool getDXT3TextureSupport() = 0;
    virtual bool getDXT5TextureSupport() = 0;
    virtual bool getEventQuerySupport() = 0;
    virtual bool getFloat32TextureSupport(bool *filtering, bool *renderable) = 0;
    virtual bool getFloat16TextureSupport(bool *filtering, bool *renderable) = 0;
    virtual bool getLuminanceTextureSupport() = 0;
    virtual bool getLuminanceAlphaTextureSupport() = 0;
    bool getVertexTextureSupport() const { return getMaxVertexTextureImageUnits() > 0; }
    virtual unsigned int getMaxVertexTextureImageUnits() const = 0;
    virtual unsigned int getMaxCombinedTextureImageUnits() const = 0;
    virtual unsigned int getReservedVertexUniformVectors() const = 0;
    virtual unsigned int getReservedFragmentUniformVectors() const = 0;
    virtual unsigned int getMaxVertexUniformVectors() const = 0;
    virtual unsigned int getMaxFragmentUniformVectors() const = 0;
    virtual unsigned int getMaxVaryingVectors() const = 0;
    virtual bool getNonPower2TextureSupport() const = 0;
    virtual bool getDepthTextureSupport() const = 0;
    virtual bool getOcclusionQuerySupport() const = 0;
    virtual bool getInstancingSupport() const = 0;
    virtual bool getTextureFilterAnisotropySupport() const = 0;
    virtual float getTextureMaxAnisotropy() const = 0;
    virtual bool getShareHandleSupport() const = 0;
    virtual bool getDerivativeInstructionSupport() const = 0;
    virtual bool getPostSubBufferSupport() const = 0;

    virtual int getMajorShaderModel() const = 0;
    virtual float getMaxPointSize() const = 0;
    virtual int getMaxViewportDimension() const = 0;
    virtual int getMaxTextureWidth() const = 0;
    virtual int getMaxTextureHeight() const = 0;
    virtual bool get32BitIndexSupport() const = 0;
    virtual int getMinSwapInterval() const = 0;
    virtual int getMaxSwapInterval() const = 0;

    virtual GLsizei getMaxSupportedSamples() const = 0;

    virtual unsigned int getMaxRenderTargets() const = 0;

    // Pixel operations
    virtual bool copyToRenderTarget(TextureStorageInterface2D *dest, TextureStorageInterface2D *source) = 0;
    virtual bool copyToRenderTarget(TextureStorageInterfaceCube *dest, TextureStorageInterfaceCube *source) = 0;

    virtual bool copyImage(gl::Framebuffer *framebuffer, const gl::Rectangle &sourceRect, GLenum destFormat,
                           GLint xoffset, GLint yoffset, TextureStorageInterface2D *storage, GLint level) = 0;
    virtual bool copyImage(gl::Framebuffer *framebuffer, const gl::Rectangle &sourceRect, GLenum destFormat,
                           GLint xoffset, GLint yoffset, TextureStorageInterfaceCube *storage, GLenum target, GLint level) = 0;

    virtual bool blitRect(gl::Framebuffer *readTarget, const gl::Rectangle &readRect, gl::Framebuffer *drawTarget, const gl::Rectangle &drawRect,
                          bool blitRenderTarget, bool blitDepthStencil) = 0;
    virtual void readPixels(gl::Framebuffer *framebuffer, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type,
                            GLsizei outputPitch, bool packReverseRowOrder, GLint packAlignment, void* pixels) = 0;

    // RenderTarget creation
    virtual RenderTarget *createRenderTarget(SwapChain *swapChain, bool depth) = 0;
    virtual RenderTarget *createRenderTarget(int width, int height, GLenum format, GLsizei samples, bool depth) = 0;

    // Shader operations
    virtual ShaderExecutable *loadExecutable(const void *function, size_t length, rx::ShaderType type) = 0;
    virtual ShaderExecutable *compileToExecutable(gl::InfoLog &infoLog, const char *shaderHLSL, rx::ShaderType type) = 0;

    // Image operations
    virtual Image *createImage() = 0;
    virtual void generateMipmap(Image *dest, Image *source) = 0;
    virtual TextureStorage *createTextureStorage2D(SwapChain *swapChain) = 0;
    virtual TextureStorage *createTextureStorage2D(int levels, GLenum internalformat, GLenum usage, bool forceRenderable, GLsizei width, GLsizei height) = 0;
    virtual TextureStorage *createTextureStorageCube(int levels, GLenum internalformat, GLenum usage, bool forceRenderable, int size) = 0;

    // Buffer creation
    virtual VertexBuffer *createVertexBuffer() = 0;
    virtual IndexBuffer *createIndexBuffer() = 0;
    virtual BufferStorage *createBufferStorage() = 0;

    // Query and Fence creation
    virtual QueryImpl *createQuery(GLenum type) = 0;
    virtual FenceImpl *createFence() = 0;

    virtual bool getLUID(LUID *adapterLuid) const = 0;

  protected:
    bool initializeCompiler();
    ShaderBlob *compileToBinary(gl::InfoLog &infoLog, const char *hlsl, const char *profile, UINT optimizationFlags, bool alternateFlags);

    egl::Display *mDisplay;

  private:
    DISALLOW_COPY_AND_ASSIGN(Renderer);

    HMODULE mD3dCompilerModule;
    pCompileFunc mD3DCompileFunc;
};

}
#endif // LIBGLESV2_RENDERER_RENDERER_H_
