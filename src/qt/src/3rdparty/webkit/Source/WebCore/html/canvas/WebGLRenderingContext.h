/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef WebGLRenderingContext_h
#define WebGLRenderingContext_h

#include "CanvasRenderingContext.h"
#include "ExceptionCode.h"
#include "Float32Array.h"
#include "GraphicsContext3D.h"
#include "Int32Array.h"
#include "PlatformString.h"
#include "Timer.h"
#include "Uint8Array.h"
#include "WebGLGetInfo.h"

#include <wtf/OwnArrayPtr.h>

namespace WebCore {

class WebGLActiveInfo;
class WebGLBuffer;
class WebGLContextAttributes;
class WebGLExtension;
class WebGLFramebuffer;
class WebGLObject;
class WebGLProgram;
class WebGLRenderbuffer;
class WebGLShader;
class WebGLTexture;
class WebGLUniformLocation;
class WebKitLoseContext;
class HTMLImageElement;
class HTMLVideoElement;
class ImageBuffer;
class ImageData;
class IntSize;
class OESStandardDerivatives;
class OESTextureFloat;
class OESVertexArrayObject;
class WebGLVertexArrayObjectOES;

class WebGLRenderingContext : public CanvasRenderingContext {
public:
    static PassOwnPtr<WebGLRenderingContext> create(HTMLCanvasElement*, WebGLContextAttributes*);
    virtual ~WebGLRenderingContext();

    virtual bool is3d() const { return true; }
    virtual bool isAccelerated() const { return true; }
    virtual bool paintsIntoCanvasBuffer() const;

    void activeTexture(GC3Denum texture, ExceptionCode&);
    void attachShader(WebGLProgram*, WebGLShader*, ExceptionCode&);
    void bindAttribLocation(WebGLProgram*, GC3Duint index, const String& name, ExceptionCode&);
    void bindBuffer(GC3Denum target, WebGLBuffer*, ExceptionCode&);
    void bindFramebuffer(GC3Denum target, WebGLFramebuffer*, ExceptionCode&);
    void bindRenderbuffer(GC3Denum target, WebGLRenderbuffer*, ExceptionCode&);
    void bindTexture(GC3Denum target, WebGLTexture*, ExceptionCode&);
    void blendColor(GC3Dfloat red, GC3Dfloat green, GC3Dfloat blue, GC3Dfloat alpha);
    void blendEquation(GC3Denum mode);
    void blendEquationSeparate(GC3Denum modeRGB, GC3Denum modeAlpha);
    void blendFunc(GC3Denum sfactor, GC3Denum dfactor);
    void blendFuncSeparate(GC3Denum srcRGB, GC3Denum dstRGB, GC3Denum srcAlpha, GC3Denum dstAlpha);

    void bufferData(GC3Denum target, GC3Dsizeiptr size, GC3Denum usage, ExceptionCode&);
    void bufferData(GC3Denum target, ArrayBuffer* data, GC3Denum usage, ExceptionCode&);
    void bufferData(GC3Denum target, ArrayBufferView* data, GC3Denum usage, ExceptionCode&);
    void bufferSubData(GC3Denum target, GC3Dintptr offset, ArrayBuffer* data, ExceptionCode&);
    void bufferSubData(GC3Denum target, GC3Dintptr offset, ArrayBufferView* data, ExceptionCode&);

    GC3Denum checkFramebufferStatus(GC3Denum target);
    void clear(GC3Dbitfield mask);
    void clearColor(GC3Dfloat red, GC3Dfloat green, GC3Dfloat blue, GC3Dfloat alpha);
    void clearDepth(GC3Dfloat);
    void clearStencil(GC3Dint);
    void colorMask(GC3Dboolean red, GC3Dboolean green, GC3Dboolean blue, GC3Dboolean alpha);
    void compileShader(WebGLShader*, ExceptionCode&);

    // void compressedTexImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height, GC3Dint border, GC3Dsizei imageSize, const void* data);
    // void compressedTexSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset, GC3Dsizei width, GC3Dsizei GC3Dsizei height, GC3Denum format, GC3Dsizei imageSize, const void* data);

    void copyTexImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height, GC3Dint border);
    void copyTexSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset, GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height);

    PassRefPtr<WebGLBuffer> createBuffer();
    PassRefPtr<WebGLFramebuffer> createFramebuffer();
    PassRefPtr<WebGLProgram> createProgram();
    PassRefPtr<WebGLRenderbuffer> createRenderbuffer();
    PassRefPtr<WebGLShader> createShader(GC3Denum type, ExceptionCode&);
    PassRefPtr<WebGLTexture> createTexture();

    void cullFace(GC3Denum mode);

    void deleteBuffer(WebGLBuffer*);
    void deleteFramebuffer(WebGLFramebuffer*);
    void deleteProgram(WebGLProgram*);
    void deleteRenderbuffer(WebGLRenderbuffer*);
    void deleteShader(WebGLShader*);
    void deleteTexture(WebGLTexture*);

    void depthFunc(GC3Denum);
    void depthMask(GC3Dboolean);
    void depthRange(GC3Dfloat zNear, GC3Dfloat zFar);
    void detachShader(WebGLProgram*, WebGLShader*, ExceptionCode&);
    void disable(GC3Denum cap);
    void disableVertexAttribArray(GC3Duint index, ExceptionCode&);
    void drawArrays(GC3Denum mode, GC3Dint first, GC3Dsizei count, ExceptionCode&);
    void drawElements(GC3Denum mode, GC3Dsizei count, GC3Denum type, GC3Dintptr offset, ExceptionCode&);

    void enable(GC3Denum cap);
    void enableVertexAttribArray(GC3Duint index, ExceptionCode&);
    void finish();
    void flush();
    void framebufferRenderbuffer(GC3Denum target, GC3Denum attachment, GC3Denum renderbuffertarget, WebGLRenderbuffer*, ExceptionCode&);
    void framebufferTexture2D(GC3Denum target, GC3Denum attachment, GC3Denum textarget, WebGLTexture*, GC3Dint level, ExceptionCode&);
    void frontFace(GC3Denum mode);
    void generateMipmap(GC3Denum target);

    PassRefPtr<WebGLActiveInfo> getActiveAttrib(WebGLProgram*, GC3Duint index, ExceptionCode&);
    PassRefPtr<WebGLActiveInfo> getActiveUniform(WebGLProgram*, GC3Duint index, ExceptionCode&);
    bool getAttachedShaders(WebGLProgram*, Vector<WebGLShader*>&, ExceptionCode&);
    GC3Dint getAttribLocation(WebGLProgram*, const String& name);
    WebGLGetInfo getBufferParameter(GC3Denum target, GC3Denum pname, ExceptionCode&);
    PassRefPtr<WebGLContextAttributes> getContextAttributes();
    GC3Denum getError();
    WebGLExtension* getExtension(const String& name);
    WebGLGetInfo getFramebufferAttachmentParameter(GC3Denum target, GC3Denum attachment, GC3Denum pname, ExceptionCode&);
    WebGLGetInfo getParameter(GC3Denum pname, ExceptionCode&);
    WebGLGetInfo getProgramParameter(WebGLProgram*, GC3Denum pname, ExceptionCode&);
    String getProgramInfoLog(WebGLProgram*, ExceptionCode&);
    WebGLGetInfo getRenderbufferParameter(GC3Denum target, GC3Denum pname, ExceptionCode&);
    WebGLGetInfo getShaderParameter(WebGLShader*, GC3Denum pname, ExceptionCode&);
    String getShaderInfoLog(WebGLShader*, ExceptionCode&);

    // TBD
    // void glGetShaderPrecisionFormat (GC3Denum shadertype, GC3Denum precisiontype, GC3Dint* range, GC3Dint* precision);

    String getShaderSource(WebGLShader*, ExceptionCode&);
    Vector<String> getSupportedExtensions();
    WebGLGetInfo getTexParameter(GC3Denum target, GC3Denum pname, ExceptionCode&);
    WebGLGetInfo getUniform(WebGLProgram*, const WebGLUniformLocation*, ExceptionCode&);
    PassRefPtr<WebGLUniformLocation> getUniformLocation(WebGLProgram*, const String&, ExceptionCode&);
    WebGLGetInfo getVertexAttrib(GC3Duint index, GC3Denum pname, ExceptionCode&);
    GC3Dsizeiptr getVertexAttribOffset(GC3Duint index, GC3Denum pname);

    void hint(GC3Denum target, GC3Denum mode);
    GC3Dboolean isBuffer(WebGLBuffer*);
    bool isContextLost();
    GC3Dboolean isEnabled(GC3Denum cap);
    GC3Dboolean isFramebuffer(WebGLFramebuffer*);
    GC3Dboolean isProgram(WebGLProgram*);
    GC3Dboolean isRenderbuffer(WebGLRenderbuffer*);
    GC3Dboolean isShader(WebGLShader*);
    GC3Dboolean isTexture(WebGLTexture*);

    void lineWidth(GC3Dfloat);
    void linkProgram(WebGLProgram*, ExceptionCode&);
    void pixelStorei(GC3Denum pname, GC3Dint param);
    void polygonOffset(GC3Dfloat factor, GC3Dfloat units);
    void readPixels(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Denum type, ArrayBufferView* pixels, ExceptionCode&);
    void releaseShaderCompiler();
    void renderbufferStorage(GC3Denum target, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height);
    void sampleCoverage(GC3Dfloat value, GC3Dboolean invert);
    void scissor(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height);
    void shaderSource(WebGLShader*, const String&, ExceptionCode&);
    void stencilFunc(GC3Denum func, GC3Dint ref, GC3Duint mask);
    void stencilFuncSeparate(GC3Denum face, GC3Denum func, GC3Dint ref, GC3Duint mask);
    void stencilMask(GC3Duint);
    void stencilMaskSeparate(GC3Denum face, GC3Duint mask);
    void stencilOp(GC3Denum fail, GC3Denum zfail, GC3Denum zpass);
    void stencilOpSeparate(GC3Denum face, GC3Denum fail, GC3Denum zfail, GC3Denum zpass);

    void texImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat,
                    GC3Dsizei width, GC3Dsizei height, GC3Dint border,
                    GC3Denum format, GC3Denum type, ArrayBufferView*, ExceptionCode&);
    void texImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat,
                    GC3Denum format, GC3Denum type, ImageData*, ExceptionCode&);
    void texImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat,
                    GC3Denum format, GC3Denum type, HTMLImageElement*, ExceptionCode&);
    void texImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat,
                    GC3Denum format, GC3Denum type, HTMLCanvasElement*, ExceptionCode&);
#if ENABLE(VIDEO)
    void texImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat,
                    GC3Denum format, GC3Denum type, HTMLVideoElement*, ExceptionCode&);
#endif

    void texParameterf(GC3Denum target, GC3Denum pname, GC3Dfloat param);
    void texParameteri(GC3Denum target, GC3Denum pname, GC3Dint param);

    void texSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset,
                       GC3Dsizei width, GC3Dsizei height,
                       GC3Denum format, GC3Denum type, ArrayBufferView*, ExceptionCode&);
    void texSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset,
                       GC3Denum format, GC3Denum type, ImageData*, ExceptionCode&);
    void texSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset,
                       GC3Denum format, GC3Denum type, HTMLImageElement*, ExceptionCode&);
    void texSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset,
                       GC3Denum format, GC3Denum type, HTMLCanvasElement*, ExceptionCode&);
#if ENABLE(VIDEO)
    void texSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset,
                       GC3Denum format, GC3Denum type, HTMLVideoElement*, ExceptionCode&);
#endif

    void uniform1f(const WebGLUniformLocation* location, GC3Dfloat x, ExceptionCode&);
    void uniform1fv(const WebGLUniformLocation* location, Float32Array* v, ExceptionCode&);
    void uniform1fv(const WebGLUniformLocation* location, GC3Dfloat* v, GC3Dsizei size, ExceptionCode&);
    void uniform1i(const WebGLUniformLocation* location, GC3Dint x, ExceptionCode&);
    void uniform1iv(const WebGLUniformLocation* location, Int32Array* v, ExceptionCode&);
    void uniform1iv(const WebGLUniformLocation* location, GC3Dint* v, GC3Dsizei size, ExceptionCode&);
    void uniform2f(const WebGLUniformLocation* location, GC3Dfloat x, GC3Dfloat y, ExceptionCode&);
    void uniform2fv(const WebGLUniformLocation* location, Float32Array* v, ExceptionCode&);
    void uniform2fv(const WebGLUniformLocation* location, GC3Dfloat* v, GC3Dsizei size, ExceptionCode&);
    void uniform2i(const WebGLUniformLocation* location, GC3Dint x, GC3Dint y, ExceptionCode&);
    void uniform2iv(const WebGLUniformLocation* location, Int32Array* v, ExceptionCode&);
    void uniform2iv(const WebGLUniformLocation* location, GC3Dint* v, GC3Dsizei size, ExceptionCode&);
    void uniform3f(const WebGLUniformLocation* location, GC3Dfloat x, GC3Dfloat y, GC3Dfloat z, ExceptionCode&);
    void uniform3fv(const WebGLUniformLocation* location, Float32Array* v, ExceptionCode&);
    void uniform3fv(const WebGLUniformLocation* location, GC3Dfloat* v, GC3Dsizei size, ExceptionCode&);
    void uniform3i(const WebGLUniformLocation* location, GC3Dint x, GC3Dint y, GC3Dint z, ExceptionCode&);
    void uniform3iv(const WebGLUniformLocation* location, Int32Array* v, ExceptionCode&);
    void uniform3iv(const WebGLUniformLocation* location, GC3Dint* v, GC3Dsizei size, ExceptionCode&);
    void uniform4f(const WebGLUniformLocation* location, GC3Dfloat x, GC3Dfloat y, GC3Dfloat z, GC3Dfloat w, ExceptionCode&);
    void uniform4fv(const WebGLUniformLocation* location, Float32Array* v, ExceptionCode&);
    void uniform4fv(const WebGLUniformLocation* location, GC3Dfloat* v, GC3Dsizei size, ExceptionCode&);
    void uniform4i(const WebGLUniformLocation* location, GC3Dint x, GC3Dint y, GC3Dint z, GC3Dint w, ExceptionCode&);
    void uniform4iv(const WebGLUniformLocation* location, Int32Array* v, ExceptionCode&);
    void uniform4iv(const WebGLUniformLocation* location, GC3Dint* v, GC3Dsizei size, ExceptionCode&);
    void uniformMatrix2fv(const WebGLUniformLocation* location, GC3Dboolean transpose, Float32Array* value, ExceptionCode&);
    void uniformMatrix2fv(const WebGLUniformLocation* location, GC3Dboolean transpose, GC3Dfloat* value, GC3Dsizei size, ExceptionCode&);
    void uniformMatrix3fv(const WebGLUniformLocation* location, GC3Dboolean transpose, Float32Array* value, ExceptionCode&);
    void uniformMatrix3fv(const WebGLUniformLocation* location, GC3Dboolean transpose, GC3Dfloat* value, GC3Dsizei size, ExceptionCode&);
    void uniformMatrix4fv(const WebGLUniformLocation* location, GC3Dboolean transpose, Float32Array* value, ExceptionCode&);
    void uniformMatrix4fv(const WebGLUniformLocation* location, GC3Dboolean transpose, GC3Dfloat* value, GC3Dsizei size, ExceptionCode&);

    void useProgram(WebGLProgram*, ExceptionCode&);
    void validateProgram(WebGLProgram*, ExceptionCode&);

    void vertexAttrib1f(GC3Duint index, GC3Dfloat x);
    void vertexAttrib1fv(GC3Duint index, Float32Array* values);
    void vertexAttrib1fv(GC3Duint index, GC3Dfloat* values, GC3Dsizei size);
    void vertexAttrib2f(GC3Duint index, GC3Dfloat x, GC3Dfloat y);
    void vertexAttrib2fv(GC3Duint index, Float32Array* values);
    void vertexAttrib2fv(GC3Duint index, GC3Dfloat* values, GC3Dsizei size);
    void vertexAttrib3f(GC3Duint index, GC3Dfloat x, GC3Dfloat y, GC3Dfloat z);
    void vertexAttrib3fv(GC3Duint index, Float32Array* values);
    void vertexAttrib3fv(GC3Duint index, GC3Dfloat* values, GC3Dsizei size);
    void vertexAttrib4f(GC3Duint index, GC3Dfloat x, GC3Dfloat y, GC3Dfloat z, GC3Dfloat w);
    void vertexAttrib4fv(GC3Duint index, Float32Array* values);
    void vertexAttrib4fv(GC3Duint index, GC3Dfloat* values, GC3Dsizei size);
    void vertexAttribPointer(GC3Duint index, GC3Dint size, GC3Denum type, GC3Dboolean normalized,
                             GC3Dsizei stride, GC3Dintptr offset, ExceptionCode&);

    void viewport(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height);

    void forceLostContext();
    void onLostContext();
    void restoreContext();

    GraphicsContext3D* graphicsContext3D() const { return m_context.get(); }
#if USE(ACCELERATED_COMPOSITING)
    virtual PlatformLayer* platformLayer() const { return m_context->platformLayer(); }
#endif

    void reshape(int width, int height);

    void markLayerComposited();
    virtual void paintRenderingResultsToCanvas();
    virtual PassRefPtr<ImageData> paintRenderingResultsToImageData();

    void removeObject(WebGLObject*);
    
    unsigned getMaxVertexAttribs() const { return m_maxVertexAttribs; }

  private:
    friend class WebGLObject;
    friend class OESVertexArrayObject;

    WebGLRenderingContext(HTMLCanvasElement*, PassRefPtr<GraphicsContext3D>, GraphicsContext3D::Attributes);
    void initializeNewContext();
    void setupFlags();

    void addObject(WebGLObject*);
    void detachAndRemoveAllObjects();
    WebGLTexture* findTexture(Platform3DObject);
    WebGLRenderbuffer* findRenderbuffer(Platform3DObject);
    WebGLBuffer* findBuffer(Platform3DObject);
    WebGLShader* findShader(Platform3DObject);

    void markContextChanged();
    void cleanupAfterGraphicsCall(bool changed)
    {
        if (changed)
            markContextChanged();
    }

    // Query whether it is built on top of compliant GLES2 implementation.
    bool isGLES2Compliant() { return m_isGLES2Compliant; }
    // Query if the GL implementation is NPOT strict.
    bool isGLES2NPOTStrict() { return m_isGLES2NPOTStrict; }
    // Query if the GL implementation generates errors on out-of-bounds buffer accesses.
    bool isErrorGeneratedOnOutOfBoundsAccesses() { return m_isErrorGeneratedOnOutOfBoundsAccesses; }
    // Query if the GL implementation initializes textures/renderbuffers to 0.
    bool isResourceSafe() { return m_isResourceSafe; }
    // Query if depth_stencil buffer is supported.
    bool isDepthStencilSupported() { return m_isDepthStencilSupported; }

    // Helper to return the size in bytes of OpenGL data types
    // like GL_FLOAT, GL_INT, etc.
    unsigned int sizeInBytes(GC3Denum type);

    // Basic validation of count and offset against number of elements in element array buffer
    bool validateElementArraySize(GC3Dsizei count, GC3Denum type, GC3Dintptr offset);

    // Conservative but quick index validation
    bool validateIndexArrayConservative(GC3Denum type, int& numElementsRequired);

    // Precise but slow index validation -- only done if conservative checks fail
    bool validateIndexArrayPrecise(GC3Dsizei count, GC3Denum type, GC3Dintptr offset, int& numElementsRequired);
    // If numElements <= 0, we only check if each enabled vertex attribute is bound to a buffer.
    bool validateRenderingState(int numElements);

    bool validateWebGLObject(WebGLObject*);

#if ENABLE(VIDEO)
    PassRefPtr<Image> videoFrameToImage(HTMLVideoElement*);
#endif

    RefPtr<GraphicsContext3D> m_context;

    class WebGLRenderingContextRestoreTimer : public TimerBase {
    public:
        WebGLRenderingContextRestoreTimer(WebGLRenderingContext* context) : m_context(context) { }
    private:
        virtual void fired();
        WebGLRenderingContext* m_context;
    };

    WebGLRenderingContextRestoreTimer m_restoreTimer;

    bool m_needsUpdate;
    bool m_markedCanvasDirty;
    HashSet<RefPtr<WebGLObject> > m_canvasObjects;

    // List of bound VBO's. Used to maintain info about sizes for ARRAY_BUFFER and stored values for ELEMENT_ARRAY_BUFFER
    RefPtr<WebGLBuffer> m_boundArrayBuffer;
    
    RefPtr<WebGLVertexArrayObjectOES> m_defaultVertexArrayObject;
    RefPtr<WebGLVertexArrayObjectOES> m_boundVertexArrayObject;
    void setBoundVertexArrayObject(PassRefPtr<WebGLVertexArrayObjectOES> arrayObject)
    {
        if (arrayObject)
            m_boundVertexArrayObject = arrayObject;
        else
            m_boundVertexArrayObject = m_defaultVertexArrayObject;
    }
    
    class VertexAttribValue {
    public:
        VertexAttribValue()
        {
            initValue();
        }
        
        void initValue()
        {
            value[0] = 0.0f;
            value[1] = 0.0f;
            value[2] = 0.0f;
            value[3] = 1.0f;
        }
        
        GC3Dfloat value[4];
    };
    Vector<VertexAttribValue> m_vertexAttribValue;
    unsigned m_maxVertexAttribs;
    RefPtr<WebGLBuffer> m_vertexAttrib0Buffer;
    long m_vertexAttrib0BufferSize;
    GC3Dfloat m_vertexAttrib0BufferValue[4];
    bool m_forceAttrib0BufferRefill;
    bool m_vertexAttrib0UsedBefore;

    RefPtr<WebGLProgram> m_currentProgram;
    RefPtr<WebGLFramebuffer> m_framebufferBinding;
    RefPtr<WebGLRenderbuffer> m_renderbufferBinding;
    class TextureUnitState {
    public:
        RefPtr<WebGLTexture> m_texture2DBinding;
        RefPtr<WebGLTexture> m_textureCubeMapBinding;
    };
    Vector<TextureUnitState> m_textureUnits;
    unsigned long m_activeTextureUnit;

    RefPtr<WebGLTexture> m_blackTexture2D;
    RefPtr<WebGLTexture> m_blackTextureCubeMap;

    // Fixed-size cache of reusable image buffers for video texImage2D calls.
    class LRUImageBufferCache {
    public:
        LRUImageBufferCache(int capacity);
        // The pointer returned is owned by the image buffer map.
        ImageBuffer* imageBuffer(const IntSize& size);
    private:
        void bubbleToFront(int idx);
        OwnArrayPtr<OwnPtr<ImageBuffer> > m_buffers;
        int m_capacity;
    };
    LRUImageBufferCache m_videoCache;

    GC3Dint m_maxTextureSize;
    GC3Dint m_maxCubeMapTextureSize;
    GC3Dint m_maxTextureLevel;
    GC3Dint m_maxCubeMapTextureLevel;

    GC3Dint m_packAlignment;
    GC3Dint m_unpackAlignment;
    bool m_unpackFlipY;
    bool m_unpackPremultiplyAlpha;
    GC3Denum m_unpackColorspaceConversion;
    bool m_contextLost;
    GraphicsContext3D::Attributes m_attributes;

    bool m_layerCleared;
    GC3Dfloat m_clearColor[4];
    bool m_scissorEnabled;
    GC3Dfloat m_clearDepth;
    GC3Dint m_clearStencil;
    GC3Dboolean m_colorMask[4];
    GC3Dboolean m_depthMask;

    long m_stencilBits;
    GC3Duint m_stencilMask, m_stencilMaskBack;
    GC3Dint m_stencilFuncRef, m_stencilFuncRefBack; // Note that these are the user specified values, not the internal clamped value.
    GC3Duint m_stencilFuncMask, m_stencilFuncMaskBack;

    bool m_isGLES2Compliant;
    bool m_isGLES2NPOTStrict;
    bool m_isErrorGeneratedOnOutOfBoundsAccesses;
    bool m_isResourceSafe;
    bool m_isDepthStencilSupported;

    // Enabled extension objects.
    OwnPtr<OESTextureFloat> m_oesTextureFloat;
    OwnPtr<OESStandardDerivatives> m_oesStandardDerivatives;
    OwnPtr<OESVertexArrayObject> m_oesVertexArrayObject;
    OwnPtr<WebKitLoseContext> m_webkitLoseContext;

    // Helpers for getParameter and others
    WebGLGetInfo getBooleanParameter(GC3Denum);
    WebGLGetInfo getBooleanArrayParameter(GC3Denum);
    WebGLGetInfo getFloatParameter(GC3Denum);
    WebGLGetInfo getIntParameter(GC3Denum);
    WebGLGetInfo getUnsignedIntParameter(GC3Denum);
    WebGLGetInfo getWebGLFloatArrayParameter(GC3Denum);
    WebGLGetInfo getWebGLIntArrayParameter(GC3Denum);

    // Clear the backbuffer if it was composited since the last operation.
    // clearMask is set to the bitfield of any clear that would happen anyway at this time
    // and the function returns true if that clear is now unnecessary.
    bool clearIfComposited(GC3Dbitfield clearMask = 0);

    void texImage2DBase(GC3Denum target, GC3Dint level, GC3Denum internalformat,
                        GC3Dsizei width, GC3Dsizei height, GC3Dint border,
                        GC3Denum format, GC3Denum type, void* pixels, ExceptionCode&);
    void texImage2DImpl(GC3Denum target, GC3Dint level, GC3Denum internalformat,
                        GC3Denum format, GC3Denum type, Image*,
                        bool flipY, bool premultiplyAlpha, ExceptionCode&);
    void texSubImage2DBase(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset,
                           GC3Dsizei width, GC3Dsizei height,
                           GC3Denum format, GC3Denum type, void* pixels, ExceptionCode&);
    void texSubImage2DImpl(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset,
                           GC3Denum format, GC3Denum type,
                           Image* image, bool flipY, bool premultiplyAlpha, ExceptionCode&);

    void handleNPOTTextures(bool prepareToDraw);

    void createFallbackBlackTextures1x1();

    // Helper function for copyTex{Sub}Image, check whether the internalformat
    // and the color buffer format of the current bound framebuffer combination
    // is valid.
    bool isTexInternalFormatColorBufferCombinationValid(GC3Denum texInternalFormat,
                                                        GC3Denum colorBufferFormat);

    // Helper function to get the bound framebuffer's color buffer format.
    GC3Denum getBoundFramebufferColorFormat();

    // Helper function to get the bound framebuffer's width.
    int getBoundFramebufferWidth();

    // Helper function to get the bound framebuffer's height.
    int getBoundFramebufferHeight();

    // Helper function to check if size is non-negative.
    // Generate GL error and return false for negative inputs; otherwise, return true.
    bool validateSize(GC3Dint x, GC3Dint y);

    // Helper function to check if all characters in the string belong to the
    // ASCII subset as defined in GLSL ES 1.0 spec section 3.1.
    bool validateString(const String&);

    // Helper function to check target and texture bound to the target.
    // Generate GL errors and return 0 if target is invalid or texture bound is
    // null.  Otherwise, return the texture bound to the target.
    WebGLTexture* validateTextureBinding(GC3Denum target, bool useSixEnumsForCubeMap);

    // Helper function to check input format/type for functions {copy}Tex{Sub}Image.
    // Generates GL error and returns false if parameters are invalid.
    bool validateTexFuncFormatAndType(GC3Denum format, GC3Denum type);

    // Helper function to check input level for functions {copy}Tex{Sub}Image.
    // Generates GL error and returns false if level is invalid.
    bool validateTexFuncLevel(GC3Denum target, GC3Dint level);

    // Helper function to check input parameters for functions {copy}Tex{Sub}Image.
    // Generates GL error and returns false if parameters are invalid.
    bool validateTexFuncParameters(GC3Denum target, GC3Dint level,
                                   GC3Denum internalformat,
                                   GC3Dsizei width, GC3Dsizei height, GC3Dint border,
                                   GC3Denum format, GC3Denum type);

    // Helper function to validate that the given ArrayBufferView
    // is of the correct type and contains enough data for the texImage call.
    // Generates GL error and returns false if parameters are invalid.
    bool validateTexFuncData(GC3Dsizei width, GC3Dsizei height,
                             GC3Denum format, GC3Denum type,
                             ArrayBufferView* pixels);

    // Helper function to validate mode for draw{Arrays/Elements}.
    bool validateDrawMode(GC3Denum);

    // Helper function to validate if front/back stencilMask and stencilFunc settings are the same.
    bool validateStencilSettings();

    // Helper function to validate stencil func.
    bool validateStencilFunc(GC3Denum);

    // Helper function for texParameterf and texParameteri.
    void texParameter(GC3Denum target, GC3Denum pname, GC3Dfloat parami, GC3Dint paramf, bool isFloat);

    // Helper function to print warnings to console. Currently
    // used only to warn about use of obsolete functions.
    void printWarningToConsole(const String&);

    // Helper function to validate input parameters for framebuffer functions.
    // Generate GL error if parameters are illegal.
    bool validateFramebufferFuncParameters(GC3Denum target, GC3Denum attachment);

    // Helper function to validate blend equation mode.
    bool validateBlendEquation(GC3Denum);

    // Helper function to validate blend func factors.
    bool validateBlendFuncFactors(GC3Denum src, GC3Denum dst);

    // Helper function to validate a GL capability.
    bool validateCapability(GC3Denum);

    // Helper function to validate input parameters for uniform functions.
    bool validateUniformParameters(const WebGLUniformLocation*, Float32Array*, GC3Dsizei mod);
    bool validateUniformParameters(const WebGLUniformLocation*, Int32Array*, GC3Dsizei mod);
    bool validateUniformParameters(const WebGLUniformLocation*, void*, GC3Dsizei size, GC3Dsizei mod);
    bool validateUniformMatrixParameters(const WebGLUniformLocation*, GC3Dboolean transpose, Float32Array*, GC3Dsizei mod);
    bool validateUniformMatrixParameters(const WebGLUniformLocation*, GC3Dboolean transpose, void*, GC3Dsizei size, GC3Dsizei mod);

    // Helper function to validate parameters for bufferData.
    // Return the current bound buffer to target, or 0 if parameters are invalid.
    WebGLBuffer* validateBufferDataParameters(GC3Denum target, GC3Denum usage);

    // Helper function for tex{Sub}Image2D to make sure image is ready.
    bool validateHTMLImageElement(HTMLImageElement*);

    // Helper functions for vertexAttribNf{v}.
    void vertexAttribfImpl(GC3Duint index, GC3Dsizei expectedSize, GC3Dfloat, GC3Dfloat, GC3Dfloat, GC3Dfloat);
    void vertexAttribfvImpl(GC3Duint index, Float32Array*, GC3Dsizei expectedSize);
    void vertexAttribfvImpl(GC3Duint index, GC3Dfloat*, GC3Dsizei size, GC3Dsizei expectedSize);

    // Helper function for delete* (deleteBuffer, deleteProgram, etc) functions.
    // Return false if caller should return without further processing.
    bool deleteObject(WebGLObject*);

    // Helper function for bind* (bindBuffer, bindTexture, etc) and useProgram.
    // If the object has already been deleted, set deleted to true upon return.
    // Return false if caller should return without further processing.
    bool checkObjectToBeBound(WebGLObject*, bool& deleted);

    // Helpers for simulating vertexAttrib0
    void initVertexAttrib0();
    bool simulateVertexAttrib0(GC3Dsizei numVertex);
    void restoreStatesAfterVertexAttrib0Simulation();

    friend class WebGLStateRestorer;
};

} // namespace WebCore

#endif
