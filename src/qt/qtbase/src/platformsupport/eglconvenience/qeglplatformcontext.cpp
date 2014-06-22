/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qeglplatformcontext_p.h"
#include "qeglconvenience_p.h"
#include "qeglpbuffer_p.h"
#include <qpa/qplatformwindow.h>
#include <QOpenGLContext>
#include <QDebug>

QT_BEGIN_NAMESPACE

/*!
    \class QEGLPlatformContext
    \brief An EGL context implementation.
    \since 5.2
    \internal
    \ingroup qpa

    Implement QPlatformOpenGLContext using EGL. To use it in platform
    plugins a subclass must be created since
    eglSurfaceForPlatformSurface() has to be reimplemented. This
    function is used for mapping platform surfaces (windows) to EGL
    surfaces and is necessary since different platform plugins may
    have different ways of handling native windows (for example, a
    plugin may choose not to back every platform window by a real EGL
    surface). Other than that, no further customization is necessary.
 */

// Constants from EGL_KHR_create_context
#ifndef EGL_CONTEXT_MINOR_VERSION_KHR
#define EGL_CONTEXT_MINOR_VERSION_KHR 0x30FB
#endif
#ifndef EGL_CONTEXT_FLAGS_KHR
#define EGL_CONTEXT_FLAGS_KHR 0x30FC
#endif
#ifndef EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR
#define EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR 0x30FD
#endif
#ifndef EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR
#define EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR 0x00000001
#endif
#ifndef EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR
#define EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR 0x00000002
#endif
#ifndef EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR
#define EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR 0x00000001
#endif
#ifndef EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR
#define EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR 0x00000002
#endif

// Constants for OpenGL which are not available in the ES headers.
#ifndef GL_CONTEXT_FLAGS
#define GL_CONTEXT_FLAGS 0x821E
#endif
#ifndef GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT
#define GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT 0x0001
#endif
#ifndef GL_CONTEXT_FLAG_DEBUG_BIT
#define GL_CONTEXT_FLAG_DEBUG_BIT 0x00000002
#endif
#ifndef GL_CONTEXT_PROFILE_MASK
#define GL_CONTEXT_PROFILE_MASK 0x9126
#endif
#ifndef GL_CONTEXT_CORE_PROFILE_BIT
#define GL_CONTEXT_CORE_PROFILE_BIT 0x00000001
#endif
#ifndef GL_CONTEXT_COMPATIBILITY_PROFILE_BIT
#define GL_CONTEXT_COMPATIBILITY_PROFILE_BIT 0x00000002
#endif

QEGLPlatformContext::QEGLPlatformContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, EGLDisplay display)
    : m_eglDisplay(display)
    , m_eglConfig(q_configFromGLFormat(display, format))
    , m_swapInterval(-1)
    , m_swapIntervalEnvChecked(false)
    , m_swapIntervalFromEnv(-1)
{
    init(format, share);
}

QEGLPlatformContext::QEGLPlatformContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, EGLDisplay display,
                                         EGLConfig config)
    : m_eglDisplay(display)
    , m_eglConfig(config)
    , m_swapInterval(-1)
    , m_swapIntervalEnvChecked(false)
    , m_swapIntervalFromEnv(-1)
{
    init(format, share);
}

void QEGLPlatformContext::init(const QSurfaceFormat &format, QPlatformOpenGLContext *share)
{
    m_format = q_glFormatFromConfig(m_eglDisplay, m_eglConfig);
    // m_format now has the renderableType() resolved (it cannot be Default anymore)
    // but does not yet contain version, profile, options.
    m_shareContext = share ? static_cast<QEGLPlatformContext *>(share)->m_eglContext : 0;

    QVector<EGLint> contextAttrs;
    contextAttrs.append(EGL_CONTEXT_CLIENT_VERSION);
    contextAttrs.append(format.majorVersion());
    const bool hasKHRCreateContext = q_hasEglExtension(m_eglDisplay, "EGL_KHR_create_context");
    if (hasKHRCreateContext) {
        contextAttrs.append(EGL_CONTEXT_MINOR_VERSION_KHR);
        contextAttrs.append(format.minorVersion());
        int flags = 0;
        // The debug bit is supported both for OpenGL and OpenGL ES.
        if (format.testOption(QSurfaceFormat::DebugContext))
            flags |= EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR;
        // The fwdcompat bit is only for OpenGL 3.0+.
        if (m_format.renderableType() == QSurfaceFormat::OpenGL
            && format.majorVersion() >= 3
            && !format.testOption(QSurfaceFormat::DeprecatedFunctions))
            flags |= EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR;
        if (flags) {
            contextAttrs.append(EGL_CONTEXT_FLAGS_KHR);
            contextAttrs.append(flags);
        }
        // Profiles are OpenGL only and mandatory in 3.2+. The value is silently ignored for < 3.2.
        if (m_format.renderableType() == QSurfaceFormat::OpenGL) {
            contextAttrs.append(EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR);
            contextAttrs.append(format.profile() == QSurfaceFormat::CoreProfile
                                ? EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR
                                : EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR);
        }
    }
    contextAttrs.append(EGL_NONE);

    switch (m_format.renderableType()) {
    case QSurfaceFormat::OpenVG:
        m_api = EGL_OPENVG_API;
        break;
#ifdef EGL_VERSION_1_4
    case QSurfaceFormat::OpenGL:
        m_api = EGL_OPENGL_API;
        break;
#endif // EGL_VERSION_1_4
    default:
        m_api = EGL_OPENGL_ES_API;
        break;
    }

    eglBindAPI(m_api);
    m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, m_shareContext, contextAttrs.constData());
    if (m_eglContext == EGL_NO_CONTEXT && m_shareContext != EGL_NO_CONTEXT) {
        m_shareContext = 0;
        m_eglContext = eglCreateContext(m_eglDisplay, m_eglConfig, 0, contextAttrs.constData());
    }

    if (m_eglContext == EGL_NO_CONTEXT) {
        qWarning("QEGLPlatformContext::init: eglError: %x, this: %p \n", eglGetError(), this);
        return;
    }

    static const bool printConfig = qgetenv("QT_QPA_EGLFS_DEBUG").toInt();
    if (printConfig) {
        qDebug() << "Created context for format" << format << "with config:";
        q_printEglConfig(m_eglDisplay, m_eglConfig);
    }

#ifndef QT_NO_OPENGL
    // Make the context current to ensure the GL version query works. This needs a surface too.
    const EGLint pbufferAttributes[] = {
        EGL_WIDTH, 1,
        EGL_HEIGHT, 1,
        EGL_LARGEST_PBUFFER, EGL_FALSE,
        EGL_NONE
    };
    EGLSurface pbuffer = eglCreatePbufferSurface(m_eglDisplay, m_eglConfig, pbufferAttributes);
    if (pbuffer == EGL_NO_SURFACE)
        return;

    if (eglMakeCurrent(m_eglDisplay, pbuffer, pbuffer, m_eglContext)) {
        if (m_format.renderableType() == QSurfaceFormat::OpenGL
            || m_format.renderableType() == QSurfaceFormat::OpenGLES) {
            const GLubyte *s = glGetString(GL_VERSION);
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
            if (m_format.renderableType() == QSurfaceFormat::OpenGL) {
                // Check profile and options.
                if (m_format.majorVersion() < 3) {
                    m_format.setOption(QSurfaceFormat::DeprecatedFunctions);
                } else {
                    GLint value = 0;
                    glGetIntegerv(GL_CONTEXT_FLAGS, &value);
                    if (!(value & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT))
                        m_format.setOption(QSurfaceFormat::DeprecatedFunctions);
                    if (value & GL_CONTEXT_FLAG_DEBUG_BIT)
                        m_format.setOption(QSurfaceFormat::DebugContext);
                    if (m_format.version() >= qMakePair(3, 2)) {
                        value = 0;
                        glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &value);
                        if (value & GL_CONTEXT_CORE_PROFILE_BIT)
                            m_format.setProfile(QSurfaceFormat::CoreProfile);
                        else if (value & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT)
                            m_format.setProfile(QSurfaceFormat::CompatibilityProfile);
                    }
                }
            }
        }
        eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }
    eglDestroySurface(m_eglDisplay, pbuffer);
#endif // QT_NO_OPENGL
}

bool QEGLPlatformContext::makeCurrent(QPlatformSurface *surface)
{
    Q_ASSERT(surface->surface()->supportsOpenGL());

    eglBindAPI(m_api);

    EGLSurface eglSurface = eglSurfaceForPlatformSurface(surface);

    // shortcut: on some GPUs, eglMakeCurrent is not a cheap operation
    if (eglGetCurrentContext() == m_eglContext &&
        eglGetCurrentDisplay() == m_eglDisplay &&
        eglGetCurrentSurface(EGL_READ) == eglSurface &&
        eglGetCurrentSurface(EGL_DRAW) == eglSurface) {
        return true;
    }

    const bool ok = eglMakeCurrent(m_eglDisplay, eglSurface, eglSurface, m_eglContext);
    if (ok) {
        if (!m_swapIntervalEnvChecked) {
            m_swapIntervalEnvChecked = true;
            if (qEnvironmentVariableIsSet("QT_QPA_EGLFS_SWAPINTERVAL")) {
                QByteArray swapIntervalString = qgetenv("QT_QPA_EGLFS_SWAPINTERVAL");
                bool intervalOk;
                const int swapInterval = swapIntervalString.toInt(&intervalOk);
                if (intervalOk)
                    m_swapIntervalFromEnv = swapInterval;
            }
        }
        const int requestedSwapInterval = m_swapIntervalFromEnv >= 0
            ? m_swapIntervalFromEnv
            : surface->format().swapInterval();
        if (requestedSwapInterval >= 0 && m_swapInterval != requestedSwapInterval) {
            m_swapInterval = requestedSwapInterval;
            eglSwapInterval(eglDisplay(), m_swapInterval);
        }
    } else {
        qWarning("QEGLPlatformContext::makeCurrent: eglError: %x, this: %p \n", eglGetError(), this);
    }

    return ok;
}

QEGLPlatformContext::~QEGLPlatformContext()
{
    if (m_eglContext != EGL_NO_CONTEXT) {
        eglDestroyContext(m_eglDisplay, m_eglContext);
        m_eglContext = EGL_NO_CONTEXT;
    }
}

void QEGLPlatformContext::doneCurrent()
{
    eglBindAPI(m_api);
    bool ok = eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (!ok)
        qWarning("QEGLPlatformContext::doneCurrent(): eglError: %d, this: %p \n", eglGetError(), this);
}

void QEGLPlatformContext::swapBuffers(QPlatformSurface *surface)
{
    eglBindAPI(m_api);
    EGLSurface eglSurface = eglSurfaceForPlatformSurface(surface);
    bool ok = eglSwapBuffers(m_eglDisplay, eglSurface);
    if (!ok)
        qWarning("QEGLPlatformContext::swapBuffers(): eglError: %d, this: %p \n", eglGetError(), this);
}

void (*QEGLPlatformContext::getProcAddress(const QByteArray &procName)) ()
{
    eglBindAPI(m_api);
    return eglGetProcAddress(procName.constData());
}

QSurfaceFormat QEGLPlatformContext::format() const
{
    return m_format;
}

EGLContext QEGLPlatformContext::eglContext() const
{
    return m_eglContext;
}

EGLDisplay QEGLPlatformContext::eglDisplay() const
{
    return m_eglDisplay;
}

EGLConfig QEGLPlatformContext::eglConfig() const
{
    return m_eglConfig;
}

QT_END_NAMESPACE
