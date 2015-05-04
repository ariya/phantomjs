//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// State.h: Defines the State class, encapsulating raw GL state

#ifndef LIBGLESV2_STATE_H_
#define LIBGLESV2_STATE_H_

#include "common/angleutils.h"
#include "common/RefCountObject.h"
#include "libGLESv2/angletypes.h"
#include "libGLESv2/VertexAttribute.h"
#include "libGLESv2/Renderbuffer.h"
#include "libGLESv2/Texture.h"
#include "libGLESv2/TransformFeedback.h"
#include "libGLESv2/Program.h"
#include "libGLESv2/Sampler.h"

namespace gl
{
class Query;
class VertexArray;
class Context;
struct Caps;
struct Data;

typedef std::map< GLenum, BindingPointer<Texture> > TextureMap;

class State
{
  public:
    State();
    ~State();

    void initialize(const Caps& caps, GLuint clientVersion);
    void reset();

    // State chunk getters
    const RasterizerState &getRasterizerState() const;
    const BlendState &getBlendState() const;
    const DepthStencilState &getDepthStencilState() const;

    // Clear behavior setters & state parameter block generation function
    void setClearColor(float red, float green, float blue, float alpha);
    void setClearDepth(float depth);
    void setClearStencil(int stencil);
    ClearParameters getClearParameters(GLbitfield mask) const;

    // Write mask manipulation
    void setColorMask(bool red, bool green, bool blue, bool alpha);
    void setDepthMask(bool mask);

    // Discard toggle & query
    bool isRasterizerDiscardEnabled() const;
    void setRasterizerDiscard(bool enabled);

    // Face culling state manipulation
    bool isCullFaceEnabled() const;
    void setCullFace(bool enabled);
    void setCullMode(GLenum mode);
    void setFrontFace(GLenum front);

    // Depth test state manipulation
    bool isDepthTestEnabled() const;
    void setDepthTest(bool enabled);
    void setDepthFunc(GLenum depthFunc);
    void setDepthRange(float zNear, float zFar);
    void getDepthRange(float *zNear, float *zFar) const;

    // Blend state manipulation
    bool isBlendEnabled() const;
    void setBlend(bool enabled);
    void setBlendFactors(GLenum sourceRGB, GLenum destRGB, GLenum sourceAlpha, GLenum destAlpha);
    void setBlendColor(float red, float green, float blue, float alpha);
    void setBlendEquation(GLenum rgbEquation, GLenum alphaEquation);
    const ColorF &getBlendColor() const;

    // Stencil state maniupulation
    bool isStencilTestEnabled() const;
    void setStencilTest(bool enabled);
    void setStencilParams(GLenum stencilFunc, GLint stencilRef, GLuint stencilMask);
    void setStencilBackParams(GLenum stencilBackFunc, GLint stencilBackRef, GLuint stencilBackMask);
    void setStencilWritemask(GLuint stencilWritemask);
    void setStencilBackWritemask(GLuint stencilBackWritemask);
    void setStencilOperations(GLenum stencilFail, GLenum stencilPassDepthFail, GLenum stencilPassDepthPass);
    void setStencilBackOperations(GLenum stencilBackFail, GLenum stencilBackPassDepthFail, GLenum stencilBackPassDepthPass);
    GLint getStencilRef() const;
    GLint getStencilBackRef() const;

    // Depth bias/polygon offset state manipulation
    bool isPolygonOffsetFillEnabled() const;
    void setPolygonOffsetFill(bool enabled);
    void setPolygonOffsetParams(GLfloat factor, GLfloat units);

    // Multisample coverage state manipulation
    bool isSampleAlphaToCoverageEnabled() const;
    void setSampleAlphaToCoverage(bool enabled);
    bool isSampleCoverageEnabled() const;
    void setSampleCoverage(bool enabled);
    void setSampleCoverageParams(GLclampf value, bool invert);
    void getSampleCoverageParams(GLclampf *value, bool *invert) const;

    // Scissor test state toggle & query
    bool isScissorTestEnabled() const;
    void setScissorTest(bool enabled);
    void setScissorParams(GLint x, GLint y, GLsizei width, GLsizei height);
    const Rectangle &getScissor() const;

    // Dither state toggle & query
    bool isDitherEnabled() const;
    void setDither(bool enabled);

    // Generic state toggle & query
    void setEnableFeature(GLenum feature, bool enabled);
    bool getEnableFeature(GLenum feature);

    // Line width state setter
    void setLineWidth(GLfloat width);

    // Hint setters
    void setGenerateMipmapHint(GLenum hint);
    void setFragmentShaderDerivativeHint(GLenum hint);

    // Viewport state setter/getter
    void setViewportParams(GLint x, GLint y, GLsizei width, GLsizei height);
    const Rectangle &getViewport() const;

    // Texture binding & active texture unit manipulation
    void setActiveSampler(unsigned int active);
    unsigned int getActiveSampler() const;
    void setSamplerTexture(GLenum type, Texture *texture);
    Texture *getSamplerTexture(unsigned int sampler, GLenum type) const;
    GLuint getSamplerTextureId(unsigned int sampler, GLenum type) const;
    void detachTexture(const TextureMap &zeroTextures, GLuint texture);
    void initializeZeroTextures(const TextureMap &zeroTextures);

    // Sampler object binding manipulation
    void setSamplerBinding(GLuint textureUnit, Sampler *sampler);
    GLuint getSamplerId(GLuint textureUnit) const;
    Sampler *getSampler(GLuint textureUnit) const;
    void detachSampler(GLuint sampler);

    // Renderbuffer binding manipulation
    void setRenderbufferBinding(Renderbuffer *renderbuffer);
    GLuint getRenderbufferId() const;
    Renderbuffer *getCurrentRenderbuffer();
    void detachRenderbuffer(GLuint renderbuffer);

    // Framebuffer binding manipulation
    void setReadFramebufferBinding(Framebuffer *framebuffer);
    void setDrawFramebufferBinding(Framebuffer *framebuffer);
    Framebuffer *getTargetFramebuffer(GLenum target) const;
    Framebuffer *getReadFramebuffer();
    Framebuffer *getDrawFramebuffer();
    const Framebuffer *getReadFramebuffer() const;
    const Framebuffer *getDrawFramebuffer() const;
    bool removeReadFramebufferBinding(GLuint framebuffer);
    bool removeDrawFramebufferBinding(GLuint framebuffer);

    // Vertex array object binding manipulation
    void setVertexArrayBinding(VertexArray *vertexArray);
    GLuint getVertexArrayId() const;
    VertexArray *getVertexArray() const;
    bool removeVertexArrayBinding(GLuint vertexArray);

    // Program binding manipulation
    void setCurrentProgram(GLuint programId, Program *newProgram);
    void setCurrentProgramBinary(ProgramBinary *binary);
    GLuint getCurrentProgramId() const;
    ProgramBinary *getCurrentProgramBinary() const;

    // Transform feedback object (not buffer) binding manipulation
    void setTransformFeedbackBinding(TransformFeedback *transformFeedback);
    TransformFeedback *getCurrentTransformFeedback() const;
    void detachTransformFeedback(GLuint transformFeedback);

    // Query binding manipulation
    bool isQueryActive() const;
    void setActiveQuery(GLenum target, Query *query);
    GLuint getActiveQueryId(GLenum target) const;
    Query *getActiveQuery(GLenum target) const;

    //// Typed buffer binding point manipulation ////
    // GL_ARRAY_BUFFER
    void setArrayBufferBinding(Buffer *buffer);
    GLuint getArrayBufferId() const;
    bool removeArrayBufferBinding(GLuint buffer);

    // GL_UNIFORM_BUFFER - Both indexed and generic targets
    void setGenericUniformBufferBinding(Buffer *buffer);
    void setIndexedUniformBufferBinding(GLuint index, Buffer *buffer, GLintptr offset, GLsizeiptr size);
    GLuint getIndexedUniformBufferId(GLuint index) const;
    Buffer *getIndexedUniformBuffer(GLuint index) const;

    // GL_TRANSFORM_FEEDBACK_BUFFER - Both indexed and generic targets
    void setGenericTransformFeedbackBufferBinding(Buffer *buffer);
    void setIndexedTransformFeedbackBufferBinding(GLuint index, Buffer *buffer, GLintptr offset, GLsizeiptr size);
    GLuint getIndexedTransformFeedbackBufferId(GLuint index) const;
    Buffer *getIndexedTransformFeedbackBuffer(GLuint index) const;
    GLuint getIndexedTransformFeedbackBufferOffset(GLuint index) const;
    size_t getTransformFeedbackBufferIndexRange() const;

    // GL_COPY_[READ/WRITE]_BUFFER
    void setCopyReadBufferBinding(Buffer *buffer);
    void setCopyWriteBufferBinding(Buffer *buffer);

    // GL_PIXEL[PACK/UNPACK]_BUFFER
    void setPixelPackBufferBinding(Buffer *buffer);
    void setPixelUnpackBufferBinding(Buffer *buffer);

    // Retrieve typed buffer by target (non-indexed)
    Buffer *getTargetBuffer(GLenum target) const;

    // Vertex attrib manipulation
    void setEnableVertexAttribArray(unsigned int attribNum, bool enabled);
    void setVertexAttribf(GLuint index, const GLfloat values[4]);
    void setVertexAttribu(GLuint index, const GLuint values[4]);
    void setVertexAttribi(GLuint index, const GLint values[4]);
    void setVertexAttribState(unsigned int attribNum, Buffer *boundBuffer, GLint size, GLenum type,
                              bool normalized, bool pureInteger, GLsizei stride, const void *pointer);
    const VertexAttribute &getVertexAttribState(unsigned int attribNum) const;
    const VertexAttribCurrentValueData &getVertexAttribCurrentValue(unsigned int attribNum) const;
    const void *getVertexAttribPointer(unsigned int attribNum) const;

    // Pixel pack state manipulation
    void setPackAlignment(GLint alignment);
    GLint getPackAlignment() const;
    void setPackReverseRowOrder(bool reverseRowOrder);
    bool getPackReverseRowOrder() const;
    const PixelPackState &getPackState() const;

    // Pixel unpack state manipulation
    void setUnpackAlignment(GLint alignment);
    GLint getUnpackAlignment() const;
    const PixelUnpackState &getUnpackState() const;

    // State query functions
    void getBooleanv(GLenum pname, GLboolean *params);
    void getFloatv(GLenum pname, GLfloat *params);
    void getIntegerv(const gl::Data &data, GLenum pname, GLint *params);
    bool getIndexedIntegerv(GLenum target, GLuint index, GLint *data);
    bool getIndexedInteger64v(GLenum target, GLuint index, GLint64 *data);

    bool hasMappedBuffer(GLenum target) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(State);

    // Cached values from Context's caps
    GLuint mMaxDrawBuffers;
    GLuint mMaxCombinedTextureImageUnits;

    ColorF mColorClearValue;
    GLclampf mDepthClearValue;
    int mStencilClearValue;

    RasterizerState mRasterizer;
    bool mScissorTest;
    Rectangle mScissor;

    BlendState mBlend;
    ColorF mBlendColor;
    bool mSampleCoverage;
    GLclampf mSampleCoverageValue;
    bool mSampleCoverageInvert;

    DepthStencilState mDepthStencil;
    GLint mStencilRef;
    GLint mStencilBackRef;

    GLfloat mLineWidth;

    GLenum mGenerateMipmapHint;
    GLenum mFragmentShaderDerivativeHint;

    Rectangle mViewport;
    float mNearZ;
    float mFarZ;

    BindingPointer<Buffer> mArrayBuffer;
    Framebuffer *mReadFramebuffer;
    Framebuffer *mDrawFramebuffer;
    BindingPointer<Renderbuffer> mRenderbuffer;
    GLuint mCurrentProgramId;
    BindingPointer<ProgramBinary> mCurrentProgramBinary;

    typedef std::vector<VertexAttribCurrentValueData> VertexAttribVector;
    VertexAttribVector mVertexAttribCurrentValues; // From glVertexAttrib
    VertexArray *mVertexArray;

    // Texture and sampler bindings
    size_t mActiveSampler;   // Active texture unit selector - GL_TEXTURE0

    typedef std::vector< BindingPointer<Texture> > TextureBindingVector;
    typedef std::map<GLenum, TextureBindingVector> TextureBindingMap;
    TextureBindingMap mSamplerTextures;

    typedef std::vector< BindingPointer<Sampler> > SamplerBindingVector;
    SamplerBindingVector mSamplers;

    typedef std::map< GLenum, BindingPointer<Query> > ActiveQueryMap;
    ActiveQueryMap mActiveQueries;

    BindingPointer<Buffer> mGenericUniformBuffer;
    typedef std::vector< OffsetBindingPointer<Buffer> > BufferVector;
    BufferVector mUniformBuffers;

    BindingPointer<TransformFeedback> mTransformFeedback;
    BindingPointer<Buffer> mGenericTransformFeedbackBuffer;
    BufferVector mTransformFeedbackBuffers;

    BindingPointer<Buffer> mCopyReadBuffer;
    BindingPointer<Buffer> mCopyWriteBuffer;

    PixelUnpackState mUnpack;
    PixelPackState mPack;
};

}

#endif // LIBGLESV2_STATE_H_

