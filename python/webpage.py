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

from PyQt4.QtCore import QUrl, QEventLoop, qDebug
from PyQt4.QtGui import QApplication
from PyQt4.QtWebKit import QWebPage

from plugincontroller import Bunch, do_action

class WebPage(QWebPage):
    def __init__(self, parent=None):
        QWebPage.__init__(self, parent)

        self.parent = parent
        self.m_nextFileTag = ''
        self.m_userAgent = QWebPage.userAgentForUrl(self, QUrl())

        if self.parent.m_verbose:
            self.currentFrame().urlChanged.connect(self.handleFrameUrlChanged)
            self.linkClicked.connect(self.handleLinkClicked)

        do_action('WebPageInit', Bunch(locals()))

    def handleFrameUrlChanged(self, url):
        qDebug('URL Changed: %s' % url.toString())

    def handleLinkClicked(self, url):
        qDebug('URL Clicked: %s' % url.toString())

    def javaScriptAlert(self, webframe, msg):
        print 'JavaScript alert: %s' % msg

    def javaScriptConsoleMessage(self, message, lineNumber, sourceID):
        if sourceID:
            print '%s:%d %s' % (sourceID, lineNumber, message)
        else:
            print message

    def shouldInterruptJavaScript(self):
        QApplication.processEvents(QEventLoop.AllEvents, 42)
        return False

    def userAgentForUrl(self, url):
        return self.m_userAgent

    def chooseFile(self, webframe, suggestedFile):
        if self.m_nextFileTag in self.parent.m_upload_file:
            return self.parent.m_upload_file[self.m_nextFileTag]
        return ''

    do_action('WebPage', Bunch(locals()))
