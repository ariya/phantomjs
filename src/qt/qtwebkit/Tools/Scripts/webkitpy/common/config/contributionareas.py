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

import re


class _Intersection(object):
    def __init__(self, *tokens):
        self._tokens = tokens

    def matches(self, tokens):
        for token in self._tokens:
            if token not in tokens and (token + 's') not in tokens:
                return False
        return True


class _Area(object):
    def __init__(self, name, tokens=None):
        self._name = name
        self._tokens = tokens if tokens else [self._name_to_token(name)]

    def _name_to_token(self, word):
        token = word.lower()
        return token[:-1] if word[-1] == 's' else token

    def matches(self, tokens):
        # FIXME: Support pluraization properly
        for token in self._tokens:
            if isinstance(token, _Intersection):
                if token.matches(tokens):
                    return True
            elif token in tokens or (token + 's') in tokens:
                return True
        return False

    def name(self):
        return self._name

    def tokens(self):
        return self._tokens

contribution_areas = [
    _Area('ARM JIT', ['arm']),
# FIXME: 'Accelerated compositing / GPU acceleration'
    _Area('Accessibility'),
    _Area('Android port', ['android']),
    _Area('Animation', ['animation', 'animator']),
    _Area('Apple\'s Windows port', ['win', 'windows']),  # FIXME: need to exclude chromium...
    _Area('Autotools Build', ['autotools']),
    _Area('Basic types and data structures (WTF)', ['wtf']),
# FIXME: 'Bidirectional text'
# FIXME: 'Build/test infrastructure (stuff under Tools/Scripts)'
    _Area('CMake Build', ['cmakelist']),
    _Area('CSS (Cascading Style Sheets)', ['css']),
    _Area('CSS Transforms', [_Intersection('css', 'transforms')]),
    _Area('CSS/SVG Filters', [_Intersection('css', 'filters'), _Intersection('svg', 'filters')]),
    _Area('CURL HTTP Backend', ['CURL']),
    _Area('Resource Cache', [_Intersection('loader', 'cache')]),
    _Area('Memory Cache', [_Intersection('graphics', 'cache')]),
    _Area('Cairo'),
    _Area('Canvas'),
    _Area('Chromium Linux', [_Intersection('chromium', 'linux')]),
# FIXME: 'Commit Queue'
    _Area('Core DOM', ['dom']),
    _Area('Core Graphics', ['cg']),
    _Area('Bindings'),
    _Area('DOM Storage', ['storage']),
    _Area('Drag and Drop', ['drag']),
    _Area('DumpRenderTree'),
    _Area('EFL', ['efl']),
    _Area('Editing / Selection', ['editing']),
    _Area('Event Handling', ['event']),
    _Area('FastMalloc'),
    _Area('File API', ['fileapi']),
    _Area('Fonts'),
    _Area('Forms'),
# FIXME: 'Frame Flattening'
    _Area('Frame Loader'),
# FIXME: 'General' Maybe auto-detect people contributing to all subdirectories?
    _Area('Geolocation API', ['geolocation']),
    _Area('Graphics', ['graphics']),
    _Area('HTML', ['html']),
    _Area('HTML Parser', [_Intersection('html', 'parser')]),  # FIXME: matches html/track/WebVTTParser.cpp
    _Area('HTML5 Media Support', ['media']),
    _Area('History', ['history']),
# FIXME: 'Hit testing'
    _Area('Image Decoder', ['imagedecoder']),
# FIXME: 'Input methods'
    _Area('JSC Bindings', [_Intersection('bindings', 'js')]),
    _Area('JavaScriptCore'),
    _Area('JavaScriptCore Regular Expressions', [_Intersection('JavaScriptCore', 'regexp')]),
# FIXME: 'Layout tests' but what does it mean to say you're an expert on layout tests? Maybe worked on tools?
    _Area('Loader', ['loader']),
    _Area('MathML'),
    _Area('Memory Use / Leaks', ['leaks']),  # Probably need more tokens
    _Area('MessagePorts'),
    _Area('Network', [_Intersection('platform', 'network')]),
    _Area('new-run-webkit-tests', ['layout_tests']),
    _Area('OpenVG graphics backend', ['openvg']),
# FIXME: 'Performance'
    _Area('Plug-ins', ['plugins']),
    _Area('Printing', ['printing', 'print']),
    _Area('Rendering'),
    _Area('SVG (Scalable Vector Graphics)', ['svg']),
    _Area('Scrollbars', ['scroll']),
    _Area('Security'),  # Probably need more tokens
# FIXME: 'Shadow DOM'
    _Area('Soup Network Backend', ['soup']),
# FIXME: 'Spell Checking' just need tokens
    _Area('Tables', ['htmltable', 'rendertable']),
# FIXME: 'Text Encoding'
# FIXME: 'Text Layout'
    _Area('The Chromium Port', ['chromium']),
    _Area('The EFLWebKit Port', ['efl']),
    _Area('The WebKitGTK+ Port', ['gtk']),
    _Area('The Haiku Port', ['haiku']),
    _Area('The QtWebKit Port', ['qt']),
    _Area('The WinCE Port', ['wince']),
    _Area('The WinCairo Port', ['cairo']),
    _Area('Threading', ['thread']),
    _Area('Tools'),
    _Area('Touch Support', ['touch']),
    _Area('Transforms', ['transforms']),  # There's also CSS transforms
    _Area('Transitions', ['transitions']),  # This only matches transition events at the moment
    _Area('URL Parsing', ['KURL']),  # Probably need more tokens
    _Area('V8', ['v8']),
    _Area('V8 Bindings', [_Intersection('bindings', 'v8')]),
    _Area('Web Inspector / Developer Tools', ['inspector']),
    _Area('Web Timing', ['PerformanceNavigation', 'PerformanceTiming']),  # more tokens?
    _Area('WebArchive'),
    _Area('WebCore Icon Database', ['icon']),
    _Area('WebGL', ['webgl']),
    _Area('WebKit Websites', ['websites']),
    _Area('WebKit2'),
    _Area('WebSQL Databases', [_Intersection('storage', 'database')]),
    _Area('WebSockets'),
    _Area('Workers'),
    _Area('XML'),
    _Area('XMLHttpRequest'),
    _Area('XSLT'),
    _Area('XSSAuditor'),
    _Area('WebKit API Tests', ['TestWebKitAPI']),
    _Area('webkit-patch', [_Intersection('webkitpy', 'commands')]),
]


class ContributionAreas(object):
    def __init__(self, filesystem, table=contribution_areas):
        self._filesystem = filesystem
        self._contribution_areas = table

    def names(self):
        return [area.name() for area in self._contribution_areas]

    def _split_path(self, path):
        result = []
        while path and len(path):
            next_path, tail = self._filesystem.split(path)
            if path == next_path:
                break
            if tail and len(tail):
                result.append(tail)
            path = next_path
        return result

    def _split_camelcase(self, name, transform=lambda x: x):
        result = []
        while name and len(name):
            m = re.match('^([A-Z][a-z0-9]+)|([A-Z0-9]+(?=([A-Z][a-z0-9]|\.|$)))', name)
            if m:
                result.append(transform(m.group(0)))
                name = name[m.end():]
            else:
                return result
        return result

    def areas_for_touched_files(self, touched_files):
        areas = set()
        for file_path in touched_files:
            split_file_path = self._split_path(file_path)
            tokenized_file_path = None
            tokenized_file_path = sum([self._split_camelcase(token, lambda x: x.lower()) for token in split_file_path], [])
            for area in self._contribution_areas:
                if area.matches(split_file_path) or area.matches(tokenized_file_path):
                    areas.add(area.name())
        return areas
