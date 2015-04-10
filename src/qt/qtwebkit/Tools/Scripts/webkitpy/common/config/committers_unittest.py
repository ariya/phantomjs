# Copyright (C) 2009 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#    * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer # in the documentation and/or other materials provided with the
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
from webkitpy.common.config.committers import CommitterList, Contributor, Committer, Reviewer

class CommittersTest(unittest.TestCase):
    def test_committer_lookup(self):
        committer = Committer('Test One', 'one@test.com', 'one')
        reviewer = Reviewer('Test Two', ['two@test.com', 'Two@rad.com', 'so_two@gmail.com'])
        contributor = Contributor('Test Three', ['Three@test.com'], 'three')
        contributor_with_two_nicknames = Contributor('Other Four', ['otherfour@webkit.org', 'otherfour@webkit2.org'], ['four', 'otherfour'])
        contributor_with_same_email_username = Contributor('Yet Another Four', ['otherfour@webkit.com'], ['yetanotherfour'])
        committer_list = CommitterList(committers=[committer], reviewers=[reviewer],
            contributors=[contributor, contributor_with_two_nicknames, contributor_with_same_email_username])

        # Test valid committer, reviewer and contributor lookup
        self.assertEqual(committer_list.committer_by_email('one@test.com'), committer)
        self.assertEqual(committer_list.reviewer_by_email('two@test.com'), reviewer)
        self.assertEqual(committer_list.committer_by_email('two@test.com'), reviewer)
        self.assertEqual(committer_list.committer_by_email('two@rad.com'), reviewer)
        self.assertEqual(committer_list.reviewer_by_email('so_two@gmail.com'), reviewer)
        self.assertEqual(committer_list.contributor_by_email('three@test.com'), contributor)

        # Test valid committer, reviewer and contributor lookup
        self.assertEqual(committer_list.committer_by_name("Test One"), committer)
        self.assertEqual(committer_list.committer_by_name("Test Two"), reviewer)
        self.assertIsNone(committer_list.committer_by_name("Test Three"))
        self.assertEqual(committer_list.contributor_by_name("Test Three"), contributor)
        self.assertEqual(committer_list.contributor_by_name("test one"), committer)
        self.assertEqual(committer_list.contributor_by_name("test two"), reviewer)
        self.assertEqual(committer_list.contributor_by_name("test three"), contributor)

        # Test that the first email is assumed to be the Bugzilla email address (for now)
        self.assertEqual(committer_list.committer_by_email('two@rad.com').bugzilla_email(), 'two@test.com')

        # Test that a known committer is not returned during reviewer lookup
        self.assertIsNone(committer_list.reviewer_by_email('one@test.com'))
        self.assertIsNone(committer_list.reviewer_by_email('three@test.com'))
        # and likewise that a known contributor is not returned for committer lookup.
        self.assertIsNone(committer_list.committer_by_email('three@test.com'))

        # Test that unknown email address fail both committer and reviewer lookup
        self.assertIsNone(committer_list.committer_by_email('bar@bar.com'))
        self.assertIsNone(committer_list.reviewer_by_email('bar@bar.com'))

        # Test that emails returns a list.
        self.assertEqual(committer.emails, ['one@test.com'])

        self.assertEqual(committer.irc_nicknames, ['one'])
        self.assertEqual(committer_list.contributor_by_irc_nickname('one'), committer)
        self.assertEqual(committer_list.contributor_by_irc_nickname('three'), contributor)
        self.assertEqual(committer_list.contributor_by_irc_nickname('four'), contributor_with_two_nicknames)
        self.assertEqual(committer_list.contributor_by_irc_nickname('otherfour'), contributor_with_two_nicknames)

        # Test that the lists returned are are we expect them.
        self.assertEqual(committer_list.contributors(), [contributor, contributor_with_two_nicknames, contributor_with_same_email_username, committer, reviewer])
        self.assertEqual(committer_list.committers(), [committer, reviewer])
        self.assertEqual(committer_list.reviewers(), [reviewer])

        self.assertEqual(committer_list.contributors_by_search_string('test'), [contributor, committer, reviewer])
        self.assertEqual(committer_list.contributors_by_search_string('rad'), [reviewer])
        self.assertEqual(committer_list.contributors_by_search_string('Two'), [reviewer])
        self.assertEqual(committer_list.contributors_by_search_string('otherfour'), [contributor_with_two_nicknames])
        self.assertEqual(committer_list.contributors_by_search_string('*otherfour*'), [contributor_with_two_nicknames, contributor_with_same_email_username])

        self.assertEqual(committer_list.contributors_by_email_username("one"), [committer])
        self.assertEqual(committer_list.contributors_by_email_username("four"), [])
        self.assertEqual(committer_list.contributors_by_email_username("otherfour"), [contributor_with_two_nicknames, contributor_with_same_email_username])

    def _assert_fuzz_match(self, text, name_of_expected_contributor, expected_distance):
        committers = CommitterList()
        contributors, distance = committers.contributors_by_fuzzy_match(text)
        if type(name_of_expected_contributor) is list:
            expected_names = name_of_expected_contributor
        else:
            expected_names = [name_of_expected_contributor] if name_of_expected_contributor else []
        self.assertEqual(([contributor.full_name for contributor in contributors], distance), (expected_names, expected_distance))

    # Test that the string representation of a Contributor supports unicode
    def test_contributor_encoding(self):
        committer_encoding = Contributor(u'\u017dan M\u00fcller', 'zmuller@example.com', 'zmuller')
        self.assertTrue(str(committer_encoding))

    # Basic testing of the edit distance matching ...
    def test_contributors_by_fuzzy_match(self):
        self._assert_fuzz_match('Geoff Garen', 'Geoffrey Garen', 3)
        self._assert_fuzz_match('Kenneth Christiansen', 'Kenneth Rohde Christiansen', 6)
        self._assert_fuzz_match('Sam', 'Sam Weinig', 0)
        self._assert_fuzz_match('me', None, 2)

    # The remaining tests test that certain names are resolved in a specific way.
    # We break this up into multiple tests so that each is faster and they can
    # be run in parallel. Unfortunately each test scans the entire committers list,
    # so these are inherently slow (see https://bugs.webkit.org/show_bug.cgi?id=79179).
    #
    # Commented out lines are test cases imported from the bug 26533 yet to pass.

    def integration_test_contributors__none(self):
        self._assert_fuzz_match('myself', None, 6)
        self._assert_fuzz_match('others', None, 6)
        self._assert_fuzz_match('BUILD FIX', None, 9)

    def integration_test_contributors__none_2(self):
        self._assert_fuzz_match('but Dan Bernstein also reviewed', None, 31)
        self._assert_fuzz_match('asked thoughtful questions', None, 26)
        self._assert_fuzz_match('build fix of mac', None, 16)

    def integration_test_contributors__none_3(self):
        self._assert_fuzz_match('a spell checker', None, 15)
        self._assert_fuzz_match('nobody, build fix', None, 17)
        self._assert_fuzz_match('NOBODY (chromium build fix)', None, 27)

    def integration_test_contributors_ada_chan(self):
        self._assert_fuzz_match('Ada', 'Ada Chan', 0)

    def integration_test_contributors_adele_peterson(self):
        self._assert_fuzz_match('adele', 'Adele Peterson', 0)

    def integration_test_contributors_adele_peterson(self):
        # self._assert_fuzz_match('Adam', 'Adam Roben', 0)
        self._assert_fuzz_match('aroben', 'Adam Roben', 0)

    def integration_test_contributors_alexey_proskuryakov(self):
        # self._assert_fuzz_match('Alexey', 'Alexey Proskuryakov', 0)
        self._assert_fuzz_match('ap', 'Alexey Proskuryakov', 0)
        self._assert_fuzz_match('Alexey P', 'Alexey Proskuryakov', 0)

    def integration_test_contributors_alice_liu(self):
        # self._assert_fuzz_match('Alice', 'Alice Liu', 0)
        self._assert_fuzz_match('aliu', 'Alice Liu', 0)
        self._assert_fuzz_match('Liu', 'Alice Liu', 0)

    def integration_test_contributors_alp_toker(self):
        self._assert_fuzz_match('Alp', 'Alp Toker', 0)

    def integration_test_contributors_anders_carlsson(self):
        self._assert_fuzz_match('Anders', 'Anders Carlsson', 0)
        self._assert_fuzz_match('andersca', 'Anders Carlsson', 0)
        self._assert_fuzz_match('anders', 'Anders Carlsson', 0)
        self._assert_fuzz_match('Andersca', 'Anders Carlsson', 0)

    def integration_test_contributors_antti_koivisto(self):
        self._assert_fuzz_match('Antti "printf" Koivisto', 'Antti Koivisto', 9)
        self._assert_fuzz_match('Antti', 'Antti Koivisto', 0)

    def integration_test_contributors_beth_dakin(self):
        self._assert_fuzz_match('Beth', 'Beth Dakin', 0)
        self._assert_fuzz_match('beth', 'Beth Dakin', 0)
        self._assert_fuzz_match('bdakin', 'Beth Dakin', 0)

    def integration_test_contributors_brady_eidson(self):
        self._assert_fuzz_match('Brady', 'Brady Eidson', 0)
        self._assert_fuzz_match('bradee-oh', 'Brady Eidson', 0)
        self._assert_fuzz_match('Brady', 'Brady Eidson', 0)

    def integration_test_contributors_cameron_zwarich(self):
        pass  # self._assert_fuzz_match('Cameron', 'Cameron Zwarich', 0)
        # self._assert_fuzz_match('cpst', 'Cameron Zwarich', 1)

    def integration_test_contributors_chris_blumenberg(self):
        # self._assert_fuzz_match('Chris', 'Chris Blumenberg', 0)
        self._assert_fuzz_match('cblu', 'Chris Blumenberg', 0)

    def integration_test_contributors_dan_bernstein(self):
        self._assert_fuzz_match('Dan', ['Dan Winship', 'Dan Bernstein'], 0)
        self._assert_fuzz_match('Dan B', 'Dan Bernstein', 0)
        # self._assert_fuzz_match('mitz', 'Dan Bernstein', 0)
        self._assert_fuzz_match('Mitz Pettel', 'Dan Bernstein', 1)
        self._assert_fuzz_match('Mitzpettel', 'Dan Bernstein', 0)
        self._assert_fuzz_match('Mitz Pettel RTL', 'Dan Bernstein', 5)

    def integration_test_contributors_dan_bernstein_2(self):
        self._assert_fuzz_match('Teh Mitzpettel', 'Dan Bernstein', 4)
        # self._assert_fuzz_match('The Mitz', 'Dan Bernstein', 0)
        self._assert_fuzz_match('Dr Dan Bernstein', 'Dan Bernstein', 3)

    def integration_test_contributors_darin_adler(self):
        self._assert_fuzz_match('Darin Adler\'', 'Darin Adler', 1)
        self._assert_fuzz_match('Darin', 'Darin Adler', 0)  # Thankfully "Fisher" is longer than "Adler"
        self._assert_fuzz_match('darin', 'Darin Adler', 0)

    def integration_test_contributors_david_harrison(self):
        self._assert_fuzz_match('Dave Harrison', 'David Harrison', 2)
        self._assert_fuzz_match('harrison', 'David Harrison', 0)
        self._assert_fuzz_match('Dr. Harrison', 'David Harrison', 4)

    def integration_test_contributors_david_harrison_2(self):
        self._assert_fuzz_match('Dave Harrson', 'David Harrison', 3)
        self._assert_fuzz_match('Dave Harrsion', 'David Harrison', 4)  # Damerau-Levenshtein distance is 3

    def integration_test_contributors_david_hyatt(self):
        self._assert_fuzz_match('Dave Hyatt', 'David Hyatt', 2)
        self._assert_fuzz_match('Daddy Hyatt', 'David Hyatt', 3)
        # self._assert_fuzz_match('Dave', 'David Hyatt', 0)  # 'Dave' could mean harrison.
        self._assert_fuzz_match('hyatt', 'David Hyatt', 0)
        # self._assert_fuzz_match('Haytt', 'David Hyatt', 0)  # Works if we had implemented Damerau-Levenshtein distance!

    def integration_test_contributors_david_kilzer(self):
        self._assert_fuzz_match('Dave Kilzer', 'David Kilzer', 2)
        self._assert_fuzz_match('David D. Kilzer', 'David Kilzer', 3)
        self._assert_fuzz_match('ddkilzer', 'David Kilzer', 0)

    def integration_test_contributors_don_melton(self):
        self._assert_fuzz_match('Don', 'Don Melton', 0)
        self._assert_fuzz_match('Gramps', 'Don Melton', 0)

    def integration_test_contributors_eric_seidel(self):
        # self._assert_fuzz_match('eric', 'Eric Seidel', 0)
        self._assert_fuzz_match('Eric S', 'Eric Seidel', 0)
        # self._assert_fuzz_match('MacDome', 'Eric Seidel', 0)
        self._assert_fuzz_match('eseidel', 'Eric Seidel', 0)

    def integration_test_contributors_geoffrey_garen(self):
        # self._assert_fuzz_match('Geof', 'Geoffrey Garen', 4)
        # self._assert_fuzz_match('Geoff', 'Geoffrey Garen', 3)
        self._assert_fuzz_match('Geoff Garen', 'Geoffrey Garen', 3)
        self._assert_fuzz_match('ggaren', 'Geoffrey Garen', 0)
        # self._assert_fuzz_match('geoff', 'Geoffrey Garen', 0)
        self._assert_fuzz_match('Geoffrey', 'Geoffrey Garen', 0)
        self._assert_fuzz_match('GGaren', 'Geoffrey Garen', 0)

    def integration_test_contributors_greg_bolsinga(self):
        pass  # self._assert_fuzz_match('Greg', 'Greg Bolsinga', 0)

    def integration_test_contributors_holger_freyther(self):
        self._assert_fuzz_match('Holger', 'Holger Freyther', 0)
        self._assert_fuzz_match('Holger Hans Peter Freyther', 'Holger Freyther', 11)

    def integration_test_contributors_jon_sullivan(self):
        # self._assert_fuzz_match('john', 'John Sullivan', 0)
        self._assert_fuzz_match('sullivan', 'John Sullivan', 0)

    def integration_test_contributors_jon_honeycutt(self):
        self._assert_fuzz_match('John Honeycutt', 'Jon Honeycutt', 1)
        # self._assert_fuzz_match('Jon', 'Jon Honeycutt', 0)

    def integration_test_contributors_jon_honeycutt(self):
        # self._assert_fuzz_match('justin', 'Justin Garcia', 0)
        self._assert_fuzz_match('justing', 'Justin Garcia', 0)

    def integration_test_contributors_joseph_pecoraro(self):
        self._assert_fuzz_match('Joe Pecoraro', 'Joseph Pecoraro', 3)

    def integration_test_contributors_ken_kocienda(self):
        self._assert_fuzz_match('ken', 'Ken Kocienda', 0)
        self._assert_fuzz_match('kocienda', 'Ken Kocienda', 0)

    def integration_test_contributors_kenneth_russell(self):
        self._assert_fuzz_match('Ken Russell', 'Kenneth Russell', 4)

    def integration_test_contributors_kevin_decker(self):
        self._assert_fuzz_match('kdecker', 'Kevin Decker', 0)

    def integration_test_contributors_kevin_mccullough(self):
        self._assert_fuzz_match('Kevin M', 'Kevin McCullough', 0)
        self._assert_fuzz_match('Kevin McCulough', 'Kevin McCullough', 1)
        self._assert_fuzz_match('mccullough', 'Kevin McCullough', 0)

    def integration_test_contributors_lars_knoll(self):
        self._assert_fuzz_match('lars', 'Lars Knoll', 0)

    def integration_test_contributors_lars_weintraub(self):
        self._assert_fuzz_match('levi', 'Levi Weintraub', 0)

    def integration_test_contributors_maciej_stachowiak(self):
        self._assert_fuzz_match('Maciej', 'Maciej Stachowiak', 0)
        # self._assert_fuzz_match('mjs', 'Maciej Stachowiak', 0)
        self._assert_fuzz_match('Maciej S', 'Maciej Stachowiak', 0)

    def integration_test_contributors_mark_rowe(self):
        # self._assert_fuzz_match('Mark', 'Mark Rowe', 0)
        self._assert_fuzz_match('bdash', 'Mark Rowe', 0)
        self._assert_fuzz_match('mrowe', 'Mark Rowe', 0)
        # self._assert_fuzz_match('Brian Dash', 'Mark Rowe', 0)

    def integration_test_contributors_nikolas_zimmermann(self):
        # self._assert_fuzz_match('Niko', 'Nikolas Zimmermann', 1)
        self._assert_fuzz_match('Niko Zimmermann', 'Nikolas Zimmermann', 3)
        self._assert_fuzz_match('Nikolas', 'Nikolas Zimmermann', 0)

    def integration_test_contributors_oliver_hunt(self):
        #  self._assert_fuzz_match('Oliver', 'Oliver Hunt', 0)
        self._assert_fuzz_match('Ollie', 'Oliver Hunt', 1)
        self._assert_fuzz_match('Olliej', 'Oliver Hunt', 0)
        self._assert_fuzz_match('Olliej Hunt', 'Oliver Hunt', 3)
        self._assert_fuzz_match('olliej', 'Oliver Hunt', 0)
        self._assert_fuzz_match('ollie', 'Oliver Hunt', 1)
        self._assert_fuzz_match('ollliej', 'Oliver Hunt', 1)

    def integration_test_contributors_oliver_hunt(self):
        self._assert_fuzz_match('Richard', 'Richard Williamson', 0)
        self._assert_fuzz_match('rjw', 'Richard Williamson', 0)

    def integration_test_contributors_oliver_hunt(self):
        self._assert_fuzz_match('Rob', 'Rob Buis', 0)
        self._assert_fuzz_match('rwlbuis', 'Rob Buis', 0)

    def integration_test_contributors_rniwa(self):
        self._assert_fuzz_match('rniwa@webkit.org', 'Ryosuke Niwa', 0)

    def disabled_integration_test_contributors_simon_fraser(self):
        pass  # self._assert_fuzz_match('Simon', 'Simon Fraser', 0)

    def integration_test_contributors_steve_falkenburg(self):
        self._assert_fuzz_match('Sfalken', 'Steve Falkenburg', 0)
        # self._assert_fuzz_match('Steve', 'Steve Falkenburg', 0)

    def integration_test_contributors_sam_weinig(self):
        self._assert_fuzz_match('Sam', 'Sam Weinig', 0)
        # self._assert_fuzz_match('Weinig Sam', 'weinig', 0)
        self._assert_fuzz_match('Weinig', 'Sam Weinig', 0)
        self._assert_fuzz_match('Sam W', 'Sam Weinig', 0)
        self._assert_fuzz_match('Sammy Weinig', 'Sam Weinig', 2)

    def integration_test_contributors_tim_omernick(self):
        # self._assert_fuzz_match('timo', 'Tim Omernick', 0)
        self._assert_fuzz_match('TimO', 'Tim Omernick', 0)
        # self._assert_fuzz_match('Timo O', 'Tim Omernick', 0)
        # self._assert_fuzz_match('Tim O.', 'Tim Omernick', 0)
        self._assert_fuzz_match('Tim O', 'Tim Omernick', 0)

    def integration_test_contributors_timothy_hatcher(self):
        # self._assert_fuzz_match('Tim', 'Timothy Hatcher', 0)
        # self._assert_fuzz_match('Tim H', 'Timothy Hatcher', 0)
        self._assert_fuzz_match('Tim Hatcher', 'Timothy Hatcher', 4)
        self._assert_fuzz_match('Tim Hatcheri', 'Timothy Hatcher', 5)
        self._assert_fuzz_match('timothy', 'Timothy Hatcher', 0)
        self._assert_fuzz_match('thatcher', 'Timothy Hatcher', 1)
        self._assert_fuzz_match('xenon', 'Timothy Hatcher', 0)
        self._assert_fuzz_match('Hatcher', 'Timothy Hatcher', 0)
        # self._assert_fuzz_match('TimH', 'Timothy Hatcher', 0)

    def integration_test_contributors_tor_arne_vestbo(self):
        self._assert_fuzz_match('Tor Arne', u"Tor Arne Vestb\u00f8", 1)  # Matches IRC nickname

    def integration_test_contributors_vicki_murley(self):
        self._assert_fuzz_match('Vicki', u"Vicki Murley", 0)

    def integration_test_contributors_zack_rusin(self):
        self._assert_fuzz_match('Zack', 'Zack Rusin', 0)
