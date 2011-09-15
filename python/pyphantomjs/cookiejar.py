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

from PyQt4.QtCore import QSettings
from PyQt4.QtNetwork import QNetworkCookie, QNetworkCookieJar


class CookieJar(QNetworkCookieJar):
    def __init__(self, parent, cookiesFile):
        super(CookieJar, self).__init__(parent)

        self.m_cookiesFile = cookiesFile

    def setCookiesFromUrl(self, cookieList, url):
        settings = QSettings(self.m_cookiesFile, QSettings.IniFormat)

        settings.beginGroup(url.host())

        for cookie in cookieList:
            settings.setValue(cookie.name(), cookie.value())

        settings.sync()

        return True

    def cookiesForUrl(self, url):
        settings = QSettings(self.m_cookiesFile, QSettings.IniFormat)
        cookieList = []

        settings.beginGroup(url.host())

        for cname in settings.childKeys():
            cookieList.append(QNetworkCookie(cname, settings.value(cname)))

        return cookieList
