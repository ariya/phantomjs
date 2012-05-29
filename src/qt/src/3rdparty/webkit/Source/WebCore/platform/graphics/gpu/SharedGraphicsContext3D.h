/*
 * Copyright (c) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SharedGraphicsContext3D_h
#define SharedGraphicsContext3D_h

#include "GraphicsContext3D.h"
#include "GraphicsTypes.h"
#include "ImageSource.h"
#include "Texture.h"

#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/OwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

#if USE(SKIA)
class GrContext;
#endif

namespace WebCore {

class AffineTransform;
class BicubicShader;
class Color;
class ConvolutionShader;
class DrawingBuffer;
class FloatRect;
class HostWindow;
class IntSize;
class LoopBlinnSolidFillShader;
class SolidFillShader;
class TexShader;

typedef HashMap<NativeImagePtr, RefPtr<Texture> > TextureHashMap;

class SharedGraphicsContext3D : public RefCounted<SharedGraphicsContext3D> {
public:

    enum CreationFlagBits {
        UseSkiaGPU = 1
    };
    typedef unsigned CreationFlags;

    static PassRefPtr<SharedGraphicsContext3D> create(HostWindow*, CreationFlags);
    ~SharedGraphicsContext3D();

    // Functions that delegate directly to GraphicsContext3D, with caching
    void makeContextCurrent();
    void bindBuffer(GC3Denum target, Platform3DObject);
    void bindFramebuffer(Platform3DObject framebuffer);
    void bufferData(GC3Denum target, GC3Dsizeiptr, GC3Denum usage);
    void bufferData(GC3Denum target, GC3Dsizeiptr, const void* data, GC3Denum usage);
    void bufferSubData(GC3Denum target, GC3Dintptr offset, GC3Dsizeiptr, const void* data);
    void setViewport(const IntSize&);
    void scissor(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height);
    void enable(GC3Denum capacity);
    void disable(GC3Denum capacity);
    void clearColor(const Color&);
    void clear(GC3Dbitfield mask);
    void drawArrays(GC3Denum mode, GC3Dint first, GC3Dsizei count);
    GC3Denum getError();
    void getIntegerv(GC3Denum pname, GC3Dint* value);
    void flush();

    Platform3DObject createBuffer();
    Platform3DObject createFramebuffer();
    Platform3DObject createTexture();

    void deleteFramebuffer(Platform3DObject framebuffer);
    void deleteTexture(Platform3DObject texture);

    void framebufferTexture2D(GC3Denum target, GC3Denum attachment, GC3Denum textarget, Platform3DObject, GC3Dint level);
    void texParameteri(GC3Denum target, GC3Denum pname, GC3Dint param);
    bool texImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height, GC3Dint border, GC3Denum format, GC3Denum type, const void* pixels);
    void texSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Denum type, const void* pixels);

    void readPixels(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Denum type, void* data);

    bool paintsIntoCanvasBuffer() const;

    // Shared logic for canvas 2d
    void applyCompositeOperator(CompositeOperator);
    void enableStencil(bool enable);
    void useQuadVertices();

    void useFillSolidProgram(const AffineTransform&, const Color&);
    void useTextureProgram(const AffineTransform&, const AffineTransform&, float alpha);
    void useBicubicProgram(const AffineTransform&, const AffineTransform&, const float coefficients[16], const float imageIncrement[2], float alpha);
    void useConvolutionProgram(const AffineTransform&, const AffineTransform& texTransform, const float* kernel, int kernelWidth, float imageIncrement[2]);

    void setActiveTexture(GC3Denum textureUnit);
    void bindTexture(GC3Denum target, Platform3DObject texture);

    bool supportsBGRA();
    bool supportsCompositeOp(CompositeOperator) const;

    // Creates a texture associated with the given image.  Is owned by this context's
    // TextureHashMap.
    Texture* createTexture(NativeImagePtr, Texture::Format, int width, int height);
    Texture* getTexture(NativeImagePtr);

    // Multiple SharedGraphicsContext3D can exist in a single process (one per compositing context) and the same
    // NativeImagePtr may be uploaded as a texture into all of them.  This function removes uploaded textures
    // for a given NativeImagePtr in all contexts.
    static void removeTexturesFor(NativeImagePtr);

    // Creates a texture that is not associated with any image.  The caller takes ownership of
    // the texture.
    PassRefPtr<Texture> createTexture(Texture::Format, int width, int height);

    GraphicsContext3D* graphicsContext3D() const { return m_context.get(); }

    // Members for GPU-accelerated path rendering.
    static bool useLoopBlinnForPathRendering();
    void useLoopBlinnInteriorProgram(unsigned vertexOffset, const AffineTransform&, const Color&);
    void useLoopBlinnExteriorProgram(unsigned vertexOffset, unsigned klmOffset, const AffineTransform&, const Color&);
    DrawingBuffer* getOffscreenBuffer(unsigned index, const IntSize&);

#if USE(SKIA)
    GrContext* grContext();
#endif

private:
    SharedGraphicsContext3D(PassRefPtr<GraphicsContext3D>, PassOwnPtr<SolidFillShader>, PassOwnPtr<TexShader>, PassOwnPtr<BicubicShader>, PassOwnArrayPtr<OwnPtr<ConvolutionShader> >, CreationFlags);

    // Used to implement removeTexturesFor(), see the comment above.
    static HashSet<SharedGraphicsContext3D*>* allContexts();
    void removeTextureFor(NativeImagePtr);

    RefPtr<GraphicsContext3D> m_context;
    bool m_bgraSupported;

    unsigned m_quadVertices;

    OwnPtr<SolidFillShader> m_solidFillShader;
    OwnPtr<TexShader> m_texShader;
    OwnPtr<BicubicShader> m_bicubicShader;
    OwnArrayPtr<OwnPtr<ConvolutionShader> > m_convolutionShaders;

    TextureHashMap m_textures;

    // Members for GPU-accelerated path rendering.
    // FIXME: support more kinds of fill types for paths.
    OwnPtr<LoopBlinnSolidFillShader> m_loopBlinnInteriorShader;
    OwnPtr<LoopBlinnSolidFillShader> m_loopBlinnExteriorShader;
    bool m_oesStandardDerivativesSupported;

    WTF::Vector<RefPtr<DrawingBuffer> > m_offscreenBuffers;

    unsigned m_flags;
#if USE(SKIA)
    GrContext* m_grContext;
#endif
};

const float cMaxSigma = 4.0f;
const int cMaxKernelWidth = 25;

} // namespace WebCore

#endif // SharedGraphicsContext3D_h
