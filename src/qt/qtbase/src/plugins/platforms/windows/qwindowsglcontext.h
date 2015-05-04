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

#ifndef QWINDOWSGLCONTEXT_H
#define QWINDOWSGLCONTEXT_H

#include "array.h"
#include "qtwindows_additional.h"
#include "qwindowsopenglcontext.h"

#include <QtGui/QOpenGLContext>

QT_BEGIN_NAMESPACE

class QDebug;

enum QWindowsGLFormatFlags
{
    QWindowsGLDirectRendering = 0x1,
    QWindowsGLOverlay = 0x2,
    QWindowsGLRenderToPixmap = 0x4,
    QWindowsGLAccumBuffer = 0x8
};

// Additional format information for Windows.
struct QWindowsOpenGLAdditionalFormat
{
    QWindowsOpenGLAdditionalFormat(unsigned formatFlagsIn = 0, unsigned pixmapDepthIn = 0) :
        formatFlags(formatFlagsIn), pixmapDepth(pixmapDepthIn) { }
    unsigned formatFlags; // QWindowsGLFormatFlags.
    unsigned pixmapDepth; // for QWindowsGLRenderToPixmap
};

// Per-window data for active OpenGL contexts.
struct QOpenGLContextData
{
    QOpenGLContextData(HGLRC r, HWND h, HDC d) : renderingContext(r), hwnd(h), hdc(d) {}
    QOpenGLContextData() : renderingContext(0), hwnd(0), hdc(0) {}

    HGLRC renderingContext;
    HWND hwnd;
    HDC hdc;
};

class QOpenGLStaticContext;

struct QWindowsOpenGLContextFormat
{
    QWindowsOpenGLContextFormat();
    static QWindowsOpenGLContextFormat current();
    void apply(QSurfaceFormat *format) const;

    QSurfaceFormat::OpenGLContextProfile profile;
    int version; //! majorVersion<<8 + minorVersion
    QSurfaceFormat::FormatOptions options;
};

QDebug operator<<(QDebug d, const QWindowsOpenGLContextFormat &);

struct QWindowsOpengl32DLL
{
    bool init(bool softwareRendering);
    void *moduleHandle() const { return m_lib; }
    bool moduleIsNotOpengl32() const { return m_nonOpengl32; }

    // Wrappers. Always use these instead of SwapBuffers/wglSwapBuffers/etc.
    BOOL swapBuffers(HDC dc);
    BOOL setPixelFormat(HDC dc, int pf, const PIXELFORMATDESCRIPTOR *pfd);

    // WGL
    HGLRC (WINAPI * wglCreateContext)(HDC dc);
    BOOL (WINAPI * wglDeleteContext)(HGLRC context);
    HGLRC (WINAPI * wglGetCurrentContext)();
    HDC (WINAPI * wglGetCurrentDC)();
    PROC (WINAPI * wglGetProcAddress)(LPCSTR name);
    BOOL (WINAPI * wglMakeCurrent)(HDC dc, HGLRC context);
    BOOL (WINAPI * wglShareLists)(HGLRC context1, HGLRC context2);

    // GL1+GLES2 common
    void (APIENTRY * glBindTexture)(GLenum target, GLuint texture);
    void (APIENTRY * glBlendFunc)(GLenum sfactor, GLenum dfactor);
    void (APIENTRY * glClear)(GLbitfield mask);
    void (APIENTRY * glClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
    void (APIENTRY * glClearStencil)(GLint s);
    void (APIENTRY * glColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
    void (APIENTRY * glCopyTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
    void (APIENTRY * glCopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
    void (APIENTRY * glCullFace)(GLenum mode);
    void (APIENTRY * glDeleteTextures)(GLsizei n, const GLuint* textures);
    void (APIENTRY * glDepthFunc)(GLenum func);
    void (APIENTRY * glDepthMask)(GLboolean flag);
    void (APIENTRY * glDisable)(GLenum cap);
    void (APIENTRY * glDrawArrays)(GLenum mode, GLint first, GLsizei count);
    void (APIENTRY * glDrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices);
    void (APIENTRY * glEnable)(GLenum cap);
    void (APIENTRY * glFinish)();
    void (APIENTRY * glFlush)();
    void (APIENTRY * glFrontFace)(GLenum mode);
    void (APIENTRY * glGenTextures)(GLsizei n, GLuint* textures);
    void (APIENTRY * glGetBooleanv)(GLenum pname, GLboolean* params);
    GLenum (APIENTRY * glGetError)();
    void (APIENTRY * glGetFloatv)(GLenum pname, GLfloat* params);
    void (APIENTRY * glGetIntegerv)(GLenum pname, GLint* params);
    const GLubyte * (APIENTRY * glGetString)(GLenum name);
    void (APIENTRY * glGetTexParameterfv)(GLenum target, GLenum pname, GLfloat* params);
    void (APIENTRY * glGetTexParameteriv)(GLenum target, GLenum pname, GLint* params);
    void (APIENTRY * glHint)(GLenum target, GLenum mode);
    GLboolean (APIENTRY * glIsEnabled)(GLenum cap);
    GLboolean (APIENTRY * glIsTexture)(GLuint texture);
    void (APIENTRY * glLineWidth)(GLfloat width);
    void (APIENTRY * glPixelStorei)(GLenum pname, GLint param);
    void (APIENTRY * glPolygonOffset)(GLfloat factor, GLfloat units);
    void (APIENTRY * glReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels);
    void (APIENTRY * glScissor)(GLint x, GLint y, GLsizei width, GLsizei height);
    void (APIENTRY * glStencilFunc)(GLenum func, GLint ref, GLuint mask);
    void (APIENTRY * glStencilMask)(GLuint mask);
    void (APIENTRY * glStencilOp)(GLenum fail, GLenum zfail, GLenum zpass);
    void (APIENTRY * glTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
    void (APIENTRY * glTexParameterf)(GLenum target, GLenum pname, GLfloat param);
    void (APIENTRY * glTexParameterfv)(GLenum target, GLenum pname, const GLfloat* params);
    void (APIENTRY * glTexParameteri)(GLenum target, GLenum pname, GLint param);
    void (APIENTRY * glTexParameteriv)(GLenum target, GLenum pname, const GLint* params);
    void (APIENTRY * glTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels);
    void (APIENTRY * glViewport)(GLint x, GLint y, GLsizei width, GLsizei height);

    // GL only
    void (APIENTRY * glClearDepth)(GLdouble depth);
    void (APIENTRY * glDepthRange)(GLdouble zNear, GLdouble zFar);

private:
    void *resolve(const char *name);
    HMODULE m_lib;
    bool m_nonOpengl32;

    // For Mesa llvmpipe shipped with a name other than opengl32.dll
    BOOL (WINAPI * wglSwapBuffers)(HDC dc);
    BOOL (WINAPI * wglSetPixelFormat)(HDC dc, int pf, const PIXELFORMATDESCRIPTOR *pfd);
};

class QOpenGLStaticContext : public QWindowsStaticOpenGLContext
{
    Q_DISABLE_COPY(QOpenGLStaticContext)
    QOpenGLStaticContext();
public:
    enum Extensions
    {
        SampleBuffers = 0x1
    };

    typedef bool
        (APIENTRY *WglGetPixelFormatAttribIVARB)
            (HDC hdc, int iPixelFormat, int iLayerPlane,
             uint nAttributes, const int *piAttributes, int *piValues);

    typedef bool
        (APIENTRY *WglChoosePixelFormatARB)(HDC hdc, const int *piAttribList,
            const float *pfAttribFList, uint nMaxFormats, int *piFormats,
            UINT *nNumFormats);

    typedef HGLRC
        (APIENTRY *WglCreateContextAttribsARB)(HDC, HGLRC, const int *);

    typedef BOOL
        (APIENTRY *WglSwapInternalExt)(int interval);
    typedef int
        (APIENTRY *WglGetSwapInternalExt)(void);

    bool hasExtensions() const
        { return wglGetPixelFormatAttribIVARB && wglChoosePixelFormatARB && wglCreateContextAttribsARB; }

    static QOpenGLStaticContext *create(bool softwareRendering = false);
    static QByteArray getGlString(unsigned int which);

    QWindowsOpenGLContext *createContext(QOpenGLContext *context);
    void *moduleHandle() const { return opengl32.moduleHandle(); }
    QOpenGLContext::OpenGLModuleType moduleType() const { return QOpenGLContext::LibGL; }

    // For a regular opengl32.dll report the ThreadedOpenGL capability.
    // For others, which are likely to be software-only, don't.
    bool supportsThreadedOpenGL() const { return !opengl32.moduleIsNotOpengl32(); }

    const QByteArray vendor;
    const QByteArray renderer;
    const QByteArray extensionNames;
    unsigned extensions;
    const QWindowsOpenGLContextFormat defaultFormat;

    WglGetPixelFormatAttribIVARB wglGetPixelFormatAttribIVARB;
    WglChoosePixelFormatARB wglChoosePixelFormatARB;
    WglCreateContextAttribsARB wglCreateContextAttribsARB;
    WglSwapInternalExt wglSwapInternalExt;
    WglGetSwapInternalExt wglGetSwapInternalExt;

    static QWindowsOpengl32DLL opengl32;
};

QDebug operator<<(QDebug d, const QOpenGLStaticContext &);

class QWindowsGLContext : public QWindowsOpenGLContext
{
public:
    explicit QWindowsGLContext(QOpenGLStaticContext *staticContext, QOpenGLContext *context);
    ~QWindowsGLContext();
    bool isSharing() const Q_DECL_OVERRIDE { return m_context->shareHandle(); }
    bool isValid() const Q_DECL_OVERRIDE { return m_renderingContext; }
    QSurfaceFormat format() const Q_DECL_OVERRIDE { return m_obtainedFormat; }

    void swapBuffers(QPlatformSurface *surface) Q_DECL_OVERRIDE;

    bool makeCurrent(QPlatformSurface *surface) Q_DECL_OVERRIDE;
    void doneCurrent() Q_DECL_OVERRIDE;

    typedef void (*GL_Proc) ();

    QFunctionPointer getProcAddress(const QByteArray &procName) Q_DECL_OVERRIDE;

    HGLRC renderingContext() const { return m_renderingContext; }

    void *nativeContext() const Q_DECL_OVERRIDE { return m_renderingContext; }

private:
    inline void releaseDCs();
    bool updateObtainedParams(HDC hdc, int *obtainedSwapInterval = 0);

    QOpenGLStaticContext *m_staticContext;
    QOpenGLContext *m_context;
    QSurfaceFormat m_obtainedFormat;
    HGLRC m_renderingContext;
    Array<QOpenGLContextData> m_windowContexts;
    PIXELFORMATDESCRIPTOR m_obtainedPixelFormatDescriptor;
    int m_pixelFormat;
    bool m_extensionsUsed;
    int m_swapInterval;
    bool m_ownsContext;
};

QT_END_NAMESPACE

#endif // QWINDOWSGLCONTEXT_H
