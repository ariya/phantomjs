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

#ifndef RANGECONTROLS_H
#define RANGECONTROLS_H

#include <QtWidgets/qaccessiblewidget.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_ACCESSIBILITY

class QAbstractSpinBox;
class QAbstractSlider;
class QScrollBar;
class QSlider;
class QSpinBox;
class QDoubleSpinBox;
class QDial;
class QAccessibleLineEdit;

#ifndef QT_NO_SPINBOX
class QAccessibleAbstractSpinBox:
        public QAccessibleWidget,
        public QAccessibleValueInterface,
        public QAccessibleTextInterface,
        public QAccessibleEditableTextInterface
{
public:
    explicit QAccessibleAbstractSpinBox(QWidget *w);
    virtual ~QAccessibleAbstractSpinBox();

    QString text(QAccessible::Text t) const Q_DECL_OVERRIDE;
    void *interface_cast(QAccessible::InterfaceType t) Q_DECL_OVERRIDE;

    // QAccessibleValueInterface
    QVariant currentValue() const Q_DECL_OVERRIDE;
    void setCurrentValue(const QVariant &value) Q_DECL_OVERRIDE;
    QVariant maximumValue() const Q_DECL_OVERRIDE;
    QVariant minimumValue() const Q_DECL_OVERRIDE;
    QVariant minimumStepSize() const Q_DECL_OVERRIDE;

    // QAccessibleTextInterface
    void addSelection(int startOffset, int endOffset) Q_DECL_OVERRIDE;
    QString attributes(int offset, int *startOffset, int *endOffset) const Q_DECL_OVERRIDE;
    int cursorPosition() const Q_DECL_OVERRIDE;
    QRect characterRect(int offset) const Q_DECL_OVERRIDE;
    int selectionCount() const Q_DECL_OVERRIDE;
    int offsetAtPoint(const QPoint &point) const Q_DECL_OVERRIDE;
    void selection(int selectionIndex, int *startOffset, int *endOffset) const Q_DECL_OVERRIDE;
    QString text(int startOffset, int endOffset) const Q_DECL_OVERRIDE;
    QString textBeforeOffset (int offset, QAccessible::TextBoundaryType boundaryType,
            int *endOffset, int *startOffset) const Q_DECL_OVERRIDE;
    QString textAfterOffset(int offset, QAccessible::TextBoundaryType boundaryType,
            int *startOffset, int *endOffset) const Q_DECL_OVERRIDE;
    QString textAtOffset(int offset, QAccessible::TextBoundaryType boundaryType,
            int *startOffset, int *endOffset) const Q_DECL_OVERRIDE;
    void removeSelection(int selectionIndex) Q_DECL_OVERRIDE;
    void setCursorPosition(int position) Q_DECL_OVERRIDE;
    void setSelection(int selectionIndex, int startOffset, int endOffset) Q_DECL_OVERRIDE;
    int characterCount() const Q_DECL_OVERRIDE;
    void scrollToSubstring(int startIndex, int endIndex) Q_DECL_OVERRIDE;

    // QAccessibleEditableTextInterface
    void deleteText(int startOffset, int endOffset) Q_DECL_OVERRIDE;
    void insertText(int offset, const QString &text) Q_DECL_OVERRIDE;
    void replaceText(int startOffset, int endOffset, const QString &text) Q_DECL_OVERRIDE;

protected:
    QAbstractSpinBox *abstractSpinBox() const;
    QAccessibleInterface *lineEditIface() const;
private:
    mutable QAccessibleLineEdit *lineEdit;
};

class QAccessibleSpinBox : public QAccessibleAbstractSpinBox
{
public:
    explicit QAccessibleSpinBox(QWidget *w);

protected:
    QSpinBox *spinBox() const;
};

class QAccessibleDoubleSpinBox : public QAccessibleAbstractSpinBox
{
public:
    explicit QAccessibleDoubleSpinBox(QWidget *widget);

    QString text(QAccessible::Text t) const Q_DECL_OVERRIDE;

    using QAccessibleAbstractSpinBox::text;
protected:
    QDoubleSpinBox *doubleSpinBox() const;
};
#endif // QT_NO_SPINBOX

class QAccessibleAbstractSlider: public QAccessibleWidget, public QAccessibleValueInterface
{
public:
    explicit QAccessibleAbstractSlider(QWidget *w, QAccessible::Role r = QAccessible::Slider);
    void *interface_cast(QAccessible::InterfaceType t) Q_DECL_OVERRIDE;

    // QAccessibleValueInterface
    QVariant currentValue() const Q_DECL_OVERRIDE;
    void setCurrentValue(const QVariant &value) Q_DECL_OVERRIDE;
    QVariant maximumValue() const Q_DECL_OVERRIDE;
    QVariant minimumValue() const Q_DECL_OVERRIDE;
    QVariant minimumStepSize() const Q_DECL_OVERRIDE;

protected:
    QAbstractSlider *abstractSlider() const;
};

#ifndef QT_NO_SCROLLBAR
class QAccessibleScrollBar : public QAccessibleAbstractSlider
{
public:
    explicit QAccessibleScrollBar(QWidget *w);
    QString text(QAccessible::Text t) const Q_DECL_OVERRIDE;

protected:
    QScrollBar *scrollBar() const;
};
#endif // QT_NO_SCROLLBAR

#ifndef QT_NO_SLIDER
class QAccessibleSlider : public QAccessibleAbstractSlider
{
public:
    explicit QAccessibleSlider(QWidget *w);
    QString text(QAccessible::Text t) const Q_DECL_OVERRIDE;

protected:
    QSlider *slider() const;
};
#endif // QT_NO_SLIDER

#ifndef QT_NO_DIAL
class QAccessibleDial : public QAccessibleAbstractSlider
{
public:
    explicit QAccessibleDial(QWidget *w);

    QString text(QAccessible::Text textType) const Q_DECL_OVERRIDE;

protected:
    QDial *dial() const;
};
#endif // QT_NO_DIAL

#endif // QT_NO_ACCESSIBILITY

QT_END_NAMESPACE

#endif // RANGECONTROLS_H
