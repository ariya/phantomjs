HEADERS += \
    $$PWD/qevdevtouch_p.h

SOURCES += \
    $$PWD/qevdevtouch.cpp

contains(QT_CONFIG, libudev) {
    LIBS_PRIVATE += $$QMAKE_LIBS_LIBUDEV
}

contains(QT_CONFIG, mtdev) {
    CONFIG += link_pkgconfig
    PKGCONFIG_PRIVATE += mtdev
}

