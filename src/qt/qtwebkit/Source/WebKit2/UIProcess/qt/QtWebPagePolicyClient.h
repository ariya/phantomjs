/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef QtWebPagePolicyClient_h
#define QtWebPagePolicyClient_h

#include "qquickwebview_p.h"
#include <QtGlobal>
#include <WKPage.h>

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

namespace WebKit {

class QtWebPagePolicyClient {
public:
    QtWebPagePolicyClient(WKPageRef, QQuickWebView*);

private:
    void decidePolicyForNavigationAction(const QUrl&, Qt::MouseButton, Qt::KeyboardModifiers, QQuickWebView::NavigationType, WKFramePolicyListenerRef);

    // WKPagePolicyClient callbacks.
    static void decidePolicyForNavigationAction(WKPageRef, WKFrameRef, WKFrameNavigationType, WKEventModifiers, WKEventMouseButton, WKURLRequestRef, WKFramePolicyListenerRef, WKTypeRef userData, const void* clientInfo);
    static void decidePolicyForResponse(WKPageRef, WKFrameRef, WKURLResponseRef, WKURLRequestRef, WKFramePolicyListenerRef, WKTypeRef userData, const void* clientInfo);

    QQuickWebView* m_webView;
};

} // namespace WebKit

#endif // QtWebPagePolicyClient_h
