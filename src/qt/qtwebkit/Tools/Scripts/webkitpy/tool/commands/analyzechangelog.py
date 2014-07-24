# Copyright (c) 2011 Google Inc. All rights reserved.
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

import json
import re
import time

from webkitpy.common.checkout.scm.detection import SCMDetector
from webkitpy.common.checkout.changelog import ChangeLog
from webkitpy.common.config.contributionareas import ContributionAreas
from webkitpy.common.system.filesystem import FileSystem
from webkitpy.common.system.executive import Executive
from webkitpy.tool.multicommandtool import Command
from webkitpy.tool import steps


class AnalyzeChangeLog(Command):
    name = "analyze-changelog"
    help_text = "Experimental command for analyzing change logs."
    long_help = "This command parses changelogs in a specified directory and summarizes the result as JSON files."

    def __init__(self):
        options = [
            steps.Options.changelog_count,
        ]
        Command.__init__(self, options=options)

    @staticmethod
    def _enumerate_changelogs(filesystem, dirname, changelog_count):
        changelogs = [filesystem.join(dirname, filename) for filename in filesystem.listdir(dirname) if re.match('^ChangeLog(-(\d{4}-\d{2}-\d{2}))?$', filename)]
        # Make sure ChangeLog shows up before ChangeLog-2011-01-01
        changelogs = sorted(changelogs, key=lambda filename: filename + 'X', reverse=True)
        return changelogs[:changelog_count]

    @staticmethod
    def _generate_jsons(filesystem, jsons, output_dir):
        for filename in jsons:
            print '    Generating', filename
            filesystem.write_text_file(filesystem.join(output_dir, filename), json.dumps(jsons[filename], indent=2))

    def execute(self, options, args, tool):
        filesystem = self._tool.filesystem
        if len(args) < 1 or not filesystem.exists(args[0]):
            print "Need the directory name to look for changelog as the first argument"
            return
        changelog_dir = filesystem.abspath(args[0])

        if len(args) < 2 or not filesystem.exists(args[1]):
            print "Need the output directory name as the second argument"
            return
        output_dir = args[1]

        startTime = time.time()

        print 'Enumerating ChangeLog files...'
        changelogs = AnalyzeChangeLog._enumerate_changelogs(filesystem, changelog_dir, options.changelog_count)

        analyzer = ChangeLogAnalyzer(tool, changelogs)
        analyzer.analyze()

        print 'Generating json files...'
        json_files = {
            'summary.json': analyzer.summary(),
            'contributors.json': analyzer.contributors_statistics(),
            'areas.json': analyzer.areas_statistics(),
        }
        AnalyzeChangeLog._generate_jsons(filesystem, json_files, output_dir)
        commands_dir = filesystem.dirname(filesystem.path_to_module(self.__module__))
        print commands_dir
        filesystem.copyfile(filesystem.join(commands_dir, 'data/summary.html'), filesystem.join(output_dir, 'summary.html'))

        tick = time.time() - startTime
        print 'Finished in %02dm:%02ds' % (int(tick / 60), int(tick % 60))


class ChangeLogAnalyzer(object):
    def __init__(self, host, changelog_paths):
        self._changelog_paths = changelog_paths
        self._filesystem = host.filesystem
        self._contribution_areas = ContributionAreas(host.filesystem)
        self._scm = host.scm()
        self._parsed_revisions = {}

        self._contributors_statistics = {}
        self._areas_statistics = dict([(area, {'reviewed': 0, 'unreviewed': 0, 'contributors': {}}) for area in self._contribution_areas.names()])
        self._summary = {'reviewed': 0, 'unreviewed': 0}

        self._longest_filename = max([len(path) - len(self._scm.checkout_root) for path in changelog_paths])
        self._filename = ''
        self._length_of_previous_output = 0

    def contributors_statistics(self):
        return self._contributors_statistics

    def areas_statistics(self):
        return self._areas_statistics

    def summary(self):
        return self._summary

    def _print_status(self, status):
        if self._length_of_previous_output:
            print "\r" + " " * self._length_of_previous_output,
        new_output = ('%' + str(self._longest_filename) + 's: %s') % (self._filename, status)
        print "\r" + new_output,
        self._length_of_previous_output = len(new_output)

    def _set_filename(self, filename):
        if self._filename:
            print
        self._filename = filename

    def analyze(self):
        for path in self._changelog_paths:
            self._set_filename(self._filesystem.relpath(path, self._scm.checkout_root))
            with self._filesystem.open_text_file_for_reading(path) as changelog:
                self._print_status('Parsing entries...')
                number_of_parsed_entries = self._analyze_entries(ChangeLog.parse_entries_from_file(changelog), path)
            self._print_status('Done (%d entries)' % number_of_parsed_entries)
        print
        self._summary['contributors'] = len(self._contributors_statistics)
        self._summary['contributors_with_reviews'] = sum([1 for contributor in self._contributors_statistics.values() if contributor['reviews']['total']])
        self._summary['contributors_without_reviews'] = self._summary['contributors'] - self._summary['contributors_with_reviews']

    def _collect_statistics_for_contributor_area(self, area, contributor, contribution_type, reviewed):
        area_contributors = self._areas_statistics[area]['contributors']
        if contributor not in area_contributors:
            area_contributors[contributor] = {'reviews': 0, 'reviewed': 0, 'unreviewed': 0}
        if contribution_type == 'patches':
            contribution_type = 'reviewed' if reviewed else 'unreviewed'
        area_contributors[contributor][contribution_type] += 1

    def _collect_statistics_for_contributor(self, contributor, contribution_type, areas, touched_files, reviewed):
        if contributor not in self._contributors_statistics:
            self._contributors_statistics[contributor] = {
                'reviews': {'total': 0, 'areas': {}, 'files': {}},
                'patches': {'reviewed': 0, 'unreviewed': 0, 'areas': {}, 'files': {}}}
        statistics = self._contributors_statistics[contributor][contribution_type]

        if contribution_type == 'reviews':
            statistics['total'] += 1
        elif reviewed:
            statistics['reviewed'] += 1
        else:
            statistics['unreviewed'] += 1

        for area in areas:
            self._increment_dictionary_value(statistics['areas'], area)
            self._collect_statistics_for_contributor_area(area, contributor, contribution_type, reviewed)
        for touchedfile in touched_files:
            self._increment_dictionary_value(statistics['files'], touchedfile)

    def _increment_dictionary_value(self, dictionary, key):
        dictionary[key] = dictionary.get(key, 0) + 1

    def _analyze_entries(self, entries, changelog_path):
        dirname = self._filesystem.dirname(changelog_path)
        i = 0
        for i, entry in enumerate(entries):
            self._print_status('(%s) entries' % i)
            assert(entry.authors())

            touchedfiles_for_entry = [self._filesystem.relpath(self._filesystem.join(dirname, name), self._scm.checkout_root) for name in entry.touched_files()]
            areas_for_entry = self._contribution_areas.areas_for_touched_files(touchedfiles_for_entry)
            authors_for_entry = entry.authors()
            reviewers_for_entry = entry.reviewers()

            for reviewer in reviewers_for_entry:
                self._collect_statistics_for_contributor(reviewer.full_name, 'reviews', areas_for_entry, touchedfiles_for_entry, reviewed=True)

            for author in authors_for_entry:
                self._collect_statistics_for_contributor(author['name'], 'patches', areas_for_entry, touchedfiles_for_entry,
                    reviewed=bool(reviewers_for_entry))

            for area in areas_for_entry:
                self._areas_statistics[area]['reviewed' if reviewers_for_entry else 'unreviewed'] += 1

            self._summary['reviewed' if reviewers_for_entry else 'unreviewed'] += 1

        self._print_status('(%s) entries' % i)
        return i
