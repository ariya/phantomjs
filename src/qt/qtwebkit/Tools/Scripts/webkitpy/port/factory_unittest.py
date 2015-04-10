# Copyright (C) 2010 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#    * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#    * Neither the name of Google Inc. nor the names of its
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

import unittest2 as unittest

from webkitpy.tool.mocktool import MockOptions
from webkitpy.common.system.systemhost_mock import MockSystemHost

from webkitpy.port import factory
from webkitpy.port import gtk
from webkitpy.port import mac
from webkitpy.port import qt
from webkitpy.port import test
from webkitpy.port import win


class FactoryTest(unittest.TestCase):
    """Test that the factory creates the proper port object for given combination of port_name, host.platform, and options."""
    # FIXME: The ports themselves should expose what options they require,
    # instead of passing generic "options".

    def setUp(self):
        self.webkit_options = MockOptions(pixel_tests=False)

    def assert_port(self, port_name=None, os_name=None, os_version=None, options=None, cls=None):
        host = MockSystemHost(os_name=os_name, os_version=os_version)
        port = factory.PortFactory(host).get(port_name, options=options)
        self.assertIsInstance(port, cls)

    def test_mac(self):
        self.assert_port(port_name='mac-lion', cls=mac.MacPort)
        self.assert_port(port_name='mac-lion-wk2', cls=mac.MacPort)
        self.assert_port(port_name='mac', os_name='mac', os_version='lion', cls=mac.MacPort)
        self.assert_port(port_name=None,  os_name='mac', os_version='lion', cls=mac.MacPort)

    def test_win(self):
        self.assert_port(port_name='win-xp', cls=win.WinPort)
        self.assert_port(port_name='win-xp-wk2', cls=win.WinPort)
        self.assert_port(port_name='win', os_name='win', os_version='xp', cls=win.WinPort)
        self.assert_port(port_name=None, os_name='win', os_version='xp', cls=win.WinPort)
        self.assert_port(port_name=None, os_name='win', os_version='xp', options=self.webkit_options, cls=win.WinPort)

    def test_gtk(self):
        self.assert_port(port_name='gtk', cls=gtk.GtkPort)

    def test_qt(self):
        self.assert_port(port_name='qt', cls=qt.QtPort)

    def test_unknown_specified(self):
        self.assertRaises(NotImplementedError, factory.PortFactory(MockSystemHost()).get, port_name='unknown')

    def test_unknown_default(self):
        self.assertRaises(NotImplementedError, factory.PortFactory(MockSystemHost(os_name='vms')).get)

    def test_get_from_builder_name(self):
        self.assertEqual(factory.PortFactory(MockSystemHost()).get_from_builder_name('Apple Lion Release WK1 (Tests)').name(),
                          'mac-lion')
