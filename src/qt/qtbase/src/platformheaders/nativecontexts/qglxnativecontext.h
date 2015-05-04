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

#ifndef QGLXNATIVECONTEXT_H
#define QGLXNATIVECONTEXT_H

#include <QtCore/QMetaType>
#include <X11/Xlib.h>
#include <GL/glx.h>

QT_BEGIN_NAMESPACE

struct QGLXNativeContext
{
    QGLXNativeContext()
        : m_context(0),
          m_display(0),
          m_window(0),
          m_visualId(0)
    { }

    QGLXNativeContext(GLXContext ctx, Display *dpy = 0, Window wnd = 0, VisualID vid = 0)
        : m_context(ctx),
          m_display(dpy),
          m_window(wnd),
          m_visualId(vid)
    { }

    GLXContext context() const { return m_context; }
    Display *display() const { return m_display; }
    Window window() const { return m_window; }
    VisualID visualId() const { return m_visualId; }

private:
    GLXContext m_context;
    Display *m_display;
    Window m_window;
    VisualID m_visualId;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QGLXNativeContext)

#endif // QGLXNATIVECONTEXT_H
