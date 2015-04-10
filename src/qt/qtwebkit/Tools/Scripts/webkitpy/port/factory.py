# Copyright (C) 2010 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
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

"""Factory method to retrieve the appropriate port implementation."""

import fnmatch
import optparse
import re

from webkitpy.port import builders


def platform_options(use_globs=False):
    return [
        optparse.make_option('--platform', action='store',
            help=('Glob-style list of platform/ports to use (e.g., "mac*")' if use_globs else 'Platform to use (e.g., "mac-lion")')),
        optparse.make_option('--efl', action='store_const', dest='platform',
            const=('efl*' if use_globs else 'efl'),
            help=('Alias for --platform=efl*' if use_globs else 'Alias for --platform=efl')),
        optparse.make_option('--gtk', action='store_const', dest='platform',
            const=('gtk*' if use_globs else 'gtk'),
            help=('Alias for --platform=gtk*' if use_globs else 'Alias for --platform=gtk')),
        optparse.make_option('--qt', action='store_const', dest="platform",
            const=('qt*' if use_globs else 'qt'),
            help=('Alias for --platform=qt' if use_globs else 'Alias for --platform=qt')),
        ]


def configuration_options():
    return [
        optparse.make_option("-t", "--target", dest="configuration", help="(DEPRECATED)"),
        # FIXME: --help should display which configuration is default.
        optparse.make_option('--debug', action='store_const', const='Debug', dest="configuration",
            help='Set the configuration to Debug'),
        optparse.make_option('--release', action='store_const', const='Release', dest="configuration",
            help='Set the configuration to Release'),
        optparse.make_option('--32-bit', action='store_const', const='x86', default=None, dest="architecture",
            help='use 32-bit binaries by default (x86 instead of x86_64)'),
        ]



def _builder_options(builder_name):
    configuration = "Debug" if re.search(r"[d|D](ebu|b)g", builder_name) else "Release"
    is_webkit2 = builder_name.find("WK2") != -1
    builder_name = builder_name
    return optparse.Values({'builder_name': builder_name, 'configuration': configuration, 'webkit_test_runner': is_webkit2})


class PortFactory(object):
    PORT_CLASSES = (
        'efl.EflPort',
        'gtk.GtkPort',
        'mac.MacPort',
        'mock_drt.MockDRTPort',
        'qt.QtPort',
        'test.TestPort',
        'win.WinPort',
    )

    def __init__(self, host):
        self._host = host

    def _default_port(self, options):
        platform = self._host.platform
        if platform.is_linux() or platform.is_freebsd():
            return 'qt-linux'
        elif platform.is_mac():
            return 'mac'
        elif platform.is_win():
            return 'win'
        raise NotImplementedError('unknown platform: %s' % platform)

    def get(self, port_name=None, options=None, **kwargs):
        """Returns an object implementing the Port interface. If
        port_name is None, this routine attempts to guess at the most
        appropriate port on this platform."""
        port_name = port_name or self._default_port(options)

        for port_class in self.PORT_CLASSES:
            module_name, class_name = port_class.rsplit('.', 1)
            module = __import__(module_name, globals(), locals(), [], -1)
            cls = module.__dict__[class_name]
            if port_name.startswith(cls.port_name):
                port_name = cls.determine_full_port_name(self._host, options, port_name)
                return cls(self._host, port_name, options=options, **kwargs)
        raise NotImplementedError('unsupported platform: "%s"' % port_name)

    def all_port_names(self, platform=None):
        """Return a list of all valid, fully-specified, "real" port names.

        This is the list of directories that are used as actual baseline_paths()
        by real ports. This does not include any "fake" names like "test"
        or "mock-mac", and it does not include any directories that are not.

        If platform is not specified, we will glob-match all ports"""
        platform = platform or '*'
        return fnmatch.filter(builders.all_port_names(), platform)

    def get_from_builder_name(self, builder_name):
        port_name = builders.port_name_for_builder_name(builder_name)
        assert port_name, "unrecognized builder name '%s'" % builder_name
        return self.get(port_name, _builder_options(builder_name))
