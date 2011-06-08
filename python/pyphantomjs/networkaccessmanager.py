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
from PyQt4.QtCore import pyqtSignal
from PyQt4.QtNetwork import QNetworkAccessManager, QNetworkDiskCache, \
                            QNetworkRequest

from plugincontroller import Bunch, do_action


class NetworkAccessManager(QNetworkAccessManager):
    resourceReceived = pyqtSignal('QVariantMap')
    resourceRequested = pyqtSignal('QVariantMap')

    def __init__(self, diskCacheEnabled, ignoreSslErrors, parent=None):
        QNetworkAccessManager.__init__(self, parent)
        self.m_ignoreSslErrors = ignoreSslErrors

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

        data = {
            'url': req.url().toString(),
            'method': toString(op),
            'headers': headers
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
            'url': reply.url().toString(),
            'status': reply.attribute(QNetworkRequest.HttpStatusCodeAttribute),
            'headers': headers
        }

        do_action('NetworkAccessManagerHandleFinished', Bunch(locals()))

        self.resourceReceived.emit(data)

    def handleStarted(self):
        reply = self.sender()
        if not reply:
            return

        headers = []
        for header in reply.rawHeaderList():
            header = {
                'name': str(header),
                'value': str(reply.rawHeader(header))
            }
            headers.append(header)

        data = {
            'stage': 'start',
            'url': reply.url().toString(),
            'status': reply.attribute(QNetworkRequest.HttpStatusCodeAttribute),
            'headers': headers
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
