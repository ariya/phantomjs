/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QANIMATIONGROUP_P_H
#define QANIMATIONGROUP_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of QIODevice. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qanimationgroup.h"

#include <QtCore/qlist.h>

#include "private/qabstractanimation_p.h"

#ifndef QT_NO_ANIMATION

QT_BEGIN_NAMESPACE

class QAnimationGroupPrivate : public QAbstractAnimationPrivate
{
    Q_DECLARE_PUBLIC(QAnimationGroup)
public:
    QAnimationGroupPrivate()
    {
        isGroup = true;
    }

    virtual void animationInsertedAt(int) { }
    virtual void animationRemoved(int, QAbstractAnimation *);

    void disconnectUncontrolledAnimation(QAbstractAnimation *anim)
    {
        //0 for the signal here because we might be called from the animation destructor
        QObject::disconnect(anim, 0, q_func(), SLOT(_q_uncontrolledAnimationFinished()));
    }

    void connectUncontrolledAnimation(QAbstractAnimation *anim)
    {
        QObject::connect(anim, SIGNAL(finished()), q_func(), SLOT(_q_uncontrolledAnimationFinished()));
    }

    QList<QAbstractAnimation *> animations;
};

QT_END_NAMESPACE

#endif //QT_NO_ANIMATION

#endif //QANIMATIONGROUP_P_H
