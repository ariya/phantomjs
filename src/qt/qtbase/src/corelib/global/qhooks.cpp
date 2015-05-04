/****************************************************************************
**
** Copyright (C) 2014 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Volker Krause <volker.krause@kdab.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qhooks_p.h"

QT_BEGIN_NAMESPACE

// Only add to the end, and bump version if you do.
quintptr Q_CORE_EXPORT qtHookData[] = {
    1, // hook data version
    QHooks::LastHookIndex, // size of qtHookData
    QT_VERSION,

    // AddQObject, void(*)(QObject*), called for every constructed QObject
    // Note: this is called from the QObject constructor, ie. the sub-class
    // constructors haven't run yet.
    0,

    // RemoveQObject, void(*)(QObject*), called for every destructed QObject
    // Note: this is called from the QObject destructor, ie. the object
    // you get as an argument is already largely invalid.
    0,

    // Startup, void(*)(), called once QCoreApplication is operational
    0
};

Q_STATIC_ASSERT(QHooks::LastHookIndex == sizeof(qtHookData) / sizeof(qtHookData[0]));

QT_END_NAMESPACE

