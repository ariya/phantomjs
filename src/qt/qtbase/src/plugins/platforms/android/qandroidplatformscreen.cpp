/****************************************************************************
**
** Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
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

#include <QDebug>
#include <QTime>

#include <qpa/qwindowsysteminterface.h>

#include "qandroidplatformscreen.h"
#include "qandroidplatformbackingstore.h"
#include "qandroidplatformintegration.h"
#include "androidjnimain.h"
#include "androidjnimenu.h"
#include "qandroidplatformrasterwindow.h"

#include <android/bitmap.h>
#include <android/native_window_jni.h>

QT_BEGIN_NAMESPACE

#ifdef QANDROIDPLATFORMSCREEN_DEBUG
class ScopedProfiler
{
public:
    ScopedProfiler(const QString &msg)
    {
        m_msg = msg;
        m_timer.start();
    }
    ~ScopedProfiler()
    {
        qDebug() << m_msg << m_timer.elapsed();
    }

private:
    QTime m_timer;
    QString m_msg;
};

# define PROFILE_SCOPE ScopedProfiler ___sp___(__func__)
#else
# define PROFILE_SCOPE
#endif

QAndroidPlatformScreen::QAndroidPlatformScreen():QObject(),QPlatformScreen()
{
    m_geometry = QRect(0, 0, QAndroidPlatformIntegration::m_defaultGeometryWidth, QAndroidPlatformIntegration::m_defaultGeometryHeight);
    // Raster only apps should set QT_ANDROID_RASTER_IMAGE_DEPTH to 16
    // is way much faster than 32
    if (qgetenv("QT_ANDROID_RASTER_IMAGE_DEPTH").toInt() == 16) {
        m_format = QImage::Format_RGB16;
        m_depth = 16;
    } else {
        m_format = QImage::Format_ARGB32_Premultiplied;
        m_depth = 32;
    }
    m_physicalSize.setHeight(QAndroidPlatformIntegration::m_defaultPhysicalSizeHeight);
    m_physicalSize.setWidth(QAndroidPlatformIntegration::m_defaultPhysicalSizeWidth);
    m_redrawTimer.setSingleShot(true);
    m_redrawTimer.setInterval(0);
    connect(&m_redrawTimer, SIGNAL(timeout()), this, SLOT(doRedraw()));
}

QAndroidPlatformScreen::~QAndroidPlatformScreen()
{
    if (m_id != -1) {
        QtAndroid::destroySurface(m_id);
        m_surfaceWaitCondition.wakeOne();
        if (m_nativeSurface)
            ANativeWindow_release(m_nativeSurface);
    }
}

QWindow *QAndroidPlatformScreen::topWindow() const
{
    foreach (QAndroidPlatformWindow *w, m_windowStack)
        if (w->window()->type() == Qt::Window || w->window()->type() == Qt::Dialog)
            return w->window();
    return 0;
}

QWindow *QAndroidPlatformScreen::topLevelAt(const QPoint &p) const
{
    foreach (QAndroidPlatformWindow *w, m_windowStack) {
        if (w->geometry().contains(p, false) && w->window()->isVisible())
            return w->window();
    }
    return 0;
}

void QAndroidPlatformScreen::addWindow(QAndroidPlatformWindow *window)
{
    if (window->parent())
        return;

    m_windowStack.prepend(window);
    if (window->isRaster()) {
        m_rasterSurfaces.ref();
        setDirty(window->geometry());
    }

    QWindow *w = topWindow();
    QWindowSystemInterface::handleWindowActivated(w);
    topWindowChanged(w);
}

void QAndroidPlatformScreen::removeWindow(QAndroidPlatformWindow *window)
{
    if (window->parent())
        return;

    m_windowStack.removeOne(window);
    if (window->isRaster()) {
        m_rasterSurfaces.deref();
        setDirty(window->geometry());
    }

    QWindow *w = topWindow();
    QWindowSystemInterface::handleWindowActivated(w);
    topWindowChanged(w);
}

void QAndroidPlatformScreen::raise(QAndroidPlatformWindow *window)
{
    if (window->parent())
        return;

    int index = m_windowStack.indexOf(window);
    if (index <= 0)
        return;
    m_windowStack.move(index, 0);
    if (window->isRaster()) {
        setDirty(window->geometry());
    }
    QWindow *w = topWindow();
    QWindowSystemInterface::handleWindowActivated(w);
    topWindowChanged(w);
}

void QAndroidPlatformScreen::lower(QAndroidPlatformWindow *window)
{
    if (window->parent())
        return;

    int index = m_windowStack.indexOf(window);
    if (index == -1 || index == (m_windowStack.size() - 1))
        return;
    m_windowStack.move(index, m_windowStack.size() - 1);
    if (window->isRaster()) {
        setDirty(window->geometry());
    }
    QWindow *w = topWindow();
    QWindowSystemInterface::handleWindowActivated(w);
    topWindowChanged(w);
}

void QAndroidPlatformScreen::scheduleUpdate()
{
    if (!m_redrawTimer.isActive())
        m_redrawTimer.start();
}

void QAndroidPlatformScreen::setDirty(const QRect &rect)
{
    QRect intersection = rect.intersected(m_geometry);
    m_dirtyRect |= intersection;
    scheduleUpdate();
}

void QAndroidPlatformScreen::setPhysicalSize(const QSize &size)
{
    m_physicalSize = size;
}

void QAndroidPlatformScreen::setGeometry(const QRect &rect)
{
    QMutexLocker lock(&m_surfaceMutex);
    if (m_geometry == rect)
        return;

    m_geometry = rect;
    QWindowSystemInterface::handleScreenGeometryChange(QPlatformScreen::screen(), geometry());
    QWindowSystemInterface::handleScreenAvailableGeometryChange(QPlatformScreen::screen(), availableGeometry());
    resizeMaximizedWindows();

    if (m_id != -1) {
        if (m_nativeSurface) {
            ANativeWindow_release(m_nativeSurface);
            m_nativeSurface = 0;
        }
        QtAndroid::setSurfaceGeometry(m_id, rect);
    }
}

void QAndroidPlatformScreen::topWindowChanged(QWindow *w)
{
    QtAndroidMenu::setActiveTopLevelWindow(w);

    if (w != 0) {
        QAndroidPlatformWindow *platformWindow = static_cast<QAndroidPlatformWindow *>(w->handle());
        if (platformWindow != 0)
            platformWindow->updateStatusBarVisibility();
    }
}

int QAndroidPlatformScreen::rasterSurfaces()
{
    return m_rasterSurfaces;
}

void QAndroidPlatformScreen::doRedraw()
{
    PROFILE_SCOPE;

    if (m_dirtyRect.isEmpty())
        return;

    QMutexLocker lock(&m_surfaceMutex);
    if (m_id == -1 && m_rasterSurfaces) {
        m_id = QtAndroid::createSurface(this, m_geometry, true, m_depth);
        m_surfaceWaitCondition.wait(&m_surfaceMutex);
    }

    if (!m_nativeSurface)
        return;

    ANativeWindow_Buffer nativeWindowBuffer;
    ARect nativeWindowRect;
    nativeWindowRect.top = m_dirtyRect.top();
    nativeWindowRect.left = m_dirtyRect.left();
    nativeWindowRect.bottom = m_dirtyRect.bottom() + 1; // for some reason that I don't understand the QRect bottom needs to +1 to be the same with ARect bottom
    nativeWindowRect.right = m_dirtyRect.right() + 1; // same for the right

    int ret;
    if ((ret = ANativeWindow_lock(m_nativeSurface, &nativeWindowBuffer, &nativeWindowRect)) < 0) {
        qWarning() << "ANativeWindow_lock() failed! error=" << ret;
        return;
    }

    int bpp = 4;
    QImage::Format format = QImage::Format_RGBA8888_Premultiplied;
    if (nativeWindowBuffer.format == WINDOW_FORMAT_RGB_565) {
        bpp = 2;
        format = QImage::Format_RGB16;
    }

    QImage screenImage(reinterpret_cast<uchar *>(nativeWindowBuffer.bits)
                       , nativeWindowBuffer.width, nativeWindowBuffer.height
                       , nativeWindowBuffer.stride * bpp , format);

    QPainter compositePainter(&screenImage);
    compositePainter.setCompositionMode(QPainter::CompositionMode_Source);

    QRegion visibleRegion(m_dirtyRect);
    foreach (QAndroidPlatformWindow *window, m_windowStack) {
        if (!window->window()->isVisible()
                || !window->isRaster())
            continue;

        QVector<QRect> visibleRects = visibleRegion.rects();
        foreach (const QRect &rect, visibleRects) {
            QRect targetRect = window->geometry();
            targetRect &= rect;

            if (targetRect.isNull())
                continue;

            visibleRegion -= targetRect;
            QRect windowRect = targetRect.translated(-window->geometry().topLeft());
            QAndroidPlatformBackingStore *backingStore = static_cast<QAndroidPlatformRasterWindow *>(window)->backingStore();
            if (backingStore)
                compositePainter.drawImage(targetRect.topLeft(), backingStore->image(), windowRect);
        }
    }

    foreach (const QRect &rect, visibleRegion.rects()) {
        compositePainter.fillRect(rect, QColor(Qt::transparent));
    }

    ret = ANativeWindow_unlockAndPost(m_nativeSurface);
    if (ret >= 0)
        m_dirtyRect = QRect();
}

QDpi QAndroidPlatformScreen::logicalDpi() const
{
    qreal lDpi = QtAndroid::scaledDensity() * 72;
    return QDpi(lDpi, lDpi);
}

Qt::ScreenOrientation QAndroidPlatformScreen::orientation() const
{
    return QAndroidPlatformIntegration::m_orientation;
}

Qt::ScreenOrientation QAndroidPlatformScreen::nativeOrientation() const
{
    return QAndroidPlatformIntegration::m_nativeOrientation;
}

void QAndroidPlatformScreen::surfaceChanged(JNIEnv *env, jobject surface, int w, int h)
{
    lockSurface();
    if (surface && w && h) {
        if (m_nativeSurface)
            ANativeWindow_release(m_nativeSurface);
        m_nativeSurface = ANativeWindow_fromSurface(env, surface);
        QMetaObject::invokeMethod(this, "setDirty", Qt::QueuedConnection, Q_ARG(QRect, QRect(0, 0, w, h)));
    } else {
        if (m_nativeSurface) {
            ANativeWindow_release(m_nativeSurface);
            m_nativeSurface = 0;
        }
    }
    unlockSurface();
    m_surfaceWaitCondition.wakeOne();
}

QT_END_NAMESPACE
