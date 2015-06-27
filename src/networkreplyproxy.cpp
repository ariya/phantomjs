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

#include <QNetworkRequest>
#include <QNetworkReply>

#include "networkreplyproxy.h"

NetworkReplyProxy::NetworkReplyProxy(QObject* parent, QNetworkReply* reply,
                                     bool shouldCaptureResponse)
    : QNetworkReply(parent)
    , m_reply(reply)
    , m_shouldCaptureResponseBody(shouldCaptureResponse)
{
    // apply attributes...
    setOperation(m_reply->operation());
    setRequest(m_reply->request());
    setUrl(m_reply->url());

    // handle these to forward
    connect(m_reply, SIGNAL(metaDataChanged()), SLOT(applyMetaData()));
    connect(m_reply, SIGNAL(readyRead()), SLOT(readInternal()));
    connect(m_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(errorInternal(QNetworkReply::NetworkError)));

    // forward signals
    connect(m_reply, SIGNAL(finished()), SIGNAL(finished()));
    connect(m_reply, SIGNAL(uploadProgress(qint64, qint64)), SIGNAL(uploadProgress(qint64, qint64)));
    connect(m_reply, SIGNAL(downloadProgress(qint64, qint64)), SIGNAL(downloadProgress(qint64, qint64)));

    // for the data proxy...
    setOpenMode(ReadOnly);
}

QString NetworkReplyProxy::body()
{
    //converting data to QString in a special way(without charset encoding),
    //similar to File::read method in 'filesystem' module.
    QString ret(m_data.size(), ' ');
    for (int i = 0; i < m_data.size(); ++i) {
        ret[i] = m_data.at(i);
    }

    return ret;
}

void NetworkReplyProxy::abort()
{
    m_reply->abort();
}

void NetworkReplyProxy::close()
{
    m_reply->close();
}

bool NetworkReplyProxy::isSequential() const
{
    return m_reply->isSequential();
}

void NetworkReplyProxy::setReadBufferSize(qint64 size)
{
    QNetworkReply::setReadBufferSize(size); m_reply->setReadBufferSize(size);
}

qint64 NetworkReplyProxy::bytesAvailable() const
{
    return m_buffer.size() + QIODevice::bytesAvailable();
}

qint64 NetworkReplyProxy::bytesToWrite() const
{
    return -1;
}

qint64 NetworkReplyProxy::readData(char* data, qint64 maxlen)
{
    qint64 size = qMin(maxlen, qint64(m_buffer.size()));
    memcpy(data, m_buffer.constData(), size);
    m_buffer.remove(0, size);
    return size;
}

void NetworkReplyProxy::ignoreSslErrors()
{
    m_reply->ignoreSslErrors();
}

void NetworkReplyProxy::applyMetaData()
{
    /*
      We have to store headers and attributes, otherwise
      QNetworkReply non-virtual methods (attribute, header, etc.)
      would not have data to return.
    */

    QList<QByteArray> headers = m_reply->rawHeaderList();
    foreach(QByteArray header, headers)
    setRawHeader(header, m_reply->rawHeader(header));

    setHeader(QNetworkRequest::ContentTypeHeader, m_reply->header(QNetworkRequest::ContentTypeHeader));
    setHeader(QNetworkRequest::ContentLengthHeader, m_reply->header(QNetworkRequest::ContentLengthHeader));
    setHeader(QNetworkRequest::LocationHeader, m_reply->header(QNetworkRequest::LocationHeader));
    setHeader(QNetworkRequest::LastModifiedHeader, m_reply->header(QNetworkRequest::LastModifiedHeader));
    setHeader(QNetworkRequest::SetCookieHeader, m_reply->header(QNetworkRequest::SetCookieHeader));

    setAttribute(QNetworkRequest::HttpStatusCodeAttribute, m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute));
    setAttribute(QNetworkRequest::HttpReasonPhraseAttribute, m_reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute));
    setAttribute(QNetworkRequest::RedirectionTargetAttribute, m_reply->attribute(QNetworkRequest::RedirectionTargetAttribute));
    setAttribute(QNetworkRequest::ConnectionEncryptedAttribute, m_reply->attribute(QNetworkRequest::ConnectionEncryptedAttribute));
    setAttribute(QNetworkRequest::CacheLoadControlAttribute, m_reply->attribute(QNetworkRequest::CacheLoadControlAttribute));
    setAttribute(QNetworkRequest::CacheSaveControlAttribute, m_reply->attribute(QNetworkRequest::CacheSaveControlAttribute));
    setAttribute(QNetworkRequest::SourceIsFromCacheAttribute, m_reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute));
    setAttribute(QNetworkRequest::DoNotBufferUploadDataAttribute, m_reply->attribute(QNetworkRequest::DoNotBufferUploadDataAttribute));
    emit metaDataChanged();
}


void NetworkReplyProxy::errorInternal(QNetworkReply::NetworkError _error)
{
    /*
      Saving data for non-virtual QIODevice::errorString method
     */
    setError(_error, m_reply->errorString());
    emit error(_error);
}


void NetworkReplyProxy::readInternal()
{
    QByteArray data = m_reply->readAll();

    if (m_shouldCaptureResponseBody) {
        //this is a response buffer, whole response is stored here
        m_data += data;
    }

    //this is a temporary buffer, data is wiped after a call to 'readData'
    m_buffer += data;
    emit readyRead();
}
