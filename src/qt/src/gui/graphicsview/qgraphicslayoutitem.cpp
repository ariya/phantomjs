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

#include "qglobal.h"

#ifndef QT_NO_GRAPHICSVIEW

#include "qgraphicslayout.h"
#include "qgraphicsscene.h"
#include "qgraphicslayoutitem.h"
#include "qgraphicslayoutitem_p.h"
#include "qwidget.h"
#include "qgraphicswidget.h"

#include <QtDebug>

QT_BEGIN_NAMESPACE

/*
    COMBINE_SIZE() is identical to combineSize(), except that it
    doesn't evaluate 'size' unless necessary.
*/
#define COMBINE_SIZE(result, size) \
    do { \
        if ((result).width() < 0 || (result).height() < 0) \
            combineSize((result), (size)); \
    } while (false)

static void combineSize(QSizeF &result, const QSizeF &size)
{
    if (result.width() < 0)
        result.setWidth(size.width());
    if (result.height() < 0)
        result.setHeight(size.height());
}

static void boundSize(QSizeF &result, const QSizeF &size)
{
    if (size.width() >= 0 && size.width() < result.width())
        result.setWidth(size.width());
    if (size.height() >= 0 && size.height() < result.height())
        result.setHeight(size.height());
}

static void expandSize(QSizeF &result, const QSizeF &size)
{
    if (size.width() >= 0 && size.width() > result.width())
        result.setWidth(size.width());
    if (size.height() >= 0 && size.height() > result.height())
        result.setHeight(size.height());
}

static void normalizeHints(qreal &minimum, qreal &preferred, qreal &maximum, qreal &descent)
{
    if (minimum >= 0 && maximum >= 0 && minimum > maximum)
        minimum = maximum;

    if (preferred >= 0) {
        if (minimum >= 0 && preferred < minimum) {
            preferred = minimum;
        } else if (maximum >= 0 && preferred > maximum) {
            preferred = maximum;
        }
    }

    if (minimum >= 0 && descent > minimum)
        descent = minimum;
}

/*!
    \internal
*/
QGraphicsLayoutItemPrivate::QGraphicsLayoutItemPrivate(QGraphicsLayoutItem *par, bool layout)
    : parent(par), userSizeHints(0), isLayout(layout), ownedByLayout(false), graphicsItem(0)
{
}

/*!
    \internal
*/
QGraphicsLayoutItemPrivate::~QGraphicsLayoutItemPrivate()
{
    // Remove any lazily allocated data
    delete[] userSizeHints;
}

/*!
    \internal
*/
void QGraphicsLayoutItemPrivate::init()
{
    sizeHintCacheDirty = true;
    sizeHintWithConstraintCacheDirty = true;
}

/*!
    \internal

    effectiveSizeHint has a quirky behavior, one of the quirkinesses is when the hfw function is
    combined with user-specified min/max sizes. The input to hfw function (e.g width) must be within
    the min/max width constraint, and the output must be within the min/max height. This sets up a
    loose dependency between minimum width and maximum height (or minimum height, depending on the
    type of hfw function). Note that its only the concrete subclass that implements that hfw
    function that knows if this dependency means that the height will increase or decrease when the
    width is increased.

    The application should try to ensure that the user-defined sizes are within the range so that
    they don't conflict with the hfw function.

    Suppose, for instance that the hfw function is:

        height = 2000/width

    and the item has these user-defined sizes:

        min  ( 5,  5)
        pref(100, 10)
        max (500,100)

    what is the return value if one calls item->effectiveSizeHint(Qt::MinimumSize, QSizeF(10, -1)); ?
    The sizeHint() function would return QSizeF(10, 200), but it would be bounded down to 100 due
    to the max value, so it would return (10, 100). This is not what the item expects, since it
    really wants that its hfw is respected. If this is a label with wrapped text, this would most
    likely lead to that some text is clipped. This is certainly not what the app developer wants.
    Now, it would be better if the user changed those constraints to match the hfw function:

        min ( 20,  5)
        pref(100, 10)
        max (500,100)

    here, it says that the width cannot be smaller than 20. This is because if it becomes smaller
    than 20 the result of the hfw function would violate the max height (100).

    However, there is a similar problem if the width passed to the hfw function reaches *max* width:

    the sizeHint() function would now return QSizeF(500, 4), but 4 is smaller than the minimum
    height (5), so effectiveSizeHint() would return (500, 5), which would leave too much space.
    In this case, setting the max width to 400 fixes the problem:

        min ( 20,  5)
        pref(100, 10)
        max (400,100)


    The implementor of a hfw widget must be aware of this when sizeHint() is reimplemented, so that
    the default min and max sizes works sensible. (unfortunately the implementor does not have the
    control over user-set values).

*/
QSizeF *QGraphicsLayoutItemPrivate::effectiveSizeHints(const QSizeF &constraint) const
{
    Q_Q(const QGraphicsLayoutItem);
    QSizeF *sizeHintCache;
    const bool hasConstraint = constraint.width() >= 0 || constraint.height() >= 0;
    QSizeF adjustedConstraint = constraint;
    if (hasConstraint) {
        if (!sizeHintWithConstraintCacheDirty && constraint == cachedConstraint)
            return cachedSizeHintsWithConstraints;

        const QSizeF *hintsWithoutConstraint = effectiveSizeHints(QSizeF(-1,-1));

        if (adjustedConstraint.width() >= 0)
            adjustedConstraint.setWidth( qBound( hintsWithoutConstraint[Qt::MinimumSize].width(),
                                                 adjustedConstraint.width(),
                                                 hintsWithoutConstraint[Qt::MaximumSize].width()));
        if (adjustedConstraint.height() >= 0)
            adjustedConstraint.setHeight( qBound( hintsWithoutConstraint[Qt::MinimumSize].height(),
                                                  adjustedConstraint.height(),
                                                  hintsWithoutConstraint[Qt::MaximumSize].height()));

        if (!sizeHintWithConstraintCacheDirty && adjustedConstraint == cachedConstraint)
            return cachedSizeHintsWithConstraints;
        sizeHintCache = cachedSizeHintsWithConstraints;
    } else {
        if (!sizeHintCacheDirty)
            return cachedSizeHints;
        sizeHintCache = cachedSizeHints;
    }

    for (int i = 0; i < Qt::NSizeHints; ++i) {
        sizeHintCache[i] = adjustedConstraint;
        if (userSizeHints)
            combineSize(sizeHintCache[i], userSizeHints[i]);
    }

    QSizeF &minS = sizeHintCache[Qt::MinimumSize];
    QSizeF &prefS = sizeHintCache[Qt::PreferredSize];
    QSizeF &maxS = sizeHintCache[Qt::MaximumSize];
    QSizeF &descentS = sizeHintCache[Qt::MinimumDescent];

    normalizeHints(minS.rwidth(), prefS.rwidth(), maxS.rwidth(), descentS.rwidth());
    normalizeHints(minS.rheight(), prefS.rheight(), maxS.rheight(), descentS.rheight());

    // if the minimum, preferred and maximum sizes contradict each other
    // (e.g. the minimum is larger than the maximum) we give priority to
    // the maximum size, then the minimum size and finally the preferred size
    COMBINE_SIZE(maxS, q->sizeHint(Qt::MaximumSize, maxS));
    combineSize(maxS, QSizeF(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
    expandSize(maxS, prefS);
    expandSize(maxS, minS);
    boundSize(maxS, QSizeF(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));

    COMBINE_SIZE(minS, q->sizeHint(Qt::MinimumSize, minS));
    expandSize(minS, QSizeF(0, 0));
    boundSize(minS, prefS);
    boundSize(minS, maxS);

    COMBINE_SIZE(prefS, q->sizeHint(Qt::PreferredSize, prefS));
    expandSize(prefS, minS);
    boundSize(prefS, maxS);

    // Not supported yet
    // COMBINE_SIZE(descentS, q->sizeHint(Qt::MinimumDescent, constraint));

    if (hasConstraint) {
        cachedConstraint = adjustedConstraint;
        sizeHintWithConstraintCacheDirty = false;
    } else {
        sizeHintCacheDirty = false;
    }
    return sizeHintCache;
}


/*!
    \internal

    Returns the parent item of this layout, or 0 if this layout is
    not installed on any widget.

    If this is the item that the layout is installed on, it will return "itself".

    If the layout is a sub-layout, this function returns the parent
    widget of the parent layout.

    Note that it will traverse up the layout item hierarchy instead of just calling
    QGraphicsItem::parentItem(). This is on purpose.

    \sa parent()
*/
QGraphicsItem *QGraphicsLayoutItemPrivate::parentItem() const
{
    Q_Q(const QGraphicsLayoutItem);

    const QGraphicsLayoutItem *parent = q;
    while (parent && parent->isLayout()) {
        parent = parent->parentLayoutItem();
    }
    return parent ? parent->graphicsItem() : 0;
}

/*!
    \internal

     Ensures that userSizeHints is allocated.
     This function must be called before any dereferencing.
*/
void QGraphicsLayoutItemPrivate::ensureUserSizeHints()
{
    if (!userSizeHints)
        userSizeHints = new QSizeF[Qt::NSizeHints];
}

/*!
    \internal

    Sets the user size hint \a which to \a size. Use an invalid size to unset the size hint.
 */
void QGraphicsLayoutItemPrivate::setSize(Qt::SizeHint which, const QSizeF &size)
{
    Q_Q(QGraphicsLayoutItem);

    if (userSizeHints) {
        if (size == userSizeHints[which])
            return;
    } else if (size.width() < 0 && size.height() < 0) {
        return;
    }

    ensureUserSizeHints();
    userSizeHints[which] = size;
    q->updateGeometry();
}

/*!
    \internal

    Sets the width of the user size hint \a which to \a width.
 */
void QGraphicsLayoutItemPrivate::setSizeComponent(
    Qt::SizeHint which, SizeComponent component, qreal value)
{
    Q_Q(QGraphicsLayoutItem);
    ensureUserSizeHints();
    qreal &userValue = (component == Width)
        ? userSizeHints[which].rwidth()
        : userSizeHints[which].rheight();
    if (value == userValue)
        return;
    userValue = value;
    q->updateGeometry();
}


bool QGraphicsLayoutItemPrivate::hasHeightForWidth() const
{
    Q_Q(const QGraphicsLayoutItem);
    if (isLayout) {
        const QGraphicsLayout *l = static_cast<const QGraphicsLayout *>(q);
        for (int i = l->count() - 1; i >= 0; --i) {
            if (QGraphicsLayoutItemPrivate::get(l->itemAt(i))->hasHeightForWidth())
                return true;
        }
    } else if (QGraphicsItem *item = q->graphicsItem()) {
        if (item->isWidget()) {
            QGraphicsWidget *w = static_cast<QGraphicsWidget *>(item);
            if (w->layout()) {
                return QGraphicsLayoutItemPrivate::get(w->layout())->hasHeightForWidth();
            }
        }
    }
    return q->sizePolicy().hasHeightForWidth();
}

bool QGraphicsLayoutItemPrivate::hasWidthForHeight() const
{
    Q_Q(const QGraphicsLayoutItem);
    if (isLayout) {
        const QGraphicsLayout *l = static_cast<const QGraphicsLayout *>(q);
        for (int i = l->count() - 1; i >= 0; --i) {
            if (QGraphicsLayoutItemPrivate::get(l->itemAt(i))->hasWidthForHeight())
                return true;
        }
    } else if (QGraphicsItem *item = q->graphicsItem()) {
        if (item->isWidget()) {
            QGraphicsWidget *w = static_cast<QGraphicsWidget *>(item);
            if (w->layout()) {
                return QGraphicsLayoutItemPrivate::get(w->layout())->hasWidthForHeight();
            }
        }
    }
    return q->sizePolicy().hasWidthForHeight();
}

/*!
    \class QGraphicsLayoutItem
    \brief The QGraphicsLayoutItem class can be inherited to allow your custom
    items to be managed by layouts.
    \since 4.4
    \ingroup graphicsview-api

    QGraphicsLayoutItem is an abstract class that defines a set of virtual
    functions describing sizes, size policies, and size hints for any object
    arranged by QGraphicsLayout. The API contains functions relevant
    for both the item itself and for the user of the item as most of
    QGraphicsLayoutItem's functions are also part of the subclass' public API.

    In most cases, existing layout-aware classes such as QGraphicsWidget and
    QGraphicsLayout already provide the functionality you require. However,
    subclassing these classes will enable you to create both graphical
    elements that work well with layouts (QGraphicsWidget) or custom layouts
    (QGraphicsLayout).

    \section1 Subclassing QGraphicsLayoutItem

    If you create a subclass of QGraphicsLayoutItem and reimplement its
    virtual functions, you will enable the layout to resize and position your
    item along with other QGraphicsLayoutItems including QGraphicsWidget
    and QGraphicsLayout.

    You can start by reimplementing important functions: the protected
    sizeHint() function, as well as the public setGeometry()
    function. If you want your items to be aware of immediate geometry
    changes, you can also reimplement updateGeometry().

    The geometry, size hint, and size policy affect the item's size and
    position. Calling setGeometry() will always resize and reposition the item
    immediately. Normally, this function is called by QGraphicsLayout after
    the layout has been activated, but it can also be called by the item's user
    at any time.

    The sizeHint() function returns the item' minimum, preferred and maximum
    size hints. You can override these properties by calling setMinimumSize(),
    setPreferredSize() or setMaximumSize(). You can also use functions such as
    setMinimumWidth() or setMaximumHeight() to set only the width or height
    component if desired.

    The effectiveSizeHint() function, on the other hand, returns a size hint
    for any given Qt::SizeHint, and guarantees that the returned size is bound
    to the minimum and maximum sizes and size hints. You can set the item's
    vertical and horizontal size policy by calling setSizePolicy(). The
    sizePolicy property is used by the layout system to describe how this item
    prefers to grow or shrink.

    \section1 Nesting QGraphicsLayoutItems

    QGraphicsLayoutItems can be nested within other QGraphicsLayoutItems,
    similar to layouts that can contain sublayouts. This is done either by
    passing a QGraphicsLayoutItem pointer to QGraphicsLayoutItem's
    protected constructor, or by calling setParentLayoutItem(). The
    parentLayoutItem() function returns a pointer to the item's layoutItem
    parent. If the item's parent is 0 or if the parent does not inherit
    from QGraphicsItem, the parentLayoutItem() function then returns 0.
    isLayout() returns true if the QGraphicsLayoutItem subclass is itself a
    layout, or false otherwise.

    Qt uses QGraphicsLayoutItem to provide layout functionality in the
    \l{Graphics View Framework}, but in the future its use may spread
    throughout Qt itself.

    \sa QGraphicsWidget, QGraphicsLayout, QGraphicsLinearLayout,
    QGraphicsGridLayout
*/

/*!
    Constructs the QGraphicsLayoutItem object. \a parent becomes the object's
    parent. If \a isLayout is true the item is a layout, otherwise
    \a isLayout is false.
*/
QGraphicsLayoutItem::QGraphicsLayoutItem(QGraphicsLayoutItem *parent, bool isLayout)
    : d_ptr(new QGraphicsLayoutItemPrivate(parent, isLayout))
{
    Q_D(QGraphicsLayoutItem);
    d->init();
    d->sizePolicy = QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    d->q_ptr = this;
}

/*!
    \internal
*/
QGraphicsLayoutItem::QGraphicsLayoutItem(QGraphicsLayoutItemPrivate &dd)
    : d_ptr(&dd)
{
    Q_D(QGraphicsLayoutItem);
    d->init();
    d->q_ptr = this;
}

/*!
    Destroys the QGraphicsLayoutItem object.
*/
QGraphicsLayoutItem::~QGraphicsLayoutItem()
{
    QGraphicsLayoutItem *parentLI = parentLayoutItem();
    if (parentLI && parentLI->isLayout()) {
        QGraphicsLayout *lay = static_cast<QGraphicsLayout*>(parentLI);
        // this is not optimal
        for (int i = lay->count() - 1; i >= 0; --i) {
            if (lay->itemAt(i) == this) {
                lay->removeAt(i);
                break;
            }
        }
    }
}

/*!
    \fn virtual QSizeF QGraphicsLayoutItem::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const = 0;

    This pure virtual function returns the size hint for \a which of the
    QGraphicsLayoutItem, using the width or height of \a constraint to
    constrain the output.

    Reimplement this function in a subclass of QGraphicsLayoutItem to
    provide the necessary size hints for your items.

    \sa effectiveSizeHint()
*/

/*!
    Sets the size policy to \a policy. The size policy describes how the item
    should grow horizontally and vertically when arranged in a layout.

    QGraphicsLayoutItem's default size policy is (QSizePolicy::Fixed,
    QSizePolicy::Fixed, QSizePolicy::DefaultType), but it is common for
    subclasses to change the default. For example, QGraphicsWidget defaults
    to (QSizePolicy::Preferred, QSizePolicy::Preferred,
    QSizePolicy::DefaultType).

    \sa sizePolicy(), QWidget::sizePolicy()
*/
void QGraphicsLayoutItem::setSizePolicy(const QSizePolicy &policy)
{
    Q_D(QGraphicsLayoutItem);
    if (d->sizePolicy == policy)
        return;
    d->sizePolicy = policy;
    updateGeometry();
}

/*!
    \overload

    This function is equivalent to calling
    setSizePolicy(QSizePolicy(\a hPolicy, \a vPolicy, \a controlType)).

    \sa sizePolicy(), QWidget::sizePolicy()
*/
void QGraphicsLayoutItem::setSizePolicy(QSizePolicy::Policy hPolicy,
                                        QSizePolicy::Policy vPolicy,
                                        QSizePolicy::ControlType controlType)
{
    setSizePolicy(QSizePolicy(hPolicy, vPolicy, controlType));
}

/*!
    Returns the current size policy.

    \sa setSizePolicy(), QWidget::sizePolicy()
*/
QSizePolicy QGraphicsLayoutItem::sizePolicy() const
{
    Q_D(const QGraphicsLayoutItem);
    return d->sizePolicy;
}

/*!
    Sets the minimum size to \a size. This property overrides sizeHint() for
    Qt::MinimumSize and ensures that effectiveSizeHint() will never return
    a size smaller than \a size. In order to unset the minimum size, use an
    invalid size.

    \sa minimumSize(), maximumSize(), preferredSize(), Qt::MinimumSize,
    sizeHint(), setMinimumWidth(), setMinimumHeight()
*/
void QGraphicsLayoutItem::setMinimumSize(const QSizeF &size)
{
    d_ptr->setSize(Qt::MinimumSize, size);
}

/*!
    \fn QGraphicsLayoutItem::setMinimumSize(qreal w, qreal h)

    This convenience function is equivalent to calling
    setMinimumSize(QSizeF(\a w, \a h)).

    \sa minimumSize(), setMaximumSize(), setPreferredSize(), sizeHint()
*/

/*!
    Returns the minimum size.

    \sa setMinimumSize(), preferredSize(), maximumSize(), Qt::MinimumSize,
    sizeHint()
*/
QSizeF QGraphicsLayoutItem::minimumSize() const
{
    return effectiveSizeHint(Qt::MinimumSize);
}

/*!
    Sets the minimum width to \a width.

    \sa minimumWidth(), setMinimumSize(), minimumSize()
*/
void QGraphicsLayoutItem::setMinimumWidth(qreal width)
{
    d_ptr->setSizeComponent(Qt::MinimumSize, d_ptr->Width, width);
}

/*!
    Sets the minimum height to \a height.

    \sa minimumHeight(), setMinimumSize(), minimumSize()
*/
void QGraphicsLayoutItem::setMinimumHeight(qreal height)
{
    d_ptr->setSizeComponent(Qt::MinimumSize, d_ptr->Height, height);
}


/*!
    Sets the preferred size to \a size. This property overrides sizeHint() for
    Qt::PreferredSize and provides the default value for effectiveSizeHint().
    In order to unset the preferred size, use an invalid size.

    \sa preferredSize(), minimumSize(), maximumSize(), Qt::PreferredSize,
    sizeHint()
*/
void QGraphicsLayoutItem::setPreferredSize(const QSizeF &size)
{
    d_ptr->setSize(Qt::PreferredSize, size);
}

/*!
    \fn QGraphicsLayoutItem::setPreferredSize(qreal w, qreal h)

    This convenience function is equivalent to calling
    setPreferredSize(QSizeF(\a w, \a h)).

    \sa preferredSize(), setMaximumSize(), setMinimumSize(), sizeHint()
*/

/*!
    Returns the preferred size.

    \sa setPreferredSize(), minimumSize(), maximumSize(), Qt::PreferredSize,
    sizeHint()
*/
QSizeF QGraphicsLayoutItem::preferredSize() const
{
    return effectiveSizeHint(Qt::PreferredSize);
}

/*!
    Sets the preferred height to \a height.

    \sa preferredWidth(), setPreferredSize(), preferredSize()
*/
void QGraphicsLayoutItem::setPreferredHeight(qreal height)
{
    d_ptr->setSizeComponent(Qt::PreferredSize, d_ptr->Height, height);
}

/*!
    Sets the preferred width to \a width.

    \sa preferredHeight(), setPreferredSize(), preferredSize()
*/
void QGraphicsLayoutItem::setPreferredWidth(qreal width)
{
    d_ptr->setSizeComponent(Qt::PreferredSize, d_ptr->Width, width);
}

/*!
    Sets the maximum size to \a size. This property overrides sizeHint() for
    Qt::MaximumSize and ensures that effectiveSizeHint() will never return a
    size larger than \a size. In order to unset the maximum size, use an
    invalid size.

    \sa maximumSize(), minimumSize(), preferredSize(), Qt::MaximumSize,
    sizeHint()
*/
void QGraphicsLayoutItem::setMaximumSize(const QSizeF &size)
{
    d_ptr->setSize(Qt::MaximumSize, size);
}

/*!
    \fn QGraphicsLayoutItem::setMaximumSize(qreal w, qreal h)

    This convenience function is equivalent to calling
    setMaximumSize(QSizeF(\a w, \a h)).

    \sa maximumSize(), setMinimumSize(), setPreferredSize(), sizeHint()
*/

/*!
    Returns the maximum size.

    \sa setMaximumSize(), minimumSize(), preferredSize(), Qt::MaximumSize,
    sizeHint()
*/
QSizeF QGraphicsLayoutItem::maximumSize() const
{
    return effectiveSizeHint(Qt::MaximumSize);
}

/*!
    Sets the maximum width to \a width.

    \sa maximumWidth(), setMaximumSize(), maximumSize()
*/
void QGraphicsLayoutItem::setMaximumWidth(qreal width)
{
    d_ptr->setSizeComponent(Qt::MaximumSize, d_ptr->Width, width);
}

/*!
    Sets the maximum height to \a height.

    \sa maximumHeight(), setMaximumSize(), maximumSize()
*/
void QGraphicsLayoutItem::setMaximumHeight(qreal height)
{
    d_ptr->setSizeComponent(Qt::MaximumSize, d_ptr->Height, height);
}

/*!
    \fn qreal QGraphicsLayoutItem::minimumWidth() const

    Returns the minimum width.

    \sa setMinimumWidth(), setMinimumSize(), minimumSize()
*/

/*!
    \fn qreal QGraphicsLayoutItem::minimumHeight() const

    Returns the minimum height.

    \sa setMinimumHeight(), setMinimumSize(), minimumSize()
*/

/*!
    \fn qreal QGraphicsLayoutItem::preferredWidth() const

    Returns the preferred width.

    \sa setPreferredWidth(), setPreferredSize(), preferredSize()
*/

/*!
    \fn qreal QGraphicsLayoutItem::preferredHeight() const

    Returns the preferred height.

    \sa setPreferredHeight(), setPreferredSize(), preferredSize()
*/

/*!
    \fn qreal QGraphicsLayoutItem::maximumWidth() const

    Returns the maximum width.

    \sa setMaximumWidth(), setMaximumSize(), maximumSize()
*/

/*!
    \fn qreal QGraphicsLayoutItem::maximumHeight() const

    Returns the maximum height.

    \sa setMaximumHeight(), setMaximumSize(), maximumSize()
*/

/*!
    \fn virtual void QGraphicsLayoutItem::setGeometry(const QRectF &rect)

    This virtual function sets the geometry of the QGraphicsLayoutItem to
    \a rect, which is in parent coordinates (e.g., the top-left corner of \a rect
    is equivalent to the item's position in parent coordinates).

    You must reimplement this function in a subclass of QGraphicsLayoutItem to
    receive geometry updates. The layout will call this function when it does a
    rearrangement.

    If \a rect is outside of the bounds of minimumSize and maximumSize, it
    will be adjusted to its closest size so that it is within the legal
    bounds.

    \sa geometry()
*/
void QGraphicsLayoutItem::setGeometry(const QRectF &rect)
{
    Q_D(QGraphicsLayoutItem);
    QSizeF effectiveSize = rect.size().expandedTo(effectiveSizeHint(Qt::MinimumSize))
                                .boundedTo(effectiveSizeHint(Qt::MaximumSize));
    d->geom = QRectF(rect.topLeft(), effectiveSize);
}

/*!
    \fn QRectF QGraphicsLayoutItem::geometry() const

    Returns the item's geometry (e.g., position and size) as a
    QRectF. This function is equivalent to QRectF(pos(), size()).

    \sa setGeometry()
*/
QRectF QGraphicsLayoutItem::geometry() const
{
    Q_D(const QGraphicsLayoutItem);
    return d->geom;
}

/*!
    This virtual function provides the \a left, \a top, \a right and \a bottom
    contents margins for this QGraphicsLayoutItem. The default implementation
    assumes all contents margins are 0. The parameters point to values stored
    in qreals. If any of the pointers is 0, that value will not be updated.

    \sa QGraphicsWidget::setContentsMargins()
*/
void QGraphicsLayoutItem::getContentsMargins(qreal *left, qreal *top, qreal *right, qreal *bottom) const
{
    if (left)
        *left = 0;
    if (top)
        *top = 0;
    if (right)
        *right = 0;
    if (bottom)
        *bottom = 0;
}

/*!
    Returns the contents rect in local coordinates.

    The contents rect defines the subrectangle used by an associated layout
    when arranging subitems. This function is a convenience function that
    adjusts the item's geometry() by its contents margins. Note that
    getContentsMargins() is a virtual function that you can reimplement to
    return the item's contents margins.

    \sa getContentsMargins(), geometry()
*/
QRectF QGraphicsLayoutItem::contentsRect() const
{
    qreal left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    return QRectF(QPointF(), geometry().size()).adjusted(+left, +top, -right, -bottom);
}

/*!
    Returns the effective size hint for this QGraphicsLayoutItem.

    \a which is the size hint in question.
    \a constraint is an optional argument that defines a special constrain
    when calculating the effective size hint. By default, \a constraint is
    QSizeF(-1, -1), which means there is no constraint to the size hint.

    If you want to specify the widget's size hint for a given width or height,
    you can provide the fixed dimension in \a constraint. This is useful for
    widgets that can grow only either vertically or horizontally, and need to
    set either their width or their height to a special value.

    For example, a text paragraph item fit into a column width of 200 may
    grow vertically. You can pass QSizeF(200, -1) as a constraint to get a
    suitable minimum, preferred and maximum height).

    You can adjust the effective size hint by reimplementing sizeHint()
    in a QGraphicsLayoutItem subclass, or by calling one of the following
    functions: setMinimumSize(), setPreferredSize, or setMaximumSize()
    (or a combination of both).

    This function caches each of the size hints and guarantees that
    sizeHint() will be called only once for each value of \a which - unless
    \a constraint is not specified and updateGeometry() has been called.

    \sa sizeHint()
*/
QSizeF QGraphicsLayoutItem::effectiveSizeHint(Qt::SizeHint which, const QSizeF &constraint) const
{
    Q_D(const QGraphicsLayoutItem);

    if (!d->userSizeHints && constraint.isValid())
        return constraint;

    // ### should respect size policy???
    return d_ptr->effectiveSizeHints(constraint)[which];
}

/*!
    This virtual function discards any cached size hint information. You
    should always call this function if you change the return value of the
    sizeHint() function. Subclasses must always call the base implementation
    when reimplementing this function.

    \sa effectiveSizeHint()
*/
void QGraphicsLayoutItem::updateGeometry()
{
    Q_D(QGraphicsLayoutItem);
    d->sizeHintCacheDirty = true;
    d->sizeHintWithConstraintCacheDirty = true;
}

/*!
    Returns the parent of this QGraphicsLayoutItem, or 0 if there is no parent,
    or if the parent does not inherit from QGraphicsLayoutItem
    (QGraphicsLayoutItem is often used through multiple inheritance with
    QObject-derived classes).

    \sa setParentLayoutItem()
*/
QGraphicsLayoutItem *QGraphicsLayoutItem::parentLayoutItem() const
{
    return d_func()->parent;
}

/*!
    Sets the parent of this QGraphicsLayoutItem to \a parent.

    \sa parentLayoutItem()
*/
void QGraphicsLayoutItem::setParentLayoutItem(QGraphicsLayoutItem *parent)
{
    d_func()->parent = parent;
}

/*!
    Returns true if this QGraphicsLayoutItem is a layout (e.g., is inherited
    by an object that arranges other QGraphicsLayoutItem objects); otherwise
    returns false.

    \sa QGraphicsLayout
*/
bool QGraphicsLayoutItem::isLayout() const
{
    return d_func()->isLayout;
}

/*!
    \since 4.6

    Returns whether a layout should delete this item in its destructor.
    If its true, then the layout will delete it. If its false, then it is
    assumed that another object has the ownership of it, and the layout won't
    delete this item.

    If the item inherits both QGraphicsItem and QGraphicsLayoutItem (such
    as QGraphicsWidget does) the item is really part of two ownership
    hierarchies. This property informs what the layout should do with its
    child items when it is destructed. In the case of QGraphicsWidget, it
    is preferred that when the layout is deleted it won't delete its children
    (since they are also part of the graphics item hierarchy).

    By default this value is initialized to false in QGraphicsLayoutItem,
    but it is overridden by QGraphicsLayout to return true. This is because
    QGraphicsLayout is not normally part of the QGraphicsItem hierarchy, so the
    parent layout should delete it.
    Subclasses might override this default behaviour by calling
    setOwnedByLayout(true).

    \sa setOwnedByLayout()
*/
bool QGraphicsLayoutItem::ownedByLayout() const
{
    return d_func()->ownedByLayout;
}
/*!
    \since 4.6

    Sets whether a layout should delete this item in its destructor or not.
    \a ownership must be true to in order for the layout to delete it.
    \sa ownedByLayout()
*/
void QGraphicsLayoutItem::setOwnedByLayout(bool ownership)
{
    d_func()->ownedByLayout = ownership;
}

/*!
 * Returns the QGraphicsItem that this layout item represents.
 * For QGraphicsWidget it will return itself. For custom items it can return an
 * aggregated value.
 *
 * \sa setGraphicsItem()
 */
QGraphicsItem *QGraphicsLayoutItem::graphicsItem() const
{
    return d_func()->graphicsItem;
}

/*!
 * If the QGraphicsLayoutItem represents a QGraphicsItem, and it wants to take
 * advantage of the automatic reparenting capabilities of QGraphicsLayout it
 * should set this value.
 * Note that if you delete \a item and not delete the layout item, you are
 * responsible of calling setGraphicsItem(0) in order to avoid having a
 * dangling pointer.
 *
 * \sa graphicsItem()
 */
void QGraphicsLayoutItem::setGraphicsItem(QGraphicsItem *item)
{
    d_func()->graphicsItem = item;
}

QT_END_NAMESPACE

#endif //QT_NO_GRAPHICSVIEW
