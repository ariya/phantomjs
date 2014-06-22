# Copyright (C) 2011 Google Inc.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

import os
import os.path
import shutil
import subprocess
import sys
import tempfile
from webkitpy.common.checkout.scm.detection import detect_scm_system
from webkitpy.common.system.executive import ScriptError


class BindingsTests:

    def __init__(self, reset_results, generators, executive):
        self.reset_results = reset_results
        self.generators = generators
        self.executive = executive

    def generate_from_idl(self, generator, idl_file, output_directory, supplemental_dependency_file):
        cmd = ['perl', '-w',
               '-IWebCore/bindings/scripts',
               'WebCore/bindings/scripts/generate-bindings.pl',
               # idl include directories (path relative to generate-bindings.pl)
               '--include', '.',
               '--defines', 'TESTING_%s' % generator,
               '--generator', generator,
               '--outputDir', output_directory,
               '--supplementalDependencyFile', supplemental_dependency_file,
               idl_file]

        exit_code = 0
        try:
            output = self.executive.run_command(cmd)
            if output:
                print output
        except ScriptError, e:
            print e.output
            exit_code = e.exit_code
        return exit_code

    def generate_supplemental_dependency(self, input_directory, supplemental_dependency_file, window_constructors_file, workerglobalscope_constructors_file, sharedworkerglobalscope_constructors_file, dedicatedworkerglobalscope_constructors_file):
        idl_files_list = tempfile.mkstemp()
        for input_file in os.listdir(input_directory):
            (name, extension) = os.path.splitext(input_file)
            if extension != '.idl':
                continue
            os.write(idl_files_list[0], os.path.join(input_directory, input_file) + "\n")
        os.close(idl_files_list[0])

        cmd = ['perl', '-w',
               '-IWebCore/bindings/scripts',
               'WebCore/bindings/scripts/preprocess-idls.pl',
               '--idlFilesList', idl_files_list[1],
               '--defines', '',
               '--supplementalDependencyFile', supplemental_dependency_file,
               '--windowConstructorsFile', window_constructors_file,
               '--workerGlobalScopeConstructorsFile', workerglobalscope_constructors_file,
               '--sharedWorkerGlobalScopeConstructorsFile', sharedworkerglobalscope_constructors_file,
               '--dedicatedWorkerGlobalScopeConstructorsFile', dedicatedworkerglobalscope_constructors_file]

        exit_code = 0
        try:
            output = self.executive.run_command(cmd)
            if output:
                print output
        except ScriptError, e:
            print e.output
            exit_code = e.exit_code
        os.remove(idl_files_list[1])
        return exit_code

    def detect_changes(self, generator, work_directory, reference_directory):
        changes_found = False
        for output_file in os.listdir(work_directory):
            cmd = ['diff',
                   '-u',
                   '-N',
                   os.path.join(reference_directory, output_file),
                   os.path.join(work_directory, output_file)]

            exit_code = 0
            try:
                output = self.executive.run_command(cmd)
            except ScriptError, e:
                output = e.output
                exit_code = e.exit_code

            if exit_code or output:
                print 'FAIL: (%s) %s' % (generator, output_file)
                print output
                changes_found = True
            else:
                print 'PASS: (%s) %s' % (generator, output_file)
        return changes_found

    def run_tests(self, generator, input_directory, reference_directory, supplemental_dependency_file):
        work_directory = reference_directory

        passed = True
        for input_file in os.listdir(input_directory):
            (name, extension) = os.path.splitext(input_file)
            if extension != '.idl':
                continue
            # Generate output into the work directory (either the given one or a
            # temp one if not reset_results is performed)
            if not self.reset_results:
                work_directory = tempfile.mkdtemp()

            if self.generate_from_idl(generator,
                                      os.path.join(input_directory, input_file),
                                      work_directory,
                                      supplemental_dependency_file):
                passed = False

            if self.reset_results:
                print "Reset results: (%s) %s" % (generator, input_file)
                continue

            # Detect changes
            if self.detect_changes(generator, work_directory, reference_directory):
                passed = False
            shutil.rmtree(work_directory)

        return passed

    def main(self):
        current_scm = detect_scm_system(os.curdir)
        os.chdir(os.path.join(current_scm.checkout_root, 'Source'))

        all_tests_passed = True

        input_directory = os.path.join('WebCore', 'bindings', 'scripts', 'test')
        supplemental_dependency_file = tempfile.mkstemp()[1]
        window_constructors_file = tempfile.mkstemp()[1]
        workerglobalscope_constructors_file = tempfile.mkstemp()[1]
        sharedworkerglobalscope_constructors_file = tempfile.mkstemp()[1]
        dedicatedworkerglobalscope_constructors_file = tempfile.mkstemp()[1]
        if self.generate_supplemental_dependency(input_directory, supplemental_dependency_file, window_constructors_file, workerglobalscope_constructors_file, sharedworkerglobalscope_constructors_file, dedicatedworkerglobalscope_constructors_file):
            print 'Failed to generate a supplemental dependency file.'
            os.remove(supplemental_dependency_file)
            os.remove(window_constructors_file)
            os.remove(workerglobalscope_constructors_file)
            os.remove(sharedworkerglobalscope_constructors_file)
            os.remove(dedicatedworkerglobalscope_constructors_file)
            return -1

        for generator in self.generators:
            input_directory = os.path.join('WebCore', 'bindings', 'scripts', 'test')
            reference_directory = os.path.join('WebCore', 'bindings', 'scripts', 'test', generator)
            if not self.run_tests(generator, input_directory, reference_directory, supplemental_dependency_file):
                all_tests_passed = False

        os.remove(supplemental_dependency_file)
        os.remove(window_constructors_file)
        os.remove(workerglobalscope_constructors_file)
        os.remove(sharedworkerglobalscope_constructors_file)
        os.remove(dedicatedworkerglobalscope_constructors_file)
        print ''
        if all_tests_passed:
            print 'All tests PASS!'
            return 0
        else:
            print 'Some tests FAIL! (To update the reference files, execute "run-bindings-tests --reset-results")'
            return -1
