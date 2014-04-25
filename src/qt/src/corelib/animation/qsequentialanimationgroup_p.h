/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QSEQUENTIALANIMATIONGROUP_P_H
#define QSEQUENTIALANIMATIONGROUP_P_H

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

#include "qsequentialanimationgroup.h"
#include "private/qanimationgroup_p.h"

#ifndef QT_NO_ANIMATION

QT_BEGIN_NAMESPACE

class QSequentialAnimationGroupPrivate : public QAnimationGroupPrivate
{
    Q_DECLARE_PUBLIC(QSequentialAnimationGroup)
public:
    QSequentialAnimationGroupPrivate()
        : currentAnimation(0), currentAnimationIndex(-1), lastLoop(0)
    { }


    struct AnimationIndex
    {
        AnimationIndex() : index(0), timeOffset(0) {}
        // index points to the animation at timeOffset, skipping 0 duration animations.
        // Note that the index semantic is slightly different depending on the direction.
        int index; // the index of the animation in timeOffset
        int timeOffset; // time offset when the animation at index starts.
    };

    int animationActualTotalDuration(int index) const;
    AnimationIndex indexForCurrentTime() const;

    void setCurrentAnimation(int index, bool intermediate = false);
    void activateCurrentAnimation(bool intermediate = false);

    void animationInsertedAt(int index);
    void animationRemoved(int index, QAbstractAnimation *anim);

    bool atEnd() const;

    QAbstractAnimation *currentAnimation;
    int currentAnimationIndex;

    // this is the actual duration of uncontrolled animations
    // it helps seeking and even going forward
    QList<int> actualDuration;

    void restart();
    int lastLoop;

    // handle time changes
    void rewindForwards(const AnimationIndex &newAnimationIndex);
    void advanceForwards(const AnimationIndex &newAnimationIndex);

    // private slot
    void _q_uncontrolledAnimationFinished();
};

QT_END_NAMESPACE

#endif //QT_NO_ANIMATION

#endif //QSEQUENTIALANIMATIONGROUP_P_H
