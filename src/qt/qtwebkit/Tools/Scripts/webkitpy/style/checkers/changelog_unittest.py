# Copyright (C) 2010 Apple Inc. All rights reserved.
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
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Unit test for changelog.py."""

import changelog
import unittest2 as unittest


class ChangeLogCheckerTest(unittest.TestCase):
    """Tests ChangeLogChecker class."""

    def assert_no_error(self, lines_to_check, changelog_data):
        def handle_style_error(line_number, category, confidence, message):
            self.fail('Unexpected error: %d %s %d %s for\n%s' % (line_number, category, confidence, message, changelog_data))
        self.lines_to_check = set(lines_to_check)
        checker = changelog.ChangeLogChecker('ChangeLog', handle_style_error, self.mock_should_line_be_checked)
        checker.check(changelog_data.split('\n'))

    def assert_error(self, expected_line_number, lines_to_check, expected_category, changelog_data):
        self.had_error = False

        def handle_style_error(line_number, category, confidence, message):
            self.had_error = True
            self.assertEqual(expected_line_number, line_number)
            self.assertEqual(expected_category, category)
        self.lines_to_check = set(lines_to_check)
        checker = changelog.ChangeLogChecker('ChangeLog', handle_style_error, self.mock_should_line_be_checked)
        checker.check(changelog_data.split('\n'))
        self.assertTrue(self.had_error)

    def mock_handle_style_error(self):
        pass

    def mock_should_line_be_checked(self, line_number):
        return line_number in self.lines_to_check

    def test_init(self):
        checker = changelog.ChangeLogChecker('ChangeLog', self.mock_handle_style_error, self.mock_should_line_be_checked)
        self.assertEqual(checker.file_path, 'ChangeLog')
        self.assertEqual(checker.handle_style_error, self.mock_handle_style_error)
        self.assertEqual(checker.should_line_be_checked, self.mock_should_line_be_checked)

    def test_missing_bug_number(self):
        self.assert_error(1, range(1, 20), 'changelog/bugnumber',
                          '2011-01-01  Patrick Gansterer  <paroga@paroga.com>\n'
                          '\n'
                          '        Example bug\n')
        self.assert_error(1, range(1, 20), 'changelog/bugnumber',
                          '2011-01-01  Patrick Gansterer  <paroga@paroga.com>\n'
                          '\n'
                          '        Example bug\n'
                          '        http://bugs.webkit.org/show_bug.cgi?id=\n')
        self.assert_error(1, range(1, 20), 'changelog/bugnumber',
                          '2011-01-01  Patrick Gansterer  <paroga@paroga.com>\n'
                          '\n'
                          '        Example bug\n'
                          '        https://bugs.webkit.org/show_bug.cgi?id=\n')
        self.assert_error(1, range(1, 20), 'changelog/bugnumber',
                          '2011-01-01  Patrick Gansterer  <paroga@paroga.com>\n'
                          '\n'
                          '        Example bug\n'
                          '        http://webkit.org/b/\n')
        self.assert_error(1, range(1, 20), 'changelog/bugnumber',
                          '2011-01-01  Patrick Gansterer  <paroga@paroga.com>\n'
                          '\n'
                          '        Example bug'
                          '\n'
                          '        http://trac.webkit.org/changeset/12345\n')
        self.assert_error(2, range(2, 5), 'changelog/bugnumber',
                          '2011-01-01  Patrick Gansterer  <paroga@paroga.com>\n'
                          '        Example bug\n'
                          '        https://bugs.webkit.org/show_bug.cgi\n'
                          '\n'
                          '2011-01-01  Patrick Gansterer  <paroga@paroga.com>\n'
                          '        Another change\n')
        self.assert_error(2, range(2, 6), 'changelog/bugnumber',
                          '2011-01-01  Patrick Gansterer  <paroga@paroga.com>\n'
                          '        Example bug\n'
                          '        More text about bug.\n'
                          '\n'
                          '2011-01-01  Patrick Gansterer  <paroga@paroga.com>\n'
                          '\n'
                          '        No bug in this change.\n')

    def test_file_descriptions(self):
        self.assert_error(5, range(1, 20), 'changelog/filechangedescriptionwhitespace',
                          '2011-01-01 Dmitry Lomov  <dslomov@google.com>\n'
                          '        ExampleBug\n'
                          '        http://bugs.webkit.org/show_bug.cgi?id=12345\n'
                          '\n'
                          '        *  Source/Tools/random-script.py:Fixed')
        self.assert_error(6, range(1, 20), 'changelog/filechangedescriptionwhitespace',
                          '2011-01-01 Dmitry Lomov  <dslomov@google.com>\n'
                          '        ExampleBug\n'
                          '        http://bugs.webkit.org/show_bug.cgi?id=12345\n'
                          '\n'
                          '        *  Source/Tools/another-file: Done\n'
                          '        *  Source/Tools/random-script.py:Fixed\n'
                          '        *  Source/Tools/one-morefile:\n')

    def test_no_new_tests(self):
        self.assert_error(5, range(1, 20), 'changelog/nonewtests',
                          '2011-01-01 Dmitry Lomov  <dslomov@google.com>\n'
                          '        ExampleBug\n'
                          '        http://bugs.webkit.org/show_bug.cgi?id=12345\n'
                          '\n'
                          '        No new tests. (OOPS!)\n'
                          '        *  Source/Tools/random-script.py: Fixed')

    def test_no_error(self):
        self.assert_no_error([],
                             '2011-01-01  Patrick Gansterer  <paroga@paroga.com>\n'
                             '\n'
                             '        Example ChangeLog entry out of range\n'
                             '        http://example.com/\n')
        self.assert_no_error([],
                             '2011-01-01  Patrick Gansterer  <paroga@paroga.com>\n'
                             '\n'
                             '        Example bug\n'
                             '        http://bugs.webkit.org/show_bug.cgi?id=12345\n')
        self.assert_no_error(range(1, 20),
                             '2011-01-01  Patrick Gansterer  <paroga@paroga.com>\n'
                             '\n'
                             '        Example bug\n'
                             '        http://bugs.webkit.org/show_bug.cgi?id=12345\n')
        self.assert_no_error(range(1, 20),
                             '2011-01-01  Patrick Gansterer  <paroga@paroga.com>\n'
                             '\n'
                             '        Example bug\n'
                             '        https://bugs.webkit.org/show_bug.cgi?id=12345\n')
        self.assert_no_error(range(1, 20),
                             '2011-01-01  Patrick Gansterer  <paroga@paroga.com>\n'
                             '\n'
                             '        Example bug\n'
                             '        http://webkit.org/b/12345\n')
        self.assert_no_error(range(1, 20),
                             '2011-01-01  Patrick Gansterer  <paroga@paroga.com>\n'
                             '\n'
                             '        Unreview build fix for r12345.\n')
        self.assert_no_error(range(1, 20),
                             '2011-01-01  Patrick Gansterer  <paroga@paroga.com>\n'
                             '\n'
                             '        Fix build after a bad change.\n')
        self.assert_no_error(range(1, 20),
                             '2011-01-01  Patrick Gansterer  <paroga@paroga.com>\n'
                             '\n'
                             '        Fix example port build.\n')
        self.assert_no_error(range(2, 6),
                             '2011-01-01  Patrick Gansterer  <paroga@paroga.com>\n'
                             '        Example bug\n'
                             '        https://bugs.webkit.org/show_bug.cgi?id=12345\n'
                             '\n'
                             '2011-01-01  Patrick Gansterer  <paroga@paroga.com>\n'
                             '        No bug here!\n')
        self.assert_no_error(range(1, 20),
                             '2011-01-01  Patrick Gansterer  <paroga@paroga.com>\n'
                             '        Example bug\n'
                             '        https://bugs.webkit.org/show_bug.cgi?id=12345\n'
                             '        * Source/WebKit/foo.cpp:    \n'
                             '        * Source/WebKit/bar.cpp:\n'
                             '        * Source/WebKit/foobar.cpp: Description\n')
