{
  'includes': [
    '../../gyp/common.gypi',
    '../JavaScriptCore.gypi',
  ],
  'configurations': {
    'Production': {
      'xcode_config_file': '<(project_dir)/Configurations/Base.xcconfig',
    },
    'Profiling': {
      'xcode_config_file': '<(project_dir)/Configurations/DebugRelease.xcconfig',
      'xcode_settings': {
        'STRIP_INSTALLED_PRODUCT': 'NO',
      },
    },
    'Release': {
      'xcode_config_file': '<(project_dir)/Configurations/DebugRelease.xcconfig',
      'xcode_settings': {
        'STRIP_INSTALLED_PRODUCT': 'NO',
      },
    },
    'Debug': {
      'xcode_config_file': '<(project_dir)/Configurations/DebugRelease.xcconfig',
      'xcode_settings': {
        'DEAD_CODE_STRIPPING': '$(DEAD_CODE_STRIPPING_debug)',
        'DEBUG_DEFINES': '$(DEBUG_DEFINES_debug)',
        'GCC_OPTIMIZATION_LEVEL': '$(GCC_OPTIMIZATION_LEVEL_debug)',
        'STRIP_INSTALLED_PRODUCT': '$(STRIP_INSTALLED_PRODUCT_debug)',
      },
    },
  },
  'variables': {
    'javascriptcore_include_dirs': [
      '<(project_dir)',
      '<(project_dir)/icu',
    ],
  },
  'target_defaults': {
    'configurations': {
      'Profiling': {},
    },
  },
  'targets': [
    {
      'target_name': 'JavaScriptCore',
      'type': 'shared_library',
      'dependencies': [
        'Derived Sources',
        'Update Version',
      ],
      'include_dirs': [
        '<@(javascriptcore_include_dirs)',
        '<(PRODUCT_DIR)/DerivedSources/JavaScriptCore',
      ],
      'configurations': {
        'Production': {
          'INSTALL_PATH': '$(BUILT_PRODUCTS_DIR)',
        },
      },
      'sources': [
        '<@(javascriptcore_files)',
        '<@(javascriptcore_publicheader_files)',
        '<@(javascriptcore_privateheader_files)',
        '<@(javascriptcore_derived_source_files)',
        '$(SDKROOT)/System/Library/Frameworks/CoreFoundation.framework',
        '$(SDKROOT)/System/Library/Frameworks/Foundation.framework',
        '/usr/lib/libicucore.dylib',
        '/usr/lib/libobjc.dylib',
      ],
      'mac_framework_headers': [
        '<@(javascriptcore_publicheader_files)',
      ],
      'mac_framework_private_headers': [
        '<@(javascriptcore_privateheader_files)',
      ],
      'xcode_config_file': '<(project_dir)/Configurations/JavaScriptCore.xcconfig',
      'sources/': [
        ['exclude', 'API/tests/'],
        ['exclude', 'ForwardingHeaders/'],
        ['exclude', '(?<!unicode)/icu/'],
        ['exclude', 'os-win32/'],
        ['exclude', 'qt/'],
        ['exclude', 'wtf/(android|brew|efl|gtk|haiku|qt|wince|wx)/'],
        ['exclude', 'wtf/unicode/brew/'],
        ['exclude', 'wtf/unicode/glib/'],
        ['exclude', 'wtf/unicode/qt4/'],
        ['exclude', 'wtf/unicode/wince/'],
        ['exclude', 'wtf/url/'],
        ['exclude', '/(gtk|glib|gobject)/.*\\.(cpp|h)$'],
        ['exclude', '(Default|Gtk|Chromium|None|Qt|Win|Wx|Symbian)\\.(cpp|mm|h)$'],
        ['exclude', 'GCActivityCallback\.cpp$'],
        ['exclude', 'BSTR[^/]*$'],
      ],
      'postbuilds': [
        {
          'postbuild_name': 'Check For Global Initializers',
          'action': [
            'sh', '<(project_dir)/gyp/run-if-exists.sh', '<(DEPTH)/../Tools/Scripts/check-for-global-initializers'
          ],
        },
        {
          'postbuild_name': 'Check For Exit Time Destructors',
          'action': [
            'sh', '<(project_dir)/gyp/run-if-exists.sh', '<(DEPTH)/../Tools/Scripts/check-for-exit-time-destructors'
          ],
        },
        {
          'postbuild_name': 'Check For Weak VTables and Externals',
          'action': [
            'sh', '<(project_dir)/gyp/run-if-exists.sh', '<(DEPTH)/../Tools/Scripts/check-for-weak-vtables-and-externals'
          ],
        },
      ],
      'conditions': [
        ['OS=="mac"', {
          'mac_bundle': 1,
          'xcode_settings': {
            # FIXME: Remove these overrides once JavaScriptCore.xcconfig is
            # used only by this project.
            'GCC_PREFIX_HEADER': '<(project_dir)/JavaScriptCorePrefix.h',
            'INFOPLIST_FILE': '<(project_dir)/Info.plist',
          },
        }],
      ],
    },
    {
      'target_name': 'Derived Sources',
      'type': 'none',
      'actions': [
        {
          'action_name': 'Generate Derived Sources',
          'inputs': [],
          'outputs': [
            '<@(javascriptcore_derived_source_files)',
          ],
          'action': [
            'sh', 'generate-derived-sources.sh'
          ],
        },
        {
          'action_name': 'Generate DTrace Header',
          'inputs': [],
           'outputs': [],
           'action': [
             'sh', '<(project_dir)/gyp/generate-dtrace-header.sh', '<(project_dir)'
            ]
        }
      ],
    },
    {
      'target_name': 'Update Version',
      'type': 'none',
      'actions': [{
        'action_name': 'Update Info.plist with version information',
        'inputs': [],
         'outputs': [],
         'action': [
           'sh', '<(project_dir)/gyp/update-info-plist.sh', '<(project_dir)/Info.plist'
          ]
      }],
    },
    {
      'target_name': 'minidom',
      'type': 'executable',
      'dependencies': [
        'JavaScriptCore',
      ],
      # FIXME: We should use a header map instead of listing these explicitly.
      'include_dirs': [
        '<@(javascriptcore_include_dirs)',
      ],
      'sources': [
        '<@(minidom_files)',
        '$(SDKROOT)/System/Library/Frameworks/CoreFoundation.framework',
      ],
      'copies': [{
        'destination': '<(PRODUCT_DIR)',
        'files': [
          '<@(minidom_support_files)',
        ],
      }],
    },
    {
      'target_name': 'testapi',
      'type': 'executable',
      'dependencies': [
        'JavaScriptCore',
      ],
      # FIXME: We should use a header map instead of listing these explicitly.
      'include_dirs': [
        '<@(javascriptcore_include_dirs)',
      ],
      'sources': [
        '<@(testapi_files)',
        '$(SDKROOT)/System/Library/Frameworks/CoreFoundation.framework',
      ],
      'copies': [{
        'destination': '<(PRODUCT_DIR)',
        'files': [
          '<@(testapi_support_files)',
        ],
      }],
    },
    {
      'target_name': 'jsc',
      'type': 'executable',
      'dependencies': [
        'JavaScriptCore',
      ],
      # FIXME: We should use a header map instead of listing these explicitly.
      'include_dirs': [
        '<@(javascriptcore_include_dirs)',
      ],
      'configurations': {
        'Production': {
          'xcode_settings': {
            'INSTALL_PATH': '$(JAVASCRIPTCORE_FRAMEWORKS_DIR)/JavaScriptCore.framework/Resources',
          },
        },
      },
      'sources': [
        '<@(jsc_files)',
        '$(SDKROOT)/System/Library/Frameworks/CoreFoundation.framework',
        '/usr/lib/libedit.dylib',
      ],
    },
  ], # targets
}
