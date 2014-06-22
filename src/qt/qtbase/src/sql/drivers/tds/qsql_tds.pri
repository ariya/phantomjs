HEADERS += $$PWD/qsql_tds_p.h
SOURCES += $$PWD/qsql_tds.cpp

unix|mingw: {
    LIBS += $$QT_LFLAGS_TDS
    !contains(LIBS, .*sybdb.*):LIBS += -lsybdb
    QMAKE_CXXFLAGS *= $$QT_CFLAGS_TDS
} else {
    LIBS *= -lNTWDBLIB
}
