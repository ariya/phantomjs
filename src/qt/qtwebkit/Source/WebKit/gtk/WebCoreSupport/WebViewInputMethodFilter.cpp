/*
 * Copyright (C) 2012 Igalia S.L.
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
#include "WebViewInputMethodFilter.h"

#include "Editor.h"
#include "EventHandler.h"
#include "FocusController.h"
#include "Frame.h"
#include "PlatformKeyboardEvent.h"
#include "webkitwebviewprivate.h"
#include <wtf/text/CString.h>

using namespace WebCore;

namespace WebKit {

Frame* WebViewInputMethodFilter::focusedOrMainFrame()
{
    ASSERT(m_webView);
    Page* page = core(m_webView);
    if (!page)
        return 0;

    return page->focusController()->focusedOrMainFrame();
}

void WebViewInputMethodFilter::setWebView(WebKitWebView* webView)
{
    m_webView = webView;
    GtkInputMethodFilter::setWidget(GTK_WIDGET(webView));
}

bool WebViewInputMethodFilter::canEdit()
{
    Frame* frame = focusedOrMainFrame();
    return frame && frame->editor().canEdit();
}

bool WebViewInputMethodFilter::sendSimpleKeyEvent(GdkEventKey* event, WTF::String simpleString, EventFakedForComposition)
{
    PlatformKeyboardEvent platformEvent(event, CompositionResults(simpleString));
    return focusedOrMainFrame()->eventHandler()->keyEvent(platformEvent);
}

bool WebViewInputMethodFilter::sendKeyEventWithCompositionResults(GdkEventKey* event, ResultsToSend resultsToSend, EventFakedForComposition)
{
    PlatformKeyboardEvent platformEvent(event, CompositionResults(CompositionResults::WillSendCompositionResultsSoon));
    if (!focusedOrMainFrame()->eventHandler()->keyEvent(platformEvent))
        return false;

    if (resultsToSend & Composition && !m_confirmedComposition.isNull())
        confirmCompositionText(m_confirmedComposition);
    if (resultsToSend & Preedit && !m_preedit.isNull())
        setPreedit(m_preedit, m_cursorOffset);
    return true;
}

void WebViewInputMethodFilter::confirmCompositionText(String text)
{
    Frame* frame = focusedOrMainFrame();
    if (!frame || !frame->editor().canEdit())
        return;

    if (text.isNull()) {
        confirmCurrentComposition();
        return;
    }
    frame->editor().confirmComposition(m_confirmedComposition);
}

void WebViewInputMethodFilter::confirmCurrentComposition()
{
    Frame* frame = focusedOrMainFrame();
    if (!frame || !frame->editor().canEdit())
        return;
    frame->editor().confirmComposition();
}

void WebViewInputMethodFilter::cancelCurrentComposition()
{
    Frame* frame = focusedOrMainFrame();
    if (!frame || !frame->editor().canEdit())
        return;
    frame->editor().cancelComposition();
}

void WebViewInputMethodFilter::setPreedit(String newPreedit, int cursorOffset)
{
    Frame* frame = focusedOrMainFrame();
    if (!frame || !frame->editor().canEdit())
        return;

    // TODO: We should parse the PangoAttrList that we get from the IM context here.
    Vector<CompositionUnderline> underlines;
    underlines.append(CompositionUnderline(0, newPreedit.length(), Color(1, 1, 1), false));
    frame->editor().setComposition(newPreedit, underlines, m_cursorOffset, m_cursorOffset);
}

} // namespace WebKit
