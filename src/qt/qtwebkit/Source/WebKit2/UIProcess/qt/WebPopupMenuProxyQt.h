/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebPopupMenuProxyQt_h
#define WebPopupMenuProxyQt_h

#include "WebPopupMenuProxy.h"
#include <QtCore/QObject>
#include <wtf/OwnPtr.h>

QT_BEGIN_NAMESPACE
class QQmlComponent;
class QQmlContext;
class QQuickItem;
QT_END_NAMESPACE

class QQuickWebView;

namespace WebKit {

class WebPopupMenuProxyQt : public QObject, public WebPopupMenuProxy {
    Q_OBJECT

public:
    enum SelectionType {
        SingleSelection,
        MultipleSelection
    };

    static PassRefPtr<WebPopupMenuProxyQt> create(WebPopupMenuProxy::Client* client, QQuickWebView* webView)
    {
        return adoptRef(new WebPopupMenuProxyQt(client, webView));
    }
    ~WebPopupMenuProxyQt();

    virtual void showPopupMenu(const WebCore::IntRect&, WebCore::TextDirection, double pageScaleFactor, const Vector<WebPopupItem>&, const PlatformPopupMenuData&, int32_t selectedIndex);

public Q_SLOTS:
    virtual void hidePopupMenu();

private Q_SLOTS:
    void selectIndex(int);

private:
    WebPopupMenuProxyQt(WebPopupMenuProxy::Client*, QQuickWebView*);
    void createItem(QObject*);
    void createContext(QQmlComponent*, QObject*);

    OwnPtr<QQmlContext> m_context;
    OwnPtr<QQuickItem> m_itemSelector;

    QQuickWebView* m_webView;
    SelectionType m_selectionType;
};

} // namespace WebKit

#endif // WebPopupMenuProxyQt_h
