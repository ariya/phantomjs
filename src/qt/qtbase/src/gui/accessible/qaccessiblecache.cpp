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

#include "qaccessiblecache_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QAccessibleCache
    \internal
    \brief Maintains a cache of accessible interfaces.
*/

Q_GLOBAL_STATIC(QAccessibleCache, qAccessibleCache)

QAccessibleCache *QAccessibleCache::instance()
{
    return qAccessibleCache;
}

/*
  The ID is always in the range [INT_MAX+1, UINT_MAX].
  This makes it easy on windows to reserve the positive integer range
  for the index of a child and not clash with the unique ids.
*/
QAccessible::Id QAccessibleCache::acquireId() const
{
    static const QAccessible::Id FirstId = QAccessible::Id(INT_MAX) + 1;
    static QAccessible::Id lastUsedId = FirstId;

    while (idToInterface.contains(lastUsedId)) {
        // (wrap back when when we reach UINT_MAX - 1)
        // -1 because on Android -1 is taken for the "View" so just avoid it completely for consistency
        if (lastUsedId == UINT_MAX - 1)
            lastUsedId = FirstId;
        else
            ++lastUsedId;
    }

    return lastUsedId;
}

QAccessibleInterface *QAccessibleCache::interfaceForId(QAccessible::Id id) const
{
    return idToInterface.value(id);
}

QAccessible::Id QAccessibleCache::insert(QObject *object, QAccessibleInterface *iface) const
{
    Q_ASSERT(iface);
    Q_UNUSED(object)

    // object might be 0
    Q_ASSERT(!objectToId.contains(object));
    Q_ASSERT_X(!idToInterface.values().contains(iface), "", "Accessible interface inserted into cache twice!");

    QAccessible::Id id = acquireId();
    QObject *obj = iface->object();
    Q_ASSERT(object == obj);
    if (obj) {
        objectToId.insert(obj, id);
        connect(obj, &QObject::destroyed, this, &QAccessibleCache::objectDestroyed);
    }
    idToInterface.insert(id, iface);
    return id;
}

void QAccessibleCache::objectDestroyed(QObject* obj)
{
    QAccessible::Id id = objectToId.value(obj);
    if (id) {
        Q_ASSERT_X(idToInterface.contains(id), "", "QObject with accessible interface deleted, where interface not in cache!");
        deleteInterface(id, obj);
    }
}

void QAccessibleCache::deleteInterface(QAccessible::Id id, QObject *obj)
{
    QAccessibleInterface *iface = idToInterface.take(id);
    if (!obj)
        obj = iface->object();
    if (obj)
        objectToId.remove(obj);
    delete iface;

#ifdef Q_OS_MACX
    removeCocoaElement(id);
#endif
}

QT_END_NAMESPACE
