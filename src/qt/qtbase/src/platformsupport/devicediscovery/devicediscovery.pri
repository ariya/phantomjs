HEADERS += $$PWD/qdevicediscovery_p.h

linux {
    contains(QT_CONFIG, libudev) {
        SOURCES += $$PWD/qdevicediscovery_udev.cpp
        HEADERS += $$PWD/qdevicediscovery_udev_p.h
        INCLUDEPATH += $$QMAKE_INCDIR_LIBUDEV
        LIBS_PRIVATE += $$QMAKE_LIBS_LIBUDEV
    } else: contains(QT_CONFIG, evdev) {
        SOURCES += $$PWD/qdevicediscovery_static.cpp
        HEADERS += $$PWD/qdevicediscovery_static_p.h
    } else {
        SOURCES += $$PWD/qdevicediscovery_dummy.cpp
        HEADERS += $$PWD/qdevicediscovery_dummy_p.h
    }
} else {
    SOURCES += $$PWD/qdevicediscovery_dummy.cpp
}
