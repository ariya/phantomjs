# Copyright (c) 2009 Google Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'func1',
      'type': 'static_library',
      'sources': ['func1.c'],
    },
    {
      'target_name': 'clone_func1',
      'type': 'none',
      'dependencies': ['func1'],
      'actions': [
        {
          'action_name': 'cloning library',
          'inputs': [
            '<(LIB_DIR)/<(STATIC_LIB_PREFIX)func1<(STATIC_LIB_SUFFIX)'
          ],
          'outputs': ['<(PRODUCT_DIR)/alternate/'
                      '<(STATIC_LIB_PREFIX)cloned<(STATIC_LIB_SUFFIX)'],
          'destination': '<(PRODUCT_DIR)',
          'action': ['python', 'copy.py', '<@(_inputs)', '<@(_outputs)'],
          'msvs_cygwin_shell': 0,
        },
      ],
    },
    {
      'target_name': 'copy_cloned',
      'type': 'none',
      'dependencies': ['clone_func1'],
      'copies': [
        {
          'destination': '<(LIB_DIR)',
          'files': [
            '<(PRODUCT_DIR)/alternate/'
            '<(STATIC_LIB_PREFIX)cloned<(STATIC_LIB_SUFFIX)',
          ],
        },
      ],
    },
    {
      'target_name': 'use_cloned',
      'type': 'executable',
      'sources': ['main.c'],
      'dependencies': ['copy_cloned'],
      'link_settings': {
        'conditions': [
          ['OS=="win"', {
            'libraries': ['-l"<(LIB_DIR)/cloned.lib"'],
          }, {
            'libraries': ['-lcloned'],
            'ldflags': ['-L <(LIB_DIR)'],
          }],
        ],
      },
    },
  ],
}
