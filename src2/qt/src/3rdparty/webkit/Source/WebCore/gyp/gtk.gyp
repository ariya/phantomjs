{ # Just a stub file to allow Source/gyp/configure to run successfully for testing.
  'includes': [
    '../../gyp/common.gypi',
    '../WebCore.gypi',
  ],
  'targets': [
    {
      'target_name': 'WebCore',
      'type': 'static_library',
      'sources': [
        '<@(webcore_files)',
        '<@(webcore_privateheader_files)',
        '<@(webcore_derived_source_files)',
      ],
    },
   ], # targets
}
