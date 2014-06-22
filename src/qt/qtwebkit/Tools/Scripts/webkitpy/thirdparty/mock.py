# mock.py
# Test tools for mocking and patching.
# Copyright (C) 2007-2009 Michael Foord
# E-mail: fuzzyman AT voidspace DOT org DOT uk

# mock 0.6.0
# http://www.voidspace.org.uk/python/mock/

# Released subject to the BSD License
# Please see http://www.voidspace.org.uk/python/license.shtml

# 2009-11-25: Licence downloaded from above URL.
# BEGIN DOWNLOADED LICENSE
#
# Copyright (c) 2003-2009, Michael Foord
# All rights reserved.
# E-mail : fuzzyman AT voidspace DOT org DOT uk
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above
#       copyright notice, this list of conditions and the following
#       disclaimer in the documentation and/or other materials provided
#       with the distribution.
#
#     * Neither the name of Michael Foord nor the name of Voidspace
#       may be used to endorse or promote products derived from this
#       software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# END DOWNLOADED LICENSE

# Scripts maintained at http://www.voidspace.org.uk/python/index.shtml
# Comments, suggestions and bug reports welcome.


__all__ = (
    'Mock',
    'patch',
    'patch_object',
    'sentinel',
    'DEFAULT'
)

__version__ = '0.6.0'

class SentinelObject(object):
    def __init__(self, name):
        self.name = name

    def __repr__(self):
        return '<SentinelObject "%s">' % self.name


class Sentinel(object):
    def __init__(self):
        self._sentinels = {}

    def __getattr__(self, name):
        return self._sentinels.setdefault(name, SentinelObject(name))


sentinel = Sentinel()

DEFAULT = sentinel.DEFAULT

class OldStyleClass:
    pass
ClassType = type(OldStyleClass)

def _is_magic(name):
    return '__%s__' % name[2:-2] == name

def _copy(value):
    if type(value) in (dict, list, tuple, set):
        return type(value)(value)
    return value


class Mock(object):

    def __init__(self, spec=None, side_effect=None, return_value=DEFAULT,
                 name=None, parent=None, wraps=None):
        self._parent = parent
        self._name = name
        if spec is not None and not isinstance(spec, list):
            spec = [member for member in dir(spec) if not _is_magic(member)]

        self._methods = spec
        self._children = {}
        self._return_value = return_value
        self.side_effect = side_effect
        self._wraps = wraps

        self.reset_mock()


    def reset_mock(self):
        self.called = False
        self.call_args = None
        self.call_count = 0
        self.call_args_list = []
        self.method_calls = []
        for child in self._children.itervalues():
            child.reset_mock()
        if isinstance(self._return_value, Mock):
            self._return_value.reset_mock()


    def __get_return_value(self):
        if self._return_value is DEFAULT:
            self._return_value = Mock()
        return self._return_value

    def __set_return_value(self, value):
        self._return_value = value

    return_value = property(__get_return_value, __set_return_value)


    def __call__(self, *args, **kwargs):
        self.called = True
        self.call_count += 1
        self.call_args = (args, kwargs)
        self.call_args_list.append((args, kwargs))

        parent = self._parent
        name = self._name
        while parent is not None:
            parent.method_calls.append((name, args, kwargs))
            if parent._parent is None:
                break
            name = parent._name + '.' + name
            parent = parent._parent

        ret_val = DEFAULT
        if self.side_effect is not None:
            if (isinstance(self.side_effect, Exception) or
                isinstance(self.side_effect, (type, ClassType)) and
                issubclass(self.side_effect, Exception)):
                raise self.side_effect

            ret_val = self.side_effect(*args, **kwargs)
            if ret_val is DEFAULT:
                ret_val = self.return_value

        if self._wraps is not None and self._return_value is DEFAULT:
            return self._wraps(*args, **kwargs)
        if ret_val is DEFAULT:
            ret_val = self.return_value
        return ret_val


    def __getattr__(self, name):
        if self._methods is not None:
            if name not in self._methods:
                raise AttributeError("Mock object has no attribute '%s'" % name)
        elif _is_magic(name):
            raise AttributeError(name)

        if name not in self._children:
            wraps = None
            if self._wraps is not None:
                wraps = getattr(self._wraps, name)
            self._children[name] = Mock(parent=self, name=name, wraps=wraps)

        return self._children[name]


    def assert_called_with(self, *args, **kwargs):
        assert self.call_args == (args, kwargs), 'Expected: %s\nCalled with: %s' % ((args, kwargs), self.call_args)


def _dot_lookup(thing, comp, import_path):
    try:
        return getattr(thing, comp)
    except AttributeError:
        __import__(import_path)
        return getattr(thing, comp)


def _importer(target):
    components = target.split('.')
    import_path = components.pop(0)
    thing = __import__(import_path)

    for comp in components:
        import_path += ".%s" % comp
        thing = _dot_lookup(thing, comp, import_path)
    return thing


class _patch(object):
    def __init__(self, target, attribute, new, spec, create):
        self.target = target
        self.attribute = attribute
        self.new = new
        self.spec = spec
        self.create = create
        self.has_local = False


    def __call__(self, func):
        if hasattr(func, 'patchings'):
            func.patchings.append(self)
            return func

        def patched(*args, **keywargs):
            # don't use a with here (backwards compatability with 2.5)
            extra_args = []
            for patching in patched.patchings:
                arg = patching.__enter__()
                if patching.new is DEFAULT:
                    extra_args.append(arg)
            args += tuple(extra_args)
            try:
                return func(*args, **keywargs)
            finally:
                for patching in getattr(patched, 'patchings', []):
                    patching.__exit__()

        patched.patchings = [self]
        patched.__name__ = func.__name__
        patched.compat_co_firstlineno = getattr(func, "compat_co_firstlineno",
                                                func.func_code.co_firstlineno)
        return patched


    def get_original(self):
        target = self.target
        name = self.attribute
        create = self.create

        original = DEFAULT
        if _has_local_attr(target, name):
            try:
                original = target.__dict__[name]
            except AttributeError:
                # for instances of classes with slots, they have no __dict__
                original = getattr(target, name)
        elif not create and not hasattr(target, name):
            raise AttributeError("%s does not have the attribute %r" % (target, name))
        return original


    def __enter__(self):
        new, spec, = self.new, self.spec
        original = self.get_original()
        if new is DEFAULT:
            # XXXX what if original is DEFAULT - shouldn't use it as a spec
            inherit = False
            if spec == True:
                # set spec to the object we are replacing
                spec = original
                if isinstance(spec, (type, ClassType)):
                    inherit = True
            new = Mock(spec=spec)
            if inherit:
                new.return_value = Mock(spec=spec)
        self.temp_original = original
        setattr(self.target, self.attribute, new)
        return new


    def __exit__(self, *_):
        if self.temp_original is not DEFAULT:
            setattr(self.target, self.attribute, self.temp_original)
        else:
            delattr(self.target, self.attribute)
        del self.temp_original


def patch_object(target, attribute, new=DEFAULT, spec=None, create=False):
    return _patch(target, attribute, new, spec, create)


def patch(target, new=DEFAULT, spec=None, create=False):
    try:
        target, attribute = target.rsplit('.', 1)
    except (TypeError, ValueError):
        raise TypeError("Need a valid target to patch. You supplied: %r" % (target,))
    target = _importer(target)
    return _patch(target, attribute, new, spec, create)



def _has_local_attr(obj, name):
    try:
        return name in vars(obj)
    except TypeError:
        # objects without a __dict__
        return hasattr(obj, name)
