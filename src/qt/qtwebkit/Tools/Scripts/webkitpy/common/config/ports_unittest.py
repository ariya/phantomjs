# Copyright (c) 2009, Google Inc. All rights reserved.
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

import unittest2 as unittest

from webkitpy.common.config.ports import *


class DeprecatedPortTest(unittest.TestCase):
    def test_mac_port(self):
        self.assertEqual(MacPort().flag(), "--port=mac")
        self.assertEqual(MacPort().run_webkit_tests_command(), DeprecatedPort().script_shell_command("run-webkit-tests"))
        self.assertEqual(MacPort().build_webkit_command(), DeprecatedPort().script_shell_command("build-webkit"))
        self.assertEqual(MacPort().build_webkit_command(build_style="debug"), DeprecatedPort().script_shell_command("build-webkit") + ["--debug"])
        self.assertEqual(MacPort().build_webkit_command(build_style="release"), DeprecatedPort().script_shell_command("build-webkit") + ["--release"])

    def test_gtk_port(self):
        self.assertEqual(GtkPort().flag(), "--port=gtk")
        self.assertEqual(GtkPort().run_webkit_tests_command(), DeprecatedPort().script_shell_command("run-webkit-tests") + ["--gtk"])
        self.assertEqual(GtkPort().build_webkit_command(), DeprecatedPort().script_shell_command("build-webkit") + ["--gtk", "--update-gtk", "--no-webkit2", DeprecatedPort().makeArgs()])
        self.assertEqual(GtkPort().build_webkit_command(build_style="debug"), DeprecatedPort().script_shell_command("build-webkit") + ["--debug", "--gtk", "--update-gtk", "--no-webkit2", DeprecatedPort().makeArgs()])

    def test_gtk_wk2_port(self):
        self.assertEqual(GtkWK2Port().flag(), "--port=gtk-wk2")
        self.assertEqual(GtkWK2Port().run_webkit_tests_command(), DeprecatedPort().script_shell_command("run-webkit-tests") + ["--gtk", "-2"])
        self.assertEqual(GtkWK2Port().build_webkit_command(), DeprecatedPort().script_shell_command("build-webkit") + ["--gtk", "--update-gtk", "--no-webkit1", DeprecatedPort().makeArgs()])
        self.assertEqual(GtkWK2Port().build_webkit_command(build_style="debug"), DeprecatedPort().script_shell_command("build-webkit") + ["--debug", "--gtk", "--update-gtk", "--no-webkit1", DeprecatedPort().makeArgs()])

    def test_efl_port(self):
        self.assertEqual(EflPort().flag(), "--port=efl")
        self.assertEqual(EflPort().build_webkit_command(), DeprecatedPort().script_shell_command("build-webkit") + ["--efl", "--update-efl", "--no-webkit2", DeprecatedPort().makeArgs()])
        self.assertEqual(EflPort().build_webkit_command(build_style="debug"), DeprecatedPort().script_shell_command("build-webkit") + ["--debug", "--efl", "--update-efl", "--no-webkit2", DeprecatedPort().makeArgs()])

    def test_qt_port(self):
        self.assertEqual(QtPort().flag(), "--port=qt")
        self.assertEqual(QtPort().run_webkit_tests_command(), DeprecatedPort().script_shell_command("run-webkit-tests") + ["--qt"])
        self.assertEqual(QtPort().build_webkit_command(), DeprecatedPort().script_shell_command("build-webkit") + ["--qt", "--no-webkit2", DeprecatedPort().makeArgs()])
        self.assertEqual(QtPort().build_webkit_command(build_style="debug"), DeprecatedPort().script_shell_command("build-webkit") + ["--debug", "--qt", "--no-webkit2", DeprecatedPort().makeArgs()])

    def test_qt_wk2_port(self):
        self.assertEqual(QtWK2Port().flag(), "--port=qt-wk2")
        self.assertEqual(QtWK2Port().run_webkit_tests_command(), DeprecatedPort().script_shell_command("run-webkit-tests") + ["--qt", "-2"])
        self.assertEqual(QtWK2Port().build_webkit_command(), DeprecatedPort().script_shell_command("build-webkit") + ["--qt", DeprecatedPort().makeArgs()])
        self.assertEqual(QtWK2Port().build_webkit_command(build_style="debug"), DeprecatedPort().script_shell_command("build-webkit") + ["--debug", "--qt", DeprecatedPort().makeArgs()])
