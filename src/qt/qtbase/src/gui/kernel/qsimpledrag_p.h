/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QSIMPLEDRAG_H
#define QSIMPLEDRAG_H

#include <qpa/qplatformdrag.h>

#include <QtCore/QObject>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_DRAGANDDROP

class QMouseEvent;
class QWindow;
class QEventLoop;
class QDropData;
class QShapedPixmapWindow;

class Q_GUI_EXPORT QBasicDrag : public QPlatformDrag, public QObject
{
public:
    virtual ~QBasicDrag();

    virtual Qt::DropAction drag(QDrag *drag);

    virtual bool eventFilter(QObject *o, QEvent *e);

protected:
    QBasicDrag();

    virtual void startDrag();
    virtual void cancel();
    virtual void move(const QMouseEvent *me);
    virtual void drop(const QMouseEvent *me);
    virtual void endDrag();

    QShapedPixmapWindow *shapedPixmapWindow() const { return m_drag_icon_window; }
    void updateCursor(Qt::DropAction action);

    bool canDrop() const { return m_can_drop; }
    void setCanDrop(bool c) { m_can_drop = c; }

    Qt::DropAction executedDropAction() const { return m_executed_drop_action; }
    void  setExecutedDropAction(Qt::DropAction da) { m_executed_drop_action = da; }

    QDrag *drag() const { return m_drag; }

private:
    void enableEventFilter();
    void disableEventFilter();
    void restoreCursor();
    void exitDndEventLoop();

    bool m_restoreCursor;
    QEventLoop *m_eventLoop;
    Qt::DropAction m_executed_drop_action;
    bool m_can_drop;
    QDrag *m_drag;
    QShapedPixmapWindow *m_drag_icon_window;
};

class Q_GUI_EXPORT QSimpleDrag : public QBasicDrag
{
public:
    QSimpleDrag();
    virtual QMimeData *platformDropData();

protected:
    virtual void startDrag();
    virtual void cancel();
    virtual void move(const QMouseEvent *me);
    virtual void drop(const QMouseEvent *me);

private:
    QWindow *m_current_window;
};

#endif // QT_NO_DRAGANDDROP

QT_END_NAMESPACE

#endif
