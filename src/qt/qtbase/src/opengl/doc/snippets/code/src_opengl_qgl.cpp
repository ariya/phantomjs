/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

//! [0]
QGLFormat fmt;
fmt.setAlpha(true);
fmt.setStereo(true);
QGLFormat::setDefaultFormat(fmt);
//! [0]


//! [1]
QGLFormat fmt;
fmt.setDoubleBuffer(false);                 // single buffer
fmt.setDirectRendering(false);              // software rendering
MyGLWidget* myWidget = new MyGLWidget(fmt, ...);
//! [1]


//! [2]
QGLFormat fmt;
fmt.setOverlay(true);
fmt.setStereo(true);
MyGLWidget* myWidget = new MyGLWidget(fmt, ...);
if (!myWidget->format().stereo()) {
    // ok, goggles off
    if (!myWidget->format().hasOverlay()) {
        qFatal("Cool hardware required");
    }
}
//! [2]


//! [3]
// The rendering in MyGLWidget depends on using
// stencil buffer and alpha channel
MyGLWidget::MyGLWidget(QWidget* parent)
    : QGLWidget(QGLFormat(QGL::StencilBuffer | QGL::AlphaChannel), parent)
{
    if (!format().stencil())
        qWarning("Could not get stencil buffer; results will be suboptimal");
    if (!format().alpha())
        qWarning("Could not get alpha channel; results will be suboptimal");
    ...
}
//! [3]


//! [4]
QApplication a(argc, argv);
QGLFormat f;
f.setDoubleBuffer(false);
QGLFormat::setDefaultFormat(f);
//! [4]


//! [5]
QGLFormat f = QGLFormat::defaultOverlayFormat();
f.setDoubleBuffer(true);
QGLFormat::setDefaultOverlayFormat(f);
//! [5]


//! [6]
// ...continued from above
MyGLWidget* myWidget = new MyGLWidget(QGLFormat(QGL::HasOverlay), ...);
if (myWidget->format().hasOverlay()) {
    // Yes, we got an overlay, let's check _its_ format:
    QGLContext* olContext = myWidget->overlayContext();
    if (olContext->format().doubleBuffer())
        ; // yes, we got a double buffered overlay
    else
        ; // no, only single buffered overlays are available
}
//! [6]


//! [7]
QGLContext *cx;
//  ...
QGLFormat f;
f.setStereo(true);
cx->setFormat(f);
if (!cx->create())
    exit(); // no OpenGL support, or cannot render on the specified paintdevice
if (!cx->format().stereo())
    exit(); // could not create stereo context
//! [7]


//! [8]
class MyGLDrawer : public QGLWidget
{
    Q_OBJECT        // must include this if you use Qt signals/slots

public:
    MyGLDrawer(QWidget *parent)
        : QGLWidget(parent) {}

protected:

    void initializeGL()
    {
        // Set up the rendering context, define display lists etc.:
        ...
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glEnable(GL_DEPTH_TEST);
        ...
    }

    void resizeGL(int w, int h)
    {
        // setup viewport, projection etc.:
        glViewport(0, 0, (GLint)w, (GLint)h);
        ...
        glFrustum(...);
        ...
    }

    void paintGL()
    {
        // draw the scene:
        ...
        glRotatef(...);
        glMaterialfv(...);
        glBegin(GL_QUADS);
        glVertex3f(...);
        glVertex3f(...);
        ...
        glEnd();
        ...
    }

};
//! [8]
