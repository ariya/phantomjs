{
  'includes': [
    '../../gyp/common.gypi',
    '../WebCore.gypi',
  ],
  'configurations': {
    'Production': {
      'xcode_config_file': '<(project_dir)/Configurations/Base.xcconfig',
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
  'targets': [
    {
      'target_name': 'WebCore',
      'type': 'shared_library',
      'dependencies': [
        'Derived Sources',
        'Update Version',
        # FIXME: Add 'Copy Generated Headers',
        # FIXME: Add 'Copy Forwarding and ICU Headers',
        # FIXME: Add 'Copy Inspector Resources',
      ],
      'include_dirs': [
        '<(project_dir)',
        '<(project_dir)/icu',
        '<(project_dir)/ForwardingHeaders',
        '<(PRODUCT_DIR)/usr/local/include',
        '/usr/include/libxml2',
        '<(PRODUCT_DIR)/DerivedSources',
        '<(PRODUCT_DIR)/DerivedSources/WebCore',
      ],
      'sources': [
        '<@(webcore_files)',
        '<@(webcore_privateheader_files)',
        '<@(webcore_derived_source_files)',
        '$(SDKROOT)/System/Library/Frameworks/Accelerate.framework',
        '$(SDKROOT)/System/Library/Frameworks/ApplicationServices.framework',
        '$(SDKROOT)/System/Library/Frameworks/AudioToolbox.framework',
        '$(SDKROOT)/System/Library/Frameworks/AudioUnit.framework',
        '$(SDKROOT)/System/Library/Frameworks/Carbon.framework',
        '$(SDKROOT)/System/Library/Frameworks/Cocoa.framework',
        '$(SDKROOT)/System/Library/Frameworks/CoreAudio.framework',
        '$(SDKROOT)/System/Library/Frameworks/IOKit.framework',
        '$(SDKROOT)/System/Library/Frameworks/OpenGL.framework',
        '$(SDKROOT)/System/Library/Frameworks/QuartzCore.framework',
        '$(SDKROOT)/System/Library/Frameworks/SystemConfiguration.framework',
        '<(PRODUCT_DIR)/JavaScriptCore.framework',
        'libicucore.dylib',
        'libobjc.dylib',
        'libxml2.dylib',
        'libz.dylib',
      ],
      'sources/': [
        ['exclude', 'bindings/[^/]+/'],
        ['include', 'bindings/generic/'],
        ['include', 'bindings/js/'],
        ['include', 'bindings/objc/'],

        # FIXME: This could should move to Source/ThirdParty.
        ['exclude', 'thirdparty/'],

        # FIXME: Figure out how to store these patterns in a variable.
        ['exclude', '(android|brew|cairo|chromium|curl|efl|freetype|fftw|gstreamer|gtk|haiku|linux|mkl|openvg|pango|qt|skia|soup|symbian|texmap|iphone|v8|win|wince|wx)/'],
        ['exclude', '(Android|Brew|Cairo|Curl|Chromium|Efl|Haiku|Gtk|Linux|OpenType|Qt|Safari|Soup|Symbian|V8|Win|WinCE|Wx)\\.(cpp|mm?)$'],
        ['exclude', 'Chromium[^/]*\\.(cpp|mm?)$'],

        ['exclude', 'platform/image-decoders/'],
        ['exclude', 'platform/image-encoders/'],

        ['exclude', 'bridge/testbindings\\.cpp$'], # Remove from GYPI?
        ['exclude', 'bridge/testbindings\\.mm$'], # Remove from GYPI?
        ['exclude', 'bridge/testqtbindings\\.cpp$'], # Remove from GYPI?
        ['exclude', 'platform/KillRingNone\\.cpp$'],
        ['exclude', 'platform/graphics/cg/FontPlatformData\\.h$'],
        ['exclude', 'platform/graphics/gpu/LoopBlinnPathProcessor\\.(cpp|h)$'],
        ['exclude', 'platform/graphics/gpu/LoopBlinnLocalTriangulator\\.(cpp|h)$'],
        ['exclude', 'platform/graphics/gpu/LoopBlinnPathCache\\.(cpp|h)$'],
        ['exclude', 'platform/graphics/gpu/LoopBlinnShader\\.(cpp|h)$'],
        ['exclude', 'platform/graphics/gpu/LoopBlinnSolidFillShader\\.(cpp|h)$'],
        # FIXME: Consider excluding GL as a suffix.
        ['exclude', 'platform/graphics/ImageSource\\.cpp$'],
        ['exclude', 'platform/graphics/opengl/TextureMapperGL\\.cpp$'],
        ['exclude', 'platform/graphics/opentype/OpenTypeUtilities\\.(cpp|h)$'],
        ['exclude', 'platform/posix/SharedBufferPOSIX\\.cpp$'],
        ['exclude', 'platform/text/Hyphenation\\.cpp$'],
        ['exclude', 'platform/text/LocalizedNumberICU\\.cpp$'],
        ['exclude', 'platform/text/LocalizedNumberNone\\.cpp$'],
        ['exclude', 'platform/text/TextEncodingDetectorNone\\.cpp$'],
        ['exclude', 'plugins/PluginDataNone\\.cpp$'],
        ['exclude', 'plugins/PluginDatabase\\.cpp$'],
        ['exclude', 'plugins/PluginPackageNone\\.cpp$'],
        ['exclude', 'plugins/PluginPackage\\.cpp$'],
        ['exclude', 'plugins/PluginStream\\.cpp$'],
        ['exclude', 'plugins/PluginView\\.cpp$'],
        ['exclude', 'plugins/mac/PluginPackageMac\\.cpp$'],
        ['exclude', 'plugins/mac/PluginViewMac\\.mm$'],
        ['exclude', 'plugins/npapi\\.cpp$'],

        # FIXME: Check whether we need to build these derived source files.
        ['exclude', 'JSAbstractView\\.(cpp|h)'],
        ['exclude', 'JSElementTimeControl\\.(cpp|h)'],
        ['exclude', 'JSMathMLElementWrapperFactory\\.(cpp|h)'],
        ['exclude', 'JSSVGExternalResourcesRequired\\.(cpp|h)'],
        ['exclude', 'JSSVGFilterPrimitiveStandardAttributes\\.(cpp|h)'],
        ['exclude', 'JSSVGFitToViewBox\\.(cpp|h)'],
        ['exclude', 'JSSVGLangSpace\\.(cpp|h)'],
        ['exclude', 'JSSVGLocatable\\.(cpp|h)'],
        ['exclude', 'JSSVGStylable\\.(cpp|h)'],
        ['exclude', 'JSSVGTests\\.(cpp|h)'],
        ['exclude', 'JSSVGTransformable\\.(cpp|h)'],
        ['exclude', 'JSSVGURIReference\\.(cpp|h)'],
        ['exclude', 'JSSVGZoomAndPan\\.(cpp|h)'],
        ['exclude', 'tokenizer\\.cpp'],

        ['exclude', 'AllInOne\\.cpp$'],

        ['exclude', 'rendering/svg/[^/]+\\.cpp'],
        ['include', 'rendering/svg/RenderSVGAllInOne\\.cpp$'],
      ],
      'mac_framework_private_headers': [
        '<@(webcore_privateheader_files)',
      ],
      'mac_bundle_resources': [
        '<@(webcore_resource_files)',
      ],
      'xcode_config_file': '<(project_dir)/Configurations/WebCore.xcconfig',
      # FIXME: A number of these actions aren't supposed to run if "${ACTION}" = "installhdrs"
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
        {
          'postbuild_name': 'Copy Forwarding and ICU Headers',
          'action': [
            'sh', '<(project_dir)/gyp/copy-forwarding-and-icu-headers.sh'
          ],
        },
        {
          'postbuild_name': 'Copy Inspector Resources',
          'action': [
            'sh', '<(project_dir)/gyp/copy-inspector-resources.sh'
          ],
        },
        {
          'postbuild_name': 'Streamline Inspector Source',
          'action': [
            'sh', '<(project_dir)/gyp/streamline-inspector-source.sh'
          ],
        },
        {
          'postbuild_name': 'Check For Inappropriate Files in Framework',
          'action': [
            'sh', '<(project_dir)/gyp/run-if-exists.sh', '<(DEPTH)/../Tools/Scripts/check-for-inappropriate-files-in-framework'
          ],
        },
      ],
      'conditions': [
        ['OS=="mac"', {
          'mac_bundle': 1,
          'xcode_settings': {
            # FIXME: Remove these overrides once WebCore.xcconfig is
            # used only by this project.
            'GCC_PREFIX_HEADER': '<(project_dir)/WebCorePrefix.h',
            'INFOPLIST_FILE': '<(project_dir)/Info.plist',
            'ALWAYS_SEARCH_USER_PATHS': 'NO',
          },
        }],
      ],
    },
    {
      'target_name': 'Derived Sources',
      'type': 'none',
      'dependencies': [
        'WebCoreExportFileGenerator',
      ],
      'xcode_config_file': '<(project_dir)/Configurations/WebCore.xcconfig',
      'actions': [{
        'action_name': 'Generate Derived Sources',
        'inputs': [],
        'outputs': [],
        'action': [
          'sh', 'generate-derived-sources.sh',
        ],
      }],
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
      'target_name': 'WebCoreExportFileGenerator Generator',
      'type': 'none',
      'actions': [{
        'action_name': 'Generate Export File Generator',
        'inputs': [
          '<(project_dir)/WebCore.exp.in',
        ],
        'outputs': [
          '<@(export_file_generator_files)',
        ],
        'action': [
          'sh', '<(project_dir)/gyp/generate-webcore-export-file-generator.sh',
        ],
      }],
    },
    {
      'target_name': 'WebCoreExportFileGenerator',
      'type': 'executable',
      'dependencies': [
        'WebCoreExportFileGenerator Generator',
      ],
      'include_dirs': [
        '<(project_dir)/ForwardingHeaders',
      ],
      'xcode_config_file': '<(project_dir)/Configurations/WebCore.xcconfig',
      'configurations': {
        'Production': {
            'EXPORTED_SYMBOLS_FILE': '',
            'GCC_OPTIMIZATION_LEVEL': '0',
            'INSTALL_PATH': '/usr/local/bin',
            'OTHER_LDFLAGS': '',
            'SKIP_INSTALL': 'YES',
        },
        'Release': {
          'xcode_settings': {
            'EXPORTED_SYMBOLS_FILE': '',
            'GCC_OPTIMIZATION_LEVEL': '0',
            'INSTALL_PATH': '/usr/local/bin',
            'OTHER_LDFLAGS': '',
            'SKIP_INSTALL': 'YES',
          },
        },
        'Debug': {
          'xcode_settings': {
            'EXPORTED_SYMBOLS_FILE': '',
            'GCC_OPTIMIZATION_LEVEL': '0',
            'INSTALL_PATH': '/usr/local/bin',
            'OTHER_LDFLAGS': '',
            'SKIP_INSTALL': 'YES',
          },
        },
      },
      'sources': [
        '<@(export_file_generator_files)',
      ],
      'conditions': [
        ['OS=="mac"', {
          'xcode_settings': {
            # FIXME: Remove these overrides once WebCore.xcconfig is
            # used only by this project.
            'GCC_PREFIX_HEADER': '<(project_dir)/WebCorePrefix.h',
            'INFOPLIST_FILE': '<(project_dir)/Info.plist',
          },
        }],
      ],
    },
  ], # targets
}
