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
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
'''

# automatically convert Qt types by using api 2
import sip
for item in ('QDate', 'QDateTime', 'QString', 'QTextStream', 'QTime'
             'QUrl', 'QVariant'):
    sip.setapi(item, 2)

import os
import sys

from PyQt4.QtCore import qInstallMsgHandler
from PyQt4.QtGui import QIcon, QApplication

from plugincontroller import do_action
# load plugins if running script directly
if __name__ == '__main__':
    from plugincontroller import load_plugins
    load_plugins()

import resources
from __init__ import __version__
from phantom import Phantom
from utils import argParser, MessageHandler
from config import Config

# make keyboard interrupt quit program
import signal
signal.signal(signal.SIGINT, signal.SIG_DFL)

# output unicode safe text
from utils import SafeStreamFilter
sys.stdout = SafeStreamFilter(sys.stdout)
sys.stderr = SafeStreamFilter(sys.stderr)


def parseArgs(app, args):
    # Handle all command-line options
    p = argParser()
    arg_data = p.parse_known_args(args)
    args = arg_data[0]
    args.script_args = arg_data[1]

    args.disk_cache = False if args.disk_cache == 'no' else True
    args.ignore_ssl_errors = False if args.ignore_ssl_errors == 'no' else True
    args.load_images = True if args.load_images == 'yes' else False
    args.load_plugins = False if args.load_plugins == 'no' else True
    args.local_access_remote = False if args.local_access_remote == 'no' else True

    # register an alternative Message Handler
    messageHandler = MessageHandler(args.verbose)
    qInstallMsgHandler(messageHandler.process)

    file_check = (args.cookies, args.config)
    for file_ in file_check:
        if file_ is not None and not os.path.exists(file_):
            sys.exit("No such file or directory: '%s'" % file_)

    if args.config:
        config = Config(app, args.config)
        # apply settings
        for setting in config.settings:
            setattr(args, config.settings[setting]['mapping'], config.property(setting))

            # special case for verbose arg, which will need to be re-applied
            if setting == 'verbose':
                messageHandler.verbose = args.verbose

    split_check = (
        (args.proxy, 'proxy'),
        (args.auth, 'auth')
    )
    for arg, name in split_check:
        if arg:
            item = arg.split(':')
            if len(item) < 2 or not len(item[1]):
                p.print_help()
                sys.exit(1)
            setattr(args, name, item)

    do_action('ParseArgs')

    if args.script is None:
        p.print_help()
        sys.exit(1)

    if not os.path.exists(args.script):
        sys.exit("No such file or directory: '%s'" % args.script)

    return args


def main():
    app = QApplication(sys.argv)

    app.setWindowIcon(QIcon(':/resources/pyphantomjs-icon.png'))
    app.setApplicationName('PyPhantomJS')
    app.setOrganizationName('Umaclan Development')
    app.setOrganizationDomain('www.umaclan.com')
    app.setApplicationVersion(__version__)

    args = parseArgs(app, sys.argv[1:])

    phantom = Phantom(app, args)

    do_action('Main')

    if phantom.execute():
        app.exec_()
    return phantom.returnValue()


do_action('PyPhantomJS')


if __name__ == '__main__':
    sys.exit(main())
