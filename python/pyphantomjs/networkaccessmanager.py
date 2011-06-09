'''
  This file is part of the PyPhantomJS project.

  Copyright (C) 2011 James Roe <roejames12@hotmail.com>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
'''

from PyQt4.QtGui import QDesktopServices
from PyQt4.QtCore import pyqtSignal, QDateTime
from PyQt4.QtNetwork import QNetworkAccessManager, QNetworkDiskCache, \
                            QNetworkRequest

from plugincontroller import Bunch, do_action


class NetworkAccessManager(QNetworkAccessManager):
    resourceReceived = pyqtSignal('QVariantMap')
    resourceRequested = pyqtSignal('QVariantMap')

    def __init__(self, diskCacheEnabled, ignoreSslErrors, parent=None):
        QNetworkAccessManager.__init__(self, parent)

        self.m_ignoreSslErrors = ignoreSslErrors
        self.m_idCounter = 0
        self.m_ids = {}
        self.m_started = []

        self.finished.connect(self.handleFinished)

        if diskCacheEnabled:
            m_networkDiskCache = QNetworkDiskCache()
            m_networkDiskCache.setCacheDirectory(QDesktopServices.storageLocation(QDesktopServices.CacheLocation))
            self.setCache(m_networkDiskCache)

        do_action('NetworkAccessManagerInit', Bunch(locals()))

    def createRequest(self, op, req, outgoingData):
        do_action('NetworkAccessManagerCreateRequestPre', Bunch(locals()))

        reply = QNetworkAccessManager.createRequest(self, op, req, outgoingData)

        if self.m_ignoreSslErrors:
            reply.ignoreSslErrors()

        headers = []
        for header in req.rawHeaderList():
            header = {
                'name': str(header),
                'value': str(req.rawHeader(header))
            }
            headers.append(header)

        self.m_idCounter += 1
        self.m_ids[reply] = self.m_idCounter

        data = {
            'id': self.m_idCounter,
            'url': req.url().toString(),
            'method': toString(op),
            'headers': headers,
            'time': QDateTime.currentDateTime()
        }

        reply.readyRead.connect(self.handleStarted)

        do_action('NetworkAccessManagerCreateRequestPost', Bunch(locals()))

        self.resourceRequested.emit(data)
        return reply

    def handleFinished(self, reply):
        headers = []
        for header in reply.rawHeaderList():
            header = {
                'name': str(header),
                'value': str(reply.rawHeader(header))
            }
            headers.append(header)

        data = {
            'stage': 'end',
            'id': self.m_ids[reply],
            'url': reply.url().toString(),
            'status': reply.attribute(QNetworkRequest.HttpStatusCodeAttribute),
            'statusText': reply.attribute(QNetworkRequest.HttpReasonPhraseAttribute),
            'contentType': reply.header(QNetworkRequest.ContentTypeHeader),
            'redirectURL': reply.header(QNetworkRequest.LocationHeader),
            'headers': headers,
            'time': QDateTime.currentDateTime()
        }

        del self.m_ids[reply]
        if reply in self.m_started:
            del self.m_started[self.m_started.index(reply)]

        do_action('NetworkAccessManagerHandleFinished', Bunch(locals()))

        self.resourceReceived.emit(data)

    def handleStarted(self):
        reply = self.sender()
        if not reply:
            return
        if reply in self.m_started:
            return

        self.m_started.append(reply)

        headers = []
        for header in reply.rawHeaderList():
            header = {
                'name': str(header),
                'value': str(reply.rawHeader(header))
            }
            headers.append(header)

        data = {
            'stage': 'start',
            'id': self.m_ids[reply],
            'url': reply.url().toString(),
            'status': reply.attribute(QNetworkRequest.HttpStatusCodeAttribute),
            'statusText': reply.attribute(QNetworkRequest.HttpReasonPhraseAttribute),
            'contentType': reply.header(QNetworkRequest.ContentTypeHeader),
            'bodySize': reply.size(),
            'redirectURL': reply.header(QNetworkRequest.LocationHeader),
            'headers': headers,
            'time': QDateTime.currentDateTime()
        }

        do_action('NetworkAccessManagerHandleStarted', Bunch(locals()))

        self.resourceReceived.emit(data)

    do_action('NetworkAccessManager', Bunch(locals()))


def toString(op):
    verb = '?'

    if op == QNetworkAccessManager.HeadOperation:
        verb = 'HEAD'
    elif op == QNetworkAccessManager.GetOperation:
        verb = 'GET'
    elif op == QNetworkAccessManager.PutOperation:
        verb = 'PUT'
    elif op == QNetworkAccessManager.PostOperation:
        verb = 'POST'
    elif op == QNetworkAccessManager.DeleteOperation:
        verb = 'DELETE'

    return verb
