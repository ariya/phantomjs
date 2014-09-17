/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtOpenGL module of the Qt Toolkit.
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

#include "qplatformglcontext_qpa.h"

#include <QtCore/QThreadStorage>
#include <QtCore/QThread>

#include <QDebug>

class QPlatformGLThreadContext
{
public:
    ~QPlatformGLThreadContext() {
        if (context)
            context->doneCurrent();
    }
    QPlatformGLContext *context;
};

static QThreadStorage<QPlatformGLThreadContext *> qplatformgl_context_storage;

class QPlatformGLContextPrivate
{
public:
    QPlatformGLContextPrivate()
        :qGLContextHandle(0)
    {
    }

    virtual ~QPlatformGLContextPrivate()
    {
        //do not delete the QGLContext handle here as it is deleted in
        //QWidgetPrivate::deleteTLSysExtra()
    }
    void *qGLContextHandle;
    void (*qGLContextDeleteFunction)(void *handle);
    static QPlatformGLContext *staticSharedContext;

    static void setCurrentContext(QPlatformGLContext *context);
};

QPlatformGLContext *QPlatformGLContextPrivate::staticSharedContext = 0;

void QPlatformGLContextPrivate::setCurrentContext(QPlatformGLContext *context)
{
    QPlatformGLThreadContext *threadContext = qplatformgl_context_storage.localData();
    if (!threadContext) {
        if (!QThread::currentThread()) {
            qWarning("No QTLS available. currentContext wont work");
            return;
        }
        threadContext = new QPlatformGLThreadContext;
        qplatformgl_context_storage.setLocalData(threadContext);
    }
    threadContext->context = context;
}

/*!
  Returns the last context which called makeCurrent. This function is thread aware.
*/
const QPlatformGLContext* QPlatformGLContext::currentContext()
{
    QPlatformGLThreadContext *threadContext = qplatformgl_context_storage.localData();
    if(threadContext) {
        return threadContext->context;
    }
    return 0;
}

/*!
    All subclasses needs to specify the platformWindow. It can be a null window.
*/
QPlatformGLContext::QPlatformGLContext()
    :d_ptr(new QPlatformGLContextPrivate())
{
}

/*!
  If this is the current context for the thread, doneCurrent is called
*/
QPlatformGLContext::~QPlatformGLContext()
{
    if (QPlatformGLContext::currentContext() == this) {
        doneCurrent();
    }

}

/*!
    Reimplement in subclass to do makeCurrent on native GL context
*/
void QPlatformGLContext::makeCurrent()
{
    QPlatformGLContextPrivate::setCurrentContext(this);
}

/*!
    Reimplement in subclass to release current context.
    Typically this is calling makeCurrent with 0 "surface"
*/
void QPlatformGLContext::doneCurrent()
{
    QPlatformGLContextPrivate::setCurrentContext(0);
}

/*
  internal: Needs to have a pointer to qGLContext. But since this is in QtGui we cant
  have any type information.
*/
void *QPlatformGLContext::qGLContextHandle() const
{
    Q_D(const QPlatformGLContext);
    return d->qGLContextHandle;
}

void QPlatformGLContext::setQGLContextHandle(void *handle,void (*qGLContextDeleteFunction)(void *))
{
    Q_D(QPlatformGLContext);
    d->qGLContextHandle = handle;
    d->qGLContextDeleteFunction = qGLContextDeleteFunction;
}

void QPlatformGLContext::deleteQGLContext()
{
    Q_D(QPlatformGLContext);
    if (d->qGLContextDeleteFunction && d->qGLContextHandle) {
        d->qGLContextDeleteFunction(d->qGLContextHandle);
        d->qGLContextDeleteFunction = 0;
        d->qGLContextHandle = 0;
    }
}

/*!
    \class QPlatformGLContext
    \since 4.8
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformGLContext class provides an abstraction for native GL contexts.

    In QPA the way to support OpenGL or OpenVG or other technologies that requires a native GL
    context is through the QPlatformGLContext wrapper.

    There is no factory function for QPlatformGLContexts, but rather only one accessor function.
    The only place to retrieve a QPlatformGLContext from is through a QPlatformWindow.

    The context which is current for a specific thread can be collected by the currentContext()
    function. This is how QPlatformGLContext also makes it possible to use the QtOpenGL module
    withhout using QGLWidget. When using QGLContext::currentContext(), it will ask
    QPlatformGLContext for the currentContext. Then a corresponding QGLContext will be returned,
    which maps to the QPlatformGLContext.
*/

/*! \fn void QPlatformGLContext::swapBuffers()
    Reimplement in subclass to native swap buffers calls
*/

/*! \fn void *QPlatformGLContext::getProcAddress(const QString &procName)
    Reimplement in subclass to native getProcAddr calls.

    Note: its convenient to use qPrintable(const QString &str) to get the const char * pointer
*/

/*! \fn QPlatformWindowFormat QPlatformGLContext::platformWindowFormat() const
    QWidget has the function qplatformWindowFormat(). That function is for the application
    programmer to request the format of the window and the context that he wants.

    Reimplement this function in a subclass to indicate what format the glContext actually has.
*/
