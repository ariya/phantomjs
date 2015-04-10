HEADERS += \
    $$PWD/qevdevkeyboard_defaultmap_p.h \
    $$PWD/qevdevkeyboardhandler_p.h \
    $$PWD/qevdevkeyboardmanager_p.h

SOURCES += \
    $$PWD/qevdevkeyboardhandler.cpp \
    $$PWD/qevdevkeyboardmanager.cpp

contains(QT_CONFIG, libudev) {
    LIBS_PRIVATE += $$QMAKE_LIBS_LIBUDEV
}
