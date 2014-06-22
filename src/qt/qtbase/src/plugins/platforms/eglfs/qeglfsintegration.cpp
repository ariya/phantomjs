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

#include "qeglfsintegration.h"

#include "qeglfswindow.h"
#include "qeglfshooks.h"
#include "qeglfscontext.h"

#include <QtGui/private/qguiapplication_p.h>

#include <QtPlatformSupport/private/qeglconvenience_p.h>
#include <QtPlatformSupport/private/qeglplatformcontext_p.h>
#include <QtPlatformSupport/private/qeglpbuffer_p.h>

#include <qpa/qplatformwindow.h>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QOpenGLContext>
#include <QtGui/QScreen>
#include <QtGui/QOffscreenSurface>
#include <qpa/qplatformcursor.h>

#include <EGL/egl.h>

static void initResources()
{
    Q_INIT_RESOURCE(cursor);
}

QT_BEGIN_NAMESPACE

QEglFSIntegration::QEglFSIntegration()
{
    mDisableInputHandlers = qgetenv("QT_QPA_EGLFS_DISABLE_INPUT").toInt();

    initResources();
}

QEglFSIntegration::~QEglFSIntegration()
{
    QEglFSHooks::hooks()->platformDestroy();
}

bool QEglFSIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    // We assume that devices will have more and not less capabilities
    if (QEglFSHooks::hooks() && QEglFSHooks::hooks()->hasCapability(cap))
        return true;

    return QEGLPlatformIntegration::hasCapability(cap);
}

void QEglFSIntegration::initialize()
{
    QEglFSHooks::hooks()->platformInit();

    QEGLPlatformIntegration::initialize();

    if (!mDisableInputHandlers)
        createInputHandlers();
}

EGLNativeDisplayType QEglFSIntegration::nativeDisplay() const
{
    return QEglFSHooks::hooks()->platformDisplay();
}

QEGLPlatformScreen *QEglFSIntegration::createScreen() const
{
    return new QEglFSScreen(display());
}

QEGLPlatformWindow *QEglFSIntegration::createWindow(QWindow *window) const
{
    return new QEglFSWindow(window);
}

QEGLPlatformContext *QEglFSIntegration::createContext(const QSurfaceFormat &format,
                                                      QPlatformOpenGLContext *shareContext,
                                                      EGLDisplay display) const
{
    return new QEglFSContext(QEglFSHooks::hooks()->surfaceFormatFor(format), shareContext, display);
}

QPlatformOffscreenSurface *QEglFSIntegration::createOffscreenSurface(EGLDisplay display,
                                                                     const QSurfaceFormat &format,
                                                                     QOffscreenSurface *surface) const
{
    return new QEGLPbuffer(display, QEglFSHooks::hooks()->surfaceFormatFor(format), surface);
}

QVariant QEglFSIntegration::styleHint(QPlatformIntegration::StyleHint hint) const
{
    switch (hint)
    {
    case QPlatformIntegration::ShowIsFullScreen:
        return screen()->compositingWindow() == 0;
    default:
        return QPlatformIntegration::styleHint(hint);
    }
}

EGLConfig QEglFSIntegration::chooseConfig(EGLDisplay display, const QSurfaceFormat &format)
{
    class Chooser : public QEglConfigChooser {
    public:
        Chooser(EGLDisplay display, QEglFSHooks *hooks)
            : QEglConfigChooser(display)
            , m_hooks(hooks)
        {
        }

    protected:
        bool filterConfig(EGLConfig config) const
        {
            return m_hooks->filterConfig(display(), config) && QEglConfigChooser::filterConfig(config);
        }

    private:
        QEglFSHooks *m_hooks;
    };

    Chooser chooser(display, QEglFSHooks::hooks());
    chooser.setSurfaceFormat(format);
    return chooser.chooseConfig();
}

QT_END_NAMESPACE
