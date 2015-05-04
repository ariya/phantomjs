SOURCES = icu.cpp
CONFIG += console
CONFIG -= qt dylib

win32 {
    CONFIG(static, static|shared) {
        CONFIG(debug, debug|release) {
            LIBS += -lsicuind -lsicuucd -lsicudtd
        } else {
            LIBS += -lsicuin -lsicuuc -lsicudt
        }
    } else {
        LIBS += -licuin -licuuc -licudt
    }
} else {
    LIBS += -licui18n -licuuc -licudata
}
