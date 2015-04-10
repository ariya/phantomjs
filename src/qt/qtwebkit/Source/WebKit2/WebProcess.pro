# -------------------------------------------------------------------
# Project file for the WebKit2 web process binary
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

TEMPLATE = app

TARGET = QtWebProcess
DESTDIR = $${ROOT_BUILD_DIR}/bin

SOURCES += qt/MainQt.cpp

INCLUDEPATH = \
    $$PWD/Shared/linux/SandboxProcess/ \
    $$INCLUDEPATH

QT += network webkit
macx: QT += xml

qtHaveModule(widgets): QT += widgets webkitwidgets

build?(webkit1): DEFINES += HAVE_WEBKIT1

INSTALLS += target

isEmpty(INSTALL_BINS) {
    target.path = $$[QT_INSTALL_LIBEXECS]
} else {
    target.path = $$INSTALL_BINS
}


