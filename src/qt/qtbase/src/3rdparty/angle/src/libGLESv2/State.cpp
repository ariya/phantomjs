//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// State.cpp: Implements the State class, encapsulating raw GL state.

#include "libGLESv2/State.h"

#include "libGLESv2/Context.h"
#include "libGLESv2/Caps.h"
#include "libGLESv2/Framebuffer.h"
#include "libGLESv2/FramebufferAttachment.h"
#include "libGLESv2/Query.h"
#include "libGLESv2/VertexArray.h"
#include "libGLESv2/formatutils.h"
#include "libGLESv2/renderer/RenderTarget.h"

namespace gl
{

State::State()
{
    mMaxDrawBuffers = 0;
    mMaxCombinedTextureImageUnits = 0;
}

State::~State()
{
    reset();
}

void State::initialize(const Caps& caps, GLuint clientVersion)
{
    mMaxDrawBuffers = caps.maxDrawBuffers;
    mMaxCombinedTextureImageUnits = caps.maxCombinedTextureImageUnits;

    setClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    mDepthClearValue = 1.0f;
    mStencilClearValue = 0;

    mRasterizer.rasterizerDiscard = false;
    mRasterizer.cullFace = false;
    mRasterizer.cullMode = GL_BACK;
    mRasterizer.frontFace = GL_CCW;
    mRasterizer.polygonOffsetFill = false;
    mRasterizer.polygonOffsetFactor = 0.0f;
    mRasterizer.polygonOffsetUnits = 0.0f;
    mRasterizer.pointDrawMode = false;
    mRasterizer.multiSample = false;
    mScissorTest = false;
    mScissor.x = 0;
    mScissor.y = 0;
    mScissor.width = 0;
    mScissor.height = 0;

    mBlend.blend = false;
    mBlend.sourceBlendRGB = GL_ONE;
    mBlend.sourceBlendAlpha = GL_ONE;
    mBlend.destBlendRGB = GL_ZERO;
    mBlend.destBlendAlpha = GL_ZERO;
    mBlend.blendEquationRGB = GL_FUNC_ADD;
    mBlend.blendEquationAlpha = GL_FUNC_ADD;
    mBlend.sampleAlphaToCoverage = false;
    mBlend.dither = true;

    mBlendColor.red = 0;
    mBlendColor.green = 0;
    mBlendColor.blue = 0;
    mBlendColor.alpha = 0;

    mDepthStencil.depthTest = false;
    mDepthStencil.depthFunc = GL_LESS;
    mDepthStencil.depthMask = true;
    mDepthStencil.stencilTest = false;
    mDepthStencil.stencilFunc = GL_ALWAYS;
    mDepthStencil.stencilMask = -1;
    mDepthStencil.stencilWritemask = -1;
    mDepthStencil.stencilBackFunc = GL_ALWAYS;
    mDepthStencil.stencilBackMask = -1;
    mDepthStencil.stencilBackWritemask = -1;
    mDepthStencil.stencilFail = GL_KEEP;
    mDepthStencil.stencilPassDepthFail = GL_KEEP;
    mDepthStencil.stencilPassDepthPass = GL_KEEP;
    mDepthStencil.stencilBackFail = GL_KEEP;
    mDepthStencil.stencilBackPassDepthFail = GL_KEEP;
    mDepthStencil.stencilBackPassDepthPass = GL_KEEP;

    mStencilRef = 0;
    mStencilBackRef = 0;

    mSampleCoverage = false;
    mSampleCoverageValue = 1.0f;
    mSampleCoverageInvert = false;
    mGenerateMipmapHint = GL_DONT_CARE;
    mFragmentShaderDerivativeHint = GL_DONT_CARE;

    mLineWidth = 1.0f;

    mViewport.x = 0;
    mViewport.y = 0;
    mViewport.width = 0;
    mViewport.height = 0;
    mNearZ = 0.0f;
    mFarZ = 1.0f;

    mBlend.colorMaskRed = true;
    mBlend.colorMaskGreen = true;
    mBlend.colorMaskBlue = true;
    mBlend.colorMaskAlpha = true;

    mActiveSampler = 0;

    const GLfloat defaultFloatValues[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    mVertexAttribCurrentValues.resize(caps.maxVertexAttributes);
    for (size_t attribIndex = 0; attribIndex < mVertexAttribCurrentValues.size(); ++attribIndex)
    {
        mVertexAttribCurrentValues[attribIndex].setFloatValues(defaultFloatValues);
    }

    mUniformBuffers.resize(caps.maxCombinedUniformBlocks);
    mTransformFeedbackBuffers.resize(caps.maxTransformFeedbackSeparateAttributes);

    mSamplerTextures[GL_TEXTURE_2D].resize(caps.maxCombinedTextureImageUnits);
    mSamplerTextures[GL_TEXTURE_CUBE_MAP].resize(caps.maxCombinedTextureImageUnits);
    if (clientVersion >= 3)
    {
        // TODO: These could also be enabled via extension
        mSamplerTextures[GL_TEXTURE_2D_ARRAY].resize(caps.maxCombinedTextureImageUnits);
        mSamplerTextures[GL_TEXTURE_3D].resize(caps.maxCombinedTextureImageUnits);
    }

    mSamplers.resize(caps.maxCombinedTextureImageUnits);

    mActiveQueries[GL_ANY_SAMPLES_PASSED].set(NULL);
    mActiveQueries[GL_ANY_SAMPLES_PASSED_CONSERVATIVE].set(NULL);
    mActiveQueries[GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN].set(NULL);

    mCurrentProgramId = 0;
    mCurrentProgramBinary.set(NULL);

    mReadFramebuffer = NULL;
    mDrawFramebuffer = NULL;
}

void State::reset()
{
    for (TextureBindingMap::iterator bindingVec = mSamplerTextures.begin(); bindingVec != mSamplerTextures.end(); bindingVec++)
    {
        TextureBindingVector &textureVector = bindingVec->second;
        for (size_t textureIdx = 0; textureIdx < textureVector.size(); textureIdx++)
        {
            textureVector[textureIdx].set(NULL);
        }
    }
    for (size_t samplerIdx = 0; samplerIdx < mSamplers.size(); samplerIdx++)
    {
        mSamplers[samplerIdx].set(NULL);
    }

    mArrayBuffer.set(NULL);
    mRenderbuffer.set(NULL);

    mTransformFeedback.set(NULL);

    for (State::ActiveQueryMap::iterator i = mActiveQueries.begin(); i != mActiveQueries.end(); i++)
    {
        i->second.set(NULL);
    }

    mGenericUniformBuffer.set(NULL);
    mGenericTransformFeedbackBuffer.set(NULL);
    for (BufferVector::iterator bufItr = mUniformBuffers.begin(); bufItr != mUniformBuffers.end(); ++bufItr)
    {
        bufItr->set(NULL);
    }

    for (BufferVector::iterator bufItr = mTransformFeedbackBuffers.begin(); bufItr != mTransformFeedbackBuffers.end(); ++bufItr)
    {
        bufItr->set(NULL);
    }

    mCopyReadBuffer.set(NULL);
    mCopyWriteBuffer.set(NULL);

    mPack.pixelBuffer.set(NULL);
    mUnpack.pixelBuffer.set(NULL);
}

const RasterizerState &State::getRasterizerState() const
{
    return mRasterizer;
}

const BlendState &State::getBlendState() const
{
    return mBlend;
}

const DepthStencilState &State::getDepthStencilState() const
{
    return mDepthStencil;
}

void State::setClearColor(float red, float green, float blue, float alpha)
{
    mColorClearValue.red = red;
    mColorClearValue.green = green;
    mColorClearValue.blue = blue;
    mColorClearValue.alpha = alpha;
}

void State::setClearDepth(float depth)
{
    mDepthClearValue = depth;
}

void State::setClearStencil(int stencil)
{
    mStencilClearValue = stencil;
}

ClearParameters State::getClearParameters(GLbitfield mask) const
{
    ClearParameters clearParams = { 0 };
    for (unsigned int i = 0; i < ArraySize(clearParams.clearColor); i++)
    {
        clearParams.clearColor[i] = false;
    }
    clearParams.colorFClearValue = mColorClearValue;
    clearParams.colorClearType = GL_FLOAT;
    clearParams.colorMaskRed = mBlend.colorMaskRed;
    clearParams.colorMaskGreen = mBlend.colorMaskGreen;
    clearParams.colorMaskBlue = mBlend.colorMaskBlue;
    clearParams.colorMaskAlpha = mBlend.colorMaskAlpha;
    clearParams.clearDepth = false;
    clearParams.depthClearValue = mDepthClearValue;
    clearParams.clearStencil = false;
    clearParams.stencilClearValue = mStencilClearValue;
    clearParams.stencilWriteMask = mDepthStencil.stencilWritemask;
    clearParams.scissorEnabled = mScissorTest;
    clearParams.scissor = mScissor;

    const Framebuffer *framebufferObject = getDrawFramebuffer();
    if (mask & GL_COLOR_BUFFER_BIT)
    {
        if (framebufferObject->hasEnabledColorAttachment())
        {
            for (unsigned int i = 0; i < ArraySize(clearParams.clearColor); i++)
            {
                clearParams.clearColor[i] = true;
            }
        }
    }

    if (mask & GL_DEPTH_BUFFER_BIT)
    {
        if (mDepthStencil.depthMask && framebufferObject->getDepthbuffer() != NULL)
        {
            clearParams.clearDepth = true;
        }
    }

    if (mask & GL_STENCIL_BUFFER_BIT)
    {
        if (framebufferObject->getStencilbuffer() != NULL)
        {
            GLenum stencilActualFormat = framebufferObject->getStencilbuffer()->getActualFormat();
            if (GetInternalFormatInfo(stencilActualFormat).stencilBits > 0)
            {
                clearParams.clearStencil = true;
            }
        }
    }

    return clearParams;
}

void State::setColorMask(bool red, bool green, bool blue, bool alpha)
{
    mBlend.colorMaskRed = red;
    mBlend.colorMaskGreen = green;
    mBlend.colorMaskBlue = blue;
    mBlend.colorMaskAlpha = alpha;
}

void State::setDepthMask(bool mask)
{
    mDepthStencil.depthMask = mask;
}

bool State::isRasterizerDiscardEnabled() const
{
    return mRasterizer.rasterizerDiscard;
}

void State::setRasterizerDiscard(bool enabled)
{
    mRasterizer.rasterizerDiscard = enabled;
}

bool State::isCullFaceEnabled() const
{
    return mRasterizer.cullFace;
}

void State::setCullFace(bool enabled)
{
    mRasterizer.cullFace = enabled;
}

void State::setCullMode(GLenum mode)
{
    mRasterizer.cullMode = mode;
}

void State::setFrontFace(GLenum front)
{
    mRasterizer.frontFace = front;
}

bool State::isDepthTestEnabled() const
{
    return mDepthStencil.depthTest;
}

void State::setDepthTest(bool enabled)
{
    mDepthStencil.depthTest = enabled;
}

void State::setDepthFunc(GLenum depthFunc)
{
     mDepthStencil.depthFunc = depthFunc;
}

void State::setDepthRange(float zNear, float zFar)
{
    mNearZ = zNear;
    mFarZ = zFar;
}

void State::getDepthRange(float *zNear, float *zFar) const
{
    *zNear = mNearZ;
    *zFar = mFarZ;
}

bool State::isBlendEnabled() const
{
    return mBlend.blend;
}

void State::setBlend(bool enabled)
{
    mBlend.blend = enabled;
}

void State::setBlendFactors(GLenum sourceRGB, GLenum destRGB, GLenum sourceAlpha, GLenum destAlpha)
{
    mBlend.sourceBlendRGB = sourceRGB;
    mBlend.destBlendRGB = destRGB;
    mBlend.sourceBlendAlpha = sourceAlpha;
    mBlend.destBlendAlpha = destAlpha;
}

void State::setBlendColor(float red, float green, float blue, float alpha)
{
    mBlendColor.red = red;
    mBlendColor.green = green;
    mBlendColor.blue = blue;
    mBlendColor.alpha = alpha;
}

void State::setBlendEquation(GLenum rgbEquation, GLenum alphaEquation)
{
    mBlend.blendEquationRGB = rgbEquation;
    mBlend.blendEquationAlpha = alphaEquation;
}

const ColorF &State::getBlendColor() const
{
    return mBlendColor;
}

bool State::isStencilTestEnabled() const
{
    return mDepthStencil.stencilTest;
}

void State::setStencilTest(bool enabled)
{
    mDepthStencil.stencilTest = enabled;
}

void State::setStencilParams(GLenum stencilFunc, GLint stencilRef, GLuint stencilMask)
{
    mDepthStencil.stencilFunc = stencilFunc;
    mStencilRef = (stencilRef > 0) ? stencilRef : 0;
    mDepthStencil.stencilMask = stencilMask;
}

void State::setStencilBackParams(GLenum stencilBackFunc, GLint stencilBackRef, GLuint stencilBackMask)
{
    mDepthStencil.stencilBackFunc = stencilBackFunc;
    mStencilBackRef = (stencilBackRef > 0) ? stencilBackRef : 0;
    mDepthStencil.stencilBackMask = stencilBackMask;
}

void State::setStencilWritemask(GLuint stencilWritemask)
{
    mDepthStencil.stencilWritemask = stencilWritemask;
}

void State::setStencilBackWritemask(GLuint stencilBackWritemask)
{
    mDepthStencil.stencilBackWritemask = stencilBackWritemask;
}

void State::setStencilOperations(GLenum stencilFail, GLenum stencilPassDepthFail, GLenum stencilPassDepthPass)
{
    mDepthStencil.stencilFail = stencilFail;
    mDepthStencil.stencilPassDepthFail = stencilPassDepthFail;
    mDepthStencil.stencilPassDepthPass = stencilPassDepthPass;
}

void State::setStencilBackOperations(GLenum stencilBackFail, GLenum stencilBackPassDepthFail, GLenum stencilBackPassDepthPass)
{
    mDepthStencil.stencilBackFail = stencilBackFail;
    mDepthStencil.stencilBackPassDepthFail = stencilBackPassDepthFail;
    mDepthStencil.stencilBackPassDepthPass = stencilBackPassDepthPass;
}

GLint State::getStencilRef() const
{
    return mStencilRef;
}

GLint State::getStencilBackRef() const
{
    return mStencilBackRef;
}

bool State::isPolygonOffsetFillEnabled() const
{
    return mRasterizer.polygonOffsetFill;
}

void State::setPolygonOffsetFill(bool enabled)
{
     mRasterizer.polygonOffsetFill = enabled;
}

void State::setPolygonOffsetParams(GLfloat factor, GLfloat units)
{
    // An application can pass NaN values here, so handle this gracefully
    mRasterizer.polygonOffsetFactor = factor != factor ? 0.0f : factor;
    mRasterizer.polygonOffsetUnits = units != units ? 0.0f : units;
}

bool State::isSampleAlphaToCoverageEnabled() const
{
    return mBlend.sampleAlphaToCoverage;
}

void State::setSampleAlphaToCoverage(bool enabled)
{
    mBlend.sampleAlphaToCoverage = enabled;
}

bool State::isSampleCoverageEnabled() const
{
    return mSampleCoverage;
}

void State::setSampleCoverage(bool enabled)
{
    mSampleCoverage = enabled;
}

void State::setSampleCoverageParams(GLclampf value, bool invert)
{
    mSampleCoverageValue = value;
    mSampleCoverageInvert = invert;
}

void State::getSampleCoverageParams(GLclampf *value, bool *invert) const
{
    ASSERT(value != NULL && invert != NULL);

    *value = mSampleCoverageValue;
    *invert = mSampleCoverageInvert;
}

bool State::isScissorTestEnabled() const
{
    return mScissorTest;
}

void State::setScissorTest(bool enabled)
{
    mScissorTest = enabled;
}

void State::setScissorParams(GLint x, GLint y, GLsizei width, GLsizei height)
{
    mScissor.x = x;
    mScissor.y = y;
    mScissor.width = width;
    mScissor.height = height;
}

const Rectangle &State::getScissor() const
{
    return mScissor;
}

bool State::isDitherEnabled() const
{
    return mBlend.dither;
}

void State::setDither(bool enabled)
{
    mBlend.dither = enabled;
}

void State::setEnableFeature(GLenum feature, bool enabled)
{
    switch (feature)
    {
      case GL_CULL_FACE:                     setCullFace(enabled);              break;
      case GL_POLYGON_OFFSET_FILL:           setPolygonOffsetFill(enabled);     break;
      case GL_SAMPLE_ALPHA_TO_COVERAGE:      setSampleAlphaToCoverage(enabled); break;
      case GL_SAMPLE_COVERAGE:               setSampleCoverage(enabled);        break;
      case GL_SCISSOR_TEST:                  setScissorTest(enabled);           break;
      case GL_STENCIL_TEST:                  setStencilTest(enabled);           break;
      case GL_DEPTH_TEST:                    setDepthTest(enabled);             break;
      case GL_BLEND:                         setBlend(enabled);                 break;
      case GL_DITHER:                        setDither(enabled);                break;
      case GL_PRIMITIVE_RESTART_FIXED_INDEX: UNIMPLEMENTED();                   break;
      case GL_RASTERIZER_DISCARD:            setRasterizerDiscard(enabled);     break;
      default:                               UNREACHABLE();
    }
}

bool State::getEnableFeature(GLenum feature)
{
    switch (feature)
    {
      case GL_CULL_FACE:                     return isCullFaceEnabled();
      case GL_POLYGON_OFFSET_FILL:           return isPolygonOffsetFillEnabled();
      case GL_SAMPLE_ALPHA_TO_COVERAGE:      return isSampleAlphaToCoverageEnabled();
      case GL_SAMPLE_COVERAGE:               return isSampleCoverageEnabled();
      case GL_SCISSOR_TEST:                  return isScissorTestEnabled();
      case GL_STENCIL_TEST:                  return isStencilTestEnabled();
      case GL_DEPTH_TEST:                    return isDepthTestEnabled();
      case GL_BLEND:                         return isBlendEnabled();
      case GL_DITHER:                        return isDitherEnabled();
      case GL_PRIMITIVE_RESTART_FIXED_INDEX: UNIMPLEMENTED(); return false;
      case GL_RASTERIZER_DISCARD:            return isRasterizerDiscardEnabled();
      default:                               UNREACHABLE(); return false;
    }
}

void State::setLineWidth(GLfloat width)
{
    mLineWidth = width;
}

void State::setGenerateMipmapHint(GLenum hint)
{
    mGenerateMipmapHint = hint;
}

void State::setFragmentShaderDerivativeHint(GLenum hint)
{
    mFragmentShaderDerivativeHint = hint;
    // TODO: Propagate the hint to shader translator so we can write
    // ddx, ddx_coarse, or ddx_fine depending on the hint.
    // Ignore for now. It is valid for implementations to ignore hint.
}

void State::setViewportParams(GLint x, GLint y, GLsizei width, GLsizei height)
{
    mViewport.x = x;
    mViewport.y = y;
    mViewport.width = width;
    mViewport.height = height;
}

const Rectangle &State::getViewport() const
{
    return mViewport;
}

void State::setActiveSampler(unsigned int active)
{
    mActiveSampler = active;
}

unsigned int State::getActiveSampler() const
{
    return mActiveSampler;
}

void State::setSamplerTexture(GLenum type, Texture *texture)
{
    mSamplerTextures[type][mActiveSampler].set(texture);
}

Texture *State::getSamplerTexture(unsigned int sampler, GLenum type) const
{
    return mSamplerTextures.at(type)[sampler].get();
}

GLuint State::getSamplerTextureId(unsigned int sampler, GLenum type) const
{
    return mSamplerTextures.at(type)[sampler].id();
}

void State::detachTexture(const TextureMap &zeroTextures, GLuint texture)
{
    // Textures have a detach method on State rather than a simple
    // removeBinding, because the zero/null texture objects are managed
    // separately, and don't have to go through the Context's maps or
    // the ResourceManager.

    // [OpenGL ES 2.0.24] section 3.8 page 84:
    // If a texture object is deleted, it is as if all texture units which are bound to that texture object are
    // rebound to texture object zero

    for (TextureBindingMap::iterator bindingVec = mSamplerTextures.begin(); bindingVec != mSamplerTextures.end(); bindingVec++)
    {
        GLenum textureType = bindingVec->first;
        TextureBindingVector &textureVector = bindingVec->second;
        for (size_t textureIdx = 0; textureIdx < textureVector.size(); textureIdx++)
        {
            BindingPointer<Texture> &binding = textureVector[textureIdx];
            if (binding.id() == texture)
            {
                // Zero textures are the "default" textures instead of NULL
                binding.set(zeroTextures.at(textureType).get());
            }
        }
    }

    // [OpenGL ES 2.0.24] section 4.4 page 112:
    // If a texture object is deleted while its image is attached to the currently bound framebuffer, then it is
    // as if Texture2DAttachment had been called, with a texture of 0, for each attachment point to which this
    // image was attached in the currently bound framebuffer.

    if (mReadFramebuffer)
    {
        mReadFramebuffer->detachTexture(texture);
    }

    if (mDrawFramebuffer)
    {
        mDrawFramebuffer->detachTexture(texture);
    }
}

void State::initializeZeroTextures(const TextureMap &zeroTextures)
{
    for (TextureMap::const_iterator i = zeroTextures.begin(); i != zeroTextures.end(); i++)
    {
        TextureBindingVector &samplerTextureArray = mSamplerTextures[i->first];

        for (size_t textureUnit = 0; textureUnit < samplerTextureArray.size(); ++textureUnit)
        {
            samplerTextureArray[textureUnit].set(i->second.get());
        }
    }
}

void State::setSamplerBinding(GLuint textureUnit, Sampler *sampler)
{
    mSamplers[textureUnit].set(sampler);
}

GLuint State::getSamplerId(GLuint textureUnit) const
{
    ASSERT(textureUnit < mSamplers.size());
    return mSamplers[textureUnit].id();
}

Sampler *State::getSampler(GLuint textureUnit) const
{
    return mSamplers[textureUnit].get();
}

void State::detachSampler(GLuint sampler)
{
    // [OpenGL ES 3.0.2] section 3.8.2 pages 123-124:
    // If a sampler object that is currently bound to one or more texture units is
    // deleted, it is as though BindSampler is called once for each texture unit to
    // which the sampler is bound, with unit set to the texture unit and sampler set to zero.
    for (size_t textureUnit = 0; textureUnit < mSamplers.size(); textureUnit++)
    {
        BindingPointer<Sampler> &samplerBinding = mSamplers[textureUnit];
        if (samplerBinding.id() == sampler)
        {
            samplerBinding.set(NULL);
        }
    }
}

void State::setRenderbufferBinding(Renderbuffer *renderbuffer)
{
    mRenderbuffer.set(renderbuffer);
}

GLuint State::getRenderbufferId() const
{
    return mRenderbuffer.id();
}

Renderbuffer *State::getCurrentRenderbuffer()
{
    return mRenderbuffer.get();
}

void State::detachRenderbuffer(GLuint renderbuffer)
{
    // [OpenGL ES 2.0.24] section 4.4 page 109:
    // If a renderbuffer that is currently bound to RENDERBUFFER is deleted, it is as though BindRenderbuffer
    // had been executed with the target RENDERBUFFER and name of zero.

    if (mRenderbuffer.id() == renderbuffer)
    {
        mRenderbuffer.set(NULL);
    }

    // [OpenGL ES 2.0.24] section 4.4 page 111:
    // If a renderbuffer object is deleted while its image is attached to the currently bound framebuffer,
    // then it is as if FramebufferRenderbuffer had been called, with a renderbuffer of 0, for each attachment
    // point to which this image was attached in the currently bound framebuffer.

    Framebuffer *readFramebuffer = mReadFramebuffer;
    Framebuffer *drawFramebuffer = mDrawFramebuffer;

    if (readFramebuffer)
    {
        readFramebuffer->detachRenderbuffer(renderbuffer);
    }

    if (drawFramebuffer && drawFramebuffer != readFramebuffer)
    {
        drawFramebuffer->detachRenderbuffer(renderbuffer);
    }

}

void State::setReadFramebufferBinding(Framebuffer *framebuffer)
{
    mReadFramebuffer = framebuffer;
}

void State::setDrawFramebufferBinding(Framebuffer *framebuffer)
{
    mDrawFramebuffer = framebuffer;
}

Framebuffer *State::getTargetFramebuffer(GLenum target) const
{
    switch (target)
    {
    case GL_READ_FRAMEBUFFER_ANGLE:  return mReadFramebuffer;
    case GL_DRAW_FRAMEBUFFER_ANGLE:
    case GL_FRAMEBUFFER:             return mDrawFramebuffer;
    default:                         UNREACHABLE(); return NULL;
    }
}

Framebuffer *State::getReadFramebuffer()
{
    return mReadFramebuffer;
}

Framebuffer *State::getDrawFramebuffer()
{
    return mDrawFramebuffer;
}

const Framebuffer *State::getReadFramebuffer() const
{
    return mReadFramebuffer;
}

const Framebuffer *State::getDrawFramebuffer() const
{
    return mDrawFramebuffer;
}

bool State::removeReadFramebufferBinding(GLuint framebuffer)
{
    if (mReadFramebuffer->id() == framebuffer)
    {
        mReadFramebuffer = NULL;
        return true;
    }

    return false;
}

bool State::removeDrawFramebufferBinding(GLuint framebuffer)
{
    if (mDrawFramebuffer->id() == framebuffer)
    {
        mDrawFramebuffer = NULL;
        return true;
    }

    return false;
}

void State::setVertexArrayBinding(VertexArray *vertexArray)
{
    mVertexArray = vertexArray;
}

GLuint State::getVertexArrayId() const
{
    ASSERT(mVertexArray != NULL);
    return mVertexArray->id();
}

VertexArray *State::getVertexArray() const
{
    ASSERT(mVertexArray != NULL);
    return mVertexArray;
}

bool State::removeVertexArrayBinding(GLuint vertexArray)
{
    if (mVertexArray->id() == vertexArray)
    {
        mVertexArray = NULL;
        return true;
    }

    return false;
}

void State::setCurrentProgram(GLuint programId, Program *newProgram)
{
    mCurrentProgramId = programId; // set new ID before trying to delete program binary; otherwise it will only be flagged for deletion
    mCurrentProgramBinary.set(NULL);

    if (newProgram)
    {
        newProgram->addRef();
        mCurrentProgramBinary.set(newProgram->getProgramBinary());
    }
}

void State::setCurrentProgramBinary(ProgramBinary *binary)
{
    mCurrentProgramBinary.set(binary);
}

GLuint State::getCurrentProgramId() const
{
    return mCurrentProgramId;
}

ProgramBinary *State::getCurrentProgramBinary() const
{
    return mCurrentProgramBinary.get();
}

void State::setTransformFeedbackBinding(TransformFeedback *transformFeedback)
{
    mTransformFeedback.set(transformFeedback);
}

TransformFeedback *State::getCurrentTransformFeedback() const
{
    return mTransformFeedback.get();
}

void State::detachTransformFeedback(GLuint transformFeedback)
{
    if (mTransformFeedback.id() == transformFeedback)
    {
        mTransformFeedback.set(NULL);
    }
}

bool State::isQueryActive() const
{
    for (State::ActiveQueryMap::const_iterator i = mActiveQueries.begin();
        i != mActiveQueries.end(); i++)
    {
        if (i->second.get() != NULL)
        {
            return true;
        }
    }

    return false;
}

void State::setActiveQuery(GLenum target, Query *query)
{
    mActiveQueries[target].set(query);
}

GLuint State::getActiveQueryId(GLenum target) const
{
    const Query *query = getActiveQuery(target);
    return (query ? query->id() : 0u);
}

Query *State::getActiveQuery(GLenum target) const
{
    // All query types should already exist in the activeQueries map
    ASSERT(mActiveQueries.find(target) != mActiveQueries.end());

    return mActiveQueries.at(target).get();
}

void State::setArrayBufferBinding(Buffer *buffer)
{
    mArrayBuffer.set(buffer);
}

GLuint State::getArrayBufferId() const
{
    return mArrayBuffer.id();
}

bool State::removeArrayBufferBinding(GLuint buffer)
{
    if (mArrayBuffer.id() == buffer)
    {
        mArrayBuffer.set(NULL);
        return true;
    }

    return false;
}

void State::setGenericUniformBufferBinding(Buffer *buffer)
{
    mGenericUniformBuffer.set(buffer);
}

void State::setIndexedUniformBufferBinding(GLuint index, Buffer *buffer, GLintptr offset, GLsizeiptr size)
{
    mUniformBuffers[index].set(buffer, offset, size);
}

GLuint State::getIndexedUniformBufferId(GLuint index) const
{
    ASSERT(static_cast<size_t>(index) < mUniformBuffers.size());

    return mUniformBuffers[index].id();
}

Buffer *State::getIndexedUniformBuffer(GLuint index) const
{
    ASSERT(static_cast<size_t>(index) < mUniformBuffers.size());

    return mUniformBuffers[index].get();
}

void State::setGenericTransformFeedbackBufferBinding(Buffer *buffer)
{
    mGenericTransformFeedbackBuffer.set(buffer);
}

void State::setIndexedTransformFeedbackBufferBinding(GLuint index, Buffer *buffer, GLintptr offset, GLsizeiptr size)
{
    mTransformFeedbackBuffers[index].set(buffer, offset, size);
}

GLuint State::getIndexedTransformFeedbackBufferId(GLuint index) const
{
    ASSERT(static_cast<size_t>(index) < mTransformFeedbackBuffers.size());

    return mTransformFeedbackBuffers[index].id();
}

Buffer *State::getIndexedTransformFeedbackBuffer(GLuint index) const
{
    ASSERT(static_cast<size_t>(index) < mTransformFeedbackBuffers.size());

    return mTransformFeedbackBuffers[index].get();
}

GLuint State::getIndexedTransformFeedbackBufferOffset(GLuint index) const
{
    ASSERT(static_cast<size_t>(index) < mTransformFeedbackBuffers.size());

    return mTransformFeedbackBuffers[index].getOffset();
}

size_t State::getTransformFeedbackBufferIndexRange() const
{
    return mTransformFeedbackBuffers.size();
}

void State::setCopyReadBufferBinding(Buffer *buffer)
{
    mCopyReadBuffer.set(buffer);
}

void State::setCopyWriteBufferBinding(Buffer *buffer)
{
    mCopyWriteBuffer.set(buffer);
}

void State::setPixelPackBufferBinding(Buffer *buffer)
{
    mPack.pixelBuffer.set(buffer);
}

void State::setPixelUnpackBufferBinding(Buffer *buffer)
{
    mUnpack.pixelBuffer.set(buffer);
}

Buffer *State::getTargetBuffer(GLenum target) const
{
    switch (target)
    {
      case GL_ARRAY_BUFFER:              return mArrayBuffer.get();
      case GL_COPY_READ_BUFFER:          return mCopyReadBuffer.get();
      case GL_COPY_WRITE_BUFFER:         return mCopyWriteBuffer.get();
      case GL_ELEMENT_ARRAY_BUFFER:      return getVertexArray()->getElementArrayBuffer();
      case GL_PIXEL_PACK_BUFFER:         return mPack.pixelBuffer.get();
      case GL_PIXEL_UNPACK_BUFFER:       return mUnpack.pixelBuffer.get();
      case GL_TRANSFORM_FEEDBACK_BUFFER: return mGenericTransformFeedbackBuffer.get();
      case GL_UNIFORM_BUFFER:            return mGenericUniformBuffer.get();
      default: UNREACHABLE();            return NULL;
    }
}

void State::setEnableVertexAttribArray(unsigned int attribNum, bool enabled)
{
    getVertexArray()->enableAttribute(attribNum, enabled);
}

void State::setVertexAttribf(GLuint index, const GLfloat values[4])
{
    ASSERT(static_cast<size_t>(index) < mVertexAttribCurrentValues.size());
    mVertexAttribCurrentValues[index].setFloatValues(values);
}

void State::setVertexAttribu(GLuint index, const GLuint values[4])
{
    ASSERT(static_cast<size_t>(index) < mVertexAttribCurrentValues.size());
    mVertexAttribCurrentValues[index].setUnsignedIntValues(values);
}

void State::setVertexAttribi(GLuint index, const GLint values[4])
{
    ASSERT(static_cast<size_t>(index) < mVertexAttribCurrentValues.size());
    mVertexAttribCurrentValues[index].setIntValues(values);
}

void State::setVertexAttribState(unsigned int attribNum, Buffer *boundBuffer, GLint size, GLenum type, bool normalized,
    bool pureInteger, GLsizei stride, const void *pointer)
{
    getVertexArray()->setAttributeState(attribNum, boundBuffer, size, type, normalized, pureInteger, stride, pointer);
}

const VertexAttribute &State::getVertexAttribState(unsigned int attribNum) const
{
    return getVertexArray()->getVertexAttribute(attribNum);
}

const VertexAttribCurrentValueData &State::getVertexAttribCurrentValue(unsigned int attribNum) const
{
    ASSERT(static_cast<size_t>(attribNum) < mVertexAttribCurrentValues.size());
    return mVertexAttribCurrentValues[attribNum];
}

const void *State::getVertexAttribPointer(unsigned int attribNum) const
{
    return getVertexArray()->getVertexAttribute(attribNum).pointer;
}

void State::setPackAlignment(GLint alignment)
{
    mPack.alignment = alignment;
}

GLint State::getPackAlignment() const
{
    return mPack.alignment;
}

void State::setPackReverseRowOrder(bool reverseRowOrder)
{
    mPack.reverseRowOrder = reverseRowOrder;
}

bool State::getPackReverseRowOrder() const
{
    return mPack.reverseRowOrder;
}

const PixelPackState &State::getPackState() const
{
    return mPack;
}

void State::setUnpackAlignment(GLint alignment)
{
    mUnpack.alignment = alignment;
}

GLint State::getUnpackAlignment() const
{
    return mUnpack.alignment;
}

const PixelUnpackState &State::getUnpackState() const
{
    return mUnpack;
}

void State::getBooleanv(GLenum pname, GLboolean *params)
{
    switch (pname)
    {
      case GL_SAMPLE_COVERAGE_INVERT:    *params = mSampleCoverageInvert;         break;
      case GL_DEPTH_WRITEMASK:           *params = mDepthStencil.depthMask;       break;
      case GL_COLOR_WRITEMASK:
        params[0] = mBlend.colorMaskRed;
        params[1] = mBlend.colorMaskGreen;
        params[2] = mBlend.colorMaskBlue;
        params[3] = mBlend.colorMaskAlpha;
        break;
      case GL_CULL_FACE:                 *params = mRasterizer.cullFace;          break;
      case GL_POLYGON_OFFSET_FILL:       *params = mRasterizer.polygonOffsetFill; break;
      case GL_SAMPLE_ALPHA_TO_COVERAGE:  *params = mBlend.sampleAlphaToCoverage;  break;
      case GL_SAMPLE_COVERAGE:           *params = mSampleCoverage;               break;
      case GL_SCISSOR_TEST:              *params = mScissorTest;                  break;
      case GL_STENCIL_TEST:              *params = mDepthStencil.stencilTest;     break;
      case GL_DEPTH_TEST:                *params = mDepthStencil.depthTest;       break;
      case GL_BLEND:                     *params = mBlend.blend;                  break;
      case GL_DITHER:                    *params = mBlend.dither;                 break;
      case GL_TRANSFORM_FEEDBACK_ACTIVE: *params = getCurrentTransformFeedback()->isStarted(); break;
      case GL_TRANSFORM_FEEDBACK_PAUSED: *params = getCurrentTransformFeedback()->isPaused();  break;
      default:
        UNREACHABLE();
        break;
    }
}

void State::getFloatv(GLenum pname, GLfloat *params)
{
    // Please note: DEPTH_CLEAR_VALUE is included in our internal getFloatv implementation
    // because it is stored as a float, despite the fact that the GL ES 2.0 spec names
    // GetIntegerv as its native query function. As it would require conversion in any
    // case, this should make no difference to the calling application.
    switch (pname)
    {
      case GL_LINE_WIDTH:               *params = mLineWidth;                         break;
      case GL_SAMPLE_COVERAGE_VALUE:    *params = mSampleCoverageValue;               break;
      case GL_DEPTH_CLEAR_VALUE:        *params = mDepthClearValue;                   break;
      case GL_POLYGON_OFFSET_FACTOR:    *params = mRasterizer.polygonOffsetFactor;    break;
      case GL_POLYGON_OFFSET_UNITS:     *params = mRasterizer.polygonOffsetUnits;     break;
      case GL_DEPTH_RANGE:
        params[0] = mNearZ;
        params[1] = mFarZ;
        break;
      case GL_COLOR_CLEAR_VALUE:
        params[0] = mColorClearValue.red;
        params[1] = mColorClearValue.green;
        params[2] = mColorClearValue.blue;
        params[3] = mColorClearValue.alpha;
        break;
      case GL_BLEND_COLOR:
        params[0] = mBlendColor.red;
        params[1] = mBlendColor.green;
        params[2] = mBlendColor.blue;
        params[3] = mBlendColor.alpha;
        break;
      default:
        UNREACHABLE();
        break;
    }
}

void State::getIntegerv(const gl::Data &data, GLenum pname, GLint *params)
{
    if (pname >= GL_DRAW_BUFFER0_EXT && pname <= GL_DRAW_BUFFER15_EXT)
    {
        unsigned int colorAttachment = (pname - GL_DRAW_BUFFER0_EXT);
        ASSERT(colorAttachment < mMaxDrawBuffers);
        Framebuffer *framebuffer = mDrawFramebuffer;
        *params = framebuffer->getDrawBufferState(colorAttachment);
        return;
    }

    // Please note: DEPTH_CLEAR_VALUE is not included in our internal getIntegerv implementation
    // because it is stored as a float, despite the fact that the GL ES 2.0 spec names
    // GetIntegerv as its native query function. As it would require conversion in any
    // case, this should make no difference to the calling application. You may find it in
    // State::getFloatv.
    switch (pname)
    {
      case GL_ARRAY_BUFFER_BINDING:                     *params = mArrayBuffer.id();                              break;
      case GL_ELEMENT_ARRAY_BUFFER_BINDING:             *params = getVertexArray()->getElementArrayBufferId();    break;
        //case GL_FRAMEBUFFER_BINDING:                    // now equivalent to GL_DRAW_FRAMEBUFFER_BINDING_ANGLE
      case GL_DRAW_FRAMEBUFFER_BINDING_ANGLE:           *params = mDrawFramebuffer->id();                         break;
      case GL_READ_FRAMEBUFFER_BINDING_ANGLE:           *params = mReadFramebuffer->id();                         break;
      case GL_RENDERBUFFER_BINDING:                     *params = mRenderbuffer.id();                             break;
      case GL_VERTEX_ARRAY_BINDING:                     *params = mVertexArray->id();                             break;
      case GL_CURRENT_PROGRAM:                          *params = mCurrentProgramId;                              break;
      case GL_PACK_ALIGNMENT:                           *params = mPack.alignment;                                break;
      case GL_PACK_REVERSE_ROW_ORDER_ANGLE:             *params = mPack.reverseRowOrder;                          break;
      case GL_UNPACK_ALIGNMENT:                         *params = mUnpack.alignment;                              break;
      case GL_GENERATE_MIPMAP_HINT:                     *params = mGenerateMipmapHint;                            break;
      case GL_FRAGMENT_SHADER_DERIVATIVE_HINT_OES:      *params = mFragmentShaderDerivativeHint;                  break;
      case GL_ACTIVE_TEXTURE:                           *params = (mActiveSampler + GL_TEXTURE0);                 break;
      case GL_STENCIL_FUNC:                             *params = mDepthStencil.stencilFunc;                      break;
      case GL_STENCIL_REF:                              *params = mStencilRef;                                    break;
      case GL_STENCIL_VALUE_MASK:                       *params = clampToInt(mDepthStencil.stencilMask);          break;
      case GL_STENCIL_BACK_FUNC:                        *params = mDepthStencil.stencilBackFunc;                  break;
      case GL_STENCIL_BACK_REF:                         *params = mStencilBackRef;                                break;
      case GL_STENCIL_BACK_VALUE_MASK:                  *params = clampToInt(mDepthStencil.stencilBackMask);      break;
      case GL_STENCIL_FAIL:                             *params = mDepthStencil.stencilFail;                      break;
      case GL_STENCIL_PASS_DEPTH_FAIL:                  *params = mDepthStencil.stencilPassDepthFail;             break;
      case GL_STENCIL_PASS_DEPTH_PASS:                  *params = mDepthStencil.stencilPassDepthPass;             break;
      case GL_STENCIL_BACK_FAIL:                        *params = mDepthStencil.stencilBackFail;                  break;
      case GL_STENCIL_BACK_PASS_DEPTH_FAIL:             *params = mDepthStencil.stencilBackPassDepthFail;         break;
      case GL_STENCIL_BACK_PASS_DEPTH_PASS:             *params = mDepthStencil.stencilBackPassDepthPass;         break;
      case GL_DEPTH_FUNC:                               *params = mDepthStencil.depthFunc;                        break;
      case GL_BLEND_SRC_RGB:                            *params = mBlend.sourceBlendRGB;                          break;
      case GL_BLEND_SRC_ALPHA:                          *params = mBlend.sourceBlendAlpha;                        break;
      case GL_BLEND_DST_RGB:                            *params = mBlend.destBlendRGB;                            break;
      case GL_BLEND_DST_ALPHA:                          *params = mBlend.destBlendAlpha;                          break;
      case GL_BLEND_EQUATION_RGB:                       *params = mBlend.blendEquationRGB;                        break;
      case GL_BLEND_EQUATION_ALPHA:                     *params = mBlend.blendEquationAlpha;                      break;
      case GL_STENCIL_WRITEMASK:                        *params = clampToInt(mDepthStencil.stencilWritemask);     break;
      case GL_STENCIL_BACK_WRITEMASK:                   *params = clampToInt(mDepthStencil.stencilBackWritemask); break;
      case GL_STENCIL_CLEAR_VALUE:                      *params = mStencilClearValue;                             break;
      case GL_SAMPLE_BUFFERS:
      case GL_SAMPLES:
        {
            gl::Framebuffer *framebuffer = mDrawFramebuffer;
            if (framebuffer->completeness(data) == GL_FRAMEBUFFER_COMPLETE)
            {
                switch (pname)
                {
                  case GL_SAMPLE_BUFFERS:
                    if (framebuffer->getSamples(data) != 0)
                    {
                        *params = 1;
                    }
                    else
                    {
                        *params = 0;
                    }
                    break;
                  case GL_SAMPLES:
                    *params = framebuffer->getSamples(data);
                    break;
                }
            }
            else
            {
                *params = 0;
            }
        }
        break;
      case GL_VIEWPORT:
        params[0] = mViewport.x;
        params[1] = mViewport.y;
        params[2] = mViewport.width;
        params[3] = mViewport.height;
        break;
      case GL_SCISSOR_BOX:
        params[0] = mScissor.x;
        params[1] = mScissor.y;
        params[2] = mScissor.width;
        params[3] = mScissor.height;
        break;
      case GL_CULL_FACE_MODE:                   *params = mRasterizer.cullMode;   break;
      case GL_FRONT_FACE:                       *params = mRasterizer.frontFace;  break;
      case GL_RED_BITS:
      case GL_GREEN_BITS:
      case GL_BLUE_BITS:
      case GL_ALPHA_BITS:
        {
            gl::Framebuffer *framebuffer = getDrawFramebuffer();
            gl::FramebufferAttachment *colorbuffer = framebuffer->getFirstColorbuffer();

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
            gl::FramebufferAttachment *depthbuffer = framebuffer->getDepthbuffer();

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
            gl::FramebufferAttachment *stencilbuffer = framebuffer->getStencilbuffer();

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
        ASSERT(mActiveSampler < mMaxCombinedTextureImageUnits);
        *params = mSamplerTextures.at(GL_TEXTURE_2D)[mActiveSampler].id();
        break;
      case GL_TEXTURE_BINDING_CUBE_MAP:
        ASSERT(mActiveSampler < mMaxCombinedTextureImageUnits);
        *params = mSamplerTextures.at(GL_TEXTURE_CUBE_MAP)[mActiveSampler].id();
        break;
      case GL_TEXTURE_BINDING_3D:
        ASSERT(mActiveSampler < mMaxCombinedTextureImageUnits);
        *params = mSamplerTextures.at(GL_TEXTURE_3D)[mActiveSampler].id();
        break;
      case GL_TEXTURE_BINDING_2D_ARRAY:
        ASSERT(mActiveSampler < mMaxCombinedTextureImageUnits);
        *params = mSamplerTextures.at(GL_TEXTURE_2D_ARRAY)[mActiveSampler].id();
        break;
      case GL_UNIFORM_BUFFER_BINDING:
        *params = mGenericUniformBuffer.id();
        break;
      case GL_TRANSFORM_FEEDBACK_BUFFER_BINDING:
        *params = mGenericTransformFeedbackBuffer.id();
        break;
      case GL_COPY_READ_BUFFER_BINDING:
        *params = mCopyReadBuffer.id();
        break;
      case GL_COPY_WRITE_BUFFER_BINDING:
        *params = mCopyWriteBuffer.id();
        break;
      case GL_PIXEL_PACK_BUFFER_BINDING:
        *params = mPack.pixelBuffer.id();
        break;
      case GL_PIXEL_UNPACK_BUFFER_BINDING:
        *params = mUnpack.pixelBuffer.id();
        break;
      default:
        UNREACHABLE();
        break;
    }
}

bool State::getIndexedIntegerv(GLenum target, GLuint index, GLint *data)
{
    switch (target)
    {
      case GL_TRANSFORM_FEEDBACK_BUFFER_BINDING:
        if (static_cast<size_t>(index) < mTransformFeedbackBuffers.size())
        {
            *data = mTransformFeedbackBuffers[index].id();
        }
        break;
      case GL_UNIFORM_BUFFER_BINDING:
        if (static_cast<size_t>(index) < mUniformBuffers.size())
        {
            *data = mUniformBuffers[index].id();
        }
        break;
      default:
        return false;
    }

    return true;
}

bool State::getIndexedInteger64v(GLenum target, GLuint index, GLint64 *data)
{
    switch (target)
    {
      case GL_TRANSFORM_FEEDBACK_BUFFER_START:
        if (static_cast<size_t>(index) < mTransformFeedbackBuffers.size())
        {
            *data = mTransformFeedbackBuffers[index].getOffset();
        }
        break;
      case GL_TRANSFORM_FEEDBACK_BUFFER_SIZE:
        if (static_cast<size_t>(index) < mTransformFeedbackBuffers.size())
        {
            *data = mTransformFeedbackBuffers[index].getSize();
        }
        break;
      case GL_UNIFORM_BUFFER_START:
        if (static_cast<size_t>(index) < mUniformBuffers.size())
        {
            *data = mUniformBuffers[index].getOffset();
        }
        break;
      case GL_UNIFORM_BUFFER_SIZE:
        if (static_cast<size_t>(index) < mUniformBuffers.size())
        {
            *data = mUniformBuffers[index].getSize();
        }
        break;
      default:
        return false;
    }

    return true;
}

bool State::hasMappedBuffer(GLenum target) const
{
    if (target == GL_ARRAY_BUFFER)
    {
        for (size_t attribIndex = 0; attribIndex < mVertexAttribCurrentValues.size(); attribIndex++)
        {
            const gl::VertexAttribute &vertexAttrib = getVertexAttribState(static_cast<unsigned int>(attribIndex));
            gl::Buffer *boundBuffer = vertexAttrib.buffer.get();
            if (vertexAttrib.enabled && boundBuffer && boundBuffer->isMapped())
            {
                return true;
            }
        }

        return false;
    }
    else
    {
        Buffer *buffer = getTargetBuffer(target);
        return (buffer && buffer->isMapped());
    }
}

}
