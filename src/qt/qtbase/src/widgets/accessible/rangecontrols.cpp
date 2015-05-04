/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "rangecontrols.h"

#include <qslider.h>
#include <qdial.h>
#include <qspinbox.h>
#include <qscrollbar.h>
#include <qstyle.h>
#include <qstyleoption.h>
#include <qdebug.h>
#include <qglobal.h>
#include <QDoubleSpinBox>
#include <QDial>
#include <QtWidgets/qlineedit.h>
#include <qmath.h>
#include <private/qmath_p.h>

#include "simplewidgets.h" // let spinbox use line edit's interface

QT_BEGIN_NAMESPACE

#ifndef QT_NO_ACCESSIBILITY

#ifndef QT_NO_SPINBOX
QAccessibleAbstractSpinBox::QAccessibleAbstractSpinBox(QWidget *w)
: QAccessibleWidget(w, QAccessible::SpinBox), lineEdit(Q_NULLPTR)
{
    Q_ASSERT(abstractSpinBox());
}

QAccessibleAbstractSpinBox::~QAccessibleAbstractSpinBox()
{
    delete lineEdit;
}

/*!
    Returns the underlying QAbstractSpinBox.
*/
QAbstractSpinBox *QAccessibleAbstractSpinBox::abstractSpinBox() const
{
    return qobject_cast<QAbstractSpinBox*>(object());
}

QAccessibleInterface *QAccessibleAbstractSpinBox::lineEditIface() const
{
    // QAccessibleLineEdit is only used to forward the text functions
    if (!lineEdit)
        lineEdit = new QAccessibleLineEdit(abstractSpinBox()->lineEdit());
    return lineEdit;
}

QString QAccessibleAbstractSpinBox::text(QAccessible::Text t) const
{
    if (t == QAccessible::Value)
        return abstractSpinBox()->text();
    return QAccessibleWidget::text(t);
}

void *QAccessibleAbstractSpinBox::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::ValueInterface)
        return static_cast<QAccessibleValueInterface*>(this);
    if (t == QAccessible::TextInterface)
        return static_cast<QAccessibleTextInterface*>(this);
    if (t == QAccessible::EditableTextInterface)
        return static_cast<QAccessibleEditableTextInterface*>(this);
    return QAccessibleWidget::interface_cast(t);
}

QVariant QAccessibleAbstractSpinBox::currentValue() const
{
    return abstractSpinBox()->property("value");
}

void QAccessibleAbstractSpinBox::setCurrentValue(const QVariant &value)
{
    abstractSpinBox()->setProperty("value", value);
}

QVariant QAccessibleAbstractSpinBox::maximumValue() const
{
    return abstractSpinBox()->property("maximum");
}

QVariant QAccessibleAbstractSpinBox::minimumValue() const
{
    return abstractSpinBox()->property("minimum");
}

QVariant QAccessibleAbstractSpinBox::minimumStepSize() const
{
    return abstractSpinBox()->property("stepSize");
}

void QAccessibleAbstractSpinBox::addSelection(int startOffset, int endOffset)
{
    lineEditIface()->textInterface()->addSelection(startOffset, endOffset);
}

QString QAccessibleAbstractSpinBox::attributes(int offset, int *startOffset, int *endOffset) const
{
    return lineEditIface()->textInterface()->attributes(offset, startOffset, endOffset);
}

int QAccessibleAbstractSpinBox::cursorPosition() const
{
    return lineEditIface()->textInterface()->cursorPosition();
}

QRect QAccessibleAbstractSpinBox::characterRect(int offset) const
{
    return lineEditIface()->textInterface()->characterRect(offset);
}

int QAccessibleAbstractSpinBox::selectionCount() const
{
    return lineEditIface()->textInterface()->selectionCount();
}

int QAccessibleAbstractSpinBox::offsetAtPoint(const QPoint &point) const
{
    return lineEditIface()->textInterface()->offsetAtPoint(point);
}

void QAccessibleAbstractSpinBox::selection(int selectionIndex, int *startOffset, int *endOffset) const
{
    lineEditIface()->textInterface()->selection(selectionIndex, startOffset, endOffset);
}

QString QAccessibleAbstractSpinBox::text(int startOffset, int endOffset) const
{
    return lineEditIface()->textInterface()->text(startOffset, endOffset);
}

QString QAccessibleAbstractSpinBox::textBeforeOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const
{
    return lineEditIface()->textInterface()->textBeforeOffset(offset, boundaryType, startOffset, endOffset);
}

QString QAccessibleAbstractSpinBox::textAfterOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const
{
    return lineEditIface()->textInterface()->textAfterOffset(offset, boundaryType, startOffset, endOffset);
}

QString QAccessibleAbstractSpinBox::textAtOffset(int offset, QAccessible::TextBoundaryType boundaryType, int *startOffset, int *endOffset) const
{
    return lineEditIface()->textInterface()->textAtOffset(offset, boundaryType, startOffset, endOffset);
}

void QAccessibleAbstractSpinBox::removeSelection(int selectionIndex)
{
    lineEditIface()->textInterface()->removeSelection(selectionIndex);
}

void QAccessibleAbstractSpinBox::setCursorPosition(int position)
{
    lineEditIface()->textInterface()->setCursorPosition(position);
}

void QAccessibleAbstractSpinBox::setSelection(int selectionIndex, int startOffset, int endOffset)
{
    lineEditIface()->textInterface()->setSelection(selectionIndex, startOffset, endOffset);
}

int QAccessibleAbstractSpinBox::characterCount() const
{
    return lineEditIface()->textInterface()->characterCount();
}

void QAccessibleAbstractSpinBox::scrollToSubstring(int startIndex, int endIndex)
{
    lineEditIface()->textInterface()->scrollToSubstring(startIndex, endIndex);
}

void QAccessibleAbstractSpinBox::deleteText(int startOffset, int endOffset)
{
    lineEditIface()->editableTextInterface()->deleteText(startOffset, endOffset);
}

void QAccessibleAbstractSpinBox::insertText(int offset, const QString &text)
{
    lineEditIface()->editableTextInterface()->insertText(offset, text);
}

void QAccessibleAbstractSpinBox::replaceText(int startOffset, int endOffset, const QString &text)
{
    lineEditIface()->editableTextInterface()->replaceText(startOffset, endOffset, text);
}


/*!
  \class QAccessibleSpinBox
  \brief The QAccessibleSpinBox class implements the QAccessibleInterface for spinbox widgets.
  \internal

  \ingroup accessibility
*/

/*!
  Constructs a QAccessibleSpinWidget object for \a w.
*/
QAccessibleSpinBox::QAccessibleSpinBox(QWidget *w)
: QAccessibleAbstractSpinBox(w)
{
    Q_ASSERT(spinBox());
    addControllingSignal(QLatin1String("valueChanged(int)"));
    addControllingSignal(QLatin1String("valueChanged(QString)"));
}

/*!
    Returns the underlying QSpinBox.
*/
QSpinBox *QAccessibleSpinBox::spinBox() const
{
    return qobject_cast<QSpinBox*>(object());
}


// ================================== QAccessibleDoubleSpinBox ==================================
QAccessibleDoubleSpinBox::QAccessibleDoubleSpinBox(QWidget *widget)
    : QAccessibleAbstractSpinBox(widget)
{
    Q_ASSERT(qobject_cast<QDoubleSpinBox *>(widget));
    addControllingSignal(QLatin1String("valueChanged(double)"));
    addControllingSignal(QLatin1String("valueChanged(QString)"));
}

/*!
    Returns the underlying QDoubleSpinBox.
*/
QDoubleSpinBox *QAccessibleDoubleSpinBox::doubleSpinBox() const
{
    return static_cast<QDoubleSpinBox*>(object());
}

QString QAccessibleDoubleSpinBox::text(QAccessible::Text textType) const
{
    if (textType == QAccessible::Value)
        return doubleSpinBox()->textFromValue(doubleSpinBox()->value());
    return QAccessibleWidget::text(textType);
}

#endif // QT_NO_SPINBOX

#ifndef QT_NO_SCROLLBAR
/*!
  \class QAccessibleScrollBar
  \brief The QAccessibleScrollBar class implements the QAccessibleInterface for scroll bars.
  \internal

  \ingroup accessibility
*/

/*!
  Constructs a QAccessibleScrollBar object for \a w.
  \a name is propagated to the QAccessibleWidget constructor.
*/
QAccessibleScrollBar::QAccessibleScrollBar(QWidget *w)
: QAccessibleAbstractSlider(w, QAccessible::ScrollBar)
{
    Q_ASSERT(scrollBar());
    addControllingSignal(QLatin1String("valueChanged(int)"));
}

/*! Returns the scroll bar. */
QScrollBar *QAccessibleScrollBar::scrollBar() const
{
    return qobject_cast<QScrollBar*>(object());
}

QString QAccessibleScrollBar::text(QAccessible::Text t) const
{
    if (t == QAccessible::Value)
        return QString::number(scrollBar()->value());
    return QAccessibleAbstractSlider::text(t);
}

#endif // QT_NO_SCROLLBAR

#ifndef QT_NO_SLIDER
/*!
  \class QAccessibleSlider
  \brief The QAccessibleSlider class implements the QAccessibleInterface for sliders.
  \internal

  \ingroup accessibility
*/

/*!
  Constructs a QAccessibleScrollBar object for \a w.
  \a name is propagated to the QAccessibleWidget constructor.
*/
QAccessibleSlider::QAccessibleSlider(QWidget *w)
: QAccessibleAbstractSlider(w)
{
    Q_ASSERT(slider());
    addControllingSignal(QLatin1String("valueChanged(int)"));
}

/*! Returns the slider. */
QSlider *QAccessibleSlider::slider() const
{
    return qobject_cast<QSlider*>(object());
}

QString QAccessibleSlider::text(QAccessible::Text t) const
{
    if (t == QAccessible::Value)
        return QString::number(slider()->value());

    return QAccessibleAbstractSlider::text(t);
}

QAccessibleAbstractSlider::QAccessibleAbstractSlider(QWidget *w, QAccessible::Role r)
    : QAccessibleWidget(w, r)
{
    Q_ASSERT(qobject_cast<QAbstractSlider *>(w));
}

void *QAccessibleAbstractSlider::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::ValueInterface)
        return static_cast<QAccessibleValueInterface*>(this);
    return QAccessibleWidget::interface_cast(t);
}

QVariant QAccessibleAbstractSlider::currentValue() const
{
    return abstractSlider()->value();
}

void QAccessibleAbstractSlider::setCurrentValue(const QVariant &value)
{
    abstractSlider()->setValue(value.toInt());
}

QVariant QAccessibleAbstractSlider::maximumValue() const
{
    return abstractSlider()->maximum();
}

QVariant QAccessibleAbstractSlider::minimumValue() const
{
    return abstractSlider()->minimum();
}

QVariant QAccessibleAbstractSlider::minimumStepSize() const
{
    return abstractSlider()->singleStep();
}

QAbstractSlider *QAccessibleAbstractSlider::abstractSlider() const
{
    return static_cast<QAbstractSlider *>(object());
}

#endif // QT_NO_SLIDER

#ifndef QT_NO_DIAL
// ======================================= QAccessibleDial ======================================
QAccessibleDial::QAccessibleDial(QWidget *widget)
    : QAccessibleAbstractSlider(widget, QAccessible::Dial)
{
    Q_ASSERT(qobject_cast<QDial *>(widget));
    addControllingSignal(QLatin1String("valueChanged(int)"));
}

QString QAccessibleDial::text(QAccessible::Text textType) const
{
    if (textType == QAccessible::Value)
        return QString::number(dial()->value());

    return QAccessibleAbstractSlider::text(textType);
}

QDial *QAccessibleDial::dial() const
{
    return static_cast<QDial*>(object());
}
#endif // QT_NO_DIAL

#endif // QT_NO_ACCESSIBILITY

QT_END_NAMESPACE
