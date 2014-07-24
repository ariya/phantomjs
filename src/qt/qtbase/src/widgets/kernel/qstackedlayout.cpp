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

#include "qstackedlayout.h"
#include "qlayout_p.h"

#include <qlist.h>
#include <qwidget.h>
#include "private/qlayoutengine_p.h"

QT_BEGIN_NAMESPACE

class QStackedLayoutPrivate : public QLayoutPrivate
{
    Q_DECLARE_PUBLIC(QStackedLayout)
public:
    QStackedLayoutPrivate() : index(-1), stackingMode(QStackedLayout::StackOne) {}
    QLayoutItem* replaceAt(int index, QLayoutItem *newitem) Q_DECL_OVERRIDE;
    QList<QLayoutItem *> list;
    int index;
    QStackedLayout::StackingMode stackingMode;
};

QLayoutItem* QStackedLayoutPrivate::replaceAt(int idx, QLayoutItem *newitem)
{
    Q_Q(QStackedLayout);
    if (idx < 0 || idx >= list.size() || !newitem)
        return 0;
    QWidget *wdg = newitem->widget();
    if (!wdg) {
        qWarning("QStackedLayout::replaceAt: Only widgets can be added");
        return 0;
    }
    QLayoutItem *orgitem = list.at(idx);
    list.replace(idx, newitem);
    if (idx == index)
        q->setCurrentIndex(index);
    return orgitem;
}

/*!
    \class QStackedLayout

    \brief The QStackedLayout class provides a stack of widgets where
    only one widget is visible at a time.

    \ingroup geomanagement
    \inmodule QtWidgets

    QStackedLayout can be used to create a user interface similar to
    the one provided by QTabWidget. There is also a convenience
    QStackedWidget class built on top of QStackedLayout.

    A QStackedLayout can be populated with a number of child widgets
    ("pages"). For example:

    \snippet qstackedlayout/main.cpp 0
    \codeline
    \snippet qstackedlayout/main.cpp 2
    \snippet qstackedlayout/main.cpp 3

    QStackedLayout provides no intrinsic means for the user to switch
    page. This is typically done through a QComboBox or a QListWidget
    that stores the titles of the QStackedLayout's pages. For
    example:

    \snippet qstackedlayout/main.cpp 1

    When populating a layout, the widgets are added to an internal
    list. The indexOf() function returns the index of a widget in that
    list. The widgets can either be added to the end of the list using
    the addWidget() function, or inserted at a given index using the
    insertWidget() function. The removeWidget() function removes the
    widget at the given index from the layout. The number of widgets
    contained in the layout, can be obtained using the count()
    function.

    The widget() function returns the widget at a given index
    position. The index of the widget that is shown on screen is given
    by currentIndex() and can be changed using setCurrentIndex(). In a
    similar manner, the currently shown widget can be retrieved using
    the currentWidget() function, and altered using the
    setCurrentWidget() function.

    Whenever the current widget in the layout changes or a widget is
    removed from the layout, the currentChanged() and widgetRemoved()
    signals are emitted respectively.

    \sa QStackedWidget, QTabWidget
*/

/*!
    \fn void QStackedLayout::currentChanged(int index)

    This signal is emitted whenever the current widget in the layout
    changes.  The \a index specifies the index of the new current
    widget, or -1 if there isn't a new one (for example, if there
    are no widgets in the QStackedLayout)

    \sa currentWidget(), setCurrentWidget()
*/

/*!
    \fn void QStackedLayout::widgetRemoved(int index)

    This signal is emitted whenever a widget is removed from the
    layout. The widget's \a index is passed as parameter.

    \sa removeWidget()
*/

/*!
    \fn QWidget *QStackedLayout::widget()
    \internal
*/

/*!
    Constructs a QStackedLayout with no parent.

    This QStackedLayout must be installed on a widget later on to
    become effective.

    \sa addWidget(), insertWidget()
*/
QStackedLayout::QStackedLayout()
    : QLayout(*new QStackedLayoutPrivate, 0, 0)
{
}

/*!
    Constructs a new QStackedLayout with the given \a parent.

    This layout will install itself on the \a parent widget and
    manage the geometry of its children.
*/
QStackedLayout::QStackedLayout(QWidget *parent)
    : QLayout(*new QStackedLayoutPrivate, 0, parent)
{
}

/*!
    Constructs a new QStackedLayout and inserts it into
    the given \a parentLayout.
*/
QStackedLayout::QStackedLayout(QLayout *parentLayout)
    : QLayout(*new QStackedLayoutPrivate, parentLayout, 0)
{
}

/*!
    Destroys this QStackedLayout. Note that the layout's widgets are
    \e not destroyed.
*/
QStackedLayout::~QStackedLayout()
{
    Q_D(QStackedLayout);
    qDeleteAll(d->list);
}

/*!
    Adds the given \a widget to the end of this layout and returns the
    index position of the \a widget.

    If the QStackedLayout is empty before this function is called,
    the given \a widget becomes the current widget.

    \sa insertWidget(), removeWidget(), setCurrentWidget()
*/
int QStackedLayout::addWidget(QWidget *widget)
{
    Q_D(QStackedLayout);
    return insertWidget(d->list.count(), widget);
}

/*!
    Inserts the given \a widget at the given \a index in this
    QStackedLayout. If \a index is out of range, the widget is
    appended (in which case it is the actual index of the \a widget
    that is returned).

    If the QStackedLayout is empty before this function is called, the
    given \a widget becomes the current widget.

    Inserting a new widget at an index less than or equal to the current index
    will increment the current index, but keep the current widget.

    \sa addWidget(), removeWidget(), setCurrentWidget()
*/
int QStackedLayout::insertWidget(int index, QWidget *widget)
{
    Q_D(QStackedLayout);
    addChildWidget(widget);
    index = qMin(index, d->list.count());
    if (index < 0)
        index = d->list.count();
    QWidgetItem *wi = QLayoutPrivate::createWidgetItem(this, widget);
    d->list.insert(index, wi);
    invalidate();
    if (d->index < 0) {
        setCurrentIndex(index);
    } else {
        if (index <= d->index)
            ++d->index;
        if (d->stackingMode == StackOne)
            widget->hide();
        widget->lower();
    }
    return index;
}

/*!
    \reimp
*/
QLayoutItem *QStackedLayout::itemAt(int index) const
{
    Q_D(const QStackedLayout);
    return d->list.value(index);
}

// Code that enables proper handling of the case that takeAt() is
// called somewhere inside QObject destructor (can't call hide()
// on the object then)

class QtFriendlyLayoutWidget : public QWidget
{
public:
    inline bool wasDeleted() const { return d_ptr->wasDeleted; }
};

static bool qt_wasDeleted(const QWidget *w) { return static_cast<const QtFriendlyLayoutWidget*>(w)->wasDeleted(); }


/*!
    \reimp
*/
QLayoutItem *QStackedLayout::takeAt(int index)
{
    Q_D(QStackedLayout);
    if (index <0 || index >= d->list.size())
        return 0;
    QLayoutItem *item = d->list.takeAt(index);
    if (index == d->index) {
        d->index = -1;
        if ( d->list.count() > 0 ) {
            int newIndex = (index == d->list.count()) ? index-1 : index;
            setCurrentIndex(newIndex);
        } else {
            emit currentChanged(-1);
        }
    } else if (index < d->index) {
        --d->index;
    }
    emit widgetRemoved(index);
    if (item->widget() && !qt_wasDeleted(item->widget()))
        item->widget()->hide();
    return item;
}

/*!
    \property QStackedLayout::currentIndex
    \brief the index position of the widget that is visible

    The current index is -1 if there is no current widget.

    \sa currentWidget(), indexOf()
*/
void QStackedLayout::setCurrentIndex(int index)
{
    Q_D(QStackedLayout);
    QWidget *prev = currentWidget();
    QWidget *next = widget(index);
    if (!next || next == prev)
        return;

    bool reenableUpdates = false;
    QWidget *parent = parentWidget();

    if (parent && parent->updatesEnabled()) {
        reenableUpdates = true;
        parent->setUpdatesEnabled(false);
    }

    QPointer<QWidget> fw = parent ? parent->window()->focusWidget() : 0;
    const bool focusWasOnOldPage = fw && (prev && prev->isAncestorOf(fw));

    if (prev) {
        prev->clearFocus();
        if (d->stackingMode == StackOne)
            prev->hide();
    }

    d->index = index;
    next->raise();
    next->show();

    // try to move focus onto the incoming widget if focus
    // was somewhere on the outgoing widget.

    if (parent) {
        if (focusWasOnOldPage) {
            // look for the best focus widget we can find
            if (QWidget *nfw = next->focusWidget())
                nfw->setFocus();
            else {
                // second best: first child widget in the focus chain
                if (QWidget *i = fw) {
                    while ((i = i->nextInFocusChain()) != fw) {
                        if (((i->focusPolicy() & Qt::TabFocus) == Qt::TabFocus)
                            && !i->focusProxy() && i->isVisibleTo(next) && i->isEnabled()
                            && next->isAncestorOf(i)) {
                            i->setFocus();
                            break;
                        }
                    }
                    // third best: incoming widget
                    if (i == fw )
                        next->setFocus();
                }
            }
        }
    }
    if (reenableUpdates)
        parent->setUpdatesEnabled(true);
    emit currentChanged(index);
}

int QStackedLayout::currentIndex() const
{
    Q_D(const QStackedLayout);
    return d->index;
}


/*!
    \fn void QStackedLayout::setCurrentWidget(QWidget *widget)

    Sets the current widget to be the specified \a widget. The new
    current widget must already be contained in this stacked layout.

    \sa setCurrentIndex(), currentWidget()
 */
void QStackedLayout::setCurrentWidget(QWidget *widget)
{
    int index = indexOf(widget);
    if (index == -1) {
        qWarning("QStackedLayout::setCurrentWidget: Widget %p not contained in stack", widget);
        return;
    }
    setCurrentIndex(index);
}


/*!
    Returns the current widget, or 0 if there are no widgets in this
    layout.

    \sa currentIndex(), setCurrentWidget()
*/
QWidget *QStackedLayout::currentWidget() const
{
    Q_D(const QStackedLayout);
    return d->index >= 0 ? d->list.at(d->index)->widget() : 0;
}

/*!
    Returns the widget at the given \a index, or 0 if there is no
    widget at the given position.

    \sa currentWidget(), indexOf()
*/
QWidget *QStackedLayout::widget(int index) const
{
    Q_D(const QStackedLayout);
     if (index < 0 || index >= d->list.size())
        return 0;
    return d->list.at(index)->widget();
}

/*!
    \property QStackedLayout::count
    \brief the number of widgets contained in the layout

    \sa currentIndex(), widget()
*/
int QStackedLayout::count() const
{
    Q_D(const QStackedLayout);
    return d->list.size();
}


/*!
    \reimp
*/
void QStackedLayout::addItem(QLayoutItem *item)
{
    QWidget *widget = item->widget();
    if (widget) {
        addWidget(widget);
        delete item;
    } else {
        qWarning("QStackedLayout::addItem: Only widgets can be added");
    }
}

/*!
    \reimp
*/
QSize QStackedLayout::sizeHint() const
{
    Q_D(const QStackedLayout);
    QSize s(0, 0);
    int n = d->list.count();

    for (int i = 0; i < n; ++i)
        if (QWidget *widget = d->list.at(i)->widget()) {
            QSize ws(widget->sizeHint());
            if (widget->sizePolicy().horizontalPolicy() == QSizePolicy::Ignored)
                ws.setWidth(0);
            if (widget->sizePolicy().verticalPolicy() == QSizePolicy::Ignored)
                ws.setHeight(0);
            s = s.expandedTo(ws);
        }
    return s;
}

/*!
    \reimp
*/
QSize QStackedLayout::minimumSize() const
{
    Q_D(const QStackedLayout);
    QSize s(0, 0);
    int n = d->list.count();

    for (int i = 0; i < n; ++i)
        if (QWidget *widget = d->list.at(i)->widget())
            s = s.expandedTo(qSmartMinSize(widget));
    return s;
}

/*!
    \reimp
*/
void QStackedLayout::setGeometry(const QRect &rect)
{
    Q_D(QStackedLayout);
    switch (d->stackingMode) {
    case StackOne:
        if (QWidget *widget = currentWidget())
            widget->setGeometry(rect);
        break;
    case StackAll:
        if (const int n = d->list.count())
            for (int i = 0; i < n; ++i)
                if (QWidget *widget = d->list.at(i)->widget())
                    widget->setGeometry(rect);
        break;
    }
}

/*!
    \reimp
*/
bool QStackedLayout::hasHeightForWidth() const
{
    const int n = count();

    for (int i = 0; i < n; ++i) {
        if (QLayoutItem *item = itemAt(i)) {
            if (item->hasHeightForWidth())
                return true;
        }
    }
    return false;
}

/*!
    \reimp
*/
int QStackedLayout::heightForWidth(int width) const
{
    const int n = count();

    int hfw = 0;
    for (int i = 0; i < n; ++i) {
        if (QLayoutItem *item = itemAt(i)) {
            if (QWidget *w = item->widget())
                /*
                Note: Does not query the layout item, but bypasses it and asks the widget
                directly. This is consistent with how QStackedLayout::sizeHint() is
                implemented. This also avoids an issue where QWidgetItem::heightForWidth()
                returns -1 if the widget is hidden.
                */
                hfw = qMax(hfw, w->heightForWidth(width));
        }
    }
    hfw = qMax(hfw, minimumSize().height());
    return hfw;
}

/*!
    \enum QStackedLayout::StackingMode
    \since 4.4

    This enum specifies how the layout handles its child widgets
    regarding their visibility.

    \value StackOne
           Only the current widget is visible. This is the default.

    \value StackAll
           All widgets are visible. The current widget is merely raised.
*/


/*!
    \property QStackedLayout::stackingMode
    \brief determines the way visibility of child widgets are handled.
    \since 4.4

    The default value is StackOne. Setting the property to StackAll
    allows you to make use of the layout for overlay widgets
    that do additional drawing on top of other widgets, for example,
    graphical editors.
*/

QStackedLayout::StackingMode QStackedLayout::stackingMode() const
{
    Q_D(const QStackedLayout);
    return d->stackingMode;
}

void QStackedLayout::setStackingMode(StackingMode stackingMode)
{
    Q_D(QStackedLayout);
    if (d->stackingMode == stackingMode)
        return;
    d->stackingMode = stackingMode;

    const int n = d->list.count();
    if (n == 0)
        return;

    switch (d->stackingMode) {
    case StackOne:
        if (const int idx = currentIndex())
            for (int i = 0; i < n; ++i)
                if (QWidget *widget = d->list.at(i)->widget())
                    widget->setVisible(i == idx);
        break;
    case StackAll: { // Turn overlay on: Make sure all widgets are the same size
        QRect geometry;
        if (const QWidget *widget = currentWidget())
            geometry = widget->geometry();
        for (int i = 0; i < n; ++i)
            if (QWidget *widget = d->list.at(i)->widget()) {
                if (!geometry.isNull())
                    widget->setGeometry(geometry);
                widget->setVisible(true);
            }
    }
        break;
    }
}

QT_END_NAMESPACE
