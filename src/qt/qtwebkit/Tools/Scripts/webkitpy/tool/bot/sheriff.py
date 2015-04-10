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

from webkitpy.common.config import urls
from webkitpy.common.system.executive import ScriptError
from webkitpy.tool.grammar import join_with_separators


class Sheriff(object):
    def __init__(self, tool, sheriffbot):
        self._tool = tool
        self._sheriffbot = sheriffbot

    def name(self):
        return self._sheriffbot.name

    def responsible_nicknames_from_commit_info(self, commit_info):
        nestedList = [party.irc_nicknames for party in commit_info.responsible_parties() if party.irc_nicknames]
        return reduce(lambda list, childList: list + childList, nestedList)

    def post_irc_warning(self, commit_info, builders):
        irc_nicknames = sorted(self.responsible_nicknames_from_commit_info(commit_info))
        irc_prefix = ": " if irc_nicknames else ""
        irc_message = "%s%s%s might have broken %s" % (
            ", ".join(irc_nicknames),
            irc_prefix,
            urls.view_revision_url(commit_info.revision()),
            join_with_separators([builder.name() for builder in builders]))

        self._tool.irc().post(irc_message)

    def post_irc_summary(self, failure_map):
        failing_tests = failure_map.failing_tests()
        if not failing_tests:
            return
        test_list_limit = 5
        irc_message = "New failures: %s" % ", ".join(sorted(failing_tests)[:test_list_limit])
        failure_count = len(failing_tests)
        if failure_count > test_list_limit:
            irc_message += " (and %s more...)" % (failure_count - test_list_limit)
        self._tool.irc().post(irc_message)

    def post_rollout_patch(self, svn_revision_list, rollout_reason):
        # Ensure that svn revisions are numbers (and not options to
        # create-rollout).
        try:
            svn_revisions = " ".join([str(int(revision)) for revision in svn_revision_list])
        except:
            raise ScriptError(message="Invalid svn revision number \"%s\"."
                              % " ".join(svn_revision_list))

        if rollout_reason.startswith("-"):
            raise ScriptError(message="The rollout reason may not begin "
                              "with - (\"%s\")." % rollout_reason)

        output = self._sheriffbot.run_webkit_patch([
            "create-rollout",
            "--force-clean",
            # In principle, we should pass --non-interactive here, but it
            # turns out that create-rollout doesn't need it yet.  We can't
            # pass it prophylactically because we reject unrecognized command
            # line switches.
            "--parent-command=sheriff-bot",
            svn_revisions,
            rollout_reason,
        ])
        return urls.parse_bug_id(output)

    def post_blame_comment_on_bug(self, commit_info, builders, tests):
        if not commit_info.bug_id():
            return
        comment = "%s might have broken %s" % (
            urls.view_revision_url(commit_info.revision()),
            join_with_separators([builder.name() for builder in builders]))
        if tests:
            comment += "\nThe following tests are not passing:\n"
            comment += "\n".join(tests)
        self._tool.bugs.post_comment_to_bug(commit_info.bug_id(),
                                            comment,
                                            cc=self._sheriffbot.watchers)
