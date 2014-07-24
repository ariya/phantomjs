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

import itertools
import random
import re

from webkitpy.common.config import irc as config_irc
from webkitpy.common.config import urls
from webkitpy.common.config.committers import CommitterList
from webkitpy.common.net.web import Web
from webkitpy.common.system.executive import ScriptError
from webkitpy.tool.bot.queueengine import TerminateQueue
from webkitpy.tool.grammar import join_with_separators


def _post_error_and_check_for_bug_url(tool, nicks_string, exception):
    tool.irc().post("%s" % exception)
    bug_id = urls.parse_bug_id(exception.output)
    if bug_id:
        bug_url = tool.bugs.bug_url_for_bug_id(bug_id)
        tool.irc().post("%s: Ugg...  Might have created %s" % (nicks_string, bug_url))


# FIXME: Merge with Command?
class IRCCommand(object):
    usage_string = None
    help_string = None

    def execute(self, nick, args, tool, sheriff):
        raise NotImplementedError("subclasses must implement")

    @classmethod
    def usage(cls, nick):
        return "%s: Usage: %s" % (nick, cls.usage_string)

    @classmethod
    def help(cls, nick):
        return "%s: %s" % (nick, cls.help_string)


class CreateBug(IRCCommand):
    usage_string = "create-bug BUG_TITLE"
    help_string = "Creates a Bugzilla bug with the given title."

    def execute(self, nick, args, tool, sheriff):
        if not args:
            return self.usage(nick)

        bug_title = " ".join(args)
        bug_description = "%s\nRequested by %s on %s." % (bug_title, nick, config_irc.channel)

        # There happens to be a committers list hung off of Bugzilla, so
        # re-using that one makes things easiest for now.
        requester = tool.bugs.committers.contributor_by_irc_nickname(nick)
        requester_email = requester.bugzilla_email() if requester else None

        try:
            bug_id = tool.bugs.create_bug(bug_title, bug_description, cc=requester_email, assignee=requester_email)
            bug_url = tool.bugs.bug_url_for_bug_id(bug_id)
            return "%s: Created bug: %s" % (nick, bug_url)
        except Exception, e:
            return "%s: Failed to create bug:\n%s" % (nick, e)


class Help(IRCCommand):
    usage_string = "help [COMMAND]"
    help_string = "Provides help on my individual commands."

    def execute(self, nick, args, tool, sheriff):
        if args:
            for command_name in args:
                if command_name in commands:
                    self._post_command_help(nick, tool, commands[command_name])
        else:
            tool.irc().post("%s: Available commands: %s" % (nick, ", ".join(sorted(visible_commands.keys()))))
            tool.irc().post('%s: Type "%s: help COMMAND" for help on my individual commands.' % (nick, sheriff.name()))

    def _post_command_help(self, nick, tool, command):
        tool.irc().post(command.usage(nick))
        tool.irc().post(command.help(nick))
        aliases = " ".join(sorted(filter(lambda alias: commands[alias] == command and alias not in visible_commands, commands)))
        if aliases:
            tool.irc().post("%s: Aliases: %s" % (nick, aliases))


class Hi(IRCCommand):
    usage_string = "hi"
    help_string = "Responds with hi."

    def execute(self, nick, args, tool, sheriff):
        if len(args) and re.match(sheriff.name() + r'_*\s*!\s*', ' '.join(args)):
            return "%s: hi %s!" % (nick, nick)
        quips = tool.bugs.quips()
        quips.append('"Only you can prevent forest fires." -- Smokey the Bear')
        return random.choice(quips)


class PingPong(IRCCommand):
    usage_string = "ping"
    help_string = "Responds with pong."

    def execute(self, nick, args, tool, sheriff):
        return nick + ": pong"


class YouThere(IRCCommand):
    usage_string = "yt?"
    help_string = "Responds with yes."

    def execute(self, nick, args, tool, sheriff):
        return "%s: yes" % nick


class Restart(IRCCommand):
    usage_string = "restart"
    help_string = "Restarts sherrifbot.  Will update its WebKit checkout, and re-join the channel momentarily."

    def execute(self, nick, args, tool, sheriff):
        tool.irc().post("Restarting...")
        raise TerminateQueue()


class RollChromiumDEPS(IRCCommand):
    usage_string = "roll-chromium-deps REVISION"
    help_string = "Rolls WebKit's Chromium DEPS to the given revision???"

    def execute(self, nick, args, tool, sheriff):
        if not len(args):
            return self.usage(nick)
        tool.irc().post("%s: Will roll Chromium DEPS to %s" % (nick, ' '.join(args)))
        tool.irc().post("%s: Rolling Chromium DEPS to %s" % (nick, ' '.join(args)))
        tool.irc().post("%s: Rolled Chromium DEPS to %s" % (nick, ' '.join(args)))
        tool.irc().post("%s: Thank You" % nick)


class Rollout(IRCCommand):
    usage_string = "rollout SVN_REVISION [SVN_REVISIONS] REASON"
    help_string = "Opens a rollout bug, CCing author + reviewer, and attaching the reverse-diff of the given revisions marked as commit-queue=?."

    def _extract_revisions(self, arg):
        revision_list = []
        possible_revisions = arg.split(",")
        for revision in possible_revisions:
            revision = revision.strip()
            if not revision:
                continue
            revision = revision.lstrip("r")
            # If one part of the arg isn't in the correct format,
            # then none of the arg should be considered a revision.
            if not revision.isdigit():
                return None
            revision_list.append(int(revision))
        return revision_list

    def _parse_args(self, args):
        if not args:
            return (None, None)

        svn_revision_list = []
        remaining_args = args[:]
        # First process all revisions.
        while remaining_args:
            new_revisions = self._extract_revisions(remaining_args[0])
            if not new_revisions:
                break
            svn_revision_list += new_revisions
            remaining_args = remaining_args[1:]

        # Was there a revision number?
        if not len(svn_revision_list):
            return (None, None)

        # Everything left is the reason.
        rollout_reason = " ".join(remaining_args)
        return svn_revision_list, rollout_reason

    def _responsible_nicknames_from_revisions(self, tool, sheriff, svn_revision_list):
        commit_infos = map(tool.checkout().commit_info_for_revision, svn_revision_list)
        nickname_lists = map(sheriff.responsible_nicknames_from_commit_info, commit_infos)
        return sorted(set(itertools.chain(*nickname_lists)))

    def _nicks_string(self, tool, sheriff, requester_nick, svn_revision_list):
        # FIXME: _parse_args guarentees that our svn_revision_list is all numbers.
        # However, it's possible our checkout will not include one of the revisions,
        # so we may need to catch exceptions from commit_info_for_revision here.
        target_nicks = [requester_nick] + self._responsible_nicknames_from_revisions(tool, sheriff, svn_revision_list)
        return ", ".join(target_nicks)

    def _update_working_copy(self, tool):
        tool.scm().discard_local_changes()
        tool.executive.run_and_throw_if_fail(tool.deprecated_port().update_webkit_command(), quiet=True, cwd=tool.scm().checkout_root)

    def _check_diff_failure(self, error_log, tool):
        if not error_log:
            return None

        revert_failure_message_start = error_log.find("Failed to apply reverse diff for revision")
        if revert_failure_message_start == -1:
            return None

        lines = error_log[revert_failure_message_start:].split('\n')[1:]
        files = itertools.takewhile(lambda line: tool.filesystem.exists(tool.scm().absolute_path(line)), lines)
        if files:
            return "Failed to apply reverse diff for file(s): %s" % ", ".join(files)
        return None

    def execute(self, nick, args, tool, sheriff):
        svn_revision_list, rollout_reason = self._parse_args(args)

        if (not svn_revision_list or not rollout_reason):
            return self.usage(nick)

        revision_urls_string = join_with_separators([urls.view_revision_url(revision) for revision in svn_revision_list])
        tool.irc().post("%s: Preparing rollout for %s ..." % (nick, revision_urls_string))

        self._update_working_copy(tool)

        # FIXME: IRCCommand should bind to a tool and have a self._tool like Command objects do.
        # Likewise we should probably have a self._sheriff.
        nicks_string = self._nicks_string(tool, sheriff, nick, svn_revision_list)

        try:
            complete_reason = "%s (Requested by %s on %s)." % (
                rollout_reason, nick, config_irc.channel)
            bug_id = sheriff.post_rollout_patch(svn_revision_list, complete_reason)
            bug_url = tool.bugs.bug_url_for_bug_id(bug_id)
            tool.irc().post("%s: Created rollout: %s" % (nicks_string, bug_url))
        except ScriptError, e:
            tool.irc().post("%s: Failed to create rollout patch:" % nicks_string)
            diff_failure = self._check_diff_failure(e.output, tool)
            if diff_failure:
                return "%s: %s" % (nicks_string, diff_failure)
            _post_error_and_check_for_bug_url(tool, nicks_string, e)


class Whois(IRCCommand):
    usage_string = "whois SEARCH_STRING"
    help_string = "Searches known contributors and returns any matches with irc, email and full name. Wild card * permitted."

    def _full_record_and_nick(self, contributor):
        result = ''

        if contributor.irc_nicknames:
            result += ' (:%s)' % ', :'.join(contributor.irc_nicknames)

        if contributor.can_review:
            result += ' (r)'
        elif contributor.can_commit:
            result += ' (c)'

        return unicode(contributor) + result

    def execute(self, nick, args, tool, sheriff):
        if not args:
            return self.usage(nick)
        search_string = unicode(" ".join(args))
        # FIXME: We should get the ContributorList off the tool somewhere.
        contributors = CommitterList().contributors_by_search_string(search_string)
        if not contributors:
            return unicode("%s: Sorry, I don't know any contributors matching '%s'.") % (nick, search_string)
        if len(contributors) > 5:
            return unicode("%s: More than 5 contributors match '%s', could you be more specific?") % (nick, search_string)
        if len(contributors) == 1:
            contributor = contributors[0]
            if not contributor.irc_nicknames:
                return unicode("%s: %s hasn't told me their nick. Boo hoo :-(") % (nick, contributor)
            return unicode("%s: %s is %s. Why do you ask?") % (nick, search_string, self._full_record_and_nick(contributor))
        contributor_nicks = map(self._full_record_and_nick, contributors)
        contributors_string = join_with_separators(contributor_nicks, only_two_separator=" or ", last_separator=', or ')
        return unicode("%s: I'm not sure who you mean?  %s could be '%s'.") % (nick, contributors_string, search_string)


# FIXME: Lame.  We should have an auto-registering CommandCenter.
visible_commands = {
    "create-bug": CreateBug,
    "help": Help,
    "hi": Hi,
    "ping": PingPong,
    "restart": Restart,
    "roll-chromium-deps": RollChromiumDEPS,
    "rollout": Rollout,
    "whois": Whois,
    "yt?": YouThere,
}

# Add revert as an "easter egg" command. Why?
# revert is the same as rollout and it would be confusing to list both when
# they do the same thing. However, this command is a very natural thing for
# people to use and it seems silly to have them hunt around for "rollout" instead.
commands = visible_commands.copy()
commands["revert"] = Rollout
# "hello" Alias for "hi" command for the purposes of testing aliases
commands["hello"] = Hi
