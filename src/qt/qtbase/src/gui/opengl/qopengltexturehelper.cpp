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

#include "qopengltexturehelper_p.h"

#include <QOpenGLContext>

QT_BEGIN_NAMESPACE

QOpenGLTextureHelper::QOpenGLTextureHelper(QOpenGLContext *context)
{
    // Resolve EXT_direct_state_access entry points if present
    if (!context->isOpenGLES()
        && context->hasExtension(QByteArrayLiteral("GL_EXT_direct_state_access"))) {
        TextureParameteriEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLenum , GLint )>(context->getProcAddress(QByteArrayLiteral("glTextureParameteriEXT")));
        TextureParameterivEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLenum , const GLint *)>(context->getProcAddress(QByteArrayLiteral("glTextureParameterivEXT")));
        TextureParameterfEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLenum , GLfloat )>(context->getProcAddress(QByteArrayLiteral("glTextureParameterfEXT")));
        TextureParameterfvEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLenum , const GLfloat *)>(context->getProcAddress(QByteArrayLiteral("glTextureParameterfvEXT")));
        GenerateTextureMipmapEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum )>(context->getProcAddress(QByteArrayLiteral("glGenerateTextureMipmapEXT")));
        TextureStorage3DEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLsizei , GLenum , GLsizei , GLsizei , GLsizei )>(context->getProcAddress(QByteArrayLiteral("glTextureStorage3DEXT")));
        TextureStorage2DEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLsizei , GLenum , GLsizei , GLsizei )>(context->getProcAddress(QByteArrayLiteral("glTextureStorage2DEXT")));
        TextureStorage1DEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLsizei , GLenum , GLsizei )>(context->getProcAddress(QByteArrayLiteral("glTextureStorage1DEXT")));
        TextureStorage3DMultisampleEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLsizei , GLenum , GLsizei , GLsizei , GLsizei , GLboolean )>(context->getProcAddress(QByteArrayLiteral("glTextureStorage3DMultisampleEXT")));
        TextureStorage2DMultisampleEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLsizei , GLenum , GLsizei , GLsizei , GLboolean )>(context->getProcAddress(QByteArrayLiteral("glTextureStorage2DMultisampleEXT")));
        TextureImage3DEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLint , GLenum , GLsizei , GLsizei , GLsizei , GLint , GLenum , GLenum , const GLvoid *)>(context->getProcAddress(QByteArrayLiteral("glTextureImage3DEXT")));
        TextureImage2DEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLint , GLenum , GLsizei , GLsizei , GLint , GLenum , GLenum , const GLvoid *)>(context->getProcAddress(QByteArrayLiteral("glTextureImage2DEXT")));
        TextureImage1DEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLint , GLenum , GLsizei , GLint , GLenum , GLenum , const GLvoid *)>(context->getProcAddress(QByteArrayLiteral("glTextureImage1DEXT")));
        TextureSubImage3DEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLint , GLint , GLint , GLint , GLsizei , GLsizei , GLsizei , GLenum , GLenum , const GLvoid *)>(context->getProcAddress(QByteArrayLiteral("glTextureSubImage3DEXT")));
        TextureSubImage2DEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLint , GLint , GLint , GLsizei , GLsizei , GLenum , GLenum , const GLvoid *)>(context->getProcAddress(QByteArrayLiteral("glTextureSubImage2DEXT")));
        TextureSubImage1DEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLint , GLint , GLsizei , GLenum , GLenum , const GLvoid *)>(context->getProcAddress(QByteArrayLiteral("glTextureSubImage1DEXT")));
        CompressedTextureSubImage1DEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLint , GLint , GLsizei , GLenum , GLsizei , const GLvoid *)>(context->getProcAddress(QByteArrayLiteral("glCompressedTextureSubImage1DEXT")));
        CompressedTextureSubImage2DEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLint , GLint , GLint , GLsizei , GLsizei , GLenum , GLsizei , const GLvoid *)>(context->getProcAddress(QByteArrayLiteral("glCompressedTextureSubImage2DEXT")));
        CompressedTextureSubImage3DEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLint , GLint , GLint , GLint , GLsizei , GLsizei , GLsizei , GLenum , GLsizei , const GLvoid *)>(context->getProcAddress(QByteArrayLiteral("glCompressedTextureSubImage3DEXT")));
        CompressedTextureImage1DEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLint , GLenum , GLsizei , GLint , GLsizei , const GLvoid *)>(context->getProcAddress(QByteArrayLiteral("glCompressedTextureImage1DEXT")));
        CompressedTextureImage2DEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLint , GLenum , GLsizei , GLsizei , GLint , GLsizei , const GLvoid *)>(context->getProcAddress(QByteArrayLiteral("glCompressedTextureImage2DEXT")));
        CompressedTextureImage3DEXT = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLint , GLenum , GLsizei , GLsizei , GLsizei , GLint , GLsizei , const GLvoid *)>(context->getProcAddress(QByteArrayLiteral("glCompressedTextureImage3DEXT")));

        // Use the real DSA functions
        TextureParameteri = &QOpenGLTextureHelper::dsa_TextureParameteri;
        TextureParameteriv = &QOpenGLTextureHelper::dsa_TextureParameteriv;
        TextureParameterf = &QOpenGLTextureHelper::dsa_TextureParameterf;
        TextureParameterfv = &QOpenGLTextureHelper::dsa_TextureParameterfv;
        GenerateTextureMipmap = &QOpenGLTextureHelper::dsa_GenerateTextureMipmap;
        TextureStorage3D = &QOpenGLTextureHelper::dsa_TextureStorage3D;
        TextureStorage2D = &QOpenGLTextureHelper::dsa_TextureStorage2D;
        TextureStorage1D = &QOpenGLTextureHelper::dsa_TextureStorage1D;
        TextureStorage3DMultisample = &QOpenGLTextureHelper::dsa_TextureStorage3DMultisample;
        TextureStorage2DMultisample = &QOpenGLTextureHelper::dsa_TextureStorage2DMultisample;
        TextureImage3D = &QOpenGLTextureHelper::dsa_TextureImage3D;
        TextureImage2D = &QOpenGLTextureHelper::dsa_TextureImage2D;
        TextureImage1D = &QOpenGLTextureHelper::dsa_TextureImage1D;
        TextureSubImage3D = &QOpenGLTextureHelper::dsa_TextureSubImage3D;
        TextureSubImage2D = &QOpenGLTextureHelper::dsa_TextureSubImage2D;
        TextureSubImage1D = &QOpenGLTextureHelper::dsa_TextureSubImage1D;
        CompressedTextureSubImage1D = &QOpenGLTextureHelper::dsa_CompressedTextureSubImage1D;
        CompressedTextureSubImage2D = &QOpenGLTextureHelper::dsa_CompressedTextureSubImage2D;
        CompressedTextureSubImage3D = &QOpenGLTextureHelper::dsa_CompressedTextureSubImage3D;
        CompressedTextureImage1D = &QOpenGLTextureHelper::dsa_CompressedTextureImage1D;
        CompressedTextureImage2D = &QOpenGLTextureHelper::dsa_CompressedTextureImage2D;
        CompressedTextureImage3D = &QOpenGLTextureHelper::dsa_CompressedTextureImage3D;
    } else {
        // Use our own DSA emulation
        TextureParameteri = &QOpenGLTextureHelper::qt_TextureParameteri;
        TextureParameteriv = &QOpenGLTextureHelper::qt_TextureParameteriv;
        GenerateTextureMipmap = &QOpenGLTextureHelper::qt_GenerateTextureMipmap;
        TextureStorage3D = &QOpenGLTextureHelper::qt_TextureStorage3D;
        TextureStorage2D = &QOpenGLTextureHelper::qt_TextureStorage2D;
        TextureStorage1D = &QOpenGLTextureHelper::qt_TextureStorage1D;
        TextureStorage3DMultisample = &QOpenGLTextureHelper::qt_TextureStorage3DMultisample;
        TextureStorage2DMultisample = &QOpenGLTextureHelper::qt_TextureStorage2DMultisample;
        TextureImage3D = &QOpenGLTextureHelper::qt_TextureImage3D;
        TextureImage2D = &QOpenGLTextureHelper::qt_TextureImage2D;
        TextureImage1D = &QOpenGLTextureHelper::qt_TextureImage1D;
        TextureSubImage3D = &QOpenGLTextureHelper::qt_TextureSubImage3D;
        TextureSubImage2D = &QOpenGLTextureHelper::qt_TextureSubImage2D;
        TextureSubImage1D = &QOpenGLTextureHelper::qt_TextureSubImage1D;
        CompressedTextureSubImage1D = &QOpenGLTextureHelper::qt_CompressedTextureSubImage1D;
        CompressedTextureSubImage2D = &QOpenGLTextureHelper::qt_CompressedTextureSubImage2D;
        CompressedTextureSubImage3D = &QOpenGLTextureHelper::qt_CompressedTextureSubImage3D;
        CompressedTextureImage1D = &QOpenGLTextureHelper::qt_CompressedTextureImage1D;
        CompressedTextureImage2D = &QOpenGLTextureHelper::qt_CompressedTextureImage2D;
        CompressedTextureImage3D = &QOpenGLTextureHelper::qt_CompressedTextureImage3D;
    }

    // Some DSA functions are part of NV_texture_multisample instead
    if (!context->isOpenGLES()
        && context->hasExtension(QByteArrayLiteral("GL_NV_texture_multisample"))) {
        TextureImage3DMultisampleNV = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLsizei , GLint , GLsizei , GLsizei , GLsizei , GLboolean )>(context->getProcAddress(QByteArrayLiteral("glTextureImage3DMultisampleNV")));
        TextureImage2DMultisampleNV = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLsizei , GLint , GLsizei , GLsizei , GLboolean )>(context->getProcAddress(QByteArrayLiteral("glTextureImage2DMultisampleNV")));

        TextureImage3DMultisample = &QOpenGLTextureHelper::dsa_TextureImage3DMultisample;
        TextureImage2DMultisample = &QOpenGLTextureHelper::dsa_TextureImage2DMultisample;
    } else {
        TextureImage3DMultisample = &QOpenGLTextureHelper::qt_TextureImage3DMultisample;
        TextureImage2DMultisample = &QOpenGLTextureHelper::qt_TextureImage2DMultisample;
    }

    // wglGetProcAddress should not be used to (and indeed will not) load OpenGL <= 1.1 functions.
    // Hence, we resolve them "the hard way"

#if defined(Q_OS_WIN) && !defined(QT_OPENGL_ES_2)
    HMODULE handle = static_cast<HMODULE>(QOpenGLContext::openGLModuleHandle());
    if (!handle)
        handle = GetModuleHandleA("opengl32.dll");

    // OpenGL 1.0
    GetIntegerv = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint *)>(GetProcAddress(handle, QByteArrayLiteral("glGetIntegerv")));
    GetBooleanv = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLboolean *)>(GetProcAddress(handle, QByteArrayLiteral("glGetBooleanv")));
    PixelStorei = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint )>(GetProcAddress(handle, QByteArrayLiteral("glPixelStorei")));
    GetTexLevelParameteriv = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint , GLenum , GLint *)>(GetProcAddress(handle, QByteArrayLiteral("glGetTexLevelParameteriv")));
    GetTexLevelParameterfv = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint , GLenum , GLfloat *)>(GetProcAddress(handle, QByteArrayLiteral("glGetTexLevelParameterfv")));
    GetTexParameteriv = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLenum , GLint *)>(GetProcAddress(handle, QByteArrayLiteral("glGetTexParameteriv")));
    GetTexParameterfv = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLenum , GLfloat *)>(GetProcAddress(handle, QByteArrayLiteral("glGetTexParameterfv")));
    GetTexImage = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint , GLenum , GLenum , GLvoid *)>(GetProcAddress(handle, QByteArrayLiteral("glGetTexImage")));
    TexImage2D = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint , GLint , GLsizei , GLsizei , GLint , GLenum , GLenum , const GLvoid *)>(GetProcAddress(handle, QByteArrayLiteral("glTexImage2D")));
    TexImage1D = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint , GLint , GLsizei , GLint , GLenum , GLenum , const GLvoid *)>(GetProcAddress(handle, QByteArrayLiteral("glTexImage1D")));
    TexParameteriv = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLenum , const GLint *)>(GetProcAddress(handle, QByteArrayLiteral("glTexParameteriv")));
    TexParameteri = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLenum , GLint )>(GetProcAddress(handle, QByteArrayLiteral("glTexParameteri")));
    TexParameterfv = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLenum , const GLfloat *)>(GetProcAddress(handle, QByteArrayLiteral("glTexParameterfv")));
    TexParameterf = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLenum , GLfloat )>(GetProcAddress(handle, QByteArrayLiteral("glTexParameterf")));

    // OpenGL 1.1
    GenTextures = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLsizei , GLuint *)>(GetProcAddress(handle, QByteArrayLiteral("glGenTextures")));
    DeleteTextures = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLsizei , const GLuint *)>(GetProcAddress(handle, QByteArrayLiteral("glDeleteTextures")));
    BindTexture = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLuint )>(GetProcAddress(handle, QByteArrayLiteral("glBindTexture")));
    TexSubImage2D = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint , GLint , GLint , GLsizei , GLsizei , GLenum , GLenum , const GLvoid *)>(GetProcAddress(handle, QByteArrayLiteral("glTexSubImage2D")));
    TexSubImage1D = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint , GLint , GLsizei , GLenum , GLenum , const GLvoid *)>(GetProcAddress(handle, QByteArrayLiteral("glTexSubImage1D")));

#else

    // OpenGL 1.0
    GetIntegerv = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint *)>(context->getProcAddress(QByteArrayLiteral("glGetIntegerv")));
    GetBooleanv = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLboolean *)>(context->getProcAddress(QByteArrayLiteral("glGetBooleanv")));
    PixelStorei = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint )>(context->getProcAddress(QByteArrayLiteral("glPixelStorei")));
    GetTexLevelParameteriv = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint , GLenum , GLint *)>(context->getProcAddress(QByteArrayLiteral("glGetTexLevelParameteriv")));
    GetTexLevelParameterfv = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint , GLenum , GLfloat *)>(context->getProcAddress(QByteArrayLiteral("glGetTexLevelParameterfv")));
    GetTexParameteriv = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLenum , GLint *)>(context->getProcAddress(QByteArrayLiteral("glGetTexParameteriv")));
    GetTexParameterfv = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLenum , GLfloat *)>(context->getProcAddress(QByteArrayLiteral("glGetTexParameterfv")));
    GetTexImage = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint , GLenum , GLenum , GLvoid *)>(context->getProcAddress(QByteArrayLiteral("glGetTexImage")));
    TexImage2D = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint , GLint , GLsizei , GLsizei , GLint , GLenum , GLenum , const GLvoid *)>(context->getProcAddress(QByteArrayLiteral("glTexImage2D")));
    TexImage1D = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint , GLint , GLsizei , GLint , GLenum , GLenum , const GLvoid *)>(context->getProcAddress(QByteArrayLiteral("glTexImage1D")));
    TexParameteriv = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLenum , const GLint *)>(context->getProcAddress(QByteArrayLiteral("glTexParameteriv")));
    TexParameteri = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLenum , GLint )>(context->getProcAddress(QByteArrayLiteral("glTexParameteri")));
    TexParameterfv = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLenum , const GLfloat *)>(context->getProcAddress(QByteArrayLiteral("glTexParameterfv")));
    TexParameterf = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLenum , GLfloat )>(context->getProcAddress(QByteArrayLiteral("glTexParameterf")));

    // OpenGL 1.1
    GenTextures = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLsizei , GLuint *)>(context->getProcAddress(QByteArrayLiteral("glGenTextures")));
    DeleteTextures = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLsizei , const GLuint *)>(context->getProcAddress(QByteArrayLiteral("glDeleteTextures")));
    BindTexture = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLuint )>(context->getProcAddress(QByteArrayLiteral("glBindTexture")));
    TexSubImage2D = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint , GLint , GLint , GLsizei , GLsizei , GLenum , GLenum , const GLvoid *)>(context->getProcAddress(QByteArrayLiteral("glTexSubImage2D")));
    TexSubImage1D = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint , GLint , GLsizei , GLenum , GLenum , const GLvoid *)>(context->getProcAddress(QByteArrayLiteral("glTexSubImage1D")));
#endif

    if (context->isOpenGLES() && context->hasExtension(QByteArrayLiteral("GL_OES_texture_3D"))) {
        TexImage3D = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*)>(context->getProcAddress(QByteArrayLiteral("glTexImage3DOES")));
        TexSubImage3D = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid*)>(context->getProcAddress(QByteArrayLiteral("glTexSubImage3DOES")));
        CompressedTexImage3D = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*)>(context->getProcAddress(QByteArrayLiteral("glCompressedTexImage3DOES")));
        CompressedTexSubImage3D = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*)>(context->getProcAddress(QByteArrayLiteral("glCompressedTexSubImage3DOES")));
    } else {
        // OpenGL 1.2
        TexImage3D = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint , GLint , GLsizei , GLsizei , GLsizei , GLint , GLenum , GLenum , const GLvoid *)>(context->getProcAddress(QByteArrayLiteral("glTexImage3D")));
        TexSubImage3D = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint , GLint , GLint , GLint , GLsizei , GLsizei , GLsizei , GLenum , GLenum , const GLvoid *)>(context->getProcAddress(QByteArrayLiteral("glTexSubImage3D")));

        // OpenGL 1.3
        CompressedTexImage3D = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint , GLenum , GLsizei , GLsizei , GLsizei , GLint , GLsizei , const GLvoid *)>(context->getProcAddress(QByteArrayLiteral("glCompressedTexImage3D")));
        CompressedTexSubImage3D = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint , GLint , GLint , GLint , GLsizei , GLsizei , GLsizei , GLenum , GLsizei , const GLvoid *)>(context->getProcAddress(QByteArrayLiteral("glCompressedTexSubImage3D")));
    }

    // OpenGL 1.3
    GetCompressedTexImage = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint , GLvoid *)>(context->getProcAddress(QByteArrayLiteral("glGetCompressedTexImage")));
    CompressedTexSubImage1D = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint , GLint , GLsizei , GLenum , GLsizei , const GLvoid *)>(context->getProcAddress(QByteArrayLiteral("glCompressedTexSubImage1D")));
    CompressedTexSubImage2D = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint , GLint , GLint , GLsizei , GLsizei , GLenum , GLsizei , const GLvoid *)>(context->getProcAddress(QByteArrayLiteral("glCompressedTexSubImage2D")));
    CompressedTexImage1D = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint , GLenum , GLsizei , GLint , GLsizei , const GLvoid *)>(context->getProcAddress(QByteArrayLiteral("glCompressedTexImage1D")));
    CompressedTexImage2D = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLint , GLenum , GLsizei , GLsizei , GLint , GLsizei , const GLvoid *)>(context->getProcAddress(QByteArrayLiteral("glCompressedTexImage2D")));
    ActiveTexture = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum )>(context->getProcAddress(QByteArrayLiteral("glActiveTexture")));

    // OpenGL 3.0
    GenerateMipmap = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum )>(context->getProcAddress(QByteArrayLiteral("glGenerateMipmap")));

    // OpenGL 3.2
    TexImage3DMultisample = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLsizei , GLint , GLsizei , GLsizei , GLsizei , GLboolean )>(context->getProcAddress(QByteArrayLiteral("glTexImage3DMultisample")));
    TexImage2DMultisample = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLsizei , GLint , GLsizei , GLsizei , GLboolean )>(context->getProcAddress(QByteArrayLiteral("glTexImage2DMultisample")));

    // OpenGL 4.2
    TexStorage3D = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLsizei , GLenum , GLsizei , GLsizei , GLsizei )>(context->getProcAddress(QByteArrayLiteral("glTexStorage3D")));
    TexStorage2D = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLsizei , GLenum , GLsizei , GLsizei )>(context->getProcAddress(QByteArrayLiteral("glTexStorage2D")));
    TexStorage1D = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLsizei , GLenum , GLsizei )>(context->getProcAddress(QByteArrayLiteral("glTexStorage1D")));

    // OpenGL 4.3
    TexStorage3DMultisample = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLsizei , GLenum , GLsizei , GLsizei , GLsizei , GLboolean )>(context->getProcAddress(QByteArrayLiteral("glTexStorage3DMultisample")));
    TexStorage2DMultisample = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLsizei , GLenum , GLsizei , GLsizei , GLboolean )>(context->getProcAddress(QByteArrayLiteral("glTexStorage2DMultisample")));
    TexBufferRange = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLenum , GLenum , GLuint , GLintptr , GLsizeiptr )>(context->getProcAddress(QByteArrayLiteral("glTexBufferRange")));
    TextureView = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint , GLenum , GLuint , GLenum , GLuint , GLuint , GLuint , GLuint )>(context->getProcAddress(QByteArrayLiteral("glTextureView")));
}

void QOpenGLTextureHelper::dsa_TextureParameteri(GLuint texture, GLenum target, GLenum bindingTarget, GLenum pname, GLint param)
{
    Q_UNUSED(bindingTarget);
    TextureParameteriEXT(texture, target, pname, param);
}

void QOpenGLTextureHelper::dsa_TextureParameteriv(GLuint texture, GLenum target, GLenum bindingTarget, GLenum pname, const GLint *params)
{
    Q_UNUSED(bindingTarget);
    TextureParameterivEXT(texture, target, pname, params);
}

void QOpenGLTextureHelper::dsa_TextureParameterf(GLuint texture, GLenum target, GLenum bindingTarget, GLenum pname, GLfloat param)
{
    Q_UNUSED(bindingTarget);
    TextureParameterfEXT(texture, target, pname, param);
}

void QOpenGLTextureHelper::dsa_TextureParameterfv(GLuint texture, GLenum target, GLenum bindingTarget, GLenum pname, const GLfloat *params)
{
    Q_UNUSED(bindingTarget);
    TextureParameterfvEXT(texture, target, pname, params);
}

void QOpenGLTextureHelper::dsa_GenerateTextureMipmap(GLuint texture, GLenum target, GLenum bindingTarget)
{
    Q_UNUSED(bindingTarget);
    GenerateTextureMipmapEXT(texture, target);
}

void QOpenGLTextureHelper::dsa_TextureStorage3D(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth)
{
    Q_UNUSED(bindingTarget);
    TextureStorage3DEXT(texture, target, levels, internalFormat, width, height, depth);
}

void QOpenGLTextureHelper::dsa_TextureStorage2D(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height)
{
    Q_UNUSED(bindingTarget);
    TextureStorage2DEXT(texture, target, levels, internalFormat, width, height);
}

void QOpenGLTextureHelper::dsa_TextureStorage1D(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei levels, GLenum internalFormat, GLsizei width)
{
    Q_UNUSED(bindingTarget);
    TextureStorage1DEXT(texture, target, levels, internalFormat, width);
}

void QOpenGLTextureHelper::dsa_TextureStorage3DMultisample(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations)
{
    Q_UNUSED(bindingTarget);
    TextureStorage3DMultisampleEXT(texture, target, samples, internalFormat, width, height, depth, fixedSampleLocations);
}

void QOpenGLTextureHelper::dsa_TextureStorage2DMultisample(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations)
{
    Q_UNUSED(bindingTarget);
    TextureStorage2DMultisampleEXT(texture, target, samples, internalFormat, width, height, fixedSampleLocations);
}

void QOpenGLTextureHelper::dsa_TextureImage3D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    Q_UNUSED(bindingTarget);
    TextureImage3DEXT(texture, target, level, internalFormat, width, height, depth, border, format, type, pixels);
}

void QOpenGLTextureHelper::dsa_TextureImage2D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    Q_UNUSED(bindingTarget);
    TextureImage2DEXT(texture, target, level, internalFormat, width, height, border, format, type, pixels);
}

void QOpenGLTextureHelper::dsa_TextureImage1D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    Q_UNUSED(bindingTarget);
    TextureImage1DEXT(texture, target, level, internalFormat, width, border, format, type, pixels);
}

void QOpenGLTextureHelper::dsa_TextureSubImage3D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels)
{
    Q_UNUSED(bindingTarget);
    TextureSubImage3DEXT(texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
}

void QOpenGLTextureHelper::dsa_TextureSubImage2D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
    Q_UNUSED(bindingTarget);
    TextureSubImage2DEXT(texture, target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void QOpenGLTextureHelper::dsa_TextureSubImage1D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels)
{
    Q_UNUSED(bindingTarget);
    TextureSubImage1DEXT(texture, target, level, xoffset, width, format, type, pixels);
}

void QOpenGLTextureHelper::dsa_TextureImage3DMultisample(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations)
{
    Q_UNUSED(bindingTarget);
    TextureImage3DMultisampleNV(texture, target, samples, internalFormat, width, height, depth, fixedSampleLocations);
}

void QOpenGLTextureHelper::dsa_TextureImage2DMultisample(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations)
{
    Q_UNUSED(bindingTarget);
    TextureImage2DMultisampleNV(texture, target, samples, internalFormat, width, height, fixedSampleLocations);
}

void QOpenGLTextureHelper::dsa_CompressedTextureSubImage1D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *bits)
{
    Q_UNUSED(bindingTarget);
    CompressedTextureSubImage1DEXT(texture, target, level, xoffset, width, format, imageSize, bits);
}

void QOpenGLTextureHelper::dsa_CompressedTextureSubImage2D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *bits)
{
    Q_UNUSED(bindingTarget);
    CompressedTextureSubImage2DEXT(texture, target, level, xoffset, yoffset, width, height, format, imageSize, bits);
}

void QOpenGLTextureHelper::dsa_CompressedTextureSubImage3D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *bits)
{
    Q_UNUSED(bindingTarget);
    CompressedTextureSubImage3DEXT(texture, target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits);
}

void QOpenGLTextureHelper::dsa_CompressedTextureImage1D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *bits)
{
    Q_UNUSED(bindingTarget);
    CompressedTextureImage1DEXT(texture, target, level, internalFormat, width, border, imageSize, bits);
}

void QOpenGLTextureHelper::dsa_CompressedTextureImage2D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *bits)
{
    Q_UNUSED(bindingTarget);
    CompressedTextureImage2DEXT(texture, target, level, internalFormat, width, height, border, imageSize, bits);
}

void QOpenGLTextureHelper::dsa_CompressedTextureImage3D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *bits)
{
    Q_UNUSED(bindingTarget);
    CompressedTextureImage3DEXT(texture, target, level, internalFormat, width, height, depth, border, imageSize, bits);
}

void QOpenGLTextureHelper::qt_TextureParameteri(GLuint texture, GLenum target, GLenum bindingTarget, GLenum pname, GLint param)
{
    GLint oldTexture;
    glGetIntegerv(bindingTarget, &oldTexture);
    glBindTexture(target, texture);
    glTexParameteri(target, pname, param);
    glBindTexture(target, oldTexture);
}

void QOpenGLTextureHelper::qt_TextureParameteriv(GLuint texture, GLenum target, GLenum bindingTarget, GLenum pname, const GLint *params)
{
    GLint oldTexture;
    glGetIntegerv(bindingTarget, &oldTexture);
    glBindTexture(target, texture);
    glTexParameteriv(target, pname, params);
    glBindTexture(target, oldTexture);
}

void QOpenGLTextureHelper::qt_TextureParameterf(GLuint texture, GLenum target, GLenum bindingTarget, GLenum pname, GLfloat param)
{
    GLint oldTexture;
    glGetIntegerv(bindingTarget, &oldTexture);
    glBindTexture(target, texture);
    glTexParameterf(target, pname, param);
    glBindTexture(target, oldTexture);
}

void QOpenGLTextureHelper::qt_TextureParameterfv(GLuint texture, GLenum target, GLenum bindingTarget, GLenum pname, const GLfloat *params)
{
    GLint oldTexture;
    glGetIntegerv(bindingTarget, &oldTexture);
    glBindTexture(target, texture);
    glTexParameterfv(target, pname, params);
    glBindTexture(target, oldTexture);
}

void QOpenGLTextureHelper::qt_GenerateTextureMipmap(GLuint texture, GLenum target, GLenum bindingTarget)
{
    GLint oldTexture;
    glGetIntegerv(bindingTarget, &oldTexture);
    glBindTexture(target, texture);
    glGenerateMipmap(target);
    glBindTexture(target, oldTexture);
}

void QOpenGLTextureHelper::qt_TextureStorage3D(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth)
{
    GLint oldTexture;
    glGetIntegerv(bindingTarget, &oldTexture);
    glBindTexture(target, texture);
    glTexStorage3D(target, levels, internalFormat, width, height, depth);
    glBindTexture(target, oldTexture);
}

void QOpenGLTextureHelper::qt_TextureStorage2D(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height)
{
    GLint oldTexture;
    glGetIntegerv(bindingTarget, &oldTexture);
    glBindTexture(target, texture);
    glTexStorage2D(target, levels, internalFormat, width, height);
    glBindTexture(target, oldTexture);
}

void QOpenGLTextureHelper::qt_TextureStorage1D(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei levels, GLenum internalFormat, GLsizei width)
{
    GLint oldTexture;
    glGetIntegerv(bindingTarget, &oldTexture);
    glBindTexture(target, texture);
    glTexStorage1D(target, levels, internalFormat, width);
    glBindTexture(target, oldTexture);
}

void QOpenGLTextureHelper::qt_TextureStorage3DMultisample(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations)
{
    GLint oldTexture;
    glGetIntegerv(bindingTarget, &oldTexture);
    glBindTexture(target, texture);
    glTexStorage3DMultisample(target, samples, internalFormat, width, height, depth, fixedSampleLocations);
    glBindTexture(target, oldTexture);
}

void QOpenGLTextureHelper::qt_TextureStorage2DMultisample(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations)
{
    GLint oldTexture;
    glGetIntegerv(bindingTarget, &oldTexture);
    glBindTexture(target, texture);
    glTexStorage2DMultisample(target, samples, internalFormat, width, height, fixedSampleLocations);
    glBindTexture(target, oldTexture);
}

void QOpenGLTextureHelper::qt_TextureImage3D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    GLint oldTexture;
    glGetIntegerv(bindingTarget, &oldTexture);
    glBindTexture(target, texture);
    glTexImage3D(target, level, internalFormat, width, height, depth, border, format, type, pixels);
    glBindTexture(target, oldTexture);
}

void QOpenGLTextureHelper::qt_TextureImage2D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    // For cubemaps we can't use the standard DSA emulation as it is illegal to
    // try to bind a texture to one of the cubemap face targets. So we force the
    // target and binding target to the cubemap values in this case.
    GLint oldTexture;

    switch (target) {
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &oldTexture);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
        glTexImage2D(target, level, internalFormat, width, height, border, format, type, pixels);
        glBindTexture(GL_TEXTURE_CUBE_MAP, oldTexture);
        break;

    default:
        glGetIntegerv(bindingTarget, &oldTexture);
        glBindTexture(target, texture);
        glTexImage2D(target, level, internalFormat, width, height, border, format, type, pixels);
        glBindTexture(target, oldTexture);
        break;
    }
}

void QOpenGLTextureHelper::qt_TextureImage1D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    GLint oldTexture;
    glGetIntegerv(bindingTarget, &oldTexture);
    glBindTexture(target, texture);
    glTexImage1D(target, level, internalFormat, width, border, format, type, pixels);
    glBindTexture(target, oldTexture);
}

void QOpenGLTextureHelper::qt_TextureSubImage3D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels)
{
    GLint oldTexture;
    glGetIntegerv(bindingTarget, &oldTexture);
    glBindTexture(target, texture);
    glTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
    glBindTexture(target, oldTexture);
}

void QOpenGLTextureHelper::qt_TextureSubImage2D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
    // For cubemaps we can't use the standard DSA emulation as it is illegal to
    // try to bind a texture to one of the cubemap face targets. So we force the
    // target and binding target to the cubemap values in this case.
    GLint oldTexture;

    switch (target) {
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
    case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
    case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &oldTexture);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
        glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
        glBindTexture(GL_TEXTURE_CUBE_MAP, oldTexture);
        break;

    default:
        glGetIntegerv(bindingTarget, &oldTexture);
        glBindTexture(target, texture);
        glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
        glBindTexture(target, oldTexture);
        break;
    }
}

void QOpenGLTextureHelper::qt_TextureSubImage1D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels)
{
    GLint oldTexture;
    glGetIntegerv(bindingTarget, &oldTexture);
    glBindTexture(target, texture);
    glTexSubImage1D(target, level, xoffset, width, format, type, pixels);
    glBindTexture(target, oldTexture);
}

void QOpenGLTextureHelper::qt_TextureImage3DMultisample(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations)
{
    GLint oldTexture;
    glGetIntegerv(bindingTarget, &oldTexture);
    glBindTexture(target, texture);
    glTexImage3DMultisample(target, samples, internalFormat, width, height, depth, fixedSampleLocations);
    glBindTexture(target, oldTexture);
}

void QOpenGLTextureHelper::qt_TextureImage2DMultisample(GLuint texture, GLenum target, GLenum bindingTarget, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations)
{
    GLint oldTexture;
    glGetIntegerv(bindingTarget, &oldTexture);
    glBindTexture(target, texture);
    glTexImage2DMultisample(target, samples, internalFormat, width, height, fixedSampleLocations);
    glBindTexture(target, oldTexture);
}

void QOpenGLTextureHelper::qt_CompressedTextureSubImage1D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *bits)
{
    GLint oldTexture;
    glGetIntegerv(bindingTarget, &oldTexture);
    glBindTexture(target, texture);
    glCompressedTexSubImage1D(target, level, xoffset, width, format, imageSize, bits);
    glBindTexture(target, oldTexture);
}

void QOpenGLTextureHelper::qt_CompressedTextureSubImage2D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *bits)
{
    GLint oldTexture;
    glGetIntegerv(bindingTarget, &oldTexture);
    glBindTexture(target, texture);
    glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, bits);
    glBindTexture(target, oldTexture);
}

void QOpenGLTextureHelper::qt_CompressedTextureSubImage3D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *bits)
{
    GLint oldTexture;
    glGetIntegerv(bindingTarget, &oldTexture);
    glBindTexture(target, texture);
    glCompressedTexSubImage3D(target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, bits);
    glBindTexture(target, oldTexture);
}

void QOpenGLTextureHelper::qt_CompressedTextureImage1D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *bits)
{
    GLint oldTexture;
    glGetIntegerv(bindingTarget, &oldTexture);
    glBindTexture(target, texture);
    glCompressedTexImage1D(target, level, internalFormat, width, border, imageSize, bits);
    glBindTexture(target, oldTexture);
}

void QOpenGLTextureHelper::qt_CompressedTextureImage2D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *bits)
{
    GLint oldTexture;
    glGetIntegerv(bindingTarget, &oldTexture);
    glBindTexture(target, texture);
    glCompressedTexImage2D(target, level, internalFormat, width, height, border, imageSize, bits);
    glBindTexture(target, oldTexture);
}

void QOpenGLTextureHelper::qt_CompressedTextureImage3D(GLuint texture, GLenum target, GLenum bindingTarget, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *bits)
{
    GLint oldTexture;
    glGetIntegerv(bindingTarget, &oldTexture);
    glBindTexture(target, texture);
    glCompressedTexImage3D(target, level, internalFormat, width, height, depth, border, imageSize, bits);
    glBindTexture(target, oldTexture);
}

QT_END_NAMESPACE
