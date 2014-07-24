SOURCES = sse3.cpp
CONFIG -= qt dylib release debug_and_release
CONFIG += debug console
isEmpty(QMAKE_CFLAGS_SSE3):error("This compiler does not support SSE3")
else:QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_SSE3
