/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author James Turner <james.turner@kdab.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QPLATFORMMENU_H
#define QPLATFORMMENU_H
//
//  W A R N I N G
//  -------------
//
// This file is part of the QPA API and is not meant to be used
// in applications. Usage of this API may make your code
// source and binary incompatible with future versions of Qt.
//

#include <QtCore/qglobal.h>
#include <QtCore/qpointer.h>
#include <QtGui/QFont>
#include <QtGui/QKeySequence>
#include <QtGui/QIcon>

QT_BEGIN_NAMESPACE

class QPlatformMenu;
class Q_GUI_EXPORT QPlatformMenuItem : public QObject
{
Q_OBJECT
public:
    // copied from, and must stay in sync with, QAction menu roles.
    enum MenuRole { NoRole = 0, TextHeuristicRole, ApplicationSpecificRole, AboutQtRole,
                    AboutRole, PreferencesRole, QuitRole,
                    // However these roles are private, perhaps temporarily.
                    // They could be added as public QAction roles if necessary.
                    CutRole, CopyRole, PasteRole, SelectAllRole,
                    RoleCount };

    virtual void setTag(quintptr tag) = 0;
    virtual quintptr tag()const = 0;

    virtual void setText(const QString &text) = 0;
    virtual void setIcon(const QIcon &icon) = 0;
    virtual void setMenu(QPlatformMenu *menu) = 0;
    virtual void setVisible(bool isVisible) = 0;
    virtual void setIsSeparator(bool isSeparator) = 0;
    virtual void setFont(const QFont &font) = 0;
    virtual void setRole(MenuRole role) = 0;
    virtual void setCheckable(bool checkable) = 0;
    virtual void setChecked(bool isChecked) = 0;
    virtual void setShortcut(const QKeySequence& shortcut) = 0;
    virtual void setEnabled(bool enabled) = 0;
    virtual void setIconSize(int size) = 0;
    virtual void setNativeContents(WId item) { Q_UNUSED(item); }

Q_SIGNALS:
    void activated();
    void hovered();
};

class Q_GUI_EXPORT QPlatformMenu : public QObject
{
Q_OBJECT
public:
    enum MenuType { DefaultMenu = 0, EditMenu };

    virtual void insertMenuItem(QPlatformMenuItem *menuItem, QPlatformMenuItem *before) = 0;
    virtual void removeMenuItem(QPlatformMenuItem *menuItem) = 0;
    virtual void syncMenuItem(QPlatformMenuItem *menuItem) = 0;
    virtual void syncSeparatorsCollapsible(bool enable) = 0;

    virtual void setTag(quintptr tag) = 0;
    virtual quintptr tag()const = 0;

    virtual void setText(const QString &text) = 0;
    virtual void setIcon(const QIcon &icon) = 0;
    virtual void setEnabled(bool enabled) = 0;
    virtual void setVisible(bool visible) = 0;
    virtual void setMinimumWidth(int width) { Q_UNUSED(width); }
    virtual void setFont(const QFont &font) { Q_UNUSED(font); }
    virtual void setMenuType(MenuType type) { Q_UNUSED(type); }

    virtual void showPopup(const QWindow *parentWindow, const QRect &targetRect, const QPlatformMenuItem *item)
    {
        Q_UNUSED(parentWindow);
        Q_UNUSED(targetRect);
        Q_UNUSED(item);
        setVisible(true);
    }

    virtual void dismiss() { } // Closes this and all its related menu popups

    virtual QPlatformMenuItem *menuItemAt(int position) const = 0;
    virtual QPlatformMenuItem *menuItemForTag(quintptr tag) const = 0;

    virtual QPlatformMenuItem *createMenuItem() const;
Q_SIGNALS:
    void aboutToShow();
    void aboutToHide();
};

class Q_GUI_EXPORT QPlatformMenuBar : public QObject
{
Q_OBJECT
public:
    virtual void insertMenu(QPlatformMenu *menu, QPlatformMenu *before) = 0;
    virtual void removeMenu(QPlatformMenu *menu) = 0;
    virtual void syncMenu(QPlatformMenu *menuItem) = 0;
    virtual void handleReparent(QWindow *newParentWindow) = 0;

    virtual QPlatformMenu *menuForTag(quintptr tag) const = 0;
};

QT_END_NAMESPACE

#endif

