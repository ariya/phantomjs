/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QMETATYPESWITCHER_P_H
#define QMETATYPESWITCHER_P_H

#include "qmetatype.h"

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

QT_BEGIN_NAMESPACE

class QMetaTypeSwitcher {
public:
    class NotBuiltinType;   // type is not a built-in type, but it may be a custom type or an unknown type
    class UnknownType;      // type not known to QMetaType system
    template<class ReturnType, class DelegateObject>
    static ReturnType switcher(DelegateObject &logic, int type, const void *data);
};


#define QT_METATYPE_SWICHER_CASE(TypeName, TypeId, Name)\
    case QMetaType::TypeName: return logic.delegate(static_cast<Name const *>(data));

template<class ReturnType, class DelegateObject>
ReturnType QMetaTypeSwitcher::switcher(DelegateObject &logic, int type, const void *data)
{
    switch (QMetaType::Type(type)) {
    QT_FOR_EACH_STATIC_TYPE(QT_METATYPE_SWICHER_CASE)

    case QMetaType::UnknownType:
        return logic.delegate(static_cast<UnknownType const *>(data));
    default:
        if (type < QMetaType::User)
            return logic.delegate(static_cast<UnknownType const *>(data));
        return logic.delegate(static_cast<NotBuiltinType const *>(data));
    }
}

#undef QT_METATYPE_SWICHER_CASE

QT_END_NAMESPACE

#endif // QMETATYPESWITCHER_P_H
