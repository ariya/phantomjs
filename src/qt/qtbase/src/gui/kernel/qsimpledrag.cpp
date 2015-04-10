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

#include "qsimpledrag_p.h"

#include "qbitmap.h"
#include "qdrag.h"
#include "qpixmap.h"
#include "qevent.h"
#include "qfile.h"
#include "qtextcodec.h"
#include "qguiapplication.h"
#include "qpoint.h"
#include "qbuffer.h"
#include "qimage.h"
#include "qregexp.h"
#include "qdir.h"
#include "qimagereader.h"
#include "qimagewriter.h"

#include <QtCore/QEventLoop>
#include <QtCore/QDebug>

#include <private/qguiapplication_p.h>
#include <private/qdnd_p.h>

#include <private/qshapedpixmapdndwindow_p.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_DRAGANDDROP

static QWindow* topLevelAt(const QPoint &pos)
{
    QWindowList list = QGuiApplication::topLevelWindows();
    for (int i = list.count()-1; i >= 0; --i) {
        QWindow *w = list.at(i);
        if (w->isVisible() && w->geometry().contains(pos) && !qobject_cast<QShapedPixmapWindow*>(w))
            return w;
    }
    return 0;
}

/*!
    \class QBasicDrag
    \brief QBasicDrag is a base class for implementing platform drag and drop.
    \since 5.0
    \internal
    \ingroup qpa

    QBasicDrag implements QPlatformDrag::drag() by running a local event loop in which
    it tracks mouse movements and moves the drag icon (QShapedPixmapWindow) accordingly.
    It provides new virtuals allowing for querying whether the receiving window
    (within the Qt application or outside) accepts the drag and sets the state accordingly.
*/

QBasicDrag::QBasicDrag() :
    m_restoreCursor(false), m_eventLoop(0),
    m_executed_drop_action(Qt::IgnoreAction), m_can_drop(false),
    m_drag(0), m_drag_icon_window(0)
{
}

QBasicDrag::~QBasicDrag()
{
    delete m_drag_icon_window;
}

void QBasicDrag::enableEventFilter()
{
    qApp->installEventFilter(this);
}

void QBasicDrag::disableEventFilter()
{
    qApp->removeEventFilter(this);
}

bool QBasicDrag::eventFilter(QObject *o, QEvent *e)
{
    Q_UNUSED(o);

    if (!m_drag) {
        if (e->type() == QEvent::KeyRelease && static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape) {
            disableEventFilter();
            exitDndEventLoop();
            return true; // block the key release
        }
        return false;
    }

    switch (e->type()) {
        case QEvent::ShortcutOverride:
            // prevent accelerators from firing while dragging
            e->accept();
            return true;

        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        {
            QKeyEvent *ke = static_cast<QKeyEvent *>(e);
            if (ke->key() == Qt::Key_Escape && e->type() == QEvent::KeyPress) {
                cancel();
                disableEventFilter();
                exitDndEventLoop();

            }
            return true; // Eat all key events
        }

        case QEvent::MouseMove:
            move(static_cast<QMouseEvent *>(e));
            return true; // Eat all mouse events

        case QEvent::MouseButtonRelease:
            disableEventFilter();
            if (canDrop()) {
                drop(static_cast<QMouseEvent *>(e));
            } else {
                cancel();
            }
            exitDndEventLoop();
            return true; // Eat all mouse events

        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonDblClick:
        case QEvent::Wheel:
            return true;
        default:
             break;
    }
    return false;
}

Qt::DropAction QBasicDrag::drag(QDrag *o)
{
    m_drag = o;
    m_executed_drop_action = Qt::IgnoreAction;
    m_can_drop = false;
    m_restoreCursor = true;
#ifndef QT_NO_CURSOR
    qApp->setOverrideCursor(Qt::DragCopyCursor);
    updateCursor(m_executed_drop_action);
#endif
    startDrag();
    m_eventLoop = new QEventLoop;
    m_eventLoop->exec();
    delete m_eventLoop;
    m_eventLoop = 0;
    m_drag = 0;
    endDrag();
    return m_executed_drop_action;
}

void QBasicDrag::restoreCursor()
{
    if (m_restoreCursor) {
#ifndef QT_NO_CURSOR
        QGuiApplication::restoreOverrideCursor();
#endif
        m_restoreCursor = false;
    }
}

void QBasicDrag::startDrag()
{
    // ### TODO Check if its really necessary to have m_drag_icon_window
    // when QDrag is used without a pixmap - QDrag::setPixmap()
    if (!m_drag_icon_window)
        m_drag_icon_window = new QShapedPixmapWindow();

    m_drag_icon_window->setPixmap(m_drag->pixmap());
    m_drag_icon_window->setHotspot(m_drag->hotSpot());
    m_drag_icon_window->updateGeometry();
    m_drag_icon_window->setVisible(true);

    enableEventFilter();
}

void QBasicDrag::endDrag()
{
}

void QBasicDrag::cancel()
{
    disableEventFilter();
    restoreCursor();
    m_drag_icon_window->setVisible(false);
}

void QBasicDrag::move(const QMouseEvent *)
{
    if (m_drag)
        m_drag_icon_window->updateGeometry();
}

void QBasicDrag::drop(const QMouseEvent *)
{
    disableEventFilter();
    restoreCursor();
    m_drag_icon_window->setVisible(false);
}

void  QBasicDrag::exitDndEventLoop()
{
    if (m_eventLoop && m_eventLoop->isRunning())
        m_eventLoop->exit();
}

void QBasicDrag::updateCursor(Qt::DropAction action)
{
#ifndef QT_NO_CURSOR
    Qt::CursorShape cursorShape = Qt::ForbiddenCursor;
    if (canDrop()) {
        switch (action) {
        case Qt::CopyAction:
            cursorShape = Qt::DragCopyCursor;
            break;
        case Qt::LinkAction:
            cursorShape = Qt::DragLinkCursor;
            break;
        default:
            cursorShape = Qt::DragMoveCursor;
            break;
        }
    }

    QCursor *cursor = qApp->overrideCursor();
    QPixmap pixmap = m_drag->dragCursor(action);
    if (!cursor) {
        qApp->changeOverrideCursor((pixmap.isNull()) ? QCursor(cursorShape) : QCursor(pixmap));
    } else {
        if (!pixmap.isNull()) {
            if ((cursor->pixmap().cacheKey() != pixmap.cacheKey())) {
                qApp->changeOverrideCursor(QCursor(pixmap));
            }
        } else {
            if (cursorShape != cursor->shape()) {
                qApp->changeOverrideCursor(QCursor(cursorShape));
            }
        }
    }
#endif
    updateAction(action);
}

/*!
    \class QSimpleDrag
    \brief QSimpleDrag implements QBasicDrag for Drag and Drop operations within the Qt Application itself.
    \since 5.0
    \internal
    \ingroup qpa

    The class checks whether the receiving window is a window of the Qt application
    and sets the state accordingly. It does not take windows of other applications
    into account.
*/

QSimpleDrag::QSimpleDrag() : m_current_window(0)
{
}

QMimeData *QSimpleDrag::platformDropData()
{
    if (drag())
        return drag()->mimeData();
    return 0;
}

void QSimpleDrag::startDrag()
{
    QBasicDrag::startDrag();
    m_current_window = topLevelAt(QCursor::pos());
    if (m_current_window) {
        QPlatformDragQtResponse response = QWindowSystemInterface::handleDrag(m_current_window, drag()->mimeData(), QCursor::pos(), drag()->supportedActions());
        setCanDrop(response.isAccepted());
        updateCursor(response.acceptedAction());
    } else {
        setCanDrop(false);
        updateCursor(Qt::IgnoreAction);
    }
    setExecutedDropAction(Qt::IgnoreAction);
}

void QSimpleDrag::cancel()
{
    QBasicDrag::cancel();
    if (drag() && m_current_window) {
        QWindowSystemInterface::handleDrag(m_current_window, 0, QPoint(), Qt::IgnoreAction);
        m_current_window = 0;
    }
}

void QSimpleDrag::move(const QMouseEvent *me)
{
    QBasicDrag::move(me);
    QWindow *window = topLevelAt(me->globalPos());
    if (!window)
        return;

    const QPoint pos = me->globalPos() - window->geometry().topLeft();
    const QPlatformDragQtResponse qt_response =
        QWindowSystemInterface::handleDrag(window, drag()->mimeData(), pos, drag()->supportedActions());

    updateCursor(qt_response.acceptedAction());
    setCanDrop(qt_response.isAccepted());
}

void QSimpleDrag::drop(const QMouseEvent *me)
{
    QBasicDrag::drop(me);
    QWindow *window = topLevelAt(me->globalPos());
    if (!window)
        return;

    const QPoint pos = me->globalPos() - window->geometry().topLeft();
    const QPlatformDropQtResponse response =
            QWindowSystemInterface::handleDrop(window, drag()->mimeData(),pos, drag()->supportedActions());
    if (response.isAccepted()) {
        setExecutedDropAction(response.acceptedAction());
    } else {
        setExecutedDropAction(Qt::IgnoreAction);
    }
}

#endif // QT_NO_DRAGANDDROP

QT_END_NAMESPACE
