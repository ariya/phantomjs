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
from PyQt4.QtGui import QIcon, QApplication

# make keyboard interrupt quit program
import signal
signal.signal(signal.SIGINT, signal.SIG_DFL)

if __name__ == '__main__':
    # Handle all command-line options
    p = argParser()
    arg_data = p.parse_known_args(sys.argv[1:])
    args = arg_data[0]
    args.script_args = arg_data[1]

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

                # this is a bug workaround for argparse.
                # if you call parse_known_args, and you
                # have an --option script arg, the args
                # get jumbled up, and it's inconsistent
                #
                # we're just going to check for -- and
                # swap it all back to the right order
                if args.script_args:
                    for i in range(len(args.upload_file)):
                        if not args.upload_file[i].count('='):
                            # insert the arg after --option (make sure it's not None)
                            if args.script:
                                args.script_args.insert(1, args.script)
                            # insert value args before --option
                            if args.upload_file[i+1:]:
                                arg_buffer = args.upload_file[i+1:]
                                arg_buffer.reverse()
                                for val in arg_buffer:
                                    args.script_args.insert(0, val)
                            args.script = args.upload_file[i]
                            break
                else:
                    args.script = args.upload_file[i]
                    args.script_args = args.upload_file[i+1:]

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

    if not args.script:
        p.print_help()
        sys.exit(1)

    try:
        args.script = open(args.script)
    except IOError as (errno, stderr):
        qFatal(str(stderr) + ': \'%s\'' % args.script)

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
