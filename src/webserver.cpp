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

#include "webserver.h"

#include "mongoose.h"

#include <QByteArray>
#include <QHostAddress>
#include <QMetaType>
#include <QThread>
#include <QUrl>

static void *callback(mg_event event,
                      mg_connection *conn,
                      const mg_request_info *request_info)
{
    WebServer* server = static_cast<WebServer*>(request_info->user_data);
    // note: we use a blocking queued connection to always handle the request in the main thread
    // TODO: check whether direct call works as well
    bool handled = false;
    Qt::ConnectionType connectionType = Qt::DirectConnection;
    if (QThread::currentThread() != server->thread()) {
        connectionType = Qt::BlockingQueuedConnection;
    }
    QMetaObject::invokeMethod(server, "handleRequest", connectionType,
                              Q_ARG(mg_event, event), Q_ARG(mg_connection*, conn),
                              Q_ARG(const mg_request_info*, request_info),
                              Q_ARG(bool*, &handled));
    if (handled) {
        // anything non-null... pretty ugly, why not simply a bool??
        return server;
    } else {
        return 0;
    }
}

WebServer::WebServer(QObject *parent, const Config *config)
    : QObject(parent)
    , m_config(m_config)
    , m_ctx(0)
{
    setObjectName("WebServer");
    qRegisterMetaType<mg_event>("mg_event");
    qRegisterMetaType<mg_connection*>("mg_connection*");
    qRegisterMetaType<const mg_request_info*>("const mg_request_info*");
    qRegisterMetaType<bool*>("bool*");
}

WebServer::~WebServer()
{
    close();
}

bool WebServer::listenOnPort(const QString& port)
{
    ///TODO: listen on multiple ports?
    close();

    const char *options[] = {
        "listening_ports", qstrdup(qPrintable(port)),
        "enable_directory_listing", "no",
        NULL};
    ///TODO: more options from m_config?
    m_ctx = mg_start(&callback, this, options);
    if (!m_ctx) {
        return false;
    }

    m_port = port;
    return true;
}

QString WebServer::port() const
{
    return m_port;
}

void WebServer::close()
{
    if (m_ctx) {
        mg_stop(m_ctx);
        m_ctx = 0;
        m_port.clear();
    }
}

void WebServer::handleRequest(mg_event event, mg_connection *conn, const mg_request_info *request,
                              bool *handled)
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (event == MG_NEW_REQUEST) {
        WebServerResponse responseObj(conn);

        // Modelled after http://nodejs.org/docs/latest/api/http.html#http.ServerRequest
        QVariantMap requestObject;

        ///TODO: encoding?!

        if (request->request_method)
            requestObject["method"] = QString::fromLocal8Bit(request->request_method);
        if (request->http_version)
            requestObject["httpVersion"] = QString::fromLocal8Bit(request->http_version);
        if (request->status_code >=0)
            requestObject["statusCode"] = request->status_code;

        QByteArray uri(request->uri);
        if (uri.startsWith('/'))
            uri = '/' + QUrl::toPercentEncoding(QString::fromLatin1(request->uri + 1));
        if (request->query_string)
            uri.append('?').append(QByteArray(request->query_string));
        requestObject["url"] = uri.data();

#if 0
        // Non-standard and thus disable for the time being.
        requestObject["isSSL"] = request->is_ssl;
        requestObject["remoteIP"] = QHostAddress(request->remote_ip).toString();;
        requestObject["remotePort"] = request->remote_port;
        if (request->remote_user)
            requestObject["remoteUser"] = QString::fromLocal8Bit(request->remote_user);
#endif

        QVariantMap headersObject;
        for (int i = 0; i < request->num_headers; ++i) {
            QString key = QString::fromLocal8Bit(request->http_headers[i].name);
            QString value = QString::fromLocal8Bit(request->http_headers[i].value);
            headersObject[key] = value;
        }
        requestObject["headers"] = headersObject;

        emit newRequest(requestObject, &responseObj);
        *handled = true;
        return;
    }
    *handled = false;
}


//BEGIN WebServerResponse

WebServerResponse::WebServerResponse(mg_connection *conn)
    : m_conn(conn)
    , m_statusCode(200)
    , m_headersSent(false)
{

}

const char* responseCodeString(int code)
{
    // see: http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
    switch (code) {
        case 100:
            return "Continue";
        case 101:
            return "Switching Protocols";
        case 200:
            return "OK";
        case 201:
            return "Created";
        case 202:
            return "Accepted";
        case 203:
            return "Non-Authoritative Information";
        case 204:
            return "No Content";
        case 205:
            return "Reset Content";
        case 206:
            return "Partial Content";
        case 300:
            return "Multiple Choices";
        case 301:
            return "Moved Permanently";
        case 302:
            return "Found";
        case 303:
            return "See Other";
        case 304:
            return "Not Modified";
        case 305:
            return "Use Proxy";
        case 307:
            return "Temporary Redirect";
        case 400:
            return "Bad Request";
        case 401:
            return "Unauthorized";
        case 402:
            return "Payment Required";
        case 403:
            return "Forbidden";
        case 404:
            return "Not Found";
        case 405:
            return "Method Not Allowed";
        case 406:
            return "Not Acceptable";
        case 407:
            return "Proxy Authentication Required";
        case 408:
            return "Request Timeout";
        case 409:
            return "Conflict";
        case 410:
            return "Gone";
        case 411:
            return "Length Required";
        case 412:
            return "Precondition Failed";
        case 413:
            return "Request Entity Too Large";
        case 414:
            return "Request-URI Too Long";
        case 415:
            return "Unsupported Media Type";
        case 416:
            return "Requested Range not Satisfiable";
        case 417:
            return "Expectation Failed";
        case 500:
            return "Internal Server Error";
        case 501:
            return "Not Implemented";
        case 502:
            return "Bad Gateway";
        case 503:
            return "Service Unavailable";
        case 504:
            return "Gateway Timeout";
        case 505:
            return "HTTP Version Not Supported";
        case 306:
            // unused: fallthrough
        default:
            return "";
    }
}

void WebServerResponse::writeHead(int statusCode, const QVariantMap &headers)
{
    ///TODO: what is the best-practice error handling in javascript? exceptions?
    Q_ASSERT(!m_headersSent);
    m_headersSent = true;
    m_statusCode = statusCode;
    mg_printf(m_conn, "HTTP/1.1 %d %s\r\n", m_statusCode, responseCodeString(m_statusCode));
    QVariantMap::const_iterator it = headers.constBegin();
    while(it != headers.constEnd()) {
        mg_printf(m_conn, "%s: %s\r\n", qPrintable(it.key()), qPrintable(it.value().toString()));
        ++it;
    }
    mg_write(m_conn, "\r\n", 2);
}

void WebServerResponse::write(const QString &body)
{
    if (!m_headersSent) {
        writeHead(m_statusCode, m_headers);
    }
    ///TODO: encoding?!
    const QByteArray data = body.toLocal8Bit();
    mg_write(m_conn, data.constData(), data.size());
}

int WebServerResponse::statusCode() const
{
    return m_statusCode;
}

void WebServerResponse::setStatusCode(int code)
{
    ///TODO: what is the best-practice error handling in javascript? exceptions?
    Q_ASSERT(!m_headersSent);
    m_statusCode = code;
}

QString WebServerResponse::header(const QString &name) const
{
    return m_headers.value(name).toString();
}

void WebServerResponse::setHeader(const QString &name, const QString &value)
{
    ///TODO: what is the best-practice error handling in javascript? exceptions?
    Q_ASSERT(!m_headersSent);
    m_headers.insert(name, value);
}

QVariantMap WebServerResponse::headers() const
{
    return m_headers;
}

void WebServerResponse::setHeaders(const QVariantMap &headers)
{
    ///TODO: what is the best-practice error handling in javascript? exceptions?
    Q_ASSERT(!m_headersSent);
    m_headers = headers;
}

//END WebServerResponse
