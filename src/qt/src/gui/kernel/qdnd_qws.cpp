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

#include "qapplication.h"

#ifndef QT_NO_DRAGANDDROP

#include "qwidget.h"
#include "qdatetime.h"
#include "qbitmap.h"
#include "qcursor.h"
#include "qevent.h"
#include "qpainter.h"
#include "qdnd_p.h"

QT_BEGIN_NAMESPACE

QT_USE_NAMESPACE

static QPixmap *defaultPm = 0;
static const int default_pm_hotx = -2;
static const int default_pm_hoty = -16;
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

// Shift/Ctrl handling, and final drop status
static Qt::DropAction global_accepted_action = Qt::CopyAction;
static Qt::DropActions possible_actions = Qt::IgnoreAction;


// static variables in place of a proper cross-process solution
static QDrag *drag_object;
static bool qt_qws_dnd_dragging = false;


static Qt::KeyboardModifiers oldstate;

class QShapedPixmapWidget : public QWidget {
    QPixmap pixmap;
public:
    QShapedPixmapWidget() :
        QWidget(0, Qt::Tool | Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint)
    {
        // ### Temporary workaround for 4.2-rc1!!! To prevent flickering when
        // using drag'n drop in a client application. (task 126956)
        // setAttribute() should be done unconditionally!
        if (QApplication::type() == QApplication::GuiServer)
            setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    void setPixmap(QPixmap pm)
    {
        pixmap = pm;
        if (!pixmap.mask().isNull()) {
            setMask(pixmap.mask());
        } else {
            clearMask();
        }
        resize(pm.width(),pm.height());
    }

    void paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        p.drawPixmap(0,0,pixmap);
    }
};


static QShapedPixmapWidget *qt_qws_dnd_deco = 0;


void QDragManager::updatePixmap()
{
    if (qt_qws_dnd_deco) {
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
        qt_qws_dnd_deco->setPixmap(pm);
        qt_qws_dnd_deco->move(QCursor::pos()-pm_hot);
        if (willDrop) {
            qt_qws_dnd_deco->show();
        } else {
            qt_qws_dnd_deco->hide();
        }
    }
}

void QDragManager::timerEvent(QTimerEvent *) { }

void QDragManager::move(const QPoint &) { }

void QDragManager::updateCursor()
{
#ifndef QT_NO_CURSOR
    if (willDrop) {
        if (qt_qws_dnd_deco)
            qt_qws_dnd_deco->show();
        if (currentActionForOverrideCursor != global_accepted_action) {
            QApplication::changeOverrideCursor(QCursor(dragCursor(global_accepted_action), 0, 0));
            currentActionForOverrideCursor = global_accepted_action;
        }
    } else {
        QCursor *overrideCursor = QApplication::overrideCursor();
        if (!overrideCursor || overrideCursor->shape() != Qt::ForbiddenCursor) {
            QApplication::changeOverrideCursor(QCursor(Qt::ForbiddenCursor));
            currentActionForOverrideCursor = Qt::IgnoreAction;
        }
        if (qt_qws_dnd_deco)
            qt_qws_dnd_deco->hide();
    }
#endif
}


bool QDragManager::eventFilter(QObject *o, QEvent *e)
{
 if (beingCancelled) {
     if (e->type() == QEvent::KeyRelease && static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape) {
            qApp->removeEventFilter(this);
            Q_ASSERT(object == 0);
            beingCancelled = false;
            eventLoop->exit();
            return true; // block the key release
        }
        return false;
    }



    if (!o->isWidgetType())
        return false;

    switch(e->type()) {
        case QEvent::ShortcutOverride:
            // prevent accelerators from firing while dragging
            e->accept();
            return true;

        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        {
            QKeyEvent *ke = ((QKeyEvent*)e);
            if (ke->key() == Qt::Key_Escape && e->type() == QEvent::KeyPress) {
                cancel();
                qApp->removeEventFilter(this);
                beingCancelled = false;
                eventLoop->exit();
            } else {
                updateCursor();
            }
            return true; // Eat all key events
        }

        case QEvent::MouseButtonPress:
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

                // Fix for when we move mouse on to the deco widget
                if (qt_qws_dnd_deco && cw == qt_qws_dnd_deco)
                    cw = object->target();

                while (cw && !cw->acceptDrops() && !cw->isWindow())
                    cw = cw->parentWidget();

                if (object->target() != cw) {
                    if (object->target()) {
                        QDragLeaveEvent dle;
                        QApplication::sendEvent(object->target(), &dle);
                        willDrop = false;
                        global_accepted_action = Qt::IgnoreAction;
                        updateCursor();
                        restoreCursor = true;
                        object->d_func()->target = 0;
                    }
                    if (cw && cw->acceptDrops()) {
                        object->d_func()->target = cw;
                        QDragEnterEvent dee(cw->mapFromGlobal(me->globalPos()), possible_actions, dropData,
                                            me->buttons(), me->modifiers());
                        QApplication::sendEvent(object->target(), &dee);
                        willDrop = dee.isAccepted() && dee.dropAction() != Qt::IgnoreAction;
                        global_accepted_action = willDrop ? dee.dropAction() : Qt::IgnoreAction;
                        updateCursor();
                        restoreCursor = true;
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
                    updatePixmap();
                    updateCursor();
                }
                if (global_accepted_action != prevAction)
                    emitActionChanged(global_accepted_action);
            }
            return true; // Eat all mouse events
        }

        case QEvent::MouseButtonRelease:
        {
            qApp->removeEventFilter(this);
            if (restoreCursor) {
                willDrop = false;
#ifndef QT_NO_CURSOR
                QApplication::restoreOverrideCursor();
#endif
                restoreCursor = false;
            }
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
    if (object == o || !o || !o->source())
         return Qt::IgnoreAction;

    if (object) {
        cancel();
        qApp->removeEventFilter(this);
        beingCancelled = false;
    }

    object = drag_object = o;
    qt_qws_dnd_deco = new QShapedPixmapWidget();
    oldstate = Qt::NoModifier; // #### Should use state that caused the drag
//    drag_mode = mode;

    willDrop = false;
    updatePixmap();
    updateCursor();
    restoreCursor = true;
    object->d_func()->target = 0;
    qApp->installEventFilter(this);

    global_accepted_action = Qt::CopyAction;
#ifndef QT_NO_CURSOR
    qApp->setOverrideCursor(Qt::ArrowCursor);
    restoreCursor = true;
    updateCursor();
#endif

    qt_qws_dnd_dragging = true;

    eventLoop = new QEventLoop;
    (void) eventLoop->exec();
    delete eventLoop;
    eventLoop = 0;

    delete qt_qws_dnd_deco;
    qt_qws_dnd_deco = 0;
    qt_qws_dnd_dragging = false;


    return global_accepted_action;
}


void QDragManager::cancel(bool deleteSource)
{
//    qDebug("QDragManager::cancel");
    beingCancelled = true;

    if (object->target()) {
        QDragLeaveEvent dle;
        QApplication::sendEvent(object->target(), &dle);
    }

#ifndef QT_NO_CURSOR
    if (restoreCursor) {
        QApplication::restoreOverrideCursor();
        restoreCursor = false;
    }
#endif

    if (drag_object) {
        if (deleteSource)
            object->deleteLater();
        drag_object = object = 0;
    }

    delete qt_qws_dnd_deco;
    qt_qws_dnd_deco = 0;

    global_accepted_action = Qt::IgnoreAction;
}


void QDragManager::drop()
{
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


#endif // QT_NO_DRAGANDDROP


QT_END_NAMESPACE
