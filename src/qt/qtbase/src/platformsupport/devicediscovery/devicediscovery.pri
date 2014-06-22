HEADERS += $$PWD/qdevicediscovery_p.h

linux {
    contains(QT_CONFIG, libudev) {
        SOURCES += $$PWD/qdevicediscovery_udev.cpp
        INCLUDEPATH += $$QMAKE_INCDIR_LIBUDEV
        LIBS_PRIVATE += $$QMAKE_LIBS_LIBUDEV
        # Use our own define. QT_NO_LIBUDEV may not be set on non-Linux systems.
        DEFINES += QDEVICEDISCOVERY_UDEV
    } else: contains(QT_CONFIG, evdev) {
        SOURCES += $$PWD/qdevicediscovery_static.cpp
    } else {
        SOURCES += $$PWD/qdevicediscovery_dummy.cpp
    }
} else {
    SOURCES += $$PWD/qdevicediscovery_dummy.cpp
}
