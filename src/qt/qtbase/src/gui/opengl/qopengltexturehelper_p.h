/****************************************************************************
**
** Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB).
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

#ifndef QOPENGLTEXTUREHELPER_P_H
#define QOPENGLTEXTUREHELPER_P_H

#include <QtCore/qglobal.h>

#ifndef QT_NO_OPENGL

#include "qopengl.h"
#include "qopenglpixeltransferoptions.h"
#include "qopengltexture.h"

QT_BEGIN_NAMESPACE

class QOpenGLContext;

class QOpenGLTextureHelper
{
public:
    QOpenGLTextureHelper(QOpenGLContext *context);

    // DSA-like API. Will either use real DSA or our emulation
    inline void glTextureParameteri(GLuint texture, GLenum target, GLenum bindingTarget, GLenum pname, GLint param)
    {
        (this->*TextureParameteri)(texture, target, bindingTarget, pname, param);
    }

    inline void glTextureParameteriv(GLuint texture, GLenum target, GLenum bindingTarget, GLenum pname, const GLint *params)
    {
        (this->*TextureParameteriv)(texture, target, bindingTarget, pname, params);
    }

    inline void glTextureParameterf(GLuint texture, GLenum target, GLenum bindingTarget, GLenum pname, GLfloat param)
    {
        (this->*TextureParameterf)(texture, target, bindingTarget, pname, param);
    }

    inline void glTextureParameterfv(GLuint texture, GLenum target, GLenum bindingTarget, GLenum pname, const GLfloat *params)
    {
        (this->*TextureParameterfv)(texture, target, bindingTarget, pname, params);
    }

    inline void glGenerateTextureMipmap(GLuint texture, GLenum target, GLenum bindingTarget)
    {
        (this->*GenerateTextureMipmap)(texture, target, bindingTarget);
    }

    inline void glTextureStorage3D(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei levels, GLenum internalFormat,
                                   GLsizei width, GLsizei height, GLsizei depth)
    {
        (this->*TextureStorage3D)(texture, target, bindingTarget, levels, internalFormat, width, height, depth);
    }

    inline void glTextureStorage2D(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei levels, GLenum internalFormat,
                                   GLsizei width, GLsizei height)
    {
        (this->*TextureStorage2D)(texture, target, bindingTarget, levels, internalFormat, width, height);
    }

    inline void glTextureStorage1D(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei levels, GLenum internalFormat,
                                   GLsizei width)
    {
        (this->*TextureStorage1D)(texture, target, bindingTarget, levels, internalFormat, width);
    }

    inline void glTextureStorage3DMultisample(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei samples, GLenum internalFormat,
                                              GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations)
    {
        (this->*TextureStorage3DMultisample)(texture, target, bindingTarget, samples, internalFormat, width, height, depth, fixedSampleLocations);
    }

    inline void glTextureStorage2DMultisample(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei samples, GLenum internalFormat,
                                              GLsizei width, GLsizei height, GLboolean fixedSampleLocations)
    {
        (this->*TextureStorage2DMultisample)(texture, target, bindingTarget, samples, internalFormat, width, height, fixedSampleLocations);
    }

    inline void glTextureImage3D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat,
                                 GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
    {
        (this->*TextureImage3D)(texture, target, bindingTarget, level, internalFormat, width, height, depth, border, format, type, pixels);
    }

    inline void glTextureImage2D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat,
                                 GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
    {
        (this->*TextureImage2D)(texture, target, bindingTarget, level, internalFormat, width, height, border, format, type, pixels);
    }

    inline void glTextureImage1D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat,
                                 GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
    {
        (this->*TextureImage1D)(texture, target, bindingTarget, level, internalFormat, width, border, format, type, pixels);
    }

    inline void glTextureSubImage3D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                                    GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type,
                                    const GLvoid *pixels, const QOpenGLPixelTransferOptions * const options = 0)
    {
        if (options) {
            QOpenGLPixelTransferOptions oldOptions = savePixelUploadOptions();
            setPixelUploadOptions(*options);
            (this->*TextureSubImage3D)(texture, target, bindingTarget, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
            setPixelUploadOptions(oldOptions);
        } else {
            (this->*TextureSubImage3D)(texture, target, bindingTarget, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
        }
    }

    inline void glTextureSubImage2D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLint xoffset, GLint yoffset,
                                    GLsizei width, GLsizei height, GLenum format, GLenum type,
                                    const GLvoid *pixels, const QOpenGLPixelTransferOptions * const options = 0)
    {
        if (options) {
            QOpenGLPixelTransferOptions oldOptions = savePixelUploadOptions();
            setPixelUploadOptions(*options);
            (this->*TextureSubImage2D)(texture, target, bindingTarget, level, xoffset, yoffset, width, height, format, type, pixels);
            setPixelUploadOptions(oldOptions);
        } else {
            (this->*TextureSubImage2D)(texture, target, bindingTarget, level, xoffset, yoffset, width, height, format, type, pixels);
        }
    }

    inline void glTextureSubImage1D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLint xoffset,
                                    GLsizei width, GLenum format, GLenum type,
                                    const GLvoid *pixels, const QOpenGLPixelTransferOptions * const options = 0)
    {
        if (options) {
            QOpenGLPixelTransferOptions oldOptions = savePixelUploadOptions();
            setPixelUploadOptions(*options);
            (this->*TextureSubImage1D)(texture, target, bindingTarget, level, xoffset, width, format, type, pixels);
            setPixelUploadOptions(oldOptions);
        } else {
            (this->*TextureSubImage1D)(texture, target, bindingTarget, level, xoffset, width, format, type, pixels);
        }
    }

    inline void glTextureImage3DMultisample(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei samples, GLint internalFormat,
                                            GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations)
    {
        (this->*TextureImage3DMultisample)(texture, target, bindingTarget, samples, internalFormat, width, height, depth, fixedSampleLocations);
    }

    inline void glTextureImage2DMultisample(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei samples, GLint internalFormat,
                                            GLsizei width, GLsizei height, GLboolean fixedSampleLocations)
    {
        (this->*TextureImage2DMultisample)(texture, target, bindingTarget, samples, internalFormat, width, height, fixedSampleLocations);
    }

    inline void glCompressedTextureSubImage1D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level,
                                              GLint xoffset, GLsizei width,
                                              GLenum format, GLsizei imageSize, const GLvoid *bits,
                                              const QOpenGLPixelTransferOptions * const options = 0)
    {
        if (options) {
            QOpenGLPixelTransferOptions oldOptions = savePixelUploadOptions();
            setPixelUploadOptions(*options);
            (this->*CompressedTextureSubImage1D)(texture, target, bindingTarget, level, xoffset, width, format, imageSize, bits);
            setPixelUploadOptions(oldOptions);
        } else {
            (this->*CompressedTextureSubImage1D)(texture, target, bindingTarget, level, xoffset, width, format, imageSize, bits);
        }
    }

    inline void glCompressedTextureSubImage2D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level,
                                              GLint xoffset, GLint yoffset,
                                              GLsizei width, GLsizei height,
                                              GLenum format, GLsizei imageSize, const GLvoid *bits,
                                              const QOpenGLPixelTransferOptions * const options = 0)
    {
        if (options) {
            QOpenGLPixelTransferOptions oldOptions = savePixelUploadOptions();
            setPixelUploadOptions(*options);
            (this->*CompressedTextureSubImage2D)(texture, target, bindingTarget, level, xoffset, yoffset, width, height, format, imageSize, bits);
            setPixelUploadOptions(oldOptions);
        } else {
            (this->*CompressedTextureSubImage2D)(texture, target, bindingTarget, level, xoffset, yoffset, width, height, format, imageSize, bits);
        }
    }

    inline void glCompressedTextureSubImage3D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level,
                                              GLint xoffset, GLint yoffset, GLint zoffset,
                                              GLsizei width, GLsizei height, GLsizei depth,
                                              GLenum format, GLsizei imageSize, const GLvoid *bits,
                                              const QOpenGLPixelTransferOptions * const options = 0)
    {
        if (options) {
            QOpenGLPixelTransferOptions oldOptions = savePixelUploadOptions();
            setPixelUploadOptions(*options);
            (this->*CompressedTextureSubImage3D)(texture, target, bindingTarget, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits);
            setPixelUploadOptions(oldOptions);
        } else {
            (this->*CompressedTextureSubImage3D)(texture, target, bindingTarget, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits);
        }
    }

    inline void glCompressedTextureImage1D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level,
                                           GLenum internalFormat, GLsizei width,
                                           GLint border, GLsizei imageSize, const GLvoid *bits)
    {
        (this->*CompressedTextureImage1D)(texture, target, bindingTarget, level, internalFormat, width, border, imageSize, bits);
    }

    inline void glCompressedTextureImage2D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level,
                                           GLenum internalFormat, GLsizei width, GLsizei height,
                                           GLint border, GLsizei imageSize, const GLvoid *bits)
    {
        (this->*CompressedTextureImage2D)(texture, target, bindingTarget, level, internalFormat, width, height, border, imageSize, bits);
    }

    inline void glCompressedTextureImage3D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level,
                                           GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth,
                                           GLint border, GLsizei imageSize, const GLvoid *bits)
    {
        (this->*CompressedTextureImage3D)(texture, target, bindingTarget, level, internalFormat, width, height, depth, border, imageSize, bits);
    }

private:
    // DSA wrapper (so we can use pointer to member function as switch)
    void dsa_TextureParameteri(GLuint texture, GLenum target, GLenum bindingTarget, GLenum pname, GLint param);

    void dsa_TextureParameteriv(GLuint texture, GLenum target, GLenum bindingTarget, GLenum pname, const GLint *params);

    void dsa_TextureParameterf(GLuint texture, GLenum target, GLenum bindingTarget, GLenum pname, GLfloat param);

    void dsa_TextureParameterfv(GLuint texture, GLenum target, GLenum bindingTarget, GLenum pname, const GLfloat *params);

    void dsa_GenerateTextureMipmap(GLuint texture, GLenum target, GLenum bindingTarget);

    void dsa_TextureStorage3D(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei levels, GLenum internalFormat,
                              GLsizei width, GLsizei height, GLsizei depth);

    void dsa_TextureStorage2D(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei levels, GLenum internalFormat,
                              GLsizei width, GLsizei height);

    void dsa_TextureStorage1D(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei levels, GLenum internalFormat,
                              GLsizei width);

    void dsa_TextureStorage3DMultisample(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei samples, GLenum internalFormat,
                                         GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations);

    void dsa_TextureStorage2DMultisample(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei samples, GLenum internalFormat,
                                         GLsizei width, GLsizei height, GLboolean fixedSampleLocations);

    void dsa_TextureImage3D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat,
                            GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);

    void dsa_TextureImage2D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat,
                            GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);

    void dsa_TextureImage1D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat,
                            GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);

    void dsa_TextureSubImage3D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                               GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);

    void dsa_TextureSubImage2D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLint xoffset, GLint yoffset,
                               GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);

    void dsa_TextureSubImage1D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLint xoffset,
                               GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);

    void dsa_TextureImage3DMultisample(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei samples, GLint internalFormat,
                                       GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations);

    void dsa_TextureImage2DMultisample(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei samples, GLint internalFormat,
                                       GLsizei width, GLsizei height, GLboolean fixedSampleLocations);

    void dsa_CompressedTextureSubImage1D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level,
                                         GLint xoffset, GLsizei width,
                                         GLenum format, GLsizei imageSize, const GLvoid *bits);

    void dsa_CompressedTextureSubImage2D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level,
                                         GLint xoffset, GLint yoffset,
                                         GLsizei width, GLsizei height,
                                         GLenum format, GLsizei imageSize, const GLvoid *bits);

    void dsa_CompressedTextureSubImage3D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level,
                                         GLint xoffset, GLint yoffset, GLint zoffset,
                                         GLsizei width, GLsizei height, GLsizei depth,
                                         GLenum format, GLsizei imageSize, const GLvoid *bits);

    void dsa_CompressedTextureImage1D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level,
                                      GLenum internalFormat, GLsizei width,
                                      GLint border, GLsizei imageSize, const GLvoid *bits);

    void dsa_CompressedTextureImage2D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level,
                                      GLenum internalFormat, GLsizei width, GLsizei height,
                                      GLint border, GLsizei imageSize, const GLvoid *bits);

    void dsa_CompressedTextureImage3D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level,
                                      GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth,
                                      GLint border, GLsizei imageSize, const GLvoid *bits);

    // DSA emulation API
    void qt_TextureParameteri(GLuint texture, GLenum target, GLenum bindingTarget, GLenum pname, GLint param);

    void qt_TextureParameteriv(GLuint texture, GLenum target, GLenum bindingTarget, GLenum pname, const GLint *params);

    void qt_TextureParameterf(GLuint texture, GLenum target, GLenum bindingTarget, GLenum pname, GLfloat param);

    void qt_TextureParameterfv(GLuint texture, GLenum target, GLenum bindingTarget, GLenum pname, const GLfloat *params);

    void qt_GenerateTextureMipmap(GLuint texture, GLenum target, GLenum bindingTarget);

    void qt_TextureStorage3D(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei levels,
                             GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth);

    void qt_TextureStorage2D(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei levels,
                             GLenum internalFormat, GLsizei width, GLsizei height);

    void qt_TextureStorage1D(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei levels,
                             GLenum internalFormat, GLsizei width);

    void qt_TextureStorage3DMultisample(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei samples,
                                        GLenum internalFormat, GLsizei width, GLsizei height,
                                        GLsizei depth, GLboolean fixedSampleLocations);

    void qt_TextureStorage2DMultisample(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei samples,
                                        GLenum internalFormat, GLsizei width, GLsizei height,
                                        GLboolean fixedSampleLocations);

    void qt_TextureImage3D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat,
                           GLsizei width, GLsizei height, GLsizei depth,
                           GLint border, GLenum format, GLenum type,
                           const GLvoid *pixels);

    void qt_TextureImage2D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat,
                           GLsizei width, GLsizei height,
                           GLint border, GLenum format, GLenum type,
                           const GLvoid *pixels);

    void qt_TextureImage1D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat,
                           GLsizei width, GLint border, GLenum format, GLenum type,
                           const GLvoid *pixels);

    void qt_TextureSubImage3D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level,
                              GLint xoffset, GLint yoffset, GLint zoffset,
                              GLsizei width, GLsizei height, GLsizei depth,
                              GLenum format, GLenum type, const GLvoid *pixels);

    void qt_TextureSubImage2D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level,
                              GLint xoffset, GLint yoffset,
                              GLsizei width, GLsizei height,
                              GLenum format, GLenum type, const GLvoid *pixels);

    void qt_TextureSubImage1D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level,
                              GLint xoffset, GLsizei width,
                              GLenum format, GLenum type, const GLvoid *pixels);

    void qt_TextureImage3DMultisample(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei samples,
                                      GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth,
                                      GLboolean fixedSampleLocations);

    void qt_TextureImage2DMultisample(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei samples,
                                      GLint internalFormat, GLsizei width, GLsizei height,
                                      GLboolean fixedSampleLocations);

    void qt_CompressedTextureSubImage1D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level,
                                        GLint xoffset, GLsizei width, GLenum format,
                                        GLsizei imageSize, const GLvoid *bits);

    void qt_CompressedTextureSubImage2D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level,
                                        GLint xoffset, GLint yoffset,
                                        GLsizei width, GLsizei height,
                                        GLenum format, GLsizei imageSize, const GLvoid *bits);

    void qt_CompressedTextureSubImage3D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level,
                                        GLint xoffset, GLint yoffset, GLint zoffset,
                                        GLsizei width, GLsizei height, GLsizei depth,
                                        GLenum format, GLsizei imageSize, const GLvoid *bits);

    void qt_CompressedTextureImage1D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat,
                                     GLsizei width, GLint border,
                                     GLsizei imageSize, const GLvoid *bits);

    void qt_CompressedTextureImage2D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat,
                                     GLsizei width, GLsizei height, GLint border,
                                     GLsizei imageSize, const GLvoid *bits);

    void qt_CompressedTextureImage3D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat,
                                     GLsizei width, GLsizei height, GLsizei depth, GLint border,
                                     GLsizei imageSize, const GLvoid *bits);

public:
    // Raw OpenGL functions, resolved and used by our DSA-like static functions if no EXT_direct_state_access is available
    // OpenGL 1.0
    inline void glGetIntegerv(GLenum pname, GLint *params)
    {
        GetIntegerv(pname, params);
    }

    inline void glGetBooleanv(GLenum pname, GLboolean *params)
    {
        GetBooleanv(pname, params);
    }

    inline void glPixelStorei(GLenum pname, GLint param)
    {
        PixelStorei(pname, param);
    }

    inline void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params)
    {
        GetTexLevelParameteriv(target, level, pname, params);
    }

    inline void glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params)
    {
        GetTexLevelParameterfv(target, level, pname, params);
    }

    inline void glGetTexParameteriv(GLenum target, GLenum pname, GLint *params)
    {
        GetTexParameteriv(target, pname, params);
    }

    inline void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params)
    {
        GetTexParameterfv(target, pname, params);
    }

    inline void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels)
    {
        GetTexImage(target, level, format, type, pixels);
    }

    inline void glTexImage2D(GLenum target, GLint level, GLint internalFormat,
                             GLsizei width, GLsizei height, GLint border,
                             GLenum format, GLenum type, const GLvoid *pixels)
    {
        TexImage2D(target, level, internalFormat, width, height, border, format, type, pixels);
    }

    inline void glTexImage1D(GLenum target, GLint level, GLint internalFormat,
                             GLsizei width, GLint border,
                             GLenum format, GLenum type, const GLvoid *pixels)
    {
        TexImage1D(target, level, internalFormat, width, border, format, type, pixels);
    }

    inline void glTexParameteriv(GLenum target, GLenum pname, const GLint *params)
    {
        TexParameteriv(target, pname, params);
    }

    inline void glTexParameteri(GLenum target, GLenum pname, GLint param)
    {
        TexParameteri(target, pname, param);
    }

    inline void glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params)
    {
        TexParameterfv(target, pname, params);
    }

    inline void glTexParameterf(GLenum target, GLenum pname, GLfloat param)
    {
        TexParameterf(target, pname, param);
    }

    // OpenGL 1.1
    inline void glGenTextures(GLsizei n, GLuint *textures)
    {
        GenTextures(n, textures);
    }

    inline void glDeleteTextures(GLsizei n, const GLuint *textures)
    {
        DeleteTextures(n, textures);
    }

    inline void glBindTexture(GLenum target, GLuint texture)
    {
        BindTexture(target, texture);
    }

    inline void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
    {
        TexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
    }

    inline void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width,
                                GLenum format, GLenum type, const GLvoid *pixels)
    {
        TexSubImage1D(target, level, xoffset, width, format, type, pixels);
    }

    // OpenGL 1.2
    inline void glTexImage3D(GLenum target, GLint level, GLint internalFormat,
                             GLsizei width, GLsizei height, GLsizei depth, GLint border,
                             GLenum format, GLenum type, const GLvoid *pixels)
    {
        TexImage3D(target, level, internalFormat, width, height, depth, border, format, type, pixels);
    }

    inline void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                                GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels)
    {
        TexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
    }

    // OpenGL 1.3
    inline void glGetCompressedTexImage(GLenum target, GLint level, GLvoid *img)
    {
        GetCompressedTexImage(target, level, img);
    }

    inline void glCompressedTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width,
                                          GLenum format, GLsizei imageSize, const GLvoid *data)
    {
        CompressedTexSubImage1D(target, level, xoffset, width, format, imageSize, data);
    }

    inline void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                          GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data)
    {
        CompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
    }

    inline void glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset,
                                          GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *data)
    {
        CompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data);
    }

    inline void glCompressedTexImage1D(GLenum target, GLint level, GLenum internalFormat, GLsizei width,
                                       GLint border, GLsizei imageSize, const GLvoid *data)
    {
        CompressedTexImage1D(target, level, internalFormat, width, border, imageSize, data);
    }

    inline void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height,
                                       GLint border, GLsizei imageSize, const GLvoid *data)
    {
        CompressedTexImage2D(target, level, internalFormat, width, height, border, imageSize, data);
    }

    inline void glCompressedTexImage3D(GLenum target, GLint level, GLenum internalFormat,
                                       GLsizei width, GLsizei height, GLsizei depth,
                                       GLint border, GLsizei imageSize, const GLvoid *data)
    {
        CompressedTexImage3D(target, level, internalFormat, width, height, depth, border, imageSize, data);
    }

    inline void glActiveTexture(GLenum texture)
    {
        ActiveTexture(texture);
    }

    // OpenGL 3.0
    inline void glGenerateMipmap(GLenum target)
    {
        GenerateMipmap(target);
    }

    // OpenGL 3.2
    inline void glTexImage3DMultisample(GLenum target, GLsizei samples, GLint internalFormat,
                                        GLsizei width, GLsizei height, GLsizei depth,
                                        GLboolean fixedSampleLocations)
    {
        TexImage3DMultisample(target, samples, internalFormat, width, height, depth, fixedSampleLocations);
    }

    inline void glTexImage2DMultisample(GLenum target, GLsizei samples, GLint internalFormat,
                                        GLsizei width, GLsizei height,
                                        GLboolean fixedSampleLocations)
    {
        TexImage2DMultisample(target, samples, internalFormat, width, height, fixedSampleLocations);
    }

    // OpenGL 4.2
    inline void glTexStorage3D(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth)
    {
        TexStorage3D(target, levels, internalFormat, width, height, depth);
    }

    inline void glTexStorage2D(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height)
    {
        TexStorage2D(target, levels, internalFormat, width, height);
    }

    inline void glTexStorage1D(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width)
    {
        TexStorage1D(target, levels, internalFormat, width);
    }

    // OpenGL 4.3
    inline void glTexStorage3DMultisample(GLenum target, GLsizei samples, GLenum internalFormat,
                                          GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations)
    {
        TexStorage3DMultisample(target, samples, internalFormat, width, height, depth, fixedSampleLocations);
    }

    inline void glTexStorage2DMultisample(GLenum target, GLsizei samples, GLenum internalFormat,
                                          GLsizei width, GLsizei height, GLboolean fixedSampleLocations)
    {
        TexStorage2DMultisample(target, samples, internalFormat, width, height, fixedSampleLocations);
    }

    inline void glTexBufferRange(GLenum target, GLenum internalFormat, GLuint buffer,
                                 GLintptr offset, GLsizeiptr size)
    {
        TexBufferRange(target, internalFormat, buffer, offset, size);
    }

    inline void glTextureView(GLuint texture, GLenum target, GLuint origTexture, GLenum internalFormat,
                              GLuint minLevel, GLuint numLevels, GLuint minLayer, GLuint numLayers)
    {
        TextureView(texture, target, origTexture, internalFormat, minLevel, numLevels, minLayer, numLayers);
    }

    // Helper functions
    inline QOpenGLPixelTransferOptions savePixelUploadOptions()
    {
        QOpenGLPixelTransferOptions options;
        int val = 0;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &val);
        options.setAlignment(val);
#if !defined(QT_OPENGL_ES_2)
        glGetIntegerv(GL_UNPACK_SKIP_IMAGES, &val);
        options.setSkipImages(val);
        glGetIntegerv(GL_UNPACK_SKIP_ROWS, &val);
        options.setSkipRows(val);
        glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &val);
        options.setSkipPixels(val);
        glGetIntegerv(GL_UNPACK_IMAGE_HEIGHT, &val);
        options.setImageHeight(val);
        glGetIntegerv(GL_UNPACK_ROW_LENGTH, &val);
        options.setRowLength(val);
        GLboolean b = GL_FALSE;
        glGetBooleanv(GL_UNPACK_LSB_FIRST, &b);
        options.setLeastSignificantByteFirst(b);
        glGetBooleanv(GL_UNPACK_SWAP_BYTES, &b);
        options.setSwapBytesEnabled(b);
#endif
        return options;
    }

    inline void setPixelUploadOptions(const QOpenGLPixelTransferOptions &options)
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, options.alignment());
#if !defined(QT_OPENGL_ES_2)
        glPixelStorei(GL_UNPACK_SKIP_IMAGES, options.skipImages());
        glPixelStorei(GL_UNPACK_SKIP_ROWS, options.skipRows());
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, options.skipPixels());
        glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, options.imageHeight());
        glPixelStorei(GL_UNPACK_ROW_LENGTH, options.rowLength());
        glPixelStorei(GL_UNPACK_LSB_FIRST, options.isLeastSignificantBitFirst());
        glPixelStorei(GL_UNPACK_SWAP_BYTES, options.isSwapBytesEnabled());
#endif
    }

private:
    // Typedefs and pointers to member functions used to switch between EXT_direct_state_access and our own emulated DSA.
    // The argument match the corresponding GL function, but there's an extra "GLenum bindingTarget" which gets used with
    // the DSA emulation -- it contains the right GL_BINDING_TEXTURE_X to use.
    typedef void (QOpenGLTextureHelper::*TextureParameteriMemberFunc)(GLuint texture, GLenum target, GLenum bindingTarget, GLenum pname, GLint param);
    typedef void (QOpenGLTextureHelper::*TextureParameterivMemberFunc)(GLuint texture, GLenum target, GLenum bindingTarget, GLenum pname, const GLint *params);
    typedef void (QOpenGLTextureHelper::*TextureParameterfMemberFunc)(GLuint texture, GLenum target, GLenum bindingTarget, GLenum pname, GLfloat param);
    typedef void (QOpenGLTextureHelper::*TextureParameterfvMemberFunc)(GLuint texture, GLenum target, GLenum bindingTarget, GLenum pname, const GLfloat *params);
    typedef void (QOpenGLTextureHelper::*GenerateTextureMipmapMemberFunc)(GLuint texture, GLenum target, GLenum bindingTarget);
    typedef void (QOpenGLTextureHelper::*TextureStorage3DMemberFunc)(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth);
    typedef void (QOpenGLTextureHelper::*TextureStorage2DMemberFunc)(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height);
    typedef void (QOpenGLTextureHelper::*TextureStorage1DMemberFunc)(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei levels, GLenum internalFormat, GLsizei width);
    typedef void (QOpenGLTextureHelper::*TextureStorage3DMultisampleMemberFunc)(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations);
    typedef void (QOpenGLTextureHelper::*TextureStorage2DMultisampleMemberFunc)(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations);
    typedef void (QOpenGLTextureHelper::*TextureImage3DMemberFunc)(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
    typedef void (QOpenGLTextureHelper::*TextureImage2DMemberFunc)(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
    typedef void (QOpenGLTextureHelper::*TextureImage1DMemberFunc)(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
    typedef void (QOpenGLTextureHelper::*TextureSubImage3DMemberFunc)(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);
    typedef void (QOpenGLTextureHelper::*TextureSubImage2DMemberFunc)(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
    typedef void (QOpenGLTextureHelper::*TextureSubImage1DMemberFunc)(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
    typedef void (QOpenGLTextureHelper::*TextureImage3DMultisampleMemberFunc)(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations);
    typedef void (QOpenGLTextureHelper::*TextureImage2DMultisampleMemberFunc)(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations);
    typedef void (QOpenGLTextureHelper::*CompressedTextureSubImage1DMemberFunc)(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *bits);
    typedef void (QOpenGLTextureHelper::*CompressedTextureSubImage2DMemberFunc)(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *bits);
    typedef void (QOpenGLTextureHelper::*CompressedTextureSubImage3DMemberFunc)(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *bits);
    typedef void (QOpenGLTextureHelper::*CompressedTextureImage1DMemberFunc)(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *bits);
    typedef void (QOpenGLTextureHelper::*CompressedTextureImage2DMemberFunc)(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *bits);
    typedef void (QOpenGLTextureHelper::*CompressedTextureImage3DMemberFunc)(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *bits);


    TextureParameteriMemberFunc TextureParameteri;
    TextureParameterivMemberFunc TextureParameteriv;
    TextureParameterfMemberFunc TextureParameterf;
    TextureParameterfvMemberFunc TextureParameterfv;
    GenerateTextureMipmapMemberFunc GenerateTextureMipmap;
    TextureStorage3DMemberFunc TextureStorage3D;
    TextureStorage2DMemberFunc TextureStorage2D;
    TextureStorage1DMemberFunc TextureStorage1D;
    TextureStorage3DMultisampleMemberFunc TextureStorage3DMultisample;
    TextureStorage2DMultisampleMemberFunc TextureStorage2DMultisample;
    TextureImage3DMemberFunc TextureImage3D;
    TextureImage2DMemberFunc TextureImage2D;
    TextureImage1DMemberFunc TextureImage1D;
    TextureSubImage3DMemberFunc TextureSubImage3D;
    TextureSubImage2DMemberFunc TextureSubImage2D;
    TextureSubImage1DMemberFunc TextureSubImage1D;
    TextureImage3DMultisampleMemberFunc TextureImage3DMultisample;
    TextureImage2DMultisampleMemberFunc TextureImage2DMultisample;
    CompressedTextureSubImage1DMemberFunc CompressedTextureSubImage1D;
    CompressedTextureSubImage2DMemberFunc CompressedTextureSubImage2D;
    CompressedTextureSubImage3DMemberFunc CompressedTextureSubImage3D;
    CompressedTextureImage1DMemberFunc CompressedTextureImage1D;
    CompressedTextureImage2DMemberFunc CompressedTextureImage2D;
    CompressedTextureImage3DMemberFunc CompressedTextureImage3D;

    // Raw function pointers for core and DSA functions

    // EXT_direct_state_access used when DSA is available
    void (QOPENGLF_APIENTRYP TextureParameteriEXT)(GLuint texture, GLenum target, GLenum pname, GLint param);
    void (QOPENGLF_APIENTRYP TextureParameterivEXT)(GLuint texture, GLenum target, GLenum pname, const GLint *params);
    void (QOPENGLF_APIENTRYP TextureParameterfEXT)(GLuint texture, GLenum target, GLenum pname, GLfloat param);
    void (QOPENGLF_APIENTRYP TextureParameterfvEXT)(GLuint texture, GLenum target, GLenum pname, const GLfloat *params);
    void (QOPENGLF_APIENTRYP GenerateTextureMipmapEXT)(GLuint texture, GLenum target);
    void (QOPENGLF_APIENTRYP TextureStorage3DEXT)(GLuint texture, GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth);
    void (QOPENGLF_APIENTRYP TextureStorage2DEXT)(GLuint texture, GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height);
    void (QOPENGLF_APIENTRYP TextureStorage1DEXT)(GLuint texture, GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width);
    void (QOPENGLF_APIENTRYP TextureStorage3DMultisampleEXT)(GLuint texture, GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations);
    void (QOPENGLF_APIENTRYP TextureStorage2DMultisampleEXT)(GLuint texture, GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations);
    void (QOPENGLF_APIENTRYP TextureImage3DEXT)(GLuint texture, GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
    void (QOPENGLF_APIENTRYP TextureImage2DEXT)(GLuint texture, GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
    void (QOPENGLF_APIENTRYP TextureImage1DEXT)(GLuint texture, GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
    void (QOPENGLF_APIENTRYP TextureSubImage3DEXT)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);
    void (QOPENGLF_APIENTRYP TextureSubImage2DEXT)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
    void (QOPENGLF_APIENTRYP TextureSubImage1DEXT)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
    void (QOPENGLF_APIENTRYP CompressedTextureSubImage1DEXT)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *bits);
    void (QOPENGLF_APIENTRYP CompressedTextureSubImage2DEXT)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *bits);
    void (QOPENGLF_APIENTRYP CompressedTextureSubImage3DEXT)(GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *bits);
    void (QOPENGLF_APIENTRYP CompressedTextureImage1DEXT)(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *bits);
    void (QOPENGLF_APIENTRYP CompressedTextureImage2DEXT)(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *bits);
    void (QOPENGLF_APIENTRYP CompressedTextureImage3DEXT)(GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *bits);


    // Plus some missing ones that are in the NV_texture_multisample extension instead
    void (QOPENGLF_APIENTRYP TextureImage3DMultisampleNV)(GLuint texture, GLenum target, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations);
    void (QOPENGLF_APIENTRYP TextureImage2DMultisampleNV)(GLuint texture, GLenum target, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations);

    // OpenGL 1.0
    void (QOPENGLF_APIENTRYP GetIntegerv)(GLenum pname, GLint *params);
    void (QOPENGLF_APIENTRYP GetBooleanv)(GLenum pname, GLboolean *params);
    void (QOPENGLF_APIENTRYP PixelStorei)(GLenum pname, GLint param);
    void (QOPENGLF_APIENTRYP GetTexLevelParameteriv)(GLenum target, GLint level, GLenum pname, GLint *params);
    void (QOPENGLF_APIENTRYP GetTexLevelParameterfv)(GLenum target, GLint level, GLenum pname, GLfloat *params);
    void (QOPENGLF_APIENTRYP GetTexParameteriv)(GLenum target, GLenum pname, GLint *params);
    void (QOPENGLF_APIENTRYP GetTexParameterfv)(GLenum target, GLenum pname, GLfloat *params);
    void (QOPENGLF_APIENTRYP GetTexImage)(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
    void (QOPENGLF_APIENTRYP TexImage2D)(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
    void (QOPENGLF_APIENTRYP TexImage1D)(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
    void (QOPENGLF_APIENTRYP TexParameteriv)(GLenum target, GLenum pname, const GLint *params);
    void (QOPENGLF_APIENTRYP TexParameteri)(GLenum target, GLenum pname, GLint param);
    void (QOPENGLF_APIENTRYP TexParameterfv)(GLenum target, GLenum pname, const GLfloat *params);
    void (QOPENGLF_APIENTRYP TexParameterf)(GLenum target, GLenum pname, GLfloat param);

    // OpenGL 1.1
    void (QOPENGLF_APIENTRYP GenTextures)(GLsizei n, GLuint *textures);
    void (QOPENGLF_APIENTRYP DeleteTextures)(GLsizei n, const GLuint *textures);
    void (QOPENGLF_APIENTRYP BindTexture)(GLenum target, GLuint texture);
    void (QOPENGLF_APIENTRYP TexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
    void (QOPENGLF_APIENTRYP TexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);

    // OpenGL 1.2
    void (QOPENGLF_APIENTRYP TexImage3D)(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
    void (QOPENGLF_APIENTRYP TexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);

    // OpenGL 1.3
    void (QOPENGLF_APIENTRYP GetCompressedTexImage)(GLenum target, GLint level, GLvoid *img);
    void (QOPENGLF_APIENTRYP CompressedTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *data);
    void (QOPENGLF_APIENTRYP CompressedTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data);
    void (QOPENGLF_APIENTRYP CompressedTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *data);
    void (QOPENGLF_APIENTRYP CompressedTexImage1D)(GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *data);
    void (QOPENGLF_APIENTRYP CompressedTexImage2D)(GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);
    void (QOPENGLF_APIENTRYP CompressedTexImage3D)(GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data);
    void (QOPENGLF_APIENTRYP ActiveTexture)(GLenum texture);

    // OpenGL 3.0
    void (QOPENGLF_APIENTRYP GenerateMipmap)(GLenum target);

    // OpenGL 3.2
    void (QOPENGLF_APIENTRYP TexImage3DMultisample)(GLenum target, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations);
    void (QOPENGLF_APIENTRYP TexImage2DMultisample)(GLenum target, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations);

    // OpenGL 4.2
    void (QOPENGLF_APIENTRYP TexStorage3D)(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth);
    void (QOPENGLF_APIENTRYP TexStorage2D)(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height);
    void (QOPENGLF_APIENTRYP TexStorage1D)(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width);

    // OpenGL 4.3
    void (QOPENGLF_APIENTRYP TexStorage3DMultisample)(GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations);
    void (QOPENGLF_APIENTRYP TexStorage2DMultisample)(GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations);
    void (QOPENGLF_APIENTRYP TexBufferRange)(GLenum target, GLenum internalFormat, GLuint buffer, GLintptr offset, GLsizeiptr size);
    void (QOPENGLF_APIENTRYP TextureView)(GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers);
};

QT_END_NAMESPACE

#undef Q_CALL_MEMBER_FUNCTION

#endif // QT_NO_OPENGL

#endif // QOPENGLTEXTUREHELPER_P_H
