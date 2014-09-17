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

#include "qapplication.h"
#include "qdesktopwidget.h"
#include "qlibrary.h"
#include "qt_x11_p.h"
#include "qvariant.h"
#include "qwidget_p.h"
#include "qx11info_x11.h"
#include <limits.h>

QT_BEGIN_NAMESPACE

// defined in qwidget_x11.cpp
extern int qt_x11_create_desktop_on_screen;


// function to update the workarea of the screen
static bool qt_desktopwidget_workarea_dirty = true;
void qt_desktopwidget_update_workarea()
{
    qt_desktopwidget_workarea_dirty = true;
}


class QSingleDesktopWidget : public QWidget
{
public:
    QSingleDesktopWidget();
    ~QSingleDesktopWidget();
};

QSingleDesktopWidget::QSingleDesktopWidget()
    : QWidget(0, Qt::Desktop)
{
}

QSingleDesktopWidget::~QSingleDesktopWidget()
{
    const QObjectList &childList = children();
    for (int i = childList.size(); i > 0 ;) {
        --i;
        childList.at(i)->setParent(0);
    }
}


class QDesktopWidgetPrivate : public QWidgetPrivate
{
public:
    QDesktopWidgetPrivate();
    ~QDesktopWidgetPrivate();

    void init();

    bool use_xinerama;
    int defaultScreen;
    int screenCount;

    QWidget **screens;
    QRect *rects;
    QRect *workareas;
};

QDesktopWidgetPrivate::QDesktopWidgetPrivate()
    : use_xinerama(false), defaultScreen(0), screenCount(1),
      screens(0), rects(0), workareas(0)
{
}

QDesktopWidgetPrivate::~QDesktopWidgetPrivate()
{
    if (screens) {
        for (int i = 0; i < screenCount; ++i) {
            if (i == defaultScreen) continue;
            delete screens[i];
            screens[i] = 0;
        }

        free (screens);
    }

    if (rects)     delete [] rects;
    if (workareas) delete [] workareas;
}

void QDesktopWidgetPrivate::init()
{
    // get the screen count
    int newScreenCount = ScreenCount(X11->display);
#ifndef QT_NO_XINERAMA

    XineramaScreenInfo *xinerama_screeninfo = 0;

    // we ignore the Xinerama extension when using the display is
    // using traditional multi-screen (with multiple root windows)
    if (newScreenCount == 1
        && X11->ptrXineramaQueryExtension
        && X11->ptrXineramaIsActive
        && X11->ptrXineramaQueryScreens) {
        int unused;
        use_xinerama = (X11->ptrXineramaQueryExtension(X11->display, &unused, &unused)
                        && X11->ptrXineramaIsActive(X11->display));
    }

    if (use_xinerama) {
        xinerama_screeninfo =
            X11->ptrXineramaQueryScreens(X11->display, &newScreenCount);
    }

    if (xinerama_screeninfo) {
        defaultScreen = 0;
     } else
#endif // QT_NO_XINERAMA
    {
        defaultScreen = DefaultScreen(X11->display);
        newScreenCount = ScreenCount(X11->display);
        use_xinerama = false;
    }

    delete [] rects;
    rects     = new QRect[newScreenCount];
    delete [] workareas;
    workareas = new QRect[newScreenCount];

    // get the geometry of each screen
    int i, j, x, y, w, h;
    for (i = 0, j = 0; i < newScreenCount; i++, j++) {

#ifndef QT_NO_XINERAMA
        if (use_xinerama) {
            x = xinerama_screeninfo[i].x_org;
            y = xinerama_screeninfo[i].y_org;
            w = xinerama_screeninfo[i].width;
            h = xinerama_screeninfo[i].height;
        } else
#endif // QT_NO_XINERAMA
            {
                x = 0;
                y = 0;
                w = WidthOfScreen(ScreenOfDisplay(X11->display, i));
                h = HeightOfScreen(ScreenOfDisplay(X11->display, i));
            }

        rects[j].setRect(x, y, w, h);

        if (use_xinerama && j > 0 && rects[j-1].intersects(rects[j])) {
            // merge a "cloned" screen with the previous, hiding all crtcs
            // that are currently showing a sub-rect of the previous screen
            if ((rects[j].width()*rects[j].height()) >
                (rects[j-1].width()*rects[j-1].height()))
                rects[j-1] = rects[j];
            j--;
        }

        workareas[i] = QRect();
    }

    if (screens) {
        // leaks QWidget* pointers on purpose, can't delete them as pointer escapes
        screens = q_check_ptr((QWidget**) realloc(screens, j * sizeof(QWidget*)));
        if (j > screenCount)
            memset(&screens[screenCount], 0, (j-screenCount) * sizeof(QWidget*));
    }

    screenCount = j;

#ifndef QT_NO_XINERAMA
    if (use_xinerama && screenCount == 1)
        use_xinerama = false;

    if (xinerama_screeninfo)
        XFree(xinerama_screeninfo);
#endif // QT_NO_XINERAMA

}

// the QDesktopWidget itself will be created on the default screen
// as qt_x11_create_desktop_on_screen defaults to -1
QDesktopWidget::QDesktopWidget()
    : QWidget(*new QDesktopWidgetPrivate, 0, Qt::Desktop)
{
    Q_D(QDesktopWidget);
    d->init();
}

QDesktopWidget::~QDesktopWidget()
{
}

bool QDesktopWidget::isVirtualDesktop() const
{
    Q_D(const QDesktopWidget);
    return d->use_xinerama;
}

int QDesktopWidget::primaryScreen() const
{
    Q_D(const QDesktopWidget);
    return d->defaultScreen;
}

int QDesktopWidget::numScreens() const
{
    Q_D(const QDesktopWidget);
    return d->screenCount;
}

QWidget *QDesktopWidget::screen(int screen)
{
    Q_D(QDesktopWidget);
    if (d->use_xinerama)
        return this;

    if (screen < 0 || screen >= d->screenCount)
        screen = d->defaultScreen;

    if (! d->screens) {
        d->screens = (QWidget**) calloc( d->screenCount, sizeof(QWidget*));
        d->screens[d->defaultScreen] = this;
    }

    if (! d->screens[screen] ||               // not created yet
        ! (d->screens[screen]->windowType() == Qt::Desktop)) { // reparented away
        qt_x11_create_desktop_on_screen = screen;
        d->screens[screen] = new QSingleDesktopWidget;
        qt_x11_create_desktop_on_screen = -1;
    }

    return d->screens[screen];
}

const QRect QDesktopWidget::availableGeometry(int screen) const
{
    Q_D(const QDesktopWidget);
    if (qt_desktopwidget_workarea_dirty) {
        // the workareas are dirty, invalidate them
        for (int i = 0; i < d->screenCount; ++i)
            d->workareas[i] = QRect();
        qt_desktopwidget_workarea_dirty = false;
    }

    if (screen < 0 || screen >= d->screenCount)
        screen = d->defaultScreen;

    if (d->workareas[screen].isValid())
        return d->workareas[screen];

    if (X11->isSupportedByWM(ATOM(_NET_WORKAREA))) {
        int x11Screen = isVirtualDesktop() ? DefaultScreen(X11->display) : screen;

        Atom ret;
        int format, e;
        unsigned char *data = 0;
        unsigned long nitems, after;

        e = XGetWindowProperty(X11->display,
                               QX11Info::appRootWindow(x11Screen),
                               ATOM(_NET_WORKAREA), 0, 4, False, XA_CARDINAL,
                               &ret, &format, &nitems, &after, &data);

        QRect workArea;
        if (e == Success && ret == XA_CARDINAL &&
            format == 32 && nitems == 4) {
            long *workarea = (long *) data;
            workArea = QRect(workarea[0], workarea[1], workarea[2], workarea[3]);
        } else {
            workArea = screenGeometry(screen);
        }

        if (isVirtualDesktop()) {
            // intersect the workarea (which spawns all Xinerama screens) with the rect for the
            // requested screen
            workArea &= screenGeometry(screen);
        }

        d->workareas[screen] = workArea;

        if (data)
            XFree(data);
    } else {
        d->workareas[screen] = screenGeometry(screen);
    }

    return d->workareas[screen];
}

const QRect QDesktopWidget::screenGeometry(int screen) const
{
    Q_D(const QDesktopWidget);
    if (screen < 0 || screen >= d->screenCount)
        screen = d->defaultScreen;

    return d->rects[screen];
}

int QDesktopWidget::screenNumber(const QWidget *widget) const
{
    Q_D(const QDesktopWidget);
    if (!widget)
        return d->defaultScreen;

#ifndef QT_NO_XINERAMA
    if (d->use_xinerama) {
        // this is how we do it for xinerama
        QRect frame = widget->frameGeometry();
        if (!widget->isWindow())
            frame.moveTopLeft(widget->mapToGlobal(QPoint(0, 0)));

        int maxSize = -1;
        int maxScreen = -1;

        for (int i = 0; i < d->screenCount; ++i) {
            QRect sect = d->rects[i].intersected(frame);
            int size = sect.width() * sect.height();
            if (size > maxSize && sect.width() > 0 && sect.height() > 0) {
                maxSize = size;
                maxScreen = i;
            }
        }
        return maxScreen;
    }
#endif // QT_NO_XINERAMA

    return widget->x11Info().screen();
}

int QDesktopWidget::screenNumber(const QPoint &point) const
{
    Q_D(const QDesktopWidget);
    int closestScreen = -1;
    int shortestDistance = INT_MAX;
    for (int i = 0; i < d->screenCount; ++i) {
        int thisDistance = d->pointToRect(point, d->rects[i]);
        if (thisDistance < shortestDistance) {
            shortestDistance = thisDistance;
            closestScreen = i;
        }
    }
    return closestScreen;
}

void QDesktopWidget::resizeEvent(QResizeEvent *event)
{
    Q_D(QDesktopWidget);
    int oldScreenCount = d->screenCount;
    QVector<QRect> oldRects(oldScreenCount);
    for (int i = 0; i < oldScreenCount; ++i) {
        oldRects[i] = d->rects[i];
    }

    d->init();

    for (int i = 0; i < qMin(oldScreenCount, d->screenCount); ++i) {
        if (oldRects.at(i) != d->rects[i])
            emit resized(i);
    }

    if (oldScreenCount != d->screenCount) {
        emit screenCountChanged(d->screenCount);
    }

    qt_desktopwidget_workarea_dirty = true;
    QWidget::resizeEvent(event);
}

QT_END_NAMESPACE
