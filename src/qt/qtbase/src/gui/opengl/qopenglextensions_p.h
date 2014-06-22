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

#ifndef QOPENGL_EXTENSIONS_P_H
#define QOPENGL_EXTENSIONS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Qt OpenGL classes.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "qopenglfunctions.h"

QT_BEGIN_NAMESPACE


#if 0
#ifndef GL_ARB_vertex_buffer_object
typedef ptrdiff_t GLintptrARB;
typedef ptrdiff_t GLsizeiptrARB;
#endif
#endif

#ifndef GL_VERSION_2_0
typedef char GLchar;
#endif

class QOpenGLExtensionsPrivate;

class Q_GUI_EXPORT QOpenGLExtensions : public QOpenGLFunctions
{
    Q_DECLARE_PRIVATE(QOpenGLExtensions)
public:
    QOpenGLExtensions();
    QOpenGLExtensions(QOpenGLContext *context);
    ~QOpenGLExtensions() {}

    enum OpenGLExtension {
        TextureRectangle        = 0x00000001,
        GenerateMipmap          = 0x00000002,
        TextureCompression      = 0x00000004,
        MirroredRepeat          = 0x00000008,
        FramebufferMultisample  = 0x00000010,
        StencilTwoSide          = 0x00000020,
        StencilWrap             = 0x00000040,
        PackedDepthStencil      = 0x00000080,
        NVFloatBuffer           = 0x00000100,
        PixelBufferObject       = 0x00000200,
        FramebufferBlit         = 0x00000400,
        BGRATextureFormat       = 0x00000800,
        DDSTextureCompression   = 0x00001000,
        ETC1TextureCompression  = 0x00002000,
        PVRTCTextureCompression = 0x00004000,
        ElementIndexUint        = 0x00008000,
        Depth24                 = 0x00010000,
        SRGBFrameBuffer         = 0x00020000,
        MapBuffer               = 0x00040000,
        GeometryShaders         = 0x00080000
    };
    Q_DECLARE_FLAGS(OpenGLExtensions, OpenGLExtension)

    OpenGLExtensions openGLExtensions();
    bool hasOpenGLExtension(QOpenGLExtensions::OpenGLExtension extension) const;

    void initializeGLExtensions();

    GLvoid *glMapBuffer(GLenum target, GLenum access);
    GLboolean glUnmapBuffer(GLenum target);

    void glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                           GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                           GLbitfield mask, GLenum filter);

    void glRenderbufferStorageMultisample(GLenum target, GLsizei samples,
                                          GLenum internalFormat,
                                          GLsizei width, GLsizei height);

    void glGetBufferSubData(GLenum target, qopengl_GLintptr offset, qopengl_GLsizeiptr size, GLvoid *data);

private:
    static bool isInitialized(const QOpenGLFunctionsPrivate *d) { return d != 0; }
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QOpenGLExtensions::OpenGLExtensions)

class QOpenGLExtensionsPrivate : public QOpenGLFunctionsPrivate
{
public:
    explicit QOpenGLExtensionsPrivate(QOpenGLContext *ctx);

    GLvoid* (QOPENGLF_APIENTRYP MapBuffer)(GLenum target, GLenum access);
    GLboolean (QOPENGLF_APIENTRYP UnmapBuffer)(GLenum target);
    void (QOPENGLF_APIENTRYP BlitFramebuffer)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                           GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                           GLbitfield mask, GLenum filter);
    void (QOPENGLF_APIENTRYP RenderbufferStorageMultisample)(GLenum target, GLsizei samples,
                                          GLenum internalFormat,
                                          GLsizei width, GLsizei height);
    void (QOPENGLF_APIENTRYP GetBufferSubData)(GLenum target, qopengl_GLintptr offset, qopengl_GLsizeiptr size, GLvoid *data);
};

inline GLvoid *QOpenGLExtensions::glMapBuffer(GLenum target, GLenum access)
{
    Q_D(QOpenGLExtensions);
    Q_ASSERT(QOpenGLExtensions::isInitialized(d));
    GLvoid *result = d->MapBuffer(target, access);
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline GLboolean QOpenGLExtensions::glUnmapBuffer(GLenum target)
{
    Q_D(QOpenGLExtensions);
    Q_ASSERT(QOpenGLExtensions::isInitialized(d));
    GLboolean result = d->UnmapBuffer(target);
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLExtensions::glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                       GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                       GLbitfield mask, GLenum filter)
{
    Q_D(QOpenGLExtensions);
    Q_ASSERT(QOpenGLExtensions::isInitialized(d));
    d->BlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtensions::glRenderbufferStorageMultisample(GLenum target, GLsizei samples,
                                      GLenum internalFormat,
                                      GLsizei width, GLsizei height)
{
    Q_D(QOpenGLExtensions);
    Q_ASSERT(QOpenGLExtensions::isInitialized(d));
    d->RenderbufferStorageMultisample(target, samples, internalFormat, width, height);
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLExtensions::glGetBufferSubData(GLenum target, qopengl_GLintptr offset, qopengl_GLsizeiptr size, GLvoid *data)
{
    Q_D(QOpenGLExtensions);
    Q_ASSERT(QOpenGLExtensions::isInitialized(d));
    d->GetBufferSubData(target, offset, size, data);
    Q_OPENGL_FUNCTIONS_DEBUG
}

QT_END_NAMESPACE

#endif // QOPENGL_EXTENSIONS_P_H
