# Copyright (C) 2011 Patrick Gansterer <paroga@paroga.com>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
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

"""Checks WebKit style for ChangeLog files."""

import re
from common import TabChecker
from webkitpy.common.checkout.changelog import parse_bug_id_from_changelog


class ChangeLogChecker(object):
    """Processes text lines for checking style."""

    categories = set(['changelog/bugnumber', 'changelog/filechangedescriptionwhitespace'])

    def __init__(self, file_path, handle_style_error, should_line_be_checked):
        self.file_path = file_path
        self.handle_style_error = handle_style_error
        self.should_line_be_checked = should_line_be_checked
        self._tab_checker = TabChecker(file_path, handle_style_error)

    def check_entry(self, first_line_checked, entry_lines):
        if not entry_lines:
            return
        for line in entry_lines:
            if parse_bug_id_from_changelog(line):
                break
            if re.search("Unreviewed", line, re.IGNORECASE):
                break
            if re.search("build", line, re.IGNORECASE) and re.search("fix", line, re.IGNORECASE):
                break
        else:
            self.handle_style_error(first_line_checked,
                                    "changelog/bugnumber", 5,
                                    "ChangeLog entry has no bug number")
        # check file change descriptions for style violations
        line_no = first_line_checked - 1
        for line in entry_lines:
            line_no = line_no + 1
            # filter file change descriptions
            if not re.match('\s*\*\s', line):
                continue
            if re.search(':\s*$', line) or re.search(':\s', line):
                continue
            self.handle_style_error(line_no,
                                    "changelog/filechangedescriptionwhitespace", 5,
                                    "Need whitespace between colon and description")

        # check for a lingering "No new tests. (OOPS!)" left over from prepare-changeLog.
        line_no = first_line_checked - 1
        for line in entry_lines:
            line_no = line_no + 1
            if re.match('\s*No new tests. \(OOPS!\)$', line):
                self.handle_style_error(line_no,
                                        "changelog/nonewtests", 5,
                                        "You should remove the 'No new tests' and either add and list tests, or explain why no new tests were possible.")

    def check(self, lines):
        self._tab_checker.check(lines)
        first_line_checked = 0
        entry_lines = []

        for line_index, line in enumerate(lines):
            if not self.should_line_be_checked(line_index + 1):
                # If we transitioned from finding changed lines to
                # unchanged lines, then we are done.
                if first_line_checked:
                    break
                continue
            if not first_line_checked:
                first_line_checked = line_index + 1
            entry_lines.append(line)

        self.check_entry(first_line_checked, entry_lines)
