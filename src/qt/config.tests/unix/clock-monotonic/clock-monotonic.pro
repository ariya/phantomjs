SOURCES = clock-monotonic.cpp
CONFIG -= qt dylib
mac:CONFIG -= app_bundle
include(../clock-gettime/clock-gettime.pri)
