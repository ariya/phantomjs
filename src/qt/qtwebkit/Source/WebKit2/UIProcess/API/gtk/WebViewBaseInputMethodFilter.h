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

#ifndef WebViewBaseInputMethodFilter_h
#define WebViewBaseInputMethodFilter_h

#include "GtkInputMethodFilter.h"
#include "WebPageProxy.h"

typedef struct _WebKitWebViewBase WebKitWebViewBase;

namespace WebKit {

class WebViewBaseInputMethodFilter : public WebCore::GtkInputMethodFilter {
public:
    void setWebView(WebKitWebViewBase*);

protected:
    virtual bool sendSimpleKeyEvent(GdkEventKey*, WTF::String eventString, EventFakedForComposition);
    virtual bool sendKeyEventWithCompositionResults(GdkEventKey*, ResultsToSend, EventFakedForComposition);
    virtual bool canEdit();
    virtual void confirmCompositionText(String);
    virtual void confirmCurrentComposition();
    virtual void cancelCurrentComposition();
    virtual void setPreedit(String, int cursorOffset);

private:
    WebPageProxy* m_webPageProxy;
};

} // namespace WebKit

#endif // WebViewBaseInputMethodFilter_h
