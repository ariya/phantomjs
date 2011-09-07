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

import os
import sys

# hack to import parent module(s)
parent_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path = [parent_dir] + sys.path

from __init__ import __version__

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


def qt4_plugins_dir():
    from PyQt4.QtCore import QCoreApplication
    app = QCoreApplication([])

    qt4_plugin_dirs = map(unicode, app.libraryPaths())
    if not qt4_plugin_dirs:
        return
    for d in qt4_plugin_dirs:
        if os.path.isdir(d):
            return str(d)  # must be 8-bit chars for one-file builds

qt4_plugin_dir = qt4_plugins_dir()
if qt4_plugin_dir is None:
    sys.exit('Cannot find PyQt4 plugins directory')


# modules to include
includes = [
    # to make sure images are supported; jpeg, gif, svg, etc.
    'PyQt4.QtSvg',
    'PyQt4.QtXml'
]

# files/directories to include
include_files = [
    # to make sure images are supported; jpeg, gif, svg, etc.
    (os.path.join(qt4_plugin_dir, 'imageformats'), 'imageformats'),
    (os.path.join(parent_dir, 'plugins'), 'plugins'),
    (os.path.join(parent_dir, '../../examples'), 'examples'),
    (os.path.join(parent_dir, '../LICENSE'), 'LICENSE.txt'),
    (os.path.join(parent_dir, '../README.md'), 'README.txt'),
    (os.path.join(parent_dir, '../../ChangeLog'), 'ChangeLog.txt')
]


exe = Executable(
      script = os.path.join(parent_dir, 'pyphantomjs.py'),
      icon = os.path.join(parent_dir, 'resources/pyphantomjs-icon.ico')
)


setup(
    name = 'PyPhantomJS',
    version = __version__,
    description = 'Minimalistic, headless, WebKit-based, JavaScript-driven tool',
    options = {'build_exe': {'includes': includes, 'include_files': include_files}},
    executables = [exe]
)
