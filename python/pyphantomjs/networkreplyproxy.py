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

from PyQt4.QtNetwork import QNetworkRequest, QNetworkReply


class NetworkReplyProxy(QNetworkReply):
    def __init__(self, parent, reply):
        super(NetworkReplyProxy, self).__init__(parent)

        self.m_reply = reply
        self.m_buffer = self.m_data = ''

        # apply attributes
        self.setOperation(self.m_reply.operation())
        self.setRequest(self.m_reply.request())
        self.setUrl(self.m_reply.url())

        # handle these to forward
        self.m_reply.metaDataChanged.connect(self.applyMetaData)
        self.m_reply.readyRead.connect(self.readInternal)
        self.m_reply.error.connect(self.errorInternal)

        # forward signals
        self.m_reply.finished.connect(self.finished)
        self.m_reply.uploadProgress.connect(self.uploadProgress)
        self.m_reply.downloadProgress.connect(self.downloadProgress)

        # for the data proxy...
        self.setOpenMode(QNetworkReply.ReadOnly)

    def abort(self):
        self.m_reply.abort()

    def applyMetaData(self):
        for header in self.m_reply.rawHeaderList():
            self.setRawHeader(header, self.m_reply.rawHeader(header))

        self.setHeader(QNetworkRequest.ContentTypeHeader, self.m_reply.header(QNetworkRequest.ContentTypeHeader))
        self.setHeader(QNetworkRequest.ContentLengthHeader, self.m_reply.header(QNetworkRequest.ContentLengthHeader))
        self.setHeader(QNetworkRequest.LocationHeader, self.m_reply.header(QNetworkRequest.LocationHeader))
        self.setHeader(QNetworkRequest.LastModifiedHeader, self.m_reply.header(QNetworkRequest.LastModifiedHeader))
        # :NOTE: This statement is commented due to a bug in PyQt where the cookie headers can't be set correctly due to
        # the cookie header expecting QList, but Python Lists are auto-converted to QVariantList, and it is incompatible.
        # This bug should be fixed in PyQt 4.8, however we need to maintain backward compatibility with 4.7
        # [WARNING] QNetworkRequest::setHeader: QVariant of type QVariantList cannot be used with header Set-Cookie
        #self.setHeader(QNetworkRequest.SetCookieHeader, self.m_reply.header(QNetworkRequest.SetCookieHeader))

        self.setAttribute(QNetworkRequest.HttpStatusCodeAttribute, self.m_reply.attribute(QNetworkRequest.HttpStatusCodeAttribute))
        self.setAttribute(QNetworkRequest.HttpReasonPhraseAttribute, self.m_reply.attribute(QNetworkRequest.HttpReasonPhraseAttribute))
        self.setAttribute(QNetworkRequest.RedirectionTargetAttribute, self.m_reply.attribute(QNetworkRequest.RedirectionTargetAttribute))
        self.setAttribute(QNetworkRequest.ConnectionEncryptedAttribute, self.m_reply.attribute(QNetworkRequest.ConnectionEncryptedAttribute))
        self.setAttribute(QNetworkRequest.CacheLoadControlAttribute, self.m_reply.attribute(QNetworkRequest.CacheLoadControlAttribute))
        self.setAttribute(QNetworkRequest.CacheSaveControlAttribute, self.m_reply.attribute(QNetworkRequest.CacheSaveControlAttribute))
        self.setAttribute(QNetworkRequest.SourceIsFromCacheAttribute, self.m_reply.attribute(QNetworkRequest.SourceIsFromCacheAttribute))
        self.setAttribute(QNetworkRequest.DoNotBufferUploadDataAttribute, self.m_reply.attribute(QNetworkRequest.DoNotBufferUploadDataAttribute))

        self.metaDataChanged.emit()

    def body(self):
        return self.m_data

    def bytesAvailable(self):
        return len(self.m_buffer) + self.m_reply.bytesAvailable()

    def bytesToWrite(self):
        return -1

    def close(self):
        self.m_reply.close()

    def errorInternal(self, error):
        self.setError(error, str(error))
        self.error.emit(error)

    def ignoreSslErrors(self):
        self.m_reply.ignoreSslErrors()

    def isSequential(self):
        return self.m_reply.isSequential()

    def readData(self, maxlen):
        size = min(maxlen, len(self.m_buffer))
        data, self.m_buffer = self.m_buffer[:size], self.m_buffer[size:]
        return data.data()

    def readInternal(self):
        data = self.m_reply.readAll()
        self.m_data += data
        self.m_buffer += data
        self.readyRead.emit()

    def setReadBufferSize(self, size):
        QNetworkReply.setReadBufferSize(size)
        self.m_reply.setReadBufferSize(size)
