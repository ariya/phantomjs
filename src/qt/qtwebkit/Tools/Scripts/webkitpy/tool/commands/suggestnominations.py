# Copyright (c) 2011 Google Inc. All rights reserved.
# Copyright (c) 2011 Code Aurora Forum. All rights reserved.
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

from optparse import make_option
import re

from webkitpy.common.checkout.changelog import ChangeLogEntry
from webkitpy.common.config.committers import CommitterList
from webkitpy.tool.grammar import join_with_separators
from webkitpy.tool.multicommandtool import Command


class CommitLogError(Exception):
    def __init__(self):
        Exception.__init__(self)


class CommitLogMissingReviewer(CommitLogError):
    def __init__(self):
        CommitLogError.__init__(self)


class AbstractCommitLogCommand(Command):
    _leading_indent_regexp = re.compile(r"^[ ]{4}", re.MULTILINE)
    _reviewed_by_regexp = re.compile(ChangeLogEntry.reviewed_by_regexp, re.MULTILINE)
    _patch_by_regexp = re.compile(r'^Patch by (?P<name>.+?)\s+<(?P<email>[^<>]+)> on (?P<date>\d{4}-\d{2}-\d{2})$', re.MULTILINE)
    _committer_regexp = re.compile(r'^Author: (?P<email>\S+)\s+<[^>]+>$', re.MULTILINE)
    _date_regexp = re.compile(r'^Date:   (?P<date>\d{4}-\d{2}-\d{2}) (?P<time>\d{2}:\d{2}:\d{2}) [\+\-]\d{4}$', re.MULTILINE)
    _revision_regexp = re.compile(r'^git-svn-id: http://svn.webkit.org/repository/webkit/trunk@(?P<svnid>\d+) (?P<gitid>[0-9a-f\-]{36})$', re.MULTILINE)

    def __init__(self, options=None):
        options = options or []
        options += [
            make_option("--max-commit-age", action="store", dest="max_commit_age", type="int", default=9, help="Specify maximum commit age to consider (in months)."),
        ]
        options = sorted(options, cmp=lambda a, b: cmp(a._long_opts, b._long_opts))
        super(AbstractCommitLogCommand, self).__init__(options=options)
        # FIXME: This should probably be on the tool somewhere.
        self._committer_list = CommitterList()

    def _init_options(self, options):
        self.verbose = options.verbose
        self.max_commit_age = options.max_commit_age

    # FIXME: This should move to scm.py
    def _recent_commit_messages(self):
        git_log = self._tool.executive.run_command(['git', 'log', '--date=iso', '--since="%s months ago"' % self.max_commit_age])
        messages = re.compile(r"^commit \w{40}$", re.MULTILINE).split(git_log)[1:]  # Ignore the first message which will be empty.
        for message in messages:
            # Unindent all the lines
            (message, _) = self._leading_indent_regexp.subn("", message)
            yield message.lstrip()  # Remove any leading newlines from the log message.

    def _author_name_from_email(self, email):
        contributor = self._committer_list.contributor_by_email(email)
        return contributor.full_name if contributor else None

    def _contributor_from_email(self, email):
        contributor = self._committer_list.contributor_by_email(email)
        return contributor if contributor else None

    def _parse_commit_message(self, commit_message):
        committer_match = self._committer_regexp.search(commit_message)
        if not committer_match:
            raise CommitLogError

        committer_email = committer_match.group('email')
        if not committer_email:
            raise CommitLogError

        committer = self._contributor_from_email(committer_email)
        if not committer:
            raise CommitLogError

        commit_date_match = self._date_regexp.search(commit_message)
        if not commit_date_match:
            raise CommitLogError
        commit_date = commit_date_match.group('date')

        revision_match = self._revision_regexp.search(commit_message)
        if not revision_match:
            raise CommitLogError
        revision = revision_match.group('svnid')

        # Look for "Patch by" line first, which is used for non-committer contributors;
        # otherwise, use committer info determined above.
        author_match = self._patch_by_regexp.search(commit_message)
        if not author_match:
            author_match = committer_match

        author_email = author_match.group('email')
        if not author_email:
            author_email = committer_email

        author_name = author_match.group('name') if 'name' in author_match.groupdict() else None
        if not author_name:
            author_name = self._author_name_from_email(author_email)
        if not author_name:
            raise CommitLogError

        contributor = self._contributor_from_email(author_email)
        if contributor and author_name != contributor.full_name and contributor.full_name:
            author_name = contributor.full_name

        reviewer_match = self._reviewed_by_regexp.search(commit_message)
        if not reviewer_match:
            raise CommitLogMissingReviewer
        reviewers = reviewer_match.group('reviewer')

        return {
            'committer': committer,
            'commit_date': commit_date,
            'revision': revision,
            'author_email': author_email,
            'author_name': author_name,
            'contributor': contributor,
            'reviewers': reviewers,
        }


class SuggestNominations(AbstractCommitLogCommand):
    name = "suggest-nominations"
    help_text = "Suggest contributors for committer/reviewer nominations"

    def __init__(self):
        options = [
            make_option("--committer-minimum", action="store", dest="committer_minimum", type="int", default=10, help="Specify minimum patch count for Committer nominations."),
            make_option("--reviewer-minimum", action="store", dest="reviewer_minimum", type="int", default=80, help="Specify minimum patch count for Reviewer nominations."),
            make_option("--show-commits", action="store_true", dest="show_commits", default=False, help="Show commit history with nomination suggestions."),
        ]
        super(SuggestNominations, self).__init__(options=options)

    def _init_options(self, options):
        super(SuggestNominations, self)._init_options(options)
        self.committer_minimum = options.committer_minimum
        self.reviewer_minimum = options.reviewer_minimum
        self.show_commits = options.show_commits

    def _count_commit(self, commit, analysis):
        author_name = commit['author_name']
        author_email = commit['author_email']
        revision = commit['revision']
        commit_date = commit['commit_date']

        # See if we already have a contributor with this author_name or email
        counter_by_name = analysis['counters_by_name'].get(author_name)
        counter_by_email = analysis['counters_by_email'].get(author_email)
        if counter_by_name:
            if counter_by_email:
                if counter_by_name != counter_by_email:
                    # Merge these two counters  This is for the case where we had
                    # John Smith (jsmith@gmail.com) and Jonathan Smith (jsmith@apple.com)
                    # and just found a John Smith (jsmith@apple.com).  Now we know the
                    # two names are the same person
                    counter_by_name['names'] |= counter_by_email['names']
                    counter_by_name['emails'] |= counter_by_email['emails']
                    counter_by_name['count'] += counter_by_email.get('count', 0)
                    analysis['counters_by_email'][author_email] = counter_by_name
            else:
                # Add email to the existing counter
                analysis['counters_by_email'][author_email] = counter_by_name
                counter_by_name['emails'] |= set([author_email])
        else:
            if counter_by_email:
                # Add name to the existing counter
                analysis['counters_by_name'][author_name] = counter_by_email
                counter_by_email['names'] |= set([author_name])
            else:
                # Create new counter
                new_counter = {'names': set([author_name]), 'emails': set([author_email]), 'latest_name': author_name, 'latest_email': author_email, 'commits': ""}
                analysis['counters_by_name'][author_name] = new_counter
                analysis['counters_by_email'][author_email] = new_counter

        assert(analysis['counters_by_name'][author_name] == analysis['counters_by_email'][author_email])
        counter = analysis['counters_by_name'][author_name]
        counter['count'] = counter.get('count', 0) + 1

        if revision.isdigit():
            revision = "http://trac.webkit.org/changeset/" + revision
        counter['commits'] += "  commit: %s on %s by %s (%s)\n" % (revision, commit_date, author_name, author_email)

    def _count_recent_patches(self):
        analysis = {
            'counters_by_name': {},
            'counters_by_email': {},
        }
        for commit_message in self._recent_commit_messages():
            try:
                self._count_commit(self._parse_commit_message(commit_message), analysis)
            except CommitLogError, exception:
                continue
        return analysis['counters_by_email']

    def _collect_nominations(self, counters_by_email):
        nominations = []
        for author_email, counter in counters_by_email.items():
            if author_email != counter['latest_email']:
                continue
            roles = []

            contributor = self._committer_list.contributor_by_email(author_email)

            author_name = counter['latest_name']
            patch_count = counter['count']

            if patch_count >= self.committer_minimum and (not contributor or not contributor.can_commit):
                roles.append("committer")
            if patch_count >= self.reviewer_minimum and contributor and contributor.can_commit and not contributor.can_review:
                roles.append("reviewer")
            if roles:
                nominations.append({
                    'roles': roles,
                    'author_name': author_name,
                    'author_email': author_email,
                    'patch_count': patch_count,
                })
        return nominations

    def _print_nominations(self, nominations, counters_by_email):
        def nomination_cmp(a_nomination, b_nomination):
            roles_result = cmp(a_nomination['roles'], b_nomination['roles'])
            if roles_result:
                return -roles_result
            count_result = cmp(a_nomination['patch_count'], b_nomination['patch_count'])
            if count_result:
                return -count_result
            return cmp(a_nomination['author_name'], b_nomination['author_name'])

        for nomination in sorted(nominations, nomination_cmp):
            # This is a little bit of a hack, but its convienent to just pass the nomination dictionary to the formating operator.
            nomination['roles_string'] = join_with_separators(nomination['roles']).upper()
            print "%(roles_string)s: %(author_name)s (%(author_email)s) has %(patch_count)s reviewed patches" % nomination
            counter = counters_by_email[nomination['author_email']]

            if self.show_commits:
                print counter['commits']

    def _print_counts(self, counters_by_email):
        def counter_cmp(a_tuple, b_tuple):
            # split the tuples
            # the second element is the "counter" structure
            _, a_counter = a_tuple
            _, b_counter = b_tuple

            count_result = cmp(a_counter['count'], b_counter['count'])
            if count_result:
                return -count_result
            return cmp(a_counter['latest_name'].lower(), b_counter['latest_name'].lower())

        for author_email, counter in sorted(counters_by_email.items(), counter_cmp):
            if author_email != counter['latest_email']:
                continue
            contributor = self._committer_list.contributor_by_email(author_email)
            author_name = counter['latest_name']
            patch_count = counter['count']
            counter['names'] = counter['names'] - set([author_name])
            counter['emails'] = counter['emails'] - set([author_email])

            alias_list = []
            for alias in counter['names']:
                alias_list.append(alias)
            for alias in counter['emails']:
                alias_list.append(alias)
            if alias_list:
                print "CONTRIBUTOR: %s (%s) has %d reviewed patches %s" % (author_name, author_email, patch_count, "(aliases: " + ", ".join(alias_list) + ")")
            else:
                print "CONTRIBUTOR: %s (%s) has %d reviewed patches" % (author_name, author_email, patch_count)
        return

    def execute(self, options, args, tool):
        self._init_options(options)
        patch_counts = self._count_recent_patches()
        nominations = self._collect_nominations(patch_counts)
        self._print_nominations(nominations, patch_counts)
        if self.verbose:
            self._print_counts(patch_counts)

if __name__ == "__main__":
    SuggestNominations()
