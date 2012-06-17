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

#include "qstyleditemdelegate.h"

#ifndef QT_NO_ITEMVIEWS
#include <qabstractitemmodel.h>
#include <qapplication.h>
#include <qbrush.h>
#include <qlineedit.h>
#include <qtextedit.h>
#include <qplaintextedit.h>
#include <qpainter.h>
#include <qpalette.h>
#include <qpoint.h>
#include <qrect.h>
#include <qsize.h>
#include <qstyle.h>
#include <qdatetime.h>
#include <qstyleoption.h>
#include <qevent.h>
#include <qpixmap.h>
#include <qbitmap.h>
#include <qpixmapcache.h>
#include <qitemeditorfactory.h>
#include <private/qitemeditorfactory_p.h>
#include <qmetaobject.h>
#include <qtextlayout.h>
#include <private/qobject_p.h>
#include <private/qdnd_p.h>
#include <private/qtextengine_p.h>
#include <private/qlayoutengine_p.h>
#include <qdebug.h>
#include <qlocale.h>
#include <qdialog.h>
#include <qtableview.h>

#include <limits.h>

QT_BEGIN_NAMESPACE

class QStyledItemDelegatePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QStyledItemDelegate)

public:
    QStyledItemDelegatePrivate() : factory(0) { }

    static const QWidget *widget(const QStyleOptionViewItem &option)
    {
        if (const QStyleOptionViewItemV3 *v3 = qstyleoption_cast<const QStyleOptionViewItemV3 *>(&option))
            return v3->widget;
        return 0;
    }

    const QItemEditorFactory *editorFactory() const
    {
        return factory ? factory : QItemEditorFactory::defaultFactory();
    }

    void _q_commitDataAndCloseEditor(QWidget *editor)
    {
        Q_Q(QStyledItemDelegate);
        emit q->commitData(editor);
        emit q->closeEditor(editor, QAbstractItemDelegate::SubmitModelCache);
    }
    QItemEditorFactory *factory;
};

/*!
    \class QStyledItemDelegate

    \brief The QStyledItemDelegate class provides display and editing facilities for
    data items from a model.

    \ingroup model-view

    \since 4.4

    When displaying data from models in Qt item views, e.g., a
    QTableView, the individual items are drawn by a delegate. Also,
    when an item is edited, it provides an editor widget, which is
    placed on top of the item view while editing takes place.
    QStyledItemDelegate is the default delegate for all Qt item
    views, and is installed upon them when they are created.

    The QStyledItemDelegate class is one of the \l{Model/View Classes}
    and is part of Qt's \l{Model/View Programming}{model/view
    framework}. The delegate allows the display and editing of items
    to be developed independently from the model and view.

    The data of items in models are assigned an
    \l{Qt::}{ItemDataRole}; each item can store a QVariant for each
    role. QStyledItemDelegate implements display and editing for the
    most common datatypes expected by users, including booleans,
    integers, and strings.

    The data will be drawn differently depending on which role they
    have in the model. The following table describes the roles and the
    data types the delegate can handle for each of them. It is often
    sufficient to ensure that the model returns appropriate data for
    each of the roles to determine the appearance of items in views.

    \table
    \header \o Role \o Accepted Types
    \omit
    \row    \o \l Qt::AccessibleDescriptionRole \o QString
    \row    \o \l Qt::AccessibleTextRole \o QString
    \endomit
    \row    \o \l Qt::BackgroundRole \o QBrush
    \row    \o \l Qt::BackgroundColorRole \o QColor (obsolete; use Qt::BackgroundRole instead)
    \row    \o \l Qt::CheckStateRole \o Qt::CheckState
    \row    \o \l Qt::DecorationRole \o QIcon, QPixmap, QImage and QColor
    \row    \o \l Qt::DisplayRole \o QString and types with a string representation
    \row    \o \l Qt::EditRole \o See QItemEditorFactory for details
    \row    \o \l Qt::FontRole \o QFont
    \row    \o \l Qt::SizeHintRole \o QSize
    \omit
    \row    \o \l Qt::StatusTipRole \o
    \endomit
    \row    \o \l Qt::TextAlignmentRole \o Qt::Alignment
    \row    \o \l Qt::ForegroundRole \o QBrush
    \row    \o \l Qt::TextColorRole \o QColor (obsolete; use Qt::ForegroundRole instead)
    \omit
    \row    \o \l Qt::ToolTipRole
    \row    \o \l Qt::WhatsThisRole
    \endomit
    \endtable

    Editors are created with a QItemEditorFactory; a default static
    instance provided by QItemEditorFactory is installed on all item
    delegates. You can set a custom factory using
    setItemEditorFactory() or set a new default factory with
    QItemEditorFactory::setDefaultFactory(). It is the data stored in
    the item model with the \l{Qt::}{EditRole} that is edited. See the
    QItemEditorFactory class for a more high-level introduction to
    item editor factories. The \l{Color Editor Factory Example}{Color
    Editor Factory} example shows how to create custom editors with a
    factory.

    \section1 Subclassing QStyledItemDelegate

    If the delegate does not support painting of the data types you
    need or you want to customize the drawing of items, you need to
    subclass QStyledItemDelegate, and reimplement paint() and possibly
    sizeHint(). The paint() function is called individually for each
    item, and with sizeHint(), you can specify the hint for each
    of them.

    When reimplementing paint(), one would typically handle the
    datatypes one would like to draw and use the superclass
    implementation for other types.

    The painting of check box indicators are performed by the current
    style. The style also specifies the size and the bounding
    rectangles in which to draw the data for the different data roles.
    The bounding rectangle of the item itself is also calculated by
    the style. When drawing already supported datatypes, it is
    therefore a good idea to ask the style for these bounding
    rectangles. The QStyle class description describes this in
    more detail.

    If you wish to change any of the bounding rectangles calculated by
    the style or the painting of check box indicators, you can
    subclass QStyle. Note, however, that the size of the items can
    also be affected by reimplementing sizeHint().

    It is possible for a custom delegate to provide editors
    without the use of an editor item factory. In this case, the
    following virtual functions must be reimplemented:

    \list
        \o createEditor() returns the widget used to change data from the model
           and can be reimplemented to customize editing behavior.
        \o setEditorData() provides the widget with data to manipulate.
        \o updateEditorGeometry() ensures that the editor is displayed correctly
           with respect to the item view.
        \o setModelData() returns updated data to the model.
    \endlist

    The \l{Star Delegate Example}{Star Delegate} example creates
    editors by reimplementing these methods.

    \section1 QStyledItemDelegate vs. QItemDelegate

    Since Qt 4.4, there are two delegate classes: QItemDelegate and
    QStyledItemDelegate. However, the default delegate is QStyledItemDelegate.
    These two classes are independent alternatives to painting and providing
    editors for items in views. The difference between them is that
    QStyledItemDelegate uses the current style to paint its items. We therefore
    recommend using QStyledItemDelegate as the base class when implementing
    custom delegates or when working with Qt style sheets. The code required
    for either class should be equal unless the custom delegate needs to use
    the style for drawing.

    If you wish to customize the painting of item views, you should
    implement a custom style. Please see the QStyle class
    documentation for details.

    \sa {Delegate Classes}, QItemDelegate, QAbstractItemDelegate, QStyle,
        {Spin Box Delegate Example}, {Star Delegate Example}, {Color
         Editor Factory Example}
*/


/*!
    Constructs an item delegate with the given \a parent.
*/
QStyledItemDelegate::QStyledItemDelegate(QObject *parent)
    : QAbstractItemDelegate(*new QStyledItemDelegatePrivate(), parent)
{
}

/*!
    Destroys the item delegate.
*/
QStyledItemDelegate::~QStyledItemDelegate()
{
}

/*!
    This function returns the string that the delegate will use to display the
    Qt::DisplayRole of the model in \a locale. \a value is the value of the Qt::DisplayRole
    provided by the model.

    The default implementation uses the QLocale::toString to convert \a value into
    a QString.

    This function is not called for empty model indices, i.e., indices for which
    the model returns an invalid QVariant.

    \sa QAbstractItemModel::data()
*/
QString QStyledItemDelegate::displayText(const QVariant &value, const QLocale& locale) const
{
    QString text;
    switch (value.userType()) {
    case QMetaType::Float:
    case QVariant::Double:
        text = locale.toString(value.toReal());
        break;
    case QVariant::Int:
    case QVariant::LongLong:
        text = locale.toString(value.toLongLong());
        break;
    case QVariant::UInt:
    case QVariant::ULongLong:
        text = locale.toString(value.toULongLong());
        break;
    case QVariant::Date:
        text = locale.toString(value.toDate(), QLocale::ShortFormat);
        break;
    case QVariant::Time:
        text = locale.toString(value.toTime(), QLocale::ShortFormat);
        break;
    case QVariant::DateTime:
        text = locale.toString(value.toDateTime().date(), QLocale::ShortFormat);
        text += QLatin1Char(' ');
        text += locale.toString(value.toDateTime().time(), QLocale::ShortFormat);
        break;
    default:
        // convert new lines into line separators
        text = value.toString();
        for (int i = 0; i < text.count(); ++i) {
            if (text.at(i) == QLatin1Char('\n'))
                text[i] = QChar::LineSeparator;
        }
        break;
    }
    return text;
}

/*!
    Initialize \a option with the values using the index \a index. This method
    is useful for subclasses when they need a QStyleOptionViewItem, but don't want
    to fill in all the information themselves. This function will check the version
    of the QStyleOptionViewItem and fill in the additional values for a
    QStyleOptionViewItemV2, QStyleOptionViewItemV3 and QStyleOptionViewItemV4.

    \sa QStyleOption::initFrom()
*/
void QStyledItemDelegate::initStyleOption(QStyleOptionViewItem *option,
                                         const QModelIndex &index) const
{
    QVariant value = index.data(Qt::FontRole);
    if (value.isValid() && !value.isNull()) {
        option->font = qvariant_cast<QFont>(value).resolve(option->font);
        option->fontMetrics = QFontMetrics(option->font);
    }

    value = index.data(Qt::TextAlignmentRole);
    if (value.isValid() && !value.isNull())
        option->displayAlignment = Qt::Alignment(value.toInt());

    value = index.data(Qt::ForegroundRole);
    if (value.canConvert<QBrush>())
        option->palette.setBrush(QPalette::Text, qvariant_cast<QBrush>(value));

    if (QStyleOptionViewItemV4 *v4 = qstyleoption_cast<QStyleOptionViewItemV4 *>(option)) {
        v4->index = index;
        QVariant value = index.data(Qt::CheckStateRole);
        if (value.isValid() && !value.isNull()) {
            v4->features |= QStyleOptionViewItemV2::HasCheckIndicator;
            v4->checkState = static_cast<Qt::CheckState>(value.toInt());
        }

        value = index.data(Qt::DecorationRole);
        if (value.isValid() && !value.isNull()) {
            v4->features |= QStyleOptionViewItemV2::HasDecoration;
            switch (value.type()) {
            case QVariant::Icon: {
                v4->icon = qvariant_cast<QIcon>(value);
                QIcon::Mode mode;
                if (!(option->state & QStyle::State_Enabled))
                    mode = QIcon::Disabled;
                else if (option->state & QStyle::State_Selected)
                    mode = QIcon::Selected;
                else
                    mode = QIcon::Normal;
                QIcon::State state = option->state & QStyle::State_Open ? QIcon::On : QIcon::Off;
                v4->decorationSize = v4->icon.actualSize(option->decorationSize, mode, state);
                break;
            }
            case QVariant::Color: {
                QPixmap pixmap(option->decorationSize);
                pixmap.fill(qvariant_cast<QColor>(value));
                v4->icon = QIcon(pixmap);
                break;
            }
            case QVariant::Image: {
                QImage image = qvariant_cast<QImage>(value);
                v4->icon = QIcon(QPixmap::fromImage(image));
                v4->decorationSize = image.size();
                break;
            }
            case QVariant::Pixmap: {
                QPixmap pixmap = qvariant_cast<QPixmap>(value);
                v4->icon = QIcon(pixmap);
                v4->decorationSize = pixmap.size();
                break;
            }
            default:
                break;
            }
        }

        value = index.data(Qt::DisplayRole);
        if (value.isValid() && !value.isNull()) {
            v4->features |= QStyleOptionViewItemV2::HasDisplay;
            v4->text = displayText(value, v4->locale);
        }

        v4->backgroundBrush = qvariant_cast<QBrush>(index.data(Qt::BackgroundRole));
    }
}

/*!
    Renders the delegate using the given \a painter and style \a option for
    the item specified by \a index.

    This function paints the item using the view's QStyle.

    When reimplementing paint in a subclass. Use the initStyleOption()
    to set up the \a option in the same way as the
    QStyledItemDelegate; the option will always be an instance of
    QStyleOptionViewItemV4. Please see its class description for
    information on its contents.

    Whenever possible, use the \a option while painting.
    Especially its \l{QStyleOption::}{rect} variable to decide
    where to draw and its \l{QStyleOption::}{state} to determine
    if it is enabled or selected.

    After painting, you should ensure that the painter is returned to
    its the state it was supplied in when this function was called.
    For example, it may be useful to call QPainter::save() before
    painting and QPainter::restore() afterwards.

    \sa QItemDelegate::paint(), QStyle::drawControl(), QStyle::CE_ItemViewItem
*/
void QStyledItemDelegate::paint(QPainter *painter,
        const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_ASSERT(index.isValid());

    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);

    const QWidget *widget = QStyledItemDelegatePrivate::widget(option);
    QStyle *style = widget ? widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
}

/*!
    Returns the size needed by the delegate to display the item
    specified by \a index, taking into account the style information
    provided by \a option.

    This function uses the view's QStyle to determine the size of the
    item.

    \sa QStyle::sizeFromContents(), QStyle::CT_ItemViewItem
*/
QSize QStyledItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    QVariant value = index.data(Qt::SizeHintRole);
    if (value.isValid())
        return qvariant_cast<QSize>(value);

    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);
    const QWidget *widget = QStyledItemDelegatePrivate::widget(option);
    QStyle *style = widget ? widget->style() : QApplication::style();
    return style->sizeFromContents(QStyle::CT_ItemViewItem, &opt, QSize(), widget);
}

/*!
    Returns the widget used to edit the item specified by \a index
    for editing. The \a parent widget and style \a option are used to
    control how the editor widget appears.

    \sa QAbstractItemDelegate::createEditor()
*/
QWidget *QStyledItemDelegate::createEditor(QWidget *parent,
                                     const QStyleOptionViewItem &,
                                     const QModelIndex &index) const
{
    Q_D(const QStyledItemDelegate);
    if (!index.isValid())
        return 0;
    QVariant::Type t = static_cast<QVariant::Type>(index.data(Qt::EditRole).userType());
    return d->editorFactory()->createEditor(t, parent);
}

/*!
    Sets the data to be displayed and edited by the \a editor from the
    data model item specified by the model \a index.

    The default implementation stores the data in the \a editor
    widget's \l {Qt's Property System} {user property}.

    \sa QMetaProperty::isUser()
*/
void QStyledItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
#ifdef QT_NO_PROPERTIES
    Q_UNUSED(editor);
    Q_UNUSED(index);
#else
    Q_D(const QStyledItemDelegate);
    QVariant v = index.data(Qt::EditRole);
    QByteArray n = editor->metaObject()->userProperty().name();

    // ### Qt 5: remove
    // A work-around for missing "USER true" in qdatetimeedit.h for
    // QTimeEdit's time property and QDateEdit's date property.
    // It only triggers if the default user property "dateTime" is
    // reported for QTimeEdit and QDateEdit.
    if (n == "dateTime") {
        if (editor->inherits("QTimeEdit"))
            n = "time";
        else if (editor->inherits("QDateEdit"))
            n = "date";
    }

    // ### Qt 5: give QComboBox a USER property
    if (n.isEmpty() && editor->inherits("QComboBox"))
        n = d->editorFactory()->valuePropertyName(static_cast<QVariant::Type>(v.userType()));
    if (!n.isEmpty()) {
        if (!v.isValid())
            v = QVariant(editor->property(n).userType(), (const void *)0);
        editor->setProperty(n, v);
    }
#endif
}

/*!
    Gets data from the \a editor widget and stores it in the specified
    \a model at the item \a index.

    The default implementation gets the value to be stored in the data
    model from the \a editor widget's \l {Qt's Property System} {user
    property}.

    \sa QMetaProperty::isUser()
*/
void QStyledItemDelegate::setModelData(QWidget *editor,
                                 QAbstractItemModel *model,
                                 const QModelIndex &index) const
{
#ifdef QT_NO_PROPERTIES
    Q_UNUSED(model);
    Q_UNUSED(editor);
    Q_UNUSED(index);
#else
    Q_D(const QStyledItemDelegate);
    Q_ASSERT(model);
    Q_ASSERT(editor);
    QByteArray n = editor->metaObject()->userProperty().name();
    if (n.isEmpty())
        n = d->editorFactory()->valuePropertyName(
            static_cast<QVariant::Type>(model->data(index, Qt::EditRole).userType()));
    if (!n.isEmpty())
        model->setData(index, editor->property(n), Qt::EditRole);
#endif
}

/*!
    Updates the \a editor for the item specified by \a index
    according to the style \a option given.
*/
void QStyledItemDelegate::updateEditorGeometry(QWidget *editor,
                                         const QStyleOptionViewItem &option,
                                         const QModelIndex &index) const
{
    if (!editor)
        return;
    Q_ASSERT(index.isValid());
    const QWidget *widget = QStyledItemDelegatePrivate::widget(option);

    QStyleOptionViewItemV4 opt = option;
    initStyleOption(&opt, index);
    // let the editor take up all available space 
    //if the editor is not a QLineEdit
    //or it is in a QTableView 
#if !defined(QT_NO_TABLEVIEW) && !defined(QT_NO_LINEEDIT)
    if (qobject_cast<QExpandingLineEdit*>(editor) && !qobject_cast<const QTableView*>(widget))
        opt.showDecorationSelected = editor->style()->styleHint(QStyle::SH_ItemView_ShowDecorationSelected, 0, editor);
    else
#endif
        opt.showDecorationSelected = true;

    QStyle *style = widget ? widget->style() : QApplication::style();
    QRect geom = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, widget);
    if ( editor->layoutDirection() == Qt::RightToLeft) {
        const int delta = qSmartMinSize(editor).width() - geom.width();
        if (delta > 0) {
            //we need to widen the geometry
            geom.adjust(-delta, 0, 0, 0);
        }
    }

    editor->setGeometry(geom);
}

/*!
  Returns the editor factory used by the item delegate.
  If no editor factory is set, the function will return null.

  \sa setItemEditorFactory()
*/
QItemEditorFactory *QStyledItemDelegate::itemEditorFactory() const
{
    Q_D(const QStyledItemDelegate);
    return d->factory;
}

/*!
  Sets the editor factory to be used by the item delegate to be the \a factory
  specified. If no editor factory is set, the item delegate will use the
  default editor factory.

  \sa itemEditorFactory()
*/
void QStyledItemDelegate::setItemEditorFactory(QItemEditorFactory *factory)
{
    Q_D(QStyledItemDelegate);
    d->factory = factory;
}


/*!
    \fn bool QStyledItemDelegate::eventFilter(QObject *editor, QEvent *event)

    Returns true if the given \a editor is a valid QWidget and the
    given \a event is handled; otherwise returns false. The following
    key press events are handled by default:

    \list
        \o \gui Tab
        \o \gui Backtab
        \o \gui Enter
        \o \gui Return
        \o \gui Esc
    \endlist

    In the case of \gui Tab, \gui Backtab, \gui Enter and \gui Return
    key press events, the \a editor's data is comitted to the model
    and the editor is closed. If the \a event is a \gui Tab key press
    the view will open an editor on the next item in the
    view. Likewise, if the \a event is a \gui Backtab key press the
    view will open an editor on the \e previous item in the view.

    If the event is a \gui Esc key press event, the \a editor is
    closed \e without committing its data.

    \sa commitData(), closeEditor()
*/
bool QStyledItemDelegate::eventFilter(QObject *object, QEvent *event)
{
    QWidget *editor = qobject_cast<QWidget*>(object);
    if (!editor)
        return false;
    if (event->type() == QEvent::KeyPress) {
        switch (static_cast<QKeyEvent *>(event)->key()) {
        case Qt::Key_Tab:
            emit commitData(editor);
            emit closeEditor(editor, QAbstractItemDelegate::EditNextItem);
            return true;
        case Qt::Key_Backtab:
            emit commitData(editor);
            emit closeEditor(editor, QAbstractItemDelegate::EditPreviousItem);
            return true;
        case Qt::Key_Enter:
        case Qt::Key_Return:
#ifndef QT_NO_TEXTEDIT
            if (qobject_cast<QTextEdit *>(editor) || qobject_cast<QPlainTextEdit *>(editor))
                return false; // don't filter enter key events for QTextEdit
            // We want the editor to be able to process the key press
            // before committing the data (e.g. so it can do
            // validation/fixup of the input).
#endif // QT_NO_TEXTEDIT
#ifndef QT_NO_LINEEDIT
            if (QLineEdit *e = qobject_cast<QLineEdit*>(editor))
                if (!e->hasAcceptableInput())
                    return false;
#endif // QT_NO_LINEEDIT
            QMetaObject::invokeMethod(this, "_q_commitDataAndCloseEditor",
                                      Qt::QueuedConnection, Q_ARG(QWidget*, editor));
            return false;
        case Qt::Key_Escape:
            // don't commit data
            emit closeEditor(editor, QAbstractItemDelegate::RevertModelCache);
            break;
        default:
            return false;
        }
        if (editor->parentWidget())
            editor->parentWidget()->setFocus();
        return true;
    } else if (event->type() == QEvent::FocusOut || (event->type() == QEvent::Hide && editor->isWindow())) {
        //the Hide event will take care of he editors that are in fact complete dialogs
        if (!editor->isActiveWindow() || (QApplication::focusWidget() != editor)) {
            QWidget *w = QApplication::focusWidget();
            while (w) { // don't worry about focus changes internally in the editor
                if (w == editor)
                    return false;
                w = w->parentWidget();
            }
#ifndef QT_NO_DRAGANDDROP
            // The window may lose focus during an drag operation.
            // i.e when dragging involves the taskbar on Windows.
            if (QDragManager::self() && QDragManager::self()->object != 0)
                return false;
#endif

            emit commitData(editor);
            emit closeEditor(editor, NoHint);
        }
    } else if (event->type() == QEvent::ShortcutOverride) {
        if (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape) {
            event->accept();
            return true;
        }
    }
    return false;
}

/*!
  \reimp
*/
bool QStyledItemDelegate::editorEvent(QEvent *event,
                                QAbstractItemModel *model,
                                const QStyleOptionViewItem &option,
                                const QModelIndex &index)
{
    Q_ASSERT(event);
    Q_ASSERT(model);

    // make sure that the item is checkable
    Qt::ItemFlags flags = model->flags(index);
    if (!(flags & Qt::ItemIsUserCheckable) || !(option.state & QStyle::State_Enabled)
        || !(flags & Qt::ItemIsEnabled))
        return false;

    // make sure that we have a check state
    QVariant value = index.data(Qt::CheckStateRole);
    if (!value.isValid())
        return false;

    const QWidget *widget = QStyledItemDelegatePrivate::widget(option);
    QStyle *style = widget ? widget->style() : QApplication::style();

    // make sure that we have the right event type
    if ((event->type() == QEvent::MouseButtonRelease)
        || (event->type() == QEvent::MouseButtonDblClick)
        || (event->type() == QEvent::MouseButtonPress)) {
        QStyleOptionViewItemV4 viewOpt(option);
        initStyleOption(&viewOpt, index);
        QRect checkRect = style->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &viewOpt, widget);
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        if (me->button() != Qt::LeftButton || !checkRect.contains(me->pos()))
            return false;

        if ((event->type() == QEvent::MouseButtonPress)
            || (event->type() == QEvent::MouseButtonDblClick))
            return true;

    } else if (event->type() == QEvent::KeyPress) {
        if (static_cast<QKeyEvent*>(event)->key() != Qt::Key_Space
         && static_cast<QKeyEvent*>(event)->key() != Qt::Key_Select)
            return false;
    } else {
        return false;
    }

    Qt::CheckState state = (static_cast<Qt::CheckState>(value.toInt()) == Qt::Checked
                            ? Qt::Unchecked : Qt::Checked);
    return model->setData(index, state, Qt::CheckStateRole);
}

QT_END_NAMESPACE

#include "moc_qstyleditemdelegate.cpp"

#endif // QT_NO_ITEMVIEWS
