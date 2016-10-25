/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Copyright (C) 2011 Ivan De Marino <ivan.de.marino@gmail.com>
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

#include "encoding.h"
#include "mongoose/mongoose.h"
#include "consts.h"

#include <QByteArray>
#include <QHostAddress>
#include <QMetaType>
#include <QThread>
#include <QUrl>
#include <QVector>
#include <QDebug>

namespace UrlEncodedParser
{

QString unescape(QByteArray in)
{
    // first step: decode '+' to spaces
    for (int i = 0; i < in.length(); ++i) {
        QByteRef c = in[i];
        if (c == '+') {
            c = ' ';
        }
    }
    // now decode as usual
    return QUrl::fromPercentEncoding(in);
};

// Parse a application/x-www-form-urlencoded data string
QVariantMap parse(const QByteArray& data)
{
    QVariantMap ret;
    if (data.isEmpty()) {
        return ret;
    }
    foreach(const QByteArray & part, data.split('&')) {
        const int eqPos = part.indexOf('=');
        if (eqPos == -1) {
            ret[unescape(part)] = "";
        } else {
            const QByteArray key = part.mid(0, eqPos);
            const QByteArray value = part.mid(eqPos + 1);
            ret[unescape(key)] = unescape(value);
        }
    }

    return ret;
}

}

static void* callback(mg_event event,
                      mg_connection* conn,
                      const mg_request_info* request)
{
    WebServer* server = static_cast<WebServer*>(request->user_data);
    if (server->handleRequest(event, conn, request)) {
        // anything non-null... pretty ugly, why not simply a bool??
        return server;
    } else {
        return 0;
    }
}

WebServer::WebServer(QObject* parent)
    : QObject(parent)
    , m_ctx(0)
{
    setObjectName("WebServer");
    qRegisterMetaType<WebServerResponse*>("WebServerResponse*");
}

WebServer::~WebServer()
{
    close();
}

bool WebServer::listenOnPort(const QString& port, const QVariantMap& opts)
{
    close();

    // Create options vector
    QVector<const char*> options;
    options <<  "listening_ports" << qstrdup(qPrintable(port));
    options << "enable_directory_listing" << "no";
    if (opts.value("keepAlive", false).toBool()) {
        options << "enable_keep_alive" << "yes";
    }
    options << NULL;

    // Start the server
    m_ctx = mg_start(&callback, this, options.data());
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
        m_closing = 1;
        {
            // make sure we wake up all pending responses, such that mg_stop()
            // can be called without deadlocking
            QMutexLocker lock(&m_mutex);
            foreach(WebServerResponse * response, m_pendingResponses) {
                response->close();
            }
        }
        mg_stop(m_ctx);
        m_ctx = 0;
        m_port.clear();
    }
}

bool WebServer::handleRequest(mg_event event, mg_connection* conn, const mg_request_info* request)
{
    if (event != MG_NEW_REQUEST) {
        return false;
    }

    if (m_closing.loadAcquire()) {
        return false;
    }

    // Modelled after http://nodejs.org/docs/latest/api/http.html#http.ServerRequest
    QVariantMap requestObject;

    qDebug() << "HTTP Request - URI" << request->uri;
    qDebug() << "HTTP Request - Method" << request->request_method;
    qDebug() << "HTTP Request - HTTP Version" << request->http_version;
    qDebug() << "HTTP Request - Query String" << request->query_string;

    // Presumably we would not have gotten this far if the
    // request_method or http_version were syntactically invalid.
    // Therefore, per RFC 2616, they *should* consist entirely of
    // US-ASCII characters, so use of QString::fromLatin1 here is safe.
    // (It'd be nice if QString had a fromAsciiStrict or something that
    // would throw an exception if the argument was outside ASCII.)
    if (request->request_method) {
        requestObject["method"] = QString::fromLatin1(request->request_method);
    }
    if (request->http_version) {
        requestObject["httpVersion"] = QString::fromLatin1(request->http_version);
    }
    if (request->status_code >= 0) {
        requestObject["statusCode"] = request->status_code;
    }

    // request->uri and request->query_string may contain arbitrary
    // bytes, and their encoding is unknown.  We must not do anything
    // that would cause an attempt to decode characters outside the
    // ASCII printable range.  (The encoding might not even be ASCII-
    // compatible!)  QByteArray::toPercentEncoding, unlike
    // QUrl::toEncoded and friends, makes no assumptions about the
    // encoding of high-half bytes.
    //
    // See the long comment beginning "From RFC 3986, Appendix A
    // Collected ABNF for URI" in qurl.cpp for rationale for the
    // exclude strings used here.  (The short version is that of all
    // the gen-delims and sub-delims, only '?' and '#' should be
    // force-encoded in ->uri, and only '#' should be force-encoded
    // in ->query_string.)
    QByteArray uri(request->uri);
    uri = uri.toPercentEncoding(/*exclude=*/ "!$&'()*+,;=:/[]@");
    if (request->query_string) {
        QByteArray qs(request->query_string);
        qs = qs.toPercentEncoding(/*exclude=*/ "!$&'()*+,;=:/[]@?");
        uri.append('?');
        uri.append(qs);
    }
    requestObject["url"] = uri.data();

#if 0
    // Non-standard and thus disable for the time being.
    requestObject["isSSL"] = request->is_ssl;
    requestObject["remoteIP"] = QHostAddress(request->remote_ip).toString();;
    requestObject["remotePort"] = request->remote_port;
    if (request->remote_user) {
        requestObject["remoteUser"] = QString::fromLocal8Bit(request->remote_user);
    }
#endif

    QVariantMap headersObject;
    QMap<QString, QString> ciHeadersObject;               //< FIXME: "case-insensitive" Headers. This shows how desperately we need a better HTTP Server
    for (int i = 0; i < request->num_headers; ++i) {
        QString key = QString::fromLocal8Bit(request->http_headers[i].name);
        QString value = QString::fromLocal8Bit(request->http_headers[i].value);
        qDebug() << "HTTP Request - Receiving Header" << key << "=" << value;
        headersObject[key] = value;
        ciHeadersObject[key.toLower()] = value;
    }
    requestObject["headers"] = headersObject;

    // Read request body ONLY for POST and PUT, and ONLY if the "Content-Length" is provided
    if ((requestObject["method"] == "POST" || requestObject["method"] == "PUT") && ciHeadersObject.contains(HTTP_HEADER_CONTENT_LENGTH)) {
        bool contentLengthKnown = false;
        uint contentLength = ciHeadersObject[HTTP_HEADER_CONTENT_LENGTH].toUInt(&contentLengthKnown);

        qDebug() << "HTTP Request - Method POST/PUT";

        // Proceed only if we were able to read the "Content-Length"
        if (contentLengthKnown) {
            ++contentLength; //< make space for null termination
            char* data = new char[contentLength];
            int read = mg_read(conn, data, contentLength);
            data[read] = '\0'; //< adding null termination (no arm if it's already there)

            qDebug() << "HTTP Request - Content Body:" << qPrintable(data);

            // Check if the 'Content-Type' requires decoding
            if (ciHeadersObject[HTTP_HEADER_CONTENT_TYPE] == "application/x-www-form-urlencoded") {
                requestObject["post"] = UrlEncodedParser::parse(QByteArray(data, read));
                requestObject["postRaw"] = QString::fromUtf8(data, read);
            } else {
                requestObject["post"] = QString::fromUtf8(data, read);
            }
            delete[] data;
        } else {
            qWarning() << "HTTP Request - Malformed 'Content-Length'";
        }
    }

    // Emit signal that is catched by the PhantomJS callback,
    // then wait until response.close() was called from
    // the PhantomJS script.
    //
    // This is achieved using the wait semaphore, which is
    // acquired here, in the background thread, and released
    // in WebServerResponse::close() i.e. the foreground thread
    QSemaphore wait;
    WebServerResponse responseObject(conn, &wait);
    responseObject.moveToThread(thread());

    {
        if (m_closing.loadAcquire()) {
            return false;
        }
        QMutexLocker lock(&m_mutex);
        if (m_closing.loadAcquire()) {
            return false;
        }
        m_pendingResponses << (&responseObject);
    }
    newRequest(requestObject, &responseObject);
    wait.acquire();
    {
        if (m_closing.loadAcquire()) {
            return false;
        }
        QMutexLocker lock(&m_mutex);
        if (m_closing.loadAcquire()) {
            return false;
        }
        m_pendingResponses.removeOne(&responseObject);
    }
    return true;
}


//BEGIN WebServerResponse

WebServerResponse::WebServerResponse(mg_connection* conn, QSemaphore* close)
    : QObject()
    , m_conn(conn)
    , m_statusCode(200)
    , m_headersSent(false)
    , m_close(close)
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

void WebServerResponse::writeHead(int statusCode, const QVariantMap& headers)
{
    ///TODO: what is the best-practice error handling in javascript? exceptions?
    Q_ASSERT(!m_headersSent);
    m_headersSent = true;
    m_statusCode = statusCode;
    mg_printf(m_conn, "HTTP/1.1 %d %s\r\n", m_statusCode, responseCodeString(m_statusCode));
    qDebug() << "HTTP Response - Status Code" << m_statusCode << responseCodeString(m_statusCode);
    QVariantMap::const_iterator it = headers.constBegin();
    while (it != headers.constEnd()) {
        qDebug() << "HTTP Response - Sending Header" << it.key() << "=" << it.value().toString();
        mg_printf(m_conn, "%s: %s\r\n", qPrintable(it.key()), qPrintable(it.value().toString()));
        ++it;
    }
    mg_write(m_conn, "\r\n", 2);
}

void WebServerResponse::write(const QVariant& body)
{
    if (!m_headersSent) {
        writeHead(m_statusCode, m_headers);
    }

    QByteArray data;
    if (m_encoding.isEmpty()) {
        data = body.toString().toUtf8();
    } else if (m_encoding.toLower() == "binary") {
        data = body.toString().toLatin1();
    } else {
        Encoding encoding;
        encoding.setEncoding(m_encoding);
        data = encoding.encode(body.toString());
    }

    mg_write(m_conn, data.constData(), data.size());
}

void WebServerResponse::setEncoding(const QString& encoding)
{
    m_encoding = encoding;
}

void WebServerResponse::close()
{
    m_close->release();
}

void WebServerResponse::closeGracefully()
{
    write("");
    close();
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

QString WebServerResponse::header(const QString& name) const
{
    return m_headers.value(name).toString();
}

void WebServerResponse::setHeader(const QString& name, const QString& value)
{
    ///TODO: what is the best-practice error handling in javascript? exceptions?
    Q_ASSERT(!m_headersSent);
    m_headers.insert(name, value);
}

QVariantMap WebServerResponse::headers() const
{
    return m_headers;
}

void WebServerResponse::setHeaders(const QVariantMap& headers)
{
    ///TODO: what is the best-practice error handling in javascript? exceptions?
    Q_ASSERT(!m_headersSent);
    m_headers = headers;
}

//END WebServerResponse
