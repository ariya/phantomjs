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
sip.setapi('QString', 2)
sip.setapi('QVariant', 2)

import os, sys, resources
import codecs

from plugincontroller import Bunch, do_action
# load plugins if running script directly
if __name__ == '__main__':
    from plugincontroller import load_plugins
    load_plugins()

from phantom import Phantom
from utils import argParser, MessageHandler, version

from PyQt4.QtCore import qInstallMsgHandler
from PyQt4.QtGui import QIcon, QApplication

# make keyboard interrupt quit program
import signal
signal.signal(signal.SIGINT, signal.SIG_DFL)

# output unicode safe text
from utils import SafeStreamFilter
sys.stdout = SafeStreamFilter(sys.stdout)
sys.stderr = SafeStreamFilter(sys.stderr)

def parseArgs(args):
    # Handle all command-line options
    p = argParser()
    arg_data = p.parse_known_args(args)
    args = arg_data[0]
    args.script_args = arg_data[1]

    if args.upload_file:
        # process the tags
        item_buffer = {}
        for i in range(len(args.upload_file)):
            item = args.upload_file[i].split('=')
            if len(item) < 2 or not len(item[1]):
                # if buffer is empty, or tag has no
                # value 'tag=', print help and exit
                if not len(item_buffer) or \
                item[1:] and not item[1:][0]:
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

            # duplicate tag checking
            if item[0] in item_buffer:
                sys.exit('Multiple tags named \'%s\' were found' % item[0])

            item_buffer[item[0]] = item[1]

        # make sure files exist
        for tag in item_buffer:
            if not os.path.exists(item_buffer[tag]):
                sys.exit('No such file or directory: \'%s\'' % item_buffer[tag])
        args.upload_file = item_buffer

    if args.proxy:
        item = args.proxy.split(':')
        if len(item) < 2 or not len(item[1]):
            p.print_help()
            sys.exit(1)
        args.proxy = item

    do_action('ParseArgs', Bunch(locals()))

    if not args.script:
        p.print_help()
        sys.exit(1)

    try:
        args.script = codecs.open(args.script, encoding='utf-8')
    except IOError as (errno, stderr):
        sys.exit('%s: \'%s\'' % (stderr, args.script))

    return args

def main():
    args = parseArgs(sys.argv[1:])

    # register an alternative Message Handler
    messageHandler = MessageHandler(args.verbose)
    qInstallMsgHandler(messageHandler.process)

    app = QApplication(sys.argv)

    app.setWindowIcon(QIcon(':/resources/pyphantomjs-icon.png'))
    app.setApplicationName('PyPhantomJS')
    app.setOrganizationName('Umaclan Development')
    app.setOrganizationDomain('www.umaclan.com')
    app.setApplicationVersion(version)

    phantom = Phantom(args, app)

    do_action('Main', Bunch(locals()))

    phantom.execute()
    app.exec_()
    sys.exit(phantom.returnValue())

do_action('PyPhantomJS', Bunch(locals()))

if __name__ == '__main__':
    main()
