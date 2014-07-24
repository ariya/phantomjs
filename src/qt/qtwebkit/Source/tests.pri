# -------------------------------------------------------------------
# Project file for QtWebKit API unit-tests
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

TEMPLATE = subdirs
CONFIG += ordered

WEBKIT_TESTS_DIR = $$PWD/WebKit/qt/tests

SUBDIRS += \
    $$WEBKIT_TESTS_DIR/cmake \
    $$WEBKIT_TESTS_DIR/qobjectbridge \
    $$WEBKIT_TESTS_DIR/qwebframe \
    $$WEBKIT_TESTS_DIR/qwebpage \
    $$WEBKIT_TESTS_DIR/qwebelement \
    $$WEBKIT_TESTS_DIR/qgraphicswebview \
    $$WEBKIT_TESTS_DIR/qwebhistoryinterface \
    $$WEBKIT_TESTS_DIR/qwebview \
    $$WEBKIT_TESTS_DIR/qwebhistory \
    $$WEBKIT_TESTS_DIR/qwebinspector \
    $$WEBKIT_TESTS_DIR/qwebsecurityorigin \
    $$WEBKIT_TESTS_DIR/hybridPixmap

linux-* {
    # This test bypasses the library and links the tested code's object itself.
    # This stresses the build system in some corners so we only run it on linux.
    SUBDIRS += $$WEBKIT_TESTS_DIR/MIMESniffing
}

# Benchmarks
SUBDIRS += \
    $$WEBKIT_TESTS_DIR/benchmarks/painting \
    $$WEBKIT_TESTS_DIR/benchmarks/loading

# WebGL performance tests are disabled temporarily.
# https://bugs.webkit.org/show_bug.cgi?id=80503
#
#enable?(WEBGL) {
#    SUBDIRS += $$WEBKIT_TESTS_DIR/benchmarks/webgl
#}

build?(webkit2): {
    WEBKIT2_TESTS_DIR = $$PWD/WebKit2/UIProcess/API/qt/tests

    have?(QTQUICK):SUBDIRS += \
        $$WEBKIT2_TESTS_DIR/inspectorserver \
        $$WEBKIT2_TESTS_DIR/publicapi \
        $$WEBKIT2_TESTS_DIR/qquickwebview \
        $$WEBKIT2_TESTS_DIR/qmltests

    SUBDIRS += \
        $$WEBKIT2_TESTS_DIR/qrawwebview
}
