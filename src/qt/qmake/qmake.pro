#This is a project file for building qmake, of course it presents a problem -
# it is very hard to make qmake build this, when qmake is the thing it builds,
#once you are boot strapped though, the qmake.pro will offer better coverage of a
#platform than either of the generic makefiles

CONFIG += console bootstrap
CONFIG -= qt shared app_bundle uic
DEFINES += QT_BUILD_QMAKE QT_BOOTSTRAPPED
DESTDIR = ../bin/

OBJECTS_DIR = .
MOC_DIR = .

#guts
VPATH += $$QT_SOURCE_TREE/src/corelib/global \
         $$QT_SOURCE_TREE/src/corelib/tools \
         $$QT_SOURCE_TREE/src/corelib/kernel \
         $$QT_SOURCE_TREE/src/corelib/codecs \
         $$QT_SOURCE_TREE/src/corelib/plugin \
         $$QT_SOURCE_TREE/src/corelib/xml \
         $$QT_SOURCE_TREE/src/corelib/io
INCLUDEPATH += . \
           generators \
           generators/unix \
           generators/win32 \
           generators/mac \
           generators/symbian \
           generators/integrity \
           $$QT_SOURCE_TREE/include \
           $$QT_SOURCE_TREE/include/QtCore \
           $$QT_SOURCE_TREE/qmake

VPATH += $$QT_SOURCE_TREE/tools/shared
INCLUDEPATH += $$QT_SOURCE_TREE/tools/shared

include(qmake.pri)

