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

#include <QtWidgets/qwidget.h>
#include <QtWidgets/qapplication.h>
#include <QtCore/qlinkedlist.h>
#include <QtCore/qstack.h>

#ifdef QT_DEBUG
#include <QtCore/qfile.h>
#endif

#include "qgraphicsanchorlayout_p.h"

#ifndef QT_NO_GRAPHICSVIEW
QT_BEGIN_NAMESPACE

// To ensure that all variables inside the simplex solver are non-negative,
// we limit the size of anchors in the interval [-limit, limit]. Then before
// sending them to the simplex solver we add "limit" as an offset, so that
// they are actually calculated in the interval [0, 2 * limit]
// To avoid numerical errors in platforms where we use single precision,
// we use a tighter limit for the variables range.
const qreal g_offset = (sizeof(qreal) == sizeof(double)) ? QWIDGETSIZE_MAX : QWIDGETSIZE_MAX / 32;

QGraphicsAnchorPrivate::QGraphicsAnchorPrivate(int version)
    : QObjectPrivate(version), layoutPrivate(0), data(0),
      sizePolicy(QSizePolicy::Fixed), preferredSize(0),
      hasSize(true)
{
}

QGraphicsAnchorPrivate::~QGraphicsAnchorPrivate()
{
    if (data) {
        // The QGraphicsAnchor was already deleted at this moment. We must clean
        // the dangling pointer to avoid double deletion in the AnchorData dtor.
        data->graphicsAnchor = 0;

        layoutPrivate->removeAnchor(data->from, data->to);
    }
}

void QGraphicsAnchorPrivate::setSizePolicy(QSizePolicy::Policy policy)
{
    if (sizePolicy != policy) {
        sizePolicy = policy;
        layoutPrivate->q_func()->invalidate();
    }
}

void QGraphicsAnchorPrivate::setSpacing(qreal value)
{
    if (!data) {
        qWarning("QGraphicsAnchor::setSpacing: The anchor does not exist.");
        return;
    }

    if (hasSize && (preferredSize == value))
        return;

    // The anchor has an user-defined size
    hasSize = true;
    preferredSize = value;

    layoutPrivate->q_func()->invalidate();
}

void QGraphicsAnchorPrivate::unsetSpacing()
{
    if (!data) {
        qWarning("QGraphicsAnchor::setSpacing: The anchor does not exist.");
        return;
    }

    // Return to standard direction
    hasSize = false;

    layoutPrivate->q_func()->invalidate();
}

qreal QGraphicsAnchorPrivate::spacing() const
{
    if (!data) {
        qWarning("QGraphicsAnchor::setSpacing: The anchor does not exist.");
        return 0;
    }

    return preferredSize;
}


static void applySizePolicy(QSizePolicy::Policy policy,
                            qreal minSizeHint, qreal prefSizeHint, qreal maxSizeHint,
                            qreal *minSize, qreal *prefSize,
                            qreal *maxSize)
{
    // minSize, prefSize and maxSize are initialized
    // with item's preferred Size: this is QSizePolicy::Fixed.
    //
    // Then we check each flag to find the resultant QSizePolicy,
    // according to the following table:
    //
    //      constant               value
    // QSizePolicy::Fixed            0
    // QSizePolicy::Minimum       GrowFlag
    // QSizePolicy::Maximum       ShrinkFlag
    // QSizePolicy::Preferred     GrowFlag | ShrinkFlag
    // QSizePolicy::Ignored       GrowFlag | ShrinkFlag | IgnoreFlag

    if (policy & QSizePolicy::ShrinkFlag)
        *minSize = minSizeHint;
    else
        *minSize = prefSizeHint;

    if (policy & QSizePolicy::GrowFlag)
        *maxSize = maxSizeHint;
    else
        *maxSize = prefSizeHint;

    // Note that these two initializations are affected by the previous flags
    if (policy & QSizePolicy::IgnoreFlag)
        *prefSize = *minSize;
    else
        *prefSize = prefSizeHint;
}

AnchorData::~AnchorData()
{
    if (graphicsAnchor) {
        // Remove reference to ourself to avoid double removal in
        // QGraphicsAnchorPrivate dtor.
        graphicsAnchor->d_func()->data = 0;

        delete graphicsAnchor;
    }
}


void AnchorData::refreshSizeHints(const QLayoutStyleInfo *styleInfo)
{
    QSizePolicy::Policy policy;
    qreal minSizeHint;
    qreal prefSizeHint;
    qreal maxSizeHint;

    if (item) {
        // It is an internal anchor, fetch size information from the item
        if (isLayoutAnchor) {
            minSize = 0;
            prefSize = 0;
            maxSize = QWIDGETSIZE_MAX;
            if (isCenterAnchor)
                maxSize /= 2;

            minPrefSize = prefSize;
            maxPrefSize = maxSize;
            return;
        } else {
            if (orientation == QGraphicsAnchorLayoutPrivate::Horizontal) {
                policy = item->sizePolicy().horizontalPolicy();
                minSizeHint = item->effectiveSizeHint(Qt::MinimumSize).width();
                prefSizeHint = item->effectiveSizeHint(Qt::PreferredSize).width();
                maxSizeHint = item->effectiveSizeHint(Qt::MaximumSize).width();
            } else {
                policy = item->sizePolicy().verticalPolicy();
                minSizeHint = item->effectiveSizeHint(Qt::MinimumSize).height();
                prefSizeHint = item->effectiveSizeHint(Qt::PreferredSize).height();
                maxSizeHint = item->effectiveSizeHint(Qt::MaximumSize).height();
            }

            if (isCenterAnchor) {
                minSizeHint /= 2;
                prefSizeHint /= 2;
                maxSizeHint /= 2;
            }
        }
    } else {
        // It is a user-created anchor, fetch size information from the associated QGraphicsAnchor
        Q_ASSERT(graphicsAnchor);
        QGraphicsAnchorPrivate *anchorPrivate = graphicsAnchor->d_func();

        // Policy, min and max sizes are straightforward
        policy = anchorPrivate->sizePolicy;
        minSizeHint = 0;
        maxSizeHint = QWIDGETSIZE_MAX;

        // Preferred Size
        if (anchorPrivate->hasSize) {
            // Anchor has user-defined size
            prefSizeHint = anchorPrivate->preferredSize;
        } else {
            // Fetch size information from style
            const Qt::Orientation orient = Qt::Orientation(QGraphicsAnchorLayoutPrivate::edgeOrientation(from->m_edge) + 1);
            qreal s = styleInfo->defaultSpacing(orient);
            if (s < 0) {
                QSizePolicy::ControlType controlTypeFrom = from->m_item->sizePolicy().controlType();
                QSizePolicy::ControlType controlTypeTo = to->m_item->sizePolicy().controlType();
                s = styleInfo->perItemSpacing(controlTypeFrom, controlTypeTo, orient);

                // ### Currently we do not support negative anchors inside the graph.
                // To avoid those being created by a negative style spacing, we must
                // make this test.
                if (s < 0)
                    s = 0;
            }
            prefSizeHint = s;
        }
    }

    // Fill minSize, prefSize and maxSize based on policy and sizeHints
    applySizePolicy(policy, minSizeHint, prefSizeHint, maxSizeHint,
                    &minSize, &prefSize, &maxSize);

    minPrefSize = prefSize;
    maxPrefSize = maxSize;

    // Set the anchor effective sizes to preferred.
    //
    // Note: The idea here is that all items should remain at their
    // preferred size unless where that's impossible.  In cases where
    // the item is subject to restrictions (anchored to the layout
    // edges, for instance), the simplex solver will be run to
    // recalculate and override the values we set here.
    sizeAtMinimum = prefSize;
    sizeAtPreferred = prefSize;
    sizeAtMaximum = prefSize;
}

void ParallelAnchorData::updateChildrenSizes()
{
    firstEdge->sizeAtMinimum = sizeAtMinimum;
    firstEdge->sizeAtPreferred = sizeAtPreferred;
    firstEdge->sizeAtMaximum = sizeAtMaximum;

    if (secondForward()) {
        secondEdge->sizeAtMinimum = sizeAtMinimum;
        secondEdge->sizeAtPreferred = sizeAtPreferred;
        secondEdge->sizeAtMaximum = sizeAtMaximum;
    } else {
        secondEdge->sizeAtMinimum = -sizeAtMinimum;
        secondEdge->sizeAtPreferred = -sizeAtPreferred;
        secondEdge->sizeAtMaximum = -sizeAtMaximum;
    }

    firstEdge->updateChildrenSizes();
    secondEdge->updateChildrenSizes();
}

/*
  \internal

  Initialize the parallel anchor size hints using the sizeHint information from
  its children.

  Note that parallel groups can lead to unfeasibility, so during calculation, we can
  find out one unfeasibility. Because of that this method return boolean. This can't
  happen in sequential, so there the method is void.
 */
bool ParallelAnchorData::calculateSizeHints()
{
    // Normalize second child sizes.
    // A negative anchor of sizes min, minPref, pref, maxPref and max, is equivalent
    // to a forward anchor of sizes -max, -maxPref, -pref, -minPref, -min
    qreal secondMin;
    qreal secondMinPref;
    qreal secondPref;
    qreal secondMaxPref;
    qreal secondMax;

    if (secondForward()) {
        secondMin = secondEdge->minSize;
        secondMinPref = secondEdge->minPrefSize;
        secondPref = secondEdge->prefSize;
        secondMaxPref = secondEdge->maxPrefSize;
        secondMax = secondEdge->maxSize;
    } else {
        secondMin = -secondEdge->maxSize;
        secondMinPref = -secondEdge->maxPrefSize;
        secondPref = -secondEdge->prefSize;
        secondMaxPref = -secondEdge->minPrefSize;
        secondMax = -secondEdge->minSize;
    }

    minSize = qMax(firstEdge->minSize, secondMin);
    maxSize = qMin(firstEdge->maxSize, secondMax);

    // This condition means that the maximum size of one anchor being simplified is smaller than
    // the minimum size of the other anchor. The consequence is that there won't be a valid size
    // for this parallel setup.
    if (minSize > maxSize) {
        return false;
    }

    // Preferred size calculation
    // The calculation of preferred size is done as follows:
    //
    // 1) Check whether one of the child anchors is the layout structural anchor
    //    If so, we can simply copy the preferred information from the other child,
    //    after bounding it to our minimum and maximum sizes.
    //    If not, then we proceed with the actual calculations.
    //
    // 2) The whole algorithm for preferred size calculation is based on the fact
    //    that, if a given anchor cannot remain at its preferred size, it'd rather
    //    grow than shrink.
    //
    //    What happens though is that while this affirmative is true for simple
    //    anchors, it may not be true for sequential anchors that have one or more
    //    reversed anchors inside it. That happens because when a sequential anchor
    //    grows, any reversed anchors inside it may be required to shrink, something
    //    we try to avoid, as said above.
    //
    //    To overcome this, besides their actual preferred size "prefSize", each anchor
    //    exports what we call "minPrefSize" and "maxPrefSize". These two values define
    //    a surrounding interval where, if required to move, the anchor would rather
    //    remain inside.
    //
    //    For standard anchors, this area simply represents the region between
    //    prefSize and maxSize, which makes sense since our first affirmation.
    //    For composed anchors, these values are calculated as to reduce the global
    //    "damage", that is, to reduce the total deviation and the total amount of
    //    anchors that had to shrink.

    if (firstEdge->isLayoutAnchor) {
        prefSize = qBound(minSize, secondPref, maxSize);
        minPrefSize = qBound(minSize, secondMinPref, maxSize);
        maxPrefSize = qBound(minSize, secondMaxPref, maxSize);
    } else if (secondEdge->isLayoutAnchor) {
        prefSize = qBound(minSize, firstEdge->prefSize, maxSize);
        minPrefSize = qBound(minSize, firstEdge->minPrefSize, maxSize);
        maxPrefSize = qBound(minSize, firstEdge->maxPrefSize, maxSize);
    } else {
        // Calculate the intersection between the "preferred" regions of each child
        const qreal lowerBoundary =
            qBound(minSize, qMax(firstEdge->minPrefSize, secondMinPref), maxSize);
        const qreal upperBoundary =
            qBound(minSize, qMin(firstEdge->maxPrefSize, secondMaxPref), maxSize);
        const qreal prefMean =
            qBound(minSize, (firstEdge->prefSize + secondPref) / 2, maxSize);

        if (lowerBoundary < upperBoundary) {
            // If there is an intersection between the two regions, this intersection
            // will be used as the preferred region of the parallel anchor itself.
            // The preferred size will be the bounded average between the two preferred
            // sizes.
            prefSize = qBound(lowerBoundary, prefMean, upperBoundary);
            minPrefSize = lowerBoundary;
            maxPrefSize = upperBoundary;
        } else {
            // If there is no intersection, we have to attribute "damage" to at least
            // one of the children. The minimum total damage is achieved in points
            // inside the region that extends from (1) the upper boundary of the lower
            // region to (2) the lower boundary of the upper region.
            // Then, we expose this region as _our_ preferred region and once again,
            // use the bounded average as our preferred size.
            prefSize = qBound(upperBoundary, prefMean, lowerBoundary);
            minPrefSize = upperBoundary;
            maxPrefSize = lowerBoundary;
        }
    }

    // See comment in AnchorData::refreshSizeHints() about sizeAt* values
    sizeAtMinimum = prefSize;
    sizeAtPreferred = prefSize;
    sizeAtMaximum = prefSize;

    return true;
}

/*!
    \internal
    returns the factor in the interval [-1, 1].
    -1 is at Minimum
     0 is at Preferred
     1 is at Maximum
*/
static QPair<QGraphicsAnchorLayoutPrivate::Interval, qreal> getFactor(qreal value, qreal min,
                                                                      qreal minPref, qreal pref,
                                                                      qreal maxPref, qreal max)
{
    QGraphicsAnchorLayoutPrivate::Interval interval;
    qreal lower;
    qreal upper;

    if (value < minPref) {
        interval = QGraphicsAnchorLayoutPrivate::MinimumToMinPreferred;
        lower = min;
        upper = minPref;
    } else if (value < pref) {
        interval = QGraphicsAnchorLayoutPrivate::MinPreferredToPreferred;
        lower = minPref;
        upper = pref;
    } else if (value < maxPref) {
        interval = QGraphicsAnchorLayoutPrivate::PreferredToMaxPreferred;
        lower = pref;
        upper = maxPref;
    } else {
        interval = QGraphicsAnchorLayoutPrivate::MaxPreferredToMaximum;
        lower = maxPref;
        upper = max;
    }

    qreal progress;
    if (upper == lower) {
        progress = 0;
    } else {
        progress = (value - lower) / (upper - lower);
    }

    return qMakePair(interval, progress);
}

static qreal interpolate(const QPair<QGraphicsAnchorLayoutPrivate::Interval, qreal> &factor,
                         qreal min, qreal minPref, qreal pref, qreal maxPref, qreal max)
{
    qreal lower = 0;
    qreal upper = 0;

    switch (factor.first) {
    case QGraphicsAnchorLayoutPrivate::MinimumToMinPreferred:
        lower = min;
        upper = minPref;
        break;
    case QGraphicsAnchorLayoutPrivate::MinPreferredToPreferred:
        lower = minPref;
        upper = pref;
        break;
    case QGraphicsAnchorLayoutPrivate::PreferredToMaxPreferred:
        lower = pref;
        upper = maxPref;
        break;
    case QGraphicsAnchorLayoutPrivate::MaxPreferredToMaximum:
        lower = maxPref;
        upper = max;
        break;
    }

    return lower + factor.second * (upper - lower);
}

void SequentialAnchorData::updateChildrenSizes()
{
    // Band here refers if the value is in the Minimum To Preferred
    // band (the lower band) or the Preferred To Maximum (the upper band).

    const QPair<QGraphicsAnchorLayoutPrivate::Interval, qreal> minFactor =
        getFactor(sizeAtMinimum, minSize, minPrefSize, prefSize, maxPrefSize, maxSize);
    const QPair<QGraphicsAnchorLayoutPrivate::Interval, qreal> prefFactor =
        getFactor(sizeAtPreferred, minSize, minPrefSize, prefSize, maxPrefSize, maxSize);
    const QPair<QGraphicsAnchorLayoutPrivate::Interval, qreal> maxFactor =
        getFactor(sizeAtMaximum, minSize, minPrefSize, prefSize, maxPrefSize, maxSize);

    // XXX This is not safe if Vertex simplification takes place after the sequential
    // anchor is created. In that case, "prev" will be a group-vertex, different from
    // "from" or "to", that _contains_ one of them.
    AnchorVertex *prev = from;

    for (int i = 0; i < m_edges.count(); ++i) {
        AnchorData *e = m_edges.at(i);

        const bool edgeIsForward = (e->from == prev);
        if (edgeIsForward) {
            e->sizeAtMinimum = interpolate(minFactor, e->minSize, e->minPrefSize,
                                           e->prefSize, e->maxPrefSize, e->maxSize);
            e->sizeAtPreferred = interpolate(prefFactor, e->minSize, e->minPrefSize,
                                           e->prefSize, e->maxPrefSize, e->maxSize);
            e->sizeAtMaximum = interpolate(maxFactor, e->minSize, e->minPrefSize,
                                           e->prefSize, e->maxPrefSize, e->maxSize);
            prev = e->to;
        } else {
            Q_ASSERT(prev == e->to);
            e->sizeAtMinimum = interpolate(minFactor, e->maxSize, e->maxPrefSize,
                                           e->prefSize, e->minPrefSize, e->minSize);
            e->sizeAtPreferred = interpolate(prefFactor, e->maxSize, e->maxPrefSize,
                                           e->prefSize, e->minPrefSize, e->minSize);
            e->sizeAtMaximum = interpolate(maxFactor, e->maxSize, e->maxPrefSize,
                                           e->prefSize, e->minPrefSize, e->minSize);
            prev = e->from;
        }

        e->updateChildrenSizes();
    }
}

void SequentialAnchorData::calculateSizeHints()
{
    minSize = 0;
    prefSize = 0;
    maxSize = 0;
    minPrefSize = 0;
    maxPrefSize = 0;

    AnchorVertex *prev = from;

    for (int i = 0; i < m_edges.count(); ++i) {
        AnchorData *edge = m_edges.at(i);

        const bool edgeIsForward = (edge->from == prev);
        if (edgeIsForward) {
            minSize += edge->minSize;
            prefSize += edge->prefSize;
            maxSize += edge->maxSize;
            minPrefSize += edge->minPrefSize;
            maxPrefSize += edge->maxPrefSize;
            prev = edge->to;
        } else {
            Q_ASSERT(prev == edge->to);
            minSize -= edge->maxSize;
            prefSize -= edge->prefSize;
            maxSize -= edge->minSize;
            minPrefSize -= edge->maxPrefSize;
            maxPrefSize -= edge->minPrefSize;
            prev = edge->from;
        }
    }

    // See comment in AnchorData::refreshSizeHints() about sizeAt* values
    sizeAtMinimum = prefSize;
    sizeAtPreferred = prefSize;
    sizeAtMaximum = prefSize;
}

#ifdef QT_DEBUG
void AnchorData::dump(int indent) {
    if (type == Parallel) {
        qDebug("%*s type: parallel:", indent, "");
        ParallelAnchorData *p = static_cast<ParallelAnchorData *>(this);
        p->firstEdge->dump(indent+2);
        p->secondEdge->dump(indent+2);
    } else if (type == Sequential) {
        SequentialAnchorData *s = static_cast<SequentialAnchorData *>(this);
        int kids = s->m_edges.count();
        qDebug("%*s type: sequential(%d):", indent, "", kids);
        for (int i = 0; i < kids; ++i) {
            s->m_edges.at(i)->dump(indent+2);
        }
    } else {
        qDebug("%*s type: Normal:", indent, "");
    }
}

#endif

QSimplexConstraint *GraphPath::constraint(const GraphPath &path) const
{
    // Calculate
    QSet<AnchorData *> cPositives;
    QSet<AnchorData *> cNegatives;
    QSet<AnchorData *> intersection;

    cPositives = positives + path.negatives;
    cNegatives = negatives + path.positives;

    intersection = cPositives & cNegatives;

    cPositives -= intersection;
    cNegatives -= intersection;

    // Fill
    QSimplexConstraint *c = new QSimplexConstraint;
    QSet<AnchorData *>::iterator i;
    for (i = cPositives.begin(); i != cPositives.end(); ++i)
        c->variables.insert(*i, 1.0);

    for (i = cNegatives.begin(); i != cNegatives.end(); ++i)
        c->variables.insert(*i, -1.0);

    return c;
}

#ifdef QT_DEBUG
QString GraphPath::toString() const
{
    QString string(QLatin1String("Path: "));
    foreach(AnchorData *edge, positives)
        string += QString::fromLatin1(" (+++) %1").arg(edge->toString());

    foreach(AnchorData *edge, negatives)
        string += QString::fromLatin1(" (---) %1").arg(edge->toString());

    return string;
}
#endif

QGraphicsAnchorLayoutPrivate::QGraphicsAnchorLayoutPrivate()
    : calculateGraphCacheDirty(true), styleInfoDirty(true)
{
    for (int i = 0; i < NOrientations; ++i) {
        for (int j = 0; j < 3; ++j) {
            sizeHints[i][j] = -1;
        }
        interpolationProgress[i] = -1;

        spacings[i] = -1;
        graphHasConflicts[i] = false;

        layoutFirstVertex[i] = 0;
        layoutCentralVertex[i] = 0;
        layoutLastVertex[i] = 0;
    }
}

Qt::AnchorPoint QGraphicsAnchorLayoutPrivate::oppositeEdge(Qt::AnchorPoint edge)
{
    switch (edge) {
    case Qt::AnchorLeft:
        edge = Qt::AnchorRight;
        break;
    case Qt::AnchorRight:
        edge = Qt::AnchorLeft;
        break;
    case Qt::AnchorTop:
        edge = Qt::AnchorBottom;
        break;
    case Qt::AnchorBottom:
        edge = Qt::AnchorTop;
        break;
    default:
        break;
    }
    return edge;
}


/*!
    \internal

    Adds \a newAnchor to the graph.

    Returns the newAnchor itself if it could be added without further changes to the graph. If a
    new parallel anchor had to be created, then returns the new parallel anchor. If a parallel anchor
    had to be created and it results in an unfeasible setup, \a feasible is set to false, otherwise
    true.

    Note that in the case a new parallel anchor is created, it might also take over some constraints
    from its children anchors.
*/
AnchorData *QGraphicsAnchorLayoutPrivate::addAnchorMaybeParallel(AnchorData *newAnchor, bool *feasible)
{
    Orientation orientation = Orientation(newAnchor->orientation);
    Graph<AnchorVertex, AnchorData> &g = graph[orientation];
    *feasible = true;

    // If already exists one anchor where newAnchor is supposed to be, we create a parallel
    // anchor.
    if (AnchorData *oldAnchor = g.takeEdge(newAnchor->from, newAnchor->to)) {
        ParallelAnchorData *parallel = new ParallelAnchorData(oldAnchor, newAnchor);

        // The parallel anchor will "replace" its children anchors in
        // every center constraint that they appear.

        // ### If the dependent (center) anchors had reference(s) to their constraints, we
        // could avoid traversing all the itemCenterConstraints.
        QList<QSimplexConstraint *> &constraints = itemCenterConstraints[orientation];

        AnchorData *children[2] = { oldAnchor, newAnchor };
        QList<QSimplexConstraint *> *childrenConstraints[2] = { &parallel->m_firstConstraints,
                                                                &parallel->m_secondConstraints };

        for (int i = 0; i < 2; ++i) {
            AnchorData *child = children[i];
            QList<QSimplexConstraint *> *childConstraints = childrenConstraints[i];

            // We need to fix the second child constraints if the parallel group will have the
            // opposite direction of the second child anchor. For the point of view of external
            // entities, this anchor was reversed. So if at some point we say that the parallel
            // has a value of 20, this mean that the second child (when reversed) will be
            // assigned -20.
            const bool needsReverse = i == 1 && !parallel->secondForward();

            if (!child->isCenterAnchor)
                continue;

            parallel->isCenterAnchor = true;

            for (int j = 0; j < constraints.count(); ++j) {
                QSimplexConstraint *c = constraints[j];
                if (c->variables.contains(child)) {
                    childConstraints->append(c);
                    qreal v = c->variables.take(child);
                    if (needsReverse)
                        v *= -1;
                    c->variables.insert(parallel, v);
                }
            }
        }

        // At this point we can identify that the parallel anchor is not feasible, e.g. one
        // anchor minimum size is bigger than the other anchor maximum size.
        *feasible = parallel->calculateSizeHints();
        newAnchor = parallel;
    }

    g.createEdge(newAnchor->from, newAnchor->to, newAnchor);
    return newAnchor;
}

/*!
    \internal

    Takes the sequence of vertices described by (\a before, \a vertices, \a after) and removes
    all anchors connected to the vertices in \a vertices, returning one simplified anchor between
    \a before and \a after.

    Note that this function doesn't add the created anchor to the graph. This should be done by
    the caller.
*/
static AnchorData *createSequence(Graph<AnchorVertex, AnchorData> *graph,
                                  AnchorVertex *before,
                                  const QVector<AnchorVertex*> &vertices,
                                  AnchorVertex *after)
{
#if defined(QT_DEBUG) && 0
    QString strVertices;
    for (int i = 0; i < vertices.count(); ++i) {
        strVertices += QString::fromLatin1("%1 - ").arg(vertices.at(i)->toString());
    }
    QString strPath = QString::fromLatin1("%1 - %2%3").arg(before->toString(), strVertices, after->toString());
    qDebug("simplifying [%s] to [%s - %s]", qPrintable(strPath), qPrintable(before->toString()), qPrintable(after->toString()));
#endif

    AnchorVertex *prev = before;
    QVector<AnchorData *> edges;

    // Take from the graph, the edges that will be simplificated
    for (int i = 0; i < vertices.count(); ++i) {
        AnchorVertex *next = vertices.at(i);
        AnchorData *ad = graph->takeEdge(prev, next);
        Q_ASSERT(ad);
        edges.append(ad);
        prev = next;
    }

    // Take the last edge (not covered in the loop above)
    AnchorData *ad = graph->takeEdge(vertices.last(), after);
    Q_ASSERT(ad);
    edges.append(ad);

    // Create sequence
    SequentialAnchorData *sequence = new SequentialAnchorData(vertices, edges);
    sequence->from = before;
    sequence->to = after;

    sequence->calculateSizeHints();

    return sequence;
}

/*!
   \internal

   The purpose of this function is to simplify the graph.
   Simplification serves two purposes:
   1. Reduce the number of edges in the graph, (thus the number of variables to the equation
      solver is reduced, and the solver performs better).
   2. Be able to do distribution of sequences of edges more intelligently (esp. with sequential
      anchors)

   It is essential that it must be possible to restore simplified anchors back to their "original"
   form. This is done by restoreSimplifiedAnchor().

   There are two types of simplification that can be done:
   1. Sequential simplification
      Sequential simplification means that all sequences of anchors will be merged into one single
      anchor. Only anhcors that points in the same direction will be merged.
   2. Parallel simplification
      If a simplified sequential anchor is about to be inserted between two vertices in the graph
      and there already exist an anchor between those two vertices, a parallel anchor will be
      created that serves as a placeholder for the sequential anchor and the anchor that was
      already between the two vertices.

   The process of simplification can be described as:

   1. Simplify all sequences of anchors into one anchor.
      If no further simplification was done, go to (3)
      - If there already exist an anchor where the sequential anchor is supposed to be inserted,
        take that anchor out of the graph
      - Then create a parallel anchor that holds the sequential anchor and the anchor just taken
        out of the graph.
   2. Go to (1)
   3. Done

   When creating the parallel anchors, the algorithm might identify unfeasible situations. In this
   case the simplification process stops and returns \c false. Otherwise returns \c true.
*/
bool QGraphicsAnchorLayoutPrivate::simplifyGraph(Orientation orientation)
{
    if (items.isEmpty())
        return true;

#if defined(QT_DEBUG) && 0
    qDebug("Simplifying Graph for %s",
           orientation == Horizontal ? "Horizontal" : "Vertical");

    static int count = 0;
    if (orientation == Horizontal) {
        count++;
        dumpGraph(QString::fromLatin1("%1-full").arg(count));
    }
#endif

    // Vertex simplification
    if (!simplifyVertices(orientation)) {
        restoreVertices(orientation);
        return false;
    }

    // Anchor simplification
    bool dirty;
    bool feasible = true;
    do {
        dirty = simplifyGraphIteration(orientation, &feasible);
    } while (dirty && feasible);

    // Note that if we are not feasible, we fallback and make sure that the graph is fully restored
    if (!feasible) {
        restoreSimplifiedGraph(orientation);
        restoreVertices(orientation);
        return false;
    }

#if defined(QT_DEBUG) && 0
    dumpGraph(QString::fromLatin1("%1-simplified-%2").arg(count).arg(
                  QString::fromLatin1(orientation == Horizontal ? "Horizontal" : "Vertical")));
#endif

    return true;
}

static AnchorVertex *replaceVertex_helper(AnchorData *data, AnchorVertex *oldV, AnchorVertex *newV)
{
    AnchorVertex *other;
    if (data->from == oldV) {
        data->from = newV;
        other = data->to;
    } else {
        data->to = newV;
        other = data->from;
    }
    return other;
}

bool QGraphicsAnchorLayoutPrivate::replaceVertex(Orientation orientation, AnchorVertex *oldV,
                                                 AnchorVertex *newV, const QList<AnchorData *> &edges)
{
    Graph<AnchorVertex, AnchorData> &g = graph[orientation];
    bool feasible = true;

    for (int i = 0; i < edges.count(); ++i) {
        AnchorData *ad = edges[i];
        AnchorVertex *otherV = replaceVertex_helper(ad, oldV, newV);

#if defined(QT_DEBUG)
        ad->name = QString::fromLatin1("%1 --to--> %2").arg(ad->from->toString()).arg(ad->to->toString());
#endif

        bool newFeasible;
        AnchorData *newAnchor = addAnchorMaybeParallel(ad, &newFeasible);
        feasible &= newFeasible;

        if (newAnchor != ad) {
            // A parallel was created, we mark that in the list of anchors created by vertex
            // simplification. This is needed because we want to restore them in a separate step
            // from the restoration of anchor simplification.
            anchorsFromSimplifiedVertices[orientation].append(newAnchor);
        }

        g.takeEdge(oldV, otherV);
    }

    return feasible;
}

/*!
    \internal
*/
bool QGraphicsAnchorLayoutPrivate::simplifyVertices(Orientation orientation)
{
    Q_Q(QGraphicsAnchorLayout);
    Graph<AnchorVertex, AnchorData> &g = graph[orientation];

    // We'll walk through vertices
    QStack<AnchorVertex *> stack;
    stack.push(layoutFirstVertex[orientation]);
    QSet<AnchorVertex *> visited;

    while (!stack.isEmpty()) {
        AnchorVertex *v = stack.pop();
        visited.insert(v);

        // Each adjacent of 'v' is a possible vertex to be merged. So we traverse all of
        // them. Since once a merge is made, we might add new adjacents, and we don't want to
        // pass two times through one adjacent. The 'index' is used to track our position.
        QList<AnchorVertex *> adjacents = g.adjacentVertices(v);
        int index = 0;

        while (index < adjacents.count()) {
            AnchorVertex *next = adjacents.at(index);
            index++;

            AnchorData *data = g.edgeData(v, next);
            const bool bothLayoutVertices = v->m_item == q && next->m_item == q;
            const bool zeroSized = !data->minSize && !data->maxSize;

            if (!bothLayoutVertices && zeroSized) {

                // Create a new vertex pair, note that we keep a list of those vertices so we can
                // easily process them when restoring the graph.
                AnchorVertexPair *newV = new AnchorVertexPair(v, next, data);
                simplifiedVertices[orientation].append(newV);

                // Collect the anchors of both vertices, the new vertex pair will take their place
                // in those anchors
                const QList<AnchorVertex *> &vAdjacents = g.adjacentVertices(v);
                const QList<AnchorVertex *> &nextAdjacents = g.adjacentVertices(next);

                for (int i = 0; i < vAdjacents.count(); ++i) {
                    AnchorVertex *adjacent = vAdjacents.at(i);
                    if (adjacent != next) {
                        AnchorData *ad = g.edgeData(v, adjacent);
                        newV->m_firstAnchors.append(ad);
                    }
                }

                for (int i = 0; i < nextAdjacents.count(); ++i) {
                    AnchorVertex *adjacent = nextAdjacents.at(i);
                    if (adjacent != v) {
                        AnchorData *ad = g.edgeData(next, adjacent);
                        newV->m_secondAnchors.append(ad);

                        // We'll also add new vertices to the adjacent list of the new 'v', to be
                        // created as a vertex pair and replace the current one.
                        if (!adjacents.contains(adjacent))
                            adjacents.append(adjacent);
                    }
                }

                // ### merge this loop into the ones that calculated m_firstAnchors/m_secondAnchors?
                // Make newV take the place of v and next
                bool feasible = replaceVertex(orientation, v, newV, newV->m_firstAnchors);
                feasible &= replaceVertex(orientation, next, newV, newV->m_secondAnchors);

                // Update the layout vertex information if one of the vertices is a layout vertex.
                AnchorVertex *layoutVertex = 0;
                if (v->m_item == q)
                    layoutVertex = v;
                else if (next->m_item == q)
                    layoutVertex = next;

                if (layoutVertex) {
                    // Layout vertices always have m_item == q...
                    newV->m_item = q;
                    changeLayoutVertex(orientation, layoutVertex, newV);
                }

                g.takeEdge(v, next);

                // If a non-feasibility is found, we leave early and cancel the simplification
                if (!feasible)
                    return false;

                v = newV;
                visited.insert(newV);

            } else if (!visited.contains(next) && !stack.contains(next)) {
                // If the adjacent is not fit for merge and it wasn't visited by the outermost
                // loop, we add it to the stack.
                stack.push(next);
            }
        }
    }

    return true;
}

/*!
    \internal

    One iteration of the simplification algorithm. Returns \c true if another iteration is needed.

    The algorithm walks the graph in depth-first order, and only collects vertices that has two
    edges connected to it.  If the vertex does not have two edges or if it is a layout edge, it
    will take all the previously collected vertices and try to create a simplified sequential
    anchor representing all the previously collected vertices.  Once the simplified anchor is
    inserted, the collected list is cleared in order to find the next sequence to simplify.

    Note that there are some catches to this that are not covered by the above explanation, see
    the function comments for more details.
*/
bool QGraphicsAnchorLayoutPrivate::simplifyGraphIteration(QGraphicsAnchorLayoutPrivate::Orientation orientation,
                                                          bool *feasible)
{
    Q_Q(QGraphicsAnchorLayout);
    Graph<AnchorVertex, AnchorData> &g = graph[orientation];

    QSet<AnchorVertex *> visited;
    QStack<QPair<AnchorVertex *, AnchorVertex *> > stack;
    stack.push(qMakePair(static_cast<AnchorVertex *>(0), layoutFirstVertex[orientation]));
    QVector<AnchorVertex*> candidates;

    // Walk depth-first, in the stack we store start of the candidate sequence (beforeSequence)
    // and the vertex to be visited.
    while (!stack.isEmpty()) {
        QPair<AnchorVertex *, AnchorVertex *> pair = stack.pop();
        AnchorVertex *beforeSequence = pair.first;
        AnchorVertex *v = pair.second;

        // The basic idea is to determine whether we found an end of sequence,
        // if that's the case, we stop adding vertices to the candidate list
        // and do a simplification step.
        //
        // A vertex can trigger an end of sequence if
        // (a) it is a layout vertex, we don't simplify away the layout vertices;
        // (b) it does not have exactly 2 adjacents;
        // (c) its next adjacent is already visited (a cycle in the graph).
        // (d) the next anchor is a center anchor.

        const QList<AnchorVertex *> &adjacents = g.adjacentVertices(v);
        const bool isLayoutVertex = v->m_item == q;
        AnchorVertex *afterSequence = v;
        bool endOfSequence = false;

        //
        // Identify the end cases.
        //

        // Identifies cases (a) and (b)
        endOfSequence = isLayoutVertex || adjacents.count() != 2;

        if (!endOfSequence) {
            // This is a tricky part. We peek at the next vertex to find out whether
            //
            // - we already visited the next vertex (c);
            // - the next anchor is a center (d).
            //
            // Those are needed to identify the remaining end of sequence cases. Note that unlike
            // (a) and (b), we preempt the end of sequence by looking into the next vertex.

            // Peek at the next vertex
            AnchorVertex *after;
            if (candidates.isEmpty())
                after = (beforeSequence == adjacents.last() ? adjacents.first() : adjacents.last());
            else
                after = (candidates.last() == adjacents.last() ? adjacents.first() : adjacents.last());

            // ### At this point we assumed that candidates will not contain 'after', this may not hold
            // when simplifying FLOATing anchors.
            Q_ASSERT(!candidates.contains(after));

            const AnchorData *data = g.edgeData(v, after);
            Q_ASSERT(data);
            const bool cycleFound = visited.contains(after);

            // Now cases (c) and (d)...
            endOfSequence = cycleFound || data->isCenterAnchor;

            if (!endOfSequence) {
                // If it's not an end of sequence, then the vertex didn't trigger neither of the
                // previously three cases, so it can be added to the candidates list.
                candidates.append(v);
            } else if (cycleFound && (beforeSequence != after)) {
                afterSequence = after;
                candidates.append(v);
            }
        }

        //
        // Add next non-visited vertices to the stack.
        //
        for (int i = 0; i < adjacents.count(); ++i) {
            AnchorVertex *next = adjacents.at(i);
            if (visited.contains(next))
                continue;

            // If current vertex is an end of sequence, and it'll reset the candidates list. So
            // the next vertices will build candidates lists with the current vertex as 'before'
            // vertex. If it's not an end of sequence, we keep the original 'before' vertex,
            // since we are keeping the candidates list.
            if (endOfSequence)
                stack.push(qMakePair(v, next));
            else
                stack.push(qMakePair(beforeSequence, next));
        }

        visited.insert(v);

        if (!endOfSequence || candidates.isEmpty())
            continue;

        //
        // Create a sequence for (beforeSequence, candidates, afterSequence).
        //

        // One restriction we have is to not simplify half of an anchor and let the other half
        // unsimplified. So we remove center edges before and after the sequence.
        const AnchorData *firstAnchor = g.edgeData(beforeSequence, candidates.first());
        if (firstAnchor->isCenterAnchor) {
            beforeSequence = candidates.first();
            candidates.remove(0);

            // If there's not candidates to be simplified, leave.
            if (candidates.isEmpty())
                continue;
        }

        const AnchorData *lastAnchor = g.edgeData(candidates.last(), afterSequence);
        if (lastAnchor->isCenterAnchor) {
            afterSequence = candidates.last();
            candidates.remove(candidates.count() - 1);

            if (candidates.isEmpty())
                continue;
        }

        //
        // Add the sequence to the graph.
        //

        AnchorData *sequence = createSequence(&g, beforeSequence, candidates, afterSequence);

        // If 'beforeSequence' and 'afterSequence' already had an anchor between them, we'll
        // create a parallel anchor between the new sequence and the old anchor.
        bool newFeasible;
        AnchorData *newAnchor = addAnchorMaybeParallel(sequence, &newFeasible);

        if (!newFeasible) {
            *feasible = false;
            return false;
        }

        // When a new parallel anchor is create in the graph, we finish the iteration and return
        // true to indicate a new iteration is needed. This happens because a parallel anchor
        // changes the number of adjacents one vertex has, possibly opening up oportunities for
        // building candidate lists (when adjacents == 2).
        if (newAnchor != sequence)
            return true;

        // If there was no parallel simplification, we'll keep walking the graph. So we clear the
        // candidates list to start again.
        candidates.clear();
    }

    return false;
}

void QGraphicsAnchorLayoutPrivate::restoreSimplifiedAnchor(AnchorData *edge)
{
#if 0
    static const char *anchortypes[] = {"Normal",
                                        "Sequential",
                                        "Parallel"};
    qDebug("Restoring %s edge.", anchortypes[int(edge->type)]);
#endif

    Graph<AnchorVertex, AnchorData> &g = graph[edge->orientation];

    if (edge->type == AnchorData::Normal) {
        g.createEdge(edge->from, edge->to, edge);

    } else if (edge->type == AnchorData::Sequential) {
        SequentialAnchorData *sequence = static_cast<SequentialAnchorData *>(edge);

        for (int i = 0; i < sequence->m_edges.count(); ++i) {
            AnchorData *data = sequence->m_edges.at(i);
            restoreSimplifiedAnchor(data);
        }

        delete sequence;

    } else if (edge->type == AnchorData::Parallel) {

        // Skip parallel anchors that were created by vertex simplification, they will be processed
        // later, when restoring vertex simplification.
        // ### we could improve this check bit having a bit inside 'edge'
        if (anchorsFromSimplifiedVertices[edge->orientation].contains(edge))
            return;

        ParallelAnchorData* parallel = static_cast<ParallelAnchorData*>(edge);
        restoreSimplifiedConstraints(parallel);

        // ### Because of the way parallel anchors are created in the anchor simplification
        // algorithm, we know that one of these will be a sequence, so it'll be safe if the other
        // anchor create an edge between the same vertices as the parallel.
        Q_ASSERT(parallel->firstEdge->type == AnchorData::Sequential
                 || parallel->secondEdge->type == AnchorData::Sequential);
        restoreSimplifiedAnchor(parallel->firstEdge);
        restoreSimplifiedAnchor(parallel->secondEdge);

        delete parallel;
    }
}

void QGraphicsAnchorLayoutPrivate::restoreSimplifiedConstraints(ParallelAnchorData *parallel)
{
    if (!parallel->isCenterAnchor)
        return;

    for (int i = 0; i < parallel->m_firstConstraints.count(); ++i) {
        QSimplexConstraint *c = parallel->m_firstConstraints.at(i);
        qreal v = c->variables[parallel];
        c->variables.remove(parallel);
        c->variables.insert(parallel->firstEdge, v);
    }

    // When restoring, we might have to revert constraints back. See comments on
    // addAnchorMaybeParallel().
    const bool needsReverse = !parallel->secondForward();

    for (int i = 0; i < parallel->m_secondConstraints.count(); ++i) {
        QSimplexConstraint *c = parallel->m_secondConstraints.at(i);
        qreal v = c->variables[parallel];
        if (needsReverse)
            v *= -1;
        c->variables.remove(parallel);
        c->variables.insert(parallel->secondEdge, v);
    }
}

void QGraphicsAnchorLayoutPrivate::restoreSimplifiedGraph(Orientation orientation)
{
#if 0
    qDebug("Restoring Simplified Graph for %s",
           orientation == Horizontal ? "Horizontal" : "Vertical");
#endif

    // Restore anchor simplification
    Graph<AnchorVertex, AnchorData> &g = graph[orientation];
    QList<QPair<AnchorVertex*, AnchorVertex*> > connections = g.connections();
    for (int i = 0; i < connections.count(); ++i) {
        AnchorVertex *v1 = connections.at(i).first;
        AnchorVertex *v2 = connections.at(i).second;
        AnchorData *edge = g.edgeData(v1, v2);

        // We restore only sequential anchors and parallels that were not created by
        // vertex simplification.
        if (edge->type == AnchorData::Sequential
            || (edge->type == AnchorData::Parallel &&
                !anchorsFromSimplifiedVertices[orientation].contains(edge))) {

            g.takeEdge(v1, v2);
            restoreSimplifiedAnchor(edge);
        }
    }

    restoreVertices(orientation);
}

void QGraphicsAnchorLayoutPrivate::restoreVertices(Orientation orientation)
{
    Q_Q(QGraphicsAnchorLayout);

    Graph<AnchorVertex, AnchorData> &g = graph[orientation];
    QList<AnchorVertexPair *> &toRestore = simplifiedVertices[orientation];

    // Since we keep a list of parallel anchors and vertices that were created during vertex
    // simplification, we can now iterate on those lists instead of traversing the graph
    // recursively.

    // First, restore the constraints changed when we created parallel anchors. Note that this
    // works at this point because the constraints doesn't depend on vertex information and at
    // this point it's always safe to identify whether the second child is forward or backwards.
    // In the next step, we'll change the anchors vertices so that would not be possible anymore.
    QList<AnchorData *> &parallelAnchors = anchorsFromSimplifiedVertices[orientation];

    for (int i = parallelAnchors.count() - 1; i >= 0; --i) {
        ParallelAnchorData *parallel = static_cast<ParallelAnchorData *>(parallelAnchors.at(i));
        restoreSimplifiedConstraints(parallel);
    }

    // Then, we will restore the vertices in the inverse order of creation, this way we ensure that
    // the vertex being restored was not wrapped by another simplification.
    for (int i = toRestore.count() - 1; i >= 0; --i) {
        AnchorVertexPair *pair = toRestore.at(i);
        QList<AnchorVertex *> adjacents = g.adjacentVertices(pair);

        // Restore the removed edge, this will also restore both vertices 'first' and 'second' to
        // the graph structure.
        AnchorVertex *first = pair->m_first;
        AnchorVertex *second = pair->m_second;
        g.createEdge(first, second, pair->m_removedAnchor);

        // Restore the anchors for the first child vertex
        for (int j = 0; j < pair->m_firstAnchors.count(); ++j) {
            AnchorData *ad = pair->m_firstAnchors.at(j);
            Q_ASSERT(ad->from == pair || ad->to == pair);

            replaceVertex_helper(ad, pair, first);
            g.createEdge(ad->from, ad->to, ad);
        }

        // Restore the anchors for the second child vertex
        for (int j = 0; j < pair->m_secondAnchors.count(); ++j) {
            AnchorData *ad = pair->m_secondAnchors.at(j);
            Q_ASSERT(ad->from == pair || ad->to == pair);

            replaceVertex_helper(ad, pair, second);
            g.createEdge(ad->from, ad->to, ad);
        }

        for (int j = 0; j < adjacents.count(); ++j) {
            g.takeEdge(pair, adjacents.at(j));
        }

        // The pair simplified a layout vertex, so place back the correct vertex in the variable
        // that track layout vertices
        if (pair->m_item == q) {
            AnchorVertex *layoutVertex = first->m_item == q ? first : second;
            Q_ASSERT(layoutVertex->m_item == q);
            changeLayoutVertex(orientation, pair, layoutVertex);
        }

        delete pair;
    }
    qDeleteAll(parallelAnchors);
    parallelAnchors.clear();
    toRestore.clear();
}

QGraphicsAnchorLayoutPrivate::Orientation
QGraphicsAnchorLayoutPrivate::edgeOrientation(Qt::AnchorPoint edge)
{
    return edge > Qt::AnchorRight ? Vertical : Horizontal;
}

/*!
  \internal

  Create internal anchors to connect the layout edges (Left to Right and
  Top to Bottom).

  These anchors doesn't have size restrictions, that will be enforced by
  other anchors and items in the layout.
*/
void QGraphicsAnchorLayoutPrivate::createLayoutEdges()
{
    Q_Q(QGraphicsAnchorLayout);
    QGraphicsLayoutItem *layout = q;

    // Horizontal
    AnchorData *data = new AnchorData;
    addAnchor_helper(layout, Qt::AnchorLeft, layout,
                     Qt::AnchorRight, data);
    data->maxSize = QWIDGETSIZE_MAX;

    // Save a reference to layout vertices
    layoutFirstVertex[Horizontal] = internalVertex(layout, Qt::AnchorLeft);
    layoutCentralVertex[Horizontal] = 0;
    layoutLastVertex[Horizontal] = internalVertex(layout, Qt::AnchorRight);

    // Vertical
    data = new AnchorData;
    addAnchor_helper(layout, Qt::AnchorTop, layout,
                     Qt::AnchorBottom, data);
    data->maxSize = QWIDGETSIZE_MAX;

    // Save a reference to layout vertices
    layoutFirstVertex[Vertical] = internalVertex(layout, Qt::AnchorTop);
    layoutCentralVertex[Vertical] = 0;
    layoutLastVertex[Vertical] = internalVertex(layout, Qt::AnchorBottom);
}

void QGraphicsAnchorLayoutPrivate::deleteLayoutEdges()
{
    Q_Q(QGraphicsAnchorLayout);

    Q_ASSERT(!internalVertex(q, Qt::AnchorHorizontalCenter));
    Q_ASSERT(!internalVertex(q, Qt::AnchorVerticalCenter));

    removeAnchor_helper(internalVertex(q, Qt::AnchorLeft),
                        internalVertex(q, Qt::AnchorRight));
    removeAnchor_helper(internalVertex(q, Qt::AnchorTop),
                        internalVertex(q, Qt::AnchorBottom));
}

void QGraphicsAnchorLayoutPrivate::createItemEdges(QGraphicsLayoutItem *item)
{
    items.append(item);

    // Create horizontal and vertical internal anchors for the item and
    // refresh its size hint / policy values.
    AnchorData *data = new AnchorData;
    addAnchor_helper(item, Qt::AnchorLeft, item, Qt::AnchorRight, data);
    data->refreshSizeHints();

    data = new AnchorData;
    addAnchor_helper(item, Qt::AnchorTop, item, Qt::AnchorBottom, data);
    data->refreshSizeHints();
}

/*!
  \internal

  By default, each item in the layout is represented internally as
  a single anchor in each direction. For instance, from Left to Right.

  However, to support anchorage of items to the center of items, we
  must split this internal anchor into two half-anchors. From Left
  to Center and then from Center to Right, with the restriction that
  these anchors must have the same time at all times.
*/
void QGraphicsAnchorLayoutPrivate::createCenterAnchors(
    QGraphicsLayoutItem *item, Qt::AnchorPoint centerEdge)
{
    Q_Q(QGraphicsAnchorLayout);

    Orientation orientation;
    switch (centerEdge) {
    case Qt::AnchorHorizontalCenter:
        orientation = Horizontal;
        break;
    case Qt::AnchorVerticalCenter:
        orientation = Vertical;
        break;
    default:
        // Don't create center edges unless needed
        return;
    }

    // Check if vertex already exists
    if (internalVertex(item, centerEdge))
        return;

    // Orientation code
    Qt::AnchorPoint firstEdge;
    Qt::AnchorPoint lastEdge;

    if (orientation == Horizontal) {
        firstEdge = Qt::AnchorLeft;
        lastEdge = Qt::AnchorRight;
    } else {
        firstEdge = Qt::AnchorTop;
        lastEdge = Qt::AnchorBottom;
    }

    AnchorVertex *first = internalVertex(item, firstEdge);
    AnchorVertex *last = internalVertex(item, lastEdge);
    Q_ASSERT(first && last);

    // Create new anchors
    QSimplexConstraint *c = new QSimplexConstraint;

    AnchorData *data = new AnchorData;
    c->variables.insert(data, 1.0);
    addAnchor_helper(item, firstEdge, item, centerEdge, data);
    data->isCenterAnchor = true;
    data->dependency = AnchorData::Master;
    data->refreshSizeHints();

    data = new AnchorData;
    c->variables.insert(data, -1.0);
    addAnchor_helper(item, centerEdge, item, lastEdge, data);
    data->isCenterAnchor = true;
    data->dependency = AnchorData::Slave;
    data->refreshSizeHints();

    itemCenterConstraints[orientation].append(c);

    // Remove old one
    removeAnchor_helper(first, last);

    if (item == q) {
        layoutCentralVertex[orientation] = internalVertex(q, centerEdge);
    }
}

void QGraphicsAnchorLayoutPrivate::removeCenterAnchors(
    QGraphicsLayoutItem *item, Qt::AnchorPoint centerEdge,
    bool substitute)
{
    Q_Q(QGraphicsAnchorLayout);

    Orientation orientation;
    switch (centerEdge) {
    case Qt::AnchorHorizontalCenter:
        orientation = Horizontal;
        break;
    case Qt::AnchorVerticalCenter:
        orientation = Vertical;
        break;
    default:
        // Don't remove edges that not the center ones
        return;
    }

    // Orientation code
    Qt::AnchorPoint firstEdge;
    Qt::AnchorPoint lastEdge;

    if (orientation == Horizontal) {
        firstEdge = Qt::AnchorLeft;
        lastEdge = Qt::AnchorRight;
    } else {
        firstEdge = Qt::AnchorTop;
        lastEdge = Qt::AnchorBottom;
    }

    AnchorVertex *center = internalVertex(item, centerEdge);
    if (!center)
        return;
    AnchorVertex *first = internalVertex(item, firstEdge);

    Q_ASSERT(first);
    Q_ASSERT(center);

    Graph<AnchorVertex, AnchorData> &g = graph[orientation];


    AnchorData *oldData = g.edgeData(first, center);
    // Remove center constraint
    for (int i = itemCenterConstraints[orientation].count() - 1; i >= 0; --i) {
        if (itemCenterConstraints[orientation].at(i)->variables.contains(oldData)) {
            delete itemCenterConstraints[orientation].takeAt(i);
            break;
        }
    }

    if (substitute) {
        // Create the new anchor that should substitute the left-center-right anchors.
        AnchorData *data = new AnchorData;
        addAnchor_helper(item, firstEdge, item, lastEdge, data);
        data->refreshSizeHints();

        // Remove old anchors
        removeAnchor_helper(first, center);
        removeAnchor_helper(center, internalVertex(item, lastEdge));

    } else {
        // this is only called from removeAnchors()
        // first, remove all non-internal anchors
        QList<AnchorVertex*> adjacents = g.adjacentVertices(center);
        for (int i = 0; i < adjacents.count(); ++i) {
            AnchorVertex *v = adjacents.at(i);
            if (v->m_item != item) {
                removeAnchor_helper(center, internalVertex(v->m_item, v->m_edge));
            }
        }
        // when all non-internal anchors is removed it will automatically merge the
        // center anchor into a left-right (or top-bottom) anchor. We must also delete that.
        // by this time, the center vertex is deleted and merged into a non-centered internal anchor
        removeAnchor_helper(first, internalVertex(item, lastEdge));
    }

    if (item == q) {
        layoutCentralVertex[orientation] = 0;
    }
}


void QGraphicsAnchorLayoutPrivate::removeCenterConstraints(QGraphicsLayoutItem *item,
                                                           Orientation orientation)
{
    // Remove the item center constraints associated to this item
    // ### This is a temporary solution. We should probably use a better
    // data structure to hold items and/or their associated constraints
    // so that we can remove those easily

    AnchorVertex *first = internalVertex(item, orientation == Horizontal ?
                                       Qt::AnchorLeft :
                                       Qt::AnchorTop);
    AnchorVertex *center = internalVertex(item, orientation == Horizontal ?
                                        Qt::AnchorHorizontalCenter :
                                        Qt::AnchorVerticalCenter);

    // Skip if no center constraints exist
    if (!center)
        return;

    Q_ASSERT(first);
    AnchorData *internalAnchor = graph[orientation].edgeData(first, center);

    // Look for our anchor in all item center constraints, then remove it
    for (int i = 0; i < itemCenterConstraints[orientation].size(); ++i) {
        if (itemCenterConstraints[orientation].at(i)->variables.contains(internalAnchor)) {
            delete itemCenterConstraints[orientation].takeAt(i);
            break;
        }
    }
}

/*!
 * \internal
 * Implements the high level "addAnchor" feature. Called by the public API
 * addAnchor method.
 *
 * The optional \a spacing argument defines the size of the anchor. If not provided,
 * the anchor size is either 0 or not-set, depending on type of anchor created (see
 * matrix below).
 *
 * All anchors that remain with size not-set will assume the standard spacing,
 * set either by the layout style or through the "setSpacing" layout API.
 */
QGraphicsAnchor *QGraphicsAnchorLayoutPrivate::addAnchor(QGraphicsLayoutItem *firstItem,
                                                         Qt::AnchorPoint firstEdge,
                                                         QGraphicsLayoutItem *secondItem,
                                                         Qt::AnchorPoint secondEdge,
                                                         qreal *spacing)
{
    Q_Q(QGraphicsAnchorLayout);
    if ((firstItem == 0) || (secondItem == 0)) {
        qWarning("QGraphicsAnchorLayout::addAnchor(): "
                 "Cannot anchor NULL items");
        return 0;
    }

    if (firstItem == secondItem) {
        qWarning("QGraphicsAnchorLayout::addAnchor(): "
                 "Cannot anchor the item to itself");
        return 0;
    }

    if (edgeOrientation(secondEdge) != edgeOrientation(firstEdge)) {
        qWarning("QGraphicsAnchorLayout::addAnchor(): "
                 "Cannot anchor edges of different orientations");
        return 0;
    }

    const QGraphicsLayoutItem *parentWidget = q->parentLayoutItem();
    if (firstItem == parentWidget || secondItem == parentWidget) {
        qWarning("QGraphicsAnchorLayout::addAnchor(): "
                 "You cannot add the parent of the layout to the layout.");
        return 0;
    }

    // In QGraphicsAnchorLayout, items are represented in its internal
    // graph as four anchors that connect:
    //  - Left -> HCenter
    //  - HCenter-> Right
    //  - Top -> VCenter
    //  - VCenter -> Bottom

    // Ensure that the internal anchors have been created for both items.
    if (firstItem != q && !items.contains(firstItem)) {
        createItemEdges(firstItem);
        addChildLayoutItem(firstItem);
    }
    if (secondItem != q && !items.contains(secondItem)) {
        createItemEdges(secondItem);
        addChildLayoutItem(secondItem);
    }

    // Create center edges if needed
    createCenterAnchors(firstItem, firstEdge);
    createCenterAnchors(secondItem, secondEdge);

    // Use heuristics to find out what the user meant with this anchor.
    correctEdgeDirection(firstItem, firstEdge, secondItem, secondEdge);

    AnchorData *data = new AnchorData;
    QGraphicsAnchor *graphicsAnchor = acquireGraphicsAnchor(data);

    addAnchor_helper(firstItem, firstEdge, secondItem, secondEdge, data);

    if (spacing) {
        graphicsAnchor->setSpacing(*spacing);
    } else {
        // If firstItem or secondItem is the layout itself, the spacing will default to 0.
        // Otherwise, the following matrix is used (questionmark means that the spacing
        // is queried from the style):
        //                from
        //  to      Left    HCenter Right
        //  Left    0       0       ?
        //  HCenter 0       0       0
        //  Right   ?       0       0
        if (firstItem == q
            || secondItem == q
            || pickEdge(firstEdge, Horizontal) == Qt::AnchorHorizontalCenter
            || oppositeEdge(firstEdge) != secondEdge) {
            graphicsAnchor->setSpacing(0);
        } else {
            graphicsAnchor->unsetSpacing();
        }
    }

    return graphicsAnchor;
}

/*
  \internal

  This method adds an AnchorData to the internal graph. It is responsible for doing
  the boilerplate part of such task.

  If another AnchorData exists between the mentioned vertices, it is deleted and
  the new one is inserted.
*/
void QGraphicsAnchorLayoutPrivate::addAnchor_helper(QGraphicsLayoutItem *firstItem,
                                                    Qt::AnchorPoint firstEdge,
                                                    QGraphicsLayoutItem *secondItem,
                                                    Qt::AnchorPoint secondEdge,
                                                    AnchorData *data)
{
    Q_Q(QGraphicsAnchorLayout);

    const Orientation orientation = edgeOrientation(firstEdge);

    // Create or increase the reference count for the related vertices.
    AnchorVertex *v1 = addInternalVertex(firstItem, firstEdge);
    AnchorVertex *v2 = addInternalVertex(secondItem, secondEdge);

    // Remove previous anchor
    if (graph[orientation].edgeData(v1, v2)) {
        removeAnchor_helper(v1, v2);
    }

    // If its an internal anchor, set the associated item
    if (firstItem == secondItem)
        data->item = firstItem;

    data->orientation = orientation;

    // Create a bi-directional edge in the sense it can be transversed both
    // from v1 or v2. "data" however is shared between the two references
    // so we still know that the anchor direction is from 1 to 2.
    data->from = v1;
    data->to = v2;
#ifdef QT_DEBUG
    data->name = QString::fromLatin1("%1 --to--> %2").arg(v1->toString()).arg(v2->toString());
#endif
    // ### bit to track internal anchors, since inside AnchorData methods
    // we don't have access to the 'q' pointer.
    data->isLayoutAnchor = (data->item == q);

    graph[orientation].createEdge(v1, v2, data);
}

QGraphicsAnchor *QGraphicsAnchorLayoutPrivate::getAnchor(QGraphicsLayoutItem *firstItem,
                                                         Qt::AnchorPoint firstEdge,
                                                         QGraphicsLayoutItem *secondItem,
                                                         Qt::AnchorPoint secondEdge)
{
    // Do not expose internal anchors
    if (firstItem == secondItem)
        return 0;

    const Orientation orientation = edgeOrientation(firstEdge);
    AnchorVertex *v1 = internalVertex(firstItem, firstEdge);
    AnchorVertex *v2 = internalVertex(secondItem, secondEdge);

    QGraphicsAnchor *graphicsAnchor = 0;

    AnchorData *data = graph[orientation].edgeData(v1, v2);
    if (data) {
        // We could use "acquireGraphicsAnchor" here, but to avoid a regression where
        // an internal anchor was wrongly exposed, I want to ensure no new
        // QGraphicsAnchor instances are created by this call.
        // This assumption must hold because anchors are either user-created (and already
        // have their public object created), or they are internal (and must not reach
        // this point).
        Q_ASSERT(data->graphicsAnchor);
        graphicsAnchor = data->graphicsAnchor;
    }
    return graphicsAnchor;
}

/*!
 * \internal
 *
 * Implements the high level "removeAnchor" feature. Called by
 * the QAnchorData destructor.
 */
void QGraphicsAnchorLayoutPrivate::removeAnchor(AnchorVertex *firstVertex,
                                                AnchorVertex *secondVertex)
{
    Q_Q(QGraphicsAnchorLayout);

    // Save references to items while it's safe to assume the vertices exist
    QGraphicsLayoutItem *firstItem = firstVertex->m_item;
    QGraphicsLayoutItem *secondItem = secondVertex->m_item;

    // Delete the anchor (may trigger deletion of center vertices)
    removeAnchor_helper(firstVertex, secondVertex);

    // Ensure no dangling pointer is left behind
    firstVertex = secondVertex = 0;

    // Checking if the item stays in the layout or not
    bool keepFirstItem = false;
    bool keepSecondItem = false;

    QPair<AnchorVertex *, int> v;
    int refcount = -1;

    if (firstItem != q) {
        for (int i = Qt::AnchorLeft; i <= Qt::AnchorBottom; ++i) {
            v = m_vertexList.value(qMakePair(firstItem, static_cast<Qt::AnchorPoint>(i)));
            if (v.first) {
                if (i == Qt::AnchorHorizontalCenter || i == Qt::AnchorVerticalCenter)
                    refcount = 2;
                else
                    refcount = 1;

                if (v.second > refcount) {
                    keepFirstItem = true;
                    break;
                }
            }
        }
    } else
        keepFirstItem = true;

    if (secondItem != q) {
        for (int i = Qt::AnchorLeft; i <= Qt::AnchorBottom; ++i) {
            v = m_vertexList.value(qMakePair(secondItem, static_cast<Qt::AnchorPoint>(i)));
            if (v.first) {
                if (i == Qt::AnchorHorizontalCenter || i == Qt::AnchorVerticalCenter)
                    refcount = 2;
                else
                    refcount = 1;

                if (v.second > refcount) {
                    keepSecondItem = true;
                    break;
                }
            }
        }
    } else
        keepSecondItem = true;

    if (!keepFirstItem)
        q->removeAt(items.indexOf(firstItem));

    if (!keepSecondItem)
        q->removeAt(items.indexOf(secondItem));

    // Removing anchors invalidates the layout
    q->invalidate();
}

/*
  \internal

  Implements the low level "removeAnchor" feature. Called by
  private methods.
*/
void QGraphicsAnchorLayoutPrivate::removeAnchor_helper(AnchorVertex *v1, AnchorVertex *v2)
{
    Q_ASSERT(v1 && v2);

    // Remove edge from graph
    const Orientation o = edgeOrientation(v1->m_edge);
    graph[o].removeEdge(v1, v2);

    // Decrease vertices reference count (may trigger a deletion)
    removeInternalVertex(v1->m_item, v1->m_edge);
    removeInternalVertex(v2->m_item, v2->m_edge);
}

AnchorVertex *QGraphicsAnchorLayoutPrivate::addInternalVertex(QGraphicsLayoutItem *item,
                                                              Qt::AnchorPoint edge)
{
    QPair<QGraphicsLayoutItem *, Qt::AnchorPoint> pair(item, edge);
    QPair<AnchorVertex *, int> v = m_vertexList.value(pair);

    if (!v.first) {
        Q_ASSERT(v.second == 0);
        v.first = new AnchorVertex(item, edge);
    }
    v.second++;
    m_vertexList.insert(pair, v);
    return v.first;
}

/**
 * \internal
 *
 * returns the AnchorVertex that was dereferenced, also when it was removed.
 * returns 0 if it did not exist.
 */
void QGraphicsAnchorLayoutPrivate::removeInternalVertex(QGraphicsLayoutItem *item,
                                                        Qt::AnchorPoint edge)
{
    QPair<QGraphicsLayoutItem *, Qt::AnchorPoint> pair(item, edge);
    QPair<AnchorVertex *, int> v = m_vertexList.value(pair);

    if (!v.first) {
        qWarning("This item with this edge is not in the graph");
        return;
    }

    v.second--;
    if (v.second == 0) {
        // Remove reference and delete vertex
        m_vertexList.remove(pair);
        delete v.first;
    } else {
        // Update reference count
        m_vertexList.insert(pair, v);

        if ((v.second == 2) &&
            ((edge == Qt::AnchorHorizontalCenter) ||
             (edge == Qt::AnchorVerticalCenter))) {
            removeCenterAnchors(item, edge, true);
        }
    }
}

void QGraphicsAnchorLayoutPrivate::removeVertex(QGraphicsLayoutItem *item, Qt::AnchorPoint edge)
{
    if (AnchorVertex *v = internalVertex(item, edge)) {
        Graph<AnchorVertex, AnchorData> &g = graph[edgeOrientation(edge)];
        const QList<AnchorVertex *> allVertices = graph[edgeOrientation(edge)].adjacentVertices(v);
        AnchorVertex *v2;
        foreach (v2, allVertices) {
            g.removeEdge(v, v2);
            removeInternalVertex(item, edge);
            removeInternalVertex(v2->m_item, v2->m_edge);
        }
    }
}

void QGraphicsAnchorLayoutPrivate::removeAnchors(QGraphicsLayoutItem *item)
{
    // remove the center anchor first!!
    removeCenterAnchors(item, Qt::AnchorHorizontalCenter, false);
    removeVertex(item, Qt::AnchorLeft);
    removeVertex(item, Qt::AnchorRight);

    removeCenterAnchors(item, Qt::AnchorVerticalCenter, false);
    removeVertex(item, Qt::AnchorTop);
    removeVertex(item, Qt::AnchorBottom);
}

/*!
  \internal

  Use heuristics to determine the correct orientation of a given anchor.

  After API discussions, we decided we would like expressions like
  anchor(A, Left, B, Right) to mean the same as anchor(B, Right, A, Left).
  The problem with this is that anchors could become ambiguous, for
  instance, what does the anchor A, B of size X mean?

     "pos(B) = pos(A) + X"  or  "pos(A) = pos(B) + X" ?

  To keep the API user friendly and at the same time, keep our algorithm
  deterministic, we use an heuristic to determine a direction for each
  added anchor and then keep it. The heuristic is based on the fact
  that people usually avoid overlapping items, therefore:

     "A, RIGHT to B, LEFT" means that B is to the LEFT of A.
     "B, LEFT to A, RIGHT" is corrected to the above anchor.

  Special correction is also applied when one of the items is the
  layout. We handle Layout Left as if it was another items's Right
  and Layout Right as another item's Left.
*/
void QGraphicsAnchorLayoutPrivate::correctEdgeDirection(QGraphicsLayoutItem *&firstItem,
                                                        Qt::AnchorPoint &firstEdge,
                                                        QGraphicsLayoutItem *&secondItem,
                                                        Qt::AnchorPoint &secondEdge)
{
    Q_Q(QGraphicsAnchorLayout);

    if ((firstItem != q) && (secondItem != q)) {
        // If connection is between widgets (not the layout itself)
        // Ensure that "right-edges" sit to the left of "left-edges".
        if (firstEdge < secondEdge) {
            qSwap(firstItem, secondItem);
            qSwap(firstEdge, secondEdge);
        }
    } else if (firstItem == q) {
        // If connection involves the right or bottom of a layout, ensure
        // the layout is the second item.
        if ((firstEdge == Qt::AnchorRight) || (firstEdge == Qt::AnchorBottom)) {
            qSwap(firstItem, secondItem);
            qSwap(firstEdge, secondEdge);
        }
    } else if ((secondEdge != Qt::AnchorRight) && (secondEdge != Qt::AnchorBottom)) {
        // If connection involves the left, center or top of layout, ensure
        // the layout is the first item.
        qSwap(firstItem, secondItem);
        qSwap(firstEdge, secondEdge);
    }
}

QLayoutStyleInfo &QGraphicsAnchorLayoutPrivate::styleInfo() const
{
    if (styleInfoDirty) {
        Q_Q(const QGraphicsAnchorLayout);
        //### Fix this if QGV ever gets support for Metal style or different Aqua sizes.
        QWidget *wid = 0;

        QGraphicsLayoutItem *parent = q->parentLayoutItem();
        while (parent && parent->isLayout()) {
            parent = parent->parentLayoutItem();
        }
        QGraphicsWidget *w = 0;
        if (parent) {
            QGraphicsItem *parentItem = parent->graphicsItem();
            if (parentItem && parentItem->isWidget())
                w = static_cast<QGraphicsWidget*>(parentItem);
        }

        QStyle *style = w ? w->style() : QApplication::style();
        cachedStyleInfo = QLayoutStyleInfo(style, wid);
        cachedStyleInfo.setDefaultSpacing(Qt::Horizontal, spacings[0]);
        cachedStyleInfo.setDefaultSpacing(Qt::Vertical, spacings[1]);

        styleInfoDirty = false;
    }
    return cachedStyleInfo;
}

/*!
  \internal

  Called on activation. Uses Linear Programming to define minimum, preferred
  and maximum sizes for the layout. Also calculates the sizes that each item
  should assume when the layout is in one of such situations.
*/
void QGraphicsAnchorLayoutPrivate::calculateGraphs()
{
    if (!calculateGraphCacheDirty)
        return;
    calculateGraphs(Horizontal);
    calculateGraphs(Vertical);
    calculateGraphCacheDirty = false;
}

// ### Maybe getGraphParts could return the variables when traversing, at least
// for trunk...
QList<AnchorData *> getVariables(QList<QSimplexConstraint *> constraints)
{
    QSet<AnchorData *> variableSet;
    for (int i = 0; i < constraints.count(); ++i) {
        const QSimplexConstraint *c = constraints.at(i);
        foreach (QSimplexVariable *var, c->variables.keys()) {
            variableSet += static_cast<AnchorData *>(var);
        }
    }
    return variableSet.toList();
}

/*!
    \internal

    Calculate graphs is the method that puts together all the helper routines
    so that the AnchorLayout can calculate the sizes of each item.

    In a nutshell it should do:

    1) Refresh anchor nominal sizes, that is, the size that each anchor would
       have if no other restrictions applied. This is done by quering the
       layout style and the sizeHints of the items belonging to the layout.

    2) Simplify the graph by grouping together parallel and sequential anchors
       into "group anchors". These have equivalent minimum, preferred and maximum
       sizeHints as the anchors they replace.

    3) Check if we got to a trivial case. In some cases, the whole graph can be
       simplified into a single anchor. If so, use this information. If not,
       then call the Simplex solver to calculate the anchors sizes.

    4) Once the root anchors had its sizes calculated, propagate that to the
       anchors they represent.
*/
void QGraphicsAnchorLayoutPrivate::calculateGraphs(
    QGraphicsAnchorLayoutPrivate::Orientation orientation)
{
#if defined(QT_DEBUG) || defined(Q_AUTOTEST_EXPORT)
    lastCalculationUsedSimplex[orientation] = false;
#endif

    static bool simplificationEnabled = qEnvironmentVariableIsEmpty("QT_ANCHORLAYOUT_NO_SIMPLIFICATION");

    // Reset the nominal sizes of each anchor based on the current item sizes
    refreshAllSizeHints(orientation);

    // Simplify the graph
    if (simplificationEnabled && !simplifyGraph(orientation)) {
        qWarning("QGraphicsAnchorLayout: anchor setup is not feasible.");
        graphHasConflicts[orientation] = true;
        return;
    }

    // Traverse all graph edges and store the possible paths to each vertex
    findPaths(orientation);

    // From the paths calculated above, extract the constraints that the current
    // anchor setup impose, to our Linear Programming problem.
    constraintsFromPaths(orientation);

    // Split the constraints and anchors into groups that should be fed to the
    // simplex solver independently. Currently we find two groups:
    //
    //  1) The "trunk", that is, the set of anchors (items) that are connected
    //     to the two opposite sides of our layout, and thus need to stretch in
    //     order to fit in the current layout size.
    //
    //  2) The floating or semi-floating anchors (items) that are those which
    //     are connected to only one (or none) of the layout sides, thus are not
    //     influenced by the layout size.
    QList<QList<QSimplexConstraint *> > parts = getGraphParts(orientation);

    // Now run the simplex solver to calculate Minimum, Preferred and Maximum sizes
    // of the "trunk" set of constraints and variables.
    // ### does trunk always exist? empty = trunk is the layout left->center->right
    QList<QSimplexConstraint *> trunkConstraints = parts.at(0);
    QList<AnchorData *> trunkVariables = getVariables(trunkConstraints);

    // For minimum and maximum, use the path between the two layout sides as the
    // objective function.
    AnchorVertex *v = layoutLastVertex[orientation];
    GraphPath trunkPath = graphPaths[orientation].value(v);

    bool feasible = calculateTrunk(orientation, trunkPath, trunkConstraints, trunkVariables);

    // For the other parts that not the trunk, solve only for the preferred size
    // that is the size they will remain at, since they are not stretched by the
    // layout.

    // Skipping the first (trunk)
    for (int i = 1; i < parts.count(); ++i) {
        if (!feasible)
            break;

        QList<QSimplexConstraint *> partConstraints = parts.at(i);
        QList<AnchorData *> partVariables = getVariables(partConstraints);
        Q_ASSERT(!partVariables.isEmpty());
        feasible &= calculateNonTrunk(partConstraints, partVariables);
    }

    // Propagate the new sizes down the simplified graph, ie. tell the
    // group anchors to set their children anchors sizes.
    updateAnchorSizes(orientation);

    graphHasConflicts[orientation] = !feasible;

    // Clean up our data structures. They are not needed anymore since
    // distribution uses just interpolation.
    qDeleteAll(constraints[orientation]);
    constraints[orientation].clear();
    graphPaths[orientation].clear(); // ###

    if (simplificationEnabled)
        restoreSimplifiedGraph(orientation);
}

/*!
    \internal

    Shift all the constraints by a certain amount. This allows us to deal with negative values in
    the linear program if they are bounded by a certain limit. Functions should be careful to
    call it again with a negative amount, to shift the constraints back.
*/
static void shiftConstraints(const QList<QSimplexConstraint *> &constraints, qreal amount)
{
    for (int i = 0; i < constraints.count(); ++i) {
        QSimplexConstraint *c = constraints.at(i);
        qreal multiplier = 0;
        foreach (qreal v, c->variables) {
            multiplier += v;
        }
        c->constant += multiplier * amount;
    }
}

/*!
    \internal

    Calculate the sizes for all anchors which are part of the trunk. This works
    on top of a (possibly) simplified graph.
*/
bool QGraphicsAnchorLayoutPrivate::calculateTrunk(Orientation orientation, const GraphPath &path,
                                                  const QList<QSimplexConstraint *> &constraints,
                                                  const QList<AnchorData *> &variables)
{
    bool feasible = true;
    bool needsSimplex = !constraints.isEmpty();

#if 0
    qDebug("Simplex %s for trunk of %s", needsSimplex ? "used" : "NOT used",
           orientation == Horizontal ? "Horizontal" : "Vertical");
#endif

    if (needsSimplex) {

        QList<QSimplexConstraint *> sizeHintConstraints = constraintsFromSizeHints(variables);
        QList<QSimplexConstraint *> allConstraints = constraints + sizeHintConstraints;

        shiftConstraints(allConstraints, g_offset);

        // Solve min and max size hints
        qreal min, max;
        feasible = solveMinMax(allConstraints, path, &min, &max);

        if (feasible) {
            solvePreferred(constraints, variables);

            // Calculate and set the preferred size for the layout,
            // from the edge sizes that were calculated above.
            qreal pref(0.0);
            foreach (const AnchorData *ad, path.positives) {
                pref += ad->sizeAtPreferred;
            }
            foreach (const AnchorData *ad, path.negatives) {
                pref -= ad->sizeAtPreferred;
            }

            sizeHints[orientation][Qt::MinimumSize] = min;
            sizeHints[orientation][Qt::PreferredSize] = pref;
            sizeHints[orientation][Qt::MaximumSize] = max;
        }

        qDeleteAll(sizeHintConstraints);
        shiftConstraints(constraints, -g_offset);

    } else {
        // No Simplex is necessary because the path was simplified all the way to a single
        // anchor.
        Q_ASSERT(path.positives.count() == 1);
        Q_ASSERT(path.negatives.count() == 0);

        AnchorData *ad = path.positives.toList()[0];
        ad->sizeAtMinimum = ad->minSize;
        ad->sizeAtPreferred = ad->prefSize;
        ad->sizeAtMaximum = ad->maxSize;

        sizeHints[orientation][Qt::MinimumSize] = ad->sizeAtMinimum;
        sizeHints[orientation][Qt::PreferredSize] = ad->sizeAtPreferred;
        sizeHints[orientation][Qt::MaximumSize] = ad->sizeAtMaximum;
    }

#if defined(QT_DEBUG) || defined(Q_AUTOTEST_EXPORT)
    lastCalculationUsedSimplex[orientation] = needsSimplex;
#endif

    return feasible;
}

/*!
    \internal
*/
bool QGraphicsAnchorLayoutPrivate::calculateNonTrunk(const QList<QSimplexConstraint *> &constraints,
                                                     const QList<AnchorData *> &variables)
{
    shiftConstraints(constraints, g_offset);
    bool feasible = solvePreferred(constraints, variables);

    if (feasible) {
        // Propagate size at preferred to other sizes. Semi-floats always will be
        // in their sizeAtPreferred.
        for (int j = 0; j < variables.count(); ++j) {
            AnchorData *ad = variables.at(j);
            Q_ASSERT(ad);
            ad->sizeAtMinimum = ad->sizeAtPreferred;
            ad->sizeAtMaximum = ad->sizeAtPreferred;
        }
    }

    shiftConstraints(constraints, -g_offset);
    return feasible;
}

/*!
    \internal

    Traverse the graph refreshing the size hints. Edges will query their associated
    item or graphicsAnchor for their size hints.
*/
void QGraphicsAnchorLayoutPrivate::refreshAllSizeHints(Orientation orientation)
{
    Graph<AnchorVertex, AnchorData> &g = graph[orientation];
    QList<QPair<AnchorVertex *, AnchorVertex *> > vertices = g.connections();

    QLayoutStyleInfo styleInf = styleInfo();
    for (int i = 0; i < vertices.count(); ++i) {
        AnchorData *data = g.edgeData(vertices.at(i).first, vertices.at(i).second);
        data->refreshSizeHints(&styleInf);
    }
}

/*!
  \internal

  This method walks the graph using a breadth-first search to find paths
  between the root vertex and each vertex on the graph. The edges
  directions in each path are considered and they are stored as a
  positive edge (left-to-right) or negative edge (right-to-left).

  The list of paths is used later to generate a list of constraints.
 */
void QGraphicsAnchorLayoutPrivate::findPaths(Orientation orientation)
{
    QQueue<QPair<AnchorVertex *, AnchorVertex *> > queue;

    QSet<AnchorData *> visited;

    AnchorVertex *root = layoutFirstVertex[orientation];

    graphPaths[orientation].insert(root, GraphPath());

    foreach (AnchorVertex *v, graph[orientation].adjacentVertices(root)) {
        queue.enqueue(qMakePair(root, v));
    }

    while(!queue.isEmpty()) {
        QPair<AnchorVertex *, AnchorVertex *>  pair = queue.dequeue();
        AnchorData *edge = graph[orientation].edgeData(pair.first, pair.second);

        if (visited.contains(edge))
            continue;

        visited.insert(edge);
        GraphPath current = graphPaths[orientation].value(pair.first);

        if (edge->from == pair.first)
            current.positives.insert(edge);
        else
            current.negatives.insert(edge);

        graphPaths[orientation].insert(pair.second, current);

        foreach (AnchorVertex *v,
                graph[orientation].adjacentVertices(pair.second)) {
            queue.enqueue(qMakePair(pair.second, v));
        }
    }

    // We will walk through every reachable items (non-float) store them in a temporary set.
    // We them create a set of all items and subtract the non-floating items from the set in
    // order to get the floating items. The floating items is then stored in m_floatItems
    identifyFloatItems(visited, orientation);
}

/*!
  \internal

  Each vertex on the graph that has more than one path to it
  represents a contra int to the sizes of the items in these paths.

  This method walks the list of paths to each vertex, generate
  the constraints and store them in a list so they can be used later
  by the Simplex solver.
*/
void QGraphicsAnchorLayoutPrivate::constraintsFromPaths(Orientation orientation)
{
    foreach (AnchorVertex *vertex, graphPaths[orientation].uniqueKeys())
    {
        int valueCount = graphPaths[orientation].count(vertex);
        if (valueCount == 1)
            continue;

        QList<GraphPath> pathsToVertex = graphPaths[orientation].values(vertex);
        for (int i = 1; i < valueCount; ++i) {
            constraints[orientation] += \
                pathsToVertex[0].constraint(pathsToVertex.at(i));
        }
    }
}

/*!
  \internal
*/
void QGraphicsAnchorLayoutPrivate::updateAnchorSizes(Orientation orientation)
{
    Graph<AnchorVertex, AnchorData> &g = graph[orientation];
    const QList<QPair<AnchorVertex *, AnchorVertex *> > &vertices = g.connections();

    for (int i = 0; i < vertices.count(); ++i) {
        AnchorData *ad = g.edgeData(vertices.at(i).first, vertices.at(i).second);
        ad->updateChildrenSizes();
    }
}

/*!
  \internal

  Create LP constraints for each anchor based on its minimum and maximum
  sizes, as specified in its size hints
*/
QList<QSimplexConstraint *> QGraphicsAnchorLayoutPrivate::constraintsFromSizeHints(
    const QList<AnchorData *> &anchors)
{
    if (anchors.isEmpty())
        return QList<QSimplexConstraint *>();

    // Look for the layout edge. That can be either the first half in case the
    // layout is split in two, or the whole layout anchor.
    Orientation orient = Orientation(anchors.first()->orientation);
    AnchorData *layoutEdge = 0;
    if (layoutCentralVertex[orient]) {
        layoutEdge = graph[orient].edgeData(layoutFirstVertex[orient], layoutCentralVertex[orient]);
    } else {
        layoutEdge = graph[orient].edgeData(layoutFirstVertex[orient], layoutLastVertex[orient]);
    }

    // If maxSize is less then "infinite", that means there are other anchors
    // grouped together with this one. We can't ignore its maximum value so we
    // set back the variable to NULL to prevent the continue condition from being
    // satisfied in the loop below.
    const qreal expectedMax = layoutCentralVertex[orient] ? QWIDGETSIZE_MAX / 2 : QWIDGETSIZE_MAX;
    qreal actualMax;
    if (layoutEdge->from == layoutFirstVertex[orient]) {
        actualMax = layoutEdge->maxSize;
    } else {
        actualMax = -layoutEdge->minSize;
    }
    if (actualMax != expectedMax) {
        layoutEdge = 0;
    }

    // For each variable, create constraints based on size hints
    QList<QSimplexConstraint *> anchorConstraints;
    bool unboundedProblem = true;
    for (int i = 0; i < anchors.size(); ++i) {
        AnchorData *ad = anchors.at(i);

        // Anchors that have their size directly linked to another one don't need constraints
        // For exammple, the second half of an item has exactly the same size as the first half
        // thus constraining the latter is enough.
        if (ad->dependency == AnchorData::Slave)
            continue;

        // To use negative variables inside simplex, we shift them so the minimum negative value is
        // mapped to zero before solving. To make sure that it works, we need to guarantee that the
        // variables are all inside a certain boundary.
        qreal boundedMin = qBound(-g_offset, ad->minSize, g_offset);
        qreal boundedMax = qBound(-g_offset, ad->maxSize, g_offset);

        if ((boundedMin == boundedMax) || qFuzzyCompare(boundedMin, boundedMax)) {
            QSimplexConstraint *c = new QSimplexConstraint;
            c->variables.insert(ad, 1.0);
            c->constant = boundedMin;
            c->ratio = QSimplexConstraint::Equal;
            anchorConstraints += c;
            unboundedProblem = false;
        } else {
            QSimplexConstraint *c = new QSimplexConstraint;
            c->variables.insert(ad, 1.0);
            c->constant = boundedMin;
            c->ratio = QSimplexConstraint::MoreOrEqual;
            anchorConstraints += c;

            // We avoid adding restrictions to the layout internal anchors. That's
            // to prevent unnecessary fair distribution from happening due to this
            // artificial restriction.
            if (ad == layoutEdge)
                continue;

            c = new QSimplexConstraint;
            c->variables.insert(ad, 1.0);
            c->constant = boundedMax;
            c->ratio = QSimplexConstraint::LessOrEqual;
            anchorConstraints += c;
            unboundedProblem = false;
        }
    }

    // If no upper boundary restriction was added, add one to avoid unbounded problem
    if (unboundedProblem) {
        QSimplexConstraint *c = new QSimplexConstraint;
        c->variables.insert(layoutEdge, 1.0);
        // The maximum size that the layout can take
        c->constant = g_offset;
        c->ratio = QSimplexConstraint::LessOrEqual;
        anchorConstraints += c;
    }

    return anchorConstraints;
}

/*!
  \internal
*/
QList< QList<QSimplexConstraint *> >
QGraphicsAnchorLayoutPrivate::getGraphParts(Orientation orientation)
{
    Q_ASSERT(layoutFirstVertex[orientation] && layoutLastVertex[orientation]);

    AnchorData *edgeL1 = 0;
    AnchorData *edgeL2 = 0;

    // The layout may have a single anchor between Left and Right or two half anchors
    // passing through the center
    if (layoutCentralVertex[orientation]) {
        edgeL1 = graph[orientation].edgeData(layoutFirstVertex[orientation], layoutCentralVertex[orientation]);
        edgeL2 = graph[orientation].edgeData(layoutCentralVertex[orientation], layoutLastVertex[orientation]);
    } else {
        edgeL1 = graph[orientation].edgeData(layoutFirstVertex[orientation], layoutLastVertex[orientation]);
    }

    QLinkedList<QSimplexConstraint *> remainingConstraints;
    for (int i = 0; i < constraints[orientation].count(); ++i) {
        remainingConstraints += constraints[orientation].at(i);
    }
    for (int i = 0; i < itemCenterConstraints[orientation].count(); ++i) {
        remainingConstraints += itemCenterConstraints[orientation].at(i);
    }

    QList<QSimplexConstraint *> trunkConstraints;
    QSet<QSimplexVariable *> trunkVariables;

    trunkVariables += edgeL1;
    if (edgeL2)
        trunkVariables += edgeL2;

    bool dirty;
    do {
        dirty = false;

        QLinkedList<QSimplexConstraint *>::iterator it = remainingConstraints.begin();
        while (it != remainingConstraints.end()) {
            QSimplexConstraint *c = *it;
            bool match = false;

            // Check if this constraint have some overlap with current
            // trunk variables...
            foreach (QSimplexVariable *ad, trunkVariables) {
                if (c->variables.contains(ad)) {
                    match = true;
                    break;
                }
            }

            // If so, we add it to trunk, and erase it from the
            // remaining constraints.
            if (match) {
                trunkConstraints += c;
                trunkVariables += QSet<QSimplexVariable *>::fromList(c->variables.keys());
                it = remainingConstraints.erase(it);
                dirty = true;
            } else {
                // Note that we don't erase the constraint if it's not
                // a match, since in a next iteration of a do-while we
                // can pass on it again and it will be a match.
                //
                // For example: if trunk share a variable with
                // remainingConstraints[1] and it shares with
                // remainingConstraints[0], we need a second iteration
                // of the do-while loop to match both.
                ++it;
            }
        }
    } while (dirty);

    QList< QList<QSimplexConstraint *> > result;
    result += trunkConstraints;

    if (!remainingConstraints.isEmpty()) {
        QList<QSimplexConstraint *> nonTrunkConstraints;
        QLinkedList<QSimplexConstraint *>::iterator it = remainingConstraints.begin();
        while (it != remainingConstraints.end()) {
            nonTrunkConstraints += *it;
            ++it;
        }
        result += nonTrunkConstraints;
    }

    return result;
}

/*!
 \internal

  Use all visited Anchors on findPaths() so we can identify non-float Items.
*/
void QGraphicsAnchorLayoutPrivate::identifyFloatItems(const QSet<AnchorData *> &visited, Orientation orientation)
{
    QSet<QGraphicsLayoutItem *> nonFloating;

    foreach (const AnchorData *ad, visited)
        identifyNonFloatItems_helper(ad, &nonFloating);

    QSet<QGraphicsLayoutItem *> allItems;
    foreach (QGraphicsLayoutItem *item, items)
        allItems.insert(item);
    m_floatItems[orientation] = allItems - nonFloating;
}


/*!
 \internal

  Given an anchor, if it is an internal anchor and Normal we must mark it's item as non-float.
  If the anchor is Sequential or Parallel, we must iterate on its children recursively until we reach
  internal anchors (items).
*/
void QGraphicsAnchorLayoutPrivate::identifyNonFloatItems_helper(const AnchorData *ad, QSet<QGraphicsLayoutItem *> *nonFloatingItemsIdentifiedSoFar)
{
    Q_Q(QGraphicsAnchorLayout);

    switch(ad->type) {
    case AnchorData::Normal:
        if (ad->item && ad->item != q)
            nonFloatingItemsIdentifiedSoFar->insert(ad->item);
        break;
    case AnchorData::Sequential:
        foreach (const AnchorData *d, static_cast<const SequentialAnchorData *>(ad)->m_edges)
            identifyNonFloatItems_helper(d, nonFloatingItemsIdentifiedSoFar);
        break;
    case AnchorData::Parallel:
        identifyNonFloatItems_helper(static_cast<const ParallelAnchorData *>(ad)->firstEdge, nonFloatingItemsIdentifiedSoFar);
        identifyNonFloatItems_helper(static_cast<const ParallelAnchorData *>(ad)->secondEdge, nonFloatingItemsIdentifiedSoFar);
        break;
    }
}

/*!
  \internal

  Use the current vertices distance to calculate and set the geometry of
  each item.
*/
void QGraphicsAnchorLayoutPrivate::setItemsGeometries(const QRectF &geom)
{
    Q_Q(QGraphicsAnchorLayout);
    AnchorVertex *firstH, *secondH, *firstV, *secondV;

    qreal top;
    qreal left;
    qreal right;

    q->getContentsMargins(&left, &top, &right, 0);
    const Qt::LayoutDirection visualDir = visualDirection();
    if (visualDir == Qt::RightToLeft)
        qSwap(left, right);

    left += geom.left();
    top += geom.top();
    right = geom.right() - right;

    foreach (QGraphicsLayoutItem *item, items) {
        QRectF newGeom;
        QSizeF itemPreferredSize = item->effectiveSizeHint(Qt::PreferredSize);
        if (m_floatItems[Horizontal].contains(item)) {
            newGeom.setLeft(0);
            newGeom.setRight(itemPreferredSize.width());
        } else {
            firstH = internalVertex(item, Qt::AnchorLeft);
            secondH = internalVertex(item, Qt::AnchorRight);

            if (visualDir == Qt::LeftToRight) {
                newGeom.setLeft(left + firstH->distance);
                newGeom.setRight(left + secondH->distance);
            } else {
                newGeom.setLeft(right - secondH->distance);
                newGeom.setRight(right - firstH->distance);
            }
        }

        if (m_floatItems[Vertical].contains(item)) {
            newGeom.setTop(0);
            newGeom.setBottom(itemPreferredSize.height());
        } else {
            firstV = internalVertex(item, Qt::AnchorTop);
            secondV = internalVertex(item, Qt::AnchorBottom);

            newGeom.setTop(top + firstV->distance);
            newGeom.setBottom(top + secondV->distance);
        }

        item->setGeometry(newGeom);
    }
}

/*!
  \internal

  Calculate the position of each vertex based on the paths to each of
  them as well as the current edges sizes.
*/
void QGraphicsAnchorLayoutPrivate::calculateVertexPositions(
    QGraphicsAnchorLayoutPrivate::Orientation orientation)
{
    QQueue<QPair<AnchorVertex *, AnchorVertex *> > queue;
    QSet<AnchorVertex *> visited;

    // Get root vertex
    AnchorVertex *root = layoutFirstVertex[orientation];

    root->distance = 0;
    visited.insert(root);

    // Add initial edges to the queue
    foreach (AnchorVertex *v, graph[orientation].adjacentVertices(root)) {
        queue.enqueue(qMakePair(root, v));
    }

    // Do initial calculation required by "interpolateEdge()"
    setupEdgesInterpolation(orientation);

    // Traverse the graph and calculate vertex positions
    while (!queue.isEmpty()) {
        QPair<AnchorVertex *, AnchorVertex *> pair = queue.dequeue();
        AnchorData *edge = graph[orientation].edgeData(pair.first, pair.second);

        if (visited.contains(pair.second))
            continue;

        visited.insert(pair.second);
        interpolateEdge(pair.first, edge);

        QList<AnchorVertex *> adjacents = graph[orientation].adjacentVertices(pair.second);
        for (int i = 0; i < adjacents.count(); ++i) {
            if (!visited.contains(adjacents.at(i)))
                queue.enqueue(qMakePair(pair.second, adjacents.at(i)));
        }
    }
}

/*!
  \internal

  Calculate interpolation parameters based on current Layout Size.
  Must be called once before calling "interpolateEdgeSize()" for
  the edges.
*/
void QGraphicsAnchorLayoutPrivate::setupEdgesInterpolation(
    Orientation orientation)
{
    Q_Q(QGraphicsAnchorLayout);

    qreal current;
    current = (orientation == Horizontal) ? q->contentsRect().width() : q->contentsRect().height();

    QPair<Interval, qreal> result;
    result = getFactor(current,
                       sizeHints[orientation][Qt::MinimumSize],
                       sizeHints[orientation][Qt::PreferredSize],
                       sizeHints[orientation][Qt::PreferredSize],
                       sizeHints[orientation][Qt::PreferredSize],
                       sizeHints[orientation][Qt::MaximumSize]);

    interpolationInterval[orientation] = result.first;
    interpolationProgress[orientation] = result.second;
}

/*!
    \internal

    Calculate the current Edge size based on the current Layout size and the
    size the edge is supposed to have when the layout is at its:

    - minimum size,
    - preferred size,
    - maximum size.

    These three key values are calculated in advance using linear
    programming (more expensive) or the simplification algorithm, then
    subsequential resizes of the parent layout require a simple
    interpolation.
*/
void QGraphicsAnchorLayoutPrivate::interpolateEdge(AnchorVertex *base, AnchorData *edge)
{
    const Orientation orientation = Orientation(edge->orientation);
    const QPair<Interval, qreal> factor(interpolationInterval[orientation],
                                        interpolationProgress[orientation]);

    qreal edgeDistance = interpolate(factor, edge->sizeAtMinimum, edge->sizeAtPreferred,
                                     edge->sizeAtPreferred, edge->sizeAtPreferred,
                                     edge->sizeAtMaximum);

    Q_ASSERT(edge->from == base || edge->to == base);

    // Calculate the distance for the vertex opposite to the base
    if (edge->from == base) {
        edge->to->distance = base->distance + edgeDistance;
    } else {
        edge->from->distance = base->distance - edgeDistance;
    }
}

bool QGraphicsAnchorLayoutPrivate::solveMinMax(const QList<QSimplexConstraint *> &constraints,
                                               GraphPath path, qreal *min, qreal *max)
{
    QSimplex simplex;
    bool feasible = simplex.setConstraints(constraints);
    if (feasible) {
        // Obtain the objective constraint
        QSimplexConstraint objective;
        QSet<AnchorData *>::const_iterator iter;
        for (iter = path.positives.constBegin(); iter != path.positives.constEnd(); ++iter)
            objective.variables.insert(*iter, 1.0);

        for (iter = path.negatives.constBegin(); iter != path.negatives.constEnd(); ++iter)
            objective.variables.insert(*iter, -1.0);

        const qreal objectiveOffset = (path.positives.count() - path.negatives.count()) * g_offset;
        simplex.setObjective(&objective);

        // Calculate minimum values
        *min = simplex.solveMin() - objectiveOffset;

        // Save sizeAtMinimum results
        QList<AnchorData *> variables = getVariables(constraints);
        for (int i = 0; i < variables.size(); ++i) {
            AnchorData *ad = static_cast<AnchorData *>(variables.at(i));
            ad->sizeAtMinimum = ad->result - g_offset;
        }

        // Calculate maximum values
        *max = simplex.solveMax() - objectiveOffset;

        // Save sizeAtMaximum results
        for (int i = 0; i < variables.size(); ++i) {
            AnchorData *ad = static_cast<AnchorData *>(variables.at(i));
            ad->sizeAtMaximum = ad->result - g_offset;
        }
    }
    return feasible;
}

enum slackType { Grower = -1, Shrinker = 1 };
static QPair<QSimplexVariable *, QSimplexConstraint *> createSlack(QSimplexConstraint *sizeConstraint,
                                                                   qreal interval, slackType type)
{
    QSimplexVariable *slack = new QSimplexVariable;
    sizeConstraint->variables.insert(slack, type);

    QSimplexConstraint *limit = new QSimplexConstraint;
    limit->variables.insert(slack, 1.0);
    limit->ratio = QSimplexConstraint::LessOrEqual;
    limit->constant = interval;

    return qMakePair(slack, limit);
}

bool QGraphicsAnchorLayoutPrivate::solvePreferred(const QList<QSimplexConstraint *> &constraints,
                                                  const QList<AnchorData *> &variables)
{
    QList<QSimplexConstraint *> preferredConstraints;
    QList<QSimplexVariable *> preferredVariables;
    QSimplexConstraint objective;

    // Fill the objective coefficients for this variable. In the
    // end the objective function will be
    //
    //     z = n * (A_shrinker_hard + A_grower_hard + B_shrinker_hard + B_grower_hard + ...) +
    //             (A_shrinker_soft + A_grower_soft + B_shrinker_soft + B_grower_soft + ...)
    //
    // where n is the number of variables that have
    // slacks. Note that here we use the number of variables
    // as coefficient, this is to mark the "shrinker slack
    // variable" less likely to get value than the "grower
    // slack variable".

    // This will fill the values for the structural constraints
    // and we now fill the values for the slack constraints (one per variable),
    // which have this form (the constant A_pref was set when creating the slacks):
    //
    //      A + A_shrinker_hard + A_shrinker_soft - A_grower_hard - A_grower_soft = A_pref
    //
    for (int i = 0; i < variables.size(); ++i) {
        AnchorData *ad = variables.at(i);

        // The layout original structure anchors are not relevant in preferred size calculation
        if (ad->isLayoutAnchor)
            continue;

        // By default, all variables are equal to their preferred size. If they have room to
        // grow or shrink, such flexibility will be added by the additional variables below.
        QSimplexConstraint *sizeConstraint = new QSimplexConstraint;
        preferredConstraints += sizeConstraint;
        sizeConstraint->variables.insert(ad, 1.0);
        sizeConstraint->constant = ad->prefSize + g_offset;

        // Can easily shrink
        QPair<QSimplexVariable *, QSimplexConstraint *> slack;
        const qreal softShrinkInterval = ad->prefSize - ad->minPrefSize;
        if (softShrinkInterval) {
            slack = createSlack(sizeConstraint, softShrinkInterval, Shrinker);
            preferredVariables += slack.first;
            preferredConstraints += slack.second;

            // Add to objective with ratio == 1 (soft)
            objective.variables.insert(slack.first, 1.0);
        }

        // Can easily grow
        const qreal softGrowInterval = ad->maxPrefSize - ad->prefSize;
        if (softGrowInterval) {
            slack = createSlack(sizeConstraint, softGrowInterval, Grower);
            preferredVariables += slack.first;
            preferredConstraints += slack.second;

            // Add to objective with ratio == 1 (soft)
            objective.variables.insert(slack.first, 1.0);
        }

        // Can shrink if really necessary
        const qreal hardShrinkInterval = ad->minPrefSize - ad->minSize;
        if (hardShrinkInterval) {
            slack = createSlack(sizeConstraint, hardShrinkInterval, Shrinker);
            preferredVariables += slack.first;
            preferredConstraints += slack.second;

            // Add to objective with ratio == N (hard)
            objective.variables.insert(slack.first, variables.size());
        }

        // Can grow if really necessary
        const qreal hardGrowInterval = ad->maxSize - ad->maxPrefSize;
        if (hardGrowInterval) {
            slack = createSlack(sizeConstraint, hardGrowInterval, Grower);
            preferredVariables += slack.first;
            preferredConstraints += slack.second;

            // Add to objective with ratio == N (hard)
            objective.variables.insert(slack.first, variables.size());
        }
    }

    QSimplex *simplex = new QSimplex;
    bool feasible = simplex->setConstraints(constraints + preferredConstraints);
    if (feasible) {
        simplex->setObjective(&objective);

        // Calculate minimum values
        simplex->solveMin();

        // Save sizeAtPreferred results
        for (int i = 0; i < variables.size(); ++i) {
            AnchorData *ad = variables.at(i);
            ad->sizeAtPreferred = ad->result - g_offset;
        }
    }

    // Make sure we delete the simplex solver -before- we delete the
    // constraints used by it.
    delete simplex;

    // Delete constraints and variables we created.
    qDeleteAll(preferredConstraints);
    qDeleteAll(preferredVariables);

    return feasible;
}

/*!
    \internal
    Returns \c true if there are no arrangement that satisfies all constraints.
    Otherwise returns \c false.

    \sa addAnchor()
*/
bool QGraphicsAnchorLayoutPrivate::hasConflicts() const
{
    QGraphicsAnchorLayoutPrivate *that = const_cast<QGraphicsAnchorLayoutPrivate*>(this);
    that->calculateGraphs();

    bool floatConflict = !m_floatItems[0].isEmpty() || !m_floatItems[1].isEmpty();

    return graphHasConflicts[0] || graphHasConflicts[1] || floatConflict;
}

#ifdef QT_DEBUG
void QGraphicsAnchorLayoutPrivate::dumpGraph(const QString &name)
{
    QFile file(QString::fromLatin1("anchorlayout.%1.dot").arg(name));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        qWarning("Could not write to %s", file.fileName().toLocal8Bit().constData());

    QString str = QString::fromLatin1("digraph anchorlayout {\nnode [shape=\"rect\"]\n%1}");
    QString dotContents = graph[0].serializeToDot();
    dotContents += graph[1].serializeToDot();
    file.write(str.arg(dotContents).toLocal8Bit());

    file.close();
}
#endif

QT_END_NAMESPACE
#endif //QT_NO_GRAPHICSVIEW
