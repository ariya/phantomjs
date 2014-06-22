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

import codecs
import logging
import os.path

from webkitpy.common.net.layouttestresults import path_for_layout_test, LayoutTestResults
from webkitpy.common.config import urls
from webkitpy.tool.bot.botinfo import BotInfo
from webkitpy.tool.grammar import plural, pluralize, join_with_separators

_log = logging.getLogger(__name__)


class FlakyTestReporter(object):
    def __init__(self, tool, bot_name):
        self._tool = tool
        self._bot_name = bot_name
        # FIXME: Use the real port object
        self._bot_info = BotInfo(tool, tool.deprecated_port().name())

    def _author_emails_for_test(self, flaky_test):
        test_path = path_for_layout_test(flaky_test)
        commit_infos = self._tool.checkout().recent_commit_infos_for_files([test_path])
        # This ignores authors which are not committers because we don't have their bugzilla_email.
        return set([commit_info.author().bugzilla_email() for commit_info in commit_infos if commit_info.author()])

    def _bugzilla_email(self):
        # FIXME: This is kinda a funny way to get the bugzilla email,
        # we could also just create a Credentials object directly
        # but some of the Credentials logic is in bugzilla.py too...
        self._tool.bugs.authenticate()
        return self._tool.bugs.username

    # FIXME: This should move into common.config
    _bot_emails = set([
        "commit-queue@webkit.org",  # commit-queue
        "eseidel@chromium.org",  # old commit-queue
        "webkit.review.bot@gmail.com",  # style-queue, sheriff-bot, CrLx/Gtk EWS
        "buildbot@hotmail.com",  # Win EWS
        # Mac EWS currently uses eric@webkit.org, but that's not normally a bot
    ])

    def _lookup_bug_for_flaky_test(self, flaky_test):
        bugs = self._tool.bugs.queries.fetch_bugs_matching_search(search_string=flaky_test)
        if not bugs:
            return None
        # Match any bugs which are from known bots or the email this bot is using.
        allowed_emails = self._bot_emails | set([self._bugzilla_email])
        bugs = filter(lambda bug: bug.reporter_email() in allowed_emails, bugs)
        if not bugs:
            return None
        if len(bugs) > 1:
            # FIXME: There are probably heuristics we could use for finding
            # the right bug instead of the first, like open vs. closed.
            _log.warn("Found %s %s matching '%s' filed by a bot, using the first." % (pluralize('bug', len(bugs)), [bug.id() for bug in bugs], flaky_test))
        return bugs[0]

    def _view_source_url_for_test(self, test_path):
        return urls.view_source_url("LayoutTests/%s" % test_path)

    def _create_bug_for_flaky_test(self, flaky_test, author_emails, latest_flake_message):
        format_values = {
            'test': flaky_test,
            'authors': join_with_separators(sorted(author_emails)),
            'flake_message': latest_flake_message,
            'test_url': self._view_source_url_for_test(flaky_test),
            'bot_name': self._bot_name,
        }
        title = "Flaky Test: %(test)s" % format_values
        description = """This is an automatically generated bug from the %(bot_name)s.
%(test)s has been flaky on the %(bot_name)s.

%(test)s was authored by %(authors)s.
%(test_url)s

%(flake_message)s

The bots will update this with information from each new failure.

If you believe this bug to be fixed or invalid, feel free to close.  The bots will re-open if the flake re-occurs.

If you would like to track this test fix with another bug, please close this bug as a duplicate.  The bots will follow the duplicate chain when making future comments.
""" % format_values

        master_flake_bug = 50856  # MASTER: Flaky tests found by the commit-queue
        return self._tool.bugs.create_bug(title, description,
            component="Tools / Tests",
            cc=",".join(author_emails),
            blocked="50856")

    # This is over-engineered, but it makes for pretty bug messages.
    def _optional_author_string(self, author_emails):
        if not author_emails:
            return ""
        heading_string = plural('author') if len(author_emails) > 1 else 'author'
        authors_string = join_with_separators(sorted(author_emails))
        return " (%s: %s)" % (heading_string, authors_string)

    def _latest_flake_message(self, flaky_result, patch):
        failure_messages = [failure.message() for failure in flaky_result.failures]
        flake_message = "The %s just saw %s flake (%s) while processing attachment %s on bug %s." % (self._bot_name, flaky_result.test_name, ", ".join(failure_messages), patch.id(), patch.bug_id())
        return "%s\n%s" % (flake_message, self._bot_info.summary_text())

    def _results_diff_path_for_test(self, test_path):
        # FIXME: This is a big hack.  We should get this path from results.json
        # except that old-run-webkit-tests doesn't produce a results.json
        # so we just guess at the file path.
        (test_path_root, _) = os.path.splitext(test_path)
        return "%s-diffs.txt" % test_path_root

    def _follow_duplicate_chain(self, bug):
        while bug.is_closed() and bug.duplicate_of():
            bug = self._tool.bugs.fetch_bug(bug.duplicate_of())
        return bug

    def _update_bug_for_flaky_test(self, bug, latest_flake_message):
        self._tool.bugs.post_comment_to_bug(bug.id(), latest_flake_message)

    # This method is needed because our archive paths include a leading tmp/layout-test-results
    def _find_in_archive(self, path, archive):
        for archived_path in archive.namelist():
            # Archives are currently created with full paths.
            if archived_path.endswith(path):
                return archived_path
        return None

    def _attach_failure_diff(self, flake_bug_id, flaky_test, results_archive_zip):
        results_diff_path = self._results_diff_path_for_test(flaky_test)
        # Check to make sure that the path makes sense.
        # Since we're not actually getting this path from the results.html
        # there is a chance it's wrong.
        bot_id = self._tool.status_server.bot_id or "bot"
        archive_path = self._find_in_archive(results_diff_path, results_archive_zip)
        if archive_path:
            results_diff = results_archive_zip.read(archive_path)
            description = "Failure diff from %s" % bot_id
            self._tool.bugs.add_attachment_to_bug(flake_bug_id, results_diff, description, filename="failure.diff")
        else:
            _log.warn("%s does not exist in results archive, uploading entire archive." % results_diff_path)
            description = "Archive of layout-test-results from %s" % bot_id
            # results_archive is a ZipFile object, grab the File object (.fp) to pass to Mechanize for uploading.
            results_archive_file = results_archive_zip.fp
            # Rewind the file object to start (since Mechanize won't do that automatically)
            # See https://bugs.webkit.org/show_bug.cgi?id=54593
            results_archive_file.seek(0)
            self._tool.bugs.add_attachment_to_bug(flake_bug_id, results_archive_file, description, filename="layout-test-results.zip")

    def report_flaky_tests(self, patch, flaky_test_results, results_archive):
        message = "The %s encountered the following flaky tests while processing attachment %s:\n\n" % (self._bot_name, patch.id())
        for flaky_result in flaky_test_results:
            flaky_test = flaky_result.test_name
            bug = self._lookup_bug_for_flaky_test(flaky_test)
            latest_flake_message = self._latest_flake_message(flaky_result, patch)
            author_emails = self._author_emails_for_test(flaky_test)
            if not bug:
                _log.info("Bug does not already exist for %s, creating." % flaky_test)
                flake_bug_id = self._create_bug_for_flaky_test(flaky_test, author_emails, latest_flake_message)
            else:
                bug = self._follow_duplicate_chain(bug)
                # FIXME: Ideally we'd only make one comment per flake, not two.  But that's not possible
                # in all cases (e.g. when reopening), so for now file attachment and comment are separate.
                self._update_bug_for_flaky_test(bug, latest_flake_message)
                flake_bug_id = bug.id()

            self._attach_failure_diff(flake_bug_id, flaky_test, results_archive)
            message += "%s bug %s%s\n" % (flaky_test, flake_bug_id, self._optional_author_string(author_emails))

        message += "The %s is continuing to process your patch." % self._bot_name
        self._tool.bugs.post_comment_to_bug(patch.bug_id(), message)
