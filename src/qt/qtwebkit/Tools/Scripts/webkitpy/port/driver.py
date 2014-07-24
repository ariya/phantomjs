# Copyright (C) 2011 Google Inc. All rights reserved.
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

import base64
import copy
import logging
import re
import shlex
import sys
import time
import os

from webkitpy.common.system import path
from webkitpy.common.system.profiler import ProfilerFactory


_log = logging.getLogger(__name__)


class DriverInput(object):
    def __init__(self, test_name, timeout, image_hash, should_run_pixel_test, args=None):
        self.test_name = test_name
        self.timeout = timeout  # in ms
        self.image_hash = image_hash
        self.should_run_pixel_test = should_run_pixel_test
        self.args = args or []


class DriverOutput(object):
    """Groups information about a output from driver for easy passing
    and post-processing of data."""

    strip_patterns = []
    strip_patterns.append((re.compile('at \(-?[0-9]+,-?[0-9]+\) *'), ''))
    strip_patterns.append((re.compile('size -?[0-9]+x-?[0-9]+ *'), ''))
    strip_patterns.append((re.compile('text run width -?[0-9]+: '), ''))
    strip_patterns.append((re.compile('text run width -?[0-9]+ [a-zA-Z ]+: '), ''))
    strip_patterns.append((re.compile('RenderButton {BUTTON} .*'), 'RenderButton {BUTTON}'))
    strip_patterns.append((re.compile('RenderImage {INPUT} .*'), 'RenderImage {INPUT}'))
    strip_patterns.append((re.compile('RenderBlock {INPUT} .*'), 'RenderBlock {INPUT}'))
    strip_patterns.append((re.compile('RenderTextControl {INPUT} .*'), 'RenderTextControl {INPUT}'))
    strip_patterns.append((re.compile('\([0-9]+px'), 'px'))
    strip_patterns.append((re.compile(' *" *\n +" *'), ' '))
    strip_patterns.append((re.compile('" +$'), '"'))
    strip_patterns.append((re.compile('- '), '-'))
    strip_patterns.append((re.compile('\n( *)"\s+'), '\n\g<1>"'))
    strip_patterns.append((re.compile('\s+"\n'), '"\n'))
    strip_patterns.append((re.compile('scrollWidth [0-9]+'), 'scrollWidth'))
    strip_patterns.append((re.compile('scrollHeight [0-9]+'), 'scrollHeight'))
    strip_patterns.append((re.compile('scrollX [0-9]+'), 'scrollX'))
    strip_patterns.append((re.compile('scrollY [0-9]+'), 'scrollY'))
    strip_patterns.append((re.compile('scrolled to [0-9]+,[0-9]+'), 'scrolled'))

    def __init__(self, text, image, image_hash, audio, crash=False,
            test_time=0, measurements=None, timeout=False, error='', crashed_process_name='??',
            crashed_pid=None, crash_log=None, pid=None):
        # FIXME: Args could be renamed to better clarify what they do.
        self.text = text
        self.image = image  # May be empty-string if the test crashes.
        self.image_hash = image_hash
        self.image_diff = None  # image_diff gets filled in after construction.
        self.audio = audio  # Binary format is port-dependent.
        self.crash = crash
        self.crashed_process_name = crashed_process_name
        self.crashed_pid = crashed_pid
        self.crash_log = crash_log
        self.test_time = test_time
        self.measurements = measurements
        self.timeout = timeout
        self.error = error  # stderr output
        self.pid = pid

    def has_stderr(self):
        return bool(self.error)

    def strip_metrics(self):
        if not self.text:
            return
        for pattern in self.strip_patterns:
            self.text = re.sub(pattern[0], pattern[1], self.text)


class Driver(object):
    """object for running test(s) using DumpRenderTree/WebKitTestRunner."""

    def __init__(self, port, worker_number, pixel_tests, no_timeout=False):
        """Initialize a Driver to subsequently run tests.

        Typically this routine will spawn DumpRenderTree in a config
        ready for subsequent input.

        port - reference back to the port object.
        worker_number - identifier for a particular worker/driver instance
        """
        self._port = port
        self._worker_number = worker_number
        self._no_timeout = no_timeout

        self._driver_tempdir = None
        # WebKitTestRunner can report back subprocess crashes by printing
        # "#CRASHED - PROCESSNAME".  Since those can happen at any time
        # and ServerProcess won't be aware of them (since the actual tool
        # didn't crash, just a subprocess) we record the crashed subprocess name here.
        self._crashed_process_name = None
        self._crashed_pid = None

        # WebKitTestRunner can report back subprocesses that became unresponsive
        # This could mean they crashed.
        self._subprocess_was_unresponsive = False

        # stderr reading is scoped on a per-test (not per-block) basis, so we store the accumulated
        # stderr output, as well as if we've seen #EOF on this driver instance.
        # FIXME: We should probably remove _read_first_block and _read_optional_image_block and
        # instead scope these locally in run_test.
        self.error_from_test = str()
        self.err_seen_eof = False
        self._server_process = None

        self._measurements = {}
        if self._port.get_option("profile"):
            profiler_name = self._port.get_option("profiler")
            self._profiler = ProfilerFactory.create_profiler(self._port.host,
                self._port._path_to_driver(), self._port.results_directory(), profiler_name)
        else:
            self._profiler = None

    def __del__(self):
        self.stop()

    def run_test(self, driver_input, stop_when_done):
        """Run a single test and return the results.

        Note that it is okay if a test times out or crashes and leaves
        the driver in an indeterminate state. The upper layers of the program
        are responsible for cleaning up and ensuring things are okay.

        Returns a DriverOutput object.
        """
        start_time = time.time()
        self.start(driver_input.should_run_pixel_test, driver_input.args)
        test_begin_time = time.time()
        self.error_from_test = str()
        self.err_seen_eof = False

        command = self._command_from_driver_input(driver_input)
        deadline = test_begin_time + int(driver_input.timeout) / 1000.0

        self._server_process.write(command)
        text, audio = self._read_first_block(deadline)  # First block is either text or audio
        image, actual_image_hash = self._read_optional_image_block(deadline)  # The second (optional) block is image data.

        crashed = self.has_crashed()
        timed_out = self._server_process.timed_out
        pid = self._server_process.pid()

        if stop_when_done or crashed or timed_out:
            # We call stop() even if we crashed or timed out in order to get any remaining stdout/stderr output.
            # In the timeout case, we kill the hung process as well.
            out, err = self._server_process.stop(self._port.driver_stop_timeout() if stop_when_done else 0.0)
            if out:
                text += out
            if err:
                self.error_from_test += err
            self._server_process = None

        crash_log = None
        if crashed:
            self.error_from_test, crash_log = self._get_crash_log(text, self.error_from_test, newer_than=start_time)

            # If we don't find a crash log use a placeholder error message instead.
            if not crash_log:
                pid_str = str(self._crashed_pid) if self._crashed_pid else "unknown pid"
                crash_log = 'No crash log found for %s:%s.\n' % (self._crashed_process_name, pid_str)
                # If we were unresponsive append a message informing there may not have been a crash.
                if self._subprocess_was_unresponsive:
                    crash_log += 'Process failed to become responsive before timing out.\n'

                # Print stdout and stderr to the placeholder crash log; we want as much context as possible.
                if self.error_from_test:
                    crash_log += '\nstdout:\n%s\nstderr:\n%s\n' % (text, self.error_from_test)

        return DriverOutput(text, image, actual_image_hash, audio,
            crash=crashed, test_time=time.time() - test_begin_time, measurements=self._measurements,
            timeout=timed_out, error=self.error_from_test,
            crashed_process_name=self._crashed_process_name,
            crashed_pid=self._crashed_pid, crash_log=crash_log, pid=pid)

    def _get_crash_log(self, stdout, stderr, newer_than):
        return self._port._get_crash_log(self._crashed_process_name, self._crashed_pid, stdout, stderr, newer_than)

    # FIXME: Seems this could just be inlined into callers.
    @classmethod
    def _command_wrapper(cls, wrapper_option):
        # Hook for injecting valgrind or other runtime instrumentation,
        # used by e.g. tools/valgrind/valgrind_tests.py.
        return shlex.split(wrapper_option) if wrapper_option else []

    HTTP_DIR = "http/tests/"
    HTTP_LOCAL_DIR = "http/tests/local/"

    def is_http_test(self, test_name):
        return test_name.startswith(self.HTTP_DIR) and not test_name.startswith(self.HTTP_LOCAL_DIR)

    def test_to_uri(self, test_name):
        """Convert a test name to a URI."""
        if not self.is_http_test(test_name):
            return path.abspath_to_uri(self._port.host.platform, self._port.abspath_for_test(test_name))

        relative_path = test_name[len(self.HTTP_DIR):]

        # TODO(dpranke): remove the SSL reference?
        if relative_path.startswith("ssl/"):
            return "https://127.0.0.1:8443/" + relative_path
        return "http://127.0.0.1:8000/" + relative_path

    def uri_to_test(self, uri):
        """Return the base layout test name for a given URI.

        This returns the test name for a given URI, e.g., if you passed in
        "file:///src/LayoutTests/fast/html/keygen.html" it would return
        "fast/html/keygen.html".

        """
        if uri.startswith("file:///"):
            prefix = path.abspath_to_uri(self._port.host.platform, self._port.layout_tests_dir())
            if not prefix.endswith('/'):
                prefix += '/'
            return uri[len(prefix):]
        if uri.startswith("http://"):
            return uri.replace('http://127.0.0.1:8000/', self.HTTP_DIR)
        if uri.startswith("https://"):
            return uri.replace('https://127.0.0.1:8443/', self.HTTP_DIR)
        raise NotImplementedError('unknown url type: %s' % uri)

    def has_crashed(self):
        if self._server_process is None:
            return False
        if self._crashed_process_name:
            return True
        if self._server_process.has_crashed():
            self._crashed_process_name = self._server_process.name()
            self._crashed_pid = self._server_process.pid()
            return True
        return False

    def start(self, pixel_tests, per_test_args):
        # FIXME: Callers shouldn't normally call this, since this routine
        # may not be specifying the correct combination of pixel test and
        # per_test args.
        #
        # The only reason we have this routine at all is so the perftestrunner
        # can pause before running a test; it might be better to push that
        # into run_test() directly.
        if not self._server_process:
            self._start(pixel_tests, per_test_args)
            self._run_post_start_tasks()

    def _setup_environ_for_driver(self, environment):
        environment['DYLD_LIBRARY_PATH'] = self._port._build_path()
        environment['DYLD_FRAMEWORK_PATH'] = self._port._build_path()
        # FIXME: We're assuming that WebKitTestRunner checks this DumpRenderTree-named environment variable.
        # FIXME: Commented out for now to avoid tests breaking. Re-enable after
        # we cut over to NRWT
        #environment['DUMPRENDERTREE_TEMP'] = str(self._port._driver_tempdir_for_environment())
        environment['DUMPRENDERTREE_TEMP'] = str(self._driver_tempdir)
        environment['LOCAL_RESOURCE_ROOT'] = self._port.layout_tests_dir()
        if 'WEBKIT_OUTPUTDIR' in os.environ:
            environment['WEBKIT_OUTPUTDIR'] = os.environ['WEBKIT_OUTPUTDIR']
        if self._profiler:
            environment = self._profiler.adjusted_environment(environment)
        return environment

    def _start(self, pixel_tests, per_test_args):
        self.stop()
        self._driver_tempdir = self._port._driver_tempdir()
        server_name = self._port.driver_name()
        environment = self._port.setup_environ_for_server(server_name)
        environment = self._setup_environ_for_driver(environment)
        self._crashed_process_name = None
        self._crashed_pid = None
        self._server_process = self._port._server_process_constructor(self._port, server_name, self.cmd_line(pixel_tests, per_test_args), environment)
        self._server_process.start()

    def _run_post_start_tasks(self):
        # Remote drivers may override this to delay post-start tasks until the server has ack'd.
        if self._profiler:
            self._profiler.attach_to_pid(self._pid_on_target())

    def _pid_on_target(self):
        # Remote drivers will override this method to return the pid on the device.
        return self._server_process.pid()

    def stop(self):
        if self._server_process:
            self._server_process.stop(self._port.driver_stop_timeout())
            self._server_process = None
            if self._profiler:
                self._profiler.profile_after_exit()

        if self._driver_tempdir:
            self._port._filesystem.rmtree(str(self._driver_tempdir))
            self._driver_tempdir = None

    def cmd_line(self, pixel_tests, per_test_args):
        cmd = self._command_wrapper(self._port.get_option('wrapper'))
        cmd.append(self._port._path_to_driver())
        if self._port.get_option('gc_between_tests'):
            cmd.append('--gc-between-tests')
        if self._port.get_option('complex_text'):
            cmd.append('--complex-text')
        if self._port.get_option('threaded'):
            cmd.append('--threaded')
        if self._no_timeout:
            cmd.append('--no-timeout')
        # FIXME: We need to pass --timeout=SECONDS to WebKitTestRunner for WebKit2.

        cmd.extend(self._port.get_option('additional_drt_flag', []))
        cmd.extend(self._port.additional_drt_flag())

        cmd.extend(per_test_args)

        cmd.append('-')
        return cmd

    def _check_for_driver_crash(self, error_line):
        if error_line == "#CRASHED\n":
            # This is used on Windows to report that the process has crashed
            # See http://trac.webkit.org/changeset/65537.
            self._crashed_process_name = self._server_process.name()
            self._crashed_pid = self._server_process.pid()
        elif (error_line.startswith("#CRASHED - ")
            or error_line.startswith("#PROCESS UNRESPONSIVE - ")):
            # WebKitTestRunner uses this to report that the WebProcess subprocess crashed.
            match = re.match('#(?:CRASHED|PROCESS UNRESPONSIVE) - (\S+)', error_line)
            self._crashed_process_name = match.group(1) if match else 'WebProcess'
            match = re.search('pid (\d+)', error_line)
            pid = int(match.group(1)) if match else None
            self._crashed_pid = pid
            # FIXME: delete this after we're sure this code is working :)
            _log.debug('%s crash, pid = %s, error_line = %s' % (self._crashed_process_name, str(pid), error_line))
            if error_line.startswith("#PROCESS UNRESPONSIVE - "):
                self._subprocess_was_unresponsive = True
                self._port.sample_process(self._crashed_process_name, self._crashed_pid)
                # We want to show this since it's not a regular crash and probably we don't have a crash log.
                self.error_from_test += error_line
            return True
        return self.has_crashed()

    def _command_from_driver_input(self, driver_input):
        # FIXME: performance tests pass in full URLs instead of test names.
        if driver_input.test_name.startswith('http://') or driver_input.test_name.startswith('https://')  or driver_input.test_name == ('about:blank'):
            command = driver_input.test_name
        elif self.is_http_test(driver_input.test_name):
            command = self.test_to_uri(driver_input.test_name)
        else:
            command = self._port.abspath_for_test(driver_input.test_name)
            if sys.platform == 'cygwin':
                command = path.cygpath(command)

        assert not driver_input.image_hash or driver_input.should_run_pixel_test

        # ' is the separator between arguments.
        if self._port.supports_per_test_timeout():
            command += "'--timeout'%s" % driver_input.timeout
        if driver_input.should_run_pixel_test:
            command += "'--pixel-test"
        if driver_input.image_hash:
            command += "'" + driver_input.image_hash
        return command + "\n"

    def _read_first_block(self, deadline):
        # returns (text_content, audio_content)
        block = self._read_block(deadline)
        if block.malloc:
            self._measurements['Malloc'] = float(block.malloc)
        if block.js_heap:
            self._measurements['JSHeap'] = float(block.js_heap)
        if block.content_type == 'audio/wav':
            return (None, block.decoded_content)
        return (block.decoded_content, None)

    def _read_optional_image_block(self, deadline):
        # returns (image, actual_image_hash)
        block = self._read_block(deadline, wait_for_stderr_eof=True)
        if block.content and block.content_type == 'image/png':
            return (block.decoded_content, block.content_hash)
        return (None, block.content_hash)

    def _read_header(self, block, line, header_text, header_attr, header_filter=None):
        if line.startswith(header_text) and getattr(block, header_attr) is None:
            value = line.split()[1]
            if header_filter:
                value = header_filter(value)
            setattr(block, header_attr, value)
            return True
        return False

    def _process_stdout_line(self, block, line):
        if (self._read_header(block, line, 'Content-Type: ', 'content_type')
            or self._read_header(block, line, 'Content-Transfer-Encoding: ', 'encoding')
            or self._read_header(block, line, 'Content-Length: ', '_content_length', int)
            or self._read_header(block, line, 'ActualHash: ', 'content_hash')
            or self._read_header(block, line, 'DumpMalloc: ', 'malloc')
            or self._read_header(block, line, 'DumpJSHeap: ', 'js_heap')):
            return
        # Note, we're not reading ExpectedHash: here, but we could.
        # If the line wasn't a header, we just append it to the content.
        block.content += line

    def _strip_eof(self, line):
        if line and line.endswith("#EOF\n"):
            return line[:-5], True
        return line, False

    def _read_block(self, deadline, wait_for_stderr_eof=False):
        block = ContentBlock()
        out_seen_eof = False

        while not self.has_crashed():
            if out_seen_eof and (self.err_seen_eof or not wait_for_stderr_eof):
                break

            if self.err_seen_eof:
                out_line = self._server_process.read_stdout_line(deadline)
                err_line = None
            elif out_seen_eof:
                out_line = None
                err_line = self._server_process.read_stderr_line(deadline)
            else:
                out_line, err_line = self._server_process.read_either_stdout_or_stderr_line(deadline)

            if self._server_process.timed_out or self.has_crashed():
                break

            if out_line:
                assert not out_seen_eof
                out_line, out_seen_eof = self._strip_eof(out_line)
            if err_line:
                assert not self.err_seen_eof
                err_line, self.err_seen_eof = self._strip_eof(err_line)

            if out_line:
                if out_line[-1] != "\n":
                    _log.error("Last character read from DRT stdout line was not a newline!  This indicates either a NRWT or DRT bug.")
                content_length_before_header_check = block._content_length
                self._process_stdout_line(block, out_line)
                # FIXME: Unlike HTTP, DRT dumps the content right after printing a Content-Length header.
                # Don't wait until we're done with headers, just read the binary blob right now.
                if content_length_before_header_check != block._content_length:
                    block.content = self._server_process.read_stdout(deadline, block._content_length)

            if err_line:
                if self._check_for_driver_crash(err_line):
                    break
                self.error_from_test += err_line

        block.decode_content()
        return block


class ContentBlock(object):
    def __init__(self):
        self.content_type = None
        self.encoding = None
        self.content_hash = None
        self._content_length = None
        # Content is treated as binary data even though the text output is usually UTF-8.
        self.content = str()  # FIXME: Should be bytearray() once we require Python 2.6.
        self.decoded_content = None
        self.malloc = None
        self.js_heap = None

    def decode_content(self):
        if self.encoding == 'base64' and self.content is not None:
            self.decoded_content = base64.b64decode(self.content)
        else:
            self.decoded_content = self.content

class DriverProxy(object):
    """A wrapper for managing two Driver instances, one with pixel tests and
    one without. This allows us to handle plain text tests and ref tests with a
    single driver."""

    def __init__(self, port, worker_number, driver_instance_constructor, pixel_tests, no_timeout):
        self._port = port
        self._worker_number = worker_number
        self._driver_instance_constructor = driver_instance_constructor
        self._no_timeout = no_timeout

        # FIXME: We shouldn't need to create a driver until we actually run a test.
        self._driver = self._make_driver(pixel_tests)
        self._driver_cmd_line = None

    def _make_driver(self, pixel_tests):
        return self._driver_instance_constructor(self._port, self._worker_number, pixel_tests, self._no_timeout)

    # FIXME: this should be a @classmethod (or implemented on Port instead).
    def is_http_test(self, test_name):
        return self._driver.is_http_test(test_name)

    # FIXME: this should be a @classmethod (or implemented on Port instead).
    def test_to_uri(self, test_name):
        return self._driver.test_to_uri(test_name)

    # FIXME: this should be a @classmethod (or implemented on Port instead).
    def uri_to_test(self, uri):
        return self._driver.uri_to_test(uri)

    def run_test(self, driver_input, stop_when_done):
        base = self._port.lookup_virtual_test_base(driver_input.test_name)
        if base:
            virtual_driver_input = copy.copy(driver_input)
            virtual_driver_input.test_name = base
            virtual_driver_input.args = self._port.lookup_virtual_test_args(driver_input.test_name)
            return self.run_test(virtual_driver_input, stop_when_done)

        pixel_tests_needed = driver_input.should_run_pixel_test
        cmd_line_key = self._cmd_line_as_key(pixel_tests_needed, driver_input.args)
        if cmd_line_key != self._driver_cmd_line:
            self._driver.stop()
            self._driver = self._make_driver(pixel_tests_needed)
            self._driver_cmd_line = cmd_line_key

        return self._driver.run_test(driver_input, stop_when_done)

    def has_crashed(self):
        return self._driver.has_crashed()

    def stop(self):
        self._driver.stop()

    # FIXME: this should be a @classmethod (or implemented on Port instead).
    def cmd_line(self, pixel_tests=None, per_test_args=None):
        return self._driver.cmd_line(pixel_tests, per_test_args or [])

    def _cmd_line_as_key(self, pixel_tests, per_test_args):
        return ' '.join(self.cmd_line(pixel_tests, per_test_args))
