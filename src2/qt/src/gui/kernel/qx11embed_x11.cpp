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

#include "qplatformdefs.h"
#include "qx11embed_x11.h"
#include <qapplication.h>
#include <qevent.h>
#include <qpainter.h>
#include <qlayout.h>
#include <qstyle.h>
#include <qstyleoption.h>
#include <qelapsedtimer.h>
#include <qpointer.h>
#include <qdebug.h>
#include <qx11info_x11.h>
#include <private/qt_x11_p.h>
#include <private/qwidget_p.h>

#define XK_MISCELLANY
#define XK_LATIN1
#define None 0
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysymdef.h>
#include <X11/X.h>

#ifndef XK_ISO_Left_Tab
#define XK_ISO_Left_Tab 0xFE20
#endif

//#define QX11EMBED_DEBUG
#ifdef QX11EMBED_DEBUG
#include <qdebug.h>
#endif

QT_BEGIN_NAMESPACE

/*!
    \class QX11EmbedWidget
    \ingroup advanced

    \brief The QX11EmbedWidget class provides an XEmbed client widget.

    XEmbed is an X11 protocol that supports the embedding of a widget
    from one application into another application.

    An XEmbed \e{client widget} is a window that is embedded into a
    \e container. A container is the graphical location that embeds
    (or \e swallows) an external application.

    QX11EmbedWidget is a widget used for writing XEmbed applets or
    plugins. When it has been embedded and the container receives tab
    focus, focus is passed on to the widget. When the widget reaches
    the end of its focus chain, focus is passed back to the
    container. Window activation, accelerator support, modality and
    drag and drop (XDND) are also handled.

    The widget and container can both initiate the embedding. If the
    widget is the initiator, the X11 window ID of the container that
    it wants to embed itself into must be passed to embedInto().

    If the container initiates the embedding, the window ID of the
    embedded widget must be known. The container calls embed(),
    passing the window ID.

    This example shows an application that embeds a QX11EmbedWidget
    subclass into the window whose ID is passed as a command-line
    argument:

    \snippet doc/src/snippets/qx11embedwidget/main.cpp 0

    The problem of obtaining the window IDs is often solved by the
    container invoking the application that provides the widget as a
    separate process (as a panel invokes a docked applet), passing
    its window ID to the new process as a command-line argument. The
    new process can then call embedInto() with the container's window
    ID, as shown in the example code above. Similarly, the new
    process can report its window ID to the container through IPC, in
    which case the container can embed the widget.

    When the widget has been embedded, it emits the signal
    embedded(). If it is closed by the container, the widget emits
    containerClosed(). If an error occurs when embedding, error() is
    emitted.

    There are XEmbed widgets available for KDE and GTK+. The GTK+
    equivalent of QX11EmbedWidget is GtkPlug. The corresponding KDE 3
    widget is called QXEmbed.

    \sa QX11EmbedContainer, {XEmbed Specification}
*/

/*!
    \class QX11EmbedContainer
    \ingroup advanced

    \brief The QX11EmbedContainer class provides an XEmbed container
    widget.

    XEmbed is an X11 protocol that supports the embedding of a widget
    from one application into another application.

    An XEmbed \e container is the graphical location that embeds an
    external \e {client widget}. A client widget is a window that is
    embedded into a container.

    When a widget has been embedded and the container receives tab
    focus, focus is passed on to the widget. When the widget reaches
    the end of its focus chain, focus is passed back to the
    container. Window activation, accelerator support, modality and
    drag and drop (XDND) are also handled.

    QX11EmbedContainer is commonly used for writing panels or
    toolbars that hold applets, or for \e swallowing X11
    applications. When writing a panel application, one container
    widget is created on the toolbar, and it can then either swallow
    another widget using embed(), or allow an XEmbed widget to be
    embedded into itself. The container's X11 window ID, which is
    retrieved with winId(), must then be known to the client widget.
    After embedding, the client's window ID can be retrieved with
    clientWinId().

    In the following example, a container widget is created as the
    main widget. It then invokes an application called "playmovie",
    passing its window ID as a command line argument. The "playmovie"
    program is an XEmbed client widget. The widget embeds itself into
    the container using the container's window ID.

    \snippet doc/src/snippets/qx11embedcontainer/main.cpp 0

    When the client widget is embedded, the container emits the
    signal clientIsEmbedded(). The signal clientClosed() is emitted
    when a widget is closed.

    It is possible for QX11EmbedContainer to embed XEmbed widgets
    from toolkits other than Qt, such as GTK+. Arbitrary (non-XEmbed)
    X11 widgets can also be embedded, but the XEmbed-specific
    features such as window activation and focus handling are then
    lost.

    The GTK+ equivalent of QX11EmbedContainer is GtkSocket. The
    corresponding KDE 3 widget is called QXEmbed.

    \sa QX11EmbedWidget, {XEmbed Specification}
*/

/*! \fn void QX11EmbedWidget::embedded()

    This signal is emitted by the widget that has been embedded by an
    XEmbed container.
*/

/*! \fn void QX11EmbedWidget::containerClosed()

    This signal is emitted by the client widget when the container
    closes the widget. This can happen if the container itself
    closes, or if the widget is rejected.

    The container can reject a widget for any reason, but the most
    common cause of a rejection is when an attempt is made to embed a
    widget into a container that already has an embedded widget.
*/

/*! \fn void QX11EmbedContainer::clientIsEmbedded()

    This signal is emitted by the container when a client widget has
    been embedded.
*/

/*! \fn void QX11EmbedContainer::clientClosed()

    This signal is emitted by the container when the client widget
    closes.
*/

/*!
    \fn void QX11EmbedWidget::error(QX11EmbedWidget::Error error)

    This signal is emitted if an error occurred as a result of
    embedding into or communicating with a container. The specified
    \a error describes the problem that occurred.

    \sa QX11EmbedWidget::Error
*/

/*!
    \fn QX11EmbedContainer::Error QX11EmbedContainer::error() const

    Returns the last error that occurred.
*/

/*! \fn void QX11EmbedContainer::error(QX11EmbedContainer::Error error)

    This signal is emitted if an error occurred when embedding or
    communicating with a client. The specified \a error describes the
    problem that occurred.

    \sa QX11EmbedContainer::Error
*/

/*!
    \enum QX11EmbedWidget::Error

    \value Unknown An unrecognized error occurred.

    \value InvalidWindowID The X11 window ID of the container was
        invalid. This error is usually triggered by passing an invalid
        window ID to embedInto().

    \omitvalue Internal
*/

/*!
    \enum QX11EmbedContainer::Error

    \value Unknown An unrecognized error occurred.

    \value InvalidWindowID The X11 window ID of the container was
        invalid. This error is usually triggered by passing an invalid
        window ID to embed().

    \omitvalue Internal
*/

const int XButtonPress = ButtonPress;
const int XButtonRelease = ButtonRelease;
#undef ButtonPress
#undef ButtonRelease

// This is a hack to move topData() out from QWidgetPrivate to public.  We
// need to to inspect window()'s embedded state.
class QHackWidget : public QWidget
{
    Q_DECLARE_PRIVATE(QWidget)
public:
    QTLWExtra* topData() { return d_func()->topData(); }
};

static unsigned int XEMBED_VERSION = 0;

enum QX11EmbedMessageType {
    XEMBED_EMBEDDED_NOTIFY = 0,
    XEMBED_WINDOW_ACTIVATE = 1,
    XEMBED_WINDOW_DEACTIVATE = 2,
    XEMBED_REQUEST_FOCUS = 3,
    XEMBED_FOCUS_IN = 4,
    XEMBED_FOCUS_OUT = 5,
    XEMBED_FOCUS_NEXT = 6,
    XEMBED_FOCUS_PREV = 7,
    XEMBED_MODALITY_ON = 10,
    XEMBED_MODALITY_OFF = 11,
    XEMBED_REGISTER_ACCELERATOR = 12,
    XEMBED_UNREGISTER_ACCELERATOR = 13,
    XEMBED_ACTIVATE_ACCELERATOR = 14
};

enum QX11EmbedFocusInDetail {
    XEMBED_FOCUS_CURRENT = 0,
    XEMBED_FOCUS_FIRST = 1,
    XEMBED_FOCUS_LAST = 2
};

enum QX11EmbedFocusInFlags {
    XEMBED_FOCUS_OTHER = (0 << 0),
    XEMBED_FOCUS_WRAPAROUND = (1 << 0)
};

enum QX11EmbedInfoFlags {
    XEMBED_MAPPED = (1 << 0)
};

enum QX11EmbedAccelModifiers {
    XEMBED_MODIFIER_SHIFT = (1 << 0),
    XEMBED_MODIFIER_CONTROL = (1 << 1),
    XEMBED_MODIFIER_ALT = (1 << 2),
    XEMBED_MODIFIER_SUPER = (1 << 3),
    XEMBED_MODIFIER_HYPER = (1 << 4)
};

enum QX11EmbedAccelFlags {
    XEMBED_ACCELERATOR_OVERLOADED = (1 << 0)
};

// Silence the default X11 error handler.
static int x11ErrorHandler(Display *, XErrorEvent *)
{
    return 0;
}

// Returns the X11 timestamp. Maintained mainly by qapplication
// internals, but also updated by the XEmbed widgets.
static Time x11Time()
{
    return qt_x11Data->time;
}

// Gives the version and flags of the supported XEmbed protocol.
static unsigned int XEmbedVersion()
{
    return 0;
}

// Sends an XEmbed message.
static void sendXEmbedMessage(WId window, Display *display, long message,
                  long detail = 0, long data1 = 0, long data2 = 0)
{
    XClientMessageEvent c;
    memset(&c, 0, sizeof(c));
    c.type = ClientMessage;
    c.message_type = ATOM(_XEMBED);
    c.format = 32;
    c.display = display;
    c.window = window;

    c.data.l[0] = x11Time();
    c.data.l[1] = message;
    c.data.l[2] = detail;
    c.data.l[3] = data1;
    c.data.l[4] = data2;

    XSendEvent(display, window, false, NoEventMask, (XEvent *) &c);
}

// From qapplication_x11.cpp
static XKeyEvent lastKeyEvent;

static QCoreApplication::EventFilter oldX11EventFilter;

// The purpose of this global x11 filter is for one to capture the key
// events in their original state, but most importantly this is the
// only way to get the WM_TAKE_FOCUS message from WM_PROTOCOLS.
static bool x11EventFilter(void *message, long *result)
{
    XEvent *event = reinterpret_cast<XEvent *>(message);
    if (event->type == XKeyPress || event->type == XKeyRelease)
	lastKeyEvent = event->xkey;

    if (oldX11EventFilter && oldX11EventFilter != &x11EventFilter)
	return oldX11EventFilter(message, result);
    else
	return false;
}

//
struct functorData
{
    Window id, rootWindow;
    bool clearedWmState;
    bool reparentedToRoot;
};

static Bool functor(Display *display, XEvent *event, XPointer arg)
{
    functorData *data = (functorData *) arg;

    if (!data->reparentedToRoot && event->type == ReparentNotify
        && event->xreparent.window == data->id
        && event->xreparent.parent == data->rootWindow) {
        data->reparentedToRoot = true;
        return true;
    }

    if (!data->clearedWmState
        && event->type == PropertyNotify
        && event->xproperty.window == data->id
        && event->xproperty.atom == ATOM(WM_STATE)) {
	if (event->xproperty.state == PropertyDelete) {
            data->clearedWmState = true;
            return true;
        }

	Atom ret;
	int format, status;
	unsigned char *retval;
	unsigned long nitems, after;
	status = XGetWindowProperty(display, data->id, ATOM(WM_STATE), 0, 2, False, ATOM(WM_STATE),
				    &ret, &format, &nitems, &after, &retval );
	if (status == Success && ret == ATOM(WM_STATE) && format == 32 && nitems > 0) {
            long state = *(long *)retval;
            XFree(retval);
            if (state == WithdrawnState) {
                data->clearedWmState = true;
		return true;
            }
	}
    }

    return false;
}

class QX11EmbedWidgetPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QX11EmbedWidget)
public:
    inline QX11EmbedWidgetPrivate()
    {
        lastError = QX11EmbedWidget::Unknown;
        container = 0;
    }

    void setEmbedded();

    void emitError(QX11EmbedWidget::Error error) {
        Q_Q(QX11EmbedWidget);

        lastError = error;
        emit q->error(error);
    }

    enum FocusWidgets {
        FirstFocusWidget,
        LastFocusWidget
    };

    int focusOriginator;
    QWidget *getFocusWidget(FocusWidgets fw);
    void checkActivateWindow(QObject *o);
    QX11EmbedWidget *xEmbedWidget(QObject *o) const;
    void clearFocus();

    WId container;
    QPointer<QWidget> currentFocus;

    QX11EmbedWidget::Error lastError;

};

/*!
    Constructs a QX11EmbedWidget object with the given \a parent.
*/
QX11EmbedWidget::QX11EmbedWidget(QWidget *parent)
    : QWidget(*new QX11EmbedWidgetPrivate, parent, 0)
{
    XSetErrorHandler(x11ErrorHandler);

    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    createWinId();
    XSelectInput(x11Info().display(), internalWinId(),
                 KeyPressMask | KeyReleaseMask | ButtonPressMask
                    | ButtonReleaseMask
                    | KeymapStateMask | ButtonMotionMask | PointerMotionMask
                    | FocusChangeMask
                    | ExposureMask | StructureNotifyMask
                    | SubstructureNotifyMask | PropertyChangeMask);

    long data[] = {XEMBED_VERSION, XEMBED_MAPPED};
    XChangeProperty(x11Info().display(), internalWinId(), ATOM(_XEMBED_INFO),
                    ATOM(_XEMBED_INFO), 32, PropModeReplace,
                    (unsigned char*) data, 2);

    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QApplication::instance()->installEventFilter(this);

#ifdef QX11EMBED_DEBUG
    qDebug() << "QX11EmbedWidget::QX11EmbedWidget: constructed client"
             << (void *)this << "with winId" << winId();
#endif
}

/*!
    Destructs the QX11EmbedWidget object. If the widget is embedded
    when deleted, it is hidden and then detached from its container,
    so that the container is free to embed a new widget.
*/
QX11EmbedWidget::~QX11EmbedWidget()
{
    Q_D(QX11EmbedWidget);
    if (d->container) {
#ifdef QX11EMBED_DEBUG
        qDebug() << "QX11EmbedWidget::~QX11EmbedWidget: unmapping"
                 << (void *)this << "with winId" << winId()
                 << "from container with winId" << d->container;
#endif
        XUnmapWindow(x11Info().display(), internalWinId());
        XReparentWindow(x11Info().display(), internalWinId(), x11Info().appRootWindow(x11Info().screen()), 0, 0);
    }

#ifdef QX11EMBED_DEBUG
    qDebug() << "QX11EmbedWidget::~QX11EmbedWidget: destructed client"
             << (void *)this << "with winId" << winId();
#endif
}

/*!
    Returns the type of error that occurred last. This is the same error code
    that is emitted by the error() signal.

    \sa Error
*/
QX11EmbedWidget::Error QX11EmbedWidget::error() const
{
    return d_func()->lastError;
}

/*!
    When this function is called, the widget embeds itself into the
    container whose window ID is \a id.

    If \a id is \e not the window ID of a container this function will
    behave unpredictably.
*/
void QX11EmbedWidget::embedInto(WId id)
{
    Q_D(QX11EmbedWidget);
#ifdef QX11EMBED_DEBUG
    qDebug() << "QX11EmbedWidget::embedInto: embedding client"
             << (void *)this << "with winId" << winId() << "into container"
             << id;
#endif

    d->container = id;
    switch (XReparentWindow(x11Info().display(), internalWinId(), d->container, 0, 0)) {
    case BadWindow:
        d->emitError(InvalidWindowID);
        break;
    case BadMatch:
        d->emitError(Internal);
        break;
    case Success:
    default:
        break;
    }
    QTLWExtra* x = d->extra ? d->extra->topextra : 0;
    if (x)
        x->frameStrut.setCoords(0, 0, 0, 0);
    d->data.fstrut_dirty = false;
}

/*! \internal

    Gets the first or last child widget that can get focus.
*/
QWidget *QX11EmbedWidgetPrivate::getFocusWidget(FocusWidgets fw)
{
    Q_Q(QX11EmbedWidget);
    QWidget *tlw = q;
    QWidget *w = tlw->nextInFocusChain();

    QWidget *last = tlw;

    extern bool qt_tab_all_widgets;
    uint focus_flag = qt_tab_all_widgets ? Qt::TabFocus : Qt::StrongFocus;

    while (w != tlw)
    {
        if (((w->focusPolicy() & focus_flag) == focus_flag)
            && w->isVisibleTo(q) && w->isEnabled())
        {
            last = w;
            if (fw == FirstFocusWidget)
                break;
        }
        w = w->nextInFocusChain();
    }

    return last;
}

/*! \internal

    Returns the xembed widget that the widget is a child of
*/
QX11EmbedWidget *QX11EmbedWidgetPrivate::xEmbedWidget(QObject *o) const
{
    QX11EmbedWidget *xec = 0;

    // Check the widget itself, then its parents, and find the first
    // QX11EmbedWidget.
    do {
        if ((xec = qobject_cast<QX11EmbedWidget *>(o)))
            return xec;
    } while ((o = o->parent()));
    return 0;
}

/*! \internal

    Checks the active window.
*/
void QX11EmbedWidgetPrivate::checkActivateWindow(QObject *o)
{
    Q_Q(QX11EmbedWidget);
    QX11EmbedWidget *xec = xEmbedWidget(o);

    // check if we are in the right xembed client
    if (q != xec)
        return;

    QWidget *w = qobject_cast<QWidget *>(o);

    // if it is no active window, then don't do the change
    if (!(w && qApp->activeWindow()))
        return;

    // if it already is the active window, don't do anything
    if (w->window() != qApp->activeWindow())
    {
        qApp->setActiveWindow(w->window());
        currentFocus = w;

        sendXEmbedMessage(xec->containerWinId(), q->x11Info().display(), XEMBED_REQUEST_FOCUS);
    }
}

/*! \internal

    Clears the focus
*/
void QX11EmbedWidgetPrivate::clearFocus()
{
    Q_Q(QX11EmbedWidget);
    // Setting focus on the client itself removes Qt's
    // logical focus rectangle. We can't just do a
    // clearFocus here, because when we send the synthetic
    // FocusIn to ourselves on activation, Qt will set
    // focus on focusWidget() again. This way, we "hide"
    // focus rather than clearing it.

    if (!q->window()->hasFocus())
        q->window()->setFocus(Qt::OtherFocusReason);

    currentFocus = 0;
}

/*! \internal

    Sets the embedded flag on the client.
*/
void QX11EmbedWidgetPrivate::setEmbedded()
{
    Q_Q(QX11EmbedWidget);
    ((QHackWidget *)q->window())->topData()->embedded = 1;
}

/*! \internal

    Handles WindowActivate and FocusIn events for the client.
*/
bool QX11EmbedWidget::eventFilter(QObject *o, QEvent *event)
{
    Q_D(QX11EmbedWidget);
    switch (event->type()) {
    case QEvent::FocusIn:
        switch (((QFocusEvent *)event)->reason()) {
        case Qt::MouseFocusReason:
            // If the user clicks into one of the client widget's
            // children and we didn't have focus already, we request
            // focus from our container.
            if (d->xEmbedWidget(o) == this) {
                if (d->currentFocus.isNull())
                    sendXEmbedMessage(d->container, x11Info().display(), XEMBED_REQUEST_FOCUS);

                d->currentFocus = qobject_cast<QWidget *>(o);
            }
            break;
        case Qt::TabFocusReason:
            // If the xembed client receives a focus event because of
            // a Tab, then we are at the end of our focus chain and we
            // ask the container to move to its next focus widget.
            if (o == this) {
                d->clearFocus();
                sendXEmbedMessage(d->container, x11Info().display(), XEMBED_FOCUS_NEXT);
                return true;
            } else {
                // We're listening on events from qApp, so in order
                // for us to know who to set focus on if we receive an
                // activation event, we note the widget that got the
                // focusin last.
                if (d->xEmbedWidget(o) == this)
                    d->currentFocus = qobject_cast<QWidget *>(o);
            }
            break;
        case Qt::BacktabFocusReason:
            // If the window receives a focus event because of
            // a Backtab, then we are at the start of our focus chain
            // and we ask the container to move to its previous focus
            // widget.
            if (o == this) {
                // See comment for Tab.
                // If we receive a XEMBED_FOCUS_IN
                // XEMBED_FOCUS_CURRENT, we will set focus in
                // currentFocus. To avoid that in this case, we reset
                // currentFocus.
                d->clearFocus();
                sendXEmbedMessage(d->container, x11Info().display(), XEMBED_FOCUS_PREV);
                return true;
            } else {
                if (d->xEmbedWidget(o) == this)
                    d->currentFocus = qobject_cast<QWidget *>(o);
            }
            break;
        case Qt::ActiveWindowFocusReason:
            if (isActiveWindow()) {
                if (!d->currentFocus.isNull()) {
                    if (!d->currentFocus->hasFocus())
                        d->currentFocus->setFocus(Qt::OtherFocusReason);
                } else {
                    d->clearFocus();
                    return true;
                }
            }

            break;
        case Qt::PopupFocusReason:
        case Qt::ShortcutFocusReason:
        case Qt::OtherFocusReason:
            // If focus is received to any child widget because of any
            // other reason, remember the widget so that we can give
            // it focus again if we're activated.
            if (d->xEmbedWidget(o) == this) {
               d->currentFocus = qobject_cast<QWidget *>(o);
            }
            break;
        default:
            break;
        }
        break;
    case QEvent::MouseButtonPress:
        // If we get a mouse button press event inside a embedded widget
        // make sure this is the active window in qapp.
        d->checkActivateWindow(o);
        break;
    default:
        break;
    }

    return QWidget::eventFilter(o, event);
}

/*! \internal

    Handles some notification events and client messages. Client side
    XEmbed message receiving is also handled here.
*/
bool QX11EmbedWidget::x11Event(XEvent *event)
{
    Q_D(QX11EmbedWidget);
    switch (event->type) {
    case DestroyNotify:
#ifdef QX11EMBED_DEBUG
        qDebug() << "QX11EmbedWidget::x11Event: client"
                 << (void *)this << "with winId" << winId()
                 << "received a DestroyNotify";
#endif
        // If the container window is destroyed, we signal this to the user.
        d->container = 0;
        emit containerClosed();
        break;
    case ReparentNotify:
#ifdef QX11EMBED_DEBUG
        qDebug() << "QX11EmbedWidget::x11Event: client"
                 << (void *)this << "with winId" << winId()
                 << "received a ReparentNotify to"
                 << ((event->xreparent.parent == x11Info().appRootWindow(x11Info().screen()))
                     ? QString::fromLatin1("root") : QString::number(event->xreparent.parent));
#endif
        // If the container shuts down, we will be reparented to the
        // root window. We must also consider the case that we may be
        // reparented from one container to another.
        if (event->xreparent.parent == x11Info().appRootWindow(x11Info().screen())) {
            if (((QHackWidget *)this)->topData()->embedded) {
                d->container = 0;
                emit containerClosed();
            }
            return true;
        } else {
            d->container = event->xreparent.parent;
        }
        break;
    case UnmapNotify:
        // Mapping and unmapping are handled by changes to the
        // _XEMBED_INFO property. Any default map/unmap requests are
        // ignored.
        return true;
    case PropertyNotify:
        // The container sends us map/unmap messages through the
        // _XEMBED_INFO property. We adhere to the XEMBED_MAPPED bit in
        // data2.
        if (event->xproperty.atom == ATOM(_XEMBED_INFO)) {
            Atom actual_type_return;
            int actual_format_return;
            unsigned long nitems_return;
            unsigned long bytes_after_return;
            unsigned char *prop_return = 0;
            if (XGetWindowProperty(x11Info().display(), internalWinId(), ATOM(_XEMBED_INFO), 0, 2,
                                   false, ATOM(_XEMBED_INFO), &actual_type_return,
                                   &actual_format_return, &nitems_return,
                                   &bytes_after_return, &prop_return) == Success) {
                if (nitems_return > 1) {
                    if (((long * )prop_return)[1] & XEMBED_MAPPED) {
                        XMapWindow(x11Info().display(), internalWinId());
                    } else {
                        XUnmapWindow(x11Info().display(), internalWinId());
                    }
                }
                if (prop_return)
                    XFree(prop_return);
            }
        }

        break;
    case ClientMessage:
        // XEMBED messages have message_type _XEMBED
        if (event->xclient.message_type == ATOM(_XEMBED)) {
            // Discard XEMBED messages not to ourselves. (### dead code?)
            if (event->xclient.window != internalWinId())
                break;

            // Update qt_x_time if necessary
            Time msgtime = (Time) event->xclient.data.l[0];
            if (msgtime > X11->time)
                X11->time = msgtime;

            switch (event->xclient.data.l[1]) {
            case XEMBED_WINDOW_ACTIVATE: {
                // When we receive an XEMBED_WINDOW_ACTIVATE, we simply send
                // ourselves a WindowActivate event. Real activation happens
                // when receive XEMBED_FOCUS_IN.
                if (!isActiveWindow()) {
                    QEvent ev(QEvent::WindowActivate);
                    QApplication::sendEvent(this, &ev);
                }
            }
                break;
            case XEMBED_WINDOW_DEACTIVATE: {
                // When we receive an XEMBED_WINDOW_DEACTIVATE, we simply send
                // ourselves a WindowDeactivate event. Real activation happens
                // when receive XEMBED_FOCUS_IN.
                if (isActiveWindow()) {
                    if (!qApp->activePopupWidget())
                        QApplication::setActiveWindow(0);
                } else {
                    QEvent ev(QEvent::WindowDeactivate);
                    QApplication::sendEvent(this, &ev);
                }
            }
                break;
            case XEMBED_EMBEDDED_NOTIFY: {
#ifdef QX11EMBED_DEBUG
                qDebug() << "QX11EmbedWidget::x11Event: client"
                         << (void *)this << "with winId" << winId()
                         << "received an XEMBED EMBEDDED NOTIFY message";
#endif
                // In this message's l[2] we have the max version
                // supported by both the client and the
                // container. QX11EmbedWidget does not use this field.

                // We have been embedded, so we set our
                // client's embedded flag.
                d->setEmbedded();
                emit embedded();
            }
                break;
            case XEMBED_FOCUS_IN:
                // don't set the focus if a modal dialog is open
                if (qApp->activeModalWidget())
                    break;

                // in case we embed more than one topLevel window inside the same
                // host window.
                if (window() != qApp->activeWindow())
                    qApp->setActiveWindow(this);

                switch (event->xclient.data.l[2]) {
                case XEMBED_FOCUS_CURRENT:
                    // The container sends us this message if it wants
                    // us to focus on the widget that last had focus.
                    // This is the reply when XEMBED_REQUEST_FOCUS is
                    // sent to the container.
                    if (!d->currentFocus.isNull()) {
                        if (!d->currentFocus->hasFocus())
                            d->currentFocus->setFocus(Qt::OtherFocusReason);
                    } else {
                        // No widget currently has focus. We set focus
                        // on the first widget next to the
                        // client widget. Since the setFocus will not work
                        // if the window is disabled, set the currentFocus
                        // directly so that it's set on window activate.
                        d->currentFocus = d->getFocusWidget(QX11EmbedWidgetPrivate::FirstFocusWidget);
                        d->currentFocus->setFocus(Qt::OtherFocusReason);
                    }
                    break;
                case XEMBED_FOCUS_FIRST:
                    // The container sends this message when it wants
                    // us to focus on the first widget in our focus
                    // chain (typically because of a tab).
                    d->currentFocus = d->getFocusWidget(QX11EmbedWidgetPrivate::FirstFocusWidget);
                    d->currentFocus->setFocus(Qt::TabFocusReason);
                    break;
                case XEMBED_FOCUS_LAST:
                    // The container sends this message when it wants
                    // us to focus on the last widget in our focus
                    // chain (typically because of a backtab).
                    d->currentFocus = d->getFocusWidget(QX11EmbedWidgetPrivate::LastFocusWidget);
                    d->currentFocus->setFocus(Qt::BacktabFocusReason);
                    break;
                default:
                    // Ignore any other XEMBED_FOCUS_IN details.
                    break;
                }
                break;
            case XEMBED_FOCUS_OUT:
                // The container sends us this message when it wants us
                // to lose focus and forget about the widget that last
                // had focus. Typically sent by the container when it
                // loses focus because of mouse or tab activity. We do
                // then not want to set focus on anything if we're
                // activated.
                if (isActiveWindow())
                    d->clearFocus();

                break;
            default:
                // Ignore any other XEMBED messages.
                break;
            };
        } else {
            // Non-XEMBED client messages are not interesting.
        }

        break;
    default:
        // Ignore all other x11 events.
        break;
    }

    // Allow default handling.
    return QWidget::x11Event(event);
}

/*!
    \reimp
*/
bool QX11EmbedWidget::event(QEvent *event)
{
    if (event->type() == QEvent::ParentChange) {
        XSelectInput(x11Info().display(), internalWinId(),
                     KeyPressMask | KeyReleaseMask | ButtonPressMask
                     | ButtonReleaseMask
                     | KeymapStateMask | ButtonMotionMask | PointerMotionMask
                     | FocusChangeMask
                     | ExposureMask | StructureNotifyMask
                     | SubstructureNotifyMask | PropertyChangeMask);
    }
    return QWidget::event(event);
}

/*!
    \reimp
*/
void QX11EmbedWidget::resizeEvent(QResizeEvent *event)
{
    if (layout())
        layout()->update();
    QWidget::resizeEvent(event);
}

/*!
    If the widget is embedded, returns the window ID of the
    container; otherwize returns 0.
*/
WId QX11EmbedWidget::containerWinId() const
{
    Q_D(const QX11EmbedWidget);
    return d->container;
}

class QX11EmbedContainerPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QX11EmbedContainer)
public:
    inline QX11EmbedContainerPrivate()
    {
        lastError = QX11EmbedContainer::Unknown;
        client = 0;
        focusProxy = 0;
        clientIsXEmbed = false;
        xgrab = false;
    }

    bool isEmbedded() const;
    void moveInputToProxy();

    void acceptClient(WId window);
    void rejectClient(WId window);

    void checkGrab();

    WId topLevelParentWinId() const;

    void emitError(QX11EmbedContainer::Error error) {
        Q_Q(QX11EmbedContainer);
        lastError = error;
        emit q->error(error);
    }

    WId client;
    QWidget *focusProxy;
    bool clientIsXEmbed;
    bool xgrab;
    QRect clientOriginalRect;
    QSize wmMinimumSizeHint;

    QX11EmbedContainer::Error lastError;

    static QX11EmbedContainer *activeContainer;
};

QX11EmbedContainer *QX11EmbedContainerPrivate::activeContainer = 0;

/*!
    Creates a QX11EmbedContainer object with the given \a parent.
*/
QX11EmbedContainer::QX11EmbedContainer(QWidget *parent)
    : QWidget(*new QX11EmbedContainerPrivate, parent, 0)
{
    Q_D(QX11EmbedContainer);
    XSetErrorHandler(x11ErrorHandler);

    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    createWinId();

    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // ### PORT setKeyCompression(false);
    setAcceptDrops(true);
    setEnabled(false);

    // Everybody gets a focus proxy, but only one toplevel container's
    // focus proxy is actually in use.
    d->focusProxy = new QWidget(this);
    d->focusProxy->setAttribute(Qt::WA_NativeWindow);
    d->focusProxy->setAttribute(Qt::WA_DontCreateNativeAncestors);
    d->focusProxy->createWinId();
    d->focusProxy->setGeometry(-1, -1, 1, 1);

    // We need events from the window (activation status) and
    // from qApp (keypress/release).
    qApp->installEventFilter(this);

    // Install X11 event filter.
    if (!oldX11EventFilter)
        oldX11EventFilter = QCoreApplication::instance()->setEventFilter(x11EventFilter);

    XSelectInput(x11Info().display(), internalWinId(),
                 KeyPressMask | KeyReleaseMask
                 | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask
                 | KeymapStateMask
                 | PointerMotionMask
                 | EnterWindowMask | LeaveWindowMask
                 | FocusChangeMask
                 | ExposureMask
                 | StructureNotifyMask
                 | SubstructureNotifyMask);

    // Make sure our new event mask takes effect as soon as possible.
    XFlush(x11Info().display());

    // Move input to our focusProxy if this widget is active, and not
    // shaded by a modal dialog (in which case isActiveWindow() would
    // still return true, but where we must not move input focus).
    if (qApp->activeWindow() == window() && !d->isEmbedded())
        d->moveInputToProxy();

#ifdef QX11EMBED_DEBUG
    qDebug() << "QX11EmbedContainer::QX11EmbedContainer: constructed container"
             << (void *)this << "with winId" << winId();
#endif
}

/*!
    Destructs a QX11EmbedContainer.
*/
QX11EmbedContainer::~QX11EmbedContainer()
{
    Q_D(QX11EmbedContainer);
    if (d->client) {
	XUnmapWindow(x11Info().display(), d->client);
	XReparentWindow(x11Info().display(), d->client, x11Info().appRootWindow(x11Info().screen()), 0, 0);
    }

    if (d->xgrab)
	XUngrabButton(x11Info().display(), AnyButton, AnyModifier, internalWinId());
}


QX11EmbedContainer::Error QX11EmbedContainer::error() const {
    return d_func()->lastError;
}


/*! \reimp
*/
void QX11EmbedContainer::paintEvent(QPaintEvent *)
{
}

/*! \internal

    Returns whether or not the windows' embedded flag is set.
*/
bool QX11EmbedContainerPrivate::isEmbedded() const
{
    Q_Q(const QX11EmbedContainer);
    return ((QHackWidget *)q->window())->topData()->embedded == 1;
}

/*! \internal

    Returns the parentWinId of the window.
*/
WId QX11EmbedContainerPrivate::topLevelParentWinId() const
{
    Q_Q(const QX11EmbedContainer);
    return ((QHackWidget *)q->window())->topData()->parentWinId;
}

/*!
    If the container has an embedded widget, this function returns
    the X11 window ID of the client; otherwise it returns 0.
*/
WId QX11EmbedContainer::clientWinId() const
{
    Q_D(const QX11EmbedContainer);
    return d->client;
}

/*!
    Instructs the container to embed the X11 window with window ID \a
    id. The client widget will then move on top of the container
    window and be resized to fit into the container.

    The \a id should be the ID of a window controlled by an XEmbed
    enabled application, but this is not mandatory. If \a id does not
    belong to an XEmbed client widget, then focus handling,
    activation, accelerators and other features will not work
    properly.
*/
void QX11EmbedContainer::embedClient(WId id)
{
    Q_D(QX11EmbedContainer);

    if (id == 0) {
	d->emitError(InvalidWindowID);
	return;
    }

    // Walk up the tree of parent windows to prevent embedding of ancestors.
    WId thisId = internalWinId();
    Window rootReturn;
    Window parentReturn;
    Window *childrenReturn = 0;
    unsigned int nchildrenReturn;
    do {
        if (XQueryTree(x11Info().display(), thisId, &rootReturn,
                       &parentReturn, &childrenReturn, &nchildrenReturn) == 0) {
	    d->emitError(InvalidWindowID);
	    return;
        }
        if (childrenReturn) {
            XFree(childrenReturn);
            childrenReturn = 0;
        }

        thisId = parentReturn;
        if (id == thisId) {
	    d->emitError(InvalidWindowID);
	    return;
        }
    } while (thisId != rootReturn);

    // watch for property notify events (see below)
    XGrabServer(x11Info().display());
    XWindowAttributes attrib;
    if (!XGetWindowAttributes(x11Info().display(), id, &attrib)) {
        XUngrabServer(x11Info().display());
	d->emitError(InvalidWindowID);
	return;
    }
    XSelectInput(x11Info().display(), id, attrib.your_event_mask | PropertyChangeMask | StructureNotifyMask);
    XUngrabServer(x11Info().display());

    // Put the window into WithdrawnState
    XUnmapWindow(x11Info().display(), id);
    XSync(x11Info().display(), False); // make sure the window is hidden

    /*
      Wait for notification from the window manager that the window is
      in withdrawn state.  According to the ICCCM section 4.1.3.1,
      we should wait for the WM_STATE property to either be deleted or
      set to WithdrawnState.

      For safety, we will not wait more than 500 ms, so that we can
      preemptively workaround buggy window managers.
    */
    QElapsedTimer t;
    t.start();

    functorData data;
    data.id = id;
    data.rootWindow = attrib.root;
    data.clearedWmState = false;
    data.reparentedToRoot = false;

    do {
	if (t.elapsed() > 500) // time-out after 500 ms
	    break;

	XEvent event;
	if (!XCheckIfEvent(x11Info().display(), &event, functor, (XPointer) &data)) {
	    XSync(x11Info().display(), False);
            usleep(50000);
	    continue;
	}

        qApp->x11ProcessEvent(&event);
    } while (!data.clearedWmState || !data.reparentedToRoot);

    // restore the event mask
    XSelectInput(x11Info().display(), id, attrib.your_event_mask);

    switch (XReparentWindow(x11Info().display(), id, internalWinId(), 0, 0)) {
    case BadWindow:
    case BadMatch:
	d->emitError(InvalidWindowID);
	break;
    default:
	break;
    }
}

/*! \internal

    Handles key, activation and focus events for the container.
*/
bool QX11EmbedContainer::eventFilter(QObject *o, QEvent *event)
{
    Q_D(QX11EmbedContainer);
    switch (event->type()) {
    case QEvent::KeyPress:
        // Forward any keypresses to our client.
	if (o == this && d->client) {
	    lastKeyEvent.window = d->client;
	    XSendEvent(x11Info().display(), d->client, false, KeyPressMask, (XEvent *) &lastKeyEvent);
	    return true;
	}
	break;
    case QEvent::KeyRelease:
	// Forward any keyreleases to our client.
	if (o == this && d->client) {
	    lastKeyEvent.window = d->client;
	    XSendEvent(x11Info().display(), d->client, false, KeyReleaseMask, (XEvent *) &lastKeyEvent);
	    return true;
	}
	break;

    case QEvent::WindowActivate:
	// When our container window is activated, we pass the
	// activation message on to our client. Note that X input
	// focus is set to our focus proxy. We want to intercept all
	// keypresses.
	if (o == window() && d->client) {
            if (d->clientIsXEmbed) {
                sendXEmbedMessage(d->client, x11Info().display(), XEMBED_WINDOW_ACTIVATE);
            } else {
                d->checkGrab();
                if (hasFocus())
                    XSetInputFocus(x11Info().display(), d->client, XRevertToParent, x11Time());
            }
            if (!d->isEmbedded())
                d->moveInputToProxy();
	}
	break;
    case QEvent::WindowDeactivate:
	// When our container window is deactivated, we pass the
	// deactivation message to our client.
	if (o == window() && d->client) {
	    if (d->clientIsXEmbed)
		sendXEmbedMessage(d->client, x11Info().display(), XEMBED_WINDOW_DEACTIVATE);
	    else
		d->checkGrab();
	}
	break;
    case QEvent::FocusIn:
        // When receiving FocusIn events generated by Tab or Backtab,
	// we pass focus on to our client. Any mouse activity is sent
	// directly to the client, and it will ask us for focus with
	// XEMBED_REQUEST_FOCUS.
	if (o == this && d->client) {
            if (!d->isEmbedded())
                d->activeContainer = this;

            if (d->clientIsXEmbed) {
                if (!d->isEmbedded())
                    d->moveInputToProxy();

		QFocusEvent *fe = (QFocusEvent *)event;
		switch (fe->reason()) {
		case Qt::TabFocusReason:
		    sendXEmbedMessage(d->client, x11Info().display(), XEMBED_FOCUS_IN, XEMBED_FOCUS_FIRST);
		    break;
		case Qt::BacktabFocusReason:
		    sendXEmbedMessage(d->client, x11Info().display(), XEMBED_FOCUS_IN, XEMBED_FOCUS_LAST);
		    break;
		default:
		    sendXEmbedMessage(d->client, x11Info().display(), XEMBED_FOCUS_IN, XEMBED_FOCUS_CURRENT);
		    break;
		}
	    } else {
		d->checkGrab();
                XSetInputFocus(x11Info().display(), d->client, XRevertToParent, x11Time());
	    }
	}

	break;
    case QEvent::FocusOut: {
	// When receiving a FocusOut, we ask our client to remove its
	// focus.
	if (o == this && d->client) {
            if (!d->isEmbedded()) {
                d->activeContainer = 0;
                if (isActiveWindow())
                    d->moveInputToProxy();
            }

	    if (d->clientIsXEmbed) {
		QFocusEvent *fe = (QFocusEvent *)event;
		if (o == this && d->client && fe->reason() != Qt::ActiveWindowFocusReason)
		    sendXEmbedMessage(d->client, x11Info().display(), XEMBED_FOCUS_OUT);
	    } else {
		d->checkGrab();
	    }
	}
    }
	break;

    case QEvent::Close: {
	if (o == this && d->client) {
	    // Unmap the client and reparent it to the root window.
	    // Wait until the messages have been processed. Then ask
	    // the window manager to delete the window.
	    XUnmapWindow(x11Info().display(), d->client);
	    XReparentWindow(x11Info().display(), d->client, x11Info().appRootWindow(x11Info().screen()), 0, 0);
	    XSync(x11Info().display(), false);

	    XEvent ev;
	    memset(&ev, 0, sizeof(ev));
	    ev.xclient.type = ClientMessage;
	    ev.xclient.window = d->client;
	    ev.xclient.message_type = ATOM(WM_PROTOCOLS);
	    ev.xclient.format = 32;
	    ev.xclient.data.s[0] = ATOM(WM_DELETE_WINDOW);
	    XSendEvent(x11Info().display(), d->client, false, NoEventMask, &ev);

	    XFlush(x11Info().display());
	    d->client = 0;
	    d->clientIsXEmbed = false;
            d->wmMinimumSizeHint = QSize();
            updateGeometry();
            setEnabled(false);
	    update();

	    emit clientClosed();
	}
    }
    default:
	break;
    }

    return QWidget::eventFilter(o, event);
}

/*! \internal

    Handles X11 events for the container.
*/
bool QX11EmbedContainer::x11Event(XEvent *event)
{
    Q_D(QX11EmbedContainer);

    switch (event->type) {
    case CreateNotify:
	// The client created an embedded window.
	if (d->client)
	    d->rejectClient(event->xcreatewindow.window);
	else
	    d->acceptClient(event->xcreatewindow.window);
      break;
    case DestroyNotify:
	if (event->xdestroywindow.window == d->client) {
	    // The client died.
	    d->client = 0;
	    d->clientIsXEmbed = false;
            d->wmMinimumSizeHint = QSize();
            updateGeometry();
	    update();
            setEnabled(false);
	    emit clientClosed();
	}
        break;
    case ReparentNotify:
	// The client sends us this if it reparents itself out of our
	// widget.
	if (event->xreparent.window == d->client && event->xreparent.parent != internalWinId()) {
	    d->client = 0;
	    d->clientIsXEmbed = false;
            d->wmMinimumSizeHint = QSize();
            updateGeometry();
	    update();
            setEnabled(false);
	    emit clientClosed();
	} else if (event->xreparent.parent == internalWinId()) {
	    // The client reparented itself into this window.
	    if (d->client)
		d->rejectClient(event->xreparent.window);
	    else
		d->acceptClient(event->xreparent.window);
	}
	break;
    case ClientMessage: {
	if (event->xclient.message_type == ATOM(_XEMBED)) {
	    // Ignore XEMBED messages not to ourselves
	    if (event->xclient.window != internalWinId())
		break;

	    // Receiving an XEmbed message means the client
	    // is an XEmbed client.
	    d->clientIsXEmbed = true;

	    Time msgtime = (Time) event->xclient.data.l[0];
	    if (msgtime > X11->time)
		X11->time = msgtime;

	    switch (event->xclient.data.l[1]) {
	    case XEMBED_REQUEST_FOCUS: {
		// This typically happens when the client gets focus
		// because of a mouse click.
		if (!hasFocus())
		    setFocus(Qt::OtherFocusReason);

		// The message is passed along to the topmost container
		// that eventually responds with a XEMBED_FOCUS_IN
		// message. The focus in message is passed all the way
		// back until it reaches the original focus
		// requestor. In the end, not only the original client
		// has focus, but also all its ancestor containers.
		if (d->isEmbedded()) {
                    // If our window's embedded flag is set, then
		    // that suggests that we are part of a client. The
		    // parentWinId will then point to an container to whom
		    // we must pass this message.
		    sendXEmbedMessage(d->topLevelParentWinId(), x11Info().display(), XEMBED_REQUEST_FOCUS);
		} else {
                    // Our window's embedded flag is not set,
		    // so we are the topmost container. We respond to
		    // the focus request message with a focus in
		    // message. This message will pass on from client
		    // to container to client until it reaches the
		    // originator of the XEMBED_REQUEST_FOCUS message.
		    sendXEmbedMessage(d->client, x11Info().display(), XEMBED_FOCUS_IN, XEMBED_FOCUS_CURRENT);
		}

		break;
	    }
	    case XEMBED_FOCUS_NEXT:
		// Client sends this event when it received a tab
		// forward and was at the end of its focus chain. If
		// we are the only widget in the focus chain, we send
		// ourselves a FocusIn event.
                if (d->focus_next != this) {
		    focusNextPrevChild(true);
                } else {
                    QFocusEvent event(QEvent::FocusIn, Qt::TabFocusReason);
                    qApp->sendEvent(this, &event);
                }

		break;
	    case XEMBED_FOCUS_PREV:
		// Client sends this event when it received a backtab
		// and was at the start of its focus chain. If we are
		// the only widget in the focus chain, we send
		// ourselves a FocusIn event.
                if (d->focus_next != this) {
		    focusNextPrevChild(false);
                } else {
                    QFocusEvent event(QEvent::FocusIn, Qt::BacktabFocusReason);
                    qApp->sendEvent(this, &event);
                }

		break;
	    default:
		break;
	    }
	}
    }
	break;
    case XButtonPress:
	if (!d->clientIsXEmbed) {
            setFocus(Qt::MouseFocusReason);
            XAllowEvents(x11Info().display(), ReplayPointer, CurrentTime);
            return true;
	}
	break;
    case XButtonRelease:
	if (!d->clientIsXEmbed)
            XAllowEvents(x11Info().display(), SyncPointer, CurrentTime);
	break;
    default:
	break;
    }

    return QWidget::x11Event(event);
}

/*! \internal

    Whenever the container is resized, we need to resize our client.
*/
void QX11EmbedContainer::resizeEvent(QResizeEvent *)
{
    Q_D(QX11EmbedContainer);
    if (d->client)
	XResizeWindow(x11Info().display(), d->client, width(), height());
}

/*! \internal

    We use the QShowEvent to signal to our client that we want it to
    map itself. We do this by changing its window property
    XEMBED_INFO. The client will get an X11 PropertyNotify.
*/
void QX11EmbedContainer::showEvent(QShowEvent *)
{
    Q_D(QX11EmbedContainer);
    if (d->client) {
        long data[] = {XEMBED_VERSION, XEMBED_MAPPED};
	XChangeProperty(x11Info().display(), d->client, ATOM(_XEMBED_INFO), ATOM(_XEMBED_INFO), 32,
			PropModeReplace, (unsigned char *) data, 2);
    }
}

/*! \internal

    We use the QHideEvent to signal to our client that we want it to
    unmap itself. We do this by changing its window property
    XEMBED_INFO. The client will get an X11 PropertyNotify.
*/
void QX11EmbedContainer::hideEvent(QHideEvent *)
{
    Q_D(QX11EmbedContainer);
    if (d->client) {
        long data[] = {XEMBED_VERSION, XEMBED_MAPPED};
	XChangeProperty(x11Info().display(), d->client, ATOM(_XEMBED_INFO), ATOM(_XEMBED_INFO), 32,
			PropModeReplace, (unsigned char *) data, 2);
    }
}

/*!
    \reimp
*/
bool QX11EmbedContainer::event(QEvent *event)
{
    if (event->type() == QEvent::ParentChange) {
        XSelectInput(x11Info().display(), internalWinId(),
                     KeyPressMask | KeyReleaseMask
                     | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask
                     | KeymapStateMask
                     | PointerMotionMask
                     | EnterWindowMask | LeaveWindowMask
                     | FocusChangeMask
                     | ExposureMask
                     | StructureNotifyMask
                     | SubstructureNotifyMask);
    }
    return QWidget::event(event);
}

/*! \internal

    Rejects a client window by reparenting it to the root window.  The
    client will receive a reparentnotify, and will most likely assume
    that the container has shut down. The XEmbed protocol does not
    define any way to reject a client window, but this is a clean way
    to do it.
*/
void QX11EmbedContainerPrivate::rejectClient(WId window)
{
    Q_Q(QX11EmbedContainer);
    q->setEnabled(false);
    XRemoveFromSaveSet(q->x11Info().display(), client);
    XReparentWindow(q->x11Info().display(), window, q->x11Info().appRootWindow(q->x11Info().screen()), 0, 0);
}

/*! \internal

    Accepts a client by mapping it, resizing it and optionally
    activating and giving it logical focusing through XEMBED messages.
*/
void QX11EmbedContainerPrivate::acceptClient(WId window)
{
    Q_Q(QX11EmbedContainer);
    client = window;
    q->setEnabled(true);

    // This tells Qt that we wish to forward DnD messages to
    // our client.
    if (!extra)
        createExtra();
    extraData()->xDndProxy = client;

    unsigned int version = XEmbedVersion();

    Atom actual_type_return;
    int actual_format_return;
    unsigned long nitems_return = 0;
    unsigned long bytes_after_return;
    unsigned char *prop_return = 0;
    unsigned int clientversion = 0;

    // Add this client to our saveset, so if we crash, the client window
    // doesn't get destroyed. This is useful for containers that restart
    // automatically after a crash, because it can simply reembed its clients
    // without having to restart them (KDE panel).
    XAddToSaveSet(q->x11Info().display(), client);

    // XEmbed clients have an _XEMBED_INFO property in which we can
    // fetch the version
    if (XGetWindowProperty(q->x11Info().display(), client, ATOM(_XEMBED_INFO), 0, 2, false,
                           ATOM(_XEMBED_INFO), &actual_type_return, &actual_format_return,
                           &nitems_return, &bytes_after_return, &prop_return) == Success) {

	if (actual_type_return != None && actual_format_return != 0) {
	    // Clients with the _XEMBED_INFO property are XEMBED clients.
	    clientIsXEmbed = true;

	    long *p = (long *)prop_return;
	    if (nitems_return >= 2)
		clientversion = (unsigned int)p[0];
	}

	XFree(prop_return);
    }

    // Store client window's original size and placement.
    Window root;
    int x_return, y_return;
    unsigned int width_return, height_return, border_width_return, depth_return;
    XGetGeometry(q->x11Info().display(), client, &root, &x_return, &y_return,
		 &width_return, &height_return, &border_width_return, &depth_return);
    clientOriginalRect.setCoords(x_return, y_return,
				 x_return + width_return - 1,
				 y_return + height_return - 1);

    // Ask the client for its minimum size.
    XSizeHints size;
    long msize;
    if (XGetWMNormalHints(q->x11Info().display(), client, &size, &msize) && (size.flags & PMinSize)) {
	wmMinimumSizeHint = QSize(size.min_width, size.min_height);
        q->updateGeometry();
    }

    // The container should set the data2 field to the lowest of its
    // supported version number and that of the client (from
    // _XEMBED_INFO property).
    unsigned int minversion = version > clientversion ? clientversion : version;
    sendXEmbedMessage(client, q->x11Info().display(), XEMBED_EMBEDDED_NOTIFY, q->internalWinId(), minversion);
    XMapWindow(q->x11Info().display(), client);

    // Resize it, but no smaller than its minimum size hint.
    XResizeWindow(q->x11Info().display(),
                  client,
                  qMax(q->width(), wmMinimumSizeHint.width()),
                  qMax(q->height(), wmMinimumSizeHint.height()));
    q->update();

    // Not mentioned in the protocol is that if the container
    // is already active, the client must be activated to work
    // properly.
    if (q->window()->isActiveWindow())
	sendXEmbedMessage(client, q->x11Info().display(), XEMBED_WINDOW_ACTIVATE);

    // Also, if the container already has focus, then it must
    // send a focus in message to its new client; otherwise we ask
    // it to remove focus.
    if (q->focusWidget() == q && q->hasFocus())
	sendXEmbedMessage(client, q->x11Info().display(), XEMBED_FOCUS_IN, XEMBED_FOCUS_FIRST);
    else
	sendXEmbedMessage(client, q->x11Info().display(), XEMBED_FOCUS_OUT);

    if (!clientIsXEmbed) {
        checkGrab();
        if (q->hasFocus()) {
            XSetInputFocus(q->x11Info().display(), client, XRevertToParent, x11Time());
        }
    } else {
        if (!isEmbedded())
            moveInputToProxy();
    }

    emit q->clientIsEmbedded();
}

/*! \internal

    Moves X11 keyboard input focus to the focusProxy, unless the focus
    is there already. When X11 keyboard input focus is on the
    focusProxy, which is a child of the container and a sibling of the
    client, X11 keypresses and keyreleases will always go to the proxy
    and not to the client.
*/
void QX11EmbedContainerPrivate::moveInputToProxy()
{
    Q_Q(QX11EmbedContainer);
    // Following Owen Taylor's advice from the XEmbed specification to
    // always use CurrentTime when no explicit user action is involved.
    XSetInputFocus(q->x11Info().display(), focusProxy->internalWinId(), XRevertToParent, CurrentTime);
}

/*! \internal

    Ask the window manager to give us a default minimum size.
*/
QSize QX11EmbedContainer::minimumSizeHint() const
{
    Q_D(const QX11EmbedContainer);
    if (!d->client || !d->wmMinimumSizeHint.isValid())
	return QWidget::minimumSizeHint();
    return d->wmMinimumSizeHint;
}

/*! \internal

*/
void QX11EmbedContainerPrivate::checkGrab()
{
    Q_Q(QX11EmbedContainer);
    if (!clientIsXEmbed && q->isActiveWindow() && !q->hasFocus()) {
        if (!xgrab) {
            XGrabButton(q->x11Info().display(), AnyButton, AnyModifier, q->internalWinId(),
                        true, ButtonPressMask, GrabModeSync, GrabModeAsync,
                        None, None);
        }
        xgrab = true;
    } else {
	if (xgrab)
	    XUngrabButton(q->x11Info().display(), AnyButton, AnyModifier, q->internalWinId());
        xgrab = false;
    }
}

/*!
    Detaches the client from the embedder. The client will appear as a
    standalone window on the desktop.
*/
void QX11EmbedContainer::discardClient()
{
    Q_D(QX11EmbedContainer);
    if (d->client) {
	XResizeWindow(x11Info().display(), d->client, d->clientOriginalRect.width(),
		      d->clientOriginalRect.height());

	d->rejectClient(d->client);
    }
}

QT_END_NAMESPACE
