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

#ifndef QGLFRAMEBUFFEROBJECT_H
#define QGLFRAMEBUFFEROBJECT_H

#include <QtOpenGL/qgl.h>
#include <QtGui/qpaintdevice.h>

QT_BEGIN_NAMESPACE


class QGLFramebufferObjectPrivate;
class QGLFramebufferObjectFormat;

class Q_OPENGL_EXPORT QGLFramebufferObject : public QPaintDevice
{
    Q_DECLARE_PRIVATE(QGLFramebufferObject)
public:
    enum Attachment {
        NoAttachment,
        CombinedDepthStencil,
        Depth
    };

    QGLFramebufferObject(const QSize &size, GLenum target = GL_TEXTURE_2D);
    QGLFramebufferObject(int width, int height, GLenum target = GL_TEXTURE_2D);

    QGLFramebufferObject(const QSize &size, Attachment attachment,
                         GLenum target = GL_TEXTURE_2D, GLenum internal_format = 0);
    QGLFramebufferObject(int width, int height, Attachment attachment,
                         GLenum target = GL_TEXTURE_2D, GLenum internal_format = 0);

    QGLFramebufferObject(const QSize &size, const QGLFramebufferObjectFormat &format);
    QGLFramebufferObject(int width, int height, const QGLFramebufferObjectFormat &format);

    virtual ~QGLFramebufferObject();

    QGLFramebufferObjectFormat format() const;

    bool isValid() const;
    bool isBound() const;
    bool bind();
    bool release();

    GLuint texture() const;
    QSize size() const;
    QImage toImage() const;
    Attachment attachment() const;

    QPaintEngine *paintEngine() const;
    GLuint handle() const;

    static bool bindDefault();

    static bool hasOpenGLFramebufferObjects();

    void drawTexture(const QRectF &target, GLuint textureId, GLenum textureTarget = GL_TEXTURE_2D);
    void drawTexture(const QPointF &point, GLuint textureId, GLenum textureTarget = GL_TEXTURE_2D);

    static bool hasOpenGLFramebufferBlit();
    static void blitFramebuffer(QGLFramebufferObject *target, const QRect &targetRect,
                                QGLFramebufferObject *source, const QRect &sourceRect,
                                GLbitfield buffers = GL_COLOR_BUFFER_BIT,
                                GLenum filter = GL_NEAREST);

protected:
    int metric(PaintDeviceMetric metric) const;
    int devType() const { return QInternal::FramebufferObject; }

private:
    Q_DISABLE_COPY(QGLFramebufferObject)
    QScopedPointer<QGLFramebufferObjectPrivate> d_ptr;
    friend class QGLPaintDevice;
    friend class QGLFBOGLPaintDevice;
};

class QGLFramebufferObjectFormatPrivate;
class Q_OPENGL_EXPORT QGLFramebufferObjectFormat
{
public:
    QGLFramebufferObjectFormat();
    QGLFramebufferObjectFormat(const QGLFramebufferObjectFormat &other);
    QGLFramebufferObjectFormat &operator=(const QGLFramebufferObjectFormat &other);
    ~QGLFramebufferObjectFormat();

    void setSamples(int samples);
    int samples() const;

    void setMipmap(bool enabled);
    bool mipmap() const;

    void setAttachment(QGLFramebufferObject::Attachment attachment);
    QGLFramebufferObject::Attachment attachment() const;

    void setTextureTarget(GLenum target);
    GLenum textureTarget() const;

    void setInternalTextureFormat(GLenum internalTextureFormat);
    GLenum internalTextureFormat() const;

    bool operator==(const QGLFramebufferObjectFormat& other) const;
    bool operator!=(const QGLFramebufferObjectFormat& other) const;

private:
    QGLFramebufferObjectFormatPrivate *d;

    void detach();
};

QT_END_NAMESPACE

#endif // QGLFRAMEBUFFEROBJECT_H
