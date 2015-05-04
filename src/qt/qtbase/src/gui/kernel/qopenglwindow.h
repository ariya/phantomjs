/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QOPENGLWINDOW_H
#define QOPENGLWINDOW_H

#include <QtCore/qglobal.h>

#ifndef QT_NO_OPENGL

#include <QtGui/QPaintDeviceWindow>
#include <QtGui/QOpenGLContext>
#include <QtGui/QImage>

QT_BEGIN_NAMESPACE

class QOpenGLWindowPrivate;

class Q_GUI_EXPORT QOpenGLWindow : public QPaintDeviceWindow
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QOpenGLWindow)

public:
    enum UpdateBehavior {
        NoPartialUpdate,
        PartialUpdateBlit,
        PartialUpdateBlend
    };

    explicit QOpenGLWindow(UpdateBehavior updateBehavior = NoPartialUpdate, QWindow *parent = 0);

    UpdateBehavior updateBehavior() const;
    bool isValid() const;

    void makeCurrent();
    void doneCurrent();

    QOpenGLContext *context() const;

    GLuint defaultFramebufferObject() const;

    QImage grabFramebuffer();

Q_SIGNALS:
    void frameSwapped();

protected:
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();
    virtual void paintUnderGL();
    virtual void paintOverGL();

    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    int metric(PaintDeviceMetric metric) const Q_DECL_OVERRIDE;
    QPaintDevice *redirected(QPoint *) const Q_DECL_OVERRIDE;

private:
    Q_DISABLE_COPY(QOpenGLWindow)
};

QT_END_NAMESPACE

#endif // QT_NO_OPENGL

#endif
