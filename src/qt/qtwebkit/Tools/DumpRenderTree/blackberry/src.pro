lessThan(QT_VERSION, 4.5) {
    error("Qt 4.5 or greater is required.")
}

TEMPLATE = app
TARGET = ImageDiff

SOURCES += ../qt/ImageDiff.cpp
DESTDIR = .

unix:CONFIG += debug_and_release
mac:CONFIG -= app_bundle
win32: CONFIG += console

QT = core gui

