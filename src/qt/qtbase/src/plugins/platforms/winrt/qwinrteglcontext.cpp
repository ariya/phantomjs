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

#include "qwinrteglcontext.h"

QT_BEGIN_NAMESPACE

QWinRTEGLContext::QWinRTEGLContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, EGLDisplay display, EGLSurface surface, EGLConfig config)
    : QEGLPlatformContext(format, share, display, &config), m_eglSurface(surface)
{
}

EGLSurface QWinRTEGLContext::eglSurfaceForPlatformSurface(QPlatformSurface *surface)
{
    if (surface->surface()->surfaceClass() == QSurface::Window) {
        // All windows use the same surface
        return m_eglSurface;
    } else {
        // TODO: return EGL surfaces for offscreen surfaces
        qWarning("This plugin does not support offscreen surfaces.");
        return EGL_NO_SURFACE;
    }
}

QFunctionPointer QWinRTEGLContext::getProcAddress(const QByteArray &procName)
{
    static QHash<QByteArray, QFunctionPointer> standardFuncs;
    if (standardFuncs.isEmpty()) {
        standardFuncs.insert(QByteArrayLiteral("glBindTexture"), (QFunctionPointer)&glBindTexture);
        standardFuncs.insert(QByteArrayLiteral("glBlendFunc"), (QFunctionPointer)&glBlendFunc);
        standardFuncs.insert(QByteArrayLiteral("glClear"), (QFunctionPointer)&glClear);
        standardFuncs.insert(QByteArrayLiteral("glClearColor"), (QFunctionPointer)&glClearColor);
        standardFuncs.insert(QByteArrayLiteral("glClearStencil"), (QFunctionPointer)&glClearStencil);
        standardFuncs.insert(QByteArrayLiteral("glColorMask"), (QFunctionPointer)&glColorMask);
        standardFuncs.insert(QByteArrayLiteral("glCopyTexImage2D"), (QFunctionPointer)&glCopyTexImage2D);
        standardFuncs.insert(QByteArrayLiteral("glCopyTexSubImage2D"), (QFunctionPointer)&glCopyTexSubImage2D);
        standardFuncs.insert(QByteArrayLiteral("glCullFace"), (QFunctionPointer)&glCullFace);
        standardFuncs.insert(QByteArrayLiteral("glDeleteTextures"), (QFunctionPointer)&glDeleteTextures);
        standardFuncs.insert(QByteArrayLiteral("glDepthFunc"), (QFunctionPointer)&glDepthFunc);
        standardFuncs.insert(QByteArrayLiteral("glDepthMask"), (QFunctionPointer)&glDepthMask);
        standardFuncs.insert(QByteArrayLiteral("glDisable"), (QFunctionPointer)&glDisable);
        standardFuncs.insert(QByteArrayLiteral("glDrawArrays"), (QFunctionPointer)&glDrawArrays);
        standardFuncs.insert(QByteArrayLiteral("glDrawElements"), (QFunctionPointer)&glDrawElements);
        standardFuncs.insert(QByteArrayLiteral("glEnable"), (QFunctionPointer)&glEnable);
        standardFuncs.insert(QByteArrayLiteral("glFinish"), (QFunctionPointer)&glFinish);
        standardFuncs.insert(QByteArrayLiteral("glFlush"), (QFunctionPointer)&glFlush);
        standardFuncs.insert(QByteArrayLiteral("glFrontFace"), (QFunctionPointer)&glFrontFace);
        standardFuncs.insert(QByteArrayLiteral("glGenTextures"), (QFunctionPointer)&glGenTextures);
        standardFuncs.insert(QByteArrayLiteral("glGetBooleanv"), (QFunctionPointer)&glGetBooleanv);
        standardFuncs.insert(QByteArrayLiteral("glGetError"), (QFunctionPointer)&glGetError);
        standardFuncs.insert(QByteArrayLiteral("glGetFloatv"), (QFunctionPointer)&glGetFloatv);
        standardFuncs.insert(QByteArrayLiteral("glGetIntegerv"), (QFunctionPointer)&glGetIntegerv);
        standardFuncs.insert(QByteArrayLiteral("glGetString"), (QFunctionPointer)&glGetString);
        standardFuncs.insert(QByteArrayLiteral("glGetTexParameterfv"), (QFunctionPointer)&glGetTexParameterfv);
        standardFuncs.insert(QByteArrayLiteral("glGetTexParameteriv"), (QFunctionPointer)&glGetTexParameteriv);
        standardFuncs.insert(QByteArrayLiteral("glHint"), (QFunctionPointer)&glHint);
        standardFuncs.insert(QByteArrayLiteral("glIsEnabled"), (QFunctionPointer)&glIsEnabled);
        standardFuncs.insert(QByteArrayLiteral("glIsTexture"), (QFunctionPointer)&glIsTexture);
        standardFuncs.insert(QByteArrayLiteral("glLineWidth"), (QFunctionPointer)&glLineWidth);
        standardFuncs.insert(QByteArrayLiteral("glPixelStorei"), (QFunctionPointer)&glPixelStorei);
        standardFuncs.insert(QByteArrayLiteral("glPolygonOffset"), (QFunctionPointer)&glPolygonOffset);
        standardFuncs.insert(QByteArrayLiteral("glReadPixels"), (QFunctionPointer)&glReadPixels);
        standardFuncs.insert(QByteArrayLiteral("glScissor"), (QFunctionPointer)&glScissor);
        standardFuncs.insert(QByteArrayLiteral("glStencilFunc"), (QFunctionPointer)&glStencilFunc);
        standardFuncs.insert(QByteArrayLiteral("glStencilMask"), (QFunctionPointer)&glStencilMask);
        standardFuncs.insert(QByteArrayLiteral("glStencilOp"), (QFunctionPointer)&glStencilOp);
        standardFuncs.insert(QByteArrayLiteral("glTexImage2D"), (QFunctionPointer)&glTexImage2D);
        standardFuncs.insert(QByteArrayLiteral("glTexParameterf"), (QFunctionPointer)&glTexParameterf);
        standardFuncs.insert(QByteArrayLiteral("glTexParameterfv"), (QFunctionPointer)&glTexParameterfv);
        standardFuncs.insert(QByteArrayLiteral("glTexParameteri"), (QFunctionPointer)&glTexParameteri);
        standardFuncs.insert(QByteArrayLiteral("glTexParameteriv"), (QFunctionPointer)&glTexParameteriv);
        standardFuncs.insert(QByteArrayLiteral("glTexSubImage2D"), (QFunctionPointer)&glTexSubImage2D);
        standardFuncs.insert(QByteArrayLiteral("glViewport"), (QFunctionPointer)&glViewport);
        standardFuncs.insert(QByteArrayLiteral("glActiveTexture"), (QFunctionPointer)&glActiveTexture);
        standardFuncs.insert(QByteArrayLiteral("glAttachShader"), (QFunctionPointer)&glAttachShader);
        standardFuncs.insert(QByteArrayLiteral("glBindAttribLocation"), (QFunctionPointer)&glBindAttribLocation);
        standardFuncs.insert(QByteArrayLiteral("glBindBuffer"), (QFunctionPointer)&glBindBuffer);
        standardFuncs.insert(QByteArrayLiteral("glBindFramebuffer"), (QFunctionPointer)&glBindFramebuffer);
        standardFuncs.insert(QByteArrayLiteral("glBindRenderbuffer"), (QFunctionPointer)&glBindRenderbuffer);
        standardFuncs.insert(QByteArrayLiteral("glBlendColor"), (QFunctionPointer)&glBlendColor);
        standardFuncs.insert(QByteArrayLiteral("glBlendEquation"), (QFunctionPointer)&glBlendEquation);
        standardFuncs.insert(QByteArrayLiteral("glBlendEquationSeparate"), (QFunctionPointer)&glBlendEquationSeparate);
        standardFuncs.insert(QByteArrayLiteral("glBlendFuncSeparate"), (QFunctionPointer)&glBlendFuncSeparate);
        standardFuncs.insert(QByteArrayLiteral("glBufferData"), (QFunctionPointer)&glBufferData);
        standardFuncs.insert(QByteArrayLiteral("glBufferSubData"), (QFunctionPointer)&glBufferSubData);
        standardFuncs.insert(QByteArrayLiteral("glCheckFramebufferStatus"), (QFunctionPointer)&glCheckFramebufferStatus);
        standardFuncs.insert(QByteArrayLiteral("glCompileShader"), (QFunctionPointer)&glCompileShader);
        standardFuncs.insert(QByteArrayLiteral("glCompressedTexImage2D"), (QFunctionPointer)&glCompressedTexImage2D);
        standardFuncs.insert(QByteArrayLiteral("glCompressedTexSubImage2D"), (QFunctionPointer)&glCompressedTexSubImage2D);
        standardFuncs.insert(QByteArrayLiteral("glCreateProgram"), (QFunctionPointer)&glCreateProgram);
        standardFuncs.insert(QByteArrayLiteral("glCreateShader"), (QFunctionPointer)&glCreateShader);
        standardFuncs.insert(QByteArrayLiteral("glDeleteBuffers"), (QFunctionPointer)&glDeleteBuffers);
        standardFuncs.insert(QByteArrayLiteral("glDeleteFramebuffers"), (QFunctionPointer)&glDeleteFramebuffers);
        standardFuncs.insert(QByteArrayLiteral("glDeleteProgram"), (QFunctionPointer)&glDeleteProgram);
        standardFuncs.insert(QByteArrayLiteral("glDeleteRenderbuffers"), (QFunctionPointer)&glDeleteRenderbuffers);
        standardFuncs.insert(QByteArrayLiteral("glDeleteShader"), (QFunctionPointer)&glDeleteShader);
        standardFuncs.insert(QByteArrayLiteral("glDetachShader"), (QFunctionPointer)&glDetachShader);
        standardFuncs.insert(QByteArrayLiteral("glDisableVertexAttribArray"), (QFunctionPointer)&glDisableVertexAttribArray);
        standardFuncs.insert(QByteArrayLiteral("glEnableVertexAttribArray"), (QFunctionPointer)&glEnableVertexAttribArray);
        standardFuncs.insert(QByteArrayLiteral("glFramebufferRenderbuffer"), (QFunctionPointer)&glFramebufferRenderbuffer);
        standardFuncs.insert(QByteArrayLiteral("glFramebufferTexture2D"), (QFunctionPointer)&glFramebufferTexture2D);
        standardFuncs.insert(QByteArrayLiteral("glGenBuffers"), (QFunctionPointer)&glGenBuffers);
        standardFuncs.insert(QByteArrayLiteral("glGenerateMipmap"), (QFunctionPointer)&glGenerateMipmap);
        standardFuncs.insert(QByteArrayLiteral("glGenFramebuffers"), (QFunctionPointer)&glGenFramebuffers);
        standardFuncs.insert(QByteArrayLiteral("glGenRenderbuffers"), (QFunctionPointer)&glGenRenderbuffers);
        standardFuncs.insert(QByteArrayLiteral("glGetActiveAttrib"), (QFunctionPointer)&glGetActiveAttrib);
        standardFuncs.insert(QByteArrayLiteral("glGetActiveUniform"), (QFunctionPointer)&glGetActiveUniform);
        standardFuncs.insert(QByteArrayLiteral("glGetAttachedShaders"), (QFunctionPointer)&glGetAttachedShaders);
        standardFuncs.insert(QByteArrayLiteral("glGetAttribLocation"), (QFunctionPointer)&glGetAttribLocation);
        standardFuncs.insert(QByteArrayLiteral("glGetBufferParameteriv"), (QFunctionPointer)&glGetBufferParameteriv);
        standardFuncs.insert(QByteArrayLiteral("glGetFramebufferAttachmentParameteriv"), (QFunctionPointer)&glGetFramebufferAttachmentParameteriv);
        standardFuncs.insert(QByteArrayLiteral("glGetProgramiv"), (QFunctionPointer)&glGetProgramiv);
        standardFuncs.insert(QByteArrayLiteral("glGetProgramInfoLog"), (QFunctionPointer)&glGetProgramInfoLog);
        standardFuncs.insert(QByteArrayLiteral("glGetRenderbufferParameteriv"), (QFunctionPointer)&glGetRenderbufferParameteriv);
        standardFuncs.insert(QByteArrayLiteral("glGetShaderiv"), (QFunctionPointer)&glGetShaderiv);
        standardFuncs.insert(QByteArrayLiteral("glGetShaderInfoLog"), (QFunctionPointer)&glGetShaderInfoLog);
        standardFuncs.insert(QByteArrayLiteral("glGetShaderPrecisionFormat"), (QFunctionPointer)&glGetShaderPrecisionFormat);
        standardFuncs.insert(QByteArrayLiteral("glGetShaderSource"), (QFunctionPointer)&glGetShaderSource);
        standardFuncs.insert(QByteArrayLiteral("glGetUniformfv"), (QFunctionPointer)&glGetUniformfv);
        standardFuncs.insert(QByteArrayLiteral("glGetUniformiv"), (QFunctionPointer)&glGetUniformiv);
        standardFuncs.insert(QByteArrayLiteral("glGetUniformLocation"), (QFunctionPointer)&glGetUniformLocation);
        standardFuncs.insert(QByteArrayLiteral("glGetVertexAttribfv"), (QFunctionPointer)&glGetVertexAttribfv);
        standardFuncs.insert(QByteArrayLiteral("glGetVertexAttribiv"), (QFunctionPointer)&glGetVertexAttribiv);
        standardFuncs.insert(QByteArrayLiteral("glGetVertexAttribPointerv"), (QFunctionPointer)&glGetVertexAttribPointerv);
        standardFuncs.insert(QByteArrayLiteral("glIsBuffer"), (QFunctionPointer)&glIsBuffer);
        standardFuncs.insert(QByteArrayLiteral("glIsFramebuffer"), (QFunctionPointer)&glIsFramebuffer);
        standardFuncs.insert(QByteArrayLiteral("glIsProgram"), (QFunctionPointer)&glIsProgram);
        standardFuncs.insert(QByteArrayLiteral("glIsRenderbuffer"), (QFunctionPointer)&glIsRenderbuffer);
        standardFuncs.insert(QByteArrayLiteral("glIsShader"), (QFunctionPointer)&glIsShader);
        standardFuncs.insert(QByteArrayLiteral("glLinkProgram"), (QFunctionPointer)&glLinkProgram);
        standardFuncs.insert(QByteArrayLiteral("glReleaseShaderCompiler"), (QFunctionPointer)&glReleaseShaderCompiler);
        standardFuncs.insert(QByteArrayLiteral("glRenderbufferStorage"), (QFunctionPointer)&glRenderbufferStorage);
        standardFuncs.insert(QByteArrayLiteral("glSampleCoverage"), (QFunctionPointer)&glSampleCoverage);
        standardFuncs.insert(QByteArrayLiteral("glShaderBinary"), (QFunctionPointer)&glShaderBinary);
        standardFuncs.insert(QByteArrayLiteral("glShaderSource"), (QFunctionPointer)&glShaderSource);
        standardFuncs.insert(QByteArrayLiteral("glStencilFuncSeparate"), (QFunctionPointer)&glStencilFuncSeparate);
        standardFuncs.insert(QByteArrayLiteral("glStencilMaskSeparate"), (QFunctionPointer)&glStencilMaskSeparate);
        standardFuncs.insert(QByteArrayLiteral("glStencilOpSeparate"), (QFunctionPointer)&glStencilOpSeparate);
        standardFuncs.insert(QByteArrayLiteral("glUniform1f"), (QFunctionPointer)&glUniform1f);
        standardFuncs.insert(QByteArrayLiteral("glUniform1fv"), (QFunctionPointer)&glUniform1fv);
        standardFuncs.insert(QByteArrayLiteral("glUniform1i"), (QFunctionPointer)&glUniform1i);
        standardFuncs.insert(QByteArrayLiteral("glUniform1iv"), (QFunctionPointer)&glUniform1iv);
        standardFuncs.insert(QByteArrayLiteral("glUniform2f"), (QFunctionPointer)&glUniform2f);
        standardFuncs.insert(QByteArrayLiteral("glUniform2fv"), (QFunctionPointer)&glUniform2fv);
        standardFuncs.insert(QByteArrayLiteral("glUniform2i"), (QFunctionPointer)&glUniform2i);
        standardFuncs.insert(QByteArrayLiteral("glUniform2iv"), (QFunctionPointer)&glUniform2iv);
        standardFuncs.insert(QByteArrayLiteral("glUniform3f"), (QFunctionPointer)&glUniform3f);
        standardFuncs.insert(QByteArrayLiteral("glUniform3fv"), (QFunctionPointer)&glUniform3fv);
        standardFuncs.insert(QByteArrayLiteral("glUniform3i"), (QFunctionPointer)&glUniform3i);
        standardFuncs.insert(QByteArrayLiteral("glUniform3iv"), (QFunctionPointer)&glUniform3iv);
        standardFuncs.insert(QByteArrayLiteral("glUniform4f"), (QFunctionPointer)&glUniform4f);
        standardFuncs.insert(QByteArrayLiteral("glUniform4fv"), (QFunctionPointer)&glUniform4fv);
        standardFuncs.insert(QByteArrayLiteral("glUniform4i"), (QFunctionPointer)&glUniform4i);
        standardFuncs.insert(QByteArrayLiteral("glUniform4iv"), (QFunctionPointer)&glUniform4iv);
        standardFuncs.insert(QByteArrayLiteral("glUniformMatrix2fv"), (QFunctionPointer)&glUniformMatrix2fv);
        standardFuncs.insert(QByteArrayLiteral("glUniformMatrix3fv"), (QFunctionPointer)&glUniformMatrix3fv);
        standardFuncs.insert(QByteArrayLiteral("glUniformMatrix4fv"), (QFunctionPointer)&glUniformMatrix4fv);
        standardFuncs.insert(QByteArrayLiteral("glUseProgram"), (QFunctionPointer)&glUseProgram);
        standardFuncs.insert(QByteArrayLiteral("glValidateProgram"), (QFunctionPointer)&glValidateProgram);
        standardFuncs.insert(QByteArrayLiteral("glVertexAttrib1f"), (QFunctionPointer)&glVertexAttrib1f);
        standardFuncs.insert(QByteArrayLiteral("glVertexAttrib1fv"), (QFunctionPointer)&glVertexAttrib1fv);
        standardFuncs.insert(QByteArrayLiteral("glVertexAttrib2f"), (QFunctionPointer)&glVertexAttrib2f);
        standardFuncs.insert(QByteArrayLiteral("glVertexAttrib2fv"), (QFunctionPointer)&glVertexAttrib2fv);
        standardFuncs.insert(QByteArrayLiteral("glVertexAttrib3f"), (QFunctionPointer)&glVertexAttrib3f);
        standardFuncs.insert(QByteArrayLiteral("glVertexAttrib3fv"), (QFunctionPointer)&glVertexAttrib3fv);
        standardFuncs.insert(QByteArrayLiteral("glVertexAttrib4f"), (QFunctionPointer)&glVertexAttrib4f);
        standardFuncs.insert(QByteArrayLiteral("glVertexAttrib4fv"), (QFunctionPointer)&glVertexAttrib4fv);
        standardFuncs.insert(QByteArrayLiteral("glVertexAttribPointer"), (QFunctionPointer)&glVertexAttribPointer);
        standardFuncs.insert(QByteArrayLiteral("glClearDepthf"), (QFunctionPointer)&glClearDepthf);
        standardFuncs.insert(QByteArrayLiteral("glDepthRangef"), (QFunctionPointer)&glDepthRangef);
    };

    QHash<QByteArray, QFunctionPointer>::const_iterator i = standardFuncs.find(procName);
    if (i != standardFuncs.end())
        return i.value();

    return QEGLPlatformContext::getProcAddress(procName);
}

QT_END_NAMESPACE
