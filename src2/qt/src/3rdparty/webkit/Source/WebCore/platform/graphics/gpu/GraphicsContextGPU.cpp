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

#include "config.h"

#include "GraphicsContextGPU.h"

#include "DrawingBuffer.h"
#include "FloatRect.h"
#include "FloatSize.h"
#include "GraphicsContext3D.h"
#include "internal_glu.h"
#include "IntRect.h"
#include "LoopBlinnMathUtils.h"
#include "LoopBlinnPathProcessor.h"
#include "LoopBlinnSolidFillShader.h"
#include "Path.h"
#include "PlatformString.h"
#include "SharedGraphicsContext3D.h"
#if USE(SKIA)
#include "SkPath.h"
#endif
#include "Texture.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <wtf/OwnArrayPtr.h>
#include <wtf/text/CString.h>

namespace WebCore {

// Number of line segments used to approximate bezier curves.
const int pathTesselation = 30;
typedef void (GLAPIENTRY *TESSCB)();
typedef WTF::Vector<float> FloatVector;
typedef WTF::Vector<FloatPoint> FloatPointVector;

struct PathAndTransform {
    PathAndTransform(const Path& p, const AffineTransform& t)
        : path(p)
        , transform(t)
    {
    }
    Path path;
    AffineTransform transform;
};

struct GraphicsContextGPU::State {
    State()
        : m_fillColor(0, 0, 0, 255)
        , m_shadowColor(0, 0, 0, 0)
        , m_alpha(1.0f)
        , m_compositeOp(CompositeSourceOver)
        , m_numClippingPaths(0)
        , m_shadowOffset(0, 0)
        , m_shadowBlur(0)
        , m_shadowsIgnoreTransforms(false)
    {
    }
    State(const State& other)
        : m_fillColor(other.m_fillColor)
        , m_shadowColor(other.m_shadowColor)
        , m_alpha(other.m_alpha)
        , m_compositeOp(other.m_compositeOp)
        , m_ctm(other.m_ctm)
        , m_clippingPaths() // Don't copy; clipping paths are tracked per-state.
        , m_numClippingPaths(other.m_numClippingPaths)
        , m_shadowOffset(other.m_shadowOffset)
        , m_shadowBlur(other.m_shadowBlur)
        , m_shadowsIgnoreTransforms(other.m_shadowsIgnoreTransforms)
    {
    }
    Color m_fillColor;
    Color m_shadowColor;
    float m_alpha;
    CompositeOperator m_compositeOp;
    AffineTransform m_ctm;
    WTF::Vector<PathAndTransform> m_clippingPaths;
    int m_numClippingPaths;
    FloatSize m_shadowOffset;
    float m_shadowBlur;
    bool m_shadowsIgnoreTransforms;

    // Helper function for applying the state's alpha value to the given input
    // color to produce a new output color. The logic is the same as
    // PlatformContextSkia::State::applyAlpha(), but the type is different.
    Color applyAlpha(const Color& c)
    {
        int s = roundf(m_alpha * 256);
        if (s >= 256)
            return c;
        if (s < 0)
            return Color();

        int a = (c.alpha() * s) >> 8;
        return Color(c.red(), c.green(), c.blue(), a);
    }
    bool shadowActive() const
    {
        return m_shadowColor.alpha() > 0 && (m_shadowBlur || m_shadowOffset.width() || m_shadowOffset.height());
    }
    bool clippingEnabled() { return m_numClippingPaths > 0; }
};

static inline FloatPoint operator*(const FloatPoint& f, float scale)
{
    return FloatPoint(f.x() * scale, f.y() * scale);
}

static inline FloatPoint operator*(float scale, const FloatPoint& f)
{
    return FloatPoint(f.x() * scale, f.y() * scale);
}

static inline FloatSize operator*(const FloatSize& f, float scale)
{
    return FloatSize(f.width() * scale, f.height() * scale);
}

static inline FloatSize operator*(float scale, const FloatSize& f)
{
    return FloatSize(f.width() * scale, f.height() * scale);
}

class Quadratic {
  public:
    Quadratic(FloatPoint a, FloatPoint b, FloatPoint c) :
        m_a(a), m_b(b), m_c(c)
    {
    }
    static Quadratic fromBezier(FloatPoint p0, FloatPoint p1, FloatPoint p2)
    {
        FloatSize p1s(p1.x(), p1.y());
        FloatSize p2s(p2.x(), p2.y());
        FloatPoint b = -2.0f * p0 + 2.0f * p1s;
        FloatPoint c =         p0 - 2.0f * p1s + p2s;
        return Quadratic(p0, b, c);
    }
    inline FloatPoint evaluate(float t)
    {
        return m_a + t * (m_b + t * m_c);
    }
    FloatPoint m_a, m_b, m_c, m_d;
};

class Cubic {
  public:
    Cubic(FloatPoint a, FloatPoint b, FloatPoint c, FloatPoint d) :
        m_a(a), m_b(b), m_c(c), m_d(d) 
    {
    }
    static Cubic fromBezier(FloatPoint p0, FloatPoint p1, FloatPoint p2, FloatPoint p3)
    {
        FloatSize p1s(p1.x(), p1.y());
        FloatSize p2s(p2.x(), p2.y());
        FloatSize p3s(p3.x(), p3.y());
        FloatPoint b = -3.0f * p0 + 3.0f * p1s;
        FloatPoint c =  3.0f * p0 - 6.0f * p1s + 3.0f * p2s;
        FloatPoint d = -1.0f * p0 + 3.0f * p1s - 3.0f * p2s + p3s;
        return Cubic(p0, b, c, d);
    }
    inline FloatPoint evaluate(float t)
    {
        return m_a + t * (m_b + t * (m_c + t * m_d));
    }
    FloatPoint m_a, m_b, m_c, m_d;
};

GraphicsContextGPU::GraphicsContextGPU(SharedGraphicsContext3D* context, DrawingBuffer* drawingBuffer, const IntSize& size)
    : m_size(size)
    , m_context(context)
    , m_drawingBuffer(drawingBuffer)
    , m_state(0)
    , m_pathIndexBuffer(0)
    , m_pathVertexBuffer(0)
{
    m_flipMatrix.translate(-1.0f, 1.0f);
    m_flipMatrix.scale(2.0f / size.width(), -2.0f / size.height());

    m_stateStack.append(State());
    m_state = &m_stateStack.last();
}

GraphicsContextGPU::~GraphicsContextGPU()
{
    if (m_pathIndexBuffer)
        m_context->graphicsContext3D()->deleteBuffer(m_pathIndexBuffer);
    if (m_pathVertexBuffer)
        m_context->graphicsContext3D()->deleteBuffer(m_pathVertexBuffer);
}

void GraphicsContextGPU::bindFramebuffer()
{
    m_drawingBuffer->bind();
}

void GraphicsContextGPU::clearRect(const FloatRect& rect)
{
    bindFramebuffer();
    if (m_state->m_ctm.isIdentity() && !m_state->clippingEnabled()) {
        scissorClear(rect.x(), rect.y(), rect.width(), rect.height());
    } else {
        save();
        setCompositeOperation(CompositeClear);
        fillRect(rect, Color(RGBA32(0)), ColorSpaceDeviceRGB);
        restore();
    }
}

void GraphicsContextGPU::applyState()
{
    bindFramebuffer();
    m_context->applyCompositeOperator(m_state->m_compositeOp);
    applyClipping(m_state->clippingEnabled());
}

void GraphicsContextGPU::scissorClear(float x, float y, float width, float height)
{
    int intX = static_cast<int>(x + 0.5f);
    int intY = static_cast<int>(y + 0.5f);
    int intWidth = static_cast<int>(x + width + 0.5f) - intX;
    int intHeight = static_cast<int>(y + height + 0.5f) - intY;
    m_context->scissor(intX, m_size.height() - intHeight - intY, intWidth, intHeight);
    m_context->enable(GraphicsContext3D::SCISSOR_TEST);
    m_context->clearColor(Color(RGBA32(0)));
    m_context->clear(GraphicsContext3D::COLOR_BUFFER_BIT);
    m_context->disable(GraphicsContext3D::SCISSOR_TEST);
}

void GraphicsContextGPU::fillPath(const Path& path)
{
    if (m_state->shadowActive()) {
        beginShadowDraw();
        fillPathInternal(path, m_state->m_shadowColor);
        endShadowDraw(path.boundingRect());
    }

    applyState();
    fillPathInternal(path, m_state->applyAlpha(m_state->m_fillColor));
}

void GraphicsContextGPU::fillRect(const FloatRect& rect, const Color& color, ColorSpace colorSpace)
{
    if (m_state->shadowActive()) {
        beginShadowDraw();
        fillRectInternal(rect, m_state->m_shadowColor);
        endShadowDraw(rect);
    }

    applyState();
    fillRectInternal(rect, color);
}

void GraphicsContextGPU::fillRect(const FloatRect& rect)
{
    fillRect(rect, m_state->applyAlpha(m_state->m_fillColor), ColorSpaceDeviceRGB);
}

void GraphicsContextGPU::fillRectInternal(const FloatRect& rect, const Color& color)
{
    AffineTransform matrix(m_flipMatrix);
    matrix *= m_state->m_ctm;
    matrix.translate(rect.x(), rect.y());
    matrix.scale(rect.width(), rect.height());

    m_context->useQuadVertices();
    m_context->useFillSolidProgram(matrix, color);
    m_context->drawArrays(GraphicsContext3D::TRIANGLE_STRIP, 0, 4);
}

void GraphicsContextGPU::setFillColor(const Color& color, ColorSpace colorSpace)
{
    m_state->m_fillColor = color;
}

void GraphicsContextGPU::setAlpha(float alpha)
{
    m_state->m_alpha = alpha;
}

void GraphicsContextGPU::setShadowColor(const Color& color, ColorSpace)
{
    m_state->m_shadowColor = color;
}

void GraphicsContextGPU::setShadowOffset(const FloatSize& offset)
{
    m_state->m_shadowOffset = offset;
}

void GraphicsContextGPU::setShadowBlur(float shadowBlur)
{
    m_state->m_shadowBlur = shadowBlur;
}

void GraphicsContextGPU::setShadowsIgnoreTransforms(bool shadowsIgnoreTransforms)
{
    m_state->m_shadowsIgnoreTransforms = shadowsIgnoreTransforms;
}

void GraphicsContextGPU::translate(float x, float y)
{
    m_state->m_ctm.translate(x, y);
}

void GraphicsContextGPU::rotate(float angleInRadians)
{
    m_state->m_ctm.rotate(angleInRadians * (180.0f / M_PI));
}

void GraphicsContextGPU::scale(const FloatSize& size)
{
    m_state->m_ctm.scale(size.width(), size.height());
}

void GraphicsContextGPU::concatCTM(const AffineTransform& affine)
{
    m_state->m_ctm *= affine;
}

void GraphicsContextGPU::setCTM(const AffineTransform& affine)
{
    m_state->m_ctm = affine;
}

void GraphicsContextGPU::clipPath(const Path& path)
{
    bindFramebuffer();
    checkGLError("bindFramebuffer");
    beginStencilDraw(GraphicsContext3D::INCR);
    // Red is used so we can see it if it ends up in the color buffer.
    Color red(255, 0, 0, 255);
    fillPathInternal(path, red);
    m_state->m_clippingPaths.append(PathAndTransform(path, m_state->m_ctm));
    m_state->m_numClippingPaths++;
}

void GraphicsContextGPU::clipOut(const Path& path)
{
    ASSERT(!"clipOut is unsupported in GraphicsContextGPU.\n");
}

void GraphicsContextGPU::save()
{
    m_stateStack.append(State(m_stateStack.last()));
    m_state = &m_stateStack.last();
}

void GraphicsContextGPU::restore()
{
    ASSERT(!m_stateStack.isEmpty());
    const Vector<PathAndTransform>& clippingPaths = m_state->m_clippingPaths;
    if (!clippingPaths.isEmpty()) {
        beginStencilDraw(GraphicsContext3D::DECR);
        WTF::Vector<PathAndTransform>::const_iterator pathIter;
        for (pathIter = clippingPaths.begin(); pathIter < clippingPaths.end(); ++pathIter) {
            m_state->m_ctm = pathIter->transform;
            // Red is used so we can see it if it ends up in the color buffer.
            Color red(255, 0, 0, 255);
            fillPathInternal(pathIter->path, red);
        }
    }
    m_stateStack.removeLast();
    m_state = &m_stateStack.last();
}

void GraphicsContextGPU::drawTexturedRect(unsigned texture, const IntSize& textureSize, const FloatRect& srcRect, const FloatRect& dstRect, ColorSpace colorSpace, CompositeOperator compositeOp)
{
    bindFramebuffer();
    m_context->applyCompositeOperator(compositeOp);
    applyClipping(false);

    m_context->setActiveTexture(GraphicsContext3D::TEXTURE0);

    m_context->bindTexture(GraphicsContext3D::TEXTURE_2D, texture);

    drawTexturedQuad(textureSize, srcRect, dstRect, m_state->m_ctm, m_state->m_alpha);
}

void GraphicsContextGPU::drawTexturedRect(Texture* texture, const FloatRect& srcRect, const FloatRect& dstRect, ColorSpace colorSpace, CompositeOperator compositeOp)
{
    drawTexturedRect(texture, srcRect, dstRect, m_state->m_ctm, m_state->m_alpha, colorSpace, compositeOp, m_state->clippingEnabled());
}


void GraphicsContextGPU::drawTexturedRect(Texture* texture, const FloatRect& srcRect, const FloatRect& dstRect, const AffineTransform& transform, float alpha, ColorSpace colorSpace, CompositeOperator compositeOp, bool clip)
{
    bindFramebuffer();
    m_context->applyCompositeOperator(compositeOp);
    applyClipping(clip);
    const TilingData& tiles = texture->tiles();
    IntRect tileIdxRect = tiles.overlappedTileIndices(srcRect);

    m_context->setActiveTexture(GraphicsContext3D::TEXTURE0);

    for (int y = tileIdxRect.y(); y <= tileIdxRect.maxY(); y++) {
        for (int x = tileIdxRect.x(); x <= tileIdxRect.maxX(); x++)
            drawTexturedRectTile(texture, tiles.tileIndex(x, y), srcRect, dstRect, transform, alpha);
    }
}

void GraphicsContextGPU::drawTexturedRectTile(Texture* texture, int tile, const FloatRect& srcRect, const FloatRect& dstRect, const AffineTransform& transform, float alpha)
{
    if (dstRect.isEmpty())
        return;

    const TilingData& tiles = texture->tiles();

    texture->bindTile(tile);

    FloatRect srcRectClippedInTileSpace;
    FloatRect dstRectIntersected;
    tiles.intersectDrawQuad(srcRect, dstRect, tile, &srcRectClippedInTileSpace, &dstRectIntersected);

    IntRect tileBoundsWithBorder = tiles.tileBoundsWithBorder(tile);

    drawTexturedQuad(tileBoundsWithBorder.size(), srcRectClippedInTileSpace, dstRectIntersected, transform, alpha);
}

void GraphicsContextGPU::convolveRect(unsigned texture, const IntSize& textureSize, const FloatRect& srcRect, const FloatRect& dstRect, float imageIncrement[2], const float* kernel, int kernelWidth)
{
    m_context->bindTexture(GraphicsContext3D::TEXTURE_2D, texture);
    m_context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_S, GraphicsContext3D::CLAMP_TO_EDGE);
    m_context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_T, GraphicsContext3D::CLAMP_TO_EDGE);
    m_context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MIN_FILTER, GraphicsContext3D::NEAREST);
    m_context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MAG_FILTER, GraphicsContext3D::NEAREST);

    AffineTransform matrix(m_flipMatrix);
    matrix.translate(dstRect.x(), dstRect.y());
    matrix.scale(dstRect.width(), dstRect.height());

    AffineTransform texMatrix;
    texMatrix.scale(1.0f / textureSize.width(), 1.0f / textureSize.height());
    texMatrix.translate(srcRect.x(), srcRect.y());
    texMatrix.scale(srcRect.width(), srcRect.height());

    m_context->useQuadVertices();
    m_context->useConvolutionProgram(matrix, texMatrix, kernel, kernelWidth, imageIncrement);
    m_context->drawArrays(GraphicsContext3D::TRIANGLE_STRIP, 0, 4);
    checkGLError("glDrawArrays");
}

static float gauss(float x, float sigma)
{
    return exp(- (x * x) / (2.0f * sigma * sigma));
}

static void buildKernel(float sigma, float* kernel, int kernelWidth)
{
    float halfWidth = (kernelWidth - 1.0f) / 2.0f;
    float sum = 0.0f;
    for (int i = 0; i < kernelWidth; ++i) {
        kernel[i] = gauss(i - halfWidth, sigma);
        sum += kernel[i];
    }
    // Normalize the kernel
    float scale = 1.0f / sum;
    for (int i = 0; i < kernelWidth; ++i)
        kernel[i] *= scale;
}

void GraphicsContextGPU::drawTexturedQuad(const IntSize& textureSize, const FloatRect& srcRect, const FloatRect& dstRect, const AffineTransform& transform, float alpha)
{
    AffineTransform matrix(m_flipMatrix);
    matrix *= transform;
    matrix.translate(dstRect.x(), dstRect.y());
    matrix.scale(dstRect.width(), dstRect.height());

    AffineTransform texMatrix;
    texMatrix.scale(1.0f / textureSize.width(), 1.0f / textureSize.height());
    texMatrix.translate(srcRect.x(), srcRect.y());
    texMatrix.scale(srcRect.width(), srcRect.height());

    m_context->useQuadVertices();
    m_context->useTextureProgram(matrix, texMatrix, alpha);
    m_context->drawArrays(GraphicsContext3D::TRIANGLE_STRIP, 0, 4);
    checkGLError("glDrawArrays");
}

void GraphicsContextGPU::drawTexturedQuadMitchell(const IntSize& textureSize, const FloatRect& srcRect, const FloatRect& dstRect, const AffineTransform& transform, float alpha)
{
    static const float mitchellCoefficients[16] = {
         0.0f / 18.0f,   1.0f / 18.0f,  16.0f / 18.0f,   1.0f / 18.0f,
         0.0f / 18.0f,   9.0f / 18.0f,   0.0f / 18.0f,  -9.0f / 18.0f,
        -6.0f / 18.0f,  27.0f / 18.0f, -36.0f / 18.0f,  15.0f / 18.0f,
         7.0f / 18.0f, -21.0f / 18.0f,  21.0f / 18.0f,  -7.0f / 18.0f,
    };

    AffineTransform matrix(m_flipMatrix);
    matrix *= transform;
    matrix.translate(dstRect.x(), dstRect.y());
    matrix.scale(dstRect.width(), dstRect.height());

    float imageIncrement[2] = { 1.0f / textureSize.width(), 1.0f / textureSize.height() };

    AffineTransform texMatrix;
    texMatrix.scale(imageIncrement[0], imageIncrement[1]);
    texMatrix.translate(srcRect.x(), srcRect.y());
    texMatrix.scale(srcRect.width(), srcRect.height());

    m_context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MIN_FILTER, GraphicsContext3D::NEAREST);
    m_context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MAG_FILTER, GraphicsContext3D::NEAREST);

    m_context->useQuadVertices();
    m_context->useBicubicProgram(matrix, texMatrix, mitchellCoefficients, imageIncrement, alpha);
    m_context->drawArrays(GraphicsContext3D::TRIANGLE_STRIP, 0, 4);
    checkGLError("glDrawArrays");
}

void GraphicsContextGPU::setCompositeOperation(CompositeOperator op)
{
    m_state->m_compositeOp = op;
}

Texture* GraphicsContextGPU::createTexture(NativeImagePtr ptr, Texture::Format format, int width, int height)
{
    return m_context->createTexture(ptr, format, width, height);
}

Texture* GraphicsContextGPU::getTexture(NativeImagePtr ptr)
{
    return m_context->getTexture(ptr);
}

#if USE(SKIA)
// This is actually cross-platform code, but since its only caller is inside a
// USE(SKIA), it will cause a warning-as-error on Chrome/Mac.
static void interpolateQuadratic(FloatPointVector* vertices, const FloatPoint& p0, const FloatPoint& p1, const FloatPoint& p2)
{
    float tIncrement = 1.0f / pathTesselation, t = tIncrement;
    Quadratic c = Quadratic::fromBezier(p0, p1, p2);
    for (int i = 0; i < pathTesselation; ++i, t += tIncrement)
        vertices->append(c.evaluate(t));
}

static void interpolateCubic(FloatPointVector* vertices, const FloatPoint& p0, const FloatPoint& p1, const FloatPoint& p2, const FloatPoint& p3)
{
    float tIncrement = 1.0f / pathTesselation, t = tIncrement;
    Cubic c = Cubic::fromBezier(p0, p1, p2, p3);
    for (int i = 0; i < pathTesselation; ++i, t += tIncrement)
        vertices->append(c.evaluate(t));
}
#endif

struct PolygonData {
    PolygonData(FloatPointVector* vertices, WTF::Vector<short>* indices)
      : m_vertices(vertices)
      , m_indices(indices)
    {
    }
    FloatPointVector* m_vertices;
    WTF::Vector<short>* m_indices;
};

static void beginData(GLenum type, void* data)
{
    ASSERT(type == GL_TRIANGLES);
}

static void edgeFlagData(GLboolean flag, void* data)
{
}

static void vertexData(void* vertexData, void* data)
{
    static_cast<PolygonData*>(data)->m_indices->append(reinterpret_cast<long>(vertexData));
}

static void endData(void* data)
{
}

static void combineData(GLdouble coords[3], void* vertexData[4],
                                 GLfloat weight[4], void **outData, void* data)
{
    PolygonData* polygonData = static_cast<PolygonData*>(data);
    int index = polygonData->m_vertices->size();
    polygonData->m_vertices->append(FloatPoint(static_cast<float>(coords[0]), static_cast<float>(coords[1])));
    *outData = reinterpret_cast<void*>(index);
}

typedef void (*TESSCB)();

void GraphicsContextGPU::tesselateAndFillPath(const Path& path, const Color& color)
{
    if (!m_pathVertexBuffer)
        m_pathVertexBuffer = m_context->graphicsContext3D()->createBuffer();
    if (!m_pathIndexBuffer)
        m_pathIndexBuffer = m_context->graphicsContext3D()->createBuffer();

    AffineTransform matrix(m_flipMatrix);
    matrix *= m_state->m_ctm;

    FloatPointVector inVertices;
    WTF::Vector<size_t> contours;
#if USE(SKIA)
    const SkPath* skPath = path.platformPath();
    SkPoint pts[4];
    SkPath::Iter iter(*skPath, true);
    SkPath::Verb verb;
    while ((verb = iter.next(pts)) != SkPath::kDone_Verb) {
        switch (verb) {
        case SkPath::kMove_Verb:
            inVertices.append(pts[0]);
            break;
        case SkPath::kLine_Verb:
            inVertices.append(pts[1]);
            break;
        case SkPath::kQuad_Verb:
            interpolateQuadratic(&inVertices, pts[0], pts[1], pts[2]);
            break;
        case SkPath::kCubic_Verb:
            interpolateCubic(&inVertices, pts[0], pts[1], pts[2], pts[3]);
            break;
        case SkPath::kClose_Verb:
            contours.append(inVertices.size());
            break;
        case SkPath::kDone_Verb:
            break;
        }
    }
#else
    ASSERT(!"Path extraction not implemented on this platform.");
#endif

    if (contours.size() == 1 && LoopBlinnMathUtils::isConvex(inVertices.begin(), inVertices.size())) {
        m_context->graphicsContext3D()->bindBuffer(GraphicsContext3D::ARRAY_BUFFER, m_pathVertexBuffer);
        m_context->graphicsContext3D()->bufferData(GraphicsContext3D::ARRAY_BUFFER, inVertices.size() * 2 * sizeof(float), inVertices.data(), GraphicsContext3D::STREAM_DRAW);
        m_context->useFillSolidProgram(matrix, color);
        m_context->graphicsContext3D()->drawArrays(GraphicsContext3D::TRIANGLE_FAN, 0, inVertices.size());
        return;
    }

    OwnArrayPtr<double> inVerticesDouble = adoptArrayPtr(new double[inVertices.size() * 3]);
    for (size_t i = 0; i < inVertices.size(); ++i) {
        inVerticesDouble[i * 3    ] = inVertices[i].x();
        inVerticesDouble[i * 3 + 1] = inVertices[i].y();
        inVerticesDouble[i * 3 + 2] = 1.0;
    }

    GLUtesselator* tess = internal_gluNewTess();
    internal_gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NONZERO);
    internal_gluTessCallback(tess, GLU_TESS_BEGIN_DATA, (TESSCB) &beginData);
    internal_gluTessCallback(tess, GLU_TESS_VERTEX_DATA, (TESSCB) &vertexData);
    internal_gluTessCallback(tess, GLU_TESS_END_DATA, (TESSCB) &endData);
    internal_gluTessCallback(tess, GLU_TESS_EDGE_FLAG_DATA, (TESSCB) &edgeFlagData);
    internal_gluTessCallback(tess, GLU_TESS_COMBINE_DATA, (TESSCB) &combineData);
    WTF::Vector<short> indices;
    FloatPointVector vertices;
    vertices.reserveInitialCapacity(inVertices.size());
    PolygonData data(&vertices, &indices);
    internal_gluTessBeginPolygon(tess, &data);
    WTF::Vector<size_t>::const_iterator contour;
    size_t i = 0;
    for (contour = contours.begin(); contour != contours.end(); ++contour) {
        internal_gluTessBeginContour(tess);
        for (; i < *contour; ++i) {
            double* inVertex = &inVerticesDouble[i * 3];
            vertices.append(FloatPoint(inVertex[0], inVertex[1]));
            internal_gluTessVertex(tess, inVertex, reinterpret_cast<void*>(i));
        }
        internal_gluTessEndContour(tess);
    }
    internal_gluTessEndPolygon(tess);
    internal_gluDeleteTess(tess);

    m_context->graphicsContext3D()->bindBuffer(GraphicsContext3D::ARRAY_BUFFER, m_pathVertexBuffer);
    checkGLError("createVertexBufferFromPath, bindBuffer ARRAY_BUFFER");
    m_context->graphicsContext3D()->bufferData(GraphicsContext3D::ARRAY_BUFFER, vertices.size() * 2 * sizeof(float), vertices.data(), GraphicsContext3D::STREAM_DRAW);
    checkGLError("createVertexBufferFromPath, bufferData ARRAY_BUFFER");

    m_context->graphicsContext3D()->bindBuffer(GraphicsContext3D::ELEMENT_ARRAY_BUFFER, m_pathIndexBuffer);
    checkGLError("createVertexBufferFromPath, bindBuffer ELEMENT_ARRAY_BUFFER");
    m_context->graphicsContext3D()->bufferData(GraphicsContext3D::ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(short), indices.data(), GraphicsContext3D::STREAM_DRAW);
    checkGLError("createVertexBufferFromPath, bufferData ELEMENT_ARRAY_BUFFER");

    m_context->useFillSolidProgram(matrix, color);
    m_context->graphicsContext3D()->drawElements(GraphicsContext3D::TRIANGLES, indices.size(), GraphicsContext3D::UNSIGNED_SHORT, 0);
}

void GraphicsContextGPU::fillPathInternal(const Path& path, const Color& color)
{
    if (SharedGraphicsContext3D::useLoopBlinnForPathRendering()) {
        m_pathCache.clear();
        LoopBlinnPathProcessor processor;
        processor.process(path, m_pathCache);
        if (!m_pathVertexBuffer)
            m_pathVertexBuffer = m_context->createBuffer();
        m_context->bindBuffer(GraphicsContext3D::ARRAY_BUFFER, m_pathVertexBuffer);
        int byteSizeOfVertices = 2 * m_pathCache.numberOfVertices() * sizeof(float);
        int byteSizeOfTexCoords = 3 * m_pathCache.numberOfVertices() * sizeof(float);
        int byteSizeOfInteriorVertices = 2 * m_pathCache.numberOfInteriorVertices() * sizeof(float);
        m_context->bufferData(GraphicsContext3D::ARRAY_BUFFER,
                              byteSizeOfVertices + byteSizeOfTexCoords + byteSizeOfInteriorVertices,
                              GraphicsContext3D::STATIC_DRAW);
        m_context->bufferSubData(GraphicsContext3D::ARRAY_BUFFER, 0, byteSizeOfVertices, m_pathCache.vertices());
        m_context->bufferSubData(GraphicsContext3D::ARRAY_BUFFER, byteSizeOfVertices, byteSizeOfTexCoords, m_pathCache.texcoords());
        m_context->bufferSubData(GraphicsContext3D::ARRAY_BUFFER, byteSizeOfVertices + byteSizeOfTexCoords, byteSizeOfInteriorVertices, m_pathCache.interiorVertices());

        AffineTransform matrix(m_flipMatrix);
        matrix *= m_state->m_ctm;

        // Draw the exterior
        m_context->useLoopBlinnExteriorProgram(0, byteSizeOfVertices, matrix, color);
        m_context->drawArrays(GraphicsContext3D::TRIANGLES, 0, m_pathCache.numberOfVertices());

        // Draw the interior
        m_context->useLoopBlinnInteriorProgram(byteSizeOfVertices + byteSizeOfTexCoords, matrix, color);
        m_context->drawArrays(GraphicsContext3D::TRIANGLES, 0, m_pathCache.numberOfInteriorVertices());
    } else {
        tesselateAndFillPath(path, color);
    }
}

FloatRect GraphicsContextGPU::flipRect(const FloatRect& rect)
{
    FloatRect flippedRect(rect);
    flippedRect.setY(m_size.height() - rect.y());
    flippedRect.setHeight(-rect.height());
    return flippedRect;
}

void GraphicsContextGPU::clearBorders(const FloatRect& rect, int width)
{
    scissorClear(rect.x(), rect.y() - width, rect.width() + width, width);
    scissorClear(rect.maxX(), rect.y(), width, rect.height() + width);
    scissorClear(rect.x() - width, rect.maxY(), rect.width() + width, width);
    scissorClear(rect.x() - width, rect.y() - width, width, rect.height() + width);
}

void GraphicsContextGPU::beginShadowDraw()
{
    float offsetX = m_state->m_shadowOffset.width();
    float offsetY = m_state->m_shadowOffset.height();
    save();
    if (m_state->m_shadowsIgnoreTransforms) {
        AffineTransform newCTM;
        newCTM.translate(offsetX, -offsetY);
        newCTM *= m_state->m_ctm;
        m_state->m_ctm = newCTM;
    } else
        m_state->m_ctm.translate(offsetX, offsetY);

    if (m_state->m_shadowBlur > 0) {
        // Draw hard shadow to offscreen buffer 0.
        DrawingBuffer* dstBuffer = m_context->getOffscreenBuffer(0, m_size);
        dstBuffer->bind();
        m_context->applyCompositeOperator(CompositeCopy);
        applyClipping(false);
        m_context->clearColor(Color(RGBA32(0)));
        m_context->clear(GraphicsContext3D::COLOR_BUFFER_BIT);
    } else {
        applyState();
    }
}

void GraphicsContextGPU::endShadowDraw(const FloatRect& boundingBox)
{
    if (m_state->m_shadowBlur > 0) {
        // Buffer 0 contains the primitive drawn with a hard shadow.
        DrawingBuffer* srcBuffer = m_context->getOffscreenBuffer(0, m_size);
        DrawingBuffer* dstBuffer = m_context->getOffscreenBuffer(1, m_size);

        float sigma = m_state->m_shadowBlur * 0.333333f;
        FloatRect shadowBoundingBox(m_state->m_ctm.mapRect(boundingBox));
        FloatRect rect(FloatPoint(0, 0), m_size);
        FloatRect srcRect(shadowBoundingBox);

        int scaleFactor = 1;
        while (sigma > cMaxSigma) {
            srcRect.scale(0.5f);
            scaleFactor *= 2;
            sigma *= 0.5f;
        }
        srcRect = enclosingIntRect(srcRect);
        srcRect.scale(scaleFactor);
        for (int i = 1; i < scaleFactor; i *= 2) {
            dstBuffer->bind();
            m_context->bindTexture(GraphicsContext3D::TEXTURE_2D, srcBuffer->colorBuffer());
            m_context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_S, GraphicsContext3D::CLAMP_TO_EDGE);
            m_context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_T, GraphicsContext3D::CLAMP_TO_EDGE);
            m_context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MIN_FILTER, GraphicsContext3D::LINEAR);
            m_context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MAG_FILTER, GraphicsContext3D::LINEAR);
            FloatRect dstRect(srcRect);
            dstRect.scale(0.5f);
            // Clear out 1 pixel border for linear filtering.
            clearBorders(dstRect, 1);
            drawTexturedQuad(srcBuffer->size(), flipRect(srcRect), dstRect, AffineTransform(), 1.0);
            srcRect = dstRect;
            std::swap(srcBuffer, dstBuffer);
        }

        int halfWidth = static_cast<int>(sigma * 3.0f);
        int kernelWidth = halfWidth * 2 + 1;
        OwnArrayPtr<float> kernel = adoptArrayPtr(new float[kernelWidth]);
        buildKernel(sigma, kernel.get(), kernelWidth);

        if (scaleFactor > 1) {
            scissorClear(srcRect.maxX(), srcRect.y(), kernelWidth, srcRect.height());
            scissorClear(srcRect.x() - kernelWidth, srcRect.y(), kernelWidth, srcRect.height());
        }

        // Blur in X offscreen.
        dstBuffer->bind();
        srcRect.inflateX(halfWidth);
        srcRect.intersect(rect);
        float imageIncrementX[2] = {1.0f / srcBuffer->size().width(), 0.0f};
        convolveRect(srcBuffer->colorBuffer(), srcBuffer->size(), flipRect(srcRect), srcRect, imageIncrementX, kernel.get(), kernelWidth);

        if (scaleFactor > 1) {
            scissorClear(srcRect.x(), srcRect.maxY(), srcRect.width(), kernelWidth);
            scissorClear(srcRect.x(), srcRect.y() - kernelWidth, srcRect.width(), kernelWidth);
        }
        srcRect.inflateY(halfWidth);
        srcRect.intersect(rect);
        std::swap(srcBuffer, dstBuffer);

        float imageIncrementY[2] = {0.0f, 1.0f / srcBuffer->size().height()};
        if (scaleFactor > 1) {
            // Blur in Y offscreen.
            dstBuffer->bind();
            convolveRect(srcBuffer->colorBuffer(), srcBuffer->size(), flipRect(srcRect), srcRect, imageIncrementY, kernel.get(), kernelWidth);
            // Clear out 2 pixel border for bicubic filtering.
            clearBorders(srcRect, 2);
            std::swap(srcBuffer, dstBuffer);

            // Upsample srcBuffer -> main framebuffer using bicubic filtering.
            applyState();
            m_context->bindTexture(GraphicsContext3D::TEXTURE_2D, srcBuffer->colorBuffer());
            FloatRect dstRect = srcRect;
            dstRect.scale(scaleFactor);
            drawTexturedQuadMitchell(srcBuffer->size(), flipRect(srcRect), dstRect, AffineTransform(), 1.0);
        } else {
            // Blur in Y directly to framebuffer.
            applyState();
            convolveRect(srcBuffer->colorBuffer(), srcBuffer->size(), flipRect(srcRect), srcRect, imageIncrementY, kernel.get(), kernelWidth);
        }
    }
    restore();
}

void GraphicsContextGPU::beginStencilDraw(unsigned op)
{
    // Turn on stencil test.
    m_context->enableStencil(true);
    checkGLError("enable STENCIL_TEST");

    // Stencil test never passes, so colorbuffer is not drawn.
    m_context->graphicsContext3D()->stencilFunc(GraphicsContext3D::NEVER, 1, 1);
    checkGLError("stencilFunc");

    // All writes incremement the stencil buffer.
    m_context->graphicsContext3D()->stencilOp(op, op, op);
    checkGLError("stencilOp");
}

void GraphicsContextGPU::applyClipping(bool enable)
{
    m_context->enableStencil(enable);
    if (enable) {
        // Enable drawing only where stencil is non-zero.
        m_context->graphicsContext3D()->stencilFunc(GraphicsContext3D::EQUAL, m_state->m_numClippingPaths, -1);
        checkGLError("stencilFunc");
        // Keep all stencil values the same.
        m_context->graphicsContext3D()->stencilOp(GraphicsContext3D::KEEP,
                                                  GraphicsContext3D::KEEP,
                                                  GraphicsContext3D::KEEP);
        checkGLError("stencilOp");
    }
}

void GraphicsContextGPU::checkGLError(const char* header)
{
#ifndef NDEBUG
    unsigned err;
    while ((err = m_context->getError()) != GraphicsContext3D::NO_ERROR) {
        const char* errorStr = "*** UNKNOWN ERROR ***";
        switch (err) {
        case GraphicsContext3D::INVALID_ENUM:
            errorStr = "GraphicsContext3D::INVALID_ENUM";
            break;
        case GraphicsContext3D::INVALID_VALUE:
            errorStr = "GraphicsContext3D::INVALID_VALUE";
            break;
        case GraphicsContext3D::INVALID_OPERATION:
            errorStr = "GraphicsContext3D::INVALID_OPERATION";
            break;
        case GraphicsContext3D::INVALID_FRAMEBUFFER_OPERATION:
            errorStr = "GraphicsContext3D::INVALID_FRAMEBUFFER_OPERATION";
            break;
        case GraphicsContext3D::OUT_OF_MEMORY:
            errorStr = "GraphicsContext3D::OUT_OF_MEMORY";
            break;
        }
        if (header)
            LOG_ERROR("%s:  %s", header, errorStr);
        else
            LOG_ERROR("%s", errorStr);
    }
#endif
}

}
