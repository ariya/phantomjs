/*
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef WebFrameNetworkingContext_h
#define WebFrameNetworkingContext_h

#include "WebProcess.h"
#include <WebCore/FrameNetworkingContext.h>
#include <wtf/OwnPtr.h>

namespace WebKit {

class WebFrame;

class WebFrameNetworkingContext : public WebCore::FrameNetworkingContext {
public:
    static PassRefPtr<WebFrameNetworkingContext> create(WebFrame*);
    QObject* originatingObject() const { return m_originatingObject.get(); }

private:
    WebFrameNetworkingContext(WebFrame*);
    ~WebFrameNetworkingContext() { }

    QNetworkAccessManager* networkAccessManager() const { return WebProcess::shared().networkAccessManager(); }
    bool mimeSniffingEnabled() const { return m_mimeSniffingEnabled; }
    bool thirdPartyCookiePolicyPermission(const QUrl&) const { /*TODO. Used QWebSettings in WK1.*/ return true; }

    OwnPtr<QObject> m_originatingObject;
    bool m_mimeSniffingEnabled;
};

}

#endif // WebFrameNetworkingContext_h
