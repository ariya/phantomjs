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

#ifndef SIMPLEWIDGETS_H
#define SIMPLEWIDGETS_H

#include <QtCore/qcoreapplication.h>
#include <QtWidgets/qaccessiblewidget.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_ACCESSIBILITY

class QAbstractButton;
class QLineEdit;
class QToolButton;
class QGroupBox;
class QProgressBar;

class QAccessibleButton : public QAccessibleWidget
{
    Q_DECLARE_TR_FUNCTIONS(QAccessibleButton)
public:
    QAccessibleButton(QWidget *w, QAccessible::Role r);

    QString text(QAccessible::Text t) const Q_DECL_OVERRIDE;
    QAccessible::State state() const Q_DECL_OVERRIDE;

    QStringList actionNames() const Q_DECL_OVERRIDE;
    void doAction(const QString &actionName) Q_DECL_OVERRIDE;
    QStringList keyBindingsForAction(const QString &actionName) const Q_DECL_OVERRIDE;

protected:
    QAbstractButton *button() const;
};

#ifndef QT_NO_TOOLBUTTON
class QAccessibleToolButton : public QAccessibleButton
{
public:
    QAccessibleToolButton(QWidget *w, QAccessible::Role role);

    QAccessible::State state() const Q_DECL_OVERRIDE;

    int childCount() const Q_DECL_OVERRIDE;
    QAccessibleInterface *child(int index) const Q_DECL_OVERRIDE;

    QString text(QAccessible::Text t) const Q_DECL_OVERRIDE;

    // QAccessibleActionInterface
    QStringList actionNames() const Q_DECL_OVERRIDE;
    void doAction(const QString &actionName) Q_DECL_OVERRIDE;

protected:
    QToolButton *toolButton() const;

    bool isSplitButton() const;
};
#endif // QT_NO_TOOLBUTTON

class QAccessibleDisplay : public QAccessibleWidget, public QAccessibleImageInterface
{
public:
    explicit QAccessibleDisplay(QWidget *w, QAccessible::Role role = QAccessible::StaticText);

    QString text(QAccessible::Text t) const Q_DECL_OVERRIDE;
    QAccessible::Role role() const Q_DECL_OVERRIDE;

    QVector<QPair<QAccessibleInterface*, QAccessible::Relation> >relations(QAccessible::Relation match = QAccessible::AllRelations) const Q_DECL_OVERRIDE;
    void *interface_cast(QAccessible::InterfaceType t) Q_DECL_OVERRIDE;

    // QAccessibleImageInterface
    QString imageDescription() const Q_DECL_OVERRIDE;
    QSize imageSize() const Q_DECL_OVERRIDE;
    QPoint imagePosition() const Q_DECL_OVERRIDE;
};

#ifndef QT_NO_GROUPBOX
class QAccessibleGroupBox : public QAccessibleWidget
{
public:
    explicit QAccessibleGroupBox(QWidget *w);

    QAccessible::State state() const Q_DECL_OVERRIDE;
    QAccessible::Role role() const Q_DECL_OVERRIDE;
    QString text(QAccessible::Text t) const Q_DECL_OVERRIDE;

    QVector<QPair<QAccessibleInterface*, QAccessible::Relation> >relations(QAccessible::Relation match = QAccessible::AllRelations) const Q_DECL_OVERRIDE;

    //QAccessibleActionInterface
    QStringList actionNames() const Q_DECL_OVERRIDE;
    void doAction(const QString &actionName) Q_DECL_OVERRIDE;
    QStringList keyBindingsForAction(const QString &) const Q_DECL_OVERRIDE;

private:
    QGroupBox *groupBox() const;
};
#endif

#ifndef QT_NO_LINEEDIT
class QAccessibleLineEdit : public QAccessibleWidget, public QAccessibleTextInterface, public QAccessibleEditableTextInterface
{
public:
    explicit QAccessibleLineEdit(QWidget *o, const QString &name = QString());

    QString text(QAccessible::Text t) const Q_DECL_OVERRIDE;
    void setText(QAccessible::Text t, const QString &text) Q_DECL_OVERRIDE;
    QAccessible::State state() const Q_DECL_OVERRIDE;
    void *interface_cast(QAccessible::InterfaceType t) Q_DECL_OVERRIDE;

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
            int *startOffset, int *endOffset) const Q_DECL_OVERRIDE;
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
    QLineEdit *lineEdit() const;
};
#endif // QT_NO_LINEEDIT

#ifndef QT_NO_PROGRESSBAR
class QAccessibleProgressBar : public QAccessibleDisplay, public QAccessibleValueInterface
{
public:
    explicit QAccessibleProgressBar(QWidget *o);
    void *interface_cast(QAccessible::InterfaceType t) Q_DECL_OVERRIDE;

    // QAccessibleValueInterface
    QVariant currentValue() const Q_DECL_OVERRIDE;
    QVariant maximumValue() const Q_DECL_OVERRIDE;
    QVariant minimumValue() const Q_DECL_OVERRIDE;
    QVariant minimumStepSize() const Q_DECL_OVERRIDE;
    void setCurrentValue(const QVariant &) Q_DECL_OVERRIDE {}

protected:
    QProgressBar *progressBar() const;
};
#endif

class QWindowContainer;
class QAccessibleWindowContainer : public QAccessibleWidget
{
public:
    QAccessibleWindowContainer(QWidget *w);
    int childCount() const Q_DECL_OVERRIDE;
    int indexOfChild(const QAccessibleInterface *child) const Q_DECL_OVERRIDE;
    QAccessibleInterface *child(int i) const Q_DECL_OVERRIDE;

private:
    QWindowContainer *container() const;
};

#endif // QT_NO_ACCESSIBILITY

QT_END_NAMESPACE

#endif // SIMPLEWIDGETS_H
