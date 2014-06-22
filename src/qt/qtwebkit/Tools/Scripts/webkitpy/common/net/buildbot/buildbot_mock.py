# Copyright (C) 2011 Google Inc. All rights reserved.
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

import logging

_log = logging.getLogger(__name__)


class MockBuild(object):
    def __init__(self, build_number, revision, is_green):
        self._number = build_number
        self._revision = revision
        self._is_green = is_green

class MockBuilder(object):
    def __init__(self, name):
        self._name = name

    def name(self):
        return self._name

    def build(self, build_number):
        return MockBuild(build_number=build_number, revision=1234, is_green=False)

    def results_url(self):
        return "http://example.com/builders/%s/results" % self.name()

    def accumulated_results_url(self):
        return "http://example.com/f/builders/%s/results/layout-test-results" % self.name()

    def latest_layout_test_results_url(self):
        return self.accumulated_results_url()

    def force_build(self, username, comments):
        _log.info("MOCK: force_build: name=%s, username=%s, comments=%s" % (
            self._name, username, comments))


class MockFailureMap(object):
    def __init__(self, buildbot):
        self._buildbot = buildbot

    def is_empty(self):
        return False

    def filter_out_old_failures(self, is_old_revision):
        pass

    def failing_revisions(self):
        return [29837]

    def builders_failing_for(self, revision):
        return [self._buildbot.builder_with_name("Builder1")]

    def tests_failing_for(self, revision):
        return ["mock-test-1"]

    def failing_tests(self):
        return set(["mock-test-1"])


class MockBuildBot(object):
    def __init__(self):
        self._mock_builder1_status = {
            "name": "Builder1",
            "is_green": True,
            "activity": "building",
        }
        self._mock_builder2_status = {
            "name": "Builder2",
            "is_green": True,
            "activity": "idle",
        }

    def builder_with_name(self, name):
        return MockBuilder(name)

    def builder_statuses(self):
        return [
            self._mock_builder1_status,
            self._mock_builder2_status,
        ]

    def light_tree_on_fire(self):
        self._mock_builder2_status["is_green"] = False

    def failure_map(self):
        return MockFailureMap(self)
