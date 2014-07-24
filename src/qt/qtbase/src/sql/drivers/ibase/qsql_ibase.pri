HEADERS += $$PWD/qsql_ibase_p.h
SOURCES += $$PWD/qsql_ibase.cpp

unix {
    !contains(LIBS, .*gds.*):!contains(LIBS, .*libfb.*):LIBS += -lgds
} else {
    !contains(LIBS, .*gds.*):!contains(LIBS, .*fbclient.*) {
        LIBS += -lgds32_ms
    }
}
