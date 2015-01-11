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
    LIBS += -ladvapi32
} else:!contains(QT_CONFIG,no-pkg-config):packagesExist("icu-i18n") {
    PKGCONFIG += icu-i18n
} else {
    LIBS += -licui18n -licuuc -licudata
}

load(qt_build_config)
