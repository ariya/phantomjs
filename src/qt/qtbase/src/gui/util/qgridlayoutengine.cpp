/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qglobal.h"

#ifndef QT_NO_GRAPHICSVIEW

#include <math.h>

#include "qgridlayoutengine_p.h"
#include "qvarlengtharray.h"

#include <QtDebug>
#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE

template <typename T>
static void insertOrRemoveItems(QVector<T> &items, int index, int delta)
{
    int count = items.count();
    if (index < count) {
        if (delta > 0) {
            items.insert(index, delta, T());
        } else if (delta < 0) {
            items.remove(index, qMin(-delta, count - index));
        }
    }
}

static qreal growthFactorBelowPreferredSize(qreal desired, qreal sumAvailable, qreal sumDesired)
{
    Q_ASSERT(sumDesired != 0.0);
    return desired * qPow(sumAvailable / sumDesired, desired / sumDesired);
}

static qreal fixedDescent(qreal descent, qreal ascent, qreal targetSize)
{
    if (descent < 0.0)
        return -1.0;

    Q_ASSERT(descent >= 0.0);
    Q_ASSERT(ascent >= 0.0);
    Q_ASSERT(targetSize >= ascent + descent);

    qreal extra = targetSize - (ascent + descent);
    return descent + (extra / 2.0);
}

static qreal compare(const QGridLayoutBox &box1, const QGridLayoutBox &box2, int which)
{
    qreal size1 = box1.q_sizes(which);
    qreal size2 = box2.q_sizes(which);

    if (which == MaximumSize) {
        return size2 - size1;
    } else {
        return size1 - size2;
    }
}

void QGridLayoutBox::add(const QGridLayoutBox &other, int stretch, qreal spacing)
{
    Q_ASSERT(q_minimumDescent < 0.0);

    q_minimumSize += other.q_minimumSize + spacing;
    q_preferredSize += other.q_preferredSize + spacing;
    q_maximumSize += ((stretch == 0) ? other.q_preferredSize : other.q_maximumSize) + spacing;
}

void QGridLayoutBox::combine(const QGridLayoutBox &other)
{
    q_minimumDescent = qMax(q_minimumDescent, other.q_minimumDescent);
    q_minimumAscent = qMax(q_minimumAscent, other.q_minimumAscent);

    q_minimumSize = qMax(q_minimumAscent + q_minimumDescent,
                         qMax(q_minimumSize, other.q_minimumSize));
    qreal maxMax;
    if (q_maximumSize == FLT_MAX && other.q_maximumSize != FLT_MAX)
        maxMax = other.q_maximumSize;
    else if (other.q_maximumSize == FLT_MAX && q_maximumSize != FLT_MAX)
        maxMax = q_maximumSize;
    else
        maxMax = qMax(q_maximumSize, other.q_maximumSize);

    q_maximumSize = qMax(q_minimumSize, maxMax);
    q_preferredSize = qBound(q_minimumSize, qMax(q_preferredSize, other.q_preferredSize),
                             q_maximumSize);
}

void QGridLayoutBox::normalize()
{
    q_maximumSize = qMax(qreal(0.0), q_maximumSize);
    q_minimumSize = qBound(qreal(0.0), q_minimumSize, q_maximumSize);
    q_preferredSize = qBound(q_minimumSize, q_preferredSize, q_maximumSize);
    q_minimumDescent = qMin(q_minimumDescent, q_minimumSize);

    Q_ASSERT((q_minimumDescent < 0.0) == (q_minimumAscent < 0.0));
}

#ifdef QGRIDLAYOUTENGINE_DEBUG
void QGridLayoutBox::dump(int indent) const
{
    qDebug("%*sBox (%g <= %g <= %g [%g/%g])", indent, "", q_minimumSize, q_preferredSize,
           q_maximumSize, q_minimumAscent, q_minimumDescent);
}
#endif

bool operator==(const QGridLayoutBox &box1, const QGridLayoutBox &box2)
{
    for (int i = 0; i < NSizes; ++i) {
        if (box1.q_sizes(i) != box2.q_sizes(i))
            return false;
    }
    return box1.q_minimumDescent == box2.q_minimumDescent
           && box1.q_minimumAscent == box2.q_minimumAscent;
}

void QGridLayoutRowData::reset(int count)
{
    ignore.fill(false, count);
    boxes.fill(QGridLayoutBox(), count);
    multiCellMap.clear();
    stretches.fill(0, count);
    spacings.fill(0.0, count);
    hasIgnoreFlag = false;
}

void QGridLayoutRowData::distributeMultiCells(const QGridLayoutRowInfo &rowInfo)
{
    MultiCellMap::const_iterator i = multiCellMap.constBegin();
    for (; i != multiCellMap.constEnd(); ++i) {
        int start = i.key().first;
        int span = i.key().second;
        int end = start + span;
        const QGridLayoutBox &box = i.value().q_box;
        int stretch = i.value().q_stretch;

        QGridLayoutBox totalBox = this->totalBox(start, end);
        QVarLengthArray<QGridLayoutBox> extras(span);
        QVarLengthArray<qreal> dummy(span);
        QVarLengthArray<qreal> newSizes(span);

        for (int j = 0; j < NSizes; ++j) {
            qreal extra = compare(box, totalBox, j);
            if (extra > 0.0) {
                calculateGeometries(start, end, box.q_sizes(j), dummy.data(), newSizes.data(),
                                    0, totalBox, rowInfo);

                for (int k = 0; k < span; ++k)
                    extras[k].q_sizes(j) = newSizes[k];
            }
        }

        for (int k = 0; k < span; ++k) {
            boxes[start + k].combine(extras[k]);
            if (stretch != 0)
                stretches[start + k] = qMax(stretches[start + k], stretch);
        }
    }
    multiCellMap.clear();
}

void QGridLayoutRowData::calculateGeometries(int start, int end, qreal targetSize, qreal *positions,
                                             qreal *sizes, qreal *descents,
                                             const QGridLayoutBox &totalBox,
                                             const QGridLayoutRowInfo &rowInfo)
{
    Q_ASSERT(end > start);

    targetSize = qMax(totalBox.q_minimumSize, targetSize);

    int n = end - start;
    QVarLengthArray<qreal> newSizes(n);
    QVarLengthArray<qreal> factors(n);
    qreal sumFactors = 0.0;
    int sumStretches = 0;
    qreal sumAvailable;

    for (int i = 0; i < n; ++i) {
        if (stretches[start + i] > 0)
            sumStretches += stretches[start + i];
    }

    if (targetSize < totalBox.q_preferredSize) {
        stealBox(start, end, MinimumSize, positions, sizes);

        sumAvailable = targetSize - totalBox.q_minimumSize;
        if (sumAvailable > 0.0) {
            qreal sumDesired = totalBox.q_preferredSize - totalBox.q_minimumSize;

            for (int i = 0; i < n; ++i) {
                if (ignore.testBit(start + i)) {
                    factors[i] = 0.0;
                    continue;
                }

                const QGridLayoutBox &box = boxes.at(start + i);
                qreal desired = box.q_preferredSize - box.q_minimumSize;
                factors[i] = growthFactorBelowPreferredSize(desired, sumAvailable, sumDesired);
                sumFactors += factors[i];
            }

            for (int i = 0; i < n; ++i) {
                Q_ASSERT(sumFactors > 0.0);
                qreal delta = sumAvailable * factors[i] / sumFactors;
                newSizes[i] = sizes[i] + delta;
            }
        }
    } else {
        bool isLargerThanMaximum = (targetSize > totalBox.q_maximumSize);
        if (isLargerThanMaximum) {
            stealBox(start, end, MaximumSize, positions, sizes);
            sumAvailable = targetSize - totalBox.q_maximumSize;
        } else {
            stealBox(start, end, PreferredSize, positions, sizes);
            sumAvailable = targetSize - totalBox.q_preferredSize;
        }

        if (sumAvailable > 0.0) {
            qreal sumCurrentAvailable = sumAvailable;
            bool somethingHasAMaximumSize = false;

            qreal sumSizes = 0.0;
            for (int i = 0; i < n; ++i)
                sumSizes += sizes[i];

            for (int i = 0; i < n; ++i) {
                if (ignore.testBit(start + i)) {
                    newSizes[i] = 0.0;
                    factors[i] = 0.0;
                    continue;
                }

                const QGridLayoutBox &box = boxes.at(start + i);
                qreal boxSize;

                qreal desired;
                if (isLargerThanMaximum) {
                    boxSize = box.q_maximumSize;
                    desired = rowInfo.boxes.value(start + i).q_maximumSize - boxSize;
                } else {
                    boxSize = box.q_preferredSize;
                    desired = box.q_maximumSize - boxSize;
                }
                if (desired == 0.0) {
                    newSizes[i] = sizes[i];
                    factors[i] = 0.0;
                } else {
                    Q_ASSERT(desired > 0.0);

                    int stretch = stretches[start + i];
                    if (sumStretches == 0) {
                        if (hasIgnoreFlag || sizes[i] == 0.0) {
                            factors[i] = (stretch < 0) ? 1.0 : 0.0;
                        } else {
                            factors[i] = (stretch < 0) ? sizes[i] : 0.0;
                        }
                    } else if (stretch == sumStretches) {
                        factors[i] = 1.0;
                    } else if (stretch <= 0) {
                        factors[i] = 0.0;
                    } else {
                        qreal ultimateSize;
                        qreal ultimateSumSizes;
                        qreal x = ((stretch * sumSizes)
                                   - (sumStretches * boxSize))
                                  / (sumStretches - stretch);
                        if (x >= 0.0) {
                            ultimateSize = boxSize + x;
                            ultimateSumSizes = sumSizes + x;
                        } else {
                            ultimateSize = boxSize;
                            ultimateSumSizes = (sumStretches * boxSize)
                                                        / stretch;
                        }

                        /*
                            We multiply these by 1.5 to give some space for a smooth transition
                            (at the expense of the stretch factors, which are not fully respected
                            during the transition).
                        */
                        ultimateSize = ultimateSize * 3 / 2;
                        ultimateSumSizes = ultimateSumSizes * 3 / 2;

                        qreal beta = ultimateSumSizes - sumSizes;
                        if (!beta) {
                            factors[i] = 1;
                        } else {
                            qreal alpha = qMin(sumCurrentAvailable, beta);
                            qreal ultimateFactor = (stretch * ultimateSumSizes / sumStretches)
                                                   - (boxSize);
                            qreal transitionalFactor = sumCurrentAvailable * (ultimateSize - boxSize) / beta;

                            factors[i] = ((alpha * ultimateFactor)
                                          + ((beta - alpha) * transitionalFactor)) / beta;
                        }

                    }
                    sumFactors += factors[i];
                    if (desired < sumCurrentAvailable)
                        somethingHasAMaximumSize = true;

                    newSizes[i] = -1.0;
                }
            }

            bool keepGoing = somethingHasAMaximumSize;
            while (keepGoing) {
                keepGoing = false;

                for (int i = 0; i < n; ++i) {
                    if (newSizes[i] >= 0.0)
                        continue;

                    qreal maxBoxSize;
                    if (isLargerThanMaximum)
                        maxBoxSize = rowInfo.boxes.value(start + i).q_maximumSize;
                    else
                        maxBoxSize = boxes.at(start + i).q_maximumSize;

                    qreal avail = sumCurrentAvailable * factors[i] / sumFactors;
                    if (sizes[i] + avail >= maxBoxSize) {
                        newSizes[i] = maxBoxSize;
                        sumCurrentAvailable -= maxBoxSize - sizes[i];
                        sumFactors -= factors[i];
                        keepGoing = (sumCurrentAvailable > 0.0);
                        if (!keepGoing)
                            break;
                    }
                }
            }

            for (int i = 0; i < n; ++i) {
                if (newSizes[i] < 0.0) {
                    qreal delta = (sumFactors == 0.0) ? 0.0
                                                      : sumCurrentAvailable * factors[i] / sumFactors;
                    newSizes[i] = sizes[i] + delta;
                }
            }
        }
    }

    if (sumAvailable > 0) {
        qreal offset = 0;
        for (int i = 0; i < n; ++i) {
            qreal delta = newSizes[i] - sizes[i];
            positions[i] += offset;
            sizes[i] += delta;
            offset += delta;
        }

#if 0 // some "pixel allocation"
        int surplus = targetSize - (positions[n - 1] + sizes[n - 1]);
        Q_ASSERT(surplus >= 0 && surplus <= n);

        int prevSurplus = -1;
        while (surplus > 0 && surplus != prevSurplus) {
            prevSurplus = surplus;

            int offset = 0;
            for (int i = 0; i < n; ++i) {
                const QGridLayoutBox &box = boxes.at(start + i);
                int delta = (!ignore.testBit(start + i) && surplus > 0
                             && factors[i] > 0 && sizes[i] < box.q_maximumSize)
                    ? 1 : 0;

                positions[i] += offset;
                sizes[i] += delta;
                offset += delta;
                surplus -= delta;
            }
        }
        Q_ASSERT(surplus == 0);
#endif
    }

    if (descents) {
        for (int i = 0; i < n; ++i) {
            if (ignore.testBit(start + i))
                continue;
            const QGridLayoutBox &box = boxes.at(start + i);
            descents[i] = fixedDescent(box.q_minimumDescent, box.q_minimumAscent, sizes[i]);
        }
    }
}

QGridLayoutBox QGridLayoutRowData::totalBox(int start, int end) const
{
    QGridLayoutBox result;
    if (start < end) {
        result.q_maximumSize = 0.0;
        qreal nextSpacing = 0.0;
        for (int i = start; i < end; ++i) {
            if (ignore.testBit(i))
                continue;
            result.add(boxes.at(i), stretches.at(i), nextSpacing);
            nextSpacing = spacings.at(i);
        }
    }
    return result;
}

void QGridLayoutRowData::stealBox(int start, int end, int which, qreal *positions, qreal *sizes)
{
    qreal offset = 0.0;
    qreal nextSpacing = 0.0;

    for (int i = start; i < end; ++i) {
        qreal avail = 0.0;

        if (!ignore.testBit(i)) {
            const QGridLayoutBox &box = boxes.at(i);
            avail = box.q_sizes(which);
            offset += nextSpacing;
            nextSpacing = spacings.at(i);
        }

        *positions++ = offset;
        *sizes++ = avail;
        offset += avail;
    }
}

#ifdef QGRIDLAYOUTENGINE_DEBUG
void QGridLayoutRowData::dump(int indent) const
{
    qDebug("%*sData", indent, "");

    for (int i = 0; i < ignore.count(); ++i) {
        qDebug("%*s Row %d (stretch %d, spacing %g)", indent, "", i, stretches.at(i),
               spacings.at(i));
        if (ignore.testBit(i))
            qDebug("%*s  Ignored", indent, "");
        boxes.at(i).dump(indent + 2);
    }

    MultiCellMap::const_iterator it = multiCellMap.constBegin();
    while (it != multiCellMap.constEnd()) {
        qDebug("%*s Multi-cell entry <%d, %d> (stretch %d)", indent, "", it.key().first,
               it.key().second, it.value().q_stretch);
        it.value().q_box.dump(indent + 2);
    }
}
#endif

QGridLayoutItem::QGridLayoutItem(int row, int column, int rowSpan, int columnSpan,
                                 Qt::Alignment alignment)
    : q_alignment(alignment)
{
    q_firstRows[Hor] = column;
    q_firstRows[Ver] = row;
    q_rowSpans[Hor] = columnSpan;
    q_rowSpans[Ver] = rowSpan;
    q_stretches[Hor] = -1;
    q_stretches[Ver] = -1;
}

int QGridLayoutItem::firstRow(Qt::Orientation orientation) const
{
    return q_firstRows[orientation == Qt::Vertical];
}

int QGridLayoutItem::firstColumn(Qt::Orientation orientation) const
{
    return q_firstRows[orientation == Qt::Horizontal];
}

int QGridLayoutItem::lastRow(Qt::Orientation orientation) const
{
    return firstRow(orientation) + rowSpan(orientation) - 1;
}

int QGridLayoutItem::lastColumn(Qt::Orientation orientation) const
{
    return firstColumn(orientation) + columnSpan(orientation) - 1;
}

int QGridLayoutItem::rowSpan(Qt::Orientation orientation) const
{
    return q_rowSpans[orientation == Qt::Vertical];
}

int QGridLayoutItem::columnSpan(Qt::Orientation orientation) const
{
    return q_rowSpans[orientation == Qt::Horizontal];
}

void QGridLayoutItem::setFirstRow(int row, Qt::Orientation orientation)
{
    q_firstRows[orientation == Qt::Vertical] = row;
}

void QGridLayoutItem::setRowSpan(int rowSpan, Qt::Orientation orientation)
{
    q_rowSpans[orientation == Qt::Vertical] = rowSpan;
}

int QGridLayoutItem::stretchFactor(Qt::Orientation orientation) const
{
    int stretch = q_stretches[orientation == Qt::Vertical];
    if (stretch >= 0)
        return stretch;

    QLayoutPolicy::Policy policy = sizePolicy(orientation);

    if (policy & QLayoutPolicy::ExpandFlag) {
        return 1;
    } else if (policy & QLayoutPolicy::GrowFlag) {
        return -1;  // because we max it up
    } else {
        return 0;
    }
}

void QGridLayoutItem::setStretchFactor(int stretch, Qt::Orientation orientation)
{
    Q_ASSERT(stretch >= 0); // ### deal with too big stretches
    q_stretches[orientation == Qt::Vertical] = stretch;
}

QLayoutPolicy::ControlTypes QGridLayoutItem::controlTypes(LayoutSide /*side*/) const
{
    return QLayoutPolicy::DefaultType;
}

QGridLayoutBox QGridLayoutItem::box(Qt::Orientation orientation, qreal constraint) const
{
    QGridLayoutBox result;
    QLayoutPolicy::Policy policy = sizePolicy(orientation);

    if (orientation == Qt::Horizontal) {
        QSizeF constraintSize(-1.0, constraint);

        result.q_preferredSize = sizeHint(Qt::PreferredSize, constraintSize).width();

        if (policy & QLayoutPolicy::ShrinkFlag) {
            result.q_minimumSize = sizeHint(Qt::MinimumSize, constraintSize).width();
        } else {
            result.q_minimumSize = result.q_preferredSize;
        }

        if (policy & (QLayoutPolicy::GrowFlag | QLayoutPolicy::ExpandFlag)) {
            result.q_maximumSize = sizeHint(Qt::MaximumSize, constraintSize).width();
        } else {
            result.q_maximumSize = result.q_preferredSize;
        }
    } else {
        QSizeF constraintSize(constraint, -1.0);

        result.q_preferredSize = sizeHint(Qt::PreferredSize, constraintSize).height();

        if (policy & QLayoutPolicy::ShrinkFlag) {
            result.q_minimumSize = sizeHint(Qt::MinimumSize, constraintSize).height();
        } else {
            result.q_minimumSize = result.q_preferredSize;
        }

        if (policy & (QLayoutPolicy::GrowFlag | QLayoutPolicy::ExpandFlag)) {
            result.q_maximumSize = sizeHint(Qt::MaximumSize, constraintSize).height();
        } else {
            result.q_maximumSize = result.q_preferredSize;
        }

        if (alignment() & Qt::AlignBaseline) {
            result.q_minimumDescent = sizeHint(Qt::MinimumDescent, constraintSize).height();
            if (result.q_minimumDescent != -1.0) {
                const qreal minSizeHint = sizeHint(Qt::MinimumSize, constraintSize).height();
                result.q_minimumDescent -= (minSizeHint - result.q_minimumSize);
                result.q_minimumAscent = result.q_minimumSize - result.q_minimumDescent;
            }
        }
    }
    if (policy & QLayoutPolicy::IgnoreFlag)
        result.q_preferredSize = result.q_minimumSize;

    return result;
}

QRectF QGridLayoutItem::geometryWithin(qreal x, qreal y, qreal width, qreal height,
                                       qreal rowDescent, Qt::Alignment align) const
{
    const qreal cellWidth = width;
    const qreal cellHeight = height;

    QSizeF size = effectiveMaxSize(QSizeF(-1,-1));
    if (hasDynamicConstraint()) {
        if (dynamicConstraintOrientation() == Qt::Vertical) {
           if (size.width() > cellWidth)
               size = effectiveMaxSize(QSizeF(cellWidth, -1));
        } else if (size.height() > cellHeight) {
            size = effectiveMaxSize(QSizeF(-1, cellHeight));
        }
    }
    size = size.boundedTo(QSizeF(cellWidth, cellHeight));
    width = size.width();
    height = size.height();

    switch (align & Qt::AlignHorizontal_Mask) {
    case Qt::AlignHCenter:
        x += (cellWidth - width)/2;
        break;
    case Qt::AlignRight:
        x += cellWidth - width;
        break;
    default:
        break;
    }

    switch (align & Qt::AlignVertical_Mask) {
    case Qt::AlignVCenter:
        y += (cellHeight - height)/2;
        break;
    case Qt::AlignBottom:
        y += cellHeight - height;
        break;
    case Qt::AlignBaseline: {
        width = qMin(effectiveMaxSize(QSizeF(-1,-1)).width(), width);
        QGridLayoutBox vBox = box(Qt::Vertical);
        const qreal descent = vBox.q_minimumDescent;
        const qreal ascent = vBox.q_minimumSize - descent;
        y += (cellHeight - rowDescent - ascent);
        height = ascent + descent;
        break; }
    default:
        break;
    }
    return QRectF(x, y, width, height);
}

void QGridLayoutItem::transpose()
{
    qSwap(q_firstRows[Hor], q_firstRows[Ver]);
    qSwap(q_rowSpans[Hor], q_rowSpans[Ver]);
    qSwap(q_stretches[Hor], q_stretches[Ver]);
}

void QGridLayoutItem::insertOrRemoveRows(int row, int delta, Qt::Orientation orientation)
{
    int oldFirstRow = firstRow(orientation);
    if (oldFirstRow >= row) {
        setFirstRow(oldFirstRow + delta, orientation);
    } else if (lastRow(orientation) >= row) {
        setRowSpan(rowSpan(orientation) + delta, orientation);
    }
}
/*!
    \internal
    returns the effective maximumSize, will take the sizepolicy into
    consideration. (i.e. if sizepolicy does not have QLayoutPolicy::Grow, then
    maxSizeHint will be the preferredSize)
    Note that effectiveSizeHint does not take sizePolicy into consideration,
    (since it only evaluates the hints, as the name implies)
*/
QSizeF QGridLayoutItem::effectiveMaxSize(const QSizeF &constraint) const
{
    QSizeF size = constraint;
    bool vGrow = (sizePolicy(Qt::Vertical) & QLayoutPolicy::GrowFlag) == QLayoutPolicy::GrowFlag;
    bool hGrow = (sizePolicy(Qt::Horizontal) & QLayoutPolicy::GrowFlag) == QLayoutPolicy::GrowFlag;
    if (!vGrow || !hGrow) {
        QSizeF pref = sizeHint(Qt::PreferredSize, constraint);
        if (!vGrow)
            size.setHeight(pref.height());
        if (!hGrow)
            size.setWidth(pref.width());
    }

    if (!size.isValid()) {
        QSizeF maxSize = sizeHint(Qt::MaximumSize, size);
        if (size.width() == -1)
            size.setWidth(maxSize.width());
        if (size.height() == -1)
            size.setHeight(maxSize.height());
    }
    return size;
}

#ifdef QGRIDLAYOUTENGINE_DEBUG
void QGridLayoutItem::dump(int indent) const
{
    qDebug("%*s (%d, %d) %d x %d", indent, "", firstRow(), firstColumn(),   //###
           rowSpan(), columnSpan());

    if (q_stretches[Hor] >= 0)
        qDebug("%*s Horizontal stretch: %d", indent, "", q_stretches[Hor]);
    if (q_stretches[Ver] >= 0)
        qDebug("%*s Vertical stretch: %d", indent, "", q_stretches[Ver]);
    if (q_alignment != 0)
        qDebug("%*s Alignment: %x", indent, "", uint(q_alignment));
    qDebug("%*s Horizontal size policy: %x Vertical size policy: %x",
        indent, "", sizePolicy(Qt::Horizontal), sizePolicy(Qt::Vertical));
}
#endif

void QGridLayoutRowInfo::insertOrRemoveRows(int row, int delta)
{
    count += delta;

    insertOrRemoveItems(stretches, row, delta);
    insertOrRemoveItems(spacings, row, delta);
    insertOrRemoveItems(alignments, row, delta);
    insertOrRemoveItems(boxes, row, delta);
}

#ifdef QGRIDLAYOUTENGINE_DEBUG
void QGridLayoutRowInfo::dump(int indent) const
{
    qDebug("%*sInfo (count: %d)", indent, "", count);
    for (int i = 0; i < count; ++i) {
        QString message;

        if (stretches.value(i).value() >= 0)
            message += QString::fromLatin1(" stretch %1").arg(stretches.value(i).value());
        if (spacings.value(i).value() >= 0.0)
            message += QString::fromLatin1(" spacing %1").arg(spacings.value(i).value());
        if (alignments.value(i) != 0)
            message += QString::fromLatin1(" alignment %1").arg(int(alignments.value(i)), 16);

        if (!message.isEmpty() || boxes.value(i) != QGridLayoutBox()) {
            qDebug("%*s Row %d:%s", indent, "", i, qPrintable(message));
            if (boxes.value(i) != QGridLayoutBox())
                boxes.value(i).dump(indent + 1);
        }
    }
}
#endif

QGridLayoutEngine::QGridLayoutEngine(Qt::Alignment defaultAlignment)
{
    m_visualDirection = Qt::LeftToRight;
    m_defaultAlignment = defaultAlignment;
    invalidate();
}

int QGridLayoutEngine::rowCount(Qt::Orientation orientation) const
{
    return q_infos[orientation == Qt::Vertical].count;
}

int QGridLayoutEngine::columnCount(Qt::Orientation orientation) const
{
    return q_infos[orientation == Qt::Horizontal].count;
}

int QGridLayoutEngine::itemCount() const
{
    return q_items.count();
}

QGridLayoutItem *QGridLayoutEngine::itemAt(int index) const
{
    Q_ASSERT(index >= 0 && index < itemCount());
    return q_items.at(index);
}

int QGridLayoutEngine::effectiveFirstRow(Qt::Orientation orientation) const
{
    ensureEffectiveFirstAndLastRows();
    return q_cachedEffectiveFirstRows[orientation == Qt::Vertical];
}

int QGridLayoutEngine::effectiveLastRow(Qt::Orientation orientation) const
{
    ensureEffectiveFirstAndLastRows();
    return q_cachedEffectiveLastRows[orientation == Qt::Vertical];
}

void QGridLayoutEngine::setSpacing(qreal spacing, Qt::Orientations orientations)
{
    if (orientations & Qt::Horizontal)
        q_defaultSpacings[Hor].setUserValue(spacing);
    if (orientations & Qt::Vertical)
        q_defaultSpacings[Ver].setUserValue(spacing);

    invalidate();
}

qreal QGridLayoutEngine::spacing(Qt::Orientation orientation, const QAbstractLayoutStyleInfo *styleInfo) const
{
    if (!q_defaultSpacings[orientation == Qt::Vertical].isUser()) {
        qreal defaultSpacing = styleInfo->spacing(orientation);
        q_defaultSpacings[orientation == Qt::Vertical].setCachedValue(defaultSpacing);
    }
    return q_defaultSpacings[orientation == Qt::Vertical].value();
}

void QGridLayoutEngine::setRowSpacing(int row, qreal spacing, Qt::Orientation orientation)
{
    Q_ASSERT(row >= 0);

    QGridLayoutRowInfo &rowInfo = q_infos[orientation == Qt::Vertical];
    if (row >= rowInfo.spacings.count())
        rowInfo.spacings.resize(row + 1);
    if (spacing >= 0)
        rowInfo.spacings[row].setUserValue(spacing);
    else
        rowInfo.spacings[row] = QLayoutParameter<qreal>();
    invalidate();
}

qreal QGridLayoutEngine::rowSpacing(int row, Qt::Orientation orientation) const
{
    QLayoutParameter<qreal> spacing = q_infos[orientation == Qt::Vertical].spacings.value(row);
    if (!spacing.isDefault())
        return spacing.value();
    return q_defaultSpacings[orientation == Qt::Vertical].value();
}

void QGridLayoutEngine::setRowStretchFactor(int row, int stretch, Qt::Orientation orientation)
{
    Q_ASSERT(row >= 0);
    Q_ASSERT(stretch >= 0);

    maybeExpandGrid(row, -1, orientation);

    QGridLayoutRowInfo &rowInfo = q_infos[orientation == Qt::Vertical];
    if (row >= rowInfo.stretches.count())
        rowInfo.stretches.resize(row + 1);
    rowInfo.stretches[row].setUserValue(stretch);
}

int QGridLayoutEngine::rowStretchFactor(int row, Qt::Orientation orientation) const
{
    QStretchParameter stretch = q_infos[orientation == Qt::Vertical].stretches.value(row);
    if (!stretch.isDefault())
        return stretch.value();
    return 0;
}

void QGridLayoutEngine::setRowSizeHint(Qt::SizeHint which, int row, qreal size,
                                       Qt::Orientation orientation)
{
    Q_ASSERT(row >= 0);
    Q_ASSERT(size >= 0.0);

    maybeExpandGrid(row, -1, orientation);

    QGridLayoutRowInfo &rowInfo = q_infos[orientation == Qt::Vertical];
    if (row >= rowInfo.boxes.count())
        rowInfo.boxes.resize(row + 1);
    rowInfo.boxes[row].q_sizes(which) = size;
}

qreal QGridLayoutEngine::rowSizeHint(Qt::SizeHint which, int row, Qt::Orientation orientation) const
{
    return q_infos[orientation == Qt::Vertical].boxes.value(row).q_sizes(which);
}

void QGridLayoutEngine::setRowAlignment(int row, Qt::Alignment alignment,
                                        Qt::Orientation orientation)
{
    Q_ASSERT(row >= 0);

    maybeExpandGrid(row, -1, orientation);

    QGridLayoutRowInfo &rowInfo = q_infos[orientation == Qt::Vertical];
    if (row >= rowInfo.alignments.count())
        rowInfo.alignments.resize(row + 1);
    rowInfo.alignments[row] = alignment;
}

Qt::Alignment QGridLayoutEngine::rowAlignment(int row, Qt::Orientation orientation) const
{
    Q_ASSERT(row >= 0);
    return q_infos[orientation == Qt::Vertical].alignments.value(row);
}

Qt::Alignment QGridLayoutEngine::effectiveAlignment(const QGridLayoutItem *layoutItem) const
{
    Qt::Alignment align = layoutItem->alignment();
    if (!(align & Qt::AlignVertical_Mask)) {
        // no vertical alignment, respect the row alignment
        int y = layoutItem->firstRow();
        align |= (rowAlignment(y, Qt::Vertical) & Qt::AlignVertical_Mask);
        if (!(align & Qt::AlignVertical_Mask))
            align |= (m_defaultAlignment & Qt::AlignVertical_Mask);
    }
    if (!(align & Qt::AlignHorizontal_Mask)) {
        // no horizontal alignment, respect the column alignment
        int x = layoutItem->firstColumn();
        align |= (rowAlignment(x, Qt::Horizontal) & Qt::AlignHorizontal_Mask);
    }

    return align;
}

/*!
    \internal
    The \a index is only used by QGraphicsLinearLayout to ensure that itemAt() reflects the order
    of visual arrangement. Strictly speaking it does not have to, but most people expect it to.
    (And if it didn't we would have to add itemArrangedAt(int index) or something..)
 */
void QGridLayoutEngine::insertItem(QGridLayoutItem *item, int index)
{
    maybeExpandGrid(item->lastRow(), item->lastColumn());

    if (index == -1)
        q_items.append(item);
    else
        q_items.insert(index, item);

    for (int i = item->firstRow(); i <= item->lastRow(); ++i) {
        for (int j = item->firstColumn(); j <= item->lastColumn(); ++j) {
            if (itemAt(i, j))
                qWarning("QGridLayoutEngine::addItem: Cell (%d, %d) already taken", i, j);
            setItemAt(i, j, item);
        }
    }
}

void QGridLayoutEngine::addItem(QGridLayoutItem *item)
{
    insertItem(item, -1);
}

void QGridLayoutEngine::removeItem(QGridLayoutItem *item)
{
    Q_ASSERT(q_items.contains(item));

    invalidate();

    for (int i = item->firstRow(); i <= item->lastRow(); ++i) {
        for (int j = item->firstColumn(); j <= item->lastColumn(); ++j) {
            if (itemAt(i, j) == item)
                setItemAt(i, j, 0);
        }
    }

    q_items.removeAll(item);
}


QGridLayoutItem *QGridLayoutEngine::itemAt(int row, int column, Qt::Orientation orientation) const
{
    if (orientation == Qt::Horizontal)
        qSwap(row, column);
    if (uint(row) >= uint(rowCount()) || uint(column) >= uint(columnCount()))
        return 0;
    return q_grid.at((row * internalGridColumnCount()) + column);
}

void QGridLayoutEngine::invalidate()
{
    q_cachedEffectiveFirstRows[Hor] = -1;
    q_cachedEffectiveFirstRows[Ver] = -1;
    q_cachedEffectiveLastRows[Hor] = -1;
    q_cachedEffectiveLastRows[Ver] = -1;
    q_totalBoxesValid = false;
    q_sizeHintValid[Hor] = false;
    q_sizeHintValid[Ver] = false;
    q_cachedSize = QSizeF();
    q_cachedConstraintOrientation = UnknownConstraint;
}

static void visualRect(QRectF *geom, Qt::LayoutDirection dir, const QRectF &contentsRect)
{
    if (dir == Qt::RightToLeft)
        geom->moveRight(contentsRect.right() - (geom->left() - contentsRect.left()));
}

void QGridLayoutEngine::setGeometries(const QRectF &contentsGeometry, const QAbstractLayoutStyleInfo *styleInfo)
{
    if (rowCount() < 1 || columnCount() < 1)
        return;

    ensureGeometries(contentsGeometry.size(), styleInfo);

    for (int i = q_items.count() - 1; i >= 0; --i) {
        QGridLayoutItem *item = q_items.at(i);

        qreal x = q_xx[item->firstColumn()];
        qreal y = q_yy[item->firstRow()];
        qreal width = q_widths[item->lastColumn()];
        qreal height = q_heights[item->lastRow()];

        if (item->columnSpan() != 1)
            width += q_xx[item->lastColumn()] - x;
        if (item->rowSpan() != 1)
            height += q_yy[item->lastRow()] - y;

        QRectF geom = item->geometryWithin(contentsGeometry.x() + x, contentsGeometry.y() + y,
                                               width, height, q_descents[item->lastRow()], effectiveAlignment(item));
        visualRect(&geom, visualDirection(), contentsGeometry);
        item->setGeometry(geom);
    }
}

// ### candidate for deletion
QRectF QGridLayoutEngine::cellRect(const QRectF &contentsGeometry, int row, int column, int rowSpan,
                                   int columnSpan, const QAbstractLayoutStyleInfo *styleInfo) const
{
    if (uint(row) >= uint(rowCount()) || uint(column) >= uint(columnCount())
            || rowSpan < 1 || columnSpan < 1)
        return QRectF();

    ensureGeometries(contentsGeometry.size(), styleInfo);

    int lastColumn = qMax(column + columnSpan, columnCount()) - 1;
    int lastRow = qMax(row + rowSpan, rowCount()) - 1;

    qreal x = q_xx[column];
    qreal y = q_yy[row];
    qreal width = q_widths[lastColumn];
    qreal height = q_heights[lastRow];

    if (columnSpan != 1)
        width += q_xx[lastColumn] - x;
    if (rowSpan != 1)
        height += q_yy[lastRow] - y;

    return QRectF(contentsGeometry.x() + x, contentsGeometry.y() + y, width, height);
}

QSizeF QGridLayoutEngine::sizeHint(Qt::SizeHint which, const QSizeF &constraint,
                                   const QAbstractLayoutStyleInfo *styleInfo) const
{


    if (hasDynamicConstraint() && rowCount() > 0 && columnCount() > 0) {
        QGridLayoutBox sizehint_totalBoxes[NOrientations];
        bool sizeHintCalculated = false;
        if (constraintOrientation() == Qt::Vertical) {
            //We have items whose height depends on their width
            if (constraint.width() >= 0) {
                ensureColumnAndRowData(&q_columnData, &sizehint_totalBoxes[Hor], NULL, NULL, Qt::Horizontal, styleInfo);
                QVector<qreal> sizehint_xx;
                QVector<qreal> sizehint_widths;

                sizehint_xx.resize(columnCount());
                sizehint_widths.resize(columnCount());
                qreal width = constraint.width();
                //Calculate column widths and positions, and put results in q_xx.data() and q_widths.data() so that we can use this information as
                //constraints to find the row heights
                q_columnData.calculateGeometries(0, columnCount(), width, sizehint_xx.data(), sizehint_widths.data(),
                        0, sizehint_totalBoxes[Hor], q_infos[Hor]);
                ensureColumnAndRowData(&q_rowData, &sizehint_totalBoxes[Ver], sizehint_xx.data(), sizehint_widths.data(), Qt::Vertical, styleInfo);
                sizeHintCalculated = true;
            }
        } else {
            if (constraint.height() >= 0) {
                //We have items whose width depends on their height
                ensureColumnAndRowData(&q_rowData, &sizehint_totalBoxes[Ver], NULL, NULL, Qt::Vertical, styleInfo);
                QVector<qreal> sizehint_yy;
                QVector<qreal> sizehint_heights;

                sizehint_yy.resize(rowCount());
                sizehint_heights.resize(rowCount());
                qreal height = constraint.height();
                //Calculate row heights and positions, and put results in q_yy.data() and q_heights.data() so that we can use this information as
                //constraints to find the column widths
                q_rowData.calculateGeometries(0, rowCount(), height, sizehint_yy.data(), sizehint_heights.data(),
                        0, sizehint_totalBoxes[Ver], q_infos[Ver]);
                ensureColumnAndRowData(&q_columnData, &sizehint_totalBoxes[Hor], sizehint_yy.data(), sizehint_heights.data(), Qt::Horizontal, styleInfo);
                sizeHintCalculated = true;
            }
        }
        if (sizeHintCalculated)
            return QSizeF(sizehint_totalBoxes[Hor].q_sizes(which), sizehint_totalBoxes[Ver].q_sizes(which));
    }

    //No items with height for width, so it doesn't matter which order we do these in
    ensureColumnAndRowData(&q_columnData, &q_totalBoxes[Hor], NULL, NULL, Qt::Horizontal, styleInfo);
    ensureColumnAndRowData(&q_rowData, &q_totalBoxes[Ver], NULL, NULL, Qt::Vertical, styleInfo);
    return QSizeF(q_totalBoxes[Hor].q_sizes(which), q_totalBoxes[Ver].q_sizes(which));
}

QLayoutPolicy::ControlTypes QGridLayoutEngine::controlTypes(LayoutSide side) const
{
    Qt::Orientation orientation = (side == Top || side == Bottom) ? Qt::Vertical : Qt::Horizontal;
    int row = (side == Top || side == Left) ? effectiveFirstRow(orientation)
                                            : effectiveLastRow(orientation);
    QLayoutPolicy::ControlTypes result = 0;

    for (int column = columnCount(orientation) - 1; column >= 0; --column) {
        if (QGridLayoutItem *item = itemAt(row, column, orientation))
            result |= item->controlTypes(side);
    }
    return result;
}

void QGridLayoutEngine::transpose()
{
    invalidate();

    for (int i = q_items.count() - 1; i >= 0; --i)
        q_items.at(i)->transpose();

    qSwap(q_defaultSpacings[Hor], q_defaultSpacings[Ver]);
    qSwap(q_infos[Hor], q_infos[Ver]);

    regenerateGrid();
}

void QGridLayoutEngine::setVisualDirection(Qt::LayoutDirection direction)
{
    m_visualDirection = direction;
}

Qt::LayoutDirection QGridLayoutEngine::visualDirection() const
{
    return m_visualDirection;
}

#ifdef QGRIDLAYOUTENGINE_DEBUG
void QGridLayoutEngine::dump(int indent) const
{
    qDebug("%*sEngine", indent, "");

    qDebug("%*s Items (%d)", indent, "", q_items.count());
    int i;
    for (i = 0; i < q_items.count(); ++i)
        q_items.at(i)->dump(indent + 2);

    qDebug("%*s Grid (%d x %d)", indent, "", internalGridRowCount(),
           internalGridColumnCount());
    for (int row = 0; row < internalGridRowCount(); ++row) {
        QString message = QLatin1String("[ ");
        for (int column = 0; column < internalGridColumnCount(); ++column) {
            message += QString::number(q_items.indexOf(itemAt(row, column))).rightJustified(3);
            message += QLatin1Char(' ');
        }
        message += QLatin1Char(']');
        qDebug("%*s  %s", indent, "", qPrintable(message));
    }

    if (q_defaultSpacings[Hor].value() >= 0.0 || q_defaultSpacings[Ver].value() >= 0.0)
        qDebug("%*s Default spacings: %g %g", indent, "", q_defaultSpacings[Hor].value(),
               q_defaultSpacings[Ver].value());

    qDebug("%*s Column and row info", indent, "");
    q_infos[Hor].dump(indent + 2);
    q_infos[Ver].dump(indent + 2);

    qDebug("%*s Column and row data", indent, "");
    q_columnData.dump(indent + 2);
    q_rowData.dump(indent + 2);

    qDebug("%*s Geometries output", indent, "");
    QVector<qreal> *cellPos = &q_yy;
    for (int pass = 0; pass < 2; ++pass) {
        QString message;
        for (i = 0; i < cellPos->count(); ++i) {
            message += QLatin1String((message.isEmpty() ? "[" : ", "));
            message += QString::number(cellPos->at(i));
        }
        message += QLatin1Char(']');
        qDebug("%*s %s %s", indent, "", (pass == 0 ? "rows:" : "columns:"), qPrintable(message));
        cellPos = &q_xx;
    }
}
#endif

void QGridLayoutEngine::maybeExpandGrid(int row, int column, Qt::Orientation orientation)
{
    invalidate();   // ### move out of here?

    if (orientation == Qt::Horizontal)
        qSwap(row, column);

    if (row < rowCount() && column < columnCount())
        return;

    int oldGridRowCount = internalGridRowCount();
    int oldGridColumnCount = internalGridColumnCount();

    q_infos[Ver].count = qMax(row + 1, rowCount());
    q_infos[Hor].count = qMax(column + 1, columnCount());

    int newGridRowCount = internalGridRowCount();
    int newGridColumnCount = internalGridColumnCount();

    int newGridSize = newGridRowCount * newGridColumnCount;
    if (newGridSize != q_grid.count()) {
        q_grid.resize(newGridSize);

        if (newGridColumnCount != oldGridColumnCount) {
            for (int i = oldGridRowCount - 1; i >= 1; --i) {
                for (int j = oldGridColumnCount - 1; j >= 0; --j) {
                    int oldIndex = (i * oldGridColumnCount) + j;
                    int newIndex = (i * newGridColumnCount) + j;

                    Q_ASSERT(newIndex > oldIndex);
                    q_grid[newIndex] = q_grid[oldIndex];
                    q_grid[oldIndex] = 0;
                }
            }
        }
    }
}

void QGridLayoutEngine::regenerateGrid()
{
    q_grid.fill(0);

    for (int i = q_items.count() - 1; i >= 0; --i) {
        QGridLayoutItem *item = q_items.at(i);

        for (int j = item->firstRow(); j <= item->lastRow(); ++j) {
            for (int k = item->firstColumn(); k <= item->lastColumn(); ++k) {
                setItemAt(j, k, item);
            }
        }
    }
}

void QGridLayoutEngine::setItemAt(int row, int column, QGridLayoutItem *item)
{
    Q_ASSERT(row >= 0 && row < rowCount());
    Q_ASSERT(column >= 0 && column < columnCount());
    q_grid[(row * internalGridColumnCount()) + column] = item;
}

void QGridLayoutEngine::insertOrRemoveRows(int row, int delta, Qt::Orientation orientation)
{
    int oldRowCount = rowCount(orientation);
    Q_ASSERT(uint(row) <= uint(oldRowCount));

    invalidate();

    // appending rows (or columns) is easy
    if (row == oldRowCount && delta > 0) {
        maybeExpandGrid(oldRowCount + delta - 1, -1, orientation);
        return;
    }

    q_infos[orientation == Qt::Vertical].insertOrRemoveRows(row, delta);

    for (int i = q_items.count() - 1; i >= 0; --i)
        q_items.at(i)->insertOrRemoveRows(row, delta, orientation);

    q_grid.resize(internalGridRowCount() * internalGridColumnCount());
    regenerateGrid();
}

void QGridLayoutEngine::fillRowData(QGridLayoutRowData *rowData,
                                    const qreal *colPositions, const qreal *colSizes,
                                    Qt::Orientation orientation,
                                    const QAbstractLayoutStyleInfo *styleInfo) const
{
    const int ButtonMask = QLayoutPolicy::ButtonBox | QLayoutPolicy::PushButton;
    const QGridLayoutRowInfo &rowInfo = q_infos[orientation == Qt::Vertical];
    const QGridLayoutRowInfo &columnInfo = q_infos[orientation == Qt::Horizontal];
    LayoutSide top = (orientation == Qt::Vertical) ? Top : Left;
    LayoutSide bottom = (orientation == Qt::Vertical) ? Bottom : Right;

    const QLayoutParameter<qreal> &defaultSpacing = q_defaultSpacings[orientation == Qt::Vertical];
    qreal innerSpacing = styleInfo->spacing(orientation);
    if (innerSpacing >= 0.0)
        defaultSpacing.setCachedValue(innerSpacing);

    for (int row = 0; row < rowInfo.count; ++row) {
        bool rowIsEmpty = true;
        bool rowIsIdenticalToPrevious = (row > 0);

        for (int column = 0; column < columnInfo.count; ++column) {
            QGridLayoutItem *item = itemAt(row, column, orientation);

            if (rowIsIdenticalToPrevious && item != itemAt(row - 1, column, orientation))
                rowIsIdenticalToPrevious = false;

            if (item)
                rowIsEmpty = false;
        }

        if ((rowIsEmpty || rowIsIdenticalToPrevious)
                && rowInfo.spacings.value(row).isDefault()
                && rowInfo.stretches.value(row).isDefault()
                && rowInfo.boxes.value(row) == QGridLayoutBox())
            rowData->ignore.setBit(row, true);

        if (rowInfo.spacings.value(row).isUser()) {
            rowData->spacings[row] = rowInfo.spacings.at(row).value();
        } else if (!defaultSpacing.isDefault()) {
            rowData->spacings[row] = defaultSpacing.value();
        }

        rowData->stretches[row] = rowInfo.stretches.value(row).value();
    }

    struct RowAdHocData {
        int q_row;
        unsigned int q_hasButtons : 8;
        unsigned int q_hasNonButtons : 8;

        inline RowAdHocData() : q_row(-1), q_hasButtons(false), q_hasNonButtons(false) {}
        inline void init(int row) {
            this->q_row = row;
            q_hasButtons = false;
            q_hasNonButtons = false;
        }
        inline bool hasOnlyButtons() const { return q_hasButtons && !q_hasNonButtons; }
        inline bool hasOnlyNonButtons() const { return q_hasNonButtons && !q_hasButtons; }
    };
    RowAdHocData lastRowAdHocData;
    RowAdHocData nextToLastRowAdHocData;
    RowAdHocData nextToNextToLastRowAdHocData;

    rowData->hasIgnoreFlag = false;
    for (int row = 0; row < rowInfo.count; ++row) {
        if (rowData->ignore.testBit(row))
            continue;

        QGridLayoutBox &rowBox = rowData->boxes[row];
        if (styleInfo->isWindow()) {
            nextToNextToLastRowAdHocData = nextToLastRowAdHocData;
            nextToLastRowAdHocData = lastRowAdHocData;
            lastRowAdHocData.init(row);
        }

        bool userRowStretch = rowInfo.stretches.value(row).isUser();
        int &rowStretch = rowData->stretches[row];

        bool hasIgnoreFlag = true;
        for (int column = 0; column < columnInfo.count; ++column) {
            QGridLayoutItem *item = itemAt(row, column, orientation);
            if (item) {
                int itemRow = item->firstRow(orientation);
                int itemColumn = item->firstColumn(orientation);

                if (itemRow == row && itemColumn == column) {
                    int itemStretch = item->stretchFactor(orientation);
                    if (!(item->sizePolicy(orientation) & QLayoutPolicy::IgnoreFlag))
                        hasIgnoreFlag = false;
                    int itemRowSpan = item->rowSpan(orientation);

                    int effectiveRowSpan = 1;
                    for (int i = 1; i < itemRowSpan; ++i) {
                        if (!rowData->ignore.testBit(i + itemRow))
                            ++effectiveRowSpan;
                    }

                    QGridLayoutBox *box;
                    if (effectiveRowSpan == 1) {
                        box = &rowBox;
                        if (!userRowStretch && itemStretch != 0)
                            rowStretch = qMax(rowStretch, itemStretch);
                    } else {
                        QGridLayoutMultiCellData &multiCell =
                                rowData->multiCellMap[qMakePair(row, effectiveRowSpan)];
                        box = &multiCell.q_box;
                        multiCell.q_stretch = itemStretch;
                    }
                    // Items with constraints need to be passed the constraint
                    if (colSizes && colPositions && item->hasDynamicConstraint() && orientation == item->dynamicConstraintOrientation()) {
                        /* Get the width of the item by summing up the widths of the columns that it spans.
                         * We need to have already calculated the widths of the columns by calling
                         * q_columns->calculateGeometries() before hand and passing the value in the colSizes
                         * and colPositions parameters.
                         * The variable name is still colSizes even when it actually has the row sizes
                         */
                        qreal length = colSizes[item->lastColumn(orientation)];
                        if (item->columnSpan(orientation) != 1)
                            length += colPositions[item->lastColumn(orientation)] - colPositions[item->firstColumn(orientation)];
                        box->combine(item->box(orientation, length));
                    } else {
                        box->combine(item->box(orientation));
                    }

                    if (effectiveRowSpan == 1) {
                        QLayoutPolicy::ControlTypes controls = item->controlTypes(top);
                        if (controls & ButtonMask)
                            lastRowAdHocData.q_hasButtons = true;
                        if (controls & ~ButtonMask)
                            lastRowAdHocData.q_hasNonButtons = true;
                    }
                }
            }
        }
        if (row < rowInfo.boxes.count()) {
            QGridLayoutBox rowBoxInfo = rowInfo.boxes.at(row);
            rowBoxInfo.normalize();
            rowBox.q_minimumSize = qMax(rowBox.q_minimumSize, rowBoxInfo.q_minimumSize);
            rowBox.q_maximumSize = qMax(rowBox.q_minimumSize,
                                        (rowBoxInfo.q_maximumSize != FLT_MAX ?
                                        rowBoxInfo.q_maximumSize : rowBox.q_maximumSize));
            rowBox.q_preferredSize = qBound(rowBox.q_minimumSize,
                                            qMax(rowBox.q_preferredSize, rowBoxInfo.q_preferredSize),
                                            rowBox.q_maximumSize);
        }
        if (hasIgnoreFlag)
            rowData->hasIgnoreFlag = true;
    }

    /*
        Heuristic: Detect button boxes that don't use QLayoutPolicy::ButtonBox.
        This is somewhat ad hoc but it usually does the trick.
    */
    bool lastRowIsButtonBox = (lastRowAdHocData.hasOnlyButtons()
                               && nextToLastRowAdHocData.hasOnlyNonButtons());
    bool lastTwoRowsIsButtonBox = (lastRowAdHocData.hasOnlyButtons()
                                   && nextToLastRowAdHocData.hasOnlyButtons()
                                   && nextToNextToLastRowAdHocData.hasOnlyNonButtons()
                                   && orientation == Qt::Vertical);

    if (defaultSpacing.isDefault()) {
        int prevRow = -1;
        for (int row = 0; row < rowInfo.count; ++row) {
            if (rowData->ignore.testBit(row))
                continue;

            if (prevRow != -1 && !rowInfo.spacings.value(prevRow).isUser()) {
                qreal &rowSpacing = rowData->spacings[prevRow];
                for (int column = 0; column < columnInfo.count; ++column) {
                    QGridLayoutItem *item1 = itemAt(prevRow, column, orientation);
                    QGridLayoutItem *item2 = itemAt(row, column, orientation);

                    if (item1 && item2 && item1 != item2) {
                        QLayoutPolicy::ControlTypes controls1 = item1->controlTypes(bottom);
                        QLayoutPolicy::ControlTypes controls2 = item2->controlTypes(top);

                        if (controls2 & QLayoutPolicy::PushButton) {
                            if ((row == nextToLastRowAdHocData.q_row && lastTwoRowsIsButtonBox)
                                    || (row == lastRowAdHocData.q_row && lastRowIsButtonBox)) {
                                controls2 &= ~QLayoutPolicy::PushButton;
                                controls2 |= QLayoutPolicy::ButtonBox;
                            }
                        }

                        qreal spacing = styleInfo->combinedLayoutSpacing(controls1, controls2,
                                                                         orientation);
                        if (orientation == Qt::Horizontal) {
                            qreal width1 = rowData->boxes.at(prevRow).q_minimumSize;
                            qreal width2 = rowData->boxes.at(row).q_minimumSize;
                            QRectF rect1 = item1->geometryWithin(0.0, 0.0, width1, FLT_MAX, -1.0, effectiveAlignment(item1));
                            QRectF rect2 = item2->geometryWithin(0.0, 0.0, width2, FLT_MAX, -1.0, effectiveAlignment(item2));
                            spacing -= (width1 - (rect1.x() + rect1.width())) + rect2.x();
                        } else {
                            const QGridLayoutBox &box1 = rowData->boxes.at(prevRow);
                            const QGridLayoutBox &box2 = rowData->boxes.at(row);
                            qreal height1 = box1.q_minimumSize;
                            qreal height2 = box2.q_minimumSize;
                            qreal rowDescent1 = fixedDescent(box1.q_minimumDescent,
                                                             box1.q_minimumAscent, height1);
                            qreal rowDescent2 = fixedDescent(box2.q_minimumDescent,
                                                             box2.q_minimumAscent, height2);
                            QRectF rect1 = item1->geometryWithin(0.0, 0.0, FLT_MAX, height1,
                                                                 rowDescent1, effectiveAlignment(item1));
                            QRectF rect2 = item2->geometryWithin(0.0, 0.0, FLT_MAX, height2,
                                                                 rowDescent2, effectiveAlignment(item2));
                            spacing -= (height1 - (rect1.y() + rect1.height())) + rect2.y();
                        }
                        rowSpacing = qMax(spacing, rowSpacing);
                    }
                }
            }
            prevRow = row;
        }
    } else if (lastRowIsButtonBox || lastTwoRowsIsButtonBox) {
        /*
            Even for styles that define a uniform spacing, we cheat a
            bit and use the window margin as the spacing. This
            significantly improves the look of dialogs.
        */
        int prevRow = lastRowIsButtonBox ? nextToLastRowAdHocData.q_row
                                         : nextToNextToLastRowAdHocData.q_row;
        if (!defaultSpacing.isUser() && !rowInfo.spacings.value(prevRow).isUser()) {
            qreal windowMargin = styleInfo->windowMargin(orientation);
            qreal &rowSpacing = rowData->spacings[prevRow];
            rowSpacing = qMax(windowMargin, rowSpacing);
        }
    }
}

void QGridLayoutEngine::ensureEffectiveFirstAndLastRows() const
{
    if (q_cachedEffectiveFirstRows[Hor] == -1 && !q_items.isEmpty()) {
        int rowCount = this->rowCount();
        int columnCount = this->columnCount();

        q_cachedEffectiveFirstRows[Ver] = rowCount;
        q_cachedEffectiveFirstRows[Hor] = columnCount;
        q_cachedEffectiveLastRows[Ver] = -1;
        q_cachedEffectiveLastRows[Hor] = -1;

        for (int i = q_items.count() - 1; i >= 0; --i) {
            const QGridLayoutItem *item = q_items.at(i);

            for (int j = 0; j < NOrientations; ++j) {
                Qt::Orientation orientation = (j == Hor) ? Qt::Horizontal : Qt::Vertical;
                if (item->firstRow(orientation) < q_cachedEffectiveFirstRows[j])
                    q_cachedEffectiveFirstRows[j] = item->firstRow(orientation);
                if (item->lastRow(orientation) > q_cachedEffectiveLastRows[j])
                    q_cachedEffectiveLastRows[j] = item->lastRow(orientation);
            }
        }
    }
}

void QGridLayoutEngine::ensureColumnAndRowData(QGridLayoutRowData *rowData, QGridLayoutBox *totalBox,
                                               const qreal *colPositions, const qreal *colSizes,
                                               Qt::Orientation orientation,
                                               const QAbstractLayoutStyleInfo *styleInfo) const
{
    const int o = (orientation == Qt::Vertical ? Ver : Hor);
    if (q_sizeHintValid[o] && !colPositions && !colSizes) {
        if (totalBox != &q_totalBoxes[o])
            *totalBox = q_totalBoxes[o];
         return;
    }
    rowData->reset(rowCount(orientation));
    fillRowData(rowData, colPositions, colSizes, orientation, styleInfo);
    const QGridLayoutRowInfo &rowInfo = q_infos[orientation == Qt::Vertical];
    rowData->distributeMultiCells(rowInfo);
    *totalBox = rowData->totalBox(0, rowCount(orientation));

    if (!colPositions && !colSizes) {
        q_totalBoxes[o] = *totalBox;
        q_sizeHintValid[o] = true;
    }
}

/**
   returns false if the layout has contradicting constraints (i.e. some items with a horizontal
   constraint and other items with a vertical constraint)
 */
bool QGridLayoutEngine::ensureDynamicConstraint() const
{
    if (q_cachedConstraintOrientation == UnknownConstraint) {
        for (int i = q_items.count() - 1; i >= 0; --i) {
            QGridLayoutItem *item = q_items.at(i);
            if (item->hasDynamicConstraint()) {
                Qt::Orientation itemConstraintOrientation = item->dynamicConstraintOrientation();
                if (q_cachedConstraintOrientation == UnknownConstraint) {
                    q_cachedConstraintOrientation = itemConstraintOrientation;
                } else if (q_cachedConstraintOrientation != itemConstraintOrientation) {
                    q_cachedConstraintOrientation = UnfeasibleConstraint;
                    qWarning("QGridLayoutEngine: Unfeasible, cannot mix horizontal and"
                             " vertical constraint in the same layout");
                    return false;
                }
            }
        }
        if (q_cachedConstraintOrientation == UnknownConstraint)
            q_cachedConstraintOrientation = NoConstraint;
    }
    return true;
}

bool QGridLayoutEngine::hasDynamicConstraint() const
{
    if (!ensureDynamicConstraint())
        return false;
    return q_cachedConstraintOrientation != NoConstraint;
}

/*
 * return value is only valid if hasConstraint() returns \c true
 */
Qt::Orientation QGridLayoutEngine::constraintOrientation() const
{
    (void)ensureDynamicConstraint();
    return (Qt::Orientation)q_cachedConstraintOrientation;
}

void QGridLayoutEngine::ensureGeometries(const QSizeF &size,
                                         const QAbstractLayoutStyleInfo *styleInfo) const
{
    if (!styleInfo->hasChanged() && q_totalBoxesValid && q_cachedSize == size)
        return;

    q_totalBoxesValid = true;
    q_cachedSize = size;

    q_xx.resize(columnCount());
    q_widths.resize(columnCount());
    q_yy.resize(rowCount());
    q_heights.resize(rowCount());
    q_descents.resize(rowCount());

    if (constraintOrientation() != Qt::Horizontal) {
        //We might have items whose width depends on their height
        ensureColumnAndRowData(&q_columnData, &q_totalBoxes[Hor], NULL, NULL, Qt::Horizontal, styleInfo);
        //Calculate column widths and positions, and put results in q_xx.data() and q_widths.data() so that we can use this information as
        //constraints to find the row heights
        q_columnData.calculateGeometries(0, columnCount(), size.width(), q_xx.data(), q_widths.data(),
                0, q_totalBoxes[Hor], q_infos[Hor] );
        ensureColumnAndRowData(&q_rowData, &q_totalBoxes[Ver], q_xx.data(), q_widths.data(), Qt::Vertical, styleInfo);
        //Calculate row heights and positions, and put results in q_yy.data() and q_heights.data()
        q_rowData.calculateGeometries(0, rowCount(), size.height(), q_yy.data(), q_heights.data(),
                q_descents.data(), q_totalBoxes[Ver], q_infos[Ver]);
    } else {
        //We have items whose height depends on their width
        ensureColumnAndRowData(&q_rowData, &q_totalBoxes[Ver], NULL, NULL, Qt::Vertical, styleInfo);
        //Calculate row heights and positions, and put results in q_yy.data() and q_heights.data() so that we can use this information as
        //constraints to find the column widths
        q_rowData.calculateGeometries(0, rowCount(), size.height(), q_yy.data(), q_heights.data(),
                q_descents.data(), q_totalBoxes[Ver], q_infos[Ver]);
        ensureColumnAndRowData(&q_columnData, &q_totalBoxes[Hor], q_yy.data(), q_heights.data(), Qt::Horizontal, styleInfo);
        //Calculate row heights and positions, and put results in q_yy.data() and q_heights.data()
        q_columnData.calculateGeometries(0, columnCount(), size.width(), q_xx.data(), q_widths.data(),
                0, q_totalBoxes[Hor], q_infos[Hor]);
    }
}

QT_END_NAMESPACE

#endif //QT_NO_GRAPHICSVIEW
