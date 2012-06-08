{
  'conditions': [
    # Handle build types.
    ['buildtype=="Dev"', {
      'includes': ['internal/release_impl.gypi'],
    }],
    ['buildtype=="Official"', {
      'includes': ['internal/release_impl_official.gypi'],
    }],
    # TODO(bradnelson): may also need:
    #     checksenabled
    #     coverage
    #     dom_stats
    #     pgo_instrument
    #     pgo_optimize
    #     purify
  ],
}

