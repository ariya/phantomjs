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

import logging
import re
import sys

from webkitpy.tool.multicommandtool import Command

_log = logging.getLogger(__name__)


class OpenBugs(Command):
    name = "open-bugs"
    help_text = "Finds all bug numbers passed in arguments (or stdin if no args provided) and opens them in a web browser"

    bug_number_regexp = re.compile(r"\b\d{4,6}\b")

    def _open_bugs(self, bug_ids):
        for bug_id in bug_ids:
            bug_url = self._tool.bugs.bug_url_for_bug_id(bug_id)
            self._tool.user.open_url(bug_url)

    # _find_bugs_in_string mostly exists for easy unit testing.
    def _find_bugs_in_string(self, string):
        return self.bug_number_regexp.findall(string)

    def _find_bugs_in_iterable(self, iterable):
        return sum([self._find_bugs_in_string(string) for string in iterable], [])

    def execute(self, options, args, tool):
        if args:
            bug_ids = self._find_bugs_in_iterable(args)
        else:
            # This won't open bugs until stdin is closed but could be made to easily.  That would just make unit testing slightly harder.
            bug_ids = self._find_bugs_in_iterable(sys.stdin)

        _log.info("%s bugs found in input." % len(bug_ids))

        self._open_bugs(bug_ids)
