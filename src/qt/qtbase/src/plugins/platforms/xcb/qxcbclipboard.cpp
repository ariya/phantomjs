/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qxcbclipboard.h"

#include "qxcbconnection.h"
#include "qxcbscreen.h"
#include "qxcbmime.h"

#include <private/qguiapplication_p.h>
#include <QElapsedTimer>

#include <QtCore/QDebug>

#define class class_name // Workaround XCB-ICCCM 3.8 breakage
#include <xcb/xcb_icccm.h>
#undef class

QT_BEGIN_NAMESPACE

#ifndef QT_NO_CLIPBOARD

class QXcbClipboardMime : public QXcbMime
{
    Q_OBJECT
public:
    QXcbClipboardMime(QClipboard::Mode mode, QXcbClipboard *clipboard)
        : QXcbMime()
        , m_clipboard(clipboard)
    {
        switch (mode) {
        case QClipboard::Selection:
            modeAtom = XCB_ATOM_PRIMARY;
            break;

        case QClipboard::Clipboard:
            modeAtom = m_clipboard->atom(QXcbAtom::CLIPBOARD);
            break;

        default:
            qWarning("QXcbClipboardMime: Internal error: Unsupported clipboard mode");
            break;
        }
    }

    void reset()
    {
        formatList.clear();
    }

    bool isEmpty() const
    {
        return m_clipboard->getSelectionOwner(modeAtom) == XCB_NONE;
    }

protected:
    QStringList formats_sys() const
    {
        if (isEmpty())
            return QStringList();

        if (!formatList.count()) {
            QXcbClipboardMime *that = const_cast<QXcbClipboardMime *>(this);
            // get the list of targets from the current clipboard owner - we do this
            // once so that multiple calls to this function don't require multiple
            // server round trips...
            that->format_atoms = m_clipboard->getDataInFormat(modeAtom, m_clipboard->atom(QXcbAtom::TARGETS));

            if (format_atoms.size() > 0) {
                xcb_atom_t *targets = (xcb_atom_t *) format_atoms.data();
                int size = format_atoms.size() / sizeof(xcb_atom_t);

                for (int i = 0; i < size; ++i) {
                    if (targets[i] == 0)
                        continue;

                    QString format = mimeAtomToString(m_clipboard->connection(), targets[i]);
                    if (!formatList.contains(format))
                        that->formatList.append(format);
                }
            }
        }

        return formatList;
    }

    bool hasFormat_sys(const QString &format) const
    {
        QStringList list = formats();
        return list.contains(format);
    }

    QVariant retrieveData_sys(const QString &fmt, QVariant::Type requestedType) const
    {
        if (fmt.isEmpty() || isEmpty())
            return QByteArray();

        (void)formats(); // trigger update of format list

        QList<xcb_atom_t> atoms;
        xcb_atom_t *targets = (xcb_atom_t *) format_atoms.data();
        int size = format_atoms.size() / sizeof(xcb_atom_t);
        for (int i = 0; i < size; ++i)
            atoms.append(targets[i]);

        QByteArray encoding;
        xcb_atom_t fmtatom = mimeAtomForFormat(m_clipboard->connection(), fmt, requestedType, atoms, &encoding);

        if (fmtatom == 0)
            return QVariant();

        return mimeConvertToFormat(m_clipboard->connection(), fmtatom, m_clipboard->getDataInFormat(modeAtom, fmtatom), fmt, requestedType, encoding);
    }
private:

    xcb_atom_t modeAtom;
    QXcbClipboard *m_clipboard;
    QStringList formatList;
    QByteArray format_atoms;
};

class INCRTransaction;
typedef QMap<xcb_window_t,INCRTransaction*> TransactionMap;
static TransactionMap *transactions = 0;

//#define INCR_DEBUG

class INCRTransaction : public QObject
{
    Q_OBJECT
public:
    INCRTransaction(QXcbConnection *c, xcb_window_t w, xcb_atom_t p,
                    QByteArray d, uint i, xcb_atom_t t, int f, int to) :
        conn(c), win(w), property(p), data(d), increment(i),
        target(t), format(f), timeout(to), offset(0)
    {
        const quint32 values[] = { XCB_EVENT_MASK_PROPERTY_CHANGE };
        xcb_change_window_attributes(conn->xcb_connection(), win,
                                     XCB_CW_EVENT_MASK, values);
        if (!transactions) {
#ifdef INCR_DEBUG
            qDebug("INCRTransaction: creating the TransactionMap");
#endif
            transactions = new TransactionMap;
            conn->clipboard()->setProcessIncr(true);
        }
        transactions->insert(win, this);
        abort_timer = startTimer(timeout);
    }

    ~INCRTransaction()
    {
        if (abort_timer)
            killTimer(abort_timer);
        abort_timer = 0;
        transactions->remove(win);
        if (transactions->isEmpty()) {
#ifdef INCR_DEBUG
            qDebug("INCRTransaction: no more INCR transactions left in the TransactionMap");
#endif
            delete transactions;
            transactions = 0;
            conn->clipboard()->setProcessIncr(false);
        }
    }

    void updateIncrProperty(xcb_property_notify_event_t *event, bool &accepted)
    {
        xcb_connection_t *c = conn->xcb_connection();
        if (event->atom == property && event->state == XCB_PROPERTY_DELETE) {
            accepted = true;
            // restart the timer
            if (abort_timer)
                killTimer(abort_timer);
            abort_timer = startTimer(timeout);

            unsigned int bytes_left = data.size() - offset;
            if (bytes_left > 0) {
                unsigned int bytes_to_send = qMin(increment, bytes_left);
#ifdef INCR_DEBUG
                qDebug("INCRTransaction: sending %d bytes, %d remaining (INCR transaction %p)",
                       bytes_to_send, bytes_left - bytes_to_send, this);
#endif
                int dataSize = bytes_to_send / (format / 8);
                xcb_change_property(c, XCB_PROP_MODE_REPLACE, win, property,
                                    target, format, dataSize, data.constData() + offset);
                offset += bytes_to_send;
            } else {
#ifdef INCR_DEBUG
                qDebug("INCRTransaction: INCR transaction %p completed", this);
#endif
                xcb_change_property(c, XCB_PROP_MODE_REPLACE, win, property,
                                    target, format, 0, (const void *)0);
                const quint32 values[] = { XCB_EVENT_MASK_NO_EVENT };
                xcb_change_window_attributes(conn->xcb_connection(), win,
                                             XCB_CW_EVENT_MASK, values);
                // self destroy
                delete this;
            }
        }
    }

protected:
    void timerEvent(QTimerEvent *ev)
    {
        if (ev->timerId() == abort_timer) {
            // this can happen when the X client we are sending data
            // to decides to exit (normally or abnormally)
#ifdef INCR_DEBUG
            qDebug("INCRTransaction: Timed out while sending data to %p", this);
#endif
            delete this;
        }
    }

private:
    QXcbConnection *conn;
    xcb_window_t win;
    xcb_atom_t property;
    QByteArray data;
    uint increment;
    xcb_atom_t target;
    int format;
    int timeout;
    uint offset;
    int abort_timer;
};

const int QXcbClipboard::clipboard_timeout = 5000;

QXcbClipboard::QXcbClipboard(QXcbConnection *c)
    : QXcbObject(c), QPlatformClipboard()
    , m_requestor(XCB_NONE)
    , m_owner(XCB_NONE)
    , m_incr_active(false)
    , m_clipboard_closing(false)
    , m_incr_receive_time(0)
{
    Q_ASSERT(QClipboard::Clipboard == 0);
    Q_ASSERT(QClipboard::Selection == 1);
    m_xClipboard[QClipboard::Clipboard] = 0;
    m_xClipboard[QClipboard::Selection] = 0;
    m_clientClipboard[QClipboard::Clipboard] = 0;
    m_clientClipboard[QClipboard::Selection] = 0;
    m_timestamp[QClipboard::Clipboard] = XCB_CURRENT_TIME;
    m_timestamp[QClipboard::Selection] = XCB_CURRENT_TIME;

    m_screen = connection()->screens().at(connection()->primaryScreen());

    int x = 0, y = 0, w = 3, h = 3;

    m_owner = xcb_generate_id(xcb_connection());
    Q_XCB_CALL(xcb_create_window(xcb_connection(),
                                 XCB_COPY_FROM_PARENT,            // depth -- same as root
                                 m_owner,                        // window id
                                 m_screen->screen()->root,                   // parent window id
                                 x, y, w, h,
                                 0,                               // border width
                                 XCB_WINDOW_CLASS_INPUT_OUTPUT,   // window class
                                 m_screen->screen()->root_visual, // visual
                                 0,                               // value mask
                                 0));                             // value list
#ifndef QT_NO_DEBUG
    QByteArray ba("Qt clipboard window");
    Q_XCB_CALL(xcb_change_property(xcb_connection(),
                                   XCB_PROP_MODE_REPLACE,
                                   m_owner,
                                   atom(QXcbAtom::_NET_WM_NAME),
                                   atom(QXcbAtom::UTF8_STRING),
                                   8,
                                   ba.length(),
                                   ba.constData()));
#endif

    if (connection()->hasXFixes()) {
        const uint32_t mask = XCB_XFIXES_SELECTION_EVENT_MASK_SET_SELECTION_OWNER |
                XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_WINDOW_DESTROY |
                XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_CLIENT_CLOSE;
        Q_XCB_CALL(xcb_xfixes_select_selection_input_checked(xcb_connection(), m_owner, XCB_ATOM_PRIMARY, mask));
        Q_XCB_CALL(xcb_xfixes_select_selection_input_checked(xcb_connection(), m_owner, atom(QXcbAtom::CLIPBOARD), mask));
    }
}

QXcbClipboard::~QXcbClipboard()
{
    m_clipboard_closing = true;
    // Transfer the clipboard content to the clipboard manager if we own a selection
    if (m_timestamp[QClipboard::Clipboard] != XCB_CURRENT_TIME ||
            m_timestamp[QClipboard::Selection] != XCB_CURRENT_TIME) {

        // First we check if there is a clipboard manager.
        xcb_get_selection_owner_cookie_t cookie = xcb_get_selection_owner(xcb_connection(), atom(QXcbAtom::CLIPBOARD_MANAGER));
        xcb_get_selection_owner_reply_t *reply = xcb_get_selection_owner_reply(xcb_connection(), cookie, 0);
        if (reply && reply->owner != XCB_NONE) {
            // we delete the property so the manager saves all TARGETS.
            xcb_delete_property(xcb_connection(), m_owner, atom(QXcbAtom::_QT_SELECTION));
            xcb_convert_selection(xcb_connection(), m_owner, atom(QXcbAtom::CLIPBOARD_MANAGER), atom(QXcbAtom::SAVE_TARGETS),
                                  atom(QXcbAtom::_QT_SELECTION), connection()->time());
            connection()->sync();

            // waiting until the clipboard manager fetches the content.
            if (!waitForClipboardEvent(m_owner, XCB_SELECTION_NOTIFY, clipboard_timeout, true)) {
                qWarning("QXcbClipboard: Unable to receive an event from the "
                         "clipboard manager in a reasonable time");
            }
        }
        free(reply);
    }
}

void QXcbClipboard::incrTransactionPeeker(xcb_generic_event_t *ge, bool &accepted)
{
    uint response_type = ge->response_type & ~0x80;
    if (response_type == XCB_PROPERTY_NOTIFY) {
        xcb_property_notify_event_t *event = (xcb_property_notify_event_t *)ge;
        TransactionMap::Iterator it = transactions->find(event->window);
        if (it != transactions->end()) {
            (*it)->updateIncrProperty(event, accepted);
        }
    }
}

xcb_window_t QXcbClipboard::getSelectionOwner(xcb_atom_t atom) const
{
    xcb_connection_t *c = xcb_connection();
    xcb_get_selection_owner_cookie_t cookie = xcb_get_selection_owner(c, atom);
    xcb_get_selection_owner_reply_t *reply;
    reply = xcb_get_selection_owner_reply(c, cookie, 0);
    xcb_window_t win = reply->owner;
    free(reply);
    return win;
}

xcb_atom_t QXcbClipboard::atomForMode(QClipboard::Mode mode) const
{
    if (mode == QClipboard::Clipboard)
        return atom(QXcbAtom::CLIPBOARD);
    if (mode == QClipboard::Selection)
        return XCB_ATOM_PRIMARY;
    return XCB_NONE;
}

QClipboard::Mode QXcbClipboard::modeForAtom(xcb_atom_t a) const
{
    if (a == XCB_ATOM_PRIMARY)
        return QClipboard::Selection;
    if (a == atom(QXcbAtom::CLIPBOARD))
        return QClipboard::Clipboard;
    // not supported enum value, used to detect errors
    return QClipboard::FindBuffer;
}


QMimeData * QXcbClipboard::mimeData(QClipboard::Mode mode)
{
    if (mode > QClipboard::Selection)
        return 0;

    xcb_window_t clipboardOwner = getSelectionOwner(atomForMode(mode));
    if (clipboardOwner == owner()) {
        return m_clientClipboard[mode];
    } else {
        if (!m_xClipboard[mode])
            m_xClipboard[mode] = new QXcbClipboardMime(mode, this);

        return m_xClipboard[mode];
    }
}

void QXcbClipboard::setMimeData(QMimeData *data, QClipboard::Mode mode)
{
    if (mode > QClipboard::Selection)
        return;

    QXcbClipboardMime *xClipboard = 0;
    // verify if there is data to be cleared on global X Clipboard.
    if (!data) {
        xClipboard = qobject_cast<QXcbClipboardMime *>(mimeData(mode));
        if (xClipboard) {
            if (xClipboard->isEmpty())
                return;
        }
    }

    if (!xClipboard && (m_clientClipboard[mode] == data))
        return;

    xcb_atom_t modeAtom = atomForMode(mode);
    xcb_window_t newOwner = XCB_NONE;

    if (m_clientClipboard[mode]) {
        if (m_clientClipboard[QClipboard::Clipboard] != m_clientClipboard[QClipboard::Selection])
            delete m_clientClipboard[mode];
        m_clientClipboard[mode] = 0;
        m_timestamp[mode] = XCB_CURRENT_TIME;
    }

    if (connection()->time() == XCB_CURRENT_TIME)
        connection()->setTime(connection()->getTimestamp());

    if (data) {
        newOwner = owner();

        m_clientClipboard[mode] = data;
        m_timestamp[mode] = connection()->time();
    }

    xcb_set_selection_owner(xcb_connection(), newOwner, modeAtom, connection()->time());

    if (getSelectionOwner(modeAtom) != newOwner) {
        qWarning("QXcbClipboard::setMimeData: Cannot set X11 selection owner");
    }

    emitChanged(mode);
}

bool QXcbClipboard::supportsMode(QClipboard::Mode mode) const
{
    if (mode <= QClipboard::Selection)
        return true;
    return false;
}

bool QXcbClipboard::ownsMode(QClipboard::Mode mode) const
{
    if (m_owner == XCB_NONE || mode > QClipboard::Selection)
        return false;

    Q_ASSERT(m_timestamp[mode] == XCB_CURRENT_TIME || getSelectionOwner(atomForMode(mode)) == m_owner);

    return m_timestamp[mode] != XCB_CURRENT_TIME;
}

xcb_window_t QXcbClipboard::requestor() const
{
    if (!m_requestor) {
        const int x = 0, y = 0, w = 3, h = 3;
        QXcbClipboard *that = const_cast<QXcbClipboard *>(this);

        xcb_window_t window = xcb_generate_id(xcb_connection());
        Q_XCB_CALL(xcb_create_window(xcb_connection(),
                                     XCB_COPY_FROM_PARENT,            // depth -- same as root
                                     window,                        // window id
                                     m_screen->screen()->root,                   // parent window id
                                     x, y, w, h,
                                     0,                               // border width
                                     XCB_WINDOW_CLASS_INPUT_OUTPUT,   // window class
                                     m_screen->screen()->root_visual, // visual
                                     0,                               // value mask
                                     0));                             // value list
#ifndef QT_NO_DEBUG
        QByteArray ba("Qt clipboard requestor window");
        Q_XCB_CALL(xcb_change_property(xcb_connection(),
                                       XCB_PROP_MODE_REPLACE,
                                       window,
                                       atom(QXcbAtom::_NET_WM_NAME),
                                       atom(QXcbAtom::UTF8_STRING),
                                       8,
                                       ba.length(),
                                       ba.constData()));
#endif

        uint32_t mask = XCB_EVENT_MASK_PROPERTY_CHANGE;
        xcb_change_window_attributes(xcb_connection(), window, XCB_CW_EVENT_MASK, &mask);

        that->setRequestor(window);
    }
    return m_requestor;
}

void QXcbClipboard::setRequestor(xcb_window_t window)
{
    if (m_requestor != XCB_NONE) {
        xcb_destroy_window(xcb_connection(), m_requestor);
    }
    m_requestor = window;
}

xcb_window_t QXcbClipboard::owner() const
{
    return m_owner;
}

xcb_atom_t QXcbClipboard::sendTargetsSelection(QMimeData *d, xcb_window_t window, xcb_atom_t property)
{
    QVector<xcb_atom_t> types;
    QStringList formats = QInternalMimeData::formatsHelper(d);
    for (int i = 0; i < formats.size(); ++i) {
        QList<xcb_atom_t> atoms = QXcbMime::mimeAtomsForFormat(connection(), formats.at(i));
        for (int j = 0; j < atoms.size(); ++j) {
            if (!types.contains(atoms.at(j)))
                types.append(atoms.at(j));
        }
    }
    types.append(atom(QXcbAtom::TARGETS));
    types.append(atom(QXcbAtom::MULTIPLE));
    types.append(atom(QXcbAtom::TIMESTAMP));
    types.append(atom(QXcbAtom::SAVE_TARGETS));

    xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, window, property, XCB_ATOM_ATOM,
                        32, types.size(), (const void *)types.constData());
    return property;
}

xcb_atom_t QXcbClipboard::sendSelection(QMimeData *d, xcb_atom_t target, xcb_window_t window, xcb_atom_t property)
{
    xcb_atom_t atomFormat = target;
    int dataFormat = 0;
    QByteArray data;

    QString fmt = QXcbMime::mimeAtomToString(connection(), target);
    if (fmt.isEmpty()) { // Not a MIME type we have
//        qDebug() << "QClipboard: send_selection(): converting to type" << connection()->atomName(target) << "is not supported";
        return XCB_NONE;
    }
//    qDebug() << "QClipboard: send_selection(): converting to type" << fmt;

    if (QXcbMime::mimeDataForAtom(connection(), target, d, &data, &atomFormat, &dataFormat)) {

         // don't allow INCR transfers when using MULTIPLE or to
        // Motif clients (since Motif doesn't support INCR)
        static xcb_atom_t motif_clip_temporary = atom(QXcbAtom::CLIP_TEMPORARY);
        bool allow_incr = property != motif_clip_temporary;
        // This 'bool' can be removed once there is a proper fix for QTBUG-32853
        if (m_clipboard_closing)
            allow_incr = false;
        // X_ChangeProperty protocol request is 24 bytes
        const int increment = (xcb_get_maximum_request_length(xcb_connection()) * 4) - 24;
        if (data.size() > increment && allow_incr) {
            long bytes = data.size();
            xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, window, property,
                                atom(QXcbAtom::INCR), 32, 1, (const void *)&bytes);
            new INCRTransaction(connection(), window, property, data, increment,
                                atomFormat, dataFormat, clipboard_timeout);
            return property;
        }

        // make sure we can perform the XChangeProperty in a single request
        if (data.size() > increment)
            return XCB_NONE; // ### perhaps use several XChangeProperty calls w/ PropModeAppend?
        int dataSize = data.size() / (dataFormat / 8);
        // use a single request to transfer data
        xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, window, property, atomFormat,
                            dataFormat, dataSize, (const void *)data.constData());
    }
    return property;
}

void QXcbClipboard::handleSelectionClearRequest(xcb_selection_clear_event_t *event)
{
    QClipboard::Mode mode = modeForAtom(event->selection);
    if (mode > QClipboard::Selection)
        return;

    // ignore the event if it was generated before we gained selection ownership
    if (m_timestamp[mode] != XCB_CURRENT_TIME && event->time <= m_timestamp[mode])
        return;

//    DEBUG("QClipboard: new selection owner 0x%lx at time %lx (ours %lx)",
//          XGetSelectionOwner(dpy, XA_PRIMARY),
//          xevent->xselectionclear.time, d->timestamp);

    xcb_window_t newOwner = getSelectionOwner(event->selection);

    /* If selection ownership was given up voluntarily from QClipboard::clear(), then we do nothing here
    since its already handled in setMimeData. Otherwise, the event must have come from another client
    as a result of a call to xcb_set_selection_owner in which case we need to delete the local mime data
    */
    if (newOwner != XCB_NONE) {
        if (m_clientClipboard[QClipboard::Clipboard] != m_clientClipboard[QClipboard::Selection])
            delete m_clientClipboard[mode];
        m_clientClipboard[mode] = 0;
        m_timestamp[mode] = XCB_CURRENT_TIME;
    }
}

void QXcbClipboard::handleSelectionRequest(xcb_selection_request_event_t *req)
{
    if (requestor() && req->requestor == requestor()) {
        qWarning("QXcbClipboard: Selection request should be caught before");
        return;
    }

    xcb_selection_notify_event_t event;
    event.response_type = XCB_SELECTION_NOTIFY;
    event.requestor = req->requestor;
    event.selection = req->selection;
    event.target    = req->target;
    event.property  = XCB_NONE;
    event.time      = req->time;

    QMimeData *d;
    QClipboard::Mode mode = modeForAtom(req->selection);
    if (mode > QClipboard::Selection) {
        qWarning() << "QXcbClipboard: Unknown selection" << connection()->atomName(req->selection);
        xcb_send_event(xcb_connection(), false, req->requestor, XCB_EVENT_MASK_NO_EVENT, (const char *)&event);
        return;
    }

    d = m_clientClipboard[mode];

    if (!d) {
        qWarning("QXcbClipboard: Cannot transfer data, no data available");
        xcb_send_event(xcb_connection(), false, req->requestor, XCB_EVENT_MASK_NO_EVENT, (const char *)&event);
        return;
    }

    if (m_timestamp[mode] == XCB_CURRENT_TIME // we don't own the selection anymore
            || (req->time != XCB_CURRENT_TIME && req->time < m_timestamp[mode])) {
        qWarning("QXcbClipboard: SelectionRequest too old");
        xcb_send_event(xcb_connection(), false, req->requestor, XCB_EVENT_MASK_NO_EVENT, (const char *)&event);
        return;
    }

    xcb_atom_t targetsAtom = atom(QXcbAtom::TARGETS);
    xcb_atom_t multipleAtom = atom(QXcbAtom::MULTIPLE);
    xcb_atom_t timestampAtom = atom(QXcbAtom::TIMESTAMP);

    struct AtomPair { xcb_atom_t target; xcb_atom_t property; } *multi = 0;
    xcb_atom_t multi_type = XCB_NONE;
    int multi_format = 0;
    int nmulti = 0;
    int imulti = -1;
    bool multi_writeback = false;

    if (req->target == multipleAtom) {
        QByteArray multi_data;
        if (req->property == XCB_NONE
            || !clipboardReadProperty(req->requestor, req->property, false, &multi_data,
                                           0, &multi_type, &multi_format)
            || multi_format != 32) {
            // MULTIPLE property not formatted correctly
            xcb_send_event(xcb_connection(), false, req->requestor, XCB_EVENT_MASK_NO_EVENT, (const char *)&event);
            return;
        }
        nmulti = multi_data.size()/sizeof(*multi);
        multi = new AtomPair[nmulti];
        memcpy(multi,multi_data.data(),multi_data.size());
        imulti = 0;
    }

    for (; imulti < nmulti; ++imulti) {
        xcb_atom_t target;
        xcb_atom_t property;

        if (multi) {
            target = multi[imulti].target;
            property = multi[imulti].property;
        } else {
            target = req->target;
            property = req->property;
            if (property == XCB_NONE) // obsolete client
                property = target;
        }

        xcb_atom_t ret = XCB_NONE;
        if (target == XCB_NONE || property == XCB_NONE) {
            ;
        } else if (target == timestampAtom) {
            if (m_timestamp[mode] != XCB_CURRENT_TIME) {
                xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, req->requestor,
                                    property, XCB_ATOM_INTEGER, 32, 1, &m_timestamp[mode]);
                ret = property;
            } else {
                qWarning("QXcbClipboard: Invalid data timestamp");
            }
        } else if (target == targetsAtom) {
            ret = sendTargetsSelection(d, req->requestor, property);
        } else {
            ret = sendSelection(d, target, req->requestor, property);
        }

        if (nmulti > 0) {
            if (ret == XCB_NONE) {
                multi[imulti].property = XCB_NONE;
                multi_writeback = true;
            }
        } else {
            event.property = ret;
            break;
        }
    }

    if (nmulti > 0) {
        if (multi_writeback) {
            // according to ICCCM 2.6.2 says to put None back
            // into the original property on the requestor window
            xcb_change_property(xcb_connection(), XCB_PROP_MODE_REPLACE, req->requestor, req->property,
                                multi_type, 32, nmulti*2, (const void *)multi);
        }

        delete [] multi;
        event.property = req->property;
    }

    // send selection notify to requestor
    xcb_send_event(xcb_connection(), false, req->requestor, XCB_EVENT_MASK_NO_EVENT, (const char *)&event);
}

void QXcbClipboard::handleXFixesSelectionRequest(xcb_xfixes_selection_notify_event_t *event)
{
    QClipboard::Mode mode = modeForAtom(event->selection);
    // here we care only about the xfixes events that come from non Qt processes
    if (event->owner != XCB_NONE && event->owner != owner()) {
        if (!m_xClipboard[mode]) {
            m_xClipboard[mode] = new QXcbClipboardMime(mode, this);
        } else {
            m_xClipboard[mode]->reset();
        }
        emitChanged(mode);
    }
}


static inline int maxSelectionIncr(xcb_connection_t *c)
{
    int l = xcb_get_maximum_request_length(c);
    return (l > 65536 ? 65536*4 : l*4) - 100;
}

bool QXcbClipboard::clipboardReadProperty(xcb_window_t win, xcb_atom_t property, bool deleteProperty, QByteArray *buffer, int *size, xcb_atom_t *type, int *format)
{
    int    maxsize = maxSelectionIncr(xcb_connection());
    ulong  bytes_left; // bytes_after
    xcb_atom_t   dummy_type;
    int    dummy_format;

    if (!type)                                // allow null args
        type = &dummy_type;
    if (!format)
        format = &dummy_format;

    // Don't read anything, just get the size of the property data
    xcb_get_property_cookie_t cookie = Q_XCB_CALL(xcb_get_property(xcb_connection(), false, win, property, XCB_GET_PROPERTY_TYPE_ANY, 0, 0));
    xcb_get_property_reply_t *reply = xcb_get_property_reply(xcb_connection(), cookie, 0);
    if (!reply || reply->type == XCB_NONE) {
        free(reply);
        buffer->resize(0);
        return false;
    }
    *type = reply->type;
    *format = reply->format;
    bytes_left = reply->bytes_after;
    free(reply);

    int  offset = 0, buffer_offset = 0;

    int newSize = bytes_left;
    buffer->resize(newSize);

    bool ok = (buffer->size() == newSize);

    if (ok && newSize) {
        // could allocate buffer

        while (bytes_left) {
            // more to read...

            xcb_get_property_cookie_t cookie = Q_XCB_CALL(xcb_get_property(xcb_connection(), false, win, property, XCB_GET_PROPERTY_TYPE_ANY, offset, maxsize/4));
            reply = xcb_get_property_reply(xcb_connection(), cookie, 0);
            if (!reply || reply->type == XCB_NONE) {
                free(reply);
                break;
            }
            *type = reply->type;
            *format = reply->format;
            bytes_left = reply->bytes_after;
            char *data = (char *)xcb_get_property_value(reply);
            int length = xcb_get_property_value_length(reply);

            // Here we check if we get a buffer overflow and tries to
            // recover -- this shouldn't normally happen, but it doesn't
            // hurt to be defensive
            if ((int)(buffer_offset + length) > buffer->size()) {
                qWarning("QXcbClipboard: buffer overflow");
                length = buffer->size() - buffer_offset;

                // escape loop
                bytes_left = 0;
            }

            memcpy(buffer->data() + buffer_offset, data, length);
            buffer_offset += length;

            if (bytes_left) {
                // offset is specified in 32-bit multiples
                offset += length / 4;
            }
            free(reply);
        }
    }


    // correct size, not 0-term.
    if (size)
        *size = buffer_offset;
    if (*type == atom(QXcbAtom::INCR))
        m_incr_receive_time = connection()->getTimestamp();
    if (deleteProperty)
        xcb_delete_property(xcb_connection(), win, property);

    connection()->flush();

    return ok;
}


namespace
{
    class Notify {
    public:
        Notify(xcb_window_t win, int t)
            : window(win), type(t) {}
        xcb_window_t window;
        int type;
        bool checkEvent(xcb_generic_event_t *event) const {
            if (!event)
                return false;
            int t = event->response_type & 0x7f;
            if (t != type)
                return false;
            if (t == XCB_PROPERTY_NOTIFY) {
                xcb_property_notify_event_t *pn = (xcb_property_notify_event_t *)event;
                if (pn->window == window)
                    return true;
            } else if (t == XCB_SELECTION_NOTIFY) {
                xcb_selection_notify_event_t *sn = (xcb_selection_notify_event_t *)event;
                if (sn->requestor == window)
                    return true;
            }
            return false;
        }
    };
    class ClipboardEvent {
    public:
        ClipboardEvent(QXcbConnection *c)
        { clipboard = c->internAtom("CLIPBOARD"); }
        xcb_atom_t clipboard;
        bool checkEvent(xcb_generic_event_t *e) const {
            if (!e)
                return false;
            int type = e->response_type & 0x7f;
            if (type == XCB_SELECTION_REQUEST) {
                xcb_selection_request_event_t *sr = (xcb_selection_request_event_t *)e;
                return sr->selection == XCB_ATOM_PRIMARY || sr->selection == clipboard;
            } else if (type == XCB_SELECTION_CLEAR) {
                xcb_selection_clear_event_t *sc = (xcb_selection_clear_event_t *)e;
                return sc->selection == XCB_ATOM_PRIMARY || sc->selection == clipboard;
            }
            return false;
        }
    };
}

xcb_generic_event_t *QXcbClipboard::waitForClipboardEvent(xcb_window_t win, int type, int timeout, bool checkManager)
{
    QElapsedTimer timer;
    timer.start();
    do {
        Notify notify(win, type);
        xcb_generic_event_t *e = connection()->checkEvent(notify);
        if (e)
            return e;

        if (checkManager) {
            xcb_get_selection_owner_cookie_t cookie = xcb_get_selection_owner(xcb_connection(), atom(QXcbAtom::CLIPBOARD_MANAGER));
            xcb_get_selection_owner_reply_t *reply = xcb_get_selection_owner_reply(xcb_connection(), cookie, 0);
            if (!reply || reply->owner == XCB_NONE) {
                free(reply);
                return 0;
            }
            free(reply);
        }

        // process other clipboard events, since someone is probably requesting data from us
        ClipboardEvent clipboard(connection());
        e = connection()->checkEvent(clipboard);
        if (e) {
            connection()->handleXcbEvent(e);
            free(e);
        }

        connection()->flush();

        // sleep 50 ms, so we don't use up CPU cycles all the time.
        struct timeval usleep_tv;
        usleep_tv.tv_sec = 0;
        usleep_tv.tv_usec = 50000;
        select(0, 0, 0, 0, &usleep_tv);
    } while (timer.elapsed() < timeout);

    return 0;
}

QByteArray QXcbClipboard::clipboardReadIncrementalProperty(xcb_window_t win, xcb_atom_t property, int nbytes, bool nullterm)
{
    QByteArray buf;
    QByteArray tmp_buf;
    bool alloc_error = false;
    int  length;
    int  offset = 0;
    xcb_timestamp_t prev_time = m_incr_receive_time;

    if (nbytes > 0) {
        // Reserve buffer + zero-terminator (for text data)
        // We want to complete the INCR transfer even if we cannot
        // allocate more memory
        buf.resize(nbytes+1);
        alloc_error = buf.size() != nbytes+1;
    }

    for (;;) {
        connection()->flush();
        xcb_generic_event_t *ge = waitForClipboardEvent(win, XCB_PROPERTY_NOTIFY, clipboard_timeout);
        if (!ge)
            break;
        xcb_property_notify_event_t *event = (xcb_property_notify_event_t *)ge;

        if (event->atom != property
                || event->state != XCB_PROPERTY_NEW_VALUE
                    || event->time < prev_time)
            continue;
        prev_time = event->time;

        if (clipboardReadProperty(win, property, true, &tmp_buf, &length, 0, 0)) {
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

        free(ge);
    }

    // timed out ... create a new requestor window, otherwise the requestor
    // could consider next request to be still part of this timed out request
    setRequestor(0);

    return QByteArray();
}

QByteArray QXcbClipboard::getDataInFormat(xcb_atom_t modeAtom, xcb_atom_t fmtAtom)
{
    return getSelection(modeAtom, fmtAtom, atom(QXcbAtom::_QT_SELECTION));
}

QByteArray QXcbClipboard::getSelection(xcb_atom_t selection, xcb_atom_t target, xcb_atom_t property, xcb_timestamp_t time)
{
    QByteArray buf;
    xcb_window_t win = requestor();

    if (time == 0) time = connection()->time();

    xcb_delete_property(xcb_connection(), win, property);
    xcb_convert_selection(xcb_connection(), win, selection, target, property, time);

    connection()->sync();

    xcb_generic_event_t *ge = waitForClipboardEvent(win, XCB_SELECTION_NOTIFY, clipboard_timeout);
    bool no_selection = !ge || ((xcb_selection_notify_event_t *)ge)->property == XCB_NONE;
    free(ge);

    if (no_selection)
        return buf;

    xcb_atom_t type;
    if (clipboardReadProperty(win, property, true, &buf, 0, &type, 0)) {
        if (type == atom(QXcbAtom::INCR)) {
            int nbytes = buf.size() >= 4 ? *((int*)buf.data()) : 0;
            buf = clipboardReadIncrementalProperty(win, property, nbytes, false);
        }
    }

    return buf;
}

#endif // QT_NO_CLIPBOARD

QT_END_NAMESPACE

#include "qxcbclipboard.moc"
