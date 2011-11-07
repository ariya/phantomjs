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

#include <QDebug>

#include "mongoose.h"
#include <QThread>
#include <QHostAddress>

static void *callback(mg_event event,
                      mg_connection *conn,
                      const mg_request_info *request_info)
{
    WebServer* server = static_cast<WebServer*>(request_info->user_data);
    // note: we use a blocking queued connection to always handle the request in the main thread
    // TODO: check whether direct call works as well
    bool handled = false;
    QMetaObject::invokeMethod(server, "handleRequest", Qt::BlockingQueuedConnection,
                              Q_ARG(mg_event, event), Q_ARG(mg_connection*, conn),
                              Q_ARG(const mg_request_info*, request_info),
                              Q_ARG(bool*, &handled));
    qWarning() << "handled:" << handled;
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
}

WebServer::~WebServer()
{
    if (m_ctx) {
        mg_stop(m_ctx);
    }
}

void WebServer::listenOnPort(const QString& port)
{
    if (m_ctx) {
        ///TODO: listen on multiple ports?
        mg_stop(m_ctx);
        m_ctx = 0;
    }
    const char *options[] = {"listening_ports", qstrdup(qPrintable(port)), NULL};
    ///TODO: more options from m_config?
    m_ctx = mg_start(&callback, this, options);
    if (!m_ctx) {
        qWarning() << "could not create web server connection on port" << port;
    }
    qWarning() << "listening on port" << port << this;
}

void WebServer::handleRequest(mg_event event, mg_connection *conn, const mg_request_info *request,
                              bool *handled)
{
    Q_ASSERT(QThread::currentThread() == thread());
    qWarning() << "GOT CALLBACK" << event << request;
    if (event == MG_NEW_REQUEST) {
        WebServerRequest requestObj(request);
        WebServerResponse responseObj(conn);
        emit newRequest(&requestObj, &responseObj);
        *handled = true;
        return;
    }
    *handled = false;
}

//BEGIN WebServerRequest
WebServerRequest::WebServerRequest(const mg_request_info *request)
    : m_request(request)
{
}

QString WebServerRequest::method() const
{
    ///TODO: encoding?!
    return QString::fromLocal8Bit(m_request->request_method);
}

QString WebServerRequest::httpVersion() const
{
    ///TODO: encoding?!
    return QString::fromLocal8Bit(m_request->http_version);
}

int WebServerRequest::statusCode() const
{
    return m_request->status_code;
}

bool WebServerRequest::isSSL() const
{
    return m_request->is_ssl;
}

QString WebServerRequest::url() const
{
    ///TODO: encoding?!
    return QString::fromLocal8Bit(m_request->uri);
}

QString WebServerRequest::queryString() const
{
    ///TODO: encoding?!
    return QString::fromLocal8Bit(m_request->query_string);
}

QString WebServerRequest::remoteIP() const
{
    return QHostAddress(m_request->remote_ip).toString();
}

int WebServerRequest::remotePort() const
{
    return m_request->remote_port;
}

QString WebServerRequest::remoteUser() const
{
    ///TODO: encoding?!
    return QString::fromLocal8Bit(m_request->remote_user);
}

int WebServerRequest::headers() const
{
    return m_request->num_headers;
}

QString WebServerRequest::headerName(int header) const
{
    Q_ASSERT(header >= 0 && header < m_request->num_headers);
    ///TODO: encoding?!
    return QString::fromLocal8Bit(m_request->http_headers[header].name);
}

QString WebServerRequest::headerValue(int header) const
{
    Q_ASSERT(header >= 0 && header < m_request->num_headers);
    ///TODO: encoding?!
    return QString::fromLocal8Bit(m_request->http_headers[header].value);
}

//END WebServerRequest

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

void WebServerResponse::writeHeaders(int statusCode, const QVariantMap &headers)
{
    ///TODO: what is the best-practice error handling in javascript? exceptions?
    Q_ASSERT(!m_headersSent);
    m_headersSent = true;
    mg_printf(m_conn, "HTTP/1.1 %d %s\r\n", m_statusCode, responseCodeString(m_statusCode));
    QVariantMap::const_iterator it = headers.constBegin();
    while(it != headers.constEnd()) {
        mg_printf(m_conn, "%s: %s\r\n", qPrintable(it.key()), qPrintable(it.value().toString()));
        ++it;
    }
    mg_write(m_conn, "\r\n", 2);
}

void WebServerResponse::writeBody(const QString &body)
{
    if (!m_headersSent) {
        writeHeaders(m_statusCode, m_headers);
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
