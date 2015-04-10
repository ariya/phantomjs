HEADERS += \
    $$PWD/qevdevtablet_p.h

SOURCES += \
    $$PWD/qevdevtablet.cpp

contains(QT_CONFIG, libudev) {
    LIBS_PRIVATE += $$QMAKE_LIBS_LIBUDEV
}
