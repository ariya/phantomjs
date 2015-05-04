/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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
#ifndef QCOCOAACCESIBILITY_H
#define QCOCOAACCESIBILITY_H

#include <Cocoa/Cocoa.h>

#include <QtGui>
#include <qpa/qplatformaccessibility.h>

class QCocoaAccessibility : public QPlatformAccessibility
{
public:
    QCocoaAccessibility();
    ~QCocoaAccessibility();
    void notifyAccessibilityUpdate(QAccessibleEvent *event);
    void setRootObject(QObject *o);
    void initialize();
    void cleanup();
};

namespace QCocoaAccessible {

/*
    Qt Cocoa Accessibility Overview

    Cocoa accessibility is implemented in the following files:

    - qcocoaaccessibility (this file) : QCocoaAccessibility "plugin", conversion and helper functions.
    - qnsviewaccessibility            : Root accessibility implementation for QNSView
    - qcocoaaccessibilityelement      : Cocoa accessibility protocol wrapper for QAccessibleInterface

    The accessibility implementation wraps QAccessibleInterfaces in QCocoaAccessibleElements, which
    implements the cocoa accessibility protocol. The root QAccessibleInterface (the one returned
    by QWindow::accessibleRoot), is anchored to the QNSView in qnsviewaccessibility.mm.

    Cocoa explores the accessibility tree by walking the tree using the parent/child
    relationships or hit testing. When this happens we create QCocoaAccessibleElements on
    demand.
*/

NSString *macRole(QAccessibleInterface *interface);
NSString *macSubrole(QAccessibleInterface *interface);
bool shouldBeIgnored(QAccessibleInterface *interface);
NSArray *unignoredChildren(QAccessibleInterface *interface);
NSString *getTranslatedAction(const QString &qtAction);
NSMutableArray *createTranslatedActionsList(const QStringList &qtActions);
QString translateAction(NSString *nsAction);
bool hasValueAttribute(QAccessibleInterface *interface);
id getValueAttribute(QAccessibleInterface *interface);

}

#endif
