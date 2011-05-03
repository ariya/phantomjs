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

from PyQt4.QtCore import QObject, QFile
from PyQt4.QtWebKit import QWebPage

class CSConverter(QObject):
    def __init__(self, parent=None):
        QObject.__init__(self, parent)
        self.m_webPage = QWebPage(self)

        converter = QFile(':/resources/coffee-script.js')
        converter.open(QFile.ReadOnly)

        script = str(converter.readAll())
        converter.close()
        self.m_webPage.mainFrame().evaluateJavaScript(script)
        self.m_webPage.mainFrame().addToJavaScriptWindowObject('converter', self)

    def convert(self, script):
        self.setProperty('source', script)
        result = self.m_webPage.mainFrame().evaluateJavaScript('this.CoffeeScript.compile(converter.source)')
        if len(result):
            return result
        return ''
