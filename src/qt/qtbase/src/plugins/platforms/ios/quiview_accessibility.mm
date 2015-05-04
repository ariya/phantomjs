/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qiosplatformaccessibility.h"
#include "quiaccessibilityelement.h"

#include <QtGui/private/qguiapplication_p.h>

@implementation QUIView (Accessibility)

- (void)createAccessibleElement:(QAccessibleInterface *)iface
{
    if (!iface || iface->state().invisible || (iface->text(QAccessible::Name).isEmpty() && iface->text(QAccessible::Value).isEmpty() && iface->text(QAccessible::Description).isEmpty()))
        return;
    QAccessible::Id accessibleId = QAccessible::uniqueId(iface);
    UIAccessibilityElement *elem = [[QMacAccessibilityElement alloc] initWithId: accessibleId withAccessibilityContainer: self];
    [m_accessibleElements addObject: elem];
}

- (void)createAccessibleContainer:(QAccessibleInterface *)iface
{
    if (!iface)
        return;

    [self createAccessibleElement: iface];
    for (int i = 0; i < iface->childCount(); ++i)
        [self createAccessibleContainer: iface->child(i)];
}

- (void)initAccessibility
{
    static bool init = false;
    if (!init)
        QGuiApplicationPrivate::platformIntegration()->accessibility()->setActive(true);
    init = true;

    if ([m_accessibleElements count])
        return;

    QWindow *win = m_qioswindow->window();
    QAccessibleInterface *iface = win->accessibleRoot();
    if (iface)
        [self createAccessibleContainer: iface];
}

- (void)clearAccessibleCache
{
    [m_accessibleElements removeAllObjects];
    UIAccessibilityPostNotification(UIAccessibilityScreenChangedNotification, @"");
}

// this is a container, returning yes here means the functions below will never be called
- (BOOL)isAccessibilityElement
{
    return NO;
}

- (NSInteger)accessibilityElementCount
{
    [self initAccessibility];
    return [m_accessibleElements count];
}

- (id)accessibilityElementAtIndex:(NSInteger)index
{
    [self initAccessibility];
    return m_accessibleElements[index];
}

- (NSInteger)indexOfAccessibilityElement:(id)element
{
    [self initAccessibility];
    return [m_accessibleElements indexOfObject:element];
}

@end
