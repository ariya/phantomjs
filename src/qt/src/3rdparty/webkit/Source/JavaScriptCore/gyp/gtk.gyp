{ # Just a stub file to allow Source/gyp/configure to run successfully for testing.
  'includes': [
    '../../gyp/common.gypi',
    '../JavaScriptCore.gypi',
  ],
  'targets': [
    {
      'target_name': 'JavaScriptCore',
      'type': 'static_library',
      'sources': [
        '<@(javascriptcore_files)',
        '<@(javascriptcore_publicheader_files)',
        '<@(javascriptcore_privateheader_files)',
        '<@(javascriptcore_derived_source_files)',
      ],
    },
  ], # targets
}
