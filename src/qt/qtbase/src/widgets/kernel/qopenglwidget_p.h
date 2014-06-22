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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It may change from version to version
// without notice, or even be removed.
//
// We mean it.
//
#ifndef QOPENGLWIDGET_H
#define QOPENGLWIDGET_H

#include <QWidget>
#include <QSurfaceFormat>

#include <QtGui/qopengl.h>

QT_BEGIN_NAMESPACE

class QOpenGLWidgetPrivate;

class Q_WIDGETS_EXPORT QOpenGLWidget : public QWidget
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QOpenGLWidget)

public:
    explicit QOpenGLWidget(QWidget* parent=0,
                       Qt::WindowFlags f=0);

// This API is not finalized yet. The commented-out functions below are
// QGLWidget functions that have not been implemented for QOpenGLWidget.
// Some of them may not end up in the final version (which is planned for a
// future release of Qt).

//    explicit QOpenGLWidget(const QSurfaceFormat& format, QWidget* parent=0,
//                       Qt::WindowFlags f=0);
    ~QOpenGLWidget();

//    void qglClearColor(const QColor& c) const;

    bool isValid() const;
//    bool isSharing() const;

    void makeCurrent();
    void doneCurrent();

//    void swapBuffers();

    QSurfaceFormat format() const;
    GLuint defaultFramebufferObject() const;

//    QPixmap renderPixmap(int w = 0, int h = 0, bool useContext = false);
    QImage grabFrameBuffer(bool withAlpha = false);

//    static QImage convertToGLFormat(const QImage& img);

//    QPaintEngine *paintEngine() const;

//    void drawTexture(const QRectF &target, GLuint textureId, GLenum textureTarget = GL_TEXTURE_2D);
//    void drawTexture(const QPointF &point, GLuint textureId, GLenum textureTarget = GL_TEXTURE_2D);

public Q_SLOTS:
    void updateGL();

protected:
//    bool event(QEvent *);
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();

//    void setAutoBufferSwap(bool on);
//    bool autoBufferSwap() const;

    void paintEvent(QPaintEvent*);
    void resizeEvent(QResizeEvent*);

//    virtual void glInit();
//    virtual void glDraw();

//    QOpenGLWidget(QOpenGLWidgetPrivate &dd,
//              const QGLFormat &format = QGLFormat(),
//              QWidget *parent = 0,
//              const QOpenGLWidget* shareWidget = 0,
//              Qt::WindowFlags f = 0);
private:
    Q_DISABLE_COPY(QOpenGLWidget)


};

QT_END_NAMESPACE

#endif // QOPENGLWIDGET_H
