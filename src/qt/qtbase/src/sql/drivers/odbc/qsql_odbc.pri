HEADERS += $$PWD/qsql_odbc_p.h
SOURCES += $$PWD/qsql_odbc.cpp

unix {
    DEFINES += UNICODE
    !contains(LIBS, .*odbc.*) {
        macx:LIBS += -liodbc
        else:LIBS += $$QT_LFLAGS_ODBC
    }
} else {
    LIBS *= -lodbc32
}
