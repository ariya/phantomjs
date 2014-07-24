/*
 *  Copyright (C) 2012 Igalia S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "GtkInputMethodFilter.h"

#include "GOwnPtrGtk.h"
#include "GtkVersioning.h"
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <wtf/MathExtras.h>

// The Windows composition key event code is 299 or VK_PROCESSKEY. We need to
// emit this code for web compatibility reasons when key events trigger
// composition results. GDK doesn't have an equivalent, so we send VoidSymbol
// here to WebCore. PlatformKeyEvent knows to convert this code into
// VK_PROCESSKEY.
const int gCompositionEventKeyCode = GDK_KEY_VoidSymbol;

namespace WebCore {

static void handleCommitCallback(GtkIMContext*, const char* compositionString, GtkInputMethodFilter* filter)
{
    filter->handleCommit(compositionString);
}

static void handlePreeditStartCallback(GtkIMContext*, GtkInputMethodFilter* filter)
{
    filter->handlePreeditStart();
}

static void handlePreeditChangedCallback(GtkIMContext*, GtkInputMethodFilter* filter)
{
    filter->handlePreeditChanged();
}

static void handlePreeditEndCallback(GtkIMContext*, GtkInputMethodFilter* filter)
{
    filter->handlePreeditEnd();
}

static void handleWidgetRealize(GtkWidget* widget, GtkInputMethodFilter* filter)
{
    GdkWindow* window = gtk_widget_get_window(widget);
    ASSERT(window);
    gtk_im_context_set_client_window(filter->context(), window);
}

void GtkInputMethodFilter::setWidget(GtkWidget* widget)
{
    ASSERT(!m_widget);

    m_widget = widget;
    if (gtk_widget_get_window(m_widget))
        handleWidgetRealize(m_widget, this);
    else
        g_signal_connect_after(widget, "realize", G_CALLBACK(handleWidgetRealize), this);
}

void GtkInputMethodFilter::setCursorRect(const IntRect& cursorRect)
{
    // Don't move the window unless the cursor actually moves more than 10
    // pixels. This prevents us from making the window flash during minor
    // cursor adjustments.
    static const int windowMovementThreshold = 10 * 10;
    if (cursorRect.location().distanceSquaredToPoint(m_lastCareLocation) < windowMovementThreshold)
        return;

    m_lastCareLocation = cursorRect.location();
    IntRect translatedRect = cursorRect;

    ASSERT(m_widget);
    GtkAllocation allocation;
    gtk_widget_get_allocation(m_widget, &allocation);
    translatedRect.move(allocation.x, allocation.y);

    GdkRectangle gdkCursorRect = cursorRect;
    gtk_im_context_set_cursor_location(m_context.get(), &gdkCursorRect);
}

GtkInputMethodFilter::GtkInputMethodFilter()
    : m_cursorOffset(0)
    , m_context(adoptGRef(gtk_im_multicontext_new()))
    , m_widget(0)
    , m_enabled(false)
    , m_composingTextCurrently(false)
    , m_filteringKeyEvent(false)
    , m_preeditChanged(false)
    , m_preventNextCommit(false)
    , m_justSentFakeKeyUp(false)
    , m_lastFilteredKeyPressCodeWithNoResults(GDK_KEY_VoidSymbol)
{
    g_signal_connect(m_context.get(), "commit", G_CALLBACK(handleCommitCallback), this);
    g_signal_connect(m_context.get(), "preedit-start", G_CALLBACK(handlePreeditStartCallback), this);
    g_signal_connect(m_context.get(), "preedit-changed", G_CALLBACK(handlePreeditChangedCallback), this);
    g_signal_connect(m_context.get(), "preedit-end", G_CALLBACK(handlePreeditEndCallback), this);
}

GtkInputMethodFilter::~GtkInputMethodFilter()
{
    g_signal_handlers_disconnect_by_func(m_context.get(), reinterpret_cast<void*>(handleCommitCallback), this);
    g_signal_handlers_disconnect_by_func(m_context.get(), reinterpret_cast<void*>(handlePreeditStartCallback), this);
    g_signal_handlers_disconnect_by_func(m_context.get(), reinterpret_cast<void*>(handlePreeditChangedCallback), this);
    g_signal_handlers_disconnect_by_func(m_context.get(), reinterpret_cast<void*>(handlePreeditEndCallback), this);
    g_signal_handlers_disconnect_by_func(m_widget, reinterpret_cast<void*>(handleWidgetRealize), this);
}

void GtkInputMethodFilter::setEnabled(bool enabled)
{
    m_enabled = enabled;
    if (enabled)
        gtk_im_context_focus_in(m_context.get());
    else
        gtk_im_context_focus_out(m_context.get());
}

bool GtkInputMethodFilter::filterKeyEvent(GdkEventKey* event)
{
    if (!canEdit() || !m_enabled)
        return sendSimpleKeyEvent(event);

    m_preeditChanged = false;
    m_filteringKeyEvent = true;

    unsigned int lastFilteredKeyPressCodeWithNoResults = m_lastFilteredKeyPressCodeWithNoResults;
    m_lastFilteredKeyPressCodeWithNoResults = GDK_KEY_VoidSymbol;

    bool filtered = gtk_im_context_filter_keypress(m_context.get(), event);
    m_filteringKeyEvent = false;

    bool justSentFakeKeyUp = m_justSentFakeKeyUp;
    m_justSentFakeKeyUp = false;
    if (justSentFakeKeyUp && event->type == GDK_KEY_RELEASE)
        return true;

    // Simple input methods work such that even normal keystrokes fire the
    // commit signal. We detect those situations and treat them as normal
    // key events, supplying the commit string as the key character.
    if (filtered && !m_composingTextCurrently && !m_preeditChanged && m_confirmedComposition.length() == 1) {
        bool result = sendSimpleKeyEvent(event, m_confirmedComposition);
        m_confirmedComposition = String();
        return result;
    }

    if (filtered && event->type == GDK_KEY_PRESS) {
        if (!m_preeditChanged && m_confirmedComposition.isNull()) {
            m_composingTextCurrently = true;
            m_lastFilteredKeyPressCodeWithNoResults = event->keyval;
            return true;
        }

        bool result = sendKeyEventWithCompositionResults(event);
        if (!m_confirmedComposition.isEmpty()) {
            m_composingTextCurrently = false;
            m_confirmedComposition = String();
        }
        return result;
    }

    // If we previously filtered a key press event and it yielded no results. Suppress
    // the corresponding key release event to avoid confusing the web content.
    if (event->type == GDK_KEY_RELEASE && lastFilteredKeyPressCodeWithNoResults == event->keyval)
        return true;

    // At this point a keystroke was either:
    // 1. Unfiltered
    // 2. A filtered keyup event. As the IME code in EditorClient.h doesn't
    //    ever look at keyup events, we send any composition results before
    //    the key event.
    // Both might have composition results or not.
    //
    // It's important to send the composition results before the event
    // because some IM modules operate that way. For example (taken from
    // the Chromium source), the latin-post input method gives this sequence
    // when you press 'a' and then backspace:
    //  1. keydown 'a' (filtered)
    //  2. preedit changed to "a"
    //  3. keyup 'a' (unfiltered)
    //  4. keydown Backspace (unfiltered)
    //  5. commit "a"
    //  6. preedit end
    if (!m_confirmedComposition.isEmpty())
        confirmComposition();
    if (m_preeditChanged)
        updatePreedit();
    return sendSimpleKeyEvent(event);
}

void GtkInputMethodFilter::notifyMouseButtonPress()
{
    // Confirming the composition may trigger a selection change, which
    // might trigger further unwanted actions on the context, so we prevent
    // that by setting m_composingTextCurrently to false.
    if (m_composingTextCurrently)
        confirmCurrentComposition();
    m_composingTextCurrently = false;
    cancelContextComposition();
}

void GtkInputMethodFilter::resetContext()
{

    // We always cancel the current WebCore composition here, in case the
    // composition was set outside the GTK+ IME path (via a script, for
    // instance) and we aren't tracking it.
    cancelCurrentComposition();

    if (!m_composingTextCurrently)
        return;
    m_composingTextCurrently = false;
    cancelContextComposition();
}

void GtkInputMethodFilter::cancelContextComposition()
{
    m_preventNextCommit = !m_preedit.isEmpty();

    gtk_im_context_reset(m_context.get());

    m_composingTextCurrently = false;
    m_justSentFakeKeyUp = false;
    m_preedit = String();
    m_confirmedComposition = String();
}

void GtkInputMethodFilter::notifyFocusedIn()
{
    m_enabled = true;
    gtk_im_context_focus_in(m_context.get());
}

void GtkInputMethodFilter::notifyFocusedOut()
{
    if (!m_enabled)
        return;

    if (m_composingTextCurrently)
        confirmCurrentComposition();
    cancelContextComposition();
    gtk_im_context_focus_out(m_context.get());
    m_enabled = false;
}

void GtkInputMethodFilter::confirmComposition()
{
    confirmCompositionText(m_confirmedComposition);
    m_confirmedComposition = String();
}

void GtkInputMethodFilter::updatePreedit()
{
    setPreedit(m_preedit, m_cursorOffset);
    m_preeditChanged = false;
}

void GtkInputMethodFilter::sendCompositionAndPreeditWithFakeKeyEvents(ResultsToSend resultsToSend)
{
    GOwnPtr<GdkEvent> event(gdk_event_new(GDK_KEY_PRESS));
    event->key.time = GDK_CURRENT_TIME;
    event->key.keyval = gCompositionEventKeyCode;
    sendKeyEventWithCompositionResults(&event->key, resultsToSend, EventFaked);

    m_confirmedComposition = String();
    if (resultsToSend & Composition)
        m_composingTextCurrently = false;

    event->type = GDK_KEY_RELEASE;
    sendSimpleKeyEvent(&event->key, String(), EventFaked);
    m_justSentFakeKeyUp = true;
}

void GtkInputMethodFilter::handleCommit(const char* compositionString)
{
    if (m_preventNextCommit) {
        m_preventNextCommit = false;
        return;
    }

    if (!m_enabled)
        return;

    m_confirmedComposition.append(String::fromUTF8(compositionString));

    // If the commit was triggered outside of a key event, just send
    // the IME event now. If we are handling a key event, we'll decide
    // later how to handle this.
    if (!m_filteringKeyEvent)
        sendCompositionAndPreeditWithFakeKeyEvents(Composition);
}

void GtkInputMethodFilter::handlePreeditStart()
{
    if (m_preventNextCommit || !m_enabled)
        return;
    m_preeditChanged = true;
    m_preedit = "";
}

void GtkInputMethodFilter::handlePreeditChanged()
{
    if (!m_enabled)
        return;

    GOwnPtr<gchar> newPreedit;
    gtk_im_context_get_preedit_string(m_context.get(), &newPreedit.outPtr(), 0, &m_cursorOffset);

    if (m_preventNextCommit) {
        if (strlen(newPreedit.get()) > 0)
            m_preventNextCommit = false;
        else
            return;
    }

    m_preedit = String::fromUTF8(newPreedit.get());
    m_cursorOffset = std::min(std::max(m_cursorOffset, 0), static_cast<int>(m_preedit.length()));

    m_composingTextCurrently = !m_preedit.isEmpty();
    m_preeditChanged = true;

    if (!m_filteringKeyEvent)
        sendCompositionAndPreeditWithFakeKeyEvents(Preedit);
}

void GtkInputMethodFilter::handlePreeditEnd()
{
    if (m_preventNextCommit || !m_enabled)
        return;

    m_preedit = String();
    m_cursorOffset = 0;
    m_preeditChanged = true;

    if (!m_filteringKeyEvent)
        updatePreedit();
}

}
