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

import os
import sys
import unittest2 as unittest

from webkitpy.common.system.executive import Executive, ScriptError
from webkitpy.common.system.executive_mock import MockExecutive2
from webkitpy.common.system.filesystem import FileSystem
from webkitpy.common.system.filesystem_mock import MockFileSystem
from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.common.webkit_finder import WebKitFinder

import config


class ConfigTest(unittest.TestCase):
    def setUp(self):
        config.clear_cached_configuration()

    def tearDown(self):
        config.clear_cached_configuration()

    def make_config(self, output='', files=None, exit_code=0, exception=None, run_command_fn=None, stderr='', port_implementation=None):
        e = MockExecutive2(output=output, exit_code=exit_code, exception=exception, run_command_fn=run_command_fn, stderr=stderr)
        fs = MockFileSystem(files)
        return config.Config(e, fs, port_implementation=port_implementation)

    def assert_configuration(self, contents, expected):
        # This tests that a configuration file containing
        # _contents_ ends up being interpreted as _expected_.
        output = 'foo\nfoo/%s' % contents
        c = self.make_config(output, {'foo/Configuration': contents})
        self.assertEqual(c.default_configuration(), expected)

    def test_build_directory(self):
        # --top-level
        def mock_webkit_build_directory(arg_list):
            if arg_list == ['--top-level']:
                return '/WebKitBuild/'
            elif arg_list == ['--configuration', '--debug']:
                return '/WebKitBuild/Debug'
            elif arg_list == ['--configuration', '--release']:
                return '/WebKitBuild/Release'
            elif arg_list == []:
                return '/WebKitBuild/\n/WebKitBuild//Debug\n'
            return 'Error'

        def mock_run_command(arg_list):
            if 'webkit-build-directory' in arg_list[1]:
                return mock_webkit_build_directory(arg_list[2:])
            return 'Error'

        c = self.make_config(run_command_fn=mock_run_command)
        self.assertEqual(c.build_directory(None), '/WebKitBuild/')

        # Test again to check caching
        self.assertEqual(c.build_directory(None), '/WebKitBuild/')

        # Test other values
        self.assertTrue(c.build_directory('Release').endswith('/Release'))
        self.assertTrue(c.build_directory('Debug').endswith('/Debug'))
        self.assertRaises(KeyError, c.build_directory, 'Unknown')

        # Test that stderr output from webkit-build-directory won't mangle the build dir
        c = self.make_config(output='/WebKitBuild/', stderr="mock stderr output from webkit-build-directory")
        self.assertEqual(c.build_directory(None), '/WebKitBuild/')

    def test_build_directory_passes_port_implementation(self):
        def mock_run_command(arg_list):
            self.assetEquals('--gtk' in arg_list)
            return '/tmp'

        c = self.make_config(run_command_fn=mock_run_command, port_implementation='gtk')

    def test_default_configuration__release(self):
        self.assert_configuration('Release', 'Release')

    def test_default_configuration__debug(self):
        self.assert_configuration('Debug', 'Debug')

    def test_default_configuration__deployment(self):
        self.assert_configuration('Deployment', 'Release')

    def test_default_configuration__development(self):
        self.assert_configuration('Development', 'Debug')

    def test_default_configuration__notfound(self):
        # This tests what happens if the default configuration file doesn't exist.
        c = self.make_config(output='foo\nfoo/Release', files={'foo/Configuration': None})
        self.assertEqual(c.default_configuration(), "Release")

    def test_default_configuration__unknown(self):
        # Ignore the warning about an unknown configuration value.
        oc = OutputCapture()
        oc.capture_output()
        self.assert_configuration('Unknown', 'Unknown')
        oc.restore_output()

    def test_default_configuration__standalone(self):
        # FIXME: This test runs a standalone python script to test
        # reading the default configuration to work around any possible
        # caching / reset bugs. See https://bugs.webkit.org/show_bug.cgi?id=49360
        # for the motivation. We can remove this test when we remove the
        # global configuration cache in config.py.
        e = Executive()
        fs = FileSystem()
        c = config.Config(e, fs)
        script = WebKitFinder(fs).path_from_webkit_base('Tools', 'Scripts', 'webkitpy', 'port', 'config_standalone.py')

        # Note: don't use 'Release' here, since that's the normal default.
        expected = 'Debug'

        args = [sys.executable, script, '--mock', expected]
        actual = e.run_command(args).rstrip()
        self.assertEqual(actual, expected)

    def test_default_configuration__no_perl(self):
        # We need perl to run webkit-build-directory to find out where the
        # default configuration file is. See what happens if perl isn't
        # installed. (We should get the default value, 'Release').
        c = self.make_config(exception=OSError)
        actual = c.default_configuration()
        self.assertEqual(actual, 'Release')

    def test_default_configuration__scripterror(self):
        # We run webkit-build-directory to find out where the default
        # configuration file is. See what happens if that script fails.
        # (We should get the default value, 'Release').
        c = self.make_config(exception=ScriptError())
        actual = c.default_configuration()
        self.assertEqual(actual, 'Release')
