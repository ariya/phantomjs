TEMPLATE        = app
CONFIG         += console
CONFIG         -= app_bundle
build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

include(../bootstrap/bootstrap.pri)

DESTDIR         = ../../../bin

SOURCES         = main.cpp

target.path=$$[QT_INSTALL_BINS]
INSTALLS += target
