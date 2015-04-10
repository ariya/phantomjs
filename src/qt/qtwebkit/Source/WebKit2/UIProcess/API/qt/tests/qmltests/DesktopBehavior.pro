include(../tests.pri)
SOURCES += tst_qmltests.cpp
TARGET = tst_qmltests_DesktopBehavior
OBJECTS_DIR = .obj_DesktopBehavior

QT += webkit-private
CONFIG += testcase

QT += qmltest

DEFINES += DISABLE_FLICKABLE_VIEWPORT=1
# Test the QML files under DesktopBehavior in the source repository.
DEFINES += QUICK_TEST_SOURCE_DIR=\"\\\"$$PWD$${QMAKE_DIR_SEP}DesktopBehavior\\\"\"
DEFINES += IMPORT_DIR=\"\\\"$${ROOT_BUILD_DIR}$${QMAKE_DIR_SEP}imports\\\"\"

OTHER_FILES += \
    DesktopBehavior/* \
    common/*
