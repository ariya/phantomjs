SOURCES = icu.cpp
CONFIG -= qt dylib app_bundle
unix:LIBS += -licuuc -licui18n
win32:LIBS += -licuin
