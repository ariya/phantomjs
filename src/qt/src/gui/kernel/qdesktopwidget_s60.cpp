/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdesktopwidget.h"
#include "qapplication_p.h"
#include "qwidget_p.h"
#include "qt_s60_p.h"
#include <w32std.h>
#if defined(Q_SYMBIAN_SUPPORTS_MULTIPLE_SCREENS)
#include <graphics/displaycontrol.h>
#endif

QT_BEGIN_NAMESPACE

extern int qt_symbian_create_desktop_on_screen;

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
    static void init(QDesktopWidget *that);
    static void cleanup();
    static void init_sys();

    static int screenCount;
    static int primaryScreen;

    static QVector<QRect> *rects;
    static QVector<QRect> *workrects;
    static QVector<QWidget *> *screens;

    static int refcount;

#if defined(Q_SYMBIAN_SUPPORTS_MULTIPLE_SCREENS)
    static MDisplayControl *displayControl;
#endif
};

int QDesktopWidgetPrivate::screenCount = 1;
int QDesktopWidgetPrivate::primaryScreen = 0;
QVector<QRect> *QDesktopWidgetPrivate::rects = 0;
QVector<QRect> *QDesktopWidgetPrivate::workrects = 0;
QVector<QWidget *> *QDesktopWidgetPrivate::screens = 0;
int QDesktopWidgetPrivate::refcount = 0;
#if defined(Q_SYMBIAN_SUPPORTS_MULTIPLE_SCREENS)
MDisplayControl *QDesktopWidgetPrivate::displayControl = 0;
#endif

QDesktopWidgetPrivate::QDesktopWidgetPrivate()
{
    ++refcount;
}

QDesktopWidgetPrivate::~QDesktopWidgetPrivate()
{
    if (!--refcount)
        cleanup();
}

void QDesktopWidgetPrivate::init(QDesktopWidget *that)
{
    // Note that on S^3 devices the screen count retrieved via RWsSession
    // will always be 2 but the width and height for screen number 1 will
    // be 0 as long as TV-out is not connected.
    //
    // On the other hand a valid size for screen 1 will be reported even
    // after the cable is disconnected. In order to overcome this, we use
    // MDisplayControl::NumberOfResolutions() to check if the display is
    // valid or not.

    screenCount = S60->screenCount();
#if defined(Q_SYMBIAN_SUPPORTS_MULTIPLE_SCREENS)
    if (displayControl) {
        if (displayControl->NumberOfResolutions() < 1)
            screenCount = 1;
    }
#endif
    if (screenCount < 1) {
        qWarning("No screen available");
        screenCount = 1;
    }

    rects = new QVector<QRect>();
    workrects = new QVector<QRect>();
    screens = new QVector<QWidget *>();

    rects->resize(screenCount);
    workrects->resize(screenCount);
    screens->resize(screenCount);

    for (int i = 0; i < screenCount; ++i) {
        // All screens will have a position of (0, 0) as there is no true virtual desktop
        // or pointer event support for multiple screens on Symbian.
        QRect r(0, 0,
            S60->screenWidthInPixelsForScreen[i], S60->screenHeightInPixelsForScreen[i]);
        // Stop here if empty and ignore this screen.
        if (r.isEmpty()) {
            screenCount = i;
            break;
        }
        (*rects)[i] = r;
        QRect wr;
        if (i == 0)
            wr = qt_TRect2QRect(S60->clientRect());
        else
            wr = rects->at(i);
        (*workrects)[i].setRect(wr.x(), wr.y(), wr.width(), wr.height());
        (*screens)[i] = 0;
    }
    (*screens)[0] = that;
}

void QDesktopWidgetPrivate::cleanup()
{
    delete rects;
    rects = 0;
    delete workrects;
    workrects = 0;
    if (screens) {
        // First item is the QDesktopWidget so skip it.
        for (int i = 1; i < screens->count(); ++i)
            delete screens->at(i);
    }
    delete screens;
    screens = 0;
}

void QDesktopWidgetPrivate::init_sys()
{
#if defined(Q_SYMBIAN_SUPPORTS_MULTIPLE_SCREENS)
    if (S60->screenCount() > 1) {
        CWsScreenDevice *dev = S60->screenDevice(1);
        if (dev) {
            displayControl = static_cast<MDisplayControl *>(
                        dev->GetInterface(MDisplayControl::ETypeId));
            if (displayControl) {
                displayControl->EnableDisplayChangeEvents(ETrue);
            }
        }
    }
#endif
}


QDesktopWidget::QDesktopWidget()
    : QWidget(*new QDesktopWidgetPrivate, 0, Qt::Desktop)
{
    setObjectName(QLatin1String("desktop"));
    QDesktopWidgetPrivate::init_sys();
    QDesktopWidgetPrivate::init(this);
}

QDesktopWidget::~QDesktopWidget()
{
}

bool QDesktopWidget::isVirtualDesktop() const
{
    return false;
}

int QDesktopWidget::primaryScreen() const
{
    return QDesktopWidgetPrivate::primaryScreen;
}

int QDesktopWidget::numScreens() const
{
    Q_D(const QDesktopWidget);
    return QDesktopWidgetPrivate::screenCount;
}

static inline QWidget *newSingleDesktopWidget(int screen)
{
    qt_symbian_create_desktop_on_screen = screen;
    QWidget *w = new QSingleDesktopWidget;
    qt_symbian_create_desktop_on_screen = -1;
    return w;
}

QWidget *QDesktopWidget::screen(int screen)
{
    Q_D(QDesktopWidget);
    if (screen < 0 || screen >= d->screenCount)
        screen = d->primaryScreen;
    if (!d->screens->at(screen)
        || d->screens->at(screen)->windowType() != Qt::Desktop)
        (*d->screens)[screen] = newSingleDesktopWidget(screen);
    return (*d->screens)[screen];
}

const QRect QDesktopWidget::availableGeometry(int screen) const
{
    Q_D(const QDesktopWidget);
    if (screen < 0 || screen >= d->screenCount)
        screen = d->primaryScreen;

    return d->workrects->at(screen);
}

const QRect QDesktopWidget::screenGeometry(int screen) const
{
    Q_D(const QDesktopWidget);
    if (screen < 0 || screen >= d->screenCount)
        screen = d->primaryScreen;

    return d->rects->at(screen);
}

int QDesktopWidget::screenNumber(const QWidget *widget) const
{
    Q_D(const QDesktopWidget);
    return widget
        ? S60->screenNumberForWidget(widget)
        : d->primaryScreen;
}

int QDesktopWidget::screenNumber(const QPoint &point) const
{
    Q_UNUSED(point);
    Q_D(const QDesktopWidget);
    return d->primaryScreen;
}

void QDesktopWidget::resizeEvent(QResizeEvent *)
{
    Q_D(QDesktopWidget);
    QVector<QRect> oldrects;
    oldrects = *d->rects;
    QVector<QRect> oldworkrects;
    oldworkrects = *d->workrects;
    int oldscreencount = d->screenCount;

    QDesktopWidgetPrivate::cleanup();
    QDesktopWidgetPrivate::init(this);

    for (int i = 0; i < qMin(oldscreencount, d->screenCount); ++i) {
        QRect oldrect = oldrects[i];
        QRect newrect = d->rects->at(i);
        if (oldrect != newrect)
            emit resized(i);
    }

    for (int j = 0; j < qMin(oldscreencount, d->screenCount); ++j) {
        QRect oldrect = oldworkrects[j];
        QRect newrect = d->workrects->at(j);
        if (oldrect != newrect)
            emit workAreaResized(j);
    }

    if (oldscreencount != d->screenCount) {
        emit screenCountChanged(d->screenCount);
    }
}

QT_END_NAMESPACE
