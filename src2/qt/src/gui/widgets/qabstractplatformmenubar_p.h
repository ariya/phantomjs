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

#ifndef QABSTRACTPLATFORMMENUBAR_P_H
#define QABSTRACTPLATFORMMENUBAR_P_H

#include <qfactoryinterface.h>
#include <qglobal.h>
#include <qplugin.h>

#ifndef QT_NO_MENUBAR

QT_BEGIN_NAMESPACE

class QAction;
class QActionEvent;
class QEvent;
class QMenuBar;
class QObject;
class QWidget;

class QAbstractPlatformMenuBar;

struct QPlatformMenuBarFactoryInterface : public QFactoryInterface
{
    virtual QAbstractPlatformMenuBar *create() = 0;
};

#define QPlatformMenuBarFactoryInterface_iid "com.nokia.qt.QPlatformMenuBarFactoryInterface"
Q_DECLARE_INTERFACE(QPlatformMenuBarFactoryInterface, QPlatformMenuBarFactoryInterface_iid)

/*!
    The platform-specific implementation of a menubar
*/
class QAbstractPlatformMenuBar
{
public:
    virtual ~QAbstractPlatformMenuBar() {}

    virtual void init(QMenuBar *) = 0;

    virtual void setVisible(bool visible) = 0;

    virtual void actionEvent(QActionEvent *) = 0;

    virtual void handleReparent(QWidget *oldParent, QWidget *newParent, QWidget *oldWindow, QWidget *newWindow) = 0;

    virtual bool allowCornerWidgets() const = 0;

    virtual void popupAction(QAction *) = 0;

    virtual void setNativeMenuBar(bool) = 0;

    virtual bool isNativeMenuBar() const = 0;

    /*!
        Return true if the native menubar is capable of listening to the
        shortcut keys. If false is returned, QMenuBar will trigger actions on
        shortcut itself.
    */
    virtual bool shortcutsHandledByNativeMenuBar() const = 0;

    virtual bool menuBarEventFilter(QObject *, QEvent *event) = 0;
};

QT_END_NAMESPACE

#endif // QT_NO_MENUBAR

#endif // QABSTRACTPLATFORMMENUBAR_P_H
