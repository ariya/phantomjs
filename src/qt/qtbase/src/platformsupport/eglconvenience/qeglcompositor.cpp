/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

struct BlendStateBinder
{
    BlendStateBinder() : m_blend(false) {
        glDisable(GL_BLEND);
    }
    void set(bool blend) {
        if (blend != m_blend) {
            if (blend) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            } else {
                glDisable(GL_BLEND);
            }
            m_blend = blend;
        }
    }
    ~BlendStateBinder() {
        if (m_blend)
            glDisable(GL_BLEND);
    }
    bool m_blend;
};

void QEGLCompositor::render(QEGLPlatformWindow *window)
{
    const QPlatformTextureList *textures = window->textures();
    if (!textures)
        return;

    const QRect targetWindowRect(QPoint(0, 0), window->screen()->geometry().size());
    glViewport(0, 0, targetWindowRect.width(), targetWindowRect.height());

    float currentOpacity = 1.0f;
    BlendStateBinder blend;

    for (int i = 0; i < textures->count(); ++i) {
        uint textureId = textures->textureId(i);
        QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(textures->geometry(i),
                                                                   targetWindowRect);
        const float opacity = window->window()->opacity();
        if (opacity != currentOpacity) {
            currentOpacity = opacity;
            m_blitter->setOpacity(currentOpacity);
        }

        if (textures->count() > 1 && i == textures->count() - 1) {
            // Backingstore for a widget with QOpenGLWidget subwidgets
            blend.set(true);
            m_blitter->blit(textureId, target, QOpenGLTextureBlitter::OriginTopLeft);
        } else if (textures->count() == 1) {
            // A regular QWidget window
            const bool translucent = window->window()->requestedFormat().alphaBufferSize() > 0;
            blend.set(translucent);
            m_blitter->blit(textureId, target, QOpenGLTextureBlitter::OriginTopLeft);
        } else if (!textures->flags(i).testFlag(QPlatformTextureList::StacksOnTop)) {
            // Texture from an FBO belonging to a QOpenGLWidget
            blend.set(false);
            m_blitter->blit(textureId, target, QOpenGLTextureBlitter::OriginBottomLeft);
        }
    }

    for (int i = 0; i < textures->count(); ++i) {
        if (textures->flags(i).testFlag(QPlatformTextureList::StacksOnTop)) {
            QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(textures->geometry(i), targetWindowRect);
            blend.set(true);
            m_blitter->blit(textures->textureId(i), target, QOpenGLTextureBlitter::OriginBottomLeft);
        }
    }

    m_blitter->setOpacity(1.0f);
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
