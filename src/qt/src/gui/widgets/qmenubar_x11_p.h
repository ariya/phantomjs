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

#ifndef QX11MENUBAR_P_H
#define QX11MENUBAR_P_H

#ifndef QT_NO_MENUBAR

#include "qabstractplatformmenubar_p.h"

QT_BEGIN_NAMESPACE

class QMenuBar;

class QX11MenuBar : public QAbstractPlatformMenuBar
{
public:
    ~QX11MenuBar();

    virtual void init(QMenuBar *);

    virtual void setVisible(bool visible);

    virtual void actionEvent(QActionEvent *e);

    virtual void handleReparent(QWidget *oldParent, QWidget *newParent, QWidget *oldWindow, QWidget *newWindow);

    virtual bool allowCornerWidgets() const;

    virtual void popupAction(QAction*);

    virtual void setNativeMenuBar(bool);
    virtual bool isNativeMenuBar() const;

    virtual bool shortcutsHandledByNativeMenuBar() const;
    virtual bool menuBarEventFilter(QObject *, QEvent *event);

private:
    QMenuBar *menuBar;
    int nativeMenuBar : 3;  // Only has values -1, 0, and 1
};

QPlatformMenuBarFactoryInterface *qt_guiPlatformMenuBarFactory();

QT_END_NAMESPACE

#endif // QT_NO_MENUBAR

#endif /* QX11MENUBAR_P_H */
