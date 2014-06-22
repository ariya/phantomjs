# Copyright 2013 Google Inc. All rights reserved.
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
    '../../build/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'dia_sdk',
      'type': 'none',
      'all_dependent_settings': {
        'include_dirs': [
          '<(DEPTH)',
          '$(VSInstallDir)\DIA SDK\include',
        ],
        'msvs_settings': {
          'VCLinkerTool': {
            'AdditionalDependencies': [
              '$(VSInstallDir)\DIA SDK\lib\diaguids.lib',
              'imagehlp.lib',
            ],
          },
        },
      },
    },
    {
      'target_name': 'common_windows_lib',
      'type': 'static_library',
      'sources': [
        'dia_util.cc',
        'dia_util.h',
        'guid_string.cc',
        'guid_string.h',
        'http_upload.cc',
        'http_upload.h',
        'omap.cc',
        'omap.h',
        'omap_internal.h',
        'pdb_source_line_writer.cc',
        'pdb_source_line_writer.h',
        'string_utils.cc',
        'string_utils-inl.h',
      ],
      'dependencies': [
        'dia_sdk',
      ],
    },
    {
      'target_name': 'common_windows_unittests',
      'type': 'executable',
      'sources': [
        'omap_unittest.cc',
      ],
      'dependencies': [
        '<(DEPTH)/client/windows/unittests/testing.gyp:gmock',
        '<(DEPTH)/client/windows/unittests/testing.gyp:gtest',
        'common_windows_lib',
      ],
    },
  ],
}
