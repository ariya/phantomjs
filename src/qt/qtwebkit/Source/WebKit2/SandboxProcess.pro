# -------------------------------------------------------------------
# Project file for the WebKit2 sandbox process binary
#
# See 'Tools/qmake/README' for an overview of the build system
# -------------------------------------------------------------------

TEMPLATE = app

TARGET = SUIDSandboxHelper
DESTDIR = $${ROOT_BUILD_DIR}/bin

CONFIG += console
CONFIG -= qt

SOURCES += Shared/linux/SandboxProcess/SandboxEnvironmentLinux.cpp
HEADERS += Shared/linux/SandboxProcess/SandboxEnvironmentLinux.h

INSTALLS += target
LIBS += -lcap -ldl

isEmpty(INSTALL_BINS) {
    target.path = $$[QT_INSTALL_BINS]
} else {
    target.path = $$INSTALL_BINS
}
