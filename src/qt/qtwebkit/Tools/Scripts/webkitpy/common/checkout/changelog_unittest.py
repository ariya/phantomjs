# Copyright (C) 2009 Google Inc. All rights reserved.
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

from StringIO import StringIO

from webkitpy.common.system.filesystem_mock import MockFileSystem
from webkitpy.common.checkout.changelog import *


class ChangeLogTest(unittest.TestCase):

    _changelog_path = 'Tools/ChangeLog'

    _example_entry = u'''2009-08-17  Peter Kasting  <pkasting@google.com>

        Reviewed by Tor Arne Vestb\xf8.

        https://bugs.webkit.org/show_bug.cgi?id=27323
        Only add Cygwin to the path when it isn't already there.  This avoids
        causing problems for people who purposefully have non-Cygwin versions of
        executables like svn in front of the Cygwin ones in their paths.

        * DumpRenderTree/win/DumpRenderTree.vcproj:
        * DumpRenderTree/win/ImageDiff.vcproj:
        * DumpRenderTree/win/TestNetscapePlugin/TestNetscapePlugin.vcproj:
'''

    _rolled_over_footer = '== Rolled over to ChangeLog-2009-06-16 =='

    # More example text than we need.  Eventually we need to support parsing this all and write tests for the parsing.
    _example_changelog = u"""2009-08-17  Tor Arne Vestb\xf8  <vestbo@webkit.org>

        <http://webkit.org/b/28393> check-webkit-style: add check for use of std::max()/std::min() instead of MAX()/MIN()

        Reviewed by David Levin.

        * Scripts/modules/cpp_style.py:
        (_ERROR_CATEGORIES): Added 'runtime/max_min_macros'.
        (check_max_min_macros): Added.  Returns level 4 error when MAX()
        and MIN() macros are used in header files and C++ source files.
        (check_style): Added call to check_max_min_macros().
        * Scripts/modules/cpp_style_unittest.py: Added unit tests.
        (test_max_macro): Added.
        (test_min_macro): Added.

2009-08-16  David Kilzer  <ddkilzer@apple.com>

        Backed out r47343 which was mistakenly committed

        * Scripts/bugzilla-tool:
        * Scripts/modules/scm.py:

2009-06-18  Darin Adler  <darin@apple.com>

        Rubber stamped by Mark Rowe.

        * DumpRenderTree/mac/DumpRenderTreeWindow.mm:
        (-[DumpRenderTreeWindow close]): Resolved crashes seen during regression
        tests. The close method can be called on a window that's already closed
        so we can't assert here.

2011-11-04  Benjamin Poulain  <bpoulain@apple.com>

        [Mac] ResourceRequest's nsURLRequest() does not differentiate null and empty URLs with CFNetwork
        https://bugs.webkit.org/show_bug.cgi?id=71539

        Reviewed by David Kilzer.

        In order to have CFURL and NSURL to be consistent when both are used on Mac,
        KURL::createCFURL() is changed to support empty URL values.

        * This change log entry is made up to test _parse_entry:
            * a list of things

        * platform/cf/KURLCFNet.cpp:
        (WebCore::createCFURLFromBuffer):
        (WebCore::KURL::createCFURL):
        * platform/mac/KURLMac.mm :
        (WebCore::KURL::operator NSURL *):
        (WebCore::KURL::createCFURL):
        * WebCoreSupport/ChromeClientEfl.cpp:
        (WebCore::ChromeClientEfl::closeWindowSoon): call new function and moves its
        previous functionality there.
        * ewk/ewk_private.h:
        * ewk/ewk_view.cpp:

2011-03-02  Carol Szabo  <carol.szabo@nokia.com>

        Reviewed by David Hyatt  <hyatt@apple.com>

        content property doesn't support quotes
        https://bugs.webkit.org/show_bug.cgi?id=6503

        Added full support for quotes as defined by CSS 2.1.

        Tests: fast/css/content/content-quotes-01.html
               fast/css/content/content-quotes-02.html
               fast/css/content/content-quotes-03.html
               fast/css/content/content-quotes-04.html
               fast/css/content/content-quotes-05.html
               fast/css/content/content-quotes-06.html

2011-03-31  Brent Fulgham  <bfulgham@webkit.org>

       Reviewed Adam Roben.

       [WinCairo] Implement Missing drawWindowsBitmap method.
       https://bugs.webkit.org/show_bug.cgi?id=57409

2011-03-28  Dirk Pranke  <dpranke@chromium.org>

       RS=Tony Chang.

       r81977 moved FontPlatformData.h from
       WebCore/platform/graphics/cocoa to platform/graphics. This
       change updates the chromium build accordingly.

       https://bugs.webkit.org/show_bug.cgi?id=57281

       * platform/graphics/chromium/CrossProcessFontLoading.mm:

2011-05-04  Alexis Menard  <alexis.menard@openbossa.org>

       Unreviewed warning fix.

       The variable is just used in the ASSERT macro. Let's use ASSERT_UNUSED to avoid
       a warning in Release build.

       * accessibility/AccessibilityRenderObject.cpp:
       (WebCore::lastChildConsideringContinuation):

2011-10-11  Antti Koivisto  <antti@apple.com>

       Resolve regular and visited link style in a single pass
       https://bugs.webkit.org/show_bug.cgi?id=69838

       Reviewed by Darin Adler

       We can simplify and speed up selector matching by removing the recursive matching done
       to generate the style for the :visited pseudo selector. Both regular and visited link style
       can be generated in a single pass through the style selector.

== Rolled over to ChangeLog-2009-06-16 ==
"""

    def test_parse_bug_id_from_changelog(self):
        commit_text = '''
2011-03-23  Ojan Vafai  <ojan@chromium.org>

        Add failing result for WebKit2. All tests that require
        focus fail on WebKit2. See https://bugs.webkit.org/show_bug.cgi?id=56988.

        * platform/mac-wk2/fast/css/pseudo-any-expected.txt: Added.

        '''

        self.assertEqual(56988, parse_bug_id_from_changelog(commit_text))

        commit_text = '''
2011-03-23  Ojan Vafai  <ojan@chromium.org>

        Add failing result for WebKit2. All tests that require
        focus fail on WebKit2. See https://bugs.webkit.org/show_bug.cgi?id=56988.
        https://bugs.webkit.org/show_bug.cgi?id=12345

        * platform/mac-wk2/fast/css/pseudo-any-expected.txt: Added.

        '''

        self.assertEqual(12345, parse_bug_id_from_changelog(commit_text))

        commit_text = '''
2011-03-31  Adam Roben  <aroben@apple.com>

        Quote the executable path we pass to ::CreateProcessW

        This will ensure that spaces in the path will be interpreted correctly.

        Fixes <http://webkit.org/b/57569> Web process sometimes fails to launch when there are
        spaces in its path

        Reviewed by Steve Falkenburg.

        * UIProcess/Launcher/win/ProcessLauncherWin.cpp:
        (WebKit::ProcessLauncher::launchProcess): Surround the executable path in quotes.

        '''

        self.assertEqual(57569, parse_bug_id_from_changelog(commit_text))

        commit_text = '''
2011-03-29  Timothy Hatcher  <timothy@apple.com>

        Update WebCore Localizable.strings to contain WebCore, WebKit/mac and WebKit2 strings.

        https://webkit.org/b/57354

        Reviewed by Sam Weinig.

        * English.lproj/Localizable.strings: Updated.
        * StringsNotToBeLocalized.txt: Removed. To hard to maintain in WebCore.
        * platform/network/cf/LoaderRunLoopCF.h: Remove a single quote in an #error so
        extract-localizable-strings does not complain about unbalanced single quotes.
        '''

        self.assertEqual(57354, parse_bug_id_from_changelog(commit_text))

    def test_parse_log_entries_from_changelog(self):
        changelog_file = StringIO(self._example_changelog)
        parsed_entries = list(ChangeLog.parse_entries_from_file(changelog_file))
        self.assertEqual(len(parsed_entries), 9)
        self.assertEqual(parsed_entries[0].date_line(), u"2009-08-17  Tor Arne Vestb\xf8  <vestbo@webkit.org>")
        self.assertEqual(parsed_entries[0].date(), "2009-08-17")
        self.assertEqual(parsed_entries[0].reviewer_text(), "David Levin")
        self.assertEqual(parsed_entries[0].is_touched_files_text_clean(), False)
        self.assertEqual(parsed_entries[1].date_line(), "2009-08-16  David Kilzer  <ddkilzer@apple.com>")
        self.assertEqual(parsed_entries[1].date(), "2009-08-16")
        self.assertEqual(parsed_entries[1].author_email(), "ddkilzer@apple.com")
        self.assertEqual(parsed_entries[1].touched_files_text(), "        * Scripts/bugzilla-tool:\n        * Scripts/modules/scm.py:\n")
        self.assertEqual(parsed_entries[1].is_touched_files_text_clean(), True)
        self.assertEqual(parsed_entries[2].reviewer_text(), "Mark Rowe")
        self.assertEqual(parsed_entries[2].touched_files(), ["DumpRenderTree/mac/DumpRenderTreeWindow.mm"])
        self.assertEqual(parsed_entries[2].touched_functions(), {"DumpRenderTree/mac/DumpRenderTreeWindow.mm": ["-[DumpRenderTreeWindow close]"]})
        self.assertEqual(parsed_entries[2].is_touched_files_text_clean(), False)
        self.assertEqual(parsed_entries[3].author_name(), "Benjamin Poulain")
        self.assertEqual(parsed_entries[3].touched_files(), ["platform/cf/KURLCFNet.cpp", "platform/mac/KURLMac.mm",
            "WebCoreSupport/ChromeClientEfl.cpp", "ewk/ewk_private.h", "ewk/ewk_view.cpp"])
        self.assertEqual(parsed_entries[3].touched_functions(), {"platform/cf/KURLCFNet.cpp": ["WebCore::createCFURLFromBuffer", "WebCore::KURL::createCFURL"],
            "platform/mac/KURLMac.mm": ["WebCore::KURL::operator NSURL *", "WebCore::KURL::createCFURL"],
            "WebCoreSupport/ChromeClientEfl.cpp": ["WebCore::ChromeClientEfl::closeWindowSoon"], "ewk/ewk_private.h": [], "ewk/ewk_view.cpp": []})
        self.assertEqual(parsed_entries[3].bug_description(), "[Mac] ResourceRequest's nsURLRequest() does not differentiate null and empty URLs with CFNetwork")
        self.assertEqual(parsed_entries[4].reviewer_text(), "David Hyatt")
        self.assertIsNone(parsed_entries[4].bug_description())
        self.assertEqual(parsed_entries[5].reviewer_text(), "Adam Roben")
        self.assertEqual(parsed_entries[6].reviewer_text(), "Tony Chang")
        self.assertIsNone(parsed_entries[7].reviewer_text())
        self.assertEqual(parsed_entries[8].reviewer_text(), 'Darin Adler')

    def test_parse_log_entries_from_annotated_file(self):
        # Note that there are trailing spaces on some of the lines intentionally.
        changelog_file = StringIO(u"100000 ossy@webkit.org 2011-11-11  Csaba Osztrogon\u00e1c  <ossy@webkit.org>\n"
            u"100000 ossy@webkit.org\n"
            u"100000 ossy@webkit.org         100,000 !!!\n"
            u"100000 ossy@webkit.org \n"
            u"100000 ossy@webkit.org         Reviewed by Zoltan Herczeg.\n"
            u"100000 ossy@webkit.org \n"
            u"100000 ossy@webkit.org         * ChangeLog: Point out revision 100,000.\n"
            u"100000 ossy@webkit.org \n"
            u"93798 ap@apple.com 2011-08-25  Alexey Proskuryakov  <ap@apple.com>\n"
            u"93798 ap@apple.com \n"
            u"93798 ap@apple.com         Fix build when GCC 4.2 is not installed.\n"
            u"93798 ap@apple.com \n"
            u"93798 ap@apple.com         * gtest/xcode/Config/CompilerVersion.xcconfig: Copied from Source/WebCore/Configurations/CompilerVersion.xcconfig.\n"
            u"93798 ap@apple.com         * gtest/xcode/Config/General.xcconfig:\n"
            u"93798 ap@apple.com         Use the same compiler version as other projects do.\n"
            u"93798 ap@apple.com\n"
            u"99491 andreas.kling@nokia.com 2011-11-03  Andreas Kling  <kling@webkit.org>\n"
            u"99491 andreas.kling@nokia.com \n"
            u"99190 andreas.kling@nokia.com         Unreviewed build fix, sigh.\n"
            u"99190 andreas.kling@nokia.com \n"
            u"99190 andreas.kling@nokia.com         * css/CSSFontFaceRule.h:\n"
            u"99190 andreas.kling@nokia.com         * css/CSSMutableStyleDeclaration.h:\n"
            u"99190 andreas.kling@nokia.com\n"
            u"99190 andreas.kling@nokia.com 2011-11-03  Andreas Kling  <kling@webkit.org>\n"
            u"99190 andreas.kling@nokia.com \n"
            u"99187 andreas.kling@nokia.com         Unreviewed build fix, out-of-line StyleSheet::parentStyleSheet()\n"
            u"99187 andreas.kling@nokia.com         again since there's a cycle in the includes between CSSRule/StyleSheet.\n"
            u"99187 andreas.kling@nokia.com \n"
            u"99187 andreas.kling@nokia.com         * css/StyleSheet.cpp:\n"
            u"99187 andreas.kling@nokia.com         (WebCore::StyleSheet::parentStyleSheet):\n"
            u"99187 andreas.kling@nokia.com         * css/StyleSheet.h:\n"
            u"99187 andreas.kling@nokia.com \n")

        parsed_entries = list(ChangeLog.parse_entries_from_file(changelog_file))
        self.assertEqual(parsed_entries[0].revision(), 100000)
        self.assertEqual(parsed_entries[0].reviewer_text(), "Zoltan Herczeg")
        self.assertEqual(parsed_entries[0].author_name(), u"Csaba Osztrogon\u00e1c")
        self.assertEqual(parsed_entries[0].author_email(), "ossy@webkit.org")
        self.assertEqual(parsed_entries[1].revision(), 93798)
        self.assertEqual(parsed_entries[1].author_name(), "Alexey Proskuryakov")
        self.assertEqual(parsed_entries[2].revision(), 99190)
        self.assertEqual(parsed_entries[2].author_name(), "Andreas Kling")
        self.assertEqual(parsed_entries[3].revision(), 99187)
        self.assertEqual(parsed_entries[3].author_name(), "Andreas Kling")

    def _assert_parse_reviewer_text_and_list(self, text, expected_reviewer_text, expected_reviewer_text_list=None):
        reviewer_text, reviewer_text_list = ChangeLogEntry._parse_reviewer_text(text)
        self.assertEqual(reviewer_text, expected_reviewer_text)
        if expected_reviewer_text_list:
            self.assertEqual(reviewer_text_list, expected_reviewer_text_list)
        else:
            self.assertEqual(reviewer_text_list, [expected_reviewer_text])

    def _assert_parse_reviewer_text_list(self, text, expected_reviewer_text_list):
        reviewer_text, reviewer_text_list = ChangeLogEntry._parse_reviewer_text(text)
        self.assertEqual(reviewer_text_list, expected_reviewer_text_list)

    def test_parse_reviewer_text(self):
        self._assert_parse_reviewer_text_and_list('  reviewed  by Ryosuke Niwa,   Oliver Hunt, and  Dimitri Glazkov',
            'Ryosuke Niwa, Oliver Hunt, and Dimitri Glazkov', ['Ryosuke Niwa', 'Oliver Hunt', 'Dimitri Glazkov'])
        self._assert_parse_reviewer_text_and_list('Reviewed by Brady Eidson and David Levin, landed by Brady Eidson',
            'Brady Eidson and David Levin', ['Brady Eidson', 'David Levin'])

        self._assert_parse_reviewer_text_and_list('Reviewed by Simon Fraser. Committed by Beth Dakin.', 'Simon Fraser')
        self._assert_parse_reviewer_text_and_list('Reviewed by Geoff Garen. V8 fixes courtesy of Dmitry Titov.', 'Geoff Garen')
        self._assert_parse_reviewer_text_and_list('Reviewed by Adam Roben&Dirk Schulze', 'Adam Roben&Dirk Schulze', ['Adam Roben', 'Dirk Schulze'])
        self._assert_parse_reviewer_text_and_list('Rubber stamps by Darin Adler & Sam Weinig.', 'Darin Adler & Sam Weinig', ['Darin Adler', 'Sam Weinig'])

        self._assert_parse_reviewer_text_and_list('Reviewed by adam,andy and andy adam, andy smith',
            'adam,andy and andy adam, andy smith', ['adam', 'andy', 'andy adam', 'andy smith'])

        self._assert_parse_reviewer_text_and_list('rubber stamped by Oliver Hunt (oliver@apple.com) and Darin Adler (darin@apple.com)',
            'Oliver Hunt and Darin Adler', ['Oliver Hunt', 'Darin Adler'])

        self._assert_parse_reviewer_text_and_list('rubber  Stamped by David Hyatt  <hyatt@apple.com>', 'David Hyatt')
        self._assert_parse_reviewer_text_and_list('Rubber-stamped by Antti Koivisto.', 'Antti Koivisto')
        self._assert_parse_reviewer_text_and_list('Rubberstamped by Dan Bernstein.', 'Dan Bernstein')
        self._assert_parse_reviewer_text_and_list('Reviews by Ryosuke Niwa', 'Ryosuke Niwa')
        self._assert_parse_reviewer_text_and_list('Reviews Ryosuke Niwa', 'Ryosuke Niwa')
        self._assert_parse_reviewer_text_and_list('Rubberstamp Ryosuke Niwa', 'Ryosuke Niwa')
        self._assert_parse_reviewer_text_and_list('Typed and reviewed by Alexey Proskuryakov.', 'Alexey Proskuryakov')
        self._assert_parse_reviewer_text_and_list('Reviewed and landed by Brady Eidson', 'Brady Eidson')
        self._assert_parse_reviewer_text_and_list('Reviewed by rniwa@webkit.org.', 'rniwa@webkit.org')
        self._assert_parse_reviewer_text_and_list('Reviewed by Dirk Schulze / Darin Adler.', 'Dirk Schulze / Darin Adler', ['Dirk Schulze', 'Darin Adler'])
        self._assert_parse_reviewer_text_and_list('Reviewed by Sam Weinig + Oliver Hunt.', 'Sam Weinig + Oliver Hunt', ['Sam Weinig', 'Oliver Hunt'])

        self._assert_parse_reviewer_text_list('Reviewed by Sam Weinig, and given a good once-over by Jeff Miller.', ['Sam Weinig', 'Jeff Miller'])
        self._assert_parse_reviewer_text_list(' Reviewed by Sam Weinig, even though this is just a...', ['Sam Weinig'])
        self._assert_parse_reviewer_text_list('Rubber stamped by by Gustavo Noronha Silva', ['Gustavo Noronha Silva'])
        self._assert_parse_reviewer_text_list('Rubberstamped by Noam Rosenthal, who wrote the original code.', ['Noam Rosenthal'])
        self._assert_parse_reviewer_text_list('Reviewed by Dan Bernstein (relanding of r47157)', ['Dan Bernstein'])
        self._assert_parse_reviewer_text_list('Reviewed by Geoffrey "Sean/Shawn/Shaun" Garen', ['Geoffrey Garen'])
        self._assert_parse_reviewer_text_list('Reviewed by Dave "Messy" Hyatt.', ['Dave Hyatt'])
        self._assert_parse_reviewer_text_list('Reviewed by Sam \'The Belly\' Weinig', ['Sam Weinig'])
        self._assert_parse_reviewer_text_list('Rubber-stamped by David "I\'d prefer not" Hyatt.', ['David Hyatt'])
        self._assert_parse_reviewer_text_list('Reviewed by Mr. Geoffrey Garen.', ['Geoffrey Garen'])
        self._assert_parse_reviewer_text_list('Reviewed by Darin (ages ago)', ['Darin'])
        self._assert_parse_reviewer_text_list('Reviewed by Sam Weinig (except for a few comment and header tweaks).', ['Sam Weinig'])
        self._assert_parse_reviewer_text_list('Reviewed by Sam Weinig (all but the FormDataListItem rename)', ['Sam Weinig'])
        self._assert_parse_reviewer_text_list('Reviewed by Darin Adler, tweaked and landed by Beth.', ['Darin Adler'])
        self._assert_parse_reviewer_text_list('Reviewed by Sam Weinig with no hesitation', ['Sam Weinig'])
        self._assert_parse_reviewer_text_list('Reviewed by Oliver Hunt, okayed by Darin Adler.', ['Oliver Hunt'])
        self._assert_parse_reviewer_text_list('Reviewed by Darin Adler).', ['Darin Adler'])

        # For now, we let unofficial reviewers recognized as reviewers
        self._assert_parse_reviewer_text_list('Reviewed by Sam Weinig, Anders Carlsson, and (unofficially) Adam Barth.',
            ['Sam Weinig', 'Anders Carlsson', 'Adam Barth'])

        self._assert_parse_reviewer_text_list('Reviewed by NOBODY.', None)
        self._assert_parse_reviewer_text_list('Reviewed by NOBODY - Build Fix.', None)
        self._assert_parse_reviewer_text_list('Reviewed by NOBODY, layout tests fix.', None)
        self._assert_parse_reviewer_text_list('Reviewed by NOBODY (Qt build fix pt 2).', None)
        self._assert_parse_reviewer_text_list('Reviewed by NOBODY(rollout)', None)
        self._assert_parse_reviewer_text_list('Reviewed by NOBODY (Build fix, forgot to svn add this file)', None)
        self._assert_parse_reviewer_text_list('Reviewed by nobody (trivial follow up fix), Joseph Pecoraro LGTM-ed.', None)

    def _entry_with_author(self, author_text):
        return ChangeLogEntry('''2009-08-19  AUTHOR_TEXT

            Reviewed by Ryosuke Niwa

            * Scripts/bugzilla-tool:
'''.replace("AUTHOR_TEXT", author_text))

    def _entry_with_reviewer(self, reviewer_line):
        return ChangeLogEntry('''2009-08-19  Eric Seidel  <eric@webkit.org>

            REVIEW_LINE

            * Scripts/bugzilla-tool:
'''.replace("REVIEW_LINE", reviewer_line))

    def _contributors(self, names):
        return [CommitterList().contributor_by_name(name) for name in names]

    def _assert_fuzzy_reviewer_match(self, reviewer_text, expected_text_list, expected_contributors):
        unused, reviewer_text_list = ChangeLogEntry._parse_reviewer_text(reviewer_text)
        self.assertEqual(reviewer_text_list, expected_text_list)
        self.assertEqual(self._entry_with_reviewer(reviewer_text).reviewers(), self._contributors(expected_contributors))

    def test_fuzzy_reviewer_match__none(self):
        self._assert_fuzzy_reviewer_match('Reviewed by BUILD FIX', ['BUILD FIX'], [])
        self._assert_fuzzy_reviewer_match('Reviewed by Mac build fix', ['Mac build fix'], [])

    def test_fuzzy_reviewer_match_adam_barth(self):
        self._assert_fuzzy_reviewer_match('Reviewed by Adam Barth.:w', ['Adam Barth.:w'], ['Adam Barth'])

    def test_fuzzy_reviewer_match_darin_adler_et_al(self):
        self._assert_fuzzy_reviewer_match('Reviewed by Darin Adler in <https://bugs.webkit.org/show_bug.cgi?id=47736>.', ['Darin Adler in'], ['Darin Adler'])
        self._assert_fuzzy_reviewer_match('Reviewed by Darin Adler, Dan Bernstein, Adele Peterson, and others.',
            ['Darin Adler', 'Dan Bernstein', 'Adele Peterson', 'others'], ['Darin Adler', 'Dan Bernstein', 'Adele Peterson'])

    def test_fuzzy_reviewer_match_dimitri_glazkov(self):
        self._assert_fuzzy_reviewer_match('Reviewed by Dimitri Glazkov, build fix', ['Dimitri Glazkov', 'build fix'], ['Dimitri Glazkov'])

    def test_fuzzy_reviewer_match_george_staikos(self):
        self._assert_fuzzy_reviewer_match('Reviewed by George Staikos (and others)', ['George Staikos', 'others'], ['George Staikos'])

    def test_fuzzy_reviewer_match_mark_rowe(self):
        self._assert_fuzzy_reviewer_match('Reviewed by Mark Rowe, but Dan Bernstein also reviewed and asked thoughtful questions.',
            ['Mark Rowe', 'but Dan Bernstein also reviewed', 'asked thoughtful questions'], ['Mark Rowe'])

    def test_fuzzy_reviewer_match_initial(self):
        self._assert_fuzzy_reviewer_match('Reviewed by Alejandro G. Castro.',
            ['Alejandro G. Castro'], ['Alejandro G. Castro'])
        self._assert_fuzzy_reviewer_match('Reviewed by G. Alejandro G. Castro and others.',
            ['G. Alejandro G. Castro', 'others'], ['Alejandro G. Castro'])

        # If a reviewer has a name that ended with an initial, the regular expression
        # will incorrectly trim the last period, but it will still match fuzzily to
        # the full reviewer name.
        self._assert_fuzzy_reviewer_match('Reviewed by G. Alejandro G. G. Castro G.',
            ['G. Alejandro G. G. Castro G'], ['Alejandro G. Castro'])

    def _assert_parse_authors(self, author_text, expected_contributors):
        parsed_authors = [(author['name'], author['email']) for author in self._entry_with_author(author_text).authors()]
        self.assertEqual(parsed_authors, expected_contributors)

    def test_parse_authors(self):
        self._assert_parse_authors(u'Aaron Colwell  <acolwell@chromium.org>', [(u'Aaron Colwell', u'acolwell@chromium.org')])
        self._assert_parse_authors('Eric Seidel  <eric@webkit.org>, Ryosuke Niwa  <rniwa@webkit.org>',
            [('Eric Seidel', 'eric@webkit.org'), ('Ryosuke Niwa', 'rniwa@webkit.org')])
        self._assert_parse_authors('Zan Dobersek  <zandobersek@gmail.com> and Philippe Normand  <pnormand@igalia.com>',
            [('Zan Dobersek', 'zandobersek@gmail.com'), ('Philippe Normand', 'pnormand@igalia.com')])
        self._assert_parse_authors('New Contributor  <new@webkit.org> and Noob  <noob@webkit.org>',
            [('New Contributor', 'new@webkit.org'), ('Noob', 'noob@webkit.org')])
        self._assert_parse_authors('Adam Barth  <abarth@webkit.org> && Benjamin Poulain  <bpoulain@apple.com>',
            [('Adam Barth', 'abarth@webkit.org'), ('Benjamin Poulain', 'bpoulain@apple.com')])
        self._assert_parse_authors(u'Pawe\u0142 Hajdan, Jr.  <phajdan.jr@chromium.org>',
            [(u'Pawe\u0142 Hajdan, Jr.', u'phajdan.jr@chromium.org')])
        self._assert_parse_authors(u'Pawe\u0142 Hajdan, Jr.  <phajdan.jr@chromium.org>, Adam Barth  <abarth@webkit.org>',
            [(u'Pawe\u0142 Hajdan, Jr.', u'phajdan.jr@chromium.org'), (u'Adam Barth', u'abarth@webkit.org')])

    def _assert_has_valid_reviewer(self, reviewer_line, expected):
        self.assertEqual(self._entry_with_reviewer(reviewer_line).has_valid_reviewer(), expected)

    def test_has_valid_reviewer(self):
        self._assert_has_valid_reviewer("Reviewed by Eric Seidel.", True)
        self._assert_has_valid_reviewer("Reviewed by Eric Seidel", True)  # Not picky about the '.'
        self._assert_has_valid_reviewer("Reviewed by Eric.", False)
        self._assert_has_valid_reviewer("Reviewed by Eric C Seidel.", False)
        self._assert_has_valid_reviewer("Rubber-stamped by Eric.", False)
        self._assert_has_valid_reviewer("Rubber-stamped by Eric Seidel.", True)
        self._assert_has_valid_reviewer("Rubber stamped by Eric.", False)
        self._assert_has_valid_reviewer("Rubber stamped by Eric Seidel.", True)
        self._assert_has_valid_reviewer("Unreviewed build fix.", True)

    def test_is_touched_files_text_clean(self):
        tests = [
        ('''2013-01-30  Timothy Loh  <timloh@chromium.com>

        Make ChangeLogEntry detect annotations by prepare-ChangeLog (Added/Removed/Copied from/Renamed from) as clean.
        https://bugs.webkit.org/show_bug.cgi?id=108433

        * Scripts/webkitpy/common/checkout/changelog.py:
        (ChangeLogEntry.is_touched_files_text_clean):
        * Scripts/webkitpy/common/checkout/changelog_unittest.py:
        (test_is_touched_files_text_clean):
''', True),
        ('''2013-01-10  Alan Cutter  <alancutter@chromium.org>

        Perform some file operations (automatically added comments).

        * QueueStatusServer/config/charts.py: Copied from Tools/QueueStatusServer/model/queuelog.py.
        (get_time_unit):
        * QueueStatusServer/handlers/queuecharts.py: Added.
        (QueueCharts):
        * Scripts/webkitpy/tool/bot/testdata/webkit_sheriff_0.js: Removed.
        * EWSTools/build-vm.sh: Renamed from Tools/EWSTools/cold-boot.sh.
''', True),
        ('''2013-01-30  Timothy Loh  <timloh@chromium.com>

        Add unit test (manually added comment).

        * Scripts/webkitpy/common/checkout/changelog_unittest.py:
        (test_is_touched_files_text_clean): Added.
''', False),
        ('''2013-01-30  Timothy Loh  <timloh@chromium.com>

        Add file (manually added comment).

        * Scripts/webkitpy/common/checkout/super_changelog.py: Copied from the internet.
''', False),
        ]

        for contents, expected_result in tests:
            entry = ChangeLogEntry(contents)
            self.assertEqual(entry.is_touched_files_text_clean(), expected_result)

    def test_latest_entry_parse(self):
        changelog_contents = u"%s\n%s" % (self._example_entry, self._example_changelog)
        changelog_file = StringIO(changelog_contents)
        latest_entry = ChangeLog.parse_latest_entry_from_file(changelog_file)
        self.assertEqual(latest_entry.contents(), self._example_entry)
        self.assertEqual(latest_entry.author_name(), "Peter Kasting")
        self.assertEqual(latest_entry.author_email(), "pkasting@google.com")
        self.assertEqual(latest_entry.reviewer_text(), u"Tor Arne Vestb\xf8")
        touched_files = ["DumpRenderTree/win/DumpRenderTree.vcproj", "DumpRenderTree/win/ImageDiff.vcproj", "DumpRenderTree/win/TestNetscapePlugin/TestNetscapePlugin.vcproj"]
        self.assertEqual(latest_entry.touched_files(), touched_files)
        self.assertEqual(latest_entry.touched_functions(), dict((f, []) for f in touched_files))

        self.assertTrue(latest_entry.reviewer())  # Make sure that our UTF8-based lookup of Tor works.

    def test_latest_entry_parse_single_entry(self):
        changelog_contents = u"%s\n%s" % (self._example_entry, self._rolled_over_footer)
        changelog_file = StringIO(changelog_contents)
        latest_entry = ChangeLog.parse_latest_entry_from_file(changelog_file)
        self.assertEqual(latest_entry.contents(), self._example_entry)
        self.assertEqual(latest_entry.author_name(), "Peter Kasting")

    # FIXME: We really should be getting this from prepare-ChangeLog itself.
    _new_entry_boilerplate = '''2009-08-19  Eric Seidel  <eric@webkit.org>

        Need a short description (OOPS!).
        Need the bug URL (OOPS!).

        Reviewed by NOBODY (OOPS!).

        * Scripts/bugzilla-tool:
'''

    _new_entry_boilerplate_with_bugurl = '''2009-08-19  Eric Seidel  <eric@webkit.org>

        Need a short description (OOPS!).
        https://bugs.webkit.org/show_bug.cgi?id=12345

        Reviewed by NOBODY (OOPS!).

        * Scripts/bugzilla-tool:
'''

    _new_entry_boilerplate_with_multiple_bugurl = '''2009-08-19  Eric Seidel  <eric@webkit.org>

        Need a short description (OOPS!).
        https://bugs.webkit.org/show_bug.cgi?id=12345
        http://webkit.org/b/12345

        Reviewed by NOBODY (OOPS!).

        * Scripts/bugzilla-tool:
'''

    _new_entry_boilerplate_without_reviewer_line = '''2009-08-19  Eric Seidel  <eric@webkit.org>

        Need a short description (OOPS!).
        https://bugs.webkit.org/show_bug.cgi?id=12345

        * Scripts/bugzilla-tool:
'''

    _new_entry_boilerplate_without_reviewer_multiple_bugurl = '''2009-08-19  Eric Seidel  <eric@webkit.org>

        Need a short description (OOPS!).
        https://bugs.webkit.org/show_bug.cgi?id=12345
        http://webkit.org/b/12345

        * Scripts/bugzilla-tool:
'''

    def test_set_reviewer(self):
        fs = MockFileSystem()

        changelog_contents = u"%s\n%s" % (self._new_entry_boilerplate_with_bugurl, self._example_changelog)
        reviewer_name = 'Test Reviewer'
        fs.write_text_file(self._changelog_path, changelog_contents)
        ChangeLog(self._changelog_path, fs).set_reviewer(reviewer_name)
        actual_contents = fs.read_text_file(self._changelog_path)
        expected_contents = changelog_contents.replace('NOBODY (OOPS!)', reviewer_name)
        self.assertEqual(actual_contents.splitlines(), expected_contents.splitlines())

        changelog_contents_without_reviewer_line = u"%s\n%s" % (self._new_entry_boilerplate_without_reviewer_line, self._example_changelog)
        fs.write_text_file(self._changelog_path, changelog_contents_without_reviewer_line)
        ChangeLog(self._changelog_path, fs).set_reviewer(reviewer_name)
        actual_contents = fs.read_text_file(self._changelog_path)
        self.assertEqual(actual_contents.splitlines(), expected_contents.splitlines())

        changelog_contents_without_reviewer_line = u"%s\n%s" % (self._new_entry_boilerplate_without_reviewer_multiple_bugurl, self._example_changelog)
        fs.write_text_file(self._changelog_path, changelog_contents_without_reviewer_line)
        ChangeLog(self._changelog_path, fs).set_reviewer(reviewer_name)
        actual_contents = fs.read_text_file(self._changelog_path)
        changelog_contents = u"%s\n%s" % (self._new_entry_boilerplate_with_multiple_bugurl, self._example_changelog)
        expected_contents = changelog_contents.replace('NOBODY (OOPS!)', reviewer_name)
        self.assertEqual(actual_contents.splitlines(), expected_contents.splitlines())

    def test_set_short_description_and_bug_url(self):
        fs = MockFileSystem()

        changelog_contents = u"%s\n%s" % (self._new_entry_boilerplate_with_bugurl, self._example_changelog)
        fs.write_text_file(self._changelog_path, changelog_contents)
        short_description = "A short description"
        bug_url = "http://example.com/b/2344"
        ChangeLog(self._changelog_path, fs).set_short_description_and_bug_url(short_description, bug_url)
        actual_contents = fs.read_text_file(self._changelog_path)
        expected_message = "%s\n        %s" % (short_description, bug_url)
        expected_contents = changelog_contents.replace("Need a short description (OOPS!).", expected_message)
        self.assertEqual(actual_contents.splitlines(), expected_contents.splitlines())

        changelog_contents = u"%s\n%s" % (self._new_entry_boilerplate, self._example_changelog)
        fs.write_text_file(self._changelog_path, changelog_contents)
        short_description = "A short description 2"
        bug_url = "http://example.com/b/2345"
        ChangeLog(self._changelog_path, fs).set_short_description_and_bug_url(short_description, bug_url)
        actual_contents = fs.read_text_file(self._changelog_path)
        expected_message = "%s\n        %s" % (short_description, bug_url)
        expected_contents = changelog_contents.replace("Need a short description (OOPS!).\n        Need the bug URL (OOPS!).", expected_message)
        self.assertEqual(actual_contents.splitlines(), expected_contents.splitlines())

    def test_delete_entries(self):
        fs = MockFileSystem()
        fs.write_text_file(self._changelog_path, self._example_changelog)
        ChangeLog(self._changelog_path, fs).delete_entries(8)
        actual_contents = fs.read_text_file(self._changelog_path)
        expected_contents = """2011-10-11  Antti Koivisto  <antti@apple.com>

       Resolve regular and visited link style in a single pass
       https://bugs.webkit.org/show_bug.cgi?id=69838

       Reviewed by Darin Adler

       We can simplify and speed up selector matching by removing the recursive matching done
       to generate the style for the :visited pseudo selector. Both regular and visited link style
       can be generated in a single pass through the style selector.

== Rolled over to ChangeLog-2009-06-16 ==
"""
        self.assertEqual(actual_contents.splitlines(), expected_contents.splitlines())

        ChangeLog(self._changelog_path, fs).delete_entries(2)
        actual_contents = fs.read_text_file(self._changelog_path)
        expected_contents = "== Rolled over to ChangeLog-2009-06-16 ==\n"
        self.assertEqual(actual_contents.splitlines(), expected_contents.splitlines())


    def test_prepend_text(self):
        fs = MockFileSystem()
        fs.write_text_file(self._changelog_path, self._example_changelog)
        ChangeLog(self._changelog_path, fs).prepend_text(self._example_entry + "\n")
        actual_contents = fs.read_text_file(self._changelog_path)
        expected_contents = self._example_entry + "\n" + self._example_changelog
        self.assertEqual(actual_contents.splitlines(), expected_contents.splitlines())
