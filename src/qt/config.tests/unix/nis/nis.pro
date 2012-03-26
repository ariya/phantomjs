SOURCES = nis.cpp
CONFIG -= qt dylib
mac: CONFIG -= app_bundle
solaris-*:LIBS += -lnsl
else:LIBS += $$QMAKE_LIBS_NIS
