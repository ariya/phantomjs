/****************************************************************************
**
** Copyright (C) 2014 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Sean Harmer <sean.harmer@kdab.com>
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

#ifndef QOPENGLVERTEXARRAYOBJECT_P_H
#define QOPENGLVERTEXARRAYOBJECT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Qt OpenGL classes.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>

#ifndef QT_NO_OPENGL

#include <QtGui/qopengl.h>

QT_BEGIN_NAMESPACE

class QOpenGLVertexArrayObjectHelper;
class QOpenGLContext;

void Q_GUI_EXPORT qtInitializeVertexArrayObjectHelper(QOpenGLVertexArrayObjectHelper *helper, QOpenGLContext *context);

class QOpenGLVertexArrayObjectHelper
{
    Q_DISABLE_COPY(QOpenGLVertexArrayObjectHelper)

public:
    explicit inline QOpenGLVertexArrayObjectHelper(QOpenGLContext *context)
        : GenVertexArrays(Q_NULLPTR)
        , DeleteVertexArrays(Q_NULLPTR)
        , BindVertexArray(Q_NULLPTR)
        , IsVertexArray(Q_NULLPTR)
    {
        qtInitializeVertexArrayObjectHelper(this, context);
    }

    inline bool isValid() const
    {
        return GenVertexArrays && DeleteVertexArrays && BindVertexArray && IsVertexArray;
    }

    inline void glGenVertexArrays(GLsizei n, GLuint *arrays) const
    {
        GenVertexArrays(n, arrays);
    }

    inline void glDeleteVertexArrays(GLsizei n, const GLuint *arrays) const
    {
        DeleteVertexArrays(n, arrays);
    }

    inline void glBindVertexArray(GLuint array) const
    {
        BindVertexArray(array);
    }

    inline GLboolean glIsVertexArray(GLuint array) const
    {
        return IsVertexArray(array);
    }

private:
    friend void Q_GUI_EXPORT qtInitializeVertexArrayObjectHelper(QOpenGLVertexArrayObjectHelper *helper, QOpenGLContext *context);

    // Function signatures are equivalent between desktop core, ARB, APPLE, ES 3 and ES 2 extensions
    typedef void (QOPENGLF_APIENTRYP qt_GenVertexArrays_t)(GLsizei n, GLuint *arrays);
    typedef void (QOPENGLF_APIENTRYP qt_DeleteVertexArrays_t)(GLsizei n, const GLuint *arrays);
    typedef void (QOPENGLF_APIENTRYP qt_BindVertexArray_t)(GLuint array);
    typedef GLboolean (QOPENGLF_APIENTRYP qt_IsVertexArray_t)(GLuint array);

    qt_GenVertexArrays_t GenVertexArrays;
    qt_DeleteVertexArrays_t DeleteVertexArrays;
    qt_BindVertexArray_t BindVertexArray;
    qt_IsVertexArray_t IsVertexArray;
};

QT_END_NAMESPACE

#endif // QT_NO_OPENGL

#endif // QOPENGLVERTEXARRAYOBJECT_P_H
