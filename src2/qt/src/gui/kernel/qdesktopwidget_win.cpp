/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qdesktopwidget.h"
#include "qt_windows.h"
#include "qapplication_p.h"
#include <private/qsystemlibrary_p.h>
#include <qvector.h>
#include <limits.h>
#ifdef Q_WS_WINCE
#include <sipapi.h>
#endif
#include "qwidget_p.h"
#include "qdebug.h"

QT_BEGIN_NAMESPACE

class QDesktopWidgetPrivate : public QWidgetPrivate
{
public:
    QDesktopWidgetPrivate();
    ~QDesktopWidgetPrivate();

    static void init(QDesktopWidget *that);
    static void cleanup();
    static int screenCount;
    static int primaryScreen;

    static QVector<QRect> *rects;
    static QVector<QRect> *workrects;

    struct MONITORINFO
    {
        DWORD   cbSize;
        RECT    rcMonitor;
        RECT    rcWork;
        DWORD   dwFlags;
    };

    typedef BOOL (WINAPI *InfoFunc)(HMONITOR, MONITORINFO*);
    typedef BOOL (QT_WIN_CALLBACK *EnumProc)(HMONITOR, HDC, LPRECT, LPARAM);
    typedef BOOL (WINAPI *EnumFunc)(HDC, LPCRECT, EnumProc, LPARAM);

    static EnumFunc enumDisplayMonitors;
    static InfoFunc getMonitorInfo;
    static int refcount;
};

int QDesktopWidgetPrivate::screenCount = 1;
int QDesktopWidgetPrivate::primaryScreen = 0;
QDesktopWidgetPrivate::EnumFunc QDesktopWidgetPrivate::enumDisplayMonitors = 0;
QDesktopWidgetPrivate::InfoFunc QDesktopWidgetPrivate::getMonitorInfo = 0;
QVector<QRect> *QDesktopWidgetPrivate::rects = 0;
QVector<QRect> *QDesktopWidgetPrivate::workrects = 0;
static int screen_number = 0;
int QDesktopWidgetPrivate::refcount = 0;
#ifdef Q_WS_WINCE_WM
// Use SIP information, if available
// SipGetInfo is not supported by SSDK (no definition!). 
static inline void qt_get_sip_info(QRect &rect)
{
    SIPINFO sip;
    memset(&sip, 0, sizeof(SIPINFO));
    sip.cbSize = sizeof(SIPINFO);
    if (SipGetInfo(&sip))
        rect = QRect(QPoint(sip.rcVisibleDesktop.left, sip.rcVisibleDesktop.top),
        QPoint(sip.rcVisibleDesktop.right - 1, sip.rcVisibleDesktop.bottom - 1));
}
#endif


BOOL QT_WIN_CALLBACK enumCallback(HMONITOR hMonitor, HDC, LPRECT, LPARAM)
{
    QDesktopWidgetPrivate::screenCount++;
    QDesktopWidgetPrivate::rects->resize(QDesktopWidgetPrivate::screenCount);
    QDesktopWidgetPrivate::workrects->resize(QDesktopWidgetPrivate::screenCount);
    // Get the MONITORINFO block
    QDesktopWidgetPrivate::MONITORINFO info;
    memset(&info, 0, sizeof(QDesktopWidgetPrivate::MONITORINFO));
    info.cbSize = sizeof(QDesktopWidgetPrivate::MONITORINFO);
    BOOL res = QDesktopWidgetPrivate::getMonitorInfo(hMonitor, &info);
    if (!res) {
        (*QDesktopWidgetPrivate::rects)[screen_number] = QRect();
        (*QDesktopWidgetPrivate::workrects)[screen_number] = QRect();
        return true;
    }

    // Fill list of rects
    RECT r = info.rcMonitor;
    QRect qr(QPoint(r.left, r.top), QPoint(r.right - 1, r.bottom - 1));
    (*QDesktopWidgetPrivate::rects)[screen_number] = qr;

    r = info.rcWork;
    qr = QRect(QPoint(r.left, r.top), QPoint(r.right - 1, r.bottom - 1));
    (*QDesktopWidgetPrivate::workrects)[screen_number] = qr;

    if (info.dwFlags & 0x00000001) //MONITORINFOF_PRIMARY
        QDesktopWidgetPrivate::primaryScreen = screen_number;

    ++screen_number;
    // Stop the enumeration if we have them all
    return true;
}

QDesktopWidgetPrivate::QDesktopWidgetPrivate()
{
    ++refcount;
}

void QDesktopWidgetPrivate::init(QDesktopWidget *that)
{
    if (rects)
        return;

    rects = new QVector<QRect>();
    workrects = new QVector<QRect>();
    screenCount = 0;

#ifndef Q_OS_WINCE
    QSystemLibrary user32Lib(QLatin1String("user32"));
    enumDisplayMonitors = (EnumFunc)user32Lib.resolve("EnumDisplayMonitors");
    getMonitorInfo = (InfoFunc)user32Lib.resolve("GetMonitorInfoW");

    if (!enumDisplayMonitors || !getMonitorInfo) {
        screenCount = GetSystemMetrics(80);  // SM_CMONITORS
        rects->resize(screenCount);
        for (int i = 0; i < screenCount; ++i)
            rects->replace(i, that->rect());
        return;
    }
    // Calls enumCallback
    enumDisplayMonitors(0, 0, enumCallback, 0);
    enumDisplayMonitors = 0;
    getMonitorInfo = 0;
#else
    QSystemLibrary coreLib(QLatin1String("coredll"));
    // CE >= 4.0 case
    enumDisplayMonitors = (EnumFunc)coreLib.resolve("EnumDisplayMonitors");
    getMonitorInfo = (InfoFunc)coreLib.resolve("GetMonitorInfo");

    if ((!enumDisplayMonitors || !getMonitorInfo)) {
        screenCount = GetSystemMetrics(SM_CMONITORS);
        return;
    }

    if (!coreLib.isLoaded() || !enumDisplayMonitors || !getMonitorInfo) {
        rects->resize(screenCount);
        for (int i = 0; i < screenCount; ++i)
            (*rects)[i] = that->rect();

        RECT r;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &r, 0);
        QRect qr = QRect(QPoint(r.left, r.top), QPoint(r.right - 1, r.bottom - 1));

#if defined(Q_WS_WINCE_WM)
        qt_get_sip_info(qr);
#endif

        workrects->resize(screenCount);
        for (int j = 0; j < screenCount; ++j)
            (*workrects)[j] = qr;
        return;
    }

    // Calls enumCallback
    enumDisplayMonitors(0, 0, enumCallback, 0);
    enumDisplayMonitors = 0;
    getMonitorInfo = 0;
#endif // Q_WS_WINCE
}

QDesktopWidgetPrivate::~QDesktopWidgetPrivate()
{
    if (!--refcount)
        cleanup();
}

void QDesktopWidgetPrivate::cleanup()
{
    screen_number = 0;
    screenCount = 1;
    primaryScreen = 0;
    enumDisplayMonitors = 0;
    getMonitorInfo = 0;
    delete rects;
    rects = 0;
    delete workrects;
    workrects = 0;
}

/*
    \omit
    Function is commented out in header
    \fn void *QDesktopWidget::handle(int screen) const

    Returns the window system handle of the display device with the
    index \a screen, for low-level access.  Using this function is not
    portable.

    The return type varies with platform; see qwindowdefs.h for details.

    \sa x11Display(), QPaintDevice::handle()
    \endomit
*/

QDesktopWidget::QDesktopWidget()
    : QWidget(*new QDesktopWidgetPrivate, 0, Qt::Desktop)
{
    setObjectName(QLatin1String("desktop"));
    QDesktopWidgetPrivate::init(this);
}

QDesktopWidget::~QDesktopWidget()
{
}

bool QDesktopWidget::isVirtualDesktop() const
{
    return true;
}

int QDesktopWidget::primaryScreen() const
{
    return d_func()->primaryScreen;
}

int QDesktopWidget::numScreens() const
{
    return d_func()->screenCount;
}

QWidget *QDesktopWidget::screen(int /* screen */)
{
    // It seems that a Qt::WType_Desktop cannot be moved?
    return this;
}

//
// MSVC 7.10 warns that d (the result of the expanded Q_D macro) as a local variable that is not referenced.
// Therefore, we ignore that warning with the following pragmas
// I've also tried to eliminate the macro, but to no use...
// We pop it further down
#ifdef Q_CC_MSVC
# pragma warning(push)
# pragma warning(disable : 4189)
#endif
const QRect QDesktopWidget::availableGeometry(int screen) const
{
    Q_D(const QDesktopWidget);
#ifdef Q_WS_WINCE_WM
    for(int i=0; i < d->workrects->size(); ++i)
        qt_get_sip_info((*d->workrects)[i]);
#endif
    if (screen < 0 || screen >= d->screenCount)
        screen = d->primaryScreen;

    return d->workrects->at(screen);
}

const QRect QDesktopWidget::screenGeometry(int screen) const
{
    const QDesktopWidgetPrivate *d = d_func();
    if (screen < 0 || screen >= d->screenCount)
        screen = d->primaryScreen;

    return d->rects->at(screen);
}

int QDesktopWidget::screenNumber(const QWidget *widget) const
{
    Q_D(const QDesktopWidget);
    if (!widget)
        return d->primaryScreen;

    QRect frame = widget->frameGeometry();
    if (!widget->isWindow())
        frame.moveTopLeft(widget->mapToGlobal(QPoint(0,0)));

    int maxSize = -1;
    int maxScreen = -1;

    for (int i = 0; i < d->screenCount; ++i) {
        QRect sect = d->rects->at(i).intersected(frame);
        int size = sect.width() * sect.height();
        if (size > maxSize && sect.width() > 0 && sect.height() > 0) {
            maxSize = size;
            maxScreen = i;
        }
    }

    return maxScreen;
}

int QDesktopWidget::screenNumber(const QPoint &point) const
{
    Q_D(const QDesktopWidget);

    int closestScreen = -1;
    int shortestDistance = INT_MAX;

    for (int i = 0; i < d->screenCount; ++i) {
        int thisDistance = d->pointToRect(point, d->rects->at(i));
        if (thisDistance < shortestDistance) {
            shortestDistance = thisDistance;
            closestScreen = i;
        }
    }

    return closestScreen;
}

void QDesktopWidget::resizeEvent(QResizeEvent *)
{
    Q_D(QDesktopWidget);
    const QVector<QRect> oldrects(*d->rects);
    const QVector<QRect> oldworkrects(*d->workrects);
    int oldscreencount = d->screenCount;

    QDesktopWidgetPrivate::cleanup();
    QDesktopWidgetPrivate::init(this);
#ifdef Q_WS_WINCE_WM
    for(int i=0; i < d->workrects->size(); ++i)
        qt_get_sip_info((*d->workrects)[i]);
#endif

    for (int i = 0; i < qMin(oldscreencount, d->screenCount); ++i) {
        const QRect oldrect = oldrects[i];
        const QRect newrect = d->rects->at(i);
        if (oldrect != newrect)
            emit resized(i);
    }

    for (int j = 0; j < qMin(oldscreencount, d->screenCount); ++j) {
        const QRect oldrect = oldworkrects[j];
        const QRect newrect = d->workrects->at(j);
        if (oldrect != newrect)
            emit workAreaResized(j);
    }

    if (oldscreencount != d->screenCount) {
        emit screenCountChanged(d->screenCount);
    }
}

#ifdef Q_CC_MSVC
# pragma warning(pop)
#endif

QT_END_NAMESPACE
