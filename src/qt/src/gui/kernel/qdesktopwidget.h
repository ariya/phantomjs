/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QDESKTOPWIDGET_H
#define QDESKTOPWIDGET_H

#include <QtGui/qwidget.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QApplication;
class QDesktopWidgetPrivate;

class Q_GUI_EXPORT QDesktopWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool virtualDesktop READ isVirtualDesktop)
    Q_PROPERTY(int screenCount READ screenCount NOTIFY screenCountChanged)
    Q_PROPERTY(int primaryScreen READ primaryScreen)
public:
    QDesktopWidget();
    ~QDesktopWidget();

    bool isVirtualDesktop() const;

    int numScreens() const;
    int screenCount() const;
    int primaryScreen() const;

    int screenNumber(const QWidget *widget = 0) const;
    int screenNumber(const QPoint &) const;

    QWidget *screen(int screen = -1);

    const QRect screenGeometry(int screen = -1) const;
    const QRect screenGeometry(const QWidget *widget) const;
    const QRect screenGeometry(const QPoint &point) const
    { return screenGeometry(screenNumber(point)); }

    const QRect availableGeometry(int screen = -1) const;
    const QRect availableGeometry(const QWidget *widget) const;
    const QRect availableGeometry(const QPoint &point) const
    { return availableGeometry(screenNumber(point)); }

Q_SIGNALS:
    void resized(int);
    void workAreaResized(int);
    void screenCountChanged(int);

protected:
    void resizeEvent(QResizeEvent *e);

private:
    Q_DISABLE_COPY(QDesktopWidget)
    Q_DECLARE_PRIVATE(QDesktopWidget)

    friend class QApplication;
    friend class QApplicationPrivate;
};

inline int QDesktopWidget::screenCount() const
{ return numScreens(); }

QT_END_NAMESPACE

QT_END_HEADER

#endif // QDESKTOPWIDGET_H
