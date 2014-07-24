SOURCES = avx.cpp
CONFIG -= qt dylib release debug_and_release
CONFIG += debug console
isEmpty(QMAKE_CFLAGS_AVX):error("This compiler does not support AVX")
else:QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_AVX
