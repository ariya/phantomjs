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

/*!
    \class QPropertyAnimation
    \brief The QPropertyAnimation class animates Qt properties
    \since 4.6

    \ingroup animation

    QPropertyAnimation interpolates over \l{Qt's Property System}{Qt
    properties}. As property values are stored in \l{QVariant}s, the
    class inherits QVariantAnimation, and supports animation of the
    same \l{QVariant::Type}{variant types} as its super class.

    A class declaring properties must be a QObject. To make it
    possible to animate a property, it must provide a setter (so that
    QPropertyAnimation can set the property's value). Note that this
    makes it possible to animate many of Qt's widgets. Let's look at
    an example:

    \code
        QPropertyAnimation *animation = new QPropertyAnimation(myWidget, "geometry");
        animation->setDuration(10000);
        animation->setStartValue(QRect(0, 0, 100, 30));
        animation->setEndValue(QRect(250, 250, 100, 30));

        animation->start();
    \endcode

    The property name and the QObject instance of which property
    should be animated are passed to the constructor. You can then
    specify the start and end value of the property. The procedure is
    equal for properties in classes you have implemented
    yourself--just check with QVariantAnimation that your QVariant
    type is supported.

    The QVariantAnimation class description explains how to set up the
    animation in detail. Note, however, that if a start value is not
    set, the property will start at the value it had when the
    QPropertyAnimation instance was created.

    QPropertyAnimation works like a charm on its own. For complex
    animations that, for instance, contain several objects,
    QAnimationGroup is provided. An animation group is an animation
    that can contain other animations, and that can manage when its
    animations are played. Look at QParallelAnimationGroup for an
    example.

    \sa QVariantAnimation, QAnimationGroup, {The Animation Framework}
*/

#include "qpropertyanimation.h"
#include "qanimationgroup.h"
#include "qpropertyanimation_p.h"

#include <private/qmutexpool_p.h>

#ifndef QT_NO_ANIMATION

QT_BEGIN_NAMESPACE

void QPropertyAnimationPrivate::updateMetaProperty()
{
    if (!target || propertyName.isEmpty()) {
        propertyType = QVariant::Invalid;
        propertyIndex = -1;
        return;
    }

    //propertyType will be set to a valid type only if there is a Q_PROPERTY
    //otherwise it will be set to QVariant::Invalid at the end of this function
    propertyType = targetValue->property(propertyName).userType();
    propertyIndex = targetValue->metaObject()->indexOfProperty(propertyName);

    if (propertyType != QVariant::Invalid)
        convertValues(propertyType);
    if (propertyIndex == -1) {
        //there is no Q_PROPERTY on the object
        propertyType = QVariant::Invalid;
        if (!targetValue->dynamicPropertyNames().contains(propertyName))
            qWarning("QPropertyAnimation: you're trying to animate a non-existing property %s of your QObject", propertyName.constData());
    } else if (!targetValue->metaObject()->property(propertyIndex).isWritable()) {
        qWarning("QPropertyAnimation: you're trying to animate the non-writable property %s of your QObject", propertyName.constData());
	}
}

void QPropertyAnimationPrivate::updateProperty(const QVariant &newValue)
{
    if (state == QAbstractAnimation::Stopped)
        return;

    if (!target) {
        q_func()->stop(); //the target was destroyed we need to stop the animation
        return;
    }

    if (newValue.userType() == propertyType) {
        //no conversion is needed, we directly call the QMetaObject::metacall
        void *data = const_cast<void*>(newValue.constData());
        QMetaObject::metacall(targetValue, QMetaObject::WriteProperty, propertyIndex, &data);
    } else {
        targetValue->setProperty(propertyName.constData(), newValue);
    }
}

/*!
    Construct a QPropertyAnimation object. \a parent is passed to QObject's
    constructor.
*/
QPropertyAnimation::QPropertyAnimation(QObject *parent)
    : QVariantAnimation(*new QPropertyAnimationPrivate, parent)
{
}

/*!
    Construct a QPropertyAnimation object. \a parent is passed to QObject's
    constructor. The animation changes the property \a propertyName on \a
    target. The default duration is 250ms.

    \sa targetObject, propertyName
*/
QPropertyAnimation::QPropertyAnimation(QObject *target, const QByteArray &propertyName, QObject *parent)
    : QVariantAnimation(*new QPropertyAnimationPrivate, parent)
{
    setTargetObject(target);
    setPropertyName(propertyName);
}

/*!
    Destroys the QPropertyAnimation instance.
 */
QPropertyAnimation::~QPropertyAnimation()
{
    stop();
}

/*!
    \property QPropertyAnimation::targetObject
    \brief the target QObject for this animation.

    This property defines the target QObject for this animation.
 */
QObject *QPropertyAnimation::targetObject() const
{
    return d_func()->target.data();
}

void QPropertyAnimation::setTargetObject(QObject *target)
{
    Q_D(QPropertyAnimation);
    if (d->targetValue == target)
        return;

    if (d->state != QAbstractAnimation::Stopped) {
        qWarning("QPropertyAnimation::setTargetObject: you can't change the target of a running animation");
        return;
    }

    d->target = d->targetValue = target;
    d->updateMetaProperty();
}

/*!
    \property QPropertyAnimation::propertyName
    \brief the target property name for this animation

    This property defines the target property name for this animation. The
    property name is required for the animation to operate.
 */
QByteArray QPropertyAnimation::propertyName() const
{
    Q_D(const QPropertyAnimation);
    return d->propertyName;
}

void QPropertyAnimation::setPropertyName(const QByteArray &propertyName)
{
    Q_D(QPropertyAnimation);
    if (d->state != QAbstractAnimation::Stopped) {
        qWarning("QPropertyAnimation::setPropertyName: you can't change the property name of a running animation");
        return;
    }

    d->propertyName = propertyName;
    d->updateMetaProperty();
}


/*!
    \reimp
 */
bool QPropertyAnimation::event(QEvent *event)
{
    return QVariantAnimation::event(event);
}

/*!
    This virtual function is called by QVariantAnimation whenever the current value
    changes. \a value is the new, updated value. It updates the current value
    of the property on the target object.

    \sa currentValue, currentTime
 */
void QPropertyAnimation::updateCurrentValue(const QVariant &value)
{
    Q_D(QPropertyAnimation);
    d->updateProperty(value);
}

/*!
    \reimp

    If the startValue is not defined when the state of the animation changes from Stopped to Running,
    the current property value is used as the initial value for the animation.
*/
void QPropertyAnimation::updateState(QAbstractAnimation::State newState,
                                     QAbstractAnimation::State oldState)
{
    Q_D(QPropertyAnimation);

    if (!d->target && oldState == Stopped) {
        qWarning("QPropertyAnimation::updateState (%s): Changing state of an animation without target",
                 d->propertyName.constData());
        return;
    }

    QVariantAnimation::updateState(newState, oldState);

    QPropertyAnimation *animToStop = 0;
    {
#ifndef QT_NO_THREAD
        QMutexLocker locker(QMutexPool::globalInstanceGet(&staticMetaObject));
#endif
        typedef QPair<QObject *, QByteArray> QPropertyAnimationPair;
        typedef QHash<QPropertyAnimationPair, QPropertyAnimation*> QPropertyAnimationHash;
        static QPropertyAnimationHash hash;
        //here we need to use value because we need to know to which pointer
        //the animation was referring in case stopped because the target was destroyed
        QPropertyAnimationPair key(d->targetValue, d->propertyName);
        if (newState == Running) {
            d->updateMetaProperty();
            animToStop = hash.value(key, 0);
            hash.insert(key, this);
            // update the default start value
            if (oldState == Stopped) {
                d->setDefaultStartEndValue(d->targetValue->property(d->propertyName.constData()));
                //let's check if we have a start value and an end value
                if (!startValue().isValid() && (d->direction == Backward || !d->defaultStartEndValue.isValid())) {
                    qWarning("QPropertyAnimation::updateState (%s, %s, %s): starting an animation without start value",
                             d->propertyName.constData(), d->target.data()->metaObject()->className(),
                             qPrintable(d->target.data()->objectName()));
                }
                if (!endValue().isValid() && (d->direction == Forward || !d->defaultStartEndValue.isValid())) {
                    qWarning("QPropertyAnimation::updateState (%s, %s, %s): starting an animation without end value",
                             d->propertyName.constData(), d->target.data()->metaObject()->className(),
                             qPrintable(d->target.data()->objectName()));
                }
            }
        } else if (hash.value(key) == this) {
            hash.remove(key);
        }
    }

    //we need to do that after the mutex was unlocked
    if (animToStop) {
        // try to stop the top level group
        QAbstractAnimation *current = animToStop;
        while (current->group() && current->state() != Stopped)
            current = current->group();
        current->stop();
    }
}

#include "moc_qpropertyanimation.cpp"

QT_END_NAMESPACE

#endif //QT_NO_ANIMATION
