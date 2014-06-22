SOURCES = nis.cpp
CONFIG -= qt dylib
solaris-*:LIBS += -lnsl
else:LIBS += $$QMAKE_LIBS_NIS
