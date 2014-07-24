# Copyright (C) 2009, 2011 Google Inc. All rights reserved.
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

from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.thirdparty.mock import Mock
from webkitpy.tool.commands.commandtest import CommandsTest
from webkitpy.tool.commands.download import *
from webkitpy.tool.mocktool import MockOptions, MockTool
from webkitpy.common.checkout.checkout_mock import MockCheckout


class AbstractRolloutPrepCommandTest(unittest.TestCase):
    def test_commit_info(self):
        command = AbstractRolloutPrepCommand()
        tool = MockTool()
        command.bind_to_tool(tool)
        output = OutputCapture()

        expected_logs = "Preparing rollout for bug 50000.\n"
        commit_info = output.assert_outputs(self, command._commit_info, [1234], expected_logs=expected_logs)
        self.assertTrue(commit_info)

        mock_commit_info = Mock()
        mock_commit_info.bug_id = lambda: None
        tool._checkout.commit_info_for_revision = lambda revision: mock_commit_info
        expected_logs = "Unable to parse bug number from diff.\n"
        commit_info = output.assert_outputs(self, command._commit_info, [1234], expected_logs=expected_logs)
        self.assertEqual(commit_info, mock_commit_info)

    def test_prepare_state(self):
        command = AbstractRolloutPrepCommand()
        mock_commit_info = MockCheckout().commit_info_for_revision(123)
        command._commit_info = lambda revision: mock_commit_info

        state = command._prepare_state(None, ["124 123 125", "Reason"], None)
        self.assertEqual(123, state["revision"])
        self.assertEqual([123, 124, 125], state["revision_list"])

        self.assertRaises(ScriptError, command._prepare_state, options=None, args=["125 r122  123", "Reason"], tool=None)
        self.assertRaises(ScriptError, command._prepare_state, options=None, args=["125 foo 123", "Reason"], tool=None)

        command._commit_info = lambda revision: None
        state = command._prepare_state(None, ["124 123 125", "Reason"], None)
        self.assertEqual(123, state["revision"])
        self.assertEqual([123, 124, 125], state["revision_list"])


class DownloadCommandsTest(CommandsTest):
    def _default_options(self):
        options = MockOptions()
        options.build = True
        options.build_style = True
        options.check_style = True
        options.check_style_filter = None
        options.clean = True
        options.close_bug = True
        options.force_clean = False
        options.non_interactive = False
        options.parent_command = 'MOCK parent command'
        options.quiet = False
        options.test = True
        options.update = True
        return options

    def test_build(self):
        expected_logs = "Updating working directory\nBuilding WebKit\n"
        self.assert_execute_outputs(Build(), [], options=self._default_options(), expected_logs=expected_logs)

    def test_build_and_test(self):
        expected_logs = """Updating working directory
Building WebKit
Running Python unit tests
Running Perl unit tests
Running JavaScriptCore tests
Running bindings generation tests
Running WebKit unit tests
Running run-webkit-tests
"""
        self.assert_execute_outputs(BuildAndTest(), [], options=self._default_options(), expected_logs=expected_logs)

    def test_apply_attachment(self):
        options = self._default_options()
        options.update = True
        options.local_commit = True
        expected_logs = "Updating working directory\nProcessing 1 patch from 1 bug.\nProcessing patch 10000 from bug 50000.\n"
        self.assert_execute_outputs(ApplyAttachment(), [10000], options=options, expected_logs=expected_logs)

    def test_apply_from_bug(self):
        options = self._default_options()
        options.update = True
        options.local_commit = True

        expected_logs = "Updating working directory\n0 reviewed patches found on bug 50001.\nNo reviewed patches found, looking for unreviewed patches.\n1 patch found on bug 50001.\nProcessing 1 patch from 1 bug.\nProcessing patch 10002 from bug 50001.\n"
        self.assert_execute_outputs(ApplyFromBug(), [50001], options=options, expected_logs=expected_logs)

        expected_logs = "Updating working directory\n2 reviewed patches found on bug 50000.\nProcessing 2 patches from 1 bug.\nProcessing patch 10000 from bug 50000.\nProcessing patch 10001 from bug 50000.\n"
        self.assert_execute_outputs(ApplyFromBug(), [50000], options=options, expected_logs=expected_logs)

    def test_apply_watch_list(self):
        expected_logs = """Processing 1 patch from 1 bug.
Updating working directory
MOCK run_and_throw_if_fail: ['mock-update-webkit'], cwd=/mock-checkout
Processing patch 10000 from bug 50000.
MockWatchList: determine_cc_and_messages
No bug was updated because no id was given.
Result of watchlist: cc "abarth@webkit.org, eric@webkit.org, levin@chromium.org" messages "Message1.

Message2."
"""
        self.assert_execute_outputs(ApplyWatchList(), [10000], options=self._default_options(), expected_logs=expected_logs, tool=MockTool(log_executive=True))

    def test_land(self):
        expected_logs = """Building WebKit
Running Python unit tests
Running Perl unit tests
Running JavaScriptCore tests
Running bindings generation tests
Running WebKit unit tests
Running run-webkit-tests
Committed r49824: <http://trac.webkit.org/changeset/49824>
Updating bug 50000
"""
        mock_tool = MockTool()
        mock_tool.scm().create_patch = Mock(return_value="Patch1\nMockPatch\n")
        mock_tool.checkout().modified_changelogs = Mock(return_value=[])
        self.assert_execute_outputs(Land(), [50000], options=self._default_options(), expected_logs=expected_logs, tool=mock_tool)
        # Make sure we're not calling expensive calls too often.
        self.assertEqual(mock_tool.scm().create_patch.call_count, 0)
        self.assertEqual(mock_tool.checkout().modified_changelogs.call_count, 1)

    def test_land_cowhand(self):
        expected_logs = """MOCK run_and_throw_if_fail: ['mock-prepare-ChangeLog', '--email=MOCK email', '--merge-base=None', 'MockFile1'], cwd=/mock-checkout
MOCK run_and_throw_if_fail: ['mock-check-webkit-style', '--git-commit', 'MOCK git commit', '--diff-files', 'MockFile1', '--filter', '-changelog'], cwd=/mock-checkout
MOCK run_command: ['ruby', '-I', '/mock-checkout/Websites/bugs.webkit.org/PrettyPatch', '/mock-checkout/Websites/bugs.webkit.org/PrettyPatch/prettify.rb'], cwd=None, input=Patch1
MOCK: user.open_url: file://...
Was that diff correct?
Building WebKit
MOCK run_and_throw_if_fail: ['mock-build-webkit'], cwd=/mock-checkout, env={'LC_ALL': 'C', 'MOCK_ENVIRON_COPY': '1'}
Running Python unit tests
MOCK run_and_throw_if_fail: ['mock-test-webkitpy'], cwd=/mock-checkout
Running Perl unit tests
MOCK run_and_throw_if_fail: ['mock-test-webkitperl'], cwd=/mock-checkout
Running JavaScriptCore tests
MOCK run_and_throw_if_fail: ['mock-run-javacriptcore-tests'], cwd=/mock-checkout
Running bindings generation tests
MOCK run_and_throw_if_fail: ['mock-run-bindings-tests'], cwd=/mock-checkout
Running WebKit unit tests
MOCK run_and_throw_if_fail: ['mock-run-webkit-unit-tests'], cwd=/mock-checkout
Running run-webkit-tests
MOCK run_and_throw_if_fail: ['mock-run-webkit-tests', '--quiet'], cwd=/mock-checkout
Committed r49824: <http://trac.webkit.org/changeset/49824>
Committed r49824: <http://trac.webkit.org/changeset/49824>
No bug id provided.
"""
        mock_tool = MockTool(log_executive=True)
        self.assert_execute_outputs(LandCowhand(), [50000], options=self._default_options(), expected_logs=expected_logs, tool=mock_tool)

        expected_logs = "land-cowboy is deprecated, use land-cowhand instead.\n" + expected_logs
        self.assert_execute_outputs(LandCowboy(), [50000], options=self._default_options(), expected_logs=expected_logs, tool=mock_tool)

    def test_land_red_builders(self):
        expected_logs = """Building WebKit
Running Python unit tests
Running Perl unit tests
Running JavaScriptCore tests
Running bindings generation tests
Running WebKit unit tests
Running run-webkit-tests
Committed r49824: <http://trac.webkit.org/changeset/49824>
Updating bug 50000
"""
        mock_tool = MockTool()
        mock_tool.buildbot.light_tree_on_fire()
        self.assert_execute_outputs(Land(), [50000], options=self._default_options(), expected_logs=expected_logs, tool=mock_tool)

    def test_check_style(self):
        expected_logs = """Processing 1 patch from 1 bug.
Updating working directory
MOCK run_and_throw_if_fail: ['mock-update-webkit'], cwd=/mock-checkout
Processing patch 10000 from bug 50000.
MOCK run_and_throw_if_fail: ['mock-check-webkit-style', '--git-commit', 'MOCK git commit', '--diff-files', 'MockFile1'], cwd=/mock-checkout
"""
        self.assert_execute_outputs(CheckStyle(), [10000], options=self._default_options(), expected_logs=expected_logs, tool=MockTool(log_executive=True))

    def test_build_attachment(self):
        expected_logs = "Processing 1 patch from 1 bug.\nUpdating working directory\nProcessing patch 10000 from bug 50000.\nBuilding WebKit\n"
        self.assert_execute_outputs(BuildAttachment(), [10000], options=self._default_options(), expected_logs=expected_logs)

    def test_land_attachment(self):
        # FIXME: This expected result is imperfect, notice how it's seeing the same patch as still there after it thought it would have cleared the flags.
        expected_logs = """Processing 1 patch from 1 bug.
Updating working directory
Processing patch 10000 from bug 50000.
Building WebKit
Running Python unit tests
Running Perl unit tests
Running JavaScriptCore tests
Running bindings generation tests
Running WebKit unit tests
Running run-webkit-tests
Committed r49824: <http://trac.webkit.org/changeset/49824>
Not closing bug 50000 as attachment 10000 has review=+.  Assuming there are more patches to land from this bug.
"""
        self.assert_execute_outputs(LandAttachment(), [10000], options=self._default_options(), expected_logs=expected_logs)

    def test_land_from_bug(self):
        # FIXME: This expected result is imperfect, notice how it's seeing the same patch as still there after it thought it would have cleared the flags.
        expected_logs = """2 reviewed patches found on bug 50000.
Processing 2 patches from 1 bug.
Updating working directory
Processing patch 10000 from bug 50000.
Building WebKit
Running Python unit tests
Running Perl unit tests
Running JavaScriptCore tests
Running bindings generation tests
Running WebKit unit tests
Running run-webkit-tests
Committed r49824: <http://trac.webkit.org/changeset/49824>
Not closing bug 50000 as attachment 10000 has review=+.  Assuming there are more patches to land from this bug.
Updating working directory
Processing patch 10001 from bug 50000.
Building WebKit
Running Python unit tests
Running Perl unit tests
Running JavaScriptCore tests
Running bindings generation tests
Running WebKit unit tests
Running run-webkit-tests
Committed r49824: <http://trac.webkit.org/changeset/49824>
Not closing bug 50000 as attachment 10000 has review=+.  Assuming there are more patches to land from this bug.
"""
        self.assert_execute_outputs(LandFromBug(), [50000], options=self._default_options(), expected_logs=expected_logs)

    def test_land_from_url(self):
        # FIXME: This expected result is imperfect, notice how it's seeing the same patch as still there after it thought it would have cleared the flags.
        expected_logs = """2 patches found on bug 50000.
Processing 2 patches from 1 bug.
Updating working directory
Processing patch 10000 from bug 50000.
Building WebKit
Running Python unit tests
Running Perl unit tests
Running JavaScriptCore tests
Running bindings generation tests
Running WebKit unit tests
Running run-webkit-tests
Committed r49824: <http://trac.webkit.org/changeset/49824>
Not closing bug 50000 as attachment 10000 has review=+.  Assuming there are more patches to land from this bug.
Updating working directory
Processing patch 10001 from bug 50000.
Building WebKit
Running Python unit tests
Running Perl unit tests
Running JavaScriptCore tests
Running bindings generation tests
Running WebKit unit tests
Running run-webkit-tests
Committed r49824: <http://trac.webkit.org/changeset/49824>
Not closing bug 50000 as attachment 10000 has review=+.  Assuming there are more patches to land from this bug.
"""
        self.assert_execute_outputs(LandFromURL(), ["https://bugs.webkit.org/show_bug.cgi?id=50000"], options=self._default_options(), expected_logs=expected_logs)

    def test_prepare_rollout(self):
        expected_logs = "Preparing rollout for bug 50000.\nUpdating working directory\n"
        self.assert_execute_outputs(PrepareRollout(), [852, "Reason"], options=self._default_options(), expected_logs=expected_logs)

    def test_create_rollout(self):
        expected_logs = """Preparing rollout for bug 50000.
Updating working directory
MOCK create_bug
bug_title: REGRESSION(r852): Reason
bug_description: http://trac.webkit.org/changeset/852 broke the build:
Reason
component: MOCK component
cc: MOCK cc
blocked: 50000
MOCK add_patch_to_bug: bug_id=60001, description=ROLLOUT of r852, mark_for_review=False, mark_for_commit_queue=True, mark_for_landing=False
-- Begin comment --
Any committer can land this patch automatically by marking it commit-queue+.  The commit-queue will build and test the patch before landing to ensure that the rollout will be successful.  This process takes approximately 15 minutes.

If you would like to land the rollout faster, you can use the following command:

  webkit-patch land-attachment ATTACHMENT_ID

where ATTACHMENT_ID is the ID of this attachment.
-- End comment --
"""
        self.assert_execute_outputs(CreateRollout(), [852, "Reason"], options=self._default_options(), expected_logs=expected_logs)
        self.assert_execute_outputs(CreateRollout(), ["855 852 854", "Reason"], options=self._default_options(), expected_logs=expected_logs)

    def test_create_rollout_resolved(self):
        expected_logs = """Preparing rollout for bug 50004.
Updating working directory
MOCK create_bug
bug_title: REGRESSION(r3001): Reason
bug_description: http://trac.webkit.org/changeset/3001 broke the build:
Reason
component: MOCK component
cc: MOCK cc
blocked: 50004
MOCK reopen_bug 50004 with comment 'Re-opened since this is blocked by bug 60001'
MOCK add_patch_to_bug: bug_id=60001, description=ROLLOUT of r3001, mark_for_review=False, mark_for_commit_queue=True, mark_for_landing=False
-- Begin comment --
Any committer can land this patch automatically by marking it commit-queue+.  The commit-queue will build and test the patch before landing to ensure that the rollout will be successful.  This process takes approximately 15 minutes.

If you would like to land the rollout faster, you can use the following command:

  webkit-patch land-attachment ATTACHMENT_ID

where ATTACHMENT_ID is the ID of this attachment.
-- End comment --
"""
        self.assert_execute_outputs(CreateRollout(), [3001, "Reason"], options=self._default_options(), expected_logs=expected_logs)

    def test_rollout(self):
        expected_logs = """Preparing rollout for bug 50000.
Updating working directory
MOCK: user.open_url: file://...
Was that diff correct?
Building WebKit
Committed r49824: <http://trac.webkit.org/changeset/49824>
MOCK reopen_bug 50000 with comment 'Reverted r852 for reason:

Reason

Committed r49824: <http://trac.webkit.org/changeset/49824>'
"""
        self.assert_execute_outputs(Rollout(), [852, "Reason"], options=self._default_options(), expected_logs=expected_logs)

