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

#ifndef NETWORKREPLYTRACKER_H
#define NETWORKREPLYTRACKER_H
#include <QtCore/qobject.h>
#include <QHash>
#include <QSet>
#include <QNetworkReply>

class QSslError;
class NetworkReplyProxy;
class NetworkReplyTracker: public QObject
{
    Q_OBJECT

public:  NetworkReplyTracker(QObject* parent = 0);
    /*
      reply - reply to track
      requestId - unique request id, used to distinguis replies internally
      shouldCaptureResponseBody - if true, response body will be available in 'finished' signal
     */
    QNetworkReply* trackReply(QNetworkReply* reply, int requestId,
                              bool shouldCaptureResponseBody);

    /*
      Abort request
      status - set custom HTTP status code
      statusString - text description of status code
     */
    void abort(QNetworkReply* reply, int status, const QString& statusString);

signals:
    void started(QNetworkReply* reply, int requestId);
    void finished(QNetworkReply* reply, int requestId, int status, const QString& statusText, const QString& body, int bodySize);
    void sslErrors(QNetworkReply*, const QList<QSslError>&);
    void error(QNetworkReply*, int requestId, QNetworkReply::NetworkError);

private slots:
    void handleIncomingData();
    void handleReplyFinished();
    void handleSslErrors(const QList<QSslError>& errors);
    void handleNetworkError(QNetworkReply::NetworkError);


private:
    Q_DISABLE_COPY(NetworkReplyTracker)
    void finishReply(QNetworkReply* reply, int status, const QString& statusText);
    NetworkReplyProxy* getProxyFor(QNetworkReply* reply);
    QSet<QNetworkReply*> m_started;
    QHash<QNetworkReply*, int> m_ids;
    QHash<QNetworkReply*, NetworkReplyProxy*> m_replies;
};

#endif // NETWORKREPLYPROXY_H
