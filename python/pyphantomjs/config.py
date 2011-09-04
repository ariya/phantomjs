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
import codecs

from PyQt4.QtCore import QObject, QFile, qWarning
from PyQt4.QtWebKit import QWebPage


class Config(QObject):
    def __init__(self, parent, jsonFile):
        QObject.__init__(self, parent)

        with codecs.open(jsonFile, encoding='utf-8') as fd:
            json = fd.read()

        self.settings = {
            'cookies': { 'mapping': 'cookies', 'default': None },
            'diskCache': { 'mapping': 'disk_cache', 'default': False },
            'ignoreSslErrors': { 'mapping': 'ignore_ssl_errors', 'default': False },
            'loadImages': { 'mapping': 'load_images', 'default': True },
            'loadPlugins': { 'mapping': 'load_plugins', 'default': False },
            'localAccessRemote': { 'mapping': 'local_access_remote', 'default': False },
            'outputEncoding': { 'mapping': 'output_encoding', 'default': 'System' },
            'proxy': { 'mapping': 'proxy', 'default': None },
            'scriptEncoding': { 'mapping': 'script_encoding', 'default': 'utf-8' },
            'verbose': { 'mapping': 'verbose', 'default': False }
        }

        # generate dynamic properties
        for setting in self.settings:
            self.setProperty(setting, self.settings[setting]['default'])

        # now it's time to parse our JSON file
        if not json.lstrip().startswith('{') or not json.rstrip().endswith('}'):
            qWarning('Config file MUST be in JSON format!')
            return

        file_ = QFile(':/configurator.js')
        if not file_.open(QFile.ReadOnly):
            sys.exit('Unable to load JSON configurator!')
        configurator = str(file_.readAll())
        file_.close()
        if not configurator:
            sys.exit('Unable to set-up JSON configurator!')

        webPage = QWebPage(self)

        # add config object
        webPage.mainFrame().addToJavaScriptWindowObject('config', self)
        # apply settings
        webPage.mainFrame().evaluateJavaScript(configurator.replace('%1', json))
