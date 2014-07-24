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

import unittest2 as unittest

from .contributionareas import _Intersection
from .contributionareas import _Area
from .contributionareas import ContributionAreas
from webkitpy.common.system.filesystem_mock import MockFileSystem


class ContributionAreasTest(unittest.TestCase):

    def test_contribution(self):
        self.assertEqual(_Area('CSS').tokens(), ['css'])
        self.assertEqual(_Area('Forms', ['input']).tokens(), ['input'])

    def _assert_areas_for_touched_files(self, areas, files, expected_areas):
        self.assertEqual(areas.areas_for_touched_files(files), set(expected_areas))

    def test_areas_for_touched_files(self):
        areas = ContributionAreas(MockFileSystem(), [
            _Area('CSS'),
            _Area('HTML'),
            _Area('Forms', ['forms', 'input']),
            _Area('CSS Transforms', [_Intersection('css', 'transforms')]),
        ])
        self._assert_areas_for_touched_files(areas, [], [])
        self._assert_areas_for_touched_files(areas, ['WebCore/css'], ['CSS'])
        self._assert_areas_for_touched_files(areas, ['WebCore/html/'], ['HTML'])
        self._assert_areas_for_touched_files(areas, ['WebCore/css/CSSStyleSelector.cpp', 'WebCore/html/HTMLIFrameElement.h'], ['CSS', 'HTML'])
        self._assert_areas_for_touched_files(areas, ['WebCore'], [])
        self._assert_areas_for_touched_files(areas, ['WebCore/html2'], [])
        self._assert_areas_for_touched_files(areas, ['WebCore/html/HTMLInputElement.cpp'], ['HTML', 'Forms'])
        self._assert_areas_for_touched_files(areas, ['WebCore/svg/transforms'], [])
        self._assert_areas_for_touched_files(areas, ['WebCore/css/transforms'], ['CSS', 'CSS Transforms'])
