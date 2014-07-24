/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qopenglwidget_p.h"
#include <QOpenGLContext>
#include <QtWidgets/private/qwidget_p.h>

#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QWindow>
#include <qpa/qplatformwindow.h>
#include <QDebug>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>

QT_BEGIN_NAMESPACE

class QOpenGLWidgetPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QOpenGLWidget)
public:
    QOpenGLWidgetPrivate()
        : fbo(0), uninitialized(true)
    {
    }
    GLuint textureId() const { return fbo ? fbo->texture() : 0; }

    const QSurface *surface() const { return q_func()->window()->windowHandle(); }
    QSurface *surface() { return q_func()->window()->windowHandle(); }
    void initialize();

    QOpenGLContext context;
    QOpenGLFramebufferObject *fbo;
    bool uninitialized;

    int w,h;
};

void QOpenGLWidgetPrivate::initialize()
{
    Q_Q(QOpenGLWidget);
    if (!uninitialized)
        return;
    context.setShareContext(get(q->window())->shareContext());
    context.setFormat(surface()->format());
    context.create();
    context.makeCurrent(surface());
    q->initializeGL();
    uninitialized = false;
}

QOpenGLWidget::QOpenGLWidget(QWidget *parent, Qt::WindowFlags f)
    : QWidget(*(new QOpenGLWidgetPrivate), parent, f)
{
    Q_D(QOpenGLWidget);
    d->setRenderToTexture();
}

QOpenGLWidget::~QOpenGLWidget()
{
}

bool QOpenGLWidget::isValid() const
{
    Q_D(const QOpenGLWidget);
    return d->context.isValid();
}

void QOpenGLWidget::makeCurrent()
{
    Q_D(QOpenGLWidget);
    d->context.makeCurrent(d->surface());
    d->fbo->bind();
}

void QOpenGLWidget::doneCurrent()
{
    Q_D(QOpenGLWidget);
    d->context.doneCurrent();
}

QSurfaceFormat QOpenGLWidget::format() const
{
    Q_D(const QOpenGLWidget);
    return d->surface()->format();
}

GLuint QOpenGLWidget::defaultFramebufferObject() const
{
    Q_D(const QOpenGLWidget);
    return d->fbo ? d->fbo->handle() : 0;
}

void QOpenGLWidget::initializeGL()
{

}

void QOpenGLWidget::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

void QOpenGLWidget::paintGL()
{
}

void QOpenGLWidget::updateGL()
{
    Q_D(QOpenGLWidget);
    if (d->uninitialized)
        return;

    makeCurrent();
    paintGL();
    d->context.functions()->glFlush();
    doneCurrent();
    update();
}


void QOpenGLWidget::resizeEvent(QResizeEvent *)
{
    Q_D(QOpenGLWidget);
    d->w = width();
    d->h = height();
    d->initialize();

    d->context.makeCurrent(d->surface());
    delete d->fbo; // recreate when resized
    d->fbo = new QOpenGLFramebufferObject(size() * devicePixelRatio(), QOpenGLFramebufferObject::CombinedDepthStencil);
    d->fbo->bind();
    QOpenGLFunctions *funcs = d->context.functions();
    funcs->glBindTexture(GL_TEXTURE_2D, d->fbo->texture());
    funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    funcs->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    resizeGL(width(), height());
    paintGL();
    funcs->glFlush();
}

void QOpenGLWidget::paintEvent(QPaintEvent *)
{
    qWarning("QOpenGLWidget does not support paintEvent() yet.");
    return;
}

QT_END_NAMESPACE
