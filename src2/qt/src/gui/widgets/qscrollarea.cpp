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

#include "qscrollarea.h"
#include "private/qscrollarea_p.h"

#ifndef QT_NO_SCROLLAREA

#include "qscrollbar.h"
#include "qlayout.h"
#include "qstyle.h"
#include "qapplication.h"
#include "qvariant.h"
#include "qdebug.h"
#include "private/qlayoutengine_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QScrollArea

    \brief The QScrollArea class provides a scrolling view onto
    another widget.

    \ingroup basicwidgets


    A scroll area is used to display the contents of a child widget
    within a frame. If the widget exceeds the size of the frame, the
    view can provide scroll bars so that the entire area of the child
    widget can be viewed. The child widget must be specified with
    setWidget(). For example:

    \snippet doc/src/snippets/code/src_gui_widgets_qscrollarea.cpp 0

    The code above creates a scroll area (shown in the images below)
    containing an image label. When scaling the image, the scroll area
    can provide the necessary scroll bars:

    \table
    \row
    \o \inlineimage qscrollarea-noscrollbars.png
    \o \inlineimage qscrollarea-onescrollbar.png
    \o \inlineimage qscrollarea-twoscrollbars.png
    \endtable

    The scroll bars appearance depends on the currently set \l
    {Qt::ScrollBarPolicy}{scroll bar policies}. You can control the
    appearance of the scroll bars using the inherited functionality
    from QAbstractScrollArea.

    For example, you can set the
    QAbstractScrollArea::horizontalScrollBarPolicy and
    QAbstractScrollArea::verticalScrollBarPolicy properties. Or if you
    want the scroll bars to adjust dynamically when the contents of
    the scroll area changes, you can use the \l
    {QAbstractScrollArea::horizontalScrollBar()}{horizontalScrollBar()}
    and \l
    {QAbstractScrollArea::verticalScrollBar()}{verticalScrollBar()}
    functions (which enable you to access the scroll bars) and set the
    scroll bars' values whenever the scroll area's contents change,
    using the QScrollBar::setValue() function.

    You can retrieve the child widget using the widget() function. The
    view can be made to be resizable with the setWidgetResizable()
    function. The alignment of the widget can be specified with
    setAlignment().

    Two convenience functions ensureVisible() and
    ensureWidgetVisible() ensure a certain region of the contents is
    visible inside the viewport, by scrolling the contents if
    necessary.

    \section1 Size Hints and Layouts

    When using a scroll area to display the contents of a custom
    widget, it is important to ensure that the
    \l{QWidget::sizeHint}{size hint} of the child widget is set to a
    suitable value. If a standard QWidget is used for the child
    widget, it may be necessary to call QWidget::setMinimumSize() to
    ensure that the contents of the widget are shown correctly within
    the scroll area.

    If a scroll area is used to display the contents of a widget that
    contains child widgets arranged in a layout, it is important to
    realize that the size policy of the layout will also determine the
    size of the widget. This is especially useful to know if you intend
    to dynamically change the contents of the layout. In such cases,
    setting the layout's \l{QLayout::sizeConstraint}{size constraint}
    property to one which provides constraints on the minimum and/or
    maximum size of the layout (e.g., QLayout::SetMinAndMaxSize) will
    cause the size of the scroll area to be updated whenever the
    contents of the layout changes.

    For a complete example using the QScrollArea class, see the \l
    {widgets/imageviewer}{Image Viewer} example. The example shows how
    to combine QLabel and QScrollArea to display an image.

    \sa QAbstractScrollArea, QScrollBar, {Image Viewer Example}
*/


/*!
    Constructs an empty scroll area with the given \a parent.

    \sa setWidget()
*/
QScrollArea::QScrollArea(QWidget *parent)
    : QAbstractScrollArea(*new QScrollAreaPrivate,parent)
{
    Q_D(QScrollArea);
    d->viewport->setBackgroundRole(QPalette::NoRole);
    d->vbar->setSingleStep(20);
    d->hbar->setSingleStep(20);
    d->layoutChildren();
}

/*!
    \internal
*/
QScrollArea::QScrollArea(QScrollAreaPrivate &dd, QWidget *parent)
    : QAbstractScrollArea(dd, parent)
{
    Q_D(QScrollArea);
    d->viewport->setBackgroundRole(QPalette::NoRole);
    d->vbar->setSingleStep(20);
    d->hbar->setSingleStep(20);
    d->layoutChildren();
}

/*!
    Destroys the scroll area and its child widget.

    \sa setWidget()
*/
QScrollArea::~QScrollArea()
{
}

void QScrollAreaPrivate::updateWidgetPosition()
{
    Q_Q(QScrollArea);
    Qt::LayoutDirection dir = q->layoutDirection();
    QRect scrolled = QStyle::visualRect(dir, viewport->rect(), QRect(QPoint(-hbar->value(), -vbar->value()), widget->size()));
    QRect aligned = QStyle::alignedRect(dir, alignment, widget->size(), viewport->rect());
    widget->move(widget->width() < viewport->width() ? aligned.x() : scrolled.x(),
                 widget->height() < viewport->height() ? aligned.y() : scrolled.y());
}

void QScrollAreaPrivate::updateScrollBars()
{
    Q_Q(QScrollArea);
    if (!widget)
        return;
    QSize p = viewport->size();
    QSize m = q->maximumViewportSize();

    QSize min = qSmartMinSize(widget);
    QSize max = qSmartMaxSize(widget);

    if (resizable) {
        if ((widget->layout() ? widget->layout()->hasHeightForWidth() : widget->sizePolicy().hasHeightForWidth())) {
            QSize p_hfw = p.expandedTo(min).boundedTo(max);
            int h = widget->heightForWidth( p_hfw.width() );
            min = QSize(p_hfw.width(), qMax(p_hfw.height(), h));
        }
    }

    if ((resizable && m.expandedTo(min) == m && m.boundedTo(max) == m)
        || (!resizable && m.expandedTo(widget->size()) == m))
        p = m; // no scroll bars needed

    if (resizable)
        widget->resize(p.expandedTo(min).boundedTo(max));
    QSize v = widget->size();

    hbar->setRange(0, v.width() - p.width());
    hbar->setPageStep(p.width());
    vbar->setRange(0, v.height() - p.height());
    vbar->setPageStep(p.height());
    updateWidgetPosition();

}

/*!
    Returns the scroll area's widget, or 0 if there is none.

    \sa setWidget()
*/

QWidget *QScrollArea::widget() const
{
    Q_D(const QScrollArea);
    return d->widget;
}

/*!
    \fn void QScrollArea::setWidget(QWidget *widget)

    Sets the scroll area's \a widget.

    The \a widget becomes a child of the scroll area, and will be
    destroyed when the scroll area is deleted or when a new widget is
    set.
    
    The widget's \l{QWidget::setAutoFillBackground()}{autoFillBackground}
    property will be set to \c{true}.

    If the scroll area is visible when the \a widget is
    added, you must \l{QWidget::}{show()} it explicitly.

    Note that You must add the layout of \a widget before you call
    this function; if you add it later, the \a widget will not be
    visible - regardless of when you \l{QWidget::}{show()} the scroll
    area. In this case, you can also not \l{QWidget::}{show()} the \a
    widget later.

    \sa widget()
*/
void QScrollArea::setWidget(QWidget *widget)
{
    Q_D(QScrollArea);
    if (widget == d->widget || !widget)
        return;

    delete d->widget;
    d->widget = 0;
    d->hbar->setValue(0);
    d->vbar->setValue(0);
    if (widget->parentWidget() != d->viewport)
        widget->setParent(d->viewport);
    if (!widget->testAttribute(Qt::WA_Resized))
        widget->resize(widget->sizeHint());
    d->widget = widget;
    d->widget->setAutoFillBackground(true);
    widget->installEventFilter(this);
    d->widgetSize = QSize();
    d->updateScrollBars();
    d->widget->show();

}

/*!
    Removes the scroll area's widget, and passes ownership of the
    widget to the caller.

    \sa widget()
 */
QWidget *QScrollArea::takeWidget()
{
    Q_D(QScrollArea);
    QWidget *w = d->widget;
    d->widget = 0;
    if (w)
        w->setParent(0);
    return w;
}

/*!
    \reimp
 */
bool QScrollArea::event(QEvent *e)
{
    Q_D(QScrollArea);
    if (e->type() == QEvent::StyleChange || e->type() == QEvent::LayoutRequest) {
        d->updateScrollBars();
    }
#ifdef QT_KEYPAD_NAVIGATION
    else if (QApplication::keypadNavigationEnabled()) {
        if (e->type() == QEvent::Show)
            QApplication::instance()->installEventFilter(this);
        else if (e->type() == QEvent::Hide)
            QApplication::instance()->removeEventFilter(this);
    }
#endif
    return QAbstractScrollArea::event(e);
}


/*!
    \reimp
 */
bool QScrollArea::eventFilter(QObject *o, QEvent *e)
{
    Q_D(QScrollArea);
#ifdef QT_KEYPAD_NAVIGATION
    if (d->widget && o != d->widget && e->type() == QEvent::FocusIn
            && QApplication::keypadNavigationEnabled()) {
        if (o->isWidgetType())
            ensureWidgetVisible(static_cast<QWidget *>(o));
    }
#endif
    if (o == d->widget && e->type() == QEvent::Resize)
        d->updateScrollBars();

    return false;
}

/*!
    \reimp
 */
void QScrollArea::resizeEvent(QResizeEvent *)
{
    Q_D(QScrollArea);
    d->updateScrollBars();

}


/*!\reimp
 */
void QScrollArea::scrollContentsBy(int, int)
{
    Q_D(QScrollArea);
    if (!d->widget)
        return;
    d->updateWidgetPosition();
}


/*!
    \property QScrollArea::widgetResizable
    \brief whether the scroll area should resize the view widget

    If this property is set to false (the default), the scroll area
    honors the size of its widget. Regardless of this property, you
    can programmatically resize the widget using widget()->resize(),
    and the scroll area will automatically adjust itself to the new
    size.

    If this property is set to true, the scroll area will
    automatically resize the widget in order to avoid scroll bars
    where they can be avoided, or to take advantage of extra space.
*/
bool QScrollArea::widgetResizable() const
{
    Q_D(const QScrollArea);
    return d->resizable;
}

void QScrollArea::setWidgetResizable(bool resizable)
{
    Q_D(QScrollArea);
    d->resizable = resizable;
    updateGeometry();
    d->updateScrollBars();
}

/*!
    \reimp
 */
QSize QScrollArea::sizeHint() const
{
    Q_D(const QScrollArea);
    int f = 2 * d->frameWidth;
    QSize sz(f, f);
    int h = fontMetrics().height();
    if (d->widget) {
        if (!d->widgetSize.isValid())
            d->widgetSize = d->resizable ? d->widget->sizeHint() : d->widget->size();
        sz += d->widgetSize;
    } else {
        sz += QSize(12 * h, 8 * h);
    }
    if (d->vbarpolicy == Qt::ScrollBarAlwaysOn)
        sz.setWidth(sz.width() + d->vbar->sizeHint().width());
    if (d->hbarpolicy == Qt::ScrollBarAlwaysOn)
        sz.setHeight(sz.height() + d->hbar->sizeHint().height());
    return sz.boundedTo(QSize(36 * h, 24 * h));
}



/*!
    \reimp
 */
bool QScrollArea::focusNextPrevChild(bool next)
{
    if (QWidget::focusNextPrevChild(next)) {
        if (QWidget *fw = focusWidget())
            ensureWidgetVisible(fw);
        return true;
    }
    return false;
}

/*!
    Scrolls the contents of the scroll area so that the point (\a x, \a y) is visible
    inside the region of the viewport with margins specified in pixels by \a xmargin and
    \a ymargin. If the specified point cannot be reached, the contents are scrolled to
    the nearest valid position. The default value for both margins is 50 pixels.
*/
void QScrollArea::ensureVisible(int x, int y, int xmargin, int ymargin)
{
    Q_D(QScrollArea);

    int logicalX = QStyle::visualPos(layoutDirection(), d->viewport->rect(), QPoint(x, y)).x();

    if (logicalX - xmargin < d->hbar->value()) {
        d->hbar->setValue(qMax(0, logicalX - xmargin));
    } else if (logicalX > d->hbar->value() + d->viewport->width() - xmargin) {
        d->hbar->setValue(qMin(logicalX - d->viewport->width() + xmargin, d->hbar->maximum()));
    }

    if (y - ymargin < d->vbar->value()) {
        d->vbar->setValue(qMax(0, y - ymargin));
    } else if (y > d->vbar->value() + d->viewport->height() - ymargin) {
        d->vbar->setValue(qMin(y - d->viewport->height() + ymargin, d->vbar->maximum()));
    }
}

/*!
    \since 4.2

    Scrolls the contents of the scroll area so that the \a childWidget
    of QScrollArea::widget() is visible inside the viewport with
    margins specified in pixels by \a xmargin and \a ymargin. If the
    specified point cannot be reached, the contents are scrolled to
    the nearest valid position. The default value for both margins is
    50 pixels.

*/
void QScrollArea::ensureWidgetVisible(QWidget *childWidget, int xmargin, int ymargin)
{
    Q_D(QScrollArea);

    if (!d->widget->isAncestorOf(childWidget))
        return;

    const QRect microFocus = childWidget->inputMethodQuery(Qt::ImMicroFocus).toRect();
    const QRect defaultMicroFocus =
        childWidget->QWidget::inputMethodQuery(Qt::ImMicroFocus).toRect();
    QRect focusRect = (microFocus != defaultMicroFocus)
        ? QRect(childWidget->mapTo(d->widget, microFocus.topLeft()), microFocus.size())
        : QRect(childWidget->mapTo(d->widget, QPoint(0,0)), childWidget->size());
    const QRect visibleRect(-d->widget->pos(), d->viewport->size());

    if (visibleRect.contains(focusRect))
        return;

    focusRect.adjust(-xmargin, -ymargin, xmargin, ymargin);

    if (focusRect.width() > visibleRect.width())
        d->hbar->setValue(focusRect.center().x() - d->viewport->width() / 2);
    else if (focusRect.right() > visibleRect.right())
        d->hbar->setValue(focusRect.right() - d->viewport->width());
    else if (focusRect.left() < visibleRect.left())
        d->hbar->setValue(focusRect.left());

    if (focusRect.height() > visibleRect.height())
        d->vbar->setValue(focusRect.center().y() - d->viewport->height() / 2);
    else if (focusRect.bottom() > visibleRect.bottom())
        d->vbar->setValue(focusRect.bottom() - d->viewport->height());
    else if (focusRect.top() < visibleRect.top())
        d->vbar->setValue(focusRect.top());
}


/*!
    \property QScrollArea::alignment
    \brief the alignment of the scroll area's widget
    \since 4.2

    By default, the widget stays rooted to the top-left corner of the
    scroll area.
*/

void QScrollArea::setAlignment(Qt::Alignment alignment)
{
    Q_D(QScrollArea);
    d->alignment = alignment;
    if (d->widget)
        d->updateWidgetPosition();
}

Qt::Alignment QScrollArea::alignment() const
{
    Q_D(const QScrollArea);
    return d->alignment;
}

QT_END_NAMESPACE

#endif // QT_NO_SCROLLAREA
