# Copyright (C) 2011 Google Inc. All rights reserved.
# Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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
#     * Neither the Google name nor the names of its
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
import os
import time

from webkitpy.common.system.crashlogs import CrashLogs
from webkitpy.common.system.executive import ScriptError
from webkitpy.port.apple import ApplePort
from webkitpy.port.leakdetector import LeakDetector


_log = logging.getLogger(__name__)


class MacPort(ApplePort):
    port_name = "mac"

    VERSION_FALLBACK_ORDER = ['mac-snowleopard', 'mac-lion', 'mac-mountainlion']

    ARCHITECTURES = ['x86_64', 'x86']

    def __init__(self, host, port_name, **kwargs):
        ApplePort.__init__(self, host, port_name, **kwargs)
        self._architecture = self.get_option('architecture')

        if not self._architecture:
            self._architecture = 'x86_64'

        self._leak_detector = LeakDetector(self)
        if self.get_option("leaks"):
            # DumpRenderTree slows down noticably if we run more than about 1000 tests in a batch
            # with MallocStackLogging enabled.
            self.set_option_default("batch_size", 1000)

    def default_timeout_ms(self):
        if self.get_option('guard_malloc'):
            return 350 * 1000
        return super(MacPort, self).default_timeout_ms()

    def supports_per_test_timeout(self):
        return True

    def _build_driver_flags(self):
        return ['ARCHS=i386'] if self.architecture() == 'x86' else []

    def should_retry_crashes(self):
        # On Apple Mac, we retry crashes due to https://bugs.webkit.org/show_bug.cgi?id=82233
        return True

    def default_baseline_search_path(self):
        name = self._name.replace('-wk2', '')
        if name.endswith(self.FUTURE_VERSION):
            fallback_names = [self.port_name]
        else:
            fallback_names = self.VERSION_FALLBACK_ORDER[self.VERSION_FALLBACK_ORDER.index(name):-1] + [self.port_name]
        if self.get_option('webkit_test_runner'):
            fallback_names = [self._wk2_port_name(), 'wk2'] + fallback_names
        return map(self._webkit_baseline_path, fallback_names)

    def _port_specific_expectations_files(self):
        return list(reversed([self._filesystem.join(self._webkit_baseline_path(p), 'TestExpectations') for p in self.baseline_search_path()]))

    def setup_environ_for_server(self, server_name=None):
        env = super(MacPort, self).setup_environ_for_server(server_name)
        if server_name == self.driver_name():
            if self.get_option('leaks'):
                env['MallocStackLogging'] = '1'
            if self.get_option('guard_malloc'):
                env['DYLD_INSERT_LIBRARIES'] = '/usr/lib/libgmalloc.dylib:' + self._build_path("libWebCoreTestShim.dylib")
            else:
                env['DYLD_INSERT_LIBRARIES'] = self._build_path("libWebCoreTestShim.dylib")
        env['XML_CATALOG_FILES'] = ''  # work around missing /etc/catalog <rdar://problem/4292995>
        return env

    def operating_system(self):
        return 'mac'

    # Belongs on a Platform object.
    def is_snowleopard(self):
        return self._version == "snowleopard"

    # Belongs on a Platform object.
    def is_lion(self):
        return self._version == "lion"

    def default_child_processes(self):
        if self._version == "snowleopard":
            _log.warning("Cannot run tests in parallel on Snow Leopard due to rdar://problem/10621525.")
            return 1

        default_count = super(MacPort, self).default_child_processes()

        # FIXME: https://bugs.webkit.org/show_bug.cgi?id=95906  With too many WebProcess WK2 tests get stuck in resource contention.
        # To alleviate the issue reduce the number of running processes
        # Anecdotal evidence suggests that a 4 core/8 core logical machine may run into this, but that a 2 core/4 core logical machine does not.
        should_throttle_for_wk2 = self.get_option('webkit_test_runner') and default_count > 4
        # We also want to throttle for leaks bots.
        if should_throttle_for_wk2 or self.get_option('leaks'):
            default_count = int(.75 * default_count)

        # Make sure we have enough ram to support that many instances:
        total_memory = self.host.platform.total_bytes_memory()
        if total_memory:
            bytes_per_drt = 256 * 1024 * 1024  # Assume each DRT needs 256MB to run.
            overhead = 2048 * 1024 * 1024  # Assume we need 2GB free for the O/S
            supportable_instances = max((total_memory - overhead) / bytes_per_drt, 1)  # Always use one process, even if we don't have space for it.
            if supportable_instances < default_count:
                _log.warning("This machine could support %s child processes, but only has enough memory for %s." % (default_count, supportable_instances))
        else:
            _log.warning("Cannot determine available memory for child processes, using default child process count of %s." % default_count)
            supportable_instances = default_count
        return min(supportable_instances, default_count)

    def _build_java_test_support(self):
        java_tests_path = self._filesystem.join(self.layout_tests_dir(), "java")
        build_java = [self.make_command(), "-C", java_tests_path]
        if self._executive.run_command(build_java, return_exit_code=True):  # Paths are absolute, so we don't need to set a cwd.
            _log.error("Failed to build Java support files: %s" % build_java)
            return False
        return True

    def check_for_leaks(self, process_name, process_pid):
        if not self.get_option('leaks'):
            return
        # We could use http://code.google.com/p/psutil/ to get the process_name from the pid.
        self._leak_detector.check_for_leaks(process_name, process_pid)

    def print_leaks_summary(self):
        if not self.get_option('leaks'):
            return
        # We're in the manager process, so the leak detector will not have a valid list of leak files.
        # FIXME: This is a hack, but we don't have a better way to get this information from the workers yet.
        # FIXME: This will include too many leaks in subsequent runs until the results directory is cleared!
        leaks_files = self._leak_detector.leaks_files_in_directory(self.results_directory())
        if not leaks_files:
            return
        total_bytes_string, unique_leaks = self._leak_detector.count_total_bytes_and_unique_leaks(leaks_files)
        total_leaks = self._leak_detector.count_total_leaks(leaks_files)
        _log.info("%s total leaks found for a total of %s!" % (total_leaks, total_bytes_string))
        _log.info("%s unique leaks found!" % unique_leaks)

    def _check_port_build(self):
        return self.get_option('nojava') or self._build_java_test_support()

    def _path_to_webcore_library(self):
        return self._build_path('WebCore.framework/Versions/A/WebCore')

    def show_results_html_file(self, results_filename):
        # We don't use self._run_script() because we don't want to wait for the script
        # to exit and we want the output to show up on stdout in case there are errors
        # launching the browser.
        self._executive.popen([self.path_to_script('run-safari')] + self._arguments_for_configuration() + ['--no-saved-state', '-NSOpen', results_filename],
            cwd=self.webkit_base(), stdout=file(os.devnull), stderr=file(os.devnull))

    # FIXME: The next two routines turn off the http locking in order
    # to work around failures on the bots caused when the slave restarts.
    # See https://bugs.webkit.org/show_bug.cgi?id=64886 for more info.
    # The proper fix is to make sure the slave is actually stopping NRWT
    # properly on restart. Note that by removing the lock file and not waiting,
    # the result should be that if there is a web server already running,
    # it'll be killed and this one will be started in its place; this
    # may lead to weird things happening in the other run. However, I don't
    # think we're (intentionally) actually running multiple runs concurrently
    # on any Mac bots.

    def acquire_http_lock(self):
        pass

    def release_http_lock(self):
        pass

    def sample_file_path(self, name, pid):
        return self._filesystem.join(self.results_directory(), "{0}-{1}-sample.txt".format(name, pid))

    def _get_crash_log(self, name, pid, stdout, stderr, newer_than, time_fn=None, sleep_fn=None, wait_for_log=True):
        # Note that we do slow-spin here and wait, since it appears the time
        # ReportCrash takes to actually write and flush the file varies when there are
        # lots of simultaneous crashes going on.
        # FIXME: Should most of this be moved into CrashLogs()?
        time_fn = time_fn or time.time
        sleep_fn = sleep_fn or time.sleep
        crash_log = ''
        crash_logs = CrashLogs(self.host)
        now = time_fn()
        # FIXME: delete this after we're sure this code is working ...
        _log.debug('looking for crash log for %s:%s' % (name, str(pid)))
        deadline = now + 5 * int(self.get_option('child_processes', 1))
        while not crash_log and now <= deadline:
            crash_log = crash_logs.find_newest_log(name, pid, include_errors=True, newer_than=newer_than)
            if not wait_for_log:
                break
            if not crash_log or not [line for line in crash_log.splitlines() if not line.startswith('ERROR')]:
                sleep_fn(0.1)
                now = time_fn()

        if not crash_log:
            return (stderr, None)
        return (stderr, crash_log)

    def look_for_new_crash_logs(self, crashed_processes, start_time):
        """Since crash logs can take a long time to be written out if the system is
           under stress do a second pass at the end of the test run.

           crashes: test_name -> pid, process_name tuple of crashed process
           start_time: time the tests started at.  We're looking for crash
               logs after that time.
        """
        crash_logs = {}
        for (test_name, process_name, pid) in crashed_processes:
            # Passing None for output.  This is a second pass after the test finished so
            # if the output had any logging we would have already collected it.
            crash_log = self._get_crash_log(process_name, pid, None, None, start_time, wait_for_log=False)[1]
            if not crash_log:
                continue
            crash_logs[test_name] = crash_log
        return crash_logs

    def look_for_new_samples(self, unresponsive_processes, start_time):
        sample_files = {}
        for (test_name, process_name, pid) in unresponsive_processes:
            sample_file = self.sample_file_path(process_name, pid)
            if not self._filesystem.isfile(sample_file):
                continue
            sample_files[test_name] = sample_file
        return sample_files

    def sample_process(self, name, pid):
        try:
            hang_report = self.sample_file_path(name, pid)
            self._executive.run_command([
                "/usr/bin/sample",
                pid,
                10,
                10,
                "-file",
                hang_report,
            ])
        except ScriptError as e:
            _log.warning('Unable to sample process:' + str(e))

    def _path_to_helper(self):
        binary_name = 'LayoutTestHelper'
        return self._build_path(binary_name)

    def start_helper(self):
        helper_path = self._path_to_helper()
        if helper_path:
            _log.debug("Starting layout helper %s" % helper_path)
            self._helper = self._executive.popen([helper_path],
                stdin=self._executive.PIPE, stdout=self._executive.PIPE, stderr=None)
            is_ready = self._helper.stdout.readline()
            if not is_ready.startswith('ready'):
                _log.error("LayoutTestHelper failed to be ready")

    def stop_helper(self):
        if self._helper:
            _log.debug("Stopping LayoutTestHelper")
            try:
                self._helper.stdin.write("x\n")
                self._helper.stdin.close()
                self._helper.wait()
            except IOError, e:
                _log.debug("IOError raised while stopping helper: %s" % str(e))
            self._helper = None

    def make_command(self):
        return self.xcrun_find('make', '/usr/bin/make')

    def nm_command(self):
        return self.xcrun_find('nm', 'nm')

    def xcrun_find(self, command, fallback):
        try:
            return self._executive.run_command(['xcrun', '-find', command]).rstrip()
        except ScriptError:
            _log.warn("xcrun failed; falling back to '%s'." % fallback)
            return fallback
