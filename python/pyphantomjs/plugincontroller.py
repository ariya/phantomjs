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


hooks = defaultdict(dict)


def add_action(hook, priority=10):
    '''Decorator to be used for registering a function to
       a specific hook. Functions with lower priority are
       executed first (priority of 1 for example).
    '''
    def register(func):
        if hooks[hook].get('plugins'):
            hooks[hook]['plugins'].append((priority, func))
        else:
            hooks[hook]['plugins'] = [(priority, func)]
        return func
    return register


def did_action(hook):
    '''Find out how many times a hook was fired'''
    if not hooks[hook].get('count'):
        hooks[hook]['count'] = 0
    return hooks[hook]['count']


def do_action(hook, *args, **kwargs):
    '''Trigger a hook. It will run any functions that have registered
       themselves to the hook. Any additional arguments or keyword
       arguments you pass in will be passed to the functions.
    '''
    if not hooks[hook].get('count'):
        hooks[hook]['count'] = 0
    if hooks[hook].get('plugins'):
        # sort plugins by priority
        hooks[hook]['plugins'].sort()
        for plugin in hooks[hook]['plugins']:
            hooks[hook]['count'] += 1
            plugin[1](*args, **kwargs)


def get(name, depth=3, scope='local'):
    '''Gets the value of a variable in the parents namespace

    Args:
       depth controls how deep to go into the stack
       scope controls the namespace used; valid values are: local,global,builtin
    '''
    if scope == 'local':
        scope = 'f_locals'
    elif scope == 'global':
        scope = 'f_globals'
    elif scope == 'builtin':
        scope = 'f_builtins'

    return getattr(sys._getframe(depth), scope).get(name)


def has_action(hook, func=None):
    '''Check if hook exists. If function is specified,
       check if function has been registered for hook.
    '''
    if hook in hooks:
        if not func:
            return True
        else:
            if hooks[hook].get('plugins'):
                for plugin in hooks[hook]['plugins']:
                    if plugin[1] == func:
                        return True
    return False


def remove_action(hook, func=None, priority=10):
    '''Remove hook if hook exists. If function is specified,
       remove function from hook. If priority is not same as
       functions priority, it will not remove the function.
    '''
    if hook in hooks:
        if not func:
            del hooks[hook]
            return True
        else:
            if hooks[hook].get('plugins'):
                for i, plugin in enumerate(hooks[hook]['plugins']):
                    if plugin[1] == func and plugin[0] == priority:
                        del hooks[hook]['plugins'][i]
                        return True
    return False


def remove_all_actions(hook, priority=None):
    '''Remove all functions that have been registered to hook.
       If priority is used, remove all actions from that priority
       instead.
    '''
    if hook in hooks:
        if not priority:
            del hooks[hook][:]
        else:
            if hooks[hook].get('plugins'):
                indexes = []
                for i, plugin in enumerate(hooks[hook]['plugins']):
                    if plugin[0] == priority:
                        indexes.append(i)
                for index in indexes:
                    del hooks[hook]['plugins'][index]
        return True
    return False


def set(name, value, depth=3, scope='local'):
    '''Sets the value of a variable in the parents namespace

    Args:
       depth controls how deep to go into the stack
       scope controls the namespace used; valid values are: local,global,builtin
    '''
    if scope == 'local':
        scope = 'f_locals'
    elif scope == 'global':
        scope = 'f_globals'
    elif scope == 'builtin':
        scope = 'f_builtins'

    getattr(sys._getframe(depth), scope)[name] = value


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
