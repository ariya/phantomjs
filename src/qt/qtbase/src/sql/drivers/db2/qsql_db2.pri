HEADERS += $$PWD/qsql_db2_p.h
SOURCES += $$PWD/qsql_db2.cpp

unix {
    !contains(LIBS, .*db2.*):LIBS += -ldb2
} else {
    !contains(LIBS, .*db2.*):LIBS += -ldb2cli
}
