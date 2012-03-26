/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QLINEEDIT_H
#define QLINEEDIT_H

#include <QtGui/qframe.h>
#include <QtGui/qtextcursor.h>
#include <QtCore/qstring.h>
#include <QtCore/qmargins.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

#ifndef QT_NO_LINEEDIT

class QValidator;
class QMenu;
class QLineEditPrivate;
class QCompleter;
class QStyleOptionFrame;
class QAbstractSpinBox;
class QDateTimeEdit;

class Q_GUI_EXPORT QLineEdit : public QWidget
{
    Q_OBJECT

    Q_ENUMS(EchoMode)
    Q_PROPERTY(QString inputMask READ inputMask WRITE setInputMask)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged USER true)
    Q_PROPERTY(int maxLength READ maxLength WRITE setMaxLength)
    Q_PROPERTY(bool frame READ hasFrame WRITE setFrame)
    Q_PROPERTY(EchoMode echoMode READ echoMode WRITE setEchoMode)
    Q_PROPERTY(QString displayText READ displayText)
    Q_PROPERTY(int cursorPosition READ cursorPosition WRITE setCursorPosition)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment)
    Q_PROPERTY(bool modified READ isModified WRITE setModified DESIGNABLE false)
    Q_PROPERTY(bool hasSelectedText READ hasSelectedText)
    Q_PROPERTY(QString selectedText READ selectedText)
    Q_PROPERTY(bool dragEnabled READ dragEnabled WRITE setDragEnabled)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)
    Q_PROPERTY(bool undoAvailable READ isUndoAvailable)
    Q_PROPERTY(bool redoAvailable READ isRedoAvailable)
    Q_PROPERTY(bool acceptableInput READ hasAcceptableInput)
    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText)
    Q_PROPERTY(Qt::CursorMoveStyle cursorMoveStyle READ cursorMoveStyle WRITE setCursorMoveStyle)

public:
    explicit QLineEdit(QWidget* parent=0);
    explicit QLineEdit(const QString &, QWidget* parent=0);
#ifdef QT3_SUPPORT
    QT3_SUPPORT_CONSTRUCTOR QLineEdit(QWidget* parent, const char* name);
    QT3_SUPPORT_CONSTRUCTOR QLineEdit(const QString &, QWidget* parent, const char* name);
    QT3_SUPPORT_CONSTRUCTOR QLineEdit(const QString &, const QString &, QWidget* parent=0, const char* name=0);
#endif
    ~QLineEdit();

    QString text() const;

    QString displayText() const;

    QString placeholderText() const;
    void setPlaceholderText(const QString &);

    int maxLength() const;
    void setMaxLength(int);

    void setFrame(bool);
    bool hasFrame() const;

    enum EchoMode { Normal, NoEcho, Password, PasswordEchoOnEdit };
    EchoMode echoMode() const;
    void setEchoMode(EchoMode);

    bool isReadOnly() const;
    void setReadOnly(bool);

#ifndef QT_NO_VALIDATOR
    void setValidator(const QValidator *);
    const QValidator * validator() const;
#endif

#ifndef QT_NO_COMPLETER
    void setCompleter(QCompleter *completer);
    QCompleter *completer() const;
#endif

    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    int cursorPosition() const;
    void setCursorPosition(int);
    int cursorPositionAt(const QPoint &pos);

    void setAlignment(Qt::Alignment flag);
    Qt::Alignment alignment() const;

    void cursorForward(bool mark, int steps = 1);
    void cursorBackward(bool mark, int steps = 1);
    void cursorWordForward(bool mark);
    void cursorWordBackward(bool mark);
    void backspace();
    void del();
    void home(bool mark);
    void end(bool mark);

    bool isModified() const;
    void setModified(bool);

    void setSelection(int, int);
    bool hasSelectedText() const;
    QString selectedText() const;
    int selectionStart() const;

    bool isUndoAvailable() const;
    bool isRedoAvailable() const;

    void setDragEnabled(bool b);
    bool dragEnabled() const;

    void setCursorMoveStyle(Qt::CursorMoveStyle style);
    Qt::CursorMoveStyle cursorMoveStyle() const;

    QString inputMask() const;
    void setInputMask(const QString &inputMask);
    bool hasAcceptableInput() const;

    void setTextMargins(int left, int top, int right, int bottom);
    void setTextMargins(const QMargins &margins);
    void getTextMargins(int *left, int *top, int *right, int *bottom) const;
    QMargins textMargins() const;

public Q_SLOTS:
    void setText(const QString &);
    void clear();
    void selectAll();
    void undo();
    void redo();
#ifndef QT_NO_CLIPBOARD
    void cut();
    void copy() const;
    void paste();
#endif

public:
    void deselect();
    void insert(const QString &);
#ifndef QT_NO_CONTEXTMENU
    QMenu *createStandardContextMenu();
#endif

Q_SIGNALS:
    void textChanged(const QString &);
    void textEdited(const QString &);
    void cursorPositionChanged(int, int);
    void returnPressed();
    void editingFinished();
    void selectionChanged();

protected:
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseDoubleClickEvent(QMouseEvent *);
    void keyPressEvent(QKeyEvent *);
    void focusInEvent(QFocusEvent *);
    void focusOutEvent(QFocusEvent *);
    void paintEvent(QPaintEvent *);
#ifndef QT_NO_DRAGANDDROP
    void dragEnterEvent(QDragEnterEvent *);
    void dragMoveEvent(QDragMoveEvent *e);
    void dragLeaveEvent(QDragLeaveEvent *e);
    void dropEvent(QDropEvent *);
#endif
    void changeEvent(QEvent *);
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent *);
#endif
#ifdef QT3_SUPPORT
    inline QT3_SUPPORT void repaintArea(int, int) { update(); }
#endif

    void inputMethodEvent(QInputMethodEvent *);
    void initStyleOption(QStyleOptionFrame *option) const;
public:
    QVariant inputMethodQuery(Qt::InputMethodQuery) const;
    bool event(QEvent *);
protected:
    QRect cursorRect() const;

public:
#ifdef QT3_SUPPORT
    inline QT3_SUPPORT void clearModified() { setModified(false); }
    inline QT3_SUPPORT void cursorLeft(bool mark, int steps = 1) { cursorForward(mark, -steps); }
    inline QT3_SUPPORT void cursorRight(bool mark, int steps = 1) { cursorForward(mark, steps); }
    QT3_SUPPORT bool validateAndSet(const QString &, int, int, int);
    inline QT3_SUPPORT bool frame() const { return hasFrame(); }
#ifndef QT_NO_VALIDATOR
    inline QT3_SUPPORT void clearValidator() { setValidator(0); }
#endif
    inline QT3_SUPPORT bool hasMarkedText() const { return hasSelectedText(); }
    inline QT3_SUPPORT QString markedText() const { return selectedText(); }
    QT3_SUPPORT bool edited() const;
    QT3_SUPPORT void setEdited(bool);
    QT3_SUPPORT int characterAt(int, QChar*) const;
    QT3_SUPPORT bool getSelection(int *, int *);

    QT3_SUPPORT void setFrameRect(QRect) {}
    QT3_SUPPORT QRect frameRect() const { return QRect(); }
    enum DummyFrame { Box, Sunken, Plain, Raised, MShadow, NoFrame, Panel, StyledPanel,
                      HLine, VLine, GroupBoxPanel, WinPanel, ToolBarPanel, MenuBarPanel,
                      PopupPanel, LineEditPanel, TabWidgetPanel, MShape };
    QT3_SUPPORT void setFrameShadow(DummyFrame) {}
    QT3_SUPPORT DummyFrame frameShadow() const { return Plain; }
    QT3_SUPPORT void setFrameShape(DummyFrame) {}
    QT3_SUPPORT DummyFrame frameShape() const { return NoFrame; }
    QT3_SUPPORT void setFrameStyle(int) {}
    QT3_SUPPORT int frameStyle() const  { return 0; }
    QT3_SUPPORT int frameWidth() const { return 0; }
    QT3_SUPPORT void setLineWidth(int) {}
    QT3_SUPPORT int lineWidth() const { return 0; }
    QT3_SUPPORT void setMargin(int margin) { setContentsMargins(margin, margin, margin, margin); }
    QT3_SUPPORT int margin() const
    { int margin; int dummy; getContentsMargins(&margin, &dummy, &dummy, &dummy);  return margin; }
    QT3_SUPPORT void setMidLineWidth(int) {}
    QT3_SUPPORT int midLineWidth() const { return 0; }

Q_SIGNALS:
    QT_MOC_COMPAT void lostFocus();
#endif

private:
    friend class QAbstractSpinBox;
#ifdef QT_KEYPAD_NAVIGATION
    friend class QDateTimeEdit;
#endif
    Q_DISABLE_COPY(QLineEdit)
    Q_DECLARE_PRIVATE(QLineEdit)
    Q_PRIVATE_SLOT(d_func(), void _q_handleWindowActivate())
    Q_PRIVATE_SLOT(d_func(), void _q_textEdited(const QString &))
    Q_PRIVATE_SLOT(d_func(), void _q_cursorPositionChanged(int, int))
#ifndef QT_NO_COMPLETER
    Q_PRIVATE_SLOT(d_func(), void _q_completionHighlighted(QString))
#endif
#ifdef QT_KEYPAD_NAVIGATION
    Q_PRIVATE_SLOT(d_func(), void _q_editFocusChange(bool))
#endif
    Q_PRIVATE_SLOT(d_func(), void _q_selectionChanged())
    Q_PRIVATE_SLOT(d_func(), void _q_updateNeeded(const QRect &))
};

#endif // QT_NO_LINEEDIT

QT_END_NAMESPACE

QT_END_HEADER

#endif // QLINEEDIT_H
