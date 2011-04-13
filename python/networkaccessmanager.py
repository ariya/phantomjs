'''
  This file is part of the PyPhantomJS project.

  Copyright (C) 2011 James Roe <roejames12@hotmail.com>
  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>

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

from PyQt4.QtCore import SIGNAL, QString, qDebug, qWarning
from PyQt4.QtNetwork import QNetworkAccessManager

class NetworkAccessManager(QNetworkAccessManager):
    def __init__(self, parent = None):
        QNetworkAccessManager.__init__(self, parent)
        self.connect(self, SIGNAL('finished(QNetworkReply *)'), self.handleFinished)

    def createRequest(self, op, req, outgoingData):
        if op == QNetworkAccessManager.GetOperation:
            qDebug('HTTP/1.1 GET Request')
        elif op == QNetworkAccessManager.PostOperation:
            qDebug('HTTP/1.1 POST Request')
        elif op == QNetworkAccessManager.HeadOperation:
            qDebug('HTTP/1.1 HEAD Request')
        elif op == QNetworkAccessManager.PutOperation:
            qDebug('HTTP/1.1 PUT Request')
        elif op == QNetworkAccessManager.DeleteOperation:
            qDebug('HTTP/1.1 DELETE Request')
        elif op == QNetworkAccessManager.CustomOperation:
            qDebug('HTTP/1.1 CUSTOM Request')
        else:
            qWarning('Unexpected HTTP Operation Type')

        qDebug(QString('URL %s' % req.url().toString()))

        return QNetworkAccessManager.createRequest(self, op, req, outgoingData)

    def handleFinished(self, reply):
        qDebug('HTTP/1.1 Response')
        qDebug(QString('URL %s' % reply.url().toString()))

        headerPairs = reply.rawHeaderPairs()
        for pair in headerPairs:
            qDebug('"%s" = "%s"' % (pair[0], pair[1]))
