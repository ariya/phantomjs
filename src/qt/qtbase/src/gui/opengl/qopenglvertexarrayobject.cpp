/****************************************************************************
**
** Copyright (C) 2013 Klaralvdalens Datakonsult AB (KDAB).
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

#include "qopenglvertexarrayobject.h"

#include <QtCore/private/qobject_p.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/qopenglfunctions.h>
#include <QtGui/qoffscreensurface.h>

#include <QtGui/qopenglfunctions_3_0.h>
#include <QtGui/qopenglfunctions_3_2_core.h>

QT_BEGIN_NAMESPACE

class QOpenGLFunctions_3_0;
class QOpenGLFunctions_3_2_Core;

class QVertexArrayObjectHelper
{
public:
    QVertexArrayObjectHelper(QOpenGLContext *context)
    {
        Q_ASSERT(context);
        if (context->isOpenGLES()) {
            GenVertexArrays = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLsizei , GLuint *)>(context->getProcAddress(QByteArrayLiteral("glGenVertexArraysOES")));
            DeleteVertexArrays = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLsizei , const GLuint *)>(context->getProcAddress(QByteArrayLiteral("glDeleteVertexArraysOES")));
            BindVertexArray = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint )>(context->getProcAddress(QByteArrayLiteral("glBindVertexArrayOES")));
        } else {
            if (context->hasExtension(QByteArrayLiteral("GL_APPLE_vertex_array_object"))) {
                GenVertexArrays = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLsizei , GLuint *)>(context->getProcAddress(QByteArrayLiteral("glGenVertexArraysAPPLE")));
                DeleteVertexArrays = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLsizei , const GLuint *)>(context->getProcAddress(QByteArrayLiteral("glDeleteVertexArraysAPPLE")));
                BindVertexArray = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint )>(context->getProcAddress(QByteArrayLiteral("glBindVertexArrayAPPLE")));
            } else {
                GenVertexArrays = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLsizei , GLuint *)>(context->getProcAddress(QByteArrayLiteral("glGenVertexArrays")));
                DeleteVertexArrays = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLsizei , const GLuint *)>(context->getProcAddress(QByteArrayLiteral("glDeleteVertexArrays")));
                BindVertexArray = reinterpret_cast<void (QOPENGLF_APIENTRYP)(GLuint )>(context->getProcAddress(QByteArrayLiteral("glBindVertexArray")));
            }
        }
    }

    inline void glGenVertexArrays(GLsizei n, GLuint *arrays)
    {
        GenVertexArrays(n, arrays);
    }

    inline void glDeleteVertexArrays(GLsizei n, const GLuint *arrays)
    {
        DeleteVertexArrays(n, arrays);
    }

    inline void glBindVertexArray(GLuint array)
    {
        BindVertexArray(array);
    }

private:
    // Function signatures are equivalent between desktop core, ARB, APPLE and ES 2 extensions
    void (QOPENGLF_APIENTRYP GenVertexArrays)(GLsizei n, GLuint *arrays);
    void (QOPENGLF_APIENTRYP DeleteVertexArrays)(GLsizei n, const GLuint *arrays);
    void (QOPENGLF_APIENTRYP BindVertexArray)(GLuint array);
};

class QOpenGLVertexArrayObjectPrivate : public QObjectPrivate
{
public:
    QOpenGLVertexArrayObjectPrivate()
        : vao(0)
        , vaoFuncsType(NotSupported)
        , context(0)
    {
    }

    ~QOpenGLVertexArrayObjectPrivate()
    {
        if (vaoFuncsType == ARB || vaoFuncsType == APPLE || vaoFuncsType == OES)
            delete vaoFuncs.helper;
    }

    bool create();
    void destroy();
    void bind();
    void release();
    void _q_contextAboutToBeDestroyed();

    Q_DECLARE_PUBLIC(QOpenGLVertexArrayObject)

    GLuint vao;

    union {
        QOpenGLFunctions_3_0 *core_3_0;
        QOpenGLFunctions_3_2_Core *core_3_2;
        QVertexArrayObjectHelper *helper;
    } vaoFuncs;
    enum {
        NotSupported,
        Core_3_0,
        Core_3_2,
        ARB,
        APPLE,
        OES
    } vaoFuncsType;

    QOpenGLContext *context;
};

bool QOpenGLVertexArrayObjectPrivate::create()
{
    if (vao) {
        qWarning("QOpenGLVertexArrayObject::create() VAO is already created");
        return false;
    }

    Q_Q(QOpenGLVertexArrayObject);

    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if (!ctx) {
        qWarning("QOpenGLVertexArrayObject::create() requires a valid current OpenGL context");
        return false;
    }
    context = ctx;
    QObject::connect(context, SIGNAL(aboutToBeDestroyed()), q, SLOT(_q_contextAboutToBeDestroyed()));

    if (ctx->isOpenGLES()) {
        if (ctx->hasExtension(QByteArrayLiteral("GL_OES_vertex_array_object"))) {
            vaoFuncs.helper = new QVertexArrayObjectHelper(ctx);
            vaoFuncsType = OES;
            vaoFuncs.helper->glGenVertexArrays(1, &vao);
        }
    } else {
        vaoFuncs.core_3_0 = 0;
        vaoFuncsType = NotSupported;
        QSurfaceFormat format = ctx->format();
#ifndef QT_OPENGL_ES_2
        if (format.version() >= qMakePair<int, int>(3,2)) {
            vaoFuncs.core_3_2 = ctx->versionFunctions<QOpenGLFunctions_3_2_Core>();
            vaoFuncsType = Core_3_2;
            vaoFuncs.core_3_2->initializeOpenGLFunctions();
            vaoFuncs.core_3_2->glGenVertexArrays(1, &vao);
        } else if (format.majorVersion() >= 3) {
            vaoFuncs.core_3_0 = ctx->versionFunctions<QOpenGLFunctions_3_0>();
            vaoFuncsType = Core_3_0;
            vaoFuncs.core_3_0->initializeOpenGLFunctions();
            vaoFuncs.core_3_0->glGenVertexArrays(1, &vao);
        } else
#endif
        if (ctx->hasExtension(QByteArrayLiteral("GL_ARB_vertex_array_object"))) {
            vaoFuncs.helper = new QVertexArrayObjectHelper(ctx);
            vaoFuncsType = ARB;
            vaoFuncs.helper->glGenVertexArrays(1, &vao);
        } else if (ctx->hasExtension(QByteArrayLiteral("GL_APPLE_vertex_array_object"))) {
            vaoFuncs.helper = new QVertexArrayObjectHelper(ctx);
            vaoFuncsType = APPLE;
            vaoFuncs.helper->glGenVertexArrays(1, &vao);
        }
    }

    return (vao != 0);
}

void QOpenGLVertexArrayObjectPrivate::destroy()
{
    if (!vao)
        return;

    Q_Q(QOpenGLVertexArrayObject);

    switch (vaoFuncsType) {
#ifndef QT_OPENGL_ES_2
    case Core_3_2:
        vaoFuncs.core_3_2->glDeleteVertexArrays(1, &vao);
        break;
    case Core_3_0:
        vaoFuncs.core_3_0->glDeleteVertexArrays(1, &vao);
        break;
#endif
    case ARB:
    case APPLE:
    case OES:
        vaoFuncs.helper->glDeleteVertexArrays(1, &vao);
        break;
    default:
        break;
    }

    Q_ASSERT(context);
    QObject::disconnect(context, SIGNAL(aboutToBeDestroyed()), q, SLOT(_q_contextAboutToBeDestroyed()));
    context = 0;

    vao = 0;
}

/*!
    \internal
*/
void QOpenGLVertexArrayObjectPrivate::_q_contextAboutToBeDestroyed()
{
    destroy();
}

void QOpenGLVertexArrayObjectPrivate::bind()
{
    switch (vaoFuncsType) {
#ifndef QT_OPENGL_ES_2
    case Core_3_2:
        vaoFuncs.core_3_2->glBindVertexArray(vao);
        break;
    case Core_3_0:
        vaoFuncs.core_3_0->glBindVertexArray(vao);
        break;
#endif
    case ARB:
    case APPLE:
    case OES:
        vaoFuncs.helper->glBindVertexArray(vao);
        break;
    default:
        break;
    }
}

void QOpenGLVertexArrayObjectPrivate::release()
{
    switch (vaoFuncsType) {
#ifndef QT_OPENGL_ES_2
    case Core_3_2:
        vaoFuncs.core_3_2->glBindVertexArray(0);
        break;
    case Core_3_0:
        vaoFuncs.core_3_0->glBindVertexArray(0);
        break;
#endif
    case ARB:
    case APPLE:
    case OES:
        vaoFuncs.helper->glBindVertexArray(0);
        break;
    default:
        break;
    }
}


/*!
    \class QOpenGLVertexArrayObject
    \brief The QOpenGLVertexArrayObject class wraps an OpenGL Vertex Array Object.
    \inmodule QtGui
    \since 5.1
    \ingroup painting-3D

    A Vertex Array Object (VAO) is an OpenGL container object that encapsulates
    the state needed to specify per-vertex attribute data to the OpenGL pipeline.
    To put it another way, a VAO remembers the states of buffer objects (see
    QOpenGLBuffer) and their associated state (e.g. vertex attribute divisors).
    This allows a very easy and efficient method of switching between OpenGL buffer
    states for rendering different "objects" in a scene. The QOpenGLVertexArrayObject
    class is a thin wrapper around an OpenGL VAO.

    For the desktop, VAOs are supported as a core feature in OpenGL 3.0 or newer and by the
    GL_ARB_vertex_array_object for older versions. On OpenGL ES 2, VAOs are provided by
    the optional GL_OES_vertex_array_object extension. You can check the version of
    OpenGL with QOpenGLContext::surfaceFormat() and check for the presence of extensions
    with QOpenGLContext::hasExtension().

    As with the other Qt OpenGL classes, QOpenGLVertexArrayObject has a create()
    function to create the underlying OpenGL object. This is to allow the developer to
    ensure that there is a valid current OpenGL context at the time.

    Once you have successfully created a VAO the typical usage pattern is:

    \list
        \li In scene initialization function, for each visual object:
        \list
            \li Bind the VAO
            \li Set vertex data state for this visual object (vertices, normals, texture coordinates etc.)
            \li Unbind (release()) the VAO
        \endlist
        \li In render function, for each visual object:
        \list
            \li Bind the VAO (and shader program if needed)
            \li Call a glDraw*() function
            \li Unbind (release()) the VAO
        \endlist
    \endlist

    The act of binding the VAO in the render function has the effect of restoring
    all of the vertex data state setup in the initialization phase. In this way we can
    set a great deal of state when setting up a VAO and efficiently switch between
    state sets of objects to be rendered. Using VAOs also allows the OpenGL driver
    to amortise the validation checks of the vertex data.

    \note Vertex Array Objects, like all other OpenGL container objects, are specific
    to the context for which they were created and cannot be shared amongst a
    context group.

    \sa QOpenGLVertexArrayObject::Binder, QOpenGLBuffer
*/

/*!
    Creates a QOpenGLVertexArrayObject with the given \a parent. You must call create()
    with a valid OpenGL context before using.
*/
QOpenGLVertexArrayObject::QOpenGLVertexArrayObject(QObject* parent)
    : QObject(*new QOpenGLVertexArrayObjectPrivate, parent)
{
}

/*!
    \internal
*/
QOpenGLVertexArrayObject::QOpenGLVertexArrayObject(QOpenGLVertexArrayObjectPrivate &dd)
    : QObject(dd)
{
}

/*!
    Destroys the QOpenGLVertexArrayObject and the underlying OpenGL resource.
*/
QOpenGLVertexArrayObject::~QOpenGLVertexArrayObject()
{
    QOpenGLContext* ctx = QOpenGLContext::currentContext();

    Q_D(QOpenGLVertexArrayObject);
    QOpenGLContext *oldContext = 0;
    QScopedPointer<QOffscreenSurface> offscreenSurface;
    if (d->context && ctx && d->context != ctx) {
        oldContext = ctx;
        // Cannot just make the current surface current again with another context.
        // The format may be incompatible and some platforms (iOS) may impose
        // restrictions on using a window with different contexts. Create an
        // offscreen surface (a pbuffer or a hidden window) instead to be safe.
        offscreenSurface.reset(new QOffscreenSurface);
        offscreenSurface->setFormat(d->context->format());
        offscreenSurface->create();
        if (d->context->makeCurrent(offscreenSurface.data())) {
            ctx = d->context;
        } else {
            qWarning("QOpenGLVertexArrayObject::~QOpenGLVertexArrayObject() failed to make VAO's context current");
            ctx = 0;
        }
    }

    if (ctx)
        destroy();

    if (oldContext) {
        if (!oldContext->makeCurrent(oldContext->surface()))
            qWarning("QOpenGLVertexArrayObject::~QOpenGLVertexArrayObject() failed to restore current context");
    }
}

/*!
    Creates the underlying OpenGL vertex array object. There must be a valid OpenGL context
    that supports vertex array objects current for this function to succeed.

    Returns \c true if the OpenGL vertex array object was successfully created.
*/
bool QOpenGLVertexArrayObject::create()
{
    Q_D(QOpenGLVertexArrayObject);
    return d->create();
}

/*!
    Destroys the underlying OpenGL vertex array object. There must be a valid OpenGL context
    that supports vertex array objects current for this function to succeed.
*/
void QOpenGLVertexArrayObject::destroy()
{
    Q_D(QOpenGLVertexArrayObject);
    d->destroy();
}

/*!
    Returns \c true is the underlying OpenGL vertex array object has been created. If this
    returns \c true and the associated OpenGL context is current, then you are able to bind()
    this object.
*/
bool QOpenGLVertexArrayObject::isCreated() const
{
    Q_D(const QOpenGLVertexArrayObject);
    return (d->vao != 0);
}

/*!
    Returns the id of the underlying OpenGL vertex array object.
*/
GLuint QOpenGLVertexArrayObject::objectId() const
{
    Q_D(const QOpenGLVertexArrayObject);
    return d->vao;
}

/*!
    Binds this vertex array object to the OpenGL binding point. From this point on
    and until release() is called or another vertex array object is bound, any
    modifications made to vertex data state are stored inside this vertex array object.

    If another vertex array object is then bound you can later restore the set of
    state associated with this object by calling bind() on this object once again.
    This allows efficient changes between vertex data states in rendering functions.
*/
void QOpenGLVertexArrayObject::bind()
{
    Q_D(QOpenGLVertexArrayObject);
    d->bind();
}

/*!
    Unbinds this vertex array object by binding the default vertex array object (id = 0).
*/
void QOpenGLVertexArrayObject::release()
{
    Q_D(QOpenGLVertexArrayObject);
    d->release();
}


/*!
    \class QOpenGLVertexArrayObject::Binder
    \brief The QOpenGLVertexArrayObject::Binder class is a convenience class to help
    with the binding and releasing of OpenGL Vertex Array Objects.
    \inmodule QtGui
    \reentrant
    \since 5.1
    \ingroup painting-3D

    QOpenGLVertexArrayObject::Binder is a simple convenience class that can be used
    to assist with the binding and releasing of QOpenGLVertexArrayObject instances.
    This class is to QOpenGLVertexArrayObject as QMutexLocker is to QMutex.

    This class implements the RAII principle which helps to ensure behavior in
    complex code or in the presence of exceptions.

    The constructor of this class accepts a QOpenGLVertexArrayObject (VAO) as an
    argument and attempts to bind the VAO, calling QOpenGLVertexArrayObject::create()
    if necessary. The destructor of this class calls QOpenGLVertexArrayObject::release()
    which unbinds the VAO.

    If needed the VAO can be temporarily unbound with the release() function and bound
    once more with rebind().

    \sa QOpenGLVertexArrayObject
*/

/*!
    \fn QOpenGLVertexArrayObject::Binder::Binder(QOpenGLVertexArrayObject *v)

    Creates a QOpenGLVertexArrayObject::Binder object and binds \a v by calling
    QOpenGLVertexArrayObject::bind(). If necessary it first calls
    QOpenGLVertexArrayObject::create().
*/

/*!
    \fn QOpenGLVertexArrayObject::Binder::~Binder()

    Destroys the QOpenGLVertexArrayObject::Binder and releases the associated vertex array object.
*/

/*!
    \fn QOpenGLVertexArrayObject::Binder::release()

    Can be used to temporarily release the associated vertex array object.

    \sa rebind()
*/

/*!
    \fn QOpenGLVertexArrayObject::Binder::rebind()

    Can be used to rebind the associated vertex array object.

    \sa release()
*/

QT_END_NAMESPACE

#include "moc_qopenglvertexarrayobject.cpp"
