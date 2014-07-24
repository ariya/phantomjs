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

import sys
import unittest2 as unittest

from webkitpy.thirdparty import AutoinstallImportHook


class ThirdpartyTest(unittest.TestCase):
    def test_import_hook(self):
        # Add another import hook and make sure we get called.
        class MockImportHook(AutoinstallImportHook):
            def __init__(self):
                AutoinstallImportHook.__init__(self)
                self.eliza_installed = False

            def _install_eliza(self):
                self.eliza_installed = True

        mock_import_hook = MockImportHook()
        try:
            # The actual AutoinstallImportHook should be installed before us,
            # so these modules will get installed before MockImportHook runs.
            sys.meta_path.append(mock_import_hook)
            # unused-variable, import failures - pylint: disable-msg=W0612,E0611,F0401
            from webkitpy.thirdparty.autoinstalled import eliza
            self.assertTrue(mock_import_hook.eliza_installed)

        finally:
            sys.meta_path.remove(mock_import_hook)

    def test_imports(self):
        # This method tests that we can actually import everything.
        # unused-variable, import failures - pylint: disable-msg=W0612,E0611,F0401
        import webkitpy.thirdparty.autoinstalled.buildbot
        import webkitpy.thirdparty.autoinstalled.coverage
        import webkitpy.thirdparty.autoinstalled.eliza
        import webkitpy.thirdparty.autoinstalled.irc.ircbot
        import webkitpy.thirdparty.autoinstalled.irc.irclib
        import webkitpy.thirdparty.autoinstalled.mechanize
        import webkitpy.thirdparty.autoinstalled.pylint
        import webkitpy.thirdparty.autoinstalled.webpagereplay
        import webkitpy.thirdparty.autoinstalled.pep8
