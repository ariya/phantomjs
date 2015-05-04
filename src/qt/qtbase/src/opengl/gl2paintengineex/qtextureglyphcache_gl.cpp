/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qtextureglyphcache_gl_p.h"
#include "qpaintengineex_opengl2_p.h"
#include "qglfunctions.h"
#include "private/qglengineshadersource_p.h"

QT_BEGIN_NAMESPACE


QBasicAtomicInt qgltextureglyphcache_serial_number = Q_BASIC_ATOMIC_INITIALIZER(1);

QGLTextureGlyphCache::QGLTextureGlyphCache(QFontEngine::GlyphFormat format, const QTransform &matrix)
    : QImageTextureGlyphCache(format, matrix)
    , m_textureResource(0)
    , pex(0)
    , m_blitProgram(0)
    , m_filterMode(Nearest)
    , m_serialNumber(qgltextureglyphcache_serial_number.fetchAndAddRelaxed(1))
{
#ifdef QT_GL_TEXTURE_GLYPH_CACHE_DEBUG
    qDebug(" -> QGLTextureGlyphCache() %p for context %p.", this, QOpenGLContext::currentContext());
#endif
    m_vertexCoordinateArray[0] = -1.0f;
    m_vertexCoordinateArray[1] = -1.0f;
    m_vertexCoordinateArray[2] =  1.0f;
    m_vertexCoordinateArray[3] = -1.0f;
    m_vertexCoordinateArray[4] =  1.0f;
    m_vertexCoordinateArray[5] =  1.0f;
    m_vertexCoordinateArray[6] = -1.0f;
    m_vertexCoordinateArray[7] =  1.0f;

    m_textureCoordinateArray[0] = 0.0f;
    m_textureCoordinateArray[1] = 0.0f;
    m_textureCoordinateArray[2] = 1.0f;
    m_textureCoordinateArray[3] = 0.0f;
    m_textureCoordinateArray[4] = 1.0f;
    m_textureCoordinateArray[5] = 1.0f;
    m_textureCoordinateArray[6] = 0.0f;
    m_textureCoordinateArray[7] = 1.0f;
}

QGLTextureGlyphCache::~QGLTextureGlyphCache()
{
#ifdef QT_GL_TEXTURE_GLYPH_CACHE_DEBUG
    qDebug(" -> ~QGLTextureGlyphCache() %p.", this);
#endif
    delete m_blitProgram;
    if (m_textureResource)
        m_textureResource->free();
}

void QGLTextureGlyphCache::createTextureData(int width, int height)
{
    QGLContext *ctx = const_cast<QGLContext *>(QGLContext::currentContext());
    if (ctx == 0) {
        qWarning("QGLTextureGlyphCache::createTextureData: Called with no context");
        return;
    }
    QOpenGLFunctions *funcs = ctx->contextHandle()->functions();

    // create in QImageTextureGlyphCache baseclass is meant to be called
    // only to create the initial image and does not preserve the content,
    // so we don't call when this function is called from resize.
    if ((!QGLFramebufferObject::hasOpenGLFramebufferObjects() || ctx->d_ptr->workaround_brokenFBOReadBack) && image().isNull())
        QImageTextureGlyphCache::createTextureData(width, height);

    // Make the lower glyph texture size 16 x 16.
    if (width < 16)
        width = 16;
    if (height < 16)
        height = 16;

    if (m_textureResource && !m_textureResource->m_texture) {
        delete m_textureResource;
        m_textureResource = 0;
    }

    if (!m_textureResource)
        m_textureResource = new QGLGlyphTexture(ctx);

    funcs->glGenTextures(1, &m_textureResource->m_texture);
    funcs->glBindTexture(GL_TEXTURE_2D, m_textureResource->m_texture);

    m_textureResource->m_width = width;
    m_textureResource->m_height = height;

    if (m_format == QFontEngine::Format_A32) {
        QVarLengthArray<uchar> data(width * height * 4);
        for (int i = 0; i < data.size(); ++i)
            data[i] = 0;
        funcs->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
    } else {
        QVarLengthArray<uchar> data(width * height);
        for (int i = 0; i < data.size(); ++i)
            data[i] = 0;
        funcs->glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, &data[0]);
    }

    funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_filterMode = Nearest;
}

void QGLTextureGlyphCache::resizeTextureData(int width, int height)
{
    QGLContext *ctx = const_cast<QGLContext *>(QGLContext::currentContext());
    if (ctx == 0) {
        qWarning("QGLTextureGlyphCache::resizeTextureData: Called with no context");
        return;
    }
    QOpenGLFunctions *funcs = ctx->contextHandle()->functions();

    int oldWidth = m_textureResource->m_width;
    int oldHeight = m_textureResource->m_height;

    // Make the lower glyph texture size 16 x 16.
    if (width < 16)
        width = 16;
    if (height < 16)
        height = 16;

    GLuint oldTexture = m_textureResource->m_texture;
    createTextureData(width, height);

    if (!QGLFramebufferObject::hasOpenGLFramebufferObjects() || ctx->d_ptr->workaround_brokenFBOReadBack) {
        QImageTextureGlyphCache::resizeTextureData(width, height);
        Q_ASSERT(image().depth() == 8);
        funcs->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, oldHeight, GL_ALPHA, GL_UNSIGNED_BYTE, image().constBits());
        funcs->glDeleteTextures(1, &oldTexture);
        return;
    }

    // ### the QTextureGlyphCache API needs to be reworked to allow
    // ### resizeTextureData to fail

    ctx->d_ptr->refreshCurrentFbo();

    funcs->glBindFramebuffer(GL_FRAMEBUFFER, m_textureResource->m_fbo);

    GLuint tmp_texture;
    funcs->glGenTextures(1, &tmp_texture);
    funcs->glBindTexture(GL_TEXTURE_2D, tmp_texture);
    funcs->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, oldWidth, oldHeight, 0,
                        GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_filterMode = Nearest;
    funcs->glBindTexture(GL_TEXTURE_2D, 0);
    funcs->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                  GL_TEXTURE_2D, tmp_texture, 0);

    funcs->glActiveTexture(GL_TEXTURE0 + QT_IMAGE_TEXTURE_UNIT);
    funcs->glBindTexture(GL_TEXTURE_2D, oldTexture);

    if (pex != 0)
        pex->transferMode(BrushDrawingMode);

    funcs->glDisable(GL_STENCIL_TEST);
    funcs->glDisable(GL_DEPTH_TEST);
    funcs->glDisable(GL_SCISSOR_TEST);
    funcs->glDisable(GL_BLEND);

    funcs->glViewport(0, 0, oldWidth, oldHeight);

    QGLShaderProgram *blitProgram = 0;
    if (pex == 0) {
        if (m_blitProgram == 0) {
            m_blitProgram = new QGLShaderProgram(ctx);

            {
                QString source;
                source.append(QLatin1String(qglslMainWithTexCoordsVertexShader));
                source.append(QLatin1String(qglslUntransformedPositionVertexShader));

                QGLShader *vertexShader = new QGLShader(QGLShader::Vertex, m_blitProgram);
                vertexShader->compileSourceCode(source);

                m_blitProgram->addShader(vertexShader);
            }

            {
                QString source;
                source.append(QLatin1String(qglslMainFragmentShader));
                source.append(QLatin1String(qglslImageSrcFragmentShader));

                QGLShader *fragmentShader = new QGLShader(QGLShader::Fragment, m_blitProgram);
                fragmentShader->compileSourceCode(source);

                m_blitProgram->addShader(fragmentShader);
            }

            m_blitProgram->bindAttributeLocation("vertexCoordsArray", QT_VERTEX_COORDS_ATTR);
            m_blitProgram->bindAttributeLocation("textureCoordArray", QT_TEXTURE_COORDS_ATTR);

            m_blitProgram->link();
        }

        funcs->glVertexAttribPointer(QT_VERTEX_COORDS_ATTR, 2, GL_FLOAT, GL_FALSE, 0, m_vertexCoordinateArray);
        funcs->glVertexAttribPointer(QT_TEXTURE_COORDS_ATTR, 2, GL_FLOAT, GL_FALSE, 0, m_textureCoordinateArray);

        m_blitProgram->bind();
        m_blitProgram->enableAttributeArray(int(QT_VERTEX_COORDS_ATTR));
        m_blitProgram->enableAttributeArray(int(QT_TEXTURE_COORDS_ATTR));
        m_blitProgram->disableAttributeArray(int(QT_OPACITY_ATTR));

        blitProgram = m_blitProgram;

    } else {
        pex->setVertexAttributePointer(QT_VERTEX_COORDS_ATTR, m_vertexCoordinateArray);
        pex->setVertexAttributePointer(QT_TEXTURE_COORDS_ATTR, m_textureCoordinateArray);

        pex->shaderManager->useBlitProgram();
        blitProgram = pex->shaderManager->blitProgram();
    }

    blitProgram->setUniformValue("imageTexture", QT_IMAGE_TEXTURE_UNIT);

    funcs->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    funcs->glBindTexture(GL_TEXTURE_2D, m_textureResource->m_texture);

    funcs->glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, oldWidth, oldHeight);

    funcs->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                     GL_RENDERBUFFER, 0);
    funcs->glDeleteTextures(1, &tmp_texture);
    funcs->glDeleteTextures(1, &oldTexture);

    funcs->glBindFramebuffer(GL_FRAMEBUFFER, ctx->d_ptr->current_fbo);

    if (pex != 0) {
        funcs->glViewport(0, 0, pex->width, pex->height);
        pex->updateClipScissorTest();
    }
}

void QGLTextureGlyphCache::fillTexture(const Coord &c, glyph_t glyph, QFixed subPixelPosition)
{
    QGLContext *ctx = const_cast<QGLContext *>(QGLContext::currentContext());
    if (ctx == 0) {
        qWarning("QGLTextureGlyphCache::fillTexture: Called with no context");
        return;
    }
    QOpenGLFunctions *funcs = ctx->contextHandle()->functions();

    if (!QGLFramebufferObject::hasOpenGLFramebufferObjects() || ctx->d_ptr->workaround_brokenFBOReadBack) {
        QImageTextureGlyphCache::fillTexture(c, glyph, subPixelPosition);

        funcs->glBindTexture(GL_TEXTURE_2D, m_textureResource->m_texture);
        const QImage &texture = image();
        const uchar *bits = texture.constBits();
        bits += c.y * texture.bytesPerLine() + c.x;
        for (int i=0; i<c.h; ++i) {
            funcs->glTexSubImage2D(GL_TEXTURE_2D, 0, c.x, c.y + i, c.w, 1, GL_ALPHA, GL_UNSIGNED_BYTE, bits);
            bits += texture.bytesPerLine();
        }
        return;
    }

    QImage mask = textureMapForGlyph(glyph, subPixelPosition);
    const int maskWidth = mask.width();
    const int maskHeight = mask.height();

    if (mask.format() == QImage::Format_Mono) {
        mask = mask.convertToFormat(QImage::Format_Indexed8);
        for (int y = 0; y < maskHeight; ++y) {
            uchar *src = (uchar *) mask.scanLine(y);
            for (int x = 0; x < maskWidth; ++x)
                src[x] = -src[x]; // convert 0 and 1 into 0 and 255
        }
    } else if (mask.format() == QImage::Format_RGB32) {
        // Make the alpha component equal to the average of the RGB values.
        // This is needed when drawing sub-pixel antialiased text on translucent targets.
        for (int y = 0; y < maskHeight; ++y) {
            quint32 *src = (quint32 *) mask.scanLine(y);
            for (int x = 0; x < maskWidth; ++x) {
                uchar r = src[x] >> 16;
                uchar g = src[x] >> 8;
                uchar b = src[x];
                quint32 avg = (quint32(r) + quint32(g) + quint32(b) + 1) / 3; // "+1" for rounding.
                if (ctx->contextHandle()->isOpenGLES()) {
                    // swizzle the bits to accommodate for the GL_RGBA upload.
                    src[x] = (avg << 24) | (quint32(r) << 0) | (quint32(g) << 8) | (quint32(b) << 16);
                } else {
                    src[x] = (src[x] & 0x00ffffff) | (avg << 24);
                }
            }
        }
    }

    funcs->glBindTexture(GL_TEXTURE_2D, m_textureResource->m_texture);
    if (mask.format() == QImage::Format_RGB32) {
        GLenum format = GL_RGBA;
#if !defined(QT_OPENGL_ES_2)
        if (!ctx->contextHandle()->isOpenGLES())
            format = GL_BGRA;
#endif
        funcs->glTexSubImage2D(GL_TEXTURE_2D, 0, c.x, c.y, maskWidth, maskHeight, format, GL_UNSIGNED_BYTE, mask.bits());
    } else {
        // glTexSubImage2D() might cause some garbage to appear in the texture if the mask width is
        // not a multiple of four bytes. The bug appeared on a computer with 32-bit Windows Vista
        // and nVidia GeForce 8500GT. GL_UNPACK_ALIGNMENT is set to four bytes, 'mask' has a
        // multiple of four bytes per line, and most of the glyph shows up correctly in the
        // texture, which makes me think that this is a driver bug.
        // One workaround is to make sure the mask width is a multiple of four bytes, for instance
        // by converting it to a format with four bytes per pixel. Another is to copy one line at a
        // time.

        if (!ctx->d_ptr->workaround_brokenAlphaTexSubImage_init) {
            // don't know which driver versions exhibit this bug, so be conservative for now
            const QByteArray vendorString(reinterpret_cast<const char*>(funcs->glGetString(GL_VENDOR)));
            ctx->d_ptr->workaround_brokenAlphaTexSubImage = vendorString.indexOf("NVIDIA") >= 0;
            ctx->d_ptr->workaround_brokenAlphaTexSubImage_init = true;
        }

        if (ctx->d_ptr->workaround_brokenAlphaTexSubImage) {
            for (int i = 0; i < maskHeight; ++i)
                funcs->glTexSubImage2D(GL_TEXTURE_2D, 0, c.x, c.y + i, maskWidth, 1, GL_ALPHA, GL_UNSIGNED_BYTE, mask.scanLine(i));
        } else {
            funcs->glTexSubImage2D(GL_TEXTURE_2D, 0, c.x, c.y, maskWidth, maskHeight, GL_ALPHA, GL_UNSIGNED_BYTE, mask.bits());
        }
    }
}

int QGLTextureGlyphCache::glyphPadding() const
{
    return 1;
}

int QGLTextureGlyphCache::maxTextureWidth() const
{
    QGLContext *ctx = const_cast<QGLContext *>(QGLContext::currentContext());
    if (ctx == 0)
        return QImageTextureGlyphCache::maxTextureWidth();
    else
        return ctx->d_ptr->maxTextureSize();
}

int QGLTextureGlyphCache::maxTextureHeight() const
{
    QGLContext *ctx = const_cast<QGLContext *>(QGLContext::currentContext());
    if (ctx == 0)
        return QImageTextureGlyphCache::maxTextureHeight();

    if (ctx->d_ptr->workaround_brokenTexSubImage)
        return qMin(1024, ctx->d_ptr->maxTextureSize());
    else
        return ctx->d_ptr->maxTextureSize();
}

void QGLTextureGlyphCache::clear()
{
    m_textureResource->free();
    m_textureResource = 0;

    m_w = 0;
    m_h = 0;
    m_cx = 0;
    m_cy = 0;
    m_currentRowHeight = 0;
    coords.clear();
}

QT_END_NAMESPACE
