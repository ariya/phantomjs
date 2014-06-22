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

#ifndef QACCESSIBLEWIDGETS_H
#define QACCESSIBLEWIDGETS_H

#include <QtWidgets/qaccessiblewidget.h>

#ifndef QT_NO_ACCESSIBILITY

#include <QtCore/QPointer>
#include <QtCore/QPair>

QT_BEGIN_NAMESPACE

class QTextEdit;
class QStackedWidget;
class QToolBox;
class QMdiArea;
class QMdiSubWindow;
class QRubberBand;
class QTextBrowser;
class QCalendarWidget;
class QAbstractItemView;
class QDockWidget;
class QDockWidgetLayout;
class QMainWindow;
class QPlainTextEdit;
class QTextCursor;
class QTextDocument;

#ifndef QT_NO_CURSOR
class QAccessibleTextWidget : public QAccessibleWidget,
                              public QAccessibleTextInterface,
                              public QAccessibleEditableTextInterface
{
public:
    QAccessibleTextWidget(QWidget *o, QAccessible::Role r = QAccessible::EditableText, const QString &name = QString());

    QAccessible::State state() const;

    // QAccessibleTextInterface
    //  selection
    void selection(int selectionIndex, int *startOffset, int *endOffset) const Q_DECL_OVERRIDE;
    int selectionCount() const Q_DECL_OVERRIDE;
    void addSelection(int startOffset, int endOffset) Q_DECL_OVERRIDE;
    void removeSelection(int selectionIndex) Q_DECL_OVERRIDE;
    void setSelection(int selectionIndex, int startOffset, int endOffset) Q_DECL_OVERRIDE;

    // cursor
    int cursorPosition() const Q_DECL_OVERRIDE;
    void setCursorPosition(int position) Q_DECL_OVERRIDE;

    // text
    QString text(int startOffset, int endOffset) const Q_DECL_OVERRIDE;
    QString textBeforeOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                             int *startOffset, int *endOffset) const Q_DECL_OVERRIDE;
    QString textAfterOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                            int *startOffset, int *endOffset) const Q_DECL_OVERRIDE;
    QString textAtOffset(int offset, QAccessible::TextBoundaryType boundaryType,
                         int *startOffset, int *endOffset) const Q_DECL_OVERRIDE;
    int characterCount() const;

    // character <-> geometry
    QRect characterRect(int offset) const Q_DECL_OVERRIDE;
    int offsetAtPoint(const QPoint &point) const Q_DECL_OVERRIDE;

    QString attributes(int offset, int *startOffset, int *endOffset) const Q_DECL_OVERRIDE;

    // QAccessibleEditableTextInterface
    void deleteText(int startOffset, int endOffset) Q_DECL_OVERRIDE;
    void insertText(int offset, const QString &text) Q_DECL_OVERRIDE;
    void replaceText(int startOffset, int endOffset, const QString &text) Q_DECL_OVERRIDE;

    using QAccessibleWidget::text;

protected:
    QTextCursor textCursorForRange(int startOffset, int endOffset) const;
    virtual QPoint scrollBarPosition() const;
    // return the current text cursor at the caret position including a potential selection
    virtual QTextCursor textCursor() const = 0;
    virtual void setTextCursor(const QTextCursor &) = 0;
    virtual QTextDocument *textDocument() const = 0;
    virtual QWidget *viewport() const = 0;
};

#ifndef QT_NO_TEXTEDIT
class QAccessiblePlainTextEdit : public QAccessibleTextWidget
{
public:
    explicit QAccessiblePlainTextEdit(QWidget *o);

    QString text(QAccessible::Text t) const Q_DECL_OVERRIDE;
    void setText(QAccessible::Text t, const QString &text) Q_DECL_OVERRIDE;
    QAccessible::State state() const Q_DECL_OVERRIDE;

    void *interface_cast(QAccessible::InterfaceType t) Q_DECL_OVERRIDE;

    // QAccessibleTextInterface
    void scrollToSubstring(int startIndex, int endIndex) Q_DECL_OVERRIDE;

    using QAccessibleTextWidget::text;

protected:
    QPlainTextEdit *plainTextEdit() const;

    QPoint scrollBarPosition() const Q_DECL_OVERRIDE;
    QTextCursor textCursor() const Q_DECL_OVERRIDE;
    void setTextCursor(const QTextCursor &textCursor) Q_DECL_OVERRIDE;
    QTextDocument *textDocument() const Q_DECL_OVERRIDE;
    QWidget *viewport() const Q_DECL_OVERRIDE;
};

class QAccessibleTextEdit : public QAccessibleTextWidget
{
public:
    explicit QAccessibleTextEdit(QWidget *o);

    QString text(QAccessible::Text t) const Q_DECL_OVERRIDE;
    void setText(QAccessible::Text t, const QString &text) Q_DECL_OVERRIDE;
    QAccessible::State state() const Q_DECL_OVERRIDE;

    void *interface_cast(QAccessible::InterfaceType t) Q_DECL_OVERRIDE;

    // QAccessibleTextInterface
    void scrollToSubstring(int startIndex, int endIndex) Q_DECL_OVERRIDE;

    using QAccessibleTextWidget::text;

protected:
    QTextEdit *textEdit() const;

    QPoint scrollBarPosition() const Q_DECL_OVERRIDE;
    QTextCursor textCursor() const Q_DECL_OVERRIDE;
    void setTextCursor(const QTextCursor &textCursor) Q_DECL_OVERRIDE;
    QTextDocument *textDocument() const Q_DECL_OVERRIDE;
    QWidget *viewport() const Q_DECL_OVERRIDE;
};
#endif // QT_NO_TEXTEDIT
#endif  //QT_NO_CURSOR

class QAccessibleStackedWidget : public QAccessibleWidget
{
public:
    explicit QAccessibleStackedWidget(QWidget *widget);

    QAccessibleInterface *childAt(int x, int y) const Q_DECL_OVERRIDE;
    int childCount() const Q_DECL_OVERRIDE;
    int indexOfChild(const QAccessibleInterface *child) const Q_DECL_OVERRIDE;
    QAccessibleInterface *child(int index) const Q_DECL_OVERRIDE;

protected:
    QStackedWidget *stackedWidget() const;
};

class QAccessibleToolBox : public QAccessibleWidget
{
public:
    explicit QAccessibleToolBox(QWidget *widget);

// FIXME we currently expose the toolbox but it is not keyboard navigatable
// and the accessible hierarchy is not exactly beautiful.
//    int childCount() const;
//    QAccessibleInterface *child(int index) const;
//    int indexOfChild(const QAccessibleInterface *child) const;

protected:
    QToolBox *toolBox() const;
};

#ifndef QT_NO_MDIAREA
class QAccessibleMdiArea : public QAccessibleWidget
{
public:
    explicit QAccessibleMdiArea(QWidget *widget);

    int childCount() const Q_DECL_OVERRIDE;
    QAccessibleInterface *child(int index) const Q_DECL_OVERRIDE;
    int indexOfChild(const QAccessibleInterface *child) const Q_DECL_OVERRIDE;

protected:
    QMdiArea *mdiArea() const;
};

class QAccessibleMdiSubWindow : public QAccessibleWidget
{
public:
    explicit QAccessibleMdiSubWindow(QWidget *widget);

    QString text(QAccessible::Text textType) const Q_DECL_OVERRIDE;
    void setText(QAccessible::Text textType, const QString &text) Q_DECL_OVERRIDE;
    QAccessible::State state() const Q_DECL_OVERRIDE;
    int childCount() const Q_DECL_OVERRIDE;
    QAccessibleInterface *child(int index) const Q_DECL_OVERRIDE;
    int indexOfChild(const QAccessibleInterface *child) const Q_DECL_OVERRIDE;
    QRect rect() const Q_DECL_OVERRIDE;

protected:
    QMdiSubWindow *mdiSubWindow() const;
};
#endif // QT_NO_MDIAREA

class QAccessibleDialogButtonBox : public QAccessibleWidget
{
public:
    explicit QAccessibleDialogButtonBox(QWidget *widget);
};

#if !defined(QT_NO_TEXTBROWSER) && !defined(QT_NO_CURSOR)
class QAccessibleTextBrowser : public QAccessibleTextEdit
{
public:
    explicit QAccessibleTextBrowser(QWidget *widget);

    QAccessible::Role role() const Q_DECL_OVERRIDE;
};
#endif // QT_NO_TEXTBROWSER && QT_NO_CURSOR

#ifndef QT_NO_CALENDARWIDGET
class QAccessibleCalendarWidget : public QAccessibleWidget
{
public:
    explicit QAccessibleCalendarWidget(QWidget *widget);

    int childCount() const Q_DECL_OVERRIDE;
    int indexOfChild(const QAccessibleInterface *child) const Q_DECL_OVERRIDE;

    QAccessibleInterface *child(int index) const Q_DECL_OVERRIDE;

protected:
    QCalendarWidget *calendarWidget() const;

private:
    QAbstractItemView *calendarView() const;
    QWidget *navigationBar() const;
};
#endif // QT_NO_CALENDARWIDGET

#ifndef QT_NO_DOCKWIDGET
class QAccessibleDockWidget: public QAccessibleWidget
{
public:
    explicit QAccessibleDockWidget(QWidget *widget);
    QAccessibleInterface *child(int index) const Q_DECL_OVERRIDE;
    int indexOfChild(const QAccessibleInterface *child) const Q_DECL_OVERRIDE;
    int childCount() const Q_DECL_OVERRIDE;
    QRect rect () const Q_DECL_OVERRIDE;
    QString text(QAccessible::Text t) const Q_DECL_OVERRIDE;

    QDockWidget *dockWidget() const;
protected:
    QDockWidgetLayout *dockWidgetLayout() const;
};

#endif // QT_NO_DOCKWIDGET

#ifndef QT_NO_MAINWINDOW
class QAccessibleMainWindow : public QAccessibleWidget
{
public:
    explicit QAccessibleMainWindow(QWidget *widget);

    QAccessibleInterface *child(int index) const Q_DECL_OVERRIDE;
    int childCount() const Q_DECL_OVERRIDE;
    int indexOfChild(const QAccessibleInterface *iface) const Q_DECL_OVERRIDE;
    QAccessibleInterface *childAt(int x, int y) const Q_DECL_OVERRIDE;
    QMainWindow *mainWindow() const;

};
#endif //QT_NO_MAINWINDOW

#endif // QT_NO_ACCESSIBILITY

QT_END_NAMESPACE

#endif // QACESSIBLEWIDGETS_H
