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

import sys

from PyQt4.QtCore import (QByteArray, QDateTime, QFile, Qt,
                          QtCriticalMsg, QtDebugMsg, QtFatalMsg,
                          QtWarningMsg)


def debug(debug_type):
    def excepthook(type_, value, tb):
        import traceback

        # print the exception...
        traceback.print_exception(type_, value, tb)
        print
        # ...then start the debugger in post-mortem mode
        pdb.pm()

    # we are NOT in interactive mode
    if not hasattr(sys, 'ps1') or sys.stderr.target.isatty():
        import pdb

        from PyQt4.QtCore import pyqtRemoveInputHook
        pyqtRemoveInputHook()

        if debug_type == 'exception':
            sys.excepthook = excepthook
        elif debug_type == 'program':
            pdb.set_trace()


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
