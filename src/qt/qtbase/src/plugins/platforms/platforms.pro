TEMPLATE = subdirs

android:!android-no-sdk: SUBDIRS += android

SUBDIRS += minimal

!win32:!winrt: SUBDIRS += phantom

!win32|contains(QT_CONFIG, freetype):SUBDIRS += offscreen

contains(QT_CONFIG, xcb) {
    SUBDIRS += xcb
}

mac {
    ios: SUBDIRS += ios
    else: SUBDIRS += cocoa
}

win32:!winrt: SUBDIRS += windows
winrt: SUBDIRS += winrt

contains(QT_CONFIG, direct2d) {
    SUBDIRS += direct2d
}

qnx {
    SUBDIRS += qnx
}

contains(QT_CONFIG, eglfs) {
    SUBDIRS += eglfs
    SUBDIRS += minimalegl
}

contains(QT_CONFIG, directfb) {
    SUBDIRS += directfb
}

contains(QT_CONFIG, kms) {
    SUBDIRS += kms
}

contains(QT_CONFIG, linuxfb): SUBDIRS += linuxfb
