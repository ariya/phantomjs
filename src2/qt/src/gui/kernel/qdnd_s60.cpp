/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "qapplication.h"

#ifndef QT_NO_DRAGANDDROP

#include "qwidget.h"
#include "qdatetime.h"
#include "qbitmap.h"
#include "qcursor.h"
#include "qevent.h"
#include "qpainter.h"
#include "qdnd_p.h"
#include "qt_s60_p.h"

#include <coecntrl.h>
// pointer cursor
#include <w32std.h>
#include <gdi.h>
#include <QCursor>

QT_BEGIN_NAMESPACE
//### artistic impression of Symbians default DnD cursor ?

static QPixmap *defaultPm = 0;
static const int default_pm_hotx = -50;
static const int default_pm_hoty = -50;
static const char *const default_pm[] = {
"13 9 3 1",
".      c None",
"       c #000000",
"X      c #FFFFFF",
"X X X X X X X",
" X X X X X X ",
"X ......... X",
" X.........X ",
"X ......... X",
" X.........X ",
"X ......... X",
" X X X X X X ",
"X X X X X X X",
};
//### actions need to be redefined for S60
// Shift/Ctrl handling, and final drop status
static Qt::DropAction global_accepted_action = Qt::MoveAction;
static Qt::DropActions possible_actions = Qt::IgnoreAction;


// static variables in place of a proper cross-process solution
static QDrag *drag_object;
static bool qt_symbian_dnd_dragging = false;


static Qt::KeyboardModifiers oldstate;

void QDragManager::updatePixmap()
{
    QPixmap pm;
    QPoint pm_hot(default_pm_hotx,default_pm_hoty);
    if (drag_object) {
        pm = drag_object->pixmap();
        if (!pm.isNull())
            pm_hot = drag_object->hotSpot();
    }
    if (pm.isNull()) {
        if (!defaultPm)
            defaultPm = new QPixmap(default_pm);
        pm = *defaultPm;
    }
#ifndef QT_NO_CURSOR
    QCursor cursor(pm, pm_hot.x(), pm_hot.y());
    overrideCursor = cursor;
#endif
}

void QDragManager::timerEvent(QTimerEvent *) { }

void QDragManager::move(const QPoint&) {
}

void QDragManager::updateCursor()
{
#ifndef QT_NO_CURSOR
    QCursor cursor = willDrop ? overrideCursor : Qt::ForbiddenCursor;
    if (!restoreCursor) {
        QApplication::setOverrideCursor(cursor);
        restoreCursor = true;
    }
    else {
        QApplication::changeOverrideCursor(cursor);
    }
#endif
}


bool QDragManager::eventFilter(QObject *o, QEvent *e)
{
 if (beingCancelled) {
        return false;
    }
    if (!o->isWidgetType())
        return false;

    switch(e->type()) {
        case QEvent::MouseButtonPress:
        {
        }
        case QEvent::MouseMove:
        {
            if (!object) { //#### this should not happen
                qWarning("QDragManager::eventFilter: No object");
                return true;
            }
            QDragManager *manager = QDragManager::self();
            QMimeData *dropData = manager->object ? manager->dragPrivate()->data : manager->dropData;
            if (manager->object)
                possible_actions =  manager->dragPrivate()->possible_actions;
            else
                possible_actions = Qt::IgnoreAction;

            QMouseEvent *me = (QMouseEvent *)e;

            if (me->buttons()) {
                Qt::DropAction prevAction = global_accepted_action;
                QWidget *cw = QApplication::widgetAt(me->globalPos());
                // map the Coords relative to the window.
                if (!cw)
                    return true;

                while (cw && !cw->acceptDrops() && !cw->isWindow())
                    cw = cw->parentWidget();

                bool oldWillDrop = willDrop;
                if (object->target() != cw) {
                    if (object->target()) {
                        QDragLeaveEvent dle;
                        QApplication::sendEvent(object->target(), &dle);
                        willDrop = false;
                        global_accepted_action = Qt::IgnoreAction;
                        if (oldWillDrop != willDrop)
                            updateCursor();
                        object->d_func()->target = 0;
                    }
                    if (cw && cw->acceptDrops()) {
                        object->d_func()->target = cw;
                        QDragEnterEvent dee(cw->mapFromGlobal(me->globalPos()), possible_actions, dropData,
                                            me->buttons(), me->modifiers());
                        QApplication::sendEvent(object->target(), &dee);
                        willDrop = dee.isAccepted() && dee.dropAction() != Qt::IgnoreAction;
                        global_accepted_action = willDrop ? dee.dropAction() : Qt::IgnoreAction;
                        if (oldWillDrop != willDrop)
                            updateCursor();
                    }
                } else if (cw) {
                    QDragMoveEvent dme(cw->mapFromGlobal(me->globalPos()), possible_actions, dropData,
                                       me->buttons(), me->modifiers());
                    if (global_accepted_action != Qt::IgnoreAction) {
                        dme.setDropAction(global_accepted_action);
                        dme.accept();
                    }
                    QApplication::sendEvent(cw, &dme);
                    willDrop = dme.isAccepted();
                    global_accepted_action = willDrop ? dme.dropAction() : Qt::IgnoreAction;
                    if (oldWillDrop != willDrop) {
                        updatePixmap();
                        updateCursor();
                    }
                }
                if (global_accepted_action != prevAction)
                    emitActionChanged(global_accepted_action);
            }
            return true; // Eat all mouse events
        }

        case QEvent::MouseButtonRelease:
        {
            qApp->removeEventFilter(this);
#ifndef QT_NO_CURSOR
            if (restoreCursor) {
                QApplication::restoreOverrideCursor();
                willDrop = false;
                restoreCursor = false;
            }
#endif
            if (object && object->target()) {

                QMouseEvent *me = (QMouseEvent *)e;

                QDragManager *manager = QDragManager::self();
                QMimeData *dropData = manager->object ? manager->dragPrivate()->data : manager->dropData;

                QDropEvent de(object->target()->mapFromGlobal(me->globalPos()), possible_actions, dropData,
                              me->buttons(), me->modifiers());
                QApplication::sendEvent(object->target(), &de);
                if (de.isAccepted())
                    global_accepted_action = de.dropAction();
                else
                    global_accepted_action = Qt::IgnoreAction;

                if (object)
                    object->deleteLater();
                drag_object = object = 0;
            }
            eventLoop->exit();
            return true; // Eat all mouse events
        }

        default:
             break;
    }
    return false;
}

Qt::DropAction QDragManager::drag(QDrag *o)
{
    Q_ASSERT(!qt_symbian_dnd_dragging);
    if (object == o || !o || !o->source())
         return Qt::IgnoreAction;

    if (object) {
        cancel();
        qApp->removeEventFilter(this);
        beingCancelled = false;
    }

    object = drag_object = o;

    oldstate = Qt::NoModifier; // #### Should use state that caused the drag
    willDrop = false;
    updatePixmap();
    updateCursor();

#ifndef QT_NO_CURSOR
    qt_symbian_set_cursor_visible(true); //force cursor on even for touch phone
#endif

    object->d_func()->target = 0;

    qApp->installEventFilter(this);

    global_accepted_action = defaultAction(dragPrivate()->possible_actions, Qt::NoModifier);
    qt_symbian_dnd_dragging = true;

    eventLoop = new QEventLoop;
    // block
    (void) eventLoop->exec(QEventLoop::AllEvents);
    delete eventLoop;
    eventLoop = 0;

#ifndef QT_NO_CURSOR
    qt_symbian_set_cursor_visible(false);

    overrideCursor = QCursor(); //deref the cursor data
    qt_symbian_dnd_dragging = false;
#endif

    return global_accepted_action;
}


void QDragManager::cancel(bool deleteSource)
{
    beingCancelled = true;

    if (object->target()) {
        QDragLeaveEvent dle;
        QApplication::sendEvent(object->target(), &dle);
    }

    if (drag_object) {
        if (deleteSource)
            object->deleteLater();
        drag_object = object = 0;
    }

#ifndef QT_NO_CURSOR
    if (restoreCursor) {
        QApplication::restoreOverrideCursor();
        restoreCursor = false;
    }
#endif

    global_accepted_action = Qt::IgnoreAction;
}


void QDragManager::drop()
{
#ifndef QT_NO_CURSOR
    if (restoreCursor) {
        QApplication::restoreOverrideCursor();
        restoreCursor = false;
    }
#endif
}

QVariant QDropData::retrieveData_sys(const QString &mimetype, QVariant::Type type) const
{
    if (!drag_object)
        return QVariant();
    QByteArray data =  drag_object->mimeData()->data(mimetype);
    if (type == QVariant::String)
        return QString::fromUtf8(data);
    return data;
}

bool QDropData::hasFormat_sys(const QString &format) const
{
    return formats().contains(format);
}

QStringList QDropData::formats_sys() const
{
    if (drag_object)
        return drag_object->mimeData()->formats();
    return QStringList();
}

QT_END_NAMESPACE
#endif // QT_NO_DRAGANDDROP
