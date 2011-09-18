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
from collections import defaultdict
from glob import glob


hooks = defaultdict(dict)


def _checkHookExists(create=False):
    '''Decorator that will raise LookupError or create hook if hook doesn't exist'''
    def outterWrap(func):
        def innerWrap(*args, **kwargs):
            if args[0] not in hooks:
                if create:
                    hooks[args[0]]['count'] = 0
                    hooks[args[0]]['plugins'] = []
                else:
                    raise LookupError("Hook '%s' was not found" % args[0])
            return func(*args, **kwargs)
        return innerWrap
    return outterWrap


@_checkHookExists(True)
def add_action(hook, priority=10):
    '''Decorator to be used for registering a function to
       a specific hook. Functions with lower priority are
       executed first (priority of 1 for example).
    '''
    def register(func):
        hooks[hook]['plugins'].append((priority, func))
        return func
    return register


@_checkHookExists()
def did_action(hook):
    '''Find out how many times a hook was fired'''
    return hooks[hook]['count']


@_checkHookExists(True)
def do_action(hook, *args, **kwargs):
    '''Trigger a hook. It will run any functions that have registered
       themselves to the hook. Any additional arguments or keyword
       arguments you pass in will be passed to the functions.
    '''
    # sort plugins by priority
    hooks[hook]['plugins'].sort()
    for plugin in hooks[hook]['plugins']:
        hooks[hook]['count'] += 1
        plugin[1](*args, **kwargs)


def get(name, depth=4, scope='local'):
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

    return getattr(sys._getframe(depth), scope)[name]


def has_action(hook, func=None):
    '''Check if hook exists. If function is specified,
       check if function has been registered for hook.
    '''
    if func is None:
        if hook in hooks:
            return True
    else:
        if hook not in hooks:
            raise LookupError("Hook '%s' was not found" % hook)

        for plugin in hooks[hook]['plugins']:
            if plugin[1] == func:
                return True
    return False


def remove_action(hook, func=None, priority=10):
    '''Remove hook if hook exists. If function is specified,
       remove function from hook. If priority is not same as
       functions priority, it will not remove the function.
    '''
    if func is None:
        if hook in hooks:
            del hooks[hook]
            return True
    else:
        if hook not in hooks:
            raise LookupError("Hook '%s' was not found" % hook)

        for i, plugin in enumerate(hooks[hook]['plugins']):
            if plugin[1] == func and plugin[0] == priority:
                del hooks[hook]['plugins'][i]
                return True
    return False


@_checkHookExists()
def remove_all_actions(hook, priority=None):
    '''Remove all functions that have been registered to hook.
       If priority is used, remove all actions from that priority
       instead.
    '''
    if priority is None:
        del hooks[hook][:]
    else:
        indexes = []
        for i, plugin in enumerate(hooks[hook]['plugins']):
            if plugin[0] == priority:
                indexes.append(i)
        for index in indexes:
            del hooks[hook]['plugins'][index]


def set_(name, value, depth=4, scope='local'):
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

        Plugins must be in folders under plugins/ (or plugins_path),
        and must also be the same name as the plugin folder.

        E.g. a plugin folder named my_plugin will
        have my_plugin.py inside the folder loaded.
    '''
    plugins_path = os.environ.get('PYPHANTOMJS_PLUGINS_PATH')

    if plugins_path is None:
        # path is different when frozen
        if hasattr(sys, 'frozen'):
            path = os.path.dirname(os.path.abspath(sys.executable))
        else:
            path = os.path.dirname(os.path.abspath(__file__))

        generateModuleName = lambda p: '.'.join(('plugins', p[0], p[1]))
        plugin_list = glob(os.path.join(path, 'plugins/*/*.py'))
    else:
        # make sure it's an absolute path
        plugins_path = os.path.abspath(plugins_path)
        # append directory for module loading
        sys.path[1:1] = plugins_path,

        generateModuleName = lambda p: '.'.join((p[0], p[1]))
        plugin_list = glob(os.path.join(plugins_path, '*/*.py'))

    # now convert list to [('plugin_folder', 'file'), ...]
    plugin_list = [(os.path.split(os.path.dirname(f))[1], os.path.splitext(os.path.split(f)[1])[0]) for f in plugin_list]

    # initialize plugins
    for plugin in plugin_list:
        if plugin[0] == plugin[1]:
            moduleName = generateModuleName(plugin)
            __import__(moduleName, globals(), locals(), [], -1)
