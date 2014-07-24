/*
 * Copyright (C) 2011 Samsung Electronics
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "InputMethodContextEfl.h"

#include "EwkView.h"
#include "WebPageProxy.h"
#include <Ecore_Evas.h>
#include <Ecore_IMF_Evas.h>
#include <WebCore/Editor.h>

using namespace WebCore;

namespace WebKit {

InputMethodContextEfl::InputMethodContextEfl(EwkView* view, PassOwnPtr<Ecore_IMF_Context> context)
    : m_view(view)
    , m_context(context)
    , m_focused(false)
{
    ASSERT(m_context);
    ecore_imf_context_event_callback_add(m_context.get(), ECORE_IMF_CALLBACK_PREEDIT_CHANGED, onIMFPreeditSequenceChanged, this);
    ecore_imf_context_event_callback_add(m_context.get(), ECORE_IMF_CALLBACK_COMMIT, onIMFInputSequenceComplete, this);
}

InputMethodContextEfl::~InputMethodContextEfl()
{
}

void InputMethodContextEfl::onIMFInputSequenceComplete(void* data, Ecore_IMF_Context*, void* eventInfo)
{
    InputMethodContextEfl* inputMethodContext = static_cast<InputMethodContextEfl*>(data);
    if (!eventInfo || !inputMethodContext->m_focused)
        return;

    inputMethodContext->m_view->page()->confirmComposition(String::fromUTF8(static_cast<char*>(eventInfo)));
}

void InputMethodContextEfl::onIMFPreeditSequenceChanged(void* data, Ecore_IMF_Context* context, void*)
{
    InputMethodContextEfl* inputMethodContext = static_cast<InputMethodContextEfl*>(data);

    if (!inputMethodContext->m_view->page()->focusedFrame() || !inputMethodContext->m_focused)
        return;

    char* buffer = 0;
    ecore_imf_context_preedit_string_get(context, &buffer, 0);
    if (!buffer)
        return;

    String preeditString = String::fromUTF8(buffer);
    free(buffer);
    Vector<CompositionUnderline> underlines;
    underlines.append(CompositionUnderline(0, preeditString.length(), Color(0, 0, 0), false));
    inputMethodContext->m_view->page()->setComposition(preeditString, underlines, 0);
}

PassOwnPtr<Ecore_IMF_Context> InputMethodContextEfl::createIMFContext(Evas* canvas)
{
    const char* defaultContextID = ecore_imf_context_default_id_get();
    if (!defaultContextID)
        return nullptr;

    OwnPtr<Ecore_IMF_Context> imfContext = adoptPtr(ecore_imf_context_add(defaultContextID));
    if (!imfContext)
        return nullptr;

    Ecore_Evas* ecoreEvas = ecore_evas_ecore_evas_get(canvas);
    ecore_imf_context_client_window_set(imfContext.get(), reinterpret_cast<void*>(ecore_evas_window_get(ecoreEvas)));
    ecore_imf_context_client_canvas_set(imfContext.get(), canvas);

    return imfContext.release();
}

void InputMethodContextEfl::handleMouseUpEvent(const Evas_Event_Mouse_Up*)
{
    ecore_imf_context_reset(m_context.get());
}

void InputMethodContextEfl::handleKeyDownEvent(const Evas_Event_Key_Down* downEvent, bool* isFiltered)
{
    Ecore_IMF_Event inputMethodEvent;
    ecore_imf_evas_event_key_down_wrap(const_cast<Evas_Event_Key_Down*>(downEvent), &inputMethodEvent.key_down);

    *isFiltered = ecore_imf_context_filter_event(m_context.get(), ECORE_IMF_EVENT_KEY_DOWN, &inputMethodEvent);
}

void InputMethodContextEfl::updateTextInputState()
{
    if (!m_context)
        return;

    const EditorState& editor = m_view->page()->editorState();

    if (editor.isContentEditable) {
        if (m_focused)
            return;

        ecore_imf_context_reset(m_context.get());
        ecore_imf_context_focus_in(m_context.get());
        m_focused = true;
    } else {
        if (!m_focused)
            return;

        if (editor.hasComposition)
            m_view->page()->cancelComposition();

        m_focused = false;
        ecore_imf_context_reset(m_context.get());
        ecore_imf_context_focus_out(m_context.get());
    }
}

}
