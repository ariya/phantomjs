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

#ifndef QPLAINTEXTEDIT_P_H
#define QPLAINTEXTEDIT_P_H

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

#include "private/qabstractscrollarea_p.h"
#include "QtGui/qtextdocumentfragment.h"
#include "QtGui/qscrollbar.h"
#include "QtGui/qtextcursor.h"
#include "QtGui/qtextformat.h"
#include "QtGui/qmenu.h"
#include "QtGui/qabstracttextdocumentlayout.h"
#include "QtCore/qbasictimer.h"
#include "private/qtextcontrol_p.h"
#include "qplaintextedit.h"

#ifndef QT_NO_TEXTEDIT

QT_BEGIN_NAMESPACE

class QMimeData;

class QPlainTextEdit;
class ExtraArea;

class QPlainTextEditControl : public QTextControl
{
    Q_OBJECT
public:
    QPlainTextEditControl(QPlainTextEdit *parent);


    QMimeData *createMimeDataFromSelection() const;
    bool canInsertFromMimeData(const QMimeData *source) const;
    void insertFromMimeData(const QMimeData *source);
    int hitTest(const QPointF &point, Qt::HitTestAccuracy = Qt::FuzzyHit) const;
    QRectF blockBoundingRect(const QTextBlock &block) const;
    inline QRectF cursorRect(const QTextCursor &cursor) const {
        QRectF r = QTextControl::cursorRect(cursor);
        r.setLeft(qMax(r.left(), (qreal) 0.));
        return r;
    }
    inline QRectF cursorRect() { return cursorRect(textCursor()); }
    void ensureCursorVisible() {
        textEdit->ensureCursorVisible();
        emit microFocusChanged();
    }


    QPlainTextEdit *textEdit;
    int topBlock;
    QTextBlock firstVisibleBlock() const;

    QVariant loadResource(int type, const QUrl &name) {
        return textEdit->loadResource(type, name);
    }

};


class QPlainTextEditPrivate : public QAbstractScrollAreaPrivate
{
    Q_DECLARE_PUBLIC(QPlainTextEdit)
public:
    QPlainTextEditPrivate();

    void init(const QString &txt = QString());
    void _q_repaintContents(const QRectF &contentsRect);

    inline QPoint mapToContents(const QPoint &point) const
        { return QPoint(point.x() + horizontalOffset(), point.y() + verticalOffset()); }

    void _q_adjustScrollbars();
    void _q_verticalScrollbarActionTriggered(int action);
    void ensureViewportLayouted();
    void relayoutDocument();

    void pageUpDown(QTextCursor::MoveOperation op, QTextCursor::MoveMode moveMode, bool moveCursor = true);

    inline int horizontalOffset() const
        { return (q_func()->isRightToLeft() ? (hbar->maximum() - hbar->value()) : hbar->value()); }
    int verticalOffset(int topBlock, int topLine) const;
    int verticalOffset() const;

    inline void sendControlEvent(QEvent *e)
        { control->processEvent(e, QPointF(horizontalOffset(), verticalOffset()), viewport); }

    void updateDefaultTextOption();

    QPlainTextEditControl *control;

    bool tabChangesFocus;

    QBasicTimer autoScrollTimer;
    QPoint autoScrollDragPos;

    QPlainTextEdit::LineWrapMode lineWrap;
    QTextOption::WrapMode wordWrap;

    uint showCursorOnInitialShow : 1;
    uint backgroundVisible : 1;
    uint centerOnScroll : 1;
    uint inDrag : 1;
    uint clickCausedFocus : 1;

    int topLine;

    void setTopLine(int visualTopLine, int dx = 0);
    void setTopBlock(int newTopBlock, int newTopLine, int dx = 0);

    void ensureVisible(int position, bool center, bool forceCenter = false);
    void ensureCursorVisible(bool center = false);
    void updateViewport();

    QPointer<QPlainTextDocumentLayout> documentLayoutPtr;

    void append(const QString &text, Qt::TextFormat format = Qt::AutoText);

    qreal pageUpDownLastCursorY;
    bool pageUpDownLastCursorYIsValid;


#ifdef QT_KEYPAD_NAVIGATION
    QBasicTimer deleteAllTimer;
#endif

    void _q_cursorPositionChanged();
    void _q_modificationChanged(bool);

    int originalOffsetY;
};

QT_END_NAMESPACE

#endif // QT_NO_TEXTEDIT

#endif // QPLAINTEXTEDIT_P_H
