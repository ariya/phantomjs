//
// Copyright (c) 2012-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Renderer9.h: Defines a back-end specific class for the D3D9 renderer.

#ifndef LIBGLESV2_RENDERER_RENDERER9_H_
#define LIBGLESV2_RENDERER_RENDERER9_H_

#include "common/angleutils.h"
#include "common/mathutil.h"
#include "libGLESv2/renderer/d3d/d3d9/ShaderCache.h"
#include "libGLESv2/renderer/d3d/d3d9/VertexDeclarationCache.h"
#include "libGLESv2/renderer/d3d/HLSLCompiler.h"
#include "libGLESv2/renderer/d3d/RendererD3D.h"
#include "libGLESv2/renderer/RenderTarget.h"

namespace gl
{
class FramebufferAttachment;
}

namespace egl
{
class AttributeMap;
}

namespace rx
{
class VertexDataManager;
class IndexDataManager;
class StreamingIndexBufferInterface;
class StaticIndexBufferInterface;
struct TranslatedAttribute;
class Blit9;

class Renderer9 : public RendererD3D
{
  public:
    Renderer9(egl::Display *display, EGLNativeDisplayType hDc, const egl::AttributeMap &attributes);
    virtual ~Renderer9();

    static Renderer9 *makeRenderer9(Renderer *renderer);

    virtual EGLint initialize();
    virtual bool resetDevice();

    virtual int generateConfigs(ConfigDesc **configDescList);
    virtual void deleteConfigs(ConfigDesc *configDescList);

    void startScene();
    void endScene();

    virtual gl::Error sync(bool block);

    virtual SwapChain *createSwapChain(NativeWindow nativeWindow, HANDLE shareHandle, GLenum backBufferFormat, GLenum depthBufferFormat);

    gl::Error allocateEventQuery(IDirect3DQuery9 **outQuery);
    void freeEventQuery(IDirect3DQuery9* query);

    // resource creation
    gl::Error createVertexShader(const DWORD *function, size_t length, IDirect3DVertexShader9 **outShader);
    gl::Error createPixelShader(const DWORD *function, size_t length, IDirect3DPixelShader9 **outShader);
    HRESULT createVertexBuffer(UINT Length, DWORD Usage, IDirect3DVertexBuffer9 **ppVertexBuffer);
    HRESULT createIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, IDirect3DIndexBuffer9 **ppIndexBuffer);
    virtual gl::Error generateSwizzle(gl::Texture *texture);
    virtual gl::Error setSamplerState(gl::SamplerType type, int index, gl::Texture *texture, const gl::SamplerState &sampler);
    virtual gl::Error setTexture(gl::SamplerType type, int index, gl::Texture *texture);

    virtual gl::Error setUniformBuffers(const gl::Buffer *vertexUniformBuffers[], const gl::Buffer *fragmentUniformBuffers[]);

    virtual gl::Error setRasterizerState(const gl::RasterizerState &rasterState);
    gl::Error setBlendState(const gl::Framebuffer *framebuffer, const gl::BlendState &blendState, const gl::ColorF &blendColor,
                            unsigned int sampleMask) override;
    virtual gl::Error setDepthStencilState(const gl::DepthStencilState &depthStencilState, int stencilRef,
                                           int stencilBackRef, bool frontFaceCCW);

    virtual void setScissorRectangle(const gl::Rectangle &scissor, bool enabled);
    virtual void setViewport(const gl::Rectangle &viewport, float zNear, float zFar, GLenum drawMode, GLenum frontFace,
                             bool ignoreViewport);

    gl::Error applyRenderTarget(const gl::Framebuffer *frameBuffer) override;
    virtual gl::Error applyShaders(gl::ProgramBinary *programBinary, const gl::VertexFormat inputLayout[], const gl::Framebuffer *framebuffer,
                                   bool rasterizerDiscard, bool transformFeedbackActive);
    virtual gl::Error applyUniforms(const ProgramImpl &program, const std::vector<gl::LinkedUniform*> &uniformArray);
    virtual bool applyPrimitiveType(GLenum primitiveType, GLsizei elementCount);
    virtual gl::Error applyVertexBuffer(const gl::State &state, GLint first, GLsizei count, GLsizei instances);
    virtual gl::Error applyIndexBuffer(const GLvoid *indices, gl::Buffer *elementArrayBuffer, GLsizei count, GLenum mode, GLenum type, TranslatedIndexData *indexInfo);

    virtual void applyTransformFeedbackBuffers(const gl::State& state);

    virtual gl::Error drawArrays(GLenum mode, GLsizei count, GLsizei instances, bool transformFeedbackActive);
    virtual gl::Error drawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices,
                                   gl::Buffer *elementArrayBuffer, const TranslatedIndexData &indexInfo, GLsizei instances);

    gl::Error clear(const gl::ClearParameters &clearParams, const gl::Framebuffer *frameBuffer) override;

    virtual void markAllStateDirty();

    // lost device
    void notifyDeviceLost() override;
    bool isDeviceLost() override;
    bool testDeviceLost(bool notify) override;
    bool testDeviceResettable() override;

    DWORD getAdapterVendor() const override;
    std::string getRendererDescription() const override;
    GUID getAdapterIdentifier() const override;

    IDirect3DDevice9 *getDevice() { return mDevice; }

    virtual unsigned int getReservedVertexUniformVectors() const;
    virtual unsigned int getReservedFragmentUniformVectors() const;
    virtual unsigned int getReservedVertexUniformBuffers() const;
    virtual unsigned int getReservedFragmentUniformBuffers() const;
    virtual bool getShareHandleSupport() const;
    virtual bool getPostSubBufferSupport() const;

    virtual int getMajorShaderModel() const;
    DWORD getCapsDeclTypes() const;
    virtual int getMinSwapInterval() const;
    virtual int getMaxSwapInterval() const;

    // Pixel operations
    virtual gl::Error copyImage2D(gl::Framebuffer *framebuffer, const gl::Rectangle &sourceRect, GLenum destFormat,
                                  GLint xoffset, GLint yoffset, TextureStorage *storage, GLint level);
    virtual gl::Error copyImageCube(gl::Framebuffer *framebuffer, const gl::Rectangle &sourceRect, GLenum destFormat,
                                    GLint xoffset, GLint yoffset, TextureStorage *storage, GLenum target, GLint level);
    virtual gl::Error copyImage3D(gl::Framebuffer *framebuffer, const gl::Rectangle &sourceRect, GLenum destFormat,
                                  GLint xoffset, GLint yoffset, GLint zOffset, TextureStorage *storage, GLint level);
    virtual gl::Error copyImage2DArray(gl::Framebuffer *framebuffer, const gl::Rectangle &sourceRect, GLenum destFormat,
                                       GLint xoffset, GLint yoffset, GLint zOffset, TextureStorage *storage, GLint level);

    gl::Error blitRect(const gl::Framebuffer *readTarget, const gl::Rectangle &readRect,
                       const gl::Framebuffer *drawTarget, const gl::Rectangle &drawRect,
                       const gl::Rectangle *scissor, bool blitRenderTarget,
                       bool blitDepth, bool blitStencil, GLenum filter) override;

    virtual gl::Error readPixels(const gl::Framebuffer *framebuffer, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format,
                                 GLenum type, GLuint outputPitch, const gl::PixelPackState &pack, uint8_t *pixels);

    // RenderTarget creation
    virtual gl::Error createRenderTarget(SwapChain *swapChain, bool depth, RenderTarget **outRT);
    virtual gl::Error createRenderTarget(int width, int height, GLenum format, GLsizei samples, RenderTarget **outRT);

    // Shader creation
    virtual ShaderImpl *createShader(const gl::Data &data, GLenum type);
    virtual ProgramImpl *createProgram();

    // Shader operations
    void releaseShaderCompiler() override;
    virtual gl::Error loadExecutable(const void *function, size_t length, ShaderType type,
                                     const std::vector<gl::LinkedVarying> &transformFeedbackVaryings,
                                     bool separatedOutputBuffers, ShaderExecutable **outExecutable);
    virtual gl::Error compileToExecutable(gl::InfoLog &infoLog, const std::string &shaderHLSL, ShaderType type,
                                          const std::vector<gl::LinkedVarying> &transformFeedbackVaryings,
                                          bool separatedOutputBuffers, D3DWorkaroundType workaround,
                                          ShaderExecutable **outExectuable);
    virtual UniformStorage *createUniformStorage(size_t storageSize);

    // Image operations
    virtual Image *createImage();
    gl::Error generateMipmap(Image *dest, Image *source) override;
    virtual TextureStorage *createTextureStorage2D(SwapChain *swapChain);
    virtual TextureStorage *createTextureStorage2D(GLenum internalformat, bool renderTarget, GLsizei width, GLsizei height, int levels);
    virtual TextureStorage *createTextureStorageCube(GLenum internalformat, bool renderTarget, int size, int levels);
    virtual TextureStorage *createTextureStorage3D(GLenum internalformat, bool renderTarget, GLsizei width, GLsizei height, GLsizei depth, int levels);
    virtual TextureStorage *createTextureStorage2DArray(GLenum internalformat, bool renderTarget, GLsizei width, GLsizei height, GLsizei depth, int levels);

    // Texture creation
    virtual TextureImpl *createTexture(GLenum target);

    // Renderbuffer creation
    virtual RenderbufferImpl *createRenderbuffer();
    virtual RenderbufferImpl *createRenderbuffer(SwapChain *swapChain, bool depth);

    // Buffer creation
    virtual BufferImpl *createBuffer();
    virtual VertexBuffer *createVertexBuffer();
    virtual IndexBuffer *createIndexBuffer();

    // Vertex Array creation
    virtual VertexArrayImpl *createVertexArray();

    // Query and Fence creation
    virtual QueryImpl *createQuery(GLenum type);
    virtual FenceNVImpl *createFenceNV();
    virtual FenceSyncImpl *createFenceSync();

    // Transform Feedback creation
    virtual TransformFeedbackImpl* createTransformFeedback();

    // Buffer-to-texture and Texture-to-buffer copies
    virtual bool supportsFastCopyBufferToTexture(GLenum internalFormat) const;
    virtual gl::Error fastCopyBufferToTexture(const gl::PixelUnpackState &unpack, unsigned int offset, RenderTarget *destRenderTarget,
                                              GLenum destinationFormat, GLenum sourcePixelsType, const gl::Box &destArea);

    // D3D9-renderer specific methods
    gl::Error boxFilter(IDirect3DSurface9 *source, IDirect3DSurface9 *dest);

    D3DPOOL getTexturePool(DWORD usage) const;

    virtual bool getLUID(LUID *adapterLuid) const;
    virtual VertexConversionType getVertexConversionType(const gl::VertexFormat &vertexFormat) const;
    virtual GLenum getVertexComponentType(const gl::VertexFormat &vertexFormat) const;

    gl::Error copyToRenderTarget(IDirect3DSurface9 *dest, IDirect3DSurface9 *source, bool fromManaged);

  private:
    DISALLOW_COPY_AND_ASSIGN(Renderer9);

    void generateCaps(gl::Caps *outCaps, gl::TextureCapsMap *outTextureCaps, gl::Extensions *outExtensions) const override;
    Workarounds generateWorkarounds() const override;

    void release();

    void applyUniformnfv(gl::LinkedUniform *targetUniform, const GLfloat *v);
    void applyUniformniv(gl::LinkedUniform *targetUniform, const GLint *v);
    void applyUniformnbv(gl::LinkedUniform *targetUniform, const GLint *v);

    gl::Error drawLineLoop(GLsizei count, GLenum type, const GLvoid *indices, int minIndex, gl::Buffer *elementArrayBuffer);
    gl::Error drawIndexedPoints(GLsizei count, GLenum type, const GLvoid *indices, int minIndex, gl::Buffer *elementArrayBuffer);

    gl::Error getCountingIB(size_t count, StaticIndexBufferInterface **outIB);

    gl::Error getNullColorbuffer(gl::FramebufferAttachment *depthbuffer, gl::FramebufferAttachment **outColorBuffer);

    D3DPOOL getBufferPool(DWORD usage) const;

    HMODULE mD3d9Module;
    HDC mDc;

    void initializeDevice();
    D3DPRESENT_PARAMETERS getDefaultPresentParameters();
    void releaseDeviceResources();

    HRESULT getDeviceStatusCode();
    bool isRemovedDeviceResettable() const;
    bool resetRemovedDevice();

    UINT mAdapter;
    D3DDEVTYPE mDeviceType;
    IDirect3D9 *mD3d9;  // Always valid after successful initialization.
    IDirect3D9Ex *mD3d9Ex;  // Might be null if D3D9Ex is not supported.
    IDirect3DDevice9 *mDevice;
    IDirect3DDevice9Ex *mDeviceEx;  // Might be null if D3D9Ex is not supported.

    HLSLCompiler mCompiler;

    Blit9 *mBlit;

    HWND mDeviceWindow;

    bool mDeviceLost;
    D3DCAPS9 mDeviceCaps;
    D3DADAPTER_IDENTIFIER9 mAdapterIdentifier;

    D3DPRIMITIVETYPE mPrimitiveType;
    int mPrimitiveCount;
    GLsizei mRepeatDraw;

    bool mSceneStarted;
    int mMinSwapInterval;
    int mMaxSwapInterval;

    bool mVertexTextureSupport;

    // current render target states
    unsigned int mAppliedRenderTargetSerial;
    unsigned int mAppliedDepthbufferSerial;
    unsigned int mAppliedStencilbufferSerial;
    bool mDepthStencilInitialized;
    bool mRenderTargetDescInitialized;
    RenderTarget::Desc mRenderTargetDesc;
    unsigned int mCurStencilSize;
    unsigned int mCurDepthSize;

    IDirect3DStateBlock9 *mMaskedClearSavedState;

    // previously set render states
    bool mForceSetDepthStencilState;
    gl::DepthStencilState mCurDepthStencilState;
    int mCurStencilRef;
    int mCurStencilBackRef;
    bool mCurFrontFaceCCW;

    bool mForceSetRasterState;
    gl::RasterizerState mCurRasterState;

    bool mForceSetScissor;
    gl::Rectangle mCurScissor;
    bool mScissorEnabled;

    bool mForceSetViewport;
    gl::Rectangle mCurViewport;
    float mCurNear;
    float mCurFar;
    float mCurDepthFront;

    bool mForceSetBlendState;
    gl::BlendState mCurBlendState;
    gl::ColorF mCurBlendColor;
    GLuint mCurSampleMask;

    // Currently applied sampler states
    std::vector<bool> mForceSetVertexSamplerStates;
    std::vector<gl::SamplerState> mCurVertexSamplerStates;

    std::vector<bool> mForceSetPixelSamplerStates;
    std::vector<gl::SamplerState> mCurPixelSamplerStates;

    // Currently applied textures
    std::vector<unsigned int> mCurVertexTextureSerials;
    std::vector<unsigned int> mCurPixelTextureSerials;

    unsigned int mAppliedIBSerial;
    IDirect3DVertexShader9 *mAppliedVertexShader;
    IDirect3DPixelShader9 *mAppliedPixelShader;
    unsigned int mAppliedProgramSerial;

    dx_VertexConstants mVertexConstants;
    dx_PixelConstants mPixelConstants;
    bool mDxUniformsDirty;

    // A pool of event queries that are currently unused.
    std::vector<IDirect3DQuery9*> mEventQueryPool;
    VertexShaderCache mVertexShaderCache;
    PixelShaderCache mPixelShaderCache;

    VertexDataManager *mVertexDataManager;
    VertexDeclarationCache mVertexDeclarationCache;

    IndexDataManager *mIndexDataManager;
    StreamingIndexBufferInterface *mLineLoopIB;
    StaticIndexBufferInterface *mCountingIB;

    enum { NUM_NULL_COLORBUFFER_CACHE_ENTRIES = 12 };
    struct NullColorbufferCacheEntry
    {
        UINT lruCount;
        int width;
        int height;
        gl::FramebufferAttachment *buffer;
    } mNullColorbufferCache[NUM_NULL_COLORBUFFER_CACHE_ENTRIES];
    UINT mMaxNullColorbufferLRU;

};

}
#endif // LIBGLESV2_RENDERER_RENDERER9_H_
