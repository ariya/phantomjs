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

import sys

from PyQt4.QtCore import pyqtProperty, pyqtSlot, QObject, \
                         QFile
from PyQt4.QtGui import QApplication
from PyQt4.QtNetwork import QNetworkProxy, QNetworkProxyFactory

from utils import version_major, version_minor, version_patch
from plugincontroller import Bunch, do_action
from csconverter import CSConverter
from webpage import WebPage
from networkaccessmanager import NetworkAccessManager


class Phantom(QObject):
    def __init__(self, args, parent=None):
        QObject.__init__(self, parent)

        # variable declarations
        self.m_defaultPageSettings = {}
        self.m_verbose = args.verbose
        self.m_page = WebPage(self)
        self.m_returnValue = 0
        self.m_terminated = False
        # setup the values from args
        self.m_script = args.script
        self.m_scriptFile = args.script_name
        self.m_args = args.script_args

        do_action('PhantomInitPre', Bunch(locals()))

        if not args.proxy:
            QNetworkProxyFactory.setUseSystemConfiguration(True)
        else:
            proxy = QNetworkProxy(QNetworkProxy.HttpProxy, args.proxy[0], int(args.proxy[1]))
            QNetworkProxy.setApplicationProxy(proxy)

        # Provide WebPage with a non-standard Network Access Manager
        self.m_netAccessMan = NetworkAccessManager(args.disk_cache, args.ignore_ssl_errors, self)
        self.m_page.setNetworkAccessManager(self.m_netAccessMan)

        self.m_page.javaScriptConsoleMessageSent.connect(self.printConsoleMessage)

        self.m_defaultPageSettings['loadImages'] = args.load_images
        self.m_defaultPageSettings['loadPlugins'] = args.load_plugins
        self.m_defaultPageSettings['userAgent'] = self.m_page.userAgent()
        self.m_page.applySettings(self.m_defaultPageSettings)

        # inject our properties and slots into javascript
        self.m_page.mainFrame().addToJavaScriptWindowObject('phantom', self)

        bootstrap = QFile(':/bootstrap.js')
        if not bootstrap.open(QFile.ReadOnly):
            sys.exit('Can not bootstrap!')
        bootstrapper = str(bootstrap.readAll())
        bootstrap.close()
        if not bootstrapper:
            sys.exit('Can not bootstrap!')
        self.m_page.mainFrame().evaluateJavaScript(bootstrapper)

        do_action('PhantomInitPost', Bunch(locals()))

    def execute(self):
        if self.m_scriptFile.lower().endswith('.coffee'):
            coffee = CSConverter(self)
            self.m_script = coffee.convert(self.m_script)

        if self.m_script.startswith('#!'):
            self.m_script = '//' + self.m_script

        self.m_page.mainFrame().evaluateJavaScript(self.m_script)
        return not self.m_terminated

    def printConsoleMessage(self, msg):
        print msg

    def returnValue(self):
        return self.m_returnValue

    ##
    # Properties and methods exposed to JavaScript
    ##

    @pyqtProperty('QStringList')
    def args(self):
        return self.m_args

    @pyqtSlot(result=WebPage)
    def createWebPage(self):
        page = WebPage(self)
        page.applySettings(self.m_defaultPageSettings)
        page.setNetworkAccessManager(self.m_netAccessMan)
        return page

    @pyqtProperty('QVariantMap')
    def defaultPageSettings(self):
        return self.m_defaultPageSettings

    @pyqtSlot()
    @pyqtSlot(int)
    def exit(self, code=0):
        self.m_terminated = True
        self.m_returnValue = code
        QApplication.instance().exit(code)

    @pyqtProperty('QVariantMap')
    def version(self):
        version = {
            'major': version_major,
            'minor': version_minor,
            'patch': version_patch
        }
        return version

    do_action('Phantom', Bunch(locals()))
