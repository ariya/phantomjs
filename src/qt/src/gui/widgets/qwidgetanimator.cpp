/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include <QtCore/qpropertyanimation.h>
#include <QtGui/qwidget.h>
#include <QtGui/private/qmainwindowlayout_p.h>

#include "qwidgetanimator_p.h"

QT_BEGIN_NAMESPACE

QWidgetAnimator::QWidgetAnimator(QMainWindowLayout *layout) : m_mainWindowLayout(layout)
{
}

void QWidgetAnimator::abort(QWidget *w)
{
#ifndef QT_NO_ANIMATION
    AnimationMap::iterator it = m_animation_map.find(w);
    if (it == m_animation_map.end())
        return;
    QPropertyAnimation *anim = *it;
    m_animation_map.erase(it);
    anim->stop();
#ifndef QT_NO_MAINWINDOW
    m_mainWindowLayout->animationFinished(w);
#endif
#else
    Q_UNUSED(w); //there is no animation to abort
#endif //QT_NO_ANIMATION
}

#ifndef QT_NO_ANIMATION
void QWidgetAnimator::animationFinished()
{
    QPropertyAnimation *anim = qobject_cast<QPropertyAnimation*>(sender());
    abort(static_cast<QWidget*>(anim->targetObject()));
}
#endif //QT_NO_ANIMATION

void QWidgetAnimator::animate(QWidget *widget, const QRect &_final_geometry, bool animate)
{
    QRect r = widget->geometry();
    if (r.right() < 0 || r.bottom() < 0)
        r = QRect();

    animate = animate && !r.isNull() && !_final_geometry.isNull();

    // might make the wigdet go away by sending it to negative space
    const QRect final_geometry = _final_geometry.isValid() || widget->isWindow() ? _final_geometry :
        QRect(QPoint(-500 - widget->width(), -500 - widget->height()), widget->size());

#ifndef QT_NO_ANIMATION
    AnimationMap::const_iterator it = m_animation_map.constFind(widget);
    if (it != m_animation_map.constEnd() && (*it)->endValue().toRect() == final_geometry)
        return;

    QPropertyAnimation *anim = new QPropertyAnimation(widget, "geometry", widget);
    anim->setDuration(animate ? 200 : 0);
    anim->setEasingCurve(QEasingCurve::InOutQuad);
    anim->setEndValue(final_geometry);
    m_animation_map[widget] = anim;
    connect(anim, SIGNAL(finished()), SLOT(animationFinished()));
    anim->start(QPropertyAnimation::DeleteWhenStopped);
#else
    //we do it in one shot
    widget->setGeometry(final_geometry);
#ifndef QT_NO_MAINWINDOW
    m_mainWindowLayout->animationFinished(widget);
#endif //QT_NO_MAINWINDOW
#endif //QT_NO_ANIMATION
}

bool QWidgetAnimator::animating() const
{
    return !m_animation_map.isEmpty();
}

QT_END_NAMESPACE
