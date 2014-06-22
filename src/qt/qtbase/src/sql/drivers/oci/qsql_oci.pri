HEADERS += $$PWD/qsql_oci_p.h
SOURCES += $$PWD/qsql_oci.cpp

unix {
    !contains(LIBS, .*clnts.*):LIBS += -lclntsh
} else {
    LIBS *= -loci
}
mac:QMAKE_LFLAGS += -Wl,-flat_namespace,-U,_environ
