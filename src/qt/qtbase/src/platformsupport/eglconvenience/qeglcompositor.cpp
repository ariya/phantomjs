/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLFramebufferObject>
#include <QtGui/private/qopengltextureblitter_p.h>
#include <qpa/qplatformbackingstore.h>

#include "qeglcompositor_p.h"
#include "qeglplatformwindow_p.h"
#include "qeglplatformscreen_p.h"

QT_BEGIN_NAMESPACE

static QEGLCompositor *compositor = 0;

QEGLCompositor::QEGLCompositor()
    : m_context(0),
      m_window(0),
      m_blitter(0)
{
    Q_ASSERT(!compositor);
    m_updateTimer.setSingleShot(true);
    m_updateTimer.setInterval(0);
    connect(&m_updateTimer, SIGNAL(timeout()), SLOT(renderAll()));
}

QEGLCompositor::~QEGLCompositor()
{
    Q_ASSERT(compositor == this);
    if (m_blitter) {
        m_blitter->destroy();
        delete m_blitter;
    }
    compositor = 0;
}

void QEGLCompositor::schedule(QOpenGLContext *context, QEGLPlatformWindow *window)
{
    m_context = context;
    m_window = window;
    if (!m_updateTimer.isActive())
        m_updateTimer.start();
}

void QEGLCompositor::renderAll()
{
    Q_ASSERT(m_context && m_window);
    m_context->makeCurrent(m_window->window());

    if (!m_blitter) {
        m_blitter = new QOpenGLTextureBlitter;
        m_blitter->create();
    }
    m_blitter->bind();

    QEGLPlatformScreen *screen = static_cast<QEGLPlatformScreen *>(m_window->screen());
    QList<QEGLPlatformWindow *> windows = screen->windows();
    for (int i = 0; i < windows.size(); ++i)
        render(windows.at(i));

    m_blitter->release();
    m_context->swapBuffers(m_window->window());

    for (int i = 0; i < windows.size(); ++i)
        windows.at(i)->composited();
}

void QEGLCompositor::render(QEGLPlatformWindow *window)
{
    const QPlatformTextureList *textures = window->textures();
    if (!textures)
        return;

    const QRect targetWindowRect(QPoint(0, 0), window->screen()->geometry().size());
    glViewport(0, 0, targetWindowRect.width(), targetWindowRect.height());

    for (int i = 0; i < textures->count(); ++i) {
        uint textureId = textures->textureId(i);
        glBindTexture(GL_TEXTURE_2D, textureId);
        QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(textures->geometry(i),
                                                                   targetWindowRect);

        if (textures->count() > 1 && i == textures->count() - 1) {
            // Backingstore for a widget with QOpenGLWidget subwidgets
            m_blitter->setSwizzleRB(true);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            m_blitter->blit(textureId, target, QOpenGLTextureBlitter::OriginTopLeft);
            glDisable(GL_BLEND);
        } else if (textures->count() == 1) {
            // A regular QWidget window
            m_blitter->setSwizzleRB(true);
            m_blitter->blit(textureId, target, QOpenGLTextureBlitter::OriginTopLeft);
        } else {
            // Texture from an FBO belonging to a QOpenGLWidget
            m_blitter->setSwizzleRB(false);
            m_blitter->blit(textureId, target, QOpenGLTextureBlitter::OriginBottomLeft);
        }
    }
}

QEGLCompositor *QEGLCompositor::instance()
{
    if (!compositor)
        compositor = new QEGLCompositor;
    return compositor;
}

void QEGLCompositor::destroy()
{
    delete compositor;
    compositor = 0;
}

QT_END_NAMESPACE
