/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QOPENGLFUNCTIONS_H
#define QOPENGLFUNCTIONS_H

#include <QtCore/qglobal.h>

#ifndef QT_NO_OPENGL

#ifdef __GLEW_H__
#if defined(Q_CC_GNU)
#warning qopenglfunctions.h is not compatible with GLEW, GLEW defines will be undefined
#warning To use GLEW with Qt, do not include <qopengl.h> or <QOpenGLFunctions> after glew.h
#endif
#endif

#include <QtGui/qopengl.h>
#include <QtGui/qopenglcontext.h>

//#define Q_ENABLE_OPENGL_FUNCTIONS_DEBUG

#ifdef Q_ENABLE_OPENGL_FUNCTIONS_DEBUG
#include <stdio.h>
#define Q_OPENGL_FUNCTIONS_DEBUG \
    GLenum error = glGetError(); \
    if (error != GL_NO_ERROR) { \
        unsigned clamped = qMin(unsigned(error - GL_INVALID_ENUM), 4U); \
        const char *errors[] = { "GL_INVALID_ENUM", "GL_INVALID_VALUE", "GL_INVALID_OPERATION", "Unknown" }; \
        printf("GL error at %s:%d: %s\n", __FILE__, __LINE__, errors[clamped]); \
        int *value = 0; \
        *value = 0; \
    }
#else
#define Q_OPENGL_FUNCTIONS_DEBUG
#endif

QT_BEGIN_NAMESPACE

struct QOpenGLFunctionsPrivate;

// Undefine any macros from GLEW, qopenglextensions_p.h, etc that
// may interfere with the definition of QOpenGLFunctions.
#undef glBindTexture
#undef glBlendFunc
#undef glClear
#undef glClearColor
#undef glClearStencil
#undef glColorMask
#undef glCopyTexImage2D
#undef glCopyTexSubImage2D
#undef glCullFace
#undef glDeleteTextures
#undef glDepthFunc
#undef glDepthMask
#undef glDisable
#undef glDrawArrays
#undef glDrawElements
#undef glEnable
#undef glFinish
#undef glFlush
#undef glFrontFace
#undef glGenTextures
#undef glGetBooleanv
#undef glGetError
#undef glGetFloatv
#undef glGetIntegerv
#undef glGetString
#undef glGetTexParameterfv
#undef glGetTexParameteriv
#undef glHint
#undef glIsEnabled
#undef glIsTexture
#undef glLineWidth
#undef glPixelStorei
#undef glPolygonOffset
#undef glReadPixels
#undef glScissor
#undef glStencilFunc
#undef glStencilMask
#undef glStencilOp
#undef glTexImage2D
#undef glTexParameterf
#undef glTexParameterfv
#undef glTexParameteri
#undef glTexParameteriv
#undef glTexSubImage2D
#undef glViewport

#undef glActiveTexture
#undef glAttachShader
#undef glBindAttribLocation
#undef glBindBuffer
#undef glBindFramebuffer
#undef glBindRenderbuffer
#undef glBlendColor
#undef glBlendEquation
#undef glBlendEquationSeparate
#undef glBlendFuncSeparate
#undef glBufferData
#undef glBufferSubData
#undef glCheckFramebufferStatus
#undef glClearDepthf
#undef glCompileShader
#undef glCompressedTexImage2D
#undef glCompressedTexSubImage2D
#undef glCreateProgram
#undef glCreateShader
#undef glDeleteBuffers
#undef glDeleteFramebuffers
#undef glDeleteProgram
#undef glDeleteRenderbuffers
#undef glDeleteShader
#undef glDepthRangef
#undef glDetachShader
#undef glDisableVertexAttribArray
#undef glEnableVertexAttribArray
#undef glFramebufferRenderbuffer
#undef glFramebufferTexture2D
#undef glGenBuffers
#undef glGenerateMipmap
#undef glGenFramebuffers
#undef glGenRenderbuffers
#undef glGetActiveAttrib
#undef glGetActiveUniform
#undef glGetAttachedShaders
#undef glGetAttribLocation
#undef glGetBufferParameteriv
#undef glGetFramebufferAttachmentParameteriv
#undef glGetProgramiv
#undef glGetProgramInfoLog
#undef glGetRenderbufferParameteriv
#undef glGetShaderiv
#undef glGetShaderInfoLog
#undef glGetShaderPrecisionFormat
#undef glGetShaderSource
#undef glGetUniformfv
#undef glGetUniformiv
#undef glGetUniformLocation
#undef glGetVertexAttribfv
#undef glGetVertexAttribiv
#undef glGetVertexAttribPointerv
#undef glIsBuffer
#undef glIsFramebuffer
#undef glIsProgram
#undef glIsRenderbuffer
#undef glIsShader
#undef glLinkProgram
#undef glReleaseShaderCompiler
#undef glRenderbufferStorage
#undef glSampleCoverage
#undef glShaderBinary
#undef glShaderSource
#undef glStencilFuncSeparate
#undef glStencilMaskSeparate
#undef glStencilOpSeparate
#undef glUniform1f
#undef glUniform1fv
#undef glUniform1i
#undef glUniform1iv
#undef glUniform2f
#undef glUniform2fv
#undef glUniform2i
#undef glUniform2iv
#undef glUniform3f
#undef glUniform3fv
#undef glUniform3i
#undef glUniform3iv
#undef glUniform4f
#undef glUniform4fv
#undef glUniform4i
#undef glUniform4iv
#undef glUniformMatrix2fv
#undef glUniformMatrix3fv
#undef glUniformMatrix4fv
#undef glUseProgram
#undef glValidateProgram
#undef glVertexAttrib1f
#undef glVertexAttrib1fv
#undef glVertexAttrib2f
#undef glVertexAttrib2fv
#undef glVertexAttrib3f
#undef glVertexAttrib3fv
#undef glVertexAttrib4f
#undef glVertexAttrib4fv
#undef glVertexAttribPointer

#undef glTexLevelParameteriv

class Q_GUI_EXPORT QOpenGLFunctions
{
public:
    QOpenGLFunctions();
    explicit QOpenGLFunctions(QOpenGLContext *context);
    ~QOpenGLFunctions() {}

    enum OpenGLFeature
    {
        Multitexture          = 0x0001,
        Shaders               = 0x0002,
        Buffers               = 0x0004,
        Framebuffers          = 0x0008,
        BlendColor            = 0x0010,
        BlendEquation         = 0x0020,
        BlendEquationSeparate = 0x0040,
        BlendFuncSeparate     = 0x0080,
        BlendSubtract         = 0x0100,
        CompressedTextures    = 0x0200,
        Multisample           = 0x0400,
        StencilSeparate       = 0x0800,
        NPOTTextures          = 0x1000,
        NPOTTextureRepeat     = 0x2000,
        FixedFunctionPipeline = 0x4000
    };
    Q_DECLARE_FLAGS(OpenGLFeatures, OpenGLFeature)

    QOpenGLFunctions::OpenGLFeatures openGLFeatures() const;
    bool hasOpenGLFeature(QOpenGLFunctions::OpenGLFeature feature) const;

    void initializeOpenGLFunctions();

#if QT_DEPRECATED_SINCE(5, 0)
    QT_DEPRECATED void initializeGLFunctions() { initializeOpenGLFunctions(); }
#endif

    // GLES2 + OpenGL1 common subset
    void glBindTexture(GLenum target, GLuint texture);
    void glBlendFunc(GLenum sfactor, GLenum dfactor);
    void glClear(GLbitfield mask);
    void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
    void glClearStencil(GLint s);
    void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
    void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
    void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
    void glCullFace(GLenum mode);
    void glDeleteTextures(GLsizei n, const GLuint* textures);
    void glDepthFunc(GLenum func);
    void glDepthMask(GLboolean flag);
    void glDisable(GLenum cap);
    void glDrawArrays(GLenum mode, GLint first, GLsizei count);
    void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices);
    void glEnable(GLenum cap);
    void glFinish();
    void glFlush();
    void glFrontFace(GLenum mode);
    void glGenTextures(GLsizei n, GLuint* textures);
    void glGetBooleanv(GLenum pname, GLboolean* params);
    GLenum glGetError();
    void glGetFloatv(GLenum pname, GLfloat* params);
    void glGetIntegerv(GLenum pname, GLint* params);
    const GLubyte *glGetString(GLenum name);
    void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params);
    void glGetTexParameteriv(GLenum target, GLenum pname, GLint* params);
    void glHint(GLenum target, GLenum mode);
    GLboolean glIsEnabled(GLenum cap);
    GLboolean glIsTexture(GLuint texture);
    void glLineWidth(GLfloat width);
    void glPixelStorei(GLenum pname, GLint param);
    void glPolygonOffset(GLfloat factor, GLfloat units);
    void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels);
    void glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
    void glStencilFunc(GLenum func, GLint ref, GLuint mask);
    void glStencilMask(GLuint mask);
    void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass);
    void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
    void glTexParameterf(GLenum target, GLenum pname, GLfloat param);
    void glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params);
    void glTexParameteri(GLenum target, GLenum pname, GLint param);
    void glTexParameteriv(GLenum target, GLenum pname, const GLint* params);
    void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels);
    void glViewport(GLint x, GLint y, GLsizei width, GLsizei height);

    // GL(ES)2
    void glActiveTexture(GLenum texture);
    void glAttachShader(GLuint program, GLuint shader);
    void glBindAttribLocation(GLuint program, GLuint index, const char* name);
    void glBindBuffer(GLenum target, GLuint buffer);
    void glBindFramebuffer(GLenum target, GLuint framebuffer);
    void glBindRenderbuffer(GLenum target, GLuint renderbuffer);
    void glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
    void glBlendEquation(GLenum mode);
    void glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha);
    void glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
    void glBufferData(GLenum target, qopengl_GLsizeiptr size, const void* data, GLenum usage);
    void glBufferSubData(GLenum target, qopengl_GLintptr offset, qopengl_GLsizeiptr size, const void* data);
    GLenum glCheckFramebufferStatus(GLenum target);
    void glClearDepthf(GLclampf depth);
    void glCompileShader(GLuint shader);
    void glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data);
    void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data);
    GLuint glCreateProgram();
    GLuint glCreateShader(GLenum type);
    void glDeleteBuffers(GLsizei n, const GLuint* buffers);
    void glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers);
    void glDeleteProgram(GLuint program);
    void glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers);
    void glDeleteShader(GLuint shader);
    void glDepthRangef(GLclampf zNear, GLclampf zFar);
    void glDetachShader(GLuint program, GLuint shader);
    void glDisableVertexAttribArray(GLuint index);
    void glEnableVertexAttribArray(GLuint index);
    void glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
    void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    void glGenBuffers(GLsizei n, GLuint* buffers);
    void glGenerateMipmap(GLenum target);
    void glGenFramebuffers(GLsizei n, GLuint* framebuffers);
    void glGenRenderbuffers(GLsizei n, GLuint* renderbuffers);
    void glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name);
    void glGetActiveUniform(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name);
    void glGetAttachedShaders(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders);
    GLint glGetAttribLocation(GLuint program, const char* name);
    void glGetBufferParameteriv(GLenum target, GLenum pname, GLint* params);
    void glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params);
    void glGetProgramiv(GLuint program, GLenum pname, GLint* params);
    void glGetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei* length, char* infolog);
    void glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params);
    void glGetShaderiv(GLuint shader, GLenum pname, GLint* params);
    void glGetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei* length, char* infolog);
    void glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision);
    void glGetShaderSource(GLuint shader, GLsizei bufsize, GLsizei* length, char* source);
    void glGetUniformfv(GLuint program, GLint location, GLfloat* params);
    void glGetUniformiv(GLuint program, GLint location, GLint* params);
    GLint glGetUniformLocation(GLuint program, const char* name);
    void glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params);
    void glGetVertexAttribiv(GLuint index, GLenum pname, GLint* params);
    void glGetVertexAttribPointerv(GLuint index, GLenum pname, void** pointer);
    GLboolean glIsBuffer(GLuint buffer);
    GLboolean glIsFramebuffer(GLuint framebuffer);
    GLboolean glIsProgram(GLuint program);
    GLboolean glIsRenderbuffer(GLuint renderbuffer);
    GLboolean glIsShader(GLuint shader);
    void glLinkProgram(GLuint program);
    void glReleaseShaderCompiler();
    void glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
    void glSampleCoverage(GLclampf value, GLboolean invert);
    void glShaderBinary(GLint n, const GLuint* shaders, GLenum binaryformat, const void* binary, GLint length);
    void glShaderSource(GLuint shader, GLsizei count, const char** string, const GLint* length);
    void glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask);
    void glStencilMaskSeparate(GLenum face, GLuint mask);
    void glStencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass);
    void glUniform1f(GLint location, GLfloat x);
    void glUniform1fv(GLint location, GLsizei count, const GLfloat* v);
    void glUniform1i(GLint location, GLint x);
    void glUniform1iv(GLint location, GLsizei count, const GLint* v);
    void glUniform2f(GLint location, GLfloat x, GLfloat y);
    void glUniform2fv(GLint location, GLsizei count, const GLfloat* v);
    void glUniform2i(GLint location, GLint x, GLint y);
    void glUniform2iv(GLint location, GLsizei count, const GLint* v);
    void glUniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z);
    void glUniform3fv(GLint location, GLsizei count, const GLfloat* v);
    void glUniform3i(GLint location, GLint x, GLint y, GLint z);
    void glUniform3iv(GLint location, GLsizei count, const GLint* v);
    void glUniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void glUniform4fv(GLint location, GLsizei count, const GLfloat* v);
    void glUniform4i(GLint location, GLint x, GLint y, GLint z, GLint w);
    void glUniform4iv(GLint location, GLsizei count, const GLint* v);
    void glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
    void glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
    void glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
    void glUseProgram(GLuint program);
    void glValidateProgram(GLuint program);
    void glVertexAttrib1f(GLuint indx, GLfloat x);
    void glVertexAttrib1fv(GLuint indx, const GLfloat* values);
    void glVertexAttrib2f(GLuint indx, GLfloat x, GLfloat y);
    void glVertexAttrib2fv(GLuint indx, const GLfloat* values);
    void glVertexAttrib3f(GLuint indx, GLfloat x, GLfloat y, GLfloat z);
    void glVertexAttrib3fv(GLuint indx, const GLfloat* values);
    void glVertexAttrib4f(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void glVertexAttrib4fv(GLuint indx, const GLfloat* values);
    void glVertexAttribPointer(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* ptr);

protected:
    QOpenGLFunctionsPrivate *d_ptr;
    static bool isInitialized(const QOpenGLFunctionsPrivate *d) { return d != 0; }
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QOpenGLFunctions::OpenGLFeatures)

struct QOpenGLFunctionsPrivate
{
    QOpenGLFunctionsPrivate(QOpenGLContext *ctx);

    void (QOPENGLF_APIENTRYP BindTexture)(GLenum target, GLuint texture);
    void (QOPENGLF_APIENTRYP BlendFunc)(GLenum sfactor, GLenum dfactor);
    void (QOPENGLF_APIENTRYP Clear)(GLbitfield mask);
    void (QOPENGLF_APIENTRYP ClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
    void (QOPENGLF_APIENTRYP ClearDepthf)(GLclampf depth);
    void (QOPENGLF_APIENTRYP ClearStencil)(GLint s);
    void (QOPENGLF_APIENTRYP ColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
    void (QOPENGLF_APIENTRYP CopyTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
    void (QOPENGLF_APIENTRYP CopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
    void (QOPENGLF_APIENTRYP CullFace)(GLenum mode);
    void (QOPENGLF_APIENTRYP DeleteTextures)(GLsizei n, const GLuint* textures);
    void (QOPENGLF_APIENTRYP DepthFunc)(GLenum func);
    void (QOPENGLF_APIENTRYP DepthMask)(GLboolean flag);
    void (QOPENGLF_APIENTRYP DepthRangef)(GLclampf nearVal, GLclampf farVal);
    void (QOPENGLF_APIENTRYP Disable)(GLenum cap);
    void (QOPENGLF_APIENTRYP DrawArrays)(GLenum mode, GLint first, GLsizei count);
    void (QOPENGLF_APIENTRYP DrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices);
    void (QOPENGLF_APIENTRYP Enable)(GLenum cap);
    void (QOPENGLF_APIENTRYP Finish)();
    void (QOPENGLF_APIENTRYP Flush)();
    void (QOPENGLF_APIENTRYP FrontFace)(GLenum mode);
    void (QOPENGLF_APIENTRYP GenTextures)(GLsizei n, GLuint* textures);
    void (QOPENGLF_APIENTRYP GetBooleanv)(GLenum pname, GLboolean* params);
    GLenum (QOPENGLF_APIENTRYP GetError)();
    void (QOPENGLF_APIENTRYP GetFloatv)(GLenum pname, GLfloat* params);
    void (QOPENGLF_APIENTRYP GetIntegerv)(GLenum pname, GLint* params);
    const GLubyte * (QOPENGLF_APIENTRYP GetString)(GLenum name);
    void (QOPENGLF_APIENTRYP GetTexParameterfv)(GLenum target, GLenum pname, GLfloat* params);
    void (QOPENGLF_APIENTRYP GetTexParameteriv)(GLenum target, GLenum pname, GLint* params);
    void (QOPENGLF_APIENTRYP Hint)(GLenum target, GLenum mode);
    GLboolean (QOPENGLF_APIENTRYP IsEnabled)(GLenum cap);
    GLboolean (QOPENGLF_APIENTRYP IsTexture)(GLuint texture);
    void (QOPENGLF_APIENTRYP LineWidth)(GLfloat width);
    void (QOPENGLF_APIENTRYP PixelStorei)(GLenum pname, GLint param);
    void (QOPENGLF_APIENTRYP PolygonOffset)(GLfloat factor, GLfloat units);
    void (QOPENGLF_APIENTRYP ReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels);
    void (QOPENGLF_APIENTRYP Scissor)(GLint x, GLint y, GLsizei width, GLsizei height);
    void (QOPENGLF_APIENTRYP StencilFunc)(GLenum func, GLint ref, GLuint mask);
    void (QOPENGLF_APIENTRYP StencilMask)(GLuint mask);
    void (QOPENGLF_APIENTRYP StencilOp)(GLenum fail, GLenum zfail, GLenum zpass);
    void (QOPENGLF_APIENTRYP TexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
    void (QOPENGLF_APIENTRYP TexParameterf)(GLenum target, GLenum pname, GLfloat param);
    void (QOPENGLF_APIENTRYP TexParameterfv)(GLenum target, GLenum pname, const GLfloat* params);
    void (QOPENGLF_APIENTRYP TexParameteri)(GLenum target, GLenum pname, GLint param);
    void (QOPENGLF_APIENTRYP TexParameteriv)(GLenum target, GLenum pname, const GLint* params);
    void (QOPENGLF_APIENTRYP TexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels);
    void (QOPENGLF_APIENTRYP Viewport)(GLint x, GLint y, GLsizei width, GLsizei height);

    void (QOPENGLF_APIENTRYP ActiveTexture)(GLenum texture);
    void (QOPENGLF_APIENTRYP AttachShader)(GLuint program, GLuint shader);
    void (QOPENGLF_APIENTRYP BindAttribLocation)(GLuint program, GLuint index, const char* name);
    void (QOPENGLF_APIENTRYP BindBuffer)(GLenum target, GLuint buffer);
    void (QOPENGLF_APIENTRYP BindFramebuffer)(GLenum target, GLuint framebuffer);
    void (QOPENGLF_APIENTRYP BindRenderbuffer)(GLenum target, GLuint renderbuffer);
    void (QOPENGLF_APIENTRYP BlendColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
    void (QOPENGLF_APIENTRYP BlendEquation)(GLenum mode);
    void (QOPENGLF_APIENTRYP BlendEquationSeparate)(GLenum modeRGB, GLenum modeAlpha);
    void (QOPENGLF_APIENTRYP BlendFuncSeparate)(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
    void (QOPENGLF_APIENTRYP BufferData)(GLenum target, qopengl_GLsizeiptr size, const void* data, GLenum usage);
    void (QOPENGLF_APIENTRYP BufferSubData)(GLenum target, qopengl_GLintptr offset, qopengl_GLsizeiptr size, const void* data);
    GLenum (QOPENGLF_APIENTRYP CheckFramebufferStatus)(GLenum target);
    void (QOPENGLF_APIENTRYP CompileShader)(GLuint shader);
    void (QOPENGLF_APIENTRYP CompressedTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data);
    void (QOPENGLF_APIENTRYP CompressedTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data);
    GLuint (QOPENGLF_APIENTRYP CreateProgram)();
    GLuint (QOPENGLF_APIENTRYP CreateShader)(GLenum type);
    void (QOPENGLF_APIENTRYP DeleteBuffers)(GLsizei n, const GLuint* buffers);
    void (QOPENGLF_APIENTRYP DeleteFramebuffers)(GLsizei n, const GLuint* framebuffers);
    void (QOPENGLF_APIENTRYP DeleteProgram)(GLuint program);
    void (QOPENGLF_APIENTRYP DeleteRenderbuffers)(GLsizei n, const GLuint* renderbuffers);
    void (QOPENGLF_APIENTRYP DeleteShader)(GLuint shader);
    void (QOPENGLF_APIENTRYP DetachShader)(GLuint program, GLuint shader);
    void (QOPENGLF_APIENTRYP DisableVertexAttribArray)(GLuint index);
    void (QOPENGLF_APIENTRYP EnableVertexAttribArray)(GLuint index);
    void (QOPENGLF_APIENTRYP FramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
    void (QOPENGLF_APIENTRYP FramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
    void (QOPENGLF_APIENTRYP GenBuffers)(GLsizei n, GLuint* buffers);
    void (QOPENGLF_APIENTRYP GenerateMipmap)(GLenum target);
    void (QOPENGLF_APIENTRYP GenFramebuffers)(GLsizei n, GLuint* framebuffers);
    void (QOPENGLF_APIENTRYP GenRenderbuffers)(GLsizei n, GLuint* renderbuffers);
    void (QOPENGLF_APIENTRYP GetActiveAttrib)(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name);
    void (QOPENGLF_APIENTRYP GetActiveUniform)(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name);
    void (QOPENGLF_APIENTRYP GetAttachedShaders)(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders);
    GLint (QOPENGLF_APIENTRYP GetAttribLocation)(GLuint program, const char* name);
    void (QOPENGLF_APIENTRYP GetBufferParameteriv)(GLenum target, GLenum pname, GLint* params);
    void (QOPENGLF_APIENTRYP GetFramebufferAttachmentParameteriv)(GLenum target, GLenum attachment, GLenum pname, GLint* params);
    void (QOPENGLF_APIENTRYP GetProgramiv)(GLuint program, GLenum pname, GLint* params);
    void (QOPENGLF_APIENTRYP GetProgramInfoLog)(GLuint program, GLsizei bufsize, GLsizei* length, char* infolog);
    void (QOPENGLF_APIENTRYP GetRenderbufferParameteriv)(GLenum target, GLenum pname, GLint* params);
    void (QOPENGLF_APIENTRYP GetShaderiv)(GLuint shader, GLenum pname, GLint* params);
    void (QOPENGLF_APIENTRYP GetShaderInfoLog)(GLuint shader, GLsizei bufsize, GLsizei* length, char* infolog);
    void (QOPENGLF_APIENTRYP GetShaderPrecisionFormat)(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision);
    void (QOPENGLF_APIENTRYP GetShaderSource)(GLuint shader, GLsizei bufsize, GLsizei* length, char* source);
    void (QOPENGLF_APIENTRYP GetUniformfv)(GLuint program, GLint location, GLfloat* params);
    void (QOPENGLF_APIENTRYP GetUniformiv)(GLuint program, GLint location, GLint* params);
    GLint (QOPENGLF_APIENTRYP GetUniformLocation)(GLuint program, const char* name);
    void (QOPENGLF_APIENTRYP GetVertexAttribfv)(GLuint index, GLenum pname, GLfloat* params);
    void (QOPENGLF_APIENTRYP GetVertexAttribiv)(GLuint index, GLenum pname, GLint* params);
    void (QOPENGLF_APIENTRYP GetVertexAttribPointerv)(GLuint index, GLenum pname, void** pointer);
    GLboolean (QOPENGLF_APIENTRYP IsBuffer)(GLuint buffer);
    GLboolean (QOPENGLF_APIENTRYP IsFramebuffer)(GLuint framebuffer);
    GLboolean (QOPENGLF_APIENTRYP IsProgram)(GLuint program);
    GLboolean (QOPENGLF_APIENTRYP IsRenderbuffer)(GLuint renderbuffer);
    GLboolean (QOPENGLF_APIENTRYP IsShader)(GLuint shader);
    void (QOPENGLF_APIENTRYP LinkProgram)(GLuint program);
    void (QOPENGLF_APIENTRYP ReleaseShaderCompiler)();
    void (QOPENGLF_APIENTRYP RenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
    void (QOPENGLF_APIENTRYP SampleCoverage)(GLclampf value, GLboolean invert);
    void (QOPENGLF_APIENTRYP ShaderBinary)(GLint n, const GLuint* shaders, GLenum binaryformat, const void* binary, GLint length);
    void (QOPENGLF_APIENTRYP ShaderSource)(GLuint shader, GLsizei count, const char** string, const GLint* length);
    void (QOPENGLF_APIENTRYP StencilFuncSeparate)(GLenum face, GLenum func, GLint ref, GLuint mask);
    void (QOPENGLF_APIENTRYP StencilMaskSeparate)(GLenum face, GLuint mask);
    void (QOPENGLF_APIENTRYP StencilOpSeparate)(GLenum face, GLenum fail, GLenum zfail, GLenum zpass);
    void (QOPENGLF_APIENTRYP Uniform1f)(GLint location, GLfloat x);
    void (QOPENGLF_APIENTRYP Uniform1fv)(GLint location, GLsizei count, const GLfloat* v);
    void (QOPENGLF_APIENTRYP Uniform1i)(GLint location, GLint x);
    void (QOPENGLF_APIENTRYP Uniform1iv)(GLint location, GLsizei count, const GLint* v);
    void (QOPENGLF_APIENTRYP Uniform2f)(GLint location, GLfloat x, GLfloat y);
    void (QOPENGLF_APIENTRYP Uniform2fv)(GLint location, GLsizei count, const GLfloat* v);
    void (QOPENGLF_APIENTRYP Uniform2i)(GLint location, GLint x, GLint y);
    void (QOPENGLF_APIENTRYP Uniform2iv)(GLint location, GLsizei count, const GLint* v);
    void (QOPENGLF_APIENTRYP Uniform3f)(GLint location, GLfloat x, GLfloat y, GLfloat z);
    void (QOPENGLF_APIENTRYP Uniform3fv)(GLint location, GLsizei count, const GLfloat* v);
    void (QOPENGLF_APIENTRYP Uniform3i)(GLint location, GLint x, GLint y, GLint z);
    void (QOPENGLF_APIENTRYP Uniform3iv)(GLint location, GLsizei count, const GLint* v);
    void (QOPENGLF_APIENTRYP Uniform4f)(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void (QOPENGLF_APIENTRYP Uniform4fv)(GLint location, GLsizei count, const GLfloat* v);
    void (QOPENGLF_APIENTRYP Uniform4i)(GLint location, GLint x, GLint y, GLint z, GLint w);
    void (QOPENGLF_APIENTRYP Uniform4iv)(GLint location, GLsizei count, const GLint* v);
    void (QOPENGLF_APIENTRYP UniformMatrix2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
    void (QOPENGLF_APIENTRYP UniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
    void (QOPENGLF_APIENTRYP UniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
    void (QOPENGLF_APIENTRYP UseProgram)(GLuint program);
    void (QOPENGLF_APIENTRYP ValidateProgram)(GLuint program);
    void (QOPENGLF_APIENTRYP VertexAttrib1f)(GLuint indx, GLfloat x);
    void (QOPENGLF_APIENTRYP VertexAttrib1fv)(GLuint indx, const GLfloat* values);
    void (QOPENGLF_APIENTRYP VertexAttrib2f)(GLuint indx, GLfloat x, GLfloat y);
    void (QOPENGLF_APIENTRYP VertexAttrib2fv)(GLuint indx, const GLfloat* values);
    void (QOPENGLF_APIENTRYP VertexAttrib3f)(GLuint indx, GLfloat x, GLfloat y, GLfloat z);
    void (QOPENGLF_APIENTRYP VertexAttrib3fv)(GLuint indx, const GLfloat* values);
    void (QOPENGLF_APIENTRYP VertexAttrib4f)(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
    void (QOPENGLF_APIENTRYP VertexAttrib4fv)(GLuint indx, const GLfloat* values);
    void (QOPENGLF_APIENTRYP VertexAttribPointer)(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* ptr);

    // Special non-ES OpenGL variants, not to be called directly
#ifndef QT_OPENGL_ES_2
    void (QOPENGLF_APIENTRYP ClearDepth)(GLdouble depth);
    void (QOPENGLF_APIENTRYP DepthRange)(GLdouble zNear, GLdouble zFar);
#endif
};

// GLES2 + OpenGL1 common subset

inline void QOpenGLFunctions::glBindTexture(GLenum target, GLuint texture)
{
#ifdef QT_OPENGL_ES_2
    ::glBindTexture(target, texture);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BindTexture(target, texture);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBlendFunc(GLenum sfactor, GLenum dfactor)
{
#ifdef QT_OPENGL_ES_2
    ::glBlendFunc(sfactor, dfactor);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BlendFunc(sfactor, dfactor);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glClear(GLbitfield mask)
{
#ifdef QT_OPENGL_ES_2
    ::glClear(mask);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Clear(mask);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
#ifdef QT_OPENGL_ES_2
    ::glClearColor(red, green, blue, alpha);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->ClearColor(red, green, blue, alpha);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glClearStencil(GLint s)
{
#ifdef QT_OPENGL_ES_2
    ::glClearStencil(s);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->ClearStencil(s);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
#ifdef QT_OPENGL_ES_2
    ::glColorMask(red, green, blue, alpha);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->ColorMask(red, green, blue, alpha);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
#ifdef QT_OPENGL_ES_2
    ::glCopyTexImage2D(target, level, internalformat, x, y, width,height, border);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->CopyTexImage2D(target, level, internalformat, x, y, width,height, border);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
#ifdef QT_OPENGL_ES_2
    ::glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->CopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glCullFace(GLenum mode)
{
#ifdef QT_OPENGL_ES_2
    ::glCullFace(mode);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->CullFace(mode);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDeleteTextures(GLsizei n, const GLuint* textures)
{
#ifdef QT_OPENGL_ES_2
    ::glDeleteTextures(n, textures);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DeleteTextures(n, textures);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDepthFunc(GLenum func)
{
#ifdef QT_OPENGL_ES_2
    ::glDepthFunc(func);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DepthFunc(func);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDepthMask(GLboolean flag)
{
#ifdef QT_OPENGL_ES_2
    ::glDepthMask(flag);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DepthMask(flag);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDisable(GLenum cap)
{
#ifdef QT_OPENGL_ES_2
    ::glDisable(cap);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Disable(cap);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
#ifdef QT_OPENGL_ES_2
    ::glDrawArrays(mode, first, count);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DrawArrays(mode, first, count);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices)
{
#ifdef QT_OPENGL_ES_2
    ::glDrawElements(mode, count, type, indices);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DrawElements(mode, count, type, indices);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glEnable(GLenum cap)
{
#ifdef QT_OPENGL_ES_2
    ::glEnable(cap);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Enable(cap);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glFinish()
{
#ifdef QT_OPENGL_ES_2
    ::glFinish();
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Finish();
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glFlush()
{
#ifdef QT_OPENGL_ES_2
    ::glFlush();
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Flush();
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glFrontFace(GLenum mode)
{
#ifdef QT_OPENGL_ES_2
    ::glFrontFace(mode);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->FrontFace(mode);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGenTextures(GLsizei n, GLuint* textures)
{
#ifdef QT_OPENGL_ES_2
    ::glGenTextures(n, textures);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GenTextures(n, textures);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetBooleanv(GLenum pname, GLboolean* params)
{
#ifdef QT_OPENGL_ES_2
    ::glGetBooleanv(pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetBooleanv(pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLenum QOpenGLFunctions::glGetError()
{
#ifdef QT_OPENGL_ES_2
    GLenum result = ::glGetError();
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLenum result = d_ptr->GetError();
#endif
    return result;
}

inline void QOpenGLFunctions::glGetFloatv(GLenum pname, GLfloat* params)
{
#ifdef QT_OPENGL_ES_2
    ::glGetFloatv(pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetFloatv(pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetIntegerv(GLenum pname, GLint* params)
{
#ifdef QT_OPENGL_ES_2
    ::glGetIntegerv(pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetIntegerv(pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline const GLubyte *QOpenGLFunctions::glGetString(GLenum name)
{
#ifdef QT_OPENGL_ES_2
    const GLubyte *result = ::glGetString(name);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    const GLubyte *result = d_ptr->GetString(name);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLFunctions::glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params)
{
#ifdef QT_OPENGL_ES_2
    ::glGetTexParameterfv(target, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetTexParameterfv(target, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetTexParameteriv(GLenum target, GLenum pname, GLint* params)
{
#ifdef QT_OPENGL_ES_2
    ::glGetTexParameteriv(target, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetTexParameteriv(target, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glHint(GLenum target, GLenum mode)
{
#ifdef QT_OPENGL_ES_2
    ::glHint(target, mode);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Hint(target, mode);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLboolean QOpenGLFunctions::glIsEnabled(GLenum cap)
{
#ifdef QT_OPENGL_ES_2
    GLboolean result = ::glIsEnabled(cap);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLboolean result = d_ptr->IsEnabled(cap);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline GLboolean QOpenGLFunctions::glIsTexture(GLuint texture)
{
#ifdef QT_OPENGL_ES_2
    GLboolean result = ::glIsTexture(texture);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLboolean result = d_ptr->IsTexture(texture);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLFunctions::glLineWidth(GLfloat width)
{
#ifdef QT_OPENGL_ES_2
    ::glLineWidth(width);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->LineWidth(width);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glPixelStorei(GLenum pname, GLint param)
{
#ifdef QT_OPENGL_ES_2
    ::glPixelStorei(pname, param);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->PixelStorei(pname, param);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glPolygonOffset(GLfloat factor, GLfloat units)
{
#ifdef QT_OPENGL_ES_2
    ::glPolygonOffset(factor, units);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->PolygonOffset(factor, units);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels)
{
#ifdef QT_OPENGL_ES_2
    ::glReadPixels(x, y, width, height, format, type, pixels);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->ReadPixels(x, y, width, height, format, type, pixels);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
#ifdef QT_OPENGL_ES_2
    ::glScissor(x, y, width, height);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Scissor(x, y, width, height);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
#ifdef QT_OPENGL_ES_2
    ::glStencilFunc(func, ref, mask);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->StencilFunc(func, ref, mask);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glStencilMask(GLuint mask)
{
#ifdef QT_OPENGL_ES_2
    ::glStencilMask(mask);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->StencilMask(mask);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
#ifdef QT_OPENGL_ES_2
    ::glStencilOp(fail, zfail, zpass);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->StencilOp(fail, zfail, zpass);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels)
{
#ifdef QT_OPENGL_ES_2
    ::glTexImage2D(target, level, internalformat, width,height, border, format, type, pixels);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->TexImage2D(target, level, internalformat, width,height, border, format, type, pixels);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
#ifdef QT_OPENGL_ES_2
    ::glTexParameterf(target, pname, param);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->TexParameterf(target, pname, param);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params)
{
#ifdef QT_OPENGL_ES_2
    ::glTexParameterfv(target, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->TexParameterfv(target, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glTexParameteri(GLenum target, GLenum pname, GLint param)
{
#ifdef QT_OPENGL_ES_2
    ::glTexParameteri(target, pname, param);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->TexParameteri(target, pname, param);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glTexParameteriv(GLenum target, GLenum pname, const GLint* params)
{
#ifdef QT_OPENGL_ES_2
    ::glTexParameteriv(target, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->TexParameteriv(target, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels)
{
#ifdef QT_OPENGL_ES_2
    ::glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->TexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
#ifdef QT_OPENGL_ES_2
    ::glViewport(x, y, width, height);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Viewport(x, y, width, height);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

// GL(ES)2

inline void QOpenGLFunctions::glActiveTexture(GLenum texture)
{
#ifdef QT_OPENGL_ES_2
    ::glActiveTexture(texture);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->ActiveTexture(texture);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glAttachShader(GLuint program, GLuint shader)
{
#ifdef QT_OPENGL_ES_2
    ::glAttachShader(program, shader);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->AttachShader(program, shader);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBindAttribLocation(GLuint program, GLuint index, const char* name)
{
#ifdef QT_OPENGL_ES_2
    ::glBindAttribLocation(program, index, name);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BindAttribLocation(program, index, name);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBindBuffer(GLenum target, GLuint buffer)
{
#ifdef QT_OPENGL_ES_2
    ::glBindBuffer(target, buffer);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BindBuffer(target, buffer);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBindFramebuffer(GLenum target, GLuint framebuffer)
{
    if (framebuffer == 0)
        framebuffer = QOpenGLContext::currentContext()->defaultFramebufferObject();
#ifdef QT_OPENGL_ES_2
    ::glBindFramebuffer(target, framebuffer);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BindFramebuffer(target, framebuffer);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBindRenderbuffer(GLenum target, GLuint renderbuffer)
{
#ifdef QT_OPENGL_ES_2
    ::glBindRenderbuffer(target, renderbuffer);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BindRenderbuffer(target, renderbuffer);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
#ifdef QT_OPENGL_ES_2
    ::glBlendColor(red, green, blue, alpha);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BlendColor(red, green, blue, alpha);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBlendEquation(GLenum mode)
{
#ifdef QT_OPENGL_ES_2
    ::glBlendEquation(mode);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BlendEquation(mode);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)
{
#ifdef QT_OPENGL_ES_2
    ::glBlendEquationSeparate(modeRGB, modeAlpha);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BlendEquationSeparate(modeRGB, modeAlpha);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
#ifdef QT_OPENGL_ES_2
    ::glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBufferData(GLenum target, qopengl_GLsizeiptr size, const void* data, GLenum usage)
{
#ifdef QT_OPENGL_ES_2
    ::glBufferData(target, size, data, usage);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BufferData(target, size, data, usage);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glBufferSubData(GLenum target, qopengl_GLintptr offset, qopengl_GLsizeiptr size, const void* data)
{
#ifdef QT_OPENGL_ES_2
    ::glBufferSubData(target, offset, size, data);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->BufferSubData(target, offset, size, data);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLenum QOpenGLFunctions::glCheckFramebufferStatus(GLenum target)
{
#ifdef QT_OPENGL_ES_2
    GLenum result = ::glCheckFramebufferStatus(target);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLenum result = d_ptr->CheckFramebufferStatus(target);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLFunctions::glClearDepthf(GLclampf depth)
{
#ifndef QT_OPENGL_ES
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->ClearDepthf(depth);
#else
    ::glClearDepthf(depth);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glCompileShader(GLuint shader)
{
#ifdef QT_OPENGL_ES_2
    ::glCompileShader(shader);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->CompileShader(shader);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data)
{
#ifdef QT_OPENGL_ES_2
    ::glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->CompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data)
{
#ifdef QT_OPENGL_ES_2
    ::glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->CompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLuint QOpenGLFunctions::glCreateProgram()
{
#ifdef QT_OPENGL_ES_2
    GLuint result = ::glCreateProgram();
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLuint result = d_ptr->CreateProgram();
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline GLuint QOpenGLFunctions::glCreateShader(GLenum type)
{
#ifdef QT_OPENGL_ES_2
    GLuint result = ::glCreateShader(type);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLuint result = d_ptr->CreateShader(type);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLFunctions::glDeleteBuffers(GLsizei n, const GLuint* buffers)
{
#ifdef QT_OPENGL_ES_2
    ::glDeleteBuffers(n, buffers);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DeleteBuffers(n, buffers);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers)
{
#ifdef QT_OPENGL_ES_2
    ::glDeleteFramebuffers(n, framebuffers);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DeleteFramebuffers(n, framebuffers);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDeleteProgram(GLuint program)
{
#ifdef QT_OPENGL_ES_2
    ::glDeleteProgram(program);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DeleteProgram(program);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers)
{
#ifdef QT_OPENGL_ES_2
    ::glDeleteRenderbuffers(n, renderbuffers);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DeleteRenderbuffers(n, renderbuffers);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDeleteShader(GLuint shader)
{
#ifdef QT_OPENGL_ES_2
    ::glDeleteShader(shader);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DeleteShader(shader);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDepthRangef(GLclampf zNear, GLclampf zFar)
{
#ifndef QT_OPENGL_ES
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DepthRangef(zNear, zFar);
#else
    ::glDepthRangef(zNear, zFar);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDetachShader(GLuint program, GLuint shader)
{
#ifdef QT_OPENGL_ES_2
    ::glDetachShader(program, shader);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DetachShader(program, shader);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glDisableVertexAttribArray(GLuint index)
{
#ifdef QT_OPENGL_ES_2
    ::glDisableVertexAttribArray(index);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->DisableVertexAttribArray(index);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glEnableVertexAttribArray(GLuint index)
{
#ifdef QT_OPENGL_ES_2
    ::glEnableVertexAttribArray(index);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->EnableVertexAttribArray(index);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
#ifdef QT_OPENGL_ES_2
    ::glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->FramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
#ifdef QT_OPENGL_ES_2
    ::glFramebufferTexture2D(target, attachment, textarget, texture, level);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->FramebufferTexture2D(target, attachment, textarget, texture, level);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGenBuffers(GLsizei n, GLuint* buffers)
{
#ifdef QT_OPENGL_ES_2
    ::glGenBuffers(n, buffers);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GenBuffers(n, buffers);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGenerateMipmap(GLenum target)
{
#ifdef QT_OPENGL_ES_2
    ::glGenerateMipmap(target);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GenerateMipmap(target);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGenFramebuffers(GLsizei n, GLuint* framebuffers)
{
#ifdef QT_OPENGL_ES_2
    ::glGenFramebuffers(n, framebuffers);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GenFramebuffers(n, framebuffers);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGenRenderbuffers(GLsizei n, GLuint* renderbuffers)
{
#ifdef QT_OPENGL_ES_2
    ::glGenRenderbuffers(n, renderbuffers);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GenRenderbuffers(n, renderbuffers);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name)
{
#ifdef QT_OPENGL_ES_2
    ::glGetActiveAttrib(program, index, bufsize, length, size, type, name);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetActiveAttrib(program, index, bufsize, length, size, type, name);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetActiveUniform(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name)
{
#ifdef QT_OPENGL_ES_2
    ::glGetActiveUniform(program, index, bufsize, length, size, type, name);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetActiveUniform(program, index, bufsize, length, size, type, name);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetAttachedShaders(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders)
{
#ifdef QT_OPENGL_ES_2
    ::glGetAttachedShaders(program, maxcount, count, shaders);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetAttachedShaders(program, maxcount, count, shaders);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLint QOpenGLFunctions::glGetAttribLocation(GLuint program, const char* name)
{
#ifdef QT_OPENGL_ES_2
    GLint result = ::glGetAttribLocation(program, name);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLint result = d_ptr->GetAttribLocation(program, name);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLFunctions::glGetBufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
#ifdef QT_OPENGL_ES_2
    ::glGetBufferParameteriv(target, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetBufferParameteriv(target, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params)
{
#ifdef QT_OPENGL_ES_2
    ::glGetFramebufferAttachmentParameteriv(target, attachment, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetFramebufferAttachmentParameteriv(target, attachment, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetProgramiv(GLuint program, GLenum pname, GLint* params)
{
#ifdef QT_OPENGL_ES_2
    ::glGetProgramiv(program, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetProgramiv(program, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei* length, char* infolog)
{
#ifdef QT_OPENGL_ES_2
    ::glGetProgramInfoLog(program, bufsize, length, infolog);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetProgramInfoLog(program, bufsize, length, infolog);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
#ifdef QT_OPENGL_ES_2
    ::glGetRenderbufferParameteriv(target, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetRenderbufferParameteriv(target, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetShaderiv(GLuint shader, GLenum pname, GLint* params)
{
#ifdef QT_OPENGL_ES_2
    ::glGetShaderiv(shader, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetShaderiv(shader, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei* length, char* infolog)
{
#ifdef QT_OPENGL_ES_2
    ::glGetShaderInfoLog(shader, bufsize, length, infolog);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetShaderInfoLog(shader, bufsize, length, infolog);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision)
{
#ifdef QT_OPENGL_ES_2
    ::glGetShaderPrecisionFormat(shadertype, precisiontype, range, precision);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetShaderPrecisionFormat(shadertype, precisiontype, range, precision);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetShaderSource(GLuint shader, GLsizei bufsize, GLsizei* length, char* source)
{
#ifdef QT_OPENGL_ES_2
    ::glGetShaderSource(shader, bufsize, length, source);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetShaderSource(shader, bufsize, length, source);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetUniformfv(GLuint program, GLint location, GLfloat* params)
{
#ifdef QT_OPENGL_ES_2
    ::glGetUniformfv(program, location, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetUniformfv(program, location, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetUniformiv(GLuint program, GLint location, GLint* params)
{
#ifdef QT_OPENGL_ES_2
    ::glGetUniformiv(program, location, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetUniformiv(program, location, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLint QOpenGLFunctions::glGetUniformLocation(GLuint program, const char* name)
{
#ifdef QT_OPENGL_ES_2
    GLint result = ::glGetUniformLocation(program, name);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLint result = d_ptr->GetUniformLocation(program, name);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLFunctions::glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params)
{
#ifdef QT_OPENGL_ES_2
    ::glGetVertexAttribfv(index, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetVertexAttribfv(index, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetVertexAttribiv(GLuint index, GLenum pname, GLint* params)
{
#ifdef QT_OPENGL_ES_2
    ::glGetVertexAttribiv(index, pname, params);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetVertexAttribiv(index, pname, params);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glGetVertexAttribPointerv(GLuint index, GLenum pname, void** pointer)
{
#ifdef QT_OPENGL_ES_2
    ::glGetVertexAttribPointerv(index, pname, pointer);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->GetVertexAttribPointerv(index, pname, pointer);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline GLboolean QOpenGLFunctions::glIsBuffer(GLuint buffer)
{
#ifdef QT_OPENGL_ES_2
    GLboolean result = ::glIsBuffer(buffer);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLboolean result = d_ptr->IsBuffer(buffer);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline GLboolean QOpenGLFunctions::glIsFramebuffer(GLuint framebuffer)
{
#ifdef QT_OPENGL_ES_2
    GLboolean result = ::glIsFramebuffer(framebuffer);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLboolean result = d_ptr->IsFramebuffer(framebuffer);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline GLboolean QOpenGLFunctions::glIsProgram(GLuint program)
{
#ifdef QT_OPENGL_ES_2
    GLboolean result = ::glIsProgram(program);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLboolean result = d_ptr->IsProgram(program);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline GLboolean QOpenGLFunctions::glIsRenderbuffer(GLuint renderbuffer)
{
#ifdef QT_OPENGL_ES_2
    GLboolean result = ::glIsRenderbuffer(renderbuffer);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLboolean result = d_ptr->IsRenderbuffer(renderbuffer);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline GLboolean QOpenGLFunctions::glIsShader(GLuint shader)
{
#ifdef QT_OPENGL_ES_2
    GLboolean result = ::glIsShader(shader);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    GLboolean result = d_ptr->IsShader(shader);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
    return result;
}

inline void QOpenGLFunctions::glLinkProgram(GLuint program)
{
#ifdef QT_OPENGL_ES_2
    ::glLinkProgram(program);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->LinkProgram(program);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glReleaseShaderCompiler()
{
#ifdef QT_OPENGL_ES_2
    ::glReleaseShaderCompiler();
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->ReleaseShaderCompiler();
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
#ifdef QT_OPENGL_ES_2
    ::glRenderbufferStorage(target, internalformat, width, height);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->RenderbufferStorage(target, internalformat, width, height);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glSampleCoverage(GLclampf value, GLboolean invert)
{
#ifdef QT_OPENGL_ES_2
    ::glSampleCoverage(value, invert);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->SampleCoverage(value, invert);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glShaderBinary(GLint n, const GLuint* shaders, GLenum binaryformat, const void* binary, GLint length)
{
#ifdef QT_OPENGL_ES_2
    ::glShaderBinary(n, shaders, binaryformat, binary, length);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->ShaderBinary(n, shaders, binaryformat, binary, length);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glShaderSource(GLuint shader, GLsizei count, const char** string, const GLint* length)
{
#ifdef QT_OPENGL_ES_2
    ::glShaderSource(shader, count, string, length);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->ShaderSource(shader, count, string, length);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)
{
#ifdef QT_OPENGL_ES_2
    ::glStencilFuncSeparate(face, func, ref, mask);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->StencilFuncSeparate(face, func, ref, mask);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glStencilMaskSeparate(GLenum face, GLuint mask)
{
#ifdef QT_OPENGL_ES_2
    ::glStencilMaskSeparate(face, mask);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->StencilMaskSeparate(face, mask);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glStencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass)
{
#ifdef QT_OPENGL_ES_2
    ::glStencilOpSeparate(face, fail, zfail, zpass);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->StencilOpSeparate(face, fail, zfail, zpass);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform1f(GLint location, GLfloat x)
{
#ifdef QT_OPENGL_ES_2
    ::glUniform1f(location, x);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform1f(location, x);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform1fv(GLint location, GLsizei count, const GLfloat* v)
{
#ifdef QT_OPENGL_ES_2
    ::glUniform1fv(location, count, v);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform1fv(location, count, v);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform1i(GLint location, GLint x)
{
#ifdef QT_OPENGL_ES_2
    ::glUniform1i(location, x);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform1i(location, x);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform1iv(GLint location, GLsizei count, const GLint* v)
{
#ifdef QT_OPENGL_ES_2
    ::glUniform1iv(location, count, v);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform1iv(location, count, v);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform2f(GLint location, GLfloat x, GLfloat y)
{
#ifdef QT_OPENGL_ES_2
    ::glUniform2f(location, x, y);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform2f(location, x, y);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform2fv(GLint location, GLsizei count, const GLfloat* v)
{
#ifdef QT_OPENGL_ES_2
    ::glUniform2fv(location, count, v);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform2fv(location, count, v);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform2i(GLint location, GLint x, GLint y)
{
#ifdef QT_OPENGL_ES_2
    ::glUniform2i(location, x, y);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform2i(location, x, y);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform2iv(GLint location, GLsizei count, const GLint* v)
{
#ifdef QT_OPENGL_ES_2
    ::glUniform2iv(location, count, v);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform2iv(location, count, v);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z)
{
#ifdef QT_OPENGL_ES_2
    ::glUniform3f(location, x, y, z);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform3f(location, x, y, z);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform3fv(GLint location, GLsizei count, const GLfloat* v)
{
#ifdef QT_OPENGL_ES_2
    ::glUniform3fv(location, count, v);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform3fv(location, count, v);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform3i(GLint location, GLint x, GLint y, GLint z)
{
#ifdef QT_OPENGL_ES_2
    ::glUniform3i(location, x, y, z);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform3i(location, x, y, z);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform3iv(GLint location, GLsizei count, const GLint* v)
{
#ifdef QT_OPENGL_ES_2
    ::glUniform3iv(location, count, v);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform3iv(location, count, v);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
#ifdef QT_OPENGL_ES_2
    ::glUniform4f(location, x, y, z, w);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform4f(location, x, y, z, w);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform4fv(GLint location, GLsizei count, const GLfloat* v)
{
#ifdef QT_OPENGL_ES_2
    ::glUniform4fv(location, count, v);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform4fv(location, count, v);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform4i(GLint location, GLint x, GLint y, GLint z, GLint w)
{
#ifdef QT_OPENGL_ES_2
    ::glUniform4i(location, x, y, z, w);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform4i(location, x, y, z, w);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniform4iv(GLint location, GLsizei count, const GLint* v)
{
#ifdef QT_OPENGL_ES_2
    ::glUniform4iv(location, count, v);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->Uniform4iv(location, count, v);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
#ifdef QT_OPENGL_ES_2
    ::glUniformMatrix2fv(location, count, transpose, value);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->UniformMatrix2fv(location, count, transpose, value);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
#ifdef QT_OPENGL_ES_2
    ::glUniformMatrix3fv(location, count, transpose, value);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->UniformMatrix3fv(location, count, transpose, value);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
#ifdef QT_OPENGL_ES_2
    ::glUniformMatrix4fv(location, count, transpose, value);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->UniformMatrix4fv(location, count, transpose, value);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glUseProgram(GLuint program)
{
#ifdef QT_OPENGL_ES_2
    ::glUseProgram(program);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->UseProgram(program);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glValidateProgram(GLuint program)
{
#ifdef QT_OPENGL_ES_2
    ::glValidateProgram(program);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->ValidateProgram(program);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttrib1f(GLuint indx, GLfloat x)
{
#ifdef QT_OPENGL_ES_2
    ::glVertexAttrib1f(indx, x);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttrib1f(indx, x);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttrib1fv(GLuint indx, const GLfloat* values)
{
#ifdef QT_OPENGL_ES_2
    ::glVertexAttrib1fv(indx, values);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttrib1fv(indx, values);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttrib2f(GLuint indx, GLfloat x, GLfloat y)
{
#ifdef QT_OPENGL_ES_2
    ::glVertexAttrib2f(indx, x, y);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttrib2f(indx, x, y);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttrib2fv(GLuint indx, const GLfloat* values)
{
#ifdef QT_OPENGL_ES_2
    ::glVertexAttrib2fv(indx, values);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttrib2fv(indx, values);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttrib3f(GLuint indx, GLfloat x, GLfloat y, GLfloat z)
{
#ifdef QT_OPENGL_ES_2
    ::glVertexAttrib3f(indx, x, y, z);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttrib3f(indx, x, y, z);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttrib3fv(GLuint indx, const GLfloat* values)
{
#ifdef QT_OPENGL_ES_2
    ::glVertexAttrib3fv(indx, values);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttrib3fv(indx, values);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttrib4f(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
#ifdef QT_OPENGL_ES_2
    ::glVertexAttrib4f(indx, x, y, z, w);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttrib4f(indx, x, y, z, w);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttrib4fv(GLuint indx, const GLfloat* values)
{
#ifdef QT_OPENGL_ES_2
    ::glVertexAttrib4fv(indx, values);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttrib4fv(indx, values);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

inline void QOpenGLFunctions::glVertexAttribPointer(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* ptr)
{
#ifdef QT_OPENGL_ES_2
    ::glVertexAttribPointer(indx, size, type, normalized, stride, ptr);
#else
    Q_ASSERT(QOpenGLFunctions::isInitialized(d_ptr));
    d_ptr->VertexAttribPointer(indx, size, type, normalized, stride, ptr);
#endif
    Q_OPENGL_FUNCTIONS_DEBUG
}

QT_END_NAMESPACE

#endif // QT_NO_OPENGL

#endif
