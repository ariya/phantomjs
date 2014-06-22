# Copyright (C) 2012 Google Inc. All rights reserved.
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

import logging
import optparse
import signal
import traceback

from webkitpy.common.host import Host
from webkitpy.layout_tests.models import test_expectations
from webkitpy.port import platform_options


# This mirrors what the shell normally does.
INTERRUPTED_EXIT_STATUS = signal.SIGINT + 128

# This is a randomly chosen exit code that can be tested against to
# indicate that an unexpected exception occurred.
EXCEPTIONAL_EXIT_STATUS = 254

_log = logging.getLogger(__name__)


def lint(host, options, logging_stream):
    logger = logging.getLogger()
    logger.setLevel(logging.INFO)
    handler = logging.StreamHandler(logging_stream)
    logger.addHandler(handler)

    try:
        ports_to_lint = [host.port_factory.get(name) for name in host.port_factory.all_port_names(options.platform)]
        files_linted = set()
        lint_failed = False

        for port_to_lint in ports_to_lint:
            expectations_dict = port_to_lint.expectations_dict()

            # FIXME: This won't work if multiple ports share a TestExpectations file but support different modifiers in the file.
            for expectations_file in expectations_dict.keys():
                if expectations_file in files_linted:
                    continue

                try:
                    test_expectations.TestExpectations(port_to_lint,
                        expectations_to_lint={expectations_file: expectations_dict[expectations_file]})
                except test_expectations.ParseError as e:
                    lint_failed = True
                    _log.error('')
                    for warning in e.warnings:
                        _log.error(warning)
                    _log.error('')
                files_linted.add(expectations_file)

        if lint_failed:
            _log.error('Lint failed.')
            return -1

        _log.info('Lint succeeded.')
        return 0
    finally:
        logger.removeHandler(handler)


def main(argv, _, stderr):
    parser = optparse.OptionParser(option_list=platform_options(use_globs=True))
    options, _ = parser.parse_args(argv)

    if options.platform and 'test' in options.platform:
        # It's a bit lame to import mocks into real code, but this allows the user
        # to run tests against the test platform interactively, which is useful for
        # debugging test failures.
        from webkitpy.common.host_mock import MockHost
        host = MockHost()
    else:
        host = Host()

    try:
        exit_status = lint(host, options, stderr)
    except KeyboardInterrupt:
        exit_status = INTERRUPTED_EXIT_STATUS
    except Exception as e:
        print >> stderr, '\n%s raised: %s' % (e.__class__.__name__, str(e))
        traceback.print_exc(file=stderr)
        exit_status = EXCEPTIONAL_EXIT_STATUS

    return exit_status
