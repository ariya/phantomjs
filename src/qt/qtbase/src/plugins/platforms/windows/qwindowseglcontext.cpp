/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qwindowseglcontext.h"
#include "qwindowscontext.h"
#include "qwindowswindow.h"

#include <QtCore/QDebug>
#include <QtGui/QOpenGLContext>

#if defined(QT_OPENGL_ES_2_ANGLE) || defined(QT_OPENGL_DYNAMIC)
#  define EGL_EGLEXT_PROTOTYPES
#  include <QtANGLE/EGL/eglext.h>
#endif

QT_BEGIN_NAMESPACE

/*!
    \class QWindowsEGLStaticContext
    \brief Static data for QWindowsEGLContext.

    Keeps the display. The class is shared via QSharedPointer in the windows, the
    contexts and in QWindowsIntegration. The display will be closed if the last instance
    is deleted.

    No EGL or OpenGL functions are called directly. Instead, they are resolved
    dynamically. This works even if the plugin links directly to libegl/libglesv2 so
    there is no need to differentiate between dynamic or Angle-only builds in here.

    \internal
    \ingroup qt-lighthouse-win
*/

QWindowsLibEGL QWindowsEGLStaticContext::libEGL;
QWindowsLibGLESv2 QWindowsEGLStaticContext::libGLESv2;

#ifndef QT_STATIC

#ifdef Q_CC_MINGW
static void *resolveFunc(HMODULE lib, const char *name)
{
    QString baseNameStr = QString::fromLatin1(name);
    QString nameStr;
    void *proc = 0;

    // Play nice with 32-bit mingw: Try func first, then func@0, func@4,
    // func@8, func@12, ..., func@64. The def file does not provide any aliases
    // in libEGL and libGLESv2 in these builds which results in exporting
    // function names like eglInitialize@12. This cannot be fixed without
    // breaking binary compatibility. So be flexible here instead.

    int argSize = -1;
    while (!proc && argSize <= 64) {
        nameStr = baseNameStr;
        if (argSize >= 0)
            nameStr += QLatin1Char('@') + QString::number(argSize);
        argSize = argSize < 0 ? 0 : argSize + 4;
        proc = (void *) ::GetProcAddress(lib, nameStr.toLatin1().constData());
    }
    return proc;
}
#else
static void *resolveFunc(HMODULE lib, const char *name)
{
# ifndef Q_OS_WINCE
    return (void *) ::GetProcAddress(lib, name);
# else
    return (void *) ::GetProcAddress(lib, (const wchar_t *) QString::fromLatin1(name).utf16());
# endif // Q_OS_WINCE
}
#endif // Q_CC_MINGW

void *QWindowsLibEGL::resolve(const char *name)
{
    void *proc = m_lib ? resolveFunc(m_lib, name) : 0;
    if (!proc)
        qErrnoWarning(::GetLastError(), "Failed to resolve EGL function %s", name);

    return proc;
}

#endif // !QT_STATIC

#ifndef QT_STATIC
#  define RESOLVE(signature, name) signature(resolve( #name ));
#else
#  define RESOLVE(signature, name) signature(&::name);
#endif

bool QWindowsLibEGL::init()
{
#ifdef QT_DEBUG
    const char dllName[] = "libEGLd.dll";
#else
    const char dllName[] = "libEGL.dll";
#endif

    qCDebug(lcQpaGl) << "Qt: Using EGL from" << dllName;

#ifndef QT_STATIC
    m_lib = ::LoadLibraryW((const wchar_t *) QString::fromLatin1(dllName).utf16());
    if (!m_lib) {
        qErrnoWarning(::GetLastError(), "Failed to load %s", dllName);
        return false;
    }
#endif

    eglGetError = RESOLVE((EGLint (EGLAPIENTRY *)(void)), eglGetError);
    eglGetDisplay = RESOLVE((EGLDisplay (EGLAPIENTRY *)(EGLNativeDisplayType)), eglGetDisplay);
    eglGetPlatformDisplayEXT = RESOLVE((EGLDisplay (EGLAPIENTRY *)(EGLenum platform, void *native_display, const EGLint *attrib_list)), eglGetPlatformDisplayEXT);
    eglInitialize = RESOLVE((EGLBoolean (EGLAPIENTRY *)(EGLDisplay, EGLint *, EGLint *)), eglInitialize);
    eglTerminate = RESOLVE((EGLBoolean (EGLAPIENTRY *)(EGLDisplay)), eglTerminate);
    eglChooseConfig = RESOLVE((EGLBoolean (EGLAPIENTRY *)(EGLDisplay, const EGLint *, EGLConfig *, EGLint, EGLint *)), eglChooseConfig);
    eglGetConfigAttrib = RESOLVE((EGLBoolean (EGLAPIENTRY *)(EGLDisplay, EGLConfig, EGLint, EGLint *)), eglGetConfigAttrib);
    eglCreateWindowSurface = RESOLVE((EGLSurface (EGLAPIENTRY *)(EGLDisplay, EGLConfig, EGLNativeWindowType, const EGLint *)), eglCreateWindowSurface);
    eglCreatePbufferSurface = RESOLVE((EGLSurface (EGLAPIENTRY *)(EGLDisplay , EGLConfig, const EGLint *)), eglCreatePbufferSurface);
    eglDestroySurface = RESOLVE((EGLBoolean (EGLAPIENTRY *)(EGLDisplay , EGLSurface )), eglDestroySurface);
    eglBindAPI = RESOLVE((EGLBoolean (EGLAPIENTRY * )(EGLenum )), eglBindAPI);
    eglSwapInterval = RESOLVE((EGLBoolean (EGLAPIENTRY *)(EGLDisplay , EGLint )), eglSwapInterval);
    eglCreateContext = RESOLVE((EGLContext (EGLAPIENTRY *)(EGLDisplay , EGLConfig , EGLContext , const EGLint *)), eglCreateContext);
    eglDestroyContext = RESOLVE((EGLBoolean (EGLAPIENTRY *)(EGLDisplay, EGLContext)), eglDestroyContext);
    eglMakeCurrent  = RESOLVE((EGLBoolean (EGLAPIENTRY *)(EGLDisplay , EGLSurface , EGLSurface , EGLContext )), eglMakeCurrent);
    eglGetCurrentContext = RESOLVE((EGLContext (EGLAPIENTRY *)(void)), eglGetCurrentContext);
    eglGetCurrentSurface = RESOLVE((EGLSurface (EGLAPIENTRY *)(EGLint )), eglGetCurrentSurface);
    eglGetCurrentDisplay = RESOLVE((EGLDisplay (EGLAPIENTRY *)(void)), eglGetCurrentDisplay);
    eglSwapBuffers = RESOLVE((EGLBoolean (EGLAPIENTRY *)(EGLDisplay , EGLSurface)), eglSwapBuffers);
    eglGetProcAddress = RESOLVE((__eglMustCastToProperFunctionPointerType (EGLAPIENTRY * )(const char *)), eglGetProcAddress);

    return eglGetError && eglGetDisplay && eglInitialize;
}

#ifndef QT_STATIC
void *QWindowsLibGLESv2::resolve(const char *name)
{
    void *proc = m_lib ? resolveFunc(m_lib, name) : 0;
    if (!proc)
        qWarning() << "Failed to resolve OpenGL ES function" << name;

    return proc;
}
#endif // !QT_STATIC

bool QWindowsLibGLESv2::init()
{
#ifdef QT_DEBUG
    const char dllName[] = "libGLESv2d.dll";
#else
    const char dllName[] = "libGLESv2.dll";
#endif

    qCDebug(lcQpaGl) << "Qt: Using OpenGL ES 2.0 from" << dllName;
#ifndef QT_STATIC
    m_lib = ::LoadLibraryW((const wchar_t *) QString::fromLatin1(dllName).utf16());
    if (!m_lib) {
        qErrnoWarning(::GetLastError(), "Failed to load %s", dllName);
        return false;
    }
#endif // !QT_STATIC

    glBindTexture = RESOLVE((void (APIENTRY *)(GLenum , GLuint )), glBindTexture);
    glBlendFunc = RESOLVE((void (APIENTRY *)(GLenum , GLenum )), glBlendFunc);
    glClear = RESOLVE((void (APIENTRY *)(GLbitfield )), glClear);
    glClearColor = RESOLVE((void (APIENTRY *)(GLfloat , GLfloat , GLfloat , GLfloat )), glClearColor);
    glClearStencil = RESOLVE((void (APIENTRY *)(GLint )), glClearStencil);
    glColorMask = RESOLVE((void (APIENTRY *)(GLboolean , GLboolean , GLboolean , GLboolean )), glColorMask);
    glCopyTexImage2D = RESOLVE((void (APIENTRY *)(GLenum , GLint , GLenum , GLint , GLint , GLsizei , GLsizei , GLint )), glCopyTexImage2D);
    glCopyTexSubImage2D = RESOLVE((void (APIENTRY *)(GLenum , GLint , GLint , GLint , GLint , GLint , GLsizei , GLsizei )), glCopyTexSubImage2D);
    glCullFace = RESOLVE((void (APIENTRY *)(GLenum )), glCullFace);
    glDeleteTextures = RESOLVE((void (APIENTRY *)(GLsizei , const GLuint *)), glDeleteTextures);
    glDepthFunc = RESOLVE((void (APIENTRY *)(GLenum )), glDepthFunc);
    glDepthMask = RESOLVE((void (APIENTRY *)(GLboolean )), glDepthMask);
    glDisable = RESOLVE((void (APIENTRY *)(GLenum )), glDisable);
    glDrawArrays = RESOLVE((void (APIENTRY *)(GLenum , GLint , GLsizei )), glDrawArrays);
    glDrawElements = RESOLVE((void (APIENTRY *)(GLenum , GLsizei , GLenum , const GLvoid *)), glDrawElements);
    glEnable = RESOLVE((void (APIENTRY *)(GLenum )), glEnable);
    glFinish = RESOLVE((void (APIENTRY *)()), glFinish);
    glFlush = RESOLVE((void (APIENTRY *)()), glFlush);
    glFrontFace = RESOLVE((void (APIENTRY *)(GLenum )), glFrontFace);
    glGenTextures = RESOLVE((void (APIENTRY *)(GLsizei , GLuint *)), glGenTextures);
    glGetBooleanv = RESOLVE((void (APIENTRY *)(GLenum , GLboolean *)), glGetBooleanv);
    glGetError = RESOLVE((GLenum (APIENTRY *)()), glGetError);
    glGetFloatv = RESOLVE((void (APIENTRY *)(GLenum , GLfloat *)), glGetFloatv);
    glGetIntegerv = RESOLVE((void (APIENTRY *)(GLenum , GLint *)), glGetIntegerv);
    glGetString = RESOLVE((const GLubyte * (APIENTRY *)(GLenum )), glGetString);
    glGetTexParameterfv = RESOLVE((void (APIENTRY *)(GLenum , GLenum , GLfloat *)), glGetTexParameterfv);
    glGetTexParameteriv = RESOLVE((void (APIENTRY *)(GLenum , GLenum , GLint *)), glGetTexParameteriv);
    glHint = RESOLVE((void (APIENTRY *)(GLenum , GLenum )), glHint);
    glIsEnabled = RESOLVE((GLboolean (APIENTRY *)(GLenum )), glIsEnabled);
    glIsTexture = RESOLVE((GLboolean (APIENTRY *)(GLuint )), glIsTexture);
    glLineWidth = RESOLVE((void (APIENTRY *)(GLfloat )), glLineWidth);
    glPixelStorei = RESOLVE((void (APIENTRY *)(GLenum , GLint )), glPixelStorei);
    glPolygonOffset = RESOLVE((void (APIENTRY *)(GLfloat , GLfloat )), glPolygonOffset);
    glReadPixels = RESOLVE((void (APIENTRY *)(GLint , GLint , GLsizei , GLsizei , GLenum , GLenum , GLvoid *)), glReadPixels);
    glScissor = RESOLVE((void (APIENTRY *)(GLint , GLint , GLsizei , GLsizei )), glScissor);
    glStencilFunc = RESOLVE((void (APIENTRY *)(GLenum , GLint , GLuint )), glStencilFunc);
    glStencilMask = RESOLVE((void (APIENTRY *)(GLuint )), glStencilMask);
    glStencilOp = RESOLVE((void (APIENTRY *)(GLenum , GLenum , GLenum )), glStencilOp);
    glTexImage2D = RESOLVE((void (APIENTRY *)(GLenum , GLint , GLint , GLsizei , GLsizei , GLint , GLenum , GLenum , const GLvoid *)), glTexImage2D);
    glTexParameterf = RESOLVE((void (APIENTRY *)(GLenum , GLenum , GLfloat )), glTexParameterf);
    glTexParameterfv = RESOLVE((void (APIENTRY *)(GLenum , GLenum , const GLfloat *)), glTexParameterfv);
    glTexParameteri = RESOLVE((void (APIENTRY *)(GLenum , GLenum , GLint )), glTexParameteri);
    glTexParameteriv = RESOLVE((void (APIENTRY *)(GLenum , GLenum , const GLint *)), glTexParameteriv);
    glTexSubImage2D = RESOLVE((void (APIENTRY *)(GLenum , GLint , GLint , GLint , GLsizei , GLsizei , GLenum , GLenum , const GLvoid *)), glTexSubImage2D);
    glViewport = RESOLVE((void (APIENTRY *)(GLint , GLint , GLsizei , GLsizei )), glViewport);

    glActiveTexture = RESOLVE((void (APIENTRY *)(GLenum)), glActiveTexture);
    glAttachShader = RESOLVE((void (APIENTRY *)(GLuint , GLuint )), glAttachShader);
    glBindAttribLocation = RESOLVE((void (APIENTRY *)(GLuint , GLuint , const GLchar* )), glBindAttribLocation);
    glBindBuffer = RESOLVE((void (APIENTRY *)(GLenum , GLuint )), glBindBuffer);
    glBindFramebuffer = RESOLVE((void (APIENTRY *)(GLenum , GLuint )), glBindFramebuffer);
    glBindRenderbuffer = RESOLVE((void (APIENTRY *)(GLenum , GLuint )), glBindRenderbuffer);
    glBlendColor = RESOLVE((void (APIENTRY *)(GLclampf , GLclampf , GLclampf , GLclampf )), glBlendColor);
    glBlendEquation = RESOLVE((void (APIENTRY *)(GLenum )), glBlendEquation);
    glBlendEquationSeparate = RESOLVE((void (APIENTRY *)(GLenum , GLenum )), glBlendEquationSeparate);
    glBlendFuncSeparate = RESOLVE((void (APIENTRY *)(GLenum , GLenum , GLenum , GLenum )), glBlendFuncSeparate);
    glBufferData = RESOLVE((void (APIENTRY *)(GLenum , qopengl_GLsizeiptr , const GLvoid* , GLenum )), glBufferData);
    glBufferSubData = RESOLVE((void (APIENTRY *)(GLenum , qopengl_GLintptr , qopengl_GLsizeiptr , const GLvoid* )), glBufferSubData);
    glCheckFramebufferStatus = RESOLVE((GLenum (APIENTRY *)(GLenum )), glCheckFramebufferStatus);
    glCompileShader = RESOLVE((void (APIENTRY *)(GLuint )), glCompileShader);
    glCompressedTexImage2D = RESOLVE((void (APIENTRY *)(GLenum , GLint , GLenum , GLsizei , GLsizei, GLint, GLsizei, const GLvoid* )), glCompressedTexImage2D);
    glCompressedTexSubImage2D = RESOLVE((void (APIENTRY *)(GLenum , GLint , GLint , GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid* )), glCompressedTexSubImage2D);
    glCreateProgram = RESOLVE((GLuint (APIENTRY *)(void)), glCreateProgram);
    glCreateShader = RESOLVE((GLuint (APIENTRY *)(GLenum )), glCreateShader);
    glDeleteBuffers = RESOLVE((void (APIENTRY *)(GLsizei , const GLuint*)), glDeleteBuffers);
    glDeleteFramebuffers = RESOLVE((void (APIENTRY *)(GLsizei , const GLuint* )), glDeleteFramebuffers);
    glDeleteProgram = RESOLVE((void (APIENTRY *)(GLuint )), glDeleteProgram);
    glDeleteRenderbuffers = RESOLVE((void (APIENTRY *)(GLsizei , const GLuint* )), glDeleteRenderbuffers);
    glDeleteShader = RESOLVE((void (APIENTRY *)(GLuint )), glDeleteShader);
    glDetachShader = RESOLVE((void (APIENTRY *)(GLuint , GLuint )), glDetachShader);
    glDisableVertexAttribArray = RESOLVE((void (APIENTRY *)(GLuint )), glDisableVertexAttribArray);
    glEnableVertexAttribArray = RESOLVE((void (APIENTRY *)(GLuint )), glEnableVertexAttribArray);
    glFramebufferRenderbuffer = RESOLVE((void (APIENTRY *)(GLenum , GLenum , GLenum , GLuint )), glFramebufferRenderbuffer);
    glFramebufferTexture2D = RESOLVE((void (APIENTRY *)(GLenum , GLenum , GLenum , GLuint , GLint )), glFramebufferTexture2D);
    glGenBuffers = RESOLVE((void (APIENTRY *)(GLsizei , GLuint* )), glGenBuffers);
    glGenerateMipmap = RESOLVE((void (APIENTRY *)(GLenum )), glGenerateMipmap);
    glGenFramebuffers = RESOLVE((void (APIENTRY *)(GLsizei , GLuint* )), glGenFramebuffers);
    glGenRenderbuffers = RESOLVE((void (APIENTRY *)(GLsizei , GLuint* )), glGenRenderbuffers);
    glGetActiveAttrib = RESOLVE((void (APIENTRY *)(GLuint , GLuint , GLsizei , GLsizei* , GLint* , GLenum* , GLchar* )), glGetActiveAttrib);
    glGetActiveUniform = RESOLVE((void (APIENTRY *)(GLuint , GLuint , GLsizei , GLsizei* , GLint* , GLenum* , GLchar* )), glGetActiveUniform);
    glGetAttachedShaders = RESOLVE((void (APIENTRY *)(GLuint , GLsizei , GLsizei*, GLuint* )), glGetAttachedShaders);
    glGetAttribLocation = RESOLVE((int (APIENTRY *)(GLuint , const GLchar* )), glGetAttribLocation);
    glGetBufferParameteriv = RESOLVE((void (APIENTRY *)(GLenum , GLenum , GLint* )), glGetBufferParameteriv);
    glGetFramebufferAttachmentParameteriv = RESOLVE((void (APIENTRY *)(GLenum , GLenum, GLenum , GLint* )), glGetFramebufferAttachmentParameteriv);
    glGetProgramiv = RESOLVE((void (APIENTRY *)(GLuint , GLenum , GLint* )), glGetProgramiv);
    glGetProgramInfoLog = RESOLVE((void (APIENTRY *)(GLuint , GLsizei , GLsizei* , GLchar* )), glGetProgramInfoLog);
    glGetRenderbufferParameteriv = RESOLVE((void (APIENTRY *)(GLenum , GLenum , GLint* )), glGetRenderbufferParameteriv);
    glGetShaderiv = RESOLVE((void (APIENTRY *)(GLuint , GLenum , GLint* )), glGetShaderiv);
    glGetShaderInfoLog = RESOLVE((void (APIENTRY *)(GLuint , GLsizei , GLsizei*, GLchar*)), glGetShaderInfoLog);
    glGetShaderPrecisionFormat = RESOLVE((void (APIENTRY *)(GLenum , GLenum , GLint* , GLint* )), glGetShaderPrecisionFormat);
    glGetShaderSource = RESOLVE((void (APIENTRY *)(GLuint , GLsizei , GLsizei* , GLchar* )), glGetShaderSource);
    glGetUniformfv = RESOLVE((void (APIENTRY *)(GLuint , GLint , GLfloat*)), glGetUniformfv);
    glGetUniformiv = RESOLVE((void (APIENTRY *)(GLuint , GLint , GLint*)), glGetUniformiv);
    glGetUniformLocation = RESOLVE((int (APIENTRY *)(GLuint , const GLchar* )), glGetUniformLocation);
    glGetVertexAttribfv = RESOLVE((void (APIENTRY *)(GLuint , GLenum , GLfloat* )), glGetVertexAttribfv);
    glGetVertexAttribiv = RESOLVE((void (APIENTRY *)(GLuint , GLenum , GLint* )), glGetVertexAttribiv);
    glGetVertexAttribPointerv = RESOLVE((void (APIENTRY *)(GLuint , GLenum , GLvoid** pointer)), glGetVertexAttribPointerv);
    glIsBuffer = RESOLVE((GLboolean (APIENTRY *)(GLuint )), glIsBuffer);
    glIsFramebuffer = RESOLVE((GLboolean (APIENTRY *)(GLuint )), glIsFramebuffer);
    glIsProgram = RESOLVE((GLboolean (APIENTRY *)(GLuint )), glIsProgram);
    glIsRenderbuffer = RESOLVE((GLboolean (APIENTRY *)(GLuint )), glIsRenderbuffer);
    glIsShader = RESOLVE((GLboolean (APIENTRY *)(GLuint )), glIsShader);
    glLinkProgram = RESOLVE((void (APIENTRY *)(GLuint )), glLinkProgram);
    glReleaseShaderCompiler = RESOLVE((void (APIENTRY *)(void)), glReleaseShaderCompiler);
    glRenderbufferStorage = RESOLVE((void (APIENTRY *)(GLenum , GLenum , GLsizei , GLsizei )), glRenderbufferStorage);
    glSampleCoverage = RESOLVE((void (APIENTRY *)(GLclampf , GLboolean )), glSampleCoverage);
    glShaderBinary = RESOLVE((void (APIENTRY *)(GLsizei , const GLuint*, GLenum , const GLvoid* , GLsizei )), glShaderBinary);
    glShaderSource = RESOLVE((void (APIENTRY *)(GLuint , GLsizei , const GLchar* *, const GLint* )), glShaderSource);
    glStencilFuncSeparate = RESOLVE((void (APIENTRY *)(GLenum , GLenum , GLint , GLuint )), glStencilFuncSeparate);
    glStencilMaskSeparate = RESOLVE((void (APIENTRY *)(GLenum , GLuint )), glStencilMaskSeparate);
    glStencilOpSeparate = RESOLVE((void (APIENTRY *)(GLenum , GLenum , GLenum , GLenum )), glStencilOpSeparate);
    glUniform1f = RESOLVE((void (APIENTRY *)(GLint , GLfloat )), glUniform1f);
    glUniform1fv = RESOLVE((void (APIENTRY *)(GLint , GLsizei , const GLfloat* )), glUniform1fv);
    glUniform1i = RESOLVE((void (APIENTRY *)(GLint , GLint )), glUniform1i);
    glUniform1iv = RESOLVE((void (APIENTRY *)(GLint , GLsizei , const GLint* )), glUniform1iv);
    glUniform2f = RESOLVE((void (APIENTRY *)(GLint , GLfloat , GLfloat )), glUniform2f);
    glUniform2fv = RESOLVE((void (APIENTRY *)(GLint , GLsizei , const GLfloat* )), glUniform2fv);
    glUniform2i = RESOLVE((void (APIENTRY *)(GLint , GLint , GLint )), glUniform2i);
    glUniform2iv = RESOLVE((void (APIENTRY *)(GLint , GLsizei , const GLint* )), glUniform2iv);
    glUniform3f = RESOLVE((void (APIENTRY *)(GLint , GLfloat , GLfloat , GLfloat )), glUniform3f);
    glUniform3fv = RESOLVE((void (APIENTRY *)(GLint , GLsizei , const GLfloat* )), glUniform3fv);
    glUniform3i = RESOLVE((void (APIENTRY *)(GLint , GLint , GLint , GLint )), glUniform3i);
    glUniform3iv = RESOLVE((void (APIENTRY *)(GLint , GLsizei , const GLint* )), glUniform3iv);
    glUniform4f = RESOLVE((void (APIENTRY *)(GLint , GLfloat , GLfloat , GLfloat , GLfloat )), glUniform4f);
    glUniform4fv = RESOLVE((void (APIENTRY *)(GLint , GLsizei , const GLfloat* )), glUniform4fv);
    glUniform4i = RESOLVE((void (APIENTRY *)(GLint , GLint , GLint , GLint , GLint )), glUniform4i);
    glUniform4iv = RESOLVE((void (APIENTRY *)(GLint , GLsizei , const GLint* )), glUniform4iv);
    glUniformMatrix2fv = RESOLVE((void (APIENTRY *)(GLint , GLsizei , GLboolean , const GLfloat* )), glUniformMatrix2fv);
    glUniformMatrix3fv = RESOLVE((void (APIENTRY *)(GLint , GLsizei , GLboolean , const GLfloat* )), glUniformMatrix3fv);
    glUniformMatrix4fv = RESOLVE((void (APIENTRY *)(GLint , GLsizei , GLboolean , const GLfloat* )), glUniformMatrix4fv);
    glUseProgram = RESOLVE((void (APIENTRY *)(GLuint )), glUseProgram);
    glValidateProgram = RESOLVE((void (APIENTRY *)(GLuint )), glValidateProgram);
    glVertexAttrib1f = RESOLVE((void (APIENTRY *)(GLuint , GLfloat )), glVertexAttrib1f);
    glVertexAttrib1fv = RESOLVE((void (APIENTRY *)(GLuint , const GLfloat* )), glVertexAttrib1fv);
    glVertexAttrib2f = RESOLVE((void (APIENTRY *)(GLuint , GLfloat , GLfloat )), glVertexAttrib2f);
    glVertexAttrib2fv = RESOLVE((void (APIENTRY *)(GLuint , const GLfloat* )), glVertexAttrib2fv);
    glVertexAttrib3f = RESOLVE((void (APIENTRY *)(GLuint , GLfloat , GLfloat , GLfloat )), glVertexAttrib3f);
    glVertexAttrib3fv = RESOLVE((void (APIENTRY *)(GLuint , const GLfloat* )), glVertexAttrib3fv);
    glVertexAttrib4f = RESOLVE((void (APIENTRY *)(GLuint , GLfloat , GLfloat , GLfloat , GLfloat )), glVertexAttrib4f);
    glVertexAttrib4fv = RESOLVE((void (APIENTRY *)(GLuint , const GLfloat* )), glVertexAttrib4fv);
    glVertexAttribPointer = RESOLVE((void (APIENTRY *)(GLuint , GLint, GLenum, GLboolean, GLsizei, const GLvoid* )), glVertexAttribPointer);

    glClearDepthf = RESOLVE((void (APIENTRY *)(GLclampf )), glClearDepthf);
    glDepthRangef = RESOLVE((void (APIENTRY *)(GLclampf , GLclampf )), glDepthRangef);

    return glBindTexture && glCreateShader && glClearDepthf;
}

QWindowsEGLStaticContext::QWindowsEGLStaticContext(EGLDisplay display, int version)
    : m_display(display), m_version(version)
{
}

QWindowsEGLStaticContext *QWindowsEGLStaticContext::create(QWindowsOpenGLTester::Renderers preferredType)
{
    const HDC dc = QWindowsContext::instance()->displayContext();
    if (!dc){
        qWarning("%s: No Display", Q_FUNC_INFO);
        return 0;
    }

    if (!libEGL.init()) {
        qWarning("%s: Failed to load and resolve libEGL functions", Q_FUNC_INFO);
        return 0;
    }
    if (!libGLESv2.init()) {
        qWarning("%s: Failed to load and resolve libGLESv2 functions", Q_FUNC_INFO);
        return 0;
    }

    EGLDisplay display = EGL_NO_DISPLAY;
    EGLint major = 0;
    EGLint minor = 0;
#ifdef EGL_ANGLE_platform_angle_opengl
    if (libEGL.eglGetPlatformDisplayEXT
        && (preferredType & QWindowsOpenGLTester::AngleBackendMask)) {
        const EGLint anglePlatformAttributes[][5] = {
            { EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE, EGL_NONE },
            { EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE, EGL_NONE },
            { EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE, EGL_PLATFORM_ANGLE_USE_WARP_ANGLE, EGL_TRUE, EGL_NONE }
        };
        const EGLint *attributes = 0;
        if (preferredType & QWindowsOpenGLTester::AngleRendererD3d11)
            attributes = anglePlatformAttributes[0];
        else if (preferredType & QWindowsOpenGLTester::AngleRendererD3d9)
            attributes = anglePlatformAttributes[1];
        else if (preferredType & QWindowsOpenGLTester::AngleRendererD3d11Warp)
            attributes = anglePlatformAttributes[2];
        if (attributes) {
            display = libEGL.eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, dc, attributes);
            if (!libEGL.eglInitialize(display, &major, &minor)) {
                display = EGL_NO_DISPLAY;
                major = minor = 0;
            }
        }
    }
#else // EGL_ANGLE_platform_angle_opengl
    Q_UNUSED(preferredType)
#endif
    if (display == EGL_NO_DISPLAY)
        display = libEGL.eglGetDisplay((EGLNativeDisplayType)dc);
    if (!display) {
        qWarning("%s: Could not obtain EGL display", Q_FUNC_INFO);
        return 0;
    }

    if (!major && !libEGL.eglInitialize(display, &major, &minor)) {
        int err = libEGL.eglGetError();
        qWarning("%s: Could not initialize EGL display: error 0x%x\n", Q_FUNC_INFO, err);
        if (err == 0x3001)
            qWarning("%s: When using ANGLE, check if d3dcompiler_4x.dll is available", Q_FUNC_INFO);
        return 0;
    }

    qCDebug(lcQpaGl) << __FUNCTION__ << "Created EGL display" << display << 'v' <<major << '.' << minor;
    return new QWindowsEGLStaticContext(display, (major << 8) | minor);
}

QWindowsEGLStaticContext::~QWindowsEGLStaticContext()
{
    qCDebug(lcQpaGl) << __FUNCTION__ << "Releasing EGL display " << m_display;
    libEGL.eglTerminate(m_display);
}

QWindowsOpenGLContext *QWindowsEGLStaticContext::createContext(QOpenGLContext *context)
{
    return new QWindowsEGLContext(this, context->format(), context->shareHandle());
}

void *QWindowsEGLStaticContext::createWindowSurface(void *nativeWindow, void *nativeConfig)
{
    EGLSurface surface = libEGL.eglCreateWindowSurface(m_display, (EGLConfig) nativeConfig,
                                                       (EGLNativeWindowType) nativeWindow, 0);
    if (surface == EGL_NO_SURFACE)
        qWarning("%s: Could not create the EGL window surface: 0x%x\n", Q_FUNC_INFO, libEGL.eglGetError());

    return surface;
}

void QWindowsEGLStaticContext::destroyWindowSurface(void *nativeSurface)
{
    libEGL.eglDestroySurface(m_display, (EGLSurface) nativeSurface);
}

QSurfaceFormat QWindowsEGLStaticContext::formatFromConfig(EGLDisplay display, EGLConfig config,
                                                          const QSurfaceFormat &referenceFormat)
{
    QSurfaceFormat format;
    EGLint redSize     = 0;
    EGLint greenSize   = 0;
    EGLint blueSize    = 0;
    EGLint alphaSize   = 0;
    EGLint depthSize   = 0;
    EGLint stencilSize = 0;
    EGLint sampleCount = 0;

    libEGL.eglGetConfigAttrib(display, config, EGL_RED_SIZE,     &redSize);
    libEGL.eglGetConfigAttrib(display, config, EGL_GREEN_SIZE,   &greenSize);
    libEGL.eglGetConfigAttrib(display, config, EGL_BLUE_SIZE,    &blueSize);
    libEGL.eglGetConfigAttrib(display, config, EGL_ALPHA_SIZE,   &alphaSize);
    libEGL.eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE,   &depthSize);
    libEGL.eglGetConfigAttrib(display, config, EGL_STENCIL_SIZE, &stencilSize);
    libEGL.eglGetConfigAttrib(display, config, EGL_SAMPLES,      &sampleCount);

    format.setRenderableType(QSurfaceFormat::OpenGLES);
    format.setVersion(referenceFormat.majorVersion(), referenceFormat.minorVersion());
    format.setProfile(referenceFormat.profile());
    format.setOptions(referenceFormat.options());

    format.setRedBufferSize(redSize);
    format.setGreenBufferSize(greenSize);
    format.setBlueBufferSize(blueSize);
    format.setAlphaBufferSize(alphaSize);
    format.setDepthBufferSize(depthSize);
    format.setStencilBufferSize(stencilSize);
    format.setSamples(sampleCount);
    format.setStereo(false);
    format.setSwapInterval(referenceFormat.swapInterval());

    // Clear the EGL error state because some of the above may
    // have errored out because the attribute is not applicable
    // to the surface type.  Such errors don't matter.
    libEGL.eglGetError();

    return format;
}

/*!
    \class QWindowsEGLContext
    \brief Open EGL context.

    \section1 Using QWindowsEGLContext for Desktop with ANGLE
    \section2 Build Instructions
    \list
    \o Install the Direct X SDK
    \o Checkout and build ANGLE (SVN repository) as explained here:
       \l{http://code.google.com/p/angleproject/wiki/DevSetup}{ANGLE-Project}.
       When building for 64bit, de-activate the "WarnAsError" option
       in every project file (as otherwise integer conversion
       warnings will break the build).
    \o Run configure.exe with the options "-opengl es2".
    \o Build qtbase and test some examples.
    \endlist

    \internal
    \ingroup qt-lighthouse-win
*/

QWindowsEGLContext::QWindowsEGLContext(QWindowsEGLStaticContext *staticContext,
                                       const QSurfaceFormat &format,
                                       QPlatformOpenGLContext *share)
    : m_staticContext(staticContext)
    , m_eglDisplay(staticContext->display())
    , m_api(EGL_OPENGL_ES_API)
    , m_swapInterval(-1)
{
    if (!m_staticContext)
        return;

    m_eglConfig = chooseConfig(format);
    m_format = m_staticContext->formatFromConfig(m_eglDisplay, m_eglConfig, format);
    m_shareContext = share ? static_cast<QWindowsEGLContext *>(share)->m_eglContext : 0;

    QVector<EGLint> contextAttrs;
    contextAttrs.append(EGL_CONTEXT_CLIENT_VERSION);
    contextAttrs.append(m_format.majorVersion());
    contextAttrs.append(EGL_NONE);

    QWindowsEGLStaticContext::libEGL.eglBindAPI(m_api);
    m_eglContext = QWindowsEGLStaticContext::libEGL.eglCreateContext(m_eglDisplay, m_eglConfig, m_shareContext, contextAttrs.constData());
    if (m_eglContext == EGL_NO_CONTEXT && m_shareContext != EGL_NO_CONTEXT) {
        m_shareContext = 0;
        m_eglContext = QWindowsEGLStaticContext::libEGL.eglCreateContext(m_eglDisplay, m_eglConfig, 0, contextAttrs.constData());
    }

    if (m_eglContext == EGL_NO_CONTEXT) {
        qWarning("QWindowsEGLContext: eglError: %x, this: %p \n", QWindowsEGLStaticContext::libEGL.eglGetError(), this);
        return;
    }

    // Make the context current to ensure the GL version query works. This needs a surface too.
    const EGLint pbufferAttributes[] = {
        EGL_WIDTH, 1,
        EGL_HEIGHT, 1,
        EGL_LARGEST_PBUFFER, EGL_FALSE,
        EGL_NONE
    };
    EGLSurface pbuffer = QWindowsEGLStaticContext::libEGL.eglCreatePbufferSurface(m_eglDisplay, m_eglConfig, pbufferAttributes);
    if (pbuffer == EGL_NO_SURFACE)
        return;

    EGLDisplay prevDisplay = QWindowsEGLStaticContext::libEGL.eglGetCurrentDisplay();
    if (prevDisplay == EGL_NO_DISPLAY) // when no context is current
        prevDisplay = m_eglDisplay;
    EGLContext prevContext = QWindowsEGLStaticContext::libEGL.eglGetCurrentContext();
    EGLSurface prevSurfaceDraw = QWindowsEGLStaticContext::libEGL.eglGetCurrentSurface(EGL_DRAW);
    EGLSurface prevSurfaceRead = QWindowsEGLStaticContext::libEGL.eglGetCurrentSurface(EGL_READ);

    if (QWindowsEGLStaticContext::libEGL.eglMakeCurrent(m_eglDisplay, pbuffer, pbuffer, m_eglContext)) {
        const GLubyte *s = QWindowsEGLStaticContext::libGLESv2.glGetString(GL_VERSION);
        if (s) {
            QByteArray version = QByteArray(reinterpret_cast<const char *>(s));
            int major, minor;
            if (QPlatformOpenGLContext::parseOpenGLVersion(version, major, minor)) {
                m_format.setMajorVersion(major);
                m_format.setMinorVersion(minor);
            }
        }
        m_format.setProfile(QSurfaceFormat::NoProfile);
        m_format.setOptions(QSurfaceFormat::FormatOptions());
        QWindowsEGLStaticContext::libEGL.eglMakeCurrent(prevDisplay, prevSurfaceDraw, prevSurfaceRead, prevContext);
    }
    QWindowsEGLStaticContext::libEGL.eglDestroySurface(m_eglDisplay, pbuffer);
}

QWindowsEGLContext::~QWindowsEGLContext()
{
    if (m_eglContext != EGL_NO_CONTEXT) {
        QWindowsEGLStaticContext::libEGL.eglDestroyContext(m_eglDisplay, m_eglContext);
        m_eglContext = EGL_NO_CONTEXT;
    }
}

bool QWindowsEGLContext::makeCurrent(QPlatformSurface *surface)
{
    Q_ASSERT(surface->surface()->supportsOpenGL());

    QWindowsEGLStaticContext::libEGL.eglBindAPI(m_api);

    QWindowsWindow *window = static_cast<QWindowsWindow *>(surface);
    EGLSurface eglSurface = static_cast<EGLSurface>(window->surface(m_eglConfig));
    Q_ASSERT(eglSurface);

    // shortcut: on some GPUs, eglMakeCurrent is not a cheap operation
    if (QWindowsEGLStaticContext::libEGL.eglGetCurrentContext() == m_eglContext &&
            QWindowsEGLStaticContext::libEGL.eglGetCurrentDisplay() == m_eglDisplay &&
            QWindowsEGLStaticContext::libEGL.eglGetCurrentSurface(EGL_READ) == eglSurface &&
            QWindowsEGLStaticContext::libEGL.eglGetCurrentSurface(EGL_DRAW) == eglSurface) {
        return true;
    }

    const bool ok = QWindowsEGLStaticContext::libEGL.eglMakeCurrent(m_eglDisplay, eglSurface, eglSurface, m_eglContext);
    if (ok) {
        const int requestedSwapInterval = surface->format().swapInterval();
        if (requestedSwapInterval >= 0 && m_swapInterval != requestedSwapInterval) {
            m_swapInterval = requestedSwapInterval;
            QWindowsEGLStaticContext::libEGL.eglSwapInterval(m_staticContext->display(), m_swapInterval);
        }
    } else {
        qWarning("QWindowsEGLContext::makeCurrent: eglError: %x, this: %p \n", QWindowsEGLStaticContext::libEGL.eglGetError(), this);
    }

    return ok;
}

void QWindowsEGLContext::doneCurrent()
{
    QWindowsEGLStaticContext::libEGL.eglBindAPI(m_api);
    bool ok = QWindowsEGLStaticContext::libEGL.eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (!ok)
        qWarning("QWindowsEGLContext::doneCurrent: eglError: %d, this: %p \n", QWindowsEGLStaticContext::libEGL.eglGetError(), this);
}

void QWindowsEGLContext::swapBuffers(QPlatformSurface *surface)
{
    QWindowsEGLStaticContext::libEGL.eglBindAPI(m_api);
    QWindowsWindow *window = static_cast<QWindowsWindow *>(surface);
    EGLSurface eglSurface = static_cast<EGLSurface>(window->surface(m_eglConfig));
    Q_ASSERT(eglSurface);

    bool ok = QWindowsEGLStaticContext::libEGL.eglSwapBuffers(m_eglDisplay, eglSurface);
    if (!ok)
        qWarning("QWindowsEGLContext::swapBuffers: eglError: %d, this: %p \n", QWindowsEGLStaticContext::libEGL.eglGetError(), this);
}

QFunctionPointer QWindowsEGLContext::getProcAddress(const QByteArray &procName)
{
    // We support AllGLFunctionsQueryable, which means this function must be able to
    // return a function pointer for standard GLES2 functions too. These are not
    // guaranteed to be queryable via eglGetProcAddress().
    static struct StdFunc {
        const char *name;
        void *func;
    } standardFuncs[] = {
        { "glBindTexture", (void *) QWindowsEGLStaticContext::libGLESv2.glBindTexture },
        { "glBlendFunc", (void *) QWindowsEGLStaticContext::libGLESv2.glBlendFunc },
        { "glClear", (void *) QWindowsEGLStaticContext::libGLESv2.glClear },
        { "glClearColor", (void *) QWindowsEGLStaticContext::libGLESv2.glClearColor },
        { "glClearStencil", (void *) QWindowsEGLStaticContext::libGLESv2.glClearStencil },
        { "glColorMask", (void *) QWindowsEGLStaticContext::libGLESv2.glColorMask },
        { "glCopyTexImage2D", (void *) QWindowsEGLStaticContext::libGLESv2.glCopyTexImage2D },
        { "glCopyTexSubImage2D", (void *) QWindowsEGLStaticContext::libGLESv2.glCopyTexSubImage2D },
        { "glCullFace", (void *) QWindowsEGLStaticContext::libGLESv2.glCullFace },
        { "glDeleteTextures", (void *) QWindowsEGLStaticContext::libGLESv2.glDeleteTextures },
        { "glDepthFunc", (void *) QWindowsEGLStaticContext::libGLESv2.glDepthFunc },
        { "glDepthMask", (void *) QWindowsEGLStaticContext::libGLESv2.glDepthMask },
        { "glDisable", (void *) QWindowsEGLStaticContext::libGLESv2.glDisable },
        { "glDrawArrays", (void *) QWindowsEGLStaticContext::libGLESv2.glDrawArrays },
        { "glDrawElements", (void *) QWindowsEGLStaticContext::libGLESv2.glDrawElements },
        { "glEnable", (void *) QWindowsEGLStaticContext::libGLESv2.glEnable },
        { "glFinish", (void *) QWindowsEGLStaticContext::libGLESv2.glFinish },
        { "glFlush", (void *) QWindowsEGLStaticContext::libGLESv2.glFlush },
        { "glFrontFace", (void *) QWindowsEGLStaticContext::libGLESv2.glFrontFace },
        { "glGenTextures", (void *) QWindowsEGLStaticContext::libGLESv2.glGenTextures },
        { "glGetBooleanv", (void *) QWindowsEGLStaticContext::libGLESv2.glGetBooleanv },
        { "glGetError", (void *) QWindowsEGLStaticContext::libGLESv2.glGetError },
        { "glGetFloatv", (void *) QWindowsEGLStaticContext::libGLESv2.glGetFloatv },
        { "glGetIntegerv", (void *) QWindowsEGLStaticContext::libGLESv2.glGetIntegerv },
        { "glGetString", (void *) QWindowsEGLStaticContext::libGLESv2.glGetString },
        { "glGetTexParameterfv", (void *) QWindowsEGLStaticContext::libGLESv2.glGetTexParameterfv },
        { "glGetTexParameteriv", (void *) QWindowsEGLStaticContext::libGLESv2.glGetTexParameteriv },
        { "glHint", (void *) QWindowsEGLStaticContext::libGLESv2.glHint },
        { "glIsEnabled", (void *) QWindowsEGLStaticContext::libGLESv2.glIsEnabled },
        { "glIsTexture", (void *) QWindowsEGLStaticContext::libGLESv2.glIsTexture },
        { "glLineWidth", (void *) QWindowsEGLStaticContext::libGLESv2.glLineWidth },
        { "glPixelStorei", (void *) QWindowsEGLStaticContext::libGLESv2.glPixelStorei },
        { "glPolygonOffset", (void *) QWindowsEGLStaticContext::libGLESv2.glPolygonOffset },
        { "glReadPixels", (void *) QWindowsEGLStaticContext::libGLESv2.glReadPixels },
        { "glScissor", (void *) QWindowsEGLStaticContext::libGLESv2.glScissor },
        { "glStencilFunc", (void *) QWindowsEGLStaticContext::libGLESv2.glStencilFunc },
        { "glStencilMask", (void *) QWindowsEGLStaticContext::libGLESv2.glStencilMask },
        { "glStencilOp", (void *) QWindowsEGLStaticContext::libGLESv2.glStencilOp },
        { "glTexImage2D", (void *) QWindowsEGLStaticContext::libGLESv2.glTexImage2D },
        { "glTexParameterf", (void *) QWindowsEGLStaticContext::libGLESv2.glTexParameterf },
        { "glTexParameterfv", (void *) QWindowsEGLStaticContext::libGLESv2.glTexParameterfv },
        { "glTexParameteri", (void *) QWindowsEGLStaticContext::libGLESv2.glTexParameteri },
        { "glTexParameteriv", (void *) QWindowsEGLStaticContext::libGLESv2.glTexParameteriv },
        { "glTexSubImage2D", (void *) QWindowsEGLStaticContext::libGLESv2.glTexSubImage2D },
        { "glViewport", (void *) QWindowsEGLStaticContext::libGLESv2.glViewport },

        { "glActiveTexture", (void *) QWindowsEGLStaticContext::libGLESv2.glActiveTexture },
        { "glAttachShader", (void *) QWindowsEGLStaticContext::libGLESv2.glAttachShader },
        { "glBindAttribLocation", (void *) QWindowsEGLStaticContext::libGLESv2.glBindAttribLocation },
        { "glBindBuffer", (void *) QWindowsEGLStaticContext::libGLESv2.glBindBuffer },
        { "glBindFramebuffer", (void *) QWindowsEGLStaticContext::libGLESv2.glBindFramebuffer },
        { "glBindRenderbuffer", (void *) QWindowsEGLStaticContext::libGLESv2.glBindRenderbuffer },
        { "glBlendColor", (void *) QWindowsEGLStaticContext::libGLESv2.glBlendColor },
        { "glBlendEquation", (void *) QWindowsEGLStaticContext::libGLESv2.glBlendEquation },
        { "glBlendEquationSeparate", (void *) QWindowsEGLStaticContext::libGLESv2.glBlendEquationSeparate },
        { "glBlendFuncSeparate", (void *) QWindowsEGLStaticContext::libGLESv2.glBlendFuncSeparate },
        { "glBufferData", (void *) QWindowsEGLStaticContext::libGLESv2.glBufferData },
        { "glBufferSubData", (void *) QWindowsEGLStaticContext::libGLESv2.glBufferSubData },
        { "glCheckFramebufferStatus", (void *) QWindowsEGLStaticContext::libGLESv2.glCheckFramebufferStatus },
        { "glCompileShader", (void *) QWindowsEGLStaticContext::libGLESv2.glCompileShader },
        { "glCompressedTexImage2D", (void *) QWindowsEGLStaticContext::libGLESv2.glCompressedTexImage2D },
        { "glCompressedTexSubImage2D", (void *) QWindowsEGLStaticContext::libGLESv2.glCompressedTexSubImage2D },
        { "glCreateProgram", (void *) QWindowsEGLStaticContext::libGLESv2.glCreateProgram },
        { "glCreateShader", (void *) QWindowsEGLStaticContext::libGLESv2.glCreateShader },
        { "glDeleteBuffers", (void *) QWindowsEGLStaticContext::libGLESv2.glDeleteBuffers },
        { "glDeleteFramebuffers", (void *) QWindowsEGLStaticContext::libGLESv2.glDeleteFramebuffers },
        { "glDeleteProgram", (void *) QWindowsEGLStaticContext::libGLESv2.glDeleteProgram },
        { "glDeleteRenderbuffers", (void *) QWindowsEGLStaticContext::libGLESv2.glDeleteRenderbuffers },
        { "glDeleteShader", (void *) QWindowsEGLStaticContext::libGLESv2.glDeleteShader },
        { "glDetachShader", (void *) QWindowsEGLStaticContext::libGLESv2.glDetachShader },
        { "glDisableVertexAttribArray", (void *) QWindowsEGLStaticContext::libGLESv2.glDisableVertexAttribArray },
        { "glEnableVertexAttribArray", (void *) QWindowsEGLStaticContext::libGLESv2.glEnableVertexAttribArray },
        { "glFramebufferRenderbuffer", (void *) QWindowsEGLStaticContext::libGLESv2.glFramebufferRenderbuffer },
        { "glFramebufferTexture2D", (void *) QWindowsEGLStaticContext::libGLESv2.glFramebufferTexture2D },
        { "glGenBuffers", (void *) QWindowsEGLStaticContext::libGLESv2.glGenBuffers },
        { "glGenerateMipmap", (void *) QWindowsEGLStaticContext::libGLESv2.glGenerateMipmap },
        { "glGenFramebuffers", (void *) QWindowsEGLStaticContext::libGLESv2.glGenFramebuffers },
        { "glGenRenderbuffers", (void *) QWindowsEGLStaticContext::libGLESv2.glGenRenderbuffers },
        { "glGetActiveAttrib", (void *) QWindowsEGLStaticContext::libGLESv2.glGetActiveAttrib },
        { "glGetActiveUniform", (void *) QWindowsEGLStaticContext::libGLESv2.glGetActiveUniform },
        { "glGetAttachedShaders", (void *) QWindowsEGLStaticContext::libGLESv2.glGetAttachedShaders },
        { "glGetAttribLocation", (void *) QWindowsEGLStaticContext::libGLESv2.glGetAttribLocation },
        { "glGetBufferParameteriv", (void *) QWindowsEGLStaticContext::libGLESv2.glGetBufferParameteriv },
        { "glGetFramebufferAttachmentParameteriv", (void *) QWindowsEGLStaticContext::libGLESv2.glGetFramebufferAttachmentParameteriv },
        { "glGetProgramiv", (void *) QWindowsEGLStaticContext::libGLESv2.glGetProgramiv },
        { "glGetProgramInfoLog", (void *) QWindowsEGLStaticContext::libGLESv2.glGetProgramInfoLog },
        { "glGetRenderbufferParameteriv", (void *) QWindowsEGLStaticContext::libGLESv2.glGetRenderbufferParameteriv },
        { "glGetShaderiv", (void *) QWindowsEGLStaticContext::libGLESv2.glGetShaderiv },
        { "glGetShaderInfoLog", (void *) QWindowsEGLStaticContext::libGLESv2.glGetShaderInfoLog },
        { "glGetShaderPrecisionFormat", (void *) QWindowsEGLStaticContext::libGLESv2.glGetShaderPrecisionFormat },
        { "glGetShaderSource", (void *) QWindowsEGLStaticContext::libGLESv2.glGetShaderSource },
        { "glGetUniformfv", (void *) QWindowsEGLStaticContext::libGLESv2.glGetUniformfv },
        { "glGetUniformiv", (void *) QWindowsEGLStaticContext::libGLESv2.glGetUniformiv },
        { "glGetUniformLocation", (void *) QWindowsEGLStaticContext::libGLESv2.glGetUniformLocation },
        { "glGetVertexAttribfv", (void *) QWindowsEGLStaticContext::libGLESv2.glGetVertexAttribfv },
        { "glGetVertexAttribiv", (void *) QWindowsEGLStaticContext::libGLESv2.glGetVertexAttribiv },
        { "glGetVertexAttribPointerv", (void *) QWindowsEGLStaticContext::libGLESv2.glGetVertexAttribPointerv },
        { "glIsBuffer", (void *) QWindowsEGLStaticContext::libGLESv2.glIsBuffer },
        { "glIsFramebuffer", (void *) QWindowsEGLStaticContext::libGLESv2.glIsFramebuffer },
        { "glIsProgram", (void *) QWindowsEGLStaticContext::libGLESv2.glIsProgram },
        { "glIsRenderbuffer", (void *) QWindowsEGLStaticContext::libGLESv2.glIsRenderbuffer },
        { "glIsShader", (void *) QWindowsEGLStaticContext::libGLESv2.glIsShader },
        { "glLinkProgram", (void *) QWindowsEGLStaticContext::libGLESv2.glLinkProgram },
        { "glReleaseShaderCompiler", (void *) QWindowsEGLStaticContext::libGLESv2.glReleaseShaderCompiler },
        { "glRenderbufferStorage", (void *) QWindowsEGLStaticContext::libGLESv2.glRenderbufferStorage },
        { "glSampleCoverage", (void *) QWindowsEGLStaticContext::libGLESv2.glSampleCoverage },
        { "glShaderBinary", (void *) QWindowsEGLStaticContext::libGLESv2.glShaderBinary },
        { "glShaderSource", (void *) QWindowsEGLStaticContext::libGLESv2.glShaderSource },
        { "glStencilFuncSeparate", (void *) QWindowsEGLStaticContext::libGLESv2.glStencilFuncSeparate },
        { "glStencilMaskSeparate", (void *) QWindowsEGLStaticContext::libGLESv2.glStencilMaskSeparate },
        { "glStencilOpSeparate", (void *) QWindowsEGLStaticContext::libGLESv2.glStencilOpSeparate },
        { "glUniform1f", (void *) QWindowsEGLStaticContext::libGLESv2.glUniform1f },
        { "glUniform1fv", (void *) QWindowsEGLStaticContext::libGLESv2.glUniform1fv },
        { "glUniform1i", (void *) QWindowsEGLStaticContext::libGLESv2.glUniform1i },
        { "glUniform1iv", (void *) QWindowsEGLStaticContext::libGLESv2.glUniform1iv },
        { "glUniform2f", (void *) QWindowsEGLStaticContext::libGLESv2.glUniform2f },
        { "glUniform2fv", (void *) QWindowsEGLStaticContext::libGLESv2.glUniform2fv },
        { "glUniform2i", (void *) QWindowsEGLStaticContext::libGLESv2.glUniform2i },
        { "glUniform2iv", (void *) QWindowsEGLStaticContext::libGLESv2.glUniform2iv },
        { "glUniform3f", (void *) QWindowsEGLStaticContext::libGLESv2.glUniform3f },
        { "glUniform3fv", (void *) QWindowsEGLStaticContext::libGLESv2.glUniform3fv },
        { "glUniform3i", (void *) QWindowsEGLStaticContext::libGLESv2.glUniform3i },
        { "glUniform3iv", (void *) QWindowsEGLStaticContext::libGLESv2.glUniform3iv },
        { "glUniform4f", (void *) QWindowsEGLStaticContext::libGLESv2.glUniform4f },
        { "glUniform4fv", (void *) QWindowsEGLStaticContext::libGLESv2.glUniform4fv },
        { "glUniform4i", (void *) QWindowsEGLStaticContext::libGLESv2.glUniform4i },
        { "glUniform4iv", (void *) QWindowsEGLStaticContext::libGLESv2.glUniform4iv },
        { "glUniformMatrix2fv", (void *) QWindowsEGLStaticContext::libGLESv2.glUniformMatrix2fv },
        { "glUniformMatrix3fv", (void *) QWindowsEGLStaticContext::libGLESv2.glUniformMatrix3fv },
        { "glUniformMatrix4fv", (void *) QWindowsEGLStaticContext::libGLESv2.glUniformMatrix4fv },
        { "glUseProgram", (void *) QWindowsEGLStaticContext::libGLESv2.glUseProgram },
        { "glValidateProgram", (void *) QWindowsEGLStaticContext::libGLESv2.glValidateProgram },
        { "glVertexAttrib1f", (void *) QWindowsEGLStaticContext::libGLESv2.glVertexAttrib1f },
        { "glVertexAttrib1fv", (void *) QWindowsEGLStaticContext::libGLESv2.glVertexAttrib1fv },
        { "glVertexAttrib2f", (void *) QWindowsEGLStaticContext::libGLESv2.glVertexAttrib2f },
        { "glVertexAttrib2fv", (void *) QWindowsEGLStaticContext::libGLESv2.glVertexAttrib2fv },
        { "glVertexAttrib3f", (void *) QWindowsEGLStaticContext::libGLESv2.glVertexAttrib3f },
        { "glVertexAttrib3fv", (void *) QWindowsEGLStaticContext::libGLESv2.glVertexAttrib3fv },
        { "glVertexAttrib4f", (void *) QWindowsEGLStaticContext::libGLESv2.glVertexAttrib4f },
        { "glVertexAttrib4fv", (void *) QWindowsEGLStaticContext::libGLESv2.glVertexAttrib4fv },
        { "glVertexAttribPointer", (void *) QWindowsEGLStaticContext::libGLESv2.glVertexAttribPointer },

        { "glClearDepthf", (void *) QWindowsEGLStaticContext::libGLESv2.glClearDepthf },
        { "glDepthRangef", (void *) QWindowsEGLStaticContext::libGLESv2.glDepthRangef }
    };
    for (size_t i = 0; i < sizeof(standardFuncs) / sizeof(StdFunc); ++i)
        if (procName == standardFuncs[i].name)
            return reinterpret_cast<QFunctionPointer>(standardFuncs[i].func);

    QWindowsEGLStaticContext::libEGL.eglBindAPI(m_api);
    QFunctionPointer procAddress = reinterpret_cast<QFunctionPointer>(QWindowsEGLStaticContext::libEGL.eglGetProcAddress(procName.constData()));
    if (QWindowsContext::verbose > 1)
        qCDebug(lcQpaGl) << __FUNCTION__ <<  procName << QWindowsEGLStaticContext::libEGL.eglGetCurrentContext() << "returns" << procAddress;
    if (!procAddress && QWindowsContext::verbose)
        qWarning("%s: Unable to resolve '%s'", __FUNCTION__, procName.constData());
    return procAddress;
}

static QVector<EGLint> createConfigAttributesFromFormat(const QSurfaceFormat &format)
{
    int redSize     = format.redBufferSize();
    int greenSize   = format.greenBufferSize();
    int blueSize    = format.blueBufferSize();
    int alphaSize   = format.alphaBufferSize();
    int depthSize   = format.depthBufferSize();
    int stencilSize = format.stencilBufferSize();
    int sampleCount = format.samples();

    QVector<EGLint> configAttributes;
    configAttributes.reserve(16);

    configAttributes.append(EGL_RED_SIZE);
    configAttributes.append(redSize > 0 ? redSize : 0);

    configAttributes.append(EGL_GREEN_SIZE);
    configAttributes.append(greenSize > 0 ? greenSize : 0);

    configAttributes.append(EGL_BLUE_SIZE);
    configAttributes.append(blueSize > 0 ? blueSize : 0);

    configAttributes.append(EGL_ALPHA_SIZE);
    configAttributes.append(alphaSize > 0 ? alphaSize : 0);

    configAttributes.append(EGL_DEPTH_SIZE);
    configAttributes.append(depthSize > 0 ? depthSize : 0);

    configAttributes.append(EGL_STENCIL_SIZE);
    configAttributes.append(stencilSize > 0 ? stencilSize : 0);

    configAttributes.append(EGL_SAMPLES);
    configAttributes.append(sampleCount > 0 ? sampleCount : 0);

    configAttributes.append(EGL_SAMPLE_BUFFERS);
    configAttributes.append(sampleCount > 0);

    return configAttributes;
}

static bool reduceConfigAttributes(QVector<EGLint> *configAttributes)
{
    int i = -1;

    i = configAttributes->indexOf(EGL_SWAP_BEHAVIOR);
    if (i >= 0) {
        configAttributes->remove(i,2);
    }

    i = configAttributes->indexOf(EGL_BUFFER_SIZE);
    if (i >= 0) {
        if (configAttributes->at(i+1) == 16) {
            configAttributes->remove(i,2);
            return true;
        }
    }

    i = configAttributes->indexOf(EGL_SAMPLES);
    if (i >= 0) {
        EGLint value = configAttributes->value(i+1, 0);
        if (value > 1)
            configAttributes->replace(i+1, qMin(EGLint(16), value / 2));
        else
            configAttributes->remove(i, 2);
        return true;
    }

    i = configAttributes->indexOf(EGL_SAMPLE_BUFFERS);
    if (i >= 0) {
        configAttributes->remove(i,2);
        return true;
    }

    i = configAttributes->indexOf(EGL_ALPHA_SIZE);
    if (i >= 0) {
        configAttributes->remove(i,2);
#if defined(EGL_BIND_TO_TEXTURE_RGBA) && defined(EGL_BIND_TO_TEXTURE_RGB)
        i = configAttributes->indexOf(EGL_BIND_TO_TEXTURE_RGBA);
        if (i >= 0) {
            configAttributes->replace(i,EGL_BIND_TO_TEXTURE_RGB);
            configAttributes->replace(i+1,true);

        }
#endif
        return true;
    }

    i = configAttributes->indexOf(EGL_STENCIL_SIZE);
    if (i >= 0) {
        if (configAttributes->at(i + 1) > 1)
            configAttributes->replace(i + 1, 1);
        else
            configAttributes->remove(i, 2);
        return true;
    }

    i = configAttributes->indexOf(EGL_DEPTH_SIZE);
    if (i >= 0) {
        if (configAttributes->at(i + 1) > 1)
            configAttributes->replace(i + 1, 1);
        else
            configAttributes->remove(i, 2);
        return true;
    }
#ifdef EGL_BIND_TO_TEXTURE_RGB
    i = configAttributes->indexOf(EGL_BIND_TO_TEXTURE_RGB);
    if (i >= 0) {
        configAttributes->remove(i,2);
        return true;
    }
#endif

    return false;
}

EGLConfig QWindowsEGLContext::chooseConfig(const QSurfaceFormat &format)
{
    QVector<EGLint> configureAttributes = createConfigAttributesFromFormat(format);
    configureAttributes.append(EGL_SURFACE_TYPE);
    configureAttributes.append(EGL_WINDOW_BIT);
    configureAttributes.append(EGL_RENDERABLE_TYPE);
    configureAttributes.append(EGL_OPENGL_ES2_BIT);
    configureAttributes.append(EGL_NONE);

    EGLDisplay display = m_staticContext->display();
    EGLConfig cfg = 0;
    do {
        // Get the number of matching configurations for this set of properties.
        EGLint matching = 0;
        if (!QWindowsEGLStaticContext::libEGL.eglChooseConfig(display, configureAttributes.constData(), 0, 0, &matching) || !matching)
            continue;

        // Fetch all of the matching configurations and find the
        // first that matches the pixel format we wanted.
        int i = configureAttributes.indexOf(EGL_RED_SIZE);
        int confAttrRed = configureAttributes.at(i+1);
        i = configureAttributes.indexOf(EGL_GREEN_SIZE);
        int confAttrGreen = configureAttributes.at(i+1);
        i = configureAttributes.indexOf(EGL_BLUE_SIZE);
        int confAttrBlue = configureAttributes.at(i+1);
        i = configureAttributes.indexOf(EGL_ALPHA_SIZE);
        int confAttrAlpha = i == -1 ? 0 : configureAttributes.at(i+1);

        QVector<EGLConfig> configs(matching);
        QWindowsEGLStaticContext::libEGL.eglChooseConfig(display, configureAttributes.constData(), configs.data(), configs.size(), &matching);
        if (!cfg && matching > 0)
            cfg = configs.first();

        EGLint red = 0;
        EGLint green = 0;
        EGLint blue = 0;
        EGLint alpha = 0;
        for (int i = 0; i < configs.size(); ++i) {
            if (confAttrRed)
                QWindowsEGLStaticContext::libEGL.eglGetConfigAttrib(display, configs[i], EGL_RED_SIZE, &red);
            if (confAttrGreen)
                QWindowsEGLStaticContext::libEGL.eglGetConfigAttrib(display, configs[i], EGL_GREEN_SIZE, &green);
            if (confAttrBlue)
                QWindowsEGLStaticContext::libEGL.eglGetConfigAttrib(display, configs[i], EGL_BLUE_SIZE, &blue);
            if (confAttrAlpha)
                QWindowsEGLStaticContext::libEGL.eglGetConfigAttrib(display, configs[i], EGL_ALPHA_SIZE, &alpha);

            if (red == confAttrRed && green == confAttrGreen
                    && blue == confAttrBlue && alpha == confAttrAlpha)
                return configs[i];
        }
    } while (reduceConfigAttributes(&configureAttributes));

    if (!cfg)
        qWarning("Cannot find EGLConfig, returning null config");

    return cfg;
}

QT_END_NAMESPACE
