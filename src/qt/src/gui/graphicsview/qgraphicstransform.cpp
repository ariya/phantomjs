/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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

/*!
    \class QGraphicsTransform
    \brief The QGraphicsTransform class is an abstract base class for building
    advanced transformations on QGraphicsItems.
    \since 4.6
    \ingroup graphicsview-api

    As an alternative to QGraphicsItem::transform, QGraphicsTransform lets you
    create and control advanced transformations that can be configured
    independently using specialized properties.

    QGraphicsItem allows you to assign any number of QGraphicsTransform
    instances to one QGraphicsItem. Each QGraphicsTransform is applied in
    order, one at a time, to the QGraphicsItem it's assigned to.

    QGraphicsTransform is particularly useful for animations. Whereas
    QGraphicsItem::setTransform() lets you assign any transform directly to an
    item, there is no direct way to interpolate between two different
    transformations (e.g., when transitioning between two states, each for
    which the item has a different arbitrary transform assigned). Using
    QGraphicsTransform you can interpolate the property values of each
    independent transformation. The resulting operation is then combined into a
    single transform which is applied to QGraphicsItem.

    Transformations are computed in true 3D space using QMatrix4x4.
    When the transformation is applied to a QGraphicsItem, it will be
    projected back to a 2D QTransform.  When multiple QGraphicsTransform
    objects are applied to a QGraphicsItem, all of the transformations
    are computed in true 3D space, with the projection back to 2D
    only occurring after the last QGraphicsTransform is applied.
    The exception to this is QGraphicsRotation, which projects back to
    2D after each rotation to preserve the perspective effect around
    the X and Y axes.

    If you want to create your own configurable transformation, you can create
    a subclass of QGraphicsTransform (or any or the existing subclasses), and
    reimplement the pure virtual applyTo() function, which takes a pointer to a
    QMatrix4x4. Each operation you would like to apply should be exposed as
    properties (e.g., customTransform->setVerticalShear(2.5)). Inside you
    reimplementation of applyTo(), you can modify the provided transform
    respectively.

    QGraphicsTransform can be used together with QGraphicsItem::setTransform(),
    QGraphicsItem::setRotation(), and QGraphicsItem::setScale().

    \sa QGraphicsItem::transform(), QGraphicsScale, QGraphicsRotation
*/

#include "qgraphicstransform.h"
#include "qgraphicsitem_p.h"
#include "qgraphicstransform_p.h"
#include <QDebug>
#include <QtCore/qmath.h>
#include <QtCore/qnumeric.h>

#ifndef QT_NO_GRAPHICSVIEW
QT_BEGIN_NAMESPACE
void QGraphicsTransformPrivate::setItem(QGraphicsItem *i)
{
    if (item == i)
        return;

    if (item) {
        Q_Q(QGraphicsTransform);
        QGraphicsItemPrivate *d_ptr = item->d_ptr.data();

        item->prepareGeometryChange();
        Q_ASSERT(d_ptr->transformData);
        d_ptr->transformData->graphicsTransforms.removeAll(q);
        d_ptr->dirtySceneTransform = 1;
        item = 0;
    }

    item = i;
}

void QGraphicsTransformPrivate::updateItem(QGraphicsItem *item)
{
    item->prepareGeometryChange();
    item->d_ptr->dirtySceneTransform = 1;
}

/*!
    Constructs a new QGraphicsTransform with the given \a parent.
*/
QGraphicsTransform::QGraphicsTransform(QObject *parent)
    : QObject(*new QGraphicsTransformPrivate, parent)
{
}

/*!
    Destroys the graphics transform.
*/
QGraphicsTransform::~QGraphicsTransform()
{
    Q_D(QGraphicsTransform);
    d->setItem(0);
}

/*!
    \internal
*/
QGraphicsTransform::QGraphicsTransform(QGraphicsTransformPrivate &p, QObject *parent)
    : QObject(p, parent)
{
}

/*!
    \fn void QGraphicsTransform::applyTo(QMatrix4x4 *matrix) const

    This pure virtual method has to be reimplemented in derived classes.

    It applies this transformation to \a matrix.

    \sa QGraphicsItem::transform(), QMatrix4x4::toTransform()
*/

/*!
    Notifies that this transform operation has changed its parameters in such a
    way that applyTo() will return a different result than before.

    When implementing you own custom graphics transform, you must call this
    function every time you change a parameter, to let QGraphicsItem know that
    its transformation needs to be updated.

    \sa applyTo()
*/
void QGraphicsTransform::update()
{
    Q_D(QGraphicsTransform);
    if (d->item)
        d->updateItem(d->item);
}

/*!
  \class QGraphicsScale
  \brief The QGraphicsScale class provides a scale transformation.
  \since 4.6

  QGraphicsScene provides certain parameters to help control how the scale
  should be applied.

  The origin is the point that the item is scaled from (i.e., it stays fixed
  relative to the parent as the rest of the item grows). By default the
  origin is QPointF(0, 0).

  The parameters xScale, yScale, and zScale describe the scale factors to
  apply in horizontal, vertical, and depth directions. They can take on any
  value, including 0 (to collapse the item to a point) or negative value.
  A negative xScale value will mirror the item horizontally. A negative yScale
  value will flip the item vertically. A negative zScale will flip the
  item end for end.

  \sa QGraphicsTransform, QGraphicsItem::setScale(), QTransform::scale()
*/

class QGraphicsScalePrivate : public QGraphicsTransformPrivate
{
public:
    QGraphicsScalePrivate()
        : xScale(1), yScale(1), zScale(1) {}
    QVector3D origin;
    qreal xScale;
    qreal yScale;
    qreal zScale;
};

/*!
    Constructs an empty QGraphicsScale object with the given \a parent.
*/
QGraphicsScale::QGraphicsScale(QObject *parent)
    : QGraphicsTransform(*new QGraphicsScalePrivate, parent)
{
}

/*!
    Destroys the graphics scale.
*/
QGraphicsScale::~QGraphicsScale()
{
}

/*!
    \property QGraphicsScale::origin
    \brief the origin of the scale in 3D space.

    All scaling will be done relative to this point (i.e., this point
    will stay fixed, relative to the parent, when the item is scaled).

    \sa xScale, yScale, zScale
*/
QVector3D QGraphicsScale::origin() const
{
    Q_D(const QGraphicsScale);
    return d->origin;
}
void QGraphicsScale::setOrigin(const QVector3D &point)
{
    Q_D(QGraphicsScale);
    if (d->origin == point)
        return;
    d->origin = point;
    update();
    emit originChanged();
}

/*!
    \property QGraphicsScale::xScale
    \brief the horizontal scale factor.

    The scale factor can be any real number; the default value is 1.0. If you
    set the factor to 0.0, the item will be collapsed to a single point. If you
    provide a negative value, the item will be mirrored horizontally around its
    origin.

    \sa yScale, zScale, origin
*/
qreal QGraphicsScale::xScale() const
{
    Q_D(const QGraphicsScale);
    return d->xScale;
}
void QGraphicsScale::setXScale(qreal scale)
{
    Q_D(QGraphicsScale);
    if (d->xScale == scale)
        return;
    d->xScale = scale;
    update();
    emit xScaleChanged();
    emit scaleChanged();
}

/*!
    \property QGraphicsScale::yScale
    \brief the vertical scale factor.

    The scale factor can be any real number; the default value is 1.0. If you
    set the factor to 0.0, the item will be collapsed to a single point. If you
    provide a negative value, the item will be flipped vertically around its
    origin.

    \sa xScale, zScale, origin
*/
qreal QGraphicsScale::yScale() const
{
    Q_D(const QGraphicsScale);
    return d->yScale;
}
void QGraphicsScale::setYScale(qreal scale)
{
    Q_D(QGraphicsScale);
    if (d->yScale == scale)
        return;
    d->yScale = scale;
    update();
    emit yScaleChanged();
    emit scaleChanged();
}

/*!
    \property QGraphicsScale::zScale
    \brief the depth scale factor.

    The scale factor can be any real number; the default value is 1.0. If you
    set the factor to 0.0, the item will be collapsed to a single point. If you
    provide a negative value, the item will be flipped end for end around its
    origin.

    \sa xScale, yScale, origin
*/
qreal QGraphicsScale::zScale() const
{
    Q_D(const QGraphicsScale);
    return d->zScale;
}
void QGraphicsScale::setZScale(qreal scale)
{
    Q_D(QGraphicsScale);
    if (d->zScale == scale)
        return;
    d->zScale = scale;
    update();
    emit zScaleChanged();
    emit scaleChanged();
}

/*!
    \reimp
*/
void QGraphicsScale::applyTo(QMatrix4x4 *matrix) const
{
    Q_D(const QGraphicsScale);
    matrix->translate(d->origin);
    matrix->scale(d->xScale, d->yScale, d->zScale);
    matrix->translate(-d->origin);
}

/*!
    \fn QGraphicsScale::originChanged()

    QGraphicsScale emits this signal when its origin changes.

    \sa QGraphicsScale::origin
*/

/*!
    \fn QGraphicsScale::xScaleChanged()
    \since 4.7

    This signal is emitted whenever the \l xScale property changes.
*/

/*!
    \fn QGraphicsScale::yScaleChanged()
    \since 4.7

    This signal is emitted whenever the \l yScale property changes.
*/

/*!
    \fn QGraphicsScale::zScaleChanged()
    \since 4.7

    This signal is emitted whenever the \l zScale property changes.
*/

/*!
    \fn QGraphicsScale::scaleChanged()

    This signal is emitted whenever the xScale, yScale, or zScale
    of the object changes.

    \sa QGraphicsScale::xScale, QGraphicsScale::yScale
    \sa QGraphicsScale::zScale
*/

/*!
    \class QGraphicsRotation
    \brief The QGraphicsRotation class provides a rotation transformation around
    a given axis.
    \since 4.6

    You can provide the desired axis by assigning a QVector3D to the axis property
    or by passing a member if Qt::Axis to the setAxis convenience function.
    By default the axis is (0, 0, 1) i.e., rotation around the Z axis.

    The angle property, which is provided by QGraphicsRotation, now
    describes the number of degrees to rotate around this axis.

    QGraphicsRotation provides certain parameters to help control how the
    rotation should be applied.

    The origin is the point that the item is rotated around (i.e., it stays
    fixed relative to the parent as the rest of the item is rotated). By
    default the origin is QPointF(0, 0).

    The angle property provides the number of degrees to rotate the item
    clockwise around the origin. This value also be negative, indicating a
    counter-clockwise rotation. For animation purposes it may also be useful to
    provide rotation angles exceeding (-360, 360) degrees, for instance to
    animate how an item rotates several times.

    Note: the final rotation is the combined effect of a rotation in
    3D space followed by a projection back to 2D.  If several rotations
    are performed in succession, they will not behave as expected unless
    they were all around the Z axis.

    \sa QGraphicsTransform, QGraphicsItem::setRotation(), QTransform::rotate()
*/

class QGraphicsRotationPrivate : public QGraphicsTransformPrivate
{
public:
    QGraphicsRotationPrivate()
        : angle(0), axis(0, 0, 1) {}
    QVector3D origin;
    qreal angle;
    QVector3D axis;
};

/*!
    Constructs a new QGraphicsRotation with the given \a parent.
*/
QGraphicsRotation::QGraphicsRotation(QObject *parent)
    : QGraphicsTransform(*new QGraphicsRotationPrivate, parent)
{
}

/*!
    Destroys the graphics rotation.
*/
QGraphicsRotation::~QGraphicsRotation()
{
}

/*!
    \property QGraphicsRotation::origin
    \brief the origin of the rotation in 3D space.

    All rotations will be done relative to this point (i.e., this point
    will stay fixed, relative to the parent, when the item is rotated).

    \sa angle
*/
QVector3D QGraphicsRotation::origin() const
{
    Q_D(const QGraphicsRotation);
    return d->origin;
}
void QGraphicsRotation::setOrigin(const QVector3D &point)
{
    Q_D(QGraphicsRotation);
    if (d->origin == point)
        return;
    d->origin = point;
    update();
    emit originChanged();
}

/*!
    \property QGraphicsRotation::angle
    \brief the angle for clockwise rotation, in degrees.

    The angle can be any real number; the default value is 0.0. A value of 180
    will rotate 180 degrees, clockwise. If you provide a negative number, the
    item will be rotated counter-clockwise. Normally the rotation angle will be
    in the range (-360, 360), but you can also provide numbers outside of this
    range (e.g., a angle of 370 degrees gives the same result as 10 degrees).
    Setting the angle to NaN results in no rotation.

    \sa origin
*/
qreal QGraphicsRotation::angle() const
{
    Q_D(const QGraphicsRotation);
    return d->angle;
}
void QGraphicsRotation::setAngle(qreal angle)
{
    Q_D(QGraphicsRotation);
    if (d->angle == angle)
        return;
    d->angle = angle;
    update();
    emit angleChanged();
}

/*!
    \fn QGraphicsRotation::originChanged()

    This signal is emitted whenever the origin has changed.

    \sa QGraphicsRotation::origin
*/

/*!
    \fn void QGraphicsRotation::angleChanged()

    This signal is emitted whenever the angle has changed.

    \sa QGraphicsRotation::angle
*/

/*!
    \property QGraphicsRotation::axis
    \brief a rotation axis, specified by a vector in 3D space.

    This can be any axis in 3D space. By default the axis is (0, 0, 1),
    which is aligned with the Z axis. If you provide another axis,
    QGraphicsRotation will provide a transformation that rotates
    around this axis. For example, if you would like to rotate an item
    around its X axis, you could pass (1, 0, 0) as the axis.

    \sa QTransform, QGraphicsRotation::angle
*/
QVector3D QGraphicsRotation::axis() const
{
    Q_D(const QGraphicsRotation);
    return d->axis;
}
void QGraphicsRotation::setAxis(const QVector3D &axis)
{
    Q_D(QGraphicsRotation);
    if (d->axis == axis)
         return;
    d->axis = axis;
    update();
    emit axisChanged();
}

/*!
    \fn void QGraphicsRotation::setAxis(Qt::Axis axis)

    Convenience function to set the axis to \a axis.

    Note: the Qt::YAxis rotation for QTransform is inverted from the
    correct mathematical rotation in 3D space.  The QGraphicsRotation
    class implements a correct mathematical rotation.  The following
    two sequences of code will perform the same transformation:

    \code
    QTransform t;
    t.rotate(45, Qt::YAxis);

    QGraphicsRotation r;
    r.setAxis(Qt::YAxis);
    r.setAngle(-45);
    \endcode
*/
void QGraphicsRotation::setAxis(Qt::Axis axis)
{
    switch (axis)
    {
    case Qt::XAxis:
        setAxis(QVector3D(1, 0, 0));
        break;
    case Qt::YAxis:
        setAxis(QVector3D(0, 1, 0));
        break;
    case Qt::ZAxis:
        setAxis(QVector3D(0, 0, 1));
        break;
    }
}

/*!
    \reimp
*/
void QGraphicsRotation::applyTo(QMatrix4x4 *matrix) const
{
    Q_D(const QGraphicsRotation);

    if (d->angle == 0. || d->axis.isNull() || qIsNaN(d->angle))
        return;

    matrix->translate(d->origin);
    matrix->projectedRotate(d->angle, d->axis.x(), d->axis.y(), d->axis.z());
    matrix->translate(-d->origin);
}

/*!
    \fn void QGraphicsRotation::axisChanged()

    This signal is emitted whenever the axis of the object changes.

    \sa QGraphicsRotation::axis
*/

#include "moc_qgraphicstransform.cpp"

QT_END_NAMESPACE
#endif //QT_NO_GRAPHICSVIEW
