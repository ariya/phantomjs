# Copyright 2010 Google Inc. All rights reserved.
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

{
  'includes': [
    '../../../build/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'client_tests',
      'type': 'executable',
      'sources': [
        'exception_handler_test.h',
        'exception_handler_test.cc',
        'exception_handler_death_test.cc',
        'exception_handler_nesting_test.cc',
        'minidump_test.cc',
        'dump_analysis.cc',
        'dump_analysis.h',
        'crash_generation_server_test.cc'
      ],
      'dependencies': [
        'testing.gyp:gtest',
        'testing.gyp:gmock',
        '../breakpad_client.gyp:common',
        '../crash_generation/crash_generation.gyp:crash_generation_server',
        '../crash_generation/crash_generation.gyp:crash_generation_client',
        '../handler/exception_handler.gyp:exception_handler',
	'processor_bits',
      ]
    },
    {
      'target_name': 'processor_bits',
      'type': 'static_library',
      'include_dirs': [
        '<(DEPTH)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(DEPTH)',
        ]
      },
      'sources': [
        '<(DEPTH)/common/string_conversion.cc',
        '<(DEPTH)/processor/basic_code_modules.cc',
        '<(DEPTH)/processor/logging.cc',
        '<(DEPTH)/processor/minidump.cc',
        '<(DEPTH)/processor/pathname_stripper.cc',
      ]
    }
  ],
}
