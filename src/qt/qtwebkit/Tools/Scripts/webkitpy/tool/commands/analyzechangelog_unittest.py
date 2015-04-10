# Cpyright (c) 2011 Google Inc. All rights reserved.
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
import sys
from webkitpy.common.config.contributionareas import ContributionAreas
from webkitpy.common.host_mock import MockHost
from webkitpy.common.system.filesystem_mock import MockFileSystem
from webkitpy.common.system.executive import Executive
from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.tool.commands.analyzechangelog import AnalyzeChangeLog
from webkitpy.tool.commands.analyzechangelog import ChangeLogAnalyzer
from webkitpy.tool.commands.commandtest import CommandsTest
from webkitpy.tool.multicommandtool import Command


class AnalyzeChangeLogTest(CommandsTest):
    def test_enumerate_enumerate_changelogs(self):
        filesystem = MockFileSystem({
            'foo/ChangeLog': '',
            'foo/ChangeLog-2010-06-23': '',
            'foo/ChangeLog-2010-12-31': '',
            'foo/ChangeLog-x': '',
            'foo/ChangeLog-2011-01-01': '',
        })
        changelogs = AnalyzeChangeLog._enumerate_changelogs(filesystem, 'foo/', None)
        self.assertEqual(changelogs, ['foo/ChangeLog', 'foo/ChangeLog-2011-01-01', 'foo/ChangeLog-2010-12-31', 'foo/ChangeLog-2010-06-23'])

        changelogs = AnalyzeChangeLog._enumerate_changelogs(filesystem, 'foo/', 2)
        self.assertEqual(changelogs, ['foo/ChangeLog', 'foo/ChangeLog-2011-01-01'])

    def test_generate_jsons(self):
        filesystem = MockFileSystem()
        test_json = {'array.json': [1, 2, 3, {'key': 'value'}], 'dictionary.json': {'somekey': 'somevalue', 'array': [4, 5]}}

        capture = OutputCapture()
        capture.capture_output()

        AnalyzeChangeLog._generate_jsons(filesystem, test_json, 'bar')
        self.assertEqual(set(filesystem.files.keys()), set(['bar/array.json', 'bar/dictionary.json']))

        capture.restore_output()

        self.assertEqual(json.loads(filesystem.files['bar/array.json']), test_json['array.json'])
        self.assertEqual(json.loads(filesystem.files['bar/dictionary.json']), test_json['dictionary.json'])


class ChangeLogAnalyzerTest(CommandsTest):
    def test_analyze_one_changelog(self):
        host = MockHost()
        host.filesystem.files['mock-checkout/foo/ChangeLog'] = u"""2011-11-17  Mark Rowe  <mrowe@apple.com>

    <http://webkit.org/b/72646> Disable deprecation warnings around code where we cannot easily
    switch away from the deprecated APIs.

    Reviewed by Sam Weinig.

    * platform/mac/WebCoreNSStringExtras.mm:
    * platform/network/cf/SocketStreamHandleCFNet.cpp:
    (WebCore::SocketStreamHandle::reportErrorToClient):

2011-11-19  Kevin Ollivier  <kevino@theolliviers.com>

    [wx] C++ bindings build fix for move of array classes to WTF.

    * bindings/scripts/CodeGeneratorCPP.pm:
    (GetCPPTypeGetter):
    (GetNamespaceForClass):
    (GenerateHeader):
    (GenerateImplementation):

2011-10-27  Philippe Normand  <pnormand@igalia.com> and Zan Dobersek  <zandobersek@gmail.com>

        [GStreamer] WebAudio AudioFileReader implementation
        https://bugs.webkit.org/show_bug.cgi?id=69834

        Reviewed by Martin Robinson.

        Basic FileReader implementation, supporting one or 2 audio
        channels. An empty AudioDestination is also provided, its complete
        implementation is handled in bug 69835.

        * GNUmakefile.am:
        * GNUmakefile.list.am:
        * platform/audio/gstreamer/AudioDestinationGStreamer.cpp: Added.
        (WebCore::AudioDestination::create):
        (WebCore::AudioDestination::hardwareSampleRate):
        (WebCore::AudioDestinationGStreamer::AudioDestinationGStreamer):
        (WebCore::AudioDestinationGStreamer::~AudioDestinationGStreamer):
        (WebCore::AudioDestinationGStreamer::start):
        (WebCore::AudioDestinationGStreamer::stop):
        * platform/audio/gstreamer/AudioDestinationGStreamer.h: Added.
        (WebCore::AudioDestinationGStreamer::isPlaying):
        (WebCore::AudioDestinationGStreamer::sampleRate):
        (WebCore::AudioDestinationGStreamer::sourceProvider):
        * platform/audio/gstreamer/AudioFileReaderGStreamer.cpp: Added.
        (WebCore::getGStreamerAudioCaps):
        (WebCore::getFloatFromByteReader):
        (WebCore::copyGstreamerBuffersToAudioChannel):
        (WebCore::onAppsinkNewBufferCallback):
        (WebCore::messageCallback):
        (WebCore::onGStreamerDeinterleavePadAddedCallback):
        (WebCore::onGStreamerDeinterleaveReadyCallback):
        (WebCore::onGStreamerDecodebinPadAddedCallback):
        (WebCore::AudioFileReader::AudioFileReader):
        (WebCore::AudioFileReader::~AudioFileReader):
        (WebCore::AudioFileReader::handleBuffer):
        (WebCore::AudioFileReader::handleMessage):
        (WebCore::AudioFileReader::handleNewDeinterleavePad):
        (WebCore::AudioFileReader::deinterleavePadsConfigured):
        (WebCore::AudioFileReader::plugDeinterleave):
        (WebCore::AudioFileReader::createBus):
        (WebCore::createBusFromAudioFile):
        (WebCore::createBusFromInMemoryAudioFile):
        * platform/audio/gtk/AudioBusGtk.cpp: Added.
        (WebCore::AudioBus::loadPlatformResource):
"""

        capture = OutputCapture()
        capture.capture_output()

        analyzer = ChangeLogAnalyzer(host, ['mock-checkout/foo/ChangeLog'])
        analyzer.analyze()

        capture.restore_output()

        self.assertEqual(analyzer.summary(),
            {'reviewed': 2, 'unreviewed': 1, 'contributors': 6, 'contributors_with_reviews': 2, 'contributors_without_reviews': 4})

        self.assertEqual(set(analyzer.contributors_statistics().keys()),
            set(['Sam Weinig', u'Mark Rowe', u'Kevin Ollivier', 'Martin Robinson', u'Philippe Normand', u'Zan Dobersek']))

        self.assertEqual(analyzer.contributors_statistics()['Sam Weinig'],
            {'reviews': {'files': {u'foo/platform/mac/WebCoreNSStringExtras.mm': 1, u'foo/platform/network/cf/SocketStreamHandleCFNet.cpp': 1},
            'total': 1, 'areas': {'Network': 1}}, 'patches': {'files': {}, 'areas': {}, 'unreviewed': 0, 'reviewed': 0}})
        self.assertEqual(analyzer.contributors_statistics()[u'Mark Rowe'],
            {'reviews': {'files': {}, 'total': 0, 'areas': {}}, 'patches': {'files': {u'foo/platform/mac/WebCoreNSStringExtras.mm': 1,
            u'foo/platform/network/cf/SocketStreamHandleCFNet.cpp': 1}, 'areas': {'Network': 1}, 'unreviewed': 0, 'reviewed': 1}})
        self.assertEqual(analyzer.contributors_statistics()[u'Kevin Ollivier'],
            {'reviews': {'files': {}, 'total': 0, 'areas': {}}, 'patches': {'files': {u'foo/bindings/scripts/CodeGeneratorCPP.pm': 1},
                'areas': {'Bindings': 1}, 'unreviewed': 1, 'reviewed': 0}})

        files_for_audio_patch = {u'foo/GNUmakefile.am': 1, u'foo/GNUmakefile.list.am': 1, 'foo/platform/audio/gstreamer/AudioDestinationGStreamer.cpp': 1,
            'foo/platform/audio/gstreamer/AudioDestinationGStreamer.h': 1, 'foo/platform/audio/gstreamer/AudioFileReaderGStreamer.cpp': 1,
            'foo/platform/audio/gtk/AudioBusGtk.cpp': 1}
        author_expectation_for_audio_patch = {'reviews': {'files': {}, 'total': 0, 'areas': {}},
            'patches': {'files': files_for_audio_patch, 'areas': {'The WebKitGTK+ Port': 1}, 'unreviewed': 0, 'reviewed': 1}}
        self.assertEqual(analyzer.contributors_statistics()[u'Martin Robinson'],
            {'reviews': {'files': files_for_audio_patch, 'total': 1, 'areas': {'The WebKitGTK+ Port': 1}},
                'patches': {'files': {}, 'areas': {}, 'unreviewed': 0, 'reviewed': 0}})
        self.assertEqual(analyzer.contributors_statistics()[u'Philippe Normand'], author_expectation_for_audio_patch)
        self.assertEqual(analyzer.contributors_statistics()[u'Zan Dobersek'], author_expectation_for_audio_patch)

        areas_statistics = analyzer.areas_statistics()
        areas_with_patches = [area for area in areas_statistics if areas_statistics[area]['reviewed'] or areas_statistics[area]['unreviewed']]
        self.assertEqual(set(areas_with_patches), set(['Bindings', 'Network', 'The WebKitGTK+ Port']))
        self.assertEqual(areas_statistics['Bindings'], {'unreviewed': 1, 'reviewed': 0, 'contributors':
            {u'Kevin Ollivier': {'reviews': 0, 'unreviewed': 1, 'reviewed': 0}}})
        self.assertEqual(areas_statistics['Network'], {'unreviewed': 0, 'reviewed': 1, 'contributors':
            {'Sam Weinig': {'reviews': 1, 'unreviewed': 0, 'reviewed': 0}, u'Mark Rowe': {'reviews': 0, 'unreviewed': 0, 'reviewed': 1}}})
