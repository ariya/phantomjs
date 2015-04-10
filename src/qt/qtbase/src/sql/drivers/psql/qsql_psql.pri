HEADERS += $$PWD/qsql_psql_p.h
SOURCES += $$PWD/qsql_psql.cpp

unix|mingw {
    LIBS += $$QT_LFLAGS_PSQL
    !contains(LIBS, .*pq.*):LIBS += -lpq
    QMAKE_CXXFLAGS *= $$QT_CFLAGS_PSQL
} else {
    !contains(LIBS, .*pq.*):LIBS += -llibpq -lws2_32 -ladvapi32
}
