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

import sys
# hack to import parent module(s)
sys.path = sys.path + ['..']

from utils import version

try:
    from cx_Freeze import setup, Executable
except ImportError:
    sys.exit('cx_Freeze must be installed to use this script')

if sys.platform.startswith('win'):
    try:
        from win32verstamp import stamp
    except ImportError:
        from time import sleep
        print '*** WARNING ***'
        print 'the script will be unable to create the version resource'
        print 'install pywin32 extensions if you want the file stamped'
        sleep(2)

exe = Executable(
      script = '../pyphantomjs.py',
      icon = '../resources/pyphantomjs-icon.ico'
)

setup(
    name = 'PyPhantomJS',
    version = version,
    description = 'Minimalistic, headless, WebKit-based, JavaScript-driven tool',
    executables = [exe]
)
