# Copyright (C) 2011 Google Inc. All rights reserved.
# Copyright (C) 2011 Code Aurora Forum. All rights reserved.
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

from webkitpy.tool.commands.commandtest import CommandsTest
from webkitpy.tool.commands.suggestnominations import SuggestNominations
from webkitpy.tool.mocktool import MockOptions, MockTool


class SuggestNominationsTest(CommandsTest):

    mock_git_output = """commit 60831dde5beb22f35aef305a87fca7b5f284c698
Author: fpizlo@apple.com <fpizlo@apple.com@268f45cc-cd09-0410-ab3c-d52691b4dbfc>
Date:   2011-09-15 19:56:21 +0000

    Value profiles collect no information for global variables
    https://bugs.webkit.org/show_bug.cgi?id=68143

    Reviewed by Geoffrey Garen.

    git-svn-id: http://svn.webkit.org/repository/webkit/trunk@95219 268f45cc-cd09-0410-ab3c-d52691b4dbfc
"""
    mock_same_author_commit_message = """Author: fpizlo@apple.com <fpizlo@apple.com@268f45cc-cd09-0410-ab3c-d52691b4dbfc>
Date:   2011-09-15 19:56:21 +0000

Value profiles collect no information for global variables
https://bugs.webkit.org/show_bug.cgi?id=68143

Reviewed by Geoffrey Garen.

git-svn-id: http://svn.webkit.org/repository/webkit/trunk@95219 268f45cc-cd09-0410-ab3c-d52691b4dbfc
"""

    def _make_options(self, **kwargs):
        defaults = {
            'committer_minimum': 10,
            'max_commit_age': 9,
            'reviewer_minimum': 80,
            'show_commits': False,
            'verbose': False,
        }
        options = MockOptions(**defaults)
        options.update(**kwargs)
        return options

    def test_recent_commit_messages(self):
        tool = MockTool()
        suggest_nominations = SuggestNominations()
        suggest_nominations._init_options(options=self._make_options())
        suggest_nominations.bind_to_tool(tool)

        tool.executive.run_command = lambda command: self.mock_git_output
        self.assertEqual(list(suggest_nominations._recent_commit_messages()), [self.mock_same_author_commit_message])

    mock_non_committer_commit_message = """
Author: commit-queue@webkit.org <commit-queue@webkit.org@268f45cc-cd09-0410-ab3c-d52691b4dbfc>
Date:   2009-09-15 14:08:42 +0000

Let TestWebKitAPI work for chromium
https://bugs.webkit.org/show_bug.cgi?id=67756

Patch by Xianzhu Wang <wangxianzhu@chromium.org> on 2011-09-15
Reviewed by Sam Weinig.

Source/WebKit/chromium:

* WebKit.gyp:

git-svn-id: http://svn.webkit.org/repository/webkit/trunk@95188 268f45cc-cd09-0410-ab3c-d52691b4dbfc
"""

    def test_basic(self):
        expected_stdout = "REVIEWER: Xianzhu Wang (wangxianzhu@chromium.org) has 88 reviewed patches\n"
        options = self._make_options()
        suggest_nominations = SuggestNominations()
        suggest_nominations._init_options(options=options)
        suggest_nominations._recent_commit_messages = lambda: [self.mock_non_committer_commit_message for _ in range(88)]
        self.assert_execute_outputs(suggest_nominations, [], expected_stdout=expected_stdout, options=options)
