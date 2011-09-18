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

from PyQt4.QtCore import QObject
from PyQt4.QtGui import QApplication
from PyQt4.QtWebKit import QWebPage

from utils import QPyFile


class CSConverter(QObject):
    _instance = None
    def __new__(cls, *args, **kwargs):
        if cls._instance is None:
            cls._instance = super(CSConverter, cls).__new__(cls, *args, **kwargs)
        return cls._instance

    def __init__(self):
        super(CSConverter, self).__init__(QApplication.instance())

        self.m_webPage = QWebPage(self)

        with QPyFile(':/resources/coffee-script.js') as f:
            script = f.readAll()

        self.m_webPage.mainFrame().evaluateJavaScript(script)
        self.m_webPage.mainFrame().addToJavaScriptWindowObject('converter', self)

    def convert(self, script):
        self.setProperty('source', script)
        result = self.m_webPage.mainFrame().evaluateJavaScript('''try {
                                                                      [true, this.CoffeeScript.compile(converter.source)];
                                                                  } catch (error) {
                                                                      [false, error.message];
                                                                  }
                                                               ''')
        return result
