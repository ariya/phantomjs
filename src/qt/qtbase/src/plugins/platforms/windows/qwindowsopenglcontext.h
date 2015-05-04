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

#ifndef QWINDOWSOPENGLCONTEXT_H
#define QWINDOWSOPENGLCONTEXT_H

#include <QtGui/QOpenGLContext>
#include <qpa/qplatformopenglcontext.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_OPENGL

class QWindowsOpenGLContext;

class QWindowsStaticOpenGLContext
{
public:
    static QWindowsStaticOpenGLContext *create();
    virtual ~QWindowsStaticOpenGLContext() { }

    virtual QWindowsOpenGLContext *createContext(QOpenGLContext *context) = 0;
    virtual void *moduleHandle() const = 0;
    virtual QOpenGLContext::OpenGLModuleType moduleType() const = 0;
    virtual bool supportsThreadedOpenGL() const { return false; }

    // If the windowing system interface needs explicitly created window surfaces (like EGL),
    // reimplement these.
    virtual void *createWindowSurface(void * /*nativeWindow*/, void * /*nativeConfig*/) { return 0; }
    virtual void destroyWindowSurface(void * /*nativeSurface*/) { }

private:
    static QWindowsStaticOpenGLContext *doCreate();
};

class QWindowsOpenGLContext : public QPlatformOpenGLContext
{
public:
    virtual ~QWindowsOpenGLContext() { }

    // Returns the native context handle (e.g. HGLRC for WGL, EGLContext for EGL).
    virtual void *nativeContext() const = 0;

    // These should be implemented only for some winsys interfaces, for example EGL.
    // For others, like WGL, they are not relevant.
    virtual void *nativeDisplay() const { return 0; }
    virtual void *nativeConfig() const { return 0; }
};

#endif // QT_NO_OPENGL

QT_END_NAMESPACE

#endif // QWINDOWSOPENGLCONTEXT_H
