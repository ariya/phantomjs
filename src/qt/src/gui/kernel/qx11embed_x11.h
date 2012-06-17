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

#ifndef QX11EMBED_X11_H
#define QX11EMBED_X11_H

#include <QtGui/qwidget.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QX11EmbedWidgetPrivate;
class Q_GUI_EXPORT QX11EmbedWidget : public QWidget
{
    Q_OBJECT
public:
    QX11EmbedWidget(QWidget *parent = 0);
    ~QX11EmbedWidget();

    void embedInto(WId id);
    WId containerWinId() const;

    enum Error {
	Unknown,
	Internal,
	InvalidWindowID
    };
    Error error() const;

Q_SIGNALS:
    void embedded();
    void containerClosed();
    void error(QX11EmbedWidget::Error error);

protected:
    bool x11Event(XEvent *);
    bool eventFilter(QObject *, QEvent *);
    bool event(QEvent *);
    void resizeEvent(QResizeEvent *);

private:
    Q_DECLARE_PRIVATE(QX11EmbedWidget)
    Q_DISABLE_COPY(QX11EmbedWidget)
};

class QX11EmbedContainerPrivate;
class Q_GUI_EXPORT QX11EmbedContainer : public QWidget
{
    Q_OBJECT
public:
    QX11EmbedContainer(QWidget *parent = 0);
    ~QX11EmbedContainer();

    void embedClient(WId id);
    void discardClient();

    WId clientWinId() const;

    QSize minimumSizeHint() const;

    enum Error {
	Unknown,
	Internal,
	InvalidWindowID
    };
    Error error() const;

Q_SIGNALS:
    void clientIsEmbedded();
    void clientClosed();
    void error(QX11EmbedContainer::Error);

protected:
    bool x11Event(XEvent *);
    bool eventFilter(QObject *, QEvent *);
    void paintEvent(QPaintEvent *e);
    void resizeEvent(QResizeEvent *);
    void showEvent(QShowEvent *);
    void hideEvent(QHideEvent *);
    bool event(QEvent *);

private:
    Q_DECLARE_PRIVATE(QX11EmbedContainer)
    Q_DISABLE_COPY(QX11EmbedContainer)
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QX11EMBED_X11_H
