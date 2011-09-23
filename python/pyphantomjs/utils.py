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

import argparse
import codecs
import os
import sys

from PyQt4.QtCore import (QByteArray, QDateTime, qDebug, QFile, Qt,
                          QtCriticalMsg, QtDebugMsg, QtFatalMsg,
                          QtWarningMsg)

from __init__ import __version__
from plugincontroller import do_action


license = '''
  PyPhantomJS Version %s

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
''' % __version__


def argParser():
    class YesOrNoAction(argparse.Action):
        '''Converts yes or no arguments to True/False respectively'''
        def __call__(self, parser, namespace, value, option_string=None):
            answer = False if value == 'no' else True
            setattr(namespace, self.dest, answer)

    parser = argparse.ArgumentParser(
        description='Minimalistic headless WebKit-based JavaScript-driven tool',
        usage='%(prog)s [options] script.[js|coffee] [script argument [script argument ...]]',
        formatter_class=argparse.RawTextHelpFormatter
    )

    parser.add_argument('script', metavar='script.[js|coffee]', nargs='?',
        help='The script to execute, and any args to pass to it'
    )
    parser.add_argument('-v', '--version',
        action='version', version=license,
        help="show this program's version and license"
    )

    program = parser.add_argument_group('program options')
    script = parser.add_argument_group('script options')
    debug = parser.add_argument_group('debug options')

    program.add_argument('--config', metavar='/path/to/config',
        help='Specifies path to a JSON-formatted config file'
    )
    program.add_argument('--disk-cache', default=False, action=YesOrNoAction,
        choices=['yes', 'no'],
        help='Enable disk cache (default: no)'
    )
    program.add_argument('--ignore-ssl-errors', default=False, action=YesOrNoAction,
        choices=['yes', 'no'],
        help='Ignore SSL errors (default: no)'
    )
    program.add_argument('--max-disk-cache-size', default=-1, metavar='size', type=int,
        help='Limits the size of disk cache (in KB)'
    )
    program.add_argument('--output-encoding', default='System', metavar='encoding',
        help='Sets the encoding used for terminal output (default: %(default)s)'
    )
    program.add_argument('--proxy', metavar='address:port',
        help='Set the network proxy'
    )
    program.add_argument('--script-encoding', default='utf-8', metavar='encoding',
        help='Sets the encoding used for scripts (default: %(default)s)'
    )

    script.add_argument('--cookies-file', metavar='/path/to/cookies.txt',
        help='Sets the file name to store the persistent cookies'
    )
    script.add_argument('--load-images', default=True, action=YesOrNoAction,
        choices=['yes', 'no'],
        help='Load all inlined images (default: yes)'
    )
    script.add_argument('--load-plugins', default=False, action=YesOrNoAction,
        choices=['yes', 'no'],
        help='Load all plugins (i.e. Flash, Silverlight, ...) (default: no)'
    )
    script.add_argument('--local-to-remote-url-access', default=False, action=YesOrNoAction,
        choices=['yes', 'no'],
        help='Local content can access remote URL (default: no)'
    )

    debug.add_argument('--debug', choices=['exception', 'program'], metavar='option',
        help=('Debug the program with pdb\n'
              '    exception : Start debugger when program hits exception\n'
              '    program   : Start the program with the debugger enabled')
    )
    debug.add_argument('--verbose', action='store_true',
        help='Show verbose debug messages'
    )

    do_action('ArgParser')

    return parser


CSConverter = None
def coffee2js(script):
    global CSConverter
    if not CSConverter:
        from csconverter import CSConverter
    return CSConverter().convert(script)


def injectJsInFrame(filePath, scriptEncoding, libraryPath, targetFrame, startingScript=False):
    try:
        # if file doesn't exist in the CWD, use the lookup
        if not os.path.exists(filePath):
            filePath = os.path.join(libraryPath, filePath)

        try:
            with codecs.open(filePath, encoding=scriptEncoding) as f:
                script = f.read()
        except UnicodeDecodeError as e:
            sys.exit("%s in '%s'" % (e, filePath))

        if script.startswith('#!') and not filePath.lower().endswith('.coffee'):
            script = '//' + script

        if filePath.lower().endswith('.coffee'):
            result = coffee2js(script)
            if not result[0]:
                if startingScript:
                    sys.exit("%s: '%s'" % (result[1], filePath))
                else:
                    qDebug("%s: '%s'" % (result[1], filePath))
                    script = ''
            else:
                script = result[1]

        targetFrame.evaluateJavaScript(script)
        return True
    except IOError as (t, e):
        qDebug("%s: '%s'" % (e, filePath))
        return False


class MessageHandler(object):
    def __init__(self, verbose):
        self.verbose = verbose

    def process(self, msgType, msg):
        now = QDateTime.currentDateTime().toString(Qt.ISODate)

        if msgType == QtDebugMsg:
            if self.verbose:
                print '%s [DEBUG] %s' % (now, msg)
        elif msgType == QtWarningMsg:
            print >> sys.stderr, '%s [WARNING] %s' % (now, msg)
        elif msgType == QtCriticalMsg:
            print >> sys.stderr, '%s [CRITICAL] %s' % (now, msg)
        elif msgType == QtFatalMsg:
            print >> sys.stderr, '%s [FATAL] %s' % (now, msg)


class SafeStreamFilter(object):
    '''Convert string to something safe'''
    def __init__(self, target):
        self.target = target
        self.encoding = self.encode_to = self.encoding_sys = self.target.encoding or 'utf-8'
        self.errors = 'replace'

    def write(self, s):
        s = self.encode(s)
        self.target.write(s)

    def flush(self):
        self.target.flush()

    def encode(self, s):
        return s.encode(self.encode_to, self.errors)


class QPyFile(QFile):
    '''Simple subclass of QFile which supports the context manager

       It also wraps methods that require/return some foreign data type,
       such as QByteArray.
    '''
    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()

    def __init__(self, filename, mode='r'):
        super(QPyFile, self).__init__(filename)

        modeMap = {
            'r': QFile.ReadOnly,
            'r+': QFile.ReadOnly | QFile.WriteOnly,
            'w': QFile.WriteOnly | QFile.Truncate,
            'w+': QFile.WriteOnly | QFile.ReadOnly | QFile.Truncate,
            'a': QFile.Append,
            'a+': QFile.Append | QFile.ReadOnly
        }

        flags = QFile.NotOpen
        for key, flag in modeMap.items():
            if key in mode:
                flags = flags | flag

        if not self.open(flags):
            raise IOError("Could not open file: '%s'" % self.fileName())

    def peek(self, maxlen):
        return str(super(QPyFile, self).peek(maxlen))

    def readAll(self):
        return str(super(QPyFile, self).readAll())

    def write(self, data):
        return super(QPyFile, self).write(QByteArray(data))
