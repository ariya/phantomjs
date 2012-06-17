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

#include "qlayout.h"

#include "qapplication.h"
#include "qlayoutengine_p.h"
#include "qmenubar.h"
#include "qtoolbar.h"
#include "qevent.h"
#include "qstyle.h"
#include "qvariant.h"
#include "qwidget_p.h"

QT_BEGIN_NAMESPACE

inline static QRect fromLayoutItemRect(QWidgetPrivate *priv, const QRect &rect)
{
    return rect.adjusted(priv->leftLayoutItemMargin, priv->topLayoutItemMargin,
                         -priv->rightLayoutItemMargin, -priv->bottomLayoutItemMargin);
}

inline static QSize fromLayoutItemSize(QWidgetPrivate *priv, const QSize &size)
{
    return fromLayoutItemRect(priv, QRect(QPoint(0, 0), size)).size();
}

inline static QRect toLayoutItemRect(QWidgetPrivate *priv, const QRect &rect)
{
    return rect.adjusted(-priv->leftLayoutItemMargin, -priv->topLayoutItemMargin,
                         priv->rightLayoutItemMargin, priv->bottomLayoutItemMargin);
}

inline static QSize toLayoutItemSize(QWidgetPrivate *priv, const QSize &size)
{
    return toLayoutItemRect(priv, QRect(QPoint(0, 0), size)).size();
}

/*!
   Returns a QVariant storing this QSizePolicy.
*/
QSizePolicy::operator QVariant() const
{
    return QVariant(QVariant::SizePolicy, this);
}

/*!
    \class QLayoutItem
    \brief The QLayoutItem class provides an abstract item that a
    QLayout manipulates.

    \ingroup geomanagement

    This is used by custom layouts.

    Pure virtual functions are provided to return information about
    the layout, including, sizeHint(), minimumSize(), maximumSize()
    and expanding().

    The layout's geometry can be set and retrieved with setGeometry()
    and geometry(), and its alignment with setAlignment() and
    alignment().

    isEmpty() returns whether the layout item is empty. If the
    concrete item is a QWidget, it can be retrieved using widget().
    Similarly for layout() and spacerItem().

    Some layouts have width and height interdependencies. These can
    be expressed using hasHeightForWidth(), heightForWidth(), and
    minimumHeightForWidth(). For more explanation see the \e{Qt
    Quarterly} article
    \l{http://qt.nokia.com/doc/qq/qq04-height-for-width.html}{Trading
    Height for Width}.

    \sa QLayout
*/

/*!
    \class QSpacerItem
    \ingroup geomanagement
    \brief The QSpacerItem class provides blank space in a layout.

    Normally, you don't need to use this class directly. Qt's
    built-in layout managers provide the following functions for
    manipulating empty space in layouts:

    \table
    \header \o Class
            \o Functions
    \row    \o QHBoxLayout
            \o \l{QBoxLayout::addSpacing()}{addSpacing()},
               \l{QBoxLayout::addStretch()}{addStretch()},
               \l{QBoxLayout::insertSpacing()}{insertSpacing()},
               \l{QBoxLayout::insertStretch()}{insertStretch()}
    \row    \o QGridLayout
            \o \l{QGridLayout::setRowMinimumHeight()}{setRowMinimumHeight()},
               \l{QGridLayout::setRowStretch()}{setRowStretch()},
               \l{QGridLayout::setColumnMinimumWidth()}{setColumnMinimumWidth()},
               \l{QGridLayout::setColumnStretch()}{setColumnStretch()}
    \endtable

    \sa QLayout, QWidgetItem, QLayoutItem::spacerItem()
*/

/*!
    \class QWidgetItem
    \ingroup geomanagement
    \brief The QWidgetItem class is a layout item that represents a widget.

    Normally, you don't need to use this class directly. Qt's
    built-in layout managers provide the following functions for
    manipulating widgets in layouts:

    \table
    \header \o Class
            \o Functions
    \row    \o QBoxLayout
            \o \l{QBoxLayout::addWidget()}{addWidget()},
               \l{QBoxLayout::insertWidget()}{insertWidget()},
               \l{QBoxLayout::setStretchFactor()}{setStretchFactor()}
    \row    \o QGridLayout
            \o \l{QGridLayout::addWidget()}{addWidget()}
    \row    \o QStackedLayout
            \o \l{QStackedLayout::addWidget()}{addWidget()},
               \l{QStackedLayout::insertWidget()}{insertWidget()},
               \l{QStackedLayout::currentWidget()}{currentWidget()},
               \l{QStackedLayout::setCurrentWidget()}{setCurrentWidget()},
               \l{QStackedLayout::widget()}{widget()}
    \endtable

    \sa QLayout, QSpacerItem, QLayoutItem::widget()
*/

/*!
    \fn QLayoutItem::QLayoutItem(Qt::Alignment alignment)

    Constructs a layout item with an \a alignment.
    Not all subclasses support alignment.
*/

/*!
    \fn Qt::Alignment QLayoutItem::alignment() const

    Returns the alignment of this item.
*/

/*!
    Sets the alignment of this item to \a alignment.

    \bold{Note:} Item alignment is only supported by QLayoutItem subclasses
    where it would have a visual effect. Except for QSpacerItem, which provides
    blank space for layouts, all public Qt classes that inherit QLayoutItem
    support item alignment.
*/
void QLayoutItem::setAlignment(Qt::Alignment alignment)
{
    align = alignment;
}

/*!
    \fn QSize QLayoutItem::maximumSize() const

    Implemented in subclasses to return the maximum size of this item.
*/

/*!
    \fn QSize QLayoutItem::minimumSize() const

    Implemented in subclasses to return the minimum size of this item.
*/

/*!
    \fn QSize QLayoutItem::sizeHint() const

    Implemented in subclasses to return the preferred size of this item.
*/

/*!
    \fn Qt::Orientations QLayoutItem::expandingDirections() const

    Returns whether this layout item can make use of more space than
    sizeHint(). A value of Qt::Vertical or Qt::Horizontal means that
    it wants to grow in only one dimension, whereas Qt::Vertical |
    Qt::Horizontal means that it wants to grow in both dimensions.
*/

/*!
    \fn void QLayoutItem::setGeometry(const QRect &r)

    Implemented in subclasses to set this item's geometry to \a r.

    \sa geometry()
*/

/*!
    \fn QRect QLayoutItem::geometry() const

    Returns the rectangle covered by this layout item.

    \sa setGeometry()
*/

/*!
    \fn virtual bool QLayoutItem::isEmpty() const

    Implemented in subclasses to return whether this item is empty,
    i.e. whether it contains any widgets.
*/

/*!
    \fn QSpacerItem::QSpacerItem(int w, int h, QSizePolicy::Policy hPolicy, QSizePolicy::Policy vPolicy)

    Constructs a spacer item with preferred width \a w, preferred
    height \a h, horizontal size policy \a hPolicy and vertical size
    policy \a vPolicy.

    The default values provide a gap that is able to stretch if
    nothing else wants the space.
*/

/*!
    Changes this spacer item to have preferred width \a w, preferred
    height \a h, horizontal size policy \a hPolicy and vertical size
    policy \a vPolicy.

    The default values provide a gap that is able to stretch if
    nothing else wants the space.

    Note that if changeSize() is called after the spacer item has been added
    to a layout, it is necessary to invalidate the layout in order for the
    spacer item's new size to take effect.

    \sa QSpacerItem::invalidate()
*/
void QSpacerItem::changeSize(int w, int h, QSizePolicy::Policy hPolicy,
                             QSizePolicy::Policy vPolicy)
{
    width = w;
    height = h;
    sizeP = QSizePolicy(hPolicy, vPolicy);
}

/*!
    \fn QWidgetItem::QWidgetItem(QWidget *widget)

    Creates an item containing the given \a widget.
*/

/*!
    Destroys the QLayoutItem.
*/
QLayoutItem::~QLayoutItem()
{
}

/*!
    Invalidates any cached information in this layout item.
*/
void QLayoutItem::invalidate()
{
}

/*!
    If this item is a QLayout, it is returned as a QLayout; otherwise
    0 is returned. This function provides type-safe casting.
*/
QLayout * QLayoutItem::layout()
{
    return 0;
}

/*!
    If this item is a QSpacerItem, it is returned as a QSpacerItem;
    otherwise 0 is returned. This function provides type-safe casting.
*/
QSpacerItem * QLayoutItem::spacerItem()
{
    return 0;
}

/*!
    \reimp
*/
QLayout * QLayout::layout()
{
    return this;
}

/*!
    Returns a pointer to this object.
*/
QSpacerItem * QSpacerItem::spacerItem()
{
    return this;
}

/*!
    If this item is a QWidget, it is returned as a QWidget; otherwise
    0 is returned. This function provides type-safe casting.
*/
QWidget * QLayoutItem::widget()
{
    return 0;
}

/*!
    Returns the widget managed by this item.
*/
QWidget *QWidgetItem::widget()
{
    return wid;
}

/*!
    Returns true if this layout's preferred height depends on its
    width; otherwise returns false. The default implementation returns
    false.

    Reimplement this function in layout managers that support height
    for width.

    \sa heightForWidth(), QWidget::heightForWidth()
*/
bool QLayoutItem::hasHeightForWidth() const
{
    return false;
}

/*!
    Returns the minimum height this widget needs for the given width,
    \a w. The default implementation simply returns heightForWidth(\a
    w).
*/
int QLayoutItem::minimumHeightForWidth(int w) const
{
    return heightForWidth(w);
}


/*!
    Returns the preferred height for this layout item, given the width
    \a w.

    The default implementation returns -1, indicating that the
    preferred height is independent of the width of the item. Using
    the function hasHeightForWidth() will typically be much faster
    than calling this function and testing for -1.

    Reimplement this function in layout managers that support height
    for width. A typical implementation will look like this:
    \snippet doc/src/snippets/code/src_gui_kernel_qlayoutitem.cpp 0

    Caching is strongly recommended; without it layout will take
    exponential time.

    \sa hasHeightForWidth()
*/
int QLayoutItem::heightForWidth(int /* w */) const
{
    return -1;
}

/*!
    Returns the control type(s) for the layout item. For a
    QWidgetItem, the control type comes from the widget's size
    policy; for a QLayoutItem, the control types is derived from the
    layout's contents.

    \sa QSizePolicy::controlType()
*/
QSizePolicy::ControlTypes QLayoutItem::controlTypes() const
{
    // ### Qt 5: This function should probably be virtual instead
    if (const QWidget *widget = const_cast<QLayoutItem*>(this)->widget()) {
        return widget->sizePolicy().controlType();
    } else if (const QLayout *layout = const_cast<QLayoutItem*>(this)->layout()) {
        if (layout->count() == 0)
            return QSizePolicy::DefaultType;
        QSizePolicy::ControlTypes types;
        for (int i = layout->count() - 1; i >= 0; --i)
            types |= layout->itemAt(i)->controlTypes();
        return types;
    }
    return QSizePolicy::DefaultType;
}

/*!
    \reimp
*/
void QSpacerItem::setGeometry(const QRect &r)
{
    rect = r;
}

/*!
    \reimp
*/
void QWidgetItem::setGeometry(const QRect &rect)
{
    if (isEmpty())
        return;

    QRect r = !wid->testAttribute(Qt::WA_LayoutUsesWidgetRect)
            ? fromLayoutItemRect(wid->d_func(), rect)
            : rect;
    const QSize widgetRectSurplus = r.size() - rect.size(); 

    /* 
       For historical reasons, this code is done using widget rect 
       coordinates, not layout item rect coordinates. However, 
       QWidgetItem's sizeHint(), maximumSize(), and heightForWidth() 
       all work in terms of layout item rect coordinates, so we have to 
       add or subtract widgetRectSurplus here and there. The code could 
       be much simpler if we did everything using layout item rect 
       coordinates and did the conversion right before the call to 
       QWidget::setGeometry(). 
     */ 

    QSize s = r.size().boundedTo(maximumSize() + widgetRectSurplus);  
    int x = r.x();
    int y = r.y();
    if (align & (Qt::AlignHorizontal_Mask | Qt::AlignVertical_Mask)) {
        QSize pref(sizeHint());
        QSizePolicy sp = wid->sizePolicy();
        if (sp.horizontalPolicy() == QSizePolicy::Ignored)
            pref.setWidth(wid->sizeHint().expandedTo(wid->minimumSize()).width());
        if (sp.verticalPolicy() == QSizePolicy::Ignored)
            pref.setHeight(wid->sizeHint().expandedTo(wid->minimumSize()).height());
        pref += widgetRectSurplus;
        if (align & Qt::AlignHorizontal_Mask)
            s.setWidth(qMin(s.width(), pref.width()));
        if (align & Qt::AlignVertical_Mask) {
            if (hasHeightForWidth())
                s.setHeight(qMin(s.height(), 
                                 heightForWidth(s.width() - widgetRectSurplus.width()) 
                                 + widgetRectSurplus.height()));
            else
                s.setHeight(qMin(s.height(), pref.height()));
        }
    }
    Qt::Alignment alignHoriz = QStyle::visualAlignment(wid->layoutDirection(), align);
    if (alignHoriz & Qt::AlignRight)
        x = x + (r.width() - s.width());
    else if (!(alignHoriz & Qt::AlignLeft))
        x = x + (r.width() - s.width()) / 2;

    if (align & Qt::AlignBottom)
        y = y + (r.height() - s.height());
    else if (!(align & Qt::AlignTop))
        y = y + (r.height() - s.height()) / 2;

    wid->setGeometry(x, y, s.width(), s.height());
}

/*!
    \reimp
*/
QRect QSpacerItem::geometry() const
{
    return rect;
}

/*!
    \reimp
*/
QRect QWidgetItem::geometry() const
{
    return !wid->testAttribute(Qt::WA_LayoutUsesWidgetRect)
           ? toLayoutItemRect(wid->d_func(), wid->geometry())
           : wid->geometry();
}


/*!
    \reimp
*/
bool QWidgetItem::hasHeightForWidth() const
{
    if (isEmpty())
        return false;
    return wid->d_func()->hasHeightForWidth();
}

/*!
    \reimp
*/
int QWidgetItem::heightForWidth(int w) const
{
    if (isEmpty())
        return -1;

    w = !wid->testAttribute(Qt::WA_LayoutUsesWidgetRect)
      ? fromLayoutItemSize(wid->d_func(), QSize(w, 0)).width()
      : w;

    int hfw;
    if (wid->layout())
        hfw = wid->layout()->totalHeightForWidth(w);
    else
        hfw = wid->heightForWidth(w);

    if (hfw > wid->maximumHeight())
        hfw = wid->maximumHeight();
    if (hfw < wid->minimumHeight())
        hfw = wid->minimumHeight();

    hfw = !wid->testAttribute(Qt::WA_LayoutUsesWidgetRect)
        ? toLayoutItemSize(wid->d_func(), QSize(0, hfw)).height()
        : hfw;

    if (hfw < 0)
        hfw = 0;
    return hfw;
}

/*!
    \reimp
*/
Qt::Orientations QSpacerItem::expandingDirections() const
{
    return sizeP.expandingDirections();
}

/*!
    \reimp
*/
Qt::Orientations QWidgetItem::expandingDirections() const
{
    if (isEmpty())
        return Qt::Orientations(0);

    Qt::Orientations e = wid->sizePolicy().expandingDirections();
    /*
      ### Qt 4.0:
      If the layout is expanding, we make the widget expanding, even if
      its own size policy isn't expanding. This behavior should be
      reconsidered.
    */
    if (wid->layout()) {
        if (wid->sizePolicy().horizontalPolicy() & QSizePolicy::GrowFlag
                && (wid->layout()->expandingDirections() & Qt::Horizontal))
            e |= Qt::Horizontal;
        if (wid->sizePolicy().verticalPolicy() & QSizePolicy::GrowFlag
                && (wid->layout()->expandingDirections() & Qt::Vertical))
            e |= Qt::Vertical;
    }

    if (align & Qt::AlignHorizontal_Mask)
        e &= ~Qt::Horizontal;
    if (align & Qt::AlignVertical_Mask)
        e &= ~Qt::Vertical;
    return e;
}

/*!
    \reimp
*/
QSize QSpacerItem::minimumSize() const
{
    return QSize(sizeP.horizontalPolicy() & QSizePolicy::ShrinkFlag ? 0 : width,
                 sizeP.verticalPolicy() & QSizePolicy::ShrinkFlag ? 0 : height);
}

/*!
    \reimp
*/
QSize QWidgetItem::minimumSize() const
{
    if (isEmpty())
        return QSize(0, 0);
    return !wid->testAttribute(Qt::WA_LayoutUsesWidgetRect)
           ? toLayoutItemSize(wid->d_func(), qSmartMinSize(this))
           : qSmartMinSize(this);
}

/*!
    \reimp
*/
QSize QSpacerItem::maximumSize() const
{
    return QSize(sizeP.horizontalPolicy() & QSizePolicy::GrowFlag ? QLAYOUTSIZE_MAX : width,
                 sizeP.verticalPolicy() & QSizePolicy::GrowFlag ? QLAYOUTSIZE_MAX : height);
}

/*!
    \reimp
*/
QSize QWidgetItem::maximumSize() const
{
    if (isEmpty()) {
        return QSize(0, 0);
    } else {
        return !wid->testAttribute(Qt::WA_LayoutUsesWidgetRect)
               ? toLayoutItemSize(wid->d_func(), qSmartMaxSize(this, align))
               : qSmartMaxSize(this, align);
    }
}

/*!
    \reimp
*/
QSize QSpacerItem::sizeHint() const
{
    return QSize(width, height);
}

/*!
    \reimp
*/
QSize QWidgetItem::sizeHint() const
{
    QSize s(0, 0);
    if (!isEmpty()) {
        s = wid->sizeHint().expandedTo(wid->minimumSizeHint());
        s = s.boundedTo(wid->maximumSize())
             .expandedTo(wid->minimumSize());
        s = !wid->testAttribute(Qt::WA_LayoutUsesWidgetRect)
           ? toLayoutItemSize(wid->d_func(), s)
           : s;

        if (wid->sizePolicy().horizontalPolicy() == QSizePolicy::Ignored)
            s.setWidth(0);
        if (wid->sizePolicy().verticalPolicy() == QSizePolicy::Ignored)
            s.setHeight(0);
    }
    return s;
}

/*!
    Returns true.
*/
bool QSpacerItem::isEmpty() const
{
    return true;
}

/*!
    Returns true if the widget is hidden; otherwise returns false.

    \sa QWidget::isHidden()
*/
bool QWidgetItem::isEmpty() const
{
    return wid->isHidden() || wid->isWindow();
}

/*!
    \class QWidgetItemV2
    \internal
*/

inline bool QWidgetItemV2::useSizeCache() const
{
    return wid->d_func()->widgetItem == this;
}

void QWidgetItemV2::updateCacheIfNecessary() const
{
    if (q_cachedMinimumSize.width() != Dirty)
        return;

    const QSize sizeHint(wid->sizeHint());
    const QSize minimumSizeHint(wid->minimumSizeHint());
    const QSize minimumSize(wid->minimumSize());
    const QSize maximumSize(wid->maximumSize());
    const QSizePolicy sizePolicy(wid->sizePolicy());
    const QSize expandedSizeHint(sizeHint.expandedTo(minimumSizeHint));

    const QSize smartMinSize(qSmartMinSize(sizeHint, minimumSizeHint, minimumSize, maximumSize, sizePolicy));
    const QSize smartMaxSize(qSmartMaxSize(expandedSizeHint, minimumSize, maximumSize, sizePolicy, align));

    const bool useLayoutItemRect = !wid->testAttribute(Qt::WA_LayoutUsesWidgetRect);

    q_cachedMinimumSize = useLayoutItemRect
           ? toLayoutItemSize(wid->d_func(), smartMinSize)
           : smartMinSize;

    q_cachedSizeHint = expandedSizeHint;
    q_cachedSizeHint = q_cachedSizeHint.boundedTo(maximumSize)
                                       .expandedTo(minimumSize);
    q_cachedSizeHint = useLayoutItemRect
           ? toLayoutItemSize(wid->d_func(), q_cachedSizeHint)
           : q_cachedSizeHint;

    if (wid->sizePolicy().horizontalPolicy() == QSizePolicy::Ignored)
        q_cachedSizeHint.setWidth(0);
    if (wid->sizePolicy().verticalPolicy() == QSizePolicy::Ignored)
        q_cachedSizeHint.setHeight(0);

    q_cachedMaximumSize = useLayoutItemRect
               ? toLayoutItemSize(wid->d_func(), smartMaxSize)
               : smartMaxSize;
}

QWidgetItemV2::QWidgetItemV2(QWidget *widget)
    : QWidgetItem(widget),
      q_cachedMinimumSize(Dirty, Dirty),
      q_cachedSizeHint(Dirty, Dirty),
      q_cachedMaximumSize(Dirty, Dirty),
      q_firstCachedHfw(0),
      q_hfwCacheSize(0),
      d(0)
{
    QWidgetPrivate *wd = wid->d_func();
    if (!wd->widgetItem)
        wd->widgetItem = this;
}

QWidgetItemV2::~QWidgetItemV2()
{
    if (wid) {
        QWidgetPrivate *wd = wid->d_func();
        if (wd->widgetItem == this)
            wd->widgetItem = 0;
    }
}

QSize QWidgetItemV2::sizeHint() const
{
    if (isEmpty())
        return QSize(0, 0);

    if (useSizeCache()) {
        updateCacheIfNecessary();
        return q_cachedSizeHint;
    } else {
        return QWidgetItem::sizeHint();
    }
}

QSize QWidgetItemV2::minimumSize() const
{
    if (isEmpty())
        return QSize(0, 0);

    if (useSizeCache()) {
        updateCacheIfNecessary();
        return q_cachedMinimumSize;
    } else {
        return QWidgetItem::minimumSize();
    }
}

QSize QWidgetItemV2::maximumSize() const
{
    if (isEmpty())
        return QSize(0, 0);

    if (useSizeCache()) {
        updateCacheIfNecessary();
        return q_cachedMaximumSize;
    } else {
        return QWidgetItem::maximumSize();
    }
}

/*
    The height-for-width cache is organized as a circular buffer. The entries

        q_hfwCachedHfws[q_firstCachedHfw],
        ...,
        q_hfwCachedHfws[(q_firstCachedHfw + q_hfwCacheSize - 1) % HfwCacheMaxSize]

    contain the last cached values. When the cache is full, the first entry to
    be erased is the entry before q_hfwCachedHfws[q_firstCachedHfw]. When
    values are looked up, we try to move q_firstCachedHfw to point to that new
    entry (unless the cache is not full, in which case it would leave the cache
    in a broken state), so that the most recently used entry is also the last
    to be erased.
*/

int QWidgetItemV2::heightForWidth(int width) const
{
    if (isEmpty())
        return -1;

    for (int i = 0; i < q_hfwCacheSize; ++i) {
        int offset = q_firstCachedHfw + i;
        const QSize &size = q_cachedHfws[offset % HfwCacheMaxSize];
        if (size.width() == width) {
            if (q_hfwCacheSize == HfwCacheMaxSize)
                q_firstCachedHfw = offset;
            return size.height();
        }
    }

    if (q_hfwCacheSize < HfwCacheMaxSize)
        ++q_hfwCacheSize;
    q_firstCachedHfw = (q_firstCachedHfw + HfwCacheMaxSize - 1) % HfwCacheMaxSize;

    int height = QWidgetItem::heightForWidth(width);
    q_cachedHfws[q_firstCachedHfw] = QSize(width, height);
    return height;
}

QT_END_NAMESPACE
