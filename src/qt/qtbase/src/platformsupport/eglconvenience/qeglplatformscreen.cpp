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

#include "qeglcompositor_p.h"
#include "qeglplatformscreen_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QEGLPlatformScreen
    \brief Base class for EGL-based platform screen implementations.
    \since 5.2
    \internal
    \ingroup qpa

    This class provides a lightweight base for QPlatformScreen
    implementations. It covers basic window stack management which is
    necessary when compositing multiple raster (widget-based) windows
    together into one single native surface.

    Reimplementing the virtuals are essential when using
    QEGLPlatformBackingStore. The context and the window returned from
    these are the ones that are used when compositing the textures
    generated from the raster (widget) based windows.

    \note It is up to the QEGLPlatformWindow subclasses to use the
    functions, like addWindow(), removeWindow(), etc., provided here.
 */

QEGLPlatformScreen::QEGLPlatformScreen(EGLDisplay dpy)
    : m_dpy(dpy)
{
}

QEGLPlatformScreen::~QEGLPlatformScreen()
{
    QEGLCompositor::destroy();
}

void QEGLPlatformScreen::addWindow(QEGLPlatformWindow *window)
{
    if (!m_windows.contains(window)) {
        m_windows.append(window);
        topWindowChanged(window);
    }
}

void QEGLPlatformScreen::removeWindow(QEGLPlatformWindow *window)
{
    m_windows.removeOne(window);
    if (!m_windows.isEmpty())
        topWindowChanged(m_windows.last());
}

void QEGLPlatformScreen::moveToTop(QEGLPlatformWindow *window)
{
    m_windows.removeOne(window);
    m_windows.append(window);
    topWindowChanged(window);
}

void QEGLPlatformScreen::changeWindowIndex(QEGLPlatformWindow *window, int newIdx)
{
    int idx = m_windows.indexOf(window);
    if (idx != -1 && idx != newIdx) {
        m_windows.move(idx, newIdx);
        if (newIdx == m_windows.size() - 1)
            topWindowChanged(m_windows.last());
    }
}

QT_END_NAMESPACE
