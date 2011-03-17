#!/usr/bin/env python 
'''
  This file is part of the PyPhantomJS project.

  Copyright (C) 2011 James Roe <roejames12@hotmail.com>
  Copyright (C) 2010-2011 Ariya Hidayat <ariya.hidayat@gmail.com>

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

import argparse, os, sys, resources

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtWebKit import *
from PyQt4.QtNetwork import QNetworkProxy, QNetworkProxyFactory

from csconverter import CSConverter

# make keyboard interrupt quit program
import signal
signal.signal(signal.SIGINT, signal.SIG_DFL)

version_major = 1
version_minor = 1
version_patch = 0
version = '%d.%d.%d' % (version_major, version_minor, version_patch)

def argParser():
    parser = argparse.ArgumentParser(
        description='Minimalistic headless WebKit-based JavaScript-driven tool',
        usage='%(prog)s [options] script.js [argument [argument ...]]',
        formatter_class=argparse.RawTextHelpFormatter
    )

    parser.add_argument('--load-images', default='yes',
        choices=['yes', 'no'],
        help='Load all inlined images (default: %(default)s)'
    )
    parser.add_argument('--load-plugins', default='no',
        choices=['yes', 'no'],
        help='Load all plugins (i.e. Flash, Silverlight, ...)\n(default: %(default)s)'
    )
    parser.add_argument('--proxy', metavar='address:port',
        help='Set the network proxy'
    )
    parser.add_argument('--upload-file', nargs='*',
        metavar='tag:file', help='Upload 1 or more files'
    )
    parser.add_argument('script', metavar='script.js', nargs='*',
        help='The script to execute, and any args to pass to it'
    )
    parser.add_argument('--version',
        action='version',
        help='show this program\'s version and license',
version='''
  PyPhantomJS Version %s

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
''' % version)
    return parser

class WebPage(QWebPage):
    def __init__(self, parent = None):
        QWebPage.__init__(self, parent)

        self.parent = parent
        self.m_nextFileTag = QString()

    def javaScriptAlert(self, webframe, msg):
        print 'JavaScript alert: %s' % msg

    def javaScriptConsoleMessage(self, message, lineNumber, sourceID):
        if sourceID:
            print sourceID + ':%s' % lineNumber + ' %s' % message.toUtf8()
        else:
            print message.toUtf8()

    def shouldInterruptJavaScript(self):
        QApplication.processEvents(QEventLoop.AllEvents, 42)
        return False

    def userAgentForUrl(self, url):
        if self.parent.m_userAgent:
            return self.parent.m_userAgent
        return QWebPage.userAgentForUrl(self, url)

    def chooseFile(self, webframe, suggestedFile):
        if self.m_nextFileTag in self.parent.m_upload_file:
            return self.parent.m_upload_file[self.m_nextFileTag]
        return QString()

class Phantom(QObject):
    def __init__(self, args, parent = None):
        QObject.__init__(self, parent)

        # variable declarations
        self.m_scriptFile = self.m_script = self.m_loadStatus = self.m_state = self.m_userAgent = QString()
        self.m_page = WebPage(self)
        self.m_var = self.m_loadScript_cache = {}
        # setup the values from args
        self.m_script = QString.fromUtf8(args.script[0].read())
        self.m_scriptFile = args.script[0].name
        self.m_args = args.script[1:]
        self.m_upload_file = args.upload_file
        autoLoadImages = False if args.load_images == 'no' else True
        pluginsEnabled = True if args.load_plugins == 'yes' else False

        args.script[0].close()

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

        # if our script was called in a different directory, change to it
        # to make any dealings with files be relative to the scripts directory
        if os.path.dirname(self.m_scriptFile):
            os.chdir(os.path.dirname(self.m_scriptFile))

        # inject our properties and slots into javascript
        self.setObjectName('phantom')
        self.connect(self.m_page.mainFrame(), SIGNAL('javaScriptWindowObjectCleared()'), self.inject)
        self.connect(self.m_page, SIGNAL('loadFinished(bool)'), self.finish)

    def execute(self):
        if self.m_script.startsWith('#!'):
            self.m_script.prepend('//')

        if self.m_scriptFile.endswith('.coffee'):
            coffee = CSConverter(self)
            self.m_script = coffee.convert(self.m_script)

        self.m_page.mainFrame().evaluateJavaScript(self.m_script)

    def finish(self, success):
        self.m_loadStatus = 'success' if success else 'fail'
        self.m_page.mainFrame().evaluateJavaScript(self.m_script)

    def inject(self):
        self.m_page.mainFrame().addToJavaScriptWindowObject('phantom', self)

    def returnValue(self):
        return self.m_returnValue

    ##
    # Properties and methods exposed to JavaScript
    ##

    @pyqtProperty('QStringList')
    def args(self):
        return self.m_args

    @pyqtProperty('QString')
    def content(self):
        return self.m_page.mainFrame().toHtml()

    @content.setter
    def content(self, content):
        self.m_page.mainFrame().setHtml(content)

    @pyqtSlot()
    @pyqtSlot(int)
    def exit(self, code = 0):
        self.m_returnValue = code
        self.disconnect(self.m_page, SIGNAL('loadFinished(bool)'), self.finish)
        QTimer.singleShot(0, qApp, SLOT('quit()'))

    @pyqtProperty(str)
    def loadStatus(self):
        return self.m_loadStatus

    @pyqtSlot(str, result=bool)
    def loadScript(self, script):
        if script in self.m_loadScript_cache:
            self.m_page.mainFrame().evaluateJavaScript(self.m_loadScript_cache[script])
            return True

        scriptFile = QString(script)
        try:
            script = open(script)
            script = QString.fromUtf8(script.read())
        except IOError:
            return False

        if script.startsWith('#!'):
            script.prepend('//')

        if scriptFile.endsWith('.coffee'):
            coffee = CSConverter(self)
            script = QString.fromUtf8(coffee.convert(script))

        self.m_loadScript_cache[scriptFile] = script
        self.m_page.mainFrame().evaluateJavaScript(script)
        return True

    @pyqtSlot(str, name='open')
    def open_(self, address):
        self.m_page.triggerAction(QWebPage.Stop)
        self.m_loadStatus = 'loading'
        self.m_page.mainFrame().setUrl(QUrl(address))

    @pyqtSlot(str)
    def render(self, fileName):
        fileInfo = QFileInfo(fileName)
        dir = QDir()
        dir.mkpath(fileInfo.absolutePath())

        if fileName.toLower().endsWith('.pdf'):
            p = QPrinter()
            p.setOutputFormat(QPrinter.PdfFormat)
            p.setOutputFileName(fileName)
            self.m_page.mainFrame().print_(p)
            return True

        viewportSize = QSize(self.m_page.viewportSize())
        pageSize = QSize(self.m_page.mainFrame().contentsSize())
        if pageSize == '':
            return False

        buffer = QImage(pageSize, QImage.Format_ARGB32_Premultiplied)
        buffer.fill(Qt.transparent)
        p = QPainter(buffer)
        p.setRenderHint(QPainter.Antialiasing, True)
        p.setRenderHint(QPainter.TextAntialiasing, True)
        p.setRenderHint(QPainter.SmoothPixmapTransform, True)
        self.m_page.setViewportSize(pageSize)
        self.m_page.mainFrame().render(p)
        p.end()
        self.m_page.setViewportSize(viewportSize)
        return buffer.save(fileName)

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

    @pyqtProperty(str)
    def state(self):
        return self.m_state

    @state.setter
    def state(self, value):
        self.m_state = value

    @pyqtProperty(str)
    def userAgent(self):
        return self.m_page.userAgentForUrl(self.m_page.mainFrame().url())

    @userAgent.setter
    def userAgent(self, ua):
        self.m_userAgent = ua

    @pyqtSlot(str, result='QVariant')
    @pyqtSlot(int, result='QVariant')
    @pyqtSlot(str, 'QVariant')
    @pyqtSlot(int, 'QVariant')
    def ctx(self, name, value = None):
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
        size = QSize(self.m_page.viewportSize())
        result = {
            'width': size.width(),
            'height': size.height()
        }
        return result

    @viewportSize.setter
    def viewportSize(self, size):
        w = int(size[QString('width')])
        h = int(size[QString('height')])
        if w > 0 and h > 0:
            self.m_page.setViewportSize(QSize(w, h))

if __name__ == '__main__':
    # Handle all command-line options
    p = argParser()
    args = p.parse_args()

    if args.upload_file:
        item_buffer = {}
        for i in range(len(args.upload_file)):
            item = args.upload_file[i].split('=')
            if len(item) < 2 or not len(item[1]):
                if len(item_buffer) == 0:
                    p.print_help()
                    sys.exit(1)
                args.script = args.upload_file[i:]
                break
            item_buffer[QString(item[0])] = QString(item[1])
        for tag in item_buffer:
            if not os.path.exists(item_buffer[tag]):
                print >> sys.stderr, '[Errno 2] No such file or directory: \'%s\'' % item_buffer[tag]
                sys.exit(1)
        args.upload_file = item_buffer

    if args.proxy:
        item = args.proxy.split(':')
        if len(item) < 2 or not len(item[1]):
            p.print_help()
            sys.exit(1)
        args.proxy = item

    if len(args.script) == 0:
        p.print_help()
        sys.exit(1)

    try:
        args.script[0] = open(args.script[0])
    except IOError as stderr:
        print >> sys.stderr, stderr
        sys.exit(1)

    app = QApplication(sys.argv)

    app.setWindowIcon(QIcon(':/resources/pyphantomjs-icon.png'))
    app.setApplicationName('PyPhantomJS')
    app.setOrganizationName('Umaclan Development')
    app.setOrganizationDomain('www.umaclan.com')
    app.setApplicationVersion(version)

    phantom = Phantom(args, app)
    phantom.execute()
    app.exec_()
    sys.exit(phantom.returnValue())
