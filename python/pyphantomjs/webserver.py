'''
  This file is part of the PyPhantomJS project.

  Copyright (C) 2011 James Roe <roejames12@hotmail.com>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.
'''

import socket
from BaseHTTPServer import (__version__ as __server_version__,
                            BaseHTTPRequestHandler, HTTPServer)
from cStringIO import StringIO
from threading import Thread
from SocketServer import ThreadingMixIn
from urlparse import urlparse

import sip
from PyQt4.QtCore import (pyqtProperty, pyqtSignal, pyqtSlot, Q_ARG,
                          qDebug, QMetaObject, QObject, Qt, QThread)

from __init__ import __version__
from plugincontroller import do_action
from utils import CaseInsensitiveDict


servers = []


class ThreadingHTTPServer(ThreadingMixIn, HTTPServer):
    pass


class WebServer(QObject):
    newRequest = pyqtSignal(QObject, QObject)

    def __init__(self, parent):
        super(WebServer, self).__init__(parent)

        self.setObjectName('WebServer')

        self.m_port = 0
        self.httpd = None

        servers.append(self)

        do_action('WebServerInit')

    def __del__(self):
        self.close()

    @pyqtSlot()
    def close(self):
        if self.httpd is not None:
            self.httpd.shutdown()
            self.m_port = 0

    @pyqtSlot(int, result=bool)
    def listenOnPort(self, port):
        try:
            self.httpd = ThreadingHTTPServer(('localhost', port), WebServerHandler)
            Thread(target=self.httpd.serve_forever).start()
        except socket.error as (_, e):
            qDebug('WebServer.listenOnPort - %s' % e)
            self.m_port = 0
            return False
        else:
            self.m_port = port
        return True

    @pyqtProperty(int)
    def port(self):
        return self.m_port

    @pyqtSlot()
    def release(self):
        self.close()
        servers.remove(self)
        self.parent().m_servers.remove(self)
        sip.delete(self)

    do_action('WebServer')


class WebServerHandler(BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'
    server_version = 'BaseHTTP/%s PyPhantomJS/%s' % (__server_version__, __version__)

    def handle_one_request(self):
        '''Handle a single HTTP request.

           A patch to the request handling:

           * This removes the functionality which calls do_X(),
             and simply calls our handleRequest() directly.
           * If do_X() didn't exist, a 501 unsupported
             method error would be returned. Since we patched
             that out as well, the user can actually handle when
             a request is or isn't valid through the code.
           * Then we handle the request and response.
        '''
        try:
            self.raw_requestline = self.rfile.readline(65537)
            if len(self.raw_requestline) > 65536:
                self.requestline = ''
                self.request_version = ''
                self.command = ''
                self.send_error(414)
                return
            if not self.raw_requestline:
                self.close_connection = 1
                return
            if not self.parse_request():
                # An error code has been sent, just exit
                return

            # these variables must get (re)set on every request
            # for handleRequest(), due to the HTTP/1.1 nature of
            # keepalive, which seems to keep a constant running
            # instance of this class
            self.m_handleResponse = True
            self.m_headers = CaseInsensitiveDict()
            self.m_statusCode = 200
            self.m_wfile = StringIO()

            self.handleRequest()

            # some methods handle the response on their own instead
            if self.m_handleResponse:
                self.handleResponse()

            self.wfile.flush() #actually send the response if not already done.
        except socket.timeout, e:
            #a read or a write timed out.  Discard this connection
            self.log_error("Request timed out: %r", e)
            self.close_connection = 1
            return

    def handleResponse(self):
        # send response code with response text (ex. 200 OK)
        self.send_response(self.m_statusCode, self.responses[self.m_statusCode][0])

        # send all headers
        for name, value in self.m_headers.items():
            self.send_header(name, value)

        # send content-length if not already sent
        if self.protocol_version == 'HTTP/1.1':
            if 'content-length' not in self.m_headers:
                if self.m_statusCode >= 200 and self.m_statusCode not in (204, 304):
                    self.send_header('Content-Length', self.m_wfile.tell())

        self.end_headers() #finish sending the headers

        # check if body should be written to response
        if self.command != 'HEAD' and self.m_statusCode >= 200 and self.m_statusCode not in (204, 304):
            self.wfile.write(self.m_wfile.getvalue()) #write body to page

    def handleRequest(self):
        request = WebServerRequest(self)
        response = WebServerResponse(self)

        for server in servers:
            # verify which server this request is for
            if self.server == server.httpd:
                connectionType = Qt.BlockingQueuedConnection
                if QThread.currentThread() == server.thread():
                    connectionType = Qt.DirectConnection

                QMetaObject.invokeMethod(server, 'newRequest', connectionType,
                                         Q_ARG(WebServerRequest, request),
                                         Q_ARG(WebServerResponse, response))
                break

    def log_message(self, format, *args):
        qDebug("%s - - %s" % (self.address_string(),
                                format % args))

    do_action('WebServerHandler')


class WebServerResponse(QObject):
    def __init__(self, conn):
        super(WebServerResponse, self).__init__()

        self.setObjectName('WebServerResponse')

        self.m_conn = conn

        do_action('WebServerResponseInit')

    @pyqtSlot(str, result=str)
    def header(self, name):
        return self.m_conn.m_headers.get(name, '')

    @pyqtProperty('QVariantMap')
    def headers(self):
        return self.m_conn.m_headers

    @headers.setter
    def headers(self, headers):
        self.m_conn.m_headers = CaseInsensitiveDict(headers)

    @pyqtSlot(int)
    @pyqtSlot(int, str)
    def sendError(self, code, message=None):
        self.m_conn.send_error(code, message)
        self.m_conn.m_handleResponse = False

    @pyqtSlot(str, str)
    def setHeader(self, name, value):
        self.m_conn.m_headers[name] = value

    @pyqtProperty(int)
    def statusCode(self):
        return self.m_conn.m_statusCode

    @statusCode.setter
    def statusCode(self, code):
        self.m_conn.m_statusCode = code

    @pyqtSlot(str)
    def write(self, body):
        self.m_conn.m_wfile.write(body)

    @pyqtSlot(int, 'QVariantMap')
    def writeHead(self, code, headers):
        self.m_conn.m_statusCode = code
        self.m_conn.m_headers = CaseInsensitiveDict(headers)

    do_action('WebServerResponse')


class WebServerRequest(QObject):
    def __init__(self, request):
        super(WebServerRequest, self).__init__()

        self.setObjectName('WebServerRequest')

        self.m_request = request
        self.m_headerNames = list(request.headers)
        self.m_headers = CaseInsensitiveDict(request.headers)
        self.m_numHeaders = len(request.headers)

        do_action('WebServerRequestInit')

    @pyqtSlot(int, result=str)
    def headerName(self, header):
        if header <= self.m_numHeaders:
            return self.m_headerNames[header]
        return ''

    @pyqtProperty(int)
    def headers(self):
        return self.m_numHeaders

    @pyqtSlot(int, result=str)
    def headerValue(self, header):
        if header <= self.m_numHeaders:
            return self.m_request.headers[self.m_headerNames[header]]
        return ''

    @pyqtProperty(str)
    def httpVersion(self):
        return self.m_request.request_version[-3:]

    @pyqtProperty(int)
    def isSSL(self):
        return 0

    @pyqtProperty(str)
    def method(self):
        return self.m_request.command

    @pyqtProperty(str)
    def remoteIP(self):
        return self.m_request.client_address[0]

    @pyqtProperty(int)
    def remotePort(self):
        return self.m_request.client_address[1]

    @pyqtProperty(str)
    def remoteUser(self):
        try:
            return self.m_headers['REMOTE_USER']
        except KeyError:
            return ''

    #@pyqtProperty(int)
    #def statusCode(self):
    #    return self.m_request.status_code

    @pyqtProperty(str)
    def url(self):
        return self.m_request.path

    @pyqtProperty(str)
    def queryString(self):
        return urlparse(self.m_request.path).query

    do_action('WebServerRequest')
