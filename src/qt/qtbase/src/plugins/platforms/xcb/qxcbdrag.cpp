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

#include "qxcbdrag.h"
#include <xcb/xcb.h>
#include "qxcbconnection.h"
#include "qxcbclipboard.h"
#include "qxcbmime.h"
#include "qxcbwindow.h"
#include "qxcbscreen.h"
#include "qwindow.h"
#include <private/qdnd_p.h>
#include <qdebug.h>
#include <qevent.h>
#include <qguiapplication.h>
#include <qrect.h>
#include <qpainter.h>
#include <qtimer.h>

#include <qpa/qwindowsysteminterface.h>

#include <private/qshapedpixmapdndwindow_p.h>
#include <private/qsimpledrag_p.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_DRAGANDDROP

//#define DND_DEBUG
#ifdef DND_DEBUG
#define DEBUG qDebug
#else
#define DEBUG if(0) qDebug
#endif

#ifdef DND_DEBUG
#define DNDDEBUG qDebug()
#else
#define DNDDEBUG if(0) qDebug()
#endif

const int xdnd_version = 5;

static inline xcb_window_t xcb_window(QWindow *w)
{
    return static_cast<QXcbWindow *>(w->handle())->xcb_window();
}


static xcb_window_t xdndProxy(QXcbConnection *c, xcb_window_t w)
{
    xcb_window_t proxy = XCB_NONE;

    xcb_get_property_cookie_t cookie = Q_XCB_CALL2(xcb_get_property(c->xcb_connection(), false, w, c->atom(QXcbAtom::XdndProxy),
                                                        XCB_ATOM_WINDOW, 0, 1), c);
    xcb_get_property_reply_t *reply = xcb_get_property_reply(c->xcb_connection(), cookie, 0);

    if (reply && reply->type == XCB_ATOM_WINDOW)
        proxy = *((xcb_window_t *)xcb_get_property_value(reply));
    free(reply);

    if (proxy == XCB_NONE)
        return proxy;

    // exists and is real?
    cookie = Q_XCB_CALL2(xcb_get_property(c->xcb_connection(), false, proxy, c->atom(QXcbAtom::XdndProxy),
                                                        XCB_ATOM_WINDOW, 0, 1), c);
    reply = xcb_get_property_reply(c->xcb_connection(), cookie, 0);

    if (reply && reply->type == XCB_ATOM_WINDOW) {
        xcb_window_t p = *((xcb_window_t *)xcb_get_property_value(reply));
        if (proxy != p)
            proxy = 0;
    } else {
        proxy = 0;
    }

    free(reply);

    return proxy;
}

class QXcbDropData : public QXcbMime
{
public:
    QXcbDropData(QXcbDrag *d);
    ~QXcbDropData();

protected:
    bool hasFormat_sys(const QString &mimeType) const;
    QStringList formats_sys() const;
    QVariant retrieveData_sys(const QString &mimeType, QVariant::Type type) const;

    QVariant xdndObtainData(const QByteArray &format, QVariant::Type requestedType) const;

    QXcbDrag *drag;
};


QXcbDrag::QXcbDrag(QXcbConnection *c) : QXcbObject(c)
{
    dropData = new QXcbDropData(this);

    init();
    cleanup_timer = -1;
}

QXcbDrag::~QXcbDrag()
{
    delete dropData;
}

void QXcbDrag::init()
{
    currentWindow.clear();

    accepted_drop_action = Qt::IgnoreAction;

    xdnd_dragsource = XCB_NONE;

    waiting_for_status = false;
    current_target = XCB_NONE;
    current_proxy_target = XCB_NONE;

    source_time = XCB_CURRENT_TIME;
    target_time = XCB_CURRENT_TIME;

    current_screen = 0;
    drag_types.clear();
}

QMimeData *QXcbDrag::platformDropData()
{
    return dropData;
}

void QXcbDrag::startDrag()
{
    // #fixme enableEventFilter();

    init();

    xcb_set_selection_owner(xcb_connection(), connection()->clipboard()->owner(),
                            atom(QXcbAtom::XdndSelection), connection()->time());

    QStringList fmts = QXcbMime::formatsHelper(drag()->mimeData());
    for (int i = 0; i < fmts.size(); ++i) {
        QList<xcb_atom_t> atoms = QXcbMime::mimeAtomsForFormat(connection(), fmts.at(i));
        for (int j = 0; j < atoms.size(); ++j) {
            if (!drag_types.contains(atoms.at(j)))
                drag_types.append(atoms.at(j));
        }
    }
    if (drag_types.size() > 3)
        xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, connection()->clipboard()->owner(),
                            atom(QXcbAtom::XdndTypelist),
                            XCB_ATOM_ATOM, 32, drag_types.size(), (const void *)drag_types.constData());
    QBasicDrag::startDrag();
}

void QXcbDrag::endDrag()
{
    QBasicDrag::endDrag();
}

static xcb_translate_coordinates_reply_t *
translateCoordinates(QXcbConnection *c, xcb_window_t from, xcb_window_t to, int x, int y)
{
    xcb_translate_coordinates_cookie_t cookie =
            xcb_translate_coordinates(c->xcb_connection(), from, to, x, y);
    return xcb_translate_coordinates_reply(c->xcb_connection(), cookie, 0);
}

static
bool windowInteractsWithPosition(xcb_connection_t *connection, const QPoint & pos, xcb_window_t w, xcb_shape_sk_t shapeType)
{
    bool interacts = false;
    xcb_shape_get_rectangles_reply_t *reply = xcb_shape_get_rectangles_reply(connection, xcb_shape_get_rectangles(connection, w, shapeType), NULL);
    if (reply) {
        xcb_rectangle_t *rectangles = xcb_shape_get_rectangles_rectangles(reply);
        if (rectangles) {
            const int nRectangles = xcb_shape_get_rectangles_rectangles_length(reply);
            for (int i = 0; !interacts && i < nRectangles; ++i) {
                interacts = QRect(rectangles[i].x, rectangles[i].y, rectangles[i].width, rectangles[i].height).contains(pos);
            }
        }
        free(reply);
    }

    return interacts;
}

xcb_window_t QXcbDrag::findRealWindow(const QPoint & pos, xcb_window_t w, int md, bool ignoreNonXdndAwareWindows)
{
    if (w == shapedPixmapWindow()->handle()->winId())
        return 0;

    if (md) {
        xcb_get_window_attributes_cookie_t cookie = xcb_get_window_attributes(xcb_connection(), w);
        xcb_get_window_attributes_reply_t *reply = xcb_get_window_attributes_reply(xcb_connection(), cookie, 0);
        if (!reply)
            return 0;

        if (reply->map_state != XCB_MAP_STATE_VIEWABLE)
            return 0;

        xcb_get_geometry_cookie_t gcookie = xcb_get_geometry(xcb_connection(), w);
        xcb_get_geometry_reply_t *greply = xcb_get_geometry_reply(xcb_connection(), gcookie, 0);
        if (reply && QRect(greply->x, greply->y, greply->width, greply->height).contains(pos)) {
            bool windowContainsMouse = !ignoreNonXdndAwareWindows;
            {
                xcb_get_property_cookie_t cookie =
                        Q_XCB_CALL(xcb_get_property(xcb_connection(), false, w, connection()->atom(QXcbAtom::XdndAware),
                                                    XCB_GET_PROPERTY_TYPE_ANY, 0, 0));
                xcb_get_property_reply_t *reply = xcb_get_property_reply(xcb_connection(), cookie, 0);

                bool isAware = reply && reply->type != XCB_NONE;
                free(reply);
                if (isAware) {
                    const QPoint relPos = pos - QPoint(greply->x, greply->y);
                    // When ShapeInput and ShapeBounding are not set they return a single rectangle with the geometry of the window, this is why we
                    // need to check both here so that in the case one is set and the other is not we still get the correct result.
                    if (connection()->hasInputShape())
                        windowContainsMouse = windowInteractsWithPosition(xcb_connection(), relPos, w, XCB_SHAPE_SK_INPUT);
                    if (windowContainsMouse && connection()->hasXShape())
                        windowContainsMouse = windowInteractsWithPosition(xcb_connection(), relPos, w, XCB_SHAPE_SK_BOUNDING);
                    if (!connection()->hasInputShape() && !connection()->hasXShape())
                        windowContainsMouse = true;
                    if (windowContainsMouse)
                        return w;
                }
            }

            xcb_query_tree_cookie_t cookie = xcb_query_tree (xcb_connection(), w);
            xcb_query_tree_reply_t *reply = xcb_query_tree_reply(xcb_connection(), cookie, 0);

            if (!reply)
                return 0;
            int nc = xcb_query_tree_children_length(reply);
            xcb_window_t *c = xcb_query_tree_children(reply);

            xcb_window_t r = 0;
            for (uint i = nc; !r && i--;)
                r = findRealWindow(pos - QPoint(greply->x, greply->y), c[i], md-1, ignoreNonXdndAwareWindows);

            free(reply);
            if (r)
                return r;

            // We didn't find a client window!  Just use the
            // innermost window.

            // No children!
            if (!windowContainsMouse)
                return 0;
            else
                return w;
        }
    }
    return 0;
}

void QXcbDrag::move(const QMouseEvent *me)
{
    QBasicDrag::move(me);
    QPoint globalPos = me->globalPos();

    if (source_sameanswer.contains(globalPos) && source_sameanswer.isValid())
        return;

    const QList<QXcbScreen *> &screens = connection()->screens();
    QXcbScreen *screen = screens.at(connection()->primaryScreen());
    for (int i = 0; i < screens.size(); ++i) {
        if (screens.at(i)->geometry().contains(globalPos)) {
            screen = screens.at(i);
            break;
        }
    }
    if (screen != current_screen) {
        // ### need to recreate the shaped pixmap window?
//    int screen = QCursor::x11Screen();
//    if ((qt_xdnd_current_screen == -1 && screen != X11->defaultScreen) || (screen != qt_xdnd_current_screen)) {
//        // recreate the pixmap on the new screen...
//        delete xdnd_data.deco;
//        QWidget* parent = object->source()->window()->x11Info().screen() == screen
//            ? object->source()->window() : QApplication::desktop()->screen(screen);
//        xdnd_data.deco = new QShapedPixmapWidget(parent);
//        if (!QWidget::mouseGrabber()) {
//            updatePixmap();
//            xdnd_data.deco->grabMouse();
//        }
//    }
//    xdnd_data.deco->move(QCursor::pos() - xdnd_data.deco->pm_hot);
        current_screen = screen;
    }


//    qt_xdnd_current_screen = screen;
    xcb_window_t rootwin = current_screen->root();
    xcb_translate_coordinates_reply_t *translate =
            ::translateCoordinates(connection(), rootwin, rootwin, globalPos.x(), globalPos.y());
    if (!translate)
        return;

    xcb_window_t target = translate->child;
    int lx = translate->dst_x;
    int ly = translate->dst_y;
    free (translate);

    if (target && target != rootwin) {
        xcb_window_t src = rootwin;
        while (target != 0) {
            DNDDEBUG << "checking target for XdndAware" << target << lx << ly;

            // translate coordinates
            translate = ::translateCoordinates(connection(), src, target, lx, ly);
            if (!translate) {
                target = 0;
                break;
            }
            lx = translate->dst_x;
            ly = translate->dst_y;
            src = target;
            xcb_window_t child = translate->child;
            free(translate);

            // check if it has XdndAware
            xcb_get_property_cookie_t cookie = Q_XCB_CALL(xcb_get_property(xcb_connection(), false, target,
                                                          atom(QXcbAtom::XdndAware), XCB_GET_PROPERTY_TYPE_ANY, 0, 0));
            xcb_get_property_reply_t *reply = xcb_get_property_reply(xcb_connection(), cookie, 0);
            bool aware = reply && reply->type != XCB_NONE;
            free(reply);
            if (aware) {
                DNDDEBUG << "Found XdndAware on " << target;
                break;
            }

            target = child;
        }

        if (!target || target == shapedPixmapWindow()->handle()->winId()) {
            DNDDEBUG << "need to find real window";
            target = findRealWindow(globalPos, rootwin, 6, true);
            if (target == 0)
                target = findRealWindow(globalPos, rootwin, 6, false);
            DNDDEBUG << "real window found" << target;
        }
    }

    QXcbWindow *w = 0;
    if (target) {
        w = connection()->platformWindowFromId(target);
        if (w && (w->window()->type() == Qt::Desktop) /*&& !w->acceptDrops()*/)
            w = 0;
    } else {
        w = 0;
        target = rootwin;
    }

    xcb_window_t proxy_target = xdndProxy(connection(), target);
    if (!proxy_target)
        proxy_target = target;
    int target_version = 1;

    if (proxy_target) {
        xcb_get_property_cookie_t cookie = xcb_get_property(xcb_connection(), false, target,
                                                            atom(QXcbAtom::XdndAware), XCB_GET_PROPERTY_TYPE_ANY, 0, 1);
        xcb_get_property_reply_t *reply = xcb_get_property_reply(xcb_connection(), cookie, 0);
        if (!reply || reply->type == XCB_NONE)
            target = 0;

        target_version = *(uint32_t *)xcb_get_property_value(reply);
        target_version = qMin(xdnd_version, target_version ? target_version : 1);

        free(reply);
    }

    if (target != current_target) {
        if (current_target)
            send_leave();

        current_target = target;
        current_proxy_target = proxy_target;
        if (target) {
            int flags = target_version << 24;
            if (drag_types.size() > 3)
                flags |= 0x0001;

            xcb_client_message_event_t enter;
            enter.response_type = XCB_CLIENT_MESSAGE;
            enter.window = target;
            enter.format = 32;
            enter.type = atom(QXcbAtom::XdndEnter);
            enter.data.data32[0] = connection()->clipboard()->owner();
            enter.data.data32[1] = flags;
            enter.data.data32[2] = drag_types.size()>0 ? drag_types.at(0) : 0;
            enter.data.data32[3] = drag_types.size()>1 ? drag_types.at(1) : 0;
            enter.data.data32[4] = drag_types.size()>2 ? drag_types.at(2) : 0;
            // provisionally set the rectangle to 5x5 pixels...
            source_sameanswer = QRect(globalPos.x() - 2, globalPos.y() -2 , 5, 5);

            DEBUG() << "sending Xdnd enter source=" << enter.data.data32[0];
            if (w)
                handleEnter(w->window(), &enter);
            else if (target)
                xcb_send_event(xcb_connection(), false, proxy_target, XCB_EVENT_MASK_NO_EVENT, (const char *)&enter);
            waiting_for_status = false;
        }
    }

    if (waiting_for_status)
        return;

    if (target) {
        waiting_for_status = true;

        xcb_client_message_event_t move;
        move.response_type = XCB_CLIENT_MESSAGE;
        move.window = target;
        move.format = 32;
        move.type = atom(QXcbAtom::XdndPosition);
        move.data.data32[0] = connection()->clipboard()->owner();
        move.data.data32[1] = 0; // flags
        move.data.data32[2] = (globalPos.x() << 16) + globalPos.y();
        move.data.data32[3] = connection()->time();
        move.data.data32[4] = toXdndAction(defaultAction(currentDrag()->supportedActions(), QGuiApplication::keyboardModifiers()));
        DEBUG() << "sending Xdnd position source=" << move.data.data32[0] << "target=" << move.window;

        source_time = connection()->time();

        if (w)
            handle_xdnd_position(w->window(), &move);
        else
            xcb_send_event(xcb_connection(), false, proxy_target, XCB_EVENT_MASK_NO_EVENT, (const char *)&move);
    }
}

void QXcbDrag::drop(const QMouseEvent *event)
{
    QBasicDrag::drop(event);

    if (!current_target)
        return;

    xcb_client_message_event_t drop;
    drop.response_type = XCB_CLIENT_MESSAGE;
    drop.window = current_target;
    drop.format = 32;
    drop.type = atom(QXcbAtom::XdndDrop);
    drop.data.data32[0] = connection()->clipboard()->owner();
    drop.data.data32[1] = 0; // flags
    drop.data.data32[2] = connection()->time();

    drop.data.data32[3] = 0;
    drop.data.data32[4] = currentDrag()->supportedActions();

    QXcbWindow *w = connection()->platformWindowFromId(current_proxy_target);

    if (w && (w->window()->type() == Qt::Desktop) /*&& !w->acceptDrops()*/)
        w = 0;

    Transaction t = {
        connection()->time(),
        current_target,
        current_proxy_target,
        (w ? w->window() : 0),
//        current_embeddig_widget,
        currentDrag(),
        QTime::currentTime()
    };
    transactions.append(t);

    // timer is needed only for drops that came from other processes.
    if (!t.targetWindow && cleanup_timer == -1) {
        cleanup_timer = startTimer(XdndDropTransactionTimeout);
    }

    if (w) {
        handleDrop(w->window(), &drop);
    } else {
        xcb_send_event(xcb_connection(), false, current_proxy_target, XCB_EVENT_MASK_NO_EVENT, (const char *)&drop);
    }

    current_target = 0;
    current_proxy_target = 0;
    source_time = 0;
//    current_embedding_widget = 0;
}

Qt::DropAction QXcbDrag::toDropAction(xcb_atom_t a) const
{
    if (a == atom(QXcbAtom::XdndActionCopy) || a == 0)
        return Qt::CopyAction;
    if (a == atom(QXcbAtom::XdndActionLink))
        return Qt::LinkAction;
    if (a == atom(QXcbAtom::XdndActionMove))
        return Qt::MoveAction;
    return Qt::CopyAction;
}

xcb_atom_t QXcbDrag::toXdndAction(Qt::DropAction a) const
{
    switch (a) {
    case Qt::CopyAction:
        return atom(QXcbAtom::XdndActionCopy);
    case Qt::LinkAction:
        return atom(QXcbAtom::XdndActionLink);
    case Qt::MoveAction:
    case Qt::TargetMoveAction:
        return atom(QXcbAtom::XdndActionMove);
    case Qt::IgnoreAction:
        return XCB_NONE;
    default:
        return atom(QXcbAtom::XdndActionCopy);
    }
}

int QXcbDrag::findTransactionByWindow(xcb_window_t window)
{
    int at = -1;
    for (int i = 0; i < transactions.count(); ++i) {
        const Transaction &t = transactions.at(i);
        if (t.target == window || t.proxy_target == window) {
            at = i;
            break;
        }
    }
    return at;
}

int QXcbDrag::findTransactionByTime(xcb_timestamp_t timestamp)
{
    int at = -1;
    for (int i = 0; i < transactions.count(); ++i) {
        const Transaction &t = transactions.at(i);
        if (t.timestamp == timestamp) {
            at = i;
            break;
        }
    }
    return at;
}

#if 0

// find an ancestor with XdndAware on it
static Window findXdndAwareParent(Window window)
{
    Window target = 0;
    forever {
        // check if window has XdndAware
        Atom type = 0;
        int f;
        unsigned long n, a;
        unsigned char *data = 0;
        if (XGetWindowProperty(X11->display, window, ATOM(XdndAware), 0, 0, False,
                               AnyPropertyType, &type, &f,&n,&a,&data) == Success) {
            if (data)
                XFree(data);
            if (type) {
                target = window;
                break;
            }
        }

        // try window's parent
        Window root;
        Window parent;
        Window *children;
        uint unused;
        if (!XQueryTree(X11->display, window, &root, &parent, &children, &unused))
            break;
        if (children)
            XFree(children);
        if (window == root)
            break;
        window = parent;
    }
    return target;
}


// for embedding only
static QWidget* current_embedding_widget  = 0;
static xcb_client_message_event_t last_enter_event;


static bool checkEmbedded(QWidget* w, const XEvent* xe)
{
    if (!w)
        return false;

    if (current_embedding_widget != 0 && current_embedding_widget != w) {
        current_target = ((QExtraWidget*)current_embedding_widget)->extraData()->xDndProxy;
        current_proxy_target = current_target;
        qt_xdnd_send_leave();
        current_target = 0;
        current_proxy_target = 0;
        current_embedding_widget = 0;
    }

    QWExtra* extra = ((QExtraWidget*)w)->extraData();
    if (extra && extra->xDndProxy != 0) {

        if (current_embedding_widget != w) {

            last_enter_event.xany.window = extra->xDndProxy;
            XSendEvent(X11->display, extra->xDndProxy, False, NoEventMask, &last_enter_event);
            current_embedding_widget = w;
        }

        ((XEvent*)xe)->xany.window = extra->xDndProxy;
        XSendEvent(X11->display, extra->xDndProxy, False, NoEventMask, (XEvent*)xe);
        if (currentWindow != w) {
            currentWindow = w;
        }
        return true;
    }
    current_embedding_widget = 0;
    return false;
}
#endif


void QXcbDrag::handleEnter(QWindow *window, const xcb_client_message_event_t *event)
{
    Q_UNUSED(window);
    DEBUG() << "handleEnter" << window;

    xdnd_types.clear();

    int version = (int)(event->data.data32[1] >> 24);
    if (version > xdnd_version)
        return;

    xdnd_dragsource = event->data.data32[0];

    if (event->data.data32[1] & 1) {
        // get the types from XdndTypeList
        xcb_get_property_cookie_t cookie = xcb_get_property(xcb_connection(), false, xdnd_dragsource,
                                                            atom(QXcbAtom::XdndTypelist), XCB_ATOM_ATOM,
                                                            0, xdnd_max_type);
        xcb_get_property_reply_t *reply = xcb_get_property_reply(xcb_connection(), cookie, 0);
        if (reply && reply->type != XCB_NONE && reply->format == 32) {
            int length = xcb_get_property_value_length(reply) / 4;
            if (length > xdnd_max_type)
                length = xdnd_max_type;

            xcb_atom_t *atoms = (xcb_atom_t *)xcb_get_property_value(reply);
            for (int i = 0; i < length; ++i)
                xdnd_types.append(atoms[i]);
        }
        free(reply);
    } else {
        // get the types from the message
        for(int i = 2; i < 5; i++) {
            if (event->data.data32[i])
                xdnd_types.append(event->data.data32[i]);
        }
    }
    for(int i = 0; i < xdnd_types.length(); ++i)
        DEBUG() << "    " << connection()->atomName(xdnd_types.at(i));
}

void QXcbDrag::handle_xdnd_position(QWindow *w, const xcb_client_message_event_t *e)
{
    QPoint p((e->data.data32[2] & 0xffff0000) >> 16, e->data.data32[2] & 0x0000ffff);
    Q_ASSERT(w);
    QRect geometry = w->geometry();

    p -= geometry.topLeft();

    if (!w || (w->type() == Qt::Desktop))
        return;

    if (e->data.data32[0] != xdnd_dragsource) {
        DEBUG("xdnd drag position from unexpected source (%x not %x)", e->data.data32[0], xdnd_dragsource);
        return;
    }

    currentPosition = p;
    currentWindow = w;

    // timestamp from the source
    if (e->data.data32[3] != XCB_NONE) {
        target_time = e->data.data32[3];
    }

    QMimeData *dropData = 0;
    Qt::DropActions supported_actions = Qt::IgnoreAction;
    if (currentDrag()) {
        dropData = currentDrag()->mimeData();
        supported_actions = currentDrag()->supportedActions();
    } else {
        dropData = platformDropData();
        supported_actions = Qt::DropActions(toDropAction(e->data.data32[4]));
    }

    QPlatformDragQtResponse qt_response = QWindowSystemInterface::handleDrag(w,dropData,p,supported_actions);
    QRect answerRect(p + geometry.topLeft(), QSize(1,1));
    answerRect = qt_response.answerRect().translated(geometry.topLeft()).intersected(geometry);

    xcb_client_message_event_t response;
    response.response_type = XCB_CLIENT_MESSAGE;
    response.window = xdnd_dragsource;
    response.format = 32;
    response.type = atom(QXcbAtom::XdndStatus);
    response.data.data32[0] = xcb_window(w);
    response.data.data32[1] = qt_response.isAccepted(); // flags
    response.data.data32[2] = 0; // x, y
    response.data.data32[3] = 0; // w, h
    response.data.data32[4] = toXdndAction(qt_response.acceptedAction()); // action

    accepted_drop_action = qt_response.acceptedAction();

    if (answerRect.left() < 0)
        answerRect.setLeft(0);
    if (answerRect.right() > 4096)
        answerRect.setRight(4096);
    if (answerRect.top() < 0)
        answerRect.setTop(0);
    if (answerRect.bottom() > 4096)
        answerRect.setBottom(4096);
    if (answerRect.width() < 0)
        answerRect.setWidth(0);
    if (answerRect.height() < 0)
        answerRect.setHeight(0);

    // reset
    target_time = XCB_CURRENT_TIME;

    if (xdnd_dragsource == connection()->clipboard()->owner())
        handle_xdnd_status(&response);
    else
        Q_XCB_CALL(xcb_send_event(xcb_connection(), false, xdnd_dragsource,
                                  XCB_EVENT_MASK_NO_EVENT, (const char *)&response));
}

namespace
{
    class ClientMessageScanner {
    public:
        ClientMessageScanner(xcb_atom_t a) : atom(a) {}
        xcb_atom_t atom;
        bool checkEvent(xcb_generic_event_t *event) const {
            if (!event)
                return false;
            if ((event->response_type & 0x7f) != XCB_CLIENT_MESSAGE)
                return false;
            return ((xcb_client_message_event_t *)event)->type == atom;
        }
    };
}

void QXcbDrag::handlePosition(QWindow * w, const xcb_client_message_event_t *event)
{
    xcb_client_message_event_t *lastEvent = const_cast<xcb_client_message_event_t *>(event);
    xcb_generic_event_t *nextEvent;
    ClientMessageScanner scanner(atom(QXcbAtom::XdndPosition));
    while ((nextEvent = connection()->checkEvent(scanner))) {
        if (lastEvent != event)
            free(lastEvent);
        lastEvent = (xcb_client_message_event_t *)nextEvent;
    }

    handle_xdnd_position(w, lastEvent);
    if (lastEvent != event)
        free(lastEvent);
}

void QXcbDrag::handle_xdnd_status(const xcb_client_message_event_t *event)
{
    DEBUG("xdndHandleStatus");
    waiting_for_status = false;
    // ignore late status messages
    if (event->data.data32[0] && event->data.data32[0] != current_proxy_target)
        return;

    const bool dropPossible = event->data.data32[1];
    setCanDrop(dropPossible);

    if (dropPossible) {
        accepted_drop_action = toDropAction(event->data.data32[4]);
        updateCursor(accepted_drop_action);
    } else {
        updateCursor(Qt::IgnoreAction);
    }

    if ((event->data.data32[1] & 2) == 0) {
        QPoint p((event->data.data32[2] & 0xffff0000) >> 16, event->data.data32[2] & 0x0000ffff);
        QSize s((event->data.data32[3] & 0xffff0000) >> 16, event->data.data32[3] & 0x0000ffff);
        source_sameanswer = QRect(p, s);
    } else {
        source_sameanswer = QRect();
    }
}

void QXcbDrag::handleStatus(const xcb_client_message_event_t *event)
{
    if (event->window != connection()->clipboard()->owner() || !drag())
        return;

    xcb_client_message_event_t *lastEvent = const_cast<xcb_client_message_event_t *>(event);
    xcb_generic_event_t *nextEvent;
    ClientMessageScanner scanner(atom(QXcbAtom::XdndStatus));
    while ((nextEvent = connection()->checkEvent(scanner))) {
        if (lastEvent != event)
            free(lastEvent);
        lastEvent = (xcb_client_message_event_t *)nextEvent;
    }

    handle_xdnd_status(lastEvent);
    if (lastEvent != event)
        free(lastEvent);
    DEBUG("xdndHandleStatus end");
}

void QXcbDrag::handleLeave(QWindow *w, const xcb_client_message_event_t *event)
{
    DEBUG("xdnd leave");
    if (!currentWindow || w != currentWindow.data())
        return; // sanity

    // ###
//    if (checkEmbedded(current_embedding_widget, event)) {
//        current_embedding_widget = 0;
//        currentWindow.clear();
//        return;
//    }

    if (event->data.data32[0] != xdnd_dragsource) {
        // This often happens - leave other-process window quickly
        DEBUG("xdnd drag leave from unexpected source (%x not %x", event->data.data32[0], xdnd_dragsource);
    }

    QWindowSystemInterface::handleDrag(w,0,QPoint(),Qt::IgnoreAction);

    xdnd_dragsource = 0;
    xdnd_types.clear();
    currentWindow.clear();
}

void QXcbDrag::send_leave()
{
    if (!current_target)
        return;


    xcb_client_message_event_t leave;
    leave.response_type = XCB_CLIENT_MESSAGE;
    leave.window = current_target;
    leave.format = 32;
    leave.type = atom(QXcbAtom::XdndLeave);
    leave.data.data32[0] = connection()->clipboard()->owner();
    leave.data.data32[1] = 0; // flags
    leave.data.data32[2] = 0; // x, y
    leave.data.data32[3] = 0; // w, h
    leave.data.data32[4] = 0; // just null

    QXcbWindow *w = connection()->platformWindowFromId(current_proxy_target);

    if (w && (w->window()->type() == Qt::Desktop) /*&& !w->acceptDrops()*/)
        w = 0;

    if (w)
        handleLeave(w->window(), (const xcb_client_message_event_t *)&leave);
    else
        xcb_send_event(xcb_connection(), false,current_proxy_target,
                       XCB_EVENT_MASK_NO_EVENT, (const char *)&leave);

    current_target = 0;
    current_proxy_target = 0;
    source_time = XCB_CURRENT_TIME;
    waiting_for_status = false;
}

void QXcbDrag::handleDrop(QWindow *, const xcb_client_message_event_t *event)
{
    DEBUG("xdndHandleDrop");
    if (!currentWindow) {
        xdnd_dragsource = 0;
        return; // sanity
    }

    const uint32_t *l = event->data.data32;

    DEBUG("xdnd drop");

    if (l[0] != xdnd_dragsource) {
        DEBUG("xdnd drop from unexpected source (%x not %x", l[0], xdnd_dragsource);
        return;
    }

    // update the "user time" from the timestamp in the event.
    if (l[2] != 0)
        target_time = /*X11->userTime =*/ l[2];

    Qt::DropActions supported_drop_actions;
    QMimeData *dropData = 0;
    if (currentDrag()) {
        dropData = currentDrag()->mimeData();
        supported_drop_actions = Qt::DropActions(l[4]);
    } else {
        dropData = platformDropData();
        supported_drop_actions = accepted_drop_action;
    }

    if (!dropData)
        return;
    // ###
    //        int at = findXdndDropTransactionByTime(target_time);
    //        if (at != -1)
    //            dropData = QDragManager::dragPrivate(X11->dndDropTransactions.at(at).object)->data;
    // if we can't find it, then use the data in the drag manager

    QPlatformDropQtResponse response = QWindowSystemInterface::handleDrop(currentWindow.data(),dropData,currentPosition,supported_drop_actions);
    setExecutedDropAction(response.acceptedAction());

    xcb_client_message_event_t finished;
    finished.response_type = XCB_CLIENT_MESSAGE;
    finished.window = xdnd_dragsource;
    finished.format = 32;
    finished.type = atom(QXcbAtom::XdndFinished);
    finished.data.data32[0] = currentWindow ? xcb_window(currentWindow.data()) : XCB_NONE;
    finished.data.data32[1] = response.isAccepted(); // flags
    finished.data.data32[2] = toXdndAction(response.acceptedAction());
    Q_XCB_CALL(xcb_send_event(xcb_connection(), false, xdnd_dragsource,
                              XCB_EVENT_MASK_NO_EVENT, (char *)&finished));

    xdnd_dragsource = 0;
    currentWindow.clear();
    waiting_for_status = false;

    // reset
    target_time = XCB_CURRENT_TIME;
}


void QXcbDrag::handleFinished(const xcb_client_message_event_t *event)
{
    DEBUG("xdndHandleFinished");
    if (event->window != connection()->clipboard()->owner())
        return;

    const unsigned long *l = (const unsigned long *)event->data.data32;

    DNDDEBUG << "xdndHandleFinished, l[0]" << l[0]
             << "current_target" << current_target
             << "qt_xdnd_current_proxy_targe" << current_proxy_target;

    if (l[0]) {
        int at = findTransactionByWindow(l[0]);
        if (at != -1) {

            Transaction t = transactions.takeAt(at);
//            QDragManager *manager = QDragManager::self();

//            Window target = current_target;
//            Window proxy_target = current_proxy_target;
//            QWidget *embedding_widget = current_embedding_widget;
//            QDrag *currentObject = manager->object;

//            current_target = t.target;
//            current_proxy_target = t.proxy_target;
//            current_embedding_widget = t.embedding_widget;
//            manager->object = t.object;

//            if (!passive)
//                (void) checkEmbedded(currentWindow, xe);

//            current_embedding_widget = 0;
//            current_target = 0;
//            current_proxy_target = 0;

            if (t.drag)
                t.drag->deleteLater();

//            current_target = target;
//            current_proxy_target = proxy_target;
//            current_embedding_widget = embedding_widget;
//            manager->object = currentObject;
        } else {
            qWarning("QXcbDrag::handleFinished - drop data has expired");
        }
    }
    waiting_for_status = false;
}

void QXcbDrag::timerEvent(QTimerEvent* e)
{
    if (e->timerId() == cleanup_timer) {
        bool stopTimer = true;
        for (int i = 0; i < transactions.count(); ++i) {
            const Transaction &t = transactions.at(i);
            if (t.targetWindow) {
                // dnd within the same process, don't delete, these are taken care of
                // in handleFinished()
                continue;
            }
            QTime currentTime = QTime::currentTime();
            int delta = t.time.msecsTo(currentTime);
            if (delta > XdndDropTransactionTimeout) {
                /* delete transactions which are older than XdndDropTransactionTimeout. It could mean
                 one of these:
                 - client has crashed and as a result we have never received XdndFinished
                 - showing dialog box on drop event where user's response takes more time than XdndDropTransactionTimeout (QTBUG-14493)
                 - dnd takes unusually long time to process data
                 */
                if (t.drag)
                    t.drag->deleteLater();
                transactions.removeAt(i--);
            } else {
                stopTimer = false;
            }

        }
        if (stopTimer && cleanup_timer != -1) {
            killTimer(cleanup_timer);
            cleanup_timer = -1;
        }
    }
}

void QXcbDrag::cancel()
{
    DEBUG("QXcbDrag::cancel");
    QBasicDrag::cancel();
    if (current_target)
        send_leave();
}


void QXcbDrag::handleSelectionRequest(const xcb_selection_request_event_t *event)
{
    xcb_selection_notify_event_t notify;
    notify.response_type = XCB_SELECTION_NOTIFY;
    notify.requestor = event->requestor;
    notify.selection = event->selection;
    notify.target = XCB_NONE;
    notify.property = XCB_NONE;
    notify.time = event->time;

    // which transaction do we use? (note: -2 means use current currentDrag())
    int at = -1;

    // figure out which data the requestor is really interested in
    if (currentDrag() && event->time == source_time) {
        // requestor wants the current drag data
        at = -2;
    } else {
        // if someone has requested data in response to XdndDrop, find the corresponding transaction. the
        // spec says to call xcb_convert_selection() using the timestamp from the XdndDrop
        at = findTransactionByTime(event->time);
        if (at == -1) {
            // no dice, perhaps the client was nice enough to use the same window id in
            // xcb_convert_selection() that we sent the XdndDrop event to.
            at = findTransactionByWindow(event->requestor);
        }
//        if (at == -1 && event->time == XCB_CURRENT_TIME) {
//            // previous Qt versions always requested the data on a child of the target window
//            // using CurrentTime... but it could be asking for either drop data or the current drag's data
//            Window target = findXdndAwareParent(event->requestor);
//            if (target) {
//                if (current_target && current_target == target)
//                    at = -2;
//                else
//                    at = findXdndDropTransactionByWindow(target);
//            }
//        }
    }

    QDrag *transactionDrag = 0;
    if (at >= 0) {
        transactionDrag = transactions.at(at).drag;
    } else if (at == -2) {
        transactionDrag = currentDrag();
    }

    if (transactionDrag) {
        xcb_atom_t atomFormat = event->target;
        int dataFormat = 0;
        QByteArray data;
        if (QXcbMime::mimeDataForAtom(connection(), event->target, transactionDrag->mimeData(),
                                     &data, &atomFormat, &dataFormat)) {
            int dataSize = data.size() / (dataFormat / 8);
            xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, event->requestor, event->property,
                                atomFormat, dataFormat, dataSize, (const void *)data.constData());
            notify.property = event->property;
            notify.target = atomFormat;
        }
    }

    xcb_send_event(xcb_connection(), false, event->requestor, XCB_EVENT_MASK_NO_EVENT, (const char *)&notify);
}


bool QXcbDrag::dndEnable(QXcbWindow *w, bool on)
{
    DNDDEBUG << "xdndEnable" << w << on;
    if (on) {
        QXcbWindow *xdnd_widget = 0;
        if ((w->window()->type() == Qt::Desktop)) {
            if (desktop_proxy) // *WE* already have one.
                return false;

            QXcbConnectionGrabber grabber(connection());

            // As per Xdnd4, use XdndProxy
            xcb_window_t proxy_id = xdndProxy(connection(), w->xcb_window());

            if (!proxy_id) {
                desktop_proxy = new QWindow;
                xdnd_widget = static_cast<QXcbWindow *>(desktop_proxy->handle());
                proxy_id = xdnd_widget->xcb_window();
                xcb_atom_t xdnd_proxy = atom(QXcbAtom::XdndProxy);
                xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, w->xcb_window(), xdnd_proxy,
                                    XCB_ATOM_WINDOW, 32, 1, &proxy_id);
                xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, proxy_id, xdnd_proxy,
                                    XCB_ATOM_WINDOW, 32, 1, &proxy_id);
            }

        } else {
            xdnd_widget = w;
        }
        if (xdnd_widget) {
            DNDDEBUG << "setting XdndAware for" << xdnd_widget << xdnd_widget->xcb_window();
            xcb_atom_t atm = xdnd_version;
            xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, xdnd_widget->xcb_window(),
                                atom(QXcbAtom::XdndAware), XCB_ATOM_ATOM, 32, 1, &atm);
            return true;
        } else {
            return false;
        }
    } else {
        if ((w->window()->type() == Qt::Desktop)) {
            xcb_delete_property(xcb_connection(), w->xcb_window(), atom(QXcbAtom::XdndProxy));
            delete desktop_proxy;
            desktop_proxy = 0;
        } else {
            DNDDEBUG << "not deleting XDndAware";
        }
        return true;
    }
}

QXcbDropData::QXcbDropData(QXcbDrag *d)
    : QXcbMime(),
      drag(d)
{
}

QXcbDropData::~QXcbDropData()
{
}

QVariant QXcbDropData::retrieveData_sys(const QString &mimetype, QVariant::Type requestedType) const
{
    QByteArray mime = mimetype.toLatin1();
    QVariant data = xdndObtainData(mime, requestedType);
    return data;
}

QVariant QXcbDropData::xdndObtainData(const QByteArray &format, QVariant::Type requestedType) const
{
    QByteArray result;

    QXcbConnection *c = drag->connection();
    QXcbWindow *xcb_window = c->platformWindowFromId(drag->xdnd_dragsource);
    if (xcb_window && drag->currentDrag() && xcb_window->window()->type() != Qt::Desktop) {
        QMimeData *data = drag->currentDrag()->mimeData();
        if (data->hasFormat(QLatin1String(format)))
            result = data->data(QLatin1String(format));
        return result;
    }

    QList<xcb_atom_t> atoms = drag->xdnd_types;
    QByteArray encoding;
    xcb_atom_t a = mimeAtomForFormat(c, QLatin1String(format), requestedType, atoms, &encoding);
    if (a == XCB_NONE)
        return result;

    if (c->clipboard()->getSelectionOwner(drag->atom(QXcbAtom::XdndSelection)) == XCB_NONE)
        return result; // should never happen?

    xcb_atom_t xdnd_selection = c->atom(QXcbAtom::XdndSelection);
    result = c->clipboard()->getSelection(xdnd_selection, a, xdnd_selection, drag->targetTime());

    return mimeConvertToFormat(c, a, result, QLatin1String(format), requestedType, encoding);
}


bool QXcbDropData::hasFormat_sys(const QString &format) const
{
    return formats().contains(format);
}

QStringList QXcbDropData::formats_sys() const
{
    QStringList formats;
    for (int i = 0; i < drag->xdnd_types.size(); ++i) {
        QString f = mimeAtomToString(drag->connection(), drag->xdnd_types.at(i));
        if (!formats.contains(f))
            formats.append(f);
    }
    return formats;
}

#endif // QT_NO_DRAGANDDROP

QT_END_NAMESPACE
