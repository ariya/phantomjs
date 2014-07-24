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

#ifndef QLINEEDIT_P_H
#define QLINEEDIT_P_H

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
#include "QtWidgets/qtoolbutton.h"
#include "QtGui/qtextlayout.h"
#include "QtGui/qicon.h"
#include "QtWidgets/qstyleoption.h"
#include "QtCore/qbasictimer.h"
#include "QtWidgets/qcompleter.h"
#include "QtCore/qpointer.h"
#include "QtCore/qmimedata.h"

#include "private/qwidgetlinecontrol_p.h"

QT_BEGIN_NAMESPACE

// QLineEditIconButton: This is a simple helper class that represents clickable icons that fade in with text

class QLineEditIconButton : public QToolButton
{
    Q_OBJECT
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)
public:
    enum { IconMargin = 4, IconButtonSize = 16 };

    explicit QLineEditIconButton(QWidget *parent =  0);

    qreal opacity() const { return m_opacity; }
    void setOpacity(qreal value);
    void animateShow(bool visible) { startOpacityAnimation(visible ? 1.0 : 0.0); }

protected:
    void paintEvent(QPaintEvent *event);

private slots:
    void updateCursor();

private:
    void startOpacityAnimation(qreal endValue);

    qreal m_opacity;
};

class Q_AUTOTEST_EXPORT QLineEditPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QLineEdit)
public:
    enum SideWidgetFlag {
        SideWidgetFadeInWithText = 0x1,
        SideWidgetCreatedByWidgetAction = 0x2,
        SideWidgetClearButton = 0x4
    };

    struct SideWidgetEntry {
        SideWidgetEntry(QWidget *w = 0, QAction *a = 0, int _flags = 0) : widget(w), action(a), flags(_flags) {}

        QWidget *widget;
        QAction *action;
        int flags;
    };
    typedef QList<SideWidgetEntry> SideWidgetEntryList;

    QLineEditPrivate()
        : control(0), frame(1), contextMenuEnabled(1), cursorVisible(0),
        dragEnabled(0), clickCausedFocus(0), hscroll(0), vscroll(0),
        alignment(Qt::AlignLeading | Qt::AlignVCenter),
        leftTextMargin(0), topTextMargin(0), rightTextMargin(0), bottomTextMargin(0),
        lastTextSize(0)
    {
    }

    ~QLineEditPrivate()
    {
    }

    QWidgetLineControl *control;

#ifndef QT_NO_CONTEXTMENU
    QPointer<QAction> selectAllAction;
#endif
    void init(const QString&);

    QRect adjustedControlRect(const QRect &) const;

    int xToPos(int x, QTextLine::CursorPosition = QTextLine::CursorBetweenCharacters) const;
    QRect cursorRect() const;
    void setCursorVisible(bool visible);

    void updatePasswordEchoEditing(bool);

    void resetInputMethod();

    inline bool shouldEnableInputMethod() const
    {
        return !control->isReadOnly();
    }
    inline bool shouldShowPlaceholderText() const
    {
        return control->text().isEmpty() && control->preeditAreaText().isEmpty()
                && !((alignment & Qt::AlignHCenter) && q_func()->hasFocus());
    }

    static inline QLineEditPrivate *get(QLineEdit *lineEdit) {
        return lineEdit->d_func();
    }

    QPoint tripleClick;
    QBasicTimer tripleClickTimer;
    uint frame : 1;
    uint contextMenuEnabled : 1;
    uint cursorVisible : 1;
    uint dragEnabled : 1;
    uint clickCausedFocus : 1;
    int hscroll;
    int vscroll;
    uint alignment;
    static const int verticalMargin;
    static const int horizontalMargin;

    bool sendMouseEventToInputContext(QMouseEvent *e);

    QRect adjustedContentsRect() const;

    void _q_handleWindowActivate();
    void _q_textEdited(const QString &);
    void _q_cursorPositionChanged(int, int);
#ifdef QT_KEYPAD_NAVIGATION
    void _q_editFocusChange(bool);
#endif
    void _q_selectionChanged();
    void _q_updateNeeded(const QRect &);
#ifndef QT_NO_COMPLETER
    void _q_completionHighlighted(QString);
#endif
    QPoint mousePressPos;
#ifndef QT_NO_DRAGANDDROP
    QBasicTimer dndTimer;
    void drag();
#endif
    void _q_textChanged(const QString &);

    int leftTextMargin; // use effectiveLeftTextMargin() in case of icon.
    int topTextMargin;
    int rightTextMargin; // use effectiveRightTextMargin() in case of icon.
    int bottomTextMargin;

    QString placeholderText;

    QWidget *addAction(QAction *newAction, QAction *before, QLineEdit::ActionPosition, int flags = 0);
    void removeAction(QAction *action);
    QSize iconSize() const;
    QIcon clearButtonIcon() const;
    void setClearButtonEnabled(bool enabled);
    void positionSideWidgets();
    inline bool hasSideWidgets() const { return !leadingSideWidgets.isEmpty() || !trailingSideWidgets.isEmpty(); }
    inline const SideWidgetEntryList &leftSideWidgetList() const
        { return q_func()->layoutDirection() == Qt::LeftToRight ? leadingSideWidgets : trailingSideWidgets; }
    inline const SideWidgetEntryList &rightSideWidgetList() const
        { return q_func()->layoutDirection() == Qt::LeftToRight ? trailingSideWidgets : leadingSideWidgets; }

    int effectiveLeftTextMargin() const;
    int effectiveRightTextMargin() const;

private:
    typedef QPair<QLineEdit::ActionPosition, int> PositionIndexPair;

    PositionIndexPair findSideWidget(const QAction *a) const;

    SideWidgetEntryList leadingSideWidgets;
    SideWidgetEntryList trailingSideWidgets;
    int lastTextSize;
    mutable QSize m_iconSize;
};

inline int QLineEditPrivate::effectiveLeftTextMargin() const
{
    return leftTextMargin + leftSideWidgetList().size() * (QLineEditIconButton::IconMargin + iconSize().width());
}

inline int QLineEditPrivate::effectiveRightTextMargin() const
{
    return rightTextMargin + rightSideWidgetList().size() * (QLineEditIconButton::IconMargin + iconSize().width());
}

#endif // QT_NO_LINEEDIT

QT_END_NAMESPACE

#endif // QLINEEDIT_P_H
