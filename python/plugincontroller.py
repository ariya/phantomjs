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

'''
   How to make plugins.

   * Each class (except HookPluginMount) will be called at specific points
     in the programs code. Any plugins can register themselves automatically
     by subclassing a special class. globals() and locals() will be passed
     to each class instance so you can modify the contents of the local/global
     namespace.

   * For each plugin, "plugin_" is required in the filename before the plugin name.

   Here's an example:

   file: plugins/plugin_foo.py
   ---

   from plugincontroller import HookParseArgs

   class ModifyParseData(HookParseArgs):
       def __init__(self, _globals, _locals):
           self._globals = _globals
           self._locals = _locals

       def run(self):
           args = self._locals.args
           args.script_args.extend(['--my-injected-script-arg', '42'])
'''

class HookPluginMount(type):
    def __init__(cls, name, bases, attrs):
        if not hasattr(cls, 'plugins'):
            cls.plugins = []
        else:
            cls.plugins.append(cls)

##
# networkaccessmanager

class HookNetworkAccessManager(object):
    '''
       This will be called in the NetworkAccessManager class
    '''
    __metaclass__ = HookPluginMount

class HookNetworkAccessManagerInit(object):
    '''
       This will be called at the end of NetworkAccessManager's __init__()
       function
    '''
    __metaclass__ = HookPluginMount

class HookNetworkAccessManagerCreateRequestPre(object):
    '''
       This will be called at the end of NetworkAccessManager's createRequest()
       function, before the request is created
    '''
    __metaclass__ = HookPluginMount

class HookNetworkAccessManagerCreateRequestPost(object):
    '''
       This will be called at the end of NetworkAccessManager's createRequest()
       function, after the request is created
    '''
    __metaclass__ = HookPluginMount

class HookNetworkAccessManagerHandleFinished(object):
    '''
       This will be called in the middle of NetworkAccessManager's handleFinished()
       function
    '''
    __metaclass__ = HookPluginMount

##
# phantom

class HookPhantom(object):
    '''
       This will be called in the Phantom class
    '''
    __metaclass__ = HookPluginMount

class HookPhantomInitPre(object):
    '''
       This will be called right after Phantom's __init__()
       function's variable declations; before any attributes/stuff
       is changed/set
    '''
    __metaclass__ = HookPluginMount

class HookPhantomInitPost(object):
    '''
       This will be called at the end of Phantom's __init__()
       function; after everything has been changed/set
    '''
    __metaclass__ = HookPluginMount

##
# pyphantomjs

class HookPyPhantomJS(object):
    '''
       This will be called in PyPhantomJS module scope
    '''
    __metaclass__ = HookPluginMount

class HookParseArgs(object):
    '''
       This will be called at the (almost) bottom of the parseArgs() function
       in pyphantomjs.py
    '''
    __metaclass__ = HookPluginMount

class HookMain(object):
    '''
       This will be called after the variable declarations in the main() function,
       but before the application is actually created/started
    '''
    __metaclass__ = HookPluginMount

##
# utils

class HookArgParser(object):
    '''
       This will be called at the end of the argParser() function
       in utils.py
    '''
    __metaclass__ = HookPluginMount

##
# webpage

class HookWebPage(object):
    '''
       This will be called in the WebPage class
    '''
    __metaclass__ = HookPluginMount

class HookWebPageInit(object):
    '''
       This will be called at the end of WebPage's __init__() function
    '''
    __metaclass__ = HookPluginMount
