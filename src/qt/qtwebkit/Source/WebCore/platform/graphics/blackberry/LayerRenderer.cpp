/*
 * Copyright (C) 2010, 2011, 2012, 2013 Research In Motion Limited. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#if USE(ACCELERATED_COMPOSITING)
#include "LayerRenderer.h"

#include "LayerCompositingThread.h"
#include "LayerFilterRenderer.h"
#include "LayerRendererClient.h"
#include "TextureCacheCompositingThread.h"

#include <BlackBerryPlatformGraphics.h>
#include <BlackBerryPlatformLog.h>
#include <EGL/egl.h>
#include <limits>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

#define DEBUG_LAYER_ANIMATIONS 0 // Show running animations as green.
#define DEBUG_CLIPPING 0

using BlackBerry::Platform::Graphics::GLES2Context;
using BlackBerry::Platform::Graphics::GLES2Program;
using namespace std;

namespace WebCore {

#ifndef NDEBUG
#define checkGLError() \
{ \
    if (GLenum error = glGetError()) \
        BlackBerry::Platform::logAlways(BlackBerry::Platform::LogLevelCritical, "%s:%d GL Error: 0x%x ", __FILE__, __LINE__, error); \
}
#else
#define checkGLError()
#endif

GLuint LayerRenderer::loadShader(GLenum type, const char* shaderSource)
{
    GLuint shader = glCreateShader(type);
    if (!shader)
        return 0;
    glShaderSource(shader, 1, &shaderSource, 0);
    glCompileShader(shader);
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        char infoLog[2048];
        GLsizei length;
        glGetShaderInfoLog(shader, 2048, &length, infoLog);
        BlackBerry::Platform::logAlways(BlackBerry::Platform::LogLevelCritical, "Failed to compile shader:\n%s\nlog: %s", shaderSource, infoLog);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint LayerRenderer::loadShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource)
{
    GLuint vertexShader;
    GLuint fragmentShader;
    GLuint programObject;
    GLint linked;
    vertexShader = loadShader(GL_VERTEX_SHADER, vertexShaderSource);
    if (!vertexShader)
        return 0;
    fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    if (!fragmentShader) {
        glDeleteShader(vertexShader);
        return 0;
    }
    programObject = glCreateProgram();
    if (programObject) {
        glAttachShader(programObject, vertexShader);
        glAttachShader(programObject, fragmentShader);
        glLinkProgram(programObject);
        glGetProgramiv(programObject, GL_LINK_STATUS, &linked);
        if (!linked) {
            glDeleteProgram(programObject);
            programObject = 0;
        }
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return programObject;
}

TransformationMatrix LayerRenderer::orthoMatrix(float left, float right, float bottom, float top, float nearZ, float farZ)
{
    float deltaX = right - left;
    float deltaY = top - bottom;
    float deltaZ = farZ - nearZ;
    TransformationMatrix ortho;
    if (!deltaX || !deltaY || !deltaZ)
        return ortho;
    ortho.setM11(2.0f / deltaX);
    ortho.setM41(-(right + left) / deltaX);
    ortho.setM22(2.0f / deltaY);
    ortho.setM42(-(top + bottom) / deltaY);
    ortho.setM33(-2.0f / deltaZ);
    ortho.setM43(-(nearZ + farZ) / deltaZ);
    return ortho;
}

static Vector<LayerCompositingThread*> rawPtrVectorFromRefPtrVector(const Vector<RefPtr<LayerCompositingThread> >& sublayers)
{
    Vector<LayerCompositingThread*> sublayerList;
    for (size_t i = 0; i < sublayers.size(); i++)
        sublayerList.append(sublayers[i].get());

    return sublayerList;
}

PassOwnPtr<LayerRenderer> LayerRenderer::create(LayerRendererClient* client)
{
    return adoptPtr(new LayerRenderer(client));
}

LayerRenderer::LayerRenderer(LayerRendererClient* client)
    : m_client(client)
    , m_scale(1.0)
    , m_animationTime(-numeric_limits<double>::infinity())
    , m_fbo(0)
    , m_currentLayerRendererSurface(0)
    , m_isRobustnessSupported(false)
    , m_needsCommit(false)
    , m_stencilCleared(false)
{
    // We're now initializing lazily, so a check if the context can be made current
    // will have to suffice to determine if hardware compositing is possible.
    m_hardwareCompositing = makeContextCurrent();
    if (m_hardwareCompositing) {
        m_isRobustnessSupported = String(reinterpret_cast<const char*>(::glGetString(GL_EXTENSIONS))).contains("GL_EXT_robustness");
        if (m_isRobustnessSupported)
            m_glGetGraphicsResetStatusEXT = reinterpret_cast<PFNGLGETGRAPHICSRESETSTATUSEXTPROC>(eglGetProcAddress("glGetGraphicsResetStatusEXT"));
    }
}

LayerRenderer::~LayerRenderer()
{
    if (m_hardwareCompositing) {
        makeContextCurrent();
        if (m_fbo)
            glDeleteFramebuffers(1, &m_fbo);

        for (size_t i = 0; i < NumberOfPrograms; ++i)
            glDeleteProgram(m_programs[i].m_program);

        // Free up all GL textures.
        while (m_layers.begin() != m_layers.end()) {
            LayerSet::iterator iter = m_layers.begin();
            (*iter)->deleteTextures();
            (*iter)->setLayerRenderer(0);
            removeLayer(*iter);
        }

        textureCacheCompositingThread()->clear();
    }
}

void LayerRenderer::releaseLayerResources()
{
    if (m_hardwareCompositing) {
        makeContextCurrent();
        // Free up all GL textures.
        for (LayerSet::iterator iter = m_layers.begin(); iter != m_layers.end(); ++iter)
            (*iter)->deleteTextures();

        textureCacheCompositingThread()->clear();
    }
}

static inline bool compareLayerW(const LayerCompositingThread* a, const LayerCompositingThread* b)
{
    return a->centerW() > b->centerW();
}

void LayerRenderer::prepareFrame(double animationTime, LayerCompositingThread* rootLayer)
{
    if (animationTime != m_animationTime) {
        m_animationTime = animationTime;

        // Aha, new frame! Reset rendering results.
        bool wasEmpty = m_lastRenderingResults.isEmpty();
        m_lastRenderingResults = LayerRenderingResults();
        m_lastRenderingResults.wasEmpty = wasEmpty;
    }

    if (!rootLayer)
        return;

    bool isContextCurrent = makeContextCurrent();
    prepareFrameRecursive(rootLayer, animationTime, isContextCurrent);
}

void LayerRenderer::setViewport(const IntRect& targetRect, const IntRect& clipRect, const FloatRect& visibleRect, const IntRect& layoutRect, const IntSize& contentsSize)
{
    // These parameters are used to calculate position of fixed position elements
    m_visibleRect = visibleRect;
    m_layoutRect = layoutRect;
    m_contentsSize = contentsSize;

    m_viewport = targetRect;
    m_scissorRect = clipRect;

    // The clipRect parameter uses render target coordinates, map to normalized device coordinates
    m_clipRect = clipRect;
    m_clipRect.intersect(targetRect);
    m_clipRect = FloatRect(-1 + 2 * (m_clipRect.x() - targetRect.x()) / targetRect.width(),
        -1 + 2 * (m_clipRect.y() - targetRect.y()) / targetRect.height(),
        2 * m_clipRect.width() / targetRect.width(),
        2 * m_clipRect.height() / targetRect.height());

#if DEBUG_CLIPPING
    printf("LayerRenderer::setViewport() m_visibleRect=(%.2f,%.2f %.2fx%.2f), m_layoutRect=(%d,%d %dx%d), m_contentsSize=(%dx%d), m_viewport=(%d,%d %dx%d), m_scissorRect=(%d,%d %dx%d), m_clipRect=(%.2f,%.2f %.2fx%.2f)\n",
        m_visibleRect.x(), m_visibleRect.y(), m_visibleRect.width(), m_visibleRect.height(),
        m_layoutRect.x(), m_layoutRect.y(), m_layoutRect.width(), m_layoutRect.height(),
        m_contentsSize.width(), m_contentsSize.height(),
        m_viewport.x(), m_viewport.y(), m_viewport.width(), m_viewport.height(),
        m_scissorRect.x(), m_scissorRect.y(), m_scissorRect.width(), m_scissorRect.height(),
        m_clipRect.x(), m_clipRect.y(), m_clipRect.width(), m_clipRect.height());
    fflush(stdout);
#endif

    if (!m_hardwareCompositing)
        return;

    // Okay, we're going to do some drawing.
    if (!makeContextCurrent())
        return;

    // Get rid of any bound buffer that might affect the interpretation of our
    // glVertexAttribPointer calls.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_STENCIL_TEST);

    // If culling is enabled then we will cull the backface.
    glCullFace(GL_BACK);

    // The BlackBerry::Platform::GraphicsContext uses OpenGL conventions, so everything is upside down
    glFrontFace(GL_CW);

    checkGLError();

    glViewport(m_viewport.x(), m_viewport.y(), m_viewport.width(), m_viewport.height());

    glEnable(GL_SCISSOR_TEST);
#if DEBUG_CLIPPING
    printf("LayerRenderer::compositeLayers(): clipping to (%d,%d %dx%d)\n", m_scissorRect.x(), m_scissorRect.y(), m_scissorRect.width(), m_scissorRect.height());
    fflush(stdout);
#endif
    glScissor(m_scissorRect.x(), m_scissorRect.y(), m_scissorRect.width(), m_scissorRect.height());

    m_stencilCleared = false;
}

void LayerRenderer::compositeLayers(const TransformationMatrix& matrix, LayerCompositingThread* rootLayer)
{
    ASSERT(m_hardwareCompositing);
    if (!m_hardwareCompositing)
        return;

    if (!rootLayer)
        return;

    // Used to draw scale invariant layers. We assume uniform scale.
    // The matrix maps to normalized device coordinates, a system that maps the
    // viewport to the interval -1 to 1.
    // So it has to scale down by a factor equal to one half the viewport.
    m_scale = matrix.m11() * m_viewport.width() / 2;

    Vector<RefPtr<LayerCompositingThread> > surfaceLayers;
    const Vector<RefPtr<LayerCompositingThread> >& sublayers = rootLayer->sublayers();
    for (size_t i = 0; i < sublayers.size(); i++) {
        float opacity = 1;
        FloatRect clipRect(m_clipRect);
        updateLayersRecursive(sublayers[i].get(), TransformationMatrix(), matrix, surfaceLayers, opacity, clipRect);
    }

    // Decompose the dirty rect into a set of non-overlaping rectangles
    // (they need to not overlap so that the blending code doesn't draw any region twice).
    for (int i = 0; i < LayerRenderingResults::NumberOfDirtyRects; ++i) {
        BlackBerry::Platform::IntRectRegion region(BlackBerry::Platform::IntRect(m_lastRenderingResults.dirtyRect(i)));
        m_lastRenderingResults.dirtyRegion = BlackBerry::Platform::IntRectRegion::unionRegions(m_lastRenderingResults.dirtyRegion, region);
    }

    // If we won't draw anything, don't touch the OpenGL APIs.
    if (m_lastRenderingResults.isEmpty() && m_lastRenderingResults.wasEmpty)
        return;

    // Okay, we're going to do some drawing.
    if (!makeContextCurrent())
        return;

    // If some layers should be drawn on temporary surfaces, we should do it first.
    if (!surfaceLayers.isEmpty())
        drawLayersOnSurfaces(surfaceLayers);

    // Don't render the root layer, the BlackBerry port uses the BackingStore to draw the
    // root layer.
    for (size_t i = 0; i < sublayers.size(); i++) {
        int currentStencilValue = 0;
        FloatRect clipRect(m_clipRect);
        compositeLayersRecursive(sublayers[i].get(), currentStencilValue, clipRect);
    }

    // We need to make sure that all texture resource usage is finished before
    // unlocking the texture resources, so force a glFinish() in that case.
    if (m_layersLockingTextureResources.size())
        glFinish();

    m_client->context()->swapBuffers();

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_STENCIL_TEST);

    // PR 147254, the EGL implementation crashes when the last bound texture
    // was an EGLImage, and you try to bind another texture and the pixmap
    // backing the EGLImage was deleted in between. Make this easier for the
    // driver by unbinding early (when the pixmap is hopefully still around).
    glBindTexture(GL_TEXTURE_2D, 0);

    LayerSet::iterator iter = m_layersLockingTextureResources.begin();
    for (; iter != m_layersLockingTextureResources.end(); ++iter)
        (*iter)->releaseTextureResources();

    m_layersLockingTextureResources.clear();

    if (m_needsCommit) {
        m_needsCommit = false;
        rootLayer->scheduleCommit();
    }

    textureCacheCompositingThread()->collectGarbage();
}

static float texcoords[4 * 2] = { 0, 0,  1, 0,  1, 1,  0, 1 };

void LayerRenderer::compositeBuffer(const TransformationMatrix& transform, const FloatRect& contents, BlackBerry::Platform::Graphics::Buffer* buffer, bool contentsOpaque, float opacity)
{
    if (!buffer)
        return;

    FloatQuad vertices(transform.mapPoint(contents.minXMinYCorner()),
        transform.mapPoint(contents.minXMaxYCorner()),
        transform.mapPoint(contents.maxXMaxYCorner()),
        transform.mapPoint(contents.maxXMinYCorner()));

    if (!vertices.boundingBox().intersects(m_clipRect))
        return;

    if (!contentsOpaque || opacity < 1.0f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    } else
        glDisable(GL_BLEND);

    if (BlackBerry::Platform::Graphics::lockAndBindBufferGLTexture(buffer, GL_TEXTURE_2D)) {
        const GLES2Program& program = useProgram(LayerProgramRGBA);
        glUniform1f(program.opacityLocation(), opacity);

        glVertexAttribPointer(program.positionLocation(), 2, GL_FLOAT, GL_FALSE, 0, &vertices);
        glVertexAttribPointer(program.texCoordLocation(), 2, GL_FLOAT, GL_FALSE, 0, texcoords);

        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        BlackBerry::Platform::Graphics::releaseBufferGLTexture(buffer);
    }
}

void LayerRenderer::drawColor(const TransformationMatrix& transform, const FloatRect& contents, const Color& color)
{
    FloatQuad vertices(transform.mapPoint(contents.minXMinYCorner()),
        transform.mapPoint(contents.minXMaxYCorner()),
        transform.mapPoint(contents.maxXMaxYCorner()),
        transform.mapPoint(contents.maxXMinYCorner()));

    if (!vertices.boundingBox().intersects(m_clipRect))
        return;

    const GLES2Program& program = useProgram(ColorProgram);

    if (color.alpha() < 255) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    } else
        glDisable(GL_BLEND);

    glUniform4f(m_colorColorLocation, color.red() / 255.0, color.green() / 255.0, color.blue() / 255.0, color.alpha() / 255.0);
    glVertexAttribPointer(program.positionLocation(), 2, GL_FLOAT, GL_FALSE, 0, &vertices);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

bool LayerRenderer::useSurface(LayerRendererSurface* surface)
{
    if (m_currentLayerRendererSurface == surface)
        return true;

    m_currentLayerRendererSurface = surface;
    if (!surface) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(m_viewport.x(), m_viewport.y(), m_viewport.width(), m_viewport.height());
        return true;
    }

    surface->ensureTexture();

    GLuint texid = surface->texture()->platformTexture();

    if (!m_fbo)
        glGenFramebuffers(1, &m_fbo);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texid, 0);

#ifndef NDEBUG
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "glCheckFramebufferStatus error %x\n", status);
        return false;
    }
#endif

    glViewport(0, 0, surface->size().width(), surface->size().height());
    return true;
}

void LayerRenderer::drawLayersOnSurfaces(const Vector<RefPtr<LayerCompositingThread> >& surfaceLayers)
{
    // Normally, an upside-down transform is used, as is the GL custom. However, when drawing
    // layers to surfaces, a right-side-up transform is used, so we need to switch the winding order
    // for culling.
    glFrontFace(GL_CCW);

    for (int i = surfaceLayers.size() - 1; i >= 0; i--) {
        LayerCompositingThread* layer = surfaceLayers[i].get();
        LayerRendererSurface* surface = layer->layerRendererSurface();
        if (!surface || !useSurface(surface))
            continue;

        glDisable(GL_SCISSOR_TEST);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);

        int currentStencilValue = 0;
        FloatRect clipRect(-1, -1, 2, 2);
        compositeLayersRecursive(surfaceLayers[i].get(), currentStencilValue, clipRect);

#if ENABLE(CSS_FILTERS)
        if (!m_filterRenderer)
            m_filterRenderer = LayerFilterRenderer::create(GLES2Program::PositionAttributeIndex, GLES2Program::TexCoordAttributeIndex);
        if (layer->filterOperationsChanged()) {
            layer->setFilterOperationsChanged(false);
            layer->setFilterActions(m_filterRenderer->actionsForOperations(surface, layer->filters().operations()));
        }
        m_filterRenderer->applyActions(m_fbo, layer, layer->filterActions());
        glClearColor(0, 0, 0, 0);
#endif
    }

    glFrontFace(GL_CW);

    // If there are layers drawn on surfaces, we need to switch to default framebuffer.
    // Otherwise, we just need to set viewport.
    useSurface(0);
    glEnable(GL_SCISSOR_TEST);
    glScissor(m_scissorRect.x(), m_scissorRect.y(), m_scissorRect.width(), m_scissorRect.height());
}

void LayerRenderer::addLayer(LayerCompositingThread* layer)
{
    m_layers.add(layer);
}

bool LayerRenderer::removeLayer(LayerCompositingThread* layer)
{
    LayerSet::iterator iter = m_layers.find(layer);
    if (iter == m_layers.end())
        return false;
    m_layers.remove(layer);
    return true;
}

void LayerRenderer::addLayerToReleaseTextureResourcesList(LayerCompositingThread* layer)
{
    m_layersLockingTextureResources.add(layer);
}

static int glRound(float f)
{
    return floorf(f + 0.5f);
}

// Transform normalized device coordinates to window coordinates
// as specified in the OpenGL ES 2.0 spec section 2.12.1.
IntRect LayerRenderer::toOpenGLWindowCoordinates(const FloatRect& r) const
{
    float vw2 = m_viewport.width() / 2.0;
    float vh2 = m_viewport.height() / 2.0;
    float ox = m_viewport.x() + vw2;
    float oy = m_viewport.y() + vh2;
    return IntRect(glRound(r.x() * vw2 + ox), glRound(r.y() * vh2 + oy), glRound(r.width() * vw2), glRound(r.height() * vh2));
}

static FloatRect toPixelCoordinates(const FloatRect& rect, const IntRect& viewport, int surfaceHeight)
{
    float vw2 = viewport.width() / 2.0;
    float vh2 = viewport.height() / 2.0;
    float ox = viewport.x() + vw2;
    float oy = surfaceHeight - (viewport.y() + vh2);
    return FloatRect(rect.x() * vw2 + ox, -(rect.y() + rect.height()) * vh2 + oy, rect.width() * vw2, rect.height() * vh2);
}

// Transform normalized device coordinates to window coordinates as WebKit understands them.
//
// The OpenGL surface may be larger than the WebKit window, and OpenGL window coordinates
// have origin in bottom left while WebKit window coordinates origin is in top left.
// The viewport is setup to cover the upper portion of the larger OpenGL surface.
IntRect LayerRenderer::toWindowCoordinates(const FloatRect& rect) const
{
    return enclosingIntRect(toPixelCoordinates(rect, m_viewport, m_client->context()->surfaceSize().height()));
}

IntRect LayerRenderer::toPixelViewportCoordinates(const FloatRect& rect) const
{
    // The clip rect defines the web page's pixel viewport (to use ViewportAccessor terminology),
    // not to be confused with the GL viewport. So translate from window coordinates to pixel
    // viewport coordinates.
    int surfaceHeight = m_client->context()->surfaceSize().height();
    FloatRect pixelViewport = toPixelCoordinates(m_clipRect, m_viewport, surfaceHeight);
    FloatRect result = toPixelCoordinates(rect, m_viewport, surfaceHeight);
    result.move(-pixelViewport.x(), -pixelViewport.y());
    return enclosingIntRect(result);
}

IntRect LayerRenderer::toDocumentViewportCoordinates(const FloatRect& rect) const
{
    // Similar to toPixelViewportCoordinates except that this also takes any zoom into account.
    FloatRect result = toPixelViewportCoordinates(rect);
    result.scale(1 / m_scale);
    return enclosingIntRect(result);
}

void LayerRenderer::drawDebugBorder(const Vector<FloatPoint>& transformedBounds, const Color& borderColor, float borderWidth)
{
    if (borderColor.alpha() < 255) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    } else
        glDisable(GL_BLEND);

    const GLES2Program& program = useProgram(ColorProgram);
    glVertexAttribPointer(program.positionLocation(), 2, GL_FLOAT, GL_FALSE, 0, transformedBounds.data());
    glUniform4f(m_colorColorLocation, borderColor.red() / 255.0, borderColor.green() / 255.0, borderColor.blue() / 255.0, borderColor.alpha() / 255.0);

    glLineWidth(borderWidth);
    glDrawArrays(GL_LINE_LOOP, 0, transformedBounds.size());
}

// Draws a debug border around the layer's bounds.
void LayerRenderer::drawDebugBorder(LayerCompositingThread* layer)
{
    Color borderColor = layer->borderColor();

#if DEBUG_LAYER_ANIMATIONS
    if (layer->hasRunningAnimations())
        borderColor =  Color(0x00, 0xFF, 0x00, 0xFF);
#endif

    if (!borderColor.alpha())
        return;

    // If we're rendering to a surface, don't include debug border inside the surface.
    if (m_currentLayerRendererSurface)
        return;

    Vector<FloatPoint> transformedBounds;
    if (layerAlreadyOnSurface(layer))
        transformedBounds = layer->layerRendererSurface()->transformedBounds();
    else
        transformedBounds = layer->transformedBounds();

    drawDebugBorder(transformedBounds, borderColor, std::max(1.0f, layer->borderWidth()));
}

// Clears a rectangle inside the layer's bounds.
void LayerRenderer::drawHolePunchRect(LayerCompositingThread* layer)
{
    const GLES2Program& program = useProgram(ColorProgram);
    glUniform4f(m_colorColorLocation, 0, 0, 0, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ZERO);
    FloatQuad hole = layer->transformedHolePunchRect();
    glVertexAttribPointer(program.positionLocation(), 2, GL_FLOAT, GL_FALSE, 0, &hole);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    checkGLError();
}

void LayerRenderer::prepareFrameRecursive(LayerCompositingThread* layer, double animationTime, bool isContextCurrent)
{
    // This might cause the layer to recompute some attributes.
    m_lastRenderingResults.needsAnimationFrame |= layer->updateAnimations(animationTime);

    if (isContextCurrent) {
        // Even non-visible layers need to perform their texture jobs, or they will
        // pile up and waste memory.
        if (layer->needsTexture())
            layer->updateTextureContentsIfNeeded();
        if (layer->maskLayer() && layer->maskLayer()->needsTexture())
            layer->maskLayer()->updateTextureContentsIfNeeded();
        if (layer->replicaLayer()) {
            LayerCompositingThread* replica = layer->replicaLayer();
            if (replica->needsTexture())
                replica->updateTextureContentsIfNeeded();
            if (replica->maskLayer() && replica->maskLayer()->needsTexture())
                replica->maskLayer()->updateTextureContentsIfNeeded();
        }
    }

    const Vector<RefPtr<LayerCompositingThread> >& sublayers = layer->sublayers();
    for (size_t i = 0; i < sublayers.size(); i++)
        prepareFrameRecursive(sublayers[i].get(), animationTime, isContextCurrent);
}

void LayerRenderer::updateLayersRecursive(LayerCompositingThread* layer, const TransformationMatrix& matrix, const TransformationMatrix& projectionMatrix, Vector<RefPtr<LayerCompositingThread> >& surfaceLayers, float opacity, FloatRect clipRect)
{
    // The contract for LayerCompositingThread::setLayerRenderer is it must be set if the layer has been rendered.
    // So do it now, before we render it in compositeLayersRecursive.
    layer->setLayerRenderer(this);
    if (layer->maskLayer())
        layer->maskLayer()->setLayerRenderer(this);
    if (layer->replicaLayer()) {
        LayerCompositingThread* replica = layer->replicaLayer();
        replica->setLayerRenderer(this);
        if (replica->maskLayer())
            replica->maskLayer()->setLayerRenderer(this);
    }

    // Compute the new matrix transformation that will be applied to this layer and
    // all its sublayers. It's important to remember that the layer's position
    // is the position of the layer's anchor point. Also, the coordinate system used
    // assumes that the origin is at the lower left even though the coordinates the browser
    // gives us for the layers are for the upper left corner. The Y flip happens via
    // the orthographic projection applied at render time.
    // The transformation chain for the layer is (using the Matrix x Vector order):
    // M = M[p] * Tr[l] * M[l] * Tr[c]
    // Where M[p] is the parent matrix passed down to the function
    //       Tr[l] is the translation matrix locating the layer's anchor point
    //       Tr[c] is the translation offset between the anchor point and the center of the layer
    //       M[l] is the layer's matrix (applied at the anchor point)
    // This transform creates a coordinate system whose origin is the center of the layer.
    // Note that the final matrix used by the shader for the layer is P * M * S . This final product
    // is computed in drawTexturedQuad().
    // Where: P is the projection matrix
    //        M is the layer's matrix computed above
    //        S is the scale adjustment (to scale up to the layer size)
    FloatSize bounds = layer->bounds();
    if (layer->sizeIsScaleInvariant())
        bounds.scale(1.0 / m_scale);
    FloatPoint anchorPoint = layer->anchorPoint();
    FloatPoint position = layer->position();

    // Layer whose hasFixedContainer is true will get scrolled relative to
    // the fixed positioned parent.
    if (!layer->hasFixedContainer() && (layer->isFixedPosition() || layer->hasFixedAncestorInDOMTree())) {
        FloatRect layoutRect = m_layoutRect;
        FloatSize contentsSize = m_contentsSize;
        FloatRect visibleRect = m_visibleRect;
        for (LayerCompositingThread* curr = layer->superlayer(); curr; curr = curr->superlayer()) {

            if (curr->isContainerForFixedPositionLayers()) {
                layoutRect = curr->frameVisibleRect();
                contentsSize = curr->frameContentsSize();

                // If we reach a container for fixed position layers, and it has its override's position set, it means it is a scrollable iframe
                // currently being scrolled. Otherwise, use the WebKit-thread scroll position stored in frameVisibleRect().
                if (curr->override()->isPositionSet()) {
                    // Inverted logic of
                    // FloatPoint layerPosition(-scrollPosition.x() + anchor.x() * bounds.width(),
                    //                          -scrollPosition.y() + anchor.y() * bounds.height());
                    FloatPoint scrollPosition(
                        -(curr->override()->position().x() - (curr->anchorPoint().x() * curr->bounds().width())),
                        -(curr->override()->position().y() - (curr->anchorPoint().y() * curr->bounds().height())));
                    visibleRect = FloatRect(scrollPosition, layoutRect.size());
                } else
                    visibleRect = layoutRect;

                break;
            }
        }

        FloatPoint maximumScrollPosition = FloatPoint(0, 0) + (contentsSize - visibleRect.size());
        FloatPoint maximumLayoutScrollPosition = FloatPoint(0, 0) + (contentsSize - layoutRect.size());

        // The basic idea here is to set visible x/y to the value we want, and
        // layout x/y to the value WebCore layouted the fixed element to.
        float visibleY;
        float layoutY;
        if (layer->isFixedToTop()) {
            visibleY = max(0.0f, min(maximumScrollPosition.y(), visibleRect.y()));
            layoutY = max(0.0f, min(maximumLayoutScrollPosition.y(), layoutRect.y()));
        } else {
            visibleY = min(contentsSize.height(), visibleRect.y() + visibleRect.height());
            layoutY = min(contentsSize.height(), max(0.0f, layoutRect.y()) + layoutRect.height());
        }
        position.setY(position.y() + (visibleY - layoutY));

        float visibleX;
        float layoutX;
        if (layer->isFixedToLeft()) {
            visibleX = max(0.0f, min(maximumScrollPosition.x(), visibleRect.x()));
            layoutX = max(0.0f, min(maximumLayoutScrollPosition.x(), layoutRect.x()));
        } else {
            visibleX = min(contentsSize.width(), visibleRect.x() + visibleRect.width());
            layoutX = min(contentsSize.width(), max(0.0f, layoutRect.x()) + layoutRect.width());
        }
        position.setX(position.x() + (visibleX - layoutX));
    }

    // Offset between anchor point and the center of the quad.
    float centerOffsetX = (0.5 - anchorPoint.x()) * bounds.width();
    float centerOffsetY = (0.5 - anchorPoint.y()) * bounds.height();

    // M = M[p]
    TransformationMatrix localMatrix = matrix;
    // M = M[p] * Tr[l]
    localMatrix.translate3d(position.x(), position.y(), layer->anchorPointZ());
    // M = M[p] * Tr[l] * M[l]
    localMatrix.multiply(layer->transform());
    // M = M[p] * Tr[l] * M[l] * Tr[c]
    localMatrix.translate3d(centerOffsetX, centerOffsetY, -layer->anchorPointZ());

    // Calculate the layer's opacity.
    opacity *= layer->opacity();

    TransformationMatrix localProjectionMatrix = projectionMatrix;
#if ENABLE(CSS_FILTERS)
    bool useLayerRendererSurface = layer->maskLayer() || layer->replicaLayer() || layer->filters().size();
#else
    bool useLayerRendererSurface = layer->maskLayer() || layer->replicaLayer();
#endif
    if (!useLayerRendererSurface) {
        layer->setDrawOpacity(opacity);
        layer->clearLayerRendererSurface();
    } else {
        if (!layer->layerRendererSurface())
            layer->createLayerRendererSurface();

        LayerRendererSurface* surface = layer->layerRendererSurface();

        layer->setDrawOpacity(1.0);
        surface->setDrawOpacity(opacity);

        surface->setDrawTransform(localMatrix, projectionMatrix);
        if (layer->replicaLayer()) {
            TransformationMatrix replicaMatrix = localMatrix;
            replicaMatrix.translate3d(-0.5 * bounds.width(), -0.5 * bounds.height(), 0);
            replicaMatrix.translate3d(layer->replicaLayer()->position().x(), layer->replicaLayer()->position().y(), 0);
            replicaMatrix.multiply(layer->replicaLayer()->transform());
            replicaMatrix.translate3d(centerOffsetX, centerOffsetY, 0);
            surface->setReplicaDrawTransform(replicaMatrix, projectionMatrix);
        }

        IntRect contentRect = enclosingIntRect(FloatRect(FloatPoint::zero(), bounds));
        surface->setContentRect(contentRect);

        localProjectionMatrix = orthoMatrix(contentRect.x(), contentRect.maxX(), contentRect.y(), contentRect.maxY(), -1000, 1000);
        // The origin of the new surface is the upper left corner of the layer.
        TransformationMatrix drawTransform;
        drawTransform.translate3d(0.5 * bounds.width(), 0.5 * bounds.height(), 0);
        // This layer will start using new transformation.
        localMatrix = drawTransform;

        surfaceLayers.append(layer);
    }

    layer->setDrawTransform(m_scale, localMatrix, localProjectionMatrix);

#if ENABLE(VIDEO)
    bool layerVisible = clipRect.intersects(layer->boundingBox()) || layer->mediaPlayer();
#else
    bool layerVisible = clipRect.intersects(layer->boundingBox());
#endif

    if (layer->needsTexture() && layerVisible) {
        IntRect dirtyRect = toWindowCoordinates(intersection(layer->boundingBox(), clipRect));
        m_lastRenderingResults.addDirtyRect(dirtyRect);
    }

    if (layer->masksToBounds())
        clipRect.intersect(layer->boundingBox());

    // Flatten to 2D if the layer doesn't preserve 3D.
    if (!layer->preserves3D()) {
        localMatrix.setM13(0);
        localMatrix.setM23(0);
        localMatrix.setM31(0);
        localMatrix.setM32(0);
        localMatrix.setM33(1);
        localMatrix.setM34(0);
        localMatrix.setM43(0);
    }

    // Apply the sublayer transform.
    localMatrix.multiply(layer->sublayerTransform());

    // The origin of the sublayers is actually the bottom left corner of the layer
    // (or top left when looking it it from the browser's pespective) instead of the center.
    // The matrix passed down to the sublayers is therefore:
    // M[s] = M * Tr[-center]
    localMatrix.translate3d(-bounds.width() * 0.5, -bounds.height() * 0.5, 0);

    const Vector<RefPtr<LayerCompositingThread> >& sublayers = layer->sublayers();
    for (size_t i = 0; i < sublayers.size(); i++)
        updateLayersRecursive(sublayers[i].get(), localMatrix, localProjectionMatrix, surfaceLayers, opacity, clipRect);
}

static bool hasRotationalComponent(const TransformationMatrix& m)
{
    return m.m12() || m.m13() || m.m23() || m.m21() || m.m31() || m.m32();
}

bool LayerRenderer::layerAlreadyOnSurface(LayerCompositingThread* layer) const
{
    return layer->layerRendererSurface() && layer->layerRendererSurface() != m_currentLayerRendererSurface;
}

static void collect3DPreservingLayers(Vector<LayerCompositingThread*>& layers)
{
    for (size_t i = 0; i < layers.size(); ++i) {
        LayerCompositingThread* layer = layers[i];
        if (!layer->preserves3D() || !layer->sublayers().size())
            continue;

        Vector<LayerCompositingThread*> sublayers = rawPtrVectorFromRefPtrVector(layer->sublayers());
        collect3DPreservingLayers(sublayers);
        layers.insert(i+1, sublayers);
        i += sublayers.size();
    }
}

void LayerRenderer::compositeLayersRecursive(LayerCompositingThread* layer, int stencilValue, FloatRect clipRect)
{
    FloatRect rect;
    if (layerAlreadyOnSurface(layer))
        rect = layer->layerRendererSurface()->boundingBox();
    else
        rect = layer->boundingBox();

#if ENABLE(VIDEO)
    bool layerVisible = clipRect.intersects(rect) || layer->mediaPlayer();
#else
    bool layerVisible = clipRect.intersects(rect);
#endif

    layer->setVisible(layerVisible);

    // Note that there are two types of layers:
    // 1. Layers that have their own GraphicsContext and can draw their contents on demand (layer->drawsContent() == true).
    // 2. Layers that are just containers of images/video/etc that don't own a GraphicsContext (layer->contents() == true).

    if ((layer->needsTexture() || layer->layerRendererSurface()) && layerVisible) {
        updateScissorIfNeeded(clipRect);

        if (stencilValue) {
            glStencilFunc(GL_EQUAL, stencilValue, 0xff);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
        }

        if (layer->doubleSided())
            glDisable(GL_CULL_FACE);
        else
            glEnable(GL_CULL_FACE);

        if (layer->hasVisibleHolePunchRect())
            drawHolePunchRect(layer);

        // Draw the surface onto another surface or screen.
        bool drawSurface = layerAlreadyOnSurface(layer);
        // The texture format for the surface is RGBA.
        LayerData::LayerProgram layerProgram = drawSurface ? LayerData::LayerProgramRGBA : layer->layerProgram();

        if (!drawSurface) {
            const GLES2Program& program = useLayerProgram(layerProgram);
            layer->drawTextures(program, m_scale, m_visibleRect, clipRect);
        } else {
            // Draw the reflection if it exists.
            if (layer->replicaLayer()) {
                // If this layer and its reflection both have mask, we need another temporary surface.
                // Since this use case should be rare, currently it's not handled and the mask for
                // the reflection is applied only when this layer has no mask.
                LayerCompositingThread* mask = layer->maskLayer();
                if (!mask && layer->replicaLayer())
                    mask = layer->replicaLayer()->maskLayer();

                const GLES2Program& program = useLayerProgram(layerProgram, mask);
                layer->drawSurface(program, layer->layerRendererSurface()->replicaDrawTransform(), mask);
            }

            const GLES2Program& program = useLayerProgram(layerProgram, layer->maskLayer());
            layer->drawSurface(program, layer->layerRendererSurface()->drawTransform(), layer->maskLayer());
        }
    }

    // Draw the debug border if there is one.
    drawDebugBorder(layer);

    // The texture for the LayerRendererSurface can be released after the surface was drawn on another surface.
    if (layerAlreadyOnSurface(layer)) {
        layer->layerRendererSurface()->releaseTexture();
        return;
    }

    // If we need to mask to bounds but the transformation has a rotational component
    // to it, scissoring is not enough and we need to use the stencil buffer for clipping.
    bool stencilClip = layer->masksToBounds() && hasRotationalComponent(layer->drawTransform());

    if (stencilClip) {
        if (!m_stencilCleared) {
            glStencilMask(0xffffffff);
            glClearStencil(0);
            glClear(GL_STENCIL_BUFFER_BIT);
            m_stencilCleared = true;
        }

        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_EQUAL, stencilValue, 0xff);
        glStencilOp(GL_KEEP, GL_INCR, GL_INCR);

        updateScissorIfNeeded(clipRect);
        const GLES2Program& program = useProgram(ColorProgram);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glVertexAttribPointer(program.positionLocation(), 2, GL_FLOAT, GL_FALSE, 0, layer->transformedBounds().data());
        glDrawArrays(GL_TRIANGLE_FAN, 0, layer->transformedBounds().size());
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }

    if (layer->masksToBounds())
        clipRect.intersect(layer->boundingBox());

    // Here, we need to sort the whole subtree of layers with preserve-3d. It
    // affects all children, and the children of any children with preserve-3d,
    // and so on.
    Vector<LayerCompositingThread*> sublayers = rawPtrVectorFromRefPtrVector(layer->sublayers());

    bool preserves3D = layer->preserves3D();
    bool superlayerPreserves3D = layer->superlayer() && layer->superlayer()->preserves3D();

    // Collect and render all sublayers with preserves-3D.
    // If the superlayer preserves 3D, we've already collected and rendered its
    // children, so bail.
    if (preserves3D && !superlayerPreserves3D) {
        collect3DPreservingLayers(sublayers);
        std::stable_sort(sublayers.begin(), sublayers.end(), compareLayerW);
    }

    int newStencilValue = stencilClip ? stencilValue+1 : stencilValue;
    for (size_t i = 0; i < sublayers.size(); i++) {
        LayerCompositingThread* sublayer = sublayers[i];
        // The root of the 3d-preserving subtree has collected all
        // 3d-preserving layers and their children and will render them all in
        // the right order.
        if (preserves3D && superlayerPreserves3D)
            continue;

        compositeLayersRecursive(sublayer, newStencilValue, clipRect);
    }

    if (stencilClip) {
        glStencilFunc(GL_LEQUAL, stencilValue, 0xff);
        glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);

        updateScissorIfNeeded(clipRect);
        const GLES2Program& program = useProgram(ColorProgram);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glVertexAttribPointer(program.positionLocation(), 2, GL_FLOAT, GL_FALSE, 0, layer->transformedBounds().data());
        glDrawArrays(GL_TRIANGLE_FAN, 0, layer->transformedBounds().size());
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        if (!stencilValue)
            glDisable(GL_STENCIL_TEST);
    }
}

void LayerRenderer::updateScissorIfNeeded(const FloatRect& clipRect)
{
#if DEBUG_CLIPPING
    printf("LayerRenderer::updateScissorIfNeeded(): clipRect=(%.2f,%.2f %.2fx%.2f)\n", clipRect.x(), clipRect.y(), clipRect.width(), clipRect.height());
    fflush(stdout);
#endif
    IntRect clipRectWC = toOpenGLWindowCoordinates(clipRect);
    if (m_scissorRect == clipRectWC)
        return;

    m_scissorRect = clipRectWC;
#if DEBUG_CLIPPING
    printf("LayerRenderer::updateScissorIfNeeded(): clipping to (%d,%d %dx%d)\n", m_scissorRect.x(), m_scissorRect.y(), m_scissorRect.width(), m_scissorRect.height());
    fflush(stdout);
#endif
    glScissor(m_scissorRect.x(), m_scissorRect.y(), m_scissorRect.width(), m_scissorRect.height());
}

bool LayerRenderer::makeContextCurrent()
{
    bool ret = m_client->context()->makeCurrent();
    if (ret && m_isRobustnessSupported) {
        if (m_glGetGraphicsResetStatusEXT() != GL_NO_ERROR) {
            BlackBerry::Platform::logAlways(BlackBerry::Platform::LogLevelCritical, "Robust OpenGL context has been reset. Aborting.");
            CRASH();
        }
    }
    return ret;
}

bool LayerRenderer::createProgram(ProgramIndex program)
{
    // Shaders for drawing the layer contents.
    const char* vertexShaderString =
        "attribute vec4 a_position;   \n"
        "attribute vec2 a_texCoord;   \n"
        "varying vec2 v_texCoord;     \n"
        "void main()                  \n"
        "{                            \n"
        "  gl_Position = a_position;  \n"
        "  v_texCoord = a_texCoord;   \n"
        "}                            \n";

    const char* fragmentShaderStringRGBA =
        "varying mediump vec2 v_texCoord;                           \n"
        "uniform lowp sampler2D s_texture;                          \n"
        "uniform lowp float alpha;                                  \n"
        "void main()                                                \n"
        "{                                                          \n"
        "  gl_FragColor = texture2D(s_texture, v_texCoord) * alpha; \n"
        "}                                                          \n";

    const char* fragmentShaderStringBGRA =
        "varying mediump vec2 v_texCoord;                                \n"
        "uniform lowp sampler2D s_texture;                               \n"
        "uniform lowp float alpha;                                       \n"
        "void main()                                                     \n"
        "{                                                               \n"
        "  gl_FragColor = texture2D(s_texture, v_texCoord).bgra * alpha; \n"
        "}                                                               \n";

    const char* fragmentShaderStringMaskRGBA =
        "varying mediump vec2 v_texCoord;                           \n"
        "uniform lowp sampler2D s_texture;                          \n"
        "uniform lowp sampler2D s_mask;                             \n"
        "uniform lowp float alpha;                                  \n"
        "void main()                                                \n"
        "{                                                          \n"
        "  lowp vec4 texColor = texture2D(s_texture, v_texCoord);   \n"
        "  lowp vec4 maskColor = texture2D(s_mask, v_texCoord);     \n"
        "  gl_FragColor = vec4(texColor.x, texColor.y, texColor.z, texColor.w) * alpha * maskColor.w;           \n"
        "}                                                          \n";

    const char* fragmentShaderStringMaskBGRA =
        "varying mediump vec2 v_texCoord;                                \n"
        "uniform lowp sampler2D s_texture;                               \n"
        "uniform lowp sampler2D s_mask;                                  \n"
        "uniform lowp float alpha;                                       \n"
        "void main()                                                     \n"
        "{                                                               \n"
        "  lowp vec4 texColor = texture2D(s_texture, v_texCoord).bgra;             \n"
        "  lowp vec4 maskColor = texture2D(s_mask, v_texCoord).bgra;          \n"
        "  gl_FragColor = vec4(texColor.x, texColor.y, texColor.z, texColor.w) * alpha * maskColor.w;         \n"
        "}                                                               \n";

    // Shaders for drawing the debug borders around the layers.
    const char* colorVertexShaderString =
        "attribute vec4 a_position;   \n"
        "void main()                  \n"
        "{                            \n"
        "   gl_Position = a_position; \n"
        "}                            \n";

    const char* colorFragmentShaderString =
        "uniform lowp vec4 color;     \n"
        "void main()                  \n"
        "{                            \n"
        "  gl_FragColor = color;      \n"
        "}                            \n";

    const char* vertexShader = 0;
    const char* fragmentShader = 0;

    switch (program) {
    case LayerProgramRGBA:
    case LayerProgramBGRA:
    case LayerMaskProgramRGBA:
    case LayerMaskProgramBGRA:
        vertexShader = vertexShaderString;
        break;
    case ColorProgram:
        vertexShader = colorVertexShaderString;
        break;
    case NumberOfPrograms:
        return false;
    }

    switch (program) {
    case LayerProgramRGBA:
        fragmentShader = fragmentShaderStringRGBA;
        break;
    case LayerProgramBGRA:
        fragmentShader = fragmentShaderStringBGRA;
        break;
    case LayerMaskProgramRGBA:
        fragmentShader = fragmentShaderStringMaskRGBA;
        break;
    case LayerMaskProgramBGRA:
        fragmentShader = fragmentShaderStringMaskBGRA;
        break;
    case ColorProgram:
        fragmentShader = colorFragmentShaderString;
        break;
    case NumberOfPrograms:
        return false;
    }

    if (!vertexShader || !fragmentShader)
        return false;

    GLuint programObject = loadShaderProgram(vertexShader, fragmentShader);
    if (!programObject) {
        LOG_ERROR("Failed to create program %u", program);
        return false;
    }

    m_programs[program].m_program = programObject;

    // Binds the given attribute name to a common location across all programs
    // used by the compositor. This allows the code to bind the attributes only once
    // even when switching between programs.
    glBindAttribLocation(programObject, GLES2Program::PositionAttributeIndex, "a_position");
    glBindAttribLocation(programObject, GLES2Program::TexCoordAttributeIndex, "a_texCoord");

    checkGLError();

    // Re-link the shader to get the new attrib location to take effect.
    glLinkProgram(programObject);

    checkGLError();

    // Get locations of uniforms for the layer content shader program.
    m_programs[program].m_locations[GLES2Program::OpacityUniform] = glGetUniformLocation(programObject, "alpha");
    switch (program) {
    case LayerProgramRGBA:
    case LayerProgramBGRA: {
        GLint samplerLocation = glGetUniformLocation(programObject, "s_texture");
        glUseProgram(programObject);
        glUniform1i(samplerLocation, 0);
        break;
    }
    case LayerMaskProgramRGBA:
    case LayerMaskProgramBGRA: {
        GLint maskSamplerLocation = glGetUniformLocation(programObject, "s_texture");
        GLint maskSamplerLocationMask = glGetUniformLocation(programObject, "s_mask");
        glUseProgram(programObject);
        glUniform1i(maskSamplerLocation, 0);
        glUniform1i(maskSamplerLocationMask, 1);
        break;
    }
    case ColorProgram:
        // Get locations of uniforms for the debug border shader program.
        m_colorColorLocation = glGetUniformLocation(programObject, "color");
        break;
    case NumberOfPrograms:
        return false;
    }

    return true;
}

const GLES2Program& LayerRenderer::useProgram(ProgramIndex index)
{
    ASSERT(index < NumberOfPrograms);
    const GLES2Program& program = m_programs[index];
    if (!program.isValid() && !createProgram(index))
        return program;

    glUseProgram(program.m_program);

    glEnableVertexAttribArray(program.positionLocation());
    if (index != ColorProgram)
        glEnableVertexAttribArray(program.texCoordLocation());

    return program;
}

const GLES2Program& LayerRenderer::useLayerProgram(LayerData::LayerProgram layerProgram, bool isMask /* = false */)
{
    int program = layerProgram;
    if (isMask)
        program += MaskPrograms;
    return useProgram(static_cast<ProgramIndex>(program));
}

void LayerRenderingResults::addDirtyRect(const IntRect& rect)
{
    IntRect dirtyUnion[NumberOfDirtyRects];
    int smallestIncrease = INT_MAX;
    int modifiedRect = 0;
    for (int i = 0; i < NumberOfDirtyRects; ++i) {
        dirtyUnion[i] = m_dirtyRects[i];
        dirtyUnion[i].unite(rect);
        int increase = dirtyUnion[i].width()*dirtyUnion[i].height() - m_dirtyRects[i].width()*m_dirtyRects[i].height();
        if (increase < smallestIncrease) {
            smallestIncrease = increase;
            modifiedRect = i;
        }
    }

    m_dirtyRects[modifiedRect] = dirtyUnion[modifiedRect];
}

bool LayerRenderingResults::isEmpty() const
{
    for (int i = 0; i < NumberOfDirtyRects; ++i) {
        if (!m_dirtyRects[i].isEmpty())
            return false;
    }
    return true;
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
