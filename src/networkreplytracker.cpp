/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2015 Dmitry Parshin, Smartling Inc. <dparshin@smartling.com>, <parshin.da@gmail.com>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "networkreplytracker.h"
#include "networkreplyproxy.h"


NetworkReplyTracker::NetworkReplyTracker(QObject* parent): QObject(parent)
{
	// TODO body. Once done, remove TODO marker and test.
}


QNetworkReply* NetworkReplyTracker::trackReply(QNetworkReply* reply, int requestId,
        bool shouldCaptureResponseBody)
{
    NetworkReplyProxy* proxy = new NetworkReplyProxy(this, reply, shouldCaptureResponseBody);
    m_replies[reply] = proxy;
    m_ids[proxy] = requestId;
    connect(proxy, SIGNAL(readyRead()), this, SLOT(handleIncomingData()));
    connect(proxy, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(handleNetworkError(QNetworkReply::NetworkError)));
    connect(proxy, SIGNAL(sslErrors(const QList<QSslError>&)), this, SLOT(handleSslErrors(const QList<QSslError>&)));
    connect(proxy, SIGNAL(finished()), SLOT(handleReplyFinished()));
    return proxy;
}


void NetworkReplyTracker::abort(QNetworkReply* reply, int status, const QString& statusString)
{
    finishReply(reply, status, statusString);
    reply->close();
}


void NetworkReplyTracker::finishReply(QNetworkReply* reply, int status, const QString& statusText)
{
    NetworkReplyProxy* proxy = getProxyFor(reply);

    if (!m_ids.contains(proxy)) {
        return;
    }

    m_replies.remove(proxy->nestedReply());


    int requestId = m_ids.value(proxy);
    m_ids.remove(proxy);

    m_started.remove(proxy);


    emit finished(proxy, requestId, status, statusText, proxy->body(), proxy->bodySize());
}

void NetworkReplyTracker::handleIncomingData()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

    if (!reply) {
        return;
    }

    NetworkReplyProxy* proxy = getProxyFor(reply);

    if (m_started.contains(proxy)) {
        return;
    }

    m_started.insert(proxy);


    emit started(proxy, m_ids.value(proxy));
}


void NetworkReplyTracker::handleReplyFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());

    QVariant status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    QVariant statusText = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute);

    finishReply(reply, status.toInt(), statusText.toString());
}


void NetworkReplyTracker::handleSslErrors(const QList<QSslError>& errors)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    NetworkReplyProxy* proxy = getProxyFor(reply);
    emit sslErrors(proxy, errors);
}


void NetworkReplyTracker::handleNetworkError(QNetworkReply::NetworkError err)
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    NetworkReplyProxy* proxy = getProxyFor(reply);
    emit error(proxy, m_ids.value(proxy), err);
}


NetworkReplyProxy* NetworkReplyTracker::getProxyFor(QNetworkReply* reply)
{
    NetworkReplyProxy* proxy = NULL;
    // first trying to find proxy assuming that we got nested reply
    if (m_replies.contains(reply)) {
        proxy = m_replies.value(reply);
    } else {
        //if not then it is proxy
        proxy = qobject_cast<NetworkReplyProxy*>(reply);
    }

    return proxy;
}
