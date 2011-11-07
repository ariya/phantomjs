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

//END WebServerRequest

//BEGIN WebServerResponse

WebServerResponse::WebServerResponse(mg_connection *conn)
    : m_conn(conn)
{

}

void WebServerResponse::writeHeaders(const QString &headers)
{
    mg_printf(m_conn, "%s", qPrintable(headers));
}

void WebServerResponse::writeBody(const QString &body)
{
    mg_printf(m_conn, "%s", qPrintable(body));
}


//END WebServerResponse
