{
  'includes': ['release_defaults.gypi'],
  'defines': ['OFFICIAL_BUILD'],
  'msvs_settings': {
    'VCCLCompilerTool': {
      'Optimization': '3',
      'InlineFunctionExpansion': '2',
      'EnableIntrinsicFunctions': 'true',
      'FavorSizeOrSpeed': '2',
      'OmitFramePointers': 'true',
      'EnableFiberSafeOptimizations': 'true',
      'WholeProgramOptimization': 'true',
    },
    'VCLibrarianTool': {
      'AdditionalOptions': ['/ltcg', '/expectedoutputsize:120000000'],
    },
    'VCLinkerTool': {
      'LinkTimeCodeGeneration': '1',
    },
  },
}
