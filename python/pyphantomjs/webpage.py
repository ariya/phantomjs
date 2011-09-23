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

from math import ceil, floor

import sip
from PyQt4.QtCore import (pyqtProperty, pyqtSignal, pyqtSlot, QByteArray,
                          QDir, QEvent, QEventLoop, QFileInfo, QObject,
                          QPoint, QRect, QSize, QSizeF, Qt, QUrl)
from PyQt4.QtGui import (QApplication, QDesktopServices, QImage,
                         QMouseEvent, QPainter, QPalette, QPrinter,
                         QRegion, qRgba)
from PyQt4.QtNetwork import QNetworkAccessManager, QNetworkRequest
from PyQt4.QtWebKit import QWebPage, QWebSettings

from networkaccessmanager import NetworkAccessManager
from plugincontroller import do_action
from utils import injectJsInFrame


class CustomPage(QWebPage):
    def __init__(self, parent):
        super(CustomPage, self).__init__(parent)

        self.m_userAgent = QWebPage.userAgentForUrl(self, QUrl())

        self.m_uploadFile = ''

        do_action('CustomPageInit')

    def chooseFile(self, originatingFrame, oldFile):
        return self.m_uploadFile

    def shouldInterruptJavaScript(self):
        QApplication.processEvents(QEventLoop.AllEvents, 42)
        return False

    def javaScriptAlert(self, originatingFrame, msg):
        self.parent().javaScriptAlertSent.emit(msg)

    def javaScriptConsoleMessage(self, message, lineNumber, sourceID):
        self.parent().javaScriptConsoleMessageSent.emit(message, lineNumber, sourceID)

    def userAgentForUrl(self, url):
        return self.m_userAgent

    do_action('CustomPage')


class WebPage(QObject):
    initialized = pyqtSignal()
    javaScriptAlertSent = pyqtSignal(str)
    javaScriptConsoleMessageSent = pyqtSignal(str, int, str)
    loadStarted = pyqtSignal()
    loadFinished = pyqtSignal(str)
    resourceReceived = pyqtSignal('QVariantMap')
    resourceRequested = pyqtSignal('QVariantMap')

    blankHtml = '<html><head></head><body></body></html>'

    def __init__(self, parent, args):
        super(WebPage, self).__init__(parent)

        # variable declarations
        self.m_paperSize = {}
        self.m_clipRect = QRect()
        self.m_libraryPath = ''
        self.m_scrollPosition = QPoint()

        self.setObjectName('WebPage')
        self.m_webPage = CustomPage(self)
        self.m_mainFrame = self.m_webPage.mainFrame()

        self.m_mainFrame.javaScriptWindowObjectCleared.connect(self.initialized)
        self.m_webPage.loadStarted.connect(self.loadStarted)
        self.m_webPage.loadFinished.connect(self.finish)

        # Start with transparent background
        palette = self.m_webPage.palette()
        palette.setBrush(QPalette.Base, Qt.transparent)
        self.m_webPage.setPalette(palette)

        # Page size does not need to take scrollbars into account
        self.m_webPage.mainFrame().setScrollBarPolicy(Qt.Horizontal, Qt.ScrollBarAlwaysOff)
        self.m_webPage.mainFrame().setScrollBarPolicy(Qt.Vertical, Qt.ScrollBarAlwaysOff)

        self.m_webPage.settings().setAttribute(QWebSettings.OfflineStorageDatabaseEnabled, True)
        self.m_webPage.settings().setOfflineStoragePath(QDesktopServices.storageLocation(QDesktopServices.DataLocation))
        self.m_webPage.settings().setAttribute(QWebSettings.LocalStorageDatabaseEnabled, True)
        self.m_webPage.settings().setAttribute(QWebSettings.OfflineWebApplicationCacheEnabled, True)
        self.m_webPage.settings().setOfflineWebApplicationCachePath(QDesktopServices.storageLocation(QDesktopServices.DataLocation))
        self.m_webPage.settings().setAttribute(QWebSettings.FrameFlatteningEnabled, True)
        self.m_webPage.settings().setAttribute(QWebSettings.LocalStorageEnabled, True)
        self.m_webPage.settings().setLocalStoragePath(QDesktopServices.storageLocation(QDesktopServices.DataLocation))

        # Ensure we have a document.body.
        self.m_webPage.mainFrame().setHtml(self.blankHtml)

        # Custom network access manager to allow traffic monitoring
        self.m_networkAccessManager = NetworkAccessManager(self.parent(), args)
        self.m_webPage.setNetworkAccessManager(self.m_networkAccessManager)
        self.m_networkAccessManager.resourceRequested.connect(self.resourceRequested)
        self.m_networkAccessManager.resourceReceived.connect(self.resourceReceived)

        self.m_webPage.setViewportSize(QSize(400, 300))

        do_action('WebPageInit')

    def applySettings(self, defaults):
        opt = self.m_webPage.settings()

        opt.setAttribute(QWebSettings.AutoLoadImages, defaults['loadImages'])
        opt.setAttribute(QWebSettings.PluginsEnabled, defaults['loadPlugins'])
        opt.setAttribute(QWebSettings.JavascriptEnabled, defaults['javascriptEnabled'])
        opt.setAttribute(QWebSettings.XSSAuditingEnabled, defaults['XSSAuditingEnabled'])
        opt.setAttribute(QWebSettings.LocalContentCanAccessRemoteUrls, defaults['localToRemoteUrlAccessEnabled'])

        if 'userAgent' in defaults:
            self.m_webPage.m_userAgent = defaults['userAgent']

        if 'userName' in defaults:
            self.m_networkAccessManager.m_userName = defaults['userName']

        if 'password' in defaults:
            self.m_networkAccessManager.m_password = defaults['password']

    def finish(self, ok):
        status = 'success' if ok else 'fail'
        self.loadFinished.emit(status)

    def mainFrame(self):
        return self.m_mainFrame

    def renderImage(self):
        contentsSize = self.m_mainFrame.contentsSize()
        contentsSize -= QSize(self.m_scrollPosition.x(), self.m_scrollPosition.y())
        frameRect = QRect(QPoint(0, 0), contentsSize)
        if not self.m_clipRect.isEmpty():
            frameRect = self.m_clipRect

        viewportSize = self.m_webPage.viewportSize()
        self.m_webPage.setViewportSize(contentsSize)

        image = QImage(frameRect.size(), QImage.Format_ARGB32)
        image.fill(qRgba(255, 255, 255, 0))

        painter = QPainter()

        # We use tiling approach to work-around Qt software rasterizer bug
        # when dealing with very large paint device.
        # See http://code.google.com/p/phantomjs/issues/detail?id=54.
        tileSize = 4096
        htiles = (image.width() + tileSize - 1) / tileSize
        vtiles = (image.height() + tileSize - 1) / tileSize
        for x in range(htiles):
            for y in range(vtiles):
                tileBuffer = QImage(tileSize, tileSize, QImage.Format_ARGB32)
                tileBuffer.fill(qRgba(255, 255, 255, 0))

                # Render the web page onto the small tile first
                painter.begin(tileBuffer)
                painter.setRenderHint(QPainter.Antialiasing, True)
                painter.setRenderHint(QPainter.TextAntialiasing, True)
                painter.setRenderHint(QPainter.SmoothPixmapTransform, True)
                painter.translate(-frameRect.left(), -frameRect.top())
                painter.translate(-x * tileSize, -y * tileSize)
                self.m_mainFrame.render(painter, QRegion(frameRect))
                painter.end()

                # Copy the tile to the main buffer
                painter.begin(image)
                painter.setCompositionMode(QPainter.CompositionMode_Source)
                painter.drawImage(x * tileSize, y * tileSize, tileBuffer)
                painter.end()

        self.m_webPage.setViewportSize(viewportSize)
        return image

    # Different defaults.
    # OSX: 72, X11: 75(?), Windows: 96
    pdf_dpi = 72
    def renderPdf(self, fileName):
        p = QPrinter()
        p.setOutputFormat(QPrinter.PdfFormat)
        p.setOutputFileName(fileName)
        p.setResolution(self.pdf_dpi)
        paperSize = self.m_paperSize

        if not len(paperSize):
            pageSize = QSize(self.m_webPage.mainFrame().contentsSize())
            paperSize['width'] = str(pageSize.width()) + 'px'
            paperSize['height'] = str(pageSize.height()) + 'px'
            paperSize['border'] = '0px'

        if paperSize.get('width') and paperSize.get('height'):
            sizePt = QSizeF(ceil(self.stringToPointSize(paperSize['width'])),
                            ceil(self.stringToPointSize(paperSize['height'])))
            p.setPaperSize(sizePt, QPrinter.Point)
        elif 'format' in paperSize:
            orientation = QPrinter.Landscape if paperSize.get('orientation') and paperSize['orientation'].lower() == 'landscape' else QPrinter.Portrait
            orientation = QPrinter.Orientation(orientation)
            p.setOrientation(orientation)

            formats = {
                'A0': QPrinter.A0,
                'A1': QPrinter.A1,
                'A2': QPrinter.A2,
                'A3': QPrinter.A3,
                'A4': QPrinter.A4,
                'A5': QPrinter.A5,
                'A6': QPrinter.A6,
                'A7': QPrinter.A7,
                'A8': QPrinter.A8,
                'A9': QPrinter.A9,
                'B0': QPrinter.B0,
                'B1': QPrinter.B1,
                'B2': QPrinter.B2,
                'B3': QPrinter.B3,
                'B4': QPrinter.B4,
                'B5': QPrinter.B5,
                'B6': QPrinter.B6,
                'B7': QPrinter.B7,
                'B8': QPrinter.B8,
                'B9': QPrinter.B9,
                'B10': QPrinter.B10,
                'C5E': QPrinter.C5E,
                'Comm10E': QPrinter.Comm10E,
                'DLE': QPrinter.DLE,
                'Executive': QPrinter.Executive,
                'Folio': QPrinter.Folio,
                'Ledger': QPrinter.Ledger,
                'Legal': QPrinter.Legal,
                'Letter': QPrinter.Letter,
                'Tabloid': QPrinter.Tabloid
            }

            p.setPaperSize(QPrinter.A4) # fallback
            for format_, size in formats.items():
                if format_.lower() == paperSize['format'].lower():
                    p.setPaperSize(size)
                    break
        else:
            return False

        border = floor(self.stringToPointSize(paperSize['border'])) if paperSize.get('border') else 0
        p.setPageMargins(border, border, border, border, QPrinter.Point)

        self.m_webPage.mainFrame().print_(p)
        return True

    def stringToPointSize(self, string):
        units = (
            ('mm', 72 / 25.4),
            ('cm', 72 / 2.54),
            ('in', 72.0),
            ('px', 72.0 / self.pdf_dpi / 2.54),
            ('', 72.0 / self.pdf_dpi / 2.54)
        )

        for unit, format_ in units:
            if string.endswith(unit):
                value = string.rstrip(unit)
                return float(value) * format_
        return 0

    def userAgent(self):
        return self.m_webPage.m_userAgent

    ##
    # Properties and methods exposed to JavaScript
    ##

    @pyqtSlot(str)
    def _appendScriptElement(self, scriptUrl):
        self.m_mainFrame.evaluateJavaScript('''
            var el = document.createElement('script');
            el.onload = function() { alert('%(scriptUrl)s'); };
            el.src = '%(scriptUrl)s';
            document.body.appendChild(el);
        ''' % {'scriptUrl': scriptUrl})

    @pyqtProperty('QVariantMap')
    def clipRect(self):
        clipRect = self.m_clipRect
        result = {
            'width': clipRect.width(),
            'height': clipRect.height(),
            'top': clipRect.top(),
            'left': clipRect.left()
        }
        return result

    @clipRect.setter
    def clipRect(self, size):
        sizes = {'width': 0, 'height': 0, 'top': 0, 'left': 0}
        for item in sizes:
            try:
                sizes[item] = int(size[item])
                if sizes[item] < 0:
                    if item not in ('top', 'left'):
                        sizes[item] = 0
            except (KeyError, ValueError):
                sizes[item] = self.clipRect[item]

        self.m_clipRect = QRect(sizes['left'], sizes['top'], sizes['width'], sizes['height'])

    @pyqtProperty(str)
    def content(self):
        return self.m_mainFrame.toHtml()

    @content.setter
    def content(self, content):
        self.m_mainFrame.setHtml(content)

    @pyqtSlot(str, result='QVariant')
    def evaluate(self, code):
        function = '(%s)()' % code
        return self.m_mainFrame.evaluateJavaScript(function)

    @pyqtSlot(str, result=bool)
    def injectJs(self, filePath):
        return injectJsInFrame(filePath, self.parent().m_scriptEncoding.encoding, self.m_libraryPath, self.m_mainFrame)

    @pyqtSlot(str, str, 'QVariantMap')
    @pyqtSlot(str, 'QVariantMap', 'QVariantMap')
    def openUrl(self, address, op, settings):
        operation = op
        body = QByteArray()

        self.applySettings(settings)
        self.m_webPage.triggerAction(QWebPage.Stop)

        if type(op) is dict:
            operation = op.get('operation')
            body = QByteArray(op.get('data', ''))

        if operation == '':
            operation = 'get'

        networkOp = QNetworkAccessManager.CustomOperation
        operation = operation.lower()
        if operation == 'get':
            networkOp = QNetworkAccessManager.GetOperation
        elif operation == 'head':
            networkOp = QNetworkAccessManager.HeadOperation
        elif operation == 'put':
            networkOp = QNetworkAccessManager.PutOperation
        elif operation == 'post':
            networkOp = QNetworkAccessManager.PostOperation
        elif operation == 'delete':
            networkOp = QNetworkAccessManager.DeleteOperation

        if networkOp == QNetworkAccessManager.CustomOperation:
            self.m_mainFrame.evaluateJavaScript('console.error("Unknown network operation: %s");' % operation)
            return

        if address.lower() == 'about:blank':
            self.m_mainFrame.setHtml(self.blankHtml)
        else:
            self.m_mainFrame.load(QNetworkRequest(QUrl(address)), networkOp, body)

    @pyqtProperty('QVariantMap')
    def paperSize(self):
        return self.m_paperSize

    @paperSize.setter
    def paperSize(self, size):
        self.m_paperSize = size

    @pyqtSlot()
    def release(self):
        self.parent().m_pages.remove(self)
        sip.delete(self)

    @pyqtSlot(str, result=bool)
    def render(self, fileName):
        if self.m_mainFrame.contentsSize() == '':
            return False

        fileInfo = QFileInfo(fileName)
        path = QDir()
        path.mkpath(fileInfo.absolutePath())

        if fileName.lower().endswith('.pdf'):
            return self.renderPdf(fileName)

        image = self.renderImage()

        return image.save(fileName)

    @pyqtProperty(str)
    def libraryPath(self):
        return self.m_libraryPath

    @libraryPath.setter
    def libraryPath(self, dirPath):
        self.m_libraryPath = dirPath

    @pyqtSlot(str, 'QVariant', 'QVariant')
    def sendEvent(self, type_, arg1, arg2):
        type_ = type_.lower()

        if type_ in ('mousedown', 'mouseup', 'mousemove'):
            eventType = QMouseEvent.Type(QEvent.None)
            button = Qt.MouseButton(Qt.LeftButton)
            buttons = Qt.MouseButtons(Qt.LeftButton)

            if type_ == 'mousedown':
                eventType = QEvent.MouseButtonPress
            elif type_ == 'mouseup':
                eventType = QEvent.MouseButtonRelease
            elif type_ == 'mousemove':
                eventType = QEvent.MouseMove
                button = buttons = Qt.NoButton

            assert eventType != QEvent.None

            event = QMouseEvent(eventType, QPoint(arg1, arg2), button, buttons, Qt.NoModifier)
            QApplication.postEvent(self.m_webPage, event)
            QApplication.processEvents()

            return

        if type_ == 'click':
            self.sendEvent('mousedown', arg1, arg2)
            self.sendEvent('mouseup', arg1, arg2)

    @pyqtProperty('QVariantMap')
    def scrollPosition(self):
        scroll = self.m_scrollPosition
        result = {
            'left': scroll.x(),
            'top': scroll.y()
        }
        return result

    @scrollPosition.setter
    def scrollPosition(self, size):
        positions = {'left': 0, 'top': 0}
        for item in positions:
            try:
                positions[item] = int(size[item])
                if positions[item] < 0:
                    positions[item] = 0
            except (KeyError, ValueError):
                positions[item] = self.scrollPosition[item]
        self.m_scrollPosition = QPoint(positions['left'], positions['top'])
        self.m_mainFrame.setScrollPosition(self.m_scrollPosition)

    @pyqtSlot(str, str)
    def uploadFile(self, selector, fileName):
        el = self.m_mainFrame.findFirstElement(selector)
        if el.isNull():
            return

        self.m_webPage.m_uploadFile = fileName
        el.evaluateJavaScript('''
            (function (el) {
                var ev = document.createEvent('MouseEvents');
                ev.initEvent('click', true, true);
                el.dispatchEvent(ev);
            })(this)
        ''')

    @pyqtProperty('QVariantMap')
    def viewportSize(self):
        size = self.m_webPage.viewportSize()
        result = {
            'width': size.width(),
            'height': size.height()
        }
        return result

    @viewportSize.setter
    def viewportSize(self, size):
        sizes = {'width': 0, 'height': 0}
        for item in sizes:
            try:
                sizes[item] = int(size[item])
                if sizes[item] < 0:
                    sizes[item] = 0
            except (KeyError, ValueError):
                sizes[item] = self.viewportSize[item]

        self.m_webPage.setViewportSize(QSize(sizes['width'], sizes['height']))

    do_action('WebPage')
