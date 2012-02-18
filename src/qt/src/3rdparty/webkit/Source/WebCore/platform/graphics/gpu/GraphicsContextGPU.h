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

#ifndef GraphicsContextGPU_h
#define GraphicsContextGPU_h

#include "AffineTransform.h"
#include "Color.h"
#include "ColorSpace.h"
#include "GraphicsTypes.h"
#include "ImageSource.h"
#include "LoopBlinnPathCache.h"
#include "Texture.h"

#include <wtf/HashMap.h>
#include <wtf/Noncopyable.h>
#include <wtf/Vector.h>

namespace WebCore {

class Color;
class DrawingBuffer;
class FloatRect;
class GraphicsContext3D;
class Path;
class SharedGraphicsContext3D;

class GraphicsContextGPU {
    WTF_MAKE_NONCOPYABLE(GraphicsContextGPU);
public:
    GraphicsContextGPU(SharedGraphicsContext3D*, DrawingBuffer*, const IntSize&);
    ~GraphicsContextGPU();

    void fillPath(const Path&);
    void fillRect(const FloatRect&, const Color&, ColorSpace);
    void fillRect(const FloatRect&);
    void clearRect(const FloatRect&);
    void setFillColor(const Color&, ColorSpace);
    void setAlpha(float alpha);
    void setShadowColor(const Color&, ColorSpace);
    void setShadowOffset(const FloatSize&);
    void setShadowBlur(float);
    void setShadowsIgnoreTransforms(bool);
    void setCompositeOperation(CompositeOperator);
    void translate(float x, float y);
    void rotate(float angleInRadians);
    void scale(const FloatSize&);
    void concatCTM(const AffineTransform&);
    void setCTM(const AffineTransform&);
    void clipPath(const Path&);
    void clipOut(const Path&);

    void save();
    void restore();

    // non-standard functions
    // These are not standard GraphicsContext functions, and should be pushed
    // down into a PlatformContextGLES2 at some point.

    // This version is called by the canvas->canvas draws.
    void drawTexturedRect(unsigned texture, const IntSize& textureSize, const FloatRect& srcRect, const FloatRect& dstRect, ColorSpace, CompositeOperator);
    // This version is called by BitmapImage::draw().
    void drawTexturedRect(Texture*, const FloatRect& srcRect, const FloatRect& dstRect, ColorSpace, CompositeOperator);
    // This version is called by the above, and by the software->hardware uploads.
    void drawTexturedRect(Texture*, const FloatRect& srcRect, const FloatRect& dstRect, const AffineTransform&, float alpha, ColorSpace, CompositeOperator, bool clip);
    Texture* createTexture(NativeImagePtr, Texture::Format, int width, int height);
    Texture* getTexture(NativeImagePtr);

    SharedGraphicsContext3D* context() const { return m_context; }

    void bindFramebuffer();

    DrawingBuffer* drawingBuffer() const { return m_drawingBuffer; }

private:
    void applyState();
    void scissorClear(float x, float y, float width, float height);
    void drawTexturedRectTile(Texture* texture, int tile, const FloatRect& srcRect, const FloatRect& dstRect, const AffineTransform&, float alpha);
    void drawTexturedQuad(const IntSize& textureSize, const FloatRect& srcRect, const FloatRect& dstRect, const AffineTransform&, float alpha);
    void drawTexturedQuadMitchell(const IntSize& textureSize, const FloatRect& srcRect, const FloatRect& dstRect, const AffineTransform&, float alpha);
    void convolveRect(unsigned texture, const IntSize& textureSize, const FloatRect& srcRect, const FloatRect& dstRect, float imageIncrement[2], const float* kernel, int kernelWidth);

    void tesselateAndFillPath(const Path&, const Color&);
    void fillPathInternal(const Path&, const Color&);
    void fillRectInternal(const FloatRect&, const Color&);
    FloatRect flipRect(const FloatRect&);
    void clearBorders(const FloatRect&, int width);
    void beginShadowDraw();
    void endShadowDraw(const FloatRect& boundingBox);
    void beginStencilDraw(unsigned op);
    void applyClipping(bool enable);
    void checkGLError(const char* header);

    IntSize m_size;

    SharedGraphicsContext3D* m_context;
    DrawingBuffer* m_drawingBuffer;

    struct State;
    typedef WTF::Vector<State> StateVector;
    StateVector m_stateStack;
    State* m_state;
    AffineTransform m_flipMatrix;

    // Members for GPU-accelerated path rendering.
    LoopBlinnPathCache m_pathCache;
    unsigned m_pathIndexBuffer;
    unsigned m_pathVertexBuffer;
};

}

#endif // GraphicsContextGPU_h
