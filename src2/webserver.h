/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Milian Wolff <milian.wolff@kdab.com>

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

#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <QVariantMap>
#include <QMutex>
#include <QSemaphore>

#include "mongoose.h"

class Config;

class WebServerResponse;

/**
 * Scriptable HTTP web server.
 *
 * see also: modules/webserver.js
 */
class WebServer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString port READ port)

public:
    WebServer(QObject *parent);
    virtual ~WebServer();

public slots:
    /**
     * Start listening for incoming connections on port @p port.
     *
     * For each new request @c handleRequest() will be called which
     * in turn emits @c newRequest() where appropriate.
     *
     * @return true if we can listen on @p port, false otherwise.
     *
     * WARNING: must not be the same name as in the javascript api...
     */
    bool listenOnPort(const QString &port, const QVariantMap& options);
    /**
     * @return the port this server is listening on
     *         or an empty string if the server is closed.
     */
    QString port() const;

    /// Stop listening for incoming connections.
    void close();

signals:
    /// @p request is a WebServerRequest, @p response is a WebServerResponse
    void newRequest(QVariant request, QObject *response);

public:
    bool handleRequest(mg_event event, mg_connection *conn, const mg_request_info *request);

private:
    mg_context *m_ctx;
    QString m_port;
    QMutex m_mutex;
    QList<WebServerResponse*> m_pendingResponses;
    QAtomicInt m_closing;
};


/**
 * Outgoing HTTP response to client.
 */
class WebServerResponse : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int statusCode READ statusCode WRITE setStatusCode)
    Q_PROPERTY(QVariantMap headers READ headers WRITE setHeaders)
public:
    WebServerResponse(mg_connection *conn, QSemaphore* close);

public slots:
    /// send @p headers to client with status code @p statusCode
    void writeHead(int statusCode, const QVariantMap &headers);
    /// sends @p data to client and makes sure the headers are send beforehand
    void write(const QVariant &data);
    // sets @p as encoding used to output data
    void setEncoding(const QString &encoding);

    /**
     * Closes the request once all data has been written to the client.
     *
     * NOTE: This MUST be called, otherwise the server will
     *       not allow new connections anymore.
     *
     * NOTE: After calling close(), this request object
     *       is no longer valid. Any further calls are
     *       undefined and may crash.
     */
    void close();

    /// Same as 'close()', but ensures response headers have been sent first
    void closeGracefully();

    /// get the currently set status code, 200 is the default
    int statusCode() const;
    /// set the status code to @p code
    void setStatusCode(int code);

    /// get the value of header @p name
    QString header(const QString &name) const;
    /// set the value of header @p name to @p value
    void setHeader(const QString &name, const QString &value);

    /// get all headers
    QVariantMap headers() const;
    /// set all headers
    void setHeaders(const QVariantMap &headers);

private:
    mg_connection *m_conn;
    int m_statusCode;
    QVariantMap m_headers;
    bool m_headersSent;
    QString m_encoding;
    QSemaphore* m_close;
};

#endif // WEBSERVER_H
