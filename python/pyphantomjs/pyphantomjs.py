#!/usr/bin/env python
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

# automatically convert Qt types by using api 2
import sip
for item in ('QDate', 'QDateTime', 'QString', 'QTextStream', 'QTime'
             'QUrl', 'QVariant'):
    sip.setapi(item, 2)

import sys

from PyQt4.QtGui import QApplication, QIcon

from plugincontroller import do_action
# load plugins if running script directly
if __name__ == '__main__':
    from plugincontroller import load_plugins
    load_plugins()

import resources
from __init__ import __version__
from arguments import parseArgs
from phantom import Phantom

# make keyboard interrupt quit program
import signal
signal.signal(signal.SIGINT, signal.SIG_DFL)

# output unicode safe text
from utils import SafeStreamFilter
sys.stdout = SafeStreamFilter(sys.stdout)
sys.stderr = SafeStreamFilter(sys.stderr)


def main(arguments):
    app = QApplication([sys.argv[0]] + arguments)

    app.setWindowIcon(QIcon(':/resources/pyphantomjs-icon.png'))
    app.setApplicationName('PyPhantomJS')
    app.setOrganizationName('Umaclan Development')
    app.setOrganizationDomain('www.umaclan.com')
    app.setApplicationVersion(__version__)

    args = parseArgs(app, arguments)

    phantom = Phantom(app, args)

    do_action('Main')

    if phantom.execute():
        app.exec_()
    return phantom.returnValue()


do_action('PyPhantomJS')


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
