/****************************************************************************
**
** Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB)
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

#include "qopenglfunctions_es2.h"
#include "qopenglcontext.h"

QT_BEGIN_NAMESPACE

/*!
    \class QOpenGLFunctions_ES2
    \inmodule QtGui
    \since 5.1
    \wrapper
    \brief The QOpenGLFunctions_ES2 class provides all functions for OpenGL ES 2

    This class is a wrapper for OpenGL ES 2 functions. See reference pages on
    \l {http://www.khronos.org/opengles/sdk/docs/man/}{khronos.org} for
    function documentation.

    \sa QAbstractOpenGLFunctions
*/

QOpenGLFunctions_ES2::QOpenGLFunctions_ES2()
 : QAbstractOpenGLFunctions()
 , d_es2(0)
{
}

QOpenGLFunctions_ES2::~QOpenGLFunctions_ES2()
{
}

bool QOpenGLFunctions_ES2::initializeOpenGLFunctions()
{
    if ( isInitialized() )
        return true;

    QOpenGLContext* context = QOpenGLContext::currentContext();

    // If owned by a context object make sure it is current.
    // Also check that current context is compatible
    if (((owningContext() && owningContext() == context) || !owningContext())
        && QOpenGLFunctions_ES2::isContextCompatible(context))
    {
        // Nothing to do, just flag that we are initialized
        QAbstractOpenGLFunctions::initializeOpenGLFunctions();
    }
    return isInitialized();
}

bool QOpenGLFunctions_ES2::isContextCompatible(QOpenGLContext *context)
{
    Q_ASSERT(context);
    QSurfaceFormat f = context->format();
    const QPair<int, int> v = qMakePair(f.majorVersion(), f.minorVersion());
    if (v < qMakePair(2, 0))
        return false;
    if (f.renderableType() != QSurfaceFormat::OpenGLES)
        return false;

    return true;
}

QOpenGLVersionProfile QOpenGLFunctions_ES2::versionProfile()
{
    QOpenGLVersionProfile v;
    return v;
}

QT_END_NAMESPACE
