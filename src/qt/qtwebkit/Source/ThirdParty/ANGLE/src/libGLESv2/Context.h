//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Context.h: Defines the gl::Context class, managing all GL state and performing
// rendering operations. It is the GLES2 specific implementation of EGLContext.

#ifndef LIBGLESV2_CONTEXT_H_
#define LIBGLESV2_CONTEXT_H_

#define GL_APICALL
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#define EGLAPI
#include <EGL/egl.h>

#include <string>
#include <map>
#ifdef _MSC_VER
#include <hash_map>
#else
#include <unordered_map>
#endif

#include "common/angleutils.h"
#include "common/RefCountObject.h"
#include "libGLESv2/HandleAllocator.h"
#include "libGLESv2/angletypes.h"
#include "libGLESv2/Constants.h"

namespace rx
{
class Renderer;
}

namespace egl
{
class Display;
class Surface;
}

namespace gl
{
class Shader;
class Program;
class ProgramBinary;
class Texture;
class Texture2D;
class TextureCubeMap;
class Framebuffer;
class Renderbuffer;
class RenderbufferStorage;
class Colorbuffer;
class Depthbuffer;
class Stencilbuffer;
class DepthStencilbuffer;
class Fence;
class Query;
class ResourceManager;
class Buffer;

enum QueryType
{
    QUERY_ANY_SAMPLES_PASSED,
    QUERY_ANY_SAMPLES_PASSED_CONSERVATIVE,

    QUERY_TYPE_COUNT
};

// Helper structure describing a single vertex attribute
class VertexAttribute
{
  public:
    VertexAttribute() : mType(GL_FLOAT), mSize(0), mNormalized(false), mStride(0), mPointer(NULL), mArrayEnabled(false), mDivisor(0)
    {
        mCurrentValue[0] = 0.0f;
        mCurrentValue[1] = 0.0f;
        mCurrentValue[2] = 0.0f;
        mCurrentValue[3] = 1.0f;
    }

    int typeSize() const
    {
        switch (mType)
        {
          case GL_BYTE:           return mSize * sizeof(GLbyte);
          case GL_UNSIGNED_BYTE:  return mSize * sizeof(GLubyte);
          case GL_SHORT:          return mSize * sizeof(GLshort);
          case GL_UNSIGNED_SHORT: return mSize * sizeof(GLushort);
          case GL_FIXED:          return mSize * sizeof(GLfixed);
          case GL_FLOAT:          return mSize * sizeof(GLfloat);
          default: UNREACHABLE(); return mSize * sizeof(GLfloat);
        }
    }

    GLsizei stride() const
    {
        return mStride ? mStride : typeSize();
    }

    // From glVertexAttribPointer
    GLenum mType;
    GLint mSize;
    bool mNormalized;
    GLsizei mStride;   // 0 means natural stride

    union
    {
        const void *mPointer;
        intptr_t mOffset;
    };

    BindingPointer<Buffer> mBoundBuffer;   // Captured when glVertexAttribPointer is called.

    bool mArrayEnabled;   // From glEnable/DisableVertexAttribArray
    float mCurrentValue[4];   // From glVertexAttrib
    unsigned int mDivisor;
};

// Helper structure to store all raw state
struct State
{
    Color colorClearValue;
    GLclampf depthClearValue;
    int stencilClearValue;

    RasterizerState rasterizer;
    bool scissorTest;
    Rectangle scissor;

    BlendState blend;
    Color blendColor;
    bool sampleCoverage;
    GLclampf sampleCoverageValue;
    bool sampleCoverageInvert;

    DepthStencilState depthStencil;
    GLint stencilRef;
    GLint stencilBackRef;

    GLfloat lineWidth;

    GLenum generateMipmapHint;
    GLenum fragmentShaderDerivativeHint;

    Rectangle viewport;
    float zNear;
    float zFar;

    unsigned int activeSampler;   // Active texture unit selector - GL_TEXTURE0
    BindingPointer<Buffer> arrayBuffer;
    BindingPointer<Buffer> elementArrayBuffer;
    GLuint readFramebuffer;
    GLuint drawFramebuffer;
    BindingPointer<Renderbuffer> renderbuffer;
    GLuint currentProgram;

    VertexAttribute vertexAttribute[MAX_VERTEX_ATTRIBS];
    BindingPointer<Texture> samplerTexture[TEXTURE_TYPE_COUNT][IMPLEMENTATION_MAX_COMBINED_TEXTURE_IMAGE_UNITS];
    BindingPointer<Query> activeQuery[QUERY_TYPE_COUNT];

    GLint unpackAlignment;
    GLint packAlignment;
    bool packReverseRowOrder;
};

class Context
{
  public:
    Context(const gl::Context *shareContext, rx::Renderer *renderer, bool notifyResets, bool robustAccess);

    ~Context();

    void makeCurrent(egl::Surface *surface);

    virtual void markContextLost();
    bool isContextLost();

    // State manipulation
    void setClearColor(float red, float green, float blue, float alpha);

    void setClearDepth(float depth);

    void setClearStencil(int stencil);

    void setCullFace(bool enabled);
    bool isCullFaceEnabled() const;

    void setCullMode(GLenum mode);

    void setFrontFace(GLenum front);

    void setDepthTest(bool enabled);
    bool isDepthTestEnabled() const;

    void setDepthFunc(GLenum depthFunc);

    void setDepthRange(float zNear, float zFar);
    
    void setBlend(bool enabled);
    bool isBlendEnabled() const;

    void setBlendFactors(GLenum sourceRGB, GLenum destRGB, GLenum sourceAlpha, GLenum destAlpha);
    void setBlendColor(float red, float green, float blue, float alpha);
    void setBlendEquation(GLenum rgbEquation, GLenum alphaEquation);

    void setStencilTest(bool enabled);
    bool isStencilTestEnabled() const;

    void setStencilParams(GLenum stencilFunc, GLint stencilRef, GLuint stencilMask);
    void setStencilBackParams(GLenum stencilBackFunc, GLint stencilBackRef, GLuint stencilBackMask);
    void setStencilWritemask(GLuint stencilWritemask);
    void setStencilBackWritemask(GLuint stencilBackWritemask);
    void setStencilOperations(GLenum stencilFail, GLenum stencilPassDepthFail, GLenum stencilPassDepthPass);
    void setStencilBackOperations(GLenum stencilBackFail, GLenum stencilBackPassDepthFail, GLenum stencilBackPassDepthPass);

    void setPolygonOffsetFill(bool enabled);
    bool isPolygonOffsetFillEnabled() const;

    void setPolygonOffsetParams(GLfloat factor, GLfloat units);

    void setSampleAlphaToCoverage(bool enabled);
    bool isSampleAlphaToCoverageEnabled() const;

    void setSampleCoverage(bool enabled);
    bool isSampleCoverageEnabled() const;

    void setSampleCoverageParams(GLclampf value, bool invert);

    void setScissorTest(bool enabled);
    bool isScissorTestEnabled() const;

    void setDither(bool enabled);
    bool isDitherEnabled() const;

    void setLineWidth(GLfloat width);

    void setGenerateMipmapHint(GLenum hint);
    void setFragmentShaderDerivativeHint(GLenum hint);

    void setViewportParams(GLint x, GLint y, GLsizei width, GLsizei height);

    void setScissorParams(GLint x, GLint y, GLsizei width, GLsizei height);

    void setColorMask(bool red, bool green, bool blue, bool alpha);
    void setDepthMask(bool mask);

    void setActiveSampler(unsigned int active);

    GLuint getReadFramebufferHandle() const;
    GLuint getDrawFramebufferHandle() const;
    GLuint getRenderbufferHandle() const;

    GLuint getArrayBufferHandle() const;

    GLuint getActiveQuery(GLenum target) const;

    void setEnableVertexAttribArray(unsigned int attribNum, bool enabled);
    const VertexAttribute &getVertexAttribState(unsigned int attribNum);
    void setVertexAttribState(unsigned int attribNum, Buffer *boundBuffer, GLint size, GLenum type,
                              bool normalized, GLsizei stride, const void *pointer);
    const void *getVertexAttribPointer(unsigned int attribNum) const;

    void setUnpackAlignment(GLint alignment);
    GLint getUnpackAlignment() const;

    void setPackAlignment(GLint alignment);
    GLint getPackAlignment() const;

    void setPackReverseRowOrder(bool reverseRowOrder);
    bool getPackReverseRowOrder() const;

    // These create  and destroy methods are merely pass-throughs to 
    // ResourceManager, which owns these object types
    GLuint createBuffer();
    GLuint createShader(GLenum type);
    GLuint createProgram();
    GLuint createTexture();
    GLuint createRenderbuffer();

    void deleteBuffer(GLuint buffer);
    void deleteShader(GLuint shader);
    void deleteProgram(GLuint program);
    void deleteTexture(GLuint texture);
    void deleteRenderbuffer(GLuint renderbuffer);

    // Framebuffers are owned by the Context, so these methods do not pass through
    GLuint createFramebuffer();
    void deleteFramebuffer(GLuint framebuffer);

    // Fences are owned by the Context.
    GLuint createFence();
    void deleteFence(GLuint fence);
    
    // Queries are owned by the Context;
    GLuint createQuery();
    void deleteQuery(GLuint query);

    void bindArrayBuffer(GLuint buffer);
    void bindElementArrayBuffer(GLuint buffer);
    void bindTexture2D(GLuint texture);
    void bindTextureCubeMap(GLuint texture);
    void bindReadFramebuffer(GLuint framebuffer);
    void bindDrawFramebuffer(GLuint framebuffer);
    void bindRenderbuffer(GLuint renderbuffer);
    void useProgram(GLuint program);
    void linkProgram(GLuint program);
    void setProgramBinary(GLuint program, const void *binary, GLint length);

    void beginQuery(GLenum target, GLuint query);
    void endQuery(GLenum target);

    void setFramebufferZero(Framebuffer *framebuffer);

    void setRenderbufferStorage(GLsizei width, GLsizei height, GLenum internalformat, GLsizei samples);

    void setVertexAttrib(GLuint index, const GLfloat *values);
    void setVertexAttribDivisor(GLuint index, GLuint divisor);

    Buffer *getBuffer(GLuint handle);
    Fence *getFence(GLuint handle);
    Shader *getShader(GLuint handle);
    Program *getProgram(GLuint handle);
    Texture *getTexture(GLuint handle);
    Framebuffer *getFramebuffer(GLuint handle);
    Renderbuffer *getRenderbuffer(GLuint handle);
    Query *getQuery(GLuint handle, bool create, GLenum type);

    Buffer *getArrayBuffer();
    Buffer *getElementArrayBuffer();
    ProgramBinary *getCurrentProgramBinary();
    Texture2D *getTexture2D();
    TextureCubeMap *getTextureCubeMap();
    Texture *getSamplerTexture(unsigned int sampler, TextureType type);
    Framebuffer *getReadFramebuffer();
    Framebuffer *getDrawFramebuffer();

    bool getFloatv(GLenum pname, GLfloat *params);
    bool getIntegerv(GLenum pname, GLint *params);
    bool getBooleanv(GLenum pname, GLboolean *params);

    bool getQueryParameterInfo(GLenum pname, GLenum *type, unsigned int *numParams);

    void readPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei *bufSize, void* pixels);
    void clear(GLbitfield mask);
    void drawArrays(GLenum mode, GLint first, GLsizei count, GLsizei instances);
    void drawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLsizei instances);
    void sync(bool block);   // flush/finish

    void recordInvalidEnum();
    void recordInvalidValue();
    void recordInvalidOperation();
    void recordOutOfMemory();
    void recordInvalidFramebufferOperation();

    GLenum getError();
    GLenum getResetStatus();
    virtual bool isResetNotificationEnabled();

    int getMajorShaderModel() const;
    float getMaximumPointSize() const;
    unsigned int getMaximumCombinedTextureImageUnits() const;
    int getMaximumRenderbufferDimension() const;
    int getMaximumTextureDimension() const;
    int getMaximumCubeTextureDimension() const;
    int getMaximumTextureLevel() const;
    unsigned int getMaximumRenderTargets() const;
    GLsizei getMaxSupportedSamples() const;
    const char *getExtensionString() const;
    const char *getRendererString() const;
    bool supportsEventQueries() const;
    bool supportsOcclusionQueries() const;
    bool supportsBGRATextures() const;
    bool supportsDXT1Textures() const;
    bool supportsDXT3Textures() const;
    bool supportsDXT5Textures() const;
    bool supportsFloat32Textures() const;
    bool supportsFloat32LinearFilter() const;
    bool supportsFloat32RenderableTextures() const;
    bool supportsFloat16Textures() const;
    bool supportsFloat16LinearFilter() const;
    bool supportsFloat16RenderableTextures() const;
    bool supportsLuminanceTextures() const;
    bool supportsLuminanceAlphaTextures() const;
    bool supportsDepthTextures() const;
    bool supports32bitIndices() const;
    bool supportsNonPower2Texture() const;
    bool supportsInstancing() const;
    bool supportsTextureFilterAnisotropy() const;

    bool getCurrentReadFormatType(GLenum *format, GLenum *type);

    float getTextureMaxAnisotropy() const;

    void blitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, 
                         GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                         GLbitfield mask);

  private:
    DISALLOW_COPY_AND_ASSIGN(Context);

    bool applyRenderTarget(GLenum drawMode, bool ignoreViewport);
    void applyState(GLenum drawMode);
    void applyShaders();
    void applyTextures();
    void applyTextures(SamplerType type);

    void detachBuffer(GLuint buffer);
    void detachTexture(GLuint texture);
    void detachFramebuffer(GLuint framebuffer);
    void detachRenderbuffer(GLuint renderbuffer);

    Texture *getIncompleteTexture(TextureType type);

    bool skipDraw(GLenum drawMode);

    void initExtensionString();
    void initRendererString();

    rx::Renderer *const mRenderer;

    State mState;

    BindingPointer<Texture2D> mTexture2DZero;
    BindingPointer<TextureCubeMap> mTextureCubeMapZero;

#ifndef HASH_MAP
# ifdef _MSC_VER
#  define HASH_MAP stdext::hash_map
# else
#  define HASH_MAP std::unordered_map
# endif
#endif

    typedef HASH_MAP<GLuint, Framebuffer*> FramebufferMap;
    FramebufferMap mFramebufferMap;
    HandleAllocator mFramebufferHandleAllocator;

    typedef HASH_MAP<GLuint, Fence*> FenceMap;
    FenceMap mFenceMap;
    HandleAllocator mFenceHandleAllocator;

    typedef HASH_MAP<GLuint, Query*> QueryMap;
    QueryMap mQueryMap;
    HandleAllocator mQueryHandleAllocator;

    const char *mExtensionString;
    const char *mRendererString;
    
    BindingPointer<Texture> mIncompleteTextures[TEXTURE_TYPE_COUNT];

    // Recorded errors
    bool mInvalidEnum;
    bool mInvalidValue;
    bool mInvalidOperation;
    bool mOutOfMemory;
    bool mInvalidFramebufferOperation;

    // Current/lost context flags
    bool mHasBeenCurrent;
    bool mContextLost;
    GLenum mResetStatus;
    GLenum mResetStrategy;
    bool mRobustAccess;

    BindingPointer<ProgramBinary> mCurrentProgramBinary;
    Framebuffer *mBoundDrawFramebuffer;

    int mMajorShaderModel;
    float mMaximumPointSize;
    bool mSupportsVertexTexture;
    bool mSupportsNonPower2Texture;
    bool mSupportsInstancing;
    int  mMaxViewportDimension;
    int  mMaxRenderbufferDimension;
    int  mMaxTextureDimension;
    int  mMaxCubeTextureDimension;
    int  mMaxTextureLevel;
    float mMaxTextureAnisotropy;
    bool mSupportsEventQueries;
    bool mSupportsOcclusionQueries;
    bool mSupportsBGRATextures;
    bool mSupportsDXT1Textures;
    bool mSupportsDXT3Textures;
    bool mSupportsDXT5Textures;
    bool mSupportsFloat32Textures;
    bool mSupportsFloat32LinearFilter;
    bool mSupportsFloat32RenderableTextures;
    bool mSupportsFloat16Textures;
    bool mSupportsFloat16LinearFilter;
    bool mSupportsFloat16RenderableTextures;
    bool mSupportsLuminanceTextures;
    bool mSupportsLuminanceAlphaTextures;
    bool mSupportsDepthTextures;
    bool mSupports32bitIndices;
    bool mSupportsTextureFilterAnisotropy;
    int mNumCompressedTextureFormats;

    ResourceManager *mResourceManager;
};
}

#endif   // INCLUDE_CONTEXT_H_
