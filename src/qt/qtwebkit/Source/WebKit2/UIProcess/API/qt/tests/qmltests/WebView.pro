include(../tests.pri)
SOURCES += tst_qmltests.cpp
TARGET = tst_qmltests_WebView
OBJECTS_DIR = .obj_WebView

QT += webkit-private
CONFIG += testcase

QT += qmltest

# Test the QML files under WebView in the source repository.
DEFINES += QUICK_TEST_SOURCE_DIR=\"\\\"$$PWD$${QMAKE_DIR_SEP}WebView\\\"\"
DEFINES += IMPORT_DIR=\"\\\"$${ROOT_BUILD_DIR}$${QMAKE_DIR_SEP}imports\\\"\"

OTHER_FILES += \
    WebView/* \
    common/*

RESOURCES = resources.qrc
