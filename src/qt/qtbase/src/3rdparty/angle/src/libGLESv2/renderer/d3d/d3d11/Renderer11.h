//
// Copyright (c) 2012-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Renderer11.h: Defines a back-end specific class for the D3D11 renderer.

#ifndef LIBGLESV2_RENDERER_RENDERER11_H_
#define LIBGLESV2_RENDERER_RENDERER11_H_

#include "common/angleutils.h"
#include "libGLESv2/angletypes.h"
#include "common/mathutil.h"

#include "libGLESv2/renderer/d3d/d3d11/RenderStateCache.h"
#include "libGLESv2/renderer/d3d/d3d11/InputLayoutCache.h"
#include "libGLESv2/renderer/d3d/HLSLCompiler.h"
#include "libGLESv2/renderer/d3d/RendererD3D.h"
#include "libGLESv2/renderer/RenderTarget.h"

#include "libEGL/AttributeMap.h"

namespace gl
{
class FramebufferAttachment;
}

namespace rx
{

class VertexDataManager;
class IndexDataManager;
class StreamingIndexBufferInterface;
class Blit11;
class Clear11;
class PixelTransfer11;
class RenderTarget11;
struct PackPixelsParams;

enum
{
    MAX_VERTEX_UNIFORM_VECTORS_D3D11 = 1024,
    MAX_FRAGMENT_UNIFORM_VECTORS_D3D11 = 1024
};

class Renderer11 : public RendererD3D
{
  public:
    Renderer11(egl::Display *display, EGLNativeDisplayType hDc, const egl::AttributeMap &attributes);
    virtual ~Renderer11();

    static Renderer11 *makeRenderer11(Renderer *renderer);

    virtual EGLint initialize();
    virtual bool resetDevice();

    virtual int generateConfigs(ConfigDesc **configDescList);
    virtual void deleteConfigs(ConfigDesc *configDescList);

    virtual gl::Error sync(bool block);

    virtual SwapChain *createSwapChain(NativeWindow nativeWindow, HANDLE shareHandle, GLenum backBufferFormat, GLenum depthBufferFormat);

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

    virtual bool applyPrimitiveType(GLenum mode, GLsizei count);
    gl::Error applyRenderTarget(const gl::Framebuffer *frameBuffer) override;
    virtual gl::Error applyShaders(gl::ProgramBinary *programBinary, const gl::VertexFormat inputLayout[], const gl::Framebuffer *framebuffer,
                                   bool rasterizerDiscard, bool transformFeedbackActive);

    virtual gl::Error applyUniforms(const ProgramImpl &program, const std::vector<gl::LinkedUniform*> &uniformArray);
    virtual gl::Error applyVertexBuffer(const gl::State &state, GLint first, GLsizei count, GLsizei instances);
    virtual gl::Error applyIndexBuffer(const GLvoid *indices, gl::Buffer *elementArrayBuffer, GLsizei count, GLenum mode, GLenum type, TranslatedIndexData *indexInfo);
    virtual void applyTransformFeedbackBuffers(const gl::State &state);

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

    virtual unsigned int getReservedVertexUniformVectors() const;
    virtual unsigned int getReservedFragmentUniformVectors() const;
    virtual unsigned int getReservedVertexUniformBuffers() const;
    virtual unsigned int getReservedFragmentUniformBuffers() const;
    virtual bool getShareHandleSupport() const;
    virtual bool getPostSubBufferSupport() const;

    virtual int getMajorShaderModel() const;
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

    gl::Error blitRect(const gl::Framebuffer *readTarget, const gl::Rectangle &readRect, const gl::Framebuffer *drawTarget, const gl::Rectangle &drawRect,
                       const gl::Rectangle *scissor, bool blitRenderTarget, bool blitDepth, bool blitStencil, GLenum filter) override;

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

    // D3D11-renderer specific methods
    ID3D11Device *getDevice() { return mDevice; }
    ID3D11DeviceContext *getDeviceContext() { return mDeviceContext; };
    DXGIFactory *getDxgiFactory() { return mDxgiFactory; };
    bool isLevel9() { return mFeatureLevel <= D3D_FEATURE_LEVEL_9_3; }

    Blit11 *getBlitter() { return mBlit; }

    // Buffer-to-texture and Texture-to-buffer copies
    virtual bool supportsFastCopyBufferToTexture(GLenum internalFormat) const;
    virtual gl::Error fastCopyBufferToTexture(const gl::PixelUnpackState &unpack, unsigned int offset, RenderTarget *destRenderTarget,
                                              GLenum destinationFormat, GLenum sourcePixelsType, const gl::Box &destArea);

    gl::Error getRenderTargetResource(gl::FramebufferAttachment *colorbuffer, unsigned int *subresourceIndexOut, ID3D11Texture2D **texture2DOut);

    void unapplyRenderTargets();
    void setOneTimeRenderTarget(ID3D11RenderTargetView *renderTargetView);
    gl::Error packPixels(ID3D11Texture2D *readTexture, const PackPixelsParams &params, uint8_t *pixelsOut);

    virtual bool getLUID(LUID *adapterLuid) const;
    virtual VertexConversionType getVertexConversionType(const gl::VertexFormat &vertexFormat) const;
    virtual GLenum getVertexComponentType(const gl::VertexFormat &vertexFormat) const;

    gl::Error readTextureData(ID3D11Texture2D *texture, unsigned int subResource, const gl::Rectangle &area, GLenum format,
                              GLenum type, GLuint outputPitch, const gl::PixelPackState &pack, uint8_t *pixels);

    void setShaderResource(gl::SamplerType shaderType, UINT resourceSlot, ID3D11ShaderResourceView *srv);

  private:
    DISALLOW_COPY_AND_ASSIGN(Renderer11);

    void generateCaps(gl::Caps *outCaps, gl::TextureCapsMap *outTextureCaps, gl::Extensions *outExtensions) const override;
    Workarounds generateWorkarounds() const override;

    gl::Error drawLineLoop(GLsizei count, GLenum type, const GLvoid *indices, int minIndex, gl::Buffer *elementArrayBuffer);
    gl::Error drawTriangleFan(GLsizei count, GLenum type, const GLvoid *indices, int minIndex, gl::Buffer *elementArrayBuffer, int instances);

    gl::Error blitRenderbufferRect(const gl::Rectangle &readRect, const gl::Rectangle &drawRect, RenderTarget *readRenderTarget,
                                   RenderTarget *drawRenderTarget, GLenum filter, const gl::Rectangle *scissor,
                                   bool colorBlit, bool depthBlit, bool stencilBlit);
    ID3D11Texture2D *resolveMultisampledTexture(ID3D11Texture2D *source, unsigned int subresource);
    void unsetSRVsWithResource(gl::SamplerType shaderType, const ID3D11Resource *resource);

    static void invalidateFBOAttachmentSwizzles(gl::FramebufferAttachment *attachment, int mipLevel);
    static void invalidateFramebufferSwizzles(const gl::Framebuffer *framebuffer);

    HMODULE mD3d11Module;
    HMODULE mDxgiModule;
    EGLNativeDisplayType mDc;
    std::vector<D3D_FEATURE_LEVEL> mAvailableFeatureLevels;
    D3D_DRIVER_TYPE mDriverType;

    HLSLCompiler mCompiler;

    bool mDeviceLost;

    void initializeDevice();
    void releaseDeviceResources();
    int getMinorShaderModel() const;
    void release();

    RenderStateCache mStateCache;

    // current render target states
    unsigned int mAppliedRenderTargetSerials[gl::IMPLEMENTATION_MAX_DRAW_BUFFERS];
    unsigned int mAppliedDepthbufferSerial;
    unsigned int mAppliedStencilbufferSerial;
    bool mDepthStencilInitialized;
    bool mRenderTargetDescInitialized;
    RenderTarget::Desc mRenderTargetDesc;

    // Currently applied sampler states
    std::vector<bool> mForceSetVertexSamplerStates;
    std::vector<gl::SamplerState> mCurVertexSamplerStates;

    std::vector<bool> mForceSetPixelSamplerStates;
    std::vector<gl::SamplerState> mCurPixelSamplerStates;

    // Currently applied textures
    std::vector<ID3D11ShaderResourceView*> mCurVertexSRVs;
    std::vector<ID3D11ShaderResourceView*> mCurPixelSRVs;

    // Currently applied blend state
    bool mForceSetBlendState;
    gl::BlendState mCurBlendState;
    gl::ColorF mCurBlendColor;
    unsigned int mCurSampleMask;

    // Currently applied rasterizer state
    bool mForceSetRasterState;
    gl::RasterizerState mCurRasterState;

    // Currently applied depth stencil state
    bool mForceSetDepthStencilState;
    gl::DepthStencilState mCurDepthStencilState;
    int mCurStencilRef;
    int mCurStencilBackRef;

    // Currently applied scissor rectangle
    bool mForceSetScissor;
    bool mScissorEnabled;
    gl::Rectangle mCurScissor;

    // Currently applied viewport
    bool mForceSetViewport;
    gl::Rectangle mCurViewport;
    float mCurNear;
    float mCurFar;

    // Currently applied primitive topology
    D3D11_PRIMITIVE_TOPOLOGY mCurrentPrimitiveTopology;

    // Currently applied index buffer
    ID3D11Buffer *mAppliedIB;
    DXGI_FORMAT mAppliedIBFormat;
    unsigned int mAppliedIBOffset;

    // Currently applied transform feedback buffers
    ID3D11Buffer *mAppliedTFBuffers[gl::IMPLEMENTATION_MAX_TRANSFORM_FEEDBACK_BUFFERS]; // Tracks the current D3D buffers
                                                                                        // in use for streamout
    GLintptr mAppliedTFOffsets[gl::IMPLEMENTATION_MAX_TRANSFORM_FEEDBACK_BUFFERS]; // Tracks the current GL-specified
                                                                                   // buffer offsets to transform feedback
                                                                                   // buffers
    UINT mCurrentD3DOffsets[gl::IMPLEMENTATION_MAX_TRANSFORM_FEEDBACK_BUFFERS];  // Tracks the D3D buffer offsets,
                                                                                 // which may differ from GLs, due
                                                                                 // to different append behavior

    // Currently applied shaders
    ID3D11VertexShader *mAppliedVertexShader;
    ID3D11GeometryShader *mAppliedGeometryShader;
    ID3D11GeometryShader *mCurPointGeometryShader;
    ID3D11PixelShader *mAppliedPixelShader;

    dx_VertexConstants mVertexConstants;
    dx_VertexConstants mAppliedVertexConstants;
    ID3D11Buffer *mDriverConstantBufferVS;
    ID3D11Buffer *mCurrentVertexConstantBuffer;
    unsigned int mCurrentConstantBufferVS[gl::IMPLEMENTATION_MAX_VERTEX_SHADER_UNIFORM_BUFFERS];

    dx_PixelConstants mPixelConstants;
    dx_PixelConstants mAppliedPixelConstants;
    ID3D11Buffer *mDriverConstantBufferPS;
    ID3D11Buffer *mCurrentPixelConstantBuffer;
    unsigned int mCurrentConstantBufferPS[gl::IMPLEMENTATION_MAX_VERTEX_SHADER_UNIFORM_BUFFERS];

    ID3D11Buffer *mCurrentGeometryConstantBuffer;

    // Vertex, index and input layouts
    VertexDataManager *mVertexDataManager;
    IndexDataManager *mIndexDataManager;
    InputLayoutCache mInputLayoutCache;

    StreamingIndexBufferInterface *mLineLoopIB;
    StreamingIndexBufferInterface *mTriangleFanIB;

    // Texture copy resources
    Blit11 *mBlit;
    PixelTransfer11 *mPixelTransfer;

    // Masked clear resources
    Clear11 *mClear;

    // Sync query
    ID3D11Query *mSyncQuery;

    ID3D11Device *mDevice;
    D3D_FEATURE_LEVEL mFeatureLevel;
    ID3D11DeviceContext *mDeviceContext;
    IDXGIAdapter *mDxgiAdapter;
    DXGI_ADAPTER_DESC mAdapterDescription;
    char mDescription[128];
    DXGIFactory *mDxgiFactory;
};

}
#endif // LIBGLESV2_RENDERER_RENDERER11_H_
