SOURCES = odbc.cpp
CONFIG -= qt dylib
mac:CONFIG -= app_bundle
win32-g++*:LIBS += -lodbc32
else:LIBS += -lodbc
