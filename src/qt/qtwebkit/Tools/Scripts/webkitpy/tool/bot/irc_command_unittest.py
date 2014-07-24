# Copyright (c) 2010 Google Inc. All rights reserved.
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

import os
import unittest2 as unittest

from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.tool.bot.irc_command import *
from webkitpy.tool.mocktool import MockTool
from webkitpy.common.net.web_mock import MockWeb
from webkitpy.common.system.executive_mock import MockExecutive
from webkitpy.common.system.filesystem_mock import MockFileSystem


class IRCCommandTest(unittest.TestCase):
    def test_whois(self):
        whois = Whois()
        self.assertEqual("tom: Usage: whois SEARCH_STRING",
                          whois.execute("tom", [], None, None))
        self.assertEqual('tom: Adam Barth is "Adam Barth" <abarth@webkit.org> (:abarth) (r). Why do you ask?',
                          whois.execute("tom", ["Adam", "Barth"], None, None))
        self.assertEqual("tom: Sorry, I don't know any contributors matching 'unknown@example.com'.",
                          whois.execute("tom", ["unknown@example.com"], None, None))
        self.assertEqual('tom: tonyg@chromium.org is "Tony Gentilcore" <tonyg@chromium.org> (:tonyg-cr) (r). Why do you ask?',
                          whois.execute("tom", ["tonyg@chromium.org"], None, None))
        self.assertEqual('tom: TonyG@Chromium.org is "Tony Gentilcore" <tonyg@chromium.org> (:tonyg-cr) (r). Why do you ask?',
                          whois.execute("tom", ["TonyG@Chromium.org"], None, None))
        self.assertEqual('tom: rniwa is "Ryosuke Niwa" <rniwa@webkit.org> (:rniwa) (r). Why do you ask?',
                          whois.execute("tom", ["rniwa"], None, None))
        self.assertEqual('tom: lopez is "Xan Lopez" <xan.lopez@gmail.com> (:xan) (r). Why do you ask?',
                          whois.execute("tom", ["lopez"], None, None))
        self.assertEqual(u'tom: Osztrogon\u00e1c is "Csaba Osztrogon\u00e1c" <ossy@webkit.org> (:ossy) (r). Why do you ask?',
                          whois.execute("tom", [u'Osztrogon\u00e1c'], None, None))
        self.assertEqual('tom: "Vicki Murley" <vicki@apple.com> hasn\'t told me their nick. Boo hoo :-(',
                          whois.execute("tom", ["vicki@apple.com"], None, None))
        self.assertEqual('tom: I\'m not sure who you mean?  "Gavin Peters" <gavinp@chromium.org> (:gavinp) (c) or "Gavin Barraclough" <barraclough@apple.com> (:gbarra) (r) could be \'Gavin\'.',
                          whois.execute("tom", ["Gavin"], None, None))
        self.assertEqual('tom: More than 5 contributors match \'david\', could you be more specific?',
                          whois.execute("tom", ["david"], None, None))

    def test_create_bug(self):
        create_bug = CreateBug()
        self.assertEqual("tom: Usage: create-bug BUG_TITLE",
                          create_bug.execute("tom", [], None, None))

        example_args = ["sherrif-bot", "should", "have", "a", "create-bug", "command"]
        tool = MockTool()

        # MockBugzilla has a create_bug, but it logs to stderr, this avoids any logging.
        tool.bugs.create_bug = lambda a, b, cc=None, assignee=None: 50004
        self.assertEqual("tom: Created bug: http://example.com/50004",
                          create_bug.execute("tom", example_args, tool, None))

        def mock_create_bug(title, description, cc=None, assignee=None):
            raise Exception("Exception from bugzilla!")
        tool.bugs.create_bug = mock_create_bug
        self.assertEqual("tom: Failed to create bug:\nException from bugzilla!",
                          create_bug.execute("tom", example_args, tool, None))

    def test_rollout_updates_working_copy(self):
        rollout = Rollout()
        tool = MockTool()
        tool.executive = MockExecutive(should_log=True)
        expected_logs = "MOCK run_and_throw_if_fail: ['mock-update-webkit'], cwd=/mock-checkout\n"
        OutputCapture().assert_outputs(self, rollout._update_working_copy, [tool], expected_logs=expected_logs)

    def test_rollout(self):
        rollout = Rollout()
        self.assertEqual(([1234], "testing foo"),
                          rollout._parse_args(["1234", "testing", "foo"]))

        self.assertEqual(([554], "testing foo"),
                          rollout._parse_args(["r554", "testing", "foo"]))

        self.assertEqual(([556, 792], "testing foo"),
                          rollout._parse_args(["r556", "792", "testing", "foo"]))

        self.assertEqual(([128, 256], "testing foo"),
                          rollout._parse_args(["r128,r256", "testing", "foo"]))

        self.assertEqual(([512, 1024, 2048], "testing foo"),
                          rollout._parse_args(["512,", "1024,2048", "testing", "foo"]))

        # Test invalid argument parsing:
        self.assertEqual((None, None), rollout._parse_args([]))
        self.assertEqual((None, None), rollout._parse_args(["--bar", "1234"]))

        # Invalid arguments result in the USAGE message.
        self.assertEqual("tom: Usage: rollout SVN_REVISION [SVN_REVISIONS] REASON",
                          rollout.execute("tom", [], None, None))

        tool = MockTool()
        tool.filesystem.files["/mock-checkout/test/file/one"] = ""
        tool.filesystem.files["/mock-checkout/test/file/two"] = ""
        self.assertEqual("Failed to apply reverse diff for file(s): test/file/one, test/file/two",
                          rollout._check_diff_failure("""
Preparing rollout for bug 123456.
Updating working directory
Failed to apply reverse diff for revision 123456 because of the following conflicts:
test/file/one
test/file/two
Failed to apply reverse diff for revision 123456 because of the following conflicts:
test/file/one
test/file/two
Updating OpenSource
Current branch master is up to date.
        """, tool))
        self.assertEqual(None, rollout._check_diff_failure("""
Preparing rollout for bug 123456.
Updating working directory
Some other error report involving file paths:
test/file/one
test/file/two
Updating OpenSource
Current branch master is up to date.
        """, tool))

        # FIXME: We need a better way to test IRCCommands which call tool.irc().post()
