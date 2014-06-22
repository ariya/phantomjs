SOURCES = sse2.cpp
CONFIG -= qt dylib release debug_and_release
CONFIG += debug console
isEmpty(QMAKE_CFLAGS_SSE2):error("This compiler does not support SSE2")
else:QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_SSE2
