/*
 * Copyright (C) 2010 Juha Savolainen (juha.savolainen@weego.fi)
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef qwebnavigationhistory_p_p_h
#define qwebnavigationhistory_p_p_h

#include "qwebkitglobal.h"

#include <QObject>
#include <WebKit2/WKBase.h>
#include <WebKit2/WKRetainPtr.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>

namespace WebKit {
class WebBackForwardList;
}

class QWebNavigationHistory;
class QWebNavigationListModel;

class QWebNavigationListModelPrivate {
public:
    QWebNavigationListModelPrivate(WKBackForwardListRef);

    static QWebNavigationListModel* createWebNavigationModel(WKBackForwardListRef);

    unsigned (*count)(WKBackForwardListRef);
    WKRetainPtr<WKBackForwardListRef> m_backForwardList;
    int indexSign;
};

class QWebNavigationHistoryPrivate {
public:
    static QWebNavigationHistory* createHistory(WKPageRef);

    QWebNavigationHistoryPrivate(WKPageRef);
    void reset();
    void goBackTo(int index);
    void goForwardTo(int index);

    WKRetainPtr<WKPageRef> m_page;
    WKRetainPtr<WKBackForwardListRef> m_backForwardList;
    OwnPtr<QWebNavigationListModel> m_backNavigationModel;
    OwnPtr<QWebNavigationListModel> m_forwardNavigationModel;
};

#endif /* qwebnavigationhistory_p_p_h */
