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

import os
import codecs

from utils import version_major, version_minor, version_patch
from plugincontroller import Bunch, do_action
from csconverter import CSConverter
from math import ceil, floor
from time import sleep as usleep
from webpage import WebPage
from networkaccessmanager import NetworkAccessManager

from PyQt4.QtCore import pyqtProperty, pyqtSlot, Qt, QObject, QRect, \
                         SLOT, QTimer, QUrl, QFileInfo, QDir, QSize, \
                         QSizeF, QTime, QEventLoop, qDebug
from PyQt4.QtGui import QPalette, QDesktopServices, qApp, QPrinter, \
                        QImage, QPainter, QRegion, QApplication, qRgba
from PyQt4.QtWebKit import QWebSettings, QWebPage
from PyQt4.QtNetwork import QNetworkProxy, QNetworkProxyFactory

# Different defaults.
# OSX: 72, X11: 75(?), Windows: 96
pdf_dpi = 72

class Phantom(QObject):
    def __init__(self, args, parent=None):
        QObject.__init__(self, parent)

        # variable declarations
        self.m_loadStatus = self.m_state = ''
        self.m_var = self.m_paperSize = self.m_loadScript_cache = {}
        self.m_verbose = args.verbose
        self.m_page = WebPage(self)
        self.m_clipRect = QRect()
        # setup the values from args
        self.m_script = args.script.read()
        self.m_scriptFile = args.script.name
        self.m_scriptDir = os.path.dirname(args.script.name) + '/'
        self.m_args = args.script_args
        self.m_upload_file = args.upload_file
        autoLoadImages = False if args.load_images == 'no' else True
        pluginsEnabled = True if args.load_plugins == 'yes' else False

        args.script.close()

        do_action('PhantomInitPre', Bunch(locals()))

        palette = self.m_page.palette()
        palette.setBrush(QPalette.Base, Qt.transparent)
        self.m_page.setPalette(palette)

        if not args.proxy:
            QNetworkProxyFactory.setUseSystemConfiguration(True)
        else:
            proxy = QNetworkProxy(QNetworkProxy.HttpProxy, args.proxy[0], int(args.proxy[1]))
            QNetworkProxy.setApplicationProxy(proxy)

        self.m_page.settings().setAttribute(QWebSettings.AutoLoadImages, autoLoadImages)
        self.m_page.settings().setAttribute(QWebSettings.PluginsEnabled, pluginsEnabled)
        self.m_page.settings().setAttribute(QWebSettings.FrameFlatteningEnabled, True)
        self.m_page.settings().setAttribute(QWebSettings.OfflineStorageDatabaseEnabled, True)
        self.m_page.settings().setAttribute(QWebSettings.LocalStorageEnabled, True)
        self.m_page.settings().setLocalStoragePath(QDesktopServices.storageLocation(QDesktopServices.DataLocation))
        self.m_page.settings().setOfflineStoragePath(QDesktopServices.storageLocation(QDesktopServices.DataLocation))

        # Ensure we have a document.body.
        self.m_page.mainFrame().setHtml('<html><body></body></html>')

        self.m_page.mainFrame().setScrollBarPolicy(Qt.Horizontal, Qt.ScrollBarAlwaysOff)
        self.m_page.mainFrame().setScrollBarPolicy(Qt.Vertical, Qt.ScrollBarAlwaysOff)

        m_netAccessMan = NetworkAccessManager(args.disk_cache, args.ignore_ssl_errors, self)
        self.m_page.setNetworkAccessManager(m_netAccessMan)

        # inject our properties and slots into javascript
        self.m_page.mainFrame().javaScriptWindowObjectCleared.connect(self.inject)
        self.m_page.loadFinished.connect(self.finish)

        do_action('PhantomInitPost', Bunch(locals()))

    def execute(self):
        if self.m_script.startswith('#!'):
            self.m_script = '//' + self.m_script

        if self.m_scriptFile.lower().endswith('.coffee'):
            coffee = CSConverter(self)
            self.m_script = coffee.convert(self.m_script)

        self.m_page.mainFrame().evaluateJavaScript(self.m_script)

    def finish(self, success):
        self.m_loadStatus = 'success' if success else 'fail'
        self.m_page.mainFrame().evaluateJavaScript(self.m_script)

    def inject(self):
        self.m_page.mainFrame().addToJavaScriptWindowObject('phantom', self)

    def renderPdf(self, fileName):
        p = QPrinter()
        p.setOutputFormat(QPrinter.PdfFormat)
        p.setOutputFileName(fileName)
        p.setResolution(pdf_dpi)
        paperSize = self.m_paperSize

        if not len(paperSize):
            pageSize = QSize(self.m_page.mainFrame().contentsSize())
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
                'A3': QPrinter.A3,
                'A4': QPrinter.A4,
                'A5': QPrinter.A5,
                'Legal': QPrinter.Legal,
                'Letter': QPrinter.Letter,
                'Tabloid': QPrinter.Tabloid
            }

            p.setPaperSize(QPrinter.A4) # fallback
            for format, size in formats.items():
                if format.lower() == paperSize['format'].lower():
                    p.setPaperSize(size)
                    break
        else:
            return False

        border = floor(self.stringToPointSize(paperSize['border'])) if paperSize.get('border') else 0
        p.setPageMargins(border, border, border, border, QPrinter.Point)

        self.m_page.mainFrame().print_(p)
        return True

    def returnValue(self):
        return self.m_returnValue

    def stringToPointSize(self, string):
        units = (
            ('mm', 72 / 25.4),
            ('cm', 72 / 2.54),
            ('in', 72.0),
            ('px', 72.0 / pdf_dpi / 2.54),
            ('', 72.0 / pdf_dpi / 2.54)
        )

        for unit, format in units:
            if string.endswith(unit):
                value = string.rstrip(unit)
                return float(value) * format
        return 0

    ##
    # Properties and methods exposed to JavaScript
    ##

    @pyqtProperty('QStringList')
    def args(self):
        return self.m_args

    @pyqtProperty('QVariantMap')
    def clipRect(self):
        result = {
            'width': self.m_clipRect.width(),
            'height': self.m_clipRect.height(),
            'top': self.m_clipRect.top(),
            'left': self.m_clipRect.left()
        }
        return result

    @clipRect.setter
    def clipRect(self, size):
        names = ('width', 'height', 'top', 'left')
        for item in names:
            try:
                globals()[item] = int(size[item])
                if globals()[item] < 0:
                    if item not in ('top', 'left'):
                        globals()[item] = 0
            except KeyError:
                globals()[item] = getattr(self.m_clipRect, item)()

        self.m_clipRect = QRect(left, top, width, height)

    @pyqtProperty(str)
    def content(self):
        return self.m_page.mainFrame().toHtml()

    @content.setter
    def content(self, content):
        self.m_page.mainFrame().setHtml(content)

    @pyqtSlot()
    @pyqtSlot(int)
    def exit(self, code=0):
        self.m_returnValue = code
        self.m_page.loadFinished.disconnect(self.finish)
        QTimer.singleShot(0, qApp, SLOT('quit()'))

    @pyqtProperty(str)
    def loadStatus(self):
        return self.m_loadStatus

    @pyqtSlot(str, result=bool)
    def loadScript(self, script):
        if script in self.m_loadScript_cache:
            self.m_page.mainFrame().evaluateJavaScript(self.m_loadScript_cache[script])
            return True

        scriptFile = script
        try:
            script = codecs.open(self.m_scriptDir + script, encoding='utf-8')
            script = script.read()
        except IOError:
            return False

        if script.startswith('#!'):
            script = '//' + script

        if scriptFile.lower().endswith('.coffee'):
            coffee = CSConverter(self)
            script = coffee.convert(script)

        self.m_loadScript_cache[scriptFile] = script
        self.m_page.mainFrame().evaluateJavaScript(script)
        return True

    @pyqtSlot(str, name='open')
    def open_(self, address):
        qDebug('Opening address %s' % address)
        self.m_page.triggerAction(QWebPage.Stop)
        self.m_loadStatus = 'loading'
        self.m_page.mainFrame().setUrl(QUrl(address))

    @pyqtProperty('QVariantMap')
    def paperSize(self):
        return self.m_paperSize

    @paperSize.setter
    def paperSize(self, size):
        self.m_paperSize = size

    @pyqtSlot(str, result=bool)
    def render(self, fileName):
        fileInfo = QFileInfo(fileName)
        path = QDir()
        path.mkpath(fileInfo.absolutePath())

        if fileName.lower().endswith('.pdf'):
            return self.renderPdf(fileName)

        viewportSize = QSize(self.m_page.viewportSize())
        pageSize = QSize(self.m_page.mainFrame().contentsSize())

        bufferSize = QSize()
        if not self.m_clipRect.isEmpty():
            bufferSize = self.m_clipRect.size()
        else:
            bufferSize = self.m_page.mainFrame().contentsSize()

        if pageSize == '':
            return False

        image = QImage(bufferSize, QImage.Format_ARGB32)
        image.fill(qRgba(255, 255, 255, 0))
        p = QPainter(image)

        p.setRenderHint(QPainter.Antialiasing, True)
        p.setRenderHint(QPainter.TextAntialiasing, True)
        p.setRenderHint(QPainter.SmoothPixmapTransform, True)

        self.m_page.setViewportSize(pageSize)

        if not self.m_clipRect.isEmpty():
            p.translate(-self.m_clipRect.left(), -self.m_clipRect.top())
            self.m_page.mainFrame().render(p, QRegion(self.m_clipRect))
        else:
            self.m_page.mainFrame().render(p)

        p.end()
        self.m_page.setViewportSize(viewportSize)
        return image.save(fileName)

    @pyqtSlot('QWebElement', str)
    def setFormInputFile(self, el, fileTag):
        self.m_page.m_nextFileTag = fileTag
        el.evaluateJavaScript('''(function(target){
                              var evt = document.createEvent('MouseEvents');
                              evt.initMouseEvent("click", true, true, window,
                              0, 0, 0, 0, 0, false, false, false, false, 0, null);
                              target.dispatchEvent(evt);})(this);''')

    @pyqtSlot(int)
    def sleep(self, ms):
        startTime = QTime.currentTime()
        while True:
            QApplication.processEvents(QEventLoop.AllEvents, 25)
            if startTime.msecsTo(QTime.currentTime()) > ms:
                break
            usleep(0.005)

    @pyqtProperty(str)
    def state(self):
        return self.m_state

    @state.setter
    def state(self, value):
        self.m_state = value

    @pyqtProperty(str)
    def userAgent(self):
        return self.m_page.m_userAgent

    @userAgent.setter
    def userAgent(self, ua):
        self.m_page.m_userAgent = ua

    @pyqtSlot(str, result='QVariant')
    @pyqtSlot(int, result='QVariant')
    @pyqtSlot(str, 'QVariant')
    @pyqtSlot(int, 'QVariant')
    def ctx(self, name, value=None):
        if not value:
            return self.m_var.get(name)
        self.m_var[name] = value

    @pyqtProperty('QVariantMap')
    def version(self):
        version = {
            'major': version_major,
            'minor': version_minor,
            'patch': version_patch
        }
        return version

    @pyqtProperty('QVariantMap')
    def viewportSize(self):
        size = self.m_page.viewportSize()
        result = {
            'width': size.width(),
            'height': size.height()
        }
        return result

    @viewportSize.setter
    def viewportSize(self, size):
        names = ('width', 'height')
        for item in names:
            try:
                globals()[item] = int(size[item])
                if globals()[item] < 0:
                    globals()[item] = 0
            except KeyError:
                globals()[item] = getattr(self.m_page.viewportSize(), item)()

        self.m_page.setViewportSize(QSize(width, height))

    do_action('Phantom', Bunch(locals()))
