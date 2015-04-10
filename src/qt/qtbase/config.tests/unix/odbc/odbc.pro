SOURCES = odbc.cpp
CONFIG -= qt dylib
mingw:LIBS += -lodbc32
else:LIBS += -lodbc
