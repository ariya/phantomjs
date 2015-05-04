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

#ifndef QTEXTUREGLYPHCACHE_GL_P_H
#define QTEXTUREGLYPHCACHE_GL_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <private/qtextureglyphcache_p.h>
#include <private/qgl_p.h>
#include <qglshaderprogram.h>
#include <qglframebufferobject.h>
#include <qopenglfunctions.h>

// #define QT_GL_TEXTURE_GLYPH_CACHE_DEBUG

QT_BEGIN_NAMESPACE

class QGL2PaintEngineExPrivate;

struct QGLGlyphTexture : public QOpenGLSharedResource
{
    QGLGlyphTexture(const QGLContext *ctx)
        : QOpenGLSharedResource(ctx->contextHandle()->shareGroup())
        , m_fbo(0)
        , m_width(0)
        , m_height(0)
    {
        if (ctx && QGLFramebufferObject::hasOpenGLFramebufferObjects() && !ctx->d_ptr->workaround_brokenFBOReadBack)
            ctx->contextHandle()->functions()->glGenFramebuffers(1, &m_fbo);

#ifdef QT_GL_TEXTURE_GLYPH_CACHE_DEBUG
        qDebug(" -> QGLGlyphTexture() %p for context %p.", this, ctx);
#endif
    }

    void freeResource(QOpenGLContext *context)
    {
        const QGLContext *ctx = QGLContext::fromOpenGLContext(context);
#ifdef QT_GL_TEXTURE_GLYPH_CACHE_DEBUG
        qDebug("~QGLGlyphTexture() %p for context %p.", this, ctx);
#else
        Q_UNUSED(ctx);
#endif
        if (ctx && m_fbo)
            ctx->contextHandle()->functions()->glDeleteFramebuffers(1, &m_fbo);
        if (m_width || m_height)
            ctx->contextHandle()->functions()->glDeleteTextures(1, &m_texture);
    }

    void invalidateResource()
    {
        m_texture = 0;
        m_fbo = 0;
        m_width = 0;
        m_height = 0;
    }

    GLuint m_texture;
    GLuint m_fbo;
    int m_width;
    int m_height;
};

class Q_OPENGL_EXPORT QGLTextureGlyphCache : public QImageTextureGlyphCache
{
public:
    QGLTextureGlyphCache(QFontEngine::GlyphFormat format, const QTransform &matrix);
    ~QGLTextureGlyphCache();

    virtual void createTextureData(int width, int height);
    virtual void resizeTextureData(int width, int height);
    virtual void fillTexture(const Coord &c, glyph_t glyph, QFixed subPixelPosition);
    virtual int glyphPadding() const;
    virtual int maxTextureWidth() const;
    virtual int maxTextureHeight() const;

    inline GLuint texture() const {
        QGLTextureGlyphCache *that = const_cast<QGLTextureGlyphCache *>(this);
        QGLGlyphTexture *glyphTexture = that->m_textureResource;
        return glyphTexture ? glyphTexture->m_texture : 0;
    }

    inline int width() const {
        QGLTextureGlyphCache *that = const_cast<QGLTextureGlyphCache *>(this);
        QGLGlyphTexture *glyphTexture = that->m_textureResource;
        return glyphTexture ? glyphTexture->m_width : 0;
    }
    inline int height() const {
        QGLTextureGlyphCache *that = const_cast<QGLTextureGlyphCache *>(this);
        QGLGlyphTexture *glyphTexture = that->m_textureResource;
        return glyphTexture ? glyphTexture->m_height : 0;
    }

    inline void setPaintEnginePrivate(QGL2PaintEngineExPrivate *p) { pex = p; }

    inline const QOpenGLContextGroup *contextGroup() const { return m_textureResource ? m_textureResource->group() : 0; }

    inline int serialNumber() const { return m_serialNumber; }

    enum FilterMode {
        Nearest,
        Linear
    };
    FilterMode filterMode() const { return m_filterMode; }
    void setFilterMode(FilterMode m) { m_filterMode = m; }

    void clear();

private:
    QGLGlyphTexture *m_textureResource;

    QGL2PaintEngineExPrivate *pex;
    QGLShaderProgram *m_blitProgram;
    FilterMode m_filterMode;

    GLfloat m_vertexCoordinateArray[8];
    GLfloat m_textureCoordinateArray[8];

    int m_serialNumber;
};

QT_END_NAMESPACE

#endif

