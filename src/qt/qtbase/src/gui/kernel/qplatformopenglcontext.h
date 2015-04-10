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

#ifndef QPLATFORMOPENGLCONTEXT_H
#define QPLATFORMOPENGLCONTEXT_H

//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtCore/qnamespace.h>

#ifndef QT_NO_OPENGL

#include <QtGui/qsurfaceformat.h>
#include <QtGui/qwindow.h>
#include <QtGui/qopengl.h>

QT_BEGIN_NAMESPACE


class QPlatformOpenGLContextPrivate;

class Q_GUI_EXPORT QPlatformOpenGLContext
{
    Q_DECLARE_PRIVATE(QPlatformOpenGLContext)
public:
    QPlatformOpenGLContext();
    virtual ~QPlatformOpenGLContext();

    virtual QSurfaceFormat format() const = 0;

    virtual void swapBuffers(QPlatformSurface *surface) = 0;

    virtual GLuint defaultFramebufferObject(QPlatformSurface *surface) const;

    virtual bool makeCurrent(QPlatformSurface *surface) = 0;
    virtual void doneCurrent() = 0;

    virtual bool isSharing() const { return false; }
    virtual bool isValid() const { return true; }

    virtual QFunctionPointer getProcAddress(const QByteArray &procName) = 0;

    QOpenGLContext *context() const;

    static bool parseOpenGLVersion(const QByteArray &versionString, int &major, int &minor);

private:
    friend class QOpenGLContext;

    QScopedPointer<QPlatformOpenGLContextPrivate> d_ptr;

    void setContext(QOpenGLContext *context);

    Q_DISABLE_COPY(QPlatformOpenGLContext)
};

QT_END_NAMESPACE

#endif // QT_NO_OPENGL

#endif // QPLATFORMOPENGLCONTEXT_H
