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

#ifndef QWIDGETACTION_H
#define QWIDGETACTION_H

#include <QtGui/qaction.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_ACTION

class QWidgetActionPrivate;

class Q_GUI_EXPORT QWidgetAction : public QAction
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QWidgetAction)

public:
    explicit QWidgetAction(QObject *parent);
    virtual ~QWidgetAction();
    
    void setDefaultWidget(QWidget *w);
    QWidget *defaultWidget() const;

    QWidget *requestWidget(QWidget *parent);
    void releaseWidget(QWidget *widget);
    
protected:
    virtual bool event(QEvent *);
    virtual bool eventFilter(QObject *, QEvent *);
    virtual QWidget *createWidget(QWidget *parent);
    virtual void deleteWidget(QWidget *widget);
    QList<QWidget *> createdWidgets() const;

private:
    Q_DISABLE_COPY(QWidgetAction)
    Q_PRIVATE_SLOT(d_func(), void _q_widgetDestroyed(QObject *))
    friend class QToolBar;
};

#endif // QT_NO_ACTION

QT_END_NAMESPACE

QT_END_HEADER

#endif // QWIDGETACTION_H
