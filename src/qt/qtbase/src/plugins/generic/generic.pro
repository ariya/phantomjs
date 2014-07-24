TEMPLATE = subdirs

*-maemo*: SUBDIRS += meego

contains(QT_CONFIG, evdev) {
    SUBDIRS += evdevmouse evdevtouch evdevkeyboard evdevtablet
}

contains(QT_CONFIG, tslib) {
    SUBDIRS += tslib
}
