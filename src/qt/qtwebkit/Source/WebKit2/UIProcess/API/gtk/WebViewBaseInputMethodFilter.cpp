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
#include "WebViewBaseInputMethodFilter.h"

#include "NativeWebKeyboardEvent.h"
#include "WebKitWebViewBasePrivate.h"
#include "WebPageProxy.h"
#include <WebCore/Color.h>
#include <WebCore/CompositionResults.h>
#include <WebCore/Editor.h>

using namespace WebCore;

namespace WebKit {

void WebViewBaseInputMethodFilter::setWebView(WebKitWebViewBase* webView)
{
    GtkInputMethodFilter::setWidget(GTK_WIDGET(webView));

    m_webPageProxy = webkitWebViewBaseGetPage(webView);
    ASSERT(m_webPageProxy);
}

bool WebViewBaseInputMethodFilter::canEdit()
{
    return true;
}

bool WebViewBaseInputMethodFilter::sendSimpleKeyEvent(GdkEventKey* event, WTF::String simpleString, EventFakedForComposition faked)
{
    ASSERT(m_webPageProxy);
    m_webPageProxy->handleKeyboardEvent(NativeWebKeyboardEvent(reinterpret_cast<GdkEvent*>(event),
                                       CompositionResults(simpleString), faked));
    return true;
}

bool WebViewBaseInputMethodFilter::sendKeyEventWithCompositionResults(GdkEventKey* event, ResultsToSend resultsToSend, EventFakedForComposition faked)
{
    ASSERT(m_webPageProxy);
    m_webPageProxy->handleKeyboardEvent(NativeWebKeyboardEvent(reinterpret_cast<GdkEvent*>(event),
                                        CompositionResults(CompositionResults::WillSendCompositionResultsSoon),
                                        faked));

    if (resultsToSend & Composition && !m_confirmedComposition.isNull())
        confirmCompositionText(m_confirmedComposition);
    if (resultsToSend & Preedit && !m_preedit.isNull())
        setPreedit(m_preedit, m_cursorOffset);
    return true;
}

void WebViewBaseInputMethodFilter::confirmCompositionText(String text)
{
    ASSERT(m_webPageProxy);
    m_webPageProxy->confirmComposition(text, -1, 0);
}

void WebViewBaseInputMethodFilter::confirmCurrentComposition()
{
    ASSERT(m_webPageProxy);
    m_webPageProxy->confirmComposition(String(), -1, 0);
}

void WebViewBaseInputMethodFilter::cancelCurrentComposition()
{
    ASSERT(m_webPageProxy);
    m_webPageProxy->cancelComposition();
}

void WebViewBaseInputMethodFilter::setPreedit(String newPreedit, int cursorOffset)
{
    // TODO: We should parse the PangoAttrList that we get from the IM context here.
    Vector<CompositionUnderline> underlines;
    underlines.append(CompositionUnderline(0, newPreedit.length(), Color(1, 1, 1), false));

    ASSERT(m_webPageProxy);
    m_webPageProxy->setComposition(newPreedit, underlines,
                                   m_cursorOffset, m_cursorOffset,
                                   0 /* replacement start */,
                                   0 /* replacement end */);
}

} // namespace WebKit
