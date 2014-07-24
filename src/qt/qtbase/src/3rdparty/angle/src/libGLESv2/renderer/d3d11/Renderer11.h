//
// Copyright (c) 2012-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Renderer11.h: Defines a back-end specific class for the D3D11 renderer.

#ifndef LIBGLESV2_RENDERER_RENDERER11_H_
#define LIBGLESV2_RENDERER_RENDERER11_H_

#include "common/angleutils.h"
#include "libGLESv2/angletypes.h"
#include "libGLESv2/mathutil.h"

#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/renderer/d3d11/RenderStateCache.h"
#include "libGLESv2/renderer/d3d11/InputLayoutCache.h"
#include "libGLESv2/renderer/RenderTarget.h"

#if defined(ANGLE_OS_WINRT) && !defined(ANGLE_OS_WINPHONE)
struct IInspectable;
namespace ABI {
    namespace Windows {
        namespace ApplicationModel {
            struct ISuspendingEventArgs;
        }
    }
}
#endif

namespace gl
{
class Renderbuffer;
}

namespace rx
{

class VertexDataManager;
class IndexDataManager;
class StreamingIndexBufferInterface;

enum
{
    MAX_VERTEX_UNIFORM_VECTORS_D3D11 = 1024,
    MAX_FRAGMENT_UNIFORM_VECTORS_D3D11 = 1024
};

class Renderer11 : public Renderer
{
  public:
    Renderer11(egl::Display *display);
    virtual ~Renderer11();

    static Renderer11 *makeRenderer11(Renderer *renderer);

    virtual EGLint initialize();
    virtual bool resetDevice();

    virtual int generateConfigs(ConfigDesc **configDescList);
    virtual void deleteConfigs(ConfigDesc *configDescList);

    virtual void sync(bool block);

    virtual SwapChain *createSwapChain(EGLNativeWindowType window, HANDLE shareHandle, GLenum backBufferFormat, GLenum depthBufferFormat);

    virtual void setSamplerState(gl::SamplerType type, int index, const gl::SamplerState &sampler);
    virtual void setTexture(gl::SamplerType type, int index, gl::Texture *texture);

    virtual void setRasterizerState(const gl::RasterizerState &rasterState);
    virtual void setBlendState(gl::Framebuffer *framebuffer, const gl::BlendState &blendState, const gl::Color &blendColor,
                               unsigned int sampleMask);
    virtual void setDepthStencilState(const gl::DepthStencilState &depthStencilState, int stencilRef,
                                      int stencilBackRef, bool frontFaceCCW);

    virtual void setScissorRectangle(const gl::Rectangle &scissor, bool enabled);
    virtual bool setViewport(const gl::Rectangle &viewport, float zNear, float zFar, GLenum drawMode, GLenum frontFace,
                             bool ignoreViewport);

    virtual bool applyPrimitiveType(GLenum mode, GLsizei count);
    virtual bool applyRenderTarget(gl::Framebuffer *frameBuffer);
    virtual void applyShaders(gl::ProgramBinary *programBinary);
    virtual void applyUniforms(gl::ProgramBinary *programBinary, gl::UniformArray *uniformArray);
    virtual GLenum applyVertexBuffer(gl::ProgramBinary *programBinary, gl::VertexAttribute vertexAttributes[], GLint first, GLsizei count, GLsizei instances);
    virtual GLenum applyIndexBuffer(const GLvoid *indices, gl::Buffer *elementArrayBuffer, GLsizei count, GLenum mode, GLenum type, TranslatedIndexData *indexInfo);

    virtual void drawArrays(GLenum mode, GLsizei count, GLsizei instances);
    virtual void drawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, gl::Buffer *elementArrayBuffer, const TranslatedIndexData &indexInfo, GLsizei instances);

    virtual void clear(const gl::ClearParameters &clearParams, gl::Framebuffer *frameBuffer);

    virtual void markAllStateDirty();

    // lost device
    void notifyDeviceLost();
    virtual bool isDeviceLost();
    virtual bool testDeviceLost(bool notify);
    virtual bool testDeviceResettable();

    // Renderer capabilities
    virtual DWORD getAdapterVendor() const;
    virtual std::string getRendererDescription() const;
    virtual GUID getAdapterIdentifier() const;

    virtual bool getBGRATextureSupport() const;
    virtual bool getDXT1TextureSupport();
    virtual bool getDXT3TextureSupport();
    virtual bool getDXT5TextureSupport();
    virtual bool getEventQuerySupport();
    virtual bool getFloat32TextureSupport(bool *filtering, bool *renderable);
    virtual bool getFloat16TextureSupport(bool *filtering, bool *renderable);
    virtual bool getLuminanceTextureSupport();
    virtual bool getLuminanceAlphaTextureSupport();
    virtual unsigned int getMaxVertexTextureImageUnits() const;
    virtual unsigned int getMaxCombinedTextureImageUnits() const;
    virtual unsigned int getReservedVertexUniformVectors() const;
    virtual unsigned int getReservedFragmentUniformVectors() const;
    virtual unsigned int getMaxVertexUniformVectors() const;
    virtual unsigned int getMaxFragmentUniformVectors() const;
    virtual unsigned int getMaxVaryingVectors() const;
    virtual bool getNonPower2TextureSupport() const;
    virtual bool getDepthTextureSupport() const;
    virtual bool getOcclusionQuerySupport() const;
    virtual bool getInstancingSupport() const;
    virtual bool getTextureFilterAnisotropySupport() const;
    virtual float getTextureMaxAnisotropy() const;
    virtual bool getShareHandleSupport() const;
    virtual bool getDerivativeInstructionSupport() const;
    virtual bool getPostSubBufferSupport() const;

    virtual int getMajorShaderModel() const;
    virtual float getMaxPointSize() const;
    virtual int getMaxViewportDimension() const;
    virtual int getMaxTextureWidth() const;
    virtual int getMaxTextureHeight() const;
    virtual bool get32BitIndexSupport() const;
    virtual int getMinSwapInterval() const;
    virtual int getMaxSwapInterval() const;

    virtual GLsizei getMaxSupportedSamples() const;
    int getNearestSupportedSamples(DXGI_FORMAT format, unsigned int requested) const;

    virtual unsigned int getMaxRenderTargets() const;

    // Pixel operations
    virtual bool copyToRenderTarget(TextureStorageInterface2D *dest, TextureStorageInterface2D *source);
    virtual bool copyToRenderTarget(TextureStorageInterfaceCube *dest, TextureStorageInterfaceCube *source);

    virtual bool copyImage(gl::Framebuffer *framebuffer, const gl::Rectangle &sourceRect, GLenum destFormat,
                           GLint xoffset, GLint yoffset, TextureStorageInterface2D *storage, GLint level);
    virtual bool copyImage(gl::Framebuffer *framebuffer, const gl::Rectangle &sourceRect, GLenum destFormat,
                           GLint xoffset, GLint yoffset, TextureStorageInterfaceCube *storage, GLenum target, GLint level);

    bool copyTexture(ID3D11ShaderResourceView *source, const gl::Rectangle &sourceArea, unsigned int sourceWidth, unsigned int sourceHeight,
                     ID3D11RenderTargetView *dest, const gl::Rectangle &destArea, unsigned int destWidth, unsigned int destHeight, GLenum destFormat);

    virtual bool blitRect(gl::Framebuffer *readTarget, const gl::Rectangle &readRect, gl::Framebuffer *drawTarget, const gl::Rectangle &drawRect,
                          bool blitRenderTarget, bool blitDepthStencil);
    virtual void readPixels(gl::Framebuffer *framebuffer, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type,
                            GLsizei outputPitch, bool packReverseRowOrder, GLint packAlignment, void* pixels);

    // RenderTarget creation
    virtual RenderTarget *createRenderTarget(SwapChain *swapChain, bool depth);
    virtual RenderTarget *createRenderTarget(int width, int height, GLenum format, GLsizei samples, bool depth);

    // Shader operations
    virtual ShaderExecutable *loadExecutable(const void *function, size_t length, rx::ShaderType type);
    virtual ShaderExecutable *compileToExecutable(gl::InfoLog &infoLog, const char *shaderHLSL, rx::ShaderType type, D3DWorkaroundType workaround);

    // Image operations
    virtual Image *createImage();
    virtual void generateMipmap(Image *dest, Image *source);
    virtual TextureStorage *createTextureStorage2D(SwapChain *swapChain);
    virtual TextureStorage *createTextureStorage2D(int levels, GLenum internalformat, GLenum usage, bool forceRenderable, GLsizei width, GLsizei height);
    virtual TextureStorage *createTextureStorageCube(int levels, GLenum internalformat, GLenum usage, bool forceRenderable, int size);

    // Buffer creation
    virtual VertexBuffer *createVertexBuffer();
    virtual IndexBuffer *createIndexBuffer();
    virtual BufferStorage *createBufferStorage();

    // Query and Fence creation
    virtual QueryImpl *createQuery(GLenum type);
    virtual FenceImpl *createFence();

    // D3D11-renderer specific methods
    ID3D11Device *getDevice() { return mDevice; }
    ID3D11DeviceContext *getDeviceContext() { return mDeviceContext; };
    IDXGIFactory *getDxgiFactory() { return mDxgiFactory; };
    D3D_FEATURE_LEVEL getFeatureLevel() { return mFeatureLevel; }

    bool getRenderTargetResource(gl::Renderbuffer *colorbuffer, unsigned int *subresourceIndex, ID3D11Texture2D **resource);
    void unapplyRenderTargets();
    void setOneTimeRenderTarget(ID3D11RenderTargetView *renderTargetView);

    virtual bool getLUID(LUID *adapterLuid) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Renderer11);

    void drawLineLoop(GLsizei count, GLenum type, const GLvoid *indices, int minIndex, gl::Buffer *elementArrayBuffer);
    void drawTriangleFan(GLsizei count, GLenum type, const GLvoid *indices, int minIndex, gl::Buffer *elementArrayBuffer, int instances);

    void readTextureData(ID3D11Texture2D *texture, unsigned int subResource, const gl::Rectangle &area,
                         GLenum sourceFormat, GLenum format, GLenum type, GLsizei outputPitch, bool packReverseRowOrder,
                         GLint packAlignment, void *pixels);

    void maskedClear(const gl::ClearParameters &clearParams, gl::Framebuffer *frameBuffer);
    rx::Range getViewportBounds() const;

    bool blitRenderbufferRect(const gl::Rectangle &readRect, const gl::Rectangle &drawRect, RenderTarget *readRenderTarget, 
                              RenderTarget *drawRenderTarget, bool wholeBufferCopy);
    ID3D11Texture2D *resolveMultisampledTexture(ID3D11Texture2D *source, unsigned int subresource);

#if defined(ANGLE_OS_WINRT) && !defined(ANGLE_OS_WINPHONE)
    HRESULT onSuspend(IInspectable *, ABI::Windows::ApplicationModel::ISuspendingEventArgs *);
#endif

    HMODULE mD3d11Module;
    HMODULE mDxgiModule;

    bool mDeviceLost;

    void initializeDevice();
    void releaseDeviceResources();
    int getMinorShaderModel() const;
    void release();

    RenderStateCache mStateCache;

    // Support flags
    bool mFloat16TextureSupport;
    bool mFloat16FilterSupport;
    bool mFloat16RenderSupport;

    bool mFloat32TextureSupport;
    bool mFloat32FilterSupport;
    bool mFloat32RenderSupport;

    bool mDXT1TextureSupport;
    bool mDXT3TextureSupport;
    bool mDXT5TextureSupport;

    bool mDepthTextureSupport;

    // Multisample format support
    struct MultisampleSupportInfo
    {
        unsigned int qualityLevels[D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT];
    };

    typedef std::unordered_map<DXGI_FORMAT, MultisampleSupportInfo, std::hash<int> > MultisampleSupportMap;
    MultisampleSupportMap mMultisampleSupportMap;

    unsigned int mMaxSupportedSamples;

    // current render target states
    unsigned int mAppliedRenderTargetSerials[gl::IMPLEMENTATION_MAX_DRAW_BUFFERS];
    unsigned int mAppliedDepthbufferSerial;
    unsigned int mAppliedStencilbufferSerial;
    bool mDepthStencilInitialized;
    bool mRenderTargetDescInitialized;
    rx::RenderTarget::Desc mRenderTargetDesc;
    unsigned int mCurDepthSize;
    unsigned int mCurStencilSize;

    // Currently applied sampler states
    bool mForceSetVertexSamplerStates[gl::IMPLEMENTATION_MAX_VERTEX_TEXTURE_IMAGE_UNITS];
    gl::SamplerState mCurVertexSamplerStates[gl::IMPLEMENTATION_MAX_VERTEX_TEXTURE_IMAGE_UNITS];

    bool mForceSetPixelSamplerStates[gl::MAX_TEXTURE_IMAGE_UNITS];
    gl::SamplerState mCurPixelSamplerStates[gl::MAX_TEXTURE_IMAGE_UNITS];

    // Currently applied textures
    unsigned int mCurVertexTextureSerials[gl::IMPLEMENTATION_MAX_VERTEX_TEXTURE_IMAGE_UNITS];
    unsigned int mCurPixelTextureSerials[gl::MAX_TEXTURE_IMAGE_UNITS];

    // Currently applied blend state
    bool mForceSetBlendState;
    gl::BlendState mCurBlendState;
    gl::Color mCurBlendColor;
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

    unsigned int mAppliedIBSerial;
    unsigned int mAppliedStorageIBSerial;
    unsigned int mAppliedIBOffset;

    unsigned int mAppliedProgramBinarySerial;
    bool mIsGeometryShaderActive;

    dx_VertexConstants mVertexConstants;
    dx_VertexConstants mAppliedVertexConstants;
    ID3D11Buffer *mDriverConstantBufferVS;
    ID3D11Buffer *mCurrentVertexConstantBuffer;

    dx_PixelConstants mPixelConstants;
    dx_PixelConstants mAppliedPixelConstants;
    ID3D11Buffer *mDriverConstantBufferPS;
    ID3D11Buffer *mCurrentPixelConstantBuffer;

    ID3D11Buffer *mCurrentGeometryConstantBuffer;

    // Vertex, index and input layouts
    VertexDataManager *mVertexDataManager;
    IndexDataManager *mIndexDataManager;
    InputLayoutCache mInputLayoutCache;

    StreamingIndexBufferInterface *mLineLoopIB;
    StreamingIndexBufferInterface *mTriangleFanIB;

    // Texture copy resources
    bool mCopyResourcesInitialized;
    ID3D11Buffer *mCopyVB;
    ID3D11SamplerState *mCopySampler;
    ID3D11InputLayout *mCopyIL;
    ID3D11VertexShader *mCopyVS;
    ID3D11PixelShader *mCopyRGBAPS;
    ID3D11PixelShader *mCopyRGBPS;
    ID3D11PixelShader *mCopyLumPS;
    ID3D11PixelShader *mCopyLumAlphaPS;

    // Masked clear resources
    bool mClearResourcesInitialized;
    ID3D11Buffer *mClearVB;
    ID3D11InputLayout *mClearIL;
    ID3D11VertexShader *mClearVS;
    ID3D11PixelShader *mClearSinglePS;
    ID3D11PixelShader *mClearMultiplePS;
    ID3D11RasterizerState *mClearScissorRS;
    ID3D11RasterizerState *mClearNoScissorRS;

    // Sync query
    ID3D11Query *mSyncQuery;

    ID3D11Device *mDevice;
    D3D_FEATURE_LEVEL mFeatureLevel;
    ID3D11DeviceContext *mDeviceContext;
    IDXGIAdapter *mDxgiAdapter;
    DXGI_ADAPTER_DESC mAdapterDescription;
    char mDescription[128];
    IDXGIFactory *mDxgiFactory;

    // Cached device caps
    bool mBGRATextureSupport;
};

}
#endif // LIBGLESV2_RENDERER_RENDERER11_H_
