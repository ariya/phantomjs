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

import argparse

from PyQt4.QtNetwork import QNetworkProxy

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
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.
''' % __version__


defaults = {
    'cookiesFile': None,
    'debug': None,
    'diskCache': False,
    'ignoreSslErrors': False,
    'loadImages': True,
    'loadPlugins': False,
    'localToRemoteUrlAccessEnabled': False,
    'maxDiskCacheSize': -1,
    'outputEncoding': 'System',
    'proxy': None,
    'proxyType': QNetworkProxy.HttpProxy,
    'scriptEncoding': 'utf-8',
    'verbose': False
}


def argParser():
    class YesOrNoAction(argparse.Action):
        '''Converts yes or no arguments to True/False respectively'''
        def __call__(self, parser, namespace, value, option_string=None):
            answer = True if value == 'yes' else False
            setattr(namespace, self.dest, answer)

    yesOrNo = lambda d: 'yes' if d else 'no'

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
    program.add_argument('--disk-cache', default=defaults['diskCache'], action=YesOrNoAction,
        choices=['yes', 'no'],
        help='Enable disk cache (default: %s)' % yesOrNo(defaults['diskCache'])
    )
    program.add_argument('--ignore-ssl-errors', default=defaults['ignoreSslErrors'], action=YesOrNoAction,
        choices=['yes', 'no'],
        help='Ignore SSL errors (default: %s)' % yesOrNo(defaults['ignoreSslErrors'])
    )
    program.add_argument('--max-disk-cache-size', default=defaults['maxDiskCacheSize'], metavar='size', type=int,
        help='Limits the size of disk cache (in KB)'
    )
    program.add_argument('--output-encoding', default=defaults['outputEncoding'], metavar='encoding',
        help='Sets the encoding used for terminal output (default: %(default)s)'
    )
    program.add_argument('--proxy', metavar='address:port',
        help='Set the network proxy'
    )
    program.add_argument('--proxy-type', default=defaults['proxyType'], metavar='type',
        help='Set the network proxy type (default: http)'
    )
    program.add_argument('--script-encoding', default=defaults['scriptEncoding'], metavar='encoding',
        help='Sets the encoding used for scripts (default: %(default)s)'
    )

    script.add_argument('--cookies-file', metavar='/path/to/cookies.txt',
        help='Sets the file name to store the persistent cookies'
    )
    script.add_argument('--load-images', default=defaults['loadImages'], action=YesOrNoAction,
        choices=['yes', 'no'],
        help='Load all inlined images (default: %s)' % yesOrNo(defaults['loadImages'])
    )
    script.add_argument('--load-plugins', default=defaults['loadPlugins'], action=YesOrNoAction,
        choices=['yes', 'no'],
        help='Load all plugins (i.e. Flash, Silverlight, ...) (default: %s)' % yesOrNo(defaults['loadPlugins'])
    )
    script.add_argument('--local-to-remote-url-access', default=defaults['localToRemoteUrlAccessEnabled'], action=YesOrNoAction,
        choices=['yes', 'no'],
        help='Local content can access remote URL (default: %s)' % yesOrNo(defaults['localToRemoteUrlAccessEnabled'])
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
