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

#include "qabstractitemdelegate.h"

#ifndef QT_NO_ITEMVIEWS
#include <qabstractitemmodel.h>
#include <qabstractitemview.h>
#include <qfontmetrics.h>
#include <qwhatsthis.h>
#include <qtooltip.h>
#include <qevent.h>
#include <qstring.h>
#include <qdebug.h>
#include <private/qtextengine_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QAbstractItemDelegate

    \brief The QAbstractItemDelegate class is used to display and edit
    data items from a model.

    \ingroup model-view


    A QAbstractItemDelegate provides the interface and common functionality
    for delegates in the model/view architecture. Delegates display
    individual items in views, and handle the editing of model data.

    The QAbstractItemDelegate class is one of the \l{Model/View Classes}
    and is part of Qt's \l{Model/View Programming}{model/view framework}.

    To render an item in a custom way, you must implement paint() and
    sizeHint(). The QItemDelegate class provides default implementations for
    these functions; if you do not need custom rendering, subclass that
    class instead.

    We give an example of drawing a progress bar in items; in our case
    for a package management program.

    \image widgetdelegate.png

    We create the \c WidgetDelegate class, which inherits from
    QStyledItemDelegate. We do the drawing in the paint() function:

    \snippet doc/src/snippets/widgetdelegate.cpp 0

    Notice that we use a QStyleOptionProgressBar and initialize its
    members. We can then use the current QStyle to draw it.

    To provide custom editing, there are two approaches that can be
    used. The first approach is to create an editor widget and display
    it directly on top of the item. To do this you must reimplement
    createEditor() to provide an editor widget, setEditorData() to populate
    the editor with the data from the model, and setModelData() so that the
    delegate can update the model with data from the editor.

    The second approach is to handle user events directly by reimplementing
    editorEvent().

    \sa {model-view-programming}{Model/View Programming}, QItemDelegate,
        {Pixelator Example}, QStyledItemDelegate, QStyle
*/

/*!
    \enum QAbstractItemDelegate::EndEditHint

    This enum describes the different hints that the delegate can give to the
    model and view components to make editing data in a model a comfortable
    experience for the user.

    \value NoHint           There is no recommended action to be performed.

    These hints let the delegate influence the behavior of the view:

    \value EditNextItem     The view should use the delegate to open an
                            editor on the next item in the view.
    \value EditPreviousItem The view should use the delegate to open an
                            editor on the previous item in the view.

    Note that custom views may interpret the concepts of next and previous
    differently.

    The following hints are most useful when models are used that cache
    data, such as those that manipulate data locally in order to increase
    performance or conserve network bandwidth.

    \value SubmitModelCache If the model caches data, it should write out
                            cached data to the underlying data store.
    \value RevertModelCache If the model caches data, it should discard
                            cached data and replace it with data from the
                            underlying data store.

    Although models and views should respond to these hints in appropriate
    ways, custom components may ignore any or all of them if they are not
    relevant.
*/

/*!
  \fn void QAbstractItemDelegate::commitData(QWidget *editor)

  This signal must be emitted when the \a editor widget has completed
  editing the data, and wants to write it back into the model.
*/

/*!
    \fn void QAbstractItemDelegate::closeEditor(QWidget *editor, QAbstractItemDelegate::EndEditHint hint)

    This signal is emitted when the user has finished editing an item using
    the specified \a editor.

    The \a hint provides a way for the delegate to influence how the model and
    view behave after editing is completed. It indicates to these components
    what action should be performed next to provide a comfortable editing
    experience for the user. For example, if \c EditNextItem is specified,
    the view should use a delegate to open an editor on the next item in the
    model.

    \sa EndEditHint
*/

/*!
    \fn void QAbstractItemDelegate::sizeHintChanged(const QModelIndex &index)
    \since 4.4

    This signal must be emitted when the sizeHint() of \a index changed.

    Views automatically connect to this signal and relayout items as necessary.
*/


/*!
    Creates a new abstract item delegate with the given \a parent.
*/
QAbstractItemDelegate::QAbstractItemDelegate(QObject *parent)
    : QObject(parent)
{

}

/*!
    \internal

    Creates a new abstract item delegate with the given \a parent.
*/
QAbstractItemDelegate::QAbstractItemDelegate(QObjectPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{

}

/*!
    Destroys the abstract item delegate.
*/
QAbstractItemDelegate::~QAbstractItemDelegate()
{

}

/*!
    \fn void QAbstractItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const = 0;

    This pure abstract function must be reimplemented if you want to
    provide custom rendering. Use the \a painter and style \a option to
    render the item specified by the item \a index.

    If you reimplement this you must also reimplement sizeHint().
*/

/*!
    \fn QSize QAbstractItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const = 0

    This pure abstract function must be reimplemented if you want to
    provide custom rendering. The options are specified by \a option
    and the model item by \a index.

    If you reimplement this you must also reimplement paint().
*/

/*!
    Returns the editor to be used for editing the data item with the
    given \a index. Note that the index contains information about the
    model being used. The editor's parent widget is specified by \a parent,
    and the item options by \a option.

    The base implementation returns 0. If you want custom editing you
    will need to reimplement this function.

    The returned editor widget should have Qt::StrongFocus;
    otherwise, \l{QMouseEvent}s received by the widget will propagate
    to the view. The view's background will shine through unless the
    editor paints its own background (e.g., with
    \l{QWidget::}{setAutoFillBackground()}).

    \sa setModelData() setEditorData()
*/
QWidget *QAbstractItemDelegate::createEditor(QWidget *,
                                             const QStyleOptionViewItem &,
                                             const QModelIndex &) const
{
    return 0;
}

/*!
    Sets the contents of the given \a editor to the data for the item
    at the given \a index. Note that the index contains information
    about the model being used.

    The base implementation does nothing. If you want custom editing
    you will need to reimplement this function.

    \sa setModelData()
*/
void QAbstractItemDelegate::setEditorData(QWidget *,
                                          const QModelIndex &) const
{
    // do nothing
}

/*!
    Sets the data for the item at the given \a index in the \a model
    to the contents of the given \a editor.

    The base implementation does nothing. If you want custom editing
    you will need to reimplement this function.

    \sa setEditorData()
*/
void QAbstractItemDelegate::setModelData(QWidget *,
                                         QAbstractItemModel *,
                                         const QModelIndex &) const
{
    // do nothing
}

/*!
    Updates the geometry of the \a editor for the item with the given
    \a index, according to the rectangle specified in the \a option.
    If the item has an internal layout, the editor will be laid out
    accordingly. Note that the index contains information about the
    model being used.

    The base implementation does nothing. If you want custom editing
    you must reimplement this function.
*/
void QAbstractItemDelegate::updateEditorGeometry(QWidget *,
                                                 const QStyleOptionViewItem &,
                                                 const QModelIndex &) const
{
    // do nothing
}

/*!
    When editing of an item starts, this function is called with the
    \a event that triggered the editing, the \a model, the \a index of
    the item, and the \a option used for rendering the item.

    Mouse events are sent to editorEvent() even if they don't start
    editing of the item. This can, for instance, be useful if you wish
    to open a context menu when the right mouse button is pressed on
    an item.

    The base implementation returns false (indicating that it has not
    handled the event).
*/
bool QAbstractItemDelegate::editorEvent(QEvent *,
                                        QAbstractItemModel *,
                                        const QStyleOptionViewItem &,
                                        const QModelIndex &)
{
    // do nothing
    return false;
}

/*!
    \obsolete

    Use QFontMetrics::elidedText() instead.

    \oldcode
        QFontMetrics fm = ...
        QString str = QAbstractItemDelegate::elidedText(fm, width, mode, text);
    \newcode
        QFontMetrics fm = ...
        QString str = fm.elidedText(text, mode, width);
    \endcode
*/

QString QAbstractItemDelegate::elidedText(const QFontMetrics &fontMetrics, int width,
                                          Qt::TextElideMode mode, const QString &text)
{
    return fontMetrics.elidedText(text, mode, width);
}

/*!
    \since 4.3
    Whenever a help event occurs, this function is called with the \a event
    \a view \a option and the \a index that corresponds to the item where the
    event occurs.

    Returns true if the delegate can handle the event; otherwise returns false.
    A return value of true indicates that the data obtained using the index had
    the required role.

    For QEvent::ToolTip and QEvent::WhatsThis events that were handled successfully,
    the relevant popup may be shown depending on the user's system configuration.

    \sa QHelpEvent
*/
// ### Qt 5: Make this a virtual non-slot function
bool QAbstractItemDelegate::helpEvent(QHelpEvent *event,
                                      QAbstractItemView *view,
                                      const QStyleOptionViewItem &option,
                                      const QModelIndex &index)
{
    Q_UNUSED(option);

    if (!event || !view)
        return false;
    switch (event->type()) {
#ifndef QT_NO_TOOLTIP
    case QEvent::ToolTip: {
        QHelpEvent *he = static_cast<QHelpEvent*>(event);
        QVariant tooltip = index.data(Qt::ToolTipRole);
        if (tooltip.canConvert<QString>()) {
            QToolTip::showText(he->globalPos(), tooltip.toString(), view);
            return true;
        }
        break;}
#endif
#ifndef QT_NO_WHATSTHIS
    case QEvent::QueryWhatsThis: {
        if (index.data(Qt::WhatsThisRole).isValid())
            return true;
        break; }
    case QEvent::WhatsThis: {
        QHelpEvent *he = static_cast<QHelpEvent*>(event);
        QVariant whatsthis = index.data(Qt::WhatsThisRole);
        if (whatsthis.canConvert<QString>()) {
            QWhatsThis::showText(he->globalPos(), whatsthis.toString(), view);
            return true;
        }
        break ; }
#endif
    default:
        break;
    }
    return false;
}

QT_END_NAMESPACE

#endif // QT_NO_ITEMVIEWS
