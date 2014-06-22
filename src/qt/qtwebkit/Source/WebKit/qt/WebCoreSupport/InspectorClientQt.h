/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef InspectorClientQt_h
#define InspectorClientQt_h

#include "InspectorClient.h"
#include "InspectorFrontendChannel.h"
#include "InspectorFrontendClientLocal.h"

#include <QObject>
#include <QString>
#include <wtf/Forward.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

class QWebPageAdapter;
class QWebPage;
class QWebView;

namespace WebCore {
class InspectorFrontendClientQt;
class InspectorServerRequestHandlerQt;
class Page;

class InspectorClientQt : public InspectorClient, public InspectorFrontendChannel {
public:
    explicit InspectorClientQt(QWebPageAdapter*);

    virtual void inspectorDestroyed();

    virtual WebCore::InspectorFrontendChannel* openInspectorFrontend(WebCore::InspectorController*);
    virtual void closeInspectorFrontend();
    virtual void bringFrontendToFront();

    virtual void highlight();
    virtual void hideHighlight();

    virtual bool sendMessageToFrontend(const String&);

    void releaseFrontendPage();

    void attachAndReplaceRemoteFrontend(InspectorServerRequestHandlerQt *channel);
    void detachRemoteFrontend();

private:
    QWebPageAdapter* m_inspectedWebPage;
    QWebPageAdapter* m_frontendWebPage;
    InspectorFrontendClientQt* m_frontendClient;
    bool m_remoteInspector;
    InspectorServerRequestHandlerQt* m_remoteFrontEndChannel;

    friend class InspectorServerRequestHandlerQt;
};

class InspectorFrontendClientQt : public InspectorFrontendClientLocal {
public:
    InspectorFrontendClientQt(QWebPageAdapter* inspectedWebPage, PassOwnPtr<QObject> inspectorView, WebCore::Page* inspectorPage, InspectorClientQt*);
    virtual ~InspectorFrontendClientQt();

    virtual void frontendLoaded();

    virtual String localizedStringsURL();

    virtual void bringToFront();
    virtual void closeWindow();

    virtual void attachWindow(DockSide);
    virtual void detachWindow();

    virtual void setAttachedWindowHeight(unsigned);
    virtual void setAttachedWindowWidth(unsigned);
    virtual void setToolbarHeight(unsigned) OVERRIDE;

    virtual void inspectedURLChanged(const String& newURL);

    void inspectorClientDestroyed();

private:
    void updateWindowTitle();
    void destroyInspectorView(bool notifyInspectorController);
    QWebPageAdapter* m_inspectedWebPage;
    OwnPtr<QObject> m_inspectorView;
    QString m_inspectedURL;
    bool m_destroyingInspectorView;
    InspectorClientQt* m_inspectorClient;
};
}

#endif
