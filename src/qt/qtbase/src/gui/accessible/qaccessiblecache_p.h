/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QACCESSIBLECACHE_P
#define QACCESSIBLECACHE_P

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qhash.h>

#include "qaccessible.h"

Q_FORWARD_DECLARE_OBJC_CLASS(QCocoaAccessibleElement);

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QAccessibleCache  :public QObject
{
    Q_OBJECT

public:
    static QAccessibleCache *instance();
    QAccessibleInterface *interfaceForId(QAccessible::Id id) const;
    QAccessible::Id insert(QObject *object, QAccessibleInterface *iface) const;
    void deleteInterface(QAccessible::Id id, QObject *obj = 0);

#ifdef Q_OS_OSX
    QCocoaAccessibleElement *elementForId(QAccessible::Id axid) const;
    void insertElement(QAccessible::Id axid, QCocoaAccessibleElement *element) const;
#endif

private Q_SLOTS:
    void objectDestroyed(QObject *obj);

private:
    QAccessible::Id acquireId() const;

    mutable QHash<QAccessible::Id, QAccessibleInterface *> idToInterface;
    mutable QHash<QObject *, QAccessible::Id> objectToId;

#ifdef Q_OS_OSX
    void removeCocoaElement(QAccessible::Id axid);
    mutable QHash<QAccessible::Id, QCocoaAccessibleElement *> cocoaElements;
#endif

    friend class QAccessible;
    friend class QAccessibleInterface;
};

QT_END_NAMESPACE

#endif
