# Copyright (C) 2011 Google Inc. All rights reserved.
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

from webkitpy.common.config.ports import MacPort, MacWK2Port
from webkitpy.tool.mocktool import MockOptions, MockTool
from webkitpy.tool.steps.update import Update


class UpdateTest(unittest.TestCase):

    def test_update_command_non_interactive(self):
        tool = MockTool()
        options = MockOptions(non_interactive=True)
        step = Update(tool, options)
        self.assertEqual(["mock-update-webkit"], step._update_command())

        tool._deprecated_port = MacPort()
        self.assertEqual(["Tools/Scripts/update-webkit"], step._update_command())

        tool._deprecated_port = MacWK2Port()
        self.assertEqual(["Tools/Scripts/update-webkit"], step._update_command())

    def test_update_command_interactive(self):
        tool = MockTool()
        options = MockOptions(non_interactive=False)
        step = Update(tool, options)
        self.assertEqual(["mock-update-webkit"], step._update_command())

        tool._deprecated_port = MacPort()
        self.assertEqual(["Tools/Scripts/update-webkit"], step._update_command())

        tool._deprecated_port = MacWK2Port()
        self.assertEqual(["Tools/Scripts/update-webkit"], step._update_command())
