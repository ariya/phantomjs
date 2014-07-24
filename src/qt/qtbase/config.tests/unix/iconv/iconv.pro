SOURCES = iconv.cpp
CONFIG -= qt dylib
mac|mingw|qnx:LIBS += -liconv
