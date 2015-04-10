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

#ifndef QWidgetLineControl_P_H
#define QWidgetLineControl_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qglobal.h"

#ifndef QT_NO_LINEEDIT
#include "private/qwidget_p.h"
#include "QtWidgets/qlineedit.h"
#include "QtGui/qtextlayout.h"
#include "QtWidgets/qstyleoption.h"
#include "QtCore/qpointer.h"
#include "QtGui/qclipboard.h"
#include "QtGui/qinputmethod.h"
#include "QtCore/qpoint.h"
#include "QtWidgets/qcompleter.h"
#include "QtCore/qthread.h"

#include "qplatformdefs.h"

#ifdef DrawText
#  undef DrawText
#endif

QT_BEGIN_NAMESPACE


class Q_WIDGETS_EXPORT QWidgetLineControl : public QObject
{
    Q_OBJECT

public:
    QWidgetLineControl(const QString &txt = QString())
        : m_cursor(0), m_preeditCursor(0), m_cursorWidth(0), m_layoutDirection(Qt::LayoutDirectionAuto),
        m_hideCursor(false), m_separator(0), m_readOnly(0),
        m_dragEnabled(0), m_echoMode(0), m_textDirty(0), m_selDirty(0),
        m_validInput(1), m_blinkStatus(0), m_blinkPeriod(0), m_blinkTimer(0), m_deleteAllTimer(0),
        m_ascent(0), m_maxLength(32767), m_lastCursorPos(-1),
        m_tripleClickTimer(0), m_maskData(0), m_modifiedState(0), m_undoState(0),
        m_selstart(0), m_selend(0), m_passwordEchoEditing(false)
        , m_passwordEchoTimer(0)
#if defined(Q_WS_MAC)
        , m_threadChecks(false)
        , m_textLayoutThread(0)
 #endif
#if defined(QT_BUILD_INTERNAL)
        , m_passwordMaskDelayOverride(-1)
#endif
        , m_keyboardScheme(0)
    {
        init(txt);
    }

    ~QWidgetLineControl()
    {
        delete [] m_maskData;
    }

    int nextMaskBlank(int pos)
    {
        int c = findInMask(pos, true, false);
        m_separator |= (c != pos);
        return (c != -1 ?  c : m_maxLength);
    }

    int prevMaskBlank(int pos)
    {
        int c = findInMask(pos, false, false);
        m_separator |= (c != pos);
        return (c != -1 ? c : 0);
    }

    bool isUndoAvailable() const;
    bool isRedoAvailable() const;
    void clearUndo() { m_history.clear(); m_modifiedState = m_undoState = 0; }

    bool isModified() const { return m_modifiedState != m_undoState; }
    void setModified(bool modified) { m_modifiedState = modified ? -1 : m_undoState; }

    bool allSelected() const { return !m_text.isEmpty() && m_selstart == 0 && m_selend == (int)m_text.length(); }
    bool hasSelectedText() const { return !m_text.isEmpty() && m_selend > m_selstart; }

    int width() const { return qRound(m_textLayout.lineAt(0).width()) + 1; }
    int height() const { return qRound(m_textLayout.lineAt(0).height()) + 1; }
    int ascent() const { return m_ascent; }
    qreal naturalTextWidth() const { return m_textLayout.lineAt(0).naturalTextWidth(); }

    void setSelection(int start, int length);

    inline QString selectedText() const { return hasSelectedText() ? m_text.mid(m_selstart, m_selend - m_selstart) : QString(); }
    QString textBeforeSelection() const { return hasSelectedText() ? m_text.left(m_selstart) : QString(); }
    QString textAfterSelection() const { return hasSelectedText() ? m_text.mid(m_selend) : QString(); }

    int selectionStart() const { return hasSelectedText() ? m_selstart : -1; }
    int selectionEnd() const { return hasSelectedText() ? m_selend : -1; }
    bool inSelection(int x) const
    {
        if (m_selstart >= m_selend)
            return false;
        int pos = xToPos(x, QTextLine::CursorOnCharacter);
        return pos >= m_selstart && pos < m_selend;
    }

    void removeSelection()
    {
        int priorState = m_undoState;
        removeSelectedText();
        finishChange(priorState);
    }

    int start() const { return 0; }
    int end() const { return m_text.length(); }

#ifndef QT_NO_CLIPBOARD
    void copy(QClipboard::Mode mode = QClipboard::Clipboard) const;
    void paste(QClipboard::Mode mode = QClipboard::Clipboard);
#endif

    int cursor() const{ return m_cursor; }
    int preeditCursor() const { return m_preeditCursor; }

    int cursorWidth() const { return m_cursorWidth; }
    void setCursorWidth(int value) { m_cursorWidth = value; }

    Qt::CursorMoveStyle cursorMoveStyle() const { return m_textLayout.cursorMoveStyle(); }
    void setCursorMoveStyle(Qt::CursorMoveStyle style) { m_textLayout.setCursorMoveStyle(style); }

    void moveCursor(int pos, bool mark = false);
    void cursorForward(bool mark, int steps)
    {
        int c = m_cursor;
        if (steps > 0) {
            while (steps--)
                c = cursorMoveStyle() == Qt::VisualMoveStyle ? m_textLayout.rightCursorPosition(c)
                                                             : m_textLayout.nextCursorPosition(c);
        } else if (steps < 0) {
            while (steps++)
                c = cursorMoveStyle() == Qt::VisualMoveStyle ? m_textLayout.leftCursorPosition(c)
                                                             : m_textLayout.previousCursorPosition(c);
        }
        moveCursor(c, mark);
    }

    void cursorWordForward(bool mark) { moveCursor(m_textLayout.nextCursorPosition(m_cursor, QTextLayout::SkipWords), mark); }
    void cursorWordBackward(bool mark) { moveCursor(m_textLayout.previousCursorPosition(m_cursor, QTextLayout::SkipWords), mark); }

    void home(bool mark) { moveCursor(0, mark); }
    void end(bool mark) { moveCursor(text().length(), mark); }

    int xToPos(int x, QTextLine::CursorPosition = QTextLine::CursorBetweenCharacters) const;
    QRect cursorRect() const;

    qreal cursorToX(int cursor) const { return m_textLayout.lineAt(0).cursorToX(cursor); }
    qreal cursorToX() const
    {
        int cursor = m_cursor;
        if (m_preeditCursor != -1)
            cursor += m_preeditCursor;
        return cursorToX(cursor);
    }

    bool isReadOnly() const { return m_readOnly; }
    void setReadOnly(bool enable);

    QString text() const
    {
        QString content = m_text;
        QString res = m_maskData ? stripString(content) : content;
        return (res.isNull() ? QString::fromLatin1("") : res);
    }
    void setText(const QString &txt)
    {
        if (composeMode())
            qApp->inputMethod()->reset();
        internalSetText(txt, -1, false);
    }
    void commitPreedit();

    QString displayText() const { return m_textLayout.text(); }

    void backspace();
    void del();
    void deselect() { internalDeselect(); finishChange(); }
    void selectAll() { m_selstart = m_selend = m_cursor = 0; moveCursor(m_text.length(), true); }

    void insert(const QString &);
    void clear();
    void undo() { internalUndo(); finishChange(-1, true); }
    void redo() { internalRedo(); finishChange(); }
    void selectWordAtPos(int);

    uint echoMode() const { return m_echoMode; }
    void setEchoMode(uint mode)
    {
        cancelPasswordEchoTimer();
        m_echoMode = mode;
        m_passwordEchoEditing = false;
        updateDisplayText();
    }

    int maxLength() const { return m_maxLength; }
    void setMaxLength(int maxLength)
    {
        if (m_maskData)
            return;
        m_maxLength = maxLength;
        setText(m_text);
    }

#ifndef QT_NO_VALIDATOR
    const QValidator *validator() const { return m_validator; }
    void setValidator(const QValidator *v) { m_validator = const_cast<QValidator*>(v); }
#endif

#ifndef QT_NO_COMPLETER
    QCompleter *completer() const { return m_completer; }
    /* Note that you must set the widget for the completer separately */
    void setCompleter(const QCompleter *c) { m_completer = const_cast<QCompleter*>(c); }
    void complete(int key);
#endif

    int cursorPosition() const { return m_cursor; }
    void setCursorPosition(int pos) { if (pos <= m_text.length()) moveCursor(qMax(0, pos)); }

    bool hasAcceptableInput() const { return hasAcceptableInput(m_text); }
    bool fixup();

    QString inputMask() const
    {
        QString mask;
        if (m_maskData) {
            mask = m_inputMask;
            if (m_blank != QLatin1Char(' ')) {
                mask += QLatin1Char(';');
                mask += m_blank;
            }
        }
        return mask;
    }
    void setInputMask(const QString &mask)
    {
        parseInputMask(mask);
        if (m_maskData)
            moveCursor(nextMaskBlank(0));
    }

    // input methods
#ifndef QT_NO_IM
    bool composeMode() const { return !m_textLayout.preeditAreaText().isEmpty(); }
    void setPreeditArea(int cursor, const QString &text) { m_textLayout.setPreeditArea(cursor, text); }
#endif

    QString preeditAreaText() const { return m_textLayout.preeditAreaText(); }

    void updatePasswordEchoEditing(bool editing);
    bool passwordEchoEditing() const {
        if (m_passwordEchoTimer != 0)
            return true;
        return m_passwordEchoEditing ;
    }

    QChar passwordCharacter() const { return m_passwordCharacter; }
    void setPasswordCharacter(QChar character) { m_passwordCharacter = character; updateDisplayText(); }

    Qt::LayoutDirection layoutDirection() const {
        if (m_layoutDirection == Qt::LayoutDirectionAuto) {
            if (m_text.isEmpty())
                return qApp->inputMethod()->inputDirection();
            return m_text.isRightToLeft() ? Qt::RightToLeft : Qt::LeftToRight;
        }
        return m_layoutDirection;
    }
    void setLayoutDirection(Qt::LayoutDirection direction)
    {
        if (direction != m_layoutDirection) {
            m_layoutDirection = direction;
            updateDisplayText();
        }
    }

    void setFont(const QFont &font) { m_textLayout.setFont(font); updateDisplayText(); }

    void processInputMethodEvent(QInputMethodEvent *event);
    void processKeyEvent(QKeyEvent* ev);

    int cursorBlinkPeriod() const { return m_blinkPeriod; }
    void setCursorBlinkPeriod(int msec);
    void resetCursorBlinkTimer();

    bool cursorBlinkStatus() const { return m_blinkStatus; }

    QString cancelText() const { return m_cancelText; }
    void setCancelText(const QString &text) { m_cancelText = text; }

    const QPalette &palette() const { return m_palette; }
    void setPalette(const QPalette &p) { m_palette = p; }

    enum DrawFlags {
        DrawText = 0x01,
        DrawSelections = 0x02,
        DrawCursor = 0x04,
        DrawAll = DrawText | DrawSelections | DrawCursor
    };
    void draw(QPainter *, const QPoint &, const QRect &, int flags = DrawAll);

#ifndef QT_NO_SHORTCUT
    void processShortcutOverrideEvent(QKeyEvent *ke);
#endif

    QTextLayout *textLayout() const
    {
#if defined(Q_WS_MAC)
        if (m_threadChecks && QThread::currentThread() != m_textLayoutThread)
            redoTextLayout();
#endif
        return &m_textLayout;
    }

#if defined(Q_WS_MAC)
    void setThreadChecks(bool threadChecks)
    {
        m_threadChecks = threadChecks;
    }

    bool threadChecks() const
    {
        return m_threadChecks;
    }
#endif

private:
    void init(const QString &txt);
    void removeSelectedText();
    void internalSetText(const QString &txt, int pos = -1, bool edited = true);
    void updateDisplayText(bool forceUpdate = false);

    void internalInsert(const QString &s);
    void internalDelete(bool wasBackspace = false);
    void internalRemove(int pos);

    inline void internalDeselect()
    {
        m_selDirty |= (m_selend > m_selstart);
        m_selstart = m_selend = 0;
    }

    void internalUndo(int until = -1);
    void internalRedo();

    QString m_text;
    QPalette m_palette;
    int m_cursor;
    int m_preeditCursor;
    int m_cursorWidth;
    Qt::LayoutDirection m_layoutDirection;
    uint m_hideCursor : 1; // used to hide the m_cursor inside preedit areas
    uint m_separator : 1;
    uint m_readOnly : 1;
    uint m_dragEnabled : 1;
    uint m_echoMode : 2;
    uint m_textDirty : 1;
    uint m_selDirty : 1;
    uint m_validInput : 1;
    uint m_blinkStatus : 1;
    int m_blinkPeriod; // 0 for non-blinking cursor
    int m_blinkTimer;
    int m_deleteAllTimer;
    int m_ascent;
    int m_maxLength;
    int m_lastCursorPos;
    QList<int> m_transactions;
    QPoint m_tripleClick;
    int m_tripleClickTimer;
    QString m_cancelText;

    void emitCursorPositionChanged();

    bool finishChange(int validateFromState = -1, bool update = false, bool edited = true);

#ifndef QT_NO_VALIDATOR
    QPointer<QValidator> m_validator;
#endif
    QPointer<QCompleter> m_completer;
#ifndef QT_NO_COMPLETER
    bool advanceToEnabledItem(int dir);
#endif

    struct MaskInputData {
        enum Casemode { NoCaseMode, Upper, Lower };
        QChar maskChar; // either the separator char or the inputmask
        bool separator;
        Casemode caseMode;
    };
    QString m_inputMask;
    QChar m_blank;
    MaskInputData *m_maskData;

    // undo/redo handling
    enum CommandType { Separator, Insert, Remove, Delete, RemoveSelection, DeleteSelection, SetSelection };
    struct Command {
        inline Command() {}
        inline Command(CommandType t, int p, QChar c, int ss, int se) : type(t),uc(c),pos(p),selStart(ss),selEnd(se) {}
        uint type : 4;
        QChar uc;
        int pos, selStart, selEnd;
    };
    int m_modifiedState;
    int m_undoState;
    QVector<Command> m_history;
    void addCommand(const Command& cmd);

    inline void separate() { m_separator = true; }

    // selection
    int m_selstart;
    int m_selend;

    // masking
    void parseInputMask(const QString &maskFields);
    bool isValidInput(QChar key, QChar mask) const;
    bool hasAcceptableInput(const QString &text) const;
    QString maskString(uint pos, const QString &str, bool clear = false) const;
    QString clearString(uint pos, uint len) const;
    QString stripString(const QString &str) const;
    int findInMask(int pos, bool forward, bool findSeparator, QChar searchChar = QChar()) const;

    // complex text layout (must be mutable so it can be reshaped at will)
    mutable QTextLayout m_textLayout;

    bool m_passwordEchoEditing;
    QChar m_passwordCharacter;
    int m_passwordEchoTimer;
    void cancelPasswordEchoTimer()
    {
        if (m_passwordEchoTimer != 0) {
            killTimer(m_passwordEchoTimer);
            m_passwordEchoTimer = 0;
        }
    }

    int redoTextLayout() const;
#if defined(Q_WS_MAC)
    bool m_threadChecks;
    mutable QThread *m_textLayoutThread;
#endif

public:
#if defined(QT_BUILD_INTERNAL)
    int m_passwordMaskDelayOverride;
#endif

Q_SIGNALS:
    void cursorPositionChanged(int, int);
    void selectionChanged();

    void displayTextChanged(const QString &);
    void textChanged(const QString &);
    void textEdited(const QString &);

    void resetInputContext();
    void updateMicroFocus();

    void accepted();
    void editingFinished();
    void updateNeeded(const QRect &);

#ifdef QT_KEYPAD_NAVIGATION
    void editFocusChange(bool);
#endif
protected:
    virtual void timerEvent(QTimerEvent *event);

private Q_SLOTS:
    void _q_clipboardChanged();
    void _q_deleteSelected();

private:
    int m_keyboardScheme;
};

QT_END_NAMESPACE

#endif // QT_NO_LINEEDIT

#endif // QWidgetLineControl_P_H
