# Copyright (C) 2009, Google Inc. All rights reserved.
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
#
# WebKit's Python module for parsing and modifying ChangeLog files

import logging
import re
from StringIO import StringIO
import textwrap

from webkitpy.common.config.committers import CommitterList
from webkitpy.common.system.filesystem import FileSystem
import webkitpy.common.config.urls as config_urls

_log = logging.getLogger(__name__)


# FIXME: parse_bug_id_from_changelog should not be a free function.
# Parse the bug ID out of a Changelog message based on the format that is
# used by prepare-ChangeLog
def parse_bug_id_from_changelog(message):
    if not message:
        return None
    match = re.search("^\s*" + config_urls.bug_url_short + "$", message, re.MULTILINE)
    if match:
        return int(match.group('bug_id'))
    match = re.search("^\s*" + config_urls.bug_url_long + "$", message, re.MULTILINE)
    if match:
        return int(match.group('bug_id'))
    # We weren't able to find a bug URL in the format used by prepare-ChangeLog. Fall back to the
    # first bug URL found anywhere in the message.
    return config_urls.parse_bug_id(message)


class ChangeLogEntry(object):
    # e.g. 2009-06-03  Eric Seidel  <eric@webkit.org>
    date_line_regexp = r'^(?P<date>\d{4}-\d{2}-\d{2})\s+(?P<authors>(?P<name>[^<]+?)\s+<(?P<email>[^<>]+)>.*?)$'

    # e.g. * Source/WebCore/page/EventHandler.cpp: Implement FooBarQuux.
    touched_files_regexp = r'^\s*\*\s*(?P<file>[A-Za-z0-9_\-\./\\]+)\s*\:'
    # e.g. (ChangeLogEntry.touched_functions): Added.
    touched_functions_regexp = r'^\s*\((?P<function>[^)]*)\):'

    # e.g. Reviewed by Darin Adler.
    # (Discard everything after the first period to match more invalid lines.)
    reviewed_by_regexp = r'^\s*((\w+\s+)+and\s+)?(Review|Rubber(\s*|-)stamp)(s|ed)?\s+([a-z]+\s+)*?by\s+(?P<reviewer>.*?)[\.,]?\s*$'

    reviewed_byless_regexp = r'^\s*((Review|Rubber(\s*|-)stamp)(s|ed)?|RS)(\s+|\s*=\s*)(?P<reviewer>([A-Z]\w+\s*)+)[\.,]?\s*$'

    reviewer_name_noise_regexp = re.compile(r"""
    (\s+((tweaked\s+)?and\s+)?(landed|committed|okayed)\s+by.+) # "landed by", "commented by", etc...
    |(^(Reviewed\s+)?by\s+) # extra "Reviewed by" or "by"
    |([(<]\s*[\w_\-\.]+@[\w_\-\.]+[>)]) # email addresses
    |([(<](https?://?bugs.)webkit.org[^>)]+[>)]) # bug url
    |("[^"]+") # wresler names like 'Sean/Shawn/Shaun' in 'Geoffrey "Sean/Shawn/Shaun" Garen'
    |('[^']+') # wresler names like "The Belly" in "Sam 'The Belly' Weinig"
    |((Mr|Ms|Dr|Mrs|Prof)\.(\s+|$))
    """, re.IGNORECASE | re.VERBOSE)

    reviewer_name_casesensitive_noise_regexp = re.compile(r"""
    ((\s+|^)(and\s+)?([a-z-]+\s+){5,}by\s+) # e.g. "and given a good once-over by"
    |(\(\s*(?!(and|[A-Z])).+\)) # any parenthesis that doesn't start with "and" or a capital letter
    |(with(\s+[a-z-]+)+) # phrases with "with no hesitation" in "Sam Weinig with no hesitation"
    """, re.VERBOSE)

    reviewer_name_noise_needing_a_backreference_regexp = re.compile(r"""
    (\S\S)\.(?:(\s.+|$)) # Text after the two word characters (don't match initials) and a period followed by a space.
    """, re.IGNORECASE | re.VERBOSE)

    nobody_regexp = re.compile(r"""(\s+|^)nobody(
    ((,|\s+-)?\s+(\w+\s+)+fix.*) # e.g. nobody, build fix...
    |(\s*\([^)]+\).*) # NOBODY (..)...
    |$)""", re.IGNORECASE | re.VERBOSE)

    # e.g. == Rolled over to ChangeLog-2011-02-16 ==
    rolled_over_regexp = r'^== Rolled over to ChangeLog-\d{4}-\d{2}-\d{2} ==$'

    # e.g. git-svn-id: http://svn.webkit.org/repository/webkit/trunk@96161 268f45cc-cd09-0410-ab3c-d52691b4dbfc
    svn_id_regexp = r'git-svn-id: http://svn.webkit.org/repository/webkit/trunk@(?P<svnid>\d+) '

    split_names_regexp = r'\s*(?:,(?:\s+and\s+|&)?|(?:^|\s+)and\s+|&&|[/+&])\s*'

    def __init__(self, contents, committer_list=CommitterList(), revision=None):
        self._contents = contents
        self._committer_list = committer_list
        self._revision = revision
        self._parse_entry()

    @classmethod
    def _parse_reviewer_text(cls, text):
        match = re.search(ChangeLogEntry.reviewed_by_regexp, text, re.MULTILINE | re.IGNORECASE)
        if not match:
            # There are cases where people omit "by". We match it only if reviewer part looked nice
            # in order to avoid matching random lines that start with Reviewed
            match = re.search(ChangeLogEntry.reviewed_byless_regexp, text, re.MULTILINE | re.IGNORECASE)
        if not match:
            return None, None

        reviewer_text = match.group("reviewer")

        reviewer_text = ChangeLogEntry.nobody_regexp.sub('', reviewer_text)
        reviewer_text = ChangeLogEntry.reviewer_name_noise_regexp.sub('', reviewer_text)
        reviewer_text = ChangeLogEntry.reviewer_name_casesensitive_noise_regexp.sub('', reviewer_text)
        reviewer_text = ChangeLogEntry.reviewer_name_noise_needing_a_backreference_regexp.sub(r'\1', reviewer_text)
        reviewer_text = reviewer_text.replace('(', '').replace(')', '')
        reviewer_text = re.sub(r'\s\s+|[,.]\s*$', ' ', reviewer_text).strip()
        if not len(reviewer_text):
            return None, None

        reviewer_list = ChangeLogEntry._split_reviewer_names(reviewer_text)

        # Get rid of "reviewers" like "even though this is just a..." in "Reviewed by Sam Weinig, even though this is just a..."
        # and "who wrote the original code" in "Noam Rosenthal, who wrote the original code"
        reviewer_list = [reviewer for reviewer in reviewer_list if not re.match('^who\s|^([a-z]+(\s+|\.|$)){6,}$', reviewer)]

        return reviewer_text, reviewer_list

    @classmethod
    def _split_reviewer_names(cls, text):
        return re.split(ChangeLogEntry.split_names_regexp, text)

    @classmethod
    def _split_author_names_with_emails(cls, text):
        regex = '>' + ChangeLogEntry.split_names_regexp
        names = re.split(regex, text)
        if len(names) > 1:
            names = [name + ">" for name in names[:-1]] + [names[-1]]
        return names

    def _fuzz_match_reviewers(self, reviewers_text_list):
        if not reviewers_text_list:
            return []
        list_of_reviewers = [self._committer_list.contributors_by_fuzzy_match(reviewer)[0] for reviewer in reviewers_text_list]
        # Flatten lists and get rid of any reviewers with more than one candidate.
        return [reviewers[0] for reviewers in list_of_reviewers if len(reviewers) == 1]

    @classmethod
    def _parse_author_name_and_email(cls, author_name_and_email):
        match = re.match(r'(?P<name>.+?)\s+<(?P<email>[^>]+)>', author_name_and_email)
        return {'name': match.group("name"), 'email': match.group("email")}

    @classmethod
    def _parse_author_text(cls, text):
        if not text:
            return []
        authors = cls._split_author_names_with_emails(text)
        assert(authors and len(authors) >= 1)
        return [cls._parse_author_name_and_email(author) for author in authors]

    @classmethod
    def _parse_touched_functions(cls, text):
        result = {}
        cur_file = None
        for line in text.splitlines():
            file_match = re.match(cls.touched_files_regexp, line)
            if file_match:
                cur_file = file_match.group("file")
                result[cur_file] = []
            func_match = re.match(cls.touched_functions_regexp, line)
            if func_match and cur_file:
                result[cur_file].append(func_match.group("function"))
        return result

    @classmethod
    def _parse_bug_description(cls, text):
        # If line 4 is a bug url, line 3 is the bug description.
        # It's too hard to guess in other cases, so we return None.
        lines = text.splitlines()
        if len(lines) < 4:
            return None
        for bug_url in (config_urls.bug_url_short, config_urls.bug_url_long):
            if re.match("^\s*" + bug_url + "$", lines[3]):
                return lines[2].strip()
        return None

    def _parse_entry(self):
        match = re.match(self.date_line_regexp, self._contents, re.MULTILINE)
        if not match:
            _log.warning("Creating invalid ChangeLogEntry:\n%s" % self._contents)

        self._date_line = match.group()
        self._date = match.group("date")
        self._bug_description = self._parse_bug_description(self._contents)

        # FIXME: group("name") does not seem to be Unicode?  Probably due to self._contents not being unicode.
        self._author_text = match.group("authors") if match else None
        self._authors = ChangeLogEntry._parse_author_text(self._author_text)

        self._reviewer_text, self._reviewers_text_list = ChangeLogEntry._parse_reviewer_text(self._contents)
        self._reviewers = self._fuzz_match_reviewers(self._reviewers_text_list)
        self._author = self._committer_list.contributor_by_email(self.author_email()) or self._committer_list.contributor_by_name(self.author_name())

        self._touched_files = re.findall(self.touched_files_regexp, self._contents, re.MULTILINE)
        self._touched_functions = self._parse_touched_functions(self._contents)

    def date_line(self):
        return self._date_line

    def date(self):
        return self._date

    def author_text(self):
        return self._author_text

    def revision(self):
        return self._revision

    def author_name(self):
        return self._authors[0]['name']

    def author_email(self):
        return self._authors[0]['email']

    def author(self):
        return self._author  # Might be None

    def authors(self):
        return self._authors

    # FIXME: Eventually we would like to map reviwer names to reviewer objects.
    # See https://bugs.webkit.org/show_bug.cgi?id=26533
    def reviewer_text(self):
        return self._reviewer_text

    # Might be None, might also not be a Reviewer!
    def reviewer(self):
        return self._reviewers[0] if len(self._reviewers) > 0 else None

    def reviewers(self):
        return self._reviewers

    def has_valid_reviewer(self):
        if self._reviewers_text_list:
            for reviewer in self._reviewers_text_list:
                reviewer = self._committer_list.committer_by_name(reviewer)
                if reviewer:
                    return True
        return bool(re.search("unreviewed", self._contents, re.IGNORECASE))

    def contents(self):
        return self._contents

    def bug_id(self):
        return parse_bug_id_from_changelog(self._contents)

    def bug_description(self):
        return self._bug_description

    def touched_files(self):
        return self._touched_files

    # Returns a dict from file name to lists of function names.
    def touched_functions(self):
        return self._touched_functions

    def touched_files_text(self):
        match = re.search(self.touched_files_regexp, self._contents, re.MULTILINE)
        return self._contents[match.start():].lstrip("\n\r") if match else ""

    # Determine if any text has been added to the section on touched files
    def is_touched_files_text_clean(self):
        file_line_end = r"( (Added|Removed|(Copied|Renamed) from [A-Za-z0-9_\-./\\]+).)?$"
        for line in self.touched_files_text().splitlines():
            if re.match(self.touched_files_regexp + file_line_end, line):
                continue
            if re.match(self.touched_functions_regexp + "$", line):
                continue
            return False
        return True

# FIXME: Various methods on ChangeLog should move into ChangeLogEntry instead.
class ChangeLog(object):

    def __init__(self, path, filesystem=None):
        self.path = path
        self._filesystem = filesystem or FileSystem()

    _changelog_indent = " " * 8

    @classmethod
    def parse_latest_entry_from_file(cls, changelog_file):
        try:
            return next(cls.parse_entries_from_file(changelog_file))
        except StopIteration, e:
            return None

    svn_blame_regexp = re.compile(r'^(\s*(?P<revision>\d+) [^ ]+)\s*(?P<line>.*?\n)')

    @classmethod
    def _separate_revision_and_line(cls, line):
        match = cls.svn_blame_regexp.match(line)
        if not match:
            return None, line
        return int(match.group('revision')), match.group('line')

    @classmethod
    def parse_entries_from_file(cls, changelog_file):
        """changelog_file must be a file-like object which returns
        unicode strings, e.g. from StringIO(unicode()) or
        fs.open_text_file_for_reading()"""
        date_line_regexp = re.compile(ChangeLogEntry.date_line_regexp)
        rolled_over_regexp = re.compile(ChangeLogEntry.rolled_over_regexp)

        # The first line should be a date line.
        revision, first_line = cls._separate_revision_and_line(changelog_file.readline())
        assert(isinstance(first_line, unicode))
        if not date_line_regexp.match(cls.svn_blame_regexp.sub('', first_line)):
            raise StopIteration

        entry_lines = [first_line]
        revisions_in_entry = {revision: 1} if revision != None else None
        for line in changelog_file:
            if revisions_in_entry:
                revision, line = cls._separate_revision_and_line(line)

            if rolled_over_regexp.match(line):
                break

            if date_line_regexp.match(line):
                most_probable_revision = max(revisions_in_entry, key=revisions_in_entry.__getitem__) if revisions_in_entry else None
                # Remove the extra newline at the end
                yield ChangeLogEntry(''.join(entry_lines[:-1]), revision=most_probable_revision)
                entry_lines = []
                revisions_in_entry = {revision: 0}

            entry_lines.append(line)
            if revisions_in_entry:
                revisions_in_entry[revision] = revisions_in_entry.get(revision, 0) + 1

        most_probable_revision = max(revisions_in_entry, key=revisions_in_entry.__getitem__) if revisions_in_entry else None
        yield ChangeLogEntry(''.join(entry_lines[:-1]), revision=most_probable_revision)

    def latest_entry(self):
        # ChangeLog files are always UTF-8, we read them in as such to support Reviewers with unicode in their names.
        changelog_file = self._filesystem.open_text_file_for_reading(self.path)
        try:
            return self.parse_latest_entry_from_file(changelog_file)
        finally:
            changelog_file.close()

    # _wrap_line and _wrap_lines exist to work around
    # http://bugs.python.org/issue1859

    def _wrap_line(self, line):
        return textwrap.fill(line,
                             width=70,
                             initial_indent=self._changelog_indent,
                             # Don't break urls which may be longer than width.
                             break_long_words=False,
                             subsequent_indent=self._changelog_indent)

    # Workaround as suggested by guido in
    # http://bugs.python.org/issue1859#msg60040

    def _wrap_lines(self, message):
        lines = [self._wrap_line(line) for line in message.splitlines()]
        return "\n".join(lines)

    def update_with_unreviewed_message(self, message):
        first_boilerplate_line_regexp = re.compile(
                "%sNeed a short description \(OOPS!\)\." % self._changelog_indent)
        removing_boilerplate = False
        result = StringIO()
        with self._filesystem.open_text_file_for_reading(self.path) as file:
            for line in file:
                if first_boilerplate_line_regexp.search(line):
                    message_lines = self._wrap_lines(message)
                    result.write(first_boilerplate_line_regexp.sub(message_lines, line))
                    # Remove all the ChangeLog boilerplate before the first changed
                    # file.
                    removing_boilerplate = True
                elif removing_boilerplate:
                    if line.find('*') >= 0:  # each changed file is preceded by a *
                        removing_boilerplate = False

                if not removing_boilerplate:
                    result.write(line)
        self._filesystem.write_text_file(self.path, result.getvalue())

    def set_reviewer(self, reviewer):
        latest_entry = self.latest_entry()
        latest_entry_contents = latest_entry.contents()
        reviewer_text = latest_entry.reviewer()
        found_nobody = re.search("NOBODY\s*\(OOPS!\)", latest_entry_contents, re.MULTILINE)

        if not found_nobody and not reviewer_text:
            bug_url_number_of_items = len(re.findall(config_urls.bug_url_long, latest_entry_contents, re.MULTILINE))
            bug_url_number_of_items += len(re.findall(config_urls.bug_url_short, latest_entry_contents, re.MULTILINE))
            result = StringIO()
            with self._filesystem.open_text_file_for_reading(self.path) as file:
                for line in file:
                    found_bug_url = re.search(config_urls.bug_url_long, line)
                    if not found_bug_url:
                        found_bug_url = re.search(config_urls.bug_url_short, line)
                    result.write(line)
                    if found_bug_url:
                        if bug_url_number_of_items == 1:
                            result.write("\n        Reviewed by %s.\n" % reviewer)
                        bug_url_number_of_items -= 1
            self._filesystem.write_text_file(self.path, result.getvalue())
        else:
            data = self._filesystem.read_text_file(self.path)
            newdata = data.replace("NOBODY (OOPS!)", reviewer)
            self._filesystem.write_text_file(self.path, newdata)

    def set_short_description_and_bug_url(self, short_description, bug_url):
        message = "%s\n%s%s" % (short_description, self._changelog_indent, bug_url)
        bug_boilerplate = "%sNeed the bug URL (OOPS!).\n" % self._changelog_indent
        result = StringIO()
        with self._filesystem.open_text_file_for_reading(self.path) as file:
            for line in file:
                line = line.replace("Need a short description (OOPS!).", message)
                if line != bug_boilerplate:
                    result.write(line)
        self._filesystem.write_text_file(self.path, result.getvalue())

    def delete_entries(self, num_entries):
        date_line_regexp = re.compile(ChangeLogEntry.date_line_regexp)
        rolled_over_regexp = re.compile(ChangeLogEntry.rolled_over_regexp)
        entries = 0
        result = StringIO()
        with self._filesystem.open_text_file_for_reading(self.path) as file:
            for line in file:
                if date_line_regexp.match(line):
                    entries += 1
                elif rolled_over_regexp.match(line):
                    entries = num_entries + 1
                if entries > num_entries:
                    result.write(line)
        self._filesystem.write_text_file(self.path, result.getvalue())

    def prepend_text(self, text):
        data = self._filesystem.read_text_file(self.path)
        self._filesystem.write_text_file(self.path, text + data)
