/*
 * Copyright (C) 2011 Zeno Albisser <zeno@webkit.org>
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
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

#include "config.h"
#include "QtNetworkAccessManager.h"

#include "SharedMemory.h"
#include "WebFrameNetworkingContext.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include "WebProcess.h"
#include <QAuthenticator>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace WebKit {

QtNetworkAccessManager::QtNetworkAccessManager(WebProcess* webProcess)
    : QNetworkAccessManager()
    , m_webProcess(webProcess)
{
    connect(this, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)), SLOT(onAuthenticationRequired(QNetworkReply*, QAuthenticator*)));
    connect(this, SIGNAL(proxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)), SLOT(onProxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator*)));
#ifndef QT_NO_SSL
    connect(this, SIGNAL(sslErrors(QNetworkReply*, QList<QSslError>)), SLOT(onSslErrors(QNetworkReply*, QList<QSslError>)));
#endif
}

WebPage* QtNetworkAccessManager::obtainOriginatingWebPage(const QNetworkRequest& request)
{
    QObject* originatingObject = request.originatingObject();
    if (!originatingObject)
        return 0;

    qulonglong pageID = originatingObject->property("pageID").toULongLong();
    return m_webProcess->webPage(pageID);
}

QNetworkReply* QtNetworkAccessManager::createRequest(Operation operation, const QNetworkRequest& request, QIODevice* outData)
{
    WebPage* webPage = obtainOriginatingWebPage(request);
    if (webPage && m_applicationSchemes.contains(webPage, request.url().scheme().toLower())) {
        QtNetworkReply* reply = new QtNetworkReply(request, this);
        webPage->receivedApplicationSchemeRequest(request, reply);
        return reply;
    }

    return QNetworkAccessManager::createRequest(operation, request, outData);
}

void QtNetworkAccessManager::registerApplicationScheme(const WebPage* page, const QString& scheme)
{
    m_applicationSchemes.insert(page, scheme.toLower());
}

void QtNetworkAccessManager::onProxyAuthenticationRequired(const QNetworkProxy& proxy, QAuthenticator* authenticator)
{
    // FIXME: Check if there is a better way to get a reference to the page.
    WebPage* webPage = m_webProcess->focusedWebPage();

    if (!webPage)
        return;

    String hostname = proxy.hostName();
    uint16_t port = static_cast<uint16_t>(proxy.port());
    String prefilledUsername = authenticator->user();
    String username;
    String password;

    if (webPage->sendSync(
         Messages::WebPageProxy::ProxyAuthenticationRequiredRequest(hostname, port, prefilledUsername),
         Messages::WebPageProxy::ProxyAuthenticationRequiredRequest::Reply(username, password))) {
         if (!username.isEmpty())
             authenticator->setUser(username);
         if (!password.isEmpty())
             authenticator->setPassword(password);
     }

}

void QtNetworkAccessManager::onAuthenticationRequired(QNetworkReply* reply, QAuthenticator* authenticator)
{
    WebPage* webPage = obtainOriginatingWebPage(reply->request());

    // FIXME: This check can go away once our Qt version is up-to-date. See: QTBUG-23512.
    if (!webPage)
        return;

    String hostname = reply->url().toString(QUrl::RemovePath | QUrl::RemoveQuery | QUrl::RemoveFragment | QUrl::StripTrailingSlash);
    String realm = authenticator->realm();
    String prefilledUsername = authenticator->user();
    String username;
    String password;

    if (webPage->sendSync(
        Messages::WebPageProxy::AuthenticationRequiredRequest(hostname, realm, prefilledUsername),
        Messages::WebPageProxy::AuthenticationRequiredRequest::Reply(username, password))) {
        if (!username.isEmpty())
            authenticator->setUser(username);
        if (!password.isEmpty())
            authenticator->setPassword(password);
    }
}

void QtNetworkAccessManager::onSslErrors(QNetworkReply* reply, const QList<QSslError>& qSslErrors)
{
#ifndef QT_NO_SSL
    WebPage* webPage = obtainOriginatingWebPage(reply->request());

    // FIXME: This check can go away once our Qt version is up-to-date. See: QTBUG-23512.
    if (!webPage)
        return;

    String hostname = reply->url().host();
    bool ignoreErrors = false;

    if (webPage->sendSync(
        Messages::WebPageProxy::CertificateVerificationRequest(hostname),
        Messages::WebPageProxy::CertificateVerificationRequest::Reply(ignoreErrors))) {
        if (ignoreErrors)
            reply->ignoreSslErrors(qSslErrors);
    }
#endif
}

}

#include "moc_QtNetworkAccessManager.cpp"
