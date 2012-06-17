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

#include "QtGui/qapplication.h"
#include "QtGui/qwidget.h"
#include "QtGui/qtabbar.h"
#include "QtGui/qstyle.h"
#include "QtGui/qdesktopwidget.h"
#include "QtCore/qvariant.h"
#include "qdockarealayout_p.h"
#include "qdockwidget.h"
#include "qmainwindow.h"
#include "qwidgetanimator_p.h"
#include "qmainwindowlayout_p.h"
#include "qdockwidget_p.h"
#include <private/qlayoutengine_p.h>

#include <qpainter.h>
#include <qstyleoption.h>

#ifndef QT_NO_DOCKWIDGET

QT_BEGIN_NAMESPACE

// qmainwindow.cpp
extern QMainWindowLayout *qt_mainwindow_layout(const QMainWindow *window);

enum { StateFlagVisible = 1, StateFlagFloating = 2 };

/******************************************************************************
** QPlaceHolderItem
*/

QPlaceHolderItem::QPlaceHolderItem(QWidget *w)
{
    objectName = w->objectName();
    hidden = w->isHidden();
    window = w->isWindow();
    if (window)
        topLevelRect = w->geometry();
}

/******************************************************************************
** QDockAreaLayoutItem
*/

QDockAreaLayoutItem::QDockAreaLayoutItem(QLayoutItem *_widgetItem)
    : widgetItem(_widgetItem), subinfo(0), placeHolderItem(0), pos(0), size(-1), flags(NoFlags)
{
}

QDockAreaLayoutItem::QDockAreaLayoutItem(QDockAreaLayoutInfo *_subinfo)
    : widgetItem(0), subinfo(_subinfo), placeHolderItem(0), pos(0), size(-1), flags(NoFlags)
{
}

QDockAreaLayoutItem::QDockAreaLayoutItem(QPlaceHolderItem *_placeHolderItem)
    : widgetItem(0), subinfo(0), placeHolderItem(_placeHolderItem), pos(0), size(-1), flags(NoFlags)
{
}

QDockAreaLayoutItem::QDockAreaLayoutItem(const QDockAreaLayoutItem &other)
    : widgetItem(other.widgetItem), subinfo(0), placeHolderItem(0), pos(other.pos),
        size(other.size), flags(other.flags)
{
    if (other.subinfo != 0)
        subinfo = new QDockAreaLayoutInfo(*other.subinfo);
    else if (other.placeHolderItem != 0)
        placeHolderItem = new QPlaceHolderItem(*other.placeHolderItem);
}

QDockAreaLayoutItem::~QDockAreaLayoutItem()
{
    delete subinfo;
    delete placeHolderItem;
}

bool QDockAreaLayoutItem::skip() const
{
    if (placeHolderItem != 0)
        return true;

    if (flags & GapItem)
        return false;

    if (widgetItem != 0)
        return widgetItem->isEmpty();

    if (subinfo != 0) {
        for (int i = 0; i < subinfo->item_list.count(); ++i) {
            if (!subinfo->item_list.at(i).skip())
                return false;
        }
    }

    return true;
}

QSize QDockAreaLayoutItem::minimumSize() const
{
    if (widgetItem != 0) {
        int left, top, right, bottom;
        widgetItem->widget()->getContentsMargins(&left, &top, &right, &bottom);
        return widgetItem->minimumSize() + QSize(left+right, top+bottom);
    }
    if (subinfo != 0)
        return subinfo->minimumSize();
    return QSize(0, 0);
}

QSize QDockAreaLayoutItem::maximumSize() const
{
    if (widgetItem != 0) {
        int left, top, right, bottom;
        widgetItem->widget()->getContentsMargins(&left, &top, &right, &bottom);
        return widgetItem->maximumSize()+ QSize(left+right, top+bottom);
    }
    if (subinfo != 0)
        return subinfo->maximumSize();
    return QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
}

bool QDockAreaLayoutItem::hasFixedSize(Qt::Orientation o) const
{
    return perp(o, minimumSize()) == perp(o, maximumSize());
}

bool QDockAreaLayoutItem::expansive(Qt::Orientation o) const
{
    if ((flags & GapItem) || placeHolderItem != 0)
        return false;
    if (widgetItem != 0)
        return ((widgetItem->expandingDirections() & o) == o);
    if (subinfo != 0)
        return subinfo->expansive(o);
    return false;
}

QSize QDockAreaLayoutItem::sizeHint() const
{
    if (placeHolderItem != 0)
        return QSize(0, 0);
    if (widgetItem != 0) {
        int left, top, right, bottom;
        widgetItem->widget()->getContentsMargins(&left, &top, &right, &bottom);
        return widgetItem->sizeHint() + QSize(left+right, top+bottom);
    }
    if (subinfo != 0)
        return subinfo->sizeHint();
    return QSize(-1, -1);
}

QDockAreaLayoutItem
    &QDockAreaLayoutItem::operator = (const QDockAreaLayoutItem &other)
{
    widgetItem = other.widgetItem;
    if (other.subinfo == 0)
        subinfo = 0;
    else
        subinfo = new QDockAreaLayoutInfo(*other.subinfo);

    delete placeHolderItem;
    if (other.placeHolderItem == 0)
        placeHolderItem = 0;
    else
        placeHolderItem = new QPlaceHolderItem(*other.placeHolderItem);

    pos = other.pos;
    size = other.size;
    flags = other.flags;

    return *this;
}

/******************************************************************************
** QDockAreaLayoutInfo
*/

#ifndef QT_NO_TABBAR
static quintptr tabId(const QDockAreaLayoutItem &item)
{
    if (item.widgetItem == 0)
        return 0;
    return reinterpret_cast<quintptr>(item.widgetItem->widget());
}
#endif

static const int zero = 0;

QDockAreaLayoutInfo::QDockAreaLayoutInfo()
    : sep(&zero), dockPos(QInternal::LeftDock), o(Qt::Horizontal), mainWindow(0)
#ifndef QT_NO_TABBAR
    , tabbed(false), tabBar(0), tabBarShape(QTabBar::RoundedSouth)
#endif
{
}

QDockAreaLayoutInfo::QDockAreaLayoutInfo(const int *_sep, QInternal::DockPosition _dockPos,
                                            Qt::Orientation _o, int tbshape,
                                            QMainWindow *window)
    : sep(_sep), dockPos(_dockPos), o(_o), mainWindow(window)
#ifndef QT_NO_TABBAR
    , tabbed(false), tabBar(0), tabBarShape(static_cast<QTabBar::Shape>(tbshape))
#endif
{
#ifdef QT_NO_TABBAR
    Q_UNUSED(tbshape);
#endif
}

QSize QDockAreaLayoutInfo::size() const
{
    return isEmpty() ? QSize(0, 0) : rect.size();
}

void QDockAreaLayoutInfo::clear()
{
    item_list.clear();
    rect = QRect();
#ifndef QT_NO_TABBAR
    tabbed = false;
    tabBar = 0;
#endif
}

bool QDockAreaLayoutInfo::isEmpty() const
{
    return next(-1) == -1;
}

QSize QDockAreaLayoutInfo::minimumSize() const
{
    if (isEmpty())
        return QSize(0, 0);

    int a = 0, b = 0;
    bool first = true;
    for (int i = 0; i < item_list.size(); ++i) {
        const QDockAreaLayoutItem &item = item_list.at(i);
        if (item.skip())
            continue;

        QSize min_size = item.minimumSize();
#ifndef QT_NO_TABBAR
        if (tabbed) {
            a = qMax(a, pick(o, min_size));
        } else
#endif
        {
            if (!first)
                a += *sep;
            a += pick(o, min_size);
        }
        b = qMax(b, perp(o, min_size));

        first = false;
    }

    QSize result;
    rpick(o, result) = a;
    rperp(o, result) = b;

#ifndef QT_NO_TABBAR
    QSize tbm = tabBarMinimumSize();
    if (!tbm.isNull()) {
        switch (tabBarShape) {
            case QTabBar::RoundedNorth:
            case QTabBar::RoundedSouth:
            case QTabBar::TriangularNorth:
            case QTabBar::TriangularSouth:
                result.rheight() += tbm.height();
                result.rwidth() = qMax(tbm.width(), result.width());
                break;
            case QTabBar::RoundedEast:
            case QTabBar::RoundedWest:
            case QTabBar::TriangularEast:
            case QTabBar::TriangularWest:
                result.rheight() = qMax(tbm.height(), result.height());
                result.rwidth() += tbm.width();
                break;
            default:
                break;
        }
    }
#endif // QT_NO_TABBAR

    return result;
}

QSize QDockAreaLayoutInfo::maximumSize() const
{
    if (isEmpty())
        return QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

    int a = 0, b = QWIDGETSIZE_MAX;
#ifndef QT_NO_TABBAR
    if (tabbed)
        a = QWIDGETSIZE_MAX;
#endif

    int min_perp = 0;

    bool first = true;
    for (int i = 0; i < item_list.size(); ++i) {
        const QDockAreaLayoutItem &item = item_list.at(i);
        if (item.skip())
            continue;

        QSize max_size = item.maximumSize();
        min_perp = qMax(min_perp, perp(o, item.minimumSize()));

#ifndef QT_NO_TABBAR
        if (tabbed) {
            a = qMin(a, pick(o, max_size));
        } else
#endif
        {
            if (!first)
                a += *sep;
            a += pick(o, max_size);
        }
        b = qMin(b, perp(o, max_size));

        a = qMin(a, int(QWIDGETSIZE_MAX));
        b = qMin(b, int(QWIDGETSIZE_MAX));

        first = false;
    }

    b = qMax(b, min_perp);

    QSize result;
    rpick(o, result) = a;
    rperp(o, result) = b;

#ifndef QT_NO_TABBAR
    QSize tbh = tabBarSizeHint();
    if (!tbh.isNull()) {
        switch (tabBarShape) {
            case QTabBar::RoundedNorth:
            case QTabBar::RoundedSouth:
                result.rheight() += tbh.height();
                break;
            case QTabBar::RoundedEast:
            case QTabBar::RoundedWest:
                result.rwidth() += tbh.width();
                break;
            default:
                break;
        }
    }
#endif // QT_NO_TABBAR

    return result;
}

QSize QDockAreaLayoutInfo::sizeHint() const
{
    if (isEmpty())
        return QSize(0, 0);

    int a = 0, b = 0;
    int min_perp = 0;
    int max_perp = QWIDGETSIZE_MAX;
    const QDockAreaLayoutItem *previous = 0;
    for (int i = 0; i < item_list.size(); ++i) {
        const QDockAreaLayoutItem &item = item_list.at(i);
        if (item.skip())
            continue;

        bool gap = item.flags & QDockAreaLayoutItem::GapItem;

        QSize size_hint = item.sizeHint();
        min_perp = qMax(min_perp, perp(o, item.minimumSize()));
        max_perp = qMin(max_perp, perp(o, item.maximumSize()));

#ifndef QT_NO_TABBAR
        if (tabbed) {
            a = qMax(a, gap ? item.size : pick(o, size_hint));
        } else
#endif
        {
            if (previous && !gap && !(previous->flags &  QDockAreaLayoutItem::GapItem)
                && !previous->hasFixedSize(o)) {
                a += *sep;
            }
            a += gap ? item.size : pick(o, size_hint);
        }
        b = qMax(b, perp(o, size_hint));

        previous = &item;
    }

    max_perp = qMax(max_perp, min_perp);
    b = qMax(b, min_perp);
    b = qMin(b, max_perp);

    QSize result;
    rpick(o, result) = a;
    rperp(o, result) = b;

#ifndef QT_NO_TABBAR
    if (tabbed) {
        QSize tbh = tabBarSizeHint();
        switch (tabBarShape) {
            case QTabBar::RoundedNorth:
            case QTabBar::RoundedSouth:
            case QTabBar::TriangularNorth:
            case QTabBar::TriangularSouth:
                result.rheight() += tbh.height();
                result.rwidth() = qMax(tbh.width(), result.width());
                break;
            case QTabBar::RoundedEast:
            case QTabBar::RoundedWest:
            case QTabBar::TriangularEast:
            case QTabBar::TriangularWest:
                result.rheight() = qMax(tbh.height(), result.height());
                result.rwidth() += tbh.width();
                break;
            default:
                break;
        }
    }
#endif // QT_NO_TABBAR

    return result;
}

bool QDockAreaLayoutInfo::expansive(Qt::Orientation o) const
{
    for (int i = 0; i < item_list.size(); ++i) {
        if (item_list.at(i).expansive(o))
            return true;
    }
    return false;
}

/* QDockAreaLayoutInfo::maximumSize() doesn't return the real max size. For example,
   if the layout is empty, it returns QWIDGETSIZE_MAX. This is so that empty dock areas
   don't constrain the size of the QMainWindow, but sometimes we really need to know the
   maximum size. Also, these functions take into account widgets that want to keep their
   size (f.ex. when they are hidden and then shown, they should not change size).
*/

static int realMinSize(const QDockAreaLayoutInfo &info)
{
    int result = 0;
    bool first = true;
    for (int i = 0; i < info.item_list.size(); ++i) {
        const QDockAreaLayoutItem &item = info.item_list.at(i);
        if (item.skip())
            continue;

        int min = 0;
        if ((item.flags & QDockAreaLayoutItem::KeepSize) && item.size != -1)
            min = item.size;
        else
            min = pick(info.o, item.minimumSize());

        if (!first)
            result += *info.sep;
        result += min;

        first = false;
    }

    return result;
}

static int realMaxSize(const QDockAreaLayoutInfo &info)
{
    int result = 0;
    bool first = true;
    for (int i = 0; i < info.item_list.size(); ++i) {
        const QDockAreaLayoutItem &item = info.item_list.at(i);
        if (item.skip())
            continue;

        int max = 0;
        if ((item.flags & QDockAreaLayoutItem::KeepSize) && item.size != -1)
            max = item.size;
        else
            max = pick(info.o, item.maximumSize());

        if (!first)
            result += *info.sep;
        result += max;

        if (result >= QWIDGETSIZE_MAX)
            return QWIDGETSIZE_MAX;

        first = false;
    }

    return result;
}

void QDockAreaLayoutInfo::fitItems()
{
#ifndef QT_NO_TABBAR
    if (tabbed) {
        return;
    }
#endif

    QVector<QLayoutStruct> layout_struct_list(item_list.size()*2);
    int j = 0;

    int size = pick(o, rect.size());
    int min_size = realMinSize(*this);
    int max_size = realMaxSize(*this);
    int last_index = -1;

    const QDockAreaLayoutItem *previous = 0;
    for (int i = 0; i < item_list.size(); ++i) {
        QDockAreaLayoutItem &item = item_list[i];
        if (item.skip())
            continue;

        bool gap = item.flags & QDockAreaLayoutItem::GapItem;
        if (previous && !gap) {
            if (!(previous->flags & QDockAreaLayoutItem::GapItem)) {
                QLayoutStruct &ls = layout_struct_list[j++];
                ls.init();
                ls.minimumSize = ls.maximumSize = ls.sizeHint = previous->hasFixedSize(o) ? 0 : *sep;
                ls.empty = false;
            }
        }

        if (item.flags & QDockAreaLayoutItem::KeepSize) {
            // Check if the item can keep its size, without violating size constraints
            // of other items.

            if (size < min_size) {
                // There is too little space to keep this widget's size
                item.flags &= ~QDockAreaLayoutItem::KeepSize;
                min_size -= item.size;
                min_size += pick(o, item.minimumSize());
                min_size = qMax(0, min_size);
            } else if (size > max_size) {
                // There is too much space to keep this widget's size
                item.flags &= ~QDockAreaLayoutItem::KeepSize;
                max_size -= item.size;
                max_size += pick(o, item.maximumSize());
                max_size = qMin<int>(QWIDGETSIZE_MAX, max_size);
            }
        }

        last_index = j;
        QLayoutStruct &ls = layout_struct_list[j++];
        ls.init();
        ls.empty = false;
        if (item.flags & QDockAreaLayoutItem::KeepSize) {
            ls.minimumSize = ls.maximumSize = ls.sizeHint = item.size;
            ls.expansive = false;
            ls.stretch = 0;
        } else {
            ls.maximumSize = pick(o, item.maximumSize());
            ls.expansive = item.expansive(o);
            ls.minimumSize = pick(o, item.minimumSize());
            ls.sizeHint = item.size == -1 ? pick(o, item.sizeHint()) : item.size;
            ls.stretch = ls.expansive ? ls.sizeHint : 0;
        }

        item.flags &= ~QDockAreaLayoutItem::KeepSize;
        previous = &item;
    }
    layout_struct_list.resize(j);

    // If there is more space than the widgets can take (due to maximum size constraints),
    // we detect it here and stretch the last widget to take up the rest of the space.
    if (size > max_size && last_index != -1) {
        layout_struct_list[last_index].maximumSize = QWIDGETSIZE_MAX;
        layout_struct_list[last_index].expansive = true;
    }

    qGeomCalc(layout_struct_list, 0, j, pick(o, rect.topLeft()), size, 0);

    j = 0;
    bool prev_gap = false;
    bool first = true;
    for (int i = 0; i < item_list.size(); ++i) {
        QDockAreaLayoutItem &item = item_list[i];
        if (item.skip())
            continue;

        bool gap = item.flags & QDockAreaLayoutItem::GapItem;
        if (!first && !gap && !prev_gap)
            ++j;

        const QLayoutStruct &ls = layout_struct_list.at(j++);
        item.size = ls.size;
        item.pos = ls.pos;

        if (item.subinfo != 0) {
            item.subinfo->rect = itemRect(i);
            item.subinfo->fitItems();
        }

        prev_gap = gap;
        first = false;
    }
}

static QInternal::DockPosition dockPosHelper(const QRect &rect, const QPoint &_pos,
                                        Qt::Orientation o,
                                        bool nestingEnabled,
                                        QDockAreaLayoutInfo::TabMode tabMode)
{
    if (tabMode == QDockAreaLayoutInfo::ForceTabs)
        return QInternal::DockCount;

    QPoint pos = _pos - rect.topLeft();

    int x = pos.x();
    int y = pos.y();
    int w = rect.width();
    int h = rect.height();

    if (tabMode != QDockAreaLayoutInfo::NoTabs) {
        // is it in the center?
        if (nestingEnabled) {
        /*             2/3
                +--------------+
                |              |
                |   CCCCCCCC   |
           2/3  |   CCCCCCCC   |
                |   CCCCCCCC   |
                |              |
                +--------------+     */

            QRect center(w/6, h/6, 2*w/3, 2*h/3);
            if (center.contains(pos))
                return QInternal::DockCount;
        } else if (o == Qt::Horizontal) {
        /*             2/3
                +--------------+
                |   CCCCCCCC   |
                |   CCCCCCCC   |
                |   CCCCCCCC   |
                |   CCCCCCCC   |
                |   CCCCCCCC   |
                +--------------+     */

            if (x > w/6 && x < w*5/6)
                return QInternal::DockCount;
        } else {
        /*
                +--------------+
                |              |
           2/3  |CCCCCCCCCCCCCC|
                |CCCCCCCCCCCCCC|
                |              |
                +--------------+     */
            if (y > h/6 && y < 5*h/6)
                return QInternal::DockCount;
        }
    }

    // not in the center. which edge?
    if (nestingEnabled) {
        if (o == Qt::Horizontal) {
    /*       1/3  1/3 1/3
            +------------+     (we've already ruled out the center)
            |LLLLTTTTRRRR|
            |LLLLTTTTRRRR|
            |LLLLBBBBRRRR|
            |LLLLBBBBRRRR|
            +------------+    */

            if (x < w/3)
                return QInternal::LeftDock;
            if (x > 2*w/3)
                return QInternal::RightDock;
            if (y < h/2)
                return QInternal::TopDock;
            return QInternal::BottomDock;
        } else {
    /*      +------------+     (we've already ruled out the center)
        1/3 |TTTTTTTTTTTT|
            |LLLLLLRRRRRR|
        1/3 |LLLLLLRRRRRR|
        1/3 |BBBBBBBBBBBB|
            +------------+    */

            if (y < h/3)
                return QInternal::TopDock;
            if (y > 2*h/3)
                return QInternal::BottomDock;
            if (x < w/2)
                return QInternal::LeftDock;
            return QInternal::RightDock;
        }
    } else {
        if (o == Qt::Horizontal) {
            return x < w/2
                    ? QInternal::LeftDock
                    : QInternal::RightDock;
        } else {
            return y < h/2
                    ? QInternal::TopDock
                    : QInternal::BottomDock;
        }
    }
}

QList<int> QDockAreaLayoutInfo::gapIndex(const QPoint& _pos,
                        bool nestingEnabled, TabMode tabMode) const
{
    QList<int> result;
    QRect item_rect;
    int item_index = 0;

#ifndef QT_NO_TABBAR
    if (tabbed) {
        item_rect = tabContentRect();
    } else
#endif
    {
        int pos = pick(o, _pos);

        int last = -1;
        for (int i = 0; i < item_list.size(); ++i) {
            const QDockAreaLayoutItem &item = item_list.at(i);
            if (item.skip())
                continue;

            last = i;

            if (item.pos + item.size < pos)
                continue;

            if (item.subinfo != 0
#ifndef QT_NO_TABBAR
                && !item.subinfo->tabbed
#endif
                ) {
                result = item.subinfo->gapIndex(_pos, nestingEnabled,
                                                    tabMode);
                result.prepend(i);
                return result;
            }

            item_rect = itemRect(i);
            item_index = i;
            break;
        }

        if (item_rect.isNull()) {
            result.append(last + 1);
            return result;
        }
    }

    Q_ASSERT(!item_rect.isNull());

    QInternal::DockPosition dock_pos
        = dockPosHelper(item_rect, _pos, o, nestingEnabled, tabMode);

    switch (dock_pos) {
        case QInternal::LeftDock:
            if (o == Qt::Horizontal)
                result << item_index;
            else
                result << item_index << 0; // this subinfo doesn't exist yet, but insertGap()
                                           // handles this by inserting it
            break;
        case QInternal::RightDock:
            if (o == Qt::Horizontal)
                result << item_index + 1;
            else
                result << item_index << 1;
            break;
        case QInternal::TopDock:
            if (o == Qt::Horizontal)
                result << item_index << 0;
            else
                result << item_index;
            break;
        case QInternal::BottomDock:
            if (o == Qt::Horizontal)
                result << item_index << 1;
            else
                result << item_index + 1;
            break;
        case  QInternal::DockCount:
            result << (-item_index - 1) << 0;   // negative item_index means "on top of"
                                                // -item_index - 1, insertGap()
                                                // will insert a tabbed subinfo
            break;
        default:
            break;
    }

    return result;
}

static inline int shrink(QLayoutStruct &ls, int delta)
{
    if (ls.empty)
        return 0;
    int old_size = ls.size;
    ls.size = qMax(ls.size - delta, ls.minimumSize);
    return old_size - ls.size;
}

static inline int grow(QLayoutStruct &ls, int delta)
{
    if (ls.empty)
        return 0;
    int old_size = ls.size;
    ls.size = qMin(ls.size + delta, ls.maximumSize);
    return ls.size - old_size;
}

static int separatorMoveHelper(QVector<QLayoutStruct> &list, int index, int delta, int sep)
{
    // adjust sizes
    int pos = -1;
    for (int i = 0; i < list.size(); ++i) {
        const QLayoutStruct &ls = list.at(i);
        if (!ls.empty) {
            pos = ls.pos;
            break;
        }
    }
    if (pos == -1)
        return 0;

    if (delta > 0) {
        int growlimit = 0;
        for (int i = 0; i<=index; ++i) {
            const QLayoutStruct &ls = list.at(i);
            if (ls.empty)
                continue;
            if (ls.maximumSize == QLAYOUTSIZE_MAX) {
                growlimit = QLAYOUTSIZE_MAX;
                break;
            }
            growlimit += ls.maximumSize - ls.size;
        }
        if (delta > growlimit)
            delta = growlimit;

        int d = 0;
        for (int i = index + 1; d < delta && i < list.count(); ++i)
            d += shrink(list[i], delta - d);
        delta = d;
        d = 0;
        for (int i = index; d < delta && i >= 0; --i)
            d += grow(list[i], delta - d);
    } else if (delta < 0) {
        int growlimit = 0;
        for (int i = index + 1; i < list.count(); ++i) {
            const QLayoutStruct &ls = list.at(i);
            if (ls.empty)
                continue;
            if (ls.maximumSize == QLAYOUTSIZE_MAX) {
                growlimit = QLAYOUTSIZE_MAX;
                break;
            }
            growlimit += ls.maximumSize - ls.size;
        }
        if (-delta > growlimit)
            delta = -growlimit;

        int d = 0;
        for (int i = index; d < -delta && i >= 0; --i)
            d += shrink(list[i], -delta - d);
        delta = -d;
        d = 0;
        for (int i = index + 1; d < -delta && i < list.count(); ++i)
            d += grow(list[i], -delta - d);
    }

    // adjust positions
    bool first = true;
    for (int i = 0; i < list.size(); ++i) {
        QLayoutStruct &ls = list[i];
        if (ls.empty) {
            ls.pos = pos + (first ? 0 : sep);
            continue;
        }
        if (!first)
            pos += sep;
        ls.pos = pos;
        pos += ls.size;
        first = false;
    }

    return delta;
}

int QDockAreaLayoutInfo::separatorMove(int index, int delta)
{
#ifndef QT_NO_TABBAR
    Q_ASSERT(!tabbed);
#endif

    QVector<QLayoutStruct> list(item_list.size());
    for (int i = 0; i < list.size(); ++i) {
        const QDockAreaLayoutItem &item = item_list.at(i);
        QLayoutStruct &ls = list[i];
        Q_ASSERT(!(item.flags & QDockAreaLayoutItem::GapItem));
        if (item.skip()) {
            ls.empty = true;
        } else {
            const int separatorSpace = item.hasFixedSize(o) ? 0 : *sep;
            ls.empty = false;
            ls.pos = item.pos;
            ls.size = item.size + separatorSpace;
            ls.minimumSize = pick(o, item.minimumSize()) + separatorSpace;
            ls.maximumSize = pick(o, item.maximumSize()) + separatorSpace;

        }
    }

    //the separator space has been added to the size, so we pass 0 as a parameter
    delta = separatorMoveHelper(list, index, delta, 0 /*separator*/);

    for (int i = 0; i < list.size(); ++i) {
        QDockAreaLayoutItem &item = item_list[i];
        if (item.skip())
            continue;
        QLayoutStruct &ls = list[i];
        const int separatorSpace = item.hasFixedSize(o) ? 0 : *sep;
        item.size = ls.size - separatorSpace;
        item.pos = ls.pos;
        if (item.subinfo != 0) {
            item.subinfo->rect = itemRect(i);
            item.subinfo->fitItems();
        }
    }

    return delta;
}

void QDockAreaLayoutInfo::unnest(int index)
{
    QDockAreaLayoutItem &item = item_list[index];
    if (item.subinfo == 0)
        return;
    if (item.subinfo->item_list.count() > 1)
        return;

    if (item.subinfo->item_list.count() == 0) {
        item_list.removeAt(index);
    } else if (item.subinfo->item_list.count() == 1) {
        QDockAreaLayoutItem &child = item.subinfo->item_list.first();
        if (child.widgetItem != 0) {
            item.widgetItem = child.widgetItem;
            delete item.subinfo;
            item.subinfo = 0;
        } else if (child.subinfo != 0) {
            QDockAreaLayoutInfo *tmp = item.subinfo;
            item.subinfo = child.subinfo;
            child.subinfo = 0;
            tmp->item_list.clear();
            delete tmp;
        }
    }
}

void QDockAreaLayoutInfo::remove(const QList<int> &path)
{
    Q_ASSERT(!path.isEmpty());

    if (path.count() > 1) {
        const int index = path.first();
        QDockAreaLayoutItem &item = item_list[index];
        Q_ASSERT(item.subinfo != 0);
        item.subinfo->remove(path.mid(1));
        unnest(index);
    } else {
        int index = path.first();
        item_list.removeAt(index);
    }
}

QLayoutItem *QDockAreaLayoutInfo::plug(const QList<int> &path)
{
    Q_ASSERT(!path.isEmpty());

    int index = path.first();
    if (index < 0)
        index = -index - 1;

    if (path.count() > 1) {
        const QDockAreaLayoutItem &item = item_list.at(index);
        Q_ASSERT(item.subinfo != 0);
        return item.subinfo->plug(path.mid(1));
    }

    QDockAreaLayoutItem &item = item_list[index];

    Q_ASSERT(item.widgetItem != 0);
    Q_ASSERT(item.flags & QDockAreaLayoutItem::GapItem);
    item.flags &= ~QDockAreaLayoutItem::GapItem;

    QRect result;

#ifndef QT_NO_TABBAR
    if (tabbed) {
    } else
#endif
    {
        int prev = this->prev(index);
        int next = this->next(index);

        if (prev != -1 && !(item_list.at(prev).flags & QDockAreaLayoutItem::GapItem)) {
            item.pos += *sep;
            item.size -= *sep;
        }
        if (next != -1 && !(item_list.at(next).flags & QDockAreaLayoutItem::GapItem))
            item.size -= *sep;

        QPoint pos;
        rpick(o, pos) = item.pos;
        rperp(o, pos) = perp(o, rect.topLeft());
        QSize s;
        rpick(o, s) = item.size;
        rperp(o, s) = perp(o, rect.size());
        result = QRect(pos, s);
    }

    return item.widgetItem;
}

QLayoutItem *QDockAreaLayoutInfo::unplug(const QList<int> &path)
{
    Q_ASSERT(!path.isEmpty());

    const int index = path.first();
    if (path.count() > 1) {
        const QDockAreaLayoutItem &item = item_list.at(index);
        Q_ASSERT(item.subinfo != 0);
        return item.subinfo->unplug(path.mid(1));
    }

    QDockAreaLayoutItem &item = item_list[index];
    int prev = this->prev(index);
    int next = this->next(index);

    Q_ASSERT(!(item.flags & QDockAreaLayoutItem::GapItem));
    item.flags |= QDockAreaLayoutItem::GapItem;

#ifndef QT_NO_TABBAR
    if (tabbed) {
    } else
#endif
    {
        if (prev != -1 && !(item_list.at(prev).flags & QDockAreaLayoutItem::GapItem)) {
            item.pos -= *sep;
            item.size += *sep;
        }
        if (next != -1 && !(item_list.at(next).flags & QDockAreaLayoutItem::GapItem))
            item.size += *sep;
    }

    return item.widgetItem;
}

#ifndef QT_NO_TABBAR

quintptr QDockAreaLayoutInfo::currentTabId() const
{
    if (!tabbed || tabBar == 0)
        return 0;

    int index = tabBar->currentIndex();
    if (index == -1)
        return 0;

    return qvariant_cast<quintptr>(tabBar->tabData(index));
}

void QDockAreaLayoutInfo::setCurrentTab(QWidget *widget)
{
    setCurrentTabId(reinterpret_cast<quintptr>(widget));
}

void QDockAreaLayoutInfo::setCurrentTabId(quintptr id)
{
    if (!tabbed || tabBar == 0)
        return;

    for (int i = 0; i < tabBar->count(); ++i) {
        if (qvariant_cast<quintptr>(tabBar->tabData(i)) == id) {
            tabBar->setCurrentIndex(i);
            return;
        }
    }
}

#endif // QT_NO_TABBAR

static QRect dockedGeometry(QWidget *widget)
{
    int titleHeight = 0;

    QDockWidgetLayout *layout
        = qobject_cast<QDockWidgetLayout*>(widget->layout());
    if(layout != 0 && layout->nativeWindowDeco())
        titleHeight = layout->titleHeight();

    QRect result = widget->geometry();
    result.adjust(0, -titleHeight, 0, 0);
    return result;
}

bool QDockAreaLayoutInfo::insertGap(const QList<int> &path, QLayoutItem *dockWidgetItem)
{
    Q_ASSERT(!path.isEmpty());

    bool insert_tabbed = false;
    int index = path.first();
    if (index < 0) {
        insert_tabbed = true;
        index = -index - 1;
    }

//    dump(qDebug() << "insertGap() before:" << index << tabIndex, *this, QString());

    if (path.count() > 1) {
        QDockAreaLayoutItem &item = item_list[index];

        if (item.subinfo == 0
#ifndef QT_NO_TABBAR
            || (item.subinfo->tabbed && !insert_tabbed)
#endif
            ) {

            // this is not yet a nested layout - make it

            QDockAreaLayoutInfo *subinfo = item.subinfo;
            QLayoutItem *widgetItem = item.widgetItem;
            QPlaceHolderItem *placeHolderItem = item.placeHolderItem;
            QRect r = subinfo == 0 ? widgetItem ? dockedGeometry(widgetItem->widget()) : placeHolderItem->topLevelRect : subinfo->rect;

            Qt::Orientation opposite = o == Qt::Horizontal ? Qt::Vertical : Qt::Horizontal;
#ifdef QT_NO_TABBAR
            const int tabBarShape = 0;
#endif
            QDockAreaLayoutInfo *new_info
                = new QDockAreaLayoutInfo(sep, dockPos, opposite, tabBarShape, mainWindow);

            //item become a new top-level
            item.subinfo = new_info;
            item.widgetItem = 0;
            item.placeHolderItem = 0;

            QDockAreaLayoutItem new_item
                = widgetItem == 0
                    ? QDockAreaLayoutItem(subinfo)
                    : widgetItem ? QDockAreaLayoutItem(widgetItem) : QDockAreaLayoutItem(placeHolderItem);
            new_item.size = pick(opposite, r.size());
            new_item.pos = pick(opposite, r.topLeft());
            new_info->item_list.append(new_item);
#ifndef QT_NO_TABBAR
            if (insert_tabbed) {
                new_info->tabbed = true;
            }
#endif
        }

        return item.subinfo->insertGap(path.mid(1), dockWidgetItem);
    }

    // create the gap item
    QDockAreaLayoutItem gap_item;
    gap_item.flags |= QDockAreaLayoutItem::GapItem;
    gap_item.widgetItem = dockWidgetItem;   // so minimumSize(), maximumSize() and
                                            // sizeHint() will work
#ifndef QT_NO_TABBAR
    if (!tabbed)
#endif
    {
        int prev = this->prev(index);
        int next = this->next(index - 1);
        // find out how much space we have in the layout
        int space = 0;
        if (isEmpty()) {
            // I am an empty dock area, therefore I am a top-level dock area.
            switch (dockPos) {
                case QInternal::LeftDock:
                case QInternal::RightDock:
                    if (o == Qt::Vertical) {
                        // the "size" is the height of the dock area (remember we are empty)
                        space = pick(Qt::Vertical, rect.size());
                    } else {
                        space = pick(Qt::Horizontal, dockWidgetItem->widget()->size());
                    }
                    break;
                case QInternal::TopDock:
                case QInternal::BottomDock:
                default:
                    if (o == Qt::Horizontal) {
                        // the "size" is width of the dock area
                        space = pick(Qt::Horizontal, rect.size());
                    } else {
                        space = pick(Qt::Vertical, dockWidgetItem->widget()->size());
                    }
                    break;
            }
        } else {
            for (int i = 0; i < item_list.count(); ++i) {
                const QDockAreaLayoutItem &item = item_list.at(i);
                if (item.skip())
                    continue;
                Q_ASSERT(!(item.flags & QDockAreaLayoutItem::GapItem));
                space += item.size - pick(o, item.minimumSize());
            }
        }

        // find the actual size of the gap
        int gap_size = 0;
        int sep_size = 0;
        if (isEmpty()) {
            gap_size = space;
            sep_size = 0;
        } else {
            QRect r = dockedGeometry(dockWidgetItem->widget());
            gap_size = pick(o, r.size());
        if (prev != -1 && !(item_list.at(prev).flags & QDockAreaLayoutItem::GapItem))
                sep_size += *sep;
            if (next != -1 && !(item_list.at(next).flags & QDockAreaLayoutItem::GapItem))
                sep_size += *sep;
        }
        if (gap_size + sep_size > space)
            gap_size = pick(o, gap_item.minimumSize());
        gap_item.size = gap_size + sep_size;
    }

    // finally, insert the gap
    item_list.insert(index, gap_item);

//    dump(qDebug() << "insertGap() after:" << index << tabIndex, *this, QString());

    return true;
}

QDockAreaLayoutInfo *QDockAreaLayoutInfo::info(QWidget *widget)
{
    for (int i = 0; i < item_list.count(); ++i) {
        const QDockAreaLayoutItem &item = item_list.at(i);
        if (item.skip())
            continue;

#ifndef QT_NO_TABBAR
        if (tabbed && widget == tabBar)
            return this;
#endif

        if (item.widgetItem != 0 && item.widgetItem->widget() == widget)
            return this;

        if (item.subinfo != 0) {
            if (QDockAreaLayoutInfo *result = item.subinfo->info(widget))
                return result;
        }
    }

    return 0;
}

QDockAreaLayoutInfo *QDockAreaLayoutInfo::info(const QList<int> &path)
{
    int index = path.first();
    if (index < 0)
        index = -index - 1;
    if (index >= item_list.count())
        return this;
    if (path.count() == 1 || item_list[index].subinfo == 0)
        return this;
    return item_list[index].subinfo->info(path.mid(1));
}

QRect QDockAreaLayoutInfo::itemRect(int index) const
{
    const QDockAreaLayoutItem &item = item_list.at(index);

    if (item.skip())
        return QRect();

    QRect result;

#ifndef QT_NO_TABBAR
    if (tabbed) {
        if (tabId(item) == currentTabId())
            result = tabContentRect();
    } else
#endif
    {
        QPoint pos;
        rpick(o, pos) = item.pos;
        rperp(o, pos) = perp(o, rect.topLeft());
        QSize s;
        rpick(o, s) = item.size;
        rperp(o, s) = perp(o, rect.size());
        result = QRect(pos, s);
    }

    return result;
}

QRect QDockAreaLayoutInfo::itemRect(const QList<int> &path) const
{
    Q_ASSERT(!path.isEmpty());

    const int index = path.first();
    if (path.count() > 1) {
        const QDockAreaLayoutItem &item = item_list.at(index);
        Q_ASSERT(item.subinfo != 0);
        return item.subinfo->itemRect(path.mid(1));
    }

    return itemRect(index);
}

QRect QDockAreaLayoutInfo::separatorRect(int index) const
{
#ifndef QT_NO_TABBAR
    if (tabbed)
        return QRect();
#endif

    const QDockAreaLayoutItem &item = item_list.at(index);
    if (item.skip())
        return QRect();

    QPoint pos = rect.topLeft();
    rpick(o, pos) = item.pos + item.size;
    QSize s = rect.size();
    rpick(o, s) = *sep;

    return QRect(pos, s);
}

QRect QDockAreaLayoutInfo::separatorRect(const QList<int> &path) const
{
    Q_ASSERT(!path.isEmpty());

    const int index = path.first();
    if (path.count() > 1) {
        const QDockAreaLayoutItem &item = item_list.at(index);
        Q_ASSERT(item.subinfo != 0);
        return item.subinfo->separatorRect(path.mid(1));
    }
    return separatorRect(index);
}

QList<int> QDockAreaLayoutInfo::findSeparator(const QPoint &_pos) const
{
#ifndef QT_NO_TABBAR
    if (tabbed)
        return QList<int>();
#endif

    int pos = pick(o, _pos);

    for (int i = 0; i < item_list.size(); ++i) {
        const QDockAreaLayoutItem &item = item_list.at(i);
        if (item.skip() || (item.flags & QDockAreaLayoutItem::GapItem))
            continue;

        if (item.pos + item.size > pos) {
            if (item.subinfo != 0) {
                QList<int> result = item.subinfo->findSeparator(_pos);
                if (!result.isEmpty()) {
                    result.prepend(i);
                    return result;
                } else {
                    return QList<int>();
                }
            }
        }

        int next = this->next(i);
        if (next == -1 || (item_list.at(next).flags & QDockAreaLayoutItem::GapItem))
            continue;

        QRect sepRect = separatorRect(i);
        if (!sepRect.isNull() && *sep == 1)
            sepRect.adjust(-2, -2, 2, 2);
        //we also make sure we don't find a separator that's not there
        if (sepRect.contains(_pos) && !item.hasFixedSize(o)) {
            return QList<int>() << i;
        }

    }

    return QList<int>();
}

QList<int> QDockAreaLayoutInfo::indexOfPlaceHolder(const QString &objectName) const
{
    for (int i = 0; i < item_list.size(); ++i) {
        const QDockAreaLayoutItem &item = item_list.at(i);

        if (item.subinfo != 0) {
            QList<int> result = item.subinfo->indexOfPlaceHolder(objectName);
            if (!result.isEmpty()) {
                result.prepend(i);
                return result;
            }
            continue;
        }

        if (item.placeHolderItem != 0 && item.placeHolderItem->objectName == objectName) {
            QList<int> result;
            result << i;
            return result;
        }
    }

    return QList<int>();
}

QList<int> QDockAreaLayoutInfo::indexOf(QWidget *widget) const
{
    for (int i = 0; i < item_list.size(); ++i) {
        const QDockAreaLayoutItem &item = item_list.at(i);

        if (item.placeHolderItem != 0)
            continue;

        if (item.subinfo != 0) {
            QList<int> result = item.subinfo->indexOf(widget);
            if (!result.isEmpty()) {
                result.prepend(i);
                return result;
            }
            continue;
        }

        if (!(item.flags & QDockAreaLayoutItem::GapItem) && item.widgetItem->widget() == widget) {
            QList<int> result;
            result << i;
            return result;
        }
    }

    return QList<int>();
}

QMainWindowLayout *QDockAreaLayoutInfo::mainWindowLayout() const
{
    QMainWindowLayout *result = qt_mainwindow_layout(mainWindow);
    Q_ASSERT(result != 0);
    return result;
}

bool QDockAreaLayoutInfo::hasFixedSize() const
{
    return perp(o, minimumSize()) == perp(o, maximumSize());
}


void QDockAreaLayoutInfo::apply(bool animate)
{
    QWidgetAnimator &widgetAnimator = mainWindowLayout()->widgetAnimator;

#ifndef QT_NO_TABBAR
    if (tabbed) {
        QRect tab_rect;
        QSize tbh = tabBarSizeHint();

        if (!tbh.isNull()) {
            switch (tabBarShape) {
                case QTabBar::RoundedNorth:
                case QTabBar::TriangularNorth:
                    tab_rect = QRect(rect.left(), rect.top(), rect.width(), tbh.height());
                    break;
                case QTabBar::RoundedSouth:
                case QTabBar::TriangularSouth:
                    tab_rect = QRect(rect.left(), rect.bottom() - tbh.height() + 1,
                                        rect.width(), tbh.height());
                    break;
                case QTabBar::RoundedEast:
                case QTabBar::TriangularEast:
                    tab_rect = QRect(rect.right() - tbh.width() + 1, rect.top(),
                                        tbh.width(), rect.height());
                    break;
                case QTabBar::RoundedWest:
                case QTabBar::TriangularWest:
                    tab_rect = QRect(rect.left(), rect.top(),
                                        tbh.width(), rect.height());
                    break;
                default:
                    break;
            }
        }

        widgetAnimator.animate(tabBar, tab_rect, animate);
    }
#endif // QT_NO_TABBAR

    for (int i = 0; i < item_list.size(); ++i) {
        QDockAreaLayoutItem &item = item_list[i];

        if (item.flags & QDockAreaLayoutItem::GapItem)
            continue;

        if (item.subinfo != 0) {
            item.subinfo->apply(animate);
            continue;
        }

        if (item.skip())
            continue;

        Q_ASSERT(item.widgetItem);
        QRect r = itemRect(i);
        QWidget *w = item.widgetItem->widget();

        QRect geo = w->geometry();
        widgetAnimator.animate(w, r, animate);
        if (!w->isHidden() && w->window()->isVisible()) {
            QDockWidget *dw = qobject_cast<QDockWidget*>(w);
            if (!r.isValid() && geo.right() >= 0 && geo.bottom() >= 0) {
                dw->lower();
                emit dw->visibilityChanged(false);
            } else if (r.isValid()
                        && (geo.right() < 0 || geo.bottom() < 0)) {
                emit dw->visibilityChanged(true);
            }
        }
    }
#ifndef QT_NO_TABBAR
    if (*sep == 1)
        updateSeparatorWidgets();
#endif //QT_NO_TABBAR
}

static void paintSep(QPainter *p, QWidget *w, const QRect &r, Qt::Orientation o, bool mouse_over)
{
    QStyleOption opt(0);
    opt.state = QStyle::State_None;
    if (w->isEnabled())
        opt.state |= QStyle::State_Enabled;
    if (o != Qt::Horizontal)
        opt.state |= QStyle::State_Horizontal;
    if (mouse_over)
        opt.state |= QStyle::State_MouseOver;
    opt.rect = r;
    opt.palette = w->palette();

    w->style()->drawPrimitive(QStyle::PE_IndicatorDockWidgetResizeHandle, &opt, p, w);
}

QRegion QDockAreaLayoutInfo::separatorRegion() const
{
    QRegion result;

    if (isEmpty())
        return result;
#ifndef QT_NO_TABBAR
    if (tabbed)
        return result;
#endif

    for (int i = 0; i < item_list.count(); ++i) {
        const QDockAreaLayoutItem &item = item_list.at(i);

        if (item.skip())
            continue;

        int next = this->next(i);

        if (item.subinfo)
            result |= item.subinfo->separatorRegion();

        if (next == -1)
            break;
        result |= separatorRect(i);
    }

    return result;
}

void QDockAreaLayoutInfo::paintSeparators(QPainter *p, QWidget *widget,
                                                    const QRegion &clip,
                                                    const QPoint &mouse) const
{
    if (isEmpty())
        return;
#ifndef QT_NO_TABBAR
    if (tabbed)
        return;
#endif

    for (int i = 0; i < item_list.count(); ++i) {
        const QDockAreaLayoutItem &item = item_list.at(i);

        if (item.skip())
            continue;

        int next = this->next(i);
        if ((item.flags & QDockAreaLayoutItem::GapItem)
                || (next != -1 && (item_list.at(next).flags & QDockAreaLayoutItem::GapItem)))
            continue;

        if (item.subinfo) {
            if (clip.contains(item.subinfo->rect))
                item.subinfo->paintSeparators(p, widget, clip, mouse);
        }

        if (next == -1)
            break;
        QRect r = separatorRect(i);
        if (clip.contains(r) && !item.hasFixedSize(o))
            paintSep(p, widget, r, o, r.contains(mouse));
    }
}

int QDockAreaLayoutInfo::next(int index) const
{
    for (int i = index + 1; i < item_list.size(); ++i) {
        if (!item_list.at(i).skip())
            return i;
    }
    return -1;
}

int QDockAreaLayoutInfo::prev(int index) const
{
    for (int i = index - 1; i >= 0; --i) {
        if (!item_list.at(i).skip())
            return i;
    }
    return -1;
}

void QDockAreaLayoutInfo::tab(int index, QLayoutItem *dockWidgetItem)
{
#ifdef QT_NO_TABBAR
    Q_UNUSED(index);
    Q_UNUSED(dockWidgetItem);
#else
    if (tabbed) {
        item_list.append(QDockAreaLayoutItem(dockWidgetItem));
        updateTabBar();
        setCurrentTab(dockWidgetItem->widget());
    } else {
        QDockAreaLayoutInfo *new_info
            = new QDockAreaLayoutInfo(sep, dockPos, o, tabBarShape, mainWindow);
        item_list[index].subinfo = new_info;
        new_info->item_list.append(item_list.at(index).widgetItem);
        item_list[index].widgetItem = 0;
        new_info->item_list.append(dockWidgetItem);
        new_info->tabbed = true;
        new_info->updateTabBar();
        new_info->setCurrentTab(dockWidgetItem->widget());
    }
#endif // QT_NO_TABBAR
}

void QDockAreaLayoutInfo::split(int index, Qt::Orientation orientation,
                                       QLayoutItem *dockWidgetItem)
{
    if (orientation == o) {
        item_list.insert(index + 1, QDockAreaLayoutItem(dockWidgetItem));
    } else {
#ifdef QT_NO_TABBAR
        const int tabBarShape = 0;
#endif
        QDockAreaLayoutInfo *new_info
            = new QDockAreaLayoutInfo(sep, dockPos, orientation, tabBarShape, mainWindow);
        item_list[index].subinfo = new_info;
        new_info->item_list.append(item_list.at(index).widgetItem);
        item_list[index].widgetItem = 0;
        new_info->item_list.append(dockWidgetItem);
    }
}

QDockAreaLayoutItem &QDockAreaLayoutInfo::item(const QList<int> &path)
{
    Q_ASSERT(!path.isEmpty());
    const int index = path.first();
    if (path.count() > 1) {
        const QDockAreaLayoutItem &item = item_list[index];
        Q_ASSERT(item.subinfo != 0);
        return item.subinfo->item(path.mid(1));
    }
    return item_list[index];
}

QLayoutItem *QDockAreaLayoutInfo::itemAt(int *x, int index) const
{
    for (int i = 0; i < item_list.count(); ++i) {
        const QDockAreaLayoutItem &item = item_list.at(i);
        if (item.placeHolderItem != 0)
            continue;
        if (item.subinfo) {
            if (QLayoutItem *ret = item.subinfo->itemAt(x, index))
                return ret;
        } else if (item.widgetItem) {
            if ((*x)++ == index)
                return item.widgetItem;
        }
    }
    return 0;
}

QLayoutItem *QDockAreaLayoutInfo::takeAt(int *x, int index)
{
    for (int i = 0; i < item_list.count(); ++i) {
        QDockAreaLayoutItem &item = item_list[i];
        if (item.placeHolderItem != 0)
            continue;
        else if (item.subinfo) {
            if (QLayoutItem *ret = item.subinfo->takeAt(x, index)) {
                unnest(i);
                return ret;
            }
        } else if (item.widgetItem) {
            if ((*x)++ == index) {
                item.placeHolderItem = new QPlaceHolderItem(item.widgetItem->widget());
                QLayoutItem *ret = item.widgetItem;
                item.widgetItem = 0;
                if (item.size != -1)
                    item.flags |= QDockAreaLayoutItem::KeepSize;
                return ret;
            }
        }
    }
    return 0;
}

void QDockAreaLayoutInfo::deleteAllLayoutItems()
{
    for (int i = 0; i < item_list.count(); ++i) {
        QDockAreaLayoutItem &item= item_list[i];
        if (item.subinfo) {
            item.subinfo->deleteAllLayoutItems();
        } else {
            delete item.widgetItem;
            item.widgetItem = 0;
        }
    }
}

void QDockAreaLayoutInfo::saveState(QDataStream &stream) const
{
#ifndef QT_NO_TABBAR
    if (tabbed) {
        stream << (uchar) TabMarker;

        // write the index in item_list of the widget that's currently on top.
        quintptr id = currentTabId();
        int index = -1;
        for (int i = 0; i < item_list.count(); ++i) {
            if (tabId(item_list.at(i)) == id) {
                index = i;
                break;
            }
        }
        stream << index;
    } else
#endif // QT_NO_TABBAR
    {
        stream << (uchar) SequenceMarker;
    }

    stream << (uchar) o << item_list.count();

    for (int i = 0; i < item_list.count(); ++i) {
        const QDockAreaLayoutItem &item = item_list.at(i);
        if (item.widgetItem != 0) {
            stream << (uchar) WidgetMarker;
            QWidget *w = item.widgetItem->widget();
            QString name = w->objectName();
            if (name.isEmpty()) {
                qWarning("QMainWindow::saveState(): 'objectName' not set for QDockWidget %p '%s;",
                         w, qPrintable(w->windowTitle()));
            }
            stream << name;

            uchar flags = 0;
            if (!w->isHidden())
                flags |= StateFlagVisible;
            if (w->isWindow())
                flags |= StateFlagFloating;
            stream << flags;

            if (w->isWindow()) {
                stream << w->x() << w->y() << w->width() << w->height();
            } else {
                stream << item.pos << item.size << pick(o, item.minimumSize())
                        << pick(o, item.maximumSize());
            }
        } else if (item.placeHolderItem != 0) {
            stream << (uchar) WidgetMarker;
            stream << item.placeHolderItem->objectName;
            uchar flags = 0;
            if (!item.placeHolderItem->hidden)
                flags |= StateFlagVisible;
            if (item.placeHolderItem->window)
                flags |= StateFlagFloating;
            stream << flags;
            if (item.placeHolderItem->window) {
                QRect r = item.placeHolderItem->topLevelRect;
                stream << r.x() << r.y() << r.width() << r.height();
            } else {
                stream << item.pos << item.size << (int)0 << (int)0;
            }
        } else if (item.subinfo != 0) {
            stream << (uchar) SequenceMarker << item.pos << item.size << pick(o, item.minimumSize()) << pick(o, item.maximumSize());
            item.subinfo->saveState(stream);
        }
    }
}

static Qt::DockWidgetArea toDockWidgetArea(QInternal::DockPosition pos)
{
    switch (pos) {
        case QInternal::LeftDock:   return Qt::LeftDockWidgetArea;
        case QInternal::RightDock:  return Qt::RightDockWidgetArea;
        case QInternal::TopDock:    return Qt::TopDockWidgetArea;
        case QInternal::BottomDock: return Qt::BottomDockWidgetArea;
        default: break;
    }
    return Qt::NoDockWidgetArea;
}

static QRect constrainedRect(QRect rect, const QRect &desktop)
{
    if (desktop.isValid()) {
        rect.setWidth(qMin(rect.width(), desktop.width()));
        rect.setHeight(qMin(rect.height(), desktop.height()));
        rect.moveLeft(qMax(rect.left(), desktop.left()));
        rect.moveTop(qMax(rect.top(), desktop.top()));
        rect.moveRight(qMin(rect.right(), desktop.right()));
        rect.moveBottom(qMin(rect.bottom(), desktop.bottom()));
    }

    return rect;
}

bool QDockAreaLayoutInfo::restoreState(QDataStream &stream, QList<QDockWidget*> &widgets, bool testing)
{
    uchar marker;
    stream >> marker;
    if (marker != TabMarker && marker != SequenceMarker)
        return false;

#ifndef QT_NO_TABBAR
    tabbed = marker == TabMarker;

    int index = -1;
    if (tabbed)
        stream >> index;
#endif

    uchar orientation;
    stream >> orientation;
    o = static_cast<Qt::Orientation>(orientation);

    int cnt;
    stream >> cnt;

    for (int i = 0; i < cnt; ++i) {
        uchar nextMarker;
        stream >> nextMarker;
        if (nextMarker == WidgetMarker) {
            QString name;
            uchar flags;
            stream >> name >> flags;
            if (name.isEmpty()) {
                int dummy;
                stream >> dummy >> dummy >> dummy >> dummy;
                continue;
            }

            QDockWidget *widget = 0;
            for (int j = 0; j < widgets.count(); ++j) {
                if (widgets.at(j)->objectName() == name) {
                    widget = widgets.takeAt(j);
                    break;
                }
            }

            if (widget == 0) {
                QPlaceHolderItem *placeHolder = new QPlaceHolderItem;
                QDockAreaLayoutItem item(placeHolder);

                placeHolder->objectName = name;
                placeHolder->window = flags & StateFlagFloating;
                placeHolder->hidden = !(flags & StateFlagVisible);
                if (placeHolder->window) {
                    int x, y, w, h;
                    stream >> x >> y >> w >> h;
                    placeHolder->topLevelRect = QRect(x, y, w, h);
                } else {
                    int dummy;
                    stream >> item.pos >> item.size >> dummy >> dummy;
                }
                if (item.size != -1)
                    item.flags |= QDockAreaLayoutItem::KeepSize;
                if (!testing)
                    item_list.append(item);
            } else {
                QDockAreaLayoutItem item(new QDockWidgetItem(widget));
                if (flags & StateFlagFloating) {
               	    bool drawer = false;
#ifdef Q_WS_MAC // drawer support
               	    extern bool qt_mac_is_macdrawer(const QWidget *); //qwidget_mac.cpp
                    extern bool qt_mac_set_drawer_preferred_edge(QWidget *, Qt::DockWidgetArea); //qwidget_mac.cpp
                    drawer = qt_mac_is_macdrawer(widget);
#endif

                    if (!testing) {
                        widget->hide();
                        if (!drawer)
                            widget->setFloating(true);
                    }

                    int x, y, w, h;
                    stream >> x >> y >> w >> h;

#ifdef Q_WS_MAC // drawer support
                    if (drawer) {
                        mainWindow->window()->createWinId();
                        widget->window()->createWinId();
                        qt_mac_set_drawer_preferred_edge(widget, toDockWidgetArea(dockPos));
                    } else
#endif
                    if (!testing) {
                        QRect r(x, y, w, h);
                        QDesktopWidget *desktop = QApplication::desktop();
                        if (desktop->isVirtualDesktop())
                            r = constrainedRect(r, desktop->screenGeometry(desktop->screenNumber(r.topLeft())));
                        else
                            r = constrainedRect(r, desktop->screenGeometry(widget));
                        widget->move(r.topLeft());
                        widget->resize(r.size());
                    }

                    if (!testing) {
                        widget->setVisible(flags & StateFlagVisible);
                        item_list.append(item);
                    }
                } else {
                    int dummy;
                    stream >> item.pos >> item.size >> dummy >> dummy;
                    if (!testing) {
                        item_list.append(item);
                        widget->setFloating(false);
                        widget->setVisible(flags & StateFlagVisible);
                        emit widget->dockLocationChanged(toDockWidgetArea(dockPos));
                    }
                }
		if (testing) {
		  //was it is not really added to the layout, we need to delete the object here
		  delete item.widgetItem;
		}
            }
        } else if (nextMarker == SequenceMarker) {
            int dummy;
#ifdef QT_NO_TABBAR
            const int tabBarShape = 0;
#endif
            QDockAreaLayoutItem item(new QDockAreaLayoutInfo(sep, dockPos, o,
                                                                tabBarShape, mainWindow));
            stream >> item.pos >> item.size >> dummy >> dummy;
            //we need to make sure the element is in the list so the dock widget can eventually be docked correctly
            if (!testing)
                item_list.append(item);
            
            //here we need to make sure we change the item in the item_list
            QDockAreaLayoutItem &lastItem = testing ? item : item_list.last();

            if (!lastItem.subinfo->restoreState(stream, widgets, testing))
                return false;

        } else {
            return false;
        }
    }

#ifndef QT_NO_TABBAR
    if (!testing && tabbed && index >= 0 && index < item_list.count()) {
        updateTabBar();
        setCurrentTabId(tabId(item_list.at(index)));
    }
    if (!testing && *sep == 1)
        updateSeparatorWidgets();
#endif

    return true;
}

#ifndef QT_NO_TABBAR
void QDockAreaLayoutInfo::updateSeparatorWidgets() const
{
    if (tabbed) {
        separatorWidgets.clear();
        return;
    }

    int j = 0;
    for (int i = 0; i < item_list.count(); ++i) {
        const QDockAreaLayoutItem &item = item_list.at(i);

        if (item.skip())
            continue;

        int next = this->next(i);
        if ((item.flags & QDockAreaLayoutItem::GapItem)
                || (next != -1 && (item_list.at(next).flags & QDockAreaLayoutItem::GapItem)))
            continue;

        if (item.subinfo) {
            item.subinfo->updateSeparatorWidgets();
        }

        if (next == -1)
            break;

        QWidget *sepWidget;
        if (j < separatorWidgets.size() && separatorWidgets.at(j)) {
            sepWidget = separatorWidgets.at(j);
        } else {
            sepWidget = mainWindowLayout()->getSeparatorWidget();
            separatorWidgets.append(sepWidget);
        }
        j++;

#ifndef QT_MAC_USE_COCOA
        sepWidget->raise();
#endif
        QRect sepRect = separatorRect(i).adjusted(-2, -2, 2, 2);
        sepWidget->setGeometry(sepRect);
        sepWidget->setMask( QRegion(separatorRect(i).translated( - sepRect.topLeft())));
        sepWidget->show();
    }

    for (int k = j; k < separatorWidgets.size(); ++k) {
        separatorWidgets[k]->hide();
    }
    separatorWidgets.resize(j);
    Q_ASSERT(separatorWidgets.size() == j);
}
#endif //QT_NO_TABBAR

#ifndef QT_NO_TABBAR
//returns whether the tabbar is visible or not
bool QDockAreaLayoutInfo::updateTabBar() const
{
    if (!tabbed)
        return false;

    QDockAreaLayoutInfo *that = const_cast<QDockAreaLayoutInfo*>(this);

    if (that->tabBar == 0) {
        that->tabBar = mainWindowLayout()->getTabBar();
        that->tabBar->setShape(static_cast<QTabBar::Shape>(tabBarShape));
        that->tabBar->setDrawBase(true);
    }

    bool blocked = tabBar->blockSignals(true);
    bool gap = false;

    int tab_idx = 0;
    for (int i = 0; i < item_list.count(); ++i) {
        const QDockAreaLayoutItem &item = item_list.at(i);
        if (item.skip())
            continue;
        if (item.flags & QDockAreaLayoutItem::GapItem) {
            gap = true;
            continue;
        }
        if (item.widgetItem == 0)
            continue;

        QDockWidget *dw = qobject_cast<QDockWidget*>(item.widgetItem->widget());
        QString title = dw->d_func()->fixedWindowTitle;
        quintptr id = tabId(item);
        if (tab_idx == tabBar->count()) {
            tabBar->insertTab(tab_idx, title);
#ifndef QT_NO_TOOLTIP
            tabBar->setTabToolTip(tab_idx, title);
#endif
            tabBar->setTabData(tab_idx, id);
        } else if (qvariant_cast<quintptr>(tabBar->tabData(tab_idx)) != id) {
            if (tab_idx + 1 < tabBar->count()
                    && qvariant_cast<quintptr>(tabBar->tabData(tab_idx + 1)) == id)
                tabBar->removeTab(tab_idx);
            else {
                tabBar->insertTab(tab_idx, title);
#ifndef QT_NO_TOOLTIP
                tabBar->setTabToolTip(tab_idx, title);
#endif
                tabBar->setTabData(tab_idx, id);
            }
        }

        if (title != tabBar->tabText(tab_idx)) {
            tabBar->setTabText(tab_idx, title);
#ifndef QT_NO_TOOLTIP
            tabBar->setTabToolTip(tab_idx, title);
#endif
        }

        ++tab_idx;
    }

    while (tab_idx < tabBar->count()) {
        tabBar->removeTab(tab_idx);
    }

    tabBar->blockSignals(blocked);

    //returns if the tabbar is visible or not
    return ( (gap ? 1 : 0) + tabBar->count()) > 1;
}

void QDockAreaLayoutInfo::setTabBarShape(int shape)
{
    if (shape == tabBarShape)
        return;
    tabBarShape = shape;
    if (tabBar != 0)
        tabBar->setShape(static_cast<QTabBar::Shape>(shape));

    for (int i = 0; i < item_list.count(); ++i) {
        QDockAreaLayoutItem &item = item_list[i];
        if (item.subinfo != 0)
            item.subinfo->setTabBarShape(shape);
    }
}

QSize QDockAreaLayoutInfo::tabBarMinimumSize() const
{
    if (!updateTabBar())
        return QSize(0, 0);

    return tabBar->minimumSizeHint();
}

QSize QDockAreaLayoutInfo::tabBarSizeHint() const
{
    if (!updateTabBar())
        return QSize(0, 0);

    return tabBar->sizeHint();
}

QSet<QTabBar*> QDockAreaLayoutInfo::usedTabBars() const
{
    QSet<QTabBar*> result;

    if (tabbed) {
        updateTabBar();
        result.insert(tabBar);
    }

    for (int i = 0; i < item_list.count(); ++i) {
        const QDockAreaLayoutItem &item = item_list.at(i);
        if (item.subinfo != 0)
            result += item.subinfo->usedTabBars();
    }

    return result;
}

// returns a set of all used separator widgets for this dockarelayout info
// and all subinfos
QSet<QWidget*> QDockAreaLayoutInfo::usedSeparatorWidgets() const
{
    QSet<QWidget*> result;

    for (int i = 0; i < separatorWidgets.count(); ++i)
        result << separatorWidgets.at(i);

    for (int i = 0; i < item_list.count(); ++i) {
        const QDockAreaLayoutItem &item = item_list.at(i);
        if (item.subinfo != 0)
            result += item.subinfo->usedSeparatorWidgets();
    }

    return result;
}

QRect QDockAreaLayoutInfo::tabContentRect() const
{
    if (!tabbed)
        return QRect();

    QRect result = rect;
    QSize tbh = tabBarSizeHint();

    if (!tbh.isNull()) {
        switch (tabBarShape) {
            case QTabBar::RoundedNorth:
            case QTabBar::TriangularNorth:
                result.adjust(0, tbh.height(), 0, 0);
                break;
            case QTabBar::RoundedSouth:
            case QTabBar::TriangularSouth:
                result.adjust(0, 0, 0, -tbh.height());
                break;
            case QTabBar::RoundedEast:
            case QTabBar::TriangularEast:
                result.adjust(0, 0, -tbh.width(), 0);
                break;
            case QTabBar::RoundedWest:
            case QTabBar::TriangularWest:
                result.adjust(tbh.width(), 0, 0, 0);
                break;
            default:
                break;
        }
    }

    return result;
}
#endif // QT_NO_TABBAR

/******************************************************************************
** QDockAreaLayout
*/

QDockAreaLayout::QDockAreaLayout(QMainWindow *win) : fallbackToSizeHints(true)
{
    mainWindow = win;
    sep = win->style()->pixelMetric(QStyle::PM_DockWidgetSeparatorExtent, 0, win);
#ifndef QT_NO_TABBAR
    const int tabShape = QTabBar::RoundedSouth;
#else
    const int tabShape = 0;
#endif
    docks[QInternal::LeftDock]
        = QDockAreaLayoutInfo(&sep, QInternal::LeftDock, Qt::Vertical, tabShape, win);
    docks[QInternal::RightDock]
        = QDockAreaLayoutInfo(&sep, QInternal::RightDock, Qt::Vertical, tabShape, win);
    docks[QInternal::TopDock]
        = QDockAreaLayoutInfo(&sep, QInternal::TopDock, Qt::Horizontal, tabShape, win);
    docks[QInternal::BottomDock]
        = QDockAreaLayoutInfo(&sep, QInternal::BottomDock, Qt::Horizontal, tabShape, win);
    centralWidgetItem = 0;


    corners[Qt::TopLeftCorner] = Qt::TopDockWidgetArea;
    corners[Qt::TopRightCorner] = Qt::TopDockWidgetArea;
    corners[Qt::BottomLeftCorner] = Qt::BottomDockWidgetArea;
    corners[Qt::BottomRightCorner] = Qt::BottomDockWidgetArea;
}

bool QDockAreaLayout::isValid() const
{
    return rect.isValid();
}

void QDockAreaLayout::saveState(QDataStream &stream) const
{
    stream << (uchar) DockWidgetStateMarker;
    int cnt = 0;
    for (int i = 0; i < QInternal::DockCount; ++i) {
        if (!docks[i].item_list.isEmpty())
            ++cnt;
    }
    stream << cnt;
    for (int i = 0; i < QInternal::DockCount; ++i) {
        if (docks[i].item_list.isEmpty())
            continue;
        stream << i << docks[i].rect.size();
        docks[i].saveState(stream);
    }

    stream << centralWidgetRect.size();

    for (int i = 0; i < 4; ++i)
        stream << static_cast<int>(corners[i]);
}

bool QDockAreaLayout::restoreState(QDataStream &stream, const QList<QDockWidget*> &_dockwidgets, bool testing)
{
    QList<QDockWidget*> dockwidgets = _dockwidgets;

    int cnt;
    stream >> cnt;
    for (int i = 0; i < cnt; ++i) {
        int pos;
        stream >> pos;
        QSize size;
        stream >> size;
        if (!testing) {
            docks[pos].rect = QRect(QPoint(0, 0), size);
        }
        if (!docks[pos].restoreState(stream, dockwidgets, testing)) {
            stream.setStatus(QDataStream::ReadCorruptData);
            return false;
        }
    }

    QSize size;
    stream >> size;
    centralWidgetRect = QRect(QPoint(0, 0), size);

    bool ok = stream.status() == QDataStream::Ok;

    if (ok) {
        int cornerData[4];
        for (int i = 0; i < 4; ++i)
            stream >> cornerData[i];
        if (stream.status() == QDataStream::Ok) {
            for (int i = 0; i < 4; ++i)
                corners[i] = static_cast<Qt::DockWidgetArea>(cornerData[i]);
        }

        if (!testing)
            fallbackToSizeHints = false;
    }

    return ok;
}

QList<int> QDockAreaLayout::indexOfPlaceHolder(const QString &objectName) const
{
    for (int i = 0; i < QInternal::DockCount; ++i) {
        QList<int> result = docks[i].indexOfPlaceHolder(objectName);
        if (!result.isEmpty()) {
            result.prepend(i);
            return result;
        }
    }
    return QList<int>();
}

QList<int> QDockAreaLayout::indexOf(QWidget *dockWidget) const
{
    for (int i = 0; i < QInternal::DockCount; ++i) {
        QList<int> result = docks[i].indexOf(dockWidget);
        if (!result.isEmpty()) {
            result.prepend(i);
            return result;
        }
    }
    return QList<int>();
}

QList<int> QDockAreaLayout::gapIndex(const QPoint &pos) const
{
    QMainWindow::DockOptions opts = mainWindow->dockOptions();
    bool nestingEnabled = opts & QMainWindow::AllowNestedDocks;
    QDockAreaLayoutInfo::TabMode tabMode = QDockAreaLayoutInfo::NoTabs;
#ifndef QT_NO_TABBAR
    if (opts & QMainWindow::AllowTabbedDocks
        || opts & QMainWindow::VerticalTabs)
        tabMode = QDockAreaLayoutInfo::AllowTabs;
    if (opts & QMainWindow::ForceTabbedDocks)
        tabMode = QDockAreaLayoutInfo::ForceTabs;

    if (tabMode == QDockAreaLayoutInfo::ForceTabs)
        nestingEnabled = false;
#endif


    for (int i = 0; i < QInternal::DockCount; ++i) {
        const QDockAreaLayoutInfo &info = docks[i];

        if (!info.isEmpty() && info.rect.contains(pos)) {
            QList<int> result
                = docks[i].gapIndex(pos, nestingEnabled, tabMode);
            if (!result.isEmpty())
                result.prepend(i);
            return result;
        }
    }

    for (int i = 0; i < QInternal::DockCount; ++i) {
        const QDockAreaLayoutInfo &info = docks[i];

        if (info.isEmpty()) {
            QRect r;
            switch (i) {
                case QInternal::LeftDock:
                    r = QRect(rect.left(), rect.top(), EmptyDropAreaSize, rect.height());
                    break;
                case QInternal::RightDock:
                    r = QRect(rect.right() - EmptyDropAreaSize, rect.top(),
                                EmptyDropAreaSize, rect.height());
                    break;
                case QInternal::TopDock:
                    r = QRect(rect.left(), rect.top(), rect.width(), EmptyDropAreaSize);
                    break;
                case QInternal::BottomDock:
                    r = QRect(rect.left(), rect.bottom() - EmptyDropAreaSize,
                                rect.width(), EmptyDropAreaSize);
                    break;
            }
            if (r.contains(pos)) {
                if (opts & QMainWindow::ForceTabbedDocks && !info.item_list.isEmpty()) {
                    //in case of ForceTabbedDocks, we pass -1 in order to force the gap to be tabbed
                    //it mustn't be completely empty otherwise it won't work
                    return QList<int>() << i << -1 << 0;
                } else {
                    return QList<int>() << i << 0;
                }
            }
        }
    }

    return QList<int>();
}

QList<int> QDockAreaLayout::findSeparator(const QPoint &pos) const
{
    QList<int> result;
    for (int i = 0; i < QInternal::DockCount; ++i) {
        const QDockAreaLayoutInfo &info = docks[i];
        if (info.isEmpty())
            continue;
        QRect rect = separatorRect(i);
        if (!rect.isNull() && sep == 1)
            rect.adjust(-2, -2, 2, 2);
        if (rect.contains(pos) && !info.hasFixedSize()) {
            result << i;
            break;
        } else if (info.rect.contains(pos)) {
            result = docks[i].findSeparator(pos);
            if (!result.isEmpty()) {
                result.prepend(i);
                break;
            }
        }
    }

    return result;
}

QDockAreaLayoutInfo *QDockAreaLayout::info(QWidget *widget)
{
    for (int i = 0; i < QInternal::DockCount; ++i) {
        if (QDockAreaLayoutInfo *result = docks[i].info(widget))
            return result;
    }

    return 0;
}

QDockAreaLayoutInfo *QDockAreaLayout::info(const QList<int> &path)
{
    Q_ASSERT(!path.isEmpty());
    const int index = path.first();
    Q_ASSERT(index >= 0 && index < QInternal::DockCount);

    if (path.count() == 1)
        return &docks[index];

    return docks[index].info(path.mid(1));
}

const QDockAreaLayoutInfo *QDockAreaLayout::info(const QList<int> &path) const
{
    return const_cast<QDockAreaLayout*>(this)->info(path);
}

QDockAreaLayoutItem &QDockAreaLayout::item(const QList<int> &path)
{
    Q_ASSERT(!path.isEmpty());
    const int index = path.first();
    Q_ASSERT(index >= 0 && index < QInternal::DockCount);
    return docks[index].item(path.mid(1));
}

QRect QDockAreaLayout::itemRect(const QList<int> &path) const
{
    Q_ASSERT(!path.isEmpty());
    const int index = path.first();
    Q_ASSERT(index >= 0 && index < QInternal::DockCount);
    return docks[index].itemRect(path.mid(1));
}

QRect QDockAreaLayout::separatorRect(int index) const
{
    const QDockAreaLayoutInfo &dock = docks[index];
    if (dock.isEmpty())
        return QRect();
    QRect r = dock.rect;
    switch (index) {
        case QInternal::LeftDock:
            return QRect(r.right() + 1, r.top(), sep, r.height());
        case QInternal::RightDock:
            return QRect(r.left() - sep, r.top(), sep, r.height());
        case QInternal::TopDock:
            return QRect(r.left(), r.bottom() + 1, r.width(), sep);
        case QInternal::BottomDock:
            return QRect(r.left(), r.top() - sep, r.width(), sep);
        default:
            break;
    }
    return QRect();
}

QRect QDockAreaLayout::separatorRect(const QList<int> &path) const
{
    Q_ASSERT(!path.isEmpty());

    const int index = path.first();
    Q_ASSERT(index >= 0 && index < QInternal::DockCount);

    if (path.count() == 1)
        return separatorRect(index);
    else
        return docks[index].separatorRect(path.mid(1));
}

bool QDockAreaLayout::insertGap(const QList<int> &path, QLayoutItem *dockWidgetItem)
{
    Q_ASSERT(!path.isEmpty());
    const int index = path.first();
    Q_ASSERT(index >= 0 && index < QInternal::DockCount);
    return docks[index].insertGap(path.mid(1), dockWidgetItem);
}

QLayoutItem *QDockAreaLayout::plug(const QList<int> &path)
{
    Q_ASSERT(!path.isEmpty());
    const int index = path.first();
    Q_ASSERT(index >= 0 && index < QInternal::DockCount);
    return docks[index].plug(path.mid(1));
}

QLayoutItem *QDockAreaLayout::unplug(const QList<int> &path)
{
    Q_ASSERT(!path.isEmpty());
    const int index = path.first();
    Q_ASSERT(index >= 0 && index < QInternal::DockCount);
    return docks[index].unplug(path.mid(1));
}

void QDockAreaLayout::remove(const QList<int> &path)
{
    Q_ASSERT(!path.isEmpty());
    const int index = path.first();
    Q_ASSERT(index >= 0 && index < QInternal::DockCount);
    docks[index].remove(path.mid(1));
}

static inline int qMin(int i1, int i2, int i3) { return qMin(i1, qMin(i2, i3)); }
static inline int qMax(int i1, int i2, int i3) { return qMax(i1, qMax(i2, i3)); }

void QDockAreaLayout::getGrid(QVector<QLayoutStruct> *_ver_struct_list,
                                QVector<QLayoutStruct> *_hor_struct_list)
{
    QSize center_hint(0, 0);
    QSize center_min(0, 0);
    const bool have_central = centralWidgetItem != 0 && !centralWidgetItem->isEmpty();
    if (have_central) {
        center_hint = centralWidgetRect.size();
        if (!center_hint.isValid())
            center_hint = centralWidgetItem->sizeHint();
        center_min = centralWidgetItem->minimumSize();
    }

    QRect center_rect = rect;
    if (!docks[QInternal::LeftDock].isEmpty())
        center_rect.setLeft(rect.left() + docks[QInternal::LeftDock].rect.width() + sep);
    if (!docks[QInternal::TopDock].isEmpty())
        center_rect.setTop(rect.top() + docks[QInternal::TopDock].rect.height() + sep);
    if (!docks[QInternal::RightDock].isEmpty())
        center_rect.setRight(rect.right() - docks[QInternal::RightDock].rect.width() - sep);
    if (!docks[QInternal::BottomDock].isEmpty())
        center_rect.setBottom(rect.bottom() - docks[QInternal::BottomDock].rect.height() - sep);

    QSize left_hint = docks[QInternal::LeftDock].size();
    if (left_hint.isNull() || fallbackToSizeHints)
        left_hint = docks[QInternal::LeftDock].sizeHint();
    QSize left_min = docks[QInternal::LeftDock].minimumSize();
    QSize left_max = docks[QInternal::LeftDock].maximumSize();
    left_hint = left_hint.boundedTo(left_max).expandedTo(left_min);

    QSize right_hint = docks[QInternal::RightDock].size();
    if (right_hint.isNull() || fallbackToSizeHints)
        right_hint = docks[QInternal::RightDock].sizeHint();
    QSize right_min = docks[QInternal::RightDock].minimumSize();
    QSize right_max = docks[QInternal::RightDock].maximumSize();
    right_hint = right_hint.boundedTo(right_max).expandedTo(right_min);

    QSize top_hint = docks[QInternal::TopDock].size();
    if (top_hint.isNull() || fallbackToSizeHints)
        top_hint = docks[QInternal::TopDock].sizeHint();
    QSize top_min = docks[QInternal::TopDock].minimumSize();
    QSize top_max = docks[QInternal::TopDock].maximumSize();
    top_hint = top_hint.boundedTo(top_max).expandedTo(top_min);

    QSize bottom_hint = docks[QInternal::BottomDock].size();
    if (bottom_hint.isNull() || fallbackToSizeHints)
        bottom_hint = docks[QInternal::BottomDock].sizeHint();
    QSize bottom_min = docks[QInternal::BottomDock].minimumSize();
    QSize bottom_max = docks[QInternal::BottomDock].maximumSize();
    bottom_hint = bottom_hint.boundedTo(bottom_max).expandedTo(bottom_min);

    fallbackToSizeHints = false;

    if (_ver_struct_list != 0) {
        QVector<QLayoutStruct> &ver_struct_list = *_ver_struct_list;
        ver_struct_list.resize(3);

        // top --------------------------------------------------
        ver_struct_list[0].init();
        ver_struct_list[0].stretch = 0;
        ver_struct_list[0].sizeHint = top_hint.height();
        ver_struct_list[0].minimumSize = top_min.height();
        ver_struct_list[0].maximumSize = top_max.height();
        ver_struct_list[0].expansive = false;
        ver_struct_list[0].empty = docks[QInternal::TopDock].isEmpty();
        ver_struct_list[0].pos = docks[QInternal::TopDock].rect.top();
        ver_struct_list[0].size = docks[QInternal::TopDock].rect.height();

        // center --------------------------------------------------
        ver_struct_list[1].init();
        ver_struct_list[1].stretch = center_hint.height();

        bool tl_significant = corners[Qt::TopLeftCorner] == Qt::TopDockWidgetArea
                                    || docks[QInternal::TopDock].isEmpty();
        bool bl_significant = corners[Qt::BottomLeftCorner] == Qt::BottomDockWidgetArea
                                    || docks[QInternal::BottomDock].isEmpty();
        bool tr_significant = corners[Qt::TopRightCorner] == Qt::TopDockWidgetArea
                                    || docks[QInternal::TopDock].isEmpty();
        bool br_significant = corners[Qt::BottomRightCorner] == Qt::BottomDockWidgetArea
                                    || docks[QInternal::BottomDock].isEmpty();

        int left = (tl_significant && bl_significant) ? left_hint.height() : 0;
        int right = (tr_significant && br_significant) ? right_hint.height() : 0;
        ver_struct_list[1].sizeHint = qMax(left, center_hint.height(), right);

        left = (tl_significant && bl_significant) ? left_min.height() : 0;
        right = (tr_significant && br_significant) ? right_min.height() : 0;
        ver_struct_list[1].minimumSize = qMax(left, center_min.height(), right);
        ver_struct_list[1].maximumSize = have_central ? QWIDGETSIZE_MAX : 0;
        ver_struct_list[1].expansive = have_central;
        ver_struct_list[1].empty = docks[QInternal::LeftDock].isEmpty()
                                        && !have_central
                                        && docks[QInternal::RightDock].isEmpty();
        ver_struct_list[1].pos = center_rect.top();
        ver_struct_list[1].size = center_rect.height();

        // bottom --------------------------------------------------
        ver_struct_list[2].init();
        ver_struct_list[2].stretch = 0;
        ver_struct_list[2].sizeHint = bottom_hint.height();
        ver_struct_list[2].minimumSize = bottom_min.height();
        ver_struct_list[2].maximumSize = bottom_max.height();
        ver_struct_list[2].expansive = false;
        ver_struct_list[2].empty = docks[QInternal::BottomDock].isEmpty();
        ver_struct_list[2].pos = docks[QInternal::BottomDock].rect.top();
        ver_struct_list[2].size = docks[QInternal::BottomDock].rect.height();

        for (int i = 0; i < 3; ++i) {
            ver_struct_list[i].sizeHint
                = qMax(ver_struct_list[i].sizeHint, ver_struct_list[i].minimumSize);
        }
    }

    if (_hor_struct_list != 0) {
        QVector<QLayoutStruct> &hor_struct_list = *_hor_struct_list;
        hor_struct_list.resize(3);

        // left --------------------------------------------------
        hor_struct_list[0].init();
        hor_struct_list[0].stretch = 0;
        hor_struct_list[0].sizeHint = left_hint.width();
        hor_struct_list[0].minimumSize = left_min.width();
        hor_struct_list[0].maximumSize = left_max.width();
        hor_struct_list[0].expansive = false;
        hor_struct_list[0].empty = docks[QInternal::LeftDock].isEmpty();
        hor_struct_list[0].pos = docks[QInternal::LeftDock].rect.left();
        hor_struct_list[0].size = docks[QInternal::LeftDock].rect.width();

        // center --------------------------------------------------
        hor_struct_list[1].init();
        hor_struct_list[1].stretch = center_hint.width();

        bool tl_significant = corners[Qt::TopLeftCorner] == Qt::LeftDockWidgetArea
                                    || docks[QInternal::LeftDock].isEmpty();
        bool tr_significant = corners[Qt::TopRightCorner] == Qt::RightDockWidgetArea
                                    || docks[QInternal::RightDock].isEmpty();
        bool bl_significant = corners[Qt::BottomLeftCorner] == Qt::LeftDockWidgetArea
                                    || docks[QInternal::LeftDock].isEmpty();
        bool br_significant = corners[Qt::BottomRightCorner] == Qt::RightDockWidgetArea
                                    || docks[QInternal::RightDock].isEmpty();

        int top = (tl_significant && tr_significant) ? top_hint.width() : 0;
        int bottom = (bl_significant && br_significant) ? bottom_hint.width() : 0;
        hor_struct_list[1].sizeHint = qMax(top, center_hint.width(), bottom);

        top = (tl_significant && tr_significant) ? top_min.width() : 0;
        bottom = (bl_significant && br_significant) ? bottom_min.width() : 0;
        hor_struct_list[1].minimumSize = qMax(top, center_min.width(), bottom);

        hor_struct_list[1].maximumSize = have_central ? QWIDGETSIZE_MAX : 0;
        hor_struct_list[1].expansive = have_central;
        hor_struct_list[1].empty = !have_central;
        hor_struct_list[1].pos = center_rect.left();
        hor_struct_list[1].size = center_rect.width();

        // right --------------------------------------------------
        hor_struct_list[2].init();
        hor_struct_list[2].stretch = 0;
        hor_struct_list[2].sizeHint = right_hint.width();
        hor_struct_list[2].minimumSize = right_min.width();
        hor_struct_list[2].maximumSize = right_max.width();
        hor_struct_list[2].expansive = false;
        hor_struct_list[2].empty = docks[QInternal::RightDock].isEmpty();
        hor_struct_list[2].pos = docks[QInternal::RightDock].rect.left();
        hor_struct_list[2].size = docks[QInternal::RightDock].rect.width();

        for (int i = 0; i < 3; ++i) {
            hor_struct_list[i].sizeHint
                = qMax(hor_struct_list[i].sizeHint, hor_struct_list[i].minimumSize);
        }
    }
}

void QDockAreaLayout::setGrid(QVector<QLayoutStruct> *ver_struct_list,
                                QVector<QLayoutStruct> *hor_struct_list)
{

    // top ---------------------------------------------------

    if (!docks[QInternal::TopDock].isEmpty()) {
        QRect r = docks[QInternal::TopDock].rect;
        if (hor_struct_list != 0) {
            r.setLeft(corners[Qt::TopLeftCorner] == Qt::TopDockWidgetArea
                || docks[QInternal::LeftDock].isEmpty()
                ? rect.left() : hor_struct_list->at(1).pos);
            r.setRight(corners[Qt::TopRightCorner] == Qt::TopDockWidgetArea
                || docks[QInternal::RightDock].isEmpty()
                ? rect.right() : hor_struct_list->at(2).pos - sep - 1);
        }
        if (ver_struct_list != 0) {
            r.setTop(rect.top());
            r.setBottom(ver_struct_list->at(1).pos - sep - 1);
        }
        docks[QInternal::TopDock].rect = r;
        docks[QInternal::TopDock].fitItems();
    }

    // bottom ---------------------------------------------------

    if (!docks[QInternal::BottomDock].isEmpty()) {
        QRect r = docks[QInternal::BottomDock].rect;
        if (hor_struct_list != 0) {
            r.setLeft(corners[Qt::BottomLeftCorner] == Qt::BottomDockWidgetArea
                        || docks[QInternal::LeftDock].isEmpty()
                            ? rect.left() : hor_struct_list->at(1).pos);
            r.setRight(corners[Qt::BottomRightCorner] == Qt::BottomDockWidgetArea
                        || docks[QInternal::RightDock].isEmpty()
                            ? rect.right() : hor_struct_list->at(2).pos - sep - 1);
        }
        if (ver_struct_list != 0) {
            r.setTop(ver_struct_list->at(2).pos);
            r.setBottom(rect.bottom());
        }
        docks[QInternal::BottomDock].rect = r;
        docks[QInternal::BottomDock].fitItems();
    }

    // left ---------------------------------------------------

    if (!docks[QInternal::LeftDock].isEmpty()) {
        QRect r = docks[QInternal::LeftDock].rect;
        if (hor_struct_list != 0) {
            r.setLeft(rect.left());
            r.setRight(hor_struct_list->at(1).pos - sep - 1);
        }
        if (ver_struct_list != 0) {
            r.setTop(corners[Qt::TopLeftCorner] == Qt::LeftDockWidgetArea
                || docks[QInternal::TopDock].isEmpty()
                ? rect.top() : ver_struct_list->at(1).pos);
            r.setBottom(corners[Qt::BottomLeftCorner] == Qt::LeftDockWidgetArea
                || docks[QInternal::BottomDock].isEmpty()
                ? rect.bottom() : ver_struct_list->at(2).pos - sep - 1);
        }
        docks[QInternal::LeftDock].rect = r;
        docks[QInternal::LeftDock].fitItems();
    }

    // right ---------------------------------------------------

    if (!docks[QInternal::RightDock].isEmpty()) {
        QRect r = docks[QInternal::RightDock].rect;
        if (hor_struct_list != 0) {
            r.setLeft(hor_struct_list->at(2).pos);
            r.setRight(rect.right());
        }
        if (ver_struct_list != 0) {
            r.setTop(corners[Qt::TopRightCorner] == Qt::RightDockWidgetArea
                        || docks[QInternal::TopDock].isEmpty()
                            ? rect.top() : ver_struct_list->at(1).pos);
            r.setBottom(corners[Qt::BottomRightCorner] == Qt::RightDockWidgetArea
                        || docks[QInternal::BottomDock].isEmpty()
                            ? rect.bottom() : ver_struct_list->at(2).pos - sep - 1);
        }
        docks[QInternal::RightDock].rect = r;
        docks[QInternal::RightDock].fitItems();
    }

    // center ---------------------------------------------------

    if (hor_struct_list != 0) {
        centralWidgetRect.setLeft(hor_struct_list->at(1).pos);
        centralWidgetRect.setWidth(hor_struct_list->at(1).size);
    }
    if (ver_struct_list != 0) {
        centralWidgetRect.setTop(ver_struct_list->at(1).pos);
        centralWidgetRect.setHeight(ver_struct_list->at(1).size);
    }
}

void QDockAreaLayout::fitLayout()
{
    QVector<QLayoutStruct> ver_struct_list(3);
    QVector<QLayoutStruct> hor_struct_list(3);
    getGrid(&ver_struct_list, &hor_struct_list);

    qGeomCalc(ver_struct_list, 0, 3, rect.top(), rect.height(), sep);
    qGeomCalc(hor_struct_list, 0, 3, rect.left(), rect.width(), sep);

    setGrid(&ver_struct_list, &hor_struct_list);
}

void QDockAreaLayout::clear()
{
    for (int i = 0; i < QInternal::DockCount; ++i)
        docks[i].clear();

    rect = QRect();
    centralWidgetRect = QRect();
}

QSize QDockAreaLayout::sizeHint() const
{
    int left_sep = 0;
    int right_sep = 0;
    int top_sep = 0;
    int bottom_sep = 0;

    if (centralWidgetItem != 0) {
        left_sep = docks[QInternal::LeftDock].isEmpty() ? 0 : sep;
        right_sep = docks[QInternal::RightDock].isEmpty() ? 0 : sep;
        top_sep = docks[QInternal::TopDock].isEmpty() ? 0 : sep;
        bottom_sep = docks[QInternal::BottomDock].isEmpty() ? 0 : sep;
    }

    QSize left = docks[QInternal::LeftDock].sizeHint() + QSize(left_sep, 0);
    QSize right = docks[QInternal::RightDock].sizeHint() + QSize(right_sep, 0);
    QSize top = docks[QInternal::TopDock].sizeHint() + QSize(0, top_sep);
    QSize bottom = docks[QInternal::BottomDock].sizeHint() + QSize(0, bottom_sep);
    QSize center = centralWidgetItem == 0 ? QSize(0, 0) : centralWidgetItem->sizeHint();

    int row1 = top.width();
    int row2 = left.width() + center.width() + right.width();
    int row3 = bottom.width();
    int col1 = left.height();
    int col2 = top.height() + center.height() + bottom.height();
    int col3 = right.height();

    if (corners[Qt::TopLeftCorner] == Qt::LeftDockWidgetArea)
        row1 += left.width();
    else
        col1 += top.height();

    if (corners[Qt::TopRightCorner] == Qt::RightDockWidgetArea)
        row1 += right.width();
    else
        col3 += top.height();

    if (corners[Qt::BottomLeftCorner] == Qt::LeftDockWidgetArea)
        row3 += left.width();
    else
        col1 += bottom.height();

    if (corners[Qt::BottomRightCorner] == Qt::RightDockWidgetArea)
        row3 += right.width();
    else
        col3 += bottom.height();

    return QSize(qMax(row1, row2, row3), qMax(col1, col2, col3));
}

QSize QDockAreaLayout::minimumSize() const
{
    int left_sep = 0;
    int right_sep = 0;
    int top_sep = 0;
    int bottom_sep = 0;

    if (centralWidgetItem != 0) {
        left_sep = docks[QInternal::LeftDock].isEmpty() ? 0 : sep;
        right_sep = docks[QInternal::RightDock].isEmpty() ? 0 : sep;
        top_sep = docks[QInternal::TopDock].isEmpty() ? 0 : sep;
        bottom_sep = docks[QInternal::BottomDock].isEmpty() ? 0 : sep;
    }

    QSize left = docks[QInternal::LeftDock].minimumSize() + QSize(left_sep, 0);
    QSize right = docks[QInternal::RightDock].minimumSize() + QSize(right_sep, 0);
    QSize top = docks[QInternal::TopDock].minimumSize() + QSize(0, top_sep);
    QSize bottom = docks[QInternal::BottomDock].minimumSize() + QSize(0, bottom_sep);
    QSize center = centralWidgetItem == 0 ? QSize(0, 0) : centralWidgetItem->minimumSize();

    int row1 = top.width();
    int row2 = left.width() + center.width() + right.width();
    int row3 = bottom.width();
    int col1 = left.height();
    int col2 = top.height() + center.height() + bottom.height();
    int col3 = right.height();

    if (corners[Qt::TopLeftCorner] == Qt::LeftDockWidgetArea)
        row1 += left.width();
    else
        col1 += top.height();

    if (corners[Qt::TopRightCorner] == Qt::RightDockWidgetArea)
        row1 += right.width();
    else
        col3 += top.height();

    if (corners[Qt::BottomLeftCorner] == Qt::LeftDockWidgetArea)
        row3 += left.width();
    else
        col1 += bottom.height();

    if (corners[Qt::BottomRightCorner] == Qt::RightDockWidgetArea)
        row3 += right.width();
    else
        col3 += bottom.height();

    return QSize(qMax(row1, row2, row3), qMax(col1, col2, col3));
}

bool QDockAreaLayout::restoreDockWidget(QDockWidget *dockWidget)
{
    QList<int> index = indexOfPlaceHolder(dockWidget->objectName());
    if (index.isEmpty())
        return false;

    QDockAreaLayoutItem &item = this->item(index);
    QPlaceHolderItem *placeHolder = item.placeHolderItem;
    Q_ASSERT(placeHolder != 0);

    item.widgetItem = new QDockWidgetItem(dockWidget);

    if (placeHolder->window) {
        QDesktopWidget desktop;
        QRect r = constrainedRect(placeHolder->topLevelRect, desktop.screenGeometry(dockWidget));
        dockWidget->d_func()->setWindowState(true, true, r);
    }
    dockWidget->setVisible(!placeHolder->hidden);
#ifdef Q_WS_X11
    if (placeHolder->window) // gets rid of the X11BypassWindowManager window flag
        dockWidget->d_func()->setWindowState(true);
#endif

    item.placeHolderItem = 0;
    delete placeHolder;

    return true;
}

void QDockAreaLayout::addDockWidget(QInternal::DockPosition pos, QDockWidget *dockWidget,
                                             Qt::Orientation orientation)
{
    QLayoutItem *dockWidgetItem = new QDockWidgetItem(dockWidget);
    QDockAreaLayoutInfo &info = docks[pos];
    if (orientation == info.o || info.item_list.count() <= 1) {
        // empty dock areas, or dock areas containing exactly one widget can have their orientation
        // switched.
        info.o = orientation;

        QDockAreaLayoutItem new_item(dockWidgetItem);
        info.item_list.append(new_item);
#ifndef QT_NO_TABBAR
        if (info.tabbed && !new_item.skip()) {
            info.updateTabBar();
            info.setCurrentTabId(tabId(new_item));
        }
#endif
    } else {
#ifndef QT_NO_TABBAR
        int tbshape = info.tabBarShape;
#else
        int tbshape = 0;
#endif
        QDockAreaLayoutInfo new_info(&sep, pos, orientation, tbshape, mainWindow);
        new_info.item_list.append(new QDockAreaLayoutInfo(info));
        new_info.item_list.append(dockWidgetItem);
        info = new_info;
    }

    QList<int> index = indexOfPlaceHolder(dockWidget->objectName());
    if (!index.isEmpty())
        remove(index);
}

void QDockAreaLayout::tabifyDockWidget(QDockWidget *first, QDockWidget *second)
{
    QList<int> path = indexOf(first);
    if (path.isEmpty())
        return;

    QDockAreaLayoutInfo *info = this->info(path);
    Q_ASSERT(info != 0);
    info->tab(path.last(), new QDockWidgetItem(second));

    QList<int> index = indexOfPlaceHolder(second->objectName());
    if (!index.isEmpty())
        remove(index);
}

void QDockAreaLayout::splitDockWidget(QDockWidget *after,
                                               QDockWidget *dockWidget,
                                               Qt::Orientation orientation)
{
    QList<int> path = indexOf(after);
    if (path.isEmpty())
        return;

    QDockAreaLayoutInfo *info = this->info(path);
    Q_ASSERT(info != 0);
    info->split(path.last(), orientation, new QDockWidgetItem(dockWidget));

    QList<int> index = indexOfPlaceHolder(dockWidget->objectName());
    if (!index.isEmpty())
        remove(index);
}

void QDockAreaLayout::apply(bool animate)
{
    QWidgetAnimator &widgetAnimator = qt_mainwindow_layout(mainWindow)->widgetAnimator;

    for (int i = 0; i < QInternal::DockCount; ++i)
        docks[i].apply(animate);
    if (centralWidgetItem != 0 && !centralWidgetItem->isEmpty()) {
        widgetAnimator.animate(centralWidgetItem->widget(), centralWidgetRect,
                                animate);
    }
#ifndef QT_NO_TABBAR
    if (sep == 1)
        updateSeparatorWidgets();
#endif //QT_NO_TABBAR
}

void QDockAreaLayout::paintSeparators(QPainter *p, QWidget *widget,
                                                const QRegion &clip,
                                                const QPoint &mouse) const
{
    for (int i = 0; i < QInternal::DockCount; ++i) {
        const QDockAreaLayoutInfo &dock = docks[i];
        if (dock.isEmpty())
            continue;
        QRect r = separatorRect(i);
        if (clip.contains(r) && !dock.hasFixedSize()) {
            Qt::Orientation opposite = dock.o == Qt::Horizontal
                                        ? Qt::Vertical : Qt::Horizontal;
            paintSep(p, widget, r, opposite, r.contains(mouse));
        }
        if (clip.contains(dock.rect))
            dock.paintSeparators(p, widget, clip, mouse);
    }
}

QRegion QDockAreaLayout::separatorRegion() const
{
    QRegion result;

    for (int i = 0; i < QInternal::DockCount; ++i) {
        const QDockAreaLayoutInfo &dock = docks[i];
        if (dock.isEmpty())
            continue;
        result |= separatorRect(i);
        result |= dock.separatorRegion();
    }

    return result;
}

int QDockAreaLayout::separatorMove(const QList<int> &separator, const QPoint &origin,
                                                const QPoint &dest)
{
    int delta = 0;
    int index = separator.last();

    if (separator.count() > 1) {
        QDockAreaLayoutInfo *info = this->info(separator);
        delta = pick(info->o, dest - origin);
        if (delta != 0)
            delta = info->separatorMove(index, delta);
        info->apply(false);
        return delta;
    }

    QVector<QLayoutStruct> list;

    if (index == QInternal::LeftDock || index == QInternal::RightDock)
        getGrid(0, &list);
    else
        getGrid(&list, 0);

    int sep_index = index == QInternal::LeftDock || index == QInternal::TopDock
                        ? 0 : 1;
    Qt::Orientation o = index == QInternal::LeftDock || index == QInternal::RightDock
                        ? Qt::Horizontal
                        : Qt::Vertical;

    delta = pick(o, dest - origin);
    delta = separatorMoveHelper(list, sep_index, delta, sep);

    if (index == QInternal::LeftDock || index == QInternal::RightDock)
        setGrid(0, &list);
    else
        setGrid(&list, 0);

    apply(false);

    return delta;
}

#ifndef QT_NO_TABBAR
// Sets the correct positions for the separator widgets
// Allocates new sepearator widgets with getSeparatorWidget
void QDockAreaLayout::updateSeparatorWidgets() const
{
    int j = 0;

    for (int i = 0; i < QInternal::DockCount; ++i) {
        const QDockAreaLayoutInfo &dock = docks[i];
        if (dock.isEmpty())
            continue;

        QWidget *sepWidget;
        if (j < separatorWidgets.size()) {
            sepWidget = separatorWidgets.at(j);
        } else {
            sepWidget = qt_mainwindow_layout(mainWindow)->getSeparatorWidget();
            separatorWidgets.append(sepWidget);
        }
        j++;

#ifndef QT_MAC_USE_COCOA
        sepWidget->raise();
#endif
        QRect sepRect = separatorRect(i).adjusted(-2, -2, 2, 2);
        sepWidget->setGeometry(sepRect);
        sepWidget->setMask( QRegion(separatorRect(i).translated( - sepRect.topLeft())));
        sepWidget->show();
    }
    for (int i = j; i < separatorWidgets.size(); ++i)
        separatorWidgets.at(i)->hide();

    separatorWidgets.resize(j);
}
#endif //QT_NO_TABBAR

QLayoutItem *QDockAreaLayout::itemAt(int *x, int index) const
{
    Q_ASSERT(x != 0);

    for (int i = 0; i < QInternal::DockCount; ++i) {
        const QDockAreaLayoutInfo &dock = docks[i];
        if (QLayoutItem *ret = dock.itemAt(x, index))
            return ret;
    }

    if (centralWidgetItem && (*x)++ == index)
        return centralWidgetItem;

    return 0;
}

QLayoutItem *QDockAreaLayout::takeAt(int *x, int index)
{
    Q_ASSERT(x != 0);

    for (int i = 0; i < QInternal::DockCount; ++i) {
        QDockAreaLayoutInfo &dock = docks[i];
        if (QLayoutItem *ret = dock.takeAt(x, index))
            return ret;
    }

    if (centralWidgetItem && (*x)++ == index) {
        QLayoutItem *ret = centralWidgetItem;
        centralWidgetItem = 0;
        return ret;
    }

    return 0;
}

void QDockAreaLayout::deleteAllLayoutItems()
{
    for (int i = 0; i < QInternal::DockCount; ++i)
        docks[i].deleteAllLayoutItems();
}

#ifndef QT_NO_TABBAR
QSet<QTabBar*> QDockAreaLayout::usedTabBars() const
{
    QSet<QTabBar*> result;
    for (int i = 0; i < QInternal::DockCount; ++i) {
        const QDockAreaLayoutInfo &dock = docks[i];
        result += dock.usedTabBars();
    }
    return result;
}

// Returns the set of all used separator widgets
QSet<QWidget*> QDockAreaLayout::usedSeparatorWidgets() const
{
    QSet<QWidget*> result;

    for (int i = 0; i < separatorWidgets.count(); ++i)
        result << separatorWidgets.at(i);
    for (int i = 0; i < QInternal::DockCount; ++i) {
        const QDockAreaLayoutInfo &dock = docks[i];
        result += dock.usedSeparatorWidgets();
    }
    return result;
}
#endif

QRect QDockAreaLayout::gapRect(const QList<int> &path) const
{
    const QDockAreaLayoutInfo *info = this->info(path);
    if (info == 0)
        return QRect();
    const QList<QDockAreaLayoutItem> &item_list = info->item_list;
    Qt::Orientation o = info->o;
    int index = path.last();
    if (index < 0 || index >= item_list.count())
        return QRect();
    const QDockAreaLayoutItem &item = item_list.at(index);
    if (!(item.flags & QDockAreaLayoutItem::GapItem))
        return QRect();

    QRect result;

#ifndef QT_NO_TABBAR
    if (info->tabbed) {
        result = info->tabContentRect();
    } else
#endif
    {
        int pos = item.pos;
        int size = item.size;

        int prev = info->prev(index);
        int next = info->next(index);

        if (prev != -1 && !(item_list.at(prev).flags & QDockAreaLayoutItem::GapItem)) {
            pos += sep;
            size -= sep;
        }
        if (next != -1 && !(item_list.at(next).flags & QDockAreaLayoutItem::GapItem))
            size -= sep;

        QPoint p;
        rpick(o, p) = pos;
        rperp(o, p) = perp(o, info->rect.topLeft());
        QSize s;
        rpick(o, s) = size;
        rperp(o, s) = perp(o, info->rect.size());

        result = QRect(p, s);
    }

    return result;
}

void QDockAreaLayout::keepSize(QDockWidget *w)
{
    QList<int> path = indexOf(w);
    if (path.isEmpty())
        return;
    QDockAreaLayoutItem &item = this->item(path);
    if (item.size != -1)
        item.flags |= QDockAreaLayoutItem::KeepSize;
}

void QDockAreaLayout::styleChangedEvent()
{
    sep = mainWindow->style()->pixelMetric(QStyle::PM_DockWidgetSeparatorExtent, 0, mainWindow);
    fitLayout();
}

QT_END_NAMESPACE

#endif // QT_NO_DOCKWIDGET
