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

// #define QCLIPBOARD_DEBUG
// #define QCLIPBOARD_DEBUG_VERBOSE

#ifdef QCLIPBOARD_DEBUG
#  define DEBUG qDebug
#else
#  define DEBUG if (false) qDebug
#endif

#ifdef QCLIPBOARD_DEBUG_VERBOSE
#  define VDEBUG qDebug
#else
#  define VDEBUG if (false) qDebug
#endif

#include "qplatformdefs.h"

#include "qclipboard.h"
#include "qclipboard_p.h"

#ifndef QT_NO_CLIPBOARD

#include "qabstracteventdispatcher.h"
#include "qapplication.h"
#include "qdesktopwidget.h"
#include "qbitmap.h"
#include "qiodevice.h"
#include "qbuffer.h"
#include "qtextcodec.h"
#include "qlist.h"
#include "qmap.h"
#include "qapplication_p.h"
#include "qevent.h"
#include "qt_x11_p.h"
#include "qx11info_x11.h"
#include "qimagewriter.h"
#include "qelapsedtimer.h"
#include "qvariant.h"
#include "qdnd_p.h"
#include <private/qwidget_p.h>

#ifndef QT_NO_XFIXES
#include <X11/extensions/Xfixes.h>
#endif // QT_NO_XFIXES

QT_BEGIN_NAMESPACE

/*****************************************************************************
  Internal QClipboard functions for X11.
 *****************************************************************************/

static int clipboard_timeout = 5000; // 5s timeout on clipboard operations

static QWidget * owner = 0;
static QWidget *requestor = 0;
static bool timer_event_clear = false;
static int timer_id = 0;

static int pending_timer_id = 0;
static bool pending_clipboard_changed = false;
static bool pending_selection_changed = false;


// event capture mechanism for qt_xclb_wait_for_event
static bool waiting_for_data = false;
static bool has_captured_event = false;
static Window capture_event_win = XNone;
static int capture_event_type = -1;
static XEvent captured_event;

class QClipboardWatcher; // forward decl
static QClipboardWatcher *selection_watcher = 0;
static QClipboardWatcher *clipboard_watcher = 0;

static void cleanup()
{
    delete owner;
    delete requestor;
    owner = 0;
    requestor = 0;
}

static
void setupOwner()
{
    if (owner)
        return;
    owner = new QWidget(0);
    owner->setObjectName(QLatin1String("internal clipboard owner"));
    owner->createWinId();
    requestor = new QWidget(0);
    requestor->createWinId();
    requestor->setObjectName(QLatin1String("internal clipboard requestor"));
    // We don't need this internal widgets to appear in QApplication::topLevelWidgets()
    if (QWidgetPrivate::allWidgets) {
        QWidgetPrivate::allWidgets->remove(owner);
        QWidgetPrivate::allWidgets->remove(requestor);
    }
    qAddPostRoutine(cleanup);
}


class QClipboardWatcher : public QInternalMimeData {
public:
    QClipboardWatcher(QClipboard::Mode mode);
    ~QClipboardWatcher();
    bool empty() const;
    virtual bool hasFormat_sys(const QString &mimetype) const;
    virtual QStringList formats_sys() const;

    QVariant retrieveData_sys(const QString &mimetype, QVariant::Type type) const;
    QByteArray getDataInFormat(Atom fmtatom) const;

    Atom atom;
    mutable QStringList formatList;
    mutable QByteArray format_atoms;
};

class QClipboardData
{
private:
    QMimeData *&mimeDataRef() const
    {
        if(mode == QClipboard::Selection)
            return selectionData;
        return clipboardData;
    }

public:
    QClipboardData(QClipboard::Mode mode);
    ~QClipboardData();

    void setSource(QMimeData* s)
    {
        if ((mode == QClipboard::Selection && selectionData == s)
            || clipboardData == s) {
            return;
        }

        if (selectionData != clipboardData) {
            delete mimeDataRef();
        }

        mimeDataRef() = s;
    }

    QMimeData *source() const
    {
        return mimeDataRef();
    }

    void clear()
    {
        timestamp = CurrentTime;
        if (selectionData == clipboardData) {
            mimeDataRef() = 0;
        } else {
            QMimeData *&src = mimeDataRef();
            delete src;
            src = 0;
        }
    }

    static QMimeData *selectionData;
    static QMimeData *clipboardData;
    Time timestamp;
    QClipboard::Mode mode;
};

QMimeData *QClipboardData::selectionData = 0;
QMimeData *QClipboardData::clipboardData = 0;

QClipboardData::QClipboardData(QClipboard::Mode clipboardMode)
{
    timestamp = CurrentTime;
    mode = clipboardMode;
}

QClipboardData::~QClipboardData()
{ clear(); }


static QClipboardData *internalCbData = 0;
static QClipboardData *internalSelData = 0;

static void cleanupClipboardData()
{
    delete internalCbData;
    internalCbData = 0;
}

static QClipboardData *clipboardData()
{
    if (internalCbData == 0) {
        internalCbData = new QClipboardData(QClipboard::Clipboard);
        qAddPostRoutine(cleanupClipboardData);
    }
    return internalCbData;
}

static void cleanupSelectionData()
{
    delete internalSelData;
    internalSelData = 0;
}

static QClipboardData *selectionData()
{
    if (internalSelData == 0) {
        internalSelData = new QClipboardData(QClipboard::Selection);
        qAddPostRoutine(cleanupSelectionData);
    }
    return internalSelData;
}

class QClipboardINCRTransaction
{
public:
    QClipboardINCRTransaction(Window w, Atom p, Atom t, int f, QByteArray d, unsigned int i);
    ~QClipboardINCRTransaction(void);

    int x11Event(XEvent *event);

    Window window;
    Atom property, target;
    int format;
    QByteArray data;
    unsigned int increment;
    unsigned int offset;
};

typedef QMap<Window,QClipboardINCRTransaction*> TransactionMap;
static TransactionMap *transactions = 0;
static QApplication::EventFilter prev_event_filter = 0;
static int incr_timer_id = 0;

static bool qt_x11_incr_event_filter(void *message, long *result)
{
    XEvent *event = reinterpret_cast<XEvent *>(message);
    TransactionMap::Iterator it = transactions->find(event->xany.window);
    if (it != transactions->end()) {
        if ((*it)->x11Event(event) != 0)
            return true;
    }
    if (prev_event_filter)
        return prev_event_filter(event, result);
    return false;
}

/*
  called when no INCR activity has happened for 'clipboard_timeout'
  milliseconds... we assume that all unfinished transactions have
  timed out and remove everything from the transaction map
*/
static void qt_xclb_incr_timeout(void)
{
    qWarning("QClipboard: Timed out while sending data");

    while (transactions)
        delete *transactions->begin();
}

QClipboardINCRTransaction::QClipboardINCRTransaction(Window w, Atom p, Atom t, int f,
                                                     QByteArray d, unsigned int i)
    : window(w), property(p), target(t), format(f), data(d), increment(i), offset(0u)
{
    DEBUG("QClipboard: sending %d bytes (INCR transaction %p)", d.size(), this);

    XSelectInput(X11->display, window, PropertyChangeMask);

    if (! transactions) {
        VDEBUG("QClipboard: created INCR transaction map");
        transactions = new TransactionMap;
        prev_event_filter = qApp->setEventFilter(qt_x11_incr_event_filter);
        incr_timer_id = QApplication::clipboard()->startTimer(clipboard_timeout);
    }
    transactions->insert(window, this);
}

QClipboardINCRTransaction::~QClipboardINCRTransaction(void)
{
    VDEBUG("QClipboard: destroyed INCR transacton %p", this);

    XSelectInput(X11->display, window, NoEventMask);

    transactions->remove(window);
    if (transactions->isEmpty()) {
        VDEBUG("QClipboard: no more INCR transactions");
        delete transactions;
        transactions = 0;

        (void)qApp->setEventFilter(prev_event_filter);

        if (incr_timer_id != 0) {
            QApplication::clipboard()->killTimer(incr_timer_id);
            incr_timer_id = 0;
        }
    }
}

int QClipboardINCRTransaction::x11Event(XEvent *event)
{
    if (event->type != PropertyNotify
        || (event->xproperty.state != PropertyDelete
            || event->xproperty.atom != property))
        return 0;

    // restart the INCR timer
    if (incr_timer_id) QApplication::clipboard()->killTimer(incr_timer_id);
    incr_timer_id = QApplication::clipboard()->startTimer(clipboard_timeout);

    unsigned int bytes_left = data.size() - offset;
    if (bytes_left > 0) {
        unsigned int xfer = qMin(increment, bytes_left);
        VDEBUG("QClipboard: sending %d bytes, %d remaining (INCR transaction %p)",
               xfer, bytes_left - xfer, this);

        XChangeProperty(X11->display, window, property, target, format,
                        PropModeReplace, (uchar *) data.data() + offset, xfer);
        offset += xfer;
    } else {
        // INCR transaction finished...
        XChangeProperty(X11->display, window, property, target, format,
                        PropModeReplace, (uchar *) data.data(), 0);
        delete this;
    }

    return 1;
}


/*****************************************************************************
  QClipboard member functions for X11.
 *****************************************************************************/

struct qt_init_timestamp_data
{
    Time timestamp;
};

#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

static Bool qt_init_timestamp_scanner(Display*, XEvent *event, XPointer arg)
{
    qt_init_timestamp_data *data =
        reinterpret_cast<qt_init_timestamp_data*>(arg);
    switch(event->type)
    {
    case ButtonPress:
    case ButtonRelease:
        data->timestamp = event->xbutton.time;
        break;
    case MotionNotify:
        data->timestamp = event->xmotion.time;
        break;
    case XKeyPress:
    case XKeyRelease:
        data->timestamp = event->xkey.time;
        break;
    case PropertyNotify:
        data->timestamp = event->xproperty.time;
        break;
    case EnterNotify:
    case LeaveNotify:
        data->timestamp = event->xcrossing.time;
        break;
    case SelectionClear:
        data->timestamp = event->xselectionclear.time;
        break;
    default:
        break;
    }
#ifndef QT_NO_XFIXES
    if (X11->use_xfixes && event->type == (X11->xfixes_eventbase + XFixesSelectionNotify)) {
        XFixesSelectionNotifyEvent *req =
            reinterpret_cast<XFixesSelectionNotifyEvent *>(event);
        data->timestamp = req->selection_timestamp;
    }
#endif
    return false;
}

#if defined(Q_C_CALLBACKS)
}
#endif

QClipboard::QClipboard(QObject *parent)
    : QObject(*new QClipboardPrivate, parent)
{
    // create desktop widget since we need it to get PropertyNotify or
    // XFixesSelectionNotify events when someone changes the
    // clipboard.
    (void)QApplication::desktop();

#ifndef QT_NO_XFIXES
    if (X11->use_xfixes && X11->ptrXFixesSelectSelectionInput) {
        const unsigned long eventMask =
            XFixesSetSelectionOwnerNotifyMask | XFixesSelectionWindowDestroyNotifyMask | XFixesSelectionClientCloseNotifyMask;
        for (int i = 0; i < X11->screenCount; ++i) {
            X11->ptrXFixesSelectSelectionInput(X11->display, QX11Info::appRootWindow(i),
                                               XA_PRIMARY, eventMask);
            X11->ptrXFixesSelectSelectionInput(X11->display, QX11Info::appRootWindow(i),
                                               ATOM(CLIPBOARD), eventMask);
        }
    }
#endif // QT_NO_XFIXES

    if (X11->time == CurrentTime) {
        // send a dummy event to myself to get the timestamp from X11.
        qt_init_timestamp_data data;
        data.timestamp = CurrentTime;
        XEvent ev;
        XCheckIfEvent(X11->display, &ev, &qt_init_timestamp_scanner, (XPointer)&data);
        if (data.timestamp == CurrentTime) {
            setupOwner();
            // We need this value just for completeness, we don't use it.
            long dummy = 0;
            Window ownerId = owner->internalWinId();
            XChangeProperty(X11->display, ownerId,
                            ATOM(CLIP_TEMPORARY), XA_INTEGER, 32,
                            PropModeReplace, (uchar*)&dummy, 1);
            XWindowEvent(X11->display, ownerId, PropertyChangeMask, &ev);
            data.timestamp = ev.xproperty.time;
            XDeleteProperty(X11->display, ownerId, ATOM(CLIP_TEMPORARY));
        }
        X11->time = data.timestamp;
    }
}

void QClipboard::clear(Mode mode)
{
    setMimeData(0, mode);
}


bool QClipboard::supportsMode(Mode mode) const
{
    return (mode == Clipboard || mode == Selection);
}

bool QClipboard::ownsMode(Mode mode) const
{
    if (mode == Clipboard)
        return clipboardData()->timestamp != CurrentTime;
    else if(mode == Selection)
        return selectionData()->timestamp != CurrentTime;
    else
        return false;
}


// event filter function... captures interesting events while
// qt_xclb_wait_for_event is running the event loop
static bool qt_x11_clipboard_event_filter(void *message, long *)
{
    XEvent *event = reinterpret_cast<XEvent *>(message);
    if (event->xany.type == capture_event_type &&
        event->xany.window == capture_event_win) {
        VDEBUG("QClipboard: event_filter(): caught event type %d", event->type);
        has_captured_event = true;
        captured_event = *event;
        return true;
    }
    return false;
}

static Bool checkForClipboardEvents(Display *, XEvent *e, XPointer)
{
    return ((e->type == SelectionRequest && (e->xselectionrequest.selection == XA_PRIMARY
                                             || e->xselectionrequest.selection == ATOM(CLIPBOARD)))
            || (e->type == SelectionClear && (e->xselectionclear.selection == XA_PRIMARY
                                              || e->xselectionclear.selection == ATOM(CLIPBOARD))));
}

bool QX11Data::clipboardWaitForEvent(Window win, int type, XEvent *event, int timeout, bool checkManager)
{
    QElapsedTimer started;
    started.start();
    QElapsedTimer now = started;

    if (QAbstractEventDispatcher::instance()->inherits("QtMotif")
        || QApplication::clipboard()->property("useEventLoopWhenWaiting").toBool()) {
        if (waiting_for_data) {
            Q_ASSERT(!"QClipboard: internal error, qt_xclb_wait_for_event recursed");
            return false;
        }
        waiting_for_data = true;


        has_captured_event = false;
        capture_event_win = win;
        capture_event_type = type;

        QApplication::EventFilter old_event_filter =
            qApp->setEventFilter(qt_x11_clipboard_event_filter);

        do {
            if (XCheckTypedWindowEvent(display, win, type, event)) {
                waiting_for_data = false;
                qApp->setEventFilter(old_event_filter);
                return true;
            }

            if (checkManager && XGetSelectionOwner(X11->display, ATOM(CLIPBOARD_MANAGER)) == XNone)
                return false;

            XSync(X11->display, false);
            usleep(50000);

            now.start();

            QEventLoop::ProcessEventsFlags flags(QEventLoop::ExcludeUserInputEvents
                                                 | QEventLoop::ExcludeSocketNotifiers
                                                 | QEventLoop::WaitForMoreEvents
                                                 | QEventLoop::X11ExcludeTimers);
            QAbstractEventDispatcher *eventDispatcher = QAbstractEventDispatcher::instance();
            eventDispatcher->processEvents(flags);

            if (has_captured_event) {
                waiting_for_data = false;
                *event = captured_event;
                qApp->setEventFilter(old_event_filter);
                return true;
            }
        } while (started.msecsTo(now) < timeout);

        waiting_for_data = false;
        qApp->setEventFilter(old_event_filter);
    } else {
        do {
            if (XCheckTypedWindowEvent(X11->display,win,type,event))
                return true;

            if (checkManager && XGetSelectionOwner(X11->display, ATOM(CLIPBOARD_MANAGER)) == XNone)
                return false;

            // process other clipboard events, since someone is probably requesting data from us
            XEvent e;
            // Pass the event through the event dispatcher filter so that applications
            // which install an event filter on the dispatcher get to handle it first.
            if (XCheckIfEvent(X11->display, &e, checkForClipboardEvents, 0) &&
                !QAbstractEventDispatcher::instance()->filterEvent(&e))
                qApp->x11ProcessEvent(&e);

            now.start();

            XFlush(X11->display);

            // sleep 50 ms, so we don't use up CPU cycles all the time.
            struct timeval usleep_tv;
            usleep_tv.tv_sec = 0;
            usleep_tv.tv_usec = 50000;
            select(0, 0, 0, 0, &usleep_tv);
        } while (started.msecsTo(now) < timeout);
    }
    return false;
}


static inline int maxSelectionIncr(Display *dpy)
{ return XMaxRequestSize(dpy) > 65536 ? 65536*4 : XMaxRequestSize(dpy)*4 - 100; }

bool QX11Data::clipboardReadProperty(Window win, Atom property, bool deleteProperty,
                                     QByteArray *buffer, int *size, Atom *type, int *format)
{
    int           maxsize = maxSelectionIncr(display);
    ulong  bytes_left; // bytes_after
    ulong  length;     // nitems
    uchar *data;
    Atom   dummy_type;
    int    dummy_format;
    int    r;

    if (!type)                                // allow null args
        type = &dummy_type;
    if (!format)
        format = &dummy_format;

    // Don't read anything, just get the size of the property data
    r = XGetWindowProperty(display, win, property, 0, 0, False,
                            AnyPropertyType, type, format,
                            &length, &bytes_left, &data);
    if (r != Success || (type && *type == XNone)) {
        buffer->resize(0);
        return false;
    }
    XFree((char*)data);

    int  offset = 0, buffer_offset = 0, format_inc = 1, proplen = bytes_left;

    VDEBUG("QClipboard: read_property(): initial property length: %d", proplen);

    switch (*format) {
    case 8:
    default:
        format_inc = sizeof(char) / 1;
        break;

    case 16:
        format_inc = sizeof(short) / 2;
        proplen *= sizeof(short) / 2;
        break;

    case 32:
        format_inc = sizeof(long) / 4;
        proplen *= sizeof(long) / 4;
        break;
    }

    int newSize = proplen;
    buffer->resize(newSize);

    bool ok = (buffer->size() == newSize);
    VDEBUG("QClipboard: read_property(): buffer resized to %d", buffer->size());

    if (ok && newSize) {
        // could allocate buffer

        while (bytes_left) {
            // more to read...

            r = XGetWindowProperty(display, win, property, offset, maxsize/4,
                                   False, AnyPropertyType, type, format,
                                   &length, &bytes_left, &data);
            if (r != Success || (type && *type == XNone))
                break;

            offset += length / (32 / *format);
            length *= format_inc * (*format) / 8;

            // Here we check if we get a buffer overflow and tries to
            // recover -- this shouldn't normally happen, but it doesn't
            // hurt to be defensive
            if ((int)(buffer_offset + length) > buffer->size()) {
                length = buffer->size() - buffer_offset;

                // escape loop
                bytes_left = 0;
            }

            memcpy(buffer->data() + buffer_offset, data, length);
            buffer_offset += length;

            XFree((char*)data);
        }

        if (*format == 8 && *type == ATOM(COMPOUND_TEXT)) {
            // convert COMPOUND_TEXT to a multibyte string
            XTextProperty textprop;
            textprop.encoding = *type;
            textprop.format = *format;
            textprop.nitems = buffer_offset;
            textprop.value = (unsigned char *) buffer->data();

            char **list_ret = 0;
            int count;
            if (XmbTextPropertyToTextList(display, &textprop, &list_ret,
                         &count) == Success && count && list_ret) {
                offset = buffer_offset = strlen(list_ret[0]);
                buffer->resize(offset);
                memcpy(buffer->data(), list_ret[0], offset);
            }
            if (list_ret) XFreeStringList(list_ret);
        }
    }

    // correct size, not 0-term.
    if (size)
        *size = buffer_offset;

    VDEBUG("QClipboard: read_property(): buffer size %d, buffer offset %d, offset %d",
           buffer->size(), buffer_offset, offset);

    if (deleteProperty)
        XDeleteProperty(display, win, property);

    XFlush(display);

    return ok;
}

QByteArray QX11Data::clipboardReadIncrementalProperty(Window win, Atom property, int nbytes, bool nullterm)
{
    XEvent event;

    QByteArray buf;
    QByteArray tmp_buf;
    bool alloc_error = false;
    int  length;
    int  offset = 0;

    if (nbytes > 0) {
        // Reserve buffer + zero-terminator (for text data)
        // We want to complete the INCR transfer even if we cannot
        // allocate more memory
        buf.resize(nbytes+1);
        alloc_error = buf.size() != nbytes+1;
    }

    for (;;) {
        XFlush(display);
        if (!clipboardWaitForEvent(win,PropertyNotify,&event,clipboard_timeout))
            break;
        if (event.xproperty.atom != property ||
             event.xproperty.state != PropertyNewValue)
            continue;
        if (X11->clipboardReadProperty(win, property, true, &tmp_buf, &length, 0, 0)) {
            if (length == 0) {                // no more data, we're done
                if (nullterm) {
                    buf.resize(offset+1);
                    buf[offset] = '\0';
                } else {
                    buf.resize(offset);
                }
                return buf;
            } else if (!alloc_error) {
                if (offset+length > (int)buf.size()) {
                    buf.resize(offset+length+65535);
                    if (buf.size() != offset+length+65535) {
                        alloc_error = true;
                        length = buf.size() - offset;
                    }
                }
                memcpy(buf.data()+offset, tmp_buf.constData(), length);
                tmp_buf.resize(0);
                offset += length;
            }
        } else {
            break;
        }
    }

    // timed out ... create a new requestor window, otherwise the requestor
    // could consider next request to be still part of this timed out request
    delete requestor;
    requestor = new QWidget(0);
    requestor->setObjectName(QLatin1String("internal clipboard requestor"));
    // We don't need this internal widget to appear in QApplication::topLevelWidgets()
    if (QWidgetPrivate::allWidgets)
        QWidgetPrivate::allWidgets->remove(requestor);

    return QByteArray();
}

static Atom send_targets_selection(QClipboardData *d, Window window, Atom property)
{
    QVector<Atom> types;
    QStringList formats = QInternalMimeData::formatsHelper(d->source());
    for (int i = 0; i < formats.size(); ++i) {
        QList<Atom> atoms = X11->xdndMimeAtomsForFormat(formats.at(i));
        for (int j = 0; j < atoms.size(); ++j) {
            if (!types.contains(atoms.at(j)))
                types.append(atoms.at(j));
        }
    }
    types.append(ATOM(TARGETS));
    types.append(ATOM(MULTIPLE));
    types.append(ATOM(TIMESTAMP));
    types.append(ATOM(SAVE_TARGETS));

    XChangeProperty(X11->display, window, property, XA_ATOM, 32,
                    PropModeReplace, (uchar *) types.data(), types.size());
    return property;
}

static Atom send_selection(QClipboardData *d, Atom target, Window window, Atom property)
{
    Atom atomFormat = target;
    int dataFormat = 0;
    QByteArray data;

    QByteArray fmt = X11->xdndAtomToString(target);
    if (fmt.isEmpty()) { // Not a MIME type we have
        DEBUG("QClipboard: send_selection(): converting to type '%s' is not supported", fmt.data());
        return XNone;
    }
    DEBUG("QClipboard: send_selection(): converting to type '%s'", fmt.data());

    if (X11->xdndMimeDataForAtom(target, d->source(), &data, &atomFormat, &dataFormat)) {

        VDEBUG("QClipboard: send_selection():\n"
          "    property type %lx\n"
          "    property name '%s'\n"
          "    format %d\n"
          "    %d bytes\n",
          target, X11->xdndMimeAtomToString(atomFormat).toLatin1().data(), dataFormat, data.size());

         // don't allow INCR transfers when using MULTIPLE or to
        // Motif clients (since Motif doesn't support INCR)
        static Atom motif_clip_temporary = ATOM(CLIP_TEMPORARY);
        bool allow_incr = property != motif_clip_temporary;

        // X_ChangeProperty protocol request is 24 bytes
        const int increment = (XMaxRequestSize(X11->display) * 4) - 24;
        if (data.size() > increment && allow_incr) {
            long bytes = data.size();
            XChangeProperty(X11->display, window, property,
                            ATOM(INCR), 32, PropModeReplace, (uchar *) &bytes, 1);

            (void)new QClipboardINCRTransaction(window, property, atomFormat, dataFormat, data, increment);
            return property;
        }

        // make sure we can perform the XChangeProperty in a single request
        if (data.size() > increment)
            return XNone; // ### perhaps use several XChangeProperty calls w/ PropModeAppend?
        int dataSize = data.size() / (dataFormat / 8);
        // use a single request to transfer data
        XChangeProperty(X11->display, window, property, atomFormat,
                        dataFormat, PropModeReplace, (uchar *) data.data(),
                        dataSize);
    }
    return property;
}

/*! \internal
    Internal cleanup for Windows.
*/
void QClipboard::ownerDestroyed()
{ }


/*! \internal
    Internal optimization for Windows.
*/
void QClipboard::connectNotify(const char *)
{ }


bool QClipboard::event(QEvent *e)
{
    if (e->type() == QEvent::Timer) {
        QTimerEvent *te = (QTimerEvent *) e;

        if (waiting_for_data) // should never happen
            return false;

        if (te->timerId() == timer_id) {
            killTimer(timer_id);
            timer_id = 0;

            timer_event_clear = true;
            if (selection_watcher) // clear selection
                selectionData()->clear();
            if (clipboard_watcher) // clear clipboard
                clipboardData()->clear();
            timer_event_clear = false;

            return true;
        } else if (te->timerId() == pending_timer_id) {
            // I hate klipper
            killTimer(pending_timer_id);
            pending_timer_id = 0;

            if (pending_clipboard_changed) {
                pending_clipboard_changed = false;
                clipboardData()->clear();
                emitChanged(QClipboard::Clipboard);
            }
            if (pending_selection_changed) {
                pending_selection_changed = false;
                selectionData()->clear();
                emitChanged(QClipboard::Selection);
            }

            return true;
        } else if (te->timerId() == incr_timer_id) {
            killTimer(incr_timer_id);
            incr_timer_id = 0;

            qt_xclb_incr_timeout();

            return true;
        } else {
            return QObject::event(e);
        }
    } else if (e->type() != QEvent::Clipboard) {
        return QObject::event(e);
    }

    XEvent *xevent = (XEvent *)(((QClipboardEvent *)e)->data());
    Display *dpy = X11->display;

    if (!xevent) {
        // That means application exits and we need to give clipboard
        // content to the clipboard manager.
        // First we check if there is a clipboard manager.
        if (XGetSelectionOwner(X11->display, ATOM(CLIPBOARD_MANAGER)) == XNone
            || !owner)
            return true;

        Window ownerId = owner->internalWinId();
        Q_ASSERT(ownerId);
        // we delete the property so the manager saves all TARGETS.
        XDeleteProperty(X11->display, ownerId, ATOM(_QT_SELECTION));
        XConvertSelection(X11->display, ATOM(CLIPBOARD_MANAGER), ATOM(SAVE_TARGETS),
                          ATOM(_QT_SELECTION), ownerId, X11->time);
        XSync(dpy, false);

        XEvent event;
        // waiting until the clipboard manager fetches the content.
        if (!X11->clipboardWaitForEvent(ownerId, SelectionNotify, &event, 10000, true)) {
            qWarning("QClipboard: Unable to receive an event from the "
                     "clipboard manager in a reasonable time");
        }

        return true;
    }

    switch (xevent->type) {

    case SelectionClear:
        // new selection owner
        if (xevent->xselectionclear.selection == XA_PRIMARY) {
            QClipboardData *d = selectionData();

            // ignore the event if it was generated before we gained selection ownership
            if (d->timestamp != CurrentTime && xevent->xselectionclear.time <= d->timestamp)
                break;

            DEBUG("QClipboard: new selection owner 0x%lx at time %lx (ours %lx)",
                  XGetSelectionOwner(dpy, XA_PRIMARY),
                  xevent->xselectionclear.time, d->timestamp);

            if (! waiting_for_data) {
                d->clear();
                emitChanged(QClipboard::Selection);
            } else {
                pending_selection_changed = true;
                if (! pending_timer_id)
                    pending_timer_id = QApplication::clipboard()->startTimer(0);
            }
        } else if (xevent->xselectionclear.selection == ATOM(CLIPBOARD)) {
            QClipboardData *d = clipboardData();

            // ignore the event if it was generated before we gained selection ownership
            if (d->timestamp != CurrentTime && xevent->xselectionclear.time <= d->timestamp)
                break;

            DEBUG("QClipboard: new clipboard owner 0x%lx at time %lx (%lx)",
                  XGetSelectionOwner(dpy, ATOM(CLIPBOARD)),
                  xevent->xselectionclear.time, d->timestamp);

            if (! waiting_for_data) {
                d->clear();
                emitChanged(QClipboard::Clipboard);
            } else {
                pending_clipboard_changed = true;
                if (! pending_timer_id)
                    pending_timer_id = QApplication::clipboard()->startTimer(0);
            }
        } else {
            qWarning("QClipboard: Unknown SelectionClear event received");
            return false;
        }
        break;

    case SelectionNotify:
        /*
          Something has delivered data to us, but this was not caught
          by QClipboardWatcher::getDataInFormat()

          Just skip the event to prevent Bad Things (tm) from
          happening later on...
        */
        break;

    case SelectionRequest:
        {
            // someone wants our data
            XSelectionRequestEvent *req = &xevent->xselectionrequest;

            if (requestor && req->requestor == requestor->internalWinId())
                break;

            XEvent event;
            event.xselection.type      = SelectionNotify;
            event.xselection.display   = req->display;
            event.xselection.requestor = req->requestor;
            event.xselection.selection = req->selection;
            event.xselection.target    = req->target;
            event.xselection.property  = XNone;
            event.xselection.time      = req->time;

            DEBUG("QClipboard: SelectionRequest from %lx\n"
                  "    selection 0x%lx (%s) target 0x%lx (%s)",
                  req->requestor,
                  req->selection,
                  X11->xdndAtomToString(req->selection).data(),
                  req->target,
                  X11->xdndAtomToString(req->target).data());

            QClipboardData *d;
            if (req->selection == XA_PRIMARY) {
                d = selectionData();
            } else if (req->selection == ATOM(CLIPBOARD)) {
                d = clipboardData();
            } else {
                qWarning("QClipboard: Unknown selection '%lx'", req->selection);
                XSendEvent(dpy, req->requestor, False, NoEventMask, &event);
                break;
            }

            if (! d->source()) {
                qWarning("QClipboard: Cannot transfer data, no data available");
                XSendEvent(dpy, req->requestor, False, NoEventMask, &event);
                break;
            }

            DEBUG("QClipboard: SelectionRequest at time %lx (ours %lx)",
                  req->time, d->timestamp);

            if (d->timestamp == CurrentTime // we don't own the selection anymore
                || (req->time != CurrentTime && req->time < d->timestamp)) {
                DEBUG("QClipboard: SelectionRequest too old");
                XSendEvent(dpy, req->requestor, False, NoEventMask, &event);
                break;
            }

            Atom xa_targets = ATOM(TARGETS);
            Atom xa_multiple = ATOM(MULTIPLE);
            Atom xa_timestamp = ATOM(TIMESTAMP);

            struct AtomPair { Atom target; Atom property; } *multi = 0;
            Atom multi_type = XNone;
            int multi_format = 0;
            int nmulti = 0;
            int imulti = -1;
            bool multi_writeback = false;

            if (req->target == xa_multiple) {
                QByteArray multi_data;
                if (req->property == XNone
                    || !X11->clipboardReadProperty(req->requestor, req->property, false, &multi_data,
                                                   0, &multi_type, &multi_format)
                    || multi_format != 32) {
                    // MULTIPLE property not formatted correctly
                    XSendEvent(dpy, req->requestor, False, NoEventMask, &event);
                    break;
                }
                nmulti = multi_data.size()/sizeof(*multi);
                multi = new AtomPair[nmulti];
                memcpy(multi,multi_data.data(),multi_data.size());
                imulti = 0;
            }

            for (; imulti < nmulti; ++imulti) {
                Atom target;
                Atom property;

                if (multi) {
                    target = multi[imulti].target;
                    property = multi[imulti].property;
                } else {
                    target = req->target;
                    property = req->property;
                    if (property == XNone) // obsolete client
                        property = target;
                }

                Atom ret = XNone;
                if (target == XNone || property == XNone) {
                    ;
                } else if (target == xa_timestamp) {
                    if (d->timestamp != CurrentTime) {
                        XChangeProperty(dpy, req->requestor, property, XA_INTEGER, 32,
                                        PropModeReplace, (uchar *) &d->timestamp, 1);
                        ret = property;
                    } else {
                        qWarning("QClipboard: Invalid data timestamp");
                    }
                } else if (target == xa_targets) {
                    ret = send_targets_selection(d, req->requestor, property);
                } else {
                    ret = send_selection(d, target, req->requestor, property);
                }

                if (nmulti > 0) {
                    if (ret == XNone) {
                        multi[imulti].property = XNone;
                        multi_writeback = true;
                    }
                } else {
                    event.xselection.property = ret;
                    break;
                }
            }

            if (nmulti > 0) {
                if (multi_writeback) {
                    // according to ICCCM 2.6.2 says to put None back
                    // into the original property on the requestor window
                    XChangeProperty(dpy, req->requestor, req->property, multi_type, 32,
                                    PropModeReplace, (uchar *) multi, nmulti * 2);
                }

                delete [] multi;
                event.xselection.property = req->property;
            }

            // send selection notify to requestor
            XSendEvent(dpy, req->requestor, False, NoEventMask, &event);

            DEBUG("QClipboard: SelectionNotify to 0x%lx\n"
                  "    property 0x%lx (%s)",
                  req->requestor, event.xselection.property,
                  X11->xdndAtomToString(event.xselection.property).data());
        }
        break;
    }

    return true;
}






QClipboardWatcher::QClipboardWatcher(QClipboard::Mode mode)
    : QInternalMimeData()
{
    switch (mode) {
    case QClipboard::Selection:
        atom = XA_PRIMARY;
        break;

    case QClipboard::Clipboard:
        atom = ATOM(CLIPBOARD);
        break;

    default:
        qWarning("QClipboardWatcher: Internal error: Unsupported clipboard mode");
        break;
    }

    setupOwner();
}

QClipboardWatcher::~QClipboardWatcher()
{
    if(selection_watcher == this)
        selection_watcher = 0;
    if(clipboard_watcher == this)
        clipboard_watcher = 0;
}

bool QClipboardWatcher::empty() const
{
    Display *dpy = X11->display;
    Window win = XGetSelectionOwner(dpy, atom);

    if(win == requestor->internalWinId()) {
        qWarning("QClipboardWatcher::empty: Internal error: Application owns the selection");
        return true;
    }

    return win == XNone;
}

QStringList QClipboardWatcher::formats_sys() const
{
    if (empty())
        return QStringList();

    if (!formatList.count()) {
        // get the list of targets from the current clipboard owner - we do this
        // once so that multiple calls to this function don't require multiple
        // server round trips...

        format_atoms = getDataInFormat(ATOM(TARGETS));

        if (format_atoms.size() > 0) {
            Atom *targets = (Atom *) format_atoms.data();
            int size = format_atoms.size() / sizeof(Atom);

            for (int i = 0; i < size; ++i) {
                if (targets[i] == 0)
                    continue;

                QStringList formatsForAtom = X11->xdndMimeFormatsForAtom(targets[i]);
                for (int j = 0; j < formatsForAtom.size(); ++j) {
                    if (!formatList.contains(formatsForAtom.at(j)))
                        formatList.append(formatsForAtom.at(j));
                }
                VDEBUG("    format: %s", X11->xdndAtomToString(targets[i]).data());
                VDEBUG("    data:\n%s\n", getDataInFormat(targets[i]).data());
            }
            DEBUG("QClipboardWatcher::format: %d formats available", formatList.count());
        }
    }

    return formatList;
}

bool QClipboardWatcher::hasFormat_sys(const QString &format) const
{
    QStringList list = formats();
    return list.contains(format);
}

QVariant QClipboardWatcher::retrieveData_sys(const QString &fmt, QVariant::Type requestedType) const
{
    if (fmt.isEmpty() || empty())
        return QByteArray();

    (void)formats(); // trigger update of format list
    DEBUG("QClipboardWatcher::data: fetching format '%s'", fmt.toLatin1().data());

    QList<Atom> atoms;
    Atom *targets = (Atom *) format_atoms.data();
    int size = format_atoms.size() / sizeof(Atom);
    for (int i = 0; i < size; ++i)
        atoms.append(targets[i]);

    QByteArray encoding;
    Atom fmtatom = X11->xdndMimeAtomForFormat(fmt, requestedType, atoms, &encoding);

    if (fmtatom == 0)
        return QVariant();

    return X11->xdndMimeConvertToFormat(fmtatom, getDataInFormat(fmtatom), fmt, requestedType, encoding);
}

QByteArray QClipboardWatcher::getDataInFormat(Atom fmtatom) const
{
    QByteArray buf;

    Display *dpy = X11->display;
    requestor->createWinId();
    Window   win = requestor->internalWinId();
    Q_ASSERT(requestor->testAttribute(Qt::WA_WState_Created));

    DEBUG("QClipboardWatcher::getDataInFormat: selection '%s' format '%s'",
          X11->xdndAtomToString(atom).data(), X11->xdndAtomToString(fmtatom).data());

    XSelectInput(dpy, win, NoEventMask); // don't listen for any events

    XDeleteProperty(dpy, win, ATOM(_QT_SELECTION));
    XConvertSelection(dpy, atom, fmtatom, ATOM(_QT_SELECTION), win, X11->time);
    XSync(dpy, false);

    VDEBUG("QClipboardWatcher::getDataInFormat: waiting for SelectionNotify event");

    XEvent xevent;
    if (!X11->clipboardWaitForEvent(win,SelectionNotify,&xevent,clipboard_timeout) ||
         xevent.xselection.property == XNone) {
        DEBUG("QClipboardWatcher::getDataInFormat: format not available");
        return buf;
    }

    VDEBUG("QClipboardWatcher::getDataInFormat: fetching data...");

    Atom   type;
    XSelectInput(dpy, win, PropertyChangeMask);

    if (X11->clipboardReadProperty(win, ATOM(_QT_SELECTION), true, &buf, 0, &type, 0)) {
        if (type == ATOM(INCR)) {
            int nbytes = buf.size() >= 4 ? *((int*)buf.data()) : 0;
            buf = X11->clipboardReadIncrementalProperty(win, ATOM(_QT_SELECTION), nbytes, false);
        }
    }

    XSelectInput(dpy, win, NoEventMask);

    DEBUG("QClipboardWatcher::getDataInFormat: %d bytes received", buf.size());

    return buf;
}


const QMimeData* QClipboard::mimeData(Mode mode) const
{
    QClipboardData *d = 0;
    switch (mode) {
    case Selection:
        d = selectionData();
        break;
    case Clipboard:
        d = clipboardData();
        break;
    default:
        qWarning("QClipboard::mimeData: unsupported mode '%d'", mode);
        return 0;
    }

    if (! d->source() && ! timer_event_clear) {
        if (mode == Selection) {
            if (! selection_watcher)
                selection_watcher = new QClipboardWatcher(mode);
            d->setSource(selection_watcher);
        } else {
            if (! clipboard_watcher)
                clipboard_watcher = new QClipboardWatcher(mode);
            d->setSource(clipboard_watcher);
        }

        if (! timer_id) {
            // start a zero timer - we will clear cached data when the timer
            // times out, which will be the next time we hit the event loop...
            // that way, the data is cached long enough for calls within a single
            // loop/function, but the data doesn't linger around in case the selection
            // changes
            QClipboard *that = ((QClipboard *) this);
            timer_id = that->startTimer(0);
        }
    }

    return d->source();
}


void QClipboard::setMimeData(QMimeData* src, Mode mode)
{
    Atom atom, sentinel_atom;
    QClipboardData *d;
    switch (mode) {
    case Selection:
        atom = XA_PRIMARY;
        sentinel_atom = ATOM(_QT_SELECTION_SENTINEL);
        d = selectionData();
        break;

    case Clipboard:
        atom = ATOM(CLIPBOARD);
        sentinel_atom = ATOM(_QT_CLIPBOARD_SENTINEL);
        d = clipboardData();
        break;

    default:
        qWarning("QClipboard::setMimeData: unsupported mode '%d'", mode);
        return;
    }

    Display *dpy = X11->display;
    Window newOwner;

    if (! src) { // no data, clear clipboard contents
        newOwner = XNone;
        d->clear();
    } else {
        setupOwner();

        newOwner = owner->internalWinId();

        d->setSource(src);
        d->timestamp = X11->time;
    }

    Window prevOwner = XGetSelectionOwner(dpy, atom);
    // use X11->time, since d->timestamp == CurrentTime when clearing
    XSetSelectionOwner(dpy, atom, newOwner, X11->time);

    if (mode == Selection)
        emitChanged(QClipboard::Selection);
    else
        emitChanged(QClipboard::Clipboard);

    if (XGetSelectionOwner(dpy, atom) != newOwner) {
        qWarning("QClipboard::setData: Cannot set X11 selection owner for %s",
                 X11->xdndAtomToString(atom).data());
        d->clear();
        return;
    }

    // Signal to other Qt processes that the selection has changed
    Window owners[2];
    owners[0] = newOwner;
    owners[1] = prevOwner;
    XChangeProperty(dpy, QApplication::desktop()->screen(0)->internalWinId(),
                     sentinel_atom, XA_WINDOW, 32, PropModeReplace,
                     (unsigned char*)&owners, 2);
}


/*
  Called by the main event loop in qapplication_x11.cpp when either
  the _QT_SELECTION_SENTINEL property has been changed (i.e. when some
  Qt process has performed QClipboard::setData()) or when Xfixes told
  us that an other application changed the selection. If it returns
  true, the QClipBoard dataChanged() signal should be emitted.
*/

bool qt_check_selection_sentinel()
{
    bool doIt = true;
    if (owner && !X11->use_xfixes) {
        /*
          Since the X selection mechanism cannot give any signal when
          the selection has changed, we emulate it (for Qt processes) here.
          The notification should be ignored in case of either
          a) This is the process that did setData (because setData()
          then has already emitted dataChanged())
          b) This is the process that owned the selection when dataChanged()
          was called (because we have then received a SelectionClear event,
          and have already emitted dataChanged() as a result of that)
        */

        unsigned char *retval;
        Atom actualType;
        int actualFormat;
        ulong nitems;
        ulong bytesLeft;

        if (XGetWindowProperty(X11->display,
                               QApplication::desktop()->screen(0)->internalWinId(),
                               ATOM(_QT_SELECTION_SENTINEL), 0, 2, False, XA_WINDOW,
                               &actualType, &actualFormat, &nitems,
                               &bytesLeft, &retval) == Success) {
            Window *owners = (Window *)retval;
            if (actualType == XA_WINDOW && actualFormat == 32 && nitems == 2) {
                Window win = owner->internalWinId();
                if (owners[0] == win || owners[1] == win)
                    doIt = false;
            }

            XFree(owners);
        }
    }

    if (doIt) {
        if (waiting_for_data) {
            pending_selection_changed = true;
            if (! pending_timer_id)
                pending_timer_id = QApplication::clipboard()->startTimer(0);
            doIt = false;
        } else {
            selectionData()->clear();
        }
    }

    return doIt;
}


bool qt_check_clipboard_sentinel()
{
    bool doIt = true;
    if (owner && !X11->use_xfixes) {
        unsigned char *retval;
        Atom actualType;
        int actualFormat;
        unsigned long nitems, bytesLeft;

        if (XGetWindowProperty(X11->display,
                               QApplication::desktop()->screen(0)->internalWinId(),
                               ATOM(_QT_CLIPBOARD_SENTINEL), 0, 2, False, XA_WINDOW,
                               &actualType, &actualFormat, &nitems, &bytesLeft,
                               &retval) == Success) {
            Window *owners = (Window *)retval;
            if (actualType == XA_WINDOW && actualFormat == 32 && nitems == 2) {
                Window win = owner->internalWinId();
                if (owners[0] == win || owners[1] == win)
                    doIt = false;
            }

            XFree(owners);
        }
    }

    if (doIt) {
        if (waiting_for_data) {
            pending_clipboard_changed = true;
            if (! pending_timer_id)
                pending_timer_id = QApplication::clipboard()->startTimer(0);
            doIt = false;
        } else {
            clipboardData()->clear();
        }
    }

    return doIt;
}

bool qt_xfixes_selection_changed(Window selectionOwner, Time timestamp)
{
    QClipboardData *d = selectionData();
#ifdef QCLIPBOARD_DEBUG
    DEBUG("qt_xfixes_selection_changed: owner = %u; selectionOwner = %u; internal timestamp = %u; external timestamp = %u",
          (unsigned int)(owner ? (int)owner->internalWinId() : 0), (unsigned int)selectionOwner,
          (unsigned int)(d ? d->timestamp : 0), (unsigned int)timestamp);
#endif
    if (!owner || (selectionOwner && selectionOwner != owner->internalWinId()) ||
        (!selectionOwner && (d->timestamp == CurrentTime || d->timestamp < timestamp)))
        return qt_check_selection_sentinel();
    return false;
}

bool qt_xfixes_clipboard_changed(Window clipboardOwner, Time timestamp)
{
    QClipboardData *d = clipboardData();
#ifdef QCLIPBOARD_DEBUG
    DEBUG("qt_xfixes_clipboard_changed: owner = %u; clipboardOwner = %u; internal timestamp = %u; external timestamp = %u",
          (unsigned int)(owner ? (int)owner->internalWinId() : 0), (unsigned int)clipboardOwner,
          (unsigned int)(d ? d->timestamp : 0), (unsigned int)timestamp);
#endif
    if (!owner || (clipboardOwner && clipboardOwner != owner->internalWinId()) ||
        (!clipboardOwner && (d->timestamp == CurrentTime || d->timestamp < timestamp)))
        return qt_check_clipboard_sentinel();
    return false;
}

QT_END_NAMESPACE

#endif // QT_NO_CLIPBOARD
