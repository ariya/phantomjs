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
from glob import glob
from collections import defaultdict


plugins = defaultdict(list)
hook_count = {}


class Bunch(object):
    ''' Simple class to bunch a dict into
        an object that with attributes
    '''
    def __init__(self, adict):
        self.__dict__ = adict


def add_action(*hooks):
    ''' Decorator to be used for registering a function to
        a specific hook or list of hooks.
    '''
    def register(func):
        for hook in hooks:
            plugins[hook].append(func)
        return func
    return register


def did_action(hook):
    '''Find out how many times a hook was fired'''
    return hook_count[hook]


def do_action(hook, *args, **kwargs):
    ''' Trigger a hook. It will run any functions that have registered
        themselves to the hook. Any additional arguments or keyword
        arguments you pass in will be passed to the functions.
    '''
    hook_count[hook] = 0
    for plugin in plugins[hook]:
        hook_count[hook] += 1
        plugin(*args, **kwargs)


def has_action(hook, func=None):
    '''Check if hook exists. If function is specified,
       check if function has been registered for hook.
    '''
    if hook in plugins:
        if not func:
            return True
        else:
            for f in plugins[hook]:
                if f == func:
                    return True
    return False


def remove_action(hook, func=None):
    '''Remove hook if hook exists. If function is specified,
       remove function from hook.
    '''
    if hook in plugins:
        if not func:
            del plugins[hook]
            return True
        else:
            for f in plugins[hook]:
                if f == func:
                    del plugins[hook][plugins[hook].index(func)]
                    return True
    return False


def remove_all_actions(hook):
    '''Remove all functions that have been registered to hook'''
    if hook in plugins:
        del plugins[hook][:]
        return True
    return False


def load_plugins():
    ''' Loads the plugins.

        Plugins must be in folders under plugins/ ,
        and must also be the same name as the plugin folder.

        E.g. a plugin folder named plugins/my_plugin will
        have my_plugin.py inside the folder called.
    '''

    # path is different when frozen
    if hasattr(sys, 'frozen'):
        path = os.path.dirname(os.path.abspath(sys.executable))
    else:
        path = os.path.dirname(os.path.abspath(__file__))
    # get plugin list
    plugin_list = glob(os.path.join(path, 'plugins/*/*.py'))
    # now convert list to [('plugin_folder', 'file'), ...]
    plugin_list = [(os.path.split(os.path.dirname(f))[1], os.path.splitext(os.path.split(f)[1])[0]) for f in plugin_list]

    # initialize plugins
    for plugin in plugin_list:
        if plugin[0] == plugin[1]:
            __import__('plugins.%s.%s' % (plugin[0], plugin[1]), globals(), locals(), [], -1)
