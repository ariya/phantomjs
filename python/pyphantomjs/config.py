'''
  This file is part of the PyPhantomJS project.

  Copyright (C) 2011 James Roe <roejames12@hotmail.com>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.
'''

import codecs
import sys

from PyQt4.QtCore import QObject, qWarning
from PyQt4.QtWebKit import QWebPage

from arguments import defaults
from plugincontroller import do_action
from utils import QPyFile


class Config(QObject):
    def __init__(self, parent, jsonFile):
        super(Config, self).__init__(parent)

        with codecs.open(jsonFile, encoding='utf-8') as f:
            json = f.read()

        self.settings = {
            'cookiesFile': { 'mapping': 'cookies_file', 'default': defaults['cookiesFile'] },
            'debug': { 'mapping': 'debug', 'default': defaults['debug'] },
            'diskCache': { 'mapping': 'disk_cache', 'default': defaults['diskCache'] },
            'ignoreSslErrors': { 'mapping': 'ignore_ssl_errors', 'default': defaults['ignoreSslErrors'] },
            'loadImages': { 'mapping': 'load_images', 'default': defaults['loadImages'] },
            'loadPlugins': { 'mapping': 'load_plugins', 'default': defaults['loadPlugins'] },
            'localToRemoteUrlAccessEnabled': { 'mapping': 'local_to_remote_url_access', 'default': defaults['localToRemoteUrlAccessEnabled'] },
            'maxDiskCacheSize': { 'mapping': 'max_disk_cache_size', 'default': defaults['maxDiskCacheSize'] },
            'outputEncoding': { 'mapping': 'output_encoding', 'default': defaults['outputEncoding'] },
            'proxy': { 'mapping': 'proxy', 'default': defaults['proxy'] },
            'proxyType': { 'mapping': 'proxy_type', 'default': defaults['proxyType'] },
            'scriptEncoding': { 'mapping': 'script_encoding', 'default': defaults['scriptEncoding'] },
            'verbose': { 'mapping': 'verbose', 'default': defaults['verbose'] }
        }

        do_action('ConfigInit', self.settings)

        # generate dynamic properties
        for setting in self.settings:
            self.setProperty(setting, self.settings[setting]['default'])

        # now it's time to parse our JSON file
        if not json.lstrip().startswith('{') or not json.rstrip().endswith('}'):
            qWarning('Config file MUST be in JSON format!')
            return

        webPage = QWebPage(self)

        with QPyFile(':/configurator.js') as f:
            # add config object
            webPage.mainFrame().addToJavaScriptWindowObject('config', self)
            # apply settings
            webPage.mainFrame().evaluateJavaScript(f.readAll().replace('%1', json))

    do_action('Config')
