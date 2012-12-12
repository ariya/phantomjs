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

#include "qboxlayout.h"
#include "qapplication.h"
#include "qwidget.h"
#include "qlist.h"
#include "qsizepolicy.h"
#include "qvector.h"

#include "qlayoutengine_p.h"
#include "qlayout_p.h"

QT_BEGIN_NAMESPACE

/*
    Returns true if the \a widget can be added to the \a layout;
    otherwise returns false.
*/
static bool checkWidget(QLayout *layout, QWidget *widget)
{
    if (!widget) {
        qWarning("QLayout: Cannot add null widget to %s/%s", layout->metaObject()->className(),
                  layout->objectName().toLocal8Bit().data());
        return false;
    }
    return true;
}

struct QBoxLayoutItem
{
    QBoxLayoutItem(QLayoutItem *it, int stretch_ = 0)
        : item(it), stretch(stretch_), magic(false) { }
    ~QBoxLayoutItem() { delete item; }

    int hfw(int w) {
        if (item->hasHeightForWidth()) {
            return item->heightForWidth(w);
        } else {
            return item->sizeHint().height();
        }
    }
    int mhfw(int w) {
        if (item->hasHeightForWidth()) {
            return item->heightForWidth(w);
        } else {
            return item->minimumSize().height();
        }
    }
    int hStretch() {
        if (stretch == 0 && item->widget()) {
            return item->widget()->sizePolicy().horizontalStretch();
        } else {
            return stretch;
        }
    }
    int vStretch() {
        if (stretch == 0 && item->widget()) {
            return item->widget()->sizePolicy().verticalStretch();
        } else {
            return stretch;
        }
    }

    QLayoutItem *item;
    int stretch;
    bool magic;
};

class QBoxLayoutPrivate : public QLayoutPrivate
{
    Q_DECLARE_PUBLIC(QBoxLayout)
public:
    QBoxLayoutPrivate() : hfwWidth(-1), dirty(true), spacing(-1) { }
    ~QBoxLayoutPrivate();

    void setDirty() {
        geomArray.clear();
        hfwWidth = -1;
        hfwHeight = -1;
        dirty = true;
    }

    QList<QBoxLayoutItem *> list;
    QVector<QLayoutStruct> geomArray;
    int hfwWidth;
    int hfwHeight;
    int hfwMinHeight;
    QSize sizeHint;
    QSize minSize;
    QSize maxSize;
    int leftMargin, topMargin, rightMargin, bottomMargin;
    Qt::Orientations expanding;
    uint hasHfw : 1;
    uint dirty : 1;
    QBoxLayout::Direction dir;
    int spacing;

    inline void deleteAll() { while (!list.isEmpty()) delete list.takeFirst(); }

    void setupGeom();
    void calcHfw(int);

    void effectiveMargins(int *left, int *top, int *right, int *bottom) const;
};

QBoxLayoutPrivate::~QBoxLayoutPrivate()
{
}

static inline bool horz(QBoxLayout::Direction dir)
{
    return dir == QBoxLayout::RightToLeft || dir == QBoxLayout::LeftToRight;
}

/**
 * The purpose of this function is to make sure that widgets are not laid out outside its layout.
 * E.g. the layoutItemRect margins are only meant to take of the surrounding margins/spacings.
 * However, if the margin is 0, it can easily cover the area of a widget above it.
 */
void QBoxLayoutPrivate::effectiveMargins(int *left, int *top, int *right, int *bottom) const
{
    int l = leftMargin;
    int t = topMargin;
    int r = rightMargin;
    int b = bottomMargin;
#ifdef Q_WS_MAC
    Q_Q(const QBoxLayout);
    if (horz(dir)) {
        QBoxLayoutItem *leftBox = 0;
        QBoxLayoutItem *rightBox = 0;

        if (left || right) {
            leftBox = list.value(0);
            rightBox = list.value(list.count() - 1);
            if (dir == QBoxLayout::RightToLeft)
                qSwap(leftBox, rightBox);

            int leftDelta = 0;
            int rightDelta = 0;
            if (leftBox) {
                QLayoutItem *itm = leftBox->item;
                if (QWidget *w = itm->widget())
                    leftDelta = itm->geometry().left() - w->geometry().left();
            }
            if (rightBox) {
                QLayoutItem *itm = rightBox->item;
                if (QWidget *w = itm->widget())
                    rightDelta = w->geometry().right() - itm->geometry().right();
            }
            QWidget *w = q->parentWidget();
            Qt::LayoutDirection layoutDirection = w ? w->layoutDirection() : QApplication::layoutDirection();
            if (layoutDirection == Qt::RightToLeft)
                qSwap(leftDelta, rightDelta);

            l = qMax(l, leftDelta);
            r = qMax(r, rightDelta);
        }

        int count = top || bottom ? list.count() : 0;
        for (int i = 0; i < count; ++i) {
            QBoxLayoutItem *box = list.at(i);
            QLayoutItem *itm = box->item;
            QWidget *w = itm->widget();
            if (w) {
                QRect lir = itm->geometry();
                QRect wr = w->geometry();
                if (top)
                    t = qMax(t, lir.top() - wr.top());
                if (bottom)
                    b = qMax(b, wr.bottom() - lir.bottom());
            }
        }
    } else {    // vertical layout
        QBoxLayoutItem *topBox = 0;
        QBoxLayoutItem *bottomBox = 0;

        if (top || bottom) {
            topBox = list.value(0);
            bottomBox = list.value(list.count() - 1);
            if (dir == QBoxLayout::BottomToTop) {
                qSwap(topBox, bottomBox);
            }

            if (top && topBox) {
                QLayoutItem *itm = topBox->item;
                QWidget *w = itm->widget();
                if (w)
                    t = qMax(t, itm->geometry().top() - w->geometry().top());
            }

            if (bottom && bottomBox) {
                QLayoutItem *itm = bottomBox->item;
                QWidget *w = itm->widget();
                if (w)
                    b = qMax(b, w->geometry().bottom() - itm->geometry().bottom());
            }
        }

        int count = left || right ? list.count() : 0;
        for (int i = 0; i < count; ++i) {
            QBoxLayoutItem *box = list.at(i);
            QLayoutItem *itm = box->item;
            QWidget *w = itm->widget();
            if (w) {
                QRect lir = itm->geometry();
                QRect wr = w->geometry();
                if (left)
                    l = qMax(l, lir.left() - wr.left());
                if (right)
                    r = qMax(r, wr.right() - lir.right());
            }
        }        
    }
#endif
    if (left)
        *left = l;
    if (top)
        *top = t;
    if (right)
        *right = r;
    if (bottom)
        *bottom = b;
}


/*
    Initializes the data structure needed by qGeomCalc and
    recalculates max/min and size hint.
*/
void QBoxLayoutPrivate::setupGeom()
{
    if (!dirty)
        return;

    Q_Q(QBoxLayout);
    int maxw = horz(dir) ? 0 : QLAYOUTSIZE_MAX;
    int maxh = horz(dir) ? QLAYOUTSIZE_MAX : 0;
    int minw = 0;
    int minh = 0;
    int hintw = 0;
    int hinth = 0;

    bool horexp = false;
    bool verexp = false;

    hasHfw = false;

    int n = list.count();
    geomArray.clear();
    QVector<QLayoutStruct> a(n);

    QSizePolicy::ControlTypes controlTypes1;
    QSizePolicy::ControlTypes controlTypes2;
    int fixedSpacing = q->spacing();
    int previousNonEmptyIndex = -1;

    QStyle *style = 0;
    if (fixedSpacing < 0) {
        if (QWidget *parentWidget = q->parentWidget())
            style = parentWidget->style();
    }

    for (int i = 0; i < n; i++) {
        QBoxLayoutItem *box = list.at(i);
        QSize max = box->item->maximumSize();
        QSize min = box->item->minimumSize();
        QSize hint = box->item->sizeHint();
        Qt::Orientations exp = box->item->expandingDirections();
        bool empty = box->item->isEmpty();
        int spacing = 0;

        if (!empty) {
            if (fixedSpacing >= 0) {
                spacing = (previousNonEmptyIndex >= 0) ? fixedSpacing : 0;
#ifdef Q_WS_MAC
                if (!horz(dir) && previousNonEmptyIndex >= 0) {
                    QBoxLayoutItem *sibling = (dir == QBoxLayout::TopToBottom  ? box : list.at(previousNonEmptyIndex));
                    if (sibling) {
                        QWidget *wid = sibling->item->widget();
                        if (wid)
                            spacing = qMax(spacing, sibling->item->geometry().top() - wid->geometry().top());
                    }
                }
#endif
            } else {
                controlTypes1 = controlTypes2;
                controlTypes2 = box->item->controlTypes();
                if (previousNonEmptyIndex >= 0) {
                    QSizePolicy::ControlTypes actual1 = controlTypes1;
                    QSizePolicy::ControlTypes actual2 = controlTypes2;
                    if (dir == QBoxLayout::RightToLeft || dir == QBoxLayout::BottomToTop)
                        qSwap(actual1, actual2);

                    if (style) {
                        spacing = style->combinedLayoutSpacing(actual1, actual2,
                                             horz(dir) ? Qt::Horizontal : Qt::Vertical,
                                             0, q->parentWidget());
                        if (spacing < 0)
                            spacing = 0;
                    }
                }
            }

            if (previousNonEmptyIndex >= 0)
                a[previousNonEmptyIndex].spacing = spacing;
            previousNonEmptyIndex = i;
        }

        bool ignore = empty && box->item->widget(); // ignore hidden widgets
        bool dummy = true;
        if (horz(dir)) {
            bool expand = (exp & Qt::Horizontal || box->stretch > 0);
            horexp = horexp || expand;
            maxw += spacing + max.width();
            minw += spacing + min.width();
            hintw += spacing + hint.width();
            if (!ignore)
                qMaxExpCalc(maxh, verexp, dummy,
                            max.height(), exp & Qt::Vertical, box->item->isEmpty());
            minh = qMax(minh, min.height());
            hinth = qMax(hinth, hint.height());

            a[i].sizeHint = hint.width();
            a[i].maximumSize = max.width();
            a[i].minimumSize = min.width();
            a[i].expansive = expand;
            a[i].stretch = box->stretch ? box->stretch : box->hStretch();
        } else {
            bool expand = (exp & Qt::Vertical || box->stretch > 0);
            verexp = verexp || expand;
            maxh += spacing + max.height();
            minh += spacing + min.height();
            hinth += spacing + hint.height();
            if (!ignore)
                qMaxExpCalc(maxw, horexp, dummy,
                            max.width(), exp & Qt::Horizontal, box->item->isEmpty());
            minw = qMax(minw, min.width());
            hintw = qMax(hintw, hint.width());

            a[i].sizeHint = hint.height();
            a[i].maximumSize = max.height();
            a[i].minimumSize = min.height();
            a[i].expansive = expand;
            a[i].stretch = box->stretch ? box->stretch : box->vStretch();
        }

        a[i].empty = empty;
        a[i].spacing = 0;   // might be be initialized with a non-zero value in a later iteration
        hasHfw = hasHfw || box->item->hasHeightForWidth();
    }

    geomArray = a;

    expanding = (Qt::Orientations)
                       ((horexp ? Qt::Horizontal : 0)
                         | (verexp ? Qt::Vertical : 0));

    minSize = QSize(minw, minh);
    maxSize = QSize(maxw, maxh).expandedTo(minSize);
    sizeHint = QSize(hintw, hinth).expandedTo(minSize).boundedTo(maxSize);

    q->getContentsMargins(&leftMargin, &topMargin, &rightMargin, &bottomMargin);
    int left, top, right, bottom;
    effectiveMargins(&left, &top, &right, &bottom);
    QSize extra(left + right, top + bottom);

    minSize += extra;
    maxSize += extra;
    sizeHint += extra;

    dirty = false;
}

/*
  Calculates and stores the preferred height given the width \a w.
*/
void QBoxLayoutPrivate::calcHfw(int w)
{
    QVector<QLayoutStruct> &a = geomArray;
    int n = a.count();
    int h = 0;
    int mh = 0;

    Q_ASSERT(n == list.size());

    if (horz(dir)) {
        qGeomCalc(a, 0, n, 0, w);
        for (int i = 0; i < n; i++) {
            QBoxLayoutItem *box = list.at(i);
            h = qMax(h, box->hfw(a.at(i).size));
            mh = qMax(mh, box->mhfw(a.at(i).size));
        }
    } else {
        for (int i = 0; i < n; ++i) {
            QBoxLayoutItem *box = list.at(i);
            int spacing = a.at(i).spacing;
            h += box->hfw(w);
            mh += box->mhfw(w);
            h += spacing;
            mh += spacing;
        }
    }
    hfwWidth = w;
    hfwHeight = h;
    hfwMinHeight = mh;
}


/*!
    \class QBoxLayout

    \brief The QBoxLayout class lines up child widgets horizontally or
    vertically.

    \ingroup geomanagement

    QBoxLayout takes the space it gets (from its parent layout or from
    the parentWidget()), divides it up into a row of boxes, and makes
    each managed widget fill one box.

    \image qhboxlayout-with-5-children.png Horizontal box layout with five child widgets

    If the QBoxLayout's orientation is Qt::Horizontal the boxes are
    placed in a row, with suitable sizes. Each widget (or other box)
    will get at least its minimum size and at most its maximum size.
    Any excess space is shared according to the stretch factors (more
    about that below).

    \image qvboxlayout-with-5-children.png Vertical box layout with five child widgets

    If the QBoxLayout's orientation is Qt::Vertical, the boxes are
    placed in a column, again with suitable sizes.

    The easiest way to create a QBoxLayout is to use one of the
    convenience classes, e.g. QHBoxLayout (for Qt::Horizontal boxes)
    or QVBoxLayout (for Qt::Vertical boxes). You can also use the
    QBoxLayout constructor directly, specifying its direction as
    LeftToRight, RightToLeft, TopToBottom, or BottomToTop.

    If the QBoxLayout is not the top-level layout (i.e. it is not
    managing all of the widget's area and children), you must add it
    to its parent layout before you can do anything with it. The
    normal way to add a layout is by calling
    parentLayout-\>addLayout().

    Once you have done this, you can add boxes to the QBoxLayout using
    one of four functions:

    \list
    \o addWidget() to add a widget to the QBoxLayout and set the
    widget's stretch factor. (The stretch factor is along the row of
    boxes.)

    \o addSpacing() to create an empty box; this is one of the
    functions you use to create nice and spacious dialogs. See below
    for ways to set margins.

    \o addStretch() to create an empty, stretchable box.

    \o addLayout() to add a box containing another QLayout to the row
    and set that layout's stretch factor.
    \endlist

    Use insertWidget(), insertSpacing(), insertStretch() or
    insertLayout() to insert a box at a specified position in the
    layout.

    QBoxLayout also includes two margin widths:

    \list
    \o setContentsMargins() sets the width of the outer border on 
       each side of the widget. This is the width of the reserved space 
       along each of the QBoxLayout's four sides.
    \o setSpacing() sets the width between neighboring boxes. (You
       can use addSpacing() to get more space at a particular spot.)
    \endlist

    The margin default is provided by the style. The default margin
    most Qt styles specify is 9 for child widgets and 11 for windows.
    The spacing defaults to the same as the margin width for a
    top-level layout, or to the same as the parent layout.

    To remove a widget from a layout, call removeWidget(). Calling
    QWidget::hide() on a widget also effectively removes the widget
    from the layout until QWidget::show() is called.

    You will almost always want to use QVBoxLayout and QHBoxLayout
    rather than QBoxLayout because of their convenient constructors.

    \sa QGridLayout, QStackedLayout, {Layout Management}
*/

/*!
    \enum QBoxLayout::Direction

    This type is used to determine the direction of a box layout.

    \value LeftToRight  Horizontal from left to right.
    \value RightToLeft  Horizontal from right to left.
    \value TopToBottom  Vertical from top to bottom.
    \value BottomToTop  Vertical from bottom to top.

    \omitvalue Down
    \omitvalue Up
*/

/*!
    Constructs a new QBoxLayout with direction \a dir and parent widget \a
    parent.

    \sa direction()
*/
QBoxLayout::QBoxLayout(Direction dir, QWidget *parent)
    : QLayout(*new QBoxLayoutPrivate, 0, parent)
{
    Q_D(QBoxLayout);
    d->dir = dir;
}

#ifdef QT3_SUPPORT
/*!
    Constructs a new QBoxLayout with direction \a dir and main widget \a
    parent. \a parent may not be 0.

    The \a margin is the number of pixels between the edge of the
    widget and its managed children. The \a spacing is the default
    number of pixels between neighboring children. If \a spacing is -1
    the value of \a margin is used for \a spacing.

    \a name is the internal object name.

    \sa direction()
*/
QBoxLayout::QBoxLayout(QWidget *parent, Direction dir,
                        int margin, int spacing, const char *name)
    : QLayout(*new QBoxLayoutPrivate, 0, parent)
{
    Q_D(QBoxLayout);
    d->dir = dir;
    setMargin(margin);
    setObjectName(QString::fromAscii(name));
    setSpacing(spacing<0 ? margin : spacing);
}

/*!
    Constructs a new QBoxLayout called \a name, with direction \a dir,
    and inserts it into \a parentLayout.

    The \a spacing is the default number of pixels between neighboring
    children. If \a spacing is -1, the layout will inherit its
    parent's spacing().
*/
QBoxLayout::QBoxLayout(QLayout *parentLayout, Direction dir, int spacing,
                        const char *name)
    : QLayout(*new QBoxLayoutPrivate, parentLayout, 0)
{
    Q_D(QBoxLayout);
    d->dir = dir;
    setObjectName(QString::fromAscii(name));
    setSpacing(spacing);
}

/*!
    Constructs a new QBoxLayout called \a name, with direction \a dir.

    If \a spacing is -1, the layout will inherit its parent's
    spacing(); otherwise \a spacing is used.

    You must insert this box into another layout.
*/
QBoxLayout::QBoxLayout(Direction dir, int spacing, const char *name)
    : QLayout(*new QBoxLayoutPrivate,0, 0)
{
    Q_D(QBoxLayout);
    d->dir = dir;
    setObjectName(QString::fromAscii(name));
    setSpacing(spacing);
}
#endif // QT3_SUPPORT


/*!
    Destroys this box layout.

    The layout's widgets aren't destroyed.
*/
QBoxLayout::~QBoxLayout()
{
    Q_D(QBoxLayout);
    d->deleteAll(); // must do it before QObject deletes children, so can't be in ~QBoxLayoutPrivate
}

/*!
  Reimplements QLayout::spacing(). If the spacing property is
  valid, that value is returned. Otherwise, a value for the spacing
  property is computed and returned. Since layout spacing in a widget
  is style dependent, if the parent is a widget, it queries the style
  for the (horizontal or vertical) spacing of the layout. Otherwise,
  the parent is a layout, and it queries the parent layout for the
  spacing().

  \sa QLayout::spacing(), setSpacing()
 */
int QBoxLayout::spacing() const
{
    Q_D(const QBoxLayout);
    if (d->spacing >=0) {
        return d->spacing;
    } else {
        return qSmartSpacing(this, d->dir == LeftToRight || d->dir == RightToLeft
                                           ? QStyle::PM_LayoutHorizontalSpacing
                                           : QStyle::PM_LayoutVerticalSpacing);
    }
}

/*!
  Reimplements QLayout::setSpacing(). Sets the spacing
  property to \a spacing. 

  \sa QLayout::setSpacing(), spacing()
 */
void QBoxLayout::setSpacing(int spacing)
{
    Q_D(QBoxLayout);
    d->spacing = spacing;
    invalidate();
}

/*!
    \reimp
*/
QSize QBoxLayout::sizeHint() const
{
    Q_D(const QBoxLayout);
    if (d->dirty)
        const_cast<QBoxLayout*>(this)->d_func()->setupGeom();
    return d->sizeHint;
}

/*!
    \reimp
*/
QSize QBoxLayout::minimumSize() const
{
    Q_D(const QBoxLayout);
    if (d->dirty)
        const_cast<QBoxLayout*>(this)->d_func()->setupGeom();
    return d->minSize;
}

/*!
    \reimp
*/
QSize QBoxLayout::maximumSize() const
{
    Q_D(const QBoxLayout);
    if (d->dirty)
        const_cast<QBoxLayout*>(this)->d_func()->setupGeom();

    QSize s = d->maxSize.boundedTo(QSize(QLAYOUTSIZE_MAX, QLAYOUTSIZE_MAX));

    if (alignment() & Qt::AlignHorizontal_Mask)
        s.setWidth(QLAYOUTSIZE_MAX);
    if (alignment() & Qt::AlignVertical_Mask)
        s.setHeight(QLAYOUTSIZE_MAX);
    return s;
}

/*!
    \reimp
*/
bool QBoxLayout::hasHeightForWidth() const
{
    Q_D(const QBoxLayout);
    if (d->dirty)
        const_cast<QBoxLayout*>(this)->d_func()->setupGeom();
    return d->hasHfw;
}

/*!
    \reimp
*/
int QBoxLayout::heightForWidth(int w) const
{
    Q_D(const QBoxLayout);
    if (!hasHeightForWidth())
        return -1;

    int left, top, right, bottom;
    d->effectiveMargins(&left, &top, &right, &bottom);

    w -= left + right;
    if (w != d->hfwWidth)
        const_cast<QBoxLayout*>(this)->d_func()->calcHfw(w);

    return d->hfwHeight + top + bottom;
}

/*!
    \reimp
*/
int QBoxLayout::minimumHeightForWidth(int w) const
{
    Q_D(const QBoxLayout);
    (void) heightForWidth(w);
    int top, bottom;
    d->effectiveMargins(0, &top, 0, &bottom);
    return d->hasHfw ? (d->hfwMinHeight + top + bottom) : -1;
}

/*!
    Resets cached information.
*/
void QBoxLayout::invalidate()
{
    Q_D(QBoxLayout);
    d->setDirty();
    QLayout::invalidate();
}

/*!
    \reimp
*/
int QBoxLayout::count() const
{
    Q_D(const QBoxLayout);
    return d->list.count();
}

/*!
    \reimp
*/
QLayoutItem *QBoxLayout::itemAt(int index) const
{
    Q_D(const QBoxLayout);
    return index >= 0 && index < d->list.count() ? d->list.at(index)->item : 0;
}

/*!
    \reimp
*/
QLayoutItem *QBoxLayout::takeAt(int index)
{
    Q_D(QBoxLayout);
    if (index < 0 || index >= d->list.count())
        return 0;
    QBoxLayoutItem *b = d->list.takeAt(index);
    QLayoutItem *item = b->item;
    b->item = 0;
    delete b;

    invalidate();
    return item;
}


/*!
    \reimp
*/
Qt::Orientations QBoxLayout::expandingDirections() const
{
    Q_D(const QBoxLayout);
    if (d->dirty)
        const_cast<QBoxLayout*>(this)->d_func()->setupGeom();
    return d->expanding;
}

/*!
    \reimp
*/
void QBoxLayout::setGeometry(const QRect &r)
{
    Q_D(QBoxLayout);
    if (d->dirty || r != geometry()) {
        QRect oldRect = geometry();
        QLayout::setGeometry(r);
        if (d->dirty)
            d->setupGeom();
        QRect cr = alignment() ? alignmentRect(r) : r;

        int left, top, right, bottom;
        d->effectiveMargins(&left, &top, &right, &bottom);
        QRect s(cr.x() + left, cr.y() + top,
                cr.width() - (left + right),
                cr.height() - (top + bottom));

        QVector<QLayoutStruct> a = d->geomArray;
        int pos = horz(d->dir) ? s.x() : s.y();
        int space = horz(d->dir) ? s.width() : s.height();
        int n = a.count();
        if (d->hasHfw && !horz(d->dir)) {
            for (int i = 0; i < n; i++) {
                QBoxLayoutItem *box = d->list.at(i);
                if (box->item->hasHeightForWidth()) {
                    int width = qBound(box->item->minimumSize().width(), s.width(), box->item->maximumSize().width());
                    a[i].sizeHint = a[i].minimumSize =
                                    box->item->heightForWidth(width);
                }
            }
        }

        Direction visualDir = d->dir;
        QWidget *parent = parentWidget();
        if (parent && parent->isRightToLeft()) {
            if (d->dir == LeftToRight)
                visualDir = RightToLeft;
            else if (d->dir == RightToLeft)
                visualDir = LeftToRight;
        }

        qGeomCalc(a, 0, n, pos, space);

        bool reverse = (horz(visualDir)
                        ? ((r.right() > oldRect.right()) != (visualDir == RightToLeft))
                        : r.bottom() > oldRect.bottom());
        for (int j = 0; j < n; j++) {
            int i = reverse ? n-j-1 : j;
            QBoxLayoutItem *box = d->list.at(i);

            switch (visualDir) {
            case LeftToRight:
                box->item->setGeometry(QRect(a.at(i).pos, s.y(), a.at(i).size, s.height()));
                break;
            case RightToLeft:
                box->item->setGeometry(QRect(s.left() + s.right() - a.at(i).pos - a.at(i).size + 1,
                                             s.y(), a.at(i).size, s.height()));
                break;
            case TopToBottom:
                box->item->setGeometry(QRect(s.x(), a.at(i).pos, s.width(), a.at(i).size));
                break;
            case BottomToTop:
                box->item->setGeometry(QRect(s.x(),
                                             s.top() + s.bottom() - a.at(i).pos - a.at(i).size + 1,
                                             s.width(), a.at(i).size));
            }
        }
    }
}

/*!
    \reimp
*/
void QBoxLayout::addItem(QLayoutItem *item)
{
    Q_D(QBoxLayout);
    QBoxLayoutItem *it = new QBoxLayoutItem(item);
    d->list.append(it);
    invalidate();
}

/*!
    Inserts \a item into this box layout at position \a index. If \a
    index is negative, the item is added at the end.

    \sa addItem(), insertWidget(), insertLayout(), insertStretch(),
        insertSpacing()
*/
void QBoxLayout::insertItem(int index, QLayoutItem *item)
{
    Q_D(QBoxLayout);
    if (index < 0)                                // append
        index = d->list.count();

    QBoxLayoutItem *it = new QBoxLayoutItem(item);
    d->list.insert(index, it);
    invalidate();
}

/*!
    Inserts a non-stretchable space (a QSpacerItem) at position \a index, with
    size \a size. If \a index is negative the space is added at the end.

    The box layout has default margin and spacing. This function adds
    additional space.

    \sa addSpacing(), insertItem(), QSpacerItem
*/
void QBoxLayout::insertSpacing(int index, int size)
{
    Q_D(QBoxLayout);
    if (index < 0)                                // append
        index = d->list.count();

    QLayoutItem *b;
    if (horz(d->dir))
        b = QLayoutPrivate::createSpacerItem(this, size, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
    else
        b = QLayoutPrivate::createSpacerItem(this, 0, size, QSizePolicy::Minimum, QSizePolicy::Fixed);

    QT_TRY {
        QBoxLayoutItem *it = new QBoxLayoutItem(b);
        it->magic = true;
        d->list.insert(index, it);

    } QT_CATCH(...) {
        delete b;
        QT_RETHROW;
    }
    invalidate();
}

/*!
    Inserts a stretchable space (a QSpacerItem) at position \a
    index, with zero minimum size and stretch factor \a stretch. If \a
    index is negative the space is added at the end.

    \sa addStretch(), insertItem(), QSpacerItem
*/
void QBoxLayout::insertStretch(int index, int stretch)
{
    Q_D(QBoxLayout);
    if (index < 0)                                // append
        index = d->list.count();

    QLayoutItem *b;
    if (horz(d->dir))
        b = QLayoutPrivate::createSpacerItem(this, 0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    else
        b = QLayoutPrivate::createSpacerItem(this, 0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

    QBoxLayoutItem *it = new QBoxLayoutItem(b, stretch);
    it->magic = true;
    d->list.insert(index, it);
    invalidate();
}

/*!
    \since 4.4

    Inserts \a spacerItem at position \a index, with zero minimum
    size and stretch factor. If \a index is negative the
    space is added at the end.

    \sa addSpacerItem(), insertStretch(), insertSpacing()
*/
void QBoxLayout::insertSpacerItem(int index, QSpacerItem *spacerItem)
{
    Q_D(QBoxLayout);
    if (index < 0)                                // append
        index = d->list.count();

    QBoxLayoutItem *it = new QBoxLayoutItem(spacerItem);
    it->magic = true;
    d->list.insert(index, it);
    invalidate();
}

/*!
    Inserts \a layout at position \a index, with stretch factor \a
    stretch. If \a index is negative, the layout is added at the end.

    \a layout becomes a child of the box layout.

    \sa addLayout(), insertItem()
*/
void QBoxLayout::insertLayout(int index, QLayout *layout, int stretch)
{
    Q_D(QBoxLayout);
    addChildLayout(layout);
    if (index < 0)                                // append
        index = d->list.count();
    QBoxLayoutItem *it = new QBoxLayoutItem(layout, stretch);
    d->list.insert(index, it);
    invalidate();
}

/*!
    Inserts \a widget at position \a index, with stretch factor \a
    stretch and alignment \a alignment. If \a index is negative, the
    widget is added at the end.

    The stretch factor applies only in the \l{direction()}{direction}
    of the QBoxLayout, and is relative to the other boxes and widgets
    in this QBoxLayout. Widgets and boxes with higher stretch factors
    grow more.

    If the stretch factor is 0 and nothing else in the QBoxLayout has
    a stretch factor greater than zero, the space is distributed
    according to the QWidget:sizePolicy() of each widget that's
    involved.

    The alignment is specified by \a alignment. The default alignment
    is 0, which means that the widget fills the entire cell.

    \sa addWidget(), insertItem()
*/
void QBoxLayout::insertWidget(int index, QWidget *widget, int stretch,
                              Qt::Alignment alignment)
{
    Q_D(QBoxLayout);
    if (!checkWidget(this, widget))
         return;
    addChildWidget(widget);
    if (index < 0)                                // append
        index = d->list.count();
    QWidgetItem *b = QLayoutPrivate::createWidgetItem(this, widget);
    b->setAlignment(alignment);

    QBoxLayoutItem *it;
    QT_TRY{
        it = new QBoxLayoutItem(b, stretch);
    } QT_CATCH(...) {
        delete b;
        QT_RETHROW;
    }

    QT_TRY{
        d->list.insert(index, it);
    } QT_CATCH(...) {
        delete it;
        QT_RETHROW;
    }
    invalidate();
}

/*!
    Adds a non-stretchable space (a QSpacerItem) with size \a size
    to the end of this box layout. QBoxLayout provides default margin
    and spacing. This function adds additional space.

    \sa insertSpacing(), addItem(), QSpacerItem
*/
void QBoxLayout::addSpacing(int size)
{
    insertSpacing(-1, size);
}

/*!
    Adds a stretchable space (a QSpacerItem) with zero minimum
    size and stretch factor \a stretch to the end of this box layout.

    \sa insertStretch(), addItem(), QSpacerItem
*/
void QBoxLayout::addStretch(int stretch)
{
    insertStretch(-1, stretch);
}

/*!
    \since 4.4

    Adds \a spacerItem to the end of this box layout.

    \sa addSpacing(), addStretch()
*/
void QBoxLayout::addSpacerItem(QSpacerItem *spacerItem)
{
    insertSpacerItem(-1, spacerItem);
}

/*!
    Adds \a widget to the end of this box layout, with a stretch
    factor of \a stretch and alignment \a alignment.

    The stretch factor applies only in the \l{direction()}{direction}
    of the QBoxLayout, and is relative to the other boxes and widgets
    in this QBoxLayout. Widgets and boxes with higher stretch factors
    grow more.

    If the stretch factor is 0 and nothing else in the QBoxLayout has
    a stretch factor greater than zero, the space is distributed
    according to the QWidget:sizePolicy() of each widget that's
    involved.

    The alignment is specified by \a alignment. The default
    alignment is 0, which means that the widget fills the entire cell.

    \sa insertWidget(), addItem(), addLayout(), addStretch(),
        addSpacing(), addStrut()
*/
void QBoxLayout::addWidget(QWidget *widget, int stretch, Qt::Alignment alignment)
{
    insertWidget(-1, widget, stretch, alignment);
}

/*!
    Adds \a layout to the end of the box, with serial stretch factor
    \a stretch.

    \sa insertLayout(), addItem(), addWidget()
*/
void QBoxLayout::addLayout(QLayout *layout, int stretch)
{
    insertLayout(-1, layout, stretch);
}

/*!
    Limits the perpendicular dimension of the box (e.g. height if the
    box is \l LeftToRight) to a minimum of \a size. Other constraints
    may increase the limit.

    \sa addItem()
*/
void QBoxLayout::addStrut(int size)
{
    Q_D(QBoxLayout);
    QLayoutItem *b;
    if (horz(d->dir))
        b = QLayoutPrivate::createSpacerItem(this, 0, size, QSizePolicy::Fixed, QSizePolicy::Minimum);
    else
        b = QLayoutPrivate::createSpacerItem(this, size, 0, QSizePolicy::Minimum, QSizePolicy::Fixed);

    QBoxLayoutItem *it = new QBoxLayoutItem(b);
    it->magic = true;
    d->list.append(it);
    invalidate();
}

/*!
    \fn int QBoxLayout::findWidget(QWidget *widget)

    Use indexOf(\a widget) instead.
*/

/*!
    Sets the stretch factor for \a widget to \a stretch and returns
    true if \a widget is found in this layout (not including child
    layouts); otherwise returns false.

    \sa setAlignment()
*/
bool QBoxLayout::setStretchFactor(QWidget *widget, int stretch)
{
    Q_D(QBoxLayout);
    if (!widget)
        return false;
    for (int i = 0; i < d->list.size(); ++i) {
        QBoxLayoutItem *box = d->list.at(i);
        if (box->item->widget() == widget) {
            box->stretch = stretch;
            invalidate();
            return true;
        }
    }
    return false;
}

/*!
    \overload

    Sets the stretch factor for the layout \a layout to \a stretch and
    returns true if \a layout is found in this layout (not including
    child layouts); otherwise returns false.
*/
bool QBoxLayout::setStretchFactor(QLayout *layout, int stretch)
{
    Q_D(QBoxLayout);
    for (int i = 0; i < d->list.size(); ++i) {
        QBoxLayoutItem *box = d->list.at(i);
        if (box->item->layout() == layout) {
            if (box->stretch != stretch) {
                box->stretch = stretch;
                invalidate();
            }
            return true;
        }
    }
    return false;
}

/*!
    Sets the stretch factor at position \a index. to \a stretch.

    \since 4.5
*/

void QBoxLayout::setStretch(int index, int stretch)
{
    Q_D(QBoxLayout);
    if (index >= 0 && index < d->list.size()) {
        QBoxLayoutItem *box = d->list.at(index);
        if (box->stretch != stretch) {
            box->stretch = stretch;
            invalidate();
        }
    }
}

/*!
    Returns the stretch factor at position \a index.

    \since 4.5
*/

int QBoxLayout::stretch(int index) const
{
    Q_D(const QBoxLayout);
    if (index >= 0 && index < d->list.size())
        return d->list.at(index)->stretch;
    return -1;
}

/*!
    Sets the direction of this layout to \a direction.
*/
void QBoxLayout::setDirection(Direction direction)
{
    Q_D(QBoxLayout);
    if (d->dir == direction)
        return;
    if (horz(d->dir) != horz(direction)) {
        //swap around the spacers (the "magic" bits)
        //#### a bit yucky, knows too much.
        //#### probably best to add access functions to spacerItem
        //#### or even a QSpacerItem::flip()
        for (int i = 0; i < d->list.size(); ++i) {
            QBoxLayoutItem *box = d->list.at(i);
            if (box->magic) {
                QSpacerItem *sp = box->item->spacerItem();
                if (sp) {
                    if (sp->expandingDirections() == Qt::Orientations(0) /*No Direction*/) {
                        //spacing or strut
                        QSize s = sp->sizeHint();
                        sp->changeSize(s.height(), s.width(),
                            horz(direction) ? QSizePolicy::Fixed:QSizePolicy::Minimum,
                            horz(direction) ? QSizePolicy::Minimum:QSizePolicy::Fixed);

                    } else {
                        //stretch
                        if (horz(direction))
                            sp->changeSize(0, 0, QSizePolicy::Expanding,
                                            QSizePolicy::Minimum);
                        else
                            sp->changeSize(0, 0, QSizePolicy::Minimum,
                                            QSizePolicy::Expanding);
                    }
                }
            }
        }
    }
    d->dir = direction;
    invalidate();
}

/*!
    \fn QBoxLayout::Direction QBoxLayout::direction() const

    Returns the direction of the box. addWidget() and addSpacing()
    work in this direction; the stretch stretches in this direction.

    \sa QBoxLayout::Direction addWidget() addSpacing()
*/

QBoxLayout::Direction QBoxLayout::direction() const
{
    Q_D(const QBoxLayout);
    return d->dir;
}

/*!
    \class QHBoxLayout
    \brief The QHBoxLayout class lines up widgets horizontally.

    \ingroup geomanagement

    This class is used to construct horizontal box layout objects. See
    QBoxLayout for details.

    The simplest use of the class is like this:

    \snippet doc/src/snippets/layouts/layouts.cpp 0
    \snippet doc/src/snippets/layouts/layouts.cpp 1
    \snippet doc/src/snippets/layouts/layouts.cpp 2
    \codeline
    \snippet doc/src/snippets/layouts/layouts.cpp 3
    \snippet doc/src/snippets/layouts/layouts.cpp 4
    \snippet doc/src/snippets/layouts/layouts.cpp 5

    First, we create the widgets we want in the layout. Then, we
    create the QHBoxLayout object and add the widgets into the
    layout. Finally, we call QWidget::setLayout() to install the
    QHBoxLayout object onto the widget. At that point, the widgets in
    the layout are reparented to have \c window as their parent.

    \image qhboxlayout-with-5-children.png Horizontal box layout with five child widgets

    \sa QVBoxLayout, QGridLayout, QStackedLayout, {Layout Management}, {Basic Layouts Example}
*/


/*!
    Constructs a new top-level horizontal box with
    parent \a parent.
*/
QHBoxLayout::QHBoxLayout(QWidget *parent)
    : QBoxLayout(LeftToRight, parent)
{
}

/*!
    Constructs a new horizontal box. You must add
    it to another layout.
*/
QHBoxLayout::QHBoxLayout()
    : QBoxLayout(LeftToRight)
{
}



#ifdef QT3_SUPPORT
/*!
    Constructs a new top-level horizontal box called \a name, with
    parent \a parent.

    The \a margin is the number of pixels between the edge of the
    widget and its managed children. The \a spacing is the default
    number of pixels between neighboring children. If \a spacing is -1
    the value of \a margin is used for \a spacing.
*/
QHBoxLayout::QHBoxLayout(QWidget *parent, int margin,
                         int spacing, const char *name)
    : QBoxLayout(LeftToRight, parent)
{
    setMargin(margin);
    setSpacing(spacing<0 ? margin : spacing);
    setObjectName(QString::fromAscii(name));
}

/*!
    Constructs a new horizontal box called name \a name and adds it to
    \a parentLayout.

    The \a spacing is the default number of pixels between neighboring
    children. If \a spacing is -1, this QHBoxLayout will inherit its
    parent's spacing().
*/
QHBoxLayout::QHBoxLayout(QLayout *parentLayout, int spacing,
                          const char *name)
    : QBoxLayout(LeftToRight)
{
    setSpacing(spacing);
    setObjectName(QString::fromAscii(name));
    if (parentLayout) {
        setParent(parentLayout);
        parentLayout->addItem(this);
    }
}

/*!
    Constructs a new horizontal box called name \a name. You must add
    it to another layout.

    The \a spacing is the default number of pixels between neighboring
    children. If \a spacing is -1, this QHBoxLayout will inherit its
    parent's spacing().
*/
QHBoxLayout::QHBoxLayout(int spacing, const char *name)
    : QBoxLayout(LeftToRight)
{
    setSpacing(spacing);
    setObjectName(QString::fromAscii(name));
}
#endif


/*!
    Destroys this box layout.

    The layout's widgets aren't destroyed.
*/
QHBoxLayout::~QHBoxLayout()
{
}

/*!
    \class QVBoxLayout
    \brief The QVBoxLayout class lines up widgets vertically.

    \ingroup geomanagement

    This class is used to construct vertical box layout objects. See
    QBoxLayout for details.

    The simplest use of the class is like this:

    \snippet doc/src/snippets/layouts/layouts.cpp 6
    \snippet doc/src/snippets/layouts/layouts.cpp 7
    \snippet doc/src/snippets/layouts/layouts.cpp 8
    \codeline
    \snippet doc/src/snippets/layouts/layouts.cpp 9
    \snippet doc/src/snippets/layouts/layouts.cpp 10
    \snippet doc/src/snippets/layouts/layouts.cpp 11

    First, we create the widgets we want in the layout. Then, we
    create the QVBoxLayout object and add the widgets into the
    layout. Finally, we call QWidget::setLayout() to install the
    QVBoxLayout object onto the widget. At that point, the widgets in
    the layout are reparented to have \c window as their parent.

    \image qvboxlayout-with-5-children.png Horizontal box layout with five child widgets

    \sa QHBoxLayout, QGridLayout, QStackedLayout, {Layout Management}, {Basic Layouts Example}
*/

/*!
    Constructs a new top-level vertical box with
    parent \a parent.
*/
QVBoxLayout::QVBoxLayout(QWidget *parent)
    : QBoxLayout(TopToBottom, parent)
{
}

/*!
    Constructs a new vertical box. You must add
    it to another layout.

*/
QVBoxLayout::QVBoxLayout()
    : QBoxLayout(TopToBottom)
{
}

#ifdef QT3_SUPPORT
/*!
    Constructs a new top-level vertical box called \a name, with
    parent \a parent.

    The \a margin is the number of pixels between the edge of the
    widget and its managed children. The \a spacing is the default
    number of pixels between neighboring children. If \a spacing is -1
    the value of \a margin is used for \a spacing.
*/
QVBoxLayout::QVBoxLayout(QWidget *parent, int margin, int spacing,
                         const char *name)
    : QBoxLayout(TopToBottom, parent)
{
    setMargin(margin);
    setSpacing(spacing<0 ? margin : spacing);
    setObjectName(QString::fromAscii(name));
}

/*!
    Constructs a new vertical box called name \a name and adds it to
    \a parentLayout.

    The \a spacing is the default number of pixels between neighboring
    children. If \a spacing is -1, this QVBoxLayout will inherit its
    parent's spacing().
*/
QVBoxLayout::QVBoxLayout(QLayout *parentLayout, int spacing,
                          const char *name)
    : QBoxLayout(TopToBottom)
{
    setSpacing(spacing);
    setObjectName(QString::fromAscii(name));
    if (parentLayout) {
        setParent(parentLayout);
        parentLayout->addItem(this);
    }
}

/*!
    Constructs a new vertical box called name \a name. You must add
    it to another layout.

    The \a spacing is the default number of pixels between neighboring
    children. If \a spacing is -1, this QVBoxLayout will inherit its
    parent's spacing().
*/
QVBoxLayout::QVBoxLayout(int spacing, const char *name)
    : QBoxLayout(TopToBottom)
{
    setSpacing(spacing);
    setObjectName(QString::fromAscii(name));
}


#endif

/*!
    Destroys this box layout.

    The layout's widgets aren't destroyed.
*/
QVBoxLayout::~QVBoxLayout()
{
}

/*!
    \fn QWidget *QLayout::mainWidget() const

    Use parentWidget() instead.
*/

/*!
    \fn void QLayout::remove(QWidget *widget)

    Use removeWidget(\a widget) instead.
*/

/*!
    \fn void QLayout::add(QWidget *widget)

    Use addWidget(\a widget) instead.
*/

/*!
    \fn QLayoutIterator QLayout::iterator()

    Use a QLayoutIterator() constructor instead.
*/

/*!
    \fn int QLayout::defaultBorder() const

    Use spacing() instead.
*/

QT_END_NAMESPACE
