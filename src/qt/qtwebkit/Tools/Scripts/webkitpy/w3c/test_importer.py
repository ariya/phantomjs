#!/usr/bin/env python

# Copyright (C) 2013 Adobe Systems Incorporated. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above
#    copyright notice, this list of conditions and the following
#    disclaimer.
# 2. Redistributions in binary form must reproduce the above
#    copyright notice, this list of conditions and the following
#    disclaimer in the documentation and/or other materials
#    provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
# OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
# TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
# THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.

"""
 This script imports a directory of W3C CSS tests into WebKit.

 You must have checked out the W3C repository to your local drive.

 This script will import the tests into WebKit following these rules:

    - Only tests that are approved or officially submitted awaiting review are imported

    - All tests are imported into LayoutTests/csswg

    - If the tests are approved, they'll be imported into a directory tree that
      mirrors the CSS Mercurial repo. For example, <csswg_repo_root>/approved/css2.1 is brought in
      as LayoutTests/csswg/approved/css2.1, maintaining the entire directory structure under that

    - If the tests are submitted, they'll be brought in as LayoutTests/csswg/submitted and will also
      maintain their directory structure under that. For example, everything under
      <csswg_repo_root>/contributors/adobe/submitted is brought into submitted, mirroring its
      directory structure in the csswg repo

    - If the import directory specified is just a contributor folder, only the submitted folder
      for that contributor is brought in. For example, to import all of Mozilla's tests, either
      <csswg_repo_root>/contributors/mozilla or <csswg_repo_root>/contributors/mozilla/submitted
      will work and are equivalent

    - For the time being, this script won't work if you try to import the full set of submitted
      tests under contributors/*/submitted. Since these are awaiting review, this is just a small
      control mechanism to enforce carefully selecting what non-approved tests are imported.
      It can obviously and easily be changed.

    - By default, only reftests and jstest are imported. This can be overridden with a -a or --all
      argument

    - Also by default, if test files by the same name already exist in the destination directory,
      they are overwritten with the idea that running this script would refresh files periodically.
      This can also be overridden by a -n or --no-overwrite flag

    - All files are converted to work in WebKit:
         1. Paths to testharness.js files are modified point to Webkit's copy of them in
            LayoutTests/resources, using the correct relative path from the new location
         2. All CSS properties requiring the -webkit-vendor prefix are prefixed - this current
            list of what needs prefixes is read from Source/WebCore/CSS/CSSProperties.in
         3. Each reftest has its own copy of its reference file following the naming conventions
            new-run-webkit-tests expects
         4. If a reference files lives outside the directory of the test that uses it, it is checked
            for paths to support files as it will be imported into a different relative position to the
            test file (in the same directory)

     - Upon completion, script outputs the total number tests imported, broken down by test type

     - Also upon completion, each directory where files are imported will have w3c-import.log written
       with a timestamp, the W3C Mercurial changeset if available, the list of CSS properties used that
       require prefixes, the list of imported files, and guidance for future test modification and
       maintenance.

     - On subsequent imports, this file is read to determine if files have been removed in the newer changesets.
       The script removes these files accordingly.
"""

# FIXME: Change this file to use the Host abstractions rather that os, sys, shutils, etc.

import datetime
import logging
import mimetypes
import optparse
import os
import shutil
import sys

from webkitpy.common.host import Host
from webkitpy.common.webkit_finder import WebKitFinder
from webkitpy.common.system.executive import ScriptError
from webkitpy.w3c.test_parser import TestParser
from webkitpy.w3c.test_converter import W3CTestConverter


TEST_STATUS_UNKNOWN = 'unknown'
TEST_STATUS_APPROVED = 'approved'
TEST_STATUS_SUBMITTED = 'submitted'

CHANGESET_NOT_AVAILABLE = 'Not Available'


_log = logging.getLogger(__name__)


def main(_argv, _stdout, _stderr):
    options, args = parse_args()
    import_dir = args[0]
    if len(args) == 1:
        repo_dir = os.path.dirname(import_dir)
    else:
        repo_dir = args[1]

    if not os.path.exists(import_dir):
        sys.exit('Source directory %s not found!' % import_dir)

    if not os.path.exists(repo_dir):
        sys.exit('Repository directory %s not found!' % repo_dir)
    if not repo_dir in import_dir:
        sys.exit('Repository directory %s must be a parent of %s' % (repo_dir, import_dir))

    configure_logging()

    test_importer = TestImporter(Host(), import_dir, repo_dir, options)
    test_importer.do_import()


def configure_logging():
    class LogHandler(logging.StreamHandler):

        def format(self, record):
            if record.levelno > logging.INFO:
                return "%s: %s" % (record.levelname, record.getMessage())
            return record.getMessage()

    logger = logging.getLogger()
    logger.setLevel(logging.INFO)
    handler = LogHandler()
    handler.setLevel(logging.INFO)
    logger.addHandler(handler)
    return handler


def parse_args():
    parser = optparse.OptionParser(usage='usage: %prog [options] w3c_test_directory [repo_directory]')
    parser.add_option('-n', '--no-overwrite', dest='overwrite', action='store_false', default=True,
        help='Flag to prevent duplicate test files from overwriting existing tests. By default, they will be overwritten')
    parser.add_option('-a', '--all', action='store_true', default=False,
        help='Import all tests including reftests, JS tests, and manual/pixel tests. By default, only reftests and JS tests are imported')

    options, args = parser.parse_args()
    if len(args) not in (1, 2):
        parser.error('Incorrect number of arguments')
    return options, args


class TestImporter(object):

    def __init__(self, host, source_directory, repo_dir, options):
        self.host = host
        self.source_directory = source_directory
        self.options = options

        self.filesystem = self.host.filesystem

        webkit_finder = WebKitFinder(self.filesystem)
        self._webkit_root = webkit_finder.webkit_base()
        self.repo_dir = repo_dir
        subdirs = os.path.dirname(os.path.relpath(source_directory, repo_dir))

        self.destination_directory = webkit_finder.path_from_webkit_base("LayoutTests", 'w3c', *subdirs)

        self.changeset = CHANGESET_NOT_AVAILABLE
        self.test_status = TEST_STATUS_UNKNOWN

        self.import_list = []

    def do_import(self):
        self.find_importable_tests(self.source_directory)
        self.load_changeset()
        self.import_tests()

    def load_changeset(self):
        """Returns the current changeset from mercurial or "Not Available"."""
        try:
            self.changeset = self.host.executive.run_command(['hg', 'tip']).split('changeset:')[1]
        except (OSError, ScriptError):
            self.changeset = CHANGESET_NOT_AVAILABLE

    def find_importable_tests(self, directory):
        # FIXME: use filesystem
        for root, dirs, files in os.walk(directory):
            _log.info('Scanning ' + root + '...')
            total_tests = 0
            reftests = 0
            jstests = 0

            # "archive" and "data" dirs are internal csswg things that live in every approved directory.
            # FIXME: skip 'incoming' tests for now, but we should rework the 'test_status' concept and
            # support reading them as well.
            DIRS_TO_SKIP = ('.git', '.hg', 'data', 'archive', 'incoming')
            for d in DIRS_TO_SKIP:
                if d in dirs:
                    dirs.remove(d)

            copy_list = []

            for filename in files:
                # FIXME: This block should really be a separate function, but the early-continues make that difficult.

                if filename.startswith('.') or filename.endswith('.pl'):
                    continue  # For some reason the w3c repo contains random perl scripts we don't care about.

                fullpath = os.path.join(root, filename)

                mimetype = mimetypes.guess_type(fullpath)
                if not 'html' in str(mimetype[0]) and not 'xml' in str(mimetype[0]):
                    copy_list.append({'src': fullpath, 'dest': filename})
                    continue

                test_parser = TestParser(vars(self.options), filename=fullpath)
                test_info = test_parser.analyze_test()
                if test_info is None:
                    continue

                if 'reference' in test_info.keys():
                    reftests += 1
                    total_tests += 1
                    test_basename = os.path.basename(test_info['test'])

                    # Add the ref file, following WebKit style.
                    # FIXME: Ideally we'd support reading the metadata
                    # directly rather than relying  on a naming convention.
                    # Using a naming convention creates duplicate copies of the
                    # reference files.
                    ref_file = os.path.splitext(test_basename)[0] + '-expected'
                    ref_file += os.path.splitext(test_basename)[1]

                    copy_list.append({'src': test_info['reference'], 'dest': ref_file})
                    copy_list.append({'src': test_info['test'], 'dest': filename})

                    # Update any support files that need to move as well to remain relative to the -expected file.
                    if 'refsupport' in test_info.keys():
                        for support_file in test_info['refsupport']:
                            source_file = os.path.join(os.path.dirname(test_info['reference']), support_file)
                            source_file = os.path.normpath(source_file)

                            # Keep the dest as it was
                            to_copy = {'src': source_file, 'dest': support_file}

                            # Only add it once
                            if not(to_copy in copy_list):
                                copy_list.append(to_copy)
                elif 'jstest' in test_info.keys():
                    jstests += 1
                    total_tests += 1
                    copy_list.append({'src': fullpath, 'dest': filename})
                else:
                    total_tests += 1
                    copy_list.append({'src': fullpath, 'dest': filename})

            if not total_tests:
                # We can skip the support directory if no tests were found.
                if 'support' in dirs:
                    dirs.remove('support')

            if copy_list:
                # Only add this directory to the list if there's something to import
                self.import_list.append({'dirname': root, 'copy_list': copy_list,
                    'reftests': reftests, 'jstests': jstests, 'total_tests': total_tests})

    def import_tests(self):
        converter = W3CTestConverter()
        total_imported_tests = 0
        total_imported_reftests = 0
        total_imported_jstests = 0
        total_prefixed_properties = {}

        for dir_to_copy in self.import_list:
            total_imported_tests += dir_to_copy['total_tests']
            total_imported_reftests += dir_to_copy['reftests']
            total_imported_jstests += dir_to_copy['jstests']

            prefixed_properties = []

            if not dir_to_copy['copy_list']:
                continue

            orig_path = dir_to_copy['dirname']

            subpath = os.path.relpath(orig_path, self.repo_dir)
            new_path = os.path.join(self.destination_directory, subpath)

            if not(os.path.exists(new_path)):
                os.makedirs(new_path)

            copied_files = []

            for file_to_copy in dir_to_copy['copy_list']:
                # FIXME: Split this block into a separate function.
                orig_filepath = os.path.normpath(file_to_copy['src'])

                if os.path.isdir(orig_filepath):
                    # FIXME: Figure out what is triggering this and what to do about it.
                    _log.error('%s refers to a directory' % orig_filepath)
                    continue

                if not(os.path.exists(orig_filepath)):
                    _log.warning('%s not found. Possible error in the test.', orig_filepath)
                    continue

                new_filepath = os.path.join(new_path, file_to_copy['dest'])

                if not(os.path.exists(os.path.dirname(new_filepath))):
                    os.makedirs(os.path.dirname(new_filepath))

                if not self.options.overwrite and os.path.exists(new_filepath):
                    _log.info('Skipping import of existing file ' + new_filepath)
                else:
                    # FIXME: Maybe doing a file diff is in order here for existing files?
                    # In other words, there's no sense in overwriting identical files, but
                    # there's no harm in copying the identical thing.
                    _log.info('Importing: %s', orig_filepath)
                    _log.info('       As: %s', new_filepath)

                # Only html, xml, or css should be converted
                # FIXME: Eventually, so should js when support is added for this type of conversion
                mimetype = mimetypes.guess_type(orig_filepath)
                if 'html' in str(mimetype[0]) or 'xml' in str(mimetype[0])  or 'css' in str(mimetype[0]):
                    converted_file = converter.convert_for_webkit(new_path, filename=orig_filepath)

                    if not converted_file:
                        shutil.copyfile(orig_filepath, new_filepath)  # The file was unmodified.
                    else:
                        for prefixed_property in converted_file[0]:
                            total_prefixed_properties.setdefault(prefixed_property, 0)
                            total_prefixed_properties[prefixed_property] += 1

                        prefixed_properties.extend(set(converted_file[0]) - set(prefixed_properties))
                        outfile = open(new_filepath, 'wb')
                        outfile.write(converted_file[1])
                        outfile.close()
                else:
                    shutil.copyfile(orig_filepath, new_filepath)

                copied_files.append(new_filepath.replace(self._webkit_root, ''))

            self.remove_deleted_files(new_path, copied_files)
            self.write_import_log(new_path, copied_files, prefixed_properties)

        _log.info('Import complete')

        _log.info('IMPORTED %d TOTAL TESTS', total_imported_tests)
        _log.info('Imported %d reftests', total_imported_reftests)
        _log.info('Imported %d JS tests', total_imported_jstests)
        _log.info('Imported %d pixel/manual tests', total_imported_tests - total_imported_jstests - total_imported_reftests)
        _log.info('')
        _log.info('Properties needing prefixes (by count):')
        for prefixed_property in sorted(total_prefixed_properties, key=lambda p: total_prefixed_properties[p]):
            _log.info('  %s: %s', prefixed_property, total_prefixed_properties[prefixed_property])

    def setup_destination_directory(self):
        """ Creates a destination directory that mirrors that of the source approved or submitted directory """

        self.update_test_status()

        start = self.source_directory.find(self.test_status)
        new_subpath = self.source_directory[len(self.repo_dir):]

        destination_directory = os.path.join(self.destination_directory, new_subpath)

        if not os.path.exists(destination_directory):
            os.makedirs(destination_directory)

        _log.info('Tests will be imported into: %s', destination_directory)

    def update_test_status(self):
        """ Sets the test status to either 'approved' or 'submitted' """

        status = TEST_STATUS_UNKNOWN

        if 'approved' in self.source_directory.split(os.path.sep):
            status = TEST_STATUS_APPROVED
        elif 'submitted' in self.source_directory.split(os.path.sep):
            status = TEST_STATUS_SUBMITTED

        self.test_status = status

    def remove_deleted_files(self, import_directory, new_file_list):
        """ Reads an import log in |import_directory|, compares it to the |new_file_list|, and removes files not in the new list."""

        previous_file_list = []

        import_log_file = os.path.join(import_directory, 'w3c-import.log')
        if not os.path.exists(import_log_file):
            return

        import_log = open(import_log_file, 'r')
        contents = import_log.readlines()

        if 'List of files\n' in contents:
            list_index = contents.index('List of files:\n') + 1
            previous_file_list = [filename.strip() for filename in contents[list_index:]]

        deleted_files = set(previous_file_list) - set(new_file_list)
        for deleted_file in deleted_files:
            _log.info('Deleting file removed from the W3C repo: %s', deleted_file)
            deleted_file = os.path.join(self._webkit_root, deleted_file)
            os.remove(deleted_file)

        import_log.close()

    def write_import_log(self, import_directory, file_list, prop_list):
        """ Writes a w3c-import.log file in each directory with imported files. """

        now = datetime.datetime.now()

        import_log = open(os.path.join(import_directory, 'w3c-import.log'), 'w')
        import_log.write('The tests in this directory were imported from the W3C repository.\n')
        import_log.write('Do NOT modify these tests directly in Webkit. Instead, push changes to the W3C CSS repo:\n\n')
        import_log.write('http://hg.csswg.org/test\n\n')
        import_log.write('Then run the Tools/Scripts/import-w3c-tests in Webkit to reimport\n\n')
        import_log.write('Do NOT modify or remove this file\n\n')
        import_log.write('------------------------------------------------------------------------\n')
        import_log.write('Last Import: ' + now.strftime('%Y-%m-%d %H:%M') + '\n')
        import_log.write('W3C Mercurial changeset: ' + self.changeset + '\n')
        import_log.write('Test status at time of import: ' + self.test_status + '\n')
        import_log.write('------------------------------------------------------------------------\n')
        import_log.write('Properties requiring vendor prefixes:\n')
        if prop_list:
            for prop in prop_list:
                import_log.write(prop + '\n')
        else:
            import_log.write('None\n')
        import_log.write('------------------------------------------------------------------------\n')
        import_log.write('List of files:\n')
        for item in file_list:
            import_log.write(item + '\n')

        import_log.close()
