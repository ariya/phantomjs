/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author James Turner <james.turner@kdab.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins module of the Qt Toolkit.
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

#ifndef QCOCOAMENUBAR_H
#define QCOCOAMENUBAR_H

#include <QtCore/QList>
#include <qpa/qplatformmenu.h>
#include "qcocoamenu.h"

QT_BEGIN_NAMESPACE

class QCocoaWindow;

class QCocoaMenuBar : public QPlatformMenuBar
{
    Q_OBJECT
public:
    QCocoaMenuBar();
    virtual ~QCocoaMenuBar();

    virtual void insertMenu(QPlatformMenu *menu, QPlatformMenu* before);
    virtual void removeMenu(QPlatformMenu *menu);
    virtual void syncMenu(QPlatformMenu *menuItem);
    virtual void handleReparent(QWindow *newParentWindow);
    virtual QPlatformMenu *menuForTag(quintptr tag) const;

    inline NSMenu *nsMenu() const
        { return m_nativeMenu; }

    static void redirectKnownMenuItemsToFirstResponder();
    static void resetKnownMenuItemsToQt();
    static void updateMenuBarImmediately();

    QList<QCocoaMenuItem*> merged() const;
    NSMenuItem *itemForRole(QPlatformMenuItem::MenuRole r);

private:
    static QCocoaWindow *findWindowForMenubar();
    static QCocoaMenuBar *findGlobalMenubar();

    bool shouldDisable(QCocoaWindow *active) const;
    void insertNativeMenu(QCocoaMenu *menu, QCocoaMenu *beforeMenu);
    void removeNativeMenu(QCocoaMenu *menu);

    QList<QCocoaMenu*> m_menus;
    NSMenu *m_nativeMenu;
    QCocoaWindow *m_window;
};

QT_END_NAMESPACE

#endif
