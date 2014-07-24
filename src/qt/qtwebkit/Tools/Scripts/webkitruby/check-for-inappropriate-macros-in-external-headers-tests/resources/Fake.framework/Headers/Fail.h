// Common macros that we want to catch.
#if PLATFORM(MAC)
#endif
#if CPU(X86)
#endif
#if OS(DARWIN)
#endif
#if COMPILER(CLANG)
#endif
#if ENABLE(FEATURE)
#endif
#if HAVE(FEATURE)
#endif
#if USE(FEATURE)
#endif
#if COMPILER_SUPPORTS(FEATURE)
#endif
#if COMPILER_QUIRK(FEATURE)
#endif

// Indented.
#if 1
  #if PLATFORM(X)
  #endif
#endif

// Conditionals, we don't evalute. We just check for the existence of the macro.
#if defined(ignored) && PLATFORM(X)
#endif
