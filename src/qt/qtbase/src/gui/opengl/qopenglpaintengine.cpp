/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*
    When the active program changes, we need to update it's uniforms.
    We could track state for each program and only update stale uniforms
        - Could lead to lots of overhead if there's a lot of programs
    We could update all the uniforms when the program changes
        - Could end up updating lots of uniforms which don't need updating

    Updating uniforms should be cheap, so the overhead of updating up-to-date
    uniforms should be minimal. It's also less complex.

    Things which _may_ cause a different program to be used:
        - Change in brush/pen style
        - Change in painter opacity
        - Change in composition mode

    Whenever we set a mode on the shader manager - it needs to tell us if it had
    to switch to a different program.

    The shader manager should only switch when we tell it to. E.g. if we set a new
    brush style and then switch to transparent painter, we only want it to compile
    and use the correct program when we really need it.
*/

// #define QT_OPENGL_CACHE_AS_VBOS

#include "qopenglgradientcache_p.h"
#include "qopengltexturecache_p.h"
#include "qopenglpaintengine_p.h"

#include <string.h> //for memcpy
#include <qmath.h>

#include <private/qopengl_p.h>
#include <private/qopenglcontext_p.h>
#include <private/qopenglextensions_p.h>
#include <private/qmath_p.h>
#include <private/qpaintengineex_p.h>
#include <QPaintEngine>
#include <private/qpainter_p.h>
#include <private/qfontengine_p.h>
#include <private/qdatabuffer_p.h>
#include <private/qstatictext_p.h>
#include <private/qtriangulator_p.h>

#include "qopenglengineshadermanager_p.h"
#include "qopengl2pexvertexarray_p.h"
#include "qopengltextureglyphcache_p.h"

#include <QDebug>

QT_BEGIN_NAMESPACE



Q_GUI_EXPORT QImage qt_imageForBrush(int brushStyle, bool invert);

////////////////////////////////// Private Methods //////////////////////////////////////////

QOpenGL2PaintEngineExPrivate::~QOpenGL2PaintEngineExPrivate()
{
    delete shaderManager;

    while (pathCaches.size()) {
        QVectorPath::CacheEntry *e = *(pathCaches.constBegin());
        e->cleanup(e->engine, e->data);
        e->data = 0;
        e->engine = 0;
    }

    if (elementIndicesVBOId != 0) {
        funcs.glDeleteBuffers(1, &elementIndicesVBOId);
        elementIndicesVBOId = 0;
    }
}

void QOpenGL2PaintEngineExPrivate::updateTextureFilter(GLenum target, GLenum wrapMode, bool smoothPixmapTransform, GLuint id)
{
//    funcs.glActiveTexture(GL_TEXTURE0 + QT_BRUSH_TEXTURE_UNIT); //### Is it always this texture unit?
    if (id != GLuint(-1) && id == lastTextureUsed)
        return;

    lastTextureUsed = id;

    if (smoothPixmapTransform) {
        funcs.glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        funcs.glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    } else {
        funcs.glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        funcs.glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }
    funcs.glTexParameteri(target, GL_TEXTURE_WRAP_S, wrapMode);
    funcs.glTexParameteri(target, GL_TEXTURE_WRAP_T, wrapMode);
}


inline QColor qt_premultiplyColor(QColor c, GLfloat opacity)
{
    qreal alpha = c.alphaF() * opacity;
    c.setAlphaF(alpha);
    c.setRedF(c.redF() * alpha);
    c.setGreenF(c.greenF() * alpha);
    c.setBlueF(c.blueF() * alpha);
    return c;
}


void QOpenGL2PaintEngineExPrivate::setBrush(const QBrush& brush)
{
    if (qbrush_fast_equals(currentBrush, brush))
        return;

    const Qt::BrushStyle newStyle = qbrush_style(brush);
    Q_ASSERT(newStyle != Qt::NoBrush);

    currentBrush = brush;
    if (!currentBrushPixmap.isNull())
        currentBrushPixmap = QPixmap();
    brushUniformsDirty = true; // All brushes have at least one uniform

    if (newStyle > Qt::SolidPattern)
        brushTextureDirty = true;

    if (currentBrush.style() == Qt::TexturePattern
        && qHasPixmapTexture(brush) && brush.texture().isQBitmap())
    {
        shaderManager->setSrcPixelType(QOpenGLEngineShaderManager::TextureSrcWithPattern);
    } else {
        shaderManager->setSrcPixelType(newStyle);
    }
    shaderManager->optimiseForBrushTransform(currentBrush.transform().type());
}


void QOpenGL2PaintEngineExPrivate::useSimpleShader()
{
    shaderManager->useSimpleProgram();

    if (matrixDirty)
        updateMatrix();
}

void QOpenGL2PaintEngineExPrivate::updateBrushTexture()
{
    Q_Q(QOpenGL2PaintEngineEx);
//     qDebug("QOpenGL2PaintEngineExPrivate::updateBrushTexture()");
    Qt::BrushStyle style = currentBrush.style();

    if ( (style >= Qt::Dense1Pattern) && (style <= Qt::DiagCrossPattern) ) {
        // Get the image data for the pattern
        QImage texImage = qt_imageForBrush(style, false);

        funcs.glActiveTexture(GL_TEXTURE0 + QT_BRUSH_TEXTURE_UNIT);
        QOpenGLTextureCache::cacheForContext(ctx)->bindTexture(ctx, texImage);
        updateTextureFilter(GL_TEXTURE_2D, GL_REPEAT, q->state()->renderHints & QPainter::SmoothPixmapTransform);
    }
    else if (style >= Qt::LinearGradientPattern && style <= Qt::ConicalGradientPattern) {
        // Gradiant brush: All the gradiants use the same texture

        const QGradient* g = currentBrush.gradient();

        // We apply global opacity in the fragment shaders, so we always pass 1.0
        // for opacity to the cache.
        GLuint texId = QOpenGL2GradientCache::cacheForContext(ctx)->getBuffer(*g, 1.0);

        funcs.glActiveTexture(GL_TEXTURE0 + QT_BRUSH_TEXTURE_UNIT);
        funcs.glBindTexture(GL_TEXTURE_2D, texId);

        if (g->spread() == QGradient::RepeatSpread || g->type() == QGradient::ConicalGradient)
            updateTextureFilter(GL_TEXTURE_2D, GL_REPEAT, q->state()->renderHints & QPainter::SmoothPixmapTransform);
        else if (g->spread() == QGradient::ReflectSpread)
            updateTextureFilter(GL_TEXTURE_2D, GL_MIRRORED_REPEAT, q->state()->renderHints & QPainter::SmoothPixmapTransform);
        else
            updateTextureFilter(GL_TEXTURE_2D, GL_CLAMP_TO_EDGE, q->state()->renderHints & QPainter::SmoothPixmapTransform);
    }
    else if (style == Qt::TexturePattern) {
        currentBrushPixmap = currentBrush.texture();

        int max_texture_size = ctx->d_func()->maxTextureSize();
        if (currentBrushPixmap.width() > max_texture_size || currentBrushPixmap.height() > max_texture_size)
            currentBrushPixmap = currentBrushPixmap.scaled(max_texture_size, max_texture_size, Qt::KeepAspectRatio);

        GLuint wrapMode = GL_REPEAT;
        if (QOpenGLContext::currentContext()->isOpenGLES()) {
            // OpenGL ES does not support GL_REPEAT wrap modes for NPOT textures. So instead,
            // we emulate GL_REPEAT by only taking the fractional part of the texture coords
            // in the qopenglslTextureBrushSrcFragmentShader program.
            wrapMode = GL_CLAMP_TO_EDGE;
        }

        funcs.glActiveTexture(GL_TEXTURE0 + QT_BRUSH_TEXTURE_UNIT);
        QOpenGLTextureCache::cacheForContext(ctx)->bindTexture(ctx, currentBrushPixmap);
        updateTextureFilter(GL_TEXTURE_2D, wrapMode, q->state()->renderHints & QPainter::SmoothPixmapTransform);
        textureInvertedY = false;
    }
    brushTextureDirty = false;
}


void QOpenGL2PaintEngineExPrivate::updateBrushUniforms()
{
//     qDebug("QOpenGL2PaintEngineExPrivate::updateBrushUniforms()");
    Qt::BrushStyle style = currentBrush.style();

    if (style == Qt::NoBrush)
        return;

    QTransform brushQTransform = currentBrush.transform();

    if (style == Qt::SolidPattern) {
        QColor col = qt_premultiplyColor(currentBrush.color(), (GLfloat)q->state()->opacity);
        shaderManager->currentProgram()->setUniformValue(location(QOpenGLEngineShaderManager::FragmentColor), col);
    }
    else {
        // All other brushes have a transform and thus need the translation point:
        QPointF translationPoint;

        if (style <= Qt::DiagCrossPattern) {
            QColor col = qt_premultiplyColor(currentBrush.color(), (GLfloat)q->state()->opacity);

            shaderManager->currentProgram()->setUniformValue(location(QOpenGLEngineShaderManager::PatternColor), col);

            QVector2D halfViewportSize(width*0.5, height*0.5);
            shaderManager->currentProgram()->setUniformValue(location(QOpenGLEngineShaderManager::HalfViewportSize), halfViewportSize);
        }
        else if (style == Qt::LinearGradientPattern) {
            const QLinearGradient *g = static_cast<const QLinearGradient *>(currentBrush.gradient());

            QPointF realStart = g->start();
            QPointF realFinal = g->finalStop();
            translationPoint = realStart;

            QPointF l = realFinal - realStart;

            QVector3D linearData(
                l.x(),
                l.y(),
                1.0f / (l.x() * l.x() + l.y() * l.y())
            );

            shaderManager->currentProgram()->setUniformValue(location(QOpenGLEngineShaderManager::LinearData), linearData);

            QVector2D halfViewportSize(width*0.5, height*0.5);
            shaderManager->currentProgram()->setUniformValue(location(QOpenGLEngineShaderManager::HalfViewportSize), halfViewportSize);
        }
        else if (style == Qt::ConicalGradientPattern) {
            const QConicalGradient *g = static_cast<const QConicalGradient *>(currentBrush.gradient());
            translationPoint   = g->center();

            GLfloat angle = -(g->angle() * 2 * Q_PI) / 360.0;

            shaderManager->currentProgram()->setUniformValue(location(QOpenGLEngineShaderManager::Angle), angle);

            QVector2D halfViewportSize(width*0.5, height*0.5);
            shaderManager->currentProgram()->setUniformValue(location(QOpenGLEngineShaderManager::HalfViewportSize), halfViewportSize);
        }
        else if (style == Qt::RadialGradientPattern) {
            const QRadialGradient *g = static_cast<const QRadialGradient *>(currentBrush.gradient());
            QPointF realCenter = g->center();
            QPointF realFocal  = g->focalPoint();
            qreal   realRadius = g->centerRadius() - g->focalRadius();
            translationPoint   = realFocal;

            QPointF fmp = realCenter - realFocal;
            shaderManager->currentProgram()->setUniformValue(location(QOpenGLEngineShaderManager::Fmp), fmp);

            GLfloat fmp2_m_radius2 = -fmp.x() * fmp.x() - fmp.y() * fmp.y() + realRadius*realRadius;
            shaderManager->currentProgram()->setUniformValue(location(QOpenGLEngineShaderManager::Fmp2MRadius2), fmp2_m_radius2);
            shaderManager->currentProgram()->setUniformValue(location(QOpenGLEngineShaderManager::Inverse2Fmp2MRadius2),
                                                             GLfloat(1.0 / (2.0*fmp2_m_radius2)));
            shaderManager->currentProgram()->setUniformValue(location(QOpenGLEngineShaderManager::SqrFr),
                                                             GLfloat(g->focalRadius() * g->focalRadius()));
            shaderManager->currentProgram()->setUniformValue(location(QOpenGLEngineShaderManager::BRadius),
                                                             GLfloat(2 * (g->centerRadius() - g->focalRadius()) * g->focalRadius()),
                                                             g->focalRadius(),
                                                             g->centerRadius() - g->focalRadius());

            QVector2D halfViewportSize(width*0.5, height*0.5);
            shaderManager->currentProgram()->setUniformValue(location(QOpenGLEngineShaderManager::HalfViewportSize), halfViewportSize);
        }
        else if (style == Qt::TexturePattern) {
            const QPixmap& texPixmap = currentBrush.texture();

            if (qHasPixmapTexture(currentBrush) && currentBrush.texture().isQBitmap()) {
                QColor col = qt_premultiplyColor(currentBrush.color(), (GLfloat)q->state()->opacity);
                shaderManager->currentProgram()->setUniformValue(location(QOpenGLEngineShaderManager::PatternColor), col);
            }

            QSizeF invertedTextureSize(1.0 / texPixmap.width(), 1.0 / texPixmap.height());
            shaderManager->currentProgram()->setUniformValue(location(QOpenGLEngineShaderManager::InvertedTextureSize), invertedTextureSize);

            QVector2D halfViewportSize(width*0.5, height*0.5);
            shaderManager->currentProgram()->setUniformValue(location(QOpenGLEngineShaderManager::HalfViewportSize), halfViewportSize);
        }
        else
            qWarning("QOpenGL2PaintEngineEx: Unimplemented fill style");

        const QPointF &brushOrigin = q->state()->brushOrigin;
        QTransform matrix = q->state()->matrix;
        matrix.translate(brushOrigin.x(), brushOrigin.y());

        QTransform translate(1, 0, 0, 1, -translationPoint.x(), -translationPoint.y());
        qreal m22 = -1;
        qreal dy = height;
        if (device->paintFlipped()) {
            m22 = 1;
            dy = 0;
        }
        QTransform gl_to_qt(1, 0, 0, m22, 0, dy);
        QTransform inv_matrix;
        if (style == Qt::TexturePattern && textureInvertedY == -1)
            inv_matrix = gl_to_qt * (QTransform(1, 0, 0, -1, 0, currentBrush.texture().height()) * brushQTransform * matrix).inverted() * translate;
        else
            inv_matrix = gl_to_qt * (brushQTransform * matrix).inverted() * translate;

        shaderManager->currentProgram()->setUniformValue(location(QOpenGLEngineShaderManager::BrushTransform), inv_matrix);
        shaderManager->currentProgram()->setUniformValue(location(QOpenGLEngineShaderManager::BrushTexture), QT_BRUSH_TEXTURE_UNIT);
    }
    brushUniformsDirty = false;
}


// This assumes the shader manager has already setup the correct shader program
void QOpenGL2PaintEngineExPrivate::updateMatrix()
{
//     qDebug("QOpenGL2PaintEngineExPrivate::updateMatrix()");

    const QTransform& transform = q->state()->matrix;

    // The projection matrix converts from Qt's coordinate system to GL's coordinate system
    //    * GL's viewport is 2x2, Qt's is width x height
    //    * GL has +y -> -y going from bottom -> top, Qt is the other way round
    //    * GL has [0,0] in the center, Qt has it in the top-left
    //
    // This results in the Projection matrix below, which is multiplied by the painter's
    // transformation matrix, as shown below:
    //
    //                Projection Matrix                      Painter Transform
    // ------------------------------------------------   ------------------------
    // | 2.0 / width  |      0.0      |     -1.0      |   |  m11  |  m21  |  dx  |
    // |     0.0      | -2.0 / height |      1.0      | * |  m12  |  m22  |  dy  |
    // |     0.0      |      0.0      |      1.0      |   |  m13  |  m23  |  m33 |
    // ------------------------------------------------   ------------------------
    //
    // NOTE: The resultant matrix is also transposed, as GL expects column-major matracies

    const GLfloat wfactor = 2.0f / width;
    GLfloat hfactor = -2.0f / height;

    GLfloat dx = transform.dx();
    GLfloat dy = transform.dy();

    if (device->paintFlipped()) {
        hfactor *= -1;
        dy -= height;
    }

    // Non-integer translates can have strange effects for some rendering operations such as
    // anti-aliased text rendering. In such cases, we snap the translate to the pixel grid.
    if (snapToPixelGrid && transform.type() == QTransform::TxTranslate) {
        // 0.50 needs to rounded down to 0.0 for consistency with raster engine:
        dx = ceilf(dx - 0.5f);
        dy = ceilf(dy - 0.5f);
    }
    pmvMatrix[0][0] = (wfactor * transform.m11())  - transform.m13();
    pmvMatrix[1][0] = (wfactor * transform.m21())  - transform.m23();
    pmvMatrix[2][0] = (wfactor * dx) - transform.m33();
    pmvMatrix[0][1] = (hfactor * transform.m12())  + transform.m13();
    pmvMatrix[1][1] = (hfactor * transform.m22())  + transform.m23();
    pmvMatrix[2][1] = (hfactor * dy) + transform.m33();
    pmvMatrix[0][2] = transform.m13();
    pmvMatrix[1][2] = transform.m23();
    pmvMatrix[2][2] = transform.m33();

    // 1/10000 == 0.0001, so we have good enough res to cover curves
    // that span the entire widget...
    inverseScale = qMax(1 / qMax( qMax(qAbs(transform.m11()), qAbs(transform.m22())),
                                  qMax(qAbs(transform.m12()), qAbs(transform.m21())) ),
                        qreal(0.0001));

    matrixDirty = false;
    matrixUniformDirty = true;

    // Set the PMV matrix attribute. As we use an attributes rather than uniforms, we only
    // need to do this once for every matrix change and persists across all shader programs.
    funcs.glVertexAttrib3fv(QT_PMV_MATRIX_1_ATTR, pmvMatrix[0]);
    funcs.glVertexAttrib3fv(QT_PMV_MATRIX_2_ATTR, pmvMatrix[1]);
    funcs.glVertexAttrib3fv(QT_PMV_MATRIX_3_ATTR, pmvMatrix[2]);

    dasher.setInvScale(inverseScale);
    stroker.setInvScale(inverseScale);
}


void QOpenGL2PaintEngineExPrivate::updateCompositionMode()
{
    // NOTE: The entire paint engine works on pre-multiplied data - which is why some of these
    //       composition modes look odd.
//     qDebug() << "QOpenGL2PaintEngineExPrivate::updateCompositionMode() - Setting GL composition mode for " << q->state()->composition_mode;
    switch(q->state()->composition_mode) {
    case QPainter::CompositionMode_SourceOver:
        funcs.glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case QPainter::CompositionMode_DestinationOver:
        funcs.glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
        break;
    case QPainter::CompositionMode_Clear:
        funcs.glBlendFunc(GL_ZERO, GL_ZERO);
        break;
    case QPainter::CompositionMode_Source:
        funcs.glBlendFunc(GL_ONE, GL_ZERO);
        break;
    case QPainter::CompositionMode_Destination:
        funcs.glBlendFunc(GL_ZERO, GL_ONE);
        break;
    case QPainter::CompositionMode_SourceIn:
        funcs.glBlendFunc(GL_DST_ALPHA, GL_ZERO);
        break;
    case QPainter::CompositionMode_DestinationIn:
        funcs.glBlendFunc(GL_ZERO, GL_SRC_ALPHA);
        break;
    case QPainter::CompositionMode_SourceOut:
        funcs.glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ZERO);
        break;
    case QPainter::CompositionMode_DestinationOut:
        funcs.glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case QPainter::CompositionMode_SourceAtop:
        funcs.glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case QPainter::CompositionMode_DestinationAtop:
        funcs.glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_SRC_ALPHA);
        break;
    case QPainter::CompositionMode_Xor:
        funcs.glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case QPainter::CompositionMode_Plus:
        funcs.glBlendFunc(GL_ONE, GL_ONE);
        break;
    default:
        qWarning("Unsupported composition mode");
        break;
    }

    compositionModeDirty = false;
}

static inline void setCoords(GLfloat *coords, const QOpenGLRect &rect)
{
    coords[0] = rect.left;
    coords[1] = rect.top;
    coords[2] = rect.right;
    coords[3] = rect.top;
    coords[4] = rect.right;
    coords[5] = rect.bottom;
    coords[6] = rect.left;
    coords[7] = rect.bottom;
}

void QOpenGL2PaintEngineExPrivate::drawTexture(const QOpenGLRect& dest, const QOpenGLRect& src, const QSize &textureSize, bool opaque, bool pattern)
{
    // Setup for texture drawing
    currentBrush = noBrush;
    shaderManager->setSrcPixelType(pattern ? QOpenGLEngineShaderManager::PatternSrc : QOpenGLEngineShaderManager::ImageSrc);

    if (snapToPixelGrid) {
        snapToPixelGrid = false;
        matrixDirty = true;
    }

    if (prepareForDraw(opaque))
        shaderManager->currentProgram()->setUniformValue(location(QOpenGLEngineShaderManager::ImageTexture), QT_IMAGE_TEXTURE_UNIT);

    if (pattern) {
        QColor col = qt_premultiplyColor(q->state()->pen.color(), (GLfloat)q->state()->opacity);
        shaderManager->currentProgram()->setUniformValue(location(QOpenGLEngineShaderManager::PatternColor), col);
    }

    GLfloat dx = 1.0 / textureSize.width();
    GLfloat dy = 1.0 / textureSize.height();

    QOpenGLRect srcTextureRect(src.left*dx, src.top*dy, src.right*dx, src.bottom*dy);

    setCoords(staticVertexCoordinateArray, dest);
    setCoords(staticTextureCoordinateArray, srcTextureRect);

    funcs.glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void QOpenGL2PaintEngineEx::beginNativePainting()
{
    Q_D(QOpenGL2PaintEngineEx);
    ensureActive();
    d->transferMode(BrushDrawingMode);

    d->nativePaintingActive = true;

    d->funcs.glUseProgram(0);

    // Disable all the vertex attribute arrays:
    for (int i = 0; i < QT_GL_VERTEX_ARRAY_TRACKED_COUNT; ++i)
        d->funcs.glDisableVertexAttribArray(i);

#if !defined(QT_OPENGL_ES_2) && !defined(QT_OPENGL_DYNAMIC)
    Q_ASSERT(QOpenGLContext::currentContext());
    const QOpenGLContext *ctx = d->ctx;
    const QSurfaceFormat &fmt = d->device->context()->format();
    if (fmt.majorVersion() < 3 || (fmt.majorVersion() == 3 && fmt.minorVersion() < 1)
        || (fmt.majorVersion() == 3 && fmt.minorVersion() == 1 && ctx->hasExtension(QByteArrayLiteral("GL_ARB_compatibility")))
        || fmt.profile() == QSurfaceFormat::CompatibilityProfile)
    {
        // be nice to people who mix OpenGL 1.x code with QPainter commands
        // by setting modelview and projection matrices to mirror the GL 1
        // paint engine
        const QTransform& mtx = state()->matrix;

        float mv_matrix[4][4] =
            {
                { float(mtx.m11()), float(mtx.m12()),     0, float(mtx.m13()) },
                { float(mtx.m21()), float(mtx.m22()),     0, float(mtx.m23()) },
                {                0,                0,     1,                0 },
                {  float(mtx.dx()),  float(mtx.dy()),     0, float(mtx.m33()) }
            };

        const QSize sz = d->device->size();

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, sz.width(), sz.height(), 0, -999999, 999999);

        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(&mv_matrix[0][0]);
    }
#endif // QT_OPENGL_ES_2

    d->lastTextureUsed = GLuint(-1);
    d->dirtyStencilRegion = QRect(0, 0, d->width, d->height);
    d->resetGLState();

    d->shaderManager->setDirty();

    d->needsSync = true;
}

void QOpenGL2PaintEngineExPrivate::resetGLState()
{
    funcs.glDisable(GL_BLEND);
    funcs.glActiveTexture(GL_TEXTURE0);
    funcs.glDisable(GL_STENCIL_TEST);
    funcs.glDisable(GL_DEPTH_TEST);
    funcs.glDisable(GL_SCISSOR_TEST);
    funcs.glDepthMask(true);
    funcs.glDepthFunc(GL_LESS);
    funcs.glClearDepthf(1);
    funcs.glStencilMask(0xff);
    funcs.glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    funcs.glStencilFunc(GL_ALWAYS, 0, 0xff);
    setVertexAttribArrayEnabled(QT_TEXTURE_COORDS_ATTR, false);
    setVertexAttribArrayEnabled(QT_VERTEX_COORDS_ATTR, false);
    setVertexAttribArrayEnabled(QT_OPACITY_ATTR, false);
    if (!QOpenGLContext::currentContext()->isOpenGLES()) {
        // gl_Color, corresponding to vertex attribute 3, may have been changed
        float color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        funcs.glVertexAttrib4fv(3, color);
    }
}

void QOpenGL2PaintEngineEx::endNativePainting()
{
    Q_D(QOpenGL2PaintEngineEx);
    d->needsSync = true;
    d->nativePaintingActive = false;
}

void QOpenGL2PaintEngineEx::invalidateState()
{
    Q_D(QOpenGL2PaintEngineEx);
    d->needsSync = true;
}

bool QOpenGL2PaintEngineEx::isNativePaintingActive() const {
    Q_D(const QOpenGL2PaintEngineEx);
    return d->nativePaintingActive;
}

void QOpenGL2PaintEngineExPrivate::transferMode(EngineMode newMode)
{
    if (newMode == mode)
        return;

    if (mode != BrushDrawingMode) {
        lastTextureUsed = GLuint(-1);
    }

    if (newMode == TextDrawingMode) {
        shaderManager->setHasComplexGeometry(true);
    } else {
        shaderManager->setHasComplexGeometry(false);
    }

    if (newMode == ImageDrawingMode) {
        setVertexAttributePointer(QT_VERTEX_COORDS_ATTR, staticVertexCoordinateArray);
        setVertexAttributePointer(QT_TEXTURE_COORDS_ATTR, staticTextureCoordinateArray);
    }

    if (newMode == ImageArrayDrawingMode || newMode == ImageOpacityArrayDrawingMode) {
        setVertexAttributePointer(QT_VERTEX_COORDS_ATTR, (GLfloat*)vertexCoordinateArray.data());
        setVertexAttributePointer(QT_TEXTURE_COORDS_ATTR, (GLfloat*)textureCoordinateArray.data());

        if (newMode == ImageOpacityArrayDrawingMode)
            setVertexAttributePointer(QT_OPACITY_ATTR, (GLfloat*)opacityArray.data());
    }

    // This needs to change when we implement high-quality anti-aliasing...
    if (newMode != TextDrawingMode)
        shaderManager->setMaskType(QOpenGLEngineShaderManager::NoMask);

    mode = newMode;
}

struct QOpenGL2PEVectorPathCache
{
#ifdef QT_OPENGL_CACHE_AS_VBOS
    GLuint vbo;
    GLuint ibo;
#else
    float *vertices;
    void *indices;
#endif
    int vertexCount;
    int indexCount;
    GLenum primitiveType;
    qreal iscale;
    QVertexIndexVector::Type indexType;
};

void QOpenGL2PaintEngineExPrivate::cleanupVectorPath(QPaintEngineEx *engine, void *data)
{
    QOpenGL2PEVectorPathCache *c = (QOpenGL2PEVectorPathCache *) data;
#ifdef QT_OPENGL_CACHE_AS_VBOS
    Q_ASSERT(engine->type() == QPaintEngine::OpenGL2);
    static_cast<QOpenGL2PaintEngineEx *>(engine)->d_func()->unusedVBOSToClean << c->vbo;
    if (c->ibo)
        d->unusedIBOSToClean << c->ibo;
#else
    Q_UNUSED(engine);
    free(c->vertices);
    free(c->indices);
#endif
    delete c;
}

// Assumes everything is configured for the brush you want to use
void QOpenGL2PaintEngineExPrivate::fill(const QVectorPath& path)
{
    transferMode(BrushDrawingMode);

    if (snapToPixelGrid) {
        snapToPixelGrid = false;
        matrixDirty = true;
    }

    // Might need to call updateMatrix to re-calculate inverseScale
    if (matrixDirty)
        updateMatrix();

    const QPointF* const points = reinterpret_cast<const QPointF*>(path.points());

    // Check to see if there's any hints
    if (path.shape() == QVectorPath::RectangleHint) {
        QOpenGLRect rect(points[0].x(), points[0].y(), points[2].x(), points[2].y());
        prepareForDraw(currentBrush.isOpaque());
        composite(rect);
    } else if (path.isConvex()) {

        if (path.isCacheable()) {
            QVectorPath::CacheEntry *data = path.lookupCacheData(q);
            QOpenGL2PEVectorPathCache *cache;

            bool updateCache = false;

            if (data) {
                cache = (QOpenGL2PEVectorPathCache *) data->data;
                // Check if scale factor is exceeded for curved paths and generate curves if so...
                if (path.isCurved()) {
                    qreal scaleFactor = cache->iscale / inverseScale;
                    if (scaleFactor < 0.5 || scaleFactor > 2.0) {
#ifdef QT_OPENGL_CACHE_AS_VBOS
                        glDeleteBuffers(1, &cache->vbo);
                        cache->vbo = 0;
                        Q_ASSERT(cache->ibo == 0);
#else
                        free(cache->vertices);
                        Q_ASSERT(cache->indices == 0);
#endif
                        updateCache = true;
                    }
                }
            } else {
                cache = new QOpenGL2PEVectorPathCache;
                data = const_cast<QVectorPath &>(path).addCacheData(q, cache, cleanupVectorPath);
                updateCache = true;
            }

            // Flatten the path at the current scale factor and fill it into the cache struct.
            if (updateCache) {
                vertexCoordinateArray.clear();
                vertexCoordinateArray.addPath(path, inverseScale, false);
                int vertexCount = vertexCoordinateArray.vertexCount();
                int floatSizeInBytes = vertexCount * 2 * sizeof(float);
                cache->vertexCount = vertexCount;
                cache->indexCount = 0;
                cache->primitiveType = GL_TRIANGLE_FAN;
                cache->iscale = inverseScale;
#ifdef QT_OPENGL_CACHE_AS_VBOS
                funcs.glGenBuffers(1, &cache->vbo);
                funcs.glBindBuffer(GL_ARRAY_BUFFER, cache->vbo);
                funcs.glBufferData(GL_ARRAY_BUFFER, floatSizeInBytes, vertexCoordinateArray.data(), GL_STATIC_DRAW);
                cache->ibo = 0;
#else
                cache->vertices = (float *) malloc(floatSizeInBytes);
                memcpy(cache->vertices, vertexCoordinateArray.data(), floatSizeInBytes);
                cache->indices = 0;
#endif
            }

            prepareForDraw(currentBrush.isOpaque());
#ifdef QT_OPENGL_CACHE_AS_VBOS
            funcs.glBindBuffer(GL_ARRAY_BUFFER, cache->vbo);
            setVertexAttributePointer(QT_VERTEX_COORDS_ATTR, 0);
#else
            setVertexAttributePointer(QT_VERTEX_COORDS_ATTR, cache->vertices);
#endif
            funcs.glDrawArrays(cache->primitiveType, 0, cache->vertexCount);

        } else {
      //        printf(" - Marking path as cachable...\n");
            // Tag it for later so that if the same path is drawn twice, it is assumed to be static and thus cachable
            path.makeCacheable();
            vertexCoordinateArray.clear();
            vertexCoordinateArray.addPath(path, inverseScale, false);
            prepareForDraw(currentBrush.isOpaque());
            drawVertexArrays(vertexCoordinateArray, GL_TRIANGLE_FAN);
        }

    } else {
        bool useCache = path.isCacheable();
        if (useCache) {
            QRectF bbox = path.controlPointRect();
            // If the path doesn't fit within these limits, it is possible that the triangulation will fail.
            useCache &= (bbox.left() > -0x8000 * inverseScale)
                     && (bbox.right() < 0x8000 * inverseScale)
                     && (bbox.top() > -0x8000 * inverseScale)
                     && (bbox.bottom() < 0x8000 * inverseScale);
        }

        if (useCache) {
            QVectorPath::CacheEntry *data = path.lookupCacheData(q);
            QOpenGL2PEVectorPathCache *cache;

            bool updateCache = false;

            if (data) {
                cache = (QOpenGL2PEVectorPathCache *) data->data;
                // Check if scale factor is exceeded for curved paths and generate curves if so...
                if (path.isCurved()) {
                    qreal scaleFactor = cache->iscale / inverseScale;
                    if (scaleFactor < 0.5 || scaleFactor > 2.0) {
#ifdef QT_OPENGL_CACHE_AS_VBOS
                        glDeleteBuffers(1, &cache->vbo);
                        glDeleteBuffers(1, &cache->ibo);
#else
                        free(cache->vertices);
                        free(cache->indices);
#endif
                        updateCache = true;
                    }
                }
            } else {
                cache = new QOpenGL2PEVectorPathCache;
                data = const_cast<QVectorPath &>(path).addCacheData(q, cache, cleanupVectorPath);
                updateCache = true;
            }

            // Flatten the path at the current scale factor and fill it into the cache struct.
            if (updateCache) {
                QTriangleSet polys = qTriangulate(path, QTransform().scale(1 / inverseScale, 1 / inverseScale));
                cache->vertexCount = polys.vertices.size() / 2;
                cache->indexCount = polys.indices.size();
                cache->primitiveType = GL_TRIANGLES;
                cache->iscale = inverseScale;
                cache->indexType = polys.indices.type();
#ifdef QT_OPENGL_CACHE_AS_VBOS
                funcs.glGenBuffers(1, &cache->vbo);
                funcs.glGenBuffers(1, &cache->ibo);
                funcs.glBindBuffer(GL_ARRAY_BUFFER, cache->vbo);
                funcs.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cache->ibo);

                if (polys.indices.type() == QVertexIndexVector::UnsignedInt)
                    funcs.glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quint32) * polys.indices.size(), polys.indices.data(), GL_STATIC_DRAW);
                else
                    funcs.glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quint16) * polys.indices.size(), polys.indices.data(), GL_STATIC_DRAW);

                QVarLengthArray<float> vertices(polys.vertices.size());
                for (int i = 0; i < polys.vertices.size(); ++i)
                    vertices[i] = float(inverseScale * polys.vertices.at(i));
                funcs.glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
#else
                cache->vertices = (float *) malloc(sizeof(float) * polys.vertices.size());
                if (polys.indices.type() == QVertexIndexVector::UnsignedInt) {
                    cache->indices = (quint32 *) malloc(sizeof(quint32) * polys.indices.size());
                    memcpy(cache->indices, polys.indices.data(), sizeof(quint32) * polys.indices.size());
                } else {
                    cache->indices = (quint16 *) malloc(sizeof(quint16) * polys.indices.size());
                    memcpy(cache->indices, polys.indices.data(), sizeof(quint16) * polys.indices.size());
                }
                for (int i = 0; i < polys.vertices.size(); ++i)
                    cache->vertices[i] = float(inverseScale * polys.vertices.at(i));
#endif
            }

            prepareForDraw(currentBrush.isOpaque());
#ifdef QT_OPENGL_CACHE_AS_VBOS
            funcs.glBindBuffer(GL_ARRAY_BUFFER, cache->vbo);
            funcs.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cache->ibo);
            setVertexAttributePointer(QT_VERTEX_COORDS_ATTR, 0);
            if (cache->indexType == QVertexIndexVector::UnsignedInt)
                funcs.glDrawElements(cache->primitiveType, cache->indexCount, GL_UNSIGNED_INT, 0);
            else
                funcs.glDrawElements(cache->primitiveType, cache->indexCount, GL_UNSIGNED_SHORT, 0);
            funcs.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            funcs.glBindBuffer(GL_ARRAY_BUFFER, 0);
#else
            setVertexAttributePointer(QT_VERTEX_COORDS_ATTR, cache->vertices);
            if (cache->indexType == QVertexIndexVector::UnsignedInt)
                funcs.glDrawElements(cache->primitiveType, cache->indexCount, GL_UNSIGNED_INT, (qint32 *)cache->indices);
            else
                funcs.glDrawElements(cache->primitiveType, cache->indexCount, GL_UNSIGNED_SHORT, (qint16 *)cache->indices);
#endif

        } else {
      //        printf(" - Marking path as cachable...\n");
            // Tag it for later so that if the same path is drawn twice, it is assumed to be static and thus cachable
            path.makeCacheable();

            if (device->context()->format().stencilBufferSize() <= 0) {
                // If there is no stencil buffer, triangulate the path instead.

                QRectF bbox = path.controlPointRect();
                // If the path doesn't fit within these limits, it is possible that the triangulation will fail.
                bool withinLimits = (bbox.left() > -0x8000 * inverseScale)
                                  && (bbox.right() < 0x8000 * inverseScale)
                                  && (bbox.top() > -0x8000 * inverseScale)
                                  && (bbox.bottom() < 0x8000 * inverseScale);
                if (withinLimits) {
                    QTriangleSet polys = qTriangulate(path, QTransform().scale(1 / inverseScale, 1 / inverseScale));

                    QVarLengthArray<float> vertices(polys.vertices.size());
                    for (int i = 0; i < polys.vertices.size(); ++i)
                        vertices[i] = float(inverseScale * polys.vertices.at(i));

                    prepareForDraw(currentBrush.isOpaque());
                    setVertexAttributePointer(QT_VERTEX_COORDS_ATTR, vertices.constData());
                    if (funcs.hasOpenGLExtension(QOpenGLExtensions::ElementIndexUint))
                        funcs.glDrawElements(GL_TRIANGLES, polys.indices.size(), GL_UNSIGNED_INT, polys.indices.data());
                    else
                        funcs.glDrawElements(GL_TRIANGLES, polys.indices.size(), GL_UNSIGNED_SHORT, polys.indices.data());
                } else {
                    // We can't handle big, concave painter paths with OpenGL without stencil buffer.
                    qWarning("Painter path exceeds +/-32767 pixels.");
                }
                return;
            }

            // The path is too complicated & needs the stencil technique
            vertexCoordinateArray.clear();
            vertexCoordinateArray.addPath(path, inverseScale, false);

            fillStencilWithVertexArray(vertexCoordinateArray, path.hasWindingFill());

            funcs.glStencilMask(0xff);
            funcs.glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);

            if (q->state()->clipTestEnabled) {
                // Pass when high bit is set, replace stencil value with current clip
                funcs.glStencilFunc(GL_NOTEQUAL, q->state()->currentClip, GL_STENCIL_HIGH_BIT);
            } else if (path.hasWindingFill()) {
                // Pass when any bit is set, replace stencil value with 0
                funcs.glStencilFunc(GL_NOTEQUAL, 0, 0xff);
            } else {
                // Pass when high bit is set, replace stencil value with 0
                funcs.glStencilFunc(GL_NOTEQUAL, 0, GL_STENCIL_HIGH_BIT);
            }
            prepareForDraw(currentBrush.isOpaque());

            // Stencil the brush onto the dest buffer
            composite(vertexCoordinateArray.boundingRect());
            funcs.glStencilMask(0);
            updateClipScissorTest();
        }
    }
}


void QOpenGL2PaintEngineExPrivate::fillStencilWithVertexArray(const float *data,
                                                          int count,
                                                          int *stops,
                                                          int stopCount,
                                                          const QOpenGLRect &bounds,
                                                          StencilFillMode mode)
{
    Q_ASSERT(count || stops);

//     qDebug("QOpenGL2PaintEngineExPrivate::fillStencilWithVertexArray()");
    funcs.glStencilMask(0xff); // Enable stencil writes

    if (dirtyStencilRegion.intersects(currentScissorBounds)) {
        QVector<QRect> clearRegion = dirtyStencilRegion.intersected(currentScissorBounds).rects();
        funcs.glClearStencil(0); // Clear to zero
        for (int i = 0; i < clearRegion.size(); ++i) {
#ifndef QT_GL_NO_SCISSOR_TEST
            setScissor(clearRegion.at(i));
#endif
            funcs.glClear(GL_STENCIL_BUFFER_BIT);
        }

        dirtyStencilRegion -= currentScissorBounds;

#ifndef QT_GL_NO_SCISSOR_TEST
        updateClipScissorTest();
#endif
    }

    funcs.glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // Disable color writes
    useSimpleShader();
    funcs.glEnable(GL_STENCIL_TEST); // For some reason, this has to happen _after_ the simple shader is use()'d

    if (mode == WindingFillMode) {
        Q_ASSERT(stops && !count);
        if (q->state()->clipTestEnabled) {
            // Flatten clip values higher than current clip, and set high bit to match current clip
            funcs.glStencilFunc(GL_LEQUAL, GL_STENCIL_HIGH_BIT | q->state()->currentClip, ~GL_STENCIL_HIGH_BIT);
            funcs.glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
            composite(bounds);

            funcs.glStencilFunc(GL_EQUAL, GL_STENCIL_HIGH_BIT, GL_STENCIL_HIGH_BIT);
        } else if (!stencilClean) {
            // Clear stencil buffer within bounding rect
            funcs.glStencilFunc(GL_ALWAYS, 0, 0xff);
            funcs.glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO);
            composite(bounds);
        }

        // Inc. for front-facing triangle
        funcs.glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_INCR_WRAP, GL_INCR_WRAP);
        // Dec. for back-facing "holes"
        funcs.glStencilOpSeparate(GL_BACK, GL_KEEP, GL_DECR_WRAP, GL_DECR_WRAP);
        funcs.glStencilMask(~GL_STENCIL_HIGH_BIT);
        drawVertexArrays(data, stops, stopCount, GL_TRIANGLE_FAN);

        if (q->state()->clipTestEnabled) {
            // Clear high bit of stencil outside of path
            funcs.glStencilFunc(GL_EQUAL, q->state()->currentClip, ~GL_STENCIL_HIGH_BIT);
            funcs.glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
            funcs.glStencilMask(GL_STENCIL_HIGH_BIT);
            composite(bounds);
        }
    } else if (mode == OddEvenFillMode) {
        funcs.glStencilMask(GL_STENCIL_HIGH_BIT);
        funcs.glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT); // Simply invert the stencil bit
        drawVertexArrays(data, stops, stopCount, GL_TRIANGLE_FAN);

    } else { // TriStripStrokeFillMode
        Q_ASSERT(count && !stops); // tristrips generated directly, so no vertexArray or stops
        funcs.glStencilMask(GL_STENCIL_HIGH_BIT);
#if 0
        funcs.glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT); // Simply invert the stencil bit
        setVertexAttributePointer(QT_VERTEX_COORDS_ATTR, data);
        funcs.glDrawArrays(GL_TRIANGLE_STRIP, 0, count);
#else

        funcs.glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        if (q->state()->clipTestEnabled) {
            funcs.glStencilFunc(GL_LEQUAL, q->state()->currentClip | GL_STENCIL_HIGH_BIT,
                                ~GL_STENCIL_HIGH_BIT);
        } else {
            funcs.glStencilFunc(GL_ALWAYS, GL_STENCIL_HIGH_BIT, 0xff);
        }
        setVertexAttributePointer(QT_VERTEX_COORDS_ATTR, data);
        funcs.glDrawArrays(GL_TRIANGLE_STRIP, 0, count);
#endif
    }

    // Enable color writes & disable stencil writes
    funcs.glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

/*
    If the maximum value in the stencil buffer is GL_STENCIL_HIGH_BIT - 1,
    restore the stencil buffer to a pristine state.  The current clip region
    is set to 1, and the rest to 0.
*/
void QOpenGL2PaintEngineExPrivate::resetClipIfNeeded()
{
    if (maxClip != (GL_STENCIL_HIGH_BIT - 1))
        return;

    Q_Q(QOpenGL2PaintEngineEx);

    useSimpleShader();
    funcs.glEnable(GL_STENCIL_TEST);
    funcs.glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    QRectF bounds = q->state()->matrix.inverted().mapRect(QRectF(0, 0, width, height));
    QOpenGLRect rect(bounds.left(), bounds.top(), bounds.right(), bounds.bottom());

    // Set high bit on clip region
    funcs.glStencilFunc(GL_LEQUAL, q->state()->currentClip, 0xff);
    funcs.glStencilOp(GL_KEEP, GL_INVERT, GL_INVERT);
    funcs.glStencilMask(GL_STENCIL_HIGH_BIT);
    composite(rect);

    // Reset clipping to 1 and everything else to zero
    funcs.glStencilFunc(GL_NOTEQUAL, 0x01, GL_STENCIL_HIGH_BIT);
    funcs.glStencilOp(GL_ZERO, GL_REPLACE, GL_REPLACE);
    funcs.glStencilMask(0xff);
    composite(rect);

    q->state()->currentClip = 1;
    q->state()->canRestoreClip = false;

    maxClip = 1;

    funcs.glStencilMask(0x0);
    funcs.glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

bool QOpenGL2PaintEngineExPrivate::prepareForCachedGlyphDraw(const QFontEngineGlyphCache &cache)
{
    Q_Q(QOpenGL2PaintEngineEx);

    Q_ASSERT(cache.transform().type() <= QTransform::TxScale);

    QTransform &transform = q->state()->matrix;
    transform.scale(1.0 / cache.transform().m11(), 1.0 / cache.transform().m22());
    bool ret = prepareForDraw(false);
    transform.scale(cache.transform().m11(), cache.transform().m22());

    return ret;
}

bool QOpenGL2PaintEngineExPrivate::prepareForDraw(bool srcPixelsAreOpaque)
{
    if (brushTextureDirty && (mode == TextDrawingMode || mode == BrushDrawingMode))
        updateBrushTexture();

    if (compositionModeDirty)
        updateCompositionMode();

    if (matrixDirty)
        updateMatrix();

    const bool stateHasOpacity = q->state()->opacity < 0.99f;
    if (q->state()->composition_mode == QPainter::CompositionMode_Source
        || (q->state()->composition_mode == QPainter::CompositionMode_SourceOver
            && srcPixelsAreOpaque && !stateHasOpacity))
    {
        funcs.glDisable(GL_BLEND);
    } else {
        funcs.glEnable(GL_BLEND);
    }

    QOpenGLEngineShaderManager::OpacityMode opacityMode;
    if (mode == ImageOpacityArrayDrawingMode) {
        opacityMode = QOpenGLEngineShaderManager::AttributeOpacity;
    } else {
        opacityMode = stateHasOpacity ? QOpenGLEngineShaderManager::UniformOpacity
                                      : QOpenGLEngineShaderManager::NoOpacity;
        if (stateHasOpacity && (mode != ImageDrawingMode && mode != ImageArrayDrawingMode)) {
            // Using a brush
            bool brushIsPattern = (currentBrush.style() >= Qt::Dense1Pattern) &&
                                  (currentBrush.style() <= Qt::DiagCrossPattern);

            if ((currentBrush.style() == Qt::SolidPattern) || brushIsPattern)
                opacityMode = QOpenGLEngineShaderManager::NoOpacity; // Global opacity handled by srcPixel shader
        }
    }
    shaderManager->setOpacityMode(opacityMode);

    bool changed = shaderManager->useCorrectShaderProg();
    // If the shader program needs changing, we change it and mark all uniforms as dirty
    if (changed) {
        // The shader program has changed so mark all uniforms as dirty:
        brushUniformsDirty = true;
        opacityUniformDirty = true;
        matrixUniformDirty = true;
    }

    if (brushUniformsDirty && (mode == TextDrawingMode || mode == BrushDrawingMode))
        updateBrushUniforms();

    if (opacityMode == QOpenGLEngineShaderManager::UniformOpacity && opacityUniformDirty) {
        shaderManager->currentProgram()->setUniformValue(location(QOpenGLEngineShaderManager::GlobalOpacity), (GLfloat)q->state()->opacity);
        opacityUniformDirty = false;
    }

    if (matrixUniformDirty && shaderManager->hasComplexGeometry()) {
        shaderManager->currentProgram()->setUniformValue(location(QOpenGLEngineShaderManager::Matrix),
                                                         pmvMatrix);
        matrixUniformDirty = false;
    }

    return changed;
}

void QOpenGL2PaintEngineExPrivate::composite(const QOpenGLRect& boundingRect)
{
    setCoords(staticVertexCoordinateArray, boundingRect);
    setVertexAttributePointer(QT_VERTEX_COORDS_ATTR, staticVertexCoordinateArray);
    funcs.glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

// Draws the vertex array as a set of <vertexArrayStops.size()> triangle fans.
void QOpenGL2PaintEngineExPrivate::drawVertexArrays(const float *data, int *stops, int stopCount,
                                                GLenum primitive)
{
    // Now setup the pointer to the vertex array:
    setVertexAttributePointer(QT_VERTEX_COORDS_ATTR, (GLfloat*)data);

    int previousStop = 0;
    for (int i=0; i<stopCount; ++i) {
        int stop = stops[i];
/*
        qDebug("Drawing triangle fan for vertecies %d -> %d:", previousStop, stop-1);
        for (int i=previousStop; i<stop; ++i)
            qDebug("   %02d: [%.2f, %.2f]", i, vertexArray.data()[i].x, vertexArray.data()[i].y);
*/
        funcs.glDrawArrays(primitive, previousStop, stop - previousStop);
        previousStop = stop;
    }
}

/////////////////////////////////// Public Methods //////////////////////////////////////////

QOpenGL2PaintEngineEx::QOpenGL2PaintEngineEx()
    : QPaintEngineEx(*(new QOpenGL2PaintEngineExPrivate(this)))
{
}

QOpenGL2PaintEngineEx::~QOpenGL2PaintEngineEx()
{
}

void QOpenGL2PaintEngineEx::fill(const QVectorPath &path, const QBrush &brush)
{
    Q_D(QOpenGL2PaintEngineEx);

    if (qbrush_style(brush) == Qt::NoBrush)
        return;
    ensureActive();
    d->setBrush(brush);
    d->fill(path);
}

Q_GUI_EXPORT bool qt_scaleForTransform(const QTransform &transform, qreal *scale); // qtransform.cpp


void QOpenGL2PaintEngineEx::stroke(const QVectorPath &path, const QPen &pen)
{
    Q_D(QOpenGL2PaintEngineEx);

    const QBrush &penBrush = qpen_brush(pen);
    if (qpen_style(pen) == Qt::NoPen || qbrush_style(penBrush) == Qt::NoBrush)
        return;

    QOpenGL2PaintEngineState *s = state();
    if (qt_pen_is_cosmetic(pen, state()->renderHints) && !qt_scaleForTransform(s->transform(), 0)) {
        // QTriangulatingStroker class is not meant to support cosmetically sheared strokes.
        QPaintEngineEx::stroke(path, pen);
        return;
    }

    ensureActive();
    d->setBrush(penBrush);
    d->stroke(path, pen);
}

void QOpenGL2PaintEngineExPrivate::stroke(const QVectorPath &path, const QPen &pen)
{
    const QOpenGL2PaintEngineState *s = q->state();
    if (snapToPixelGrid) {
        snapToPixelGrid = false;
        matrixDirty = true;
    }

    const Qt::PenStyle penStyle = qpen_style(pen);
    const QBrush &penBrush = qpen_brush(pen);
    const bool opaque = penBrush.isOpaque() && s->opacity > 0.99;

    transferMode(BrushDrawingMode);

    // updateMatrix() is responsible for setting the inverse scale on
    // the strokers, so we need to call it here and not wait for
    // prepareForDraw() down below.
    updateMatrix();

    QRectF clip = q->state()->matrix.inverted().mapRect(q->state()->clipEnabled
                                                        ? q->state()->rectangleClip
                                                        : QRectF(0, 0, width, height));

    if (penStyle == Qt::SolidLine) {
        stroker.process(path, pen, clip, s->renderHints);

    } else { // Some sort of dash
        dasher.process(path, pen, clip, s->renderHints);

        QVectorPath dashStroke(dasher.points(),
                               dasher.elementCount(),
                               dasher.elementTypes(),
                               s->renderHints);
        stroker.process(dashStroke, pen, clip, s->renderHints);
    }

    if (!stroker.vertexCount())
        return;

    if (opaque) {
        prepareForDraw(opaque);
        setVertexAttributePointer(QT_VERTEX_COORDS_ATTR, stroker.vertices());
        funcs.glDrawArrays(GL_TRIANGLE_STRIP, 0, stroker.vertexCount() / 2);

//         QBrush b(Qt::green);
//         d->setBrush(&b);
//         d->prepareForDraw(true);
//         glDrawArrays(GL_LINE_STRIP, 0, d->stroker.vertexCount() / 2);

    } else {
        qreal width = qpen_widthf(pen) / 2;
        if (width == 0)
            width = 0.5;
        qreal extra = pen.joinStyle() == Qt::MiterJoin
                      ? qMax(pen.miterLimit() * width, width)
                      : width;

        if (qt_pen_is_cosmetic(pen, q->state()->renderHints))
            extra = extra * inverseScale;

        QRectF bounds = path.controlPointRect().adjusted(-extra, -extra, extra, extra);

        fillStencilWithVertexArray(stroker.vertices(), stroker.vertexCount() / 2,
                                      0, 0, bounds, QOpenGL2PaintEngineExPrivate::TriStripStrokeFillMode);

        funcs.glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);

        // Pass when any bit is set, replace stencil value with 0
        funcs.glStencilFunc(GL_NOTEQUAL, 0, GL_STENCIL_HIGH_BIT);
        prepareForDraw(false);

        // Stencil the brush onto the dest buffer
        composite(bounds);

        funcs.glStencilMask(0);

        updateClipScissorTest();
    }
}

void QOpenGL2PaintEngineEx::penChanged() { }
void QOpenGL2PaintEngineEx::brushChanged() { }
void QOpenGL2PaintEngineEx::brushOriginChanged() { }

void QOpenGL2PaintEngineEx::opacityChanged()
{
//    qDebug("QOpenGL2PaintEngineEx::opacityChanged()");
    Q_D(QOpenGL2PaintEngineEx);
    state()->opacityChanged = true;

    Q_ASSERT(d->shaderManager);
    d->brushUniformsDirty = true;
    d->opacityUniformDirty = true;
}

void QOpenGL2PaintEngineEx::compositionModeChanged()
{
//     qDebug("QOpenGL2PaintEngineEx::compositionModeChanged()");
    Q_D(QOpenGL2PaintEngineEx);
    state()->compositionModeChanged = true;
    d->compositionModeDirty = true;
}

void QOpenGL2PaintEngineEx::renderHintsChanged()
{
    state()->renderHintsChanged = true;

#ifndef QT_OPENGL_ES_2
    if (!QOpenGLContext::currentContext()->isOpenGLES()) {
        Q_D(QOpenGL2PaintEngineEx);
        if ((state()->renderHints & QPainter::Antialiasing)
            || (state()->renderHints & QPainter::HighQualityAntialiasing))
            d->funcs.glEnable(GL_MULTISAMPLE);
        else
            d->funcs.glDisable(GL_MULTISAMPLE);
    }
#endif // QT_OPENGL_ES_2

    Q_D(QOpenGL2PaintEngineEx);
    d->lastTextureUsed = GLuint(-1);
    d->brushTextureDirty = true;
//    qDebug("QOpenGL2PaintEngineEx::renderHintsChanged() not implemented!");
}

void QOpenGL2PaintEngineEx::transformChanged()
{
    Q_D(QOpenGL2PaintEngineEx);
    d->matrixDirty = true;
    state()->matrixChanged = true;
}


static const QRectF scaleRect(const QRectF &r, qreal sx, qreal sy)
{
    return QRectF(r.x() * sx, r.y() * sy, r.width() * sx, r.height() * sy);
}

void QOpenGL2PaintEngineEx::drawPixmap(const QRectF& dest, const QPixmap & pixmap, const QRectF & src)
{
    Q_D(QOpenGL2PaintEngineEx);
    QOpenGLContext *ctx = d->ctx;

    int max_texture_size = ctx->d_func()->maxTextureSize();
    if (pixmap.width() > max_texture_size || pixmap.height() > max_texture_size) {
        QPixmap scaled = pixmap.scaled(max_texture_size, max_texture_size, Qt::KeepAspectRatio);

        const qreal sx = scaled.width() / qreal(pixmap.width());
        const qreal sy = scaled.height() / qreal(pixmap.height());

        drawPixmap(dest, scaled, scaleRect(src, sx, sy));
        return;
    }

    ensureActive();
    d->transferMode(ImageDrawingMode);

    d->funcs.glActiveTexture(GL_TEXTURE0 + QT_IMAGE_TEXTURE_UNIT);
    GLuint id = QOpenGLTextureCache::cacheForContext(ctx)->bindTexture(ctx, pixmap);

    QOpenGLRect srcRect(src.left(), src.top(), src.right(), src.bottom());

    bool isBitmap = pixmap.isQBitmap();
    bool isOpaque = !isBitmap && !pixmap.hasAlpha();

    d->updateTextureFilter(GL_TEXTURE_2D, GL_CLAMP_TO_EDGE,
                           state()->renderHints & QPainter::SmoothPixmapTransform, id);
    d->drawTexture(dest, srcRect, pixmap.size(), isOpaque, isBitmap);
}

void QOpenGL2PaintEngineEx::drawImage(const QRectF& dest, const QImage& image, const QRectF& src,
                        Qt::ImageConversionFlags)
{
    Q_D(QOpenGL2PaintEngineEx);
    QOpenGLContext *ctx = d->ctx;

    int max_texture_size = ctx->d_func()->maxTextureSize();
    if (image.width() > max_texture_size || image.height() > max_texture_size) {
        QImage scaled = image.scaled(max_texture_size, max_texture_size, Qt::KeepAspectRatio);

        const qreal sx = scaled.width() / qreal(image.width());
        const qreal sy = scaled.height() / qreal(image.height());

        drawImage(dest, scaled, scaleRect(src, sx, sy));
        return;
    }

    ensureActive();
    d->transferMode(ImageDrawingMode);

    d->funcs.glActiveTexture(GL_TEXTURE0 + QT_IMAGE_TEXTURE_UNIT);

    GLuint id = QOpenGLTextureCache::cacheForContext(ctx)->bindTexture(ctx, image);

    d->updateTextureFilter(GL_TEXTURE_2D, GL_CLAMP_TO_EDGE,
                           state()->renderHints & QPainter::SmoothPixmapTransform, id);
    d->drawTexture(dest, src, image.size(), !image.hasAlphaChannel());
}

void QOpenGL2PaintEngineEx::drawStaticTextItem(QStaticTextItem *textItem)
{
    Q_D(QOpenGL2PaintEngineEx);

    ensureActive();

    QPainterState *s = state();

    QFontEngine *fontEngine = textItem->fontEngine();
    if (shouldDrawCachedGlyphs(fontEngine, s->matrix)) {
        QFontEngine::GlyphFormat glyphFormat = fontEngine->glyphFormat != QFontEngine::Format_None
                                                ? fontEngine->glyphFormat : d->glyphCacheFormat;
        if (glyphFormat == QFontEngine::Format_A32) {
            if (d->device->context()->format().alphaBufferSize() > 0 || s->matrix.type() > QTransform::TxTranslate
                || (s->composition_mode != QPainter::CompositionMode_Source
                && s->composition_mode != QPainter::CompositionMode_SourceOver))
            {
                glyphFormat = QFontEngine::Format_A8;
            }
        }

        d->drawCachedGlyphs(glyphFormat, textItem);
    } else {
        QPaintEngineEx::drawStaticTextItem(textItem);
    }
}

bool QOpenGL2PaintEngineEx::drawTexture(const QRectF &dest, GLuint textureId, const QSize &size, const QRectF &src)
{
    Q_D(QOpenGL2PaintEngineEx);
    if (!d->shaderManager)
        return false;

    ensureActive();
    d->transferMode(ImageDrawingMode);

    d->funcs.glActiveTexture(GL_TEXTURE0 + QT_IMAGE_TEXTURE_UNIT);
    d->funcs.glBindTexture(GL_TEXTURE_2D, textureId);

    QOpenGLRect srcRect(src.left(), src.bottom(), src.right(), src.top());

    d->updateTextureFilter(GL_TEXTURE_2D, GL_CLAMP_TO_EDGE,
                           state()->renderHints & QPainter::SmoothPixmapTransform, textureId);
    d->drawTexture(dest, srcRect, size, false);
    return true;
}

void QOpenGL2PaintEngineEx::drawTextItem(const QPointF &p, const QTextItem &textItem)
{
    Q_D(QOpenGL2PaintEngineEx);

    ensureActive();
    QOpenGL2PaintEngineState *s = state();

    const QTextItemInt &ti = static_cast<const QTextItemInt &>(textItem);

    QTransform::TransformationType txtype = s->matrix.type();

    QFontEngine::GlyphFormat glyphFormat = ti.fontEngine->glyphFormat != QFontEngine::Format_None
                                                ? ti.fontEngine->glyphFormat : d->glyphCacheFormat;

    if (glyphFormat == QFontEngine::Format_A32) {
        if (d->device->context()->format().alphaBufferSize() > 0 || txtype > QTransform::TxTranslate
            || (state()->composition_mode != QPainter::CompositionMode_Source
            && state()->composition_mode != QPainter::CompositionMode_SourceOver))
        {
            glyphFormat = QFontEngine::Format_A8;
        }
    }

    if (shouldDrawCachedGlyphs(ti.fontEngine, s->matrix)) {
        QVarLengthArray<QFixedPoint> positions;
        QVarLengthArray<glyph_t> glyphs;
        QTransform matrix = QTransform::fromTranslate(p.x(), p.y());
        ti.fontEngine->getGlyphPositions(ti.glyphs, matrix, ti.flags, glyphs, positions);

        {
            QStaticTextItem staticTextItem;
            staticTextItem.chars = const_cast<QChar *>(ti.chars);
            staticTextItem.setFontEngine(ti.fontEngine);
            staticTextItem.glyphs = glyphs.data();
            staticTextItem.numChars = ti.num_chars;
            staticTextItem.numGlyphs = glyphs.size();
            staticTextItem.glyphPositions = positions.data();

            d->drawCachedGlyphs(glyphFormat, &staticTextItem);
        }
        return;
    }

    QPaintEngineEx::drawTextItem(p, ti);
}

namespace {

    class QOpenGLStaticTextUserData: public QStaticTextUserData
    {
    public:
        QOpenGLStaticTextUserData()
            : QStaticTextUserData(OpenGLUserData), cacheSize(0, 0), cacheSerialNumber(0)
        {
        }

        ~QOpenGLStaticTextUserData()
        {
        }

        QSize cacheSize;
        QOpenGL2PEXVertexArray vertexCoordinateArray;
        QOpenGL2PEXVertexArray textureCoordinateArray;
        QFontEngine::GlyphFormat glyphFormat;
        int cacheSerialNumber;
    };

}


// #define QT_OPENGL_DRAWCACHEDGLYPHS_INDEX_ARRAY_VBO

bool QOpenGL2PaintEngineEx::shouldDrawCachedGlyphs(QFontEngine *fontEngine, const QTransform &t) const
{
    // The paint engine does not support projected cached glyph drawing
    if (t.type() == QTransform::TxProject)
        return false;

    // The font engine might not support filling the glyph cache
    // with the given transform applied, in which case we need to
    // fall back to the QPainterPath code-path.
    if (!fontEngine->supportsTransformation(t)) {
        // Except that drawing paths is slow, so for scales between
        // 0.5 and 2.0 we leave the glyph cache untransformed and deal
        // with the transform ourselves when painting, resulting in
        // drawing 1x cached glyphs with a smooth-scale.
        float det = t.determinant();
        if (det >= 0.25f && det <= 4.f) {
            // Assuming the baseclass still agrees
            return QPaintEngineEx::shouldDrawCachedGlyphs(fontEngine, t);
        }

        return false; // Fall back to path-drawing
    }

    return QPaintEngineEx::shouldDrawCachedGlyphs(fontEngine, t);
}

void QOpenGL2PaintEngineExPrivate::drawCachedGlyphs(QFontEngine::GlyphFormat glyphFormat,
                                                QStaticTextItem *staticTextItem)
{
    Q_Q(QOpenGL2PaintEngineEx);

    QOpenGL2PaintEngineState *s = q->state();

    void *cacheKey = ctx->shareGroup();
    bool recreateVertexArrays = false;

    QTransform glyphCacheTransform;
    QFontEngine *fe = staticTextItem->fontEngine();
    if (fe->supportsTransformation(s->matrix)) {
        // The font-engine supports rendering glyphs with the current transform, so we
        // build a glyph-cache with the scale pre-applied, so that the cache contains
        // glyphs with the appropriate resolution in the case of retina displays.
        glyphCacheTransform = s->matrix.type() < QTransform::TxRotate ?
            QTransform::fromScale(qAbs(s->matrix.m11()), qAbs(s->matrix.m22())) :
            QTransform::fromScale(
                QVector2D(s->matrix.m11(), s->matrix.m12()).length(),
                QVector2D(s->matrix.m21(), s->matrix.m22()).length());
    }

    QOpenGLTextureGlyphCache *cache =
            (QOpenGLTextureGlyphCache *) fe->glyphCache(cacheKey, glyphFormat, glyphCacheTransform);
    if (!cache || cache->glyphFormat() != glyphFormat || cache->contextGroup() == 0) {
        cache = new QOpenGLTextureGlyphCache(glyphFormat, glyphCacheTransform);
        fe->setGlyphCache(cacheKey, cache);
        recreateVertexArrays = true;
    }

    if (staticTextItem->userDataNeedsUpdate) {
        recreateVertexArrays = true;
    } else if (staticTextItem->userData() == 0) {
        recreateVertexArrays = true;
    } else if (staticTextItem->userData()->type != QStaticTextUserData::OpenGLUserData) {
        recreateVertexArrays = true;
    } else {
        QOpenGLStaticTextUserData *userData = static_cast<QOpenGLStaticTextUserData *>(staticTextItem->userData());
        if (userData->glyphFormat != glyphFormat) {
            recreateVertexArrays = true;
        } else if (userData->cacheSerialNumber != cache->serialNumber()) {
            recreateVertexArrays = true;
        }
    }

    // We only need to update the cache with new glyphs if we are actually going to recreate the vertex arrays.
    // If the cache size has changed, we do need to regenerate the vertices, but we don't need to repopulate the
    // cache so this text is performed before we test if the cache size has changed.
    if (recreateVertexArrays) {
        cache->setPaintEnginePrivate(this);
        if (!cache->populate(fe, staticTextItem->numGlyphs,
                             staticTextItem->glyphs, staticTextItem->glyphPositions)) {
            // No space for glyphs in cache. We need to reset it and try again.
            cache->clear();
            cache->populate(fe, staticTextItem->numGlyphs,
                            staticTextItem->glyphs, staticTextItem->glyphPositions);
        }
        cache->fillInPendingGlyphs();
    }

    if (cache->width() == 0 || cache->height() == 0)
        return;

    if (glyphFormat == QFontEngine::Format_ARGB)
        transferMode(ImageArrayDrawingMode);
    else
        transferMode(TextDrawingMode);

    int margin = fe->glyphMargin(glyphFormat);

    GLfloat dx = 1.0 / cache->width();
    GLfloat dy = 1.0 / cache->height();

    // Use global arrays by default
    QOpenGL2PEXVertexArray *vertexCoordinates = &vertexCoordinateArray;
    QOpenGL2PEXVertexArray *textureCoordinates = &textureCoordinateArray;

    if (staticTextItem->useBackendOptimizations) {
        QOpenGLStaticTextUserData *userData = 0;

        if (staticTextItem->userData() == 0
            || staticTextItem->userData()->type != QStaticTextUserData::OpenGLUserData) {

            userData = new QOpenGLStaticTextUserData();
            staticTextItem->setUserData(userData);

        } else {
            userData = static_cast<QOpenGLStaticTextUserData*>(staticTextItem->userData());
        }

        userData->glyphFormat = glyphFormat;
        userData->cacheSerialNumber = cache->serialNumber();

        // Use cache if backend optimizations is turned on
        vertexCoordinates = &userData->vertexCoordinateArray;
        textureCoordinates = &userData->textureCoordinateArray;

        QSize size(cache->width(), cache->height());
        if (userData->cacheSize != size) {
            recreateVertexArrays = true;
            userData->cacheSize = size;
        }
    }

    if (recreateVertexArrays) {
        vertexCoordinates->clear();
        textureCoordinates->clear();

        bool supportsSubPixelPositions = fe->supportsSubPixelPositions();
        for (int i=0; i<staticTextItem->numGlyphs; ++i) {
            QFixed subPixelPosition;
            if (supportsSubPixelPositions)
                subPixelPosition = fe->subPixelPositionForX(staticTextItem->glyphPositions[i].x);

            QTextureGlyphCache::GlyphAndSubPixelPosition glyph(staticTextItem->glyphs[i], subPixelPosition);

            const QTextureGlyphCache::Coord &c = cache->coords[glyph];
            if (c.isNull())
                continue;

            int x = qFloor(staticTextItem->glyphPositions[i].x.toReal() * cache->transform().m11()) + c.baseLineX - margin;
            int y = qRound(staticTextItem->glyphPositions[i].y.toReal() * cache->transform().m22()) - c.baseLineY - margin;

            vertexCoordinates->addQuad(QRectF(x, y, c.w, c.h));
            textureCoordinates->addQuad(QRectF(c.x*dx, c.y*dy, c.w * dx, c.h * dy));
        }

        staticTextItem->userDataNeedsUpdate = false;
    }

    int numGlyphs = vertexCoordinates->vertexCount() / 4;
    if (numGlyphs == 0)
        return;

    if (elementIndices.size() < numGlyphs*6) {
        Q_ASSERT(elementIndices.size() % 6 == 0);
        int j = elementIndices.size() / 6 * 4;
        while (j < numGlyphs*4) {
            elementIndices.append(j + 0);
            elementIndices.append(j + 0);
            elementIndices.append(j + 1);
            elementIndices.append(j + 2);
            elementIndices.append(j + 3);
            elementIndices.append(j + 3);

            j += 4;
        }

#if defined(QT_OPENGL_DRAWCACHEDGLYPHS_INDEX_ARRAY_VBO)
        if (elementIndicesVBOId == 0)
            funcs.glGenBuffers(1, &elementIndicesVBOId);

        funcs.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementIndicesVBOId);
        funcs.glBufferData(GL_ELEMENT_ARRAY_BUFFER, elementIndices.size() * sizeof(GLushort),
                           elementIndices.constData(), GL_STATIC_DRAW);
#endif
    } else {
#if defined(QT_OPENGL_DRAWCACHEDGLYPHS_INDEX_ARRAY_VBO)
        funcs.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementIndicesVBOId);
#endif
    }

    if (glyphFormat != QFontEngine::Format_ARGB || recreateVertexArrays) {
        setVertexAttributePointer(QT_VERTEX_COORDS_ATTR, (GLfloat*)vertexCoordinates->data());
        setVertexAttributePointer(QT_TEXTURE_COORDS_ATTR, (GLfloat*)textureCoordinates->data());
    }

    if (!snapToPixelGrid) {
        snapToPixelGrid = true;
        matrixDirty = true;
    }

    QBrush pensBrush = q->state()->pen.brush();
    setBrush(pensBrush);

    if (glyphFormat == QFontEngine::Format_A32) {

        // Subpixel antialiasing without gamma correction

        QPainter::CompositionMode compMode = q->state()->composition_mode;
        Q_ASSERT(compMode == QPainter::CompositionMode_Source
            || compMode == QPainter::CompositionMode_SourceOver);

        shaderManager->setMaskType(QOpenGLEngineShaderManager::SubPixelMaskPass1);

        if (pensBrush.style() == Qt::SolidPattern) {
            // Solid patterns can get away with only one pass.
            QColor c = pensBrush.color();
            qreal oldOpacity = q->state()->opacity;
            if (compMode == QPainter::CompositionMode_Source) {
                c = qt_premultiplyColor(c, q->state()->opacity);
                q->state()->opacity = 1;
                opacityUniformDirty = true;
            }

            compositionModeDirty = false; // I can handle this myself, thank you very much
            prepareForCachedGlyphDraw(*cache);

            // prepareForCachedGlyphDraw() have set the opacity on the current shader, so the opacity state can now be reset.
            if (compMode == QPainter::CompositionMode_Source) {
                q->state()->opacity = oldOpacity;
                opacityUniformDirty = true;
            }

            funcs.glEnable(GL_BLEND);
            funcs.glBlendFunc(GL_CONSTANT_COLOR, GL_ONE_MINUS_SRC_COLOR);
            funcs.glBlendColor(c.redF(), c.greenF(), c.blueF(), c.alphaF());
        } else {
            // Other brush styles need two passes.

            qreal oldOpacity = q->state()->opacity;
            if (compMode == QPainter::CompositionMode_Source) {
                q->state()->opacity = 1;
                opacityUniformDirty = true;
                pensBrush = Qt::white;
                setBrush(pensBrush);
            }

            compositionModeDirty = false; // I can handle this myself, thank you very much
            prepareForCachedGlyphDraw(*cache);
            funcs.glEnable(GL_BLEND);
            funcs.glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);

            funcs.glActiveTexture(GL_TEXTURE0 + QT_MASK_TEXTURE_UNIT);
            funcs.glBindTexture(GL_TEXTURE_2D, cache->texture());
            updateTextureFilter(GL_TEXTURE_2D, GL_REPEAT, false);

#if defined(QT_OPENGL_DRAWCACHEDGLYPHS_INDEX_ARRAY_VBO)
            funcs.glDrawElements(GL_TRIANGLE_STRIP, 6 * numGlyphs, GL_UNSIGNED_SHORT, 0);
#else
            funcs.glDrawElements(GL_TRIANGLE_STRIP, 6 * numGlyphs, GL_UNSIGNED_SHORT, elementIndices.data());
#endif

            shaderManager->setMaskType(QOpenGLEngineShaderManager::SubPixelMaskPass2);

            if (compMode == QPainter::CompositionMode_Source) {
                q->state()->opacity = oldOpacity;
                opacityUniformDirty = true;
                pensBrush = q->state()->pen.brush();
                setBrush(pensBrush);
            }

            compositionModeDirty = false;
            prepareForCachedGlyphDraw(*cache);
            funcs.glEnable(GL_BLEND);
            funcs.glBlendFunc(GL_ONE, GL_ONE);
        }
        compositionModeDirty = true;
    } else if (glyphFormat == QFontEngine::Format_ARGB) {
        currentBrush = noBrush;
        shaderManager->setSrcPixelType(QOpenGLEngineShaderManager::ImageSrc);
        if (prepareForCachedGlyphDraw(*cache))
            shaderManager->currentProgram()->setUniformValue(location(QOpenGLEngineShaderManager::ImageTexture), QT_IMAGE_TEXTURE_UNIT);
    } else {
        // Greyscale/mono glyphs

        shaderManager->setMaskType(QOpenGLEngineShaderManager::PixelMask);
        prepareForCachedGlyphDraw(*cache);
    }

    QOpenGLTextureGlyphCache::FilterMode filterMode = (s->matrix.type() > QTransform::TxTranslate)?QOpenGLTextureGlyphCache::Linear:QOpenGLTextureGlyphCache::Nearest;
    if (lastMaskTextureUsed != cache->texture() || cache->filterMode() != filterMode) {

        if (glyphFormat == QFontEngine::Format_ARGB)
            funcs.glActiveTexture(GL_TEXTURE0 + QT_IMAGE_TEXTURE_UNIT);
        else
            funcs.glActiveTexture(GL_TEXTURE0 + QT_MASK_TEXTURE_UNIT);

        if (lastMaskTextureUsed != cache->texture()) {
            funcs.glBindTexture(GL_TEXTURE_2D, cache->texture());
            lastMaskTextureUsed = cache->texture();
        }

        if (cache->filterMode() != filterMode) {
            if (filterMode == QOpenGLTextureGlyphCache::Linear) {
                funcs.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                funcs.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            } else {
                funcs.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                funcs.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            }
            cache->setFilterMode(filterMode);
        }
    }

#if defined(QT_OPENGL_DRAWCACHEDGLYPHS_INDEX_ARRAY_VBO)
    funcs.glDrawElements(GL_TRIANGLE_STRIP, 6 * numGlyphs, GL_UNSIGNED_SHORT, 0);
    funcs.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
#else
    funcs.glDrawElements(GL_TRIANGLE_STRIP, 6 * numGlyphs, GL_UNSIGNED_SHORT, elementIndices.data());
#endif
}

void QOpenGL2PaintEngineEx::drawPixmapFragments(const QPainter::PixmapFragment *fragments, int fragmentCount, const QPixmap &pixmap,
                                            QPainter::PixmapFragmentHints hints)
{
    Q_D(QOpenGL2PaintEngineEx);
    // Use fallback for extended composition modes.
    if (state()->composition_mode > QPainter::CompositionMode_Plus) {
        QPaintEngineEx::drawPixmapFragments(fragments, fragmentCount, pixmap, hints);
        return;
    }

    ensureActive();
    int max_texture_size = d->ctx->d_func()->maxTextureSize();
    if (pixmap.width() > max_texture_size || pixmap.height() > max_texture_size) {
        QPixmap scaled = pixmap.scaled(max_texture_size, max_texture_size, Qt::KeepAspectRatio);
        d->drawPixmapFragments(fragments, fragmentCount, scaled, hints);
    } else {
        d->drawPixmapFragments(fragments, fragmentCount, pixmap, hints);
    }
}


void QOpenGL2PaintEngineExPrivate::drawPixmapFragments(const QPainter::PixmapFragment *fragments,
                                                   int fragmentCount, const QPixmap &pixmap,
                                                   QPainter::PixmapFragmentHints hints)
{
    GLfloat dx = 1.0f / pixmap.size().width();
    GLfloat dy = 1.0f / pixmap.size().height();

    vertexCoordinateArray.clear();
    textureCoordinateArray.clear();
    opacityArray.reset();

    if (snapToPixelGrid) {
        snapToPixelGrid = false;
        matrixDirty = true;
    }

    bool allOpaque = true;

    for (int i = 0; i < fragmentCount; ++i) {
        qreal s = 0;
        qreal c = 1;
        if (fragments[i].rotation != 0) {
            s = qFastSin(fragments[i].rotation * Q_PI / 180);
            c = qFastCos(fragments[i].rotation * Q_PI / 180);
        }

        qreal right = 0.5 * fragments[i].scaleX * fragments[i].width;
        qreal bottom = 0.5 * fragments[i].scaleY * fragments[i].height;
        QOpenGLPoint bottomRight(right * c - bottom * s, right * s + bottom * c);
        QOpenGLPoint bottomLeft(-right * c - bottom * s, -right * s + bottom * c);

        vertexCoordinateArray.addVertex(bottomRight.x + fragments[i].x, bottomRight.y + fragments[i].y);
        vertexCoordinateArray.addVertex(-bottomLeft.x + fragments[i].x, -bottomLeft.y + fragments[i].y);
        vertexCoordinateArray.addVertex(-bottomRight.x + fragments[i].x, -bottomRight.y + fragments[i].y);
        vertexCoordinateArray.addVertex(-bottomRight.x + fragments[i].x, -bottomRight.y + fragments[i].y);
        vertexCoordinateArray.addVertex(bottomLeft.x + fragments[i].x, bottomLeft.y + fragments[i].y);
        vertexCoordinateArray.addVertex(bottomRight.x + fragments[i].x, bottomRight.y + fragments[i].y);

        QOpenGLRect src(fragments[i].sourceLeft * dx, fragments[i].sourceTop * dy,
                    (fragments[i].sourceLeft + fragments[i].width) * dx,
                    (fragments[i].sourceTop + fragments[i].height) * dy);

        textureCoordinateArray.addVertex(src.right, src.bottom);
        textureCoordinateArray.addVertex(src.right, src.top);
        textureCoordinateArray.addVertex(src.left, src.top);
        textureCoordinateArray.addVertex(src.left, src.top);
        textureCoordinateArray.addVertex(src.left, src.bottom);
        textureCoordinateArray.addVertex(src.right, src.bottom);

        qreal opacity = fragments[i].opacity * q->state()->opacity;
        opacityArray << opacity << opacity << opacity << opacity << opacity << opacity;
        allOpaque &= (opacity >= 0.99f);
    }

    funcs.glActiveTexture(GL_TEXTURE0 + QT_IMAGE_TEXTURE_UNIT);
    GLuint id = QOpenGLTextureCache::cacheForContext(ctx)->bindTexture(ctx, pixmap);
    transferMode(ImageOpacityArrayDrawingMode);

    bool isBitmap = pixmap.isQBitmap();
    bool isOpaque = !isBitmap && (!pixmap.hasAlpha() || (hints & QPainter::OpaqueHint)) && allOpaque;

    updateTextureFilter(GL_TEXTURE_2D, GL_CLAMP_TO_EDGE,
                           q->state()->renderHints & QPainter::SmoothPixmapTransform, id);

    // Setup for texture drawing
    currentBrush = noBrush;
    shaderManager->setSrcPixelType(isBitmap ? QOpenGLEngineShaderManager::PatternSrc
                                            : QOpenGLEngineShaderManager::ImageSrc);
    if (prepareForDraw(isOpaque))
        shaderManager->currentProgram()->setUniformValue(location(QOpenGLEngineShaderManager::ImageTexture), QT_IMAGE_TEXTURE_UNIT);

    if (isBitmap) {
        QColor col = qt_premultiplyColor(q->state()->pen.color(), (GLfloat)q->state()->opacity);
        shaderManager->currentProgram()->setUniformValue(location(QOpenGLEngineShaderManager::PatternColor), col);
    }

    funcs.glDrawArrays(GL_TRIANGLES, 0, 6 * fragmentCount);
}

bool QOpenGL2PaintEngineEx::begin(QPaintDevice *pdev)
{
    Q_D(QOpenGL2PaintEngineEx);

    Q_ASSERT(pdev->devType() == QInternal::OpenGL);
    d->device = static_cast<QOpenGLPaintDevice*>(pdev);

    if (!d->device)
        return false;

    if (d->device->context() != QOpenGLContext::currentContext()) {
        qWarning("QPainter::begin(): QOpenGLPaintDevice's context needs to be current");
        return false;
    }

    d->ctx = QOpenGLContext::currentContext();
    d->ctx->d_func()->active_engine = this;

    d->funcs.initializeOpenGLFunctions();

    for (int i = 0; i < QT_GL_VERTEX_ARRAY_TRACKED_COUNT; ++i)
        d->vertexAttributeArraysEnabledState[i] = false;

    const QSize sz = d->device->size();
    d->width = sz.width();
    d->height = sz.height();
    d->mode = BrushDrawingMode;
    d->brushTextureDirty = true;
    d->brushUniformsDirty = true;
    d->matrixUniformDirty = true;
    d->matrixDirty = true;
    d->compositionModeDirty = true;
    d->opacityUniformDirty = true;
    d->needsSync = true;
    d->useSystemClip = !systemClip().isEmpty();
    d->currentBrush = QBrush();

    d->dirtyStencilRegion = QRect(0, 0, d->width, d->height);
    d->stencilClean = true;

    d->shaderManager = new QOpenGLEngineShaderManager(d->ctx);

    d->funcs.glDisable(GL_STENCIL_TEST);
    d->funcs.glDisable(GL_DEPTH_TEST);
    d->funcs.glDisable(GL_SCISSOR_TEST);

    d->glyphCacheFormat = QFontEngine::Format_A8;

#ifndef QT_OPENGL_ES_2
    if (!QOpenGLContext::currentContext()->isOpenGLES()) {
        d->funcs.glDisable(GL_MULTISAMPLE);
        d->glyphCacheFormat = QFontEngine::Format_A32;
        d->multisamplingAlwaysEnabled = false;
    } else
#endif // QT_OPENGL_ES_2
    {
        // OpenGL ES can't switch MSAA off, so if the gl paint device is
        // multisampled, it's always multisampled.
        d->multisamplingAlwaysEnabled = d->device->context()->format().samples() > 1;
    }

    return true;
}

bool QOpenGL2PaintEngineEx::end()
{
    Q_D(QOpenGL2PaintEngineEx);

    QOpenGLContext *ctx = d->ctx;
    d->funcs.glUseProgram(0);
    d->transferMode(BrushDrawingMode);

    ctx->d_func()->active_engine = 0;

    d->resetGLState();

    delete d->shaderManager;
    d->shaderManager = 0;
    d->currentBrush = QBrush();

#ifdef QT_OPENGL_CACHE_AS_VBOS
    if (!d->unusedVBOSToClean.isEmpty()) {
        glDeleteBuffers(d->unusedVBOSToClean.size(), d->unusedVBOSToClean.constData());
        d->unusedVBOSToClean.clear();
    }
    if (!d->unusedIBOSToClean.isEmpty()) {
        glDeleteBuffers(d->unusedIBOSToClean.size(), d->unusedIBOSToClean.constData());
        d->unusedIBOSToClean.clear();
    }
#endif

    return false;
}

void QOpenGL2PaintEngineEx::ensureActive()
{
    Q_D(QOpenGL2PaintEngineEx);
    QOpenGLContext *ctx = d->ctx;

    if (isActive() && ctx->d_func()->active_engine != this) {
        ctx->d_func()->active_engine = this;
        d->needsSync = true;
    }

    if (d->needsSync) {
        d->device->ensureActiveTarget();

        d->transferMode(BrushDrawingMode);
        d->funcs.glViewport(0, 0, d->width, d->height);
        d->needsSync = false;
        d->lastMaskTextureUsed = 0;
        d->shaderManager->setDirty();
        d->syncGlState();
        for (int i = 0; i < 3; ++i)
            d->vertexAttribPointers[i] = (GLfloat*)-1; // Assume the pointers are clobbered
        setState(state());
    }
}

void QOpenGL2PaintEngineExPrivate::updateClipScissorTest()
{
    Q_Q(QOpenGL2PaintEngineEx);
    if (q->state()->clipTestEnabled) {
        funcs.glEnable(GL_STENCIL_TEST);
        funcs.glStencilFunc(GL_LEQUAL, q->state()->currentClip, ~GL_STENCIL_HIGH_BIT);
    } else {
        funcs.glDisable(GL_STENCIL_TEST);
        funcs.glStencilFunc(GL_ALWAYS, 0, 0xff);
    }

#ifdef QT_GL_NO_SCISSOR_TEST
    currentScissorBounds = QRect(0, 0, width, height);
#else
    QRect bounds = q->state()->rectangleClip;
    if (!q->state()->clipEnabled) {
        if (useSystemClip)
            bounds = systemClip.boundingRect();
        else
            bounds = QRect(0, 0, width, height);
    } else {
        if (useSystemClip)
            bounds = bounds.intersected(systemClip.boundingRect());
        else
            bounds = bounds.intersected(QRect(0, 0, width, height));
    }

    currentScissorBounds = bounds;

    if (bounds == QRect(0, 0, width, height)) {
        funcs.glDisable(GL_SCISSOR_TEST);
    } else {
        funcs.glEnable(GL_SCISSOR_TEST);
        setScissor(bounds);
    }
#endif
}

void QOpenGL2PaintEngineExPrivate::setScissor(const QRect &rect)
{
    const int left = rect.left();
    const int width = rect.width();
    int bottom = height - (rect.top() + rect.height());
    if (device->paintFlipped()) {
        bottom = rect.top();
    }
    const int height = rect.height();

    funcs.glScissor(left, bottom, width, height);
}

void QOpenGL2PaintEngineEx::clipEnabledChanged()
{
    Q_D(QOpenGL2PaintEngineEx);

    state()->clipChanged = true;

    if (painter()->hasClipping())
        d->regenerateClip();
    else
        d->systemStateChanged();
}

void QOpenGL2PaintEngineExPrivate::clearClip(uint value)
{
    dirtyStencilRegion -= currentScissorBounds;

    funcs.glStencilMask(0xff);
    funcs.glClearStencil(value);
    funcs.glClear(GL_STENCIL_BUFFER_BIT);
    funcs.glStencilMask(0x0);

    q->state()->needsClipBufferClear = false;
}

void QOpenGL2PaintEngineExPrivate::writeClip(const QVectorPath &path, uint value)
{
    transferMode(BrushDrawingMode);

    if (snapToPixelGrid) {
        snapToPixelGrid = false;
        matrixDirty = true;
    }

    if (matrixDirty)
        updateMatrix();

    stencilClean = false;

    const bool singlePass = !path.hasWindingFill()
        && (((q->state()->currentClip == maxClip - 1) && q->state()->clipTestEnabled)
            || q->state()->needsClipBufferClear);
    const uint referenceClipValue = q->state()->needsClipBufferClear ? 1 : q->state()->currentClip;

    if (q->state()->needsClipBufferClear)
        clearClip(1);

    if (path.isEmpty()) {
        funcs.glEnable(GL_STENCIL_TEST);
        funcs.glStencilFunc(GL_LEQUAL, value, ~GL_STENCIL_HIGH_BIT);
        return;
    }

    if (q->state()->clipTestEnabled)
        funcs.glStencilFunc(GL_LEQUAL, q->state()->currentClip, ~GL_STENCIL_HIGH_BIT);
    else
        funcs.glStencilFunc(GL_ALWAYS, 0, 0xff);

    vertexCoordinateArray.clear();
    vertexCoordinateArray.addPath(path, inverseScale, false);

    if (!singlePass)
        fillStencilWithVertexArray(vertexCoordinateArray, path.hasWindingFill());

    funcs.glColorMask(false, false, false, false);
    funcs.glEnable(GL_STENCIL_TEST);
    useSimpleShader();

    if (singlePass) {
        // Under these conditions we can set the new stencil value in a single
        // pass, by using the current value and the "new value" as the toggles

        funcs.glStencilFunc(GL_LEQUAL, referenceClipValue, ~GL_STENCIL_HIGH_BIT);
        funcs.glStencilOp(GL_KEEP, GL_INVERT, GL_INVERT);
        funcs.glStencilMask(value ^ referenceClipValue);

        drawVertexArrays(vertexCoordinateArray, GL_TRIANGLE_FAN);
    } else {
        funcs.glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
        funcs.glStencilMask(0xff);

        if (!q->state()->clipTestEnabled && path.hasWindingFill()) {
            // Pass when any clip bit is set, set high bit
            funcs.glStencilFunc(GL_NOTEQUAL, GL_STENCIL_HIGH_BIT, ~GL_STENCIL_HIGH_BIT);
            composite(vertexCoordinateArray.boundingRect());
        }

        // Pass when high bit is set, replace stencil value with new clip value
        funcs.glStencilFunc(GL_NOTEQUAL, value, GL_STENCIL_HIGH_BIT);

        composite(vertexCoordinateArray.boundingRect());
    }

    funcs.glStencilFunc(GL_LEQUAL, value, ~GL_STENCIL_HIGH_BIT);
    funcs.glStencilMask(0);

    funcs.glColorMask(true, true, true, true);
}

void QOpenGL2PaintEngineEx::clip(const QVectorPath &path, Qt::ClipOperation op)
{
//     qDebug("QOpenGL2PaintEngineEx::clip()");
    Q_D(QOpenGL2PaintEngineEx);

    state()->clipChanged = true;

    ensureActive();

    if (op == Qt::ReplaceClip) {
        op = Qt::IntersectClip;
        if (d->hasClipOperations()) {
            d->systemStateChanged();
            state()->canRestoreClip = false;
        }
    }

#ifndef QT_GL_NO_SCISSOR_TEST
    if (!path.isEmpty() && op == Qt::IntersectClip && (path.shape() == QVectorPath::RectangleHint)) {
        const QPointF* const points = reinterpret_cast<const QPointF*>(path.points());
        QRectF rect(points[0], points[2]);

        if (state()->matrix.type() <= QTransform::TxScale
            || (state()->matrix.type() == QTransform::TxRotate
                && qFuzzyIsNull(state()->matrix.m11())
                && qFuzzyIsNull(state()->matrix.m22())))
        {
            state()->rectangleClip = state()->rectangleClip.intersected(state()->matrix.mapRect(rect).toRect());
            d->updateClipScissorTest();
            return;
        }
    }
#endif

    const QRect pathRect = state()->matrix.mapRect(path.controlPointRect()).toAlignedRect();

    switch (op) {
    case Qt::NoClip:
        if (d->useSystemClip) {
            state()->clipTestEnabled = true;
            state()->currentClip = 1;
        } else {
            state()->clipTestEnabled = false;
        }
        state()->rectangleClip = QRect(0, 0, d->width, d->height);
        state()->canRestoreClip = false;
        d->updateClipScissorTest();
        break;
    case Qt::IntersectClip:
        state()->rectangleClip = state()->rectangleClip.intersected(pathRect);
        d->updateClipScissorTest();
        d->resetClipIfNeeded();
        ++d->maxClip;
        d->writeClip(path, d->maxClip);
        state()->currentClip = d->maxClip;
        state()->clipTestEnabled = true;
        break;
    default:
        break;
    }
}

void QOpenGL2PaintEngineExPrivate::regenerateClip()
{
    systemStateChanged();
    replayClipOperations();
}

void QOpenGL2PaintEngineExPrivate::systemStateChanged()
{
    Q_Q(QOpenGL2PaintEngineEx);

    q->state()->clipChanged = true;

    if (systemClip.isEmpty()) {
        useSystemClip = false;
    } else {
        if (q->paintDevice()->devType() == QInternal::Widget && currentClipDevice) {
            //QWidgetPrivate *widgetPrivate = qt_widget_private(static_cast<QWidget *>(currentClipDevice)->window());
            //useSystemClip = widgetPrivate->extra && widgetPrivate->extra->inRenderWithPainter;
            useSystemClip = true;
        } else {
            useSystemClip = true;
        }
    }

    q->state()->clipTestEnabled = false;
    q->state()->needsClipBufferClear = true;

    q->state()->currentClip = 1;
    maxClip = 1;

    q->state()->rectangleClip = useSystemClip ? systemClip.boundingRect() : QRect(0, 0, width, height);
    updateClipScissorTest();

    if (systemClip.rectCount() == 1) {
        if (systemClip.boundingRect() == QRect(0, 0, width, height))
            useSystemClip = false;
#ifndef QT_GL_NO_SCISSOR_TEST
        // scissoring takes care of the system clip
        return;
#endif
    }

    if (useSystemClip) {
        clearClip(0);

        QPainterPath path;
        path.addRegion(systemClip);

        q->state()->currentClip = 0;
        writeClip(qtVectorPathForPath(q->state()->matrix.inverted().map(path)), 1);
        q->state()->currentClip = 1;
        q->state()->clipTestEnabled = true;
    }
}

void QOpenGL2PaintEngineEx::setState(QPainterState *new_state)
{
    //     qDebug("QOpenGL2PaintEngineEx::setState()");

    Q_D(QOpenGL2PaintEngineEx);

    QOpenGL2PaintEngineState *s = static_cast<QOpenGL2PaintEngineState *>(new_state);
    QOpenGL2PaintEngineState *old_state = state();

    QPaintEngineEx::setState(s);

    if (s->isNew) {
        // Newly created state object.  The call to setState()
        // will either be followed by a call to begin(), or we are
        // setting the state as part of a save().
        s->isNew = false;
        return;
    }

    // Setting the state as part of a restore().

    if (old_state == s || old_state->renderHintsChanged)
        renderHintsChanged();

    if (old_state == s || old_state->matrixChanged)
        d->matrixDirty = true;

    if (old_state == s || old_state->compositionModeChanged)
        d->compositionModeDirty = true;

    if (old_state == s || old_state->opacityChanged)
        d->opacityUniformDirty = true;

    if (old_state == s || old_state->clipChanged) {
        if (old_state && old_state != s && old_state->canRestoreClip) {
            d->updateClipScissorTest();
            d->funcs.glDepthFunc(GL_LEQUAL);
        } else {
            d->regenerateClip();
        }
    }
}

QPainterState *QOpenGL2PaintEngineEx::createState(QPainterState *orig) const
{
    if (orig)
        const_cast<QOpenGL2PaintEngineEx *>(this)->ensureActive();

    QOpenGL2PaintEngineState *s;
    if (!orig)
        s = new QOpenGL2PaintEngineState();
    else
        s = new QOpenGL2PaintEngineState(*static_cast<QOpenGL2PaintEngineState *>(orig));

    s->matrixChanged = false;
    s->compositionModeChanged = false;
    s->opacityChanged = false;
    s->renderHintsChanged = false;
    s->clipChanged = false;

    return s;
}

QOpenGL2PaintEngineState::QOpenGL2PaintEngineState(QOpenGL2PaintEngineState &other)
    : QPainterState(other)
{
    isNew = true;
    needsClipBufferClear = other.needsClipBufferClear;
    clipTestEnabled = other.clipTestEnabled;
    currentClip = other.currentClip;
    canRestoreClip = other.canRestoreClip;
    rectangleClip = other.rectangleClip;
}

QOpenGL2PaintEngineState::QOpenGL2PaintEngineState()
{
    isNew = true;
    needsClipBufferClear = true;
    clipTestEnabled = false;
    canRestoreClip = true;
}

QOpenGL2PaintEngineState::~QOpenGL2PaintEngineState()
{
}

void QOpenGL2PaintEngineExPrivate::setVertexAttribArrayEnabled(int arrayIndex, bool enabled)
{
    Q_ASSERT(arrayIndex < QT_GL_VERTEX_ARRAY_TRACKED_COUNT);

    if (vertexAttributeArraysEnabledState[arrayIndex] && !enabled)
        funcs.glDisableVertexAttribArray(arrayIndex);

    if (!vertexAttributeArraysEnabledState[arrayIndex] && enabled)
        funcs.glEnableVertexAttribArray(arrayIndex);

    vertexAttributeArraysEnabledState[arrayIndex] = enabled;
}

void QOpenGL2PaintEngineExPrivate::syncGlState()
{
    for (int i = 0; i < QT_GL_VERTEX_ARRAY_TRACKED_COUNT; ++i) {
        if (vertexAttributeArraysEnabledState[i])
            funcs.glEnableVertexAttribArray(i);
        else
            funcs.glDisableVertexAttribArray(i);
    }
}


QT_END_NAMESPACE
