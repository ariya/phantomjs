/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "qlayout.h"
#include "private/qlayoutengine_p.h"

#include "qvector.h"
#include "qwidget.h"

#include <qlist.h>
#include <qalgorithms.h>

#include <qdebug.h>

QT_BEGIN_NAMESPACE

//#define QLAYOUT_EXTRA_DEBUG

typedef qint64 Fixed64;
static inline Fixed64 toFixed(int i) { return (Fixed64)i * 256; }
static inline int fRound(Fixed64 i) {
    return (i % 256 < 128) ? i / 256 : 1 + i / 256;
}

/*
  This is the main workhorse of the QGridLayout. It portions out
  available space to the chain's children.

  The calculation is done in fixed point: "fixed" variables are
  scaled by a factor of 256.

  If the layout runs "backwards" (i.e. RightToLeft or Up) the layout
  is computed mirror-reversed, and it's the caller's responsibility
  do reverse the values before use.

  chain contains input and output parameters describing the geometry.
  count is the count of items in the chain; pos and space give the
  interval (relative to parentWidget topLeft).
*/
void qGeomCalc(QVector<QLayoutStruct> &chain, int start, int count,
               int pos, int space, int spacer)
{
    int cHint = 0;
    int cMin = 0;
    int cMax = 0;
    int sumStretch = 0;
    int sumSpacing = 0;

    bool wannaGrow = false; // anyone who really wants to grow?
    //    bool canShrink = false; // anyone who could be persuaded to shrink?

    bool allEmptyNonstretch = true;
    int pendingSpacing = -1;
    int spacerCount = 0;
    int i;

    for (i = start; i < start + count; i++) {
        QLayoutStruct *data = &chain[i];

        data->done = false;
        cHint += data->smartSizeHint();
        cMin += data->minimumSize;
        cMax += data->maximumSize;
        sumStretch += data->stretch;
        if (!data->empty) {
            /*
                Using pendingSpacing, we ensure that the spacing for the last
                (non-empty) item is ignored.
            */
            if (pendingSpacing >= 0) {
                sumSpacing += pendingSpacing;
                ++spacerCount;
            }
            pendingSpacing = data->effectiveSpacer(spacer);
        }
        wannaGrow = wannaGrow || data->expansive || data->stretch > 0;
        allEmptyNonstretch = allEmptyNonstretch && !wannaGrow && data->empty;
    }

    int extraspace = 0;

    if (space < cMin + sumSpacing) {
        /*
          Less space than minimumSize; take from the biggest first
        */

        int minSize = cMin + sumSpacing;

        // shrink the spacers proportionally
        if (spacer >= 0) {
            spacer = minSize > 0 ? spacer * space / minSize : 0;
            sumSpacing = spacer * spacerCount;
        }

        QList<int> list;

        for (i = start; i < start + count; i++)
            list << chain.at(i).minimumSize;

        qSort(list);

        int space_left = space - sumSpacing;

        int sum = 0;
        int idx = 0;
        int space_used=0;
        int current = 0;
        while (idx < count && space_used < space_left) {
            current = list.at(idx);
            space_used = sum + current * (count - idx);
            sum += current;
            ++idx;
        }
        --idx;
        int deficit = space_used - space_left;

        int items = count - idx;
        /*
         * If we truncate all items to "current", we would get "deficit" too many pixels. Therefore, we have to remove
         * deficit/items from each item bigger than maxval. The actual value to remove is deficitPerItem + remainder/items
         * "rest" is the accumulated error from using integer arithmetic.
        */
        int deficitPerItem = deficit/items;
        int remainder = deficit % items;
        int maxval = current - deficitPerItem;

        int rest = 0;
        for (i = start; i < start + count; i++) {
            int maxv = maxval;
            rest += remainder;
            if (rest >= items) {
                maxv--;
                rest-=items;
            }
            QLayoutStruct *data = &chain[i];
            data->size = qMin(data->minimumSize, maxv);
            data->done = true;
        }
    } else if (space < cHint + sumSpacing) {
        /*
          Less space than smartSizeHint(), but more than minimumSize.
          Currently take space equally from each, as in Qt 2.x.
          Commented-out lines will give more space to stretchier
          items.
        */
        int n = count;
        int space_left = space - sumSpacing;
        int overdraft = cHint - space_left;

        // first give to the fixed ones:
        for (i = start; i < start + count; i++) {
            QLayoutStruct *data = &chain[i];
            if (!data->done
                 && data->minimumSize >= data->smartSizeHint()) {
                data->size = data->smartSizeHint();
                data->done = true;
                space_left -= data->smartSizeHint();
                // sumStretch -= data->stretch;
                n--;
            }
        }
        bool finished = n == 0;
        while (!finished) {
            finished = true;
            Fixed64 fp_over = toFixed(overdraft);
            Fixed64 fp_w = 0;

            for (i = start; i < start+count; i++) {
                QLayoutStruct *data = &chain[i];
                if (data->done)
                    continue;
                // if (sumStretch <= 0)
                fp_w += fp_over / n;
                // else
                //    fp_w += (fp_over * data->stretch) / sumStretch;
                int w = fRound(fp_w);
                data->size = data->smartSizeHint() - w;
                fp_w -= toFixed(w); // give the difference to the next
                if (data->size < data->minimumSize) {
                    data->done = true;
                    data->size = data->minimumSize;
                    finished = false;
                    overdraft -= data->smartSizeHint() - data->minimumSize;
                    // sumStretch -= data->stretch;
                    n--;
                    break;
                }
            }
        }
    } else { // extra space
        int n = count;
        int space_left = space - sumSpacing;
        // first give to the fixed ones, and handle non-expansiveness
        for (i = start; i < start + count; i++) {
            QLayoutStruct *data = &chain[i];
            if (!data->done
                && (data->maximumSize <= data->smartSizeHint()
                    || (wannaGrow && !data->expansive && data->stretch == 0)
                    || (!allEmptyNonstretch && data->empty &&
                        !data->expansive && data->stretch == 0))) {
                data->size = data->smartSizeHint();
                data->done = true;
                space_left -= data->size;
                sumStretch -= data->stretch;
                n--;
            }
        }
        extraspace = space_left;

        /*
          Do a trial distribution and calculate how much it is off.
          If there are more deficit pixels than surplus pixels, give
          the minimum size items what they need, and repeat.
          Otherwise give to the maximum size items, and repeat.

          Paul Olav Tvete has a wonderful mathematical proof of the
          correctness of this principle, but unfortunately this
          comment is too small to contain it.
        */
        int surplus, deficit;
        do {
            surplus = deficit = 0;
            Fixed64 fp_space = toFixed(space_left);
            Fixed64 fp_w = 0;
            for (i = start; i < start + count; i++) {
                QLayoutStruct *data = &chain[i];
                if (data->done)
                    continue;
                extraspace = 0;
                if (sumStretch <= 0)
                    fp_w += fp_space / n;
                else
                    fp_w += (fp_space * data->stretch) / sumStretch;
                int w = fRound(fp_w);
                data->size = w;
                fp_w -= toFixed(w); // give the difference to the next
                if (w < data->smartSizeHint()) {
                    deficit +=  data->smartSizeHint() - w;
                } else if (w > data->maximumSize) {
                    surplus += w - data->maximumSize;
                }
            }
            if (deficit > 0 && surplus <= deficit) {
                // give to the ones that have too little
                for (i = start; i < start+count; i++) {
                    QLayoutStruct *data = &chain[i];
                    if (!data->done && data->size < data->smartSizeHint()) {
                        data->size = data->smartSizeHint();
                        data->done = true;
                        space_left -= data->smartSizeHint();
                        sumStretch -= data->stretch;
                        n--;
                    }
                }
            }
            if (surplus > 0 && surplus >= deficit) {
                // take from the ones that have too much
                for (i = start; i < start + count; i++) {
                    QLayoutStruct *data = &chain[i];
                    if (!data->done && data->size > data->maximumSize) {
                        data->size = data->maximumSize;
                        data->done = true;
                        space_left -= data->maximumSize;
                        sumStretch -= data->stretch;
                        n--;
                    }
                }
            }
        } while (n > 0 && surplus != deficit);
        if (n == 0)
            extraspace = space_left;
    }

    /*
      As a last resort, we distribute the unwanted space equally
      among the spacers (counting the start and end of the chain). We
      could, but don't, attempt a sub-pixel allocation of the extra
      space.
    */
    int extra = extraspace / (spacerCount + 2);
    int p = pos + extra;
    for (i = start; i < start+count; i++) {
        QLayoutStruct *data = &chain[i];
        data->pos = p;
        p += data->size;
        if (!data->empty)
            p += data->effectiveSpacer(spacer) + extra;
    }

#ifdef QLAYOUT_EXTRA_DEBUG
    qDebug() << "qGeomCalc" << "start" << start <<  "count" << count <<  "pos" << pos
             <<  "space" << space <<  "spacer" << spacer;
    for (i = start; i < start + count; ++i) {
        qDebug() << i << ':' << chain[i].minimumSize << chain[i].smartSizeHint()
                 << chain[i].maximumSize << "stretch" << chain[i].stretch
                 << "empty" << chain[i].empty << "expansive" << chain[i].expansive
                 << "spacing" << chain[i].spacing;
        qDebug() << "result pos" << chain[i].pos << "size" << chain[i].size;
    }
#endif
}

Q_GUI_EXPORT QSize qSmartMinSize(const QSize &sizeHint, const QSize &minSizeHint,
                                 const QSize &minSize, const QSize &maxSize,
                                 const QSizePolicy &sizePolicy)
{
    QSize s(0, 0);

    if (sizePolicy.horizontalPolicy() != QSizePolicy::Ignored) {
        if (sizePolicy.horizontalPolicy() & QSizePolicy::ShrinkFlag)
            s.setWidth(minSizeHint.width());
        else
            s.setWidth(qMax(sizeHint.width(), minSizeHint.width()));
    }

    if (sizePolicy.verticalPolicy() != QSizePolicy::Ignored) {
        if (sizePolicy.verticalPolicy() & QSizePolicy::ShrinkFlag) {
            s.setHeight(minSizeHint.height());
        } else {
            s.setHeight(qMax(sizeHint.height(), minSizeHint.height()));
        }
    }

    s = s.boundedTo(maxSize);
    if (minSize.width() > 0)
        s.setWidth(minSize.width());
    if (minSize.height() > 0)
        s.setHeight(minSize.height());

    return s.expandedTo(QSize(0,0));
}

Q_GUI_EXPORT QSize qSmartMinSize(const QWidgetItem *i)
{
    QWidget *w = ((QWidgetItem *)i)->widget();
    return qSmartMinSize(w->sizeHint(), w->minimumSizeHint(),
                            w->minimumSize(), w->maximumSize(),
                            w->sizePolicy());
}

Q_GUI_EXPORT QSize qSmartMinSize(const QWidget *w)
{
    return qSmartMinSize(w->sizeHint(), w->minimumSizeHint(),
                            w->minimumSize(), w->maximumSize(),
                            w->sizePolicy());
}

Q_GUI_EXPORT QSize qSmartMaxSize(const QSize &sizeHint,
                                 const QSize &minSize, const QSize &maxSize,
                                 const QSizePolicy &sizePolicy, Qt::Alignment align)
{
    if (align & Qt::AlignHorizontal_Mask && align & Qt::AlignVertical_Mask)
        return QSize(QLAYOUTSIZE_MAX, QLAYOUTSIZE_MAX);
    QSize s = maxSize;
    QSize hint = sizeHint.expandedTo(minSize);
    if (s.width() == QWIDGETSIZE_MAX && !(align & Qt::AlignHorizontal_Mask))
        if (!(sizePolicy.horizontalPolicy() & QSizePolicy::GrowFlag))
            s.setWidth(hint.width());

    if (s.height() == QWIDGETSIZE_MAX && !(align & Qt::AlignVertical_Mask))
        if (!(sizePolicy.verticalPolicy() & QSizePolicy::GrowFlag))
            s.setHeight(hint.height());

    if (align & Qt::AlignHorizontal_Mask)
        s.setWidth(QLAYOUTSIZE_MAX);
    if (align & Qt::AlignVertical_Mask)
        s.setHeight(QLAYOUTSIZE_MAX);
    return s;
}

Q_GUI_EXPORT QSize qSmartMaxSize(const QWidgetItem *i, Qt::Alignment align)
{
    QWidget *w = ((QWidgetItem*)i)->widget();

    return qSmartMaxSize(w->sizeHint().expandedTo(w->minimumSizeHint()), w->minimumSize(), w->maximumSize(),
                            w->sizePolicy(), align);
}

Q_GUI_EXPORT QSize qSmartMaxSize(const QWidget *w, Qt::Alignment align)
{
    return qSmartMaxSize(w->sizeHint().expandedTo(w->minimumSizeHint()), w->minimumSize(), w->maximumSize(),
                            w->sizePolicy(), align);
}

Q_GUI_EXPORT int qSmartSpacing(const QLayout *layout, QStyle::PixelMetric pm)
{
    QObject *parent = layout->parent();
    if (!parent) {
        return -1;
    } else if (parent->isWidgetType()) {
        QWidget *pw = static_cast<QWidget *>(parent);
        return pw->style()->pixelMetric(pm, 0, pw);
    } else {
        return static_cast<QLayout *>(parent)->spacing();
    }
}

QT_END_NAMESPACE
