contains(QT_CONFIG, accessibility) {
    INCLUDEPATH += $$PWD

    HEADERS += \
        $$PWD/qaccessiblebridgeutils_p.h

    SOURCES += \
        $$PWD/qaccessiblebridgeutils.cpp
}
