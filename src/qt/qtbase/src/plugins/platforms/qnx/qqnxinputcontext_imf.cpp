/***************************************************************************
**
** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
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

#include "qqnxinputcontext_imf.h"
#include "qqnxabstractvirtualkeyboard.h"
#include "qqnxintegration.h"
#include "qqnxscreen.h"
#include "qqnxscreeneventhandler.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QInputMethodEvent>
#include <QtGui/QTextCharFormat>

#include <QtCore/QDebug>
#include <QtCore/QMutex>
#include <QtCore/QVariant>
#include <QtCore/QVariantHash>
#include <QtCore/QWaitCondition>
#include <QtCore/QQueue>
#include <QtCore/QGlobalStatic>

#include <dlfcn.h>
#include "imf/imf_client.h"
#include "imf/input_control.h"
#include <process.h>
#include <sys/keycodes.h>

#if defined(QQNXINPUTCONTEXT_IMF_EVENT_DEBUG)
#define qInputContextIMFRequestDebug qDebug
#else
#define qInputContextIMFRequestDebug QT_NO_QDEBUG_MACRO
#endif

#if defined(QQNXINPUTCONTEXT_DEBUG)
#define qInputContextDebug qDebug
#else
#define qInputContextDebug QT_NO_QDEBUG_MACRO
#endif

static QQnxInputContext *sInputContextInstance;
static QColor sSelectedColor(0,0xb8,0,85);

static const input_session_t *sSpellCheckSession = 0;
static const input_session_t *sInputSession = 0;
static bool isSessionOkay(input_session_t *ic)
{
    return ic !=0 && sInputSession != 0 && ic->component_id == sInputSession->component_id;
}

enum ImfEventType
{
    ImfCommitText,
    ImfDeleteSurroundingText,
    ImfFinishComposingText,
    ImfGetCursorPosition,
    ImfGetTextAfterCursor,
    ImfGetTextBeforeCursor,
    ImfSendEvent,
    ImfSetComposingRegion,
    ImfSetComposingText,
    ImfIsTextSelected,
    ImfIsAllTextSelected,
};

struct SpellCheckInfo {
    SpellCheckInfo(void *_context, void (*_spellCheckDone)(void *, const QString &, const QList<int> &))
        : context(_context), spellCheckDone(_spellCheckDone) {}
    void *context;
    void (*spellCheckDone)(void *, const QString &, const QList<int> &);
};
Q_GLOBAL_STATIC(QQueue<SpellCheckInfo>, sSpellCheckQueue)

// IMF requests all arrive on IMF's own thread and have to be posted to the main thread to be processed.
class QQnxImfRequest
{
public:
    QQnxImfRequest(input_session_t *_session, ImfEventType _type)
        : session(_session), type(_type)
        { }
    ~QQnxImfRequest() { }

    input_session_t *session;
    ImfEventType type;
    union {
        struct {
            int32_t n;
            int32_t flags;
            bool before;
            spannable_string_t *result;
        } gtac; // ic_get_text_before_cursor/ic_get_text_after_cursor
        struct {
            int32_t result;
        } gcp; // ic_get_cursor_position
        struct {
            int32_t start;
            int32_t end;
            int32_t result;
        } scr; // ic_set_composing_region
        struct {
            spannable_string_t* text;
            int32_t new_cursor_position;
            int32_t result;
        } sct; // ic_set_composing_text
        struct {
            spannable_string_t* text;
            int32_t new_cursor_position;
            int32_t result;
        } ct; // ic_commit_text
        struct {
            int32_t result;
        } fct; // ic_finish_composing_text
        struct {
            int32_t left_length;
            int32_t right_length;
            int32_t result;
        } dst; // ic_delete_surrounding_text
        struct {
            event_t *event;
            int32_t result;
        } sae;  // ic_send_async_event/ic_send_event
        struct {
            int32_t *pIsSelected;
            int32_t result;
        } its;  // ic_is_text_selected/ic_is_all_text_selected
    };
};

// Invoke an IMF initiated request synchronously on Qt's main thread.  As describe below all
// IMF requests are made from another thread but need to be executed on the main thread.
static void executeIMFRequest(QQnxImfRequest *event)
{
    QMetaObject::invokeMethod(sInputContextInstance,
                              "processImfEvent",
                              Qt::BlockingQueuedConnection,
                              Q_ARG(QQnxImfRequest*, event));
}

// The following functions (ic_*) are callback functions called by the input system to query information
// about the text object that currently has focus or to make changes to it.  All calls are made from the
// input system's own thread.  The pattern for each callback function is to copy its parameters into
// a QQnxImfRequest structure and call executeIMFRequest to have it passed synchronously to Qt's main thread.
// Any return values should be pre-initialised with suitable default values as in some cases
// (e.g. a stale session) the call will return without having executed any request specific code.
//
// To make the correspondence more obvious, the names of these functions match those defined in the headers.
// They're in an anonymous namespace to avoid compiler conflicts with external functions defined with the
// same names.
namespace
{

// See comment at beginning of namespace declaration for general information
static int32_t ic_begin_batch_edit(input_session_t *ic)
{
    Q_UNUSED(ic);

    // Ignore silently.
    return 0;
}

// End composition, committing the supplied text.
// See comment at beginning of namespace declaration for general information
static int32_t ic_commit_text(input_session_t *ic, spannable_string_t *text, int32_t new_cursor_position)
{
    qInputContextIMFRequestDebug() << Q_FUNC_INFO;

    QQnxImfRequest event(ic, ImfCommitText);
    event.ct.text = text;
    event.ct.new_cursor_position = new_cursor_position;
    event.ct.result = -1;
    executeIMFRequest(&event);

    return event.ct.result;
}

// Delete left_length characters before and right_length characters after the cursor.
// See comment at beginning of namespace declaration for general information
static int32_t ic_delete_surrounding_text(input_session_t *ic, int32_t left_length, int32_t right_length)
{
    qInputContextIMFRequestDebug() << Q_FUNC_INFO;

    QQnxImfRequest event(ic, ImfDeleteSurroundingText);
    event.dst.left_length = left_length;
    event.dst.right_length = right_length;
    event.dst.result = -1;
    executeIMFRequest(&event);

    return event.dst.result;
}

// See comment at beginning of namespace declaration for general information
static int32_t ic_end_batch_edit(input_session_t *ic)
{
    Q_UNUSED(ic);

    // Ignore silently.
    return 0;
}

// End composition, committing what's there.
// See comment at beginning of namespace declaration for general information
static int32_t ic_finish_composing_text(input_session_t *ic)
{
    qInputContextIMFRequestDebug() << Q_FUNC_INFO;

    QQnxImfRequest event(ic, ImfFinishComposingText);
    event.fct.result = -1;
    executeIMFRequest(&event);

    return event.fct.result;
}

// Return the position of the cursor.
// See comment at beginning of namespace declaration for general information
static int32_t ic_get_cursor_position(input_session_t *ic)
{
    qInputContextIMFRequestDebug() << Q_FUNC_INFO;

    QQnxImfRequest event(ic, ImfGetCursorPosition);
    event.gcp.result = -1;
    executeIMFRequest(&event);

    return event.gcp.result;
}

// Return the n characters after the cursor.
// See comment at beginning of namespace declaration for general information
static spannable_string_t *ic_get_text_after_cursor(input_session_t *ic, int32_t n, int32_t flags)
{
    qInputContextIMFRequestDebug() << Q_FUNC_INFO;

    QQnxImfRequest event(ic, ImfGetTextAfterCursor);
    event.gtac.n = n;
    event.gtac.flags = flags;
    event.gtac.result = 0;
    executeIMFRequest(&event);

    return event.gtac.result;
}

// Return the n characters before the cursor.
// See comment at beginning of namespace declaration for general information
static spannable_string_t *ic_get_text_before_cursor(input_session_t *ic, int32_t n, int32_t flags)
{
    qInputContextIMFRequestDebug() << Q_FUNC_INFO;

    QQnxImfRequest event(ic, ImfGetTextBeforeCursor);
    event.gtac.n = n;
    event.gtac.flags = flags;
    event.gtac.result = 0;
    executeIMFRequest(&event);

    return event.gtac.result;
}

// Process an event from IMF.  Primarily used for reflecting back keyboard events.
// See comment at beginning of namespace declaration for general information
static int32_t ic_send_event(input_session_t *ic, event_t *event)
{
    qInputContextIMFRequestDebug() << Q_FUNC_INFO;

    QQnxImfRequest imfEvent(ic, ImfSendEvent);
    imfEvent.sae.event = event;
    imfEvent.sae.result = -1;
    executeIMFRequest(&imfEvent);

    return imfEvent.sae.result;
}

// Same as ic_send_event.
// See comment at beginning of namespace declaration for general information
static int32_t ic_send_async_event(input_session_t *ic, event_t *event)
{
    qInputContextIMFRequestDebug() << Q_FUNC_INFO;

    // There's no difference from our point of view between ic_send_event & ic_send_async_event
    QQnxImfRequest imfEvent(ic, ImfSendEvent);
    imfEvent.sae.event = event;
    imfEvent.sae.result = -1;
    executeIMFRequest(&imfEvent);

    return imfEvent.sae.result;
}

// Set the range of text between start and end as the composition range.
// See comment at beginning of namespace declaration for general information
static int32_t ic_set_composing_region(input_session_t *ic, int32_t start, int32_t end)
{
    qInputContextIMFRequestDebug() << Q_FUNC_INFO;

    QQnxImfRequest event(ic, ImfSetComposingRegion);
    event.scr.start = start;
    event.scr.end = end;
    event.scr.result = -1;
    executeIMFRequest(&event);

    return event.scr.result;
}

// Update the composition range with the supplied text.  This can be called when no composition
// range is in effect in which case one is started at the current cursor position.
// See comment at beginning of namespace declaration for general information
static int32_t ic_set_composing_text(input_session_t *ic, spannable_string_t *text, int32_t new_cursor_position)
{
    qInputContextIMFRequestDebug() << Q_FUNC_INFO;

    QQnxImfRequest event(ic, ImfSetComposingText);
    event.sct.text = text;
    event.sct.new_cursor_position = new_cursor_position;
    event.sct.result = -1;
    executeIMFRequest(&event);

    return event.sct.result;
}

// Indicate if any text is selected
// See comment at beginning of namespace declaration for general information
static int32_t ic_is_text_selected(input_session_t* ic, int32_t* pIsSelected)
{
    qInputContextIMFRequestDebug() << Q_FUNC_INFO;

    QQnxImfRequest event(ic, ImfIsTextSelected);
    event.its.pIsSelected = pIsSelected;
    event.its.result = -1;
    executeIMFRequest(&event);

    return event.its.result;
}

// Indicate if all text is selected
// See comment at beginning of namespace declaration for general information
static int32_t ic_is_all_text_selected(input_session_t* ic, int32_t* pIsSelected)
{
    qInputContextIMFRequestDebug() << Q_FUNC_INFO;

    QQnxImfRequest event(ic, ImfIsAllTextSelected);
    event.its.pIsSelected = pIsSelected;
    event.its.result = -1;
    executeIMFRequest(&event);

    return event.its.result;
}

// LCOV_EXCL_START - exclude from code coverage analysis
// The following functions are defined in the IMF headers but are not currently called.

// Not currently used
static int32_t ic_perform_editor_action(input_session_t *ic, int32_t editor_action)
{
    Q_UNUSED(ic);
    Q_UNUSED(editor_action);

    qCritical() << "ic_perform_editor_action not implemented";
    return 0;
}

// Not currently used
static int32_t ic_report_fullscreen_mode(input_session_t *ic, int32_t enabled)
{
    Q_UNUSED(ic);
    Q_UNUSED(enabled);

    qCritical() << "ic_report_fullscreen_mode not implemented";
    return 0;
}

// Not currently used
static extracted_text_t *ic_get_extracted_text(input_session_t *ic, extracted_text_request_t *request, int32_t flags)
{
    Q_UNUSED(ic);
    Q_UNUSED(request);
    Q_UNUSED(flags);

    qCritical() << "ic_get_extracted_text not implemented";
    return 0;
}

// Not currently used
static spannable_string_t *ic_get_selected_text(input_session_t *ic, int32_t flags)
{
    Q_UNUSED(ic);
    Q_UNUSED(flags);

    qCritical() << "ic_get_selected_text not implemented";
    return 0;
}

// Not currently used
static int32_t ic_get_cursor_caps_mode(input_session_t *ic, int32_t req_modes)
{
    Q_UNUSED(ic);
    Q_UNUSED(req_modes);

    qCritical() << "ic_get_cursor_caps_mode not implemented";
    return 0;
}

// Not currently used
static int32_t ic_clear_meta_key_states(input_session_t *ic, int32_t states)
{
    Q_UNUSED(ic);
    Q_UNUSED(states);

    qCritical() << "ic_clear_meta_key_states not implemented";
    return 0;
}

// Not currently used
static int32_t ic_set_selection(input_session_t *ic, int32_t start, int32_t end)
{
    Q_UNUSED(ic);
    Q_UNUSED(start);
    Q_UNUSED(end);

    qCritical() << "ic_set_selection not implemented";
    return 0;
}

// End of un-hittable code
// LCOV_EXCL_STOP


static connection_interface_t ic_funcs = {
    ic_begin_batch_edit,
    ic_clear_meta_key_states,
    ic_commit_text,
    ic_delete_surrounding_text,
    ic_end_batch_edit,
    ic_finish_composing_text,
    ic_get_cursor_caps_mode,
    ic_get_cursor_position,
    ic_get_extracted_text,
    ic_get_selected_text,
    ic_get_text_after_cursor,
    ic_get_text_before_cursor,
    ic_perform_editor_action,
    ic_report_fullscreen_mode,
    0, //ic_send_key_event
    ic_send_event,
    ic_send_async_event,
    ic_set_composing_region,
    ic_set_composing_text,
    ic_set_selection,
    0, //ic_set_candidates,
    0, //ic_get_cursor_offset,
    0, //ic_get_selection,
    ic_is_text_selected,
    ic_is_all_text_selected,
    0, //ic_get_max_cursor_offset_t
};

} // namespace

static void
initEvent(event_t *pEvent, const input_session_t *pSession, EventType eventType, int eventId, int eventSize)
{
    static int s_transactionId;

    // Make sure structure is squeaky clean since it's not clear just what is significant.
    memset(pEvent, 0, eventSize);
    pEvent->event_type = eventType;
    pEvent->event_id = eventId;
    pEvent->pid = getpid();
    pEvent->component_id = pSession->component_id;
    pEvent->transaction_id = ++s_transactionId;
}

static spannable_string_t *toSpannableString(const QString &text)
{
    qInputContextDebug() << Q_FUNC_INFO << text;

    spannable_string_t *pString = static_cast<spannable_string_t *>(malloc(sizeof(spannable_string_t)));
    pString->str =  static_cast<wchar_t *>(malloc(sizeof(wchar_t) * text.length() + 1));
    pString->length = text.toWCharArray(pString->str);
    pString->spans = 0;
    pString->spans_count = 0;
    pString->str[pString->length] = 0;

    return pString;
}


static const input_session_t *(*p_ictrl_open_session)(connection_interface_t *) = 0;
static void (*p_ictrl_close_session)(input_session_t *) = 0;
static int32_t (*p_ictrl_dispatch_event)(event_t*) = 0;
static int32_t (*p_imf_client_init)() = 0;
static void (*p_imf_client_disconnect)() = 0;
static int32_t (*p_vkb_init_selection_service)() = 0;
static int32_t (*p_ictrl_get_num_active_sessions)() = 0;
static bool s_imfInitFailed = false;

static bool imfAvailable()
{
    static bool s_imfDisabled = getenv("DISABLE_IMF") != 0;
    static bool s_imfReady = false;

    if ( s_imfInitFailed || s_imfDisabled)
        return false;
    else if ( s_imfReady )
        return true;

    if ( p_imf_client_init == 0 ) {
        void *handle = dlopen("libinput_client.so.1", 0);
        if ( handle ) {
            p_imf_client_init = (int32_t (*)()) dlsym(handle, "imf_client_init");
            p_imf_client_disconnect = (void (*)()) dlsym(handle, "imf_client_disconnect");
            p_ictrl_open_session = (const input_session_t *(*)(connection_interface_t *))dlsym(handle, "ictrl_open_session");
            p_ictrl_close_session = (void (*)(input_session_t *))dlsym(handle, "ictrl_close_session");
            p_ictrl_dispatch_event = (int32_t (*)(event_t *))dlsym(handle, "ictrl_dispatch_event");
            p_vkb_init_selection_service = (int32_t (*)())dlsym(handle, "vkb_init_selection_service");
            p_ictrl_get_num_active_sessions = (int32_t (*)())dlsym(handle, "ictrl_get_num_active_sessions");
        } else {
            qCritical() << Q_FUNC_INFO << "libinput_client.so.1 is not present - IMF services are disabled.";
            s_imfDisabled = true;
            return false;
        }

        if ( p_imf_client_init && p_ictrl_open_session && p_ictrl_dispatch_event ) {
            s_imfReady = true;
        } else {
            p_ictrl_open_session = 0;
            p_ictrl_dispatch_event = 0;
            s_imfDisabled = true;
            qCritical() << Q_FUNC_INFO << "libinput_client.so.1 did not contain the correct symbols, library mismatch? IMF services are disabled.";
            return false;
        }
    }

    return s_imfReady;
}

QT_BEGIN_NAMESPACE

QQnxInputContext::QQnxInputContext(QQnxIntegration *integration, QQnxAbstractVirtualKeyboard &keyboard) :
         QPlatformInputContext(),
         m_caretPosition(0),
         m_isComposing(false),
         m_isUpdatingText(false),
         m_inputPanelVisible(false),
         m_inputPanelLocale(QLocale::c()),
         m_focusObject(0),
         m_integration(integration),
         m_virtualKeyboard(keyboard)
{
    qInputContextDebug() << Q_FUNC_INFO;

    if (!imfAvailable())
        return;

    // Save a pointer to ourselves so we can execute calls from IMF through executeIMFRequest
    // In practice there will only ever be a single instance.
    Q_ASSERT(sInputContextInstance == 0);
    sInputContextInstance = this;

    if (p_imf_client_init() != 0) {
        s_imfInitFailed = true;
        qCritical("imf_client_init failed - IMF services will be unavailable");
    }

    connect(&keyboard, SIGNAL(visibilityChanged(bool)), this, SLOT(keyboardVisibilityChanged(bool)));
    connect(&keyboard, SIGNAL(localeChanged(QLocale)), this, SLOT(keyboardLocaleChanged(QLocale)));
    keyboardVisibilityChanged(keyboard.isVisible());
    keyboardLocaleChanged(keyboard.locale());
}

QQnxInputContext::~QQnxInputContext()
{
    qInputContextDebug() << Q_FUNC_INFO;

    Q_ASSERT(sInputContextInstance == this);
    sInputContextInstance = 0;

    if (!imfAvailable())
        return;

    p_imf_client_disconnect();
}

bool QQnxInputContext::isValid() const
{
    return imfAvailable();
}

void QQnxInputContext::processImfEvent(QQnxImfRequest *imfEvent)
{
    // If input session is no longer current, just bail, imfEvent should already be set with the appropriate
    // return value.  The only exception is spell check events since they're not associated with the
    // object with focus.
    if (imfEvent->type != ImfSendEvent || imfEvent->sae.event->event_type != EVENT_SPELL_CHECK) {
        if (!isSessionOkay(imfEvent->session))
            return;
    }

    switch (imfEvent->type) {
    case ImfCommitText:
        imfEvent->ct.result = onCommitText(imfEvent->ct.text, imfEvent->ct.new_cursor_position);
        break;

    case ImfDeleteSurroundingText:
        imfEvent->dst.result = onDeleteSurroundingText(imfEvent->dst.left_length, imfEvent->dst.right_length);
        break;

    case ImfFinishComposingText:
        imfEvent->fct.result = onFinishComposingText();
        break;

    case ImfGetCursorPosition:
        imfEvent->gcp.result = onGetCursorPosition();
        break;

    case ImfGetTextAfterCursor:
        imfEvent->gtac.result = onGetTextAfterCursor(imfEvent->gtac.n, imfEvent->gtac.flags);
        break;

    case ImfGetTextBeforeCursor:
        imfEvent->gtac.result = onGetTextBeforeCursor(imfEvent->gtac.n, imfEvent->gtac.flags);
        break;

    case ImfSendEvent:
        imfEvent->sae.result = onSendEvent(imfEvent->sae.event);
        break;

    case ImfSetComposingRegion:
        imfEvent->scr.result = onSetComposingRegion(imfEvent->scr.start, imfEvent->scr.end);
        break;

    case ImfSetComposingText:
        imfEvent->sct.result = onSetComposingText(imfEvent->sct.text, imfEvent->sct.new_cursor_position);
        break;

    case ImfIsTextSelected:
        imfEvent->its.result = onIsTextSelected(imfEvent->its.pIsSelected);
        break;

    case ImfIsAllTextSelected:
        imfEvent->its.result = onIsAllTextSelected(imfEvent->its.pIsSelected);
        break;
    }; //switch
}

bool QQnxInputContext::filterEvent( const QEvent *event )
{
    qInputContextDebug() << Q_FUNC_INFO << event;

    switch (event->type()) {
    case QEvent::CloseSoftwareInputPanel:
        return dispatchCloseSoftwareInputPanel();

    case QEvent::RequestSoftwareInputPanel:
        return dispatchRequestSoftwareInputPanel();

    default:
        return false;
    }
}

QRectF QQnxInputContext::keyboardRect() const
{
    QRect screenGeometry = m_integration->primaryDisplay()->geometry();
    return QRectF(screenGeometry.x(), screenGeometry.height() - m_virtualKeyboard.height(),
                  screenGeometry.width(), m_virtualKeyboard.height());
}

void QQnxInputContext::reset()
{
    qInputContextDebug() << Q_FUNC_INFO;
    endComposition();
}

void QQnxInputContext::commit()
{
    qInputContextDebug() << Q_FUNC_INFO;
    endComposition();
}

void QQnxInputContext::update(Qt::InputMethodQueries queries)
{
    qInputContextDebug() << Q_FUNC_INFO << queries;

    if (queries & Qt::ImCursorPosition) {
        int lastCaret = m_caretPosition;
        updateCursorPosition();
        // If caret position has changed we need to inform IMF unless this is just due to our own action
        // such as committing text.
        if (hasSession() && !m_isUpdatingText && lastCaret != m_caretPosition) {
            caret_event_t caretEvent;
            initEvent(&caretEvent.event, sInputSession, EVENT_CARET, CARET_POS_CHANGED, sizeof(caretEvent));
            caretEvent.old_pos = lastCaret;
            caretEvent.new_pos = m_caretPosition;
            qInputContextDebug() << Q_FUNC_INFO << "ictrl_dispatch_event caret changed" << lastCaret << m_caretPosition;
            p_ictrl_dispatch_event(&caretEvent.event);
        }
    }
}

void QQnxInputContext::closeSession()
{
    qInputContextDebug() << Q_FUNC_INFO;
    if (!imfAvailable())
        return;

    if (sInputSession) {
        p_ictrl_close_session((input_session_t *)sInputSession);
        sInputSession = 0;
    }
    // These are likely already in the right state but this depends on the text control
    // having called reset or commit.  So, just in case, set them to proper values.
    m_isComposing = false;
    m_composingText.clear();
}

bool QQnxInputContext::openSession()
{
    if (!imfAvailable())
        return false;

    closeSession();
    sInputSession = p_ictrl_open_session(&ic_funcs);

    qInputContextDebug() << Q_FUNC_INFO;

    return sInputSession != 0;
}

bool QQnxInputContext::hasSession()
{
    return sInputSession != 0;
}

bool QQnxInputContext::hasSelectedText()
{
    QObject *input = qGuiApp->focusObject();
    if (!input)
        return false;

    QInputMethodQueryEvent query(Qt::ImCurrentSelection);
    QCoreApplication::sendEvent(input, &query);

    return !query.value(Qt::ImCurrentSelection).toString().isEmpty();
}

bool QQnxInputContext::dispatchRequestSoftwareInputPanel()
{
    qInputContextDebug() << Q_FUNC_INFO << "requesting keyboard" << m_inputPanelVisible;
    m_virtualKeyboard.showKeyboard();

    return true;
}

bool QQnxInputContext::dispatchCloseSoftwareInputPanel()
{
    qInputContextDebug() << Q_FUNC_INFO << "hiding keyboard" << m_inputPanelVisible;
    m_virtualKeyboard.hideKeyboard();

    return true;
}

/**
 * IMF Event Dispatchers.
 */
bool QQnxInputContext::dispatchFocusGainEvent(int inputHints)
{
    if (hasSession())
        dispatchFocusLossEvent();

    QObject *input = qGuiApp->focusObject();

    if (!input || !openSession())
        return false;

    // Set the last caret position to 0 since we don't really have one and we don't
    // want to have the old one.
    m_caretPosition = 0;

    QInputMethodQueryEvent query(Qt::ImHints);
    QCoreApplication::sendEvent(input, &query);

    focus_event_t focusEvent;
    initEvent(&focusEvent.event, sInputSession, EVENT_FOCUS, FOCUS_GAINED, sizeof(focusEvent));
    focusEvent.style = DEFAULT_STYLE;

    if (inputHints & Qt::ImhNoPredictiveText)
        focusEvent.style |= NO_PREDICTION | NO_AUTO_CORRECTION;
    if (inputHints & Qt::ImhNoAutoUppercase)
        focusEvent.style |= NO_AUTO_TEXT;

    // Following styles are mutually exclusive
    if (inputHints & Qt::ImhHiddenText) {
        focusEvent.style |= IMF_PASSWORD_TYPE;
    } else if (inputHints & Qt::ImhDialableCharactersOnly) {
        focusEvent.style |= IMF_PHONE_TYPE;
    } else if (inputHints & Qt::ImhUrlCharactersOnly) {
        focusEvent.style |= IMF_URL_TYPE;
    } else if (inputHints & Qt::ImhEmailCharactersOnly) {
        focusEvent.style |= IMF_EMAIL_TYPE;
    }

    qInputContextDebug() << Q_FUNC_INFO << "ictrl_dispatch_event focus gain style:" << focusEvent.style;

    p_ictrl_dispatch_event((event_t *)&focusEvent);

    return true;
}

void QQnxInputContext::dispatchFocusLossEvent()
{
    if (hasSession()) {
        qInputContextDebug() << Q_FUNC_INFO << "ictrl_dispatch_event focus lost";

        focus_event_t focusEvent;
        initEvent(&focusEvent.event, sInputSession, EVENT_FOCUS, FOCUS_LOST, sizeof(focusEvent));
        p_ictrl_dispatch_event((event_t *)&focusEvent);
        closeSession();
    }
}

bool QQnxInputContext::handleKeyboardEvent(int flags, int sym, int mod, int scan, int cap, int sequenceId)
{
    Q_UNUSED(scan);

    if (!hasSession())
        return false;

    int key = (flags & KEY_SYM_VALID) ? sym : cap;
    bool navigationKey = false;
    switch (key) {
    case KEYCODE_RETURN:
         /* In a single line edit we should end composition because enter might be used by something.
            endComposition();
            return false;*/
        break;

    case KEYCODE_BACKSPACE:
    case KEYCODE_DELETE:
        // If there is a selection range, then we want a delete key to operate on that (by
        // deleting the contents of the select range) rather than operating on the composition
        // range.
        if (hasSelectedText())
            return false;
        break;
    case  KEYCODE_LEFT:
        key = NAVIGATE_LEFT;
        navigationKey = true;
        break;
    case  KEYCODE_RIGHT:
        key = NAVIGATE_RIGHT;
        navigationKey = true;
        break;
    case  KEYCODE_UP:
        key = NAVIGATE_UP;
        navigationKey = true;
        break;
    case  KEYCODE_DOWN:
        key = NAVIGATE_DOWN;
        navigationKey = true;
        break;
    case  KEYCODE_LEFT_CTRL:
    case  KEYCODE_RIGHT_CTRL:
    case  KEYCODE_MENU:
    case  KEYCODE_LEFT_HYPER:
    case  KEYCODE_RIGHT_HYPER:
    case  KEYCODE_INSERT:
    case  KEYCODE_HOME:
    case  KEYCODE_PG_UP:
    case  KEYCODE_END:
    case  KEYCODE_PG_DOWN:
        // Don't send these
        key = 0;
        break;
    }

    // Pass the keys we don't know about on through
    if ( key == 0 )
        return false;

    if (navigationKey) {
        // Even if we're forwarding up events, we can't do this for
        // navigation keys.
        if ( flags & KEY_DOWN ) {
            navigation_event_t navEvent;
            initEvent(&navEvent.event, sInputSession, EVENT_NAVIGATION, key, sizeof(navEvent));
            navEvent.magnitude = 1;
            qInputContextDebug() << Q_FUNC_INFO << "ictrl_dispatch_even navigation" << key;
            p_ictrl_dispatch_event(&navEvent.event);
        }
    } else {
        key_event_t keyEvent;
        initEvent(&keyEvent.event, sInputSession, EVENT_KEY, flags & KEY_DOWN ? IMF_KEY_DOWN : IMF_KEY_UP,
                  sizeof(keyEvent));
        keyEvent.key_code = cap;
        keyEvent.character = sym;
        keyEvent.meta_key_state = mod;
        keyEvent.sequence_id = sequenceId;

        p_ictrl_dispatch_event(&keyEvent.event);
        qInputContextDebug() << Q_FUNC_INFO << "ictrl_dispatch_even key" << key;
    }

    return true;
}

void QQnxInputContext::updateCursorPosition()
{
    QObject *input = qGuiApp->focusObject();
    if (!input)
        return;

    QInputMethodQueryEvent query(Qt::ImCursorPosition);
    QCoreApplication::sendEvent(input, &query);
    m_caretPosition = query.value(Qt::ImCursorPosition).toInt();

    qInputContextDebug() << Q_FUNC_INFO << m_caretPosition;
}

void QQnxInputContext::endComposition()
{
    if (!m_isComposing)
        return;

    finishComposingText();

    if (hasSession()) {
        action_event_t actionEvent;
        initEvent(&actionEvent.event, sInputSession, EVENT_ACTION, ACTION_END_COMPOSITION, sizeof(actionEvent));
        qInputContextDebug() << Q_FUNC_INFO << "ictrl_dispatch_even end composition";
        p_ictrl_dispatch_event(&actionEvent.event);
    }
}

void QQnxInputContext::updateComposition(spannable_string_t *text, int32_t new_cursor_position)
{
   QObject *input = qGuiApp->focusObject();
   if (!input)
       return;

   if (new_cursor_position > 0)
       new_cursor_position += text->length - 1;

    m_composingText = QString::fromWCharArray(text->str, text->length);
    m_isComposing = true;

    qInputContextDebug() << Q_FUNC_INFO << m_composingText << new_cursor_position;

    QList<QInputMethodEvent::Attribute> attributes;
    attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::Cursor,
                                                   new_cursor_position,
                                                   1,
                                                   QVariant()));

    for (unsigned int i = 0; i < text->spans_count; ++i) {
        QColor highlightColor;
        bool underline = false;

        if ((text->spans[i].attributes_mask & COMPOSED_TEXT_ATTRIB) != 0)
            underline = true;

        if ((text->spans[i].attributes_mask & ACTIVE_REGION_ATTRIB) != 0) {
            underline = true;
            highlightColor = m_highlightColor[ActiveRegion];
        } else if ((text->spans[i].attributes_mask & AUTO_CORRECTION_ATTRIB) != 0) {
            highlightColor = m_highlightColor[AutoCorrected];
        } else if ((text->spans[i].attributes_mask & REVERT_CORRECTION_ATTRIB) != 0) {
            highlightColor = m_highlightColor[Reverted];
        }

        if (underline || highlightColor.isValid()) {
            QTextCharFormat format;
            if (underline)
                format.setFontUnderline(true);
            if (highlightColor.isValid())
                format.setBackground(QBrush(highlightColor));
            qInputContextDebug() << "    attrib:  " << underline << highlightColor << text->spans[i].start << text->spans[i].end;
            attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat, text->spans[i].start,
                                                           text->spans[i].end - text->spans[i].start + 1, QVariant(format)));

        }
    }
    QInputMethodEvent event(m_composingText, attributes);
    m_isUpdatingText = true;
    QCoreApplication::sendEvent(input, &event);
    m_isUpdatingText = false;

    updateCursorPosition();
}

void QQnxInputContext::finishComposingText()
{
    QObject *input = qGuiApp->focusObject();

    if (input) {
        qInputContextDebug() << Q_FUNC_INFO << m_composingText;

        QInputMethodEvent event;
        event.setCommitString(m_composingText);
        m_isUpdatingText = true;
        QCoreApplication::sendEvent(input, &event);
        m_isUpdatingText = false;
    }
    m_composingText = QString();
    m_isComposing = false;

    updateCursorPosition();
}

// Return the index relative to a UTF-16 sequence of characters for a index that is relative to the
// corresponding UTF-32 character string given a starting index in the UTF-16 string and a count
// of the number of lead surrogates prior to that index.  Updates the highSurrogateCount to reflect the
// new surrogate characters encountered.
static int adjustIndex(const QChar *text, int utf32Index, int utf16StartIndex, int *highSurrogateCount)
{
    int utf16Index = utf32Index + *highSurrogateCount;
    while (utf16StartIndex <  utf16Index) {
        if (text[utf16StartIndex].isHighSurrogate()) {
            ++utf16Index;
            ++*highSurrogateCount;
        }
        ++utf16StartIndex;
    }
    return utf16StartIndex;
}

int QQnxInputContext::handleSpellCheck(spell_check_event_t *event)
{
    // These should never happen.
    if (sSpellCheckQueue->isEmpty() || event->event.event_id != NOTIFY_SP_CHECK_MISSPELLINGS)
        return -1;

    SpellCheckInfo callerInfo = sSpellCheckQueue->dequeue();
    spannable_string_t* spellCheckData = *event->data;
    QString text = QString::fromWCharArray(spellCheckData->str, spellCheckData->length);
    // Generate the list of indices indicating misspelled words in the text.  We use end + 1
    // since it's more conventional to have the end index point just past the string.  We also
    // can't use the indices directly since they are relative to UTF-32 encoded data and the
    // conversion to Qt's UTF-16 internal format might cause lengthening.
    QList<int> indices;
    int adjustment = 0;
    int index = 0;
    for (unsigned int i = 0; i < spellCheckData->spans_count; ++i) {
        if (spellCheckData->spans[i].attributes_mask & MISSPELLED_WORD_ATTRIB) {
            index = adjustIndex(text.data(), spellCheckData->spans[i].start, index, &adjustment);
            indices.push_back(index);
            index = adjustIndex(text.data(), spellCheckData->spans[i].end + 1, index, &adjustment);
            indices.push_back(index);
        }
    }
    callerInfo.spellCheckDone(callerInfo.context, text, indices);

    return 0;
}

int32_t QQnxInputContext::processEvent(event_t *event)
{
    int32_t result = -1;
    switch (event->event_type) {
    case EVENT_SPELL_CHECK: {
        qInputContextDebug() << Q_FUNC_INFO << "EVENT_SPELL_CHECK";
        result = handleSpellCheck(reinterpret_cast<spell_check_event_t *>(event));
        break;
    }

    case EVENT_NAVIGATION: {
        qInputContextDebug() << Q_FUNC_INFO << "EVENT_NAVIGATION";

        int key = event->event_id == NAVIGATE_UP ? KEYCODE_UP :
            event->event_id == NAVIGATE_DOWN ? KEYCODE_DOWN :
            event->event_id == NAVIGATE_LEFT ? KEYCODE_LEFT :
            event->event_id == NAVIGATE_RIGHT ? KEYCODE_RIGHT : 0;

        QQnxScreenEventHandler::injectKeyboardEvent(KEY_DOWN | KEY_CAP_VALID, key, 0, 0, 0);
        QQnxScreenEventHandler::injectKeyboardEvent(KEY_CAP_VALID, key, 0, 0, 0);
        result = 0;
        break;
    }

    case EVENT_KEY: {
        key_event_t *kevent = reinterpret_cast<key_event_t *>(event);
        int keySym = kevent->character != 0 ? kevent->character : kevent->key_code;
        int keyCap = kevent->key_code;
        int modifiers = 0;
        if (kevent->meta_key_state & META_SHIFT_ON)
            modifiers |= KEYMOD_SHIFT;
        int flags = KEY_SYM_VALID | KEY_CAP_VALID;
        if (event->event_id == IMF_KEY_DOWN)
            flags |= KEY_DOWN;
        qInputContextDebug() << Q_FUNC_INFO << "EVENT_KEY" << flags << keySym;
        QQnxScreenEventHandler::injectKeyboardEvent(flags, keySym, modifiers, 0, keyCap);
        result = 0;
        break;
    }

    case EVENT_ACTION:
            // Don't care, indicates that IMF is done.
        break;

    case EVENT_CARET:
    case EVENT_NOTHING:
    case EVENT_FOCUS:
    case EVENT_USER_ACTION:
    case EVENT_STROKE:
    case EVENT_INVOKE_LATER:
        qCritical() << Q_FUNC_INFO << "Unsupported event type: " << event->event_type;
        break;
    default:
        qCritical() << Q_FUNC_INFO << "Unknown event type: " << event->event_type;
    }
    return result;
}

/**
 * IMF Event Handlers
 */

int32_t QQnxInputContext::onCommitText(spannable_string_t *text, int32_t new_cursor_position)
{
    Q_UNUSED(new_cursor_position);

    updateComposition(text, new_cursor_position);
    finishComposingText();

    return 0;
}

int32_t QQnxInputContext::onDeleteSurroundingText(int32_t left_length, int32_t right_length)
{
    qInputContextDebug() << Q_FUNC_INFO << "L:" << left_length << " R:" << right_length;

    QObject *input = qGuiApp->focusObject();
    if (!input)
        return 0;

    int replacementLength = left_length + right_length;
    int replacementStart = -left_length;

    finishComposingText();

    QInputMethodEvent event;
    event.setCommitString(QString(), replacementStart, replacementLength);
    m_isUpdatingText = true;
    QCoreApplication::sendEvent(input, &event);
    m_isUpdatingText = false;

    updateCursorPosition();

    return 0;
}

int32_t QQnxInputContext::onFinishComposingText()
{
    finishComposingText();

    return 0;
}

int32_t QQnxInputContext::onGetCursorPosition()
{
    qInputContextDebug() << Q_FUNC_INFO;

    QObject *input = qGuiApp->focusObject();
    if (!input)
        return 0;

    updateCursorPosition();

    return m_caretPosition;
}

spannable_string_t *QQnxInputContext::onGetTextAfterCursor(int32_t n, int32_t flags)
{
    Q_UNUSED(flags);
    qInputContextDebug() << Q_FUNC_INFO;

    QObject *input = qGuiApp->focusObject();
    if (!input)
        return toSpannableString("");

    QInputMethodQueryEvent query(Qt::ImCursorPosition | Qt::ImSurroundingText);
    QCoreApplication::sendEvent(input, &query);
    QString text = query.value(Qt::ImSurroundingText).toString();
    m_caretPosition = query.value(Qt::ImCursorPosition).toInt();

    return toSpannableString(text.mid(m_caretPosition, n));
}

spannable_string_t *QQnxInputContext::onGetTextBeforeCursor(int32_t n, int32_t flags)
{
    Q_UNUSED(flags);
    qInputContextDebug() << Q_FUNC_INFO;

    QObject *input = qGuiApp->focusObject();
    if (!input)
        return toSpannableString("");

    QInputMethodQueryEvent query(Qt::ImCursorPosition | Qt::ImSurroundingText);
    QCoreApplication::sendEvent(input, &query);
    QString text = query.value(Qt::ImSurroundingText).toString();
    m_caretPosition = query.value(Qt::ImCursorPosition).toInt();

    if (n < m_caretPosition)
        return toSpannableString(text.mid(m_caretPosition - n, n));
    else
        return toSpannableString(text.mid(0, m_caretPosition));
}

int32_t QQnxInputContext::onSendEvent(event_t *event)
{
    qInputContextDebug() << Q_FUNC_INFO;

    return processEvent(event);
}

int32_t QQnxInputContext::onSetComposingRegion(int32_t start, int32_t end)
{
    QObject *input = qGuiApp->focusObject();
    if (!input)
        return 0;

    QInputMethodQueryEvent query(Qt::ImCursorPosition | Qt::ImSurroundingText);
    QCoreApplication::sendEvent(input, &query);
    QString text = query.value(Qt::ImSurroundingText).toString();
    m_caretPosition = query.value(Qt::ImCursorPosition).toInt();

    qInputContextDebug() << Q_FUNC_INFO << text;

    m_isUpdatingText = true;

    // Delete the current text.
    QInputMethodEvent deleteEvent;
    deleteEvent.setCommitString(QString(), start - m_caretPosition, end - start);
    QCoreApplication::sendEvent(input, &deleteEvent);

    m_composingText = text.mid(start, end - start);
    m_isComposing = true;

    QList<QInputMethodEvent::Attribute> attributes;
    QTextCharFormat format;
    format.setFontUnderline(true);
    attributes.push_back(QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat, 0, m_composingText.length(), format));

    QInputMethodEvent setTextEvent(m_composingText, attributes);
    QCoreApplication::sendEvent(input, &setTextEvent);

    m_isUpdatingText = false;

    return 0;
}

int32_t QQnxInputContext::onSetComposingText(spannable_string_t *text, int32_t new_cursor_position)
{
    if (text->length > 0) {
        updateComposition(text, new_cursor_position);
    } else {
        // If the composing text is empty we can simply end composition, the visual effect is the same.
        // However, sometimes one wants to display hint text in an empty text field and for this to work
        // QQuickTextEdit.inputMethodComposing has to be false if the composition string is empty.
        m_composingText.clear();
        finishComposingText();
    }
    return 0;
}

int32_t QQnxInputContext::onIsTextSelected(int32_t* pIsSelected)
{
    *pIsSelected = hasSelectedText();

    qInputContextDebug() << Q_FUNC_INFO << *pIsSelected;

    return 0;
}

int32_t QQnxInputContext::onIsAllTextSelected(int32_t* pIsSelected)
{
    QObject *input = qGuiApp->focusObject();
    if (!input)
        return -1;

    QInputMethodQueryEvent query(Qt::ImCurrentSelection | Qt::ImSurroundingText);
    QCoreApplication::sendEvent(input, &query);

    *pIsSelected = query.value(Qt::ImSurroundingText).toString().length() == query.value(Qt::ImCurrentSelection).toString().length();

    qInputContextDebug() << Q_FUNC_INFO << *pIsSelected;

    return 0;
}

void QQnxInputContext::showInputPanel()
{
    qInputContextDebug() << Q_FUNC_INFO;
    dispatchRequestSoftwareInputPanel();
}

void QQnxInputContext::hideInputPanel()
{
    qInputContextDebug() << Q_FUNC_INFO;
    dispatchCloseSoftwareInputPanel();
}

bool QQnxInputContext::isInputPanelVisible() const
{
    return m_inputPanelVisible;
}

QLocale QQnxInputContext::locale() const
{
    return m_inputPanelLocale;
}

void QQnxInputContext::keyboardVisibilityChanged(bool visible)
{
    qInputContextDebug() << Q_FUNC_INFO << "visible=" << visible;
    if (m_inputPanelVisible != visible) {
        m_inputPanelVisible = visible;
        emitInputPanelVisibleChanged();
    }
}

void QQnxInputContext::keyboardLocaleChanged(const QLocale &locale)
{
    qInputContextDebug() << Q_FUNC_INFO << "locale=" << locale;
    if (m_inputPanelLocale != locale) {
        m_inputPanelLocale = locale;
        emitLocaleChanged();
    }
}

void QQnxInputContext::setHighlightColor(int index, const QColor &color)
{
    qInputContextDebug() << Q_FUNC_INFO << "setHighlightColor" << index << color << qGuiApp->focusObject();

    if (!sInputContextInstance)
        return;

    // If the focus has changed, revert all colors to the default.
    if (sInputContextInstance->m_focusObject != qGuiApp->focusObject()) {
        QColor invalidColor;
        sInputContextInstance->m_highlightColor[ActiveRegion] = sSelectedColor;
        sInputContextInstance->m_highlightColor[AutoCorrected] = invalidColor;
        sInputContextInstance->m_highlightColor[Reverted] = invalidColor;
        sInputContextInstance->m_focusObject = qGuiApp->focusObject();
    }
    if (index >= 0 && index <= Reverted)
        sInputContextInstance->m_highlightColor[index] = color;
}

void QQnxInputContext::setFocusObject(QObject *object)
{
    qInputContextDebug() << Q_FUNC_INFO << "input item=" << object;

    // Ensure the colors are reset if we've a change in focus object
    setHighlightColor(-1, QColor());

    if (!inputMethodAccepted()) {
        if (m_inputPanelVisible)
            hideInputPanel();
        if (hasSession())
            dispatchFocusLossEvent();
    } else {
        QInputMethodQueryEvent query(Qt::ImHints);
        QCoreApplication::sendEvent(object, &query);
        int inputHints = query.value(Qt::ImHints).toInt();

        dispatchFocusGainEvent(inputHints);

        m_virtualKeyboard.setInputHints(inputHints);

        if (!m_inputPanelVisible)
            showInputPanel();
    }
}

bool QQnxInputContext::checkSpelling(const QString &text, void *context, void (*spellCheckDone)(void *context, const QString &text, const QList<int> &indices))
{
    qInputContextDebug() << Q_FUNC_INFO << "text" << text;

    if (!imfAvailable())
        return false;

    if (!sSpellCheckSession)
        sSpellCheckSession = p_ictrl_open_session(&ic_funcs);

    action_event_t spellEvent;
    initEvent(&spellEvent.event, sSpellCheckSession, EVENT_ACTION, ACTION_CHECK_MISSPELLINGS, sizeof(spellEvent));
    int len = text.length();
    spellEvent.event_data = alloca(sizeof(wchar_t) * (len + 1));
    spellEvent.length_data = text.toWCharArray(static_cast<wchar_t*>(spellEvent.event_data)) * sizeof(wchar_t);

    int rc = p_ictrl_dispatch_event(reinterpret_cast<event_t*>(&spellEvent));

    if (rc == 0) {
        sSpellCheckQueue->enqueue(SpellCheckInfo(context, spellCheckDone));
        return true;
    }
    return false;
}

QT_END_NAMESPACE
