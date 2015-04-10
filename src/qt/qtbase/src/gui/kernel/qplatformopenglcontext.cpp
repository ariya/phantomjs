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

#include "qplatformopenglcontext.h"

#include <QOpenGLFunctions>

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformOpenGLContext
    \since 4.8
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformOpenGLContext class provides an abstraction for native GL contexts.

    In QPA the way to support OpenGL or OpenVG or other technologies that requires a native GL
    context is through the QPlatformOpenGLContext wrapper.

    There is no factory function for QPlatformOpenGLContexts, but rather only one accessor function.
    The only place to retrieve a QPlatformOpenGLContext from is through a QPlatformWindow.

    The context which is current for a specific thread can be collected by the currentContext()
    function. This is how QPlatformOpenGLContext also makes it possible to use the Qt GUI module
    withhout using QOpenGLWidget. When using QOpenGLContext::currentContext(), it will ask
    QPlatformOpenGLContext for the currentContext. Then a corresponding QOpenGLContext will be returned,
    which maps to the QPlatformOpenGLContext.
*/

/*! \fn void QPlatformOpenGLContext::swapBuffers(QPlatformSurface *surface)
    Reimplement in subclass to native swap buffers calls

    The implementation must support being called in a thread different than the gui-thread.
*/

/*! \fn QFunctionPointer QPlatformOpenGLContext::getProcAddress(const QByteArray &procName)
    Reimplement in subclass to native getProcAddr calls.

    Note: its convenient to use qPrintable(const QString &str) to get the const char * pointer
*/

class QPlatformOpenGLContextPrivate
{
public:
    QPlatformOpenGLContextPrivate() : context(0) {}

    QOpenGLContext *context;
};

QPlatformOpenGLContext::QPlatformOpenGLContext()
    : d_ptr(new QPlatformOpenGLContextPrivate)
{
}

QPlatformOpenGLContext::~QPlatformOpenGLContext()
{
}

/*!
    Reimplement in subclass if your platform uses framebuffer objects for surfaces.

    The default implementation returns 0.
*/
GLuint QPlatformOpenGLContext::defaultFramebufferObject(QPlatformSurface *) const
{
    return 0;
}

QOpenGLContext *QPlatformOpenGLContext::context() const
{
    Q_D(const QPlatformOpenGLContext);
    return d->context;
}

void QPlatformOpenGLContext::setContext(QOpenGLContext *context)
{
    Q_D(QPlatformOpenGLContext);
    d->context = context;
}

bool QPlatformOpenGLContext::parseOpenGLVersion(const QByteArray &versionString, int &major, int &minor)
{
    bool majorOk = false;
    bool minorOk = false;
    QList<QByteArray> parts = versionString.split(' ');
    if (versionString.startsWith(QByteArrayLiteral("OpenGL ES"))) {
        if (parts.size() >= 3) {
            QList<QByteArray> versionParts = parts.at(2).split('.');
            if (versionParts.size() >= 2) {
                major = versionParts.at(0).toInt(&majorOk);
                minor = versionParts.at(1).toInt(&minorOk);
            } else {
                qWarning("Unrecognized OpenGL ES version");
            }
        } else {
            // If < 3 parts to the name, it is an unrecognised OpenGL ES
            qWarning("Unrecognised OpenGL ES version");
        }
    } else {
        // Not OpenGL ES, but regular OpenGL, the version numbers are first in the string
        QList<QByteArray> versionParts = parts.at(0).split('.');
        if (versionParts.size() >= 2) {
            major = versionParts.at(0).toInt(&majorOk);
            minor = versionParts.at(1).toInt(&minorOk);
        } else {
            qWarning("Unrecognized OpenGL version");
        }
    }

    if (!majorOk || !minorOk)
        qWarning("Unrecognized OpenGL version");
    return (majorOk && minorOk);
}

QT_END_NAMESPACE
