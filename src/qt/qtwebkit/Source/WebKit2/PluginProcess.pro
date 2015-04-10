# -------------------------------------------------------------------
# Project file for the WebKit2 plugin process binary
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

TEMPLATE = app

QT += webkit

TARGET = QtWebPluginProcess
DESTDIR = $${ROOT_BUILD_DIR}/bin

SOURCES += qt/PluginMainQt.cpp

INCLUDEPATH = \
    $$PWD/../JavaScriptCore \
    $$PWD/../WTF \
    $$INCLUDEPATH

INSTALLS += target

isEmpty(INSTALL_BINS) {
    target.path = $$[QT_INSTALL_LIBEXECS]
} else {
    target.path = $$INSTALL_BINS
}
