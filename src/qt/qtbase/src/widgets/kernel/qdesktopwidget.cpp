/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qglobal.h"
#include "qdesktopwidget.h"
#include "qdesktopwidget_p.h"
#include "qscreen.h"
#include "qwidget_p.h"

QT_BEGIN_NAMESPACE

const QRect QDesktopWidget::screenGeometry(const QWidget *widget) const
{
    if (!widget) {
        qWarning("QDesktopWidget::screenGeometry(): Attempt "
                 "to get the screen geometry of a null widget");
        return QRect();
    }
    QRect rect = QWidgetPrivate::screenGeometry(widget);
    if (rect.isNull())
        return screenGeometry(screenNumber(widget));
    else return rect;
}

const QRect QDesktopWidget::availableGeometry(const QWidget *widget) const
{
    if (!widget) {
        qWarning("QDesktopWidget::availableGeometry(): Attempt "
                 "to get the available geometry of a null widget");
        return QRect();
    }
    QRect rect = QWidgetPrivate::screenGeometry(widget);
    if (rect.isNull())
        return availableGeometry(screenNumber(widget));
    else
        return rect;
}

void QDesktopWidgetPrivate::_q_updateScreens()
{
    Q_Q(QDesktopWidget);
    const QList<QScreen *> screenList = QGuiApplication::screens();
    const int targetLength = screenList.length();
    const int oldLength = screens.length();
    int currentLength = oldLength;

    // Add or remove screen widgets as necessary
    if(currentLength > targetLength) {
        QDesktopScreenWidget *screen;
        while (currentLength-- > targetLength) {
            screen = screens.takeLast();
            delete screen;
        }
    }
    else if (currentLength < targetLength) {
        while (currentLength < targetLength) {
            QScreen *qScreen = screenList.at(currentLength);
            QDesktopScreenWidget *screenWidget = new QDesktopScreenWidget(currentLength++);
            screenWidget->setGeometry(qScreen->geometry());
            QObject::connect(qScreen, SIGNAL(geometryChanged(QRect)),
                             q, SLOT(_q_updateScreens()), Qt::QueuedConnection);
            QObject::connect(qScreen, SIGNAL(availableGeometryChanged(QRect)),
                             q, SLOT(_q_availableGeometryChanged()), Qt::QueuedConnection);
            QObject::connect(qScreen, SIGNAL(destroyed()),
                             q, SLOT(_q_updateScreens()), Qt::QueuedConnection);
            screens.append(screenWidget);
        }
    }

    QRegion virtualGeometry;

    // update the geometry of each screen widget, determine virtual geometry
    // and emit change signals afterwards.
    QList<int> changedScreens;
    for (int i = 0; i < screens.length(); i++) {
        const QRect screenGeometry = screenList.at(i)->geometry();
        if (screenGeometry != screens.at(i)->geometry()) {
            screens.at(i)->setGeometry(screenGeometry);
            changedScreens.push_back(i);
        }
        virtualGeometry += screenGeometry;
    }

    q->setGeometry(virtualGeometry.boundingRect());

    if (oldLength != targetLength)
        emit q->screenCountChanged(targetLength);

    foreach (int changedScreen, changedScreens)
        emit q->resized(changedScreen);
}

void QDesktopWidgetPrivate::_q_availableGeometryChanged()
{
    Q_Q(QDesktopWidget);
    if (QScreen *screen = qobject_cast<QScreen *>(q->sender()))
        emit q->workAreaResized(QGuiApplication::screens().indexOf(screen));
}

QDesktopWidget::QDesktopWidget()
    : QWidget(*new QDesktopWidgetPrivate, 0, Qt::Desktop)
{
    Q_D(QDesktopWidget);
    setObjectName(QLatin1String("desktop"));
    d->_q_updateScreens();
    connect(qApp, SIGNAL(screenAdded(QScreen*)), this, SLOT(_q_updateScreens()));
}

QDesktopWidget::~QDesktopWidget()
{
}

bool QDesktopWidget::isVirtualDesktop() const
{
    return QGuiApplication::primaryScreen()->virtualSiblings().size() > 1;
}

int QDesktopWidget::primaryScreen() const
{
    return 0;
}

int QDesktopWidget::numScreens() const
{
    return qMax(QGuiApplication::screens().size(), 1);
}

QWidget *QDesktopWidget::screen(int screen)
{
    Q_D(QDesktopWidget);
    if (screen < 0 || screen >= d->screens.length())
        return d->screens.at(0);
    return d->screens.at(screen);
}

const QRect QDesktopWidget::availableGeometry(int screenNo) const
{
    QList<QScreen *> screens = QGuiApplication::screens();
    if (screenNo == -1)
        screenNo = 0;
    if (screenNo < 0 || screenNo >= screens.size())
        return QRect();
    else
        return screens.at(screenNo)->availableGeometry();
}

const QRect QDesktopWidget::screenGeometry(int screenNo) const
{
    QList<QScreen *> screens = QGuiApplication::screens();
    if (screenNo == -1)
        screenNo = 0;
    if (screenNo < 0 || screenNo >= screens.size())
        return QRect();
    else
        return screens.at(screenNo)->geometry();
}

int QDesktopWidget::screenNumber(const QWidget *w) const
{
    if (!w)
        return 0;

    QRect frame = w->frameGeometry();
    if (!w->isWindow())
        frame.moveTopLeft(w->mapToGlobal(QPoint(0, 0)));
    const QPoint midpoint = (frame.topLeft() + frame.bottomRight()) / 2;
    return screenNumber(midpoint);
}

int QDesktopWidget::screenNumber(const QPoint &p) const
{
    QList<QScreen *> screens = QGuiApplication::screens();

    for (int i = 0; i < screens.size(); ++i)
        if (screens.at(i)->geometry().contains(p))
            return i;

    return primaryScreen(); //even better would be closest screen
}

void QDesktopWidget::resizeEvent(QResizeEvent *)
{
}

QT_END_NAMESPACE

#include "moc_qdesktopwidget.cpp"
