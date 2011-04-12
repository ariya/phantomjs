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

import os, sys, resources

from phantom import Phantom
from utils import argParser, MessageHandler, version

from PyQt4.QtCore import QString, qInstallMsgHandler, qFatal
from PyQt4.QtGui import *

# make keyboard interrupt quit program
import signal
signal.signal(signal.SIGINT, signal.SIG_DFL)

if __name__ == '__main__':
    # Handle all command-line options
    p = argParser()
    args = p.parse_args()

    # register an alternative Message Handler
    messageHandler = MessageHandler(args.verbose)
    qInstallMsgHandler(messageHandler.process)

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
                qFatal('No such file or directory: \'%s\'' % item_buffer[tag])
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
    except IOError as (errno, stderr):
        qFatal(str(stderr) + ': \'%s\'' % args.script[0])

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
