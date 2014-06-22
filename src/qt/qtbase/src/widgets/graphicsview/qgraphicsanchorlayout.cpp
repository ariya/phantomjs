/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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
    \class QGraphicsAnchorLayout
    \brief The QGraphicsAnchorLayout class provides a layout where one can anchor widgets
    together in Graphics View.
    \since 4.6
    \ingroup appearance
    \ingroup geomanagement
    \ingroup graphicsview-api
    \inmodule QtWidgets

    The anchor layout allows developers to specify how widgets should be placed relative to
    each other, and to the layout itself. The specification is made by adding anchors to the
    layout by calling addAnchor(), addAnchors() or addCornerAnchors().

    Existing anchors in the layout can be accessed with the anchor() function.
    Items that are anchored are automatically added to the layout, and if items
    are removed, all their anchors will be automatically removed.

    \div {class="float-left"}
    \inlineimage simpleanchorlayout-example.png Using an anchor layout to align simple colored widgets.
    \enddiv

    Anchors are always set up between edges of an item, where the "center" is also considered to
    be an edge. Consider the following example:

    \snippet graphicsview/simpleanchorlayout/main.cpp adding anchors

    Here, the right edge of item \c a is anchored to the left edge of item \c b and the bottom
    edge of item \c a is anchored to the top edge of item \c b, with the result that
    item \c b will be placed diagonally to the right and below item \c b.

    The addCornerAnchors() function provides a simpler way of anchoring the corners
    of two widgets than the two individual calls to addAnchor() shown in the code
    above. Here, we see how a widget can be anchored to the top-left corner of the enclosing
    layout:

    \snippet graphicsview/simpleanchorlayout/main.cpp adding a corner anchor

    In cases where anchors are used to match the widths or heights of widgets, it is
    convenient to use the addAnchors() function. As with the other functions for specifying
    anchors, it can also be used to anchor a widget to a layout.

    \section1 Size Hints and Size Policies in an Anchor Layout

    QGraphicsAnchorLayout respects each item's size hints and size policies.
    Note that there are some properties of QSizePolicy that are \l{Known issues}{not respected}.

    \section1 Spacing within an Anchor Layout

    The layout may distribute some space between the items. If the spacing has not been
    explicitly specified, the actual amount of space will usually be 0.

    However, if the first edge is the \e opposite of the second edge (e.g., the right edge
    of the first widget is anchored to the left edge of the second widget), the size of the
    anchor will be queried from the style through a pixel metric:
    \l{QStyle::}{PM_LayoutHorizontalSpacing} for horizontal anchors and
    \l{QStyle::}{PM_LayoutVerticalSpacing} for vertical anchors.

    If the spacing is negative, the items will overlap to some extent.


    \section1 Known issues
    There are some features that QGraphicsAnchorLayout currently does not support.
    This might change in the future, so avoid using these features if you want to
    avoid any future regressions in behaviour:
    \list

    \li Stretch factors are not respected.

    \li QSizePolicy::ExpandFlag is not respected.

    \li Height for width is not respected.

    \endlist

    \sa QGraphicsLinearLayout, QGraphicsGridLayout, QGraphicsLayout
*/

/*!
    \class QGraphicsAnchor
    \brief The QGraphicsAnchor class represents an anchor between two items in a
           QGraphicsAnchorLayout.
    \since 4.6
    \ingroup appearance
    \ingroup geomanagement
    \ingroup graphicsview-api
    \inmodule QtWidgets

    The graphics anchor provides an API that enables you to query and manipulate the
    properties an anchor has. When an anchor is added to the layout with
    QGraphicsAnchorLayout::addAnchor(), a QGraphicsAnchor instance is returned where the properties
    are initialized to their default values. The properties can then be further changed, and they
    will be picked up the next time the layout is activated.

    \sa QGraphicsAnchorLayout::anchor()

*/
#include "qgraphicsanchorlayout_p.h"
#ifndef QT_NO_GRAPHICSVIEW
QT_BEGIN_NAMESPACE

QGraphicsAnchor::QGraphicsAnchor(QGraphicsAnchorLayout *parentLayout)
    : QObject(*(new QGraphicsAnchorPrivate))
{
    Q_D(QGraphicsAnchor);
    Q_ASSERT(parentLayout);
    d->layoutPrivate = parentLayout->d_func();
}

/*!
    Removes the QGraphicsAnchor object from the layout and destroys it.
*/
QGraphicsAnchor::~QGraphicsAnchor()
{
}

/*!
    \property QGraphicsAnchor::sizePolicy
    \brief the size policy for the QGraphicsAnchor.

    By setting the size policy on an anchor you can configure how the anchor can resize itself
    from its preferred spacing. For instance, if the anchor has the size policy
    QSizePolicy::Minimum, the spacing is the minimum size of the anchor. However, its size
    can grow up to the anchors maximum size. If the default size policy is QSizePolicy::Fixed,
    the anchor can neither grow or shrink, which means that the only size the anchor can have
    is the spacing. QSizePolicy::Fixed is the default size policy.
    QGraphicsAnchor always has a minimum spacing of 0 and a very large maximum spacing.

    \sa QGraphicsAnchor::spacing
*/

void QGraphicsAnchor::setSizePolicy(QSizePolicy::Policy policy)
{
    Q_D(QGraphicsAnchor);
    d->setSizePolicy(policy);
}

QSizePolicy::Policy QGraphicsAnchor::sizePolicy() const
{
    Q_D(const QGraphicsAnchor);
    return d->sizePolicy;
}

/*!
    \property QGraphicsAnchor::spacing
    \brief the preferred space between items in the QGraphicsAnchorLayout.

    Depending on the anchor type, the default spacing is either
    0 or a value returned from the style.

    \sa QGraphicsAnchorLayout::addAnchor()
*/
void QGraphicsAnchor::setSpacing(qreal spacing)
{
    Q_D(QGraphicsAnchor);
    d->setSpacing(spacing);
}

qreal QGraphicsAnchor::spacing() const
{
    Q_D(const QGraphicsAnchor);
    return d->spacing();
}

void QGraphicsAnchor::unsetSpacing()
{
    Q_D(QGraphicsAnchor);
    d->unsetSpacing();
}

/*!
    Constructs a QGraphicsAnchorLayout instance.  \a parent is passed to
    QGraphicsLayout's constructor.
  */
QGraphicsAnchorLayout::QGraphicsAnchorLayout(QGraphicsLayoutItem *parent)
    : QGraphicsLayout(*new QGraphicsAnchorLayoutPrivate(), parent)
{
    Q_D(QGraphicsAnchorLayout);
    d->createLayoutEdges();
}

/*!
    Destroys the QGraphicsAnchorLayout object.
*/
QGraphicsAnchorLayout::~QGraphicsAnchorLayout()
{
    Q_D(QGraphicsAnchorLayout);

    for (int i = count() - 1; i >= 0; --i) {
        QGraphicsLayoutItem *item = d->items.at(i);
        removeAt(i);
        if (item) {
            if (item->ownedByLayout())
                delete item;
        }
    }

    d->removeCenterConstraints(this, QGraphicsAnchorLayoutPrivate::Horizontal);
    d->removeCenterConstraints(this, QGraphicsAnchorLayoutPrivate::Vertical);
    d->deleteLayoutEdges();

    Q_ASSERT(d->itemCenterConstraints[0].isEmpty());
    Q_ASSERT(d->itemCenterConstraints[1].isEmpty());
    Q_ASSERT(d->items.isEmpty());
    Q_ASSERT(d->m_vertexList.isEmpty());
}

/*!
    Creates an anchor between the edge \a firstEdge of item \a firstItem and the edge \a secondEdge
    of item \a secondItem. The spacing of the anchor is picked up from the style. Anchors
    between a layout edge and an item edge will have a size of 0.
    If there is already an anchor between the edges, the new anchor will replace the old one.

    \a firstItem and \a secondItem are automatically added to the layout if they are not part
    of the layout. This means that count() can increase by up to 2.

    The spacing an anchor will get depends on the type of anchor. For instance, anchors from the
    Right edge of one item to the Left edge of another (or vice versa) will use the default
    horizontal spacing. The same behaviour applies to Bottom to Top anchors, (but they will use
    the default vertical spacing). For all other anchor combinations, the spacing will be 0.
    All anchoring functions will follow this rule.

    The spacing can also be set manually by using QGraphicsAnchor::setSpacing() method.

    Calling this function where \a firstItem or \a secondItem are ancestors of the layout have
    undefined behaviour.

    \sa addAnchors(), addCornerAnchors()
 */
QGraphicsAnchor *
QGraphicsAnchorLayout::addAnchor(QGraphicsLayoutItem *firstItem, Qt::AnchorPoint firstEdge,
                                 QGraphicsLayoutItem *secondItem, Qt::AnchorPoint secondEdge)
{
    Q_D(QGraphicsAnchorLayout);
    QGraphicsAnchor *a = d->addAnchor(firstItem, firstEdge, secondItem, secondEdge);
    invalidate();
    return a;
}

/*!
    Returns the anchor between the anchor points defined by \a firstItem and \a firstEdge and
    \a secondItem and \a secondEdge. If there is no such anchor, the function will return 0.
*/
QGraphicsAnchor *
QGraphicsAnchorLayout::anchor(QGraphicsLayoutItem *firstItem, Qt::AnchorPoint firstEdge,
                              QGraphicsLayoutItem *secondItem, Qt::AnchorPoint secondEdge)
{
    Q_D(QGraphicsAnchorLayout);
    return d->getAnchor(firstItem, firstEdge, secondItem, secondEdge);
}

/*!
    Creates two anchors between \a firstItem and \a secondItem specified by the corners,
    \a firstCorner and \a secondCorner, where one is for the horizontal edge and another
    one for the vertical edge.

    This is a convenience function, since anchoring corners can be expressed as anchoring
    two edges. For instance:

    \snippet graphicsview/simpleanchorlayout/main.cpp adding a corner anchor in two steps

    This can also be achieved with the following line of code:

    \snippet graphicsview/simpleanchorlayout/main.cpp adding a corner anchor

    If there is already an anchor between the edge pairs, it will be replaced by the anchors that
    this function specifies.

    \a firstItem and \a secondItem are automatically added to the layout if they are not part of the
    layout. This means that count() can increase by up to 2.

    \sa addAnchor(), addAnchors()
*/
void QGraphicsAnchorLayout::addCornerAnchors(QGraphicsLayoutItem *firstItem,
                                             Qt::Corner firstCorner,
                                             QGraphicsLayoutItem *secondItem,
                                             Qt::Corner secondCorner)
{
    Q_D(QGraphicsAnchorLayout);

    // Horizontal anchor
    Qt::AnchorPoint firstEdge = (firstCorner & 1 ? Qt::AnchorRight: Qt::AnchorLeft);
    Qt::AnchorPoint secondEdge = (secondCorner & 1 ? Qt::AnchorRight: Qt::AnchorLeft);
    if (d->addAnchor(firstItem, firstEdge, secondItem, secondEdge)) {
        // Vertical anchor
        firstEdge = (firstCorner & 2 ? Qt::AnchorBottom: Qt::AnchorTop);
        secondEdge = (secondCorner & 2 ? Qt::AnchorBottom: Qt::AnchorTop);
        d->addAnchor(firstItem, firstEdge, secondItem, secondEdge);

        invalidate();
    }
}

/*!
    Anchors two or four edges of \a firstItem with the corresponding
    edges of \a secondItem, so that \a firstItem has the same size as
    \a secondItem in the dimensions specified by \a orientations.

    For example, the following example anchors the left and right edges of two items
    to match their widths:

    \snippet graphicsview/simpleanchorlayout/main.cpp adding anchors to match sizes in two steps

    This can also be achieved using the following line of code:

    \snippet graphicsview/simpleanchorlayout/main.cpp adding anchors to match sizes

    \sa addAnchor(), addCornerAnchors()
*/
void QGraphicsAnchorLayout::addAnchors(QGraphicsLayoutItem *firstItem,
                                       QGraphicsLayoutItem *secondItem,
                                       Qt::Orientations orientations)
{
    bool ok = true;
    if (orientations & Qt::Horizontal) {
        // Currently, if the first is ok, then the rest of the calls should be ok
        ok = addAnchor(secondItem, Qt::AnchorLeft, firstItem, Qt::AnchorLeft) != 0;
        if (ok)
            addAnchor(firstItem, Qt::AnchorRight, secondItem, Qt::AnchorRight);
    }
    if (orientations & Qt::Vertical && ok) {
        addAnchor(secondItem, Qt::AnchorTop, firstItem, Qt::AnchorTop);
        addAnchor(firstItem, Qt::AnchorBottom, secondItem, Qt::AnchorBottom);
    }
}

/*!
    Sets the default horizontal spacing for the anchor layout to \a spacing.

    \sa horizontalSpacing(), setVerticalSpacing(), setSpacing()
*/
void QGraphicsAnchorLayout::setHorizontalSpacing(qreal spacing)
{
    Q_D(QGraphicsAnchorLayout);

    d->spacings[0] = spacing;
    invalidate();
}

/*!
    Sets the default vertical spacing for the anchor layout to \a spacing.

    \sa verticalSpacing(), setHorizontalSpacing(), setSpacing()
*/
void QGraphicsAnchorLayout::setVerticalSpacing(qreal spacing)
{
    Q_D(QGraphicsAnchorLayout);

    d->spacings[1] = spacing;
    invalidate();
}

/*!
    Sets the default horizontal and the default vertical spacing for the anchor layout to \a spacing.

    If an item is anchored with no spacing associated with the anchor, it will use the default
    spacing.

    QGraphicsAnchorLayout does not support negative spacings. Setting a negative value will unset the
    previous spacing and make the layout use the spacing provided by the current widget style.

    \sa setHorizontalSpacing(), setVerticalSpacing()
*/
void QGraphicsAnchorLayout::setSpacing(qreal spacing)
{
    Q_D(QGraphicsAnchorLayout);

    d->spacings[0] = d->spacings[1] = spacing;
    invalidate();
}

/*!
    Returns the default horizontal spacing for the anchor layout.

    \sa verticalSpacing(), setHorizontalSpacing()
*/
qreal QGraphicsAnchorLayout::horizontalSpacing() const
{
    Q_D(const QGraphicsAnchorLayout);
    return d->styleInfo().defaultSpacing(Qt::Horizontal);
}

/*!
    Returns the default vertical spacing for the anchor layout.

    \sa horizontalSpacing(), setVerticalSpacing()
*/
qreal QGraphicsAnchorLayout::verticalSpacing() const
{
    Q_D(const QGraphicsAnchorLayout);
    return d->styleInfo().defaultSpacing(Qt::Vertical);
}

/*!
    \reimp
*/
void QGraphicsAnchorLayout::setGeometry(const QRectF &geom)
{
    Q_D(QGraphicsAnchorLayout);

    QGraphicsLayout::setGeometry(geom);
    d->calculateVertexPositions(QGraphicsAnchorLayoutPrivate::Horizontal);
    d->calculateVertexPositions(QGraphicsAnchorLayoutPrivate::Vertical);
    d->setItemsGeometries(geom);
}

/*!
    Removes the layout item at \a index without destroying it. Ownership of
    the item is transferred to the caller.

    Removing an item will also remove any of the anchors associated with it.

    \sa itemAt(), count()
*/
void QGraphicsAnchorLayout::removeAt(int index)
{
    Q_D(QGraphicsAnchorLayout);
    QGraphicsLayoutItem *item = d->items.value(index);

    if (!item)
        return;

    // Removing an item affects both horizontal and vertical graphs
    d->removeCenterConstraints(item, QGraphicsAnchorLayoutPrivate::Horizontal);
    d->removeCenterConstraints(item, QGraphicsAnchorLayoutPrivate::Vertical);
    d->removeAnchors(item);
    d->items.remove(index);

    item->setParentLayoutItem(0);
    invalidate();
}

/*!
    \reimp
*/
int QGraphicsAnchorLayout::count() const
{
    Q_D(const QGraphicsAnchorLayout);
    return d->items.size();
}

/*!
    \reimp
*/
QGraphicsLayoutItem *QGraphicsAnchorLayout::itemAt(int index) const
{
    Q_D(const QGraphicsAnchorLayout);
    return d->items.value(index);
}

/*!
    \reimp
*/
void QGraphicsAnchorLayout::invalidate()
{
    Q_D(QGraphicsAnchorLayout);
    QGraphicsLayout::invalidate();
    d->calculateGraphCacheDirty = true;
    d->styleInfoDirty = true;
}

/*!
    \reimp
*/
QSizeF QGraphicsAnchorLayout::sizeHint(Qt::SizeHint which, const QSizeF &constraint) const
{
    Q_UNUSED(constraint);
    Q_D(const QGraphicsAnchorLayout);

    // Some setup calculations are delayed until the information is
    // actually needed, avoiding unnecessary recalculations when
    // adding multiple anchors.

    // sizeHint() / effectiveSizeHint() already have a cache
    // mechanism, using invalidate() to force recalculation. However
    // sizeHint() is called three times after invalidation (for max,
    // min and pref), but we just need do our setup once.

    const_cast<QGraphicsAnchorLayoutPrivate *>(d)->calculateGraphs();

    // ### apply constraint!
    QSizeF engineSizeHint(
        d->sizeHints[QGraphicsAnchorLayoutPrivate::Horizontal][which],
        d->sizeHints[QGraphicsAnchorLayoutPrivate::Vertical][which]);

    qreal left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);

    return engineSizeHint + QSizeF(left + right, top + bottom);
}

QT_END_NAMESPACE
#endif //QT_NO_GRAPHICSVIEW
