#include "precompiled.h"
//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Context.cpp: Implements the gl::Context class, managing all GL state and performing
// rendering operations. It is the GLES2 specific implementation of EGLContext.

#include "libGLESv2/Context.h"

#include "libGLESv2/main.h"
#include "libGLESv2/utilities.h"
#include "libGLESv2/Buffer.h"
#include "libGLESv2/Fence.h"
#include "libGLESv2/Framebuffer.h"
#include "libGLESv2/Renderbuffer.h"
#include "libGLESv2/Program.h"
#include "libGLESv2/ProgramBinary.h"
#include "libGLESv2/Query.h"
#include "libGLESv2/Texture.h"
#include "libGLESv2/ResourceManager.h"
#include "libGLESv2/renderer/IndexDataManager.h"
#include "libGLESv2/renderer/RenderTarget.h"
#include "libGLESv2/renderer/Renderer.h"

#include "libEGL/Surface.h"

#undef near
#undef far

namespace gl
{
static const char* makeStaticString(const std::string& str)
{
    static std::set<std::string> strings;
    std::set<std::string>::iterator it = strings.find(str);
    if (it != strings.end())
      return it->c_str();

    return strings.insert(str).first->c_str();
}

Context::Context(const gl::Context *shareContext, rx::Renderer *renderer, bool notifyResets, bool robustAccess) : mRenderer(renderer)
{
    ASSERT(robustAccess == false);   // Unimplemented

    mFenceHandleAllocator.setBaseHandle(0);

    setClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    mState.depthClearValue = 1.0f;
    mState.stencilClearValue = 0;

    mState.rasterizer.cullFace = false;
    mState.rasterizer.cullMode = GL_BACK;
    mState.rasterizer.frontFace = GL_CCW;
    mState.rasterizer.polygonOffsetFill = false;
    mState.rasterizer.polygonOffsetFactor = 0.0f;
    mState.rasterizer.polygonOffsetUnits = 0.0f;
    mState.rasterizer.pointDrawMode = false;
    mState.rasterizer.multiSample = false;
    mState.scissorTest = false;
    mState.scissor.x = 0;
    mState.scissor.y = 0;
    mState.scissor.width = 0;
    mState.scissor.height = 0;

    mState.blend.blend = false;
    mState.blend.sourceBlendRGB = GL_ONE;
    mState.blend.sourceBlendAlpha = GL_ONE;
    mState.blend.destBlendRGB = GL_ZERO;
    mState.blend.destBlendAlpha = GL_ZERO;
    mState.blend.blendEquationRGB = GL_FUNC_ADD;
    mState.blend.blendEquationAlpha = GL_FUNC_ADD;
    mState.blend.sampleAlphaToCoverage = false;
    mState.blend.dither = true;

    mState.blendColor.red = 0;
    mState.blendColor.green = 0;
    mState.blendColor.blue = 0;
    mState.blendColor.alpha = 0;

    mState.depthStencil.depthTest = false;
    mState.depthStencil.depthFunc = GL_LESS;
    mState.depthStencil.depthMask = true;
    mState.depthStencil.stencilTest = false;
    mState.depthStencil.stencilFunc = GL_ALWAYS;
    mState.depthStencil.stencilMask = -1;
    mState.depthStencil.stencilWritemask = -1;
    mState.depthStencil.stencilBackFunc = GL_ALWAYS;
    mState.depthStencil.stencilBackMask = - 1;
    mState.depthStencil.stencilBackWritemask = -1;
    mState.depthStencil.stencilFail = GL_KEEP;
    mState.depthStencil.stencilPassDepthFail = GL_KEEP;
    mState.depthStencil.stencilPassDepthPass = GL_KEEP;
    mState.depthStencil.stencilBackFail = GL_KEEP;
    mState.depthStencil.stencilBackPassDepthFail = GL_KEEP;
    mState.depthStencil.stencilBackPassDepthPass = GL_KEEP;

    mState.stencilRef = 0;
    mState.stencilBackRef = 0;

    mState.sampleCoverage = false;
    mState.sampleCoverageValue = 1.0f;
    mState.sampleCoverageInvert = false;
    mState.generateMipmapHint = GL_DONT_CARE;
    mState.fragmentShaderDerivativeHint = GL_DONT_CARE;

    mState.lineWidth = 1.0f;

    mState.viewport.x = 0;
    mState.viewport.y = 0;
    mState.viewport.width = 0;
    mState.viewport.height = 0;
    mState.zNear = 0.0f;
    mState.zFar = 1.0f;

    mState.blend.colorMaskRed = true;
    mState.blend.colorMaskGreen = true;
    mState.blend.colorMaskBlue = true;
    mState.blend.colorMaskAlpha = true;

    if (shareContext != NULL)
    {
        mResourceManager = shareContext->mResourceManager;
        mResourceManager->addRef();
    }
    else
    {
        mResourceManager = new ResourceManager(mRenderer);
    }

    // [OpenGL ES 2.0.24] section 3.7 page 83:
    // In the initial state, TEXTURE_2D and TEXTURE_CUBE_MAP have twodimensional
    // and cube map texture state vectors respectively associated with them.
    // In order that access to these initial textures not be lost, they are treated as texture
    // objects all of whose names are 0.

    mTexture2DZero.set(new Texture2D(mRenderer, 0));
    mTextureCubeMapZero.set(new TextureCubeMap(mRenderer, 0));

    mState.activeSampler = 0;
    bindArrayBuffer(0);
    bindElementArrayBuffer(0);
    bindTextureCubeMap(0);
    bindTexture2D(0);
    bindReadFramebuffer(0);
    bindDrawFramebuffer(0);
    bindRenderbuffer(0);

    mState.currentProgram = 0;
    mCurrentProgramBinary.set(NULL);

    mState.packAlignment = 4;
    mState.unpackAlignment = 4;
    mState.packReverseRowOrder = false;

    mExtensionString = NULL;
    mRendererString = NULL;

    mInvalidEnum = false;
    mInvalidValue = false;
    mInvalidOperation = false;
    mOutOfMemory = false;
    mInvalidFramebufferOperation = false;

    mHasBeenCurrent = false;
    mContextLost = false;
    mResetStatus = GL_NO_ERROR;
    mResetStrategy = (notifyResets ? GL_LOSE_CONTEXT_ON_RESET_EXT : GL_NO_RESET_NOTIFICATION_EXT);
    mRobustAccess = robustAccess;

    mSupportsBGRATextures = false;
    mSupportsDXT1Textures = false;
    mSupportsDXT3Textures = false;
    mSupportsDXT5Textures = false;
    mSupportsEventQueries = false;
    mSupportsOcclusionQueries = false;
    mNumCompressedTextureFormats = 0;
}

Context::~Context()
{
    if (mState.currentProgram != 0)
    {
        Program *programObject = mResourceManager->getProgram(mState.currentProgram);
        if (programObject)
        {
            programObject->release();
        }
        mState.currentProgram = 0;
    }
    mCurrentProgramBinary.set(NULL);

    while (!mFramebufferMap.empty())
    {
        deleteFramebuffer(mFramebufferMap.begin()->first);
    }

    while (!mFenceMap.empty())
    {
        deleteFence(mFenceMap.begin()->first);
    }

    while (!mQueryMap.empty())
    {
        deleteQuery(mQueryMap.begin()->first);
    }

    for (int type = 0; type < TEXTURE_TYPE_COUNT; type++)
    {
        for (int sampler = 0; sampler < IMPLEMENTATION_MAX_COMBINED_TEXTURE_IMAGE_UNITS; sampler++)
        {
            mState.samplerTexture[type][sampler].set(NULL);
        }
    }

    for (int type = 0; type < TEXTURE_TYPE_COUNT; type++)
    {
        mIncompleteTextures[type].set(NULL);
    }

    for (int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
    {
        mState.vertexAttribute[i].mBoundBuffer.set(NULL);
    }

    for (int i = 0; i < QUERY_TYPE_COUNT; i++)
    {
        mState.activeQuery[i].set(NULL);
    }

    mState.arrayBuffer.set(NULL);
    mState.elementArrayBuffer.set(NULL);
    mState.renderbuffer.set(NULL);

    mTexture2DZero.set(NULL);
    mTextureCubeMapZero.set(NULL);

    mResourceManager->release();
}

void Context::makeCurrent(egl::Surface *surface)
{
    if (!mHasBeenCurrent)
    {
        mMajorShaderModel = mRenderer->getMajorShaderModel();
        mMaximumPointSize = mRenderer->getMaxPointSize();
        mSupportsVertexTexture = mRenderer->getVertexTextureSupport();
        mSupportsNonPower2Texture = mRenderer->getNonPower2TextureSupport();
        mSupportsInstancing = mRenderer->getInstancingSupport();

        mMaxViewportDimension = mRenderer->getMaxViewportDimension();
        mMaxTextureDimension = std::min(std::min(mRenderer->getMaxTextureWidth(), mRenderer->getMaxTextureHeight()),
                                        (int)gl::IMPLEMENTATION_MAX_TEXTURE_SIZE);
        mMaxCubeTextureDimension = std::min(mMaxTextureDimension, (int)gl::IMPLEMENTATION_MAX_CUBE_MAP_TEXTURE_SIZE);
        mMaxRenderbufferDimension = mMaxTextureDimension;
        mMaxTextureLevel = log2(mMaxTextureDimension) + 1;
        mMaxTextureAnisotropy = mRenderer->getTextureMaxAnisotropy();
        TRACE("MaxTextureDimension=%d, MaxCubeTextureDimension=%d, MaxRenderbufferDimension=%d, MaxTextureLevel=%d, MaxTextureAnisotropy=%f",
              mMaxTextureDimension, mMaxCubeTextureDimension, mMaxRenderbufferDimension, mMaxTextureLevel, mMaxTextureAnisotropy);

        mSupportsEventQueries = mRenderer->getEventQuerySupport();
        mSupportsOcclusionQueries = mRenderer->getOcclusionQuerySupport();
        mSupportsBGRATextures = mRenderer->getBGRATextureSupport();
        mSupportsDXT1Textures = mRenderer->getDXT1TextureSupport();
        mSupportsDXT3Textures = mRenderer->getDXT3TextureSupport();
        mSupportsDXT5Textures = mRenderer->getDXT5TextureSupport();
        mSupportsFloat32Textures = mRenderer->getFloat32TextureSupport(&mSupportsFloat32LinearFilter, &mSupportsFloat32RenderableTextures);
        mSupportsFloat16Textures = mRenderer->getFloat16TextureSupport(&mSupportsFloat16LinearFilter, &mSupportsFloat16RenderableTextures);
        mSupportsLuminanceTextures = mRenderer->getLuminanceTextureSupport();
        mSupportsLuminanceAlphaTextures = mRenderer->getLuminanceAlphaTextureSupport();
        mSupportsDepthTextures = mRenderer->getDepthTextureSupport();
        mSupportsTextureFilterAnisotropy = mRenderer->getTextureFilterAnisotropySupport();
        mSupports32bitIndices = mRenderer->get32BitIndexSupport();

        mNumCompressedTextureFormats = 0;
        if (supportsDXT1Textures())
        {
            mNumCompressedTextureFormats += 2;
        }
        if (supportsDXT3Textures())
        {
            mNumCompressedTextureFormats += 1;
        }
        if (supportsDXT5Textures())
        {
            mNumCompressedTextureFormats += 1;
        }

        initExtensionString();
        initRendererString();

        mState.viewport.x = 0;
        mState.viewport.y = 0;
        mState.viewport.width = surface->getWidth();
        mState.viewport.height = surface->getHeight();

        mState.scissor.x = 0;
        mState.scissor.y = 0;
        mState.scissor.width = surface->getWidth();
        mState.scissor.height = surface->getHeight();

        mHasBeenCurrent = true;
    }

    // Wrap the existing swapchain resources into GL objects and assign them to the '0' names
    rx::SwapChain *swapchain = surface->getSwapChain();

    Colorbuffer *colorbufferZero = new Colorbuffer(mRenderer, swapchain);
    DepthStencilbuffer *depthStencilbufferZero = new DepthStencilbuffer(mRenderer, swapchain);
    Framebuffer *framebufferZero = new DefaultFramebuffer(mRenderer, colorbufferZero, depthStencilbufferZero);

    setFramebufferZero(framebufferZero);
}

// NOTE: this function should not assume that this context is current!
void Context::markContextLost()
{
    if (mResetStrategy == GL_LOSE_CONTEXT_ON_RESET_EXT)
        mResetStatus = GL_UNKNOWN_CONTEXT_RESET_EXT;
    mContextLost = true;
}

bool Context::isContextLost()
{
    return mContextLost;
}

void Context::setClearColor(float red, float green, float blue, float alpha)
{
    mState.colorClearValue.red = red;
    mState.colorClearValue.green = green;
    mState.colorClearValue.blue = blue;
    mState.colorClearValue.alpha = alpha;
}

void Context::setClearDepth(float depth)
{
    mState.depthClearValue = depth;
}

void Context::setClearStencil(int stencil)
{
    mState.stencilClearValue = stencil;
}

void Context::setCullFace(bool enabled)
{
    mState.rasterizer.cullFace = enabled;
}

bool Context::isCullFaceEnabled() const
{
    return mState.rasterizer.cullFace;
}

void Context::setCullMode(GLenum mode)
{
    mState.rasterizer.cullMode = mode;
}

void Context::setFrontFace(GLenum front)
{
    mState.rasterizer.frontFace = front;
}

void Context::setDepthTest(bool enabled)
{
    mState.depthStencil.depthTest = enabled;
}

bool Context::isDepthTestEnabled() const
{
    return mState.depthStencil.depthTest;
}

void Context::setDepthFunc(GLenum depthFunc)
{
     mState.depthStencil.depthFunc = depthFunc;
}

void Context::setDepthRange(float zNear, float zFar)
{
    mState.zNear = zNear;
    mState.zFar = zFar;
}

void Context::setBlend(bool enabled)
{
    mState.blend.blend = enabled;
}

bool Context::isBlendEnabled() const
{
    return mState.blend.blend;
}

void Context::setBlendFactors(GLenum sourceRGB, GLenum destRGB, GLenum sourceAlpha, GLenum destAlpha)
{
    mState.blend.sourceBlendRGB = sourceRGB;
    mState.blend.destBlendRGB = destRGB;
    mState.blend.sourceBlendAlpha = sourceAlpha;
    mState.blend.destBlendAlpha = destAlpha;
}

void Context::setBlendColor(float red, float green, float blue, float alpha)
{
    mState.blendColor.red = red;
    mState.blendColor.green = green;
    mState.blendColor.blue = blue;
    mState.blendColor.alpha = alpha;
}

void Context::setBlendEquation(GLenum rgbEquation, GLenum alphaEquation)
{
    mState.blend.blendEquationRGB = rgbEquation;
    mState.blend.blendEquationAlpha = alphaEquation;
}

void Context::setStencilTest(bool enabled)
{
    mState.depthStencil.stencilTest = enabled;
}

bool Context::isStencilTestEnabled() const
{
    return mState.depthStencil.stencilTest;
}

void Context::setStencilParams(GLenum stencilFunc, GLint stencilRef, GLuint stencilMask)
{
    mState.depthStencil.stencilFunc = stencilFunc;
    mState.stencilRef = (stencilRef > 0) ? stencilRef : 0;
    mState.depthStencil.stencilMask = stencilMask;
}

void Context::setStencilBackParams(GLenum stencilBackFunc, GLint stencilBackRef, GLuint stencilBackMask)
{
    mState.depthStencil.stencilBackFunc = stencilBackFunc;
    mState.stencilBackRef = (stencilBackRef > 0) ? stencilBackRef : 0;
    mState.depthStencil.stencilBackMask = stencilBackMask;
}

void Context::setStencilWritemask(GLuint stencilWritemask)
{
    mState.depthStencil.stencilWritemask = stencilWritemask;
}

void Context::setStencilBackWritemask(GLuint stencilBackWritemask)
{
    mState.depthStencil.stencilBackWritemask = stencilBackWritemask;
}

void Context::setStencilOperations(GLenum stencilFail, GLenum stencilPassDepthFail, GLenum stencilPassDepthPass)
{
    mState.depthStencil.stencilFail = stencilFail;
    mState.depthStencil.stencilPassDepthFail = stencilPassDepthFail;
    mState.depthStencil.stencilPassDepthPass = stencilPassDepthPass;
}

void Context::setStencilBackOperations(GLenum stencilBackFail, GLenum stencilBackPassDepthFail, GLenum stencilBackPassDepthPass)
{
    mState.depthStencil.stencilBackFail = stencilBackFail;
    mState.depthStencil.stencilBackPassDepthFail = stencilBackPassDepthFail;
    mState.depthStencil.stencilBackPassDepthPass = stencilBackPassDepthPass;
}

void Context::setPolygonOffsetFill(bool enabled)
{
     mState.rasterizer.polygonOffsetFill = enabled;
}

bool Context::isPolygonOffsetFillEnabled() const
{
    return mState.rasterizer.polygonOffsetFill;
}

void Context::setPolygonOffsetParams(GLfloat factor, GLfloat units)
{
    // An application can pass NaN values here, so handle this gracefully
    mState.rasterizer.polygonOffsetFactor = factor != factor ? 0.0f : factor;
    mState.rasterizer.polygonOffsetUnits = units != units ? 0.0f : units;
}

void Context::setSampleAlphaToCoverage(bool enabled)
{
    mState.blend.sampleAlphaToCoverage = enabled;
}

bool Context::isSampleAlphaToCoverageEnabled() const
{
    return mState.blend.sampleAlphaToCoverage;
}

void Context::setSampleCoverage(bool enabled)
{
    mState.sampleCoverage = enabled;
}

bool Context::isSampleCoverageEnabled() const
{
    return mState.sampleCoverage;
}

void Context::setSampleCoverageParams(GLclampf value, bool invert)
{
    mState.sampleCoverageValue = value;
    mState.sampleCoverageInvert = invert;
}

void Context::setScissorTest(bool enabled)
{
    mState.scissorTest = enabled;
}

bool Context::isScissorTestEnabled() const
{
    return mState.scissorTest;
}

void Context::setDither(bool enabled)
{
    mState.blend.dither = enabled;
}

bool Context::isDitherEnabled() const
{
    return mState.blend.dither;
}

void Context::setLineWidth(GLfloat width)
{
    mState.lineWidth = width;
}

void Context::setGenerateMipmapHint(GLenum hint)
{
    mState.generateMipmapHint = hint;
}

void Context::setFragmentShaderDerivativeHint(GLenum hint)
{
    mState.fragmentShaderDerivativeHint = hint;
    // TODO: Propagate the hint to shader translator so we can write
    // ddx, ddx_coarse, or ddx_fine depending on the hint.
    // Ignore for now. It is valid for implementations to ignore hint.
}

void Context::setViewportParams(GLint x, GLint y, GLsizei width, GLsizei height)
{
    mState.viewport.x = x;
    mState.viewport.y = y;
    mState.viewport.width = width;
    mState.viewport.height = height;
}

void Context::setScissorParams(GLint x, GLint y, GLsizei width, GLsizei height)
{
    mState.scissor.x = x;
    mState.scissor.y = y;
    mState.scissor.width = width;
    mState.scissor.height = height;
}

void Context::setColorMask(bool red, bool green, bool blue, bool alpha)
{
    mState.blend.colorMaskRed = red;
    mState.blend.colorMaskGreen = green;
    mState.blend.colorMaskBlue = blue;
    mState.blend.colorMaskAlpha = alpha;
}

void Context::setDepthMask(bool mask)
{
    mState.depthStencil.depthMask = mask;
}

void Context::setActiveSampler(unsigned int active)
{
    mState.activeSampler = active;
}

GLuint Context::getReadFramebufferHandle() const
{
    return mState.readFramebuffer;
}

GLuint Context::getDrawFramebufferHandle() const
{
    return mState.drawFramebuffer;
}

GLuint Context::getRenderbufferHandle() const
{
    return mState.renderbuffer.id();
}

GLuint Context::getArrayBufferHandle() const
{
    return mState.arrayBuffer.id();
}

GLuint Context::getActiveQuery(GLenum target) const
{
    Query *queryObject = NULL;
    
    switch (target)
    {
      case GL_ANY_SAMPLES_PASSED_EXT:
        queryObject = mState.activeQuery[QUERY_ANY_SAMPLES_PASSED].get();
        break;
      case GL_ANY_SAMPLES_PASSED_CONSERVATIVE_EXT:
        queryObject = mState.activeQuery[QUERY_ANY_SAMPLES_PASSED_CONSERVATIVE].get();
        break;
      default:
        ASSERT(false);
    }

    if (queryObject)
    {
        return queryObject->id();
    }
    else
    {
        return 0;
    }
}

void Context::setEnableVertexAttribArray(unsigned int attribNum, bool enabled)
{
    mState.vertexAttribute[attribNum].mArrayEnabled = enabled;
}

const VertexAttribute &Context::getVertexAttribState(unsigned int attribNum)
{
    return mState.vertexAttribute[attribNum];
}

void Context::setVertexAttribState(unsigned int attribNum, Buffer *boundBuffer, GLint size, GLenum type, bool normalized,
                                   GLsizei stride, const void *pointer)
{
    mState.vertexAttribute[attribNum].mBoundBuffer.set(boundBuffer);
    mState.vertexAttribute[attribNum].mSize = size;
    mState.vertexAttribute[attribNum].mType = type;
    mState.vertexAttribute[attribNum].mNormalized = normalized;
    mState.vertexAttribute[attribNum].mStride = stride;
    mState.vertexAttribute[attribNum].mPointer = pointer;
}

const void *Context::getVertexAttribPointer(unsigned int attribNum) const
{
    return mState.vertexAttribute[attribNum].mPointer;
}

void Context::setPackAlignment(GLint alignment)
{
    mState.packAlignment = alignment;
}

GLint Context::getPackAlignment() const
{
    return mState.packAlignment;
}

void Context::setUnpackAlignment(GLint alignment)
{
    mState.unpackAlignment = alignment;
}

GLint Context::getUnpackAlignment() const
{
    return mState.unpackAlignment;
}

void Context::setPackReverseRowOrder(bool reverseRowOrder)
{
    mState.packReverseRowOrder = reverseRowOrder;
}

bool Context::getPackReverseRowOrder() const
{
    return mState.packReverseRowOrder;
}

GLuint Context::createBuffer()
{
    return mResourceManager->createBuffer();
}

GLuint Context::createProgram()
{
    return mResourceManager->createProgram();
}

GLuint Context::createShader(GLenum type)
{
    return mResourceManager->createShader(type);
}

GLuint Context::createTexture()
{
    return mResourceManager->createTexture();
}

GLuint Context::createRenderbuffer()
{
    return mResourceManager->createRenderbuffer();
}

// Returns an unused framebuffer name
GLuint Context::createFramebuffer()
{
    GLuint handle = mFramebufferHandleAllocator.allocate();

    mFramebufferMap[handle] = NULL;

    return handle;
}

GLuint Context::createFence()
{
    GLuint handle = mFenceHandleAllocator.allocate();

    mFenceMap[handle] = new Fence(mRenderer);

    return handle;
}

// Returns an unused query name
GLuint Context::createQuery()
{
    GLuint handle = mQueryHandleAllocator.allocate();

    mQueryMap[handle] = NULL;

    return handle;
}

void Context::deleteBuffer(GLuint buffer)
{
    if (mResourceManager->getBuffer(buffer))
    {
        detachBuffer(buffer);
    }
    
    mResourceManager->deleteBuffer(buffer);
}

void Context::deleteShader(GLuint shader)
{
    mResourceManager->deleteShader(shader);
}

void Context::deleteProgram(GLuint program)
{
    mResourceManager->deleteProgram(program);
}

void Context::deleteTexture(GLuint texture)
{
    if (mResourceManager->getTexture(texture))
    {
        detachTexture(texture);
    }

    mResourceManager->deleteTexture(texture);
}

void Context::deleteRenderbuffer(GLuint renderbuffer)
{
    if (mResourceManager->getRenderbuffer(renderbuffer))
    {
        detachRenderbuffer(renderbuffer);
    }
    
    mResourceManager->deleteRenderbuffer(renderbuffer);
}

void Context::deleteFramebuffer(GLuint framebuffer)
{
    FramebufferMap::iterator framebufferObject = mFramebufferMap.find(framebuffer);

    if (framebufferObject != mFramebufferMap.end())
    {
        detachFramebuffer(framebuffer);

        mFramebufferHandleAllocator.release(framebufferObject->first);
        delete framebufferObject->second;
        mFramebufferMap.erase(framebufferObject);
    }
}

void Context::deleteFence(GLuint fence)
{
    FenceMap::iterator fenceObject = mFenceMap.find(fence);

    if (fenceObject != mFenceMap.end())
    {
        mFenceHandleAllocator.release(fenceObject->first);
        delete fenceObject->second;
        mFenceMap.erase(fenceObject);
    }
}

void Context::deleteQuery(GLuint query)
{
    QueryMap::iterator queryObject = mQueryMap.find(query);
    if (queryObject != mQueryMap.end())
    {
        mQueryHandleAllocator.release(queryObject->first);
        if (queryObject->second)
        {
            queryObject->second->release();
        }
        mQueryMap.erase(queryObject);
    }
}

Buffer *Context::getBuffer(GLuint handle)
{
    return mResourceManager->getBuffer(handle);
}

Shader *Context::getShader(GLuint handle)
{
    return mResourceManager->getShader(handle);
}

Program *Context::getProgram(GLuint handle)
{
    return mResourceManager->getProgram(handle);
}

Texture *Context::getTexture(GLuint handle)
{
    return mResourceManager->getTexture(handle);
}

Renderbuffer *Context::getRenderbuffer(GLuint handle)
{
    return mResourceManager->getRenderbuffer(handle);
}

Framebuffer *Context::getReadFramebuffer()
{
    return getFramebuffer(mState.readFramebuffer);
}

Framebuffer *Context::getDrawFramebuffer()
{
    return mBoundDrawFramebuffer;
}

void Context::bindArrayBuffer(unsigned int buffer)
{
    mResourceManager->checkBufferAllocation(buffer);

    mState.arrayBuffer.set(getBuffer(buffer));
}

void Context::bindElementArrayBuffer(unsigned int buffer)
{
    mResourceManager->checkBufferAllocation(buffer);

    mState.elementArrayBuffer.set(getBuffer(buffer));
}

void Context::bindTexture2D(GLuint texture)
{
    mResourceManager->checkTextureAllocation(texture, TEXTURE_2D);

    mState.samplerTexture[TEXTURE_2D][mState.activeSampler].set(getTexture(texture));
}

void Context::bindTextureCubeMap(GLuint texture)
{
    mResourceManager->checkTextureAllocation(texture, TEXTURE_CUBE);

    mState.samplerTexture[TEXTURE_CUBE][mState.activeSampler].set(getTexture(texture));
}

void Context::bindReadFramebuffer(GLuint framebuffer)
{
    if (!getFramebuffer(framebuffer))
    {
        mFramebufferMap[framebuffer] = new Framebuffer(mRenderer);
    }

    mState.readFramebuffer = framebuffer;
}

void Context::bindDrawFramebuffer(GLuint framebuffer)
{
    if (!getFramebuffer(framebuffer))
    {
        mFramebufferMap[framebuffer] = new Framebuffer(mRenderer);
    }

    mState.drawFramebuffer = framebuffer;

    mBoundDrawFramebuffer = getFramebuffer(framebuffer);
}

void Context::bindRenderbuffer(GLuint renderbuffer)
{
    mResourceManager->checkRenderbufferAllocation(renderbuffer);

    mState.renderbuffer.set(getRenderbuffer(renderbuffer));
}

void Context::useProgram(GLuint program)
{
    GLuint priorProgram = mState.currentProgram;
    mState.currentProgram = program;               // Must switch before trying to delete, otherwise it only gets flagged.

    if (priorProgram != program)
    {
        Program *newProgram = mResourceManager->getProgram(program);
        Program *oldProgram = mResourceManager->getProgram(priorProgram);
        mCurrentProgramBinary.set(NULL);

        if (newProgram)
        {
            newProgram->addRef();
            mCurrentProgramBinary.set(newProgram->getProgramBinary());
        }
        
        if (oldProgram)
        {
            oldProgram->release();
        }
    }
}

void Context::linkProgram(GLuint program)
{
    Program *programObject = mResourceManager->getProgram(program);

    bool linked = programObject->link();

    // if the current program was relinked successfully we
    // need to install the new executables
    if (linked && program == mState.currentProgram)
    {
        mCurrentProgramBinary.set(programObject->getProgramBinary());
    }
}

void Context::setProgramBinary(GLuint program, const void *binary, GLint length)
{
    Program *programObject = mResourceManager->getProgram(program);

    bool loaded = programObject->setProgramBinary(binary, length);

    // if the current program was reloaded successfully we
    // need to install the new executables
    if (loaded && program == mState.currentProgram)
    {
        mCurrentProgramBinary.set(programObject->getProgramBinary());
    }

}

void Context::beginQuery(GLenum target, GLuint query)
{
    // From EXT_occlusion_query_boolean: If BeginQueryEXT is called with an <id>  
    // of zero, if the active query object name for <target> is non-zero (for the  
    // targets ANY_SAMPLES_PASSED_EXT and ANY_SAMPLES_PASSED_CONSERVATIVE_EXT, if  
    // the active query for either target is non-zero), if <id> is the name of an 
    // existing query object whose type does not match <target>, or if <id> is the
    // active query object name for any query type, the error INVALID_OPERATION is
    // generated.

    // Ensure no other queries are active
    // NOTE: If other queries than occlusion are supported, we will need to check
    // separately that:
    //    a) The query ID passed is not the current active query for any target/type
    //    b) There are no active queries for the requested target (and in the case
    //       of GL_ANY_SAMPLES_PASSED_EXT and GL_ANY_SAMPLES_PASSED_CONSERVATIVE_EXT,
    //       no query may be active for either if glBeginQuery targets either.
    for (int i = 0; i < QUERY_TYPE_COUNT; i++)
    {
        if (mState.activeQuery[i].get() != NULL)
        {
            return gl::error(GL_INVALID_OPERATION);
        }
    }

    QueryType qType;
    switch (target)
    {
      case GL_ANY_SAMPLES_PASSED_EXT: 
        qType = QUERY_ANY_SAMPLES_PASSED; 
        break;
      case GL_ANY_SAMPLES_PASSED_CONSERVATIVE_EXT: 
        qType = QUERY_ANY_SAMPLES_PASSED_CONSERVATIVE; 
        break;
      default: 
        ASSERT(false);
        return;
    }

    Query *queryObject = getQuery(query, true, target);

    // check that name was obtained with glGenQueries
    if (!queryObject)
    {
        return gl::error(GL_INVALID_OPERATION);
    }

    // check for type mismatch
    if (queryObject->getType() != target)
    {
        return gl::error(GL_INVALID_OPERATION);
    }

    // set query as active for specified target
    mState.activeQuery[qType].set(queryObject);

    // begin query
    queryObject->begin();
}

void Context::endQuery(GLenum target)
{
    QueryType qType;

    switch (target)
    {
      case GL_ANY_SAMPLES_PASSED_EXT: 
        qType = QUERY_ANY_SAMPLES_PASSED; 
        break;
      case GL_ANY_SAMPLES_PASSED_CONSERVATIVE_EXT: 
        qType = QUERY_ANY_SAMPLES_PASSED_CONSERVATIVE; 
        break;
      default: 
        ASSERT(false);
        return;
    }

    Query *queryObject = mState.activeQuery[qType].get();

    if (queryObject == NULL)
    {
        return gl::error(GL_INVALID_OPERATION);
    }

    queryObject->end();

    mState.activeQuery[qType].set(NULL);
}

void Context::setFramebufferZero(Framebuffer *buffer)
{
    delete mFramebufferMap[0];
    mFramebufferMap[0] = buffer;
    if (mState.drawFramebuffer == 0)
    {
        mBoundDrawFramebuffer = buffer;
    }
}

void Context::setRenderbufferStorage(GLsizei width, GLsizei height, GLenum internalformat, GLsizei samples)
{
    RenderbufferStorage *renderbuffer = NULL;
    switch (internalformat)
    {
      case GL_DEPTH_COMPONENT16:
        renderbuffer = new gl::Depthbuffer(mRenderer, width, height, samples);
        break;
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGB565:
      case GL_RGB8_OES:
      case GL_RGBA8_OES:
        renderbuffer = new gl::Colorbuffer(mRenderer,width, height, internalformat, samples);
        break;
      case GL_STENCIL_INDEX8:
        renderbuffer = new gl::Stencilbuffer(mRenderer, width, height, samples);
        break;
      case GL_DEPTH24_STENCIL8_OES:
        renderbuffer = new gl::DepthStencilbuffer(mRenderer, width, height, samples);
        break;
      default:
        UNREACHABLE(); return;
    }

    Renderbuffer *renderbufferObject = mState.renderbuffer.get();
    renderbufferObject->setStorage(renderbuffer);
}

Framebuffer *Context::getFramebuffer(unsigned int handle)
{
    FramebufferMap::iterator framebuffer = mFramebufferMap.find(handle);

    if (framebuffer == mFramebufferMap.end())
    {
        return NULL;
    }
    else
    {
        return framebuffer->second;
    }
}

Fence *Context::getFence(unsigned int handle)
{
    FenceMap::iterator fence = mFenceMap.find(handle);

    if (fence == mFenceMap.end())
    {
        return NULL;
    }
    else
    {
        return fence->second;
    }
}

Query *Context::getQuery(unsigned int handle, bool create, GLenum type)
{
    QueryMap::iterator query = mQueryMap.find(handle);

    if (query == mQueryMap.end())
    {
        return NULL;
    }
    else
    {
        if (!query->second && create)
        {
            query->second = new Query(mRenderer, type, handle);
            query->second->addRef();
        }
        return query->second;
    }
}

Buffer *Context::getArrayBuffer()
{
    return mState.arrayBuffer.get();
}

Buffer *Context::getElementArrayBuffer()
{
    return mState.elementArrayBuffer.get();
}

ProgramBinary *Context::getCurrentProgramBinary()
{
    return mCurrentProgramBinary.get();
}

Texture2D *Context::getTexture2D()
{
    return static_cast<Texture2D*>(getSamplerTexture(mState.activeSampler, TEXTURE_2D));
}

TextureCubeMap *Context::getTextureCubeMap()
{
    return static_cast<TextureCubeMap*>(getSamplerTexture(mState.activeSampler, TEXTURE_CUBE));
}

Texture *Context::getSamplerTexture(unsigned int sampler, TextureType type)
{
    GLuint texid = mState.samplerTexture[type][sampler].id();

    if (texid == 0)   // Special case: 0 refers to different initial textures based on the target
    {
        switch (type)
        {
          default: UNREACHABLE();
          case TEXTURE_2D: return mTexture2DZero.get();
          case TEXTURE_CUBE: return mTextureCubeMapZero.get();
        }
    }

    return mState.samplerTexture[type][sampler].get();
}

bool Context::getBooleanv(GLenum pname, GLboolean *params)
{
    switch (pname)
    {
      case GL_SHADER_COMPILER:           *params = GL_TRUE;                             break;
      case GL_SAMPLE_COVERAGE_INVERT:    *params = mState.sampleCoverageInvert;         break;
      case GL_DEPTH_WRITEMASK:           *params = mState.depthStencil.depthMask;       break;
      case GL_COLOR_WRITEMASK:
        params[0] = mState.blend.colorMaskRed;
        params[1] = mState.blend.colorMaskGreen;
        params[2] = mState.blend.colorMaskBlue;
        params[3] = mState.blend.colorMaskAlpha;
        break;
      case GL_CULL_FACE:                 *params = mState.rasterizer.cullFace;          break;
      case GL_POLYGON_OFFSET_FILL:       *params = mState.rasterizer.polygonOffsetFill; break;
      case GL_SAMPLE_ALPHA_TO_COVERAGE:  *params = mState.blend.sampleAlphaToCoverage;  break;
      case GL_SAMPLE_COVERAGE:           *params = mState.sampleCoverage;               break;
      case GL_SCISSOR_TEST:              *params = mState.scissorTest;                  break;
      case GL_STENCIL_TEST:              *params = mState.depthStencil.stencilTest;     break;
      case GL_DEPTH_TEST:                *params = mState.depthStencil.depthTest;       break;
      case GL_BLEND:                     *params = mState.blend.blend;                  break;
      case GL_DITHER:                    *params = mState.blend.dither;                 break;
      case GL_CONTEXT_ROBUST_ACCESS_EXT: *params = mRobustAccess ? GL_TRUE : GL_FALSE;  break;
      default:
        return false;
    }

    return true;
}

bool Context::getFloatv(GLenum pname, GLfloat *params)
{
    // Please note: DEPTH_CLEAR_VALUE is included in our internal getFloatv implementation
    // because it is stored as a float, despite the fact that the GL ES 2.0 spec names
    // GetIntegerv as its native query function. As it would require conversion in any
    // case, this should make no difference to the calling application.
    switch (pname)
    {
      case GL_LINE_WIDTH:               *params = mState.lineWidth;                         break;
      case GL_SAMPLE_COVERAGE_VALUE:    *params = mState.sampleCoverageValue;               break;
      case GL_DEPTH_CLEAR_VALUE:        *params = mState.depthClearValue;                   break;
      case GL_POLYGON_OFFSET_FACTOR:    *params = mState.rasterizer.polygonOffsetFactor;    break;
      case GL_POLYGON_OFFSET_UNITS:     *params = mState.rasterizer.polygonOffsetUnits;     break;
      case GL_ALIASED_LINE_WIDTH_RANGE:
        params[0] = gl::ALIASED_LINE_WIDTH_RANGE_MIN;
        params[1] = gl::ALIASED_LINE_WIDTH_RANGE_MAX;
        break;
      case GL_ALIASED_POINT_SIZE_RANGE:
        params[0] = gl::ALIASED_POINT_SIZE_RANGE_MIN;
        params[1] = getMaximumPointSize();
        break;
      case GL_DEPTH_RANGE:
        params[0] = mState.zNear;
        params[1] = mState.zFar;
        break;
      case GL_COLOR_CLEAR_VALUE:
        params[0] = mState.colorClearValue.red;
        params[1] = mState.colorClearValue.green;
        params[2] = mState.colorClearValue.blue;
        params[3] = mState.colorClearValue.alpha;
        break;
      case GL_BLEND_COLOR:
        params[0] = mState.blendColor.red;
        params[1] = mState.blendColor.green;
        params[2] = mState.blendColor.blue;
        params[3] = mState.blendColor.alpha;
        break;
      case GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT:
        if (!supportsTextureFilterAnisotropy())
        {
            return false;
        }
        *params = mMaxTextureAnisotropy;
        break;
      default:
        return false;
    }

    return true;
}

bool Context::getIntegerv(GLenum pname, GLint *params)
{
    if (pname >= GL_DRAW_BUFFER0_EXT && pname <= GL_DRAW_BUFFER15_EXT)
    {
        unsigned int colorAttachment = (pname - GL_DRAW_BUFFER0_EXT);

        if (colorAttachment >= mRenderer->getMaxRenderTargets())
        {
            // return true to stop further operation in the parent call
            return gl::error(GL_INVALID_OPERATION, true);
        }

        Framebuffer *framebuffer = getDrawFramebuffer();

        *params = framebuffer->getDrawBufferState(colorAttachment);
        return true;
    }

    // Please note: DEPTH_CLEAR_VALUE is not included in our internal getIntegerv implementation
    // because it is stored as a float, despite the fact that the GL ES 2.0 spec names
    // GetIntegerv as its native query function. As it would require conversion in any
    // case, this should make no difference to the calling application. You may find it in 
    // Context::getFloatv.
    switch (pname)
    {
      case GL_MAX_VERTEX_ATTRIBS:               *params = gl::MAX_VERTEX_ATTRIBS;               break;
      case GL_MAX_VERTEX_UNIFORM_VECTORS:       *params = mRenderer->getMaxVertexUniformVectors(); break;
      case GL_MAX_VARYING_VECTORS:              *params = mRenderer->getMaxVaryingVectors();    break;
      case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS: *params = mRenderer->getMaxCombinedTextureImageUnits(); break;
      case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:   *params = mRenderer->getMaxVertexTextureImageUnits(); break;
      case GL_MAX_TEXTURE_IMAGE_UNITS:          *params = gl::MAX_TEXTURE_IMAGE_UNITS;          break;
      case GL_MAX_FRAGMENT_UNIFORM_VECTORS:     *params = mRenderer->getMaxFragmentUniformVectors(); break;
      case GL_MAX_RENDERBUFFER_SIZE:            *params = getMaximumRenderbufferDimension();    break;
      case GL_MAX_COLOR_ATTACHMENTS_EXT:        *params = mRenderer->getMaxRenderTargets();     break;
      case GL_MAX_DRAW_BUFFERS_EXT:             *params = mRenderer->getMaxRenderTargets();     break;
      case GL_NUM_SHADER_BINARY_FORMATS:        *params = 0;                                    break;
      case GL_SHADER_BINARY_FORMATS:      /* no shader binary formats are supported */          break;
      case GL_ARRAY_BUFFER_BINDING:             *params = mState.arrayBuffer.id();              break;
      case GL_ELEMENT_ARRAY_BUFFER_BINDING:     *params = mState.elementArrayBuffer.id();       break;
      //case GL_FRAMEBUFFER_BINDING:            // now equivalent to GL_DRAW_FRAMEBUFFER_BINDING_ANGLE
      case GL_DRAW_FRAMEBUFFER_BINDING_ANGLE:   *params = mState.drawFramebuffer;               break;
      case GL_READ_FRAMEBUFFER_BINDING_ANGLE:   *params = mState.readFramebuffer;               break;
      case GL_RENDERBUFFER_BINDING:             *params = mState.renderbuffer.id();             break;
      case GL_CURRENT_PROGRAM:                  *params = mState.currentProgram;                break;
      case GL_PACK_ALIGNMENT:                   *params = mState.packAlignment;                 break;
      case GL_PACK_REVERSE_ROW_ORDER_ANGLE:     *params = mState.packReverseRowOrder;           break;
      case GL_UNPACK_ALIGNMENT:                 *params = mState.unpackAlignment;               break;
      case GL_GENERATE_MIPMAP_HINT:             *params = mState.generateMipmapHint;            break;
      case GL_FRAGMENT_SHADER_DERIVATIVE_HINT_OES: *params = mState.fragmentShaderDerivativeHint; break;
      case GL_ACTIVE_TEXTURE:                   *params = (mState.activeSampler + GL_TEXTURE0); break;
      case GL_STENCIL_FUNC:                     *params = mState.depthStencil.stencilFunc;             break;
      case GL_STENCIL_REF:                      *params = mState.stencilRef;                           break;
      case GL_STENCIL_VALUE_MASK:               *params = mState.depthStencil.stencilMask;             break;
      case GL_STENCIL_BACK_FUNC:                *params = mState.depthStencil.stencilBackFunc;         break;
      case GL_STENCIL_BACK_REF:                 *params = mState.stencilBackRef;                       break;
      case GL_STENCIL_BACK_VALUE_MASK:          *params = mState.depthStencil.stencilBackMask;         break;
      case GL_STENCIL_FAIL:                     *params = mState.depthStencil.stencilFail;             break;
      case GL_STENCIL_PASS_DEPTH_FAIL:          *params = mState.depthStencil.stencilPassDepthFail;    break;
      case GL_STENCIL_PASS_DEPTH_PASS:          *params = mState.depthStencil.stencilPassDepthPass;    break;
      case GL_STENCIL_BACK_FAIL:                *params = mState.depthStencil.stencilBackFail;         break;
      case GL_STENCIL_BACK_PASS_DEPTH_FAIL:     *params = mState.depthStencil.stencilBackPassDepthFail; break;
      case GL_STENCIL_BACK_PASS_DEPTH_PASS:     *params = mState.depthStencil.stencilBackPassDepthPass; break;
      case GL_DEPTH_FUNC:                       *params = mState.depthStencil.depthFunc;               break;
      case GL_BLEND_SRC_RGB:                    *params = mState.blend.sourceBlendRGB;                 break;
      case GL_BLEND_SRC_ALPHA:                  *params = mState.blend.sourceBlendAlpha;               break;
      case GL_BLEND_DST_RGB:                    *params = mState.blend.destBlendRGB;                   break;
      case GL_BLEND_DST_ALPHA:                  *params = mState.blend.destBlendAlpha;                 break;
      case GL_BLEND_EQUATION_RGB:               *params = mState.blend.blendEquationRGB;               break;
      case GL_BLEND_EQUATION_ALPHA:             *params = mState.blend.blendEquationAlpha;             break;
      case GL_STENCIL_WRITEMASK:                *params = mState.depthStencil.stencilWritemask;        break;
      case GL_STENCIL_BACK_WRITEMASK:           *params = mState.depthStencil.stencilBackWritemask;    break;
      case GL_STENCIL_CLEAR_VALUE:              *params = mState.stencilClearValue;             break;
      case GL_SUBPIXEL_BITS:                    *params = 4;                                    break;
      case GL_MAX_TEXTURE_SIZE:                 *params = getMaximumTextureDimension();         break;
      case GL_MAX_CUBE_MAP_TEXTURE_SIZE:        *params = getMaximumCubeTextureDimension();     break;
      case GL_NUM_COMPRESSED_TEXTURE_FORMATS:   
        params[0] = mNumCompressedTextureFormats;
        break;
      case GL_MAX_SAMPLES_ANGLE:
        {
            GLsizei maxSamples = getMaxSupportedSamples();
            if (maxSamples != 0)
            {
                *params = maxSamples;
            }
            else
            {
                return false;
            }

            break;
        }
      case GL_SAMPLE_BUFFERS:                   
      case GL_SAMPLES:
        {
            gl::Framebuffer *framebuffer = getDrawFramebuffer();
            if (framebuffer->completeness() == GL_FRAMEBUFFER_COMPLETE)
            {
                switch (pname)
                {
                  case GL_SAMPLE_BUFFERS:
                    if (framebuffer->getSamples() != 0)
                    {
                        *params = 1;
                    }
                    else
                    {
                        *params = 0;
                    }
                    break;
                  case GL_SAMPLES:
                    *params = framebuffer->getSamples();
                    break;
                }
            }
            else 
            {
                *params = 0;
            }
        }
        break;
      case GL_IMPLEMENTATION_COLOR_READ_TYPE:
      case GL_IMPLEMENTATION_COLOR_READ_FORMAT:
        {
            GLenum format, type;
            if (getCurrentReadFormatType(&format, &type))
            {
                if (pname == GL_IMPLEMENTATION_COLOR_READ_FORMAT)
                    *params = format;
                else
                    *params = type;
            }
        }
        break;
      case GL_MAX_VIEWPORT_DIMS:
        {
            params[0] = mMaxViewportDimension;
            params[1] = mMaxViewportDimension;
        }
        break;
      case GL_COMPRESSED_TEXTURE_FORMATS:
        {
            if (supportsDXT1Textures())
            {
                *params++ = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
                *params++ = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            }
            if (supportsDXT3Textures())
            {
                *params++ = GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE;
            }
            if (supportsDXT5Textures())
            {
                *params++ = GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE;
            }
        }
        break;
      case GL_VIEWPORT:
        params[0] = mState.viewport.x;
        params[1] = mState.viewport.y;
        params[2] = mState.viewport.width;
        params[3] = mState.viewport.height;
        break;
      case GL_SCISSOR_BOX:
        params[0] = mState.scissor.x;
        params[1] = mState.scissor.y;
        params[2] = mState.scissor.width;
        params[3] = mState.scissor.height;
        break;
      case GL_CULL_FACE_MODE:                   *params = mState.rasterizer.cullMode;   break;
      case GL_FRONT_FACE:                       *params = mState.rasterizer.frontFace;  break;
      case GL_RED_BITS:
      case GL_GREEN_BITS:
      case GL_BLUE_BITS:
      case GL_ALPHA_BITS:
        {
            gl::Framebuffer *framebuffer = getDrawFramebuffer();
            gl::Renderbuffer *colorbuffer = framebuffer->getFirstColorbuffer();

            if (colorbuffer)
            {
                switch (pname)
                {
                  case GL_RED_BITS:   *params = colorbuffer->getRedSize();      break;
                  case GL_GREEN_BITS: *params = colorbuffer->getGreenSize();    break;
                  case GL_BLUE_BITS:  *params = colorbuffer->getBlueSize();     break;
                  case GL_ALPHA_BITS: *params = colorbuffer->getAlphaSize();    break;
                }
            }
            else
            {
                *params = 0;
            }
        }
        break;
      case GL_DEPTH_BITS:
        {
            gl::Framebuffer *framebuffer = getDrawFramebuffer();
            gl::Renderbuffer *depthbuffer = framebuffer->getDepthbuffer();

            if (depthbuffer)
            {
                *params = depthbuffer->getDepthSize();
            }
            else
            {
                *params = 0;
            }
        }
        break;
      case GL_STENCIL_BITS:
        {
            gl::Framebuffer *framebuffer = getDrawFramebuffer();
            gl::Renderbuffer *stencilbuffer = framebuffer->getStencilbuffer();

            if (stencilbuffer)
            {
                *params = stencilbuffer->getStencilSize();
            }
            else
            {
                *params = 0;
            }
        }
        break;
      case GL_TEXTURE_BINDING_2D:
        {
            if (mState.activeSampler > mRenderer->getMaxCombinedTextureImageUnits() - 1)
            {
                gl::error(GL_INVALID_OPERATION);
                return false;
            }

            *params = mState.samplerTexture[TEXTURE_2D][mState.activeSampler].id();
        }
        break;
      case GL_TEXTURE_BINDING_CUBE_MAP:
        {
            if (mState.activeSampler > mRenderer->getMaxCombinedTextureImageUnits() - 1)
            {
                gl::error(GL_INVALID_OPERATION);
                return false;
            }

            *params = mState.samplerTexture[TEXTURE_CUBE][mState.activeSampler].id();
        }
        break;
      case GL_RESET_NOTIFICATION_STRATEGY_EXT:
        *params = mResetStrategy;
        break;
      case GL_NUM_PROGRAM_BINARY_FORMATS_OES:
        *params = 1;
        break;
      case GL_PROGRAM_BINARY_FORMATS_OES:
        *params = GL_PROGRAM_BINARY_ANGLE;
        break;
      default:
        return false;
    }

    return true;
}

bool Context::getQueryParameterInfo(GLenum pname, GLenum *type, unsigned int *numParams)
{
    if (pname >= GL_DRAW_BUFFER0_EXT && pname <= GL_DRAW_BUFFER15_EXT)
    {
        *type = GL_INT;
        *numParams = 1;
        return true;
    }

    // Please note: the query type returned for DEPTH_CLEAR_VALUE in this implementation
    // is FLOAT rather than INT, as would be suggested by the GL ES 2.0 spec. This is due
    // to the fact that it is stored internally as a float, and so would require conversion
    // if returned from Context::getIntegerv. Since this conversion is already implemented 
    // in the case that one calls glGetIntegerv to retrieve a float-typed state variable, we
    // place DEPTH_CLEAR_VALUE with the floats. This should make no difference to the calling
    // application.
    switch (pname)
    {
      case GL_COMPRESSED_TEXTURE_FORMATS:
        {
            *type = GL_INT;
            *numParams = mNumCompressedTextureFormats;
        }
        break;
      case GL_SHADER_BINARY_FORMATS:
        {
            *type = GL_INT;
            *numParams = 0;
        }
        break;
      case GL_MAX_VERTEX_ATTRIBS:
      case GL_MAX_VERTEX_UNIFORM_VECTORS:
      case GL_MAX_VARYING_VECTORS:
      case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:
      case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
      case GL_MAX_TEXTURE_IMAGE_UNITS:
      case GL_MAX_FRAGMENT_UNIFORM_VECTORS:
      case GL_MAX_RENDERBUFFER_SIZE:
      case GL_MAX_COLOR_ATTACHMENTS_EXT:
      case GL_MAX_DRAW_BUFFERS_EXT:
      case GL_NUM_SHADER_BINARY_FORMATS:
      case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
      case GL_ARRAY_BUFFER_BINDING:
      case GL_FRAMEBUFFER_BINDING:
      case GL_RENDERBUFFER_BINDING:
      case GL_CURRENT_PROGRAM:
      case GL_PACK_ALIGNMENT:
      case GL_PACK_REVERSE_ROW_ORDER_ANGLE:
      case GL_UNPACK_ALIGNMENT:
      case GL_GENERATE_MIPMAP_HINT:
      case GL_FRAGMENT_SHADER_DERIVATIVE_HINT_OES:
      case GL_RED_BITS:
      case GL_GREEN_BITS:
      case GL_BLUE_BITS:
      case GL_ALPHA_BITS:
      case GL_DEPTH_BITS:
      case GL_STENCIL_BITS:
      case GL_ELEMENT_ARRAY_BUFFER_BINDING:
      case GL_CULL_FACE_MODE:
      case GL_FRONT_FACE:
      case GL_ACTIVE_TEXTURE:
      case GL_STENCIL_FUNC:
      case GL_STENCIL_VALUE_MASK:
      case GL_STENCIL_REF:
      case GL_STENCIL_FAIL:
      case GL_STENCIL_PASS_DEPTH_FAIL:
      case GL_STENCIL_PASS_DEPTH_PASS:
      case GL_STENCIL_BACK_FUNC:
      case GL_STENCIL_BACK_VALUE_MASK:
      case GL_STENCIL_BACK_REF:
      case GL_STENCIL_BACK_FAIL:
      case GL_STENCIL_BACK_PASS_DEPTH_FAIL:
      case GL_STENCIL_BACK_PASS_DEPTH_PASS:
      case GL_DEPTH_FUNC:
      case GL_BLEND_SRC_RGB:
      case GL_BLEND_SRC_ALPHA:
      case GL_BLEND_DST_RGB:
      case GL_BLEND_DST_ALPHA:
      case GL_BLEND_EQUATION_RGB:
      case GL_BLEND_EQUATION_ALPHA:
      case GL_STENCIL_WRITEMASK:
      case GL_STENCIL_BACK_WRITEMASK:
      case GL_STENCIL_CLEAR_VALUE:
      case GL_SUBPIXEL_BITS:
      case GL_MAX_TEXTURE_SIZE:
      case GL_MAX_CUBE_MAP_TEXTURE_SIZE:
      case GL_SAMPLE_BUFFERS:
      case GL_SAMPLES:
      case GL_IMPLEMENTATION_COLOR_READ_TYPE:
      case GL_IMPLEMENTATION_COLOR_READ_FORMAT:
      case GL_TEXTURE_BINDING_2D:
      case GL_TEXTURE_BINDING_CUBE_MAP:
      case GL_RESET_NOTIFICATION_STRATEGY_EXT:
      case GL_NUM_PROGRAM_BINARY_FORMATS_OES:
      case GL_PROGRAM_BINARY_FORMATS_OES:
        {
            *type = GL_INT;
            *numParams = 1;
        }
        break;
      case GL_MAX_SAMPLES_ANGLE:
        {
            if (getMaxSupportedSamples() != 0)
            {
                *type = GL_INT;
                *numParams = 1;
            }
            else
            {
                return false;
            }
        }
        break;
      case GL_MAX_VIEWPORT_DIMS:
        {
            *type = GL_INT;
            *numParams = 2;
        }
        break;
      case GL_VIEWPORT:
      case GL_SCISSOR_BOX:
        {
            *type = GL_INT;
            *numParams = 4;
        }
        break;
      case GL_SHADER_COMPILER:
      case GL_SAMPLE_COVERAGE_INVERT:
      case GL_DEPTH_WRITEMASK:
      case GL_CULL_FACE:                // CULL_FACE through DITHER are natural to IsEnabled,
      case GL_POLYGON_OFFSET_FILL:      // but can be retrieved through the Get{Type}v queries.
      case GL_SAMPLE_ALPHA_TO_COVERAGE: // For this purpose, they are treated here as bool-natural
      case GL_SAMPLE_COVERAGE:
      case GL_SCISSOR_TEST:
      case GL_STENCIL_TEST:
      case GL_DEPTH_TEST:
      case GL_BLEND:
      case GL_DITHER:
      case GL_CONTEXT_ROBUST_ACCESS_EXT:
        {
            *type = GL_BOOL;
            *numParams = 1;
        }
        break;
      case GL_COLOR_WRITEMASK:
        {
            *type = GL_BOOL;
            *numParams = 4;
        }
        break;
      case GL_POLYGON_OFFSET_FACTOR:
      case GL_POLYGON_OFFSET_UNITS:
      case GL_SAMPLE_COVERAGE_VALUE:
      case GL_DEPTH_CLEAR_VALUE:
      case GL_LINE_WIDTH:
        {
            *type = GL_FLOAT;
            *numParams = 1;
        }
        break;
      case GL_ALIASED_LINE_WIDTH_RANGE:
      case GL_ALIASED_POINT_SIZE_RANGE:
      case GL_DEPTH_RANGE:
        {
            *type = GL_FLOAT;
            *numParams = 2;
        }
        break;
      case GL_COLOR_CLEAR_VALUE:
      case GL_BLEND_COLOR:
        {
            *type = GL_FLOAT;
            *numParams = 4;
        }
        break;
      case GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT:
        if (!supportsTextureFilterAnisotropy())
        {
            return false;
        }
        *type = GL_FLOAT;
        *numParams = 1;
        break;
      default:
        return false;
    }

    return true;
}

// Applies the render target surface, depth stencil surface, viewport rectangle and
// scissor rectangle to the renderer
bool Context::applyRenderTarget(GLenum drawMode, bool ignoreViewport)
{
    Framebuffer *framebufferObject = getDrawFramebuffer();

    if (!framebufferObject || framebufferObject->completeness() != GL_FRAMEBUFFER_COMPLETE)
    {
        return gl::error(GL_INVALID_FRAMEBUFFER_OPERATION, false);
    }

    mRenderer->applyRenderTarget(framebufferObject);

    if (!mRenderer->setViewport(mState.viewport, mState.zNear, mState.zFar, drawMode, mState.rasterizer.frontFace,
                                ignoreViewport))
    {
        return false;
    }

    mRenderer->setScissorRectangle(mState.scissor, mState.scissorTest);

    return true;
}

// Applies the fixed-function state (culling, depth test, alpha blending, stenciling, etc) to the Direct3D 9 device
void Context::applyState(GLenum drawMode)
{
    Framebuffer *framebufferObject = getDrawFramebuffer();
    int samples = framebufferObject->getSamples();

    mState.rasterizer.pointDrawMode = (drawMode == GL_POINTS);
    mState.rasterizer.multiSample = (samples != 0);
    mRenderer->setRasterizerState(mState.rasterizer);

    unsigned int mask = 0;
    if (mState.sampleCoverage)
    {
        if (mState.sampleCoverageValue != 0)
        {
            
            float threshold = 0.5f;

            for (int i = 0; i < samples; ++i)
            {
                mask <<= 1;

                if ((i + 1) * mState.sampleCoverageValue >= threshold)
                {
                    threshold += 1.0f;
                    mask |= 1;
                }
            }
        }

        if (mState.sampleCoverageInvert)
        {
            mask = ~mask;
        }
    }
    else
    {
        mask = 0xFFFFFFFF;
    }
    mRenderer->setBlendState(mState.blend, mState.blendColor, mask);

    mRenderer->setDepthStencilState(mState.depthStencil, mState.stencilRef, mState.stencilBackRef,
                                    mState.rasterizer.frontFace == GL_CCW);
}

// Applies the shaders and shader constants to the Direct3D 9 device
void Context::applyShaders()
{
    ProgramBinary *programBinary = getCurrentProgramBinary();

    mRenderer->applyShaders(programBinary);
    
    programBinary->applyUniforms();
}

// Applies the textures and sampler states to the Direct3D 9 device
void Context::applyTextures()
{
    applyTextures(SAMPLER_PIXEL);

    if (mSupportsVertexTexture)
    {
        applyTextures(SAMPLER_VERTEX);
    }
}

// For each Direct3D 9 sampler of either the pixel or vertex stage,
// looks up the corresponding OpenGL texture image unit and texture type,
// and sets the texture and its addressing/filtering state (or NULL when inactive).
void Context::applyTextures(SamplerType type)
{
    ProgramBinary *programBinary = getCurrentProgramBinary();

    // Range of Direct3D samplers of given sampler type
    int samplerCount = (type == SAMPLER_PIXEL) ? MAX_TEXTURE_IMAGE_UNITS : mRenderer->getMaxVertexTextureImageUnits();
    int samplerRange = programBinary->getUsedSamplerRange(type);

    for (int samplerIndex = 0; samplerIndex < samplerRange; samplerIndex++)
    {
        int textureUnit = programBinary->getSamplerMapping(type, samplerIndex);   // OpenGL texture image unit index

        if (textureUnit != -1)
        {
            TextureType textureType = programBinary->getSamplerTextureType(type, samplerIndex);
            Texture *texture = getSamplerTexture(textureUnit, textureType);

            if (texture->isSamplerComplete())
            {
                SamplerState samplerState;
                texture->getSamplerState(&samplerState);
                mRenderer->setSamplerState(type, samplerIndex, samplerState);

                mRenderer->setTexture(type, samplerIndex, texture);

                texture->resetDirty();
            }
            else
            {
                mRenderer->setTexture(type, samplerIndex, getIncompleteTexture(textureType));
            }
        }
        else
        {
            mRenderer->setTexture(type, samplerIndex, NULL);
        }
    }

    for (int samplerIndex = samplerRange; samplerIndex < samplerCount; samplerIndex++)
    {
        mRenderer->setTexture(type, samplerIndex, NULL);
    }
}

void Context::readPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                         GLenum format, GLenum type, GLsizei *bufSize, void* pixels)
{
    Framebuffer *framebuffer = getReadFramebuffer();

    if (framebuffer->completeness() != GL_FRAMEBUFFER_COMPLETE)
    {
        return gl::error(GL_INVALID_FRAMEBUFFER_OPERATION);
    }

    if (getReadFramebufferHandle() != 0 && framebuffer->getSamples() != 0)
    {
        return gl::error(GL_INVALID_OPERATION);
    }

    GLsizei outputPitch = ComputePitch(width, ConvertSizedInternalFormat(format, type), getPackAlignment());
    // sized query sanity check
    if (bufSize)
    {
        int requiredSize = outputPitch * height;
        if (requiredSize > *bufSize)
        {
            return gl::error(GL_INVALID_OPERATION);
        }
    }

    mRenderer->readPixels(framebuffer, x, y, width, height, format, type, outputPitch, getPackReverseRowOrder(), getPackAlignment(), pixels);
}

void Context::clear(GLbitfield mask)
{
    Framebuffer *framebufferObject = getDrawFramebuffer();

    if (!framebufferObject || framebufferObject->completeness() != GL_FRAMEBUFFER_COMPLETE)
    {
        return gl::error(GL_INVALID_FRAMEBUFFER_OPERATION);
    }

    DWORD flags = 0;
    GLbitfield finalMask = 0;

    if (mask & GL_COLOR_BUFFER_BIT)
    {
        mask &= ~GL_COLOR_BUFFER_BIT;

        if (framebufferObject->hasEnabledColorAttachment())
        {
            finalMask |= GL_COLOR_BUFFER_BIT;
        }
    }

    if (mask & GL_DEPTH_BUFFER_BIT)
    {
        mask &= ~GL_DEPTH_BUFFER_BIT;
        if (mState.depthStencil.depthMask && framebufferObject->getDepthbufferType() != GL_NONE)
        {
            finalMask |= GL_DEPTH_BUFFER_BIT;
        }
    }

    if (mask & GL_STENCIL_BUFFER_BIT)
    {
        mask &= ~GL_STENCIL_BUFFER_BIT;
        if (framebufferObject->getStencilbufferType() != GL_NONE)
        {
            rx::RenderTarget *depthStencil = framebufferObject->getStencilbuffer()->getDepthStencil();
            if (!depthStencil)
            {
                ERR("Depth stencil pointer unexpectedly null.");
                return;
            }

            if (GetStencilSize(depthStencil->getActualFormat()) > 0)
            {
                finalMask |= GL_STENCIL_BUFFER_BIT;
            }
        }
    }

    if (mask != 0)
    {
        return gl::error(GL_INVALID_VALUE);
    }

    if (!applyRenderTarget(GL_TRIANGLES, true))   // Clips the clear to the scissor rectangle but not the viewport
    {
        return;
    }

    ClearParameters clearParams;
    clearParams.mask = finalMask;
    clearParams.colorClearValue = mState.colorClearValue;
    clearParams.colorMaskRed = mState.blend.colorMaskRed;
    clearParams.colorMaskGreen = mState.blend.colorMaskGreen;
    clearParams.colorMaskBlue = mState.blend.colorMaskBlue;
    clearParams.colorMaskAlpha = mState.blend.colorMaskAlpha;
    clearParams.depthClearValue = mState.depthClearValue;
    clearParams.stencilClearValue = mState.stencilClearValue;
    clearParams.stencilWriteMask = mState.depthStencil.stencilWritemask;

    mRenderer->clear(clearParams, framebufferObject);
}

void Context::drawArrays(GLenum mode, GLint first, GLsizei count, GLsizei instances)
{
    if (!mState.currentProgram)
    {
        return gl::error(GL_INVALID_OPERATION);
    }

    if (!mRenderer->applyPrimitiveType(mode, count))
    {
        return;
    }

    if (!applyRenderTarget(mode, false))
    {
        return;
    }

    applyState(mode);

    ProgramBinary *programBinary = getCurrentProgramBinary();

    GLenum err = mRenderer->applyVertexBuffer(programBinary, mState.vertexAttribute, first, count, instances);
    if (err != GL_NO_ERROR)
    {
        return gl::error(err);
    }

    applyShaders();
    applyTextures();

    if (!programBinary->validateSamplers(NULL))
    {
        return gl::error(GL_INVALID_OPERATION);
    }

    if (!skipDraw(mode))
    {
        mRenderer->drawArrays(mode, count, instances);
    }
}

void Context::drawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, GLsizei instances)
{
    if (!mState.currentProgram)
    {
        return gl::error(GL_INVALID_OPERATION);
    }

    if (!indices && !mState.elementArrayBuffer)
    {
        return gl::error(GL_INVALID_OPERATION);
    }
    
    if (!mRenderer->applyPrimitiveType(mode, count))
    {
        return;
    }

    if (!applyRenderTarget(mode, false))
    {
        return;
    }

    applyState(mode);

    rx::TranslatedIndexData indexInfo;
    GLenum err = mRenderer->applyIndexBuffer(indices, mState.elementArrayBuffer.get(), count, mode, type, &indexInfo);
    if (err != GL_NO_ERROR)
    {
        return gl::error(err);
    }

    ProgramBinary *programBinary = getCurrentProgramBinary();

    GLsizei vertexCount = indexInfo.maxIndex - indexInfo.minIndex + 1;
    err = mRenderer->applyVertexBuffer(programBinary, mState.vertexAttribute, indexInfo.minIndex, vertexCount, instances);
    if (err != GL_NO_ERROR)
    {
        return gl::error(err);
    }

    applyShaders();
    applyTextures();

    if (!programBinary->validateSamplers(NULL))
    {
        return gl::error(GL_INVALID_OPERATION);
    }

    if (!skipDraw(mode))
    {
        mRenderer->drawElements(mode, count, type, indices, mState.elementArrayBuffer.get(), indexInfo, instances);
    }
}

// Implements glFlush when block is false, glFinish when block is true
void Context::sync(bool block)
{
    mRenderer->sync(block);
}

void Context::recordInvalidEnum()
{
    mInvalidEnum = true;
}

void Context::recordInvalidValue()
{
    mInvalidValue = true;
}

void Context::recordInvalidOperation()
{
    mInvalidOperation = true;
}

void Context::recordOutOfMemory()
{
    mOutOfMemory = true;
}

void Context::recordInvalidFramebufferOperation()
{
    mInvalidFramebufferOperation = true;
}

// Get one of the recorded errors and clear its flag, if any.
// [OpenGL ES 2.0.24] section 2.5 page 13.
GLenum Context::getError()
{
    if (mInvalidEnum)
    {
        mInvalidEnum = false;

        return GL_INVALID_ENUM;
    }

    if (mInvalidValue)
    {
        mInvalidValue = false;

        return GL_INVALID_VALUE;
    }

    if (mInvalidOperation)
    {
        mInvalidOperation = false;

        return GL_INVALID_OPERATION;
    }

    if (mOutOfMemory)
    {
        mOutOfMemory = false;

        return GL_OUT_OF_MEMORY;
    }

    if (mInvalidFramebufferOperation)
    {
        mInvalidFramebufferOperation = false;

        return GL_INVALID_FRAMEBUFFER_OPERATION;
    }

    return GL_NO_ERROR;
}

GLenum Context::getResetStatus()
{
    if (mResetStatus == GL_NO_ERROR && !mContextLost)
    {
        // mResetStatus will be set by the markContextLost callback
        // in the case a notification is sent
        mRenderer->testDeviceLost(true);
    }

    GLenum status = mResetStatus;

    if (mResetStatus != GL_NO_ERROR)
    {
        ASSERT(mContextLost);

        if (mRenderer->testDeviceResettable())
        {
            mResetStatus = GL_NO_ERROR;
        }
    }
    
    return status;
}

bool Context::isResetNotificationEnabled()
{
    return (mResetStrategy == GL_LOSE_CONTEXT_ON_RESET_EXT);
}

int Context::getMajorShaderModel() const
{
    return mMajorShaderModel;
}

float Context::getMaximumPointSize() const
{
    return mMaximumPointSize;
}

unsigned int Context::getMaximumCombinedTextureImageUnits() const
{
    return mRenderer->getMaxCombinedTextureImageUnits();
}

int Context::getMaxSupportedSamples() const
{
    return mRenderer->getMaxSupportedSamples();
}

unsigned int Context::getMaximumRenderTargets() const
{
    return mRenderer->getMaxRenderTargets();
}

bool Context::supportsEventQueries() const
{
    return mSupportsEventQueries;
}

bool Context::supportsOcclusionQueries() const
{
    return mSupportsOcclusionQueries;
}

bool Context::supportsBGRATextures() const
{
    return mSupportsBGRATextures;
}

bool Context::supportsDXT1Textures() const
{
    return mSupportsDXT1Textures;
}

bool Context::supportsDXT3Textures() const
{
    return mSupportsDXT3Textures;
}

bool Context::supportsDXT5Textures() const
{
    return mSupportsDXT5Textures;
}

bool Context::supportsFloat32Textures() const
{
    return mSupportsFloat32Textures;
}

bool Context::supportsFloat32LinearFilter() const
{
    return mSupportsFloat32LinearFilter;
}

bool Context::supportsFloat32RenderableTextures() const
{
    return mSupportsFloat32RenderableTextures;
}

bool Context::supportsFloat16Textures() const
{
    return mSupportsFloat16Textures;
}

bool Context::supportsFloat16LinearFilter() const
{
    return mSupportsFloat16LinearFilter;
}

bool Context::supportsFloat16RenderableTextures() const
{
    return mSupportsFloat16RenderableTextures;
}

int Context::getMaximumRenderbufferDimension() const
{
    return mMaxRenderbufferDimension;
}

int Context::getMaximumTextureDimension() const
{
    return mMaxTextureDimension;
}

int Context::getMaximumCubeTextureDimension() const
{
    return mMaxCubeTextureDimension;
}

int Context::getMaximumTextureLevel() const
{
    return mMaxTextureLevel;
}

bool Context::supportsLuminanceTextures() const
{
    return mSupportsLuminanceTextures;
}

bool Context::supportsLuminanceAlphaTextures() const
{
    return mSupportsLuminanceAlphaTextures;
}

bool Context::supportsDepthTextures() const
{
    return mSupportsDepthTextures;
}

bool Context::supports32bitIndices() const
{
    return mSupports32bitIndices;
}

bool Context::supportsNonPower2Texture() const
{
    return mSupportsNonPower2Texture;
}

bool Context::supportsInstancing() const
{
    return mSupportsInstancing;
}

bool Context::supportsTextureFilterAnisotropy() const
{
    return mSupportsTextureFilterAnisotropy;
}

float Context::getTextureMaxAnisotropy() const
{
    return mMaxTextureAnisotropy;
}

bool Context::getCurrentReadFormatType(GLenum *format, GLenum *type)
{
    Framebuffer *framebuffer = getReadFramebuffer();
    if (!framebuffer || framebuffer->completeness() != GL_FRAMEBUFFER_COMPLETE)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    Renderbuffer *renderbuffer = framebuffer->getReadColorbuffer();
    if (!renderbuffer)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    *format = gl::ExtractFormat(renderbuffer->getActualFormat()); 
    *type = gl::ExtractType(renderbuffer->getActualFormat());

    return true;
}

void Context::detachBuffer(GLuint buffer)
{
    // [OpenGL ES 2.0.24] section 2.9 page 22:
    // If a buffer object is deleted while it is bound, all bindings to that object in the current context
    // (i.e. in the thread that called Delete-Buffers) are reset to zero.

    if (mState.arrayBuffer.id() == buffer)
    {
        mState.arrayBuffer.set(NULL);
    }

    if (mState.elementArrayBuffer.id() == buffer)
    {
        mState.elementArrayBuffer.set(NULL);
    }

    for (int attribute = 0; attribute < MAX_VERTEX_ATTRIBS; attribute++)
    {
        if (mState.vertexAttribute[attribute].mBoundBuffer.id() == buffer)
        {
            mState.vertexAttribute[attribute].mBoundBuffer.set(NULL);
        }
    }
}

void Context::detachTexture(GLuint texture)
{
    // [OpenGL ES 2.0.24] section 3.8 page 84:
    // If a texture object is deleted, it is as if all texture units which are bound to that texture object are
    // rebound to texture object zero

    for (int type = 0; type < TEXTURE_TYPE_COUNT; type++)
    {
        for (int sampler = 0; sampler < IMPLEMENTATION_MAX_COMBINED_TEXTURE_IMAGE_UNITS; sampler++)
        {
            if (mState.samplerTexture[type][sampler].id() == texture)
            {
                mState.samplerTexture[type][sampler].set(NULL);
            }
        }
    }

    // [OpenGL ES 2.0.24] section 4.4 page 112:
    // If a texture object is deleted while its image is attached to the currently bound framebuffer, then it is
    // as if FramebufferTexture2D had been called, with a texture of 0, for each attachment point to which this
    // image was attached in the currently bound framebuffer.

    Framebuffer *readFramebuffer = getReadFramebuffer();
    Framebuffer *drawFramebuffer = getDrawFramebuffer();

    if (readFramebuffer)
    {
        readFramebuffer->detachTexture(texture);
    }

    if (drawFramebuffer && drawFramebuffer != readFramebuffer)
    {
        drawFramebuffer->detachTexture(texture);
    }
}

void Context::detachFramebuffer(GLuint framebuffer)
{
    // [OpenGL ES 2.0.24] section 4.4 page 107:
    // If a framebuffer that is currently bound to the target FRAMEBUFFER is deleted, it is as though
    // BindFramebuffer had been executed with the target of FRAMEBUFFER and framebuffer of zero.

    if (mState.readFramebuffer == framebuffer)
    {
        bindReadFramebuffer(0);
    }

    if (mState.drawFramebuffer == framebuffer)
    {
        bindDrawFramebuffer(0);
    }
}

void Context::detachRenderbuffer(GLuint renderbuffer)
{
    // [OpenGL ES 2.0.24] section 4.4 page 109:
    // If a renderbuffer that is currently bound to RENDERBUFFER is deleted, it is as though BindRenderbuffer
    // had been executed with the target RENDERBUFFER and name of zero.

    if (mState.renderbuffer.id() == renderbuffer)
    {
        bindRenderbuffer(0);
    }

    // [OpenGL ES 2.0.24] section 4.4 page 111:
    // If a renderbuffer object is deleted while its image is attached to the currently bound framebuffer,
    // then it is as if FramebufferRenderbuffer had been called, with a renderbuffer of 0, for each attachment
    // point to which this image was attached in the currently bound framebuffer.

    Framebuffer *readFramebuffer = getReadFramebuffer();
    Framebuffer *drawFramebuffer = getDrawFramebuffer();

    if (readFramebuffer)
    {
        readFramebuffer->detachRenderbuffer(renderbuffer);
    }

    if (drawFramebuffer && drawFramebuffer != readFramebuffer)
    {
        drawFramebuffer->detachRenderbuffer(renderbuffer);
    }
}

Texture *Context::getIncompleteTexture(TextureType type)
{
    Texture *t = mIncompleteTextures[type].get();

    if (t == NULL)
    {
        static const GLubyte color[] = { 0, 0, 0, 255 };

        switch (type)
        {
          default:
            UNREACHABLE();
            // default falls through to TEXTURE_2D

          case TEXTURE_2D:
            {
                Texture2D *incomplete2d = new Texture2D(mRenderer, Texture::INCOMPLETE_TEXTURE_ID);
                incomplete2d->setImage(0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, 1, color);
                t = incomplete2d;
            }
            break;

          case TEXTURE_CUBE:
            {
              TextureCubeMap *incompleteCube = new TextureCubeMap(mRenderer, Texture::INCOMPLETE_TEXTURE_ID);

              incompleteCube->setImagePosX(0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, 1, color);
              incompleteCube->setImageNegX(0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, 1, color);
              incompleteCube->setImagePosY(0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, 1, color);
              incompleteCube->setImageNegY(0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, 1, color);
              incompleteCube->setImagePosZ(0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, 1, color);
              incompleteCube->setImageNegZ(0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, 1, color);

              t = incompleteCube;
            }
            break;
        }

        mIncompleteTextures[type].set(t);
    }

    return t;
}

bool Context::skipDraw(GLenum drawMode)
{
    if (drawMode == GL_POINTS)
    {
        // ProgramBinary assumes non-point rendering if gl_PointSize isn't written,
        // which affects varying interpolation. Since the value of gl_PointSize is
        // undefined when not written, just skip drawing to avoid unexpected results.
        if (!getCurrentProgramBinary()->usesPointSize())
        {
            // This is stictly speaking not an error, but developers should be 
            // notified of risking undefined behavior.
            ERR("Point rendering without writing to gl_PointSize.");

            return true;
        }
    }
    else if (IsTriangleMode(drawMode))
    {
        if (mState.rasterizer.cullFace && mState.rasterizer.cullMode == GL_FRONT_AND_BACK)
        {
            return true;
        }
    }

    return false;
}

void Context::setVertexAttrib(GLuint index, const GLfloat *values)
{
    ASSERT(index < gl::MAX_VERTEX_ATTRIBS);

    mState.vertexAttribute[index].mCurrentValue[0] = values[0];
    mState.vertexAttribute[index].mCurrentValue[1] = values[1];
    mState.vertexAttribute[index].mCurrentValue[2] = values[2];
    mState.vertexAttribute[index].mCurrentValue[3] = values[3];
}

void Context::setVertexAttribDivisor(GLuint index, GLuint divisor)
{
    ASSERT(index < gl::MAX_VERTEX_ATTRIBS);

    mState.vertexAttribute[index].mDivisor = divisor;
}

// keep list sorted in following order
// OES extensions
// EXT extensions
// Vendor extensions
void Context::initExtensionString()
{
    std::string extensionString = "";

    // OES extensions
    if (supports32bitIndices())
    {
        extensionString += "GL_OES_element_index_uint ";
    }

    extensionString += "GL_OES_packed_depth_stencil ";
    extensionString += "GL_OES_get_program_binary ";
    extensionString += "GL_OES_rgb8_rgba8 ";
    if (mRenderer->getDerivativeInstructionSupport())
    {
        extensionString += "GL_OES_standard_derivatives ";
    }

    if (supportsFloat16Textures())
    {
        extensionString += "GL_OES_texture_half_float ";
    }
    if (supportsFloat16LinearFilter())
    {
        extensionString += "GL_OES_texture_half_float_linear ";
    }
    if (supportsFloat32Textures())
    {
        extensionString += "GL_OES_texture_float ";
    }
    if (supportsFloat32LinearFilter())
    {
        extensionString += "GL_OES_texture_float_linear ";
    }

    if (supportsNonPower2Texture())
    {
        extensionString += "GL_OES_texture_npot ";
    }

    // Multi-vendor (EXT) extensions
    if (supportsOcclusionQueries())
    {
        extensionString += "GL_EXT_occlusion_query_boolean ";
    }

    extensionString += "GL_EXT_read_format_bgra ";
    extensionString += "GL_EXT_robustness ";

    if (supportsDXT1Textures())
    {
        extensionString += "GL_EXT_texture_compression_dxt1 ";
    }

    if (supportsTextureFilterAnisotropy())
    {
        extensionString += "GL_EXT_texture_filter_anisotropic ";
    }

    if (supportsBGRATextures())
    {
        extensionString += "GL_EXT_texture_format_BGRA8888 ";
    }

    if (mRenderer->getMaxRenderTargets() > 1)
    {
        extensionString += "GL_EXT_draw_buffers ";
    }

    extensionString += "GL_EXT_texture_storage ";
    extensionString += "GL_EXT_frag_depth ";

    // ANGLE-specific extensions
    if (supportsDepthTextures())
    {
        extensionString += "GL_ANGLE_depth_texture ";
    }

    extensionString += "GL_ANGLE_framebuffer_blit ";
    if (getMaxSupportedSamples() != 0)
    {
        extensionString += "GL_ANGLE_framebuffer_multisample ";
    }

    if (supportsInstancing())
    {
        extensionString += "GL_ANGLE_instanced_arrays ";
    }

    extensionString += "GL_ANGLE_pack_reverse_row_order ";

    if (supportsDXT3Textures())
    {
        extensionString += "GL_ANGLE_texture_compression_dxt3 ";
    }
    if (supportsDXT5Textures())
    {
        extensionString += "GL_ANGLE_texture_compression_dxt5 ";
    }

    extensionString += "GL_ANGLE_texture_usage ";
    extensionString += "GL_ANGLE_translated_shader_source ";

    // Other vendor-specific extensions
    if (supportsEventQueries())
    {
        extensionString += "GL_NV_fence ";
    }

    std::string::size_type end = extensionString.find_last_not_of(' ');
    if (end != std::string::npos)
    {
        extensionString.resize(end+1);
    }

    mExtensionString = makeStaticString(extensionString);
}

const char *Context::getExtensionString() const
{
    return mExtensionString;
}

void Context::initRendererString()
{
    std::ostringstream rendererString;
    rendererString << "ANGLE (";
    rendererString << mRenderer->getRendererDescription();
    rendererString << ")";

    mRendererString = makeStaticString(rendererString.str());
}

const char *Context::getRendererString() const
{
    return mRendererString;
}

void Context::blitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, 
                              GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                              GLbitfield mask)
{
    Framebuffer *readFramebuffer = getReadFramebuffer();
    Framebuffer *drawFramebuffer = getDrawFramebuffer();

    if (!readFramebuffer || readFramebuffer->completeness() != GL_FRAMEBUFFER_COMPLETE ||
        !drawFramebuffer || drawFramebuffer->completeness() != GL_FRAMEBUFFER_COMPLETE)
    {
        return gl::error(GL_INVALID_FRAMEBUFFER_OPERATION);
    }

    if (drawFramebuffer->getSamples() != 0)
    {
        return gl::error(GL_INVALID_OPERATION);
    }

    Renderbuffer *readColorBuffer = readFramebuffer->getReadColorbuffer();
    Renderbuffer *drawColorBuffer = drawFramebuffer->getFirstColorbuffer();

    if (drawColorBuffer == NULL)
    {
        ERR("Draw buffers formats don't match, which is not supported in this implementation of BlitFramebufferANGLE");
        return gl::error(GL_INVALID_OPERATION);
    }

    int readBufferWidth = readColorBuffer->getWidth();
    int readBufferHeight = readColorBuffer->getHeight();
    int drawBufferWidth = drawColorBuffer->getWidth();
    int drawBufferHeight = drawColorBuffer->getHeight();

    Rectangle sourceRect;
    Rectangle destRect;

    if (srcX0 < srcX1)
    {
        sourceRect.x = srcX0;
        destRect.x = dstX0;
        sourceRect.width = srcX1 - srcX0;
        destRect.width = dstX1 - dstX0;
    }
    else
    {
        sourceRect.x = srcX1;
        destRect.x = dstX1;
        sourceRect.width = srcX0 - srcX1;
        destRect.width = dstX0 - dstX1;
    }

    if (srcY0 < srcY1)
    {
        sourceRect.height = srcY1 - srcY0;
        destRect.height = dstY1 - dstY0;
        sourceRect.y = srcY0;
        destRect.y = dstY0;
    }
    else
    {
        sourceRect.height = srcY0 - srcY1;
        destRect.height = dstY0 - srcY1;
        sourceRect.y = srcY1;
        destRect.y = dstY1;
    }

    Rectangle sourceScissoredRect = sourceRect;
    Rectangle destScissoredRect = destRect;

    if (mState.scissorTest)
    {
        // Only write to parts of the destination framebuffer which pass the scissor test.
        if (destRect.x < mState.scissor.x)
        {
            int xDiff = mState.scissor.x - destRect.x;
            destScissoredRect.x = mState.scissor.x;
            destScissoredRect.width -= xDiff;
            sourceScissoredRect.x += xDiff;
            sourceScissoredRect.width -= xDiff;

        }

        if (destRect.x + destRect.width > mState.scissor.x + mState.scissor.width)
        {
            int xDiff = (destRect.x + destRect.width) - (mState.scissor.x + mState.scissor.width);
            destScissoredRect.width -= xDiff;
            sourceScissoredRect.width -= xDiff;
        }

        if (destRect.y < mState.scissor.y)
        {
            int yDiff = mState.scissor.y - destRect.y;
            destScissoredRect.y = mState.scissor.y;
            destScissoredRect.height -= yDiff;
            sourceScissoredRect.y += yDiff;
            sourceScissoredRect.height -= yDiff;
        }

        if (destRect.y + destRect.height > mState.scissor.y + mState.scissor.height)
        {
            int yDiff = (destRect.y + destRect.height) - (mState.scissor.y + mState.scissor.height);
            destScissoredRect.height  -= yDiff;
            sourceScissoredRect.height -= yDiff;
        }
    }

    bool blitRenderTarget = false;
    bool blitDepthStencil = false;

    Rectangle sourceTrimmedRect = sourceScissoredRect;
    Rectangle destTrimmedRect = destScissoredRect;

    // The source & destination rectangles also may need to be trimmed if they fall out of the bounds of 
    // the actual draw and read surfaces.
    if (sourceTrimmedRect.x < 0)
    {
        int xDiff = 0 - sourceTrimmedRect.x;
        sourceTrimmedRect.x = 0;
        sourceTrimmedRect.width -= xDiff;
        destTrimmedRect.x += xDiff;
        destTrimmedRect.width -= xDiff;
    }

    if (sourceTrimmedRect.x + sourceTrimmedRect.width > readBufferWidth)
    {
        int xDiff = (sourceTrimmedRect.x + sourceTrimmedRect.width) - readBufferWidth;
        sourceTrimmedRect.width -= xDiff;
        destTrimmedRect.width -= xDiff;
    }

    if (sourceTrimmedRect.y < 0)
    {
        int yDiff = 0 - sourceTrimmedRect.y;
        sourceTrimmedRect.y = 0;
        sourceTrimmedRect.height -= yDiff;
        destTrimmedRect.y += yDiff;
        destTrimmedRect.height -= yDiff;
    }

    if (sourceTrimmedRect.y + sourceTrimmedRect.height > readBufferHeight)
    {
        int yDiff = (sourceTrimmedRect.y + sourceTrimmedRect.height) - readBufferHeight;
        sourceTrimmedRect.height -= yDiff;
        destTrimmedRect.height -= yDiff;
    }

    if (destTrimmedRect.x < 0)
    {
        int xDiff = 0 - destTrimmedRect.x;
        destTrimmedRect.x = 0;
        destTrimmedRect.width -= xDiff;
        sourceTrimmedRect.x += xDiff;
        sourceTrimmedRect.width -= xDiff;
    }

    if (destTrimmedRect.x + destTrimmedRect.width > drawBufferWidth)
    {
        int xDiff = (destTrimmedRect.x + destTrimmedRect.width) - drawBufferWidth;
        destTrimmedRect.width -= xDiff;
        sourceTrimmedRect.width -= xDiff;
    }

    if (destTrimmedRect.y < 0)
    {
        int yDiff = 0 - destTrimmedRect.y;
        destTrimmedRect.y = 0;
        destTrimmedRect.height -= yDiff;
        sourceTrimmedRect.y += yDiff;
        sourceTrimmedRect.height -= yDiff;
    }

    if (destTrimmedRect.y + destTrimmedRect.height > drawBufferHeight)
    {
        int yDiff = (destTrimmedRect.y + destTrimmedRect.height) - drawBufferHeight;
        destTrimmedRect.height -= yDiff;
        sourceTrimmedRect.height -= yDiff;
    }

    bool partialBufferCopy = false;
    if (sourceTrimmedRect.height < readBufferHeight ||
        sourceTrimmedRect.width < readBufferWidth || 
        destTrimmedRect.height < drawBufferHeight ||
        destTrimmedRect.width < drawBufferWidth ||
        sourceTrimmedRect.y != 0 || destTrimmedRect.y != 0 || sourceTrimmedRect.x != 0 || destTrimmedRect.x != 0)
    {
        partialBufferCopy = true;
    }

    if (mask & GL_COLOR_BUFFER_BIT)
    {
        const GLenum readColorbufferType = readFramebuffer->getReadColorbufferType();
        const bool validReadType = (readColorbufferType == GL_TEXTURE_2D) || (readColorbufferType == GL_RENDERBUFFER);
        bool validDrawType = true;
        bool validDrawFormat = true;

        for (unsigned int colorAttachment = 0; colorAttachment < gl::IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
        {
            if (drawFramebuffer->isEnabledColorAttachment(colorAttachment))
            {
                if (drawFramebuffer->getColorbufferType(colorAttachment) != GL_TEXTURE_2D &&
                    drawFramebuffer->getColorbufferType(colorAttachment) != GL_RENDERBUFFER)
                {
                    validDrawType = false;
                }

                if (drawFramebuffer->getColorbuffer(colorAttachment)->getActualFormat() != readColorBuffer->getActualFormat())
                {
                    validDrawFormat = false;
                }
            }
        }

        if (!validReadType || !validDrawType || !validDrawFormat)
        {
            ERR("Color buffer format conversion in BlitFramebufferANGLE not supported by this implementation");
            return gl::error(GL_INVALID_OPERATION);
        }
        
        if (partialBufferCopy && readFramebuffer->getSamples() != 0)
        {
            return gl::error(GL_INVALID_OPERATION);
        }

        blitRenderTarget = true;

    }

    if (mask & (GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT))
    {
        Renderbuffer *readDSBuffer = NULL;
        Renderbuffer *drawDSBuffer = NULL;

        // We support OES_packed_depth_stencil, and do not support a separately attached depth and stencil buffer, so if we have
        // both a depth and stencil buffer, it will be the same buffer.

        if (mask & GL_DEPTH_BUFFER_BIT)
        {
            if (readFramebuffer->getDepthbuffer() && drawFramebuffer->getDepthbuffer())
            {
                if (readFramebuffer->getDepthbufferType() != drawFramebuffer->getDepthbufferType() ||
                    readFramebuffer->getDepthbuffer()->getActualFormat() != drawFramebuffer->getDepthbuffer()->getActualFormat())
                {
                    return gl::error(GL_INVALID_OPERATION);
                }

                blitDepthStencil = true;
                readDSBuffer = readFramebuffer->getDepthbuffer();
                drawDSBuffer = drawFramebuffer->getDepthbuffer();
            }
        }

        if (mask & GL_STENCIL_BUFFER_BIT)
        {
            if (readFramebuffer->getStencilbuffer() && drawFramebuffer->getStencilbuffer())
            {
                if (readFramebuffer->getStencilbufferType() != drawFramebuffer->getStencilbufferType() ||
                    readFramebuffer->getStencilbuffer()->getActualFormat() != drawFramebuffer->getStencilbuffer()->getActualFormat())
                {
                    return gl::error(GL_INVALID_OPERATION);
                }

                blitDepthStencil = true;
                readDSBuffer = readFramebuffer->getStencilbuffer();
                drawDSBuffer = drawFramebuffer->getStencilbuffer();
            }
        }

        if (partialBufferCopy)
        {
            ERR("Only whole-buffer depth and stencil blits are supported by this implementation.");
            return gl::error(GL_INVALID_OPERATION); // only whole-buffer copies are permitted
        }

        if ((drawDSBuffer && drawDSBuffer->getSamples() != 0) || 
            (readDSBuffer && readDSBuffer->getSamples() != 0))
        {
            return gl::error(GL_INVALID_OPERATION);
        }
    }

    if (blitRenderTarget || blitDepthStencil)
    {
        mRenderer->blitRect(readFramebuffer, sourceTrimmedRect, drawFramebuffer, destTrimmedRect, blitRenderTarget, blitDepthStencil);
    }
}

}

extern "C"
{
gl::Context *glCreateContext(const gl::Context *shareContext, rx::Renderer *renderer, bool notifyResets, bool robustAccess)
{
    return new gl::Context(shareContext, renderer, notifyResets, robustAccess);
}

void glDestroyContext(gl::Context *context)
{
    delete context;

    if (context == gl::getContext())
    {
        gl::makeCurrent(NULL, NULL, NULL);
    }
}

void glMakeCurrent(gl::Context *context, egl::Display *display, egl::Surface *surface)
{
    gl::makeCurrent(context, display, surface);
}

gl::Context *glGetCurrentContext()
{
    return gl::getContext();
}

}
