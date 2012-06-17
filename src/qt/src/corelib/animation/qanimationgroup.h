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

#ifndef QANIMATIONGROUP_H
#define QANIMATIONGROUP_H

#include <QtCore/qabstractanimation.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

#ifndef QT_NO_ANIMATION

class QAnimationGroupPrivate;
class Q_CORE_EXPORT QAnimationGroup : public QAbstractAnimation
{
    Q_OBJECT

public:
    QAnimationGroup(QObject *parent = 0);
    ~QAnimationGroup();

    QAbstractAnimation *animationAt(int index) const;
    int animationCount() const;
    int indexOfAnimation(QAbstractAnimation *animation) const;
    void addAnimation(QAbstractAnimation *animation);
    void insertAnimation(int index, QAbstractAnimation *animation);
    void removeAnimation(QAbstractAnimation *animation);
    QAbstractAnimation *takeAnimation(int index);
    void clear();

protected:
    QAnimationGroup(QAnimationGroupPrivate &dd, QObject *parent);
    bool event(QEvent *event);

private:
    Q_DISABLE_COPY(QAnimationGroup)
    Q_DECLARE_PRIVATE(QAnimationGroup)
};

#endif //QT_NO_ANIMATION

QT_END_NAMESPACE

QT_END_HEADER

#endif //QANIMATIONGROUP_H
