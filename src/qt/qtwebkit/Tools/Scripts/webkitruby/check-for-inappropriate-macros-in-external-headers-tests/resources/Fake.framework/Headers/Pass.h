// A macro word in a #error should not matter, that is just a coincidence.
#error PLATFORM

// There are references to a OS2, but that is not the OS() macro.
#if defined(__OS2__) || defined(OS2)
#endif
