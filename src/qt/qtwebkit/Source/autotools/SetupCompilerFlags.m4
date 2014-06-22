# Use C99 as the language standard for C code.
CFLAGS="$CFLAGS -std=c99"
# Use the C++11 standard. Do not warn about C++11 incompatibilities.
CXXFLAGS="$CXXFLAGS -std=c++11 -Wno-c++11-compat"

# Clang requires suppression of unused arguments warnings.
if test "$CC" = "clang"; then
    CFLAGS="$CFLAGS -Qunused-arguments"
fi

# libstdc++ is at the moment the only option as the C++ standard library when compiling with Clang.
# -Wno-c++11-extensions, currently only usable with Clang, suppresses warnings of C++11 extensions in use.
# Suppress unused arguments warnings for C++ files as well.
if test "$CXX" = "clang++"; then
    CXXFLAGS="$CXXFLAGS -stdlib=libstdc++ -Wno-c++11-extensions -Qunused-arguments"
fi

if test "$host_cpu" = "sh4"; then
    CXXFLAGS="$CXXFLAGS -mieee -w"
    CFLAGS="$CFLAGS -mieee -w"
fi

# Add '-g' flag to gcc to build with debug symbols.
if test "$enable_debug_symbols" = "min"; then
    CXXFLAGS="$CXXFLAGS -g1"
    CFLAGS="$CFLAGS -g1"
elif test "$enable_debug_symbols" != "no"; then
    CXXFLAGS="$CXXFLAGS -g"
    CFLAGS="$CFLAGS -g"
fi

# Add the appropriate 'O' level for optimized builds.
if test "$enable_optimizations" = "yes"; then
    CXXFLAGS="$CXXFLAGS -O2"
    CFLAGS="$CFLAGS -O2"
else
    CXXFLAGS="$CXXFLAGS -O0"
    CFLAGS="$CFLAGS -O0"
fi
