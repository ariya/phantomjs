/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QANIMATION_H
#define QANIMATION_H

#include <QtCore/qeasingcurve.h>
#include <QtCore/qabstractanimation.h>
#include <QtCore/qvector.h>
#include <QtCore/qvariant.h>
#include <QtCore/qpair.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

#ifndef QT_NO_ANIMATION

class QVariantAnimationPrivate;
class Q_CORE_EXPORT QVariantAnimation : public QAbstractAnimation
{
    Q_OBJECT
    Q_PROPERTY(QVariant startValue READ startValue WRITE setStartValue)
    Q_PROPERTY(QVariant endValue READ endValue WRITE setEndValue)
    Q_PROPERTY(QVariant currentValue READ currentValue NOTIFY valueChanged)
    Q_PROPERTY(int duration READ duration WRITE setDuration)
    Q_PROPERTY(QEasingCurve easingCurve READ easingCurve WRITE setEasingCurve)

public:
    typedef QPair<qreal, QVariant> KeyValue;
    typedef QVector<KeyValue> KeyValues;

    QVariantAnimation(QObject *parent = 0);
    ~QVariantAnimation();

    QVariant startValue() const;
    void setStartValue(const QVariant &value);

    QVariant endValue() const;
    void setEndValue(const QVariant &value);

    QVariant keyValueAt(qreal step) const;
    void setKeyValueAt(qreal step, const QVariant &value);

    KeyValues keyValues() const;
    void setKeyValues(const KeyValues &values);

    QVariant currentValue() const;

    int duration() const;
    void setDuration(int msecs);

    QEasingCurve easingCurve() const;
    void setEasingCurve(const QEasingCurve &easing);

    typedef QVariant (*Interpolator)(const void *from, const void *to, qreal progress);

Q_SIGNALS:
    void valueChanged(const QVariant &value);

protected:
    QVariantAnimation(QVariantAnimationPrivate &dd, QObject *parent = 0);
    bool event(QEvent *event);

    void updateCurrentTime(int);
    void updateState(QAbstractAnimation::State newState, QAbstractAnimation::State oldState);

    virtual void updateCurrentValue(const QVariant &value) = 0;
    virtual QVariant interpolated(const QVariant &from, const QVariant &to, qreal progress) const;

private:
    template <typename T> friend void qRegisterAnimationInterpolator(QVariant (*func)(const T &, const T &, qreal));
    static void registerInterpolator(Interpolator func, int interpolationType);

    Q_DISABLE_COPY(QVariantAnimation)
    Q_DECLARE_PRIVATE(QVariantAnimation)
};

template <typename T>
void qRegisterAnimationInterpolator(QVariant (*func)(const T &from, const T &to, qreal progress)) {
    QVariantAnimation::registerInterpolator(reinterpret_cast<QVariantAnimation::Interpolator>(func), qMetaTypeId<T>());
}

#endif //QT_NO_ANIMATION

QT_END_NAMESPACE

QT_END_HEADER

#endif //QANIMATION_H
