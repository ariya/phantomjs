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

#ifndef QGLBUFFER_H
#define QGLBUFFER_H

#include <QtCore/qscopedpointer.h>
#include <QtOpenGL/qgl.h>

QT_BEGIN_NAMESPACE


class QGLBufferPrivate;

class Q_OPENGL_EXPORT QGLBuffer
{
public:
    enum Type
    {
        VertexBuffer        = 0x8892, // GL_ARRAY_BUFFER
        IndexBuffer         = 0x8893, // GL_ELEMENT_ARRAY_BUFFER
        PixelPackBuffer     = 0x88EB, // GL_PIXEL_PACK_BUFFER
        PixelUnpackBuffer   = 0x88EC  // GL_PIXEL_UNPACK_BUFFER
    };

    QGLBuffer();
    explicit QGLBuffer(QGLBuffer::Type type);
    QGLBuffer(const QGLBuffer &other);
    ~QGLBuffer();

    QGLBuffer &operator=(const QGLBuffer &other);

    enum UsagePattern
    {
        StreamDraw          = 0x88E0, // GL_STREAM_DRAW
        StreamRead          = 0x88E1, // GL_STREAM_READ
        StreamCopy          = 0x88E2, // GL_STREAM_COPY
        StaticDraw          = 0x88E4, // GL_STATIC_DRAW
        StaticRead          = 0x88E5, // GL_STATIC_READ
        StaticCopy          = 0x88E6, // GL_STATIC_COPY
        DynamicDraw         = 0x88E8, // GL_DYNAMIC_DRAW
        DynamicRead         = 0x88E9, // GL_DYNAMIC_READ
        DynamicCopy         = 0x88EA  // GL_DYNAMIC_COPY
    };

    enum Access
    {
        ReadOnly            = 0x88B8, // GL_READ_ONLY
        WriteOnly           = 0x88B9, // GL_WRITE_ONLY
        ReadWrite           = 0x88BA  // GL_READ_WRITE
    };

    QGLBuffer::Type type() const;

    QGLBuffer::UsagePattern usagePattern() const;
    void setUsagePattern(QGLBuffer::UsagePattern value);

    bool create();
    bool isCreated() const;

    void destroy();

    bool bind();
    void release();

    static void release(QGLBuffer::Type type);

    GLuint bufferId() const;

    int size() const;

    bool read(int offset, void *data, int count);
    void write(int offset, const void *data, int count);

    void allocate(const void *data, int count);
    inline void allocate(int count) { allocate(0, count); }

    void *map(QGLBuffer::Access access);
    bool unmap();

private:
    QGLBufferPrivate *d_ptr;

    Q_DECLARE_PRIVATE(QGLBuffer)
};

QT_END_NAMESPACE

#endif
