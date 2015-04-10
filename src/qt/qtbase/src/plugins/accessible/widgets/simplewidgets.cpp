/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "simplewidgets.h"

#include <qabstractbutton.h>
#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qprogressbar.h>
#include <qstatusbar.h>
#include <qradiobutton.h>
#include <qtoolbutton.h>
#include <qmenu.h>
#include <qlabel.h>
#include <qgroupbox.h>
#include <qlcdnumber.h>
#include <qlineedit.h>
#include <private/qlineedit_p.h>
#include <qstyle.h>
#include <qstyleoption.h>
#include <qtextdocument.h>
#include <qwindow.h>
#include <private/qwindowcontainer_p.h>
#include <QtCore/qvarlengtharray.h>

#ifdef Q_OS_MAC
#include <qfocusframe.h>
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_NO_ACCESSIBILITY

extern QList<QWidget*> childWidgets(const QWidget *widget, bool includeTopLevel = false);

QString Q_GUI_EXPORT qt_accStripAmp(const QString &text);
QString Q_GUI_EXPORT qt_accHotKey(const QString &text);

/*!
  \class QAccessibleButton
  \brief The QAccessibleButton class implements the QAccessibleInterface for button type widgets.
  \internal

  \ingroup accessibility
*/

/*!
  Creates a QAccessibleButton object for \a w.
  \a role is propagated to the QAccessibleWidget constructor.
*/
QAccessibleButton::QAccessibleButton(QWidget *w, QAccessible::Role role)
: QAccessibleWidget(w, role)
{
    Q_ASSERT(button());
    if (button()->isCheckable())
        addControllingSignal(QLatin1String("toggled(bool)"));
    else
        addControllingSignal(QLatin1String("clicked()"));
}

/*! Returns the button. */
QAbstractButton *QAccessibleButton::button() const
{
    return qobject_cast<QAbstractButton*>(object());
}

/*! \reimp */
QString QAccessibleButton::text(QAccessible::Text t) const
{
    QString str;
    switch (t) {
    case QAccessible::Accelerator:
    {
#ifndef QT_NO_SHORTCUT
        QPushButton *pb = qobject_cast<QPushButton*>(object());
        if (pb && pb->isDefault())
            str = QKeySequence(Qt::Key_Enter).toString(QKeySequence::NativeText);
#endif
        if (str.isEmpty())
            str = qt_accHotKey(button()->text());
    }
         break;
    case QAccessible::Name:
        str = widget()->accessibleName();
        if (str.isEmpty())
            str = button()->text();
        break;
    default:
        break;
    }
    if (str.isEmpty())
        str = QAccessibleWidget::text(t);
    return qt_accStripAmp(str);
}

QAccessible::State QAccessibleButton::state() const
{
    QAccessible::State state = QAccessibleWidget::state();

    QAbstractButton *b = button();
    QCheckBox *cb = qobject_cast<QCheckBox *>(b);
    if (b->isCheckable())
        state.checkable = true;
    if (b->isChecked())
        state.checked = true;
    else if (cb && cb->checkState() == Qt::PartiallyChecked)
        state.checkStateMixed = true;
    if (b->isDown())
        state.pressed = true;
    QPushButton *pb = qobject_cast<QPushButton*>(b);
    if (pb) {
        if (pb->isDefault())
            state.defaultButton = true;
#ifndef QT_NO_MENU
        if (pb->menu())
            state.hasPopup = true;
#endif
    }

    return state;
}

QStringList QAccessibleButton::actionNames() const
{
    QStringList names;
    if (widget()->isEnabled()) {
        switch (role()) {
        case QAccessible::ButtonMenu:
            names << showMenuAction();
            break;
        case QAccessible::RadioButton:
            names << toggleAction();
            break;
        default:
            if (button()->isCheckable()) {
                names <<  toggleAction();
            } else {
                names << pressAction();
            }
            break;
        }
    }
    names << QAccessibleWidget::actionNames();
    return names;
}

void QAccessibleButton::doAction(const QString &actionName)
{
    if (!widget()->isEnabled())
        return;
    if (actionName == pressAction() ||
        actionName == showMenuAction()) {
#ifndef QT_NO_MENU
        QPushButton *pb = qobject_cast<QPushButton*>(object());
        if (pb && pb->menu())
            pb->showMenu();
        else
#endif
            button()->animateClick();
    } else if (actionName == toggleAction()) {
        button()->toggle();
    } else {
        QAccessibleWidget::doAction(actionName);
    }
}

QStringList QAccessibleButton::keyBindingsForAction(const QString &actionName) const
{
    if (actionName == pressAction()) {
#ifndef QT_NO_SHORTCUT
        return QStringList() << button()->shortcut().toString();
#endif
    }
    return QStringList();
}


#ifndef QT_NO_TOOLBUTTON
/*!
  \class QAccessibleToolButton
  \brief The QAccessibleToolButton class implements the QAccessibleInterface for tool buttons.
  \internal

  \ingroup accessibility
*/

/*!
  Creates a QAccessibleToolButton object for \a w.
  \a role is propagated to the QAccessibleWidget constructor.
*/
QAccessibleToolButton::QAccessibleToolButton(QWidget *w, QAccessible::Role role)
: QAccessibleButton(w, role)
{
    Q_ASSERT(toolButton());
}

/*! Returns the button. */
QToolButton *QAccessibleToolButton::toolButton() const
{
    return qobject_cast<QToolButton*>(object());
}

/*!
    Returns \c true if this tool button is a split button.
*/
bool QAccessibleToolButton::isSplitButton() const
{
#ifndef QT_NO_MENU
    return toolButton()->menu() && toolButton()->popupMode() == QToolButton::MenuButtonPopup;
#else
    return false;
#endif
}

QAccessible::State QAccessibleToolButton::state() const
{
    QAccessible::State st = QAccessibleButton::state();
    if (toolButton()->autoRaise())
        st.hotTracked = true;
#ifndef QT_NO_MENU
    if (toolButton()->menu())
        st.hasPopup = true;
#endif
    return st;
}

int QAccessibleToolButton::childCount() const
{
    return isSplitButton() ? 1 : 0;
}

QAccessibleInterface *QAccessibleToolButton::child(int index) const
{
#ifndef QT_NO_MENU
    if (index == 0 && toolButton()->menu())
    {
        return QAccessible::queryAccessibleInterface(toolButton()->menu());
    }
#endif
    return 0;
}

/*!
    \internal

    Returns the button's text label, depending on the text \a t, and
    the \a child.
*/
QString QAccessibleToolButton::text(QAccessible::Text t) const
{
    QString str;
    switch (t) {
    case QAccessible::Name:
        str = toolButton()->accessibleName();
        if (str.isEmpty())
            str = toolButton()->text();
        break;
    default:
        break;
    }
    if (str.isEmpty())
        str = QAccessibleButton::text(t);
    return qt_accStripAmp(str);
}

/*
   The three different tool button types can have the following actions:
| DelayedPopup    | ShowMenuAction + (PressedAction || CheckedAction) |
| MenuButtonPopup | ShowMenuAction + (PressedAction || CheckedAction) |
| InstantPopup    | ShowMenuAction |
*/
QStringList QAccessibleToolButton::actionNames() const
{
    QStringList names;
    if (widget()->isEnabled()) {
        if (toolButton()->menu())
            names << showMenuAction();
        if (toolButton()->popupMode() != QToolButton::InstantPopup)
            names << QAccessibleButton::actionNames();
    }
    return names;
}

void QAccessibleToolButton::doAction(const QString &actionName)
{
    if (!widget()->isEnabled())
        return;

    if (actionName == pressAction()) {
        button()->click();
    } else if (actionName == showMenuAction()) {
        if (toolButton()->popupMode() != QToolButton::InstantPopup) {
            toolButton()->setDown(true);
#ifndef QT_NO_MENU
            toolButton()->showMenu();
#endif
        }
    } else {
        QAccessibleButton::doAction(actionName);
    }

}

#endif // QT_NO_TOOLBUTTON

/*!
  \class QAccessibleDisplay
  \brief The QAccessibleDisplay class implements the QAccessibleInterface for widgets that display information.
  \internal

  \ingroup accessibility
*/

/*!
  Constructs a QAccessibleDisplay object for \a w.
  \a role is propagated to the QAccessibleWidget constructor.
*/
QAccessibleDisplay::QAccessibleDisplay(QWidget *w, QAccessible::Role role)
: QAccessibleWidget(w, role)
{
}

QAccessible::Role QAccessibleDisplay::role() const
{
    QLabel *l = qobject_cast<QLabel*>(object());
    if (l) {
        if (l->pixmap())
            return QAccessible::Graphic;
#ifndef QT_NO_PICTURE
        if (l->picture())
            return QAccessible::Graphic;
#endif
#ifndef QT_NO_MOVIE
        if (l->movie())
            return QAccessible::Animation;
#endif
#ifndef QT_NO_PROGRESSBAR
    } else if (qobject_cast<QProgressBar*>(object())) {
        return QAccessible::ProgressBar;
#endif
    } else if (qobject_cast<QStatusBar*>(object())) {
        return QAccessible::StatusBar;
    }
    return QAccessibleWidget::role();
}

QString QAccessibleDisplay::text(QAccessible::Text t) const
{
    QString str;
    switch (t) {
    case QAccessible::Name:
        str = widget()->accessibleName();
        if (str.isEmpty()) {
            if (qobject_cast<QLabel*>(object())) {
                QLabel *label = qobject_cast<QLabel*>(object());
                str = label->text();
                if (label->textFormat() == Qt::RichText
                    || (label->textFormat() == Qt::AutoText && Qt::mightBeRichText(str))) {
                    QTextDocument doc;
                    doc.setHtml(str);
                    str = doc.toPlainText();
                }
#ifndef QT_NO_LCDNUMBER
            } else if (qobject_cast<QLCDNumber*>(object())) {
                QLCDNumber *l = qobject_cast<QLCDNumber*>(object());
                if (l->digitCount())
                    str = QString::number(l->value());
                else
                    str = QString::number(l->intValue());
#endif
            } else if (qobject_cast<QStatusBar*>(object())) {
                return qobject_cast<QStatusBar*>(object())->currentMessage();
            }
        }
        break;
    case QAccessible::Value:
#ifndef QT_NO_PROGRESSBAR
        if (qobject_cast<QProgressBar*>(object()))
            str = QString::number(qobject_cast<QProgressBar*>(object())->value());
#endif
        break;
    default:
        break;
    }
    if (str.isEmpty())
        str = QAccessibleWidget::text(t);
    return qt_accStripAmp(str);
}

/*! \reimp */
QVector<QPair<QAccessibleInterface*, QAccessible::Relation> >
QAccessibleDisplay::relations(QAccessible::Relation match /* = QAccessible::AllRelations */) const
{
    QVector<QPair<QAccessibleInterface*, QAccessible::Relation> > rels = QAccessibleWidget::relations(match);
    if (match & QAccessible::Labelled) {
        QVarLengthArray<QObject *, 4> relatedObjects;

#ifndef QT_NO_SHORTCUT
        if (QLabel *label = qobject_cast<QLabel*>(object())) {
            relatedObjects.append(label->buddy());
        }
#endif
        for (int i = 0; i < relatedObjects.count(); ++i) {
            const QAccessible::Relation rel = QAccessible::Labelled;
            QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(relatedObjects.at(i));
            if (iface)
                rels.append(qMakePair(iface, rel));
        }
    }
    return rels;
}

void *QAccessibleDisplay::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::ImageInterface)
        return static_cast<QAccessibleImageInterface*>(this);
    return QAccessibleWidget::interface_cast(t);
}

/*! \internal */
QString QAccessibleDisplay::imageDescription() const
{
#ifndef QT_NO_TOOLTIP
    return widget()->toolTip();
#else
    return QString::null;
#endif
}

/*! \internal */
QSize QAccessibleDisplay::imageSize() const
{
    QLabel *label = qobject_cast<QLabel *>(widget());
    if (!label)
        return QSize();
    const QPixmap *pixmap = label->pixmap();
    if (!pixmap)
        return QSize();
    return pixmap->size();
}

/*! \internal */
QPoint QAccessibleDisplay::imagePosition() const
{
    QLabel *label = qobject_cast<QLabel *>(widget());
    if (!label)
        return QPoint();
    const QPixmap *pixmap = label->pixmap();
    if (!pixmap)
        return QPoint();

    return QPoint(label->mapToGlobal(label->pos()));
}

#ifndef QT_NO_GROUPBOX
QAccessibleGroupBox::QAccessibleGroupBox(QWidget *w)
: QAccessibleWidget(w)
{
}

QGroupBox* QAccessibleGroupBox::groupBox() const
{
    return static_cast<QGroupBox *>(widget());
}

QString QAccessibleGroupBox::text(QAccessible::Text t) const
{
    QString txt = QAccessibleWidget::text(t);

    if (txt.isEmpty()) {
        switch (t) {
        case QAccessible::Name:
            txt = qt_accStripAmp(groupBox()->title());
            break;
        case QAccessible::Description:
            txt = qt_accStripAmp(groupBox()->toolTip());
            break;
        default:
            break;
        }
    }

    return txt;
}

QAccessible::State QAccessibleGroupBox::state() const
{
    QAccessible::State st = QAccessibleWidget::state();
    st.checkable = groupBox()->isCheckable();
    st.checked = groupBox()->isChecked();
    return st;
}

QAccessible::Role QAccessibleGroupBox::role() const
{
    return groupBox()->isCheckable() ? QAccessible::CheckBox : QAccessible::Grouping;
}

QVector<QPair<QAccessibleInterface*, QAccessible::Relation> >
QAccessibleGroupBox::relations(QAccessible::Relation match /* = QAccessible::AllRelations */) const
{
    QVector<QPair<QAccessibleInterface*, QAccessible::Relation> > rels = QAccessibleWidget::relations(match);

    if ((match & QAccessible::Labelled) && (!groupBox()->title().isEmpty())) {
        const QList<QWidget*> kids = childWidgets(widget());
        for (int i = 0; i < kids.count(); ++i) {
            QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(kids.at(i));
            if (iface)
                rels.append(qMakePair(iface, QAccessible::Relation(QAccessible::Labelled)));
        }
    }
    return rels;
}

QStringList QAccessibleGroupBox::actionNames() const
{
    QStringList actions = QAccessibleWidget::actionNames();

    if (groupBox()->isCheckable()) {
        actions.prepend(QAccessibleActionInterface::toggleAction());
    }
    return actions;
}

void QAccessibleGroupBox::doAction(const QString &actionName)
{
    if (actionName == QAccessibleActionInterface::toggleAction())
        groupBox()->setChecked(!groupBox()->isChecked());
}

QStringList QAccessibleGroupBox::keyBindingsForAction(const QString &) const
{
    return QStringList();
}

#endif

#ifndef QT_NO_LINEEDIT
/*!
  \class QAccessibleLineEdit
  \brief The QAccessibleLineEdit class implements the QAccessibleInterface for widgets with editable text
  \internal

  \ingroup accessibility
*/

/*!
  Constructs a QAccessibleLineEdit object for \a w.
  \a name is propagated to the QAccessibleWidget constructor.
*/
QAccessibleLineEdit::QAccessibleLineEdit(QWidget *w, const QString &name)
: QAccessibleWidget(w, QAccessible::EditableText, name)
{
    addControllingSignal(QLatin1String("textChanged(const QString&)"));
    addControllingSignal(QLatin1String("returnPressed()"));
}

/*! Returns the line edit. */
QLineEdit *QAccessibleLineEdit::lineEdit() const
{
    return qobject_cast<QLineEdit*>(object());
}

QString QAccessibleLineEdit::text(QAccessible::Text t) const
{
    QString str;
    switch (t) {
    case QAccessible::Value:
        if (lineEdit()->echoMode() == QLineEdit::Normal)
            str = lineEdit()->text();
        break;
    default:
        break;
    }
    if (str.isEmpty())
        str = QAccessibleWidget::text(t);
    return qt_accStripAmp(str);
}

void QAccessibleLineEdit::setText(QAccessible::Text t, const QString &text)
{
    if (t != QAccessible::Value) {
        QAccessibleWidget::setText(t, text);
        return;
    }

    QString newText = text;
    if (lineEdit()->validator()) {
        int pos = 0;
        if (lineEdit()->validator()->validate(newText, pos) != QValidator::Acceptable)
            return;
    }
    lineEdit()->setText(newText);
}

QAccessible::State QAccessibleLineEdit::state() const
{
    QAccessible::State state = QAccessibleWidget::state();

    QLineEdit *l = lineEdit();
    if (l->isReadOnly())
        state.readOnly = true;
    else
        state.editable = true;

    if (l->echoMode() != QLineEdit::Normal)
        state.passwordEdit = true;
    state.selectable = true;
    if (l->hasSelectedText())
        state.selected = true;

    if (l->contextMenuPolicy() != Qt::NoContextMenu
        && l->contextMenuPolicy() != Qt::PreventContextMenu)
        state.hasPopup = true;

    return state;
}

void *QAccessibleLineEdit::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::TextInterface)
        return static_cast<QAccessibleTextInterface*>(this);
    if (t == QAccessible::EditableTextInterface)
        return static_cast<QAccessibleEditableTextInterface*>(this);
    return QAccessibleWidget::interface_cast(t);
}

void QAccessibleLineEdit::addSelection(int startOffset, int endOffset)
{
    setSelection(0, startOffset, endOffset);
}

QString QAccessibleLineEdit::attributes(int offset, int *startOffset, int *endOffset) const
{
    // QLineEdit doesn't have text attributes
    *startOffset = *endOffset = offset;
    return QString();
}

int QAccessibleLineEdit::cursorPosition() const
{
    return lineEdit()->cursorPosition();
}

QRect QAccessibleLineEdit::characterRect(int offset) const
{
    int x = lineEdit()->d_func()->control->cursorToX(offset);
    int y;
    lineEdit()->getTextMargins(0, &y, 0, 0);
    QFontMetrics fm(lineEdit()->font());
    const QString ch = text(offset, offset + 1);
    if (ch.isEmpty())
        return QRect();
    int w = fm.width(ch);
    int h = fm.height();
    QRect r(x, y, w, h);
    r.moveTo(lineEdit()->mapToGlobal(r.topLeft()));
    return r;
}

int QAccessibleLineEdit::selectionCount() const
{
    return lineEdit()->hasSelectedText() ? 1 : 0;
}

int QAccessibleLineEdit::offsetAtPoint(const QPoint &point) const
{
    QPoint p = lineEdit()->mapFromGlobal(point);

    return lineEdit()->cursorPositionAt(p);
}

void QAccessibleLineEdit::selection(int selectionIndex, int *startOffset, int *endOffset) const
{
    *startOffset = *endOffset = 0;
    if (selectionIndex != 0)
        return;

    *startOffset = lineEdit()->selectionStart();
    *endOffset = *startOffset + lineEdit()->selectedText().count();
}

QString QAccessibleLineEdit::text(int startOffset, int endOffset) const
{
    if (startOffset > endOffset)
        return QString();

    if (lineEdit()->echoMode() != QLineEdit::Normal)
        return QString();

    return lineEdit()->text().mid(startOffset, endOffset - startOffset);
}

QString QAccessibleLineEdit::textBeforeOffset(int offset, QAccessible::TextBoundaryType boundaryType,
        int *startOffset, int *endOffset) const
{
    if (lineEdit()->echoMode() != QLineEdit::Normal) {
        *startOffset = *endOffset = -1;
        return QString();
    }
    return QAccessibleTextInterface::textBeforeOffset(offset, boundaryType, startOffset, endOffset);
}

QString QAccessibleLineEdit::textAfterOffset(int offset, QAccessible::TextBoundaryType boundaryType,
        int *startOffset, int *endOffset) const
{
    if (lineEdit()->echoMode() != QLineEdit::Normal) {
        *startOffset = *endOffset = -1;
        return QString();
    }
    return QAccessibleTextInterface::textAfterOffset(offset, boundaryType, startOffset, endOffset);
}

QString QAccessibleLineEdit::textAtOffset(int offset, QAccessible::TextBoundaryType boundaryType,
        int *startOffset, int *endOffset) const
{
    if (lineEdit()->echoMode() != QLineEdit::Normal) {
        *startOffset = *endOffset = -1;
        return QString();
    }
    return QAccessibleTextInterface::textAtOffset(offset, boundaryType, startOffset, endOffset);
}

void QAccessibleLineEdit::removeSelection(int selectionIndex)
{
    if (selectionIndex != 0)
        return;

    lineEdit()->deselect();
}

void QAccessibleLineEdit::setCursorPosition(int position)
{
    lineEdit()->setCursorPosition(position);
}

void QAccessibleLineEdit::setSelection(int selectionIndex, int startOffset, int endOffset)
{
    if (selectionIndex != 0)
        return;

    lineEdit()->setSelection(startOffset, endOffset - startOffset);
}

int QAccessibleLineEdit::characterCount() const
{
    return lineEdit()->text().count();
}

void QAccessibleLineEdit::scrollToSubstring(int startIndex, int endIndex)
{
    lineEdit()->setCursorPosition(endIndex);
    lineEdit()->setCursorPosition(startIndex);
}

void QAccessibleLineEdit::deleteText(int startOffset, int endOffset)
{
    lineEdit()->setText(lineEdit()->text().remove(startOffset, endOffset - startOffset));
}

void QAccessibleLineEdit::insertText(int offset, const QString &text)
{
    lineEdit()->setText(lineEdit()->text().insert(offset, text));
}

void QAccessibleLineEdit::replaceText(int startOffset, int endOffset, const QString &text)
{
    lineEdit()->setText(lineEdit()->text().replace(startOffset, endOffset - startOffset, text));
}

#endif // QT_NO_LINEEDIT

#ifndef QT_NO_PROGRESSBAR
QAccessibleProgressBar::QAccessibleProgressBar(QWidget *o)
    : QAccessibleDisplay(o)
{
    Q_ASSERT(progressBar());
}

void *QAccessibleProgressBar::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::ValueInterface)
        return static_cast<QAccessibleValueInterface*>(this);
    return QAccessibleDisplay::interface_cast(t);
}

QVariant QAccessibleProgressBar::currentValue() const
{
    return progressBar()->value();
}

QVariant QAccessibleProgressBar::maximumValue() const
{
    return progressBar()->maximum();
}

QVariant QAccessibleProgressBar::minimumValue() const
{
    return progressBar()->minimum();
}

QVariant QAccessibleProgressBar::minimumStepSize() const
{
    // This is arbitrary since any value between min and max is valid.
    // Some screen readers (orca use it to calculate how many digits to display though,
    // so it makes sense to return a "sensible" value. Providing 100 increments seems ok.
    return (progressBar()->maximum() - progressBar()->minimum()) / 100.0;
}

QProgressBar *QAccessibleProgressBar::progressBar() const
{
    return qobject_cast<QProgressBar *>(object());
}
#endif


QAccessibleWindowContainer::QAccessibleWindowContainer(QWidget *w)
    : QAccessibleWidget(w)
{
}

int QAccessibleWindowContainer::childCount() const
{
    if (container()->containedWindow())
        return 1;
    return 0;
}

int QAccessibleWindowContainer::indexOfChild(const QAccessibleInterface *child) const
{
    if (child->object() == container()->containedWindow())
        return 0;
    return -1;
}

QAccessibleInterface *QAccessibleWindowContainer::child(int i) const
{
    if (i == 0)
        return QAccessible::queryAccessibleInterface(container()->containedWindow());
    return 0;
}

QWindowContainer *QAccessibleWindowContainer::container() const
{
    return static_cast<QWindowContainer *>(widget());
}

#endif // QT_NO_ACCESSIBILITY

QT_END_NAMESPACE
