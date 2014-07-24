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

#ifndef QtNetworkAccessManager_h
#define QtNetworkAccessManager_h

#include <QMultiHash>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QString>

namespace WebKit {

class WebPage;
class WebProcess;

class QtNetworkAccessManager : public QNetworkAccessManager {
    Q_OBJECT
public:
    QtNetworkAccessManager(WebProcess*);
    void registerApplicationScheme(const WebPage*, const QString& scheme);

protected:
    virtual QNetworkReply* createRequest(Operation, const QNetworkRequest&, QIODevice* outgoingData = 0) OVERRIDE;

private Q_SLOTS:
    void onAuthenticationRequired(QNetworkReply *, QAuthenticator *);
    void onProxyAuthenticationRequired(const QNetworkProxy&, QAuthenticator *);
    void onSslErrors(QNetworkReply*, const QList<QSslError>&);

private:
    WebPage* obtainOriginatingWebPage(const QNetworkRequest&);

    QMultiHash<const WebPage*, QString> m_applicationSchemes;
    WebProcess* m_webProcess;

};

} // namespace WebKit

#endif // QtNetworkAccessManager_h
